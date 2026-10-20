# Estudo do `iscle/qemu-mhi2` — blueprint para o nosso sandbox MOI3 (11/10/2026)

Análise a pedido do Lipe ("estuda o iscle/qemu-mhi2 a fundo"). Clone read‑only em scratchpad, nada executado. Repo: https://github.com/iscle/qemu-mhi2. É um **fork completo do QEMU** (214 MB, 11600 arquivos) — o que interessa é o que o iscle **acrescentou** (série de patches linear só‑MHI2). Estudo feito com 3 agentes em paralelo (máquina, device/bridges, fluxo de run). **Conclusão de cabeçalho: não serve direto pro Jetta (é Tegra3/QNX, o MOI3 é Qualcomm/AGL‑Linux/aarch64), mas é o BLUEPRINT exato do que o nosso [mib3-sandbox](NOTAS-RE-DINAMICA.md) teria que construir — e resolve, do jeito certo, o problema de "GL falso" que a gente penou.**

## O que ele é

Emulação de **sistema completo**: modela o SoC **NVIDIA Tegra 3 (`tegra30`)** do zero e uma máquina **`mhi2-harman`** (head unit Harman/e.solutions MIB2 High, VW/Audi/Porsche), boota o **firmware de produção** (QNX + HMI Java original) e mostra a tela numa janela. Binário `qemu-system-arm` (ARM 32‑bit). Diferença‑chave pro nosso sandbox: nós tentamos **rodar os binários de userspace direto** num container (LD_PRELOAD + stubs) e batemos em incompatibilidade de ABI/glibc; ele **emula o SoC inteiro e boota o kernel+userspace nativos**, o que elimina o problema de ABI — ao custo de ter que modelar o SoC.

## Arquitetura (o padrão reaproveitável)

Três peças, todas conectadas por **chardev (pipe/socket)** — **não há passthrough de GPU, nem áudio de host, nem USB passthrough**:
1. **Máquina + SoC** (`hw/arm/mhi2_harman.c` 472 linhas, `hw/arm/tegra30.c`): 4× Cortex‑A9 "G" + 1 A9 "LP" + 1 AVP (boot), GIC A9MPCore @`0x50040000`, DRAM 2 GiB @`0x80000000`, NOR CFI **64 MiB** @`0x48000000` (`-bios`), IRAM/BIT 256 KiB @`0x40000000` (`iram=`). Boot **sem `-kernel`**: sempre NOR+IRAM (QuickBoot) → PC inicial `0x480e0000` (AVP) → stage2 autenticado (CMAC AES chave‑zero) → QNX → HMI. **BootROM/fuses/secure‑boot NÃO são emulados** — o QEMU simplesmente não impõe a raiz de confiança (por isso é ferramenta de análise, não de deploy).
2. **Device models dos barramentos do carro** (o mapa do que modelar): **RCC** (CPU companheiro "Radio/Car‑Control" como endpoint PCIe `hw/misc/tegra30_pcie.c` — NIC Ethernet + ATA/DVD + **janela MOST**); **IOC** (MCU Renesas V850 como slave I2C2, empurra frames de status "ignição ligada" a cada 100 ms, CRC‑8 poly `0xa6`); **PMIC** (TPS65911/MAX20024, banco de registradores stub); **mv8787** WiFi/BT SDIO; **fixtures USB** (hub USB2532, HFC media‑connector, Pico‑as‑Ethernet ASIX AX88772, modem LTE Cinterion, **Android Auto AOA**).
3. **Bridges de host via 3 chardevs**: `glbridge` (GLES), `mostvideo` (cluster), `rcceth` (RCC/entrada).

## As duas peças de OURO pro MOI3

### (A) Forwarding de GLES — `glbridge` (o jeito CERTO do "GL falso")
O guest tem as libs **`libGLESv2.so`/`libEGL.so`/`libnvvsenc.so` substituídas por shims** (compilados `arm-none-eabi-gcc -mcpu=cortex-a9`) que **serializam as chamadas GLES** e as empurram por: uma janela MMIO doorbell em `0x5f000000` (magic "GLBR") + **staging RAM de 2 MiB em `0x5e000000`** → `glbridge` chardev → processo host **`mhi2-glhost`** (Mesa, **EGL surfaceless pbuffer**, `EGL_PLATFORM_SURFACELESS_MESA`) que renderiza num `.ppm` → janela **PySide6/Qt** faz polling a cada 50 ms. **Isto é exatamente o que o nosso sandbox precisava:** em vez do nosso hack de `LD_PRELOAD` apontando Mesa como "Adreno", a forma limpa é **shimar as libs GLES do guest e encaminhar pra um `glhost` com Mesa surfaceless**. (No Mac cai em **llvmpipe = GLES por software**; decode H.264 por VideoToolbox não implica 3D acelerado.)

### (B) Tap do vídeo de cluster — `mostvideo`
O guest escreve blocos ISO no MOST (BAR3); **só o canal 3** (vídeo do cluster, "unencrypted ISO transmit") é exportado **length‑framed** pelo `mostvideo` chardev → `most_sink.py` decodifica H.264 → `cluster.ppm` → janela de cluster do viewer. **É o análogo MIB2 do nosso `ff14::2:4`** — mostra como **tapear e visualizar** o stream do cluster na emulação. No MOI3 isso fica **mais fácil**: o cluster é **IP/Ethernet (multicast `ff14::2:4`)**, não MOST óptico — então o "tap" é só capturar o multicast numa rede virtual do QEMU.

## Como o firmware entra (sem dump de hardware)
`tools/mhi2/build_firmware.py` (perfis `audi-a3`/`vw`/`porsche`): recebe o **`.7z` de update do fabricante** (fornecido pelo usuário; não vem no repo), confere o train em `metainfo2.txt`, e instala os membros `MMX2/.../70/default/*.img` (qb_primary, mifs‑stage1/2, efs‑sys, efs‑pers, app) num **bootstrap** (`nor.bin`/`iram.bin`/`emmc-template.raw` baixado de release). **Recalcula os CMAC (AES chave‑zero), não patcheia instrução nenhuma** — o QuickBoot ainda confere o CMAC. Depois `prepare_ui.py` troca as libs GL pelos shims e abre uma shell diagnóstica. Invocação real (de `debug_shell_local.py`): `qemu-system-arm -M mhi2-harman,iram=$IRAM -bios $NOR -drive if=sd,...,snapshot=on,file=$EMMC -display none -qmp pipe:... -serial stdio` + `-chardev pipe glbridge/rcceth` + `-chardev file mostvideo` + `-nic user,model=mv8787-sdio`.

## Síntese: o que PORTA pro MOI3 e o que tem que ser reconstruído

| Peça | Porta pro MOI3? |
|---|---|
| **Padrão de arquitetura** (QEMU full‑system + device models dos barramentos + bridges de host por chardev + shim das libs GL) | ✅ **Blueprint direto.** É o que o nosso sandbox deveria ser. |
| **Forwarding de GLES (glbridge → Mesa surfaceless)** | ✅ **Reaproveitável conceitualmente.** Resolve o "GL falso". No MOI3 as libs são **Adreno** (`libGLESv2_adreno` — a gente já viu no sandbox); shimar essas. |
| **Tap do cluster (`mostvideo`)** | ✅ **Mais fácil no MOI3:** capturar o multicast IP `ff14::2:4` numa rede virtual, em vez de MOST óptico. |
| **Carregar firmware do pacote de update sem dump** | ✅ **Método reaproveitável:** extrair o SWUP do MOI3 e montar rootfs; **secure boot não precisa ser satisfeito na emulação** (QEMU não impõe). |
| **Checklist de device models dos barramentos** (RCC, IOC, PMIC, CAN wake, USB fixtures) | ✅ **Como mapa de categorias;** os componentes do MOI3 diferem (Qualcomm), mas as categorias são as mesmas. |
| **SoC `tegra30` (NVIDIA, Cortex‑A9, ARM 32‑bit)** | ❌ **Não serve.** O Jetta é **Qualcomm Snapdragon (aarch64)** — precisaria de um **machine model Qualcomm** no QEMU (o grande esforço). |
| **OS QNX + boot QuickBoot (NOR+IRAM/BIT, CMAC)** | ❌ **Não serve.** MOI3 é **AGL/Linux** (boot ABL/XBL Qualcomm). **Porém:** Linux em QEMU é mainline e **mais fácil** de bootar que QNX — vantagem nossa. |
| **Tegra DC / host1x GR2D (framebuffer por software)** | ❌ Específico do Tegra; o MOI3 usa pipeline Adreno/Weston. |

## Limites honestos
- Mesmo o qemu‑mhi2 renderiza em **llvmpipe (software)** no Mac; boot a frio leva **minutos**; precisa do **`.7z` oficial** do usuário; **secure boot não é emulado** (é análise/estudo, não caminho de deploy).
- Portar pro MOI3 = **modelar um SoC Qualcomm em QEMU** (esforço grande; QEMU tem algum suporte MSM/Qualcomm, mas provavelmente não o SoC exato do MIB3) **ou** revisitar a abordagem "rodar userspace direto" com um userspace aarch64/Linux casado ao MOI3 + o forwarding de GL estilo glbridge. De qualquer forma, a **metodologia (bridges, firmware loading, GL forward, tap de cluster) é um blueprint completo e validado**.

## Próximo passo possível (se formos retomar o sandbox)
Estudar `tools/mhi2/glforward/` (o shim GLES + `glhost.c`) em detalhe pra copiar o padrão de forwarding; e decidir a estratégia de SoC pro MOI3 (machine Qualcomm em QEMU × userspace‑direto com GL‑forward). Cruzar também os símbolos de `dio_manager`/`displaymanager` da RE do MOI3 com o fluxo `vc_render`/`displaymanager` dos repos MIB2 (joeyQuery/kamgurgul).

---

## Mergulho em `tools/mhi2/glforward/` (11/10/2026) — a receita de GL‑forward, e como aplicar no nosso sandbox

Lido o diretório inteiro (~4000 linhas; núcleo em `guest/gl_shim.c` 827 e `host/glhost.c` 847, mais `guest/bridge_transport.h` 166 e `guest/NVSS-EMULATION.txt`). **É a solução limpa pro problema de "GL falso" que a gente penou no [NOTAS-RE-DINAMICA](NOTAS-RE-DINAMICA.md).**

### O princípio: FORWARD, não FAKE
O nosso sandbox linkava libs GL falsas (symlink do Mesa como `libEGL_adreno.so`) e stub‑ava — e quebrava, porque uma lib falsa não **renderiza** e a ABI/comportamento não batia. O glforward faz o certo: a lib do guest **não tenta SER uma implementação GL**; ela **encaminha cada chamada** para um processo Mesa de verdade, fora do guest. Assim o guest roda contra uma ABI correta (os símbolos que o shim exporta) e o render real é feito por Mesa out‑of‑process.

### Protocolo (de `bridge_transport.h` + `glhost.c`)
- Cada chamada GLES/EGL vira um **record**: `[u32 opcode][u32 length][payload]`, little‑endian. Header real tem 5 u32: `[magic 119][versão 4][getpid()][op][total]` (o PID separa namespaces por processo).
- **Record montado em thread‑local storage** antes de tomar o transporte (o HMI renderiza de várias threads). Chamadas que **retornam valor** (getters: ops 100–109, 120/121, 123/124, 126/127, 131/132, 134–138) setam flag `reply` e **bloqueiam** esperando a resposta pelo mesmo canal.
- **Transporte no fork:** janela **MMIO QEMU em `0x5f000000`** (doorbell: offset 4 = TX chunk, offset 28 = bulk TX, offset 16 = lock, magic lido em offset 0 = `0x474c4252` "GLBR") + **aperture bulk de 2 MiB em `0x5e000000`** (1 MiB TX + 1 MiB RX). Mas o `glhost.c` lê o stream de **`stdin`** e responde em **`stdout`** — ou seja, o transporte é **agnóstico**: pode ser a MMIO (precisa do device model QEMU) **ou um pipe/socket comum**.

### O host `glhost.c` (o que a gente reusaria quase direto)
- `main` → `serve(STDIN_FILENO)`: loop lê header de 8 bytes + payload, `valid_record()` (tabela de tamanhos por op — rejeita record corrompido), `select_client(pid)` e `switch(op)`.
- **EGL surfaceless (Mesa):** `ensure_window()` faz `eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA)` + `eglChooseConfig(EGL_PBUFFER_BIT, EGL_OPENGL_ES2_BIT, RGBA8/D24/S8)` + `eglCreatePbufferSurface` + `eglCreateContext(GLES2)` + `eglMakeCurrent`. Default **800×480**.
- **Por‑cliente:** cada PID do guest ganha EGLContext + pbuffer + mapa de objetos próprios. **Tradução de IDs:** `gmap[guest_id] = real_gl_id` (função `M()`), porque os nomes de objeto GL do guest ≠ do host.
- **Dispatch (a tabela de opcodes):** 1=eglInitialize(w,h), 2=glViewport, 3=glClearColor, 4=glClear, **5=swap**, 6=glFinish, 7–14=shader/program, 15–18=buffers, 19–21/128/138=vertex attribs (ponteiros do guest são offsets num VBO), 22–30/80–87=uniforms, **31/32/129=draws**, 33–43=estado, 50–58=texturas, 60–79=FBO/renderbuffer/stencil, 100–109=getters (resposta em stdout), 120/131=glReadPixels, 110=shader binário.
- **Saída de frame:** op 5 (swap) → `dump_frame()`: `glReadPixels(RGBA)` → escreve **PPM** (flip vertical, RGBA→RGB) em `/tmp/glhost_frame.ppm` (atômico via `.tmp`+`rename`). Um viewer (PySide6) faz polling.
- **Caminho de cluster/vídeo:** ops 134–137 (video_open/decode/attributes/close) + 127 (`encoder_submit` = EGLImage do cluster → H.264) + 126 (`encoder_poll`). Output 0 → **cluster** (408×448) em `MHI2_CLUSTER_VIDEO_FRAME`; output 1 → centro 800×480. Encoder/decoder = FFmpeg (VideoToolbox no Mac / VA‑API no Linux / software).
- **Específico do Tegra (provavelmente DESNECESSÁRIO no MOI3):** op 110 = shader **binário NVIDIA** → traduzido a GLSL por **hash FNV‑1a** em `known_shaders.h` (Mesa não roda binário Tegra); e `begin/end_binary_blend` reconstrói o source‑over que o shader NVIDIA embute. **No MOI3 (Adreno), a HMI Kanzi/Java submete GLSL‑fonte** (op 8, forwarded unchanged) → essa camada de tradução não é precisa.

> **Atualização de 11/10/2026.** Este plano foi executado no Marco 3 da sessão de modelagem ([modelagem-soc.md](modelagem-soc.md), código em [glforward-moi3/](glforward-moi3/README.md)), com três diferenças: o protocolo foi escrito do zero para OpenGL ES 3.2 (o do fork é ES 2 e a ponte da HMI liga o ES 3.2 inteiro), o `glhost` também é novo, e o item 5 se resolveu ligando o shim contra a glibc 2.23 do próprio firmware. A HMI renderiza a tela "Welcome".

### Plano de adaptação pro nosso sandbox MOI3 (concreto)
1. **Shim das libs Adreno do guest.** Escrever `libEGL_adreno.so` + `libGLESv2_adreno.so` que **exportam exatamente os símbolos GLES2/EGL** que a HMI do MOI3 chama e, em cada um, serializam `[op][len][payload]` igual ao `gl_shim.c` (thread‑local record; getters bloqueiam por reply). É o substituto correto do nosso symlink‑pra‑Mesa que meio‑funcionava.
2. **Transporte = pipe/socket Unix, NÃO QEMU.** Como o nosso sandbox roda a HMI direto num container (não QEMU full‑system), **dispensa o device model MMIO**: o shim escreve num fd (socket) e o `glhost` lê dele. O `glhost.c` já lê de `stdin` — adaptação mínima.
3. **Reusar o `glhost.c` quase como está** — é replay GLES2 genérico sobre Mesa surfaceless. Remover/ignorar a tradução de shader binário NVIDIA (op 110 → no MOI3 usa‑se op 8 GLSL‑fonte) e o `binary_blend`. Manter `gmap`, per‑cliente, `dump_frame`→PPM, e o viewer.
4. **Cluster no MOI3 = mais fácil:** em vez do encoder/`mostvideo`, **capturar o multicast IP `ff14::2:4`** na rede do container e decodificar o H.264 pra um `cluster.ppm` (o stream já é H.264, como no `most_sink`).
5. **ABI/glibc:** o problema que nos travou no sandbox era rodar binários do firmware sob glibc incompatível. O shim **não resolve isso sozinho** — ainda é preciso um userspace aarch64/AGL casado (ou full‑system QEMU com SoC Qualcomm). O glforward resolve só a parte **gráfica** (que era onde a gente mais apanhou). Decisão de arquitetura (container‑direto × QEMU‑Qualcomm) continua em aberto, mas a peça de GL agora tem receita pronta.

**Resumo:** o `glforward` é um **replay GLES2 genérico (shim no guest → pipe → Mesa surfaceless no host → PPM → viewer)**, pouco acoplado ao Tegra. Dá pra portar a parte gráfica pro nosso sandbox com esforço moderado (shims Adreno + reusar o `glhost`), sobre transporte de socket (sem QEMU). O que o glforward NÃO resolve: o SoC/ABI do MOI3 — essa decisão fica pro próximo passo.

## SoC identificado (11/10/2026) — e por que isso é BOM pra viabilidade

O SoC do MIB3 LG do Jetta é o **Snapdragon 820A (MSM8996 / APQ8096AU, automotivo), GPU Adreno 530, driver kgsl** — já estava nas nossas notas ([NOTAS-RE-DINAMICA.md](NOTAS-RE-DINAMICA.md) linha ~528) e **corroborado agora nos binários do firmware** (traces da HMI `hmi*_out`/`hs_err.log` com "820", "8996", "Adreno", "kgsl"; `traces/adreno_exports.txt`). Adreno 530 ⇒ MSM8996, consistente.

> **Correção de 11/10/2026 (Marco 0 da sessão de modelagem, [modelagem-soc.md](modelagem-soc.md)).** Dois pontos do parágrafo acima mudaram depois de ler o `boot.img` e as libs: (1) **o driver não é kgsl** — o kernel tem `CONFIG_QCOM_KGSL` desligado e a `libgsl.so` usa as chamadas do DRM `msm` (`DRM_IOCTL_MSM_GEM_SUBMIT` etc.) em `/dev/dri/card0`; o "kgsl" que aparece nos traces é o nome do nó no device tree; (2) o device tree diz **"MSM 8996pro AUTO ADP"** (identificador 316, `CONFIG_MACH_APQ8096_MIB3`), a variante Pro automotiva, e não o APQ8096AU. A família (MSM8996, Adreno 530) e a referência ao DB820c continuam valendo.

**Por que de‑risca a feasibility (ponto‑chave pra decidir A×B):**
- **MSM8996 é dos SoCs Qualcomm mais bem documentados e com suporte MAINLINE** — a placa de referência **DragonBoard 820c (APQ8096/DB820c)** tem device tree upstream no kernel Linux (`msm8996.dtsi` + `apq8096-db820c.dts`). Não é um SoC automotivo obscuro. No **Caminho A (QEMU full‑system)**, o QEMU não traz uma máquina MSM8996 pronta, mas o kernel mainline + o DT do DB820c dão um baita ponto de partida (há precedente de bootar Linux no MSM8996). Continua sendo esforço, mas o SoC **não é caixa‑preta**.
- **A Adreno 530 tem driver ABERTO: o `freedreno` do Mesa** (suporta Adreno 5xx nativamente). Ou seja: no **Caminho B (container‑direto)** num host aarch64 real, o `libGLESv2` da HMI poderia até rodar **de verdade** via freedreno — e o nosso shim de GL‑forward é o plano B que dispensa a GPU. (As notas já citavam freedreno e o padrão "Scutum"/winlator — bridge glibc↔GL por socket Unix, a mesma ideia do glforward.)

**Conclusão pra decisão A×B:** o SoC estar pinado como **820A/MSM8996 (mainline, DB820c) + Adreno 530 (freedreno)** é a melhor notícia possível de feasibility — os dois pontos que mais assustavam (modelar o SoC; a GPU) têm **referência pública**. A sessão dedicada pode começar já sabendo o alvo, não descobrindo‑o.

## Busca no GitHub por solução pronta de modelagem do SoC (11/10/2026)

Antes de abrir a sessão dedicada, varri GitHub/web atrás de uma **máquina QEMU pronta pro MSM8996/APQ8096** e de blocos reaproveitáveis. Resultado honesto:

**Não existe máquina QEMU pronta pro MSM8996/APQ8096 (Snapdragon 820A).** O QEMU upstream não traz board/machine model do SoC do MIB3. Modelá‑lo do zero seria o esforço escala‑`tegra30` (o que o `iscle` fez à mão pro Tegra). Não há atalho de "clonar um repo e rodar".

**Mas há blocos de construção fortes — a sessão ADAPTA mainline, não modela do zero:**
- **Kernel mainline do MSM8996 + device trees** — pacote `linux-postmarketos-qcom-msm8996` do postmarketOS (grupo GitLab `msm8996-mainline`, mantido por Yassine Oudjana/Tooniis; builds recentes até kernel 6.19). É um kernel perto do mainline com patches MSM8996 + os DTs dos aparelhos reais. **Melhor ativo:** um kernel que boota em hardware MSM8996. Fonte: [pkgs.postmarketos.org — linux-postmarketos-qcom-msm8996](https://pkgs.postmarketos.org/package/master/postmarketos/aarch64/linux-postmarketos-qcom-msm8996).
- **DragonBoard 820c (APQ8096) upstream** — DT `apq8096` no kernel (primeiro proposto em nov/2015; série de Konrad Dybcio adicionou MSM8996v3.0 + PMIC PMI8996). É a placa de referência pública do SoC. Fontes: [LKML — apq8096 dragonboard dts skeletons](https://lkml.iu.edu/hypermail/linux/kernel/1511.3/01974.html), [lore.kernel.org — PMI8996 DTSI](https://lore.kernel.org/all/YK2+FfLrt+8ODVHb@builder.lan/T/).
- **Caminho de GPU virtual (alternativa ao nosso shim GL‑forward):** `virtio-gpu` + protocolo **Venus** (Vulkan serializado) com **virglrenderer** no host — QEMU expõe `virtio-gpu-gl`/`virtio-gpu-rutabaga` e o guest fala virtio em vez da Adreno real. Precisa de kernel guest ≥ 5.16 e `/dev/udmabuf` (CONFIG_UDMABUF) no host. **Turnip** (driver Vulkan Adreno, base freedreno) aparece como driver **host‑side** do Venus. Fontes: [Mesa — Virtio-GPU Venus](https://docs.mesa3d.org/drivers/venus.html), [Linaro — Building QEMU and Linux with virtio-vulkan and venus](https://linaro.atlassian.net/wiki/spaces/ORKO/pages/29013573633/Building+QEMU+and+Linux+with+virtio-vulkan+and+venus+support), [Collabora — Venus on QEMU](https://www.collabora.com/news-and-blog/blog/2021/11/26/venus-on-qemu-enabling-new-virtual-vulkan-driver).

**Leitura estratégica (entra na decisão A×B):** como existe kernel mainline + DT pro MSM8996, o caminho pragmático de QEMU talvez **nem precise de um machine model SoC‑exato**. Opção de menor esforço: bootar um **kernel mainline** na máquina **`qemu-system-aarch64 -M virt`** (plataforma genérica, não SoC‑fiel) e entregar o userspace do firmware por rootfs/initramfs — **se** a HMI não depender de devices MSM8996‑específicos. A GPU sai por `virtio-gpu`+Venus (QEMU) ou pelo nosso GL‑forward. Isso rebaixa a pergunta de "modelar o SoC" para "a HMI aguenta rodar na plataforma `virt` com kernel mainline?" — a primeira coisa a testar na sessão dedicada.

## Comparação com o nosso sandbox, a pedido do Lipe (11/10/2026, fim do dia)

Com o sandbox MOI3 já desenhando e aceitando entrada, o Lipe perguntou se o simulador da MIB2 "traz mais coisas". Traz. Reli a documentação dele na cópia local (`tools/mhi2/README.md`, `RCC_INPUT.md` e `MISSING_MENU_DATA.md`); **não rodei o emulador**, então o que segue é o que os documentos dizem.

| | `qemu-mhi2` (MIB2 High) | sandbox MOI3 |
|---|---|---|
| O que roda | o firmware inteiro: QuickBoot, QNX, drivers e a HMI | só a HMI e quatro daemons do firmware, num container |
| Tela e toque | sim | sim |
| Teclas | RADIO, MEDIA, PHONE, VOICE à esquerda; NAV, TRAFFIC, CAR, MENU à direita; BACK | Menu, Rádio, Mídia, Telefone, Carro, Clima, Som, Navegação, Ajustes |
| Botões giratórios | os dois: clicar aperta, rolar gira, − e + | os dois, do mesmo jeito (feito hoje, depois desta comparação) |
| Painel de instrumentos | janela "Virtual Cockpit", com o vídeo MOST do cluster | não tem |
| Navegação, voz, áudio | mapas preparados e desenhados; motores de voz rodam no guest; caminho de áudio em parte | nada disso |
| Serviços de veículo | faltam, e limitam várias telas (está em `MISSING_MENU_DATA.md`) | faltam também |
| Partida | vários minutos, segundo o autor | cerca de 1 min, com a espera por serviços reduzida |

O que confere com o nosso trabalho: **a entrada dele usa o mesmo caminho que achei no MOI3**. O `RCC_INPUT.md` diz que a HMI recebe toque pelo serviço `DSIKeyPanel` original, sem adaptar a HMI; que os controles físicos usam o teclado 13, tirado da tabela de teclas do `configurationmanager.res`, e não o teclado genérico 1; que os gestos de encostar, tocar e soltar são 4, 1 e 3; e que os botões giratórios de seleção e volume são as teclas 16 e 17, por `updateEncoder2`. Cheguei aos mesmos números pelo firmware do MOI3 (Marco 5 do [modelagem-soc.md](modelagem-soc.md)). Eu tinha estudado só a parte gráfica deste repositório; ler este documento antes teria poupado a rodada em que testei o teclado 1.

O próprio autor delimita o resultado: "This validates an interactive firmware UI, not a complete hardware-accurate head unit". No mesmo `RCC_INPUT.md` ele avisa que rádio, áudio, navegação e funções de veículo não passam a funcionar só por haver entrada; trechos mais novos do `README.md` já falam em mapas desenhados.

O que dá para trazer de lá, em ordem de custo: as teclas do volante; o vídeo do painel de instrumentos (no MOI3 é um fluxo de rede, não MOST); e, bem mais caro, subir o sistema inteiro, que no nosso caso seria o Caminho A (QEMU `-M virt`, já provado para a HMI no Marco 1).

## Segunda olhada, a pedido do Lipe: os modelos dos serviços do carro (11/10/2026, à noite)

O Lipe mandou o endereço do repositório de novo e pediu: "Da uma olhada". Desta vez olhei o que eu ainda não tinha lido e que serve ao passo em que o nosso simulador está: os dados de veículo.

**Estado do repositório.** Consultado pela API do GitHub em 11/10/2026: o último envio é de 09/10/2026, commit `78b8cb2`, o mesmo da cópia local que já temos. Não há nada novo desde o estudo de cima. A descrição dele fala em HMI sobre QNX, RCC e IOC, pontes de gráficos e de áudio, painel por MOST e subida de Linux. O GitHub não identifica a licença.

**O que li agora**, na cópia local e sem rodar nada: `tools/mhi2/MISSING_MENU_DATA.md` (inteiro, de novo), `recover_dsi_schema.py` e `recover_vehicle_profile.py` (inteiros), `rcc_services.py` (o cabeçalho, a montagem dos valores iniciais e o tratamento de assinaturas e dos ajustes do painel de teclas; o resto só pela lista de funções), o começo do `vehicle_profile.json` e três seções do `README.md`. **Não li** `rcc_peer.py`, `rcc_persistence.py`, o visualizador (`view_ui.py`, `config_dialog.py`, `emulator_config.py`) nem os outros documentos (`HOST_PARITY`, `CONSOLIDATION`, `MOST`, `WIRELESS`, `AUDI_A3`, `DIAGNOSTIC_SHELLS`, `FILE_TRANSFER`).

### O que ele tem que nós ainda não temos

Ele já passou pelo ponto em que estamos. O `MISSING_MENU_DATA.md` conta que o menu do carro e a tela de versões ficavam vazios pela mesma razão que os nossos: os serviços de veículo não existiam. A correção dele são três peças:

| Peça | O que faz |
|---|---|
| `dsi_schema.json`, feito pelo `recover_dsi_schema.py` (42 linhas) | um esquema de **todos** os serviços DSI tirado das classes decompiladas do firmware: número de cada método, número de cada atributo e a ordem e o tipo dos campos de cada estrutura. Não foi escrito à mão |
| `rcc_services.py` | um respondedor genérico: quando a HMI assina um atributo, ele acha no esquema a mensagem de atualização daquele atributo e manda um valor padrão, marcado como válido. Por cima, uns poucos valores escolhidos para um **carro parado** |
| `vehicle_profile.json`, feito pelo `recover_vehicle_profile.py` | os valores padrão de codificação, adaptação e identificação, tirados das classes de diagnóstico da HMI, mais uma identidade declaradamente simulada (ele não inventa número de série) |

Os valores escolhidos para o carro parado, como estão no `rcc_services.py`:

| Serviço | O que ele manda |
|---|---|
| `DSICarKombi` | nas opções do computador de bordo, liga com estado 2: consumo instantâneo, autonomia, distância total, os três grupos de médias (desde a partida, longo prazo, desde o abastecimento), nível do tanque, as três configurações de menu e os três zeramentos, lista de estados do veículo, temperatura externa e velocidade digital; motor primário do tipo 1. Nas opções de revisão, tudo em estado 2 |
| `DSICarVehicleStates` | o chassi da identidade simulada |
| `DSIGeneralVehicleStates` | carro parado e freio de estacionamento puxado |
| `DSICarTimeUnitsLanguage` | idioma do menu, hora e data do computador, e as opções de relógio e de unidades todas em estado 2 |
| `DSICarEco` | opções do start-stop em estado 2 |
| `DSIAMFMTuner` | lista de faixas (FM de 87,5 a 108 MHz, AM de 531 a 1602 kHz), sintonizador presente e pronto, emissora em 88,3 MHz |
| `DSISound` | faixas de volume e de tom, volume inicial |
| `DSIKeyPanel` | responde ao pedido e à gravação de ajuste genérico devolvendo o valor guardado, e ao pedido de propriedade com um bloco vazio |

### O que isso confirma e explica do nosso lado

- **A regra por trás do relógio.** Na rodada `c3` o relógio só apareceu quando mandei as opções de exibição com estado diferente de zero ([simulador-jetta.md](simulador-jetta.md)). O documento dele dá a regra geral, lida na mesma HMI da VW: cada função nasce com estado 0, e existir é ter estado diferente de 0; quem informa são as mensagens de opções de cada serviço. Sem elas a função fica escondida. Vale para o menu do carro inteiro, não só para o relógio.
- **Um pedido nosso que fica sem resposta.** No Marco 5 anotei que o `setGenericSetting` do painel de teclas estoura o tempo depois de 10 s. Ele responde a esse pedido com a resposta de ajuste genérico. É o que o nosso `dsistub` deveria fazer; não testei.
- **Identidade separada do que é sabido.** O Marco 6 chegou sozinho à mesma divisão (o que foi lido do carro num arquivo, os palpites em outro) e à mesma família de classes de diagnóstico.

### O que passa para o MOI3 e o que não passa

| | Passa? |
|---|---|
| O método: esquema tirado do firmware, resposta padrão a toda assinatura, perfil do carro por cima | **Sim.** Conferi que as classes do nosso firmware têm a mesma forma: a camada da LG tem 62 pacotes de serviço, 66 classes que despacham cada mensagem por número para o método `update…` com o tipo da mensagem, e 139 classes de mensagem; as interfaces da VW têm as constantes de atributo (o `DSICarKombi` tem 114; no `DSICarTimeUnitsLanguage` os atributos 1, 2 e 3 são as opções, a data e a hora, os mesmos números que usei à mão) |
| A lista do que ligar para um carro parado | **Como ponto de partida.** Os nomes dos métodos são da mesma interface da VW; se os campos são os mesmos no MOI3 tem de ser conferido no esquema do nosso firmware |
| Os números (métodos, atributos, ordem dos campos) | **Não.** São do firmware K3342 da MIB2. Os nossos saem das classes do MU0883 |
| O transporte | **Não.** Lá é o protocolo da e.solutions até o processador companheiro; aqui é o barramento kipc com mensagens protobuf |
| O código | **Não.** É Python preso ao transporte dele, num repositório sem licença identificada. Reaproveitamos a ideia, não os arquivos |
| O modelo do sintonizador | **Só a ideia.** O MOI3 usa outros serviços (`DSIUnifiedTuner`, `DSIRadioData`) |

### O que proponho fazer com isso

Trocar o plano do item 2 do roteiro do simulador. Eu ia escrever o stub de veículo mensagem por mensagem, como fiz com o relógio. O caminho dele é melhor: gerar o esquema de todos os serviços a partir das classes do nosso firmware, escrever um respondedor que atende qualquer assinatura com o valor padrão, e pôr por cima um perfil do Jetta parado (relógio, opções do computador de bordo, carro parado, freio puxado). Isso deve popular de uma vez o menu do carro e os campos que hoje aparecem como "---", e serve depois para rádio, mídia e som. **É proposta: nada disso foi feito nem testado no MOI3.**

### O "Virtual Cockpit" dele (pergunta do Lipe na mesma noite)

O Lipe perguntou: "Se não me egano no deels também tem um Virtual Cookpit, certo?" Tem. Li o começo do `MOST.md` (até a linha 75) e as linhas do visualizador que tratam disso.

- No visualizador há um botão "Virtual Cockpit" que abre uma segunda janela. Ela mostra o vídeo H.264 que a central manda para o painel de instrumentos; enquanto a central não manda nada, mostra um aviso de espera.
- O próprio autor delimita: é um receptor de vídeo emulado. Não é o firmware do painel nem um painel com velocímetro e ponteiros.
- O que passa por ali é o mapa de navegação que a central desenha para o painel. Segundo o mesmo documento, o desenho do mapa ainda está incompleto, por causa de shaders da Tegra que a ponte gráfica dele não cobre. O `README.md` relata uma partida em que os dois mapas foram desenhados.
- O caminho, lá: o programa de navegação do firmware, o driver MOST original, a caixa de mensagens do processador companheiro, e do lado de fora um decodificador que grava os quadros para a janela.

**No MOI3 do Jetta** o vídeo do painel não vai por MOST: é um fluxo de rede (levantado no começo do estudo, endereço `ff14::2:4`). Para ter uma janela dessas no nosso simulador faltam três coisas, na ordem: o motor de navegação do firmware rodando (hoje a tela para em "Starting navigation…"), o pacote oficial de mapas, e a captura e decodificação desse fluxo numa segunda janela. É o item 8 do roteiro do [simulador-jetta.md](simulador-jetta.md), o maior de todos. Nada disso foi tentado.
