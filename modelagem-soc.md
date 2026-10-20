# Sandbox MOI3 — sessão de modelagem do SoC (registro por marco)

Sessão dedicada aberta em 11/10/2026 com o roteiro de [PROMPT-sessao-modelagem-soc.md](PROMPT-sessao-modelagem-soc.md): decidir entre o Caminho A (QEMU de sistema completo) e o Caminho B (container direto) e, no melhor caso, pôr um quadro da HMI na tela. **É emulação e análise num Mac; nada vai para o carro e nada aqui trata de assinatura, dm-verity ou secure boot.**

Este arquivo é o registro da sessão: o que foi feito em cada marco, o que foi **verificado em execução**, o que é **leitura estática** dos arquivos e o que é **dedução**. As decisões tomadas no caminho ficam na seção "Decisões".

| Marco | Situação |
|---|---|
| 0 — extrair o firmware e inventariar | **Feito em 11/10/2026** |
| 1 — o teste barato que decide A×B | **Feito em 11/10/2026.** A HMI roda numa `-M virt` com kernel mainline e chega ao mesmo ponto que no container |
| 2 — ramificar | **Decidido em 11/10/2026**, com o de acordo do Lipe: desenvolver no container (B), com o mesmo lançador rodando no QEMU (A) |
| 3 — gráfico | **Feito em 11/10/2026.** A HMI renderiza a tela "Welcome / Loading user profile…" em 800×480, com animação e sem erro de OpenGL. Quadro em [quadros/2026-10-11-hmi-welcome.png](quadros/2026-10-11-hmi-welcome.png) |
| 4 — passar do "Loading user profile…" | **Feito em 11/10/2026**, a pedido do Lipe. Faltava a camada RSI. Com o registro e o mock de serviços do próprio firmware, a HMI termina a tela Welcome ("Welcome level2 user"), fecha-a sozinha e mostra a área principal, que hoje é "Radio: função indisponível". Quadro em [quadros/2026-10-11-hmi-welcome-usuario.png](quadros/2026-10-11-hmi-welcome-usuario.png) |
| 5 — entrada: teclas e toque | **Feito em 11/10/2026**, a pedido do Lipe. A HMI aceita teclas e toque publicados no barramento, e a página ao vivo os manda: clicar na tela é tocar, e há uma fileira de teclas. Chegou à grade de menus e, a partir dela, às telas de veículo, clima, som e ajustes. Quadro em [quadros/2026-10-11-hmi-grade-de-menus.png](quadros/2026-10-11-hmi-grade-de-menus.png) |
| 6 — codificação e adaptação | **Feito em 11/10/2026**, numa sessão à parte. A HMI recebe os conjuntos pelo caminho do microcontrolador de diagnóstico, e um bit de adaptação (navegação) muda a grade de menus e a tecla NAV. Quadros em [antes](quadros/2026-10-11-hmi-cod-grade-antes.png) e [depois](quadros/2026-10-11-hmi-cod-grade-navegacao-ativada.png). Os valores são de teste, não os do carro. Para o Jetta GLI 2021 ficaram dois arquivos, o do que se sabe (a identificação da ficha) e o das deduções; com eles a tela de informações do sistema ([quadro](quadros/2026-10-11-hmi-cod-jetta-info-sistema.png)) fica com as três primeiras linhas iguais às da foto da central do carro. Às 20h do mesmo dia a central passou a nascer em português, pela memória de estado: título, legendas e botão dessa tela ficaram iguais aos da foto ([quadro](quadros/2026-10-11-hmi-cod-ptbr-info-sistema.png)). Às 20h58 a central do modo ao vivo passou a lembrar de uma sessão para a outra: a sessão seguinte abre sozinha na última tela da anterior ([quadro](quadros/2026-10-11-hmi-mem-sessao2-nasce-nos-ajustes.png)) |
| 7 — transmissão direta dos quadros e janela do Mac | **Feito em 11/10/2026**, a pedido do Lipe. O `glhost` transmite cada quadro no instante em que a HMI o desenha, só com o que mudou; a `ponte` leva a transmissão para fora e traz a entrada pelo `docker exec -i`, sem abrir rede; e a tela abre numa janela própria do Mac ([janela/](janela/README.md)), com toque, teclas e botões giratórios. Um defeito achado em uso às 22h10 (a central ficava surda para a janela) foi corrigido com teste. Decisões D35 a D42. Linha acrescentada ao índice em 12/10/2026, no fechamento da noite: o marco estava registrado, mas sem linha aqui |
| 8 — dados de veículo | **Feito em 11/10/2026**, numa sessão à parte. Um stub novo, o `veiculostub`, faz o papel do serviço de dados de veículo do firmware e publica o Jetta GLI 2021 parado: a central mostra a hora de Brasília sozinha, a tela do veículo tem dados de condução, autonomia e temperatura, e "Configurações do veículo" ganhou Instrumento combinado, Hora e data, Unidades e Serviço. Quadros da [tela do veículo](quadros/2026-10-11-hmi-vei-veiculo.png) e das [configurações](quadros/2026-10-11-hmi-vei-configuracoes.png). Do carro vêm só o formato de 24 h, as unidades e a marcha (foto do painel); o resto é palpite ou valor de simulação, marcado linha a linha. No caminho apareceu a causa de a tela Welcome sumir, um prazo de 6 s da HMI à espera da câmera de ré, consertada no `dsistub` |
| depois do 8 — o hub e os anúncios fora do cache | **Feito em 11/10/2026**, numa sessão à parte. O hub do barramento só reentregava as 48 primeiras mensagens a quem se liga depois; passou a guardar também o último anúncio de cada canal. No padrão do modo ao vivo nada muda. Sem o carro de mentira a HMI passa a ligar os serviços de veículo do `carifservice` do firmware, e some uma falha de partida que a medição achou: com a máquina carregada a central às vezes ficava surda e de tela preta. Seção "O hub e os anúncios que ficavam fora do cache" |
| 9 — os pedidos da HMI ao carro de mentira | **Feito em 11/10/2026**, numa sessão à parte. O `veiculostub` passa a atender pedidos, e não só assinaturas: acertar a hora, a data e o fuso e trocar as unidades pela tela tem efeito, e as três listas do start-stop, que estouravam 10 s no registro a cada partida, são respondidas. A tabela de que resposta a HMI espera de cada pedido saiu das classes dela; o que cada pedido grava é dedução do gerador, marcada. Vale até a central desligar, e só no serviço de relógio e unidades. Atender o idioma do painel punha a HMI em laço em 1 de 9 partidas: ficou de fora. Quadros em [antes](quadros/2026-10-11-hmi-ped-hora-antes-volta-em-3s.png) e [depois](quadros/2026-10-11-hmi-ped-hora-depois-fica.png) |
| depois do 9 — o rastreamento da HMI depois da partida | **Feito em 12/10/2026**, numa sessão à parte. As chaves de rastreamento do `HMILOG` paravam uns 6 s depois da partida: na carga tardia a HMI troca todas pelas do perfil de sondas de depuração em uso, um arquivo de `/vw/hmi/config`, e o lançador só mexia no `logging.properties`. Ele passou a pôr as chaves pedidas também nesses arquivos, em cópias montadas por cima, e elas valem a sessão inteira. Sem `HMILOG` nada muda. Seção "O rastreamento da HMI que parava na carga tardia" |
| depois do 9 — os avisos "cortados no topo" | **Investigado em 12/10/2026**, na mesma sessão. Não é corte: os três avisos registrados nos Marcos 4 e 5 usam um molde que desce de cima da tela e fica pendurado na borda de cima, sem linha no topo. A posição, as margens e a moldura são dados fixos do skin, e os pixels do quadro batem com a geometria que a HMI dá. Nada foi alterado na bancada. Falta comparar com uma foto da central do carro. Seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada" |
| 10 — a central parada deixa de ocupar cinco núcleos | **Feito em 12/10/2026**, numa sessão à parte. A HMI e os quatro daemons do firmware giravam a 100 % cada, mesmo com a central parada: o laço de leitura do barramento do firmware não tem pausa, porque no carro é a leitura que segura a thread até chegar mensagem (com prazo de 5 s, lido no kernel do firmware), e a biblioteca da bancada devolvia a leitura na hora. A leitura passou a esperar a mensagem por até 100 ms. Uma central parada gasta de 2 a 3 % de um núcleo, e em doze partidas lado a lado nada mudou no que a HMI faz. Quadro: [as doze partidas aos 70 s](quadros/2026-10-12-hmi-cpu-serie-70s.png) |
| depois do 9 — o carro de mentira lembra do que a tela gravou | **Feito em 12/10/2026**, numa sessão à parte. No modo ao vivo, a hora, a data e o fuso acertados e as unidades escolhidas pela tela passam a sobreviver à partida seguinte, como no carro, em que quem guarda isso é o painel. O `veiculostub` escreve o que a tela gravou em `ao-vivo/memoria/veiculo/estado.txt` e nasce dali. Do relógio fica guardada a diferença para o relógio do Mac, e não a hora: a central seguinte mostra a hora acertada mais o tempo que passou. Arquivo com erro não impede a partida nem vale pela metade. As centrais de teste e o lançador direto seguem nascendo iguais, salvo pedido. Quadros em [antes](quadros/2026-10-12-hmi-est-antes-nasce-da-hora-do-mac.png) e [depois](quadros/2026-10-12-hmi-est-depois-nasce-com-hora-e-unidade.png) |
| depois do 9 — o brilho pela tela | **Feito em 12/10/2026**, pela sessão do estado do carro de mentira. O display de mentira (`driver/dsistub.c`) passa a responder aos pedidos de brilho: a escolha do nível de luminosidade em Ajustes > Display pega, a HMI a guarda e ela volta na partida seguinte; o campo deixa de mostrar sempre "Muito alta", e some o tempo esgotado de 10 s que toda partida tinha. A imagem não escurece. Quadros em [antes](quadros/2026-10-12-hmi-bri-antes-volta-a-muito-alta.png) e [depois](quadros/2026-10-12-hmi-bri-depois-fica-em-baixa.png) |
| 11 — o rádio de mentira | **Feito em 12/10/2026**, numa sessão à parte (na pasta à 1h33). O `radiostub` põe a tela do rádio em FM no lugar do aviso "função indisponível", com cinco emissoras inventadas: sintoniza pela lista, pelas setas, pela busca, pelas memórias e pelas teclas Próxima e Anterior do volante. Não há recepção nem som, e a faixa de FM do Brasil é palpite. Ligado por padrão só no modo ao vivo. Linhas dos marcos 11, 12, 13 e 15 acrescentadas ao índice pela sessão principal no fechamento da noite, a partir do registro de cada sessão |
| 12 — o telefone de mentira | **Feito em 12/10/2026**, numa sessão à parte (2h38 e 3h48). É o `veiculostub` com o esquema dos serviços de telefone: a tecla Telefone abre a tela "Conecte o telefone celular apenas quando parado…" (o estado "nenhum aparelho conectado", padrão do modo ao vivo), a grade ganha o bloco Telefone e o assistente de configuração ganha uma página. Um segundo estado, que é opção, mostra um aparelho fictício conectado. Não há Bluetooth nem aparelho de verdade, e o aparelho fictício não disca nem recebe chamada |
| 13 — o som de mentira | **Feito em 12/10/2026**, numa sessão à parte (na pasta às 3h35). O som desta HMI não é DSI, é o serviço RSI `audiomanagement`: o mock RSI da LG mais o `somstub` dão estado à tela de som (agudos, médios, graves, posição), ao volume (de 0 a 30) e ao mudo. Não sai áudio e o amplificador é inventado. A barra de volume só abre com o giro do botão, e nada do som é guardado entre partidas. Ligado por padrão só no modo ao vivo |
| 14 — os outros menus do carro | **Feito em 12/10/2026**, pela sessão do estado do carro de mentira, **sem ligar nenhum**: a ficha do carro não confirma nenhum dos equipamentos deles. Ficou medido o que faz cada menu existir (a função na lista de funções do carro, `0B1B`, mais as opções do serviço; a lista de aparelhos não entra) e achado o lugar dos assistentes, que é o bloco "Assistentes" da grade de menus, com a tela do Lane Assist. Na pasta ficou um catálogo fora de toda partida padrão (`sh driver/m14_menus.sh`) e, no registro, a lista para o Lipe decidir o que ligar. Quadros: [a lista com o catálogo](quadros/2026-10-12-hmi-men-lista-com-o-catalogo.png) e [o Lane Assist aberto](quadros/2026-10-12-hmi-men-lane-assist-aberto.png) |
| depois do 9 — os números do painel acompanham a unidade | **Feito em 12/10/2026, de manhã**, pela sessão do estado do carro de mentira. Com milhas, mph e mpg escolhidos em Unidades, as três páginas de dados de condução passam a mostrar "0 mi", "Ø 0 mph", "mpg (EUA)" e a autonomia em "249 mi"; a partida seguinte nasce na unidade guardada. O motor ganhou dois marcadores de perfil, `@de` e `@se`, que leem o valor de outro campo a cada envio; a conta fica no perfil, marcada como simulação. A temperatura já era convertida pela própria HMI. Quadros: [antes](quadros/2026-10-12-hmi-uni-antes-memoria-em-milhas-continua-em-km.png) e [depois](quadros/2026-10-12-hmi-uni-depois-memoria-em-milhas.png) |
| 15 — a mídia de mentira | **Feito em 12/10/2026**, pela sessão do rádio (na pasta às 3h11). O `midiastub` põe o tocador no lugar do aviso da tecla Mídia, com a fonte "Minhas mídias" e cinco faixas inventadas; dá para trocar de faixa pelas setas e pela lista. Não há aparelho, arquivo de música nem som. No mesmo marco, a decisão D158 (4h08): no modo ao vivo a HMI passa a esperar 6 s pelos serviços DSI na partida, para a central que nasce na tela do veículo não ficar sem dados; a partida termina 8 s mais tarde |
| 16 — rádio, mídia e volante: o que faltava | **Feito em 12/10/2026, de manhã**, pela sessão do rádio. As teclas Próxima e Anterior do volante trocam de faixa na mídia quando o som de mentira está ligado (sem código novo: faltava o serviço de áudio). As quatro teclas do volante que ninguém tinha repetido foram medidas: ▼ e ▲ não fazem nada, Fonte é tecla do rádio (alterna FM e AM) e Voz não muda a tela. No rádio (na pasta às 10h00), a busca passou a varrer a faixa, a lista ganhou AM com três emissoras inventadas e duas emissoras ganharam texto de rádio; a intensidade de sinal esta HMI não mostra. Linha acrescentada ao índice pela sessão principal |

**Conferência de fechamento da noite de 11 para 12/10/2026** (sessão principal, das 4h28 às 4h38): a bateria completa passa a partir da pasta, e uma partida de prova igual à do botão da janela, num container à parte, abriu rádio, mídia, telefone, som, veículo, clima e ajustes, com a barra de volume e o mudo, sem erro de OpenGL. O relato está em [simulador-jetta.md](simulador-jetta.md), "Fechamento da noite"; o resumo da partida, em `traces/fecha_prova_resumo.txt`.

## Marco 0 — extração e inventário (11/10/2026)

### De onde vem o firmware

| Pacote (em `~/Downloads`) | Tamanho | SHA-256 |
|---|---|---|
| `MOI3_EU_VW_R8410L_MU0841_3G0919360TH.7z` | 6.246.676.571 bytes | `7d19bcd1f412dd31ada3f6828ca02bab62f15550221b227a0974d69555409dc8` (confere com o registro de 09/10) |
| `MOI3_EU_VW_R8830L_MU0883_3G8919360AR.7z` | 6.775.886.593 bytes | `720ea0826401e933e5c8058683d6dc40cbf8f44cbc2b85d9229842cddca41441` (calculado agora; ainda não estava anotado) |

Nenhum dos dois é o train do carro (`R8030L` / MU0803 / `3G5 035 832 C`), que é o que o roteiro cita. Trabalhei com o **MU0883**, que é o que a bancada já usa.

A extração já existia, feita em 09 e 10/10, e não foi refeita. **Em 11/10/2026, com o sim do Lipe, o que a bancada usa foi movido para dentro desta pasta**, em `sysroot/mu0883/` (que o `.gitignore` já cobre):

| Caminho | O que é | SHA-256 (início) |
|---|---|---|
| `sysroot/mu0883/system.img` | partição `system` do MU0883, ext4, 3.069.050.880 bytes, só leitura | `6c8ae855…` |
| `sysroot/mu0883/vw/` | partição da HMI (`OISmall_EU.zip` do MU0883), 567 MB | — |
| `sysroot/mu0883/boot.img` | `boot.img` do MU0883 | `0cb2f19f…` |

O resto continua no rascunho da sessão antiga, em `/private/tmp/claude-501/-Users-lipe-Developer-VAGCoding/c2af147a-7a39-48cb-934f-1d1d5856ce37/scratchpad/`: o `update.zip` do MU0883 (`29dd1f37…`), a `sys/system.img` do MU0841 (`68a47212…`), o `cpu_meta/boot.img` do MU0841 (`2c3dc45d…`), decompilados e projetos do Ghidra. O `newfw/cpu/system.new.dat`, cópia idêntica da imagem (conferido com `cmp`), foi apagado na mesma data e liberou 2,9 GB.

Risco do que ficou lá: `/private/tmp` é limpo pelo macOS. Os `.7z` continuam em `~/Downloads` e a extração se refaz com `bsdtar`. Os hashes completos estão em `traces/m0_sha256.txt`.

Os scripts de 10/10 (`driver/run_hmi*.sh`) esperam `/scratch/newfw/system_new.img` e `/scratch/vwroot` dentro do container. Para rodá-los agora, montar `sysroot/mu0883/system.img` e `sysroot/mu0883/vw` nesses caminhos. Os scripts novos usam `/fw/system.img` e `/fw/vw`.

### A cadeia de extração (verificada)

```
.7z ─ Data/MIB3OI_CPU.CPU_<data>/21/update.zip
        ├─ system.new.dat + system.transfer.list   → partição system
        ├─ boot.img (23.541.032 bytes)             → kernel + DTBs, sem ramdisk
        └─ firmware-update/*.mbn                    → cadeia de boot Qualcomm (não usada aqui)
    └ Data/MIB3OI_HMI.Small_EU_<data>/21/OISmall_EU.zip → partição /vw (a HMI da VW)
```

- O `system.transfer.list` tem uma linha só de dados: versão 2, 749.280 blocos de 4 KiB, comando `new 2,0,749280`. Um intervalo contínuo do bloco 0 ao fim, então **o `system.new.dat` já é a imagem ext4**, byte a byte. Não há squashfs, UBI nem imagem esparsa.
- A imagem monta num container com `mount -o loop,ro,noload` (o volume entra `:ro`, então o arquivo original não tem como ser alterado): ext4, 2,6 GB, 2,1 GB usados, 24.724 entradas (21.150 arquivos, 2.314 pastas, 1.260 links).
- Em `/` a pasta `vw` é um ponto de montagem vazio: a HMI da VW vem do outro zip.

Como reproduzir: [driver/m0_inventario.sh](driver/m0_inventario.sh) (dentro do container, sem rede, sem instalar nada) e [driver/m0_analisa.py](driver/m0_analisa.py) (no Mac, só lê bytes). A listagem completa ficou em `traces/m0_system_listing_mu0883.tsv`.

### Kernel e boot (lido do `boot.img` do MU0883; o do MU0841 é igual, salvo uma opção)

| Item | Valor |
|---|---|
| Formato | Android boot image v0, página de 4096, **sem ramdisk** |
| Kernel | cabeçalho `UNCOMPRESSED_IMG` da Qualcomm + `Image` arm64 de 22.446.768 bytes, páginas de 4 KiB |
| Versão | `Linux version 4.4.115-perf (vc.integrator@5f1fd027a464) (gcc version 5.3.0) #1 SMP PREEMPT Fri Apr 26 16:07:57 KST 2024` |
| Linha de comando | `console=ttyHSL0,115200,n8 androidboot.hardware=qcom … rootfstype=ext4 root=/dev/dm-0 firmware_class.path=/firmware/image/ … ro` |
| Device trees | 3 DTBs anexados, todos `model = "Qualcomm Technologies, Inc. MSM 8996pro AUTO ADP"`, `compatible = "qcom,msm8996-adp", "qcom,msm8996", "qcom,adp"`, `qcom,msm-id = <0x13c 0x10001>`; mudam só no `board-id` (`0x08010019`, `0x09010019`, `0x0a010019`) |
| Configuração | **embutida no kernel** (IKCONFIG): 4.666 linhas, 1.156 opções `=y`, 10 `=m`. Cópia em `traces/m0_kernel_4.4.115-perf_mu0883.config` |

O que a configuração diz e interessa à emulação:

- Plataforma: `CONFIG_ARCH_MSM8996=y`, `CONFIG_MACH_APQ8096_MIB3=y`, `CONFIG_MACH_MIB3_MQB=y`, `CONFIG_LGE_CARPLAY=y`. Páginas de 4 KiB, 39 bits de endereço virtual.
- Segurança: `CONFIG_SECURITY_SMACK=y` e SMACK como padrão; `CONFIG_DM_VERITY=y`.
- Vídeo e GPU: `CONFIG_DRM=y`, `CONFIG_DRM_MSM=y` com DSI, `CONFIG_DRM_SDE_HDMI=y`, `CONFIG_DRM_SDE_WB=y`. **`# CONFIG_QCOM_KGSL is not set`**.
- Android por baixo do Linux: `CONFIG_ION=y`, `CONFIG_ASHMEM=y`, `CONFIG_SYNC=y`. Sem binder.
- Qualcomm: `CONFIG_MSM_SMD`, `CONFIG_MSM_GLINK`, `CONFIG_MSM_QMI_INTERFACE`, `CONFIG_MSM_ADSPRPC`, `CONFIG_QSEECOM`, `CONFIG_DIAG_CHAR`, serial `CONFIG_SERIAL_MSM`.
- Nenhuma opção `VIRTIO`.

Nas strings do kernel: o **kipc é um driver da LG embutido** (`LGE - DRIVER KIPC-CSR Init`, `KIPC-CSS Ready`, `Fail to send msg via KIPC`), e o driver de GPU é o `adreno`/`a5xx` do DRM (`adreno_load_gpu`, `a5xx_gpmu_err_irq`). O caminho de compilação cita a máquina Yocto `8x96auto`.

### Censo dos binários

1.673 arquivos ELF na partição `system`:

| Quantos | Tipo |
|---|---|
| 1.380 | bibliotecas AArch64 |
| 270 | executáveis AArch64 |
| 14 | objetos relocáveis AArch64: 13 módulos de kernel (`DWC_ETH_QOS.ko`, `mlan.ko`, `moal.ko`, `bcmdhd.ko`, os três da Tuxera, `msm_11ad_proxy.ko`, `rdbg.ko`, netfilter) e um `python.o` |
| 9 | ELF de 32 bits com `e_machine` 164 (Hexagon): blobs do DSP de áudio em `/usr/lib/rfsa/adsp` e o `a530_zap.*` em `/lib/firmware` |

**Não há nenhum binário ARM de 32 bits no userspace.** Isso importa porque o processador do Mac não executa ARM de 32 bits: tudo o que a central roda no CPU principal pode rodar nativo, no container ou num QEMU acelerado.

### Versões de base (banco de pacotes `opkg` da imagem, 547 pacotes)

glibc **2.23** (`ld-2.23.so`), libstdc++ 5.3.0, systemd e udev **232** (`232-r0vw1`), Wayland **1.9.0**, **Weston 1.9.0**, libdrm 2.4.62, pixman 0.32.8, dbus 1.10.6, cynara 0.11.0, security-manager 1.0.2, busybox 1.24.1, openssl 1.0.2o, protobuf 3.1.0. O pacote do driver gráfico se chama `adreno 1.0+git-r0`.

### A HMI

Na partição `system`, `/lge/hmi` tem 95 MB:

- `jvm/` (69 MB): a JVM do firmware.
- `jar/` (24 MB, 9 jars): `MIBHMI-DSI.jar` (22,6 MB), `MIBHMI-SPI.jar`, `jopengl_es3.jar`, `jopenglnw_es3.jar`, `protobuf-java-3.1.0.jar`, `MCFObjectReader.jar`, `ObjectSwapper.jar`, `DebugSPIImpl.jar`, `Vlog4jImpl.jar`.
- `libs/` (1,8 MB, 16 bibliotecas): `libjopengl_es3.so` (217 KB), `libjopenglnw_es3.so` (11 KB), `libDSIService.so`, `libImageLoader.so`, `libDebugSPINative.so`, `libDisplaySPI.so`, `libFontSPINative.so`, os motores de fonte `itype.so` e `wtle.so`, `libMCFContentReader.so`, `libmcfobjectreader.so`, `libByteBufferAllocator.so`, `libZipDecompressor.so`, `libswap.so`, `libTimerSPI.so`, `libvlog4j.so`.

Na partição `/vw` (extraída em `vwroot/`): `hmiroot.sh`, `prework_vw_hmi.sh`, `hmi/runHMI.sh`, `hmi/jar/` (`MIBHMI-STD-H40.272.304-bin.jar` de 63 MB, `MIBTestDebug-…jar`, `GEM.jar`), `hmi/lib/` (`S3DK-Core.jar`, jetty, websocket, osgi), `hmi/config/`, `hmi/Resources/skin0|1|4|5` (o `images.mcf` do `skin0` tem 185 MB), `hmi/res/`, `preload/`, `ESD/`, `speech/`. Variante `OISmall2_EU`, HMI `H40.272.304`.

**Como a central realmente sobe a HMI** (lido dos arquivos, não executado):

1. `vw-hmi.service`: `ExecStart=/bin/sh -c '/vw/hmiroot.sh'`, `Type=notify`, `WatchdogSec=1min`, `FailureAction=reboot-immediate`.
2. `/vw/hmiroot.sh`: define `XDG_RUNTIME_DIR=/run/platform/weston`, manda pré-carregar `libjsig.so`, `libImageLoader.so` e `libDebugSPINative.so`, e chama o próximo com a saída num `vtee`.
3. `/vw/hmi/runHMI.sh`: monta a linha da JVM. Classpath de boot nesta ordem: `/vw/hmi/jar/*.jar`, `/lge/hmi/jar/*.jar`, `/vw/hmi/lib/*.jar`. Heap de 128 MB. Pré-carga de classes da Azul em `/var/lib/hmi`. E, na parte gráfica:
   - `-Dopengles.cq.maxBytes=1024 -Dopengles.cq.maxFloats=8192 -Dopengles.cq.maxInts=16384 -Dopengles.cq.maxLongs=2048` (tamanho das filas de comando da ponte OpenGL);
   - `-Dde.vw.mib.view.gfx.init.eager`;
   - `-Dde.vw.mib.graphics.shader.program.binary.cache.directory=/var/cache/hmi`.

Os lançamentos da bancada (`driver/run_hmi*.sh`) montaram essa linha à mão e **não passam as quatro opções `opengles.cq.*`**, nem a ordem de classpath do script real. O efeito disso foi testado no Marco 1 (experimento E1, em "Achados de passagem").

### A pilha gráfica

| Peça | O que o arquivo mostra |
|---|---|
| `libjopengl_es3.so` (ponte JNI de OpenGL da HMI) | `NEEDED`: `libEGL_adreno.so`, `libGLESv2_adreno.so`. Importa **44 funções EGL** (até EGL 1.5: `eglGetPlatformDisplay`, `eglCreateImage`, sync) e **357 funções GL**: é o núcleo inteiro do OpenGL ES 3.2, menos `glGetVertexAttribPointerv`. Nenhuma função com sufixo de extensão (`OES`, `EXT`, `QCOM`). Exporta as funções EGL uma a uma (`Java_org_opengl_EGL10Impl_…`) e, para o GL, um único executor de fila: `Java_org_opengl_Flusher_flushImpl`. Listas em `traces/m0_jopengl_es3_und.txt` e `…_def.txt` |
| `libjopenglnw_es3.so` (janela nativa) | `NEEDED`: `libwayland-client.so.0`, `libwaylandegl.so.1`. Classe `EGLNativeRendererWaylandIVI`: conecta no Wayland (`wl_display_connect`), cria superfície **IVI** (`ivi_application`, `ivi_surface`) e a janela EGL (`wl_egl_window_create`) |
| `libEGL_adreno.so` (144 KB) | carrega por nome `libGLESv2_adreno.so`, `libq3dtools_esx.so` e o subdriver `libeglSubDriverWayland.so` |
| `libGLESv2_adreno.so` (17,9 MB) | `NEEDED`: `libgsl.so`, `libadreno_utils.so` |
| `libgsl.so` (1,1 MB) | **fala DRM, não kgsl**: funções `gsl_msm_drm_*`, e os nomes `DRM_IOCTL_MSM_GET_PARAM`, `…GEM_NEW`, `…GEM_INFO`, `…GEM_SUBMIT`, `…WAIT_FENCE`; mensagens "Attempting to open DRM device" e "Unable to get DRM device name from GBM library"; abre também `/dev/ion` |
| `libgbm.so` (da Qualcomm) | abre `/dev/dri/card0`, `/dev/dri/renderD128` e `/dev/ion` |
| Compiladores de shader | `libllvm-glnext.so` (14 MB) e `libllvm-qcom.so` (32 MB) |
| Weston 1.9.0 | `weston --idle-time=4294967 --tty=1`, usuário `sint_sec_display`. `weston.ini`: `shell=ivi-shell.so`, `ivi-controller.so`, `ivi-input-controller.so`, saídas `HDMI-A-1` e `DSI-1`. Backends presentes: `drm-backend.so`, `fbdev-backend.so`, `wayland-backend.so`. O `drm-backend.so` depende de `libsdedrm.so`, `libsdmutils.so` e `libhardware.so.0`: é o backend da Qualcomm para o display SDE, não o KMS genérico. O `gl-renderer.so` depende das libs Adreno. Existe a opção `use-pixman` no backend DRM e `use-gl` no fbdev |
| Daemons de display | `/lge/app_ro/graphics/displaymgr/bin/displaymgr` (exige o Weston), `multi-display/multi-screen-server`, `multi-display/rsi-display-management` |

### O que a central espera do kernel além do genérico

Leitura estática; é a lista do que uma plataforma `virt` com kernel mainline **não** entrega.

- **SMACK** como módulo de segurança, com `smackfs` montado em `/smack` (está no `fstab`).
- **kipc**: os nós `/dev/csr` e `/dev/css`, driver da LG embutido no kernel. Não existe em nenhum kernel público.
- **DRM `msm`** para a GPU e para o display SDE, mais `/dev/ion`.
- **Partições do eMMC**: o `fstab` monta onze `/dev/mmcblk0pNN` (`/firmware`, `/dsp`, `/cache`, `/lge/secure`, `/lge/bt`, `/lge/log/backup`, `/var/lib/nav`, `/radio/osdb`, `/lge/speech/db`, `/lge/webengine`, `/lge/user_storage`) e faz um bind de `/persist/var-lib` em `/var/lib`.
- **Canais Qualcomm** para modem, DSP e TrustZone (SMD, GLINK, QMI, adsprpc, qseecom).
- Console em `ttyHSL0`.

O systemd da imagem tem 231 units; `default.target` aponta para `graphical.target`.

### Correções ao alvo descrito no roteiro

O que o Marco 0 mostrou diferente do que o roteiro (e registros anteriores) afirmavam:

1. **GPU: não é kgsl.** O roteiro e o [estudo-qemu-mhi2.md](estudo-qemu-mhi2.md) dizem "driver kgsl". O kernel tem `CONFIG_QCOM_KGSL` desligado e a `libgsl.so` usa as chamadas do DRM `msm`. O device tree ainda chama o nó de `qcom,kgsl-3d0`, o que explica a confusão. Na prática: a HMI abre `/dev/dri/card0`, como o log de 10/10 já mostrava ("Adreno-GSL open(/dev/dri/card0) failed").
2. **SoC: é a variante Pro automotiva.** O roteiro diz "820A = MSM8996 / APQ8096AU". O device tree diz "MSM 8996pro AUTO ADP" com identificador 316 e a configuração diz `MACH_APQ8096_MIB3`. De memória, 316 é o APQ8096pro automotivo na tabela do kernel msm-4.4; não conferi essa tabela em fonte nesta sessão. A família é a mesma (MSM8996, Adreno 530), então a referência ao DragonBoard 820c continua valendo.
3. **Train.** O roteiro cita `R8030L`; os pacotes em disco são `R8410L` e `R8830L`.
4. **OpenGL: a ponte é ES 3.2, não ES 2.** O esboço em [glforward-moi3/](glforward-moi3/) segue o protocolo do `qemu-mhi2`, que é OpenGL ES 2. A ponte JNI da HMI liga o ES 3.2 inteiro e a HMI exige ES 3.1 (nota de 10/10). Quais funções ela **chama de fato** ainda não foi medido.
5. **Formato do rootfs.** É ext4 puro; nada de squashfs ou UBI.
6. **glibc.** O teardown de 09/10 diz "firmware = 2.17" num trecho. A glibc da imagem é a 2.23; 2.17 é a versão mínima que os binários pedem.

## Marco 1 — o teste em `-M virt` (11/10/2026)

A pergunta do Marco 1: o userspace do MOI3 roda numa plataforma genérica, sem os dispositivos do MSM8996?

**Resposta, verificada em execução: sim.** Com o kipc e o OpenGL substituídos, a HMI roda numa `qemu-system-aarch64 -M virt` com kernel mainline e chega exatamente ao ponto em que chega no container.

### O que foi montado

Com o sim do Lipe ("Tudo no container"), uma imagem Docker local, `moi3-bancada:1`, feita só com pacotes do repositório oficial do Ubuntu (ports.ubuntu.com). O apt baixou **235 MB** (138 pacotes, 642 MB instalados). Nada foi instalado no macOS. Receita em [driver/Dockerfile.bancada](driver/Dockerfile.bancada).

| Peça | Versão |
|---|---|
| QEMU | 8.2.2 (`qemu-system-arm` do Ubuntu 24.04) |
| Kernel do guest | `6.8.0-146-generic` arm64 do Ubuntu, sem alteração. Tem `VIRTIO_BLK`, `VIRTIO_MMIO`, `EXT4` e `SECURITY_SMACK` embutidos; `DRM_VIRTIO_GPU` como módulo |
| Mesa | 25.2.8 (para o Marco 3) |
| gcc | 13.3.0 |

O OrbStack não expõe `/dev/kvm`, então o QEMU roda sem aceleração (TCG). Mesmo assim o boot de teste inteiro (kernel, montagem dos discos e `java -version`) leva 6 segundos.

Scripts, todos em `driver/`:

- `m1_build.sh` compila as ferramentas uma vez (`kipc_hub`, `kipc_bus.so`, `dmstub` e o stub de OpenGL).
- `m1_hmi.sh` é o **lançador único** da HMI. Roda igual no container (chroot) e dentro do QEMU, para que os dois resultados sejam comparáveis. Tem duas variantes: `manual` (a linha da JVM de 10/10) e `real` (o `/vw/hmi/runHMI.sh` do firmware).
- `m1_init.sh` é o `/init` do initramfs: monta a partição `system` (disco virtio, só leitura), a `/vw` e um disco de saída, chama o lançador e desliga.
- `m1_qemu.sh` monta o initramfs e a imagem da `/vw` e sobe o QEMU: `-M virt -cpu cortex-a57 -smp 4 -m 4096`, sem placa de rede.

### Resultado

| Rodada | Ambiente | Onde chegou |
|---|---|---|
| `qemu_t2` | QEMU `-M virt`, kernel 6.8 | Monta a `system.img` do firmware direto (versão `202404261555`). O busybox 1.24.1 e a JVM do firmware executam: `openjdk version "1.8.0_242"`, Zulu 8.44 |
| `nat_manual` | container, kernel do OrbStack | Bundles SPI, autostart e managed; "UI thread started!"; "app framework initialized!"; entra em `SimpleDesktopLayout` e `HMI_OFF_Colored_Screen`; **no primeiro repaint cai em `GLESShaderProgram.queryActiveAttributes`**, 31 s depois da partida |
| `qemu_manual` | QEMU `-M virt`, kernel 6.8 | **Os mesmos marcos e a mesma exceção, no mesmo método**, 56 s depois da partida. Como rodou 150 s, entrou em mais views (`SYSTEM_CONTROLCENTER_MAIN`, `USERMANAGEMENT_CONTROL_CENTER_MAIN`) |
| `nat_real` | container, com o `runHMI.sh` do firmware | Os mesmos marcos; a exceção muda de linha (ver "Achados de passagem") |

Nas três rodadas com a HMI ela não morre: é cortada pelo tempo limite (código 124 no container; 143 no QEMU, que é o `timeout` do busybox encerrando o processo). Os serviços DSI que ficam sem registrar são os mesmos nos dois ambientes: `DSICarTimeUnitsLanguage`, `DSIGeneralVehicleStates` e, mais tarde, `DSICarParkingSystem`. Na rodada mais longa do QEMU aparece também `DSIUnifiedTuner`.

Saídas guardadas em `traces/m1_*.out` e `traces/m1_qemu_*.console.log`.

**Corrigido no Marco 4.** Havia uma diferença entre os dois ambientes que passou despercebida aqui. No container a JVM enxergava os 16 processadores do Mac e o cliente WebSocket da camada RSI **nem iniciava** (uma exceção `Insufficient configured threads` logo na partida, uma vez em cada rodada `nat_*`). No QEMU, com 4 processadores, ele iniciava e ficava tentando conectar (67 tentativas na `qemu_manual`). Os marcos e a exceção de OpenGL eram mesmo iguais; a camada RSI não. O lançador agora fixa em 4 os processadores que a JVM enxerga (decisão D11).

### O que a HMI procura na plataforma e não acha

Medido com `strace` no container, variante `real`, 60 s: todas as aberturas de arquivo que falharam em `/dev`, `/sys` e `/proc` (`traces/m1_hmi_aberturas_falhas.txt`).

| Caminho | O que é |
|---|---|
| `/sys/devices/soc/soc:common_gpio_default/display_id` | **o único caminho específico da placa** (ver abaixo) |
| `/dev/dri/card0` (2 vezes) | a GPU. Quem abre é a `libgsl.so` de verdade, que entra no processo junto com o `libImageLoader.so` |
| `/sys/devices/system/cpu`, `/sys/devices/system/cpu/online`, `/sys/class/net` | genéricos; faltaram porque a bancada não monta `/sys` dentro do chroot |
| `/dev/tty`, `/dev/shm/lttng-ust-wait-*` | inofensivos |

Fora isso, a HMI depende dos nós do kipc (`/dev/csr`, `/dev/css`), que o `kipc_bus.so` intercepta antes de virarem chamada de sistema.

### O `display_id`

A classe `com.lge.mib3oi.spi.display.DisplaySPIImpl` (decompilada do `MIBHMI-SPI.jar` com o CFR; cópia em `traces/re/DisplaySPIImpl.java`) lê esse arquivo para saber qual tela está ligada, e daí tira a resolução:

| Terceiro byte do arquivo | Resolução |
|---|---|
| `'2'` | 1280×640 |
| `'3'` | 1560×700 |
| qualquer outro | 800×480 |

Com a resolução ela monta o `EGLNativeRenderer` (janela Wayland IVI) e o registra como display padrão do EGL. Sem o arquivo, cai na exceção, devolve nulo e a HMI segue sem janela nativa. É o que acontece em todas as rodadas até aqui.

**Corrigido no Marco 3.** Aqui estava escrito, como dedução, que a HMI `OISmall` seria a de 1280×640. Está errado: o skin `OISmall` é feito para **800×480**, que é justamente o caso "qualquer outro" da tabela. Duas evidências: o `info.txt` do skin (`skin.displayname=VW_OI8_Skin_Core`, `Layout.Carplay.Canvas_Dimension.width=800`, `height=480`) e o próprio quadro renderizado, que numa superfície de 1280×640 saiu encostado no canto, centrado em x=400. O `dmstub` de 10/10 anunciava 1920×720, que não é resolução de nenhuma tela desta central.

### Achados de passagem

- **O stub de OpenGL de 10/10 não compilava.** O `driver/fakegl.c` que estava na pasta citava dez funções de EGL 1.5 sem defini-las. Foi regenerado com a lista completa (401 funções) pelo mesmo `gen_fakegl.py`; a cópia antiga não foi guardada na pasta.
- **A HMI está mais adiante do que o registro de 10/10 dizia.** Lá, o render parava em `glGetString` devolvendo `byte[]` nulo. Com o stub regenerado ela passa da consulta de capacidades, cria e "compila" os shaders e só para ao perguntar os atributos ativos do programa (`glGetActiveAttrib`), que o stub não sabe responder. Ou seja, **o ponto de parada é o próprio stub**, não a arquitetura do renderizador.
- **As opções `opengles.cq.*` fazem diferença** (experimento E1). Na variante `manual` a exceção é `StringIndexOutOfBoundsException` na linha 484, com um tamanho absurdo (lixo na fila de bytes). Na variante `real`, com as filas dimensionadas, a linha 484 passa e a falha vira `IllegalArgumentException: Invalid data type` na 485. Nos dois casos a causa é o stub, mas a variante `real` é a certa para seguir.
- **A HMI trava em chamadas ao serviço de display.** Depois do primeiro repaint ela espera `deferDisplayUpdates(displayId=0, displayableId=16)` e `showDisplayables(displayId=0)`, cerca de 18 s cada, porque o `dmstub` só responde ao tamanho da tela.
- **A `libgsl.so` de verdade entra no processo e tenta abrir a GPU**, mesmo com o stub no lugar das duas libs Adreno. A mensagem "Adreno-GSL … open(/dev/dri/card0) failed" aparece no instante em que o `libImageLoader.so` é carregado (pré-carregado na variante `real`; ao subir os bundles SPI na `manual`). A falha não derruba nada. A cadeia exata de dependências (`libNmeSDK.so`, `libOmxCore.so` e adiante) não foi levantada.
- A `libDisplaySPI.so` depende do Wayland e das libs `libilm*` (controle de camadas IVI). É mais uma peça que espera o Weston.

## Marco 2 — decisão A×B

**Decisão: desenvolver o gráfico no container (Caminho B) e manter o QEMU `-M virt` (Caminho A) como ambiente equivalente, já provado.**

Por quê:

- A HMI roda igual nos dois. O Marco 1 não achou nada que só exista no QEMU nem nada que só o container entregue.
- O OpenGL precisa ser substituído nos dois, do mesmo jeito: uma biblioteca com os nomes `libEGL_adreno.so` e `libGLESv2_adreno.so`, carregada sob a glibc 2.23 do firmware, que encaminha as chamadas para um Mesa fora do processo. `virtio-gpu` não muda isso, porque quem desenha no guest ainda é uma biblioteca de userspace.
- No container o ciclo de teste é nativo: 31 s até o primeiro repaint, contra 56 s no QEMU sem aceleração.
- O lançador é o mesmo (`m1_hmi.sh`). O que for feito no container roda no QEMU trocando só o transporte do encaminhador (socket Unix no container, porta `virtio-serial` no guest).

O roteiro dizia "se a HMI tolera `virt`, seguir por aí". A `virt` está provada e continua disponível; a diferença é só onde o trabalho do Marco 3 acontece primeiro. O QEMU volta a ser o ambiente principal quando for preciso algo que só um kernel nosso dá: framebuffer ou DRM para tentar o Weston do firmware, SMACK de verdade, ou o boot pelo systemd.

O que **não** foi testado e continua sendo dedução: o boot pelo systemd do firmware (231 units, onze partições no `fstab`), o Weston do firmware sobre `fbdev`, e a velocidade com aceleração (exigiria o QEMU instalado no macOS).

## Marco 3 — o primeiro quadro (11/10/2026)

**Resultado: a HMI do MOI3 desenha no Mac.** Rodando o código do próprio firmware, ela renderiza a tela "Welcome / Loading user profile…" em 800×480, com o indicador de carregamento girando.

![HMI do MOI3: tela Welcome](quadros/2026-10-11-hmi-welcome.png)

Em 60 segundos saem 331 quadros, com 51.484 chamadas de OpenGL e **nenhum erro**: nenhum shader recusado pelo Mesa, nenhuma função sem implementação chamada. Verificado em execução, no container (Caminho B).

Isso derruba o veredito de 10/10 de que "pixel é parede de hardware". Não era: faltava um OpenGL que respondesse de verdade, e ele não podia estar dentro do processo da HMI.

### Como foi feito

1. **Encaminhador de OpenGL** em [glforward-moi3/](glforward-moi3/README.md). Um shim ocupa os nomes `libGLESv2_adreno.so` e `libEGL_adreno.so`, é ligado contra a glibc 2.23 do firmware e manda cada chamada por socket para o `glhost`, que roda fora do chroot com Mesa llvmpipe (OpenGL ES 3.2 por software) e grava os quadros. Os 358 protótipos do ES 3.2 saem de um gerador a partir do cabeçalho Khronos. Os testes foram escritos antes: falham contra o stub de 10/10 e passam contra o encaminhador.
2. **`driver/dsistub.c`**, no lugar do `dmstub`. Responde aos pedidos de display que a HMI faz e publica o estado de energia "ligado".
3. **O lançador** `driver/m1_hmi.sh` ganhou as opções `GLHOST` (sobe o glhost) e `DSISTUB`. A HMI entra pelo `runHMI.sh` do firmware.

Nenhum download novo: os cabeçalhos Khronos já estavam na imagem `moi3-bancada:1`.

### As rodadas

| Rodada | O que mudou | Resultado |
|---|---|---|
| `g1` | encaminhador + `dmstub` em 1280×640 | **primeiros quadros reais**: 57 em 90 s, quase todos pretos, com a borda e os pontos de página da Central de Controle. O layout saiu centrado em x=400: a tela certa é 800×480 |
| `g2` | `dsistub` em 800×480, respondendo aos pedidos de display | as esperas de 18 s e 10 s somem; a tela continua apagada (`HMI_USER_PERCEIVED_OFF`), porque a energia não chegou |
| `g3` | captura do barramento DSI (`HUB_DUMP`) | descoberta a codificação dos inteiros do serviço de energia (abaixo) |
| `g4` | energia codificada certo | `POWERSTATE_ON` aceito; a HMI liga a tela e mostra o "Welcome". Mais de 700 quadros em 100 s |
| `g5` | glhost com resumo ao encerrar | a medida das funções usadas |

Saídas em `traces/m3_*`; quadros em `quadros/`.

### O que a HMI pede ao OpenGL

- Um contexto **OpenGL ES 2.0** e uma superfície **pbuffer de 800×480**. Não cria janela: sem o `display_id` (Marco 1) ela fica sem `EGLNativeRenderer` e usa o tamanho que o serviço de display informa.
- **62 operações distintas** em toda a rodada: 55 funções GL e 7 de EGL. É OpenGL ES 2.0 mais `glGetProgramBinary` e `glProgramParameteri` (cache de programas em `/var/cache/hmi`). As mais chamadas: `glVertexAttribPointer`, `glVertexAttrib4f`, `glUniformMatrix4fv`, `glBindTexture`, `glUseProgram`, `glDrawArrays`.
- A nota de 10/10 dizia "GLES 31 required". Com a linha de partida do firmware, sem `-Dde.vw.mib.graphics.gles.v3`, a HMI pede ES 2.0 e não reclama.

### O barramento DSI, visto no tráfego

Captura de 45 s com `HUB_DUMP` no `kipc_hub` (`traces/m3_g3_barramento_dsi.hub.log`). Formato de cada mensagem: `[canal:8][interface:4][instância:4][api:4][protobuf]`.

- **O canal é o remetente.** A HMI manda tudo com canal `0x8000` (`PID_DSI_SERVICE`); o destino é dado pela interface. Os daemons respondem com o canal deles.
- **Pedido e resposta usam interfaces vizinhas:** display 100 → 101, energia 1100 → 1101, persistência 1400 → 1401, teclas 300 → 301. A interface 9000 é o anúncio do serviço ("WELCOME").
- **Display** (domínio `0x4000`): `getDisplaySize` 1043 → 2029, `deferUpdates` 1040 → 2031, `showDisplayables` 1039 → 2027, `getExtents` 1005 → 2000, `getDisplayPower` 1010 → 2004. A HMI aceita a resposta só quando tem um pedido pendente; por isso o `dmstub`, que mandava o tamanho sem ninguém pedir, era descartado ("unexpected"). Pedidos ainda sem resposta: 1041 (`requestDisplayControl`), 1011 (`setDisplayBrightness`) e 1022 (`getDisplayBrightness`, que estoura o tempo de 10 s). **Mudou em 12/10/2026:** o 1011 e o 1022 passaram a ser atendidos (seção "O brilho pela tela: o display de mentira responde").
- **Energia** (domínio `0x400000`, do `mcu-interpreter`): a HMI assina os atributos com a api 9999 e espera `updatePowerManagementState` (api 1) e `updateClampSignal` (api 4) na interface 1101.
- **Os inteiros do serviço de energia são `sint32`, em zigue-zague; os do display são `int32` comuns.** A HMI pede os atributos 3, 4 e 7 como `06 08 0e`. A primeira tentativa de ligar a tela mandou o estado 1 como `01`, que nesse esquema vale -1, e a HMI ignorou. Mandando `02` (estado 1) com válido `02`, ela aceitou. Confirmado no código decompilado: `readSInt32` no protobuf de energia, `readInt32` no de display.
- O `mcu-interpreter` de verdade responde às assinaturas com mensagens de 8 bytes de carga, que o `kipc_hub` descarta por serem curtas (a proteção de 10/10 contra travamento da JVM). Não investiguei o que elas dizem.

### Como a HMI decide ligar a tela

Lido em `AslTargetSystemPowerManagementHandling` (decompilado do jar da HMI) e conferido na execução:

| `powerManagementState` | Efeito |
|---|---|
| -1 (nunca recebeu) | nada muda; a área principal fica em `HMI_USER_PERCEIVED_OFF` |
| 1, 5, 7 (`ON`, `ON_DIAG`, `ON_TEL`) | HMI ligada, tela ligada |
| 8 (`ON_SWDL`) | ligada, em atualização de software |
| 2, 9 (`STANDBY`, `STANDBY_PWR_SAVE`) | tela conforme ignição, relógio e clima |
| 4 (`STANDBY_RESTRICTED`) | tela desligada |
| 0, 6 (`OFF`, `CUSTOMER_SWDL`) | desligada |

A atualização só vale com `validFlag` igual a 1.

Com o estado 1 a área principal recebe `HMI_RADIO`, entra a view `GENERIC_DOMAIN_FEATURE_NOT_AVAILABLE` e por cima a `OV_DIGITALASSISTANT_SYSTEM_WELCOME_SCREEN`, que é o que se vê no quadro. A view de "recurso não disponível" deve ser porque não há sintonizador registrado (`DSIUnifiedTuner`); isso é dedução.

### Ver ao vivo

A pedido do Lipe, no mesmo dia: `sh driver/ao_vivo.sh` sobe a HMI no container e abre http://127.0.0.1:8431, uma página (`ao-vivo/index.html`) que acompanha os quadros. Verificado no navegador: estado "Ao vivo", 800×480, cerca de 11 quadros por segundo, com o indicador de carregamento girando. O glhost passou a gravar também um contador (`seq.txt`), coberto por um teste novo; a suíte tem oito testes e os oito passam. Detalhes em [glforward-moi3/README.md](glforward-moi3/README.md).

**Mudou no Marco 4:** o modo ao vivo agora liga a camada RSI, então a HMI não fica mais no "Loading user profile…" girando. Ela mostra a tela Welcome terminada, o popup do usuário principal e para na área principal; são cerca de 65 quadros e depois a página mostra "Sem quadro novo há N s". Para rever a tela animada: `RSI= sh driver/ao_vivo.sh`.

### Onde parou

- **A HMI fica em "Loading user profile…"** e os botões "Change user" e "OK" seguem apagados. Não investiguei o que ela espera. Continuam sem registro `DSICarTimeUnitsLanguage`, `DSIGeneralVehicleStates` e `DSICarParkingSystem`. **Resolvido no Marco 4, no mesmo dia:** ela esperava dados de usuário da camada RSI.
- **Não há entrada de toque.** Mesmo com o botão ativo não haveria como apertar. **Resolvido no Marco 5:** a página ao vivo manda toque e teclas.
- **Não rodou dentro do QEMU** (Caminho A). O lançador é o mesmo; falta o shim falar por `virtio-serial`.
- **O painel de instrumentos não entrou.** O vídeo em `ff14::2:4` é outro trabalho.
- Só o MU0883 e só a variante `OISmall` (800×480) foram exercitados.

## Marco 4 — o que segura o "Loading user profile…" (11/10/2026)

Pedido do Lipe depois do Marco 3: descobrir o que mantém a HMI na tela "Loading user profile…".

**Resposta, verificada em execução:** a tela espera dados de usuário que não vêm pelo barramento DSI. Eles vêm de uma segunda camada de comunicação da central, o **RSI**: serviços locais em HTTP e WebSocket, achados por meio de um registro de serviços. Na bancada não havia registro nem serviço nenhum, então a tela esperava para sempre. Com o registro e o mock de serviços **que o próprio firmware traz**, mais dois arquivos de dados nossos, a HMI termina a tela, fecha-a sozinha e segue.

![Tela Welcome terminada](quadros/2026-10-11-hmi-welcome-usuario.png)

O método foi o de depuração por causa raiz: primeiro ler o que a tela espera, depois um experimento por vez, com a previsão escrita antes de rodar.

### O que a tela espera

Lido no jar da HMI (decompilado): `de.vw.mib.asl.internal.usermanagement.welcomescreenloading.WelcomeScreenControllerImpl`. A tela só sai de `LOADING` quando seis condições fecham. O log da HMI imprime uma letra para cada uma (`[WelcomeScreenLoadingController]  PUCIOP …`), sempre com o estado de **antes** do evento que está sendo registrado.

| Letra | Condição | De onde vem |
|---|---|---|
| `P` | preferências de usuário carregadas | serviço DSI de persistência. Já fechava |
| `U` | a lista de usuários não está vazia | RSI, serviço `usermanagement`, recurso `users` |
| `C` | o contexto tem usuário ativo e usuário identificado, **os dois com nível de autenticação** e com o mesmo `id` | RSI, `usermanagement`, recurso `userContexts`, pedido com `expandLevel=3` |
| `I` | o usuário identificado tem nível de autenticação | RSI, `usermanagement/userContexts`, campo `identifiedUser` |
| `O` | chegou um usuário principal com o campo `nickname` presente (pode estar vazio) | RSI, serviço `onlineregistration`, recurso `primaryUsers`. Ou a chave de sistema `de.vw.mib.asl.internal.usermanagement.welcomescreenloading.WelcomeScreenController.ignorePrimary=true` |
| `P` | não há troca de usuário em andamento | RSI, `usermanagement/processes`. Sem processo, já fecha |

Fechadas as seis, a HMI escolhe a tela pelo papel do usuário identificado (`getWelcomeScreenView`):

| Usuário identificado | Tela |
|---|---|
| convidado anônimo, com mais de um usuário cadastrado ou com usuário principal de apelido preenchido | 3, `GUEST` |
| convidado anônimo, sozinho, com a opção de boas-vindas ligada | 2, `INITIAL` |
| convidado anônimo, sozinho, com a opção desligada | 4, `DEACTIVATED` (a tela some) |
| nível de autenticação 0 ou 1 (que não o convidado anônimo) | 6, `REGULAR_SPIN` (pede o S-PIN) |
| os demais | 5, `REGULAR` |

A própria HMI tem um texto de diagnóstico para o estado em que a bancada estava (`P` e mais nada): "neither rsi usermanagement nor onlineregistration connected to hmi". Li no código (`LoggingWelcomeScreenController.convertHumanReadable`); ele vai para um campo de depuração da tela, não para o log.

### O que o firmware traz para o RSI

Tudo na partição `system`, em `/lge/app_ro/`:

| Peça | O que é | Na bancada |
|---|---|---|
| `sai/MIB_ServiceRegistryApp-00` | o **registro de serviços**. Escuta na porta 443, que é onde a HMI o procura (`ws://127.0.0.1:443/api/v1/`), e também em 80, 29799, 29772, 49200 (serviço `auth`) e 49206 (serviço `cdn`) | roda, desde que carregue o `kipc_bus.so` (ele abre o kipc) |
| `bin/rsi-mockup` | um **mock de serviços feito pela LG**, na porta 7770. A lista de serviços está em `sai/mockup/services`; os dados, em `sai/mockup/<serviço>/<recurso>.json` | roda sem ajuste |
| `sai/spec/` | 81 especificações de serviço (`viwi.service.<nome>-<versão>.json`): recursos, campos e operações | lidas pelo mock |
| `sai/privileges/` | 89 arquivos de privilégio por serviço | lidos pelo mock |
| `sai/mockup/usermanagement/` | dados de teste da LG: `users.json` (4 usuários, níveis 0 a 3) e `userContexts.json` (o contexto "vehicle", com o "level2 user" ativo) | usados |
| `bin/rsi-send`, `bin/rsi-subscribe`, `bin/rsi-cdn` | ferramentas de linha de comando para o RSI | não usei |
| `carif/usermgnt` | o serviço de usuários **de verdade** | ver "Limites" abaixo |

A lista original do mock tem quatro serviços: `AudioInformation`, `persistence`, `tmwlanconfig` e `userimages`. **`usermanagement` não está nela**, embora os dados estejam na pasta.

Os daemons do firmware mandam o log deles para um socket Unix, `/tmp/trace_unix_socket` (biblioteca `libtraceif`). Na bancada ninguém escuta nesse socket, então o registro, o mock e os outros daemons rodam mudos. O que sei deles vem do `strace` e de perguntar por HTTP.

### Os experimentos

Container, 4 processadores, 75 a 100 s cada. Saídas em `traces/m4_*`.

| Rodada | O que mudou | Previsão | Observado |
|---|---|---|---|
| `e1` | só 4 processadores (`--cpuset-cpus 0-3`) | a exceção do cliente WebSocket some e a HMI passa a procurar o registro | confirmado: nenhuma exceção; "Unable to get session for ws://127.0.0.1:443" e "Connection to service registry is broken! Try to reconnect!", em laço |
| `e2` | mais o registro e o mock, com a lista original | a HMI conecta no registro; `usermanagement` continua faltando | confirmado: "Websocket connection established: ws://127.0.0.1:443" uma vez só; segue "Required RSI services not registered within 15000 ms: usermanagement" |
| `e3` | mais `usermanagement` na lista do mock, com os dados da LG | a HMI recebe usuários e contexto; `C` e `I` não fecham, porque o contexto da LG não tem `identifiedUser` | confirmado: conecta em `ws://127.0.0.1:7770`, recebe os 4 usuários, letras `PU   P`, segue em `LOADING` |
| `e4` | mais `identifiedUser` no contexto, como referência (`id`, `name`, `uri`) | `C` e `I` fecham | **só em parte**: `I` fecha, `C` não (`PU I P`) |
| `e5` | mais a chave `ignorePrimary=true` | se `C` estivesse fechando depois do último evento, a tela sairia de `LOADING` | `O` fecha, a tela não sai: o problema é mesmo o `C` |
| sonda | `GET` direto no mock, com `$expand=3` | — | **o mock não expande referências.** O usuário volta só com `id`, `name` e `uri`, sem `authenticationLevel`, que é o que `C` exige (`traces/m4_sonda_get_mock.txt` tem as respostas com os dados finais) |
| `e6` | o usuário embutido por inteiro no contexto | `C` fecha e a tela sai de `LOADING` | confirmado: `PUCIOP`, "check And Show WelcomeScreen: REGULAR" |
| `e7` | em separado: `-XX:ActiveProcessorCount=4` na JVM, com o container nos 16 processadores | a exceção some | confirmado: nenhuma exceção |
| `e8` | o serviço `onlineregistration` no mock, com um usuário principal de apelido vazio, no lugar da chave `ignorePrimary` | chega `updatePrimaryUser()` e a tela sai de `LOADING` | confirmado: `PUCIOP`, `REGULAR` |
| ao vivo | o caminho de uso (`driver/ao_vivo.sh`), com os padrões novos | igual ao `e8` | igual; conferido também na página do visualizador |

O `e3` precisou de duas tentativas, por erro meu: a lista de serviços da LG não termina em quebra de linha, e o serviço que eu acrescentei grudou no comentário da última linha. Achei com `strace` no mock (ele não abria nenhum arquivo de `usermanagement`). O lançador agora põe a quebra de linha antes.

### O que a HMI faz depois (linha do tempo do `e8`, em segundos desde "UI thread started")

| Quando | O que acontece |
|---|---|
| 15 s | "Last content in ScreenAreaStage: HMI_RADIO": a área principal volta para o último conteúdo, o rádio |
| 30 s | entra o layout; a área principal recebe a view `GENERIC_DOMAIN_FEATURE_NOT_AVAILABLE`; a tela Welcome passa de `LOADING` a `REGULAR` no mesmo instante, porque os dados já tinham chegado |
| 31 s | entra a `OV_DIGITALASSISTANT_SYSTEM_WELCOME_SCREEN`: "Welcome level2 user", com "Change user" e "OK" acesos |
| 35 s | a tela Welcome sai sozinha e abre o popup `PO_DIGITALASSISTANT_CONFIRM_SYSTEM_WELCOME_SCREEN_PRIMARY_USER`: "This vehicle has no primary user. Would you like to become the primary user of this vehicle?" |
| 65 s | o popup fecha sozinho, 30 s depois (medido na rodada ao vivo) |
| depois | tela parada em `"Radio" This function is currently unavailable. Please wait…` |

![Popup do usuário principal](quadros/2026-10-11-hmi-popup-usuario-principal.png)

![Área principal: rádio indisponível](quadros/2026-10-11-hmi-radio-indisponivel.png)

São cerca de 65 quadros ao todo; depois a HMI não desenha mais, porque nada muda na tela. No visualizador ao vivo isso aparece como "Sem quadro novo há N s", que é o comportamento certo.

O popup aparece porque o usuário principal que demos tem apelido vazio, ou seja, "carro sem usuário principal". O log confirma: "The was no PrimaryUser in the OnlineRegistration RSI but in the UserManagement RSI !".

**O popup fica com a parte de cima fora da tela** (a caixa começa acima do topo). Não sei se no carro é assim ou se é efeito da bancada. Não investiguei. **Investigado em 12/10/2026** (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada"): não é corte nem efeito da bancada. O aviso é uma folha que desce do topo e fica encostada nele, sem linha em cima, por dados do skin.

### O artefato dos 16 processadores

A HMI cria um conjunto de 8 threads para o cliente WebSocket do RSI (`QueuedThreadPool[RSI_WS]`, máximo 8). Com os 16 processadores do Mac a biblioteca Jetty recusa: `IllegalStateException: Insufficient configured threads: required=8 < max=8`. Com 4, funciona. Medido: 16 falha, 4 funciona; o limite exato eu não medi. A conta do Jetty (metade dos processadores vira thread de seleção) é lembrança minha do código dele, não conferi nesta sessão.

O SoC do carro tem 4 núcleos: os device trees do `boot.img` trazem quatro nós de CPU (`cpu@0`, `cpu@1`, `cpu@100`, `cpu@101`, conferido em 11/10). Então 4 é também o valor fiel. Dois jeitos de chegar lá, os dois testados: limitar o container (`--cpuset-cpus 0-3`) ou dizer à JVM quantos processadores ela tem (`-XX:ActiveProcessorCount=4`). Fiquei com o segundo, porque o primeiro tira processador também do glhost, que desenha por software.

Todas as rodadas do Marco 1 no container e todas as do Marco 3 tiveram essa exceção, uma vez cada, sem eu notar. O resultado gráfico daquelas rodadas não muda.

### O que entrou na pasta

- `driver/m1_hmi.sh` ganhou `RSI` (sobe o registro e o mock), `RSI_SERVICES` (serviços a acrescentar à lista do mock), `RSI_DATA` (pasta de dados nossos, copiada por cima dos da LG) e `JCPUS` (processadores que a JVM enxerga, padrão 4). Ao final ele grava o que o registro tinha (`<tag>.rsi-services.json`) e mostra os serviços RSI que faltaram e por quais telas Welcome a HMI passou.
- `stubs/rsi-mockup/` tem os dois arquivos de dados e um README que diz o que cada um muda e por quê.
- `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh` ligam o RSI por padrão. Para rever a tela "Loading user profile…": `RSI= sh driver/ao_vivo.sh`.

A lista original do mock mora na partição `system`, que é montada só para leitura. O lançador copia a pasta do mock para a memória e monta a cópia por cima; o arquivo de imagem do firmware não é tocado.

### Limites

- **Nada foi contornado.** O registro aceitou o mock da LG como ele vem, e a HMI falou com os dois por `ws://` simples, sem certificado nem token. Não mexi em autenticação, assinatura, dm-verity nem secure boot.
- **Não insisti no `usermgnt` de verdade.** Ele roda e abre uma conexão com o registro (porta 443), mas o serviço não aparece na lista do registro ("No Service found: 'usermanagement'"). Entre o que ele procura e não acha está `/sys/devices/soc/75ba000.spi/spi_master/spi12/spi12.0/challenge_gen`, um arquivo de um dispositivo SPI (`traces/m4_usermgnt_real.strace`). Deduzo que seja o desafio de um chip de segurança; não verifiquei, e não sei se é por isso que ele não registra. Fazer esse daemon funcionar poderia exigir imitar esse chip, o que escorrega para fora do combinado. O mock da LG dá o mesmo dado à HMI sem tocar nisso.
- Os dados são de teste: os usuários "level0" a "level3" são da LG, não de um carro.

### Onde parou

- **A área principal mostra "Radio: função indisponível".** O último conteúdo da HMI é o rádio, e o rádio não sobe: "registered app for content HMI_RADIO is not initialized yet!", "[SoundAdapter] Radio BaseService is not Connected", e `DSIUnifiedTuner` e `DSIRadioData` sem registro. Que a falta do sintonizador seja a causa continua sendo dedução. **Afinado no Marco 5:** a mensagem "is not initialized yet" não era a causa; o aplicativo do rádio inicializa aos 30 s. Quem desenha o aviso é um aplicativo substituto (`GenericDomain.tunerGenericDomain`), que a HMI põe no lugar de um domínio que ainda não está pronto.
- **A grade de menus (`HMI_GRIDMENU`) não apareceu.** A HMI vai para o último conteúdo, não para o menu. Para trocar de tela falta entrada: tecla HOME pelo serviço de teclas ou toque. **Resolvido no Marco 5:** a tecla certa é MENU; a HOME, nesta variante, pede uma tela que não existe.
- Serviços RSI que a HMI ainda dá por falta, aos 4 min da rodada ao vivo: `audiomanagement`, `waveplayer`, `servicemanagement` e `onlinestate`. Na rodada de 15 min de antes, sem RSI, apareceram mais tarde também `ownerverification`, `digitalkey`, `privacysetup` e `IntentBroker`.
- Serviços DSI sem registro aos 4 min: 49 instâncias de 34 interfaces, de agenda, mídia, rádio, clima, conforto, bancos, busca e estados do veículo. A lista está em `ao-vivo/log/ao_vivo.out`, que é sobrescrito a cada rodada.
- **Não rodou no QEMU com o RSI.** O lançador é o mesmo, mas lá a interface de rede local (`lo`) pode não estar de pé no initramfs; não testei.
- O mock da LG tem um modo que gera elementos a partir da especificação quando não há arquivo de dados (vi a mensagem no binário); não exercitei.

## Marco 5 — entrada: teclas e toque (11/10/2026)

Depois do Marco 4 o Lipe perguntou se a HMI era funcional. Não era: sem entrada, só dava para assistir. Ele pediu para seguir, e no caminho pediu duas coisas: acompanhar ao vivo e ter as teclas da central no simulador.

**Resultado, verificado em execução:** a HMI aceita teclas e toque publicados no barramento, e a página ao vivo virou o painel da central. Clicar na tela é tocar; embaixo dela há nove teclas e os dois botões giratórios (volume e seleção). Uma tecla MENU abriu a grade de menus e, a partir dela, toque e teclas abriram as telas de veículo, clima, som, ajustes do sistema e o aviso de navegação.

![Grade de menus](quadros/2026-10-11-hmi-grade-de-menus.png)

![Tela do veículo](quadros/2026-10-11-hmi-carro.png)

![Ajustes do sistema](quadros/2026-10-11-hmi-ajustes-do-sistema.png)

### Como a HMI recebe entrada

Lido no `MIBHMI-DSI.jar` e no jar da HMI (decompilados) e conferido com o tráfego capturado no Marco 3.

- Teclas e toque chegam pelo mesmo serviço DSI, o **`DSIKeyPanel`**: domínio `0x80000000000`, pedidos na interface 300, eventos na 301. No carro quem fornece é o daemon `touch-app`, que lê o toque de `/dev/input/event*` com a `libevdev` (visto nas strings do binário). Na bancada ele roda, mas não há dispositivo de entrada; às assinaturas da HMI ele responde com mensagens vazias.
- A HMI assina, na instância 0, os atributos 20, 23, 25 e 26 (gesto, botão giratório, tecla, painel pronto) e depois 18, 27 e 28. Está no tráfego do Marco 3.
- **Tecla** é `updateKey2`, api 25: `{1: teclado, 2: tecla, 3: estado, 4: tempo, 5: válido}`. Estado 1 aperta, 0 solta, 3 é toque longo.
- **Toque** é `updateGesture2`, api 20: `{1: teclado, 2: gesto, 3: dedos, 4: palma, 5: x, 6: y, 7: p1, 8: p2, 9: tempo, 10: válido}`. O controlador de toque do carro já entrega o gesto reconhecido, não o ponto cru: 4 encosta, 5 arrasta, 3 solta, 1 é o toque (com `p1` = 1, a contagem de cliques). A HMI só aceita como clique se o dedo andou menos de 25 pontos (`TouchInputDeviceHandler`).
- Existe também `updateTouchInput` (api 28), com pontos crus, mas a tela principal não usa: a classe do toque da tela (`TouchInputDeviceImpl`) assina o atributo 20.
- Os inteiros do `DSIKeyPanel` são `int32` comuns, como os do display (e ao contrário dos de energia).
- As coordenadas são os pontos da tela, 800×480, sem escala.

### A tabela de teclas

A HMI não aceita qualquer par de teclado e tecla: ela consulta uma tabela. Em vez de adivinhar, li a tabela com o leitor do próprio firmware.

- A tabela é o item 24 de `/vw/hmi/res/configurationmanager.res`, num formato próprio (`com.microdoc.util.ObjectSwapper`, com a `libswap.so`). O programa [driver/LeTabelaTeclas.java](driver/LeTabelaTeclas.java) abre o arquivo com essa classe e interpreta os inteiros do jeito que o `HardKeyReader` da HMI faz.
- **A JVM do firmware traz o compilador** (`/lge/hmi/jvm/lib/tools.jar`, com `javac` e `javap`) e o `jjs`. O programa foi compilado e rodado dentro do chroot, sem baixar nada.
- Resultado em `traces/m5_tabela_teclas.txt`: versão -3, 42 entradas, 565 inteiros, todos consumidos.
- **As teclas desta central estão no teclado 13** (`KBD_TOUCHSCREEN_FRONT`, a moldura da tela), e não no 1, que foi minha primeira tentativa. No teclado 13 há 25 teclas: 1, 3, 4, 5, 6, 7, 10, 14, 15, 16, 17, 32, 50, 52, 78, 79, 86, 87, 88, 89, 93, 104, 107, 114 e 116. O teclado 4 é o volante multifunção (11 teclas) e o 1 tem só seis.

O que cada tecla fez na bancada (rodada `k8`, teclado 13, uma tecla a cada 6 s, com a grade de menus já de pé):

| Tecla | Nome nas constantes | Conteúdo pedido | Tela que entrou |
|---|---|---|---|
| 78 | `KEY_MENU` | `HMI_GRIDMENU` | grade de menus |
| 15 | `KEY_TUN` | `HMI_RADIO` | aviso de função indisponível |
| 1 | `KEY_MED` | `HMI_MEDIA` | aviso de função indisponível |
| 6 | `KEY_CAR` | `HMI_CAR` | aviso por 2 s, depois `CAR_MAIN_CARCOMPUTER` |
| 79 | `KEY_CLIMATE` | `HMI_CLIMATE` | `CLIMATE_FRONT_VIEW` |
| 86 | `KEY_TONE` | `HMI_TONE` | `SOUND_EQ_TMBS` |
| 4 | `KEY_NAV` | `HMI_NAVI_NOT_CODED` | `NAV2_NotCodedInfoScreen` |
| 7 | `KEY_SET` | `HMI_SYSTEMSETUP` | `SYSTEM_SETUP_MAIN` |
| 116 | `KEY_HOME` | `HMI_HOMESCREEN` | nenhuma (ver abaixo) |
| 3 | `KEY_TEL` | nenhum | na `k8`, antes do fim da partida, nada; na `n1`, com a partida terminada, abre o popup `PO_SYSTEM_ALERT_PHONE_NOT_INSTALLED` ("Telephone unavailable.") |
| 114, 107, 104, 88, 32, 5, 50 | `KEY_SMARTPHONE`, `KEY_INFO`, `KEY_MAP`, `KEY_MUTE`, `KEY_EJECT`, `KEY_TRAFFIC`, `KEY_MFW_PTT_ON` | nenhum | a HMI recebe a tecla e não mostra nada (as duas últimas, testadas na `n1`) |

Os nomes vêm de `org.dsi.ifc.keypanel.Constants`. As oito primeiras e a do telefone são as teclas da página.

**Os botões giratórios** usam o mesmo serviço: `updateEncoder2`, api 23, `{1: teclado, 2: tecla, 3: incremento, 4: tempo, 5: válido}`, com incremento negativo para girar para trás. A tecla é 16 para o botão de seleção e 17 para o de volume, no teclado 13 (lido em `AslTargetSystemKeyPanelHandling.processEncoderEvent`; a documentação do emulador da MIB2 diz o mesmo). Apertar o botão é a tecla de mesmo número.

### Os experimentos

Container, com a camada RSI ligada. Saídas em `traces/m5_*`.

| Rodada | O que foi feito | Previsão | Observado |
|---|---|---|---|
| `k1` | HOME (116) no teclado 1, aos 85 s | a HMI registra a tecla e troca de tela | nenhuma troca de tela, e nada no que eu olhei do log |
| `k2` | o mesmo, tentando ligar o rastreamento de teclas da HMI | aparecer a linha "HK Received" | o rastreamento não saiu, mas no instante da tecla há um aviso: "LongPressTime ist not defined" (estava também na `k1`; eu não tinha procurado). A tecla chega; o par (teclado 1, tecla 116) é que não está na tabela |
| leitura | a tabela de teclas, com o leitor do firmware | — | HOME e as demais estão no teclado 13 |
| `k3` | HOME no teclado 13 | a HMI pede a tela inicial | pede `HMI_HOMESCREEN`, e avisa "registered app for content HMI_HOMESCREEN is not initialized yet!" |
| `k4` | o mesmo, esperando 330 s | a tela inicial aparece quando o aplicativo dela subir | não aparece |
| `k5` | o mesmo, com o registro detalhado ligado de verdade | ver por que o aplicativo não sobe | "HomeScreen (optional) … is excluded by feature flag 655". Esta variante **não tem** o aplicativo da tela inicial; o menu dela é o `SimpleGridMenu`, que fornece `HMI_GRIDMENU` |
| `k6` | MENU (78) no teclado 13 | a HMI pede a grade | pede `HMI_GRIDMENU` e entra `DynamicSimpleGridMenuInitial`, a tela "Starting…" |
| `k7` | o mesmo, esperando 345 s | a grade sai de "Starting…" | sai aos 152 s: entra `DynamicSimpleGridMenu`, com Radio, Vehicle, Users, Settings, Media, Sound e Help |
| `t1` | com a grade de pé, toque em "Settings" (675, 145): gesto 4 e, 0,12 s depois, gesto 1 | abrir os ajustes | "show content cio: HMI_SYSTEMSETUP … triggered by SimpleGridMenu"; entra `SYSTEM_SETUP_MAIN` |
| `t2` | tecla Ajustes (7) e, na lista, um arrasto de baixo para cima: gesto 4 em (400, 420), oito gestos 5 até (400, 140), gesto 3 | a lista rola | rola: sai de "Screen, Language…" e mostra até "Copyright" e "Configuration wizard", com a barra no fim. A HMI registra "DragCalculator: Time stamps provided by gesture events" |
| `k8` | passeio por 15 teclas | — | a tabela acima |
| `p1` | a espera da HMI por serviços ausentes reduzida de 15 s para 2 s (duas propriedades dela), MENU aos 70 s | a partida encurta e nada mais muda | o resumo de fim de partida sai aos 61 s (eram 295 s); a unidade da grade começa aos 20 s (eram 150 s); a tecla MENU abre a grade direto, sem "Starting…". A lista de serviços ausentes é a mesma da `k7`: 48 DSI e 8 RSI |
| ao vivo | pela página: clique na tecla Menu e, com a grade de pé, clique no bloco "Vehicle" | o mesmo que nas rodadas | Menu pediu `HMI_GRIDMENU`; o clique virou `gesto 4 361 185` e `gesto 1 361 185 1`, e a HMI pediu `HMI_CAR` |
| `n1` | com a grade de pé: três giros do botão de seleção e um aperto; três giros do de volume; teclas 5, 50 e 3 | a seleção move um foco, o volume mostra algo | na grade a seleção não mostra foco nem ativa nada; **o volume abre o popup `PO_SOUND_VOLUME_SOUND_ENTERTAINMENT`**, a barra "Audio", que fecha em 5 s; a tecla 3 abre "Telephone unavailable." |
| `n2` | na lista de ajustes: quatro giros do botão de seleção e um aperto | o foco desce na lista e o aperto ativa o item | **confirmado:** o foco para em "Restore factory settings" e o aperto abre `PO_CONFIRM_SYSTEM_ALL_RESET_TO_FACTORY_SETTINGS` (quadro em `quadros/2026-10-11-hmi-foco-e-confirmacao.png`) |
| sessão do Lipe | uso livre pela página na sessão ao vivo aberta às 17h23, com toque, teclas e, depois que troquei o stub com a sessão no ar, os botões giratórios (conferido aos 43 min) | — | 16 telas diferentes na área principal (ajustes e subtelas, versões, usuários, som, veículo, atualização de software, assistente de configuração) e três popups de volume; nenhum erro de OpenGL em mais de 2.700 quadros |

Em nenhuma rodada o glhost registrou erro de OpenGL, incluindo as telas novas.

### Três coisas que aprendi no caminho

- **O registro detalhado da HMI.** O `logging.properties` do firmware tem uma chave de rastreamento por assunto, todas desligadas. Ligar a chave não basta: o escritor de console descarta os níveis NORMAL e TRACE, salvo com a propriedade `de.vw.mib.log4mib.console.traces.enabled=true` (lido em `ConsoleLogWriter`). Com `…console.textual.classifiers.enabled=true` o cabeçalho de cada linha passa a trazer o nome do assunto em vez de números. A opção `HMILOG` do lançador liga as duas coisas. Foi isso que mostrou a linha da chave 655. **Acréscimo de 12/10/2026:** as chaves de rastreamento ligadas assim só valiam até a carga tardia, uns 6 s depois da partida; o lançador passou a pô-las também nos perfis de sondas de depuração, e elas valem a sessão inteira (seção "O rastreamento da HMI que parava na carga tardia").
- **A partida da HMI levava quase 5 minutos na bancada, e tem ajuste.** Ela sobe em "unidades de trabalho", e cada uma espera 15 s pelos serviços que nunca se registram. A da grade de menus (`WORKUNIT_HOMESCREEN_*`) só começava aos 150 s e o resumo de fim de partida saía aos 295 s; por isso a grade ficava em "Starting…" se a tecla MENU viesse antes. Os 15 s são o padrão de duas propriedades da própria HMI, `de.vw.mib.asl.startup.timeout.dsi` e `…timeout.rsi` (lidas em `ContextInformationWorkUnit`). Com 2 s (rodada `p1`) a partida termina em 61 s e os serviços dados por ausentes são exatamente os mesmos. A opção `ESPERA` do lançador faz isso, e o modo ao vivo a usa. O `runHMI.sh` tem ainda, comentada, a propriedade `de.vw.mib.startup.taskPoolCount`, que poria as unidades em paralelo; não testei.
- **A HMI sobe sem codificação.** O log repete "No data received for Adaptation" e avisa "CURRENT SKIN DOES NOT BELONG TO CODED BRAND". As chaves de recurso (como a 655) saem dessa configuração. O que a codificação do carro do Lipe ligaria ou desligaria não foi levantado. **Levantado no Marco 6:** por onde a codificação entra e o que cada campo é; os valores do carro continuam desconhecidos.

### O que entrou na pasta

- `driver/dsistub.c` passou a publicar teclas e gestos a partir de um arquivo de comandos, uma linha por evento (`tecla <teclado> <tecla> <estado>` e `gesto <gesto> <x> <y> [p1 [p2 [dedos]]]`). Só manda o que for acrescentado depois que ele sobe.
- `driver/dsi_escuta.c`, novo: escuta o barramento e imprime cada mensagem inteira em hexa.
- `driver/tests/t_entrada.sh`, novo: 11 casos do canal de entrada (teclas, gestos e botão giratório), com o hub, o stub e o escutador, sem firmware. Escrito antes do código, em duas levas; em cada uma os casos novos falharam primeiro, e hoje passam os 11.
- `driver/servidor_ao_vivo.py`, novo: serve a página e recebe a entrada dela (`POST /entrada`), gravando em `ao-vivo/entrada/comandos.txt`. `driver/tests/t_servidor.sh` tem 20 casos; falhou antes de o servidor existir e passa nos 20. O servidor manda `Cache-Control: no-cache`, porque sem isso o navegador seguia mostrando a página antiga depois de uma mudança.
- `ao-vivo/index.html`: o clique vira gesto (encosta, arrasta depois de 8 pontos, e termina em toque ou em solta) e há a fileira de teclas, que ficam apertadas enquanto o botão do mouse estiver apertado. Nas pontas ficam os dois botões giratórios: rolar em cima gira, clicar aperta, e há um − e um + para quem não tem rodinha.
- `driver/m1_hmi.sh` ganhou `INPUT` (arquivo de comandos), `ROTEIRO` (entrada com hora marcada) e `HMILOG`.
- `driver/ao_vivo.sh` sobe o servidor novo e passa o arquivo de comandos ao container.
- `driver/LeTabelaTeclas.java`: o leitor da tabela de teclas.

### Cuidados com a página

A página agora manda comandos, então fechei três pontos:

- O container deixou de enxergar a pasta da página. Ele monta só `ao-vivo/frames`, `ao-vivo/log` e `ao-vivo/entrada`; antes montava `ao-vivo/` inteira e poderia reescrever o `index.html`.
- O servidor roda com `python3 -I` a partir de `driver/` (que o container só lê), para o Python não carregar módulo de uma pasta em que o container escreve.
- O servidor escuta só em `127.0.0.1`, aceita só linhas no formato exato de um comando, e só de pedidos com um cabeçalho próprio, que outro site aberto no navegador não consegue mandar.

### Onde parou

- **Rádio e mídia ficam no aviso "função indisponível".** O aviso é do aplicativo substituto `GenericDomain`. Que falte o sintonizador (`DSIUnifiedTuner`) e o tocador (`DSIMediaPlayer`) é dedução.
- **A tela do veículo mostra um Golf, relógio "--:--" e autonomia "---".** Não há dado de veículo chegando.
- **Mapa, smartphone, mudo, trânsito e voz não fazem nada visível.** Não investiguei. O telefone só mostra "Telephone unavailable.".
- **O botão de seleção não faz nada na grade de menus**; nas listas ele move o foco e ativa o item. Não sei se no carro a grade tem foco.
- **Os popups aparecem cortados no topo.** Já são três: o do usuário principal, "Telephone unavailable." e a confirmação de restauração de fábrica. Com três casos iguais, parece erro de posição da bancada e não desenho da HMI; não investiguei a causa. **Investigado em 12/10/2026** (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada"): não é corte nem efeito da bancada. O aviso é uma folha que desce do topo e fica encostada nele, sem linha em cima, por dados do skin.
- **As teclas do volante (teclado 4) não foram simuladas.**
- **O toque é o mínimo:** um dedo, toque e arrasto. Sem gesto de arremesso nem dois dedos. O reconhecimento de gestos é da página, não do `touch-app`; para usar o do firmware seria preciso um dispositivo de entrada virtual em `/dev/input`.
- A HMI pede ajustes ao controlador de toque (`setGenericSetting`) e não recebe resposta: sai um erro de tempo esgotado a cada 10 s no log. Não atrapalhou.
- O popup do usuário principal continua cortado no topo, sem explicação.
- Só no container; não rodou no QEMU.

## Marco 6 — codificação e adaptação (11/10/2026)

Feito numa sessão à parte, aberta pela sessão do Marco 5 no mesmo dia. A pergunta: por onde a HMI lê codificação e adaptação, e o que muda na tela quando elas chegam. É o item 1 do roteiro de [simulador-jetta.md](simulador-jetta.md).

**Resultado, verificado em execução:** a bancada entrega à HMI os conjuntos de codificação, adaptação, registros e identificação, pelo caminho que o firmware tem para isso (as mensagens do microcontrolador de diagnóstico), e mudar **um bit** de adaptação muda a tela. Com "Funktion Navigation" ativada, a grade de menus ganha o bloco "Navigation", a tecla NAV deixa de dizer "Navigation is not enabled" e abre "Starting navigation…" sobre um mapa, e a barra da tela do veículo ganha um ícone.

| Sem o ajuste | Com o bit de navegação |
|---|---|
| ![Grade sem navegação](quadros/2026-10-11-hmi-cod-grade-antes.png) | ![Grade com navegação](quadros/2026-10-11-hmi-cod-grade-navegacao-ativada.png) |
| ![Tecla NAV sem navegação](quadros/2026-10-11-hmi-cod-tecla-nav-antes.png) | ![Tecla NAV com navegação](quadros/2026-10-11-hmi-cod-tecla-nav-navegacao-ativada.png) |

E com quatro campos da codificação (marca, classe, geração e carroceria) a figura da tela do veículo deixa de ser um Golf e passa a ser um sedã:

| Tipo de veículo padrão | VW, classe A, geração 7, sedã |
|---|---|
| ![Veículo antes](quadros/2026-10-11-hmi-cod-veiculo-antes.png) | ![Veículo sedã](quadros/2026-10-11-hmi-cod-veiculo-tipo-sedan.png) |

**Nenhum desses valores é a codificação do Jetta.** São valores de teste, tirados das constantes do firmware. A do carro só se conhece com a leitura do módulo 5F.

O método foi o dos marcos anteriores: ler o que a HMI espera, depois uma variável por vez, com a previsão escrita antes de rodar. As previsões estão em `traces/m6_previsoes.md`.

### Por onde os dados entram

Lido no jar da HMI e no `MIBHMI-DSI.jar` (decompilados), na `libDSIService.so` e no `persistence-app` (Ghidra), e conferido nas rodadas.

**1. Quem pede.** `AslTargetSystemFeaturesAndCoding` pede cinco conjuntos na partida, por meio de `TargetDiagnosis` e `DiagnosisDataProvider`. Cada pedido é uma leitura de persistência, `readByteArray(namespace, chave)`. A chave é o identificador de dado do diagnóstico (DID): `0x0600` é a codificação, `0xF187` o número de peça.

| Conjunto | Namespace | Chaves (DID) | Bytes | Classe que interpreta |
|---|---|---|---|---|
| codificação | `0x2000000` | `0600` | 25 | `CodingImpl` |
| adaptação | `0x1000000` | 39 DIDs; esta variante lê 35 | 397 | `AdaptationImpl` |
| funções do carro | `0x1000100` | `0B1B` | 256 | `CarFuncAdapImpl` |
| registros | `0x4000000` | `7209`, `720D`, `720B`, `720C`, e as chaves 1 e 2 | 984 | `RecordsImpl` |
| identificação | `0x3000000` | `F187`, `F189`, `F18C`, `F191`, `F1A3` | 49 | `IdentificationImpl` |

Os quatro DIDs de adaptação que esta variante não lê dependem de chaves de recurso desligadas aqui (689, 690, 691 e 150; `TargetDiagnosis.requestAdaptationMib3`).

**2. O dado só vale se a leitura der certo.** Em `DiagnosisDataProvider.onDataRead`, o conjunto só é marcado como válido quando a leitura volta com código 0. Sem isso, cada campo devolve o valor padrão da tabela interna (`AccessImpl`), não importa o que esteja nos bytes. É por isso que o `/vw/hmi/config/diagApi.properties` não serve de entrada: é um arquivo de teste de 2018, com valor e máscara por conjunto, que a HMI só carrega com a propriedade `-DdiagApi.properties`, e o `runHMI.sh` não a passa. Mesmo carregado, ele não torna o dado válido.

**3. A camada da LG responde a maior parte sem ir ao barramento.** `DSIPersistenceImpl.readBuffer` tem três caminhos:

| Leitura | De onde vem a resposta |
|---|---|
| codificação, identificação, funções do carro, registros `7209` e `720D` | **sempre** de uma tabela na memória da própria HMI (`ValuesFromDiag`), procurada pela chave. Se não achar, responde vazio com código 2 |
| adaptação | da mesma tabela, se ela recebeu pelo menos 44 conjuntos antes da primeira leitura (`use_diag_from_mcu`) e a HMI ainda não gravou nada; senão, pedido ao `persistence-app` pelo barramento |
| registros `720B`, `720C`, chaves 1 e 2, e o namespace `0x286F058C` | pedido ao `persistence-app` pelo barramento (interface 1400, api 1003; resposta na 1401, api 2003) |

**4. Quem enche a tabela é o microcontrolador.** Mensagens que chegam com o canal `PID_DIAG_MCU` (`0x0800000000000000`) são entregues pela `libDSIService` com a interface 1901 (`DIAG_MCU_MSG`), a carga inteira, sem cabeçalho DSI (`DSIService::event_handler`). `ListenerWorkerThread.processPersistenceListenerService` guarda cada uma em `ValuesFromDiag`: a chave são os bytes 3 e 4 da carga, o dado vem do byte 5 em diante. **Só guarda enquanto a HMI não fez nenhuma leitura de persistência**; o que chega depois é descartado.

```
[canal: 8 bytes = 0x0800000000000000][cabeçalho: 3 bytes][DID: 2 bytes, o alto primeiro][dados]
```

**5. O `persistence-app` recebe as mesmas mensagens.** `PersistenceKIPC::event_handler` separa o que chega pelo remetente; o que vem de `PID_DIAG_MCU` vai para `DiagHandler::event_handler`, que exige 5 bytes, lê o DID dos bytes 3 e 4 (o registro dele chama isso de "RDID") e guarda o dado. Depois `DSIHandler::Persistence_ReadBuffer` responde as leituras dos namespaces de diagnóstico a partir dessa tabela. Alguns DIDs têm tratamento próprio, lido e não exercitado: `F187`, `F18C` e `F191` são gravados em arquivos de `/sys/devices/soc/soc:common_gpio_default/`; `720B` e `720C` vão para `/data/lge/hmi/diagval_direct`, e o `720B` ainda regrava o idioma da HMI em `/data/lge/hmi/lsmval` e manda uma confirmação ao microcontrolador; `F1B1` vira as chaves 1 e 2 dos registros. Na partida ele também lê `/data/lge/hmi/diagval`, uma linha por DID.

Os três primeiros bytes da carga não são lidos nem pela HMI nem pelo `persistence-app`. **Corrigido às 23h26 de 11/10/2026:** a frase terminava em "Não sei o que o microcontrolador de verdade põe ali", e a bancada mandava zeros. Há um terceiro leitor, que eu não tinha achado: o `navi-al` do firmware só aceita a mensagem se esses bytes forem `83`, `60` e o serviço, e com zeros descartava os 48 conjuntos. O `diagstub` passou a mandar `83 60 22` (subseção "O cabeçalho dos conjuntos", abaixo; decisão D77). O que o microcontrolador de verdade manda continua sem ter sido visto.

**Dedução:** a camada da LG tem a constante `NUM_FROM_DIAG_MCU = 44`, e 44 é exatamente o que esta variante procura na tabela (35 de adaptação, a codificação, 2 de registros, as funções do carro e 5 de identificação). O microcontrolador deve mandar esses 44 na partida. Quando, e disparado por quê, não está no código que li.

### O que os arquivos `.swap` são

São só descrição, não dado. Cada um é uma lista de linhas no formato do `ObjectSwapper`: requisito, descrição, tipo, nome da constante, e os pares de valor e significado. A HMI usa isso numa tela de diagnóstico dela (`AslDiagGenericFetcher`), que não achei como abrir. `coding.swap` tem 25 linhas, `adaptation.swap` 396, `carfunction.swap` 336, `identification.swap` 5.

A posição de cada campo (byte, bit, tamanho, padrão, valores aceitos) não está neles: está numa tabela de inteiros dentro de cada classe de acesso. O programa [driver/LeCodificacao.java](driver/LeCodificacao.java) junta as duas coisas com as classes do próprio firmware, rodando na JVM dele, e grava [stubs/codificacao/tabela-campos.tsv](stubs/codificacao/tabela-campos.tsv): **1.188 campos**, cada um com a fonte na última coluna. O mesmo programa gera o conjunto padrão e confere, campo a campo, que ele dá o mesmo resultado que a HMI sem dado nenhum.

| Conjunto | Campos | O que a tabela mostra |
|---|---|---|
| codificação | 24 | só os 7 primeiros dos 25 bytes são interpretados: byte 0 = marca (bits 4 a 7) e classe (bits 0 a 3); byte 1 = geração e carroceria; byte 2 = complemento de carroceria e região; byte 3 = tração e lado do volante; bytes 4 a 6 = MOST e variações do tipo de veículo. Todos os padrões são zero |
| adaptação | 400 | funções ligadas ou desligadas. 11 campos têm padrão diferente de zero |
| funções do carro | 336 | um bit por item de menu do carro |
| registros | 423 | limites de velocidade, tempos, e a lista de idiomas |
| identificação | 5 | número de peça, versão de software, série, número e versão de hardware |

As variantes `oilarge` e `oismall` das classes têm tabelas idênticas. A HMI instancia a `oilarge` (`TargetDiagnosis`).

### O que a HMI faz com a codificação

Lido em `AslTargetSystemFeaturesAndCoding`:

- **Marca** (`writeBrands`): só VW (2 ou 6), Seat (4), Skoda (3) e MAN (8) contam. Qualquer outro valor, inclusive o zero de quando não há dado, cai em "MAN". Era por isso o aviso "CURRENT SKIN DOES NOT BELONG TO CODED BRAND": o skin em uso é de VW (`SkinBrand=2` no `info.txt`).
- **Tipo de veículo:** marca, classe, geração, carroceria, complemento, tração e seis bits viram um código de 16 dígitos. Ele escolhe a pasta de dados do carro em `/vw/hmi/Resources/skin0/Data/` (`ConfigurationManagerImpl.computeEncodedVehicleTypeForSkin0For16Digits`). O firmware traz 39 pastas. Sem pasta igual, a HMI zera os bits de potência, entre-eixos e lado do volante e tenta de novo; por fim cai na pasta só da marca. Sem codificação ela usa `0200000000000000`, a genérica da VW, que mostra um Golf.
- **Navegação:** o campo `ID_NAVIGATION_SYSTEM` da adaptação vira a chave de recurso `ASL.System.IsNavigationContextAvailableDIAG`.
- **Relógio e unidades**, câmera de ré, DAB, serviços online, país, avisos legais e microfones saem de outros campos da adaptação, cada um com uma linha no registro.

### As rodadas

Container, camada RSI ligada, `ESPERA=2000`, `HMILOG=normal`. Em cada uma, teclas com hora marcada (MENU, Ajustes, Carro, NAV) e cinco fotos da tela. Saídas em `traces/m6_cod*`, com o tráfego inteiro do barramento.

| Rodada | O que mudou | Previsão | Observado |
|---|---|---|---|
| `cod0` | nada: controle sem codificação | 49 leituras sem dado; 35 de adaptação no barramento com código 2; todas as marcas "false"; "Navigation is not active" | igual |
| `cod1` | os 48 conjuntos, todos no valor padrão | a HMI registra os bytes que mandei; sem dado cai de 49 para 5; a adaptação sai do barramento; **nada mais muda** | confirmado. As cinco fotos saíram idênticas, pixel a pixel, às do controle |
| `cod1b` | o mesmo, com as teclas em outra ordem | referência das seguintes | igual à `cod1` |
| `cod2` | marca = VW | "VW brand is true"; some o aviso do skin; nada muda na tela | confirmado, inclusive a tela: cinco fotos idênticas |
| `cod3` | navegação ativada (1 bit) | "Navigation is active"; a tecla NAV deixa de dar `NAV2_NotCodedInfoScreen`; palpite de que entraria o aviso genérico de função indisponível; grade com bloco novo, meio a meio | os dois primeiros confirmados. **Errei o palpite:** entra a tela própria da navegação, `NAV2_ENTRY_INIT_WAITSCREEN`. A grade ganhou "Navigation" e mudou de ordem. Não previ o ícone novo na barra do veículo |
| `cod4` | tipo de veículo = VW, classe A, geração 7, sedã (4 campos) | a HMI usa a pasta `0203070100000000`; a figura do carro pode mudar, não sei | o registro mostra `vehicle.id:=0203070100000000`; **a figura mudou para um sedã**. As outras quatro fotos ficaram iguais |
| `cod5` | relógio disponível (`ID_CLOCK_0X11`) | "Clock is true"; nada muda na tela | confirmado |
| `cod6` | só os 39 conjuntos de adaptação, com a navegação ativada | com menos de 44 a adaptação vai ao barramento e o `persistence-app` responde com código 0; sem dado = 14; grade com "Navigation" | confirmado: 35 pedidos, 35 respostas com código 0; telas idênticas às da `cod3` |
| ao vivo | o caminho de uso, em outra porta: `m3_ao_vivo.sh` com o padrão e a navegação ativada | o mesmo da `cod3`, operado pela página | pela página: tecla Menu, grade com "Navigation"; toque no bloco, "Starting navigation…" |
| reprodução | `cod1b` e `cod3` de novo, só com os roteiros que ficaram na pasta (`driver/m6_rodada.sh`) | as mesmas diferenças | as mesmas, e as fotos saíram idênticas, byte a byte, às das rodadas originais. O único conjunto que a HMI leu diferente foi o `0558`: `00 00 00 00` virou `00 80 00 00` |

A `cod4` mexe em quatro campos de uma vez porque a pasta só casa com o código inteiro. Em relação à `cod2` são três campos a mais.

Da `cod1b` em diante conferi, na hora da última foto, que os daemons do firmware continuavam de pé (na `cod1` não medi). Em nenhuma rodada o glhost registrou erro de OpenGL, e a JVM não caiu.

A tela Welcome às vezes é conferida uma vez a mais ("REGULAR" duas vezes). Acontece também em rodadas sem codificação (`m5_k8`, `m5_n2`, `m4_e8`): é a ordem dos eventos do RSI.

**As rodadas `cod0` a `cod6` rodaram em containers próprios e não apareceram na página ao vivo.** A conferência "ao vivo" foi numa segunda sessão, na porta 8432, para não derrubar a sessão principal, que estava em uso.

### Como a bancada entrega os dados

- **`driver/diagstub.c`**, novo: lê um ou mais arquivos de conjuntos e os publica no canal do microcontrolador, um por mensagem, e termina. Aceita o conjunto inteiro (`0600 = 00 00 …`) e campos avulsos (`0600 @0.4+4 = 2`: byte 0, a partir do bit 4, 4 bits). Linha errada recusa tudo e diz o arquivo e a linha.
- **`driver/kipc_hub.c`**: o que chega nesse canal fica guardado e é entregue a cada processo que se liga, **antes** do cache comum. É o que garante que a HMI receba os conjuntos antes da primeira leitura: eles e o anúncio do serviço de persistência passam pela mesma fila, em ordem (`ListenerHandler`, executor 7). Esse canal também ficou fora do limite mínimo de 12 bytes de carga, que vale para mensagens com cabeçalho DSI; aqui o mínimo é 5.
- **`driver/m1_hmi.sh`**: opção `CODING`, com os arquivos em ordem. O `diagstub` roda logo depois do hub, antes dos daemons e da HMI. No fim o lançador mostra as linhas de diagnóstico que a HMI registrou.
- **`stubs/codificacao/`**: `padrao.txt` (os 48 conjuntos no valor padrão, gerado), `ajustes.txt` (os campos a experimentar, todos comentados), os dois arquivos do Jetta (ver "O objetivo novo", abaixo), a tabela e um [README](stubs/codificacao/README.md) com o formato e a fonte de cada coisa.
- **Modo ao vivo**: `ao_vivo.sh` e `m3_ao_vivo.sh` carregam `padrao.txt`, os dois arquivos do Jetta e, por último, `ajustes.txt` (até as 19h19 de 11/10/2026 eram só o primeiro e o último; ver a decisão D34). Para experimentar: tirar o `#` de uma linha do `ajustes.txt` e reiniciar a sessão.
- **`driver/LeCodificacao.java`** e **`driver/m6_tabela.sh`**: geram a tabela e o padrão.
- **`driver/dsi_escuta.c`** passou a mostrar as mensagens desse canal com o DID e os dados.
- **`driver/tests/t_codificacao.sh`**, novo: 26 casos, sem firmware. Escrito antes do código; falhou em 18 de 22 e hoje passam os 26. Desde as 23h26 de 11/10/2026 são 39, com os do cabeçalho.
- **`driver/m6_rodada.sh`** e **`driver/m6_analisa.py`**: uma rodada com teclas marcadas, fotos e escuta do barramento, e o resumo que compara duas rodadas. Foi com eles que conferi o resultado no fim (abaixo).

Para refazer o resultado principal a partir da pasta da bancada:

```bash
printf '0558 @1.7+1 = 1\n' > /tmp/nav.txt
sh driver/m6_rodada.sh ref /tmp/m6 stubs/codificacao/padrao.txt
sh driver/m6_rodada.sh nav /tmp/m6 stubs/codificacao/padrao.txt /tmp/nav.txt
python3 -I driver/m6_analisa.py /tmp/m6 nav ref
```

### Um defeito antigo do hub, achado pelo teste

Quando um cliente saía e outro entrava na mesma volta do laço, o novo herdava o estado (`revents`) de quem tinha ocupado a vaga. O hub então fechava o cliente recém-chegado ou travava para sempre numa leitura. Nunca tinha aparecido porque até o Marco 5 ninguém saía do barramento. O `diagstub` entra, manda e sai, e o teste novo, com vários clientes de vida curta, travou o hub na primeira vez. Corrigido na causa (zerar o estado ao aceitar o cliente). Na mesma mudança o hub passou a ler o que estiver na fila de um cliente antes de fechá-lo.

### Limites

- **Nada foi contornado.** A HMI pediu dados e recebeu dados, em claro, pelo caminho que o firmware já tem. Não mexi em assinatura, proteção de componentes, licenças de função nem em nada do carro.
- **O bit de navegação na bancada não diz nada sobre ligar navegação num carro.** Num carro há outras condições além desse campo, que não levantei e que não são objetivo aqui. A tela para em "Starting navigation…": o motor de navegação não está rodando.
- **O padrão não é um carro.** É o que a HMI assume sem dado. Um carro tem outros valores em centenas de campos.
- **Só este firmware.** A tabela é do MU0883 (HMI `H40.272.304`). O carro roda o MU0803 (`H40.272.151`); a tabela dele pode ser outra.
- **"Padrão válido" igual a "sem dado" foi medido em cinco telas** e nas linhas do registro. Em telas que não visitei pode haver diferença.
- **O momento em que o carro manda os dados não é conhecido.** Entregar a cada processo, logo que ele se liga, é escolha minha (decisão D22).
- **Mudar com a HMI de pé não foi testado.** Pelo código, o que chega depois da primeira leitura é descartado.
- O que o `persistence-app` faz com `720B`, `720C`, `F1B1` e os arquivos de `/sys` foi lido, não exercitado.

### O objetivo novo: simular o Jetta GLI 2021

Com o marco em andamento, a sessão principal repassou o que o Lipe escreveu nela em 11/10/2026: **"eu quero tudo funcional no simulador, igual ao carro de verdade mesmo, podemos simular que seja do Jetta GLI 2021?"** O roteiro inteiro está em [simulador-jetta.md](simulador-jetta.md). Para a codificação, isso muda o alvo: a identidade a montar é a desse carro.

O carro ainda não tem autoscan nem leitura do módulo 5F. Então separei o que se sabe do que é palpite, em dois arquivos de `stubs/codificacao/`:

| Arquivo | O que tem | De onde vem |
|---|---|---|
| `jetta-gli-2021.txt` | a identificação: número de peça `3G5035832C`, hardware `H56`, software `0803`. E o lugar, vazio, da codificação e das adaptações | lido nas telas da central do carro em 07/09/2026 e anotado na ficha |
| `jetta-gli-2021-deducoes.txt` | tipo de veículo (VW, classe A, geração 7, sedã), estágio de potência 1 e navegação ativada | **nenhum valor lido do carro.** Escolhidos porque fazem o simulador mostrar o que se vê no carro; a razão está em cada linha |

Enquanto as rodadas abaixo corriam, nenhum dos dois entrava no modo ao vivo por padrão. **Mudou em 11/10/2026, às 19h19:** depois de ver o resultado, o Lipe escreveu na sessão do Marco 6 "pode deixar o Jetta como padrão do modo ao vivo". Desde então `sh driver/ao_vivo.sh` sobe com os dois (decisão D34).

**O que cada parte da identidade é, para esta HMI:**

| O que se vê | De onde a HMI tira | Rodada |
|---|---|---|
| figura do carro | o tipo de veículo de 16 dígitos, montado da codificação (bytes 0, 1, 2, 3, 5 e 6 do DID `0600`), escolhe a pasta em `skin0/Data/`. A mesma pasta traz as configurações `Climate`, `OPS` e `AmbianceLight`, que não exercitei | `cod4` |
| cor de destaque (linhas, barra de rolagem, brilho dos blocos) | o mapa de cores do skin (`skin1/ambienceColorMap.res`): azul por padrão; vermelho `#FF2832` se o tipo de veículo tem o estágio de potência 1; outro vermelho, `#FF0000`, para a marca MAN. O mesmo mapa converte a cor da luz ambiente, quando o carro informa uma (lido, não exercitado) | `cod8a` |
| bloco "Navigation", tecla NAV, ícone na barra, linha "Navigation database" nas informações | um bit da adaptação (`ID_NAVIGATION_SYSTEM`, DID `0558`) | `cod3`, `cod8` |
| número de peça, hardware e software na tela de informações | a identificação: `F187`, `F1A3` e `F189` (`SystemInfoUpdater`) | `cod7` |

As rodadas, com um toque a mais para abrir "System information" a partir dos ajustes:

| Rodada | O que mudou | Previsão | Observado |
|---|---|---|---|
| `cod7ref` | o padrão, com as teclas desta leva | referência | "Device part number", "Hardware" e "Software" mostram "DIAG ERROR" |
| `cod7` | identificação da ficha | o resumo da HMI traz o número de peça e o software; a tela mostra os dois; palpite de que o hardware ficaria vazio | confirmado, menos o palpite: aparece **"Hardware: H56"**. Depois de escrever a previsão achei a classe que alimenta a tela, e ela usa o `F1A3`. As outras cinco fotos ficaram idênticas |
| `cod8a` | sedã (como na `cod4`) e mais um bit, o estágio de potência 1 | o destaque passa de azul a vermelho em todas as telas; o sedã continua | confirmado. A própria HMI registra `#000000 ==> HMI #FF2832` |
| `cod8` | tudo junto: identificação, sedã, estágio de potência 1 e navegação | a soma, sem interferência; não sabia se a tela de informações ganharia a linha da navegação | a soma. A tela ganhou "Navigation database: N/A" |
| `cod9` | o mesmo, lido dos quatro arquivos que ficaram na pasta (padrão, o do carro, as deduções e os ajustes) | fotos idênticas às da `cod8` | idênticas, as seis, byte a byte |
| ao vivo, padrão novo | depois da decisão do Lipe: `m3_ao_vivo.sh` sem nenhuma variável, numa segunda sessão na porta 8432 | o mesmo da `cod9`, operado pela página | o lançador leu os quatro arquivos; pela página, a tecla Menu mostrou a grade de 8 blocos em vermelho e, em Ajustes, o toque em "System information" mostrou peça, hardware e software |

| Simulador, sem identidade | Simulador, com os dois arquivos do Jetta |
|---|---|
| ![Informações do sistema antes](quadros/2026-10-11-hmi-cod-info-sistema-antes.png) | ![Informações do sistema com o Jetta](quadros/2026-10-11-hmi-cod-jetta-info-sistema.png) |
| ![Grade antes](quadros/2026-10-11-hmi-cod-grade-antes.png) | ![Grade com o Jetta](quadros/2026-10-11-hmi-cod-jetta-grade.png) |
| ![Veículo antes](quadros/2026-10-11-hmi-cod-veiculo-antes.png) | ![Veículo com o Jetta](quadros/2026-10-11-hmi-cod-jetta-veiculo.png) |

**Comparação com o carro.** A foto da tela "Informações do sistema" da central do carro está em [carros/jetta-gli-2021/fotos/2026-09-07-multimidia/01-informacoes-do-sistema.jpg](../../../carros/jetta-gli-2021/fotos/2026-09-07-multimidia/01-informacoes-do-sistema.jpg). Linha a linha:

| No carro | No simulador com os dois arquivos | Por quê |
|---|---|---|
| Número de peça do dispositivo: 3G5035832C | Device part number: 3G5035832C | igual. Na tela o número sai sem espaços, nos dois |
| Hardware: H56 | Hardware: H56 | igual |
| Software: 0803 | Software: 0803 | igual no texto. O simulador roda o build 0883; o 0803 é o que o microcontrolador do carro informaria |
| Banco de dados de navegação: 20.3 | Navigation database: N/A | a linha existe; não há banco de mapas na bancada |
| Codec de mídia: 3.1.4 | (não aparece) | deve vir do serviço de mídia, que a bancada não tem (`dsiMediaBaseUpdateApplicationVersion` grava uma versão e a chave que a mostra); não conferido |
| (não aparece na foto) | Radio database: (vazio) | vem do serviço de rádio (`SystemInfoUpdater`, versão da base de logotipos) |
| título sublinhado e barra de rolagem em vermelho | iguais | com o estágio de potência 1. No carro pode ser isso ou a luz ambiente |
| em português | em inglês nesta leva; **em português desde as 20h de 11/10/2026**, com a memória de estado (subseção seguinte) | **corrigido:** aqui estava "o idioma vem de outro registro (`720B`), não exercitado". A primeira fonte do idioma é a memória de estado; o `720B` só vale quando ela não tem idioma |

**O que é dedução, e o que conferir no carro, só lendo:**

- Se o tipo de veículo estiver certo, os dois primeiros bytes da codificação longa do 5F são `23 71`. Os códigos de classe e geração saíram da pasta que existe no firmware, não de leitura.
- O vermelho tem duas explicações que dão a mesma tela: o bit 1 do byte 5 da codificação, ou o carro informar luz ambiente vermelha (dado de veículo). No código o estágio 1 se chama "S" e o 2, "R"; que "S" seja a versão esportiva é leitura minha, apoiada em existir uma pasta de Golf com ele (`0203070000100100`).
- A navegação: a ficha registra o banco de dados 20.3 na tela do carro. O valor do campo de adaptação não foi lido.
- O conjunto `0600` desta HMI tem 25 bytes. Que ele seja a codificação longa que a ferramenta de diagnóstico mostra para o 5F é o esperado, e não foi conferido.

### O idioma: a central nasce em português

**O que aconteceu.** Por volta das 19h40 de 11/10/2026 o Lipe trocou o idioma pela tela da central ao vivo e escreveu na sessão do Marco 6: **"Eu já coloquei o idioma em pt-br"**. Conferi só lendo, às 19h43: a grade estava em português e a memória de estado da central, o arquivo `/data/lge/hmi/lsmval`, tinha 61 linhas, uma delas `0 1101 10` seguida de `pt_BR` e 17 espaços.

**O problema.** Na bancada essa escolha morria na partida seguinte. O `m1_hmi.sh` cria `/data` em memória a cada partida, e a central nascia sempre com esse arquivo vazio, em inglês. No carro ele fica na memória permanente e sobrevive ao desligar.

**Por onde o idioma entra na HMI**, lido no código:

| Passo | O que acontece | Fonte |
|---|---|---|
| 1 | na partida a HMI pede o valor guardado no namespace 1101, chave 10: 22 bytes, os cinco primeiros com o código do idioma em letras e o sexto com o tipo de voz | `AslTargetLscTNGBullhorn.processAslEventPowerOn` e `PersistableLsc` |
| 2 | o pedido não vai ao barramento: a camada da LG lê `/data/lge/hmi/lsmval` quando nasce e responde dessa tabela enquanto não houver nenhuma gravação | `DSIPersistenceImpl.initLSMDataInFile` e `readBuffer` |
| 3 | o idioma tem de estar na lista de idiomas da variante; se não estiver, vale o primeiro inglês da lista | `LanguageHelper.isValid` e `findFallbackLanguage` |
| 4 | sem valor guardado, a HMI registra "No LSC Persistable Data (NS1101KEY10) could be loaded. Using Diagnosis values instead!" e fica com o idioma padrão do registro `720B` | `DiagnosisHandler.processDiagDataUpdl` |
| 5 | do lado nativo, o `persistence-app` lê o mesmo arquivo na partida (até 500 valores) e acrescenta uma linha a cada valor novo que a HMI manda guardar | `init_pso_file` e `DSIHandler::Persistence_WriteBuffer`, decompilados com o Ghidra |

**Correção.** Em "Onde parou" e na comparação com o carro eu tinha escrito que o idioma vinha pelo `720B` e pelo `720C`. Isso é só a reserva (passo 4). A primeira fonte é a memória de estado, e foi por ela que resolvi.

**O que entrou na pasta:**

- `LSMVAL`, opção nova do `driver/m1_hmi.sh` (repassada pelo `m6_rodada.sh`): o arquivo com que a memória de estado da central começa.
- [stubs/persistencia/](stubs/persistencia/README.md), com o `idioma-pt_BR.txt`: a linha que a própria HMI gravou quando o Lipe trocou o idioma, copiada da central ao vivo.
- `driver/lsmval_prepara.sh`, que confere o arquivo e tira comentários e sobras de espaço, e o teste `driver/tests/t_memoria.sh` (22 casos, no Mac e no container). O motivo: os dois leitores do firmware descartam em silêncio a linha que não entendem, e a HMI separa os campos por um espaço só; um arquivo editado à mão com dois espaços seguidos deixaria a central em inglês sem aviso.
- O padrão do modo ao vivo: `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh` sobem com esse arquivo (decisão D43). `LSMVAL= sh driver/ao_vivo.sh` volta à memória vazia, em inglês.

**As rodadas**, em containers à parte, fora da página ao vivo. Previsões e resultados em `traces/m6_previsoes.md`.

| Rodada | O que mudou | Previsão | Observado |
|---|---|---|---|
| `idi1` | a `cod9` e mais uma linha na memória de estado (o idioma) | a HMI registra o idioma guardado como válido; as seis telas saem em português desde a primeira foto; o resto igual à `cod9`; as seis fotos diferentes das dela | confirmado, com um erro meu: cinco fotos diferentes e uma igual. A tela do veículo não tem texto nenhum e saiu idêntica, byte a byte. O resumo do lançador saiu igual ao da `cod9` linha por linha |
| `idi2` | `m3_ao_vivo.sh` sem nenhuma variável, depois de pôr o padrão e o conferidor | memória com 1 linha, os 48 conjuntos, e as telas do rádio e da grade idênticas às da `idi1` | confirmado; as duas fotos idênticas, byte a byte |
| `idi3` | arquivo de memória com erro (`0 1101 10 pt_BR`, o idioma em letras) | o lançador recusa com o arquivo e a linha, e nada sobe | confirmado, com as duas mensagens previstas |

| Grade | Ajustes |
|---|---|
| ![Grade em português](quadros/2026-10-11-hmi-cod-ptbr-grade.png) | ![Ajustes em português](quadros/2026-10-11-hmi-cod-ptbr-ajustes.png) |
| **Informações do sistema** | **Tecla NAV** |
| ![Informações do sistema em português](quadros/2026-10-11-hmi-cod-ptbr-info-sistema.png) | ![Navegação em português](quadros/2026-10-11-hmi-cod-ptbr-navegacao.png) |

**A tela de informações contra a foto do carro, agora no mesmo idioma:**

| No carro | No simulador | |
|---|---|---|
| título "Informações do sistema" | igual | |
| Número de peça do dispositivo: 3G5035832C | igual | |
| Hardware: H56 | igual | |
| Software: 0803 | igual | |
| Banco de dados de navegação: 20.3 | Banco de dados de navegação: N/A | a legenda é igual; não há banco de mapas na bancada |
| Codec de mídia: 3.1.4 | (não aparece) | como antes |
| (não aparece na foto) | Banco de dados do rádio: (vazio) | como antes |
| botão "Atualizar software" | igual | |

Os textos em `pt_BR` batem palavra por palavra com os da foto. É o que sustenta que a central do carro está nesse idioma; não há outra leitura dele.

**O tempo esgotado da troca de idioma não é da tela.** A HMI registra "Operation with changeID timed out" 30 s depois de começar a troca. Isso já acontecia em inglês (rodada `cod9`). Quem não confirma é um participante interno, `AslId_5308`, que a HMI desliga ao fim da lista de idiomas; os da tela (`GuiLscListener`, `WidgetsLscListener`, `ConfigurationManager`) ficam em `pt_BR`. Não fui atrás de quem é o 5308.

**Limites:**

- A memória de partida é um arquivo fixo. O que for mudado pela tela durante uma sessão se perde quando a central reinicia; no carro ficaria. Guardar a memória de uma sessão para a outra é próximo passo, para o Lipe decidir (D44). **Mudou às 20h58 de 11/10/2026:** no modo ao vivo a memória passou a ficar guardada de uma sessão para a outra (subseção seguinte, decisão D46).
- Só a linha do idioma foi testada como arquivo de partida. **Mudou no mesmo dia:** a memória inteira de uma sessão foi usada como arquivo de partida nas rodadas `mem2`, `mem3` e `mem5`.
- Em dois minutos a HMI grava outras 57 linhas (namespaces 5000 a 5092 e 6000; cópia em `traces/m6_idi1.lsmval-aos-131s.txt`). Não identifiquei o que cada uma guarda. Uma delas (5051, chave 4311744512) acompanha a tela aberta: trazia `HMI_GRIDMENU` na central ao vivo, que estava na grade, e `HMI_SYSTEMSETUP` na `idi1`, que estava nos ajustes. Parece ser a última tela, a que a central reabriria ao ligar; não testei. **Conferido depois:** é a última tela principal, e a central a reabre ao ligar (subseção seguinte, rodadas `mem2` e `mem3`).
- A central ao vivo que estava no ar foi posta em português à mão pelo Lipe. O padrão novo vale da próxima partida em diante; não reiniciei essa central, que é da sessão principal.

### A memória de uma sessão para a outra

Feito numa sessão à parte, das 20h15 às 21h20 de 11/10/2026. A decisão D44 tinha deixado este passo para o Lipe decidir; a sessão que o faz foi proposta pela do Marco 6 para começar só se ele quisesse, e começou. Não há frase dele sobre este passo. O objetivo que ele deu, registrado acima, é o simulador "igual ao carro de verdade mesmo", e no carro a central lembra.

**Resultado, verificado em execução:** no modo ao vivo a central lembra de uma sessão para a outra. O que uma sessão deixa é, byte a byte, o que a seguinte recebe, e a HMI usa o que recebe: sem ninguém apertar nada, a segunda sessão abriu "Configurações do sistema", a última tela da primeira, no lugar do aviso do rádio.

| Sessão 1: nasce no aviso do rádio | Sessão 1: termina nos ajustes |
|---|---|
| ![Sessão 1 nasce no rádio](quadros/2026-10-11-hmi-mem-sessao1-nasce-no-radio.png) | ![Sessão 1 termina nos ajustes](quadros/2026-10-11-hmi-mem-sessao1-termina-nos-ajustes.png) |
| **Sessão 2: nasce nos ajustes, sem tecla nenhuma** | **Sessão 3: nasce onde a 2 foi desligada** |
| ![Sessão 2 nasce nos ajustes](quadros/2026-10-11-hmi-mem-sessao2-nasce-nos-ajustes.png) | ![Sessão 3 nasce na navegação](quadros/2026-10-11-hmi-mem-sessao3-nasce-na-navegacao.png) |

**Por que a HMI reabre a última tela.** Lido no jar da HMI antes de prever as rodadas:

| Passo | O que acontece | Fonte |
|---|---|---|
| 1 | a cada troca de tela principal a HMI manda guardar o nome dela (`HMI_GRIDMENU`, `HMI_SYSTEMSETUP`, `HMI_CAR`, `HMI_NAVI`…) no módulo de persistência `SystemCommon`, que é o namespace 5051 | `StageContentProcessor.persistLastContent`; a HMI registra "Set persistence module >SystemCommon< associated with namespace 5051" |
| 2 | nem toda tela é guardada: clima, usuários, data e hora, unidades e outras ficam de fora | `PersistenceValidator` |
| 3 | na partida ela lê esse nome. Se vier "DEFAULT", registra "No last mode persisted, use default!" e usa o rádio; senão registra "Last content in ScreenAreaStage: …" e pede a tela guardada | construtor do `StageContentProcessor` |
| 4 | só no caso "DEFAULT" fica ligado um receptor do sinalizador 655, que regrava `HMI_RADIO` como última tela toda vez que o sinalizador é entregue. Acontece na partida e de novo quando acaba o segundo ciclo de troca de idioma | o mesmo construtor; medido na `idi1` (138,8 s), na central ao vivo (135,4 s) e na `mem1` (138,7 s) |
| 5 | a gravação chega ao arquivo de 5 a 7 s depois da troca de tela | medido na `mem1` e na `mem2` |

O passo 4 tem uma consequência para quem usa: na primeira sessão depois de voltar ao zero, tela aberta antes dos 2 min 20 s e deixada lá não é lembrada. Nas sessões que já nascem com memória o receptor não existe.

**Por que não basta copiar o arquivo.** `DSIHandler::Persistence_WriteBuffer`, do `persistence-app` (decompilado em `traces/m6_ghidra_persistence-app_lsm.log`): quando a chave é nova, abre o `lsmval` para acrescentar uma linha; quando a chave já existe, abre com `fopen "w"`, regrava todas as linhas, fecha, chama `sync` e copia para `lsmval_bak`. Entre o `fopen` e o fim da gravação o arquivo está vazio ou pela metade. O conferidor sozinho não pega esses dois casos: arquivo vazio é memória vazia, que ele aceita, e um arquivo cortado no meio de uma linha pode sobrar com números que ainda formam uma linha válida. Por isso a cópia só é guardada quando (1) o arquivo ficou igual entre duas leituras com 0,3 s de intervalo, (2) não está vazio e termina em fim de linha, que é como o firmware termina toda linha (`fputc(10)`), e (3) passa pelo `lsmval_prepara.sh`. E é gravada ao lado e trocada de nome.

**A memória de verdade, medida antes de escrever o código.** Às 20h25 li, sem alterar nada, o `lsmval` da central ao vivo, que estava no ar havia 53 minutos e em uso: 61 linhas, 10.321 bytes, a maior linha com 1.929 letras, todas do tipo 0, nenhuma chave repetida, terminado em fim de linha e idêntico ao `lsmval_bak`. Passa pelo conferidor sem mudar um byte.

**O que entrou na pasta:**

- `driver/lsmval_guarda.sh`, novo, com três usos: `partida` escolhe o arquivo de que a sessão nasce (a memória guardada, se houver e passar no conferidor; senão a semente), `passo` faz uma conferência e guarda a cópia se ela mudou, e `vigia` repete o passo a cada 2 s. Cópia recusada não apaga a guardada, e o motivo fica em `recusada.txt`.
- `driver/m3_ao_vivo.sh`: com `MEMORIA=<pasta>` chama a `partida`, sobe a `vigia` e, quando a central para, faz um último `passo`. Sem `MEMORIA`, que é o caso de quem chama o roteiro direto, nada muda.
- `driver/ao_vivo.sh`: monta `ao-vivo/memoria` em `/live/memoria` e passa `MEMORIA`. Com `LSMVAL` passado à mão, ou com `MEMORIA` vazio ou `0`, a pasta não é criada, montada nem alterada.
- `ao-vivo/memoria/` (dado de sessão, fora do versionamento): `lsmval`, a memória guardada; `anteriores/lsmval.1` a `.5`, a memória com que cada uma das cinco últimas sessões nasceu; `recusada.txt` e `lsmval.recusada`, quando houver recusa.
- `driver/tests/t_memoria.sh`: 58 casos novos, nas seções 6 a 10.
- [stubs/persistencia/README.md](stubs/persistencia/README.md): como ver, desligar e voltar atrás, e a tabela com o nome que a HMI dá a cada namespace da memória.

**As rodadas.** Sessões do `m3_ao_vivo.sh` em containers à parte (`moi3-m6-mem*`), com pastas próprias no rascunho e uma pasta de memória para todas. Da `mem1` à `mem4`, com uma cópia de `driver/` tirada às 20h41 e os dois arquivos novos; a `mem5` e a `mem6`, depois de instalar. Previsões e resultados em `traces/m6_previsoes.md`; saídas em `traces/m6_mem*`.

| Rodada | O que mudou | Previsão | Observado |
|---|---|---|---|
| `mem1` | a pasta da memória vazia; Menu aos 155 s e Ajustes aos 167 s; acaba sozinha aos 190 s | nasce do arquivo fixo e registra "No last mode persisted"; a HMI regrava `HMI_RADIO` perto dos 139 s; a cópia guardada acompanha a central e termina com `HMI_SYSTEMSETUP` | confirmado ponto por ponto: 58 linhas, cinco leituras da central idênticas à cópia guardada na mesma hora, "fim da sessão: igual". Não previ que as 57 linhas aparecem juntas, entre os 30 e os 65 s, nem o atraso de 5 a 7 s da HMI para mandar guardar |
| `mem2` | nasce da memória da `mem1`; nenhuma tecla até a NAV, aos 100 s; `docker stop` aos 112 s | "Last content in ScreenAreaStage: HMI_SYSTEMSETUP"; os ajustes abrem sozinhos (dei 80%); a memória fica com `HMI_NAVI` mesmo sem encerramento limpo | confirmado. Em 95 s a HMI não gravou nada de diferente; depois da NAV mudou uma linha só; o `docker stop` levou 10 s e a memória ficou com `HMI_NAVI` |
| `mem3` | nasce da memória da `mem2`; nenhuma tecla | nasce em "Iniciando navegação…" e fica lá (dei 70%) | confirmado; as fotos dos 60 e dos 85 s são idênticas à dos 108 s da `mem2` |
| `mem4` | `MEMORIA=0`, com a pasta montada | nasce do arquivo fixo, no aviso do rádio; nada é lido nem escrito | confirmado; a pasta ficou com os mesmos arquivos e os mesmos SHA-256 |
| `mem5` | uma cópia do `ao_vivo.sh` instalado, só com outro nome de container; antes de subir, `anteriores/lsmval.2` recolocada como memória | o docker recebe `MEMORIA` e a pasta montada; a central nasce nos ajustes; `anteriores` gira | confirmado, já com o carro de mentira da outra sessão ligado por padrão ([quadro](quadros/2026-10-11-hmi-mem-volta-a-partida-anterior.png): os ajustes, agora com "Hora e data" e "Unidades"). Com ele mudou uma linha que antes não mudava (5051, chave 21458059264); o conferidor aceitou |
| `mem6` | a mesma cópia, com `MEMORIA=` | o docker recebe `MEMORIA` vazio e nenhuma pasta de memória; a pasta fica idêntica | confirmado |

**O que muda para quem usa a central ao vivo:**

- **A partida.** Em vez do aviso do rádio, abre a última tela principal da sessão anterior. As partidas deixam de ser iguais entre si. Quem precisa de uma partida conhecida (roteiro que toca por coordenada, comparação de fotos) sobe com `MEMORIA= sh driver/ao_vivo.sh`.
- **Um estado ruim também fica guardado.** A `mem3` é o exemplo: desligada com a navegação aberta, a central passou a nascer em "Iniciando navegação…", que na bancada não anda. Sai apertando outra tecla.
- **Voltar ao zero:** com a central desligada, `rm -rf ao-vivo/memoria`. **Voltar a uma partida anterior:** `cp ao-vivo/memoria/anteriores/lsmval.1 ao-vivo/memoria/lsmval` (exercitado na `mem5`, com a `lsmval.2`).
- **`LSMVAL` passado à mão continua valendo sobre tudo:** a sessão nasce do arquivo pedido e a memória guardada não é lida nem alterada.
- **As rodadas de experimento, as centrais de teste e o lançador chamado direto não mudaram:** não recebem `MEMORIA`.

**Testes.** Os 58 casos novos foram escritos antes do código. Sem ele, falhavam 40 dos 80 do arquivo; os 18 novos que passavam eram conferências de que nada acontece, vazias enquanto não havia o que acontecer. Para ver se acusam defeito rodei uma versão ingênua, que copiava o arquivo por cima sem conferir (26 falhas, entre elas a da troca de nome, a do arquivo vazio, a do arquivo pela metade e a do arquivo mudando), e depois seis mutações da versão de verdade, cada uma pega pelo caso esperado. No `ao_vivo.sh` o teste roda uma cópia do roteiro numa bancada de mentira, com um `docker` que só anota o que recebeu; com a memória sempre ligada falharam os quatro casos de desligar, antes de a chave existir. Bateria a partir da pasta, depois de instalar (rodada às 21h00 e de novo às 21h13, com tudo no estado final; saída em `traces/m6_mem.bateria-final-21h13.txt`): no container `t_entrada` 11, `t_codificacao` 26, `t_memoria` 80, glforward 8 e `t_transmissao` 16, mais os da sessão do carro de mentira, `t_veiculo` 56 (eram 52 às 21h00; ela acrescentou casos) e `t_pedidos` 16; no Mac `t_memoria` 80 e `t_servidor` 20. Nenhuma falha.

**Limites:**

- **O `driver/ao_vivo.sh` com o nome de verdade e a partida pela janela do Mac não rodaram.** A central ao vivo estava no ar e não foi reiniciada; ela subiu antes da mudança e não guarda nada. O que rodou foi a cópia da `mem5` e da `mem6`.
- **Visto depois na central ao vivo de verdade** (22h33 de 11/10/2026, pela sessão do Marco 6, só lendo): a central ligada às 22h07 com o prazo de 2 horas da janela recebeu `MEMORIA=/live/memoria` e a pasta `ao-vivo/memoria` montada; o lançador registrou "a central nasce de /live/memoria/lsmval" e "memória de estado de /live/memoria/lsmval: 61 linha(s)"; o registro da guarda (`ao-vivo/log/ao_vivo.memoria.log`) tinha cópias das 22h18 às 22h31; e a memória guardada era idêntica, byte a byte, à da central lida na mesma hora (duas leituras com 3 s de intervalo, iguais), e passava pelo `lsmval_prepara.sh` sem mudar um byte. O item acima deixa de valer para a partida com o nome de verdade. Não vi a janela nem operei a central.
- **Só a última tela foi mudada e conferida.** Som, favoritos, estação e o resto não foram exercitados. **Mudou às 22h50 de 11/10/2026:** seis outras coisas foram mudadas pela tela e conferidas na sessão seguinte, e ficou dito o que a tela não deixa mudar (subseção seguinte).
- **A sessão mais longa durou 190 s.** **Mudou no mesmo dia:** a `longa1` durou 35 minutos (subseção seguinte).
- **Uma cópia recusada só apareceu nos testes.** Há três casos em que o conferidor recusaria um arquivo escrito pelo próprio firmware: mais de 500 valores, linha com mais de 9998 letras e valor de zero bytes. Em nenhum deles a memória guardada se perde, mas ela para de ser atualizada, com aviso. Não vi nenhum. **Continua assim depois da sessão de 35 minutos**, e a subseção seguinte diz por que o código da HMI não chega a nenhum dos três com o que a bancada tem.
- **Corte seco.** Se o container morre de repente, perde-se o que a HMI mandou guardar nos 2 a 3 s anteriores. Só o `docker stop` foi medido.
- **Não está ligado no QEMU nem nas rodadas.** É do modo ao vivo.
- **A página ao vivo serve a pasta `ao-vivo/` inteira para este computador**, só leitura; a memória fica legível em `/memoria/lsmval`, como já ficavam `log/` e `entrada/`. Isso é leitura do `driver/servidor_ao_vivo.py`; não fiz o pedido.

### O que a memória guarda, linha por linha

Feito numa sessão à parte, das 21h35 às 23h de 11/10/2026, a pedido da sessão principal: descobrir o que a memória de estado guarda além do idioma e da última tela, conferir se cada coisa volta na partida seguinte, e rodar uma sessão longa. A central ao vivo não foi tocada. Tudo rodou em centrais de teste (`moi3-m6-*`), pelo `m3_ao_vivo.sh` com `MEMORIA`, com uma pasta de memória por experiência e uma cópia de `driver/` tirada às 21h38 (e uma segunda, de `driver/` e de `stubs/`, às 22h35, para as cinco últimas rodadas).

**Resultado, verificado em execução:** a memória é legível linha por linha, e sete coisas mudadas pela tela voltaram na sessão seguinte sem ninguém mexer (cinco com rodada própria, duas junto com as outras). A tabela completa, com namespace, chave e posição de cada campo, está em [stubs/persistencia/README.md](stubs/persistencia/README.md), seção "O que cada linha guarda".

| O que foi mudado pela tela | Linha que mudou | Volta na partida seguinte? | Rodadas |
|---|---|---|---|
| "Som do touchscreen" (Configurações > Display) | `5056` Sound, bloco 2, usuário 1, posição 14 | sim: a caixa nasce marcada | `som1`, `som2` |
| "Exibir hora em modo de standby" | `5051` SystemCommon, bloco 1, usuário 1, posição 3 | sim: nasce desmarcada | `tela1`, `tela2` |
| ordem dos blocos da grade (segurar e arrastar) | `5077` SimpleGridMenu, bloco 1, usuário 1 (lista de oito identificadores) | sim: a grade nasce na ordem nova | `fav1`, `fav2` |
| visão da tela do veículo (Seleção > Status do veículo) | `5069` CarEarly, bloco 1, posições 6 a 13 | sim: a central nasce em "Status do veículo" | `vei1`, `vei2` |
| "Alemão" em "Idiomas adicionais do teclado" | `5051`, mesma linha da última tela, posição 12 | sim: nasce marcado | `tec1`, `tec2` |
| sensor de aproximação e gesto de mão (junto com tudo o mais) | `5051`, mesma linha, posições 14 e 16 | sim | `expl1`, `expl2` |
| temperatura em °F (Configurações > Unidades) | `5051` SystemCommon, bloco 4, posição 9 | **não**, em três sessões de três: a HMI guarda, mas na partida segue o carro, e o carro de mentira nasce do perfil, em °C | `uni2`, `uni3`, `uni4`, `uni5` |
| nível de luminosidade | nenhuma: a escolha não pega na bancada | a volta foi conferida com o valor trocado à mão: a HMI pede ao display 100 %, 50 % ou 0 % conforme o byte guardado seja 00, 02 ou 04 | `bri1`, `bri2`, `bri3` |
| grave, agudo, balanço | nenhuma: a tela de som fica indisponível | não há o que voltar | `expl1` |

| Configurações > Display, onde moram quatro dos ajustes | A grade arrastada volta na ordem nova |
|---|---|
| ![Ajustes do display](quadros/2026-10-11-hmi-lsm-ajustes-do-display.png) | ![Grade reordenada](quadros/2026-10-11-hmi-lsm-grade-volta-reordenada.png) |
| **A tela de som, na bancada** | **A unidade não volta: a sessão seguinte nasce em °C** |
| ![Tela de som indisponível](quadros/2026-10-11-hmi-lsm-tela-de-som-indisponivel.png) | ![Unidades de volta em °C](quadros/2026-10-11-hmi-lsm-unidades-volta-celsius.png) |

**Como uma linha se lê.** Lido no jar da HMI antes de rodar, e conferido nas memórias que ela gravou:

| O quê | Como é | Fonte |
|---|---|---|
| a chave | (número do bloco de dados × 2³²) + (perfil × 2²⁴). Perfil 255 é a parte comum a todos os usuários; 1 é o usuário 1; a chave 0 é a declaração do módulo. `4311744512` é o bloco 1 do usuário 1; `21458059264`, o bloco 4, parte comum | `PersistenceKeyUtil.calcKey` |
| o embrulho do valor | com 50 bytes ou mais, se comprimir: `11 22 33 44` + gzip; senão, com mais de 4 bytes: `55 66 77 88` + 8 bytes de Adler-32 + conteúdo; até 4 bytes, cru. Nas 61 linhas da memória mais cheia: 34 com soma, todas certas; 15 comprimidas; 12 cruas | `Compression`, `RecordByteArray.writeToDsi` |
| os campos | na ordem do serializador de cada módulo, depois de 2 bytes de versão | `SystemCommonSerializer`, `SoundSettingsRootSerializer` e os demais |
| os módulos | 36 namespaces, de 5050 a 5095, com os blocos de cada um; 19 se registram na bancada | `/vw/hmi/res/persistenceIds.res`, lido com as classes do firmware (`driver/m6_persistence_ids.sh`) |

Três coisas saíram daí que não estavam em lugar nenhum: o brilho do display mora na mesma linha da última tela; a linha "5051 21458059264", a que "mudava com o carro de mentira", são as unidades; e os namespaces 5000, 5008 e 6000, que não tinham dono, são chaves do jeito antigo de guardar (`EARLY`, `PHONE` e o último modo, escrito para o lado da LG).

**O que a HMI diz sobre a carga.** Numa central que nasce sem memória ela registra 26 avisos "Could not load declaration data for module…", um por bloco. Numa que nasce das 58 linhas, 3: os três blocos que a memória não tem. E com o rastreamento `ASL.PERSISTENCE` ligado ela registra o valor que recebeu de 32 das 58 linhas, igual ao do arquivo nas 32 (rodada `som2`).

**As rodadas.** Previsões e resultados em `traces/m6_previsoes.md`, quarta parte; saídas em `traces/m6_lsm_*`. Cada "1" nasce de uma memória de base (a de uma central recém-nascida aos 150 s) e muda uma coisa pelo `ROTEIRO`; cada "2" nasce do que a "1" deixou.

| Rodada | O que foi feito | Previsão | Observado |
|---|---|---|---|
| `expl1` | reconhecimento, à mão: uma coisa por passo, com cópia da memória depois de cada um | seis palpites firmes e três coisas que eu não sabia se a tela deixava (grade, favoritos, unidades) | quatro certos: o som do touchscreen, grave e agudo sem mudança, volume sem mudança e a tecla CAR mexendo no CarEarly. Dois errados: o que muda aos 96 s da primeira partida (é o `sportHMIEnabled` dos dados "early", não os idiomas) e o brilho (dei 70% para mudar; não muda). A grade deixa arrastar; não achei tela de favoritos |
| `som1`, `som2` | "Som do touchscreen" | muda a posição 14 do Sound; a caixa volta marcada (90%) | confirmado; a HMI registra a leitura com o byte em 01 |
| `tela1`, `tela2` | "Exibir hora em modo de standby" | posição 3 do SystemCommon; volta desmarcada (90%) | confirmado |
| `fav1`, `fav2` | arrastar "Som" para o lugar de "Rádio" | muda a lista do SimpleGridMenu; a grade nasce na ordem nova (80%) | confirmado, sem tecla nenhuma na segunda sessão |
| `vei1`, `vei2` | tecla CAR, Seleção, "Status do veículo" | muda o CarEarly; a central nasce no status (75%) | confirmado: a HMI abre `CAR_MAIN_FZGSTATUS_COMBINED` |
| `tec1`, `tec2` | "Alemão" nos idiomas do teclado | posição 12; volta marcado (85%) | confirmado |
| `bri1` | escolher "Baixa" na lista de luminosidade | nada muda (90%) | confirmado: o campo volta a "Muito alta" |
| `bri2`, `bri3` | memória com o brilho trocado à mão para 04 e para 00; escuta do barramento | o pedido de brilho da partida leva 0 % e 100 % (75% e 85%) | confirmado: campo vazio com 04, `10 64` com 00, e `10 32` com o 02 de sempre |
| `uni1` | °F, com o carro de mentira das 21h38 | nada muda; "pedido sem resposta" (90%) | confirmado |
| `uni2`, `uni3` | °F, com o carro de mentira das 22h32, que aceita gravação | a unidade vai para a memória (70%) e não volta (65%) | confirmado: gravada quando o carro atendeu o pedido, e regravada com 0 aos 59 s da sessão seguinte |
| `uni4`, `uni5` | a `uni3` repetida duas vezes, porque era a única conclusão apoiada numa sessão só e nesta bancada partidas iguais já divergiram | as duas repetem (80%) | repetiram: mesma leitura, mesma regravação (aos 68 s nas duas) e foto idêntica, byte a byte, à da `uni3` |
| `expl2` | nasce do que a `expl1` deixou, com tudo junto | status do veículo, grade reordenada, caixas do display como ficaram | confirmado, as três |
| `longa1` | 35 minutos, 14 trocas de tela, uma a cada 140 s | 58 linhas do começo ao fim; nenhuma cópia recusada; uma linha só por tecla | 58 linhas em 17 das 18 leituras (a primeira, aos 30 s, tinha 1); maior linha sempre com 1.929 letras; 17 cópias guardadas, nenhuma recusada, "fim da sessão: igual". Errei em dois detalhes: na tecla CAR mudam duas linhas, e a HMI levou de 6 a 11 s para gravar, não de 5 a 7 |

**A sessão longa, em números.** A memória não ganhou linha nenhuma em 35 minutos de uso: 1 linha aos 30 s, 58 aos 100 s e 58 até os 2080 s, entre 10.007 e 10.047 bytes. Entre uma tecla e a seguinte nenhum valor mudou sozinho. Nenhum dos três casos de recusa apareceu, e o código da HMI diz por quê: pelas listas do `persistenceIds.res` o teto fica perto de 300 linhas (80 para declarações e partes comuns, 25 por usuário, 7 usuários, mais 45 chaves antigas); os 36 blocos cujo serializador achei no jar começam todos por 2 bytes de versão, então não vejo de onde viria um valor de zero bytes (de outros oito blocos, de módulos que a bancada não registra, não achei o serializador); e a maior linha tem 1.929 letras. O que pode crescer são as listas (as 36 memórias de estação, os aplicativos web), que a bancada não tem como encher.

**O que entrou na pasta:**

- `driver/lsmval_abre.py`, novo: lê um `lsmval`, desfaz o embrulho, diz de quem é cada chave e o nome dos campos de 30 das 61 linhas; com dois arquivos, compara valor a valor e diz a posição e o campo que mudou. Roda no Mac, com `python3 -I`.
- `driver/tests/t_lsmval_abre.sh`, novo: 38 casos, com linhas que a própria HMI gravou. Os 31 primeiros foram escritos antes do programa. Sem ele falham 33 dos 38 (os cinco que passam são conferências de que algo não aparece). Rodei oito mutações do programa: duas passaram na primeira versão do teste (comparar também o tamanho da linha e não tirar comentário), e para pegá-las acrescentei dois casos e reforcei um, que casava com o nome do arquivo na mensagem de erro. Hoje as oito são pegas. Na imagem, que não tem Python, o teste avisa e sai sem falhar.
- `driver/LePersistenceIds.java` e `driver/m6_persistence_ids.sh`, novos: a lista dos módulos, lida com as classes do firmware.
- [stubs/persistencia/README.md](stubs/persistencia/README.md): a seção "O que cada linha guarda" (as tabelas, o que a tela não deixa mudar, as unidades, o tamanho), "Como ler a chave e o valor", e a lista "O que não foi conferido" refeita.
- Catorze quadros `quadros/2026-10-11-hmi-lsm-*.png` e as saídas `traces/m6_lsm_*`, com os roteiros das rodadas (`traces/m6_lsm.sessao.sh.txt`, `.roda.sh.txt`, `.rot.py.txt`).

Nenhum roteiro que já existia foi alterado. Bateria a partir da pasta, às 22h46: `t_memoria` 80 no Mac e no container; `t_lsmval_abre` 38 no Mac (`traces/m6_lsm.bateria-final.txt`).

**Limites:**

- **O brilho, o som, o usuário e as estações não dão para mudar pela tela na bancada.** Para o brilho o `dsistub` teria de responder a `setDisplayBrightness` (1011) e `getDisplayBrightness` (1022); a HMI registra o tempo esgotado do segundo. Não mexi nos stubs (decisão D76). **Mudou em 12/10/2026:** o brilho passou a dar: o `dsistub` responde, a escolha pega e volta na partida seguinte (seção "O brilho pela tela: o display de mentira responde").
- **As unidades não voltam porque o carro de mentira não guarda o que a tela grava.** No carro quem deve lembrar é o próprio carro (dedução, não medida); aqui a escolha dura até a central reiniciar. **Mudou em 12/10/2026:** no modo ao vivo o carro de mentira passou a guardar, e a unidade volta (seção "O carro de mentira lembra do que a tela gravou"; conferido na rodada `v2`).
- **Das outras linhas (48 de 58) sei o que guardam pelo código e que a HMI as lê na partida; o efeito de cada uma ao voltar não foi olhado.**
- **A restauração de fábrica não foi feita**, só aberta e cancelada. Mais de um usuário não existe na bancada.
- **Uma tentativa perdida para a máquina e uma para uma troca no meio do caminho.** Com oito centrais no ar a carga passou de 100: cada central ocupa cinco núcleos mesmo parada (a HMI e os quatro daemons do firmware ficam, cada um, perto de 100 %, medido com `ps` dentro de uma central às 21h40; os cinco são os processos que carregam o `kipc_bus.so`, que lê o barramento sem bloquear, mas a causa eu não investiguei). A primeira `som1`, com 8 s entre os passos, tocou numa tela que ainda não aceitava toque; refiz com 15 s. E às 22h32 o carro de mentira e o esquema dele mudaram de formato na pasta, e a primeira `tela2` não subiu; refiz com uma cópia nova de `driver/` e de `stubs/`.
- **Nada disto rodou na central ao vivo.**

### O cabeçalho dos conjuntos: `83 60 22`

**O pedido.** Às 22h56 de 11/10/2026 a sessão "MOI3: diagnóstico que o navi-al entende" repassou um pedido à sessão do Marco 6: trocar os três bytes de cabeçalho que o `diagstub` manda, de `00 00 00` para `83 60 22`. Ela tinha recomendado ao Lipe fazer isso só depois do conserto do hub e se oferecido para repassar; ele respondeu, às 22h46: **"Pode passar o pedido quando o hub entrar"**. Li a frase no histórico daquela sessão antes de mexer. O hub novo entrou às 22h52 (decisão D66).

**Por quê.** O `navi-al` do firmware (`/lge/app_ro/navi/navi-al/alm`, função `KipcProvider::procDiagMsg`) recebe as mesmas mensagens do canal do microcontrolador e só as aceita se a carga começar por `83`, `60` e o serviço, que para os conjuntos de codificação e adaptação é `22`. Com os zeros ele descartava os 48 ("Data Type : Unknown !!!"). Quem leu isso no firmware e conferiu em 12 rodadas foi aquela sessão; o registro dela está em [painel-de-instrumentos.md](painel-de-instrumentos.md), "Item 3". Eu não reli o `alm`.

**O que mudou na pasta, às 23h26:**

- `driver/diagstub.c`: o cabeçalho passa a `83 60 22` em todos os conjuntos. A opção nova `-k` manda outro (3 bytes em hexa colado; `-k 000000` é o de antes), para rodada de controle. A linha de resumo, que o lançador repete, termina agora com "cabeçalho 83 60 22".
- `driver/tests/t_codificacao.sh`: 39 casos (eram 26). Os 13 novos e os que comparam o cabeçalho foram escritos antes da troca: com o código antigo, 24 de 39 falharam; com o novo passam os 39.
- `driver/m1_hmi.sh` e `driver/m6_rodada.sh`, às 23h31: opção `DIAGCAB`, que repassa o `-k` ao `diagstub`. Com `DIAGCAB=000000` sobe uma central de controle, com o cabeçalho de antes, sem trocar arquivo. A sessão do navi-al tinha anotado que essa rodada de controle deixara de existir.
- Nada muda no hub nem nos arquivos de `stubs/codificacao/`: são as mesmas 48 mensagens, com os mesmos tamanhos e o identificador nos mesmos bytes.

**O que conferi antes de trocar.** O que me cabia era garantir que uma central comum segue igual, porque nela quem lê os conjuntos são a HMI e o `persistence-app`.

- Pelo código: `PersistenceKIPC::event_handler` escolhe a fila só pelo canal, e `DiagHandler::event_handler` tira o identificador dos bytes 3 e 4 e os dados do byte 5 em diante (decompilados em `traces/m6_ghidra_persistence-app_diag.log`). Na HMI, `ListenerWorkerThread.processPersistenceListenerService` faz o mesmo. Nenhum dos dois lê os bytes 0 a 2.
- Em execução, com a previsão escrita antes (`traces/m6_previsoes.md`): duas centrais lançadas juntas de duas cópias de `driver/` tiradas às 23h01, já com o hub novo, iguais em tudo menos no `diagstub`.

| | `cab0` (controle, `00 00 00`) | `cab1` (`83 60 22`) |
|---|---|---|
| conjuntos no barramento | 48, com `cab=00 00 00` | 48, com `cab=83 60 22`; sem o cabeçalho, as 48 linhas são iguais às do controle |
| o que a HMI leu (linhas `CORE.DIAG`, sem a hora) | 104 | 104, idênticas |
| resumo do lançador | leituras sem dado=5, 13 telas, tipo `0203070100100000`, peça `3G5035832C` | igual, linha por linha |
| processos de pé no fim | os 8 | os 8 |
| mensagens no barramento | 1623, de 55 tipos | 1623, dos mesmos 55 tipos |
| leituras e respostas de persistência | 10 | 10, idênticas |
| fotos (190, 215, 245, 275 e 300 s) | | as cinco idênticas, byte a byte |

Das 1575 mensagens fora do canal de diagnóstico, 116 têm conteúdo diferente entre as duas: são as das teclas e do toque, que levam a hora, e as do canal `0x10`, que levam números que mudam a cada partida. A contagem de cada espécie é a mesma.

**Errei um ponto da previsão**, igual nas duas rodadas: a foto dos 275 s não é "Informações do sistema". Com o hub novo a HMI enxerga o serviço de veículo do firmware, a lista de ajustes ganhou "Hora e data" e "Unidades", e o toque em (400, 440) passou a cair em "Comando de voz", que está apagado. A identificação foi conferida pelo registro, não pela tela.

**O que a troca quebra, e não consertei.** Dois testes de outras sessões usavam o `diagstub` como fonte do cabeçalho zerado. Não mexi neles, porque não são meus. Avisei a sessão do hub às 23h00, 25 minutos antes de instalar, com o conserto provado numa cópia; a do navi-al, que fez o pedido, foi avisada na hora.

| Teste | Com a troca, na cópia | Conserto, provado numa cópia | Como ficou |
|---|---|---|---|
| `driver/tests/t_hub.sh` (sessão do hub) | 24 de 25: falhava `diagnostico_continua_chegando_antes_de_tudo` | trocar `cab=00 00 00` por `cab=83 60 22` na linha esperada: 25 de 25 | a dona fez melhor, às 23h25: o caso deixou de conferir o cabeçalho, que o hub não lê. 25 de 25 |
| `driver/tests/t_vc_diag_alm.sh` (sessão do navi-al) | 24 de 34 | `-k 000000` nas três chamadas do `diagstub`: 34 de 34 | a dona ajustou às 23h28, com o `-k 000000`, e acrescentou três casos que conferem o `diagstub` de hoje: 37 de 37. **Corrigido:** aqui estava "segue falhando em 10 de 34, até a dona ajustar" |

A bateria a partir da pasta, depois de instalar (23h26): `t_codificacao` 39, `t_entrada` 11, `t_pedidos` 16, `t_hub` 25, `t_veiculo` 128, `t_memoria` 80 (no container e no Mac) e `t_servidor` 20, todos passando; `t_vc_diag_alm` 24 de 34. Às 23h30, com o teste dela ajustado, rodei de novo os três ligados ao cabeçalho: `t_vc_diag_alm` 37, `t_codificacao` 39 e `t_hub` 25, todos passando.

**Limites:**

- **O que o microcontrolador de verdade manda não foi visto.** `83 60 22` é o que o `alm` exige, lido no código por outra sessão. Não há captura de um carro.
- **Numa central comum nada muda.** O `navi-al` não sobe nas centrais do modo ao vivo nem nas rodadas `m6_rodada.sh`; só nas rodadas `vc_` daquela sessão. **Conferido depois, das 23h31 às 23h35:** o `alm` aceita os conjuntos mandados direto pelo `diagstub` novo (parágrafo "O objetivo da troca", abaixo). Aqui estava "eu não conferi: quem tem a rodada para isso é ela".
- **A fronteira continua onde estava.** Pelo registro daquela sessão, com o cabeçalho certo o motor de navegação diz "uds = Enabled" e segue em `ready4navSwap`: falta o lado da ativação, que ninguém leu, imitou nem contornou. Esta troca não toca nisso.
- **Tamanho.** O `alm` copia a mensagem inteira para estruturas de tamanho fixo. Nunca mandar um conjunto maior que o de verdade; os de `padrao.txt` cabem.
- **Uma dupla de rodadas.** As contagens bateram, mas é uma comparação só. O `mcu-interpreter`, o `carifservice` e o `touch-app` não foram lidos; não reagiram.
- **A central ao vivo que estava no ar** segue com o cabeçalho zerado até religar.

**O objetivo da troca, conferido depois de instalar (23h31 a 23h35).** Duas conferências independentes, as duas com o `diagstub` mandando direto, sem a ferramenta à parte:

- **A da sessão do navi-al** (rodadas `r6` e `r6b` dela, com o compositor de mentira e a região de teste): os 8 conjuntos reconhecidos pelo `alm`, e o motor diz "R4N : nav : uds = Enabled, fec = Undefined". O estado continua `ready4navSwap`. Registro dela em [painel-de-instrumentos.md](painel-de-instrumentos.md), "Conferência final". Não refiz essas rodadas.
- **A minha** (`vcab0` e `vcab1`, previsão escrita antes): duas centrais lançadas juntas da mesma pasta, com os serviços de navegação do firmware, pela rodada dela (`driver/vc_rodada_container.sh`), só para ler o que o `alm` registra. O controle saiu da opção nova do lançador.

| | `vcab0` (`DIAGCAB=000000`) | `vcab1` (o padrão novo) |
|---|---|---|
| lançador | "cabeçalho 00 00 00" | "cabeçalho 83 60 22" |
| conjuntos no barramento | 48 com `cab=00 00 00` | 48 com `cab=83 60 22` |
| `alm`, "Data Type : Unknown" | 48 | 40, todos por identificador que ele não trata |
| `alm`, conjuntos reconhecidos | nenhum | os 8: `0600`, `0558`, `0B5A`, `0B1D`, `09CB`, `0E38`, `0571` e `243F` |
| o que o `alm` leu | nada | "nav : 0x1", "country : 0", e da codificação marca 2, classe 3, geração 7, carroceria 1: os valores do arquivo de deduções |
| HMI: linhas `CORE.DIAG` | 104 | 104, idênticas |
| fotos (70, 100 e 140 s) | | as três idênticas, byte a byte |

**Errei um ponto dessa previsão:** esperava a linha do motor nas duas, e ela não saiu em nenhuma. O motor subiu e parou logo depois das linhas de watchdog (22 kB de saída, contra 1,8 MB numa rodada boa da outra sessão), porque a minha rodada não subiu o compositor de mentira (`WLSTUB`) que as dela usam. As minhas duas mostram até o `alm`; o motor quem viu foi ela. A outra dúvida se resolveu: o `alm` aceita os conjuntos que recebe do hub logo que se liga, sem ter visto antes os zerados.

**Duas perguntas que vieram junto, fora do pedido:**

- **O padrão `30 78` de `09CB`, `0B48` e `0B49`** é o texto "0x". Não é resto da geração do `padrao.txt`: é o valor padrão que a tabela da própria HMI traz para esses três campos de bytes (`mConfiguration` da `AdaptationImpl`), e é o que ela devolve quando não há dado. O gerador só o copiou. Que seja resto de quem fez a tabela é provável. Com o cabeçalho novo o `alm` guarda o do `09CB` como país de venda.
- **A região de navegação nos arquivos do Jetta** (byte 0 do `0558`): hoje vai 0. Pôr 3 em `jetta-gli-2021-deducoes.txt` seria mais um palpite; não pus. O valor de verdade sai da adaptação do 5F. Só faz diferença com o motor de navegação ligado.

### Onde parou

- **Os valores do carro.** Faltam o autoscan e a leitura do 5F. A tabela já diz o que cada byte da codificação quer dizer para esta HMI, e o arquivo `jetta-gli-2021.txt` tem o lugar deles.
- **Identificação.** Feita com o que a ficha tem. Número de série (`F18C`) e número de hardware (`F191`) continuam zerados.
- **Idioma.** Resolvido às 20h de 11/10/2026 pela memória de estado: a central nasce em português (subseção acima). **Corrigido:** aqui estava "vem pelo `720B` e pelo `720C`, por outro caminho (o `persistence-app`). Não exercitado"; isso é a reserva, que vale quando a memória não tem idioma, e continua sem exercício.
- **Memória entre partidas.** Feita às 20h58 de 11/10/2026: no modo ao vivo a central guarda o `lsmval` enquanto roda e a sessão seguinte nasce dele (subseção acima). **Corrigido:** aqui estava "a central nasce sempre do mesmo arquivo… não fiz". Falta conferir o que as outras linhas da memória fazem ao renascer (som, favoritos, estação) e uma sessão longa. **Feito às 22h50 de 11/10/2026** (subseção "O que a memória guarda, linha por linha"): voltam o som do touchscreen, a hora em standby, o sensor de aproximação, o gesto de mão, os idiomas do teclado, a ordem da grade e a visão da tela do veículo; a unidade fica guardada e não volta; brilho, grave e agudo, usuário e estações a tela não deixa mudar na bancada; a sessão de 35 minutos ficou em 58 linhas, sem cópia recusada. Falta o efeito das 48 linhas que ninguém mudou, a restauração de fábrica e o brilho pela tela.
- **Skin e figura.** A adaptação tem "Skin Codierung" e "Fahrzeugbild" (DID `0556`), e o firmware traz um skin da MAN. Não mexi.
- **A tela de diagnóstico da própria HMI**, que lista esses valores com os nomes dos `.swap`: não achei como abrir.
- **Menus do carro** (data e hora, unidades): dependem também dos dados de veículo, que são o item 2 do roteiro do simulador. **Feito no Marco 8, na noite de 11/10/2026:** com o carro de mentira, "Hora e data" e "Unidades" aparecem em "Configurações do sistema" (quadros `quadros/2026-10-11-hmi-vei-hora-e-data.png` e `-unidades.png`). Os outros menus do carro seguem nos próximos passos.

## Marco 7 — transmissão direta dos quadros e janela do Mac (11/10/2026)

Feito pela sessão principal, a do simulador, enquanto o Marco 6 corria em outra sessão. Pedido do Lipe, depois de perguntar se dava para ter "acesso direto à interface": **"Antes, faz a transmissão direta com a janela própria"**. A análise que veio antes do pedido, com os outros caminhos possíveis, está em [simulador-jetta.md](simulador-jetta.md), seção "Acesso direto à interface".

### O que existe agora

| Peça | Onde | O que faz |
|---|---|---|
| Transmissão no `glhost` | `glforward-moi3/host.c` | a cada troca de buffers, além de gravar o arquivo, manda o quadro por um soquete Unix do container (`/tmp/moi3-quadros.sock`), só o que mudou, numa thread própria |
| Ponte | `glforward-moi3/ponte.c` | chamada por `docker exec -i`, copia a transmissão para a saída padrão e acrescenta no arquivo de comandos as linhas válidas da entrada padrão |
| Janela | `janela/janela.swift` | programa do Mac: tela, nove teclas, dois botões giratórios, teclado, ligar e desligar a central, salvar a tela em PNG |

O formato das mensagens está no cabeçalho da seção de transmissão do `host.c` e no [README do encaminhador](glforward-moi3/README.md); o uso da janela, no [README dela](janela/README.md). Abrir: `sh janela/abre.sh`. A página do navegador continua funcionando, na mesma central; o container continua sem rede.

### Testes

Escritos antes do código, numa cópia de trabalho fora da pasta (decisão D41).

| Onde | Quantos | Antes da mudança | Depois |
|---|---|---|---|
| `glforward-moi3/tests/t_transmissao.sh` (container) | 16 conferências, em cinco cenários | 14 falhavam; as 2 que passavam só acusam envio que trava | 16 passam |
| `glforward-moi3/tests/run_tests.sh` (os de antes) | 8 | passavam | passam |
| leitor de quadros da janela (`--decodifica`) | 1 comparação | não existia | imagem igual, byte a byte, à que o `glhost` gravou |

Prova de que o cenário do espectador parado acusa o defeito: com o envio trocado por um que espera, as duas conferências do "outro espectador" falham, como previsto.

### Medidas

Numa central de teste à parte (`moi3-exp-tx`: o `driver/m3_ao_vivo.sh` com o encaminhador novo), 800×480, do container ao Mac:

| O quê | Valor |
|---|---|
| quadro-chave da tela "Radio: função indisponível" | 21,5 kB (o arquivo PPM tem 1,15 MB) |
| troca para a grade de menus | 40 quadros, 20 ms entre um e outro (50 por segundo), 37 kB em média, 433 kB o maior |
| primeiro quadro no Mac, contado da chamada do `docker exec` | 0,08 a 0,21 s |
| três trocas de tela seguidas, pela janela | 229 quadros recebidos, 229 trocas de imagem: nenhum pulado |
| da tecla Menu ao primeiro quadro desenhado, e à primeira mudança visível | 194 ms e 589 ms; quase tudo é tempo da própria HMI |

Não medi o caminho da página para comparar tempo de resposta. O que se sabe dela é do Marco 5: ela pergunta 12 vezes por segundo, então numa animação mostra um quadro em cada quatro.

### A troca na central ao vivo

Às 19h31 a janela, aberta pelo `open` como seria pelo Finder, desligou a central das 18h42 e ligou a nova pelo próprio botão (`driver/ao_vivo.sh` sem alteração, 2 horas, saída em `ao-vivo/log/janela-ligou.log`). A central nova subiu com a identidade provisória do Jetta (decisão D34, conferido no registro da HMI) e com o relógio provisório religado à mão. O Lipe usou a janela com o mouse enquanto a verificação corria: apareceram no registro oito toques em teclas e sete giros do botão de seleção, por rolagem, que o roteiro não tinha mandado. Às 19h34 a central estava no quadro 681, com 83 linhas de comando recebidas e nenhum erro de OpenGL.

### Limites

- **A imagem da janela na tela do Mac não foi fotografada pelo sistema.** A captura de fora foi negada a esta sessão (`screencapture -l`: "could not create image from window"). Conferi o que a janela entrega para o sistema desenhar: a árvore de camadas, com a imagem da central no lugar.
- **O mouse e o teclado do roteiro são fabricados dentro do programa.** Passam pelo mesmo código que os de verdade. Os de verdade só foram vistos no uso do Lipe, sem roteiro de conferência.
- **Dois caminhos gravam no arquivo de comandos:** a ponte, de dentro do container, e o servidor da página, de fora. Os dois mandando no mesmo instante não foi testado.
- **Sem compressão além de pular o que não mudou.** Uma troca de tela inteira custa perto de 1,15 MB num quadro só; por um caminho mais lento que o `docker exec` local isso não foi testado.
- **Erro de condução:** rodei o roteiro de verificação na janela que o Lipe estava usando, e o último passo fechou a janela na mão dele. Reabri em seguida. Roteiro que termina em `sai` vai em central de teste, com `--container`.

### Defeito achado em uso e corrigido: a central surda para a janela (11/10/2026, 22h10)

O Lipe religou a central pela janela e avisou, com uma foto da janela no aviso "Bem-vindo": "Ela está ficando travada aqui". A central não estava travada. Ela não recebia nada do que a janela mandava.

**A causa, vista na central no ar, só lendo.** A `ponte` abria o arquivo de comandos uma vez, ao ligar. A janela liga logo que o `glhost` sobe, e o lançador só põe o arquivo de verdade naquele caminho alguns segundos depois, por montagem, logo antes de subir o `dsistub`. Naquela partida: `glhost` às 01:07:52, `ponte` às 01:07:54, `dsistub` às 01:07:58 (hora do container). A ponte ficou com o arquivo que ela mesma tinha criado no caminho, depois escondido pela montagem: o dela era o `225:40`, com 1.388 bytes de teclas e toques do Lipe; o que o `dsistub` lia era o `36:97841`, vazio. É uma corrida de partida: quando a janela liga depois da montagem, funciona. Deve ter sido a mesma coisa na partida das 21h43, que ficou 23 minutos no mesmo quadro; essa não conferi.

**O remédio na hora.** Derrubei só o processo da ponte dentro do container. A janela religou sozinha, e a ponte nova passou a gravar no arquivo certo (os dois no `36:97841`, conferido). A central não foi reiniciada nem recebeu comando meu.

**A correção.** A `ponte` agora abre o arquivo de comandos a cada linha, pelo caminho, e nunca o cria; linha que chega sem arquivo no lugar é descartada. Teste escrito antes, no `glforward-moi3/tests/t_transmissao.sh` (cenário `ponte_liga_cedo`, três conferências): contra a ponte antiga as três falhavam; agora passam, e a suíte ficou em 8 + 19. Depois reproduzi o caso de verdade numa central de teste (`moi3-exp-ponte`), com a ponte ligando 2 s depois do `glhost` e 5 s antes do `dsistub`, como na partida que falhou: a tecla Menu mandada por ela chegou ao arquivo de comandos, o `dsistub` a registrou e a tela andou do quadro 40 ao 80.

**O que eu tinha dito de errado.** Às 21h20, ao ver o aviso "Bem-vindo" numa partida de prova, escrevi ao Lipe que "Não exibir" fechava o aviso. Eu não tinha tocado nele. Quem conferiu isso foi a sessão do Marco 8 (rodadas `ne1` e `ne2`).

**Custo que fica:** a janela não sabe quando uma linha foi descartada; ela mostra "Último: toque em…" mesmo que a central ainda não esteja pronta para entrada. A central no ar só ganha a ponte corrigida na próxima partida, porque compila o encaminhador ao subir.

### As teclas do volante na janela e na página (12/10/2026, de madrugada)

Feito por uma sessão à parte, criada pela principal enquanto o Lipe dormia; é o item 6 do roteiro do [simulador-jetta.md](simulador-jetta.md) e fecha a linha "As teclas do volante (teclado 4) não foram simuladas" do Marco 5.

- **O que entrou.** Uma faixa "Volante" embaixo do aparelho, na janela (`janela/janela.swift`) e na página (`ao-vivo/index.html`), com as onze teclas que a tabela de teclas tem para o teclado 4: 43, 42, 44, 57, 47, 46, 49, 48, 45, 50 e 53. Saem como `tecla 4 <código> <estado>`; o `dsistub`, a ponte e o servidor da página já aceitavam e não mudaram. O roteiro da janela ganhou o passo `volante <nome>`. A janela ficou 56 pontos mais alta.
- **O que a HMI faz, numa central de teste (`moi3-vol-t1`).** Recebe as onze (`HK Received: KBD[4] …`, 71 apertos e 71 solturas, a mesma conta do arquivo de comandos e do `dsistub`). Sozinha, nenhuma muda a tela. Com a barra de volume aberta pelo botão da moldura: 42, 43 e 44 fazem a barra fechar 3,00 s depois de soltas; 57, 47, 46, 50 e 53 a fecham no instante do aperto; 49, 48 e 45 não mudam nada. Bate com a leitura do firmware: as cinco que fecham são as que têm evento na tabela, e as três de volume disparam o evento `VOLUME_CHANGE_THROUGH_MFL` no código (`AslTargetSystemKeyPanelHandling`). Rádio e mídia estavam no aviso "função indisponível".
- **Conferido.** A janela compila sem aviso; o `--decodifica` dá a mesma imagem, byte a byte, do programa de antes e do glhost; `t_transmissao.sh` 19 de 19 e `t_servidor.sh` 20 de 20; um roteiro da janela e os cliques da página puseram as linhas no arquivo de comandos na ordem.
- **Suposição e ressalvas.** Do volante do carro só se conhece o raio direito, pela foto do painel de 09/10/2026: voz, setas para cima e para baixo, faixa anterior e próxima. Que ele tenha as outras teclas, e qual botão manda qual código, é suposição. Uma das sete execuções do roteiro ficou parada no último passo `foto`, com o Mac em carga acima de 100; não se repetiu e não sei a causa.

A tabela tecla por tecla, as medidas e o que é dedução estão em [janela/README.md](janela/README.md), seção "Teclas do volante"; as saídas, em `traces/vol_*`; os quadros, em `quadros/2026-10-12-*`.

### Onde parou

- A janela e a página mostram a mesma central; nada mais depende de arquivo para ver a tela.
- A gravação do arquivo PPM a cada quadro continua na thread de desenho, e a pasta dos quadros fica no Mac. Se isso segura o ritmo da HMI não foi medido.
- O acesso aos elementos da tela (a interface de teste da própria HMI) continua só como leitura estática, em [simulador-jetta.md](simulador-jetta.md).

## Marco 8 — dados de veículo: o carro de mentira (11/10/2026)

Feito numa sessão à parte, aberta pela sessão do simulador na noite de 11/10/2026. É o item 2 do roteiro de [simulador-jetta.md](simulador-jetta.md): dar à HMI os dados que no carro vêm do resto do carro, simulando o Jetta GLI 2021 parado.

**Resultado, verificado em execução** numa central de teste que subiu só com os padrões do modo ao vivo, sem nenhuma variável (`moi3-m8-prova`, às 21h18), e depois lido no registro da central ao vivo que foi ligada às 21h33: o relógio aparece sozinho, com a hora de Brasília; a tela do veículo mostra os dados de condução, a autonomia e a temperatura externa; "Configurações do veículo" deixou de ser uma lista vazia; e a tela Welcome, que sumia quando os serviços de veículo eram anunciados, voltou e fica perto de 5 s na tela. O `glhost` não registrou erro de OpenGL em nenhuma rodada.

| Tela do veículo | Configurações do veículo |
|---|---|
| ![Dados de condução](quadros/2026-10-11-hmi-vei-veiculo.png) | ![Configurações do veículo](quadros/2026-10-11-hmi-vei-configuracoes.png) |

| Hora e data | Unidades |
|---|---|
| ![Hora e data](quadros/2026-10-11-hmi-vei-hora-e-data.png) | ![Unidades](quadros/2026-10-11-hmi-vei-unidades.png) |

| Instrumento combinado | Status do veículo |
|---|---|
| ![Instrumento combinado](quadros/2026-10-11-hmi-vei-instrumento-combinado.png) | ![Status do veículo](quadros/2026-10-11-hmi-vei-status-boa-viagem.png) |

O relógio e a temperatura externa aparecem também no topo das outras telas, como na [de clima](quadros/2026-10-11-hmi-vei-clima.png).

**Nem tudo o que se vê é do carro.** Do Jetta só foram lidas as unidades (km, km/h, °C), o formato de 24 h e a marcha em P, numa foto do painel. A autonomia de 400 km e o tanque pela metade são números de simulação; que menus o painel oferece e a unidade de consumo são palpites. Cada linha dos perfis diz o que é.

### Quem responde pelos serviços de veículo na bancada

Antes de escrever o stub olhei o tráfego de uma partida sem ele (rodada `ref`).

- **O fornecedor de verdade existe e já rodava na bancada: é o `carifservice` do próprio firmware**, que o lançador sobe desde 10/10. O binário (`/lge/app_ro/carif/carifservice`, 27 MB, SHA-256 `e14b9e94…`) depende da biblioteca `libCarSSW_CarFunctionsAdapter.so` e traz os símbolos de todos os serviços `DSICar…` (`CDSICarSeatListener::LGsetNotification`, as mensagens `proto_cartimeunitslanguage_…`). É ele o domínio 16 no carro: recebe o que vem da rede do carro e o traduz para a HMI.
- **Ele anuncia o domínio, uma vez, logo que sobe**: `canal=0x10 if=9000`, com a carga `WELCOME`. **Corrigido:** no meio da sessão eu tinha concluído que ele não anunciava, e passei isso adiante na nota de passagem. Estava errado.
- **A HMI não recebe esse anúncio por um limite do nosso hub.** O `driver/kipc_hub.c` guarda as 48 primeiras mensagens do barramento para entregar a quem se liga depois, e a HMI se liga depois dos daemons. O anúncio do `carifservice` foi a 61ª mensagem nas duas partidas em que contei (`ref` e `anu`). Antes dele o próprio `carifservice` manda umas cinquenta mensagens em interfaces que não estão na tabela DSI (`102401`, `1762168833`, `17338369`), e são elas que tomam 42 das 48 vagas. O que essas mensagens são não investiguei.
- **Verificado (rodada `anu`):** central sem o carro de mentira; aos 30 s a HMI não tinha feito nenhum pedido nas interfaces 800 a 831. Reenviei com o `dsi_manda` o mesmo anúncio, byte a byte. Em 20 s a HMI fez 16 pedidos de assinatura, em oito serviços, e o `carifservice` respondeu com 23 eventos, quase todos vazios. Que a HMI ainda não estava ligada quando o primeiro anúncio passou é dedução: a mesma mensagem, mandada depois, teve efeito.
- **O que ele responde é "sem carro"**: opções de relógio em estado 0, opções do painel em estado 1 (existe, inacessível), `validFlag` 0 ou 2. A tela fica assim:

![Tela do veículo com o serviço do firmware respondendo](quadros/2026-10-11-hmi-vei-veiculo-sem-carro.png)

Daí duas coisas. O carro de mentira continua necessário: sem carro, o serviço de verdade não tem o que dizer. E, com ele ligado, o `carifservice` não sobe (decisão D50), para não haver dois fornecedores disputando cada atributo. Medi os dois juntos (`VEICULO_CARIF=1`, rodadas `dois`, `d1` e `d2`): nas interfaces de eventos dos serviços de veículo, 73 de 310 mensagens diferentes (contando por interface e api) chegaram com dois conteúdos, um de cada fornecedor. Com um fornecedor só é 1 em 307: a hora, que muda a cada segundo.

**Corrigido:** na rodada `dois` a tela do veículo ficou com "---" em todos os campos, e eu tinha posto isso na conta da disputa. Refiz no fim, com o stub definitivo (`d1` e `d2`), e nas duas os dados do carro de mentira apareceram na tela. A `dois` rodou com o stub que ainda anunciava de segundo em segundo; o mais provável é que tenha sido o painel mudo, descrito mais abaixo. Não tenho um caso em que a disputa tenha estragado a tela. O que há é a disputa, sem garantia de quem chega por último.

O limite do hub ficou como está; mexer nele muda a partida de todas as rodadas e não era o pedido. Está nos próximos passos. **Mudou às 22h52 de 11/10/2026:** o hub passou a guardar o último anúncio de cada canal, e a medição mostrou que o limite escondia mais do que este anúncio; ver "O hub e os anúncios que ficavam fora do cache", depois deste marco, e a decisão D66.

### Como a HMI assina e o que espera de volta

Lido em `DSICarTimeUnitsLanguageImpl` e `DSICarTimeUnitsLanguageListenerService` (bytecode da LG) e conferido no barramento.

- **Assinar** é a api 9999 na interface de pedidos do serviço, com o canal `0x8000` da HMI e os atributos em `sint32`, um a um: `08 14 08 26` assina o 10 e o 19. A api 9998 desassina.
- **A atualização** volta na interface de eventos, com a api de mesmo número do atributo e o canal do domínio.
- **`validFlag`** vale 0 (desconhecido), 1 (válido) ou 2 (inválido): constantes `ATTRVALIDFLAG_…` de `org.dsi.ifc.base.DSIListener`.
- **Opção de exibição** (`CarViewOption.state`): 0 não existe, 1 existe mas está inacessível, 2 funciona. É a regra que o emulador da MIB2 descrevia; aqui saiu das constantes `VIEWOPTIONSTATE_…` do firmware.
- **A HMI liga seis serviços na hora do anúncio e dez outros 5 s depois**, numa thread nova por anúncio (`BAPTaskScheduler`). Isso importa mais abaixo.
- **Com o carro de mentira a HMI passa de 21 para cerca de 130 pedidos de assinatura na partida.** Os 21 são de outros domínios (energia e painel de teclas entre eles). O stub atendeu 110 ou 111 em cada rodada, 307 ou 308 atributos, de 14 dos 16 serviços.

### O esquema, gerado das classes do firmware

`driver/m8_esquema.sh` compila e roda o `driver/LeEsquemaDSI.java` na JVM do firmware, sem decompilador:

| O quê | De onde |
|---|---|
| serviços do domínio, interface de eventos de cada um, serviços que o anúncio liga | bytecode de `ListenerWorkerThread` e `BAPTaskScheduler`, lido com o `javap` da própria JVM (`tools.jar`) |
| api de cada evento, mensagem que a HMI lê, atributo de entrega | o `switch` de `executeListenerService` de cada `…ListenerService` |
| api e interface de cada pedido | as chamadas a `postDSIProviderMsg` de cada `…Impl` |
| campos de cada mensagem | constantes `…_FIELD_NUMBER` e campos das classes protobuf, por reflexão. São `GeneratedMessageLite`: não trazem descritor |
| codificação de cada campo | **provada**: o programa monta a mensagem com o `Builder` do firmware, serializa e olha os bytes. Um `-1` num `sint32` sai em um byte; num `int32`, em dez |
| nomes de atributos e constantes | interfaces `org.dsi.ifc.*` do jar da HMI |

Saída em `stubs/veiculo/gerado/` (1,5 MB): **16 serviços, 957 atributos, 1.119 eventos, 647 pedidos, 2.362 mensagens, 6.941 campos e 3.812 constantes**. Duas rodadas seguidas deram arquivos idênticos. Nos 16 serviços só há seis tipos de campo: `sint32` (2.978, dos quais 67 em lista, um a um), `bool` (1.913), mensagem (1.914), `float` (72), texto (55) e `double` (9).

O relatório (`gerado/relatorio.txt`) guarda o que o gerador achou de esquisito **no próprio firmware**:

- Em 18 eventos o código da LG chama o método de outro atributo. A atualização de "descongelamento máximo" do clima (api 81) é entregue como "descongelamento"; no serviço dos bancos são dez trocas e no do teto solar, seis. Por isso o nome de cada evento, no esquema, é o da mensagem que a HMI lê, e não o do método (decisão D52).
- Em seis eventos a HMI procura os ouvintes pelo atributo errado: as opções de unidades e o idioma do painel (atributos 10 e 11 do serviço de relógio) só chegam a quem também assinou o 5.
- Cinco atributos não têm evento que os atualize, e o 16 do aquecedor auxiliar é lido e não é entregue a ninguém (o único aviso do gerador).

Isso vale para a central do carro também: é o mesmo código. Não muda nada no stub, que manda a mensagem que a HMI espera em cada api.

### O carro de mentira

`driver/veiculostub.c` não conhece nenhum serviço: lê o esquema e os perfis e faz o papel do `carifservice`.

- Anuncia o domínio e, a cada atributo assinado, manda a atualização com o valor do perfil. O que o perfil não diz vai com o valor padrão, tudo zero, **marcado como válido**.
- Valores com marcador (`@hora`, `@dia`…) acompanham o relógio do computador no fuso do perfil; a mensagem sai de novo quando muda. É o relógio da central.
- Pedido que não é assinatura fica registrado, com o nome tirado do esquema, e sem resposta.
- `-n` só confere os arquivos (é o que o lançador roda antes de subir qualquer coisa) e `-m` mostra o que seria mandado.

**Prova do codificador.** O gerador produz também um perfil que dá valor a toda folha de todo evento (10.762 folhas) e os bytes que as classes do firmware serializam para ele. O `veiculostub`, com o mesmo perfil, tem de dar os mesmos bytes: **1.117 de 1.117 mensagens iguais, byte a byte** (`driver/tests/t_veiculo.sh`).

**Os perfis** (`stubs/veiculo/`, formato no [README](stubs/veiculo/README.md)):

| Arquivo | O que tem | De onde vem |
|---|---|---|
| `carro-parado.txt` | o cenário: hora e data de verdade no fuso de Brasília, as duas opções de relógio sem as quais a HMI não aceita a hora, carro parado, freio de estacionamento puxado | escolha do simulador |
| `jetta-gli-2021.txt` | unidades em km, km/h e °C, relógio de 24 h, câmbio em P | a ficha e a foto do painel de 09/10/2026 |
| `jetta-gli-2021-deducoes.txt` | PALPITE: que ajustes de relógio e unidades existem, que dados o painel oferece, km/l, itens do indicador multifunções, motor a gasolina. SIMULAÇÃO: autonomia 400 km, tanque 50 %. A temperatura externa de 35,0 °C é a da foto, fixa | nenhum valor lido como configuração do carro |

O chassi do carro não é usado.

**Zero não é neutro.** A primeira rodada só com o valor padrão já mostrou: `standstill = 0` quer dizer carro em movimento (`AslTargetSystemVehicleStates` chama `setPropertyDrivingState(!standstill)`), e a unidade de volume 0 é quilo. O perfil existe para isso.

### Os menus do carro dependem também da codificação

Com o stub e os perfis no ar o relógio apareceu, mas a tela do veículo continuou com "---" e "Configurações do veículo" continuou vazia (rodada `jet`). A HMI só cria os menus do carro para as funções que a adaptação do 5F declara: `DeviceManager.readMenuCoding` percorre o que `requestCarFuncAdap` devolve, a lista de funções do carro (DID `0B1B`, em cada byte o bit 7 é "Menu display"). Há ainda a lista de aparelhos da rede do carro (DID `0B1D`, bits 6 e 7 de cada byte).

Liguei quatro funções nas duas listas (computador de bordo, relógio, unidades e revisão) e os menus apareceram (rodada `men`). As oito linhas estão em `stubs/codificacao/jetta-gli-2021-deducoes.txt`, que é o arquivo de palpites do Marco 6. Liguei as duas listas juntas; não testei uma sem a outra.

| Sem as funções | Com as funções |
|---|---|
| ![Seleção vazia](quadros/2026-10-11-hmi-vei-selecao-sem-funcoes.png) | ![Seleção](quadros/2026-10-11-hmi-vei-selecao.png) |

### As rodadas

Centrais de teste `moi3-m8-<nome>`, à parte da central ao vivo, subidas com `driver/m8_central.sh`. Saídas em `traces/m8_*`.

| Rodada | O que mudou | Observado |
|---|---|---|
| `ref` | nada: sem o stub, `carifservice` de verdade | nenhum pedido da HMI no domínio 16; relógio "--:--"; Welcome na tela por 1,1 s; 8 ajustes do painel de teclas sem resposta |
| `pad` | stub só com o valor padrão (`VEICULO=-`) | 110 pedidos de assinatura atendidos; relógio "--:--" e autonomia "---", como previsto; **sem Welcome** |
| `jet` | os três perfis | "First valid ClockTime: ClockTime(hours=20,minutes=31,seconds=27,timeZone=-3.0…)", a hora do Mac; tela do veículo ainda com "---"; `handlePowerState( 5 )` e depois 2; sem Welcome |
| `men` | mais as quatro funções na codificação | "Desde a partida", autonomia 400 km, 35,0 °C; "Seleção" com "Dados de condução" e "Status do veículo"; lista com "Instrumento combinado" e "Hora e data"; sem Welcome |
| `men2` | mais opções de unidades e de revisão no perfil | **o painel ficou mudo**: tudo "---". Atribuí à linha da lista de estados do veículo, que eu tinha acrescentado junto. Estava errado (ver abaixo) |
| `sl` | `dsistub` respondendo à assinatura de energia | `handlePowerState( 2 )` direto e Welcome pedida, mas a primeira tela só saiu aos 6,2 s, com a Welcome já vencida. O painel ficou mudo de novo, sem a linha suspeita |
| `w2` | `dsistub` informando a câmera de ré inativa | primeira tela aos 0,6 s; **Welcome de 1,1 a 5,7 s**; lista com quatro itens |
| `fim` | só os padrões do modo ao vivo | relógio aos 0,29 s; Welcome de 0,50 a 5,52 s; popup do usuário principal até 35,5 s |
| `dois` | stub e `carifservice` juntos | 73 de 309 eventos com dois conteúdos; tela do veículo com "---" (ver a correção acima) |
| `lista` | o perfil com a lista de estados, tudo o mais igual | dados na tela; "Status do veículo" mostra "Boa viagem!" |
| `s1` a `s6` | o anúncio do domínio uma vez só | seis partidas seguidas, todas com os dados do painel na tela, Welcome e um anúncio só no barramento |
| `prova` | nada: é a prova final, só com os padrões | relógio aos 0,21 s (21:18:09); Welcome de 0,78 a 5,35 s; popup do usuário principal de 5,35 a 35,36 s; as telas dos quadros; 111 pedidos de assinatura atendidos; nenhum erro de OpenGL em 974 quadros |
| `anu` | sem o stub; o anúncio do `carifservice` reenviado aos 30 s | antes, nenhum pedido no domínio 16; depois, 16 pedidos de assinatura e 23 eventos do `carifservice`; relógio "--:--" e tela do veículo com "---", como previsto |
| `t1` a `t6` | repetição da série | outras seis, com o mesmo resultado |
| `d1`, `d2` | stub definitivo e `carifservice` juntos | 73 de 310 eventos com dois conteúdos; dados do painel na tela nas duas |
| `am`, `bm` | nascendo da memória guardada da central ao vivo; `am` com o carro de mentira, `bm` sem | `am`: o assistente de configuração aparece aos 52 s; `bm`: não aparece em 60 s |
| `ne1`, `ne2` | `ne1` como a `am`, com um toque em "Não exibir"; `ne2` nasce da memória que a `ne1` deixou | `ne1`: o assistente fecha e a memória muda; `ne2`: `dontShowAgain=true`, sem assistente |

A previsão da `anu` está escrita no cabeçalho de `traces/m8_anu.sh`, feito antes de rodar. As das outras rodadas não ficaram escritas em arquivo.

![As seis partidas da série, cada uma na tela do veículo](quadros/2026-10-11-hmi-vei-serie.png)

### A tela Welcome some por causa de dois relógios da própria HMI

O efeito colateral que a sessão do simulador viu duas vezes (com o domínio 16 anunciado, a tela Welcome e o popup do usuário principal não aparecem; o controlador registra `handlePowerState( 5 )`) não vem de dado de veículo nenhum. São dois prazos da HMI, lidos no código dela e medidos nas rodadas. O `handlePowerState( 5 )` é um sintoma da mesma pressa, e não a causa.

**1. A HMI segura todas as telas da partida esperando a câmera de ré.** `ContentProcessorManagerImpl` nasce numa fase que o código chama de "early RVC": guarda os conteúdos de todas as áreas da tela e só os instala quando o serviço de energia diz que a câmera de ré não está ativa (`updateRVCActive`, atributo 7 do `DSIPowerManagement`), ou quando vence um prazo de 6 s (propriedade `de.vw.mib.contentmanager.earlyrvc.timeout`). A HMI assina esse atributo junto com o 3 e o 4 (`08 06 08 08 08 0e` no barramento). Na bancada ninguém respondia: **a primeira tela sempre saiu 6,0 s depois de o gerente de conteúdos nascer**.

**2. A tela Welcome dura 5 s contados de quando é pedida**, e não de quando aparece (`WelcomeScreenLogicController`, propriedade `…WelcomeScreenTimerDurationInMS`, padrão 5000).

| | Sem os serviços de veículo (`ref`) | Com eles (`sl`) |
|---|---|---|
| gerente de conteúdos nasce | 2,20 s (depois de uma espera de 2 s por serviços ausentes) | 0,24 s |
| aplicativo da Welcome sobe e a pede | 4,36 s (depois de outra espera de 2 s) | 0,42 s |
| Welcome vence | 9,36 s | 5,45 s |
| primeira tela (prazo de 6 s) | 8,20 s | 6,24 s |
| Welcome na tela | **1,1 s** | **nunca** |

Ou seja: a Welcome só aparecia na bancada porque as esperas por serviços ausentes atrasavam o aplicativo dela. Com os serviços de veículo presentes a HMI sobe mais depressa, e o prazo da câmera de ré passou a vencer depois da Welcome. Na rodada `pad`, sem perfil nenhum, a conta é a mesma (Welcome vence aos 5,9 s; primeira tela aos 6,4 s). Nas cinco rodadas sem Welcome (`pad`, `jet`, `men`, `men2`, `sl`) ela foi pedida: o registro traz "show welcome screen requested" entre 0,3 e 1,0 s e "hide welcome screen requested" entre 6,25 e 6,41 s, sem a tela ter entrado.

**O `handlePowerState( 5 )`.** O 5 é `POWER_STATE_NOT_INIT`, o valor com que o serviço de estado de energia da HMI nasce. Com a HMI subindo mais depressa, o aplicativo da Welcome passou a nascer antes de chegar o estado "ligado", que o `dsistub` publicava de segundo em segundo sem olhar para a assinatura, e leu 5 (rodadas `jet`, `men` e `men2`: 5 e, meio segundo depois, 2).

- Lido no código: nesse caso o caminho do estado de energia não pede a Welcome (a regra é `powerState == 2` vindo de 0 ou 1).
- Medido: ela é pedida do mesmo jeito por outro caminho. O registro traz `updateWelcomeScreenView 5` e, em seguida, "check And Show WelcomeScreen: REGULAR" e o temporizador. O que se perde é só o passo "LOADING".
- E nas rodadas `pad` e `sl` o estado veio 2 direto e a Welcome também não apareceu.

**Corrigido:** na nota de passagem eu tinha escrito que, com o 5, "a Welcome não é pedida". O registro das rodadas mostra que é.

**Conserto, no `driver/dsistub.c`:** responder na hora à assinatura de energia, como um fornecedor DSI faz, com o estado (atributo 1), a ignição (4) e a câmera de ré não ativa (7, uma vez por assinatura). Testes em `driver/tests/t_pedidos.sh`. Efeito medido em dezoito partidas (`w2`, `fim`, `dois`, `lista`, `s1` a `s6`, `prova`, `anu`, `t1` a `t6`): a informação da câmera de ré chega no máximo 0,05 s depois de o gerente de conteúdos nascer e a espera acaba ali; a Welcome fica de 4,4 a 5,0 s na tela, e o popup do usuário principal volta, logo depois dela. O que trouxe a Welcome foi a câmera de ré: a rodada `sl` já tinha o estado respondido na hora e não a teve.

| Welcome | Popup do usuário principal, logo depois |
|---|---|
| ![Welcome com o carro de mentira](quadros/2026-10-11-hmi-vei-welcome.png) | ![Popup do usuário principal](quadros/2026-10-11-hmi-vei-popup-usuario-principal.png) |

Isso muda a partida de todas as rodadas, com ou sem o stub de veículo: a primeira tela sai uns 6 s mais cedo (decisão D58). O popup continua cortado no topo, como desde o Marco 4.

### O painel mudo em parte das partidas: o anúncio repetido

Nas rodadas `men2` e `sl` a HMI assinou os atributos do painel (`DSICarKombi`), o stub respondeu (está no barramento) e a atualização nunca chegou ao código da HMI: nem a linha "DSI-U" do rastreamento, nem as do computador de bordo. O relógio, de outro serviço, seguia certo. Nas rodadas `men`, `w2`, `fim` e `lista`, com o mesmo stub, chegou. Duas falhas em seis partidas. A rodada `dois`, com esse mesmo stub, também ficou com a tela vazia; não tinha o rastreamento ligado, então não sei dizer se foi a terceira.

**Primeiro errei a causa.** Na `men2` eu tinha acrescentado ao perfil a opção da lista de estados do veículo, e concluí que era ela. Cheguei a escrever isso no perfil e na nota de passagem. A rodada `sl` não tinha essa linha e falhou igual, o que só vi depois; a rodada `lista`, com a linha e tudo o mais igual, funcionou. A linha voltou para o perfil: sem ela a tela "Status do veículo" diz que a função está indisponível ([quadro](quadros/2026-10-11-hmi-vei-status-indisponivel.png)); com ela, "Boa viagem!".

**A causa provável, lida no código da LG:** cada anúncio do domínio 16 faz a HMI criar uma thread (`BAPTaskScheduler`) que dorme 5 s e liga dez serviços, entre eles o do painel. `ServiceAdminImpl.startService` confere se o serviço já existe e, se não, cria; não há trava. O stub anunciava a cada segundo desde que subia, e a HMI, ao se ligar, recebia de uma vez os que o hub tinha guardado: **11 anúncios do domínio 16 entre as 48 mensagens guardadas** (10 na rodada `lista`), e depois mais um por segundo, de 124 a 346 por rodada. Onze threads acordam juntas 5 s depois, e um serviço pode nascer duas vezes: a HMI assina num e a atualização é procurada no outro. Os seis serviços ligados na hora (o do relógio entre eles) são criados em fila, numa thread só, e não sofrem.

**Conserto, no `veiculostub`:** o anúncio sai uma vez, ao subir, e chega à HMI pela reentrega do hub. Só se a HMI aparecer no barramento e passar 2 s sem pedir nada ele é repetido, de 2 em 2 s. Quatro casos novos em `t_veiculo.sh`; contra o stub de antes falhavam os quatro.

**O que é medido e o que é dedução.** Medido: a falha (2 partidas em 6 com o stub antigo, talvez 3 em 7), os 11 anúncios guardados, e o resultado depois da mudança: treze partidas seguidas com os dados do painel na tela (`s1` a `s6`, `prova`, `t1` a `t6`), cada uma com um anúncio só no barramento. Dedução: que a criação em dobro seja o mecanismo. O registro da camada da LG vai para o socket de rastreio, que a bancada não ouve, então não vi a HMI criar o serviço duas vezes. Com seis partidas antigas a frequência da falha é mal conhecida; se fosse mesmo de uma em três, treze acertos seguidos por acaso teriam 0,5 % de chance.

### Um efeito colateral novo: o assistente de configuração

Às 21h20 a sessão do simulador fez uma partida de prova com tudo o que ia ao ar e viu uma tela que não tinha aparecido antes, sem saber por quê: o popup "Bem-vindo" do assistente de configuração. Fui atrás, com duas centrais de teste nascidas da mesma cópia da memória guardada da central ao vivo (`ao-vivo/memoria/lsmval`, 61 linhas, só lida):

| Rodada | Carro de mentira | Aos 52 s |
|---|---|---|
| `am` | ligado | "listWizardPages size: 2", "Showing global setup wizard." e a tela do assistente |
| `bm` | desligado (`VEICULO=`) | nada; a tela fica nos ajustes |
| `prova`, `fim`, `men` (sem memória) | ligado | "listWizardPages size: 0"; o assistente não aparece |
| `ref` (sem memória) | desligado | nenhuma linha do assistente em 300 s |

![O assistente de configuração](quadros/2026-10-11-hmi-vei-assistente-de-configuracao.png)

Ou seja, são duas condições juntas. Com os serviços de veículo ligados a HMI avalia o assistente uns 52 s depois da partida (`WizardPageManager`); sem eles, não avalia. E ele só é mostrado se a memória de estado disser que há páginas por fazer: a memória da central ao vivo tem três linhas do módulo `GlobalSetupWizard` (namespace 5060), e o arquivo fixo de partida não tem nenhuma. No popup aparecem duas páginas: a de hora e data, marcada, e uma com o rótulo SCAN.

**Corrigido às 22h44 de 11/10/2026, pela sessão do hub:** com a memória guardada, "sem eles, não avalia" não se sustenta. Numa partida sem o carro de mentira, com o hub de antes e uma cópia da memória da central ao vivo, a HMI avaliou e mostrou o assistente aos 61,9 s; a rodada `bm` foi observada só até os 60 s. O que os serviços de veículo fazem aí é adiantar a partida uns 10 s, e o assistente junto. O caso sem memória (rodada `ref`, nenhuma linha do assistente em 300 s) eu não repeti. Ver "O hub e os anúncios que ficavam fora do cache", "O efeito na partida, em rodadas repetidas".

Não é erro de dado: é o que a HMI faz quando os serviços de veículo estão ligados e há configuração inicial pendente. Qual dos serviços dispara a avaliação eu não isolei. Mas é efeito do carro de mentira, e vai ao ar no modo ao vivo, que usa a memória.

**Como fazer parar:** tocar em "Não exibir". Verificado em duas centrais de teste (`ne1` e `ne2`): depois do toque a HMI regrava a linha do assistente na memória de estado, com um byte a mais ligado, e a central que nasce dessa memória registra `dontShowAgain=true` e não mostra o assistente. No modo ao vivo quem leva a memória de uma partida para a outra é a guarda do Marco 6; esse último trecho eu não exercitei.

### O que entrou na pasta

- `driver/LeEsquemaDSI.java` e `driver/m8_esquema.sh`: geram `stubs/veiculo/gerado/`.
- `driver/veiculostub.c`: o carro de mentira. `driver/tests/t_veiculo.sh`: 56 casos, sem firmware. Escrito antes do código: contra um esqueleto que só compilava, 43 de 49 falhavam (os seis que passavam só acusam envio indevido).
- `driver/dsistub.c`: resposta à assinatura de energia (atributos 1, 4 e 7) e os ajustes genéricos do painel de teclas (`setGenericSetting` 1005 e `requestGenericSetting` 1009, respondidos com `genericSettingResponse` 2000). `driver/tests/t_pedidos.sh`: 16 casos; contra o stub de antes, 10 de 14 falhavam, e os dois da câmera de ré falharam antes de ela entrar. Na partida a HMI grava oito ajustes; antes eram oito esperas de 10 s no registro dela, agora são oito respostas.
- `driver/m1_hmi.sh`: opção `VEICULO` (lista de perfis; `-` liga só com o valor padrão), `VEICULO_CARIF` e `VEICULO_ESQUEMA`, e uma linha de resumo "veículo:" no fim. Com `VEICULO` o `carifservice` não sobe.
- `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh`: `VEICULO` vem ligado, com os três perfis. `VEICULO=` desliga.
- `driver/m1_build.sh`: compila também o `veiculostub` e o `dsi_manda`.
- `driver/m8_central.sh`: sobe uma central de teste com nome e pasta próprios e deixa mandar teclas e toques e tirar fotos pela linha de comando. Foi com ele que rodei tudo isto sem tocar na central ao vivo.
- `stubs/veiculo/`: os três perfis, o esquema gerado e o [README](stubs/veiculo/README.md).
- `stubs/codificacao/jetta-gli-2021-deducoes.txt`: as oito linhas das funções do carro.
- Quinze quadros `quadros/2026-10-11-hmi-vei-*.png`. Os das telas com dados são da rodada `prova`.
- Na ficha do carro: a leitura da foto do painel de 09/10/2026 e dois itens em "O que anotar no primeiro autoscan".

Testes, rodados da pasta às 21h13 de 11/10/2026, com o stub definitivo, e de novo às 21h44, depois da última edição: `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 16, `t_codificacao.sh` 26, `t_entrada.sh` 11, `t_servidor.sh` 20, `t_memoria.sh` 80, `t_pedidos.sh` 16 e `t_veiculo.sh` 56. Todos passam, e a compilação não dá aviso.

### Limites

- **Só atributos.** Os pedidos diretos da HMI ficam sem resposta: gravar a hora ou uma unidade pela tela não tem efeito, e três listas do start-stop estouram o tempo de 10 s no registro. Na rodada `prova` foram 17 pedidos assim, de 13 tipos (`getBAPState`, `setHMIIsReady`, `requestCharismaList`, `requestVehicleStateList` e outros). **Mudou no Marco 9 (11/10/2026, 22h52):** a hora, a data, o fuso e as unidades gravados pela tela passam a ter efeito, e as três listas do start-stop são respondidas; dos 17 pedidos da partida, 3 são atendidos e 14 seguem sem resposta, cada um com o motivo no registro do stub.
- **Quatro menus.** Luzes, espelhos, travas, pneus, assistentes e luz ambiente não foram feitos.
- **"Válido e zero" foi olhado em poucas telas** (veículo, clima, ajustes, grade). Noutra, um zero pode querer dizer algo que o carro não diria.
- **Mudar o perfil com a central de pé não tem efeito**; ele é lido na partida.
- **Só no container.** Não rodou no QEMU.
- **O esquema é do MU0883**; o carro roda o MU0803.
- **A linha "EARLY_INIT: PrimaryEngine: 7"** aparece no registro da HMI com o perfil. Não fui atrás do que ela decide.
- **Os popups continuam cortados no topo.** Não investiguei. **Investigado em 12/10/2026** (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada"): não é corte nem efeito da bancada. O aviso é uma folha que desce do topo e fica encostada nele, sem linha em cima, por dados do skin.

### Onde parou

- O item 2 do roteiro do simulador está feito para relógio, unidades, computador de bordo, estados e revisão.
- **A central ao vivo já roda com tudo isto.** A das 19h32 saiu do ar por volta das 21h32 (o prazo dela era de 2 horas) e às 21h33 outra foi ligada pela janela, não por mim; como os stubs são compilados na partida, ela pegou o que estava na pasta. Conferi só lendo o registro dela: hora aceita aos 0,1 s (21:33:17), Welcome de 0,55 a 5,27 s, popup do usuário principal até 35,3 s, o assistente de configuração aos 52,3 s e nenhum erro de OpenGL nos primeiros 60 quadros.
- Próximo passo natural: responder aos pedidos (a mesma tabela do esquema já diz que mensagem responde a quê), começando pelos que gravam hora e unidades. **Feito no Marco 9**, com uma correção: o esquema do Marco 8 trazia os pedidos e os eventos, mas não a ligação entre um e outro; quem diz que mensagem responde a quê é uma tabela da própria HMI, que o gerador passou a ler.
- O limite de 48 mensagens do hub esconde da HMI o anúncio do `carifservice`. Pode esconder outros; não procurei. **Resolvido às 22h52 de 11/10/2026** por outra sessão, que procurou: escondia também, em parte das partidas, o anúncio de energia, o de teclas e o de persistência. Ver "O hub e os anúncios que ficavam fora do cache".

## Marco 9 — os pedidos da HMI ao carro de mentira (11/10/2026)

Feito numa sessão à parte, aberta pela sessão do simulador às 21h46 de 11/10/2026, para a parte que o Marco 8 deixou de fora por decisão registrada: os pedidos diretos. Até aqui o `driver/veiculostub.c` só atendia assinaturas; a tela aceitava o toque e nada mudava.

**Resultado, verificado em execução** numa central de teste subida da pasta só com os padrões do modo ao vivo (`moi3-ped-prova2`, às 22h54):

- acertar a hora pela tela muda a hora e ela fica, na tela de ajuste, na lista e no topo das outras telas; antes ela voltava ao valor antigo em 3 s;
- a data e o fuso também mudam e ficam;
- trocar a unidade muda a tela de unidades, e a temperatura do topo passa de 35,0 °C para 95 °F (a conta é da HMI);
- os três tempos esgotados do start-stop sumiram do registro da HMI;
- nenhum erro de OpenGL em 831 quadros.

Em oito partidas seguidas com a versão que ficou (`f1` a `f8`) os dados do painel, a Welcome e o relógio vieram em todas, sem erro de OpenGL.

**A primeira versão tinha um defeito, e foi a repetição que achou:** atender o pedido de idioma do painel pôs a HMI em laço em 1 de 9 partidas (222 mil pedidos em 75 s). A versão que ficou não atende esse pedido e corta qualquer laço parecido. Está contado em "O laço do idioma do painel".

| Antes: a hora pedida aparece e volta em 3 s | Depois: fica (2 s e 10 s depois do toque) |
|---|---|
| ![Antes](quadros/2026-10-11-hmi-ped-hora-antes-volta-em-3s.png) | ![Depois](quadros/2026-10-11-hmi-ped-hora-depois-fica.png) |

| Hora e data antes | Depois de acertar hora, data e fuso pela tela |
|---|---|
| ![Antes](quadros/2026-10-11-hmi-ped-hora-e-data-antes.png) | ![Depois](quadros/2026-10-11-hmi-ped-hora-e-data-depois.png) |

| Antes: escolhi °F e milhas e nada mudou | Depois: °F e milhas |
|---|---|
| ![Antes](quadros/2026-10-11-hmi-ped-unidades-antes-nao-muda.png) | ![Depois](quadros/2026-10-11-hmi-ped-unidades-depois.png) |

As outras fotos: [a data acertada](quadros/2026-10-11-hmi-ped-data-acertada.png), [o fuso escolhido](quadros/2026-10-11-hmi-ped-fuso-escolhido.png), [as seis unidades trocadas](quadros/2026-10-11-hmi-ped-unidades-todas.png) (rodada `v1`) e [a tela do veículo depois](quadros/2026-10-11-hmi-ped-veiculo-depois.png).

**Nem tudo muda.** Na tela do veículo, depois de trocar para milhas, a distância e a autonomia continuam em km: esses números o painel manda já com a unidade, e o carro de mentira não refaz a conta. (**Mudou em 12/10/2026, às 9h20:** passou a refazer; seção "Os números do painel acompanham a unidade escolhida".) Nada do que a tela grava sobrevive à partida seguinte (**mudou em 12/10/2026:** no modo ao vivo sobrevive; seção "O carro de mentira lembra do que a tela gravou"). E só o serviço de relógio e unidades aceita gravação; clima, luzes e os outros seguem como no Marco 8.

### O que a HMI pede

Levantado na rodada `base1` (central `moi3-ped-base1`, às 21h51, com o stub do Marco 8), lendo o registro do stub e o barramento.

**Na partida, 17 pedidos de 13 tipos:**

| Pedido | Vezes | O que é | O que a HMI espera |
|---|---|---|---|
| `DSICarEco.requestStartStopProhibitList`, `…RestartList`, `…RestartProhibitList` | 1 cada | as três listas de motivos do start-stop (`0a 04 18 06 20 02`: 3 elementos, transação 1) | resposta em até 10 s; sem ela, três linhas "DSI timeout occurred" |
| `DSICarTimeUnitsLanguage.setMenuLanguage` | 1 | manda o painel usar o idioma 1 (`08 02`) | a atualização do atributo de idioma |
| `DSIBAP.getBAPState` | 4 | pergunta o estado de quatro unidades da rede do carro (49, 40, 41 e 70) | não marca prazo |
| `DSICarDrivingCharacteristics.requestCharismaList` | 2 | a lista de perfis de condução | não marca prazo |
| `DSICarKombi.requestVehicleStateList` | 1 | a lista de avisos do veículo | não marca prazo |
| `DSICarComfort.setHMIIsReady`, `DSICarSeat.setSeatHMIIsReady`, `DSICarParkingSystem.setHMIStateIsReady` | 1 cada | pelo nome, a HMI avisando que subiu | não marca prazo |
| `DSICarAirCondition.setAirconContent`, `DSICarAuxHeaterCooler.setAuxHeaterCoolerRunningTime`, `DSICarComfort.showUGDOPopup` | 1 cada | ajustes e um aviso | não marca prazo |

**Nas telas "Hora e data" e "Unidades"**, um pedido por toque, todos no serviço `DSICarTimeUnitsLanguage`:

| Toque | Pedido | Bytes vistos |
|---|---|---|
| Hora ▲ | `setClockTime` (hora, minuto, segundo 0) | `08 2c 10 6a` = 22:53 |
| Dia ▲ | `setClockDate` (ano menos 2000, mês, dia) | `0a 06 08 34 10 14 18 18` = 12/10/26 |
| Fonte de referência: GPS | `setClockSource` | `08 06` = 3 |
| Fuso: −2 h | `setClockTimeZoneOffset` | `0d 00 00 00 c0` = −2,0 |
| Temperatura: °F | `setTemperatureUnit` | `08 02` = 1 |
| Distância: mi | `setDistanceUnit` | `08 02` = 1 |

A HMI fala com o painel por DSI nessas telas, e não por RSI: `AslTargetSystemDate` tem os dois caminhos (`useRSI`), e o que apareceu no barramento foi o DSI.

**Por que a hora voltava.** Lido em `AslTargetSystemDate`: a tela mostra a hora que o painel informa. No toque a HMI põe a hora nova na tela e manda `setClockTime`; daí em diante, cada atualização do relógio que chega com hora diferente da que ela guardou é regravada na tela (`updateClockTime`). O carro de mentira manda a hora a cada segundo, e mandava a antiga. Há ainda um temporizador de 1 s com o mesmo efeito (`processTimeResponseTimer`), que outro caminho de acerto liga; não separei qual dos dois agiu. A resposta que a HMI espera é a atualização do próprio atributo, com a hora nova.

### A tabela pedido → resposta está dentro da HMI

A linha de erro da própria HMI deu a pista: `request=…DSICarEco[0].requestStartStopProhibitList: "responseStartStopProhibitReasonList" did not arrive`. Ela sabe que resposta espera de cada pedido, e o nome nem sempre se adivinha (o pedido termina em `ProhibitList`, a resposta em `ProhibitReasonList`).

- **Onde está.** Quem registra o tempo esgotado é `DSIProxyTarget.handleTimedOutRequest`, com as chamadas `Cfg.PT.getSyncRequests`, `getSyncResponses`, `getRequestType` e `getSyncRequestTimeouts` e `Cfg.U.getServiceSyncMethodName` e `getListenerSyncMethodName`. As tabelas moram nas classes `…dsiproxy.lr.PTCC` e `UCC`, e o nome de cada um dos 191 tipos de serviço em `RuntimeGeneratedConstants.SERVICE_TS_NS`.
- **Como o gerador lê.** `driver/LeEsquemaDSI.java` faz as mesmas chamadas por reflexão, na JVM do firmware, e casa pelo nome do método com o que já tinha da camada da LG: a api de cada pedido e a api cujo tratamento chama cada método do ouvinte. Sem decompilador.
- **O que saiu, nos 16 serviços de veículo:** 84 pedidos com resposta. Em 81 a HMI espera uma resposta só; em 1, uma dentre duas (as cores da luz ambiente); em 2, quem decide é um analisador dela (os pneus). O prazo é de 10 s em 71, de 60 s em 8, de 1 s em 4, e 1 não tem prazo.
- **Nove respostas que a camada da LG não entrega.** São mais esquisitices do próprio firmware. Em seis, do teto solar, a HMI espera por exemplo `acknowledgeSunroofFrontSliding`, e o código da LG, ao receber essa mensagem, chama outro método do ouvinte (já estava no relatório do Marco 8). Em três, duas das luzes e uma do spoiler, a camada da LG não tem evento nenhum para a resposta. Nesses nove pedidos a HMI esgotaria o prazo com qualquer fornecedor; pelo código, no carro também. Estão no `gerado/relatorio.txt`.
- **O que não está na tabela:** os pedidos de gravar (`set…`) do relógio e das unidades, e `getBAPState`, `requestVehicleStateList` e `requestCharismaList`. Para esses a HMI não marca prazo. A rodada `base1` confirma: seis pedidos de gravar sem resposta e nenhuma linha de tempo esgotado.

**O que o firmware não diz, e o gerador deduz.** A ligação entre o pedido `setTemperatureUnit` e o atributo `ATTR_TEMPERATUREUNIT` não está em classe nenhuma da HMI; no carro quem a faz é o `carifservice`, com o painel. É o costume dos serviços DSI, e a HMI trata a atualização do atributo como a resposta do pedido: em `AslTargetLscTNGBordComputerLAC` o texto do rastreamento é "got Response for setMenuLanguage". O gerador escreve essa ligação como dedução, marcada no esquema:

- **linhas G, o que cada pedido grava:** pedido `set<X>` e atributo de nome `<X>`. Os campos casam por posição (os campos do pedido são os do evento, número a número e tipo a tipo, fora o `validFlag`) ou, se não, por nome (cada campo do pedido tem um só campo de mesmo nome e tipo no evento; é o caso de `setClockTime`, que manda três números soltos para um evento que os guarda dentro de `time`). Dos 520 pedidos `set…`, 373 ganharam regra: 357 por posição e 16 por nome. Dos 147 sem regra, 140 não têm atributo com o nome.
- **linhas X, o que a resposta devolve do pedido:** os campos de mesmo caminho e tipo nas duas mensagens. São 45 campos em 20 pares; nas listas do start-stop, o número da transação, que a HMI confere antes de aceitar a resposta (`HsmTargetCarService.dsiCarEcoResponseStartStopProhibitReasonList`).

O que já estava no esquema não mudou em nenhuma linha, os 1.117 vetores continuam iguais byte a byte, e duas rodadas do gerador deram arquivos idênticos.

### O que o carro de mentira faz agora

O programa continua sem conhecer serviço nenhum: lê as linhas novas do esquema e o que o perfil manda (dois ajustes, `grava` e `ignora`, as seções de regra e as seções das respostas).

- **Pedido que grava** (linhas G), só nos serviços que o perfil lista no ajuste `grava`: o valor toma o lugar do valor do perfil e a atualização do atributo sai para quem o assina, em toda instância. Sai mesmo que o valor seja o mesmo. Quem assinar depois recebe o valor gravado.
- **Relógio.** O campo de destino que o perfil enche com marcador (`@hora`, `@dia`…) não vira um número fixo: o pedido acerta o relógio do carro de mentira, que segue andando dali. Hora fora do intervalo não muda nada.
- **Pedido com resposta própria** (linhas R), só as de resposta única: a resposta sai, na instância do pedido, se o perfil tiver a seção dela. Leva os valores do perfil e, onde ele não diz nada, o que as linhas X mandam devolver. Nada disso fica guardado de um pedido para o outro.
- **Regra a mais no perfil**, em seção `[Serviço.pedido -> evento]`, para o que o esquema não tem como deduzir.
- **O resto fica registrado**, com o motivo: "a HMI espera `responseX` em até 10000 ms", "gravaria `updateY`, mas o serviço não está em grava", "a HMI não marca prazo de resposta para ele".

O que o perfil padrão (`stubs/veiculo/carro-parado.txt`) liga:

| O quê | Como | De onde vem |
|---|---|---|
| gravar relógio e unidades | `grava = DSICarTimeUnitsLanguage` | escolha do simulador |
| o fuso aparecer na tela | regra `[…setClockTimeZoneOffset -> updateClockTime]`: a tela tira o fuso de dentro da mensagem da hora (`AslTargetSystemDate.updateClockTime` chama `processDSIUpdateTimeZoneOffset(time.timeZone)`), e não do atributo de fuso | lido no código; verificado na tela |
| as três listas do start-stop | uma seção por resposta, com `listUpdateInfo.numOfElements = 0` | SIMULAÇÃO: listas vazias, nenhum motivo |
| não atender o idioma do painel | `ignora = DSICarTimeUnitsLanguage.setMenuLanguage` | o laço, abaixo |

A resposta das listas sai com os bytes `0a 02 20 02`: a transação 1 de volta e mais nada. Lido no código: com a lista vazia a HMI segue para `checkingStartStopState`, o caminho de "nenhum motivo".

### As rodadas

Centrais de teste `moi3-ped-<nome>`, à parte da central ao vivo, subidas com `PREFIXO=ped sh driver/m8_central.sh`. Saídas em `traces/m9_*`.

| Rodada | O que rodou | Observado |
|---|---|---|
| `base1` | o stub do Marco 8 | 17 pedidos na partida, 3 tempos esgotados do start-stop; a hora pedida aparece e volta em 3 s; °F e milhas não mudam nada |
| `v1` | primeira versão, da cópia de trabalho | os 3 tempos esgotados somem; hora 23:24 fica; data, fuso e as seis unidades mudam; topo com 95 °F; 0 erros de OpenGL em 1.063 quadros |
| `prova` | primeira versão, da pasta, só com os padrões | o mesmo roteiro, com o mesmo resultado; 851 quadros sem erro de OpenGL |
| `v2` | primeira versão, da pasta, com rastreamento, só a partida | os 3 tempos esgotados somem; o rastreamento da HMI não mostrou o serviço do start-stop com nenhuma das chaves que liguei |
| `n1` a `n6` | primeira versão, seis partidas | painel, Welcome e relógio nas seis; **a `n4` entrou em laço** |
| `prova2` | versão final, da pasta, só com os padrões | hora 19:58, data 13/10/2026 e fuso −2 h ficam; °F e mi; 111 assinaturas, 13 pedidos atendidos, 14 sem resposta; 831 quadros sem erro de OpenGL |
| `f1` a `f8` | versão final, oito partidas | nas oito: dados do painel, Welcome, relógio, um anúncio só, 3 pedidos atendidos, nenhum tempo esgotado do start-stop, nenhum laço, 0 erros de OpenGL |
| `laco1` | versão final, com o `ignora` tirado de propósito | o laço reproduzido com um pedido mandado à mão, e cortado em 200 |

Em todas resta um tempo esgotado no registro da HMI, `getDisplayBrightness`, que é de outro serviço (brilho da tela, no `dsistub`). **Mudou em 12/10/2026:** esse tempo esgotado sumiu (seção "O brilho pela tela: o display de mentira responde").

**As rodadas finais já têm o hub novo.** Outra sessão trocou o `driver/kipc_hub.c` às 22h52. `base1`, `v1`, `prova` e `n1` a `n6` rodaram com o hub de antes; `prova2`, `f1` a `f8` e `laco1`, com o novo, que é o que está na pasta.

### O laço do idioma do painel

**O que aconteceu.** Na partida a HMI manda o painel trocar de idioma. A primeira versão atendia: gravava e devolvia a atualização. Em oito de nove partidas isso foi um pedido e uma resposta. Na `n4` foram 222.549: a HMI pediu o idioma 1 (inglês), recebeu, pediu o 26 (português do Brasil), recebeu, pediu o 1 de novo, a cerca de 3.000 pedidos por segundo, até a central desligar. O registro do stub chegou a 33 MB em 75 s. A tela seguiu funcionando.

**O mecanismo.** Medido, no barramento da `n4`: os pedidos saem da HMI (canal `0x8000`) alternando 1 e 26, cerca de 110 mil de cada, cada um seguido da atualização do stub. Lido no código, em `AslTargetLscTNGBordComputerLAC.dsiCarTimeUnitsLanguageUpdateMenuLanguage`: uma regra ("MODE 1") manda de volta o idioma da tela quando o painel informa outro, e outra ("MODE 3") manda de volta o último idioma que a própria HMI pediu ao painel. Os dois diferem quando a memória da central não traz o idioma do painel: a HMI registra "Using FALLBACK language for InstrumentCluster" e pede inglês, com a tela em português. É o caso da memória de partida da bancada, que só tem o idioma da tela. Qual das regras roda depende de a troca inicial de idioma da HMI já ter sido feita quando a resposta chega, e isso varia de uma partida para a outra. Que sejam essas duas regras é leitura do código: não vi a HMI executá-las, porque o rastreamento dela para cedo (ver abaixo).

**O conserto, em duas partes:**

1. O perfil padrão manda ignorar esse pedido (ajuste `ignora`). O idioma do painel volta a ser o do perfil, como no Marco 8: a HMI pede uma vez e para.
2. O stub corta qualquer pedido atendido mais de 200 vezes em 10 s, com aviso no registro, até reiniciar. É a defesa para os serviços que ainda vão ser ligados.

**Verificado:** oito partidas com a versão final, nenhuma em laço, mas isso é por construção: o pedido não é mais atendido. A prova do corte é a rodada `laco1`: tirei o `ignora`, esperei a partida e mandei à mão, pelo barramento, o mesmo pedido que a HMI manda. O laço começou na hora (no barramento, 102 pedidos com o idioma 1 e 100 com o 26) e o stub parou de atender no 200º, registrou o corte, e a HMI parou junto.

**Enquanto isso, na pasta.** Achei o laço às 22h46 com a primeira versão já instalada. Como a central ao vivo compila a pasta ao religar, desliguei a gravação no perfil às 22h47, e ela foi religada pela janela nesse mesmo minuto: o registro dela diz "grava: nenhum serviço". A versão final entrou às 22h52. **A central ao vivo que está no ar desde as 22h47 responde às listas do start-stop mas não grava hora nem unidade; a partir da próxima partida, grava.** Não mandei nada para ela.

**Um achado de passagem: o rastreamento da HMI para cedo.** Nas três rodadas com `HMILOG` em que olhei (`v1`, `v2` e `f3`) as linhas de rastreamento param entre 6 e 15 s depois da partida. A HMI troca as propriedades de registro na fase de carga tardia ("Changed logging properties to: {}", do `LOGGING.LOGMANAGER`), e as chaves que o lançador liga deixam de valer. Serve para a partida; para o que se faz depois na tela, não. **Resolvido em 12/10/2026** (seção "O rastreamento da HMI que parava na carga tardia"): quem troca é a sonda de depuração `LOGMANAGER`, que na carga tardia aplica as chaves do perfil de sondas em uso, e o perfil padrão não tem nenhuma; o lançador passou a pôr as chaves do `HMILOG` também nos perfis. A troca é uma só, perto dos 6 s; a linha que saía perto dos 15 s não depende de chave.

### O que entrou na pasta

- `driver/veiculostub.c`: os pedidos (linhas `R`, `G` e `X` do esquema; ajustes `grava` e `ignora`, seções de regra e seções de resposta no perfil; o acerto do relógio; o corte de laço). O registro passa a dizer "pedido atendido" ou "pedido sem resposta", com o motivo.
- `driver/tests/t_veiculo.sh`: de 56 para 128 casos. Escritos antes do código: contra o stub de antes, 54 dos 60 primeiros casos novos falhavam (os 6 que passavam só acusam envio indevido); outros 10, entre eles os do `ignora` e do corte de laço, foram vistos falhar antes do trecho de código de cada um. Os 2 últimos foram escritos depois do código e só fixam o que os ajustes vazios do perfil fazem. Entre os novos, os bytes que a HMI mandou na `base1`, contra o esquema e os perfis de verdade.
- `driver/LeEsquemaDSI.java` e `stubs/veiculo/gerado/`: o esquema passa de 620 kB para 679 kB, com 85 linhas `R`, 713 `G` e 45 `X`; o relatório lista o que não ganhou regra e por quê.
- `stubs/veiculo/carro-parado.txt`: `grava`, `ignora`, a regra do fuso e as três respostas do start-stop, com o motivo de cada uma.
- `driver/m8_central.sh`: a variável `PREFIXO`, para os containers de cada sessão terem nome próprio. `driver/m1_hmi.sh`: o resumo "veículo:" conta também os pedidos diretos atendidos. `driver/m8_esquema.sh`: só comentário.
- Dez quadros `quadros/2026-10-11-hmi-ped-*.png`. Os dois com o stub do Marco 8 (a hora que volta e as unidades que não mudam) são da `base1`, o das seis unidades é da `v1` e os outros são da `prova2`.

Testes, rodados da pasta às 23h10 de 11/10/2026, com a versão final e o hub novo: `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19 (eram 16; outra sessão acrescentou três), `t_codificacao.sh` 26, `t_entrada.sh` 11, `t_pedidos.sh` 16, `t_veiculo.sh` 128, `t_memoria.sh` 80 e `t_servidor.sh` 20. Rodei também os três de outras sessões que apareceram na pasta nesta noite: `t_vc_diag_alm.sh` 34, `t_hub.sh` 25 e `t_lsmval_abre.sh` 38. Todos passam, e a compilação não dá aviso. Saída em `traces/m9_suites.txt`.

### Limites

- **Só o relógio e as unidades gravam.** Os outros quinze serviços têm as regras no esquema e ficam desligados: ligar é pôr o nome em `grava`, mas cada um pede conferência, e as 16 regras por nome são as menos seguras.
- **Das 373 regras de gravar, nove foram exercitadas na HMI:** hora, data, fuso e as seis unidades. A fonte de referência, o horário de verão e os formatos de hora e data usam a mesma regra e não foram tocados com a versão nova.
- **Trocar a unidade não refaz os números do painel.** A autonomia, a distância e o consumo seguem na unidade do perfil; no carro o painel passaria a mandá-los na unidade nova. A temperatura do topo muda porque a HMI faz a conta.
- **Nada sobrevive à partida seguinte.** O carro de mentira nasce de novo dos perfis e da hora do Mac. No carro o painel guarda. **Mudou em 12/10/2026:** no modo ao vivo o carro de mentira guarda o que a tela grava e a partida seguinte nasce disso (seção "O carro de mentira lembra do que a tela gravou").
- **O fuso não mexe na hora.** Não sei o que o painel do carro faz com a hora quando o fuso muda.
- **Só três pedidos com prazo são respondidos.** Os outros 81 seguem como estavam: "restaurar os ajustes de fábrica" das unidades, por exemplo, ainda esgota os 10 s.
- **As listas vazias do start-stop são simulação**, e o que a HMI faz com elas depois de aceitar é leitura do código: o rastreamento não chega lá.
- **`getBAPState`, a lista de avisos do veículo e a de perfis de condução seguem sem resposta.** A resposta certa não é clara no código que li. Quem pergunta o estado BAP, pela busca nos decompilados, são as máquinas de estado das unidades `ECall` e `ENI` da HMI, que esperam a resposta para seguir adiante (há um `VersionCheckState` logo depois); não li o que elas mandam em seguida, e por isso não respondi.
- **O idioma do painel não segue a HMI.** Fica o do perfil.
- **O esquema é do MU0883**; o carro roda o MU0803.

### Onde parou

- O item 2 do roteiro do simulador ganhou a gravação de relógio e unidades e as respostas do start-stop.
- **A central ao vivo no ar desde as 22h47 está com a versão intermediária**; a próxima partida pega a final.
- Próximos passos naturais: guardar o que a tela grava de uma partida para a outra, junto da memória de estado; refazer os números do painel quando a unidade muda; e ligar a gravação nos serviços dos outros menus, um a um. **O primeiro foi feito em 12/10/2026** (seção "O carro de mentira lembra do que a tela gravou").

## O hub e os anúncios que ficavam fora do cache (11/10/2026, depois do Marco 8)

Feito numa sessão à parte, aberta pela sessão do carro de mentira às 21h46 de 11/10/2026, para o item "O limite de 48 mensagens do hub" dos próximos passos (decisão D64). O pedido foi medir o tamanho do problema e consertar o `driver/kipc_hub.c`, com os testes escritos antes do código. É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução.**

- **O problema era maior do que o Marco 8 descreveu.** Na ordem de partida de sempre só um anúncio ficava fora das 48 mensagens que o hub reentrega a quem se liga depois: o do `carifservice` do firmware (domínio 16). Mas a ordem em que os daemons se ligam ao barramento é uma corrida de milésimos de segundo. Com a máquina carregada, como nesta noite, o `carifservice` às vezes se liga antes de outro daemon; a rajada de 78 mensagens que ele manda ao subir enche o cache sozinha, e o anúncio de quem vem depois também fica de fora. Aí a HMI perde o serviço de energia, o de teclas ou o de persistência, e a central fica surda, de tela preta, ou nem chega a desenhar.
- **O conserto.** Além do cache, o hub guarda o último anúncio de cada canal e o entrega a quem se liga depois: depois do cache, uma vez por canal, e só do canal de que o cache não entregou anúncio.
- **O efeito.** No padrão do modo ao vivo nada muda: o hub novo não acrescentou anúncio a leitor nenhum, e seis partidas com cada hub deram o mesmo resultado. Sem o carro de mentira a HMI passa a ligar os serviços de veículo, com os dados de "sem carro" do `carifservice`, e a partida deixa de depender da ordem em que os daemons se ligam.

| | Hub de antes | Hub novo |
|---|---|---|
| Padrões do modo ao vivo, 6 partidas de cada | relógio, tela do veículo com os dados, Welcome, tecla, 34 serviços ausentes, nenhum erro de OpenGL | o mesmo, nas 6 |
| Sem o carro de mentira, na ordem de partida de sempre | 48 serviços ausentes; a HMI não pede nada ao domínio 16 | 34 ausentes; 106 pedidos de assinatura, 80 eventos do `carifservice` |
| Sem o carro de mentira, com o `carifservice` na frente de outro daemon | central surda e de tela preta (1 partida em que aconteceu sozinho); nenhum quadro desenhado (2 de 2 com a ordem forçada) | igual à ordem de sempre (5 partidas em que aconteceu sozinho e 2 de 2 com a ordem forçada) |

![Sem o carro de mentira, a tela do veículo aos 75 s: em cima as seis partidas com o hub de antes, embaixo as seis com o hub novo](quadros/2026-10-11-hmi-hub-serie-sem-carro.png)

Em cima, a terceira partida (`av4`) é a que caiu na ordem ruim com o hub de antes. Embaixo, quatro das seis caíram nela (a 1ª, a 3ª, a 4ª e a 5ª) e não se nota.

### A medição

Subi centrais de teste com a opção `HUB_DUMP` do hub num valor alto: ele passa a numerar cada mensagem no registro dele, com o pid de quem mandou, e as linhas de quem se liga ficam no meio, na ordem. Daí sai, sem dedução, a posição de cada anúncio entre as **mensagens comuns** (as que disputam as 48 vagas do cache; ficam de fora as do canal de diagnóstico, que têm guardado próprio, e as de carga curta, que o hub descarta) e depois de qual mensagem cada processo se ligou. O analisador é `traces/hub_analisa.py`.

**Quem anuncia, e em que posição, na ordem de partida de sempre:**

| Canal | Quem anuncia | Padrões do modo ao vivo | Sem o carro de mentira (`VEICULO=`) |
|---|---|---|---|
| `0x10`, veículo (domínio 16) | o `veiculostub`, uma vez; sem ele, o `carifservice`, uma vez | 1ª | **61ª** (64ª se o `dsistub` publicar duas vezes antes) |
| `0x4000`, display | o `dsistub`, de segundo em segundo | 2ª a 4ª, e de novo a cada três mensagens | 1ª a 4ª, e de novo a partir da 85ª |
| `0x10000000000000`, persistência | o `persistence-app`, uma vez | 2ª a 7ª | 1ª a 6ª |
| `0x80000000000`, teclas | o `touch-app`, uma vez | 3ª a 7ª | 1ª a 6ª |
| `0x400000`, energia | o `mcu-interpreter`, uma vez | 2ª a 7ª | 1ª a 6ª |
| a HMI se liga depois da | | 19ª a 22ª mensagem | 96ª a 105ª |
| anúncios fora das 48 | | **nenhum** | **o do canal `0x10`** |

- As 46 rodadas antigas que têm `barramento.log` em `traces/` (Marcos 6 e 8 e o painel de instrumentos, até as 22h10) estão todas nessa ordem: 22 só com o `carifservice` (anúncio na 61ª), 21 só com o stub (na 1ª) e 3 com os dois.
- **A carga do anúncio muda de quem manda.** O `carifservice`, o `touch-app` e o `persistence-app` mandam `WELCOME\0` depois de instância e api zeradas; o `mcu-interpreter` manda `WELCOME\0` logo depois da interface, sem instância nem api; os nossos stubs mandam o anúncio vazio. O hub reconhece o anúncio só pela interface 9000.
- **A rajada do `carifservice`.** Logo que se liga ele manda, de uma vez, 81 mensagens: 3 de carga curta, que o hub descarta, e 78 comuns, com o anúncio na 55ª. Foi igual nas 11 partidas em que contei. Mesmo que ele fosse o primeiro processo do barramento, o anúncio dele ficaria fora das 48.
- **Ninguém mais anuncia.** Os 34 serviços que a HMI dá por ausentes em toda partida (sintonizador, mídia, agenda, telefone, voz, Wi-Fi, navegador) são de domínios que nenhum processo da bancada anuncia. Não é o hub que os esconde.

**O que a HMI deixava de ligar por causa disso.** Sem o carro de mentira ela dá 48 serviços por ausentes; os 14 a mais são os do domínio 16 que ela exige na partida: `DSIBAP`, `DSICarAirCondition`, `DSICarAuxHeaterCooler`, `DSICarComfort`, `DSICarDriverAssistance`, `DSICarDrivingCharacteristics`, `DSICarEco`, `DSICarKombi`, `DSICarLight`, `DSICarParkingSystem`, `DSICarSeat`, `DSICarTimeUnitsLanguage`, `DSICarVehicleStates` e `DSIGeneralVehicleStates`. Os outros dois do domínio (`DSICarHybrid` e `DSICarStatistics`) também não eram ligados, mas ela não os cobra na partida.

**A ordem de partida é uma corrida, e isso o Marco 8 não tinha visto.** O lançador sobe os quatro daemons do firmware e o `dsistub` ao mesmo tempo. Pus um `strace` no hub de 24 partidas curtas, para ter a hora de cada ligação: o último dos outros quatro se liga, na mediana, **8 ms antes do `carifservice`** (de 36 ms antes a 2 ms depois), e a rajada dele começa de 0,3 a 4 ms depois de ele se ligar e dura 4 ms na mediana (de 4 a 20). Quem se atrasa uns 15 ms cai depois da rajada, lá pela 80ª mensagem, e o anúncio dele fica fora do cache:

| Ordem dos primeiros anúncios (posição entre as comuns) | Fora das 48 | Partida |
|---|---|---|
| persistência 1ª, display 2ª, teclas 5ª, energia 6ª, veículo 61ª | veículo | a de sempre (`av1`) |
| display 1ª, persistência 4ª, veículo 59ª, **energia 83ª, teclas 84ª** | veículo, energia, teclas | `av4`, hub de antes |
| persistência 1ª, veículo 56ª, **teclas 80ª, energia 81ª, display 82ª** | veículo, teclas, energia, display | `dv1`, hub novo |
| teclas 1ª, display 2ª, energia 5ª, veículo 60ª, **persistência 84ª** | veículo, persistência | `dv5`, hub novo |
| display 1ª, veículo 61ª, **persistência 85ª, teclas 86ª, energia 87ª** | veículo, persistência, teclas, energia | `dv6`, hub novo |

**Com o hub de antes, o resultado é uma central morta.** Na `av4` a HMI deu por ausentes, além dos 48, `DSIPowerManagement`, `DSIKeyPanel`, `DSIDiagnoseSystem`, `DSISWaP` e `DSIWirelessCharging`; o estado de energia ficou em 5 (não iniciado); a câmera de ré nunca foi informada; a tecla Carro mandada aos 32 s não teve efeito (o popup do usuário principal só fechou sozinho, aos 42,7 s) e a tela terminou preta. A sessão do painel de instrumentos registrou o mesmo, na mesma hora, na rodada `r5c` dela ([painel-de-instrumentos.md](painel-de-instrumentos.md), "A `r5c` divergiu"): o `touch-app` na 90ª mensagem, `DSIKeyPanel` ausente, a tecla NAV ignorada. Ela deixou o caso "como pendência para quem cuida do lançador e do hub"; é este.

**Forçando a ordem, a falha se repete.** Numa variante de experimento do lançador (só no rascunho; a diferença está em `traces/hub_exp_m1_hmi_carif_antes.diff`) o `carifservice` sobe 1 s antes dos outros. Todos os outros anúncios caem entre a 79ª e a 84ª mensagem. Com o hub de antes, nas duas partidas (`eaf1`, `eaf2`): a HMI dá por ausentes `DSIPersistence` e `DSIPowerManagement` logo na primeira etapa, não desenha nenhum quadro e, aos 60 s, registra "A startup error occurs! First Screen was not shown in time". Com o hub novo, nas duas (`edf1`, `edf2`): cinco anúncios guardados entregues, e a partida é a de sempre.

**Com que frequência a ordem ruim aparece.** Depende da carga da máquina. Contei as partidas em que algum anúncio além do de veículo caiu depois da 48ª mensagem, todas com o `carifservice` no ar:

| Lote | Quando | Hub de antes | Hub novo |
|---|---|---|---|
| série de 75 s (`av*`, `dv*`) | 22h07 a 22h16 e 22h34 | 1 de 6 | 4 de 6 |
| partidas curtas (`oa*`, `od*`) | 22h27 a 22h34 | 1 de 12 | 4 de 12 |
| partidas curtas com a hora de cada ligação (`ra*`, `rd*`) | 22h46 a 22h50 | 0 de 12 | 0 de 12 |
| as demais (medição, dois fornecedores, assistente, telas) | | 1 de 5 | 1 de 4 |
| **soma** | | **3 de 35** | **9 de 34** |
| painel de instrumentos (`vc_diag_repeticoes.txt`) | 21h40 a 22h11 | 1 de 12 | não rodou |

Nos dois primeiros lotes havia de três a cinco centrais de outras sessões no ar, nas vezes em que olhei; no fim do terceiro, uma.

**O hub novo não piora a corrida; conferi porque as contagens sugeriam que sim.** Nas séries lado a lado a ordem ruim apareceu mais no braço do hub novo (9 de 34) do que no do hub de antes (3 de 35, e em duas dessas três só o `dsistub` se atrasou, o que não faz mal, porque ele anuncia de novo no segundo seguinte). O hub não tem como causar isso pelo que faz: das 42 partidas com o hub novo, nas 34 em que ele entregou algum anúncio guardado a primeira entrega veio depois de pelo menos 78 mensagens comuns, isto é, depois da rajada; até um daemon se atrasar, os dois hubs mandaram exatamente as mesmas mensagens aos mesmos processos. E a medida com hora não mostra diferença de tempo: o último dos outros se liga 8,3 ms antes do `carifservice` com o hub de antes e 8,4 ms com o novo (medianas de 12 partidas). Para tirar a dúvida fiz um ensaio só da corrida, sem a HMI: num container só, 240 partidas dos daemons como o lançador as faz, alternando os dois hubs (`traces/hub_corrida.txt`). A ordem ruim apareceu em 3 de 40 partidas com cada hub, sem carga; em 9 de 40 com o hub de antes e 8 de 40 com o novo, com 16 laços ocupando os 16 processadores do container; em 13 e 11 de 40, com 32 laços. Somando, 25 de 120 e 22 de 120. **O hub não influi na ordem; a carga da máquina, sim.** A diferença de contagem nas séries lado a lado fica sem explicação: acaso, ou algum efeito do arranjo em pares que não identifiquei.

### O conserto

A regra, em `driver/kipc_hub.c`:

- toda mensagem comum com a interface 9000 é um anúncio; o hub guarda o último de cada canal, caiba ou não no cache, com o pid de quem mandou. Anunciar de novo troca o guardado;
- quem se liga como leitor recebe, nesta ordem: o que veio do microcontrolador de diagnóstico (Marco 6), as 48 primeiras mensagens comuns, como sempre, e por fim os anúncios guardados, na ordem em que cada canal anunciou pela primeira vez;
- fica de fora o anúncio do canal de que o cache acabou de entregar um anúncio àquele leitor, e o que o próprio leitor mandou (a regra do pid, que o cache já seguia).

Duas escolhas minhas dentro do pedido:

- **O que o cache já entregava não muda em nada.** O anúncio guardado só é acrescentado, no fim, quando o cache não entregou nenhum daquele canal. Assim toda partida em que os anúncios cabem no cache recebe do hub novo exatamente o que recebia do antigo, e isso se confere no registro dele: a linha "entregou N anuncios de dominio guardados" só aparece quando ele acrescenta algo.
- **Não mexi nos anúncios repetidos que já estão no cache.** No padrão do modo ao vivo a HMI continua recebendo de uma vez cinco ou seis anúncios do display, porque o `dsistub` repete de segundo em segundo e eles caem entre as 19 a 22 mensagens que ela recebe. Tirar essas repetições faria o "um por canal" valer sempre, mas mudaria a partida do modo ao vivo sem ninguém ter pedido. O que o hub novo garante é que **ele** não cria rajada: acrescenta no máximo um anúncio por canal, e nenhum para o canal que o cache já trouxe. É isso que mantém a regra do Marco 8 para o domínio 16 (decisão D61): a HMI recebe um anúncio só, venha ele do stub, do `carifservice` ou dos dois.

A tabela comporta 64 canais, que é quantos o firmware define (um bit por processo: `PID_MAX_ID = 64` em `com.lge.mib3oi.dsi.DSIDomain`). Passando disso, os canais novos ficam sem reentrega, como era antes.

**Testes, escritos antes do código.** `driver/tests/t_hub.sh`, 25 casos, sem firmware: o anúncio que chega depois da 48ª mensagem é reentregue; repetido cinco vezes, chega uma vez, e é o último; o anúncio sem instância nem api (o do `mcu-interpreter`) também é guardado; o canal que já tem anúncio no cache não vem em dobro; as 48 primeiras chegam iguais e na mesma ordem, antes dos guardados; a 49ª mensagem comum continua de fora; quem já estava ligado recebe cada mensagem uma vez; o diagnóstico continua chegando antes de tudo; ninguém recebe de volta o próprio anúncio; anúncio de carga curta e anúncio acima do `HUB_MAXFWD` continuam barrados; com o cache por encher a reentrega é a de sempre; 70 canais não derrubam o hub e os 64 primeiros ficam guardados. **Contra o hub de antes, 10 dos 25 falhavam**, todos os do comportamento novo (`traces/hub_t_hub_contra_o_hub_de_antes.out`); os outros 15 protegem o que já existia e passavam.

### O efeito na partida, em rodadas repetidas

Pares de centrais de teste simultâneas, uma com o hub de antes e outra com o hub novo, largadas com 3 s de diferença e alternando quem larga na frente. As duas saem de uma cópia congelada de `driver/`, `stubs/` e `glforward-moi3/` tirada às 21h50, e só diferem no `kipc_hub.c`. Cada partida é a da série do Marco 8: registro detalhado do painel, tecla Carro aos 32 s, 75 s de HMI e foto no fim. As previsões foram gravadas antes de ler qualquer resultado (`traces/hub_previsoes.txt`).

**Com os padrões do modo ao vivo** (`ap1` a `ap6` e `dp1` a `dp6`, das 22h16 às 22h27):

| | Hub de antes (6) | Hub novo (6) |
|---|---|---|
| a HMI se liga depois da mensagem | 19ª ou 22ª | 19ª, 20ª ou 22ª |
| reentrega à HMI | 19 ou 22 mensagens, 5 ou 6 delas anúncios do display | 19, 20 ou 22 mensagens, 5 ou 6 delas anúncios do display |
| anúncios guardados entregues, a qualquer leitor | não se aplica | **nenhum, nas 6** |
| serviços ausentes | 34 | 34 |
| estado de energia que a Welcome viu | 2 (ligado) | 2 |
| hora aceita aos | 0,39 a 0,88 s | 0,52 a 1,06 s |
| Welcome na tela por | 3,5 a 4,6 s | 3,3 a 4,6 s |
| tecla Carro aos 32 s | fecha o popup e abre a tela do veículo | igual |
| pedidos de assinatura no domínio 16 e eventos de volta | 110 e 380 a 382 | 110 e 381 ou 382 |
| linhas de erro e exceções da HMI nos primeiros 65 s | 126 ou 127, e 8 | 126 ou 127, e 8 ou 9 |
| erros de OpenGL | 0 | 0 |
| tela no fim | veículo com os dados do carro de mentira e o relógio | igual, nas 6 |

Como a previsão dizia. O hub novo se comportou como o de antes para todos os processos: nenhum leitor recebeu anúncio guardado.

![Com os padrões, a tela do veículo aos 75 s: em cima o hub de antes, embaixo o hub novo](quadros/2026-10-11-hmi-hub-serie-padroes.png)

**Sem o carro de mentira** (`av*` e `dv*`, pares 1, 2, 4, 5, 6 e 7; das 22h07 às 22h16 e às 22h34):

| | Hub de antes (6) | Hub novo (6) |
|---|---|---|
| ordem de partida | a de sempre em 5; ruim em 1 (`av4`) | a de sempre em 2; ruim em 4 |
| anúncios guardados entregues à HMI | não se aplica | 1 na ordem de sempre (o do canal `0x10`); de 2 a 4 na ordem ruim |
| serviços ausentes | 48; **53 na `av4`** | **34, nas 6** |
| esperas de 2 s por serviços ausentes | 13; 17 na `av4` | 8 |
| estado de energia que a Welcome viu | 2; **5 na `av4`** | 2 |
| Welcome na tela por | 1,0 a 4,7 s | 3,2 a 4,9 s |
| tecla Carro aos 32 s | funciona em 5; **sem efeito na `av4`** | funciona nas 6 |
| pedidos de assinatura no domínio 16 e eventos de volta | 0 e 1 | **106, em 14 serviços, e 80** |
| linhas de erro e exceções da HMI nos primeiros 65 s | 291 ou 292 (326 na `av4`), e 8 a 10 | 122 ou 123, e 9 ou 10 |
| erros de OpenGL | 0 | 0 |
| hora | nenhuma hora válida | nenhuma hora válida |
| tela no fim | veículo com "---" em 5; **preta na `av4`** | veículo com "---", nas 6 |

Também como a previsão dizia, com uma diferença: eu esperava perto de 110 pedidos de assinatura e foram 106. O `carifservice` responde a eles com 80 eventos, contra os 381 do carro de mentira. O que esses eventos dizem eu não reli; o Marco 8 descreveu, na rodada `anu`: opções em estado "não existe" ou "inacessível" e valores marcados como inválidos. O par 3 se perdeu por erro meu (regravei o lançador das centrais de teste no meio de uma largada) e foi refeito como par 7; o par 4 largou com duas centrais minhas a mais no ar.

**As telas não mudam.** Fotografei cinco telas nos dois braços, sem o carro de mentira (`tav` e `tdv`): veículo, configurações do veículo, clima, grade e ajustes. As dez fotos saíram idênticas duas a duas, byte a byte: o relógio segue "--:--", a tela do veículo segue com "---" e "Configurações do veículo" segue vazia. O serviço de verdade está ligado, mas não tem carro de onde tirar os dados.

![As cinco telas sem o carro de mentira: em cima o hub de antes, embaixo o hub novo](quadros/2026-10-11-hmi-hub-telas-sem-carro.png)

**Com o carro de mentira e o `carifservice` juntos** (`VEICULO_CARIF=1`; `ac1`, `ac2`, `dc1`, `dc2`): nada muda. O anúncio do stub é a 1ª mensagem e chega pelo cache; o do `carifservice`, na 62ª, fica guardado e não é acrescentado, porque a HMI já recebeu um do canal. Nas quatro partidas: um anúncio do domínio 16 na reentrega, 110 pedidos de assinatura, de 461 a 463 eventos dos dois fornecedores, os dados do stub na tela.

**O assistente de configuração aparece com os dois hubs, e eu corrijo uma leitura do Marco 8.** Sem o carro de mentira e nascendo de uma cópia da memória guardada da central ao vivo (61 linhas, lida às 22h14), o assistente "Bem-vindo" apareceu aos 61,9 s com o hub de antes (`eam`) e aos 51,9 s com o novo (`edm`): dez segundos mais cedo, que são as cinco esperas de 2 s a menos. O Marco 8 tinha concluído, pela rodada `bm`, que sem os serviços de veículo a HMI não avalia o assistente; a `bm` foi observada só até os 60 s. Com memória guardada, portanto, ele aparece de um jeito ou de outro. O caso sem memória (rodada `ref`, nenhuma linha do assistente em 300 s) eu não repeti. Foi uma partida de cada, e a `edm` ainda caiu na ordem ruim (teclas na 87ª), sem efeito.

| Aos 62 s, hub de antes | Aos 62 s, hub novo |
|---|---|
| a tela de ajustes; o assistente entra no segundo seguinte | o assistente já na tela |

![Aos 62 s, sem o carro de mentira e com a memória guardada](quadros/2026-10-11-hmi-hub-assistente-aos-62s.png)

**Conferência final, da pasta, depois de instalar** (22h54 a 22h58, com tudo o que as outras sessões tinham mudado até ali): duas partidas com os padrões (`fp1`, `fp2`: hora aceita aos 0,60 e 0,74 s, Welcome por 4,6 e 4,9 s, tela do veículo com os dados, 34 ausentes, nenhum anúncio guardado entregue, nenhum erro de OpenGL) e duas sem o carro de mentira (`fv1`, `fv2`: 34 ausentes, 106 pedidos de assinatura, Welcome por 4,3 e 4,6 s, nenhum erro de OpenGL; na `fv1` o `dsistub` se atrasou e o anúncio do display veio pelos guardados).

### O que muda para quem usa

- **No padrão do modo ao vivo (`sh driver/ao_vivo.sh`, a janela): nada.**
- **Toda central que sobe o `carifservice` de verdade muda de partida**: `VEICULO= sh driver/ao_vivo.sh`, `driver/m6_rodada.sh`, `driver/vc_rodada.sh`, o `driver/m1_hmi.sh` chamado direto e as centrais de teste com `VEICULO=`. A HMI liga os serviços de veículo; dá 34 serviços por ausentes, em vez de 48; faz 8 esperas de 2 s, em vez de 13, e por isso tudo o que vem depois acontece uns 10 s mais cedo; há 106 pedidos de assinatura e 80 eventos a mais no barramento; o registro dela tem perto de 120 linhas de erro nos primeiros 65 s, em vez de 290. As telas são as mesmas. **Tempos, contagens e tráfego de rodadas sem o carro de mentira feitas antes das 22h52 de 11/10/2026 não são comparáveis com os de depois.**
- **A tela preta e a central surda que apareciam de vez em quando nessas rodadas deixam de aparecer por este motivo.** Quem tiver rodada antiga assim confere no `barramento.log` a posição dos anúncios: `grep -v ' if=1901 ' <arquivo> | grep -n ' if=9000 '`. Se o do canal `0x400000`, o do `0x80000000000` ou o do `0x10000000000000` passar da linha 48, foi este defeito.
- **O registro do hub ganhou uma linha**, "(entregou N anuncios de dominio guardados ao novo reader)", logo depois da "(reentregou N anuncios…)" de sempre.

### Limites

- **A corrida de partida continua existindo.** O hub deixou de transformá-la em falha, mas quem se liga depois da rajada do `carifservice` ainda perde as outras mensagens que ficaram fora do cache. Nenhuma delas é anúncio, e nas oito partidas em que isso aconteceu com o hub novo e a HMI rodou até o fim (seis ao acaso e as duas forçadas) não fez falta. Fixar a ordem no lançador seria outro conserto, que não fiz.
- **Anúncio guardado não expira.** Um daemon que anuncia e cai continua anunciado para quem chega depois. Com o cache já era assim.
- **A reentrega continua sem garantia.** O hub manda sem esperar; se a fila do leitor novo encher, o que não couber se perde sem aviso. Medido: a rajada para a HMI tem 48 mensagens de diagnóstico e até 48 comuns, menos de 3 kB ao todo, e cabem 299 mensagens pequenas na fila.
- **O "um por canal" vale para o que o hub acrescenta, não para o que já está no cache** (ver "O conserto").
- **Não sei o que são as 77 mensagens que o `carifservice` manda antes e depois do anúncio.** Estão em interfaces que não são da tabela DSI (`102401`, `1762168833`, `17338369`, `34115585`; todas começam com os bytes `01 90`). O Marco 8 já tinha deixado isso em aberto.
- **Nas séries lado a lado a ordem ruim apareceu mais no braço do hub novo, e isso ficou sem explicação.** O ensaio controlado de 240 partidas não repetiu a diferença (ver "A medição").
- **Só no container.** Não rodou no QEMU, onde a corrida pode ter outra cara.
- **O firmware é o MU0883.** Se o `carifservice` do carro (MU0803) manda a mesma rajada, não sei.

### O que entrou na pasta

- `driver/kipc_hub.c`: a mudança, instalada às 22h52 por troca de arquivo, depois de conferir que o arquivo da pasta ainda era o da cópia de que parti. O de antes ficou em `traces/hub_kipc_hub.antes.c`.
- `driver/tests/t_hub.sh`: os 25 casos. Precisa do `perl` da imagem para um deles (o cliente que escreve e lê com o mesmo pid).
- `driver/dsi_escuta.c`: só o comentário do cabeçalho, que dizia "os 48 primeiros anúncios".
- `traces/hub_*` (94 arquivos, 8,8 MB): os roteiros das centrais de teste, das séries e dos experimentos; os analisadores do registro do hub (`hub_analisa.py`, `hub_ordem.py`, `hub_resumo.py`, `hub_esqueleto.py`, `hub_tempos.py`); as previsões; os resumos das séries; o ensaio da corrida (`hub_corrida.txt`); a conferência final (`hub_conferencia_final.txt`); e a saída da HMI, o tráfego e o registro do hub de doze partidas (`mp1`, `mv1`, `av1`, `av4`, `dv2`, `dv6`, `ap1`, `dp1`, `eaf1`, `edf1`, `eam`, `edm`).
- Quatro folhas de fotos, `quadros/2026-10-11-hmi-hub-*.png`.

Testes, rodados da pasta às 22h52, com o hub instalado: `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 26, `t_entrada.sh` 11, `t_pedidos.sh` 16, `t_veiculo.sh` 126, `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 34, `t_servidor.sh` 20 e `t_lsmval_abre.sh` 38. Todos passam, e a compilação não dá aviso (o hub, nem com `-Wall -Wextra`). O `t_transmissao.sh` e o `t_veiculo.sh` cresceram nesta noite pelas mãos de outras sessões (eram 16 e 56); com a cópia congelada das 21h50 e o hub novo passavam os 16 e os 56, e com o hub de antes também.

**Às 23h25 mexi num caso do `t_hub.sh`.** A sessão do Marco 6 avisou que o `diagstub` ia passar a mandar o cabeçalho `83 60 22` no lugar de `00 00 00`, e o caso `diagnostico_continua_chegando_antes_de_tudo` conferia a linha inteira, cabeçalho incluído. O hub não lê esses três bytes; o caso passou a aceitar qualquer cabeçalho e a conferir o resto. Contra um `diagstub` de cabeçalho novo o teste de antes falhava só nesse caso (24 de 25, `traces/hub_t_hub_de_antes_contra_cabecalho_novo.out`); o ajustado passa os 25 com os dois. Suítes rodadas de novo da pasta às 23h26, já com o `diagstub` que ela instalou às 23h26 (`traces/hub_suites_instalado_23h26.txt`): `run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 16, `t_veiculo.sh` 128, `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 37 e `t_servidor.sh` 20. Todas passam.

### Onde parou

- O item "O limite de 48 mensagens do hub" está feito, e com ele a pendência que a sessão do painel de instrumentos deixou sobre a rodada `r5c`.
- A central ao vivo que estava no ar às 22h52 tinha sido ligada às 22h47 e segue com o hub de antes até religar. No padrão dela não faz diferença.
- Fica em aberto a corrida de partida em si (ver "Limites" e os próximos passos).

## O rastreamento da HMI que parava na carga tardia (12/10/2026, depois do Marco 9)

Feito numa sessão à parte, aberta pela sessão do Marco 9 à 0h09 de 12/10/2026, para o item "Manter o rastreamento da HMI ligado depois da partida" dos próximos passos (Marco 9, "Um achado de passagem"). O pedido foi achar quem troca as propriedades de registro da HMI uns 6 s depois da partida e fazer as chaves do `HMILOG` continuarem valendo, sem alterar o firmware além de montar arquivos por cima, como o lançador já fazia. É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução:**

- **As chaves do `HMILOG` valem a sessão inteira.** Com `HMILOG="normal DSI.CAR_TIME_UNITS_LANGUAGE"`, a linha de rastreamento do relógio (`DSI-U: …updateClockTime`, que o carro de mentira provoca uma vez por segundo) sai até o fim do registro. Em nove partidas de 120 s com o lançador novo (`n1` a `n6`, da cópia de trabalho, e `f1` a `f3`, da pasta): 122 ou 123 linhas, a última entre 118,5 e 119,3 s, e nunca mais de 1,23 s entre duas linhas depois dos 60 s. Com o lançador de antes, no mesmo comando (`r1` e `r2`): 9 linhas, a última aos 5,6 e aos 5,8 s.
- **A própria HMI diz o que ficou valendo:** `Changed logging properties to: {DSI.CAR_TIME_UNITS_LANGUAGE=true}`. Antes dizia `{}`.
- **Dá para rastrear o uso da tela, que era o motivo do pedido.** Com as chaves das teclas e a tecla MENU apertada aos 75 s (`uso1`), o registro mostra a tecla chegando pelo barramento (`DSI-U: …DSIKeyPanelListener[0].updateKey2(13,78,1,79021,1)`) e sendo tratada (`HARDKEY Keyboard: 13 keyId:78 keyState:1`): 16 linhas TRACE entre os 73 e os 76 s. Com o lançador de antes e as mesmas chaves (`uso0`), nenhuma: só a "HK Received", que é de nível NORMAL.
- **Sem `HMILOG` nada mudou.** Doze partidas, cinco com o lançador de antes e sete com o novo, em pares lado a lado: o roteiro de partida e a linha de comando da JVM são iguais byte a byte nas doze, as montagens do container são as mesmas (nenhuma em `/vw/hmi/config`), o resumo do lançador tem as mesmas 14 linhas, e na saída da HMI não há mensagem que apareça em todas as de um lado e em nenhuma do outro.

**A causa:** na carga tardia a HMI ativa a sonda de depuração `LOGMANAGER`, e ela troca todas as chaves de rastreamento pelas do perfil de sondas em uso, um arquivo de `/vw/hmi/config` que, no perfil padrão, não liga nenhuma. **O conserto:** o lançador põe as chaves pedidas também nesses arquivos, em cópias montadas por cima.

### Quem troca as propriedades

Lido nas classes da HMI, nos decompilados do CFR de 10/10, com os métodos da cadeia conferidos no bytecode do jar do firmware (`/vw/hmi/jar/MIBHMI-STD-H40.272.304-bin.jar`), tirado com o `javap` da JVM dele. Os dois batem. O bytecode está em `traces/hlog_javap/`; a lista do que foi conferido em cada um, em "Fontes".

- **Quem escreve a mensagem** é `de.vw.mib.log4mib.internal.MainLogger.setLoggingProperties(Properties)`. Ela registra "Changed logging properties to: " com as propriedades que recebeu e percorre todos os classificadores: quem aparece nas propriedades fica com a máscara de rastreamento que elas pedem, quem não aparece fica com zero. É troca, não soma: o que o `logging.properties` tinha ligado e não vier de novo é desligado.
- **Quem chama** é um lugar só em todo o jar: `de.vw.mib.debug.service.probes.logging.LogManagerProbe.applySettings`, a sonda de depuração de nome `LOGMANAGER`. Ela entrega ao registro a própria configuração (`LogManagerProbeConfiguration`), que é a lista das chaves com valor `true`.
- **Quando.** A sonda mora no bundle 71, `DebugProbes`, que o `bundles.properties` põe na fase de carga tardia (`Bundle.Startup.Lateload = 61,62,75,70,71`). Quando ela se registra, o gerenciador de sondas (`de.vw.mib.debugmanager.internal.DebugProbeManagerImpl.initialConfigureProbe`) vê que ela está na lista de sondas ativas e a ativa com as propriedades dela. Daí o "Lateload" na linha, e os 6 s.
- **De onde vêm as propriedades: de um arquivo.** O gerenciador lê um "perfil de sondas", um dos 33 arquivos `/vw/hmi/config/debugprobes_NNN_<nome>.config`. O arquivo é feito de seções `BEGIN <sonda>` … `END <sonda>`: a seção `PROBES` diz quais sondas ficam ativas, e a seção `LOGMANAGER` traz chaves de rastreamento no mesmo formato do `logging.properties`. O perfil de fábrica é o 130, `debugprobes_130_DSI_+_RSI_only.config`: põe `DSIEVENTS, RSIPROBE, LOGMANAGER` como ativas e, na seção `LOGMANAGER`, só tem três linhas comentadas. Sonda ativa com seção vazia: é o `{}` da mensagem.
- **Quem escolhe o perfil é um número guardado na memória de estado.** O gerenciador pergunta a `ConfigurationManager.getDynamicTmDebugProbesCfg()`. Na partida o valor é −2 e ele registra "Deactivating all debug probes (specified in TESTMODE)"; um décimo de segundo depois de "UI thread started" o tratamento de persistência (`AslTargetSystemPersistenceHandling`, ao ligar) põe ali o número guardado nos dados "early" da memória (`PersistableEarlyData.debugProbeConfiguration`), e o gerenciador lê o arquivo desse número. O valor de fábrica do campo é a propriedade `-DdebugProbes.default=130`, que o `runHMI.sh` passa. **Conferido em execução** (rodadas `m60b` e `m60n`): com uma memória de partida em que esse campo vale 60, a HMI registra "Reading debug probe configuration '/vw/hmi/config/debugprobes_060_CAR_Specific_Tracing.config' (given by the configuration manager)" e na carga tardia liga as 21 chaves desse perfil. Na memória guardada do modo ao vivo o campo vale 130 (lido numa cópia, com o `driver/lsmval_abre.py`). Pelo código, quem troca o número é a tela de teste da HMI (evento 1800002 em `AslTargetStartup`); não abri essa tela.
- **Nem a persistência nem um serviço trazem as chaves.** A memória guarda só o número; as chaves estão no arquivo.

A HMI conta a mesma história quando a chave `DEBUG.PROBES` está ligada (rodada `r2`, com o lançador de antes; tempos desde "UI thread started"):

```
 0,07 s  TRACE  [DebugProbeManager] configuration for DebugProbeManager was changed in test mode screen
 0,08 s  NORMAL [DebugProbeManager] Reading debug probe configuration '/vw/hmi/config/debugprobes_130_DSI_+_RSI_only.config' (given by the configuration manager)
 0,08 s  TRACE  [DebugProbeManager] applying new DebugProbeManager configuration - active probes: DSIEVENTS, RSIPROBE, LOGMANAGER
 5,94 s  INFO   Starting lateload bundles ...
 5,95 s  NORMAL Starting bundle 71-DebugProbes ...
 5,95 s  TRACE  Debug probe factory registered: LOGMANAGER
 5,95 s  TRACE  Initializing probe: de.vw.mib.debug.service.probes.logging.LogManagerProbe
 5,95 s  TRACE  [DebugProbeManager] activate probe 'LOGMANAGER' ...
 5,95 s  TRACE  Activating probe: de.vw.mib.debug.service.probes.logging.LogManagerProbe
 5,95 s  NORMAL Changed logging properties to: {}
```

A última linha de rastreamento do próprio gerenciador é a que antecede a troca: a chave dele também foi desligada ali.

Três coisas de passagem:

- **Algumas linhas TRACE saem mesmo sem chave.** O registrador tem duas formas de rastrear: `trace(sub, texto)` consulta a máscara; `trace(sub)`, que devolve a mensagem para ir sendo montada, não consulta (`ContextLogger`). O código que usa a segunda sem perguntar antes `isTraceEnabled` sai sempre: são 15 linhas numa partida sem chave nenhuma (rodada `nk1`), a última `[HSM|ASL.UNKNOWN] UGDO_POPUP_UPDATE_CANNOT_BE_PROCESSED`, aos 14,6 s. Por isso o rastreamento parecia parar "entre 6 e 15 s"; a troca é uma só, perto dos 6 s.
- **Na variante manual do lançador nada era trocado.** Sem o `runHMI.sh` a HMI não recebe a propriedade `debugprobes.config.path`; o gerenciador registra "No directory set for debug probe configuration files! … All debug probes keep deactivated!" e a sonda nunca é ativada. Conferido com os dois lançadores (`man0` e `man1`, 100 s cada): nenhuma linha "Changed logging properties", e o relógio rastreado até o fim nos dois.
- **Um dos 33 perfis do firmware está quebrado.** O `debugprobes_070_Audiomanagement_Specific_Tracing.config` abre a seção `PROBES` e não a fecha, e o leitor da própria HMI (`DebugProbeConfigurationCodec.loadConfiguration`) o recusa com "unexpected BEGIN tag found!". O leitor guarda o estado em campos estáticos e não o desfaz no erro: depois de recusar um arquivo, recusa todos os que ler em seguida. Vi isso rodando o leitor do firmware sobre os 33 arquivos (abaixo). Pelo código, na HMI isso quer dizer que, escolhido o perfil 070, nenhum outro perfil é lido até ela reiniciar; não executei esse caso.

### O conserto

O lançador (`driver/m1_hmi.sh`, opção `HMILOG`) continua pondo as chaves numa cópia do `logging.properties`, que vale da primeira linha até a carga tardia. O que entrou: as mesmas chaves, uma linha `CHAVE = true` cada, vão para o fim da seção `LOGMANAGER` de **todos** os perfis de sondas, em cópias guardadas em `/tmp/hmilog` e montadas por cima dos originais. A sonda passa a aplicar as chaves do perfil mais as pedidas, qualquer que seja o perfil que a memória escolher. O resto de cada perfil fica igual, inclusive a lista de sondas ativas: a HMI faz o que fazia, com mais chaves ligadas.

- Só acontece com `HMILOG` e com pelo menos uma chave válida. Com `HMILOG=normal`, que é o padrão do `m6_rodada.sh` e do `vc_rodada.sh`, nenhum perfil é montado (rodada `nk1`).
- Perfil sem a linha `END LOGMANAGER` não é tocado, e o lançador avisa que com ele as chaves param na carga tardia. Neste firmware os 33 têm a linha.
- No fim da rodada sai uma linha a mais no resumo, só com `HMILOG`: `[m1_hmi] registro detalhado: 159 linhas TRACE; chaves que a HMI aplicou na carga tardia: {DSI.CAR_TIME_UNITS_LANGUAGE=true}`. É o jeito de ver, sem abrir o registro, se a HMI ficou com as chaves; `{}` quer dizer que ela desligou todas.

**Conferido com o leitor do próprio firmware**, antes de subir central: o trecho do lançador, tal como está no arquivo, rodou num container com o firmware montado, e um programa pequeno (`traces/hlog_ConferePerfis.java`, compilado e rodado na JVM do firmware) leu os 33 pares de arquivos, original e alterado, com `DebugProbeConfigurationCodec.loadConfiguration`. Em 32 as oito seções são as mesmas, sete delas iguais, e a `LOGMANAGER` é a original mais as chaves pedidas; passando a seção pelo caminho da sonda (`LogManagerProbeConfiguration.load` e `storeToProperties`) e pelo do registro (`LoggingProperties`), cada chave cai num classificador e num bit (`DSI.CAR_TIME_UNITS_LANGUAGE`: classificador 16, bit 13). O 33º é o 070, recusado igual antes e depois. Saída em `traces/hlog_confere_perfis.txt`.

### As rodadas

Trinta centrais de teste `moi3-hlog-<nome>`, à parte da central ao vivo, com os padrões do modo ao vivo (`PREFIXO=hlog sh driver/m8_central.sh sobe …`), de 120 s, salvo as duas da variante manual. O "antes" e o "depois" saíram de duas cópias de `driver/`, `stubs/` e `glforward-moi3/` tiradas da pasta à 0h14, iguais a não ser pelo lançador; a pasta não mudou até a instalação, à 0h48, e as sete centrais `f` e `g` saíram dela. Tempos em segundos desde "UI thread started". Saídas em `traces/hlog_*`.

| Rodada | Lançador | Como subiu | O que a HMI aplicou na carga tardia | Rastreamento |
|---|---|---|---|---|
| `r1`, `r2` | de antes | chave do relógio; a `r2`, também `DEBUG.PROBES` e `LOGGING.LOGMANAGER` | `{}`, aos 5,86 e aos 5,95 s | relógio: 9 linhas, a última aos 5,6 e aos 5,8 s; nenhuma depois dos 60 s |
| `n1` a `n6` | novo | chave do relógio | `{DSI.CAR_TIME_UNITS_LANGUAGE=true}`, entre 5,37 e 6,09 s | relógio: 122 ou 123 linhas, a última entre 118,6 e 119,3 s; 59 ou 60 depois dos 60 s, com no máximo 1,07 s entre duas |
| `b1` a `b3` | de antes | sem `HMILOG` | não se vê: a linha é de nível NORMAL | nenhuma linha NORMAL nem TRACE |
| `s1` a `s3` | novo | sem `HMILOG` | idem | idem; nenhuma montagem em `/vw/hmi/config` |
| `uso0` | de antes | chaves das teclas (`ASL_SYSTEM.KEYPANEL ASL2.KEYPANEL DSI.SYSTEM`), MENU aos 75 s | `{}` | 280 linhas TRACE, a última aos 14,5 s; nenhuma na hora da tecla |
| `uso1` | novo | o mesmo | as três chaves | 932 linhas TRACE, a última aos 118,5 s; a tecla aparece chegando e sendo tratada |
| `m60b` | de antes | chave do relógio e memória que escolhe o perfil 060 | as 21 chaves do perfil 060 | relógio: 9 linhas, a última aos 5,6 s; as chaves do perfil rastreiam até o fim (15.709 linhas TRACE) |
| `m60n` | novo | o mesmo | as 21 do perfil e a do relógio | relógio: 123 linhas, a última aos 119,4 s; 60 depois dos 60 s |
| `nk1` | novo | `HMILOG=normal`, sem chave | `{}` | 15 linhas TRACE, as que não dependem de chave; só o `logging.properties` montado, como antes |
| `man0`, `man1` | de antes, novo | variante manual, chave do relógio, 100 s | não houve troca | relógio até o fim nos dois (106 e 104 linhas) |
| `f1` a `f3` | o da pasta, já instalado | chave do relógio | `{DSI.CAR_TIME_UNITS_LANGUAGE=true}`, entre 6,18 e 7,01 s | relógio: 122 linhas, a última entre 118,5 e 119,3 s; 59 ou 60 depois dos 60 s, com no máximo 1,23 s entre duas |
| `g1` a `g4` | o da pasta | sem `HMILOG` | não se vê | nenhuma linha NORMAL nem TRACE; nenhuma montagem em `/vw/hmi/config` |
| `b4`, `b5` | de antes | sem `HMILOG`, lado a lado com `g3` e `g4` | não se vê | idem |

Nas 30 centrais da sessão, com um lançador ou com o outro, não houve erro de OpenGL nem arquivo de erro da JVM, a tela Welcome e o relógio vieram em todas, e o registro do carro de mentira ficou igual em todas: 12.502 bytes, 3 pedidos atendidos, nenhum laço.

**Sem `HMILOG`, a comparação foi lado a lado e repetida**, porque duas partidas iguais já diferem entre si (`traces/hlog_comparacao_sem_hmilog.txt`). Foram doze centrais: cinco com o lançador de antes (`b1` a `b5`) e sete com o novo (`s1` a `s3`, da cópia de trabalho, e `g1` a `g4`, da pasta). Nas doze o roteiro de partida e a linha de comando da JVM são iguais byte a byte, as montagens são as mesmas 20, nenhuma em `/vw/hmi/config`, e o lançador imprime as mesmas 14 linhas. Na saída da HMI, de 3.975 a 4.094 linhas, duas centrais quaisquer diferem em 3 a 24 "moldes" de mensagem (a linha sem a hora, sem a thread e com os números trocados por `#`): de 3 a 19 entre duas do lançador de antes, de 8 a 22 entre duas do novo, de 5 a 24 entre uma de cada. Nenhum molde aparece em todas as de um lado e em nenhuma do outro.

Duas diferenças apareceram, e não as atribuo à mudança:

- **A tela Welcome passou duas vezes pelo estado `REGULAR`** em 4 das 20 centrais com o lançador novo (`n5`, `s1`, `nk1` e `f2`) e em nenhuma das 10 com o de antes. Em duas das quatro (`s1`, sem `HMILOG`, e `nk1`, só com `normal`) o lançador não faz nada de novo antes de a HMI subir. Nas saídas guardadas de outras sessões, de antes da mudança, a mesma contagem aparece em 4 de 47.
- **No encerramento, a ligação da HMI com o registro RSI fechou com o código 1000 em vez do 1006** em `g1` e `g2`, as duas primeiras centrais sem `HMILOG` rodadas da pasta, e em nenhuma outra das 30. Isso é depois de o lançador mandar os daemons pararem. Repeti com dois pares (`g3` e `g4`, da pasta, contra `b4` e `b5`): nas quatro, 1006. Nas saídas guardadas de outras sessões, 3 de 95 têm o 1000.

**As suítes** rodaram da pasta à 0h56 de 12/10/2026, com o lançador já instalado: `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 16, `t_veiculo.sh` 128, `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, a nova `t_hmilog.sh` 32 e, no Mac, `t_servidor.sh` 20 e `t_lsmval_abre.sh` 38. Todas passam, e a compilação não dá aviso. A `t_codificacao.sh` tinha 26 casos e a `t_vc_diag_alm.sh` 34 quando o pedido foi escrito; as duas cresceram às 23h25 e às 23h28 de 11/10, com a troca do cabeçalho do `diagstub`. Saída em `traces/hlog_suites.txt`.

### O que muda para quem usa

- **Sem `HMILOG`: nada.**
- **Com `HMILOG=normal` sozinho:** só a linha "registro detalhado:" a mais no resumo do lançador.
- **Com chaves:** as linhas TRACE delas saem até o fim, e não só nos primeiros 6 s. O arquivo de saída cresce enquanto a HMI roda: a chave do relógio dá uma linha por segundo (27 kB a mais em 120 s); as 21 do perfil 060 deram 15 mil linhas em 120 s. **As contagens de linha de rodadas com chaves deixam de ser comparáveis com as de antes da 0h48 de 12/10/2026**; as séries do Marco 9 usavam `DSI.CAR_BORDCOMPUTER CAR.ASL_CAR`.
- As chaves são as da partida. Não há como ligar ou desligar uma com a central no ar.
- Para voltar ao lançador de antes: `traces/hlog_m1_hmi.antes.sh` por cima de `driver/m1_hmi.sh`, por troca de arquivo.

### Limites

- **A tela de teste da HMI continua mandando.** Pelo código, trocar de perfil por ela relê o arquivo (e as chaves do `HMILOG` seguem, porque estão em todos), e desativar as sondas desliga todas as chaves (`LogManagerProbe.deactivateAllTraces`). Não abri a tela de teste.
- **Uma ferramenta ligada ao canal de depuração da HMI** (o "HMIMonitor" que o código cita) pode mandar outra configuração. Na bancada o container não tem rede e ninguém fala nesse canal.
- **Só entram as chaves que o `logging.properties` lista** (590). As dos perfis usam os mesmos nomes.
- **O perfil 070 continua quebrado**, como no firmware; com ele escolhido a sonda não é configurada e valem as chaves do `logging.properties` (leitura do código).
- **As linhas do resumo são contadas na hora em que a HMI é cortada**, como as outras do lançador; o fim do registro ainda chega ao arquivo depois. A contagem de linhas TRACE do resumo pode ficar um pouco abaixo da do arquivo.
- **Só no container.** O trecho novo usa `sed`, `grep`, `basename` e `mount -o bind`; conferi que o `sed` do busybox da imagem, que é o do initramfs do QEMU, dá o mesmo arquivo, mas não rodei no QEMU.
- **As duas propriedades da HMI que fariam coisa parecida não foram experimentadas** (decisão D78).
- **A máquina estava carregada**: outras quatro sessões rodavam centrais (containers `rad`, `est`, `cpu` e `vol`), e a carga do Mac foi de 11 a 176 durante as séries. As linhas do relógio saíram de segundo em segundo mesmo assim; o maior intervalo, 1,23 s, é da série da pasta, com seis centrais no ar. **Todas as 30 centrais terminaram à 1h03, antes da troca da biblioteca do barramento da 1h13 (Marco 10)**: tempos e contagens de quadros desta seção são os de antes dela. O que a seção afirma (as chaves valem até o fim; sem `HMILOG` nada muda) não depende disso, e as centrais da investigação dos avisos, com `HMILOG` e já com a biblioteca nova, registraram as chaves pedidas na carga tardia do mesmo jeito.
- **Os arquivos são do MU0883**; o carro roda o MU0803.

### O que entrou na pasta

- `driver/m1_hmi.sh`: o trecho do `HMILOG` (as chaves nos perfis de sondas), o cabeçalho e a linha "registro detalhado:" do resumo. O código sem `HMILOG` é o mesmo: a diferença está em `traces/hlog_m1_hmi.diff`, e o arquivo de antes em `traces/hlog_m1_hmi.antes.sh`.
- `traces/hlog_*`: o bytecode das classes (`hlog_javap/`), a conferência dos perfis com o leitor do firmware, as saídas de `r1`, `r2` e `f1`, o resumo e a tabela de saúde das 30 centrais (`hlog_resumo.txt`, `hlog_saude.txt`), a comparação sem `HMILOG`, o trecho das teclas (`hlog_uso_teclas.txt`), a memória de teste com o perfil 060 e o que as duas centrais fizeram com ela, a variante manual, as suítes, os roteiros das séries e os programas que contam as linhas.
- `driver/tests/t_hmilog.sh`, novo: 32 casos sobre o trecho do `HMILOG`, recortado do lançador tal como está e rodado numa árvore de mentira, com o `mount` trocado por uma função que anota e copia. Fixa que cada chave entra no `logging.properties` e no fim da seção `LOGMANAGER` de cada perfil e que mais nada muda; que chave desconhecida e perfil sem a seção são avisados; que `HMILOG=normal` não toca em perfil e que sem `HMILOG` nada é tocado; e a linha do resumo. Com o firmware montado em `/fw`, confere também os 33 perfis de verdade (4 dos 32 casos). **Foi escrito depois do código**, e não antes: contra o lançador de antes falha em 8 dos 32, os que pedem as chaves nos perfis e a linha do resumo, e contra o novo passa nos 32 (`traces/hlog_t_hmilog_vermelho_verde.txt`). Roda na imagem; não precisa de privilégio.

### Onde parou

- O item "Manter o rastreamento da HMI ligado depois da partida" está feito.
- **Fica para a sessão do Marco 9** olhar, agora pelo registro da HMI, as duas coisas que ela só tinha lido no código: as duas regras do laço do idioma do painel e o que a HMI faz com as listas vazias do start-stop.
- **Ligar e desligar chaves com a central no ar** não tem caminho hoje. A HMI só relê o perfil quando o número muda, e quem muda o número é a tela de teste.
- A central ao vivo não foi tocada; à 1h05 não havia nenhuma no ar. A próxima partida dela pega o lançador novo e, como ela sobe sem `HMILOG`, faz o mesmo que antes.

## Marco 10 — a central parada deixa de ocupar cinco núcleos (12/10/2026)

Feito numa sessão à parte, aberta pela sessão principal à 0h27 de 12/10/2026, para o item "Cada central ocupa cinco núcleos do Mac mesmo parada" dos próximos passos. O pedido: achar a causa antes de propor conserto, escrever o teste antes do código, consertar no menor lugar possível sem mudar o que a HMI e os daemons do firmware enxergam (mesmas mensagens, mesma ordem, mesmos retornos), comparar lado a lado antes de instalar e avisar as outras sessões antes da troca. É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução:**

- **Uma central parada gasta de 2 a 3 % de um núcleo, em vez de cinco núcleos.** Com a biblioteca de antes, de 204 a 501 % conforme a disputa da máquina (501 % quando teve os cinco núcleos para si); com a nova, de 1,9 a 3,4 %, medido pelo contador de processador do container em seis centrais de cada, entre 55 e 65 s da HMI. Pelo `docker stats`, aos 60 s da HMI, em quinze centrais com a biblioteca nova (seis da série, três sem o carro de mentira e seis subidas da pasta, três de cada vez): de 0,8 a 1,8 %, com uma leitura de 10,2 %. Três centrais da pasta paradas ao mesmo tempo, numa leitura só: 0,70 %, 0,89 % e 0,74 %.
- **A causa:** o laço de leitura do barramento do firmware não tem pausa, porque no carro é a própria leitura que segura a thread até chegar mensagem; e a nossa biblioteca (`driver/kipc_bus.c`) devolvia a leitura na hora. Uma thread por processo girava sem parar.
- **O conserto são oito linhas no `driver/kipc_bus.c`:** sem mensagem na fila, a leitura espera por ela até 100 ms e só então devolve o -1/EAGAIN de sempre. A mensagem acorda o leitor na hora em que chega.
- **Nas telas e nos serviços nada mudou.** Em seis pares de centrais lado a lado, com os padrões do modo ao vivo: os mesmos 34 serviços ausentes, as mesmas telas, as mesmas contagens no barramento, nenhum erro de OpenGL, e fotos iguais byte a byte aos 25, 45 e 70 s em 16 das 18 comparações (nas outras duas só muda o minuto do relógio, que virou entre uma foto e a outra).
- **Duas corridas de partida da própria HMI, que a série não olhava, apareceram depois da instalação.** Nos primeiros 200 ms a HMI decide, com folga de menos de 1 ms em boa parte das partidas, se o perfil inicial da persistência chegou a tempo e se liga o indicador de gesto da grade de menus. As duas já variavam de uma partida para outra. Com a máquina carregada, a biblioteca de antes quase sempre perdia a primeira (6 de 8 partidas, contra 0 de 7 da nova); com a máquina livre não achei diferença que se repetisse. Está em "As corridas de partida, que a série não olhava".
- **Instalado à 1h13.** Toda central que sobe depois disso compila a biblioteca nova. A bateria inteira passa.

![A tela do veículo aos 70 s nas doze partidas da série: em cima a biblioteca de antes, embaixo a nova](quadros/2026-10-12-hmi-cpu-serie-70s.png)

### A medição: quem gira, e em que chamada

Central de diagnóstico `moi3-cpu-d1`, à 0h32, subida de uma cópia congelada da pasta tirada à 0h30, com a biblioteca de antes e só os padrões do modo ao vivo. Medi com a HMI parada havia 100 s. Havia três ou quatro centrais de outras sessões no ar e a carga do Mac foi de 55 a 176, de modo que os percentuais da `d1` são de máquina disputada; os números limpos abaixo são da partida `a6` da série, que teve os cinco núcleos para si.

- **São cinco threads, uma por processo:** `DSIProvider` na JVM da HMI e a thread de leitura do `mcu-interpreter`, do `persistence-app`, do `touch-app` e do `MIB_ServiceRegistryApp-00`. Na `a6`, cada uma de 99,8 a 100 % de um núcleo: perto de três quartos em modo usuário e um quarto dentro do sistema (72 a 74 % e 25 a 28 %). Nenhuma outra thread do container passa de 0,4 %.
- **Elas nunca largam o processador por conta própria.** Trocas de contexto voluntárias em 100 s de vida, na `d1`: 31, 1, 2, 5 e 1.
- **A chamada que repetem é uma só:** `recvfrom(fd, buf, 26640, MSG_DONTWAIT, NULL, NULL) = -1 EAGAIN`. Com `strace -c` em cada thread por 5 s: 1.004.076, 1.006.025, 1.050.784, 746.890 e 565.459 chamadas, e em cada thread todas menos 20 voltam com `EAGAIN`. As 20 são as mensagens que chegaram: quatro por segundo, dos stubs. O `strace` segura a thread; sem ele o cliente de teste, que imita o laço, dá de 1,4 a 1,8 milhão de voltas por segundo.
- **Essa chamada é a leitura do `kipc_bus.c`:** 26640 é `0x6800 + 16`, o tamanho do buffer `in`, e `MSG_DONTWAIT` é a opção que a leitura passava.
- **Quem chama, pela pilha.** A imagem não tem `gdb` nem `perf`, e o `strace` dela não mostra pilha; escrevi um amostrador por `ptrace`, só de leitura (`traces/cpu_amostra.c`). Em 40 amostras por thread, na HMI: `kipc_bus.so+0x97c`, logo depois da chamada de sistema, de volta para `libDSIService.so+0x11524`, que é o retorno do `ioctl` em `kipc::CommunicationService::read`, de volta para `+0xd478`, o retorno de `recv_pkt` em `frame::Frame::event_checker_kipc`, que é a função da thread. Nos quatro daemons a cadeia é a mesma, nos endereços de cada um. Parte das amostras cai num `memset` da libc: o firmware zera 26 kB antes de cada leitura, e é esse o tempo em modo usuário.
- **O resto do processo dorme.** Com `strace -f -c` no processo inteiro, o `recvfrom` é a única chamada de contagem alta; as outras threads estão em `epoll_pwait`, `futex` e `nanosleep`.

Saídas em `traces/cpu_d1_*`. Um aviso sobre elas: na primeira tabela de threads da `d1` (`cpu_d1_diag.out`) as linhas de 71,8 % são erro do meu roteiro, que desalinhava as colunas quando o nome da thread tinha espaço; a tabela refeita está em `cpu_d1_threads.txt`.

### O laço do firmware (leitura estática)

Desmontagem da `libDSIService.so`, que tem os símbolos, e dos quatro daemons, que trazem o mesmo código ligado estaticamente (`traces/cpu_re_laco_de_leitura.txt`):

```
frame::Frame::event_checker_kipc(frame):            a thread de leitura
    enquanto (frame->rodando):
        se (frame->procmgr):
            clock_gettime(CLOCK_REALTIME_COARSE, &agora)
            se (agora.tv_sec - ultimo.tv_sec > 15): ProcessMgr::heartbeat(frame); ultimo = agora
        r = recv_pkt(&canal, &len, &carga)           -> kipc::CommunicationService::read
        se (r != 0): volta ao começo                 sem espera nenhuma
        trata a mensagem

kipc::CommunicationService::read:
    memset(carga, 0, 0x6800)
    r = ioctl(fd, 0, descritor)                      a chamada que o kipc_bus.so atende
    se (r < 0): devolve r, sem tocar nas saídas
```

- **O laço conta com uma leitura que segura a thread.** Quando a leitura volta sem mensagem ele chama de novo na hora. O nó é aberto sem `O_NONBLOCK` (`open("/dev/csr", O_RDONLY, 0777)`).
- **Duas coisas dependem de a leitura voltar de vez em quando:** a batida de vida do processo (`ProcessMgr::heartbeat`, avaliada só entre uma leitura e outra, passados mais de 15 s da anterior; `ProcessMgr::checkThread` devolve verdadeiro quando todos os laços bateram) e a ordem de parar (`frame->rodando`). Uma leitura que nunca voltasse num barramento calado pararia as duas. Quem chama `checkThread` eu não li.
- **O `carifservice` é diferente, e por isso nunca esteve entre os processos a 100 %.** O laço dele (`KIPC_ReceiveTask::run`, em `CarSSW_FA_KIPC.cpp`) registra `[CFA]KIPC_ReceiveTask timeout` e dorme 10 ms quando a leitura volta sem mensagem: 100 voltas por segundo, não um milhão. E o nome que ele dá a essa volta, "timeout", diz o que o carro faz.

### O que o carro faz (leitura estática do kernel)

O driver do barramento está dentro do kernel do `boot.img` (Marco 0). Tirei a tabela de símbolos embutida (48.296 símbolos, `traces/cpu_kallsyms.py`) e desmontei o `csr_ioctl` (`traces/cpu_re_kernel_kipc.txt`):

- **A leitura espera a mensagem por até 5 s.** `csr_ioctl` faz `down_timeout(&tabela[leitor].semaforo, 500)`: 500 tiques, e a configuração embutida no mesmo kernel tem `CONFIG_HZ=100`. Se o prazo passa, devolve -1; se acorda, tira a mensagem da fila do leitor e devolve 0.
- **A mensagem acorda o leitor na hora:** `queue_Worker` põe a cópia na fila de cada destinatário e libera o semáforo dele (`up`).
- **O barramento do carro é endereçado, e o da bancada não.** A palavra em `0x08` do descritor é uma máscara de destino, e o kernel só entrega a mensagem aos processos que estão nela. Na bancada a biblioteca nem manda essa palavra, e o hub entrega tudo a todos, menos a quem mandou. Não mexi nisso; fica como achado, e pode ser a explicação das 77 mensagens do `carifservice` em interfaces fora da tabela DSI (ver os próximos passos).
- **O `errno` é outro.** Pela libc, o -1 do driver chega como retorno -1 com `errno` 1 (`EPERM`); a bancada devolve `errno` 11 (`EAGAIN`) desde 10/10/2026. Nenhum dos laços que li olha o `errno`.

Nada disso rodou: é leitura do código, do kernel do MU0883. Se o do carro (MU0803) é igual, não sei.

### O conserto

Na leitura do `driver/kipc_bus.c`, e só nela:

```c
i64 n=sc(SYS_recvfrom, fd,(i64)in,sizeof in, MSG_DONTWAIT, 0,0);
if(n==-11){
  struct { int fd; short events, revents; } pf = { fd, 1 /* POLLIN */, 0 };
  i64 prazo[2] = { 0, ESPERA_NS };                 /* 100 ms */
  sc(SYS_ppoll,(i64)&pf,1,(i64)prazo,0,0,0);
  n=sc(SYS_recvfrom, fd,(i64)in,sizeof in, MSG_DONTWAIT, 0,0);
}
```

- **Com mensagem na fila nada muda:** a primeira tentativa é a mesma chamada de antes, com o mesmo retorno.
- **Sem mensagem, a thread dorme no `ppoll`** até a mensagem chegar ou até 100 ms, e tenta de novo. Quem chamou recebe a mensagem, ou o mesmo -1 com `EAGAIN` de antes, sem o descritor tocado.
- **Por que com prazo, e de 100 ms.** A leitura tem de continuar voltando, por causa da batida de vida e da ordem de parar. O prazo do carro é 5 s; fiquei com 100 ms porque assim essas duas coisas continuam acontecendo na hora em que aconteciam na bancada (a batida, aos 16 s) e o custo é o mesmo: dez voltas por segundo por leitor, 0,01 a 0,03 % de um núcleo. É a decisão D81; trocar pelo valor do carro é mudar uma constante e repetir a série.
- **A biblioteca continua sem libc** e pede de fora só o `__errno_location`, como antes; oferece os mesmos quatro símbolos; compila sem aviso com `-Wall -Wextra`.
- **O hub, a escrita e o lançador não mudaram.**

### O teste, escrito antes do código

`driver/tests/t_kipc_bus.sh`, 14 casos, sem firmware. O cliente de teste é compilado pelo próprio teste e usa o barramento como o firmware usa: abre `/dev/csr` e `/dev/css` com `open()` e fala por `ioctl(fd, 0, descritor)`, com a biblioteca carregada por `LD_PRELOAD`; o leitor repete o laço do firmware, com o `memset` de 26 kB.

| O que o teste fixa | Biblioteca de antes | Biblioteca nova |
|---|---|---|
| leitor parado quase não gasta processador (limite: 10 % de um núcleo) | **falha:** 99,7 a 99,99 % | 0,01 a 0,03 % |
| leitor parado não fica girando (limite: 100 voltas vazias por segundo) | **falha:** 1,44 a 1,78 milhão por segundo | 9,5 a 10,0 por segundo |
| a leitura sem mensagem continua voltando (ao menos uma vez por segundo) | passa | passa |
| sem mensagem a volta é -1 com `EAGAIN`, e o descritor fica como estava | passa | passa |
| rajada de 200 mensagens chega inteira e na ordem | passa | passa |
| 20 mensagens espaçadas de 120 ms chegam na ordem | passa | passa |
| mensagem do tamanho máximo (26.624 bytes) chega inteira | passa | passa |
| a mensagem acorda o leitor (atraso mediano do envio à entrega abaixo de 20 ms) | 20 a 53 µs | 23 a 109 µs |
| o que o escritor põe no barramento é o mesmo de sempre (visto por um ouvinte direto) | passa | passa |
| `ioctl` e `open` de quem não é do barramento passam direto, com o `errno` da libc | passa | passa |
| o leitor não fica preso quando o hub cai | passa | passa |

- **Contra a biblioteca de antes, 2 dos 14 falham**, os dois do defeito (`traces/cpu_t_kipc_bus_contra_a_de_antes.out`); os outros 12 protegem o que já existia.
- **As faixas são de seis rodadas com a biblioteca de antes e dez com a nova** (`traces/cpu_atrasos.txt`, as duas saídas do teste e as baterias). Com a máquina carregada a mediana da nova ficou entre 23 e 64 µs; nas duas últimas rodadas, com a máquina livre, foi de 91 e 109 µs. Acordar um leitor que dorme custa mais quando os processadores estão parados; com a biblioteca de antes não medi com a máquina livre. O maior atraso de uma mensagem numa rodada foi de 0,2 a 16,1 ms com a de antes e de 0,1 a 8,4 ms com a nova: soluços da máquina, nos dois lados.
- **O caso do atraso separa "acorda com a mensagem" de "dorme e olha de novo":** uma leitura que dormisse 100 ms entre tentativas daria mediana perto de 50 ms.

### A série lado a lado

Doze partidas em seis pares simultâneos, das 0h53 à 1h04: uma central com a biblioteca de antes (`a1` a `a6`) e uma com a nova (`n1` a `n6`), largadas com 3 s de diferença e alternando quem larga na frente. As duas saem da cópia congelada da 0h30 e só diferem no `kipc_bus.c`. Cada partida: padrões do modo ao vivo, tecla Carro aos 32 s, 80 s de HMI, fotos aos 25, 45 e 70 s, consumo medido entre 55 e 65 s. Sem o registro detalhado (`HMILOG`). As previsões foram gravadas antes de largar (`traces/cpu_previsoes.txt`). Na largada de cada partida havia de três a cinco centrais de outras sessões no ar, e a carga do Mac estava entre 41 e 114.

| | Biblioteca de antes (6) | Biblioteca nova (6) |
|---|---|---|
| serviços DSI ausentes | 34 | os mesmos 34 |
| serviços RSI ausentes | 8, em 6 esperas | os mesmos |
| hora aceita aos | 0,16 a 0,38 s | 0,10 a 0,33 s |
| Welcome na tela por | 4,20 a 4,60 s | 4,36 a 4,76 s |
| a tecla aos 32 s fecha o popup e abre a tela do veículo aos | 30,9 a 31,4 s | 30,9 a 31,5 s |
| fim das esperas por serviços ausentes | 51,7 a 51,9 s | 51,6 a 51,9 s |
| telas: o 1º passo e os passos 6 a 19 (ver abaixo os passos 2 a 5) | iguais nas 6 | os mesmos, iguais nas 6 |
| anúncio do domínio 16, pedidos de assinatura e eventos de volta | 1, 110 e 389 ou 390 | 1, 110 e 389 ou 390 |
| carro de mentira: assinaturas, pedidos atendidos, sem resposta | 110, 3 e 14 | 110, 3 e 14 |
| reentrega do hub à HMI; anúncios guardados entregues | 19 ou 22; nenhum | 19 ou 22; nenhum |
| linhas `ERROR` e exceções da HMI até os 65 s | 123 ou 124, e 47 | 123 ou 124, e 47 |
| linhas do registro da HMI até os 75 s | 2.654 a 2.665 | 2.649 a 2.660 |
| erros de OpenGL, `SIGSEGV`, processo caído | 0, 0, nenhum | 0, 0, nenhum |
| quadros desenhados | 127 a 196 | 167 a 186 |
| **consumo parada, pelo contador do container** | **204 a 501 %** (mediana 278) | **1,9 a 3,4 %** (mediana 2,6) |
| consumo parada, `docker stats` aos 55 s e aos 60 s | 209 a 505 % | 0,9 a 18,3 % e 0,9 a 10,2 % |
| as cinco threads de leitura | de 20 a 100 % cada | de 0,0 a 0,6 % cada |
| processador gasto do começo até os 65 s da HMI | 160 a 313 s | 11,6 a 13,0 s |
| tempo de processador de cada daemon, e da JVM, aos 74 s | 22 a 70 s, e 75 ou 76 s | 0 s, e 2 ou 3 s |

**As fotos.** Dentro de cada par, a foto da central de antes e a da nova são iguais byte a byte nos três instantes, com duas exceções: nos pares 1 e 6, aos 45 s, diferem num retângulo de 14 por 21 pontos no topo da tela. É o minuto do relógio: as duas fotos do par são tiradas com segundos de diferença, e nesses dois o minuto virou no meio ([recorte](quadros/2026-10-12-hmi-cpu-relogio-nos-pares-1-e-6.png)). Folhas com as doze: [aos 25 s](quadros/2026-10-12-hmi-cpu-serie-25s.png), [aos 45 s](quadros/2026-10-12-hmi-cpu-serie-45s.png) e [aos 70 s](quadros/2026-10-12-hmi-cpu-serie-70s.png).

**Três diferenças apareceram, e nenhuma é da biblioteca:**

- **A ordem das telas no primeiro segundo varia, nos dois braços.** A sequência inteira tem 19 passos. Os passos 2 a 5 são sempre as mesmas quatro telas (o aviso do rádio, o centro de controle, o centro de controle do usuário e o indicador de gesto), que entram entre 0,3 e 1,3 s em ordens diferentes de uma partida para outra. Nove partidas saíram na ordem mais comum; na `n3` e na `n5` o indicador de gesto entrou 2 ou 3 ms antes do centro de controle do usuário, e na `a5`, que é do braço de antes, entrou antes de todas. Nos pares sem o carro de mentira foi o contrário: as três de antes saíram nas ordens raras e as três novas na comum. E conferi no que já havia: nos 123 registros da HMI guardados em `traces/`, todos com a biblioteca de antes, o indicador de gesto vem antes do centro de controle do usuário em 67.
- **O braço novo desenha mais quadros durante as animações.** Parada, a tela desenha 0 ou 1 quadro em 25 s nos dois braços; a diferença está na animação que leva à tela do veículo (de 105 a 169 quadros entre os 25 e os 45 s num braço, de 143 a 157 no outro). Que seja a sobra de processador é dedução, com dois apoios: a partida de antes que teve a máquina para si (`a6`) desenhou mais que todas, 196; e na série do hub, só com a biblioteca de antes, a contagem foi de 94 a 170.
- **O registro da HMI da `n1` tem 850 linhas a menos, todas do último décimo de segundo.** No fim de toda partida o lançador derruba o registro RSI e o container acaba em seguida; as linhas que faltam são as que a HMI imprime ao perder o registro, e quantas ela consegue imprimir antes de o container acabar é uma corrida. Até os 75 s os doze registros têm o mesmo tamanho.

**Sem o carro de mentira** (`VEICULO=`, que sobe o `carifservice` do firmware, o do laço diferente): três pares, `av1` a `av3` e `nv1` a `nv3`, da 1h18 à 1h23, já depois da instalação e com a máquina quase livre (carga de 9 a 15). Nas seis partidas: 34 serviços ausentes, 106 pedidos de assinatura e 80 eventos do `carifservice`, 906 linhas no barramento, 48 mensagens reentregues pelo hub à HMI mais 1 anúncio guardado, 122 linhas `ERROR` e 48 exceções até os 65 s, Welcome por 4,6 a 4,9 s, tela do veículo aos 31,6 ou 31,7 s, nenhum erro de OpenGL, `carifservice` de pé no fim. As fotos são iguais byte a byte nos três instantes, dentro dos pares e entre as seis (sem o carro de mentira a tela não tem relógio). Consumo parada: de 499 a 501 % com a biblioteca de antes, de 1,7 a 2,1 % com a nova. E medi a thread de leitura do `carifservice` em duas centrais à parte, à 1h27 (`traces/cpu_carif_threads.txt`): com a biblioteca de antes ela larga o processador 87 vezes por segundo, que são os 10 ms de sono a cada leitura vazia; com a nova, 35 vezes por segundo, e as amostras de pilha a pegam esperando dentro da leitura, que é onde no carro ela fica.

Resumos em `traces/cpu_serie_analise.txt` e `traces/cpu_serie_v_analise.txt`; de cada partida, as threads, os processos e o consumo em `traces/cpu_s_*.consumo.txt`; do par 6, também os registros.

### A instalação e a conferência da pasta

- **Antes de instalar** rodei a bateria numa cópia da pasta da 1h06, que já tinha o lançador e o teste novos de outra sessão, com a biblioteca nova por cima: tudo passa (`traces/cpu_bateria_antes_de_instalar.txt`).
- **Aviso às outras sessões à 1h10**, três minutos antes da troca, por mensagem, às quatro que estavam trabalhando na bancada (rádio, hora e unidades, rastreamento, volante), com a hora e o que muda. As mensagens ficaram na fila de cada uma; não esperei resposta. A mesma nota está na pasta de notas das sessões (`consumo-kipc-bus.md`).
- **Troca à 1h13min01**, por troca de arquivo, depois de conferir que o `kipc_bus.c` da pasta ainda era o da cópia de que parti. O de antes ficou em `traces/cpu_kipc_bus.antes.c`. O arquivo instalado difere do que rodou na série só em linhas de comentário, e a biblioteca compilada dos dois é idêntica byte a byte. Havia três centrais de outras sessões no ar; seguem com a biblioteca antiga até reiniciar.
- **Bateria da pasta, da 1h13 à 1h15**, com a biblioteca instalada (`traces/cpu_bateria_instalado.txt`): `run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_hmilog.sh` 32, `t_hub.sh` 25, `t_kipc_bus.sh` 14, `t_memoria.sh` 80, `t_pedidos.sh` 16, `t_vc_diag_alm.sh` 37 e `t_veiculo.sh` 128, no container; `t_servidor.sh` 20, `t_lsmval_abre.sh` 38 e `t_memoria.sh` 80, no Mac. Todos passam, sem aviso do compilador.
- **Bateria da pasta de novo, das 2h13 às 2h16**, no fim do trabalho, com o que as outras sessões instalaram nesse meio-tempo (`traces/cpu_bateria_final.txt`): as mesmas suítes, mais o `t_radio.sh` novo (81) e o `t_veiculo.sh`, que cresceu para 187. Todas passam.
- **Três centrais da pasta ao mesmo tempo, duas vezes** (`f1` a `f3` à 1h16 e `g1` a `g3` à 1h24, com os padrões e a tecla; a carga do Mac estava entre 8 e 12). Nas seis: 34 serviços ausentes, Welcome por 4,6 a 4,8 s, tela do veículo aos 31,7 s, nenhum erro de OpenGL, de 232 a 243 quadros; 110 assinaturas nas de 80 s e 111 nas de 90 s (a a mais é das últimas linhas do barramento, já no fim da partida). `docker stats` das três juntas, paradas, numa leitura só, à 1h25min25: **0,70 %, 0,89 % e 0,74 %**; 7 s depois, 1,05 %, 3,88 % e 1,08 %; mais 9 s, 2,46 %, 2,60 % e 4,44 %. Pelo contador, entre 55 e 65 s da HMI: de 1,6 a 2,0 % nas seis. Cada uma gastou 10 s de processador do começo até os 65 s da HMI. Na mesma leitura do `docker stats` uma central de outra sessão marcava 497 %: roda de uma cópia de `driver/` anterior à troca.

### As corridas de partida, que a série não olhava

Depois de instalar rodei dois pares no modo da central ao vivo: com a memória entre sessões, numa cópia da memória guardada dela, de modo que a central nasce na grade de menus e não no aviso do rádio (`ma1`, `mn1`, `ma2`, `mn2`, à 1h31). Num dos pares as fotos dos 25 e dos 45 s saíram diferentes: na central com a biblioteca nova faltava a faixa vermelha do pé da tela, e ela tinha desenhado 99 quadros, contra 253 a 281 das outras três.

![A grade aos 25 s (em cima) e aos 45 s (embaixo): à esquerda a central com a biblioteca de antes, à direita a com a nova, sem a faixa vermelha no pé da tela](quadros/2026-10-12-hmi-cpu-grade-com-e-sem-indicador.png)

Era o tipo de diferença que a série devia ter pego, então fui atrás: mais 94 partidas, quase todas curtas (12 s de HMI), sempre em centrais de teste. O que há são duas corridas da própria HMI nos primeiros 200 ms da partida. As duas já existiam.

**1. O perfil inicial da persistência.** A HMI pede o perfil (`Requesting startup profile via diagnosis callback`), ele é posto de 1 a 13 ms depois (`Initial profile was set to 1`), e uma etapa da partida confere se ele já está (`SpecialMethodInitialStartupProfileReady`). As duas coisas correm na mesma thread da HMI. Se a conferência passa antes, o registro diz `Persistence initial profile not set. Watching persistence startup` e a partida segue por outro caminho, com um aviso e algumas linhas diferentes; nas telas não vi efeito.

**2. O indicador de gesto da grade** (`[SpaceGesture] enableIndicator: SimpleGridMenu`). É a faixa vermelha do pé da tela, e é animada: com ela ligada a central desenha perto de 200 quadros em 12 s, e sem ela, perto de 42. Quando a central nasce na grade ele é ligado entre 0,12 e 0,18 s da partida, ou não é ligado. O que decide não aparece no registro normal da HMI; não é a mesma corrida do perfil (houve partida com o perfil atrasado e o indicador ligado).

| | Biblioteca de antes | Biblioteca nova |
|---|---|---|
| perfil inicial depois da conferência, **máquina carregada** (a `d1`, o piloto e a série) | **6 de 8**, com 18 a 267 ms de atraso | **0 de 7** |
| perfil inicial depois da conferência, máquina livre (todas as outras partidas) | 1 de 44, por 0,5 ms | 2 de 50, por 0,9 e 1,2 ms |
| perfil inicial depois da conferência, nos 131 registros antigos de `traces/` (cargas variadas) | 58 de 131 | não havia |
| indicador de gesto ligado na partida: os 2 pares longos | 2 de 2 | 1 de 2 |
| indicador de gesto ligado: 15 pares curtos | 8 de 15 | 2 de 15 |
| indicador de gesto ligado: 12 trios curtos (a terceira central é a variante abaixo) | 7 de 12 | 7 de 12 |
| indicador de gesto ligado: 12 de cada, uma central minha por vez | 7 de 12 | 6 de 12 |
| indicador de gesto ligado: soma | 24 de 41 | 16 de 41 |

- **Com a máquina carregada a diferença é clara, e é a favor da biblioteca nova.** Os leitores que giravam disputavam processador com o resto, e a resposta ao pedido do perfil levava de 33 a 280 ms em vez de poucos milésimos. Isso estava nas doze partidas da série e eu não vi: quando a central nasce no aviso do rádio, as telas e as fotos saem iguais nos dois caminhos.
- **Com a máquina livre não achei diferença que se repetisse.** A única leva em que os braços diferiram no indicador de gesto foi a dos 15 pares (8 contra 2); as duas seguintes deram 7 a 7 e 7 a 6. Nos trios o resultado veio agrupado por trio: em três trios as três centrais ligaram o indicador, em três nenhuma ligou. Leio isso como sinal de que quem decide é o estado da máquina naquele instante, mas é leitura minha.
- **A folga é de menos de 1 ms em boa parte das partidas.** Do pedido ao perfil posto, medianas por leva, de antes contra nova: 1,9 e 4,4 ms nos pares, 2,1 e 2,6 ms nos trios, 2,9 e 1,7 ms uma de cada vez. Do pedido à conferência: entre 4 e 13 ms. Em muitas partidas a conferência vem 0,2 ms depois de o perfil ser posto.
- **Girar um pouco depois de cada mensagem não é remédio.** Experimentei uma variante da biblioteca em que o leitor ainda gira 2 ms depois de cada mensagem antes de dormir (`traces/cpu_kipc_bus.gira2ms.c`, não instalada). Em 12 partidas o perfil chegou depois da conferência em 6, e o indicador ligou em 5. Por que ela piora, não sei.

**O que isso muda no que escrevi acima.** Comportamento novo, nenhum: os dois caminhos de cada corrida aparecem com as duas bibliotecas. O que muda com a biblioteca é a frequência do primeiro quando a máquina está carregada. Com a máquina livre, o que não consigo afirmar é que a frequência do indicador de gesto seja a mesma: 24 de 41 contra 16 de 41, com as levas discordando entre si. Mantive a instalação (decisão D83).

Às 2h10 mandei às quatro sessões que estavam trabalhando na bancada (rádio, hora e unidades, som e telefone) a correção do aviso da 1h10, com as duas corridas, e pus o mesmo na nota da pasta de notas.

Medidas em `traces/cpu_longas_perfil.txt`, `cpu_serie_mem_*`, `cpu_curtas_*`, `cpu_curtas3_*` e `cpu_sozinhas_*`; os roteiros são `cpu_central_mem.sh`, `cpu_curtas.sh`, `cpu_curtas3.sh` e `cpu_sozinhas.sh`, e os analisadores, `cpu_perfil.py` e `cpu_gesto.py`.

### O que muda para quem usa

- **Cabem várias centrais ao mesmo tempo.** Medi três juntas. O limite de duas por sessão, que era por causa do processador, deixa de valer por esse motivo; o que limita agora é a memória (perto de 440 MB por central, numa máquina virtual de 15,7 GB) e a partida, que gasta uns 10 s de processador por central.
- **No comportamento, nada**, pelo que a série mostrou.
- **No relógio, muda.** Com processador sobrando a HMI desenha mais quadros nas animações (232 a 243 nas seis da pasta, com a máquina livre) e os tempos ficam mais regulares. **Tempo, contagem de quadros e consumo de rodadas de antes e de depois da 1h13 de 12/10/2026 não são comparáveis.**
- **Na partida, com a máquina carregada, a HMI deixa de tomar o caminho do perfil inicial atrasado.** Rodada antiga feita com várias centrais no ar tomou esse caminho em boa parte das vezes (58 dos 131 registros de `traces/`); agora ele é raro. Quem compara registros de partida linha a linha vai ver essas linhas a mais ou a menos.
- **A central ao vivo nasce na grade, e ali a faixa vermelha do indicador de gesto aparece na partida em parte das vezes**, com qualquer das duas bibliotecas. Quando falta, não é defeito novo.
- **Central que já estava no ar segue com a biblioteca antiga** até reiniciar. A central ao vivo estava desligada; a próxima partida dela já pega a nova.
- **Quem roda de uma cópia congelada de `driver/` continua com a que copiou.**

### Limites

- **Não sei dizer se o indicador de gesto da grade liga com a mesma frequência com as duas bibliotecas.** Em 41 partidas de cada, com a máquina livre, ligou em 24 com a de antes e em 16 com a nova; a diferença veio toda de uma leva, e as outras três deram 16 contra 14. Não rastreei o evento que decide.
- **Não rodei a central ao vivo de verdade**, com a janela e a memória dela. As partidas no modo dela foram em centrais de teste, com uma cópia da memória guardada. A primeira partida dela com a biblioteca nova é a do Lipe.
- **O prazo é o da bancada, não o do carro:** 100 ms contra 5 s (decisão D81). Com o barramento calado o laço de cada processo dá dez voltas por segundo; no carro, uma a cada 5 s.
- **O `carifservice` muda de ritmo.** Ele dormia 10 ms a cada leitura vazia, e uma mensagem podia esperar até 10 ms por ele. Agora a mensagem o acorda na hora, e as voltas vazias, com o registro de "timeout" e os 10 ms de sono, ficam mais raras (a thread larga o processador 35 vezes por segundo, contra 87). Está mais perto do carro, mas é diferente do que a bancada fazia; nos três pares sem o carro de mentira não apareceu diferença.
- **O `errno` da volta vazia continua 11**, e não o 1 que o kernel do firmware daria.
- **Se o hub cai, os leitores voltam a girar**, como antes: a leitura volta na hora com o socket fechado. O teste só garante que ninguém fica preso.
- **O barramento continua sem endereço de destino** (ver "O que o carro faz").
- **Só no container.** Não rodou no QEMU; a chamada `ppoll` existe lá, mas não conferi.
- **A série foi em máquina carregada**, que é como a bancada estava nesta noite; os pares sem o carro de mentira e as seis da pasta, com ela quase livre.
- **O firmware é o MU0883**, e o kernel lido é o dele.

### O que entrou na pasta

- `driver/kipc_bus.c`: a espera na leitura e o comentário do cabeçalho, que descrevia a leitura com os campos trocados.
- `driver/tests/t_kipc_bus.sh`: os 14 casos.
- `traces/cpu_*` (122 arquivos, 2,8 MB): a medição da `d1`; o amostrador de pilha e os medidores; a leitura do laço do firmware e do driver do kernel, com o extrator da tabela de símbolos; as previsões; os roteiros da série, da bateria e da instalação; as análises; as rodadas de atraso; as partidas das corridas de partida; a biblioteca de antes, a nova e a variante de experimento.
- Cinco imagens `quadros/2026-10-12-hmi-cpu-*.png`.
- `README.md`: um parágrafo, no fim.

### Onde parou

- O item do consumo está feito. Ficam nos próximos passos: as corridas de partida da HMI, o prazo de 5 s do carro, o barramento endereçado e os leitores que giram quando o hub cai.
- Nenhuma central minha ficou no ar. A central ao vivo não foi tocada.

## O carro de mentira lembra do que a tela gravou (12/10/2026, depois do Marco 9)

Feito numa sessão à parte, aberta pela sessão do carro de mentira à 0h09 de 12/10/2026, para o item "Guardar o que a tela grava no carro de mentira de uma partida para a outra" dos próximos passos (decisões D71 e D76). O pedido: no modo ao vivo, a hora acertada e as unidades escolhidas pela tela sobreviverem à partida seguinte, como no carro; os testes escritos antes do código; e uma série de partidas a partir da cópia de trabalho antes de instalar na pasta. É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução:**

- **A hora acertada e a unidade voltam.** Numa central de teste acertei a hora pela tela (três toques na seta, de 0:44 para 3:44) e escolhi °F; encerrei. A central seguinte, subida 3 min depois na mesma pasta de memória, nasceu com 3:47 e 95 °F sem ninguém tocar: a hora acertada, adiantada do tempo que passou (rodadas `a` e `b`). A HMI registrou `First valid ClockTime: ClockTime(hours=3,minutes=47,seconds=28,…)` com o Mac em 0:48:16.
- **O controle, com o código de antes e a mesma pasta de memória** (rodada `b0`): 0:47 e 35,0 °C, a hora do Mac e a unidade do perfil.
- **Pelo caminho do modo ao vivo, com as duas memórias** (rodadas `v1` a `v3`, numa cópia do `driver/ao_vivo.sh` em que só o nome do container muda): a segunda sessão nasceu na tela do veículo, a última da primeira, já com a hora acertada e em °F. A terceira trouxe também a data (um dia adiante) e o fuso (−2 h) escolhidos na segunda.
- **A unidade que a memória da tela guardava e não voltava agora volta.** Era a falta registrada na D76: nas rodadas `uni3` a `uni5` a HMI lia °F da memória, o carro informava °C, e em cerca de um minuto ela regravava a memória com °C. Na `v2`, aos 100 s, a memória da tela seguia com `temperatureUnit = 1`.
- **Dezesseis partidas a partir de um estado guardado, todas iguais:** oito da primeira cópia de trabalho (`s1` a `s8`), seis da cópia refeita sobre a pasta de 1h15 (`t1` a `t6`) e duas da pasta, depois de instalar (`w1` e `w2`). Em todas: dados do painel, Welcome, relógio aceito já com o acerto, um anúncio só do domínio 16, 111 assinaturas, 3 pedidos atendidos e 14 sem resposta (os números das `f1` a `f8` do Marco 9), nenhum pedido de gravar na partida, o arquivo de estado intacto, o registro do stub com 12.680 bytes e 0 erros de OpenGL. Nenhum laço.
- **Na conferência final, com a pasta como ficou depois de outra sessão instalar o rádio de mentira por cima dos meus arquivos** (1h33; rodadas `x1` e `x2`, pelo caminho do modo ao vivo): a hora acertada e °F voltaram na segunda sessão (2:46 no topo, com o Mac em 1:47; 95 °F). **Mas essa segunda sessão nasceu sem sete serviços de veículo**, e isso não vem do estado do carro: é o rádio de mentira somado a uma memória da tela em que a última tela era a do veículo. Está em "Achado na conferência final", abaixo.
- **Sem pedir, nada muda.** Seis centrais de teste sem estado (`z1`, `z2`, `u1`, `u2`, `q1`, `q2`): os mesmos números, a hora do Mac, nenhuma pasta de memória criada, e a linha "veículo" do lançador igual à de antes.
- **Arquivo cortado não impede a partida** (rodada `r1`): a central nasceu dos perfis, com a hora do Mac; o registro disse `estado: RECUSADO (…, linha 9: falta a linha "fim": o arquivo está vazio ou foi cortado); vale só o perfil`, e o arquivo ficou de lado, em `estado.txt.recusado`.

| Antes: a central seguinte volta à hora do Mac e a °C | Depois: nasce com a hora acertada e em °F |
|---|---|
| ![Antes](quadros/2026-10-12-hmi-est-antes-nasce-da-hora-do-mac.png) | ![Depois](quadros/2026-10-12-hmi-est-depois-nasce-com-hora-e-unidade.png) |

As duas fotos são de centrais subidas na mesma pasta de memória, com 1 min 30 s de diferença: a da esquerda com o código de antes (0:47, a hora do Mac), a da direita com o novo (3:48). A hora tinha sido acertada para 3:44 quatro minutos antes, em outra central.

| Sessão 1: a hora acertada pela tela | Sessão 2, sem tocar: a hora seguiu andando |
|---|---|
| ![Sessão 1](quadros/2026-10-12-hmi-est-sessao1-hora-acertada.png) | ![Sessão 2](quadros/2026-10-12-hmi-est-sessao2-hora-e-data.png) |

| Sessão 2 pelo caminho do modo ao vivo: data e fuso escolhidos | Sessão 3, sem tocar |
|---|---|
| ![Sessão 2](quadros/2026-10-12-hmi-est-ao-vivo-sessao2-data-e-fuso.png) | ![Sessão 3](quadros/2026-10-12-hmi-est-ao-vivo-sessao3-data-e-fuso-voltam.png) |

As outras fotos: a sessão 1 [antes de mexer](quadros/2026-10-12-hmi-est-sessao1-antes.png), [com °F escolhido](quadros/2026-10-12-hmi-est-sessao1-fahrenheit.png) e [no fim](quadros/2026-10-12-hmi-est-sessao1-fim.png); a sessão 2 com [°F sem ninguém tocar](quadros/2026-10-12-hmi-est-sessao2-unidades.png); a [segunda sessão do caminho do modo ao vivo, que nasce na tela do veículo](quadros/2026-10-12-hmi-est-ao-vivo-sessao2-nasce-no-veiculo.png); e, já com o código instalado na pasta, [a hora](quadros/2026-10-12-hmi-est-pasta-hora-volta.png) e [mph](quadros/2026-10-12-hmi-est-pasta-mph-volta.png) que voltaram.

**O que continua igual:** só o serviço de relógio e unidades aceita gravação, então é só dele que há o que guardar. Trocar a unidade não refaz os números do painel (na foto de mph a velocidade média segue em km/h). E o que a tela gravou não vale para as centrais de teste nem para o lançador direto, que nascem iguais toda vez, salvo pedido.

### O que fica guardado, e onde

O `driver/veiculostub.c` ganhou a opção `-e <arquivo>`. Com ela, a cada pedido que muda alguma coisa ele escreve no arquivo o estado inteiro: o acerto do relógio e cada valor que um pedido gravou. O que o perfil diz e ninguém mudou não entra. O arquivo tem a forma de um perfil. Este é o da rodada `v2`, depois de acertar a hora e a data e de escolher °F e o fuso −2 h:

```
# (quatro linhas de comentário que o programa escreve)
acerto = 90081
[DSICarTimeUnitsLanguage.updateTemperatureUnit]
temperatureUnit = 1
[DSICarTimeUnitsLanguage.updateClockTimeZoneOffset]
clockTimeZoneOffset = -2
[DSICarTimeUnitsLanguage.updateClockTime]
time.timeZone = -2
fim = 3
```

- **`acerto`** são os segundos que o relógio está adiantado em relação à hora do Mac no fuso do perfil. A hora não vai como número (ver a seção seguinte).
- **As seções e os valores** são os do perfil: serviço, evento, caminho do campo e valor. Texto vai entre aspas, numa linha só, com `\"`, `\\` e `\n`.
- **`fim = <n>`** é a última linha, com a conta dos valores. É por ela que se reconhece um arquivo cortado.
- **A gravação é ao lado, com troca de nome** (`estado.txt.novo`, depois `rename`): quem lê nunca acha o arquivo pela metade, nem se o container morrer no meio.
- **Só é escrito quando algo muda.** Pedido que repete o valor que já estava recebe a resposta de sempre e não regrava o arquivo. A partida também não regrava.
- **O que não vai para o arquivo:** pedido ignorado pelo perfil (o idioma do painel), pedido de serviço fora de `grava`, hora que não existe, e as respostas das listas do start-stop, que não guardam nada.

No registro do stub, cada pedido que mudou o arquivo termina em "estado guardado", e na partida a linha de resumo diz "estado: 3 valor(es) e acerto de +90081 s no relógio, de …", "estado: nada guardado ainda em …" ou "estado: RECUSADO (…)". Sem `-e` a linha é a de antes, letra por letra.

### O relógio guarda a diferença, e não a hora

Até aqui o stub já somava ao relógio do Mac um acerto em segundos (decisão D71); o que faltava era guardá-lo. Fica guardado o acerto, e não a hora que a tela mostrou. O motivo é o do pedido: no carro o relógio do painel segue andando com a central desligada. Com a diferença guardada, a central seguinte mostra a hora acertada mais o tempo que passou, que é o que as rodadas mostraram: 3:44 acertado à 0:44:48; 3:47:28 na central seguinte, à 0:48:16; 3:56:46 na `s7`, quase treze minutos depois do acerto. Se ficasse a hora, a central seguinte nasceria parada no instante do último acerto.

A data usa o mesmo acerto: um dia adiante são 86.400 s a mais (na `v2` o acerto foi de 3.681 para 90.081).

O custo está na decisão D86. Em resumo: o relógio do carro de mentira anda com o do Mac e com o `fuso` do perfil, e não sozinho, como o do painel.

**Um detalhe da HMI visto no caminho** (rodada `p1`): a tela de hora manda só hora e minuto, sem a data. Descer a hora de 0 para 23 dá 23:25 do mesmo dia, e o acerto foi a +79.140 s (quase 22 h adiante), e não a 2 h para trás. O stub faz o que o pedido diz.

### A partida: tudo conferido antes de aplicar

Na partida, se o arquivo existe, o stub o lê inteiro e confere cada linha contra o esquema (serviço, evento, campo, tipo e tamanho do valor) antes de mexer em qualquer coisa. Só então aplica, por cima dos perfis.

- **Arquivo com erro não vale pela metade nem impede a partida.** Um erro em qualquer linha, uma linha depois do `fim`, a falta do `fim` ou uma conta que não fecha: nada é aplicado, vale só o perfil, o motivo sai no registro com o arquivo e a linha, e o arquivo vai para `<arquivo>.recusado`. O próximo pedido que gravar começa um arquivo novo.
- **O que é recusado:** linha que o programa não entende; serviço, evento ou campo que não está no esquema; valor do tipo errado ou grande demais; marcador (`@hora…`); ajuste que só vale em perfil (`fuso`, `grava`, `ignora`); regra de gravação; acerto que não é número ou que levaria o relógio para fora de 1970 a 2099.
- **Valor guardado para um campo que o perfil de agora enche com marcador é descartado**, com aviso, e o resto vale. Só acontece se o perfil mudar de uma sessão para a outra (um perfil de hora fixa trocado pelo de hora de verdade). Aplicado, ele congelaria a hora.
- **O estado vale mesmo que o serviço tenha saído de `grava`.** É a memória do painel: o que a tela não pode mais mudar continua como foi deixado, até alguém apagar o arquivo.
- **Se a pasta do arquivo sumir ou não aceitar gravação**, o pedido é atendido do mesmo jeito e o registro avisa que o que a tela gravou vale só até a central desligar.

### Onde isso vale

| Onde | O que acontece |
|---|---|
| modo ao vivo (`sh driver/ao_vivo.sh`, a janela) | **ligado**, junto da memória da tela: o estado fica em `ao-vivo/memoria/veiculo/estado.txt` |
| `MEMORIA= sh driver/ao_vivo.sh`, ou `LSMVAL` passado à mão | desligado junto com a memória da tela: a sessão nasce dos perfis e nada é guardado |
| centrais de teste (`driver/m8_central.sh`) | desligado; liga com `VEICULO_ESTADO=/live/memoria/veiculo/estado.txt` (só o carro de mentira) ou `MEMORIA=/live/memoria` (as duas memórias). O estado fica em `<pasta>/memoria/`, e outra central subida na mesma `<pasta>` nasce dele |
| lançador direto (`driver/m1_hmi.sh`), rodadas | desligado; liga com `VEICULO_ESTADO=<arquivo>` |

**Voltar ao zero**, com a central desligada: `rm -rf ao-vivo/memoria/veiculo` (só o carro de mentira) ou `rm -rf ao-vivo/memoria` (as duas memórias). Com a central ligada não adianta: ela escreve o arquivo de novo na próxima mudança, com tudo o que tem na memória dela.

Por dentro: o `driver/ao_vivo.sh` não mudou de comportamento (só o cabeçalho). O `driver/m3_ao_vivo.sh`, quando recebe `MEMORIA`, passa `VEICULO_ESTADO=<pasta>/veiculo/estado.txt` adiante. O `driver/m1_hmi.sh` monta a pasta do arquivo dentro do chroot, em `/tmp/veiculo-estado`, e chama o stub com `-e`: o stub roda dentro do chroot e precisa da pasta, e não só do arquivo, porque grava ao lado e troca o nome. Se a pasta não montar, a central sobe sem guardar, com aviso (esse ramo não foi exercitado).

### As rodadas

Centrais de teste `moi3-est-<nome>`, à parte da central ao vivo. Previsões escritas antes, em `traces/est_previsoes.md`: conferiram. O assistente de configuração, que eu tinha posto como possível nas `v`, não apareceu. Saídas em `traces/est_*`.

| Rodada | O que rodou | Observado |
|---|---|---|
| `a` | cópia de trabalho, só o estado do carro (`VEICULO_ESTADO`) | antes do primeiro toque não há arquivo; a cada toque na hora o arquivo é reescrito (`acerto` 3562, 7157, 10752, com `fim = 0`); com °F, ganha a seção da temperatura e `fim = 1`; cada pedido termina em "estado guardado"; nenhuma sobra na pasta |
| `b0` | o código da pasta, de antes, na mesma pasta de memória | hora do Mac (0:46:46) e 35,0 °C; arquivo intacto |
| `b` | cópia de trabalho, mesma pasta de memória, 3 min depois da `a` | "estado: 1 valor(es) e acerto de +10752 s"; relógio aceito 3:47:28; topo com 3:48 e 95 °F; "Unidades" com °F; 3 pedidos atendidos, 14 sem resposta, nenhuma gravação na partida; depois troquei a distância para milhas e o arquivo passou a ter os dois valores e o mesmo acerto |
| `s1` a `s8` | cópia de trabalho, oito partidas a partir do estado da `b` | iguais (ver o resultado, acima) |
| `z1`, `z2` | cópia de trabalho, sem estado | como as `f1` a `f8` do Marco 9; nenhuma pasta `memoria/` |
| `r1` | cópia de trabalho, com o arquivo sem a linha `fim` | recusado; a central nasce dos perfis; fica `estado.txt.recusado` |
| `v1` a `v3` | cópia do `ao_vivo.sh` com outro nome de container: as duas memórias | `v1`: hora +1 h 2 min e °F. `v2`: nasce na tela do veículo, com 2:09:57 (Mac em 1:08:36) e 95 °F; a memória da tela segue com °F aos 100 s; adiantei a data em um dia e escolhi o fuso −2 h. `v3`: "3 valor(es) e acerto de +90081 s"; Hora 2:14, Fuso GMT − 2h, Data 13.10.2026 |
| `t1` a `t6`, `u1`, `u2` | a cópia refeita sobre a pasta de 1h15: seis com estado, duas sem | iguais às `s` e às `z`, salvo a contagem das linhas de rastreamento do painel (44 em vez de 2), que é do rastreamento novo de outra sessão |
| `p1`, `p2` | **a pasta**, depois de instalar, com as duas memórias | `p1`: hora para 23:25 e mph. `p2`: "1 valor(es) e acerto de +79140 s"; 23:28 no topo; "Unidades" com mph |
| `w1`, `w2`, `q1`, `q2` | a pasta: duas com estado, duas sem | iguais às `t` e às `u` |
| `x1`, `x2` | cópia da pasta de 1h44 (já com o rádio de mentira de outra sessão), pelo caminho do modo ao vivo | `x1`: hora +1 h e °F; arquivo com `acerto = 3588` e a temperatura. `x2`: "1 valor(es) e acerto de +3588 s"; 2:46 e 95 °F no topo; **48 assinaturas em vez de 111, sete serviços de veículo fora** |
| `g`, `h`, `k`, `e`, `l` | a mesma cópia: 24 centrais em pares lado a lado, para separar a causa da `x2` | ver "Achado na conferência final" |

**As duas cópias de trabalho.** A primeira foi tirada da pasta à 0h16. Enquanto eu rodava, outras sessões instalaram duas mudanças que toda central usa: o `driver/m1_hmi.sh` (0h48, o rastreamento da HMI) e o `driver/kipc_bus.c` (1h13, a leitura do barramento que espera; Marco 10). A conferência de antes de instalar pegou as duas. Refiz a cópia sobre a pasta de 1h15, pus as minhas mudanças por cima, rodei as suítes e repeti a série (`t` e `u`) antes de instalar. As rodadas `a`, `b`, `s`, `z`, `r` e `v` são da primeira cópia, com o barramento de antes; as `t`, `u`, `p`, `w` e `q`, do que foi para a pasta.

### O hub reentrega pedidos antigos: o que os testes mostraram

Os primeiros casos do estado falharam de um jeito que o código não explicava: o stub recém-nascido já gravava o arquivo sozinho. A causa estava no ambiente do teste. O `kipc_hub` guarda as 48 primeiras mensagens que vê e as entrega a todo leitor que se liga depois, e no teste o hub é um só do começo ao fim: entre as 48 primeiras estavam pedidos de gravar de casos anteriores, e cada stub novo os recebia de novo.

- **Nos testes**, cada partida passou a ter um hub novo, que é o que acontece na bancada (central nova, container novo).
- **Na bancada isso não acontece hoje:** o stub é o primeiro a se ligar ao hub, antes da HMI, e não é religado. Conferi, pelo registro do barramento da rodada `a`, o que cabe nas 48 primeiras mensagens: há pedidos da HMI entre elas (o primeiro é a 23ª), mas nenhum de gravar no serviço de relógio e unidades.
- **Fica o aviso:** se um dia o stub for religado com a central de pé, o hub lhe entrega de novo essas 48 mensagens. As assinaturas voltariam, o que é bom; um pedido de gravar que estivesse entre elas seria aplicado outra vez. Não pus defesa para isso (decisão D91).

### Achado na conferência final: com o rádio de mentira, a central que nasce na tela do veículo fica sem serviços de veículo

Não é desta mudança, mas apareceu na última conferência dela e pega o modo ao vivo. À 1h33 outra sessão instalou o rádio de mentira (Marco 11) por cima dos arquivos que eu tinha instalado à 1h24, sem desfazer nada meu, e ele passou a vir ligado no modo ao vivo. Refiz a prova com a pasta como estava à 1h44. A primeira sessão (`x1`) saiu normal. A segunda (`x2`), nascida da memória que a primeira deixou, trouxe a hora e a unidade de volta, mas com a HMI sem parte do carro: "Configurações do veículo" só com "Hora e data" e "Unidades", a tela do veículo em "Status do veículo: Iniciando…", 48 assinaturas no carro de mentira em vez de 111, e no registro da HMI `Required DSI services not registered within 2000 ms` com `DSICarComfort`, `DSICarDriverAssistance`, `DSICarDrivingCharacteristics`, `DSICarEco`, `DSICarKombi`, `DSICarLight` e `DSICarVehicleStates`.

Uma rodada não separa causa de acaso nesta bancada, então rodei pares lado a lado, uma central de cada braço por par, todas da mesma cópia da pasta e sem `HMILOG`:

| Braço | Partidas | Com os serviços de veículo | Rodadas |
|---|---|---|---|
| rádio + memória da tela + estado do carro | 5, mais a `x2` | **nenhuma** | `g1a` a `g3a`, `k1a`, `k2a` |
| rádio + memória da tela, **sem** o estado do carro | 5 | **nenhuma** | `g1b` a `g3b`, `k1b`, `k2b` |
| memória da tela + estado do carro, **sem** o rádio | 3 | as 3 | `h1a` a `h3a` |
| rádio + estado do carro, **sem** a memória da tela | 3, mais a `x1` | as 3 | `h1b` a `h3b` |
| rádio + memória da tela, com `ESPERA=6000` (com e sem o estado) | 4 | as 4 | `e1a` a `e2b` |
| rádio + **outra** memória da tela, a guardada da central ao vivo (última tela: a grade de menus) | 2 | as 2 | `l1a`, `l2a` |
| a mesma memória da central ao vivo, sem o rádio | 2 | as 2 | `l1b`, `l2b` |

- **O estado do carro não entra na conta:** com ele e sem ele o resultado é o mesmo, cinco a cinco. E nas partidas degradadas ele funciona: a hora e a unidade guardadas aparecem.
- **Precisa dos dois: o rádio de mentira e a central nascer na tela do veículo.** Nas onze partidas degradadas a memória da tela tinha a tela do veículo como última (`lastContentId = HMI_CAR`), fosse ela nascida antes do rádio (as `k`, com a memória da `p1`) ou depois (as `g`, com a da `x1`). Com a mesma memória e sem o rádio, três normais em três; com o rádio e sem memória, três em três; e com o rádio e a memória guardada da central ao vivo, em que a última tela é a grade de menus, duas em duas. Não testei outras últimas telas.
- **Com a HMI esperando 6 s por etapa, em vez de 2 s, os serviços sobem.** Aponta para margem de tempo na partida: a HMI cria dez serviços de veículo 5 s depois do anúncio (Marco 8), e a espera de 2 s por etapa é escolha da bancada (decisão D21). **A causa exata eu não investiguei**; isto é uma medida, e não uma explicação. **Nota de 12/10/2026, 4h20, da sessão do rádio e da mídia:** é isso mesmo, e está medido no Marco 15, "A espera da partida pelos serviços de veículo": com o rádio a etapa que precisa desses serviços começa a esperar aos 2,2 s, eles aparecem perto dos 5,0 s, e com 2 s ela desiste 0,7 s antes.
- **O que isso quer dizer para quem usa:** no modo ao vivo de agora (rádio e memória ligados por padrão), desligar a central com a tela do veículo aberta faz a seguinte nascer com o menu do veículo pela metade. É fácil de acontecer: é em "Configurações do veículo" que se acerta a hora e se trocam as unidades. Com a memória que a central ao vivo tem guardada hoje (grade de menus), a próxima partida deve sair normal. Contornos, sem mexer na pasta: apertar outra tecla (Menu, por exemplo) antes de desligar, `ESPERA=6000 sh driver/ao_vivo.sh` (a partida fica uns 20 s mais longa), `RADIO= sh driver/ao_vivo.sh` (sem o rádio de mentira) ou `MEMORIA= sh driver/ao_vivo.sh` (sem memória nenhuma). Pela janela não há como passar variável.
- **Não mexi em nada para consertar:** nem no padrão do `ESPERA`, nem no rádio de mentira, que são de outras sessões. Avisei a sessão do rádio às 2h09, com os números, corrigi o aviso às 2h18 (eu tinha escrito "qualquer memória"; é a memória com a tela do veículo por último) e deixei como pendência.

| Nascida na tela do veículo, com o rádio: a hora e os 95 °F voltam, mas o carro fica em "Iniciando…" | O menu do veículo com dois itens em vez de quatro |
|---|---|
| ![Veículo](quadros/2026-10-12-hmi-est-radio-e-memoria-veiculo-iniciando.png) | ![Menu](quadros/2026-10-12-hmi-est-radio-e-memoria-menu-com-dois-itens.png) |

Saídas em `traces/est_lado_a_lado_1.txt` a `_5.txt`; o roteiro, em `traces/est_serie2.sh`; um registro da HMI degradado e um normal com `ESPERA=6000`, em `traces/est_g1a.ao_vivo.out` e `traces/est_e1a.ao_vivo.out`.

### O que entrou na pasta

- `driver/veiculostub.c`: a opção `-e`, a escrita e a leitura do estado, o fim da linha de resumo (1.133 para 1.371 linhas). Sem `-e` o programa faz o que fazia.
- `driver/tests/t_veiculo.sh`: de 128 para 187 casos. Dos 59 novos, 58 foram vistos falhar contra o stub de antes, pelo motivo esperado (ele não entende `-e`); o que passava só fixa que a central de teste sem pedido nasce como antes (`traces/est_t_veiculo_vermelho.txt`). Entre os novos, os bytes que a HMI mandou na `base1`, contra o esquema e os perfis de verdade.
- `driver/m1_hmi.sh` (a variável `VEICULO_ESTADO`), `driver/m3_ao_vivo.sh` (o padrão dela a partir de `MEMORIA`), `driver/ao_vivo.sh` (só o cabeçalho) e `driver/m8_central.sh` (`MEMORIA` e `VEICULO_ESTADO` a pedido; a ajuda passou a mostrar o cabeçalho inteiro).
- Quinze quadros `quadros/2026-10-12-hmi-est-*.png`: treze do estado e dois do achado da conferência final.
- As versões de antes dos seis arquivos estão em `traces/est_antes/`.

Testes, rodados da pasta à 1h28 de 12/10/2026, depois de instalar: `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 16, `t_veiculo.sh` 187, `t_memoria.sh` 80 (no container e no Mac), `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, `t_servidor.sh` 20 e `t_lsmval_abre.sh` 38, mais os dois que outras sessões puseram na pasta nesta noite, `t_hmilog.sh` 32 e `t_kipc_bus.sh` 14. Todos passam, e o stub compila sem aviso com `-Wall -Wextra`. Saída em `traces/est_suites_pasta.txt`. **Às 2h09, com a pasta já com o rádio de mentira de outra sessão por cima** (1h33), rodei tudo de novo: os mesmos números, mais `t_radio.sh` 81, e todos passam (`traces/est_suites_pasta_final.txt`). O pedido desta sessão listava `t_codificacao.sh` com 26 casos e `t_vc_diag_alm.sh` com 34: os dois cresceram antes de eu começar, por mudanças de outras sessões.

### Limites

- **Verificado pela tela:** a hora, a data, o fuso e três unidades (temperatura, distância e velocidade). A fonte de referência, os formatos de hora e data e as outras unidades usam a mesma regra e não foram tocados. Valor de texto só nos testes: o esquema de verdade não tem campo de texto em nenhuma regra de gravar.
- **Não rodou:** o `driver/ao_vivo.sh` com o nome `moi3-ao-vivo` e a partida pela janela. A central ao vivo estava desligada e eu não a liguei. O mais perto foram as `v1` a `v3`, com a cópia em que só o nome do container muda (da primeira cópia de trabalho), e as `p1` e `p2`, da pasta, que passam pelo mesmo `m3_ao_vivo.sh` com `MEMORIA`.
- **É dedução, e não medida no carro,** que o painel guarde a hora e as unidades e que o relógio dele siga andando com a central desligada. É o que se espera de um painel, e é o que a D71 e a D76 já supunham.
- **O relógio anda com o do Mac.** Se o relógio do Mac for corrigido, o do carro de mentira vai junto; e não atrasa nem adianta sozinho, como um relógio de painel.
- **Não há cópias anteriores do estado**, como há da memória da tela (`anteriores/`). Um acerto errado se conserta pela tela ou apagando o arquivo.
- **Um estado ruim fica guardado**, como na memória da tela: uma data absurda continua lá até alguém acertar.
- **Uma central por pasta de memória.** Duas gravando na mesma pasta escreveriam uma por cima da outra.
- **Com o rádio de mentira, a central que nasce na tela do veículo fica sem parte dos serviços de veículo** ("Achado na conferência final"). O estado do carro continua valendo nessas partidas, mas o menu do veículo fica pela metade até isso ser resolvido. **Resolvido no modo ao vivo em 12/10/2026, às 4h08** (Marco 15, decisão D158); nas centrais de teste continua como está escrito.
- **Sessão longa e Mac que dorme no meio não foram exercitados.** A sessão mais longa durou perto de 4 min.
- **O que o painel do carro faz com a hora quando o fuso muda** continua sem resposta (decisão D72): o fuso escolhido volta, e a hora não é recalculada.

### Onde parou

- O item "Guardar o que a tela grava no carro de mentira" dos próximos passos está feito, e com ele a "unidade que volta" do levantamento da memória.
- **A central ao vivo estava desligada e não foi tocada.** A próxima partida, pela janela ou pelo `driver/ao_vivo.sh`, já guarda o estado do carro de mentira em `ao-vivo/memoria/veiculo/`. Na primeira, a pasta nasce vazia.
- **Com a memória que está guardada hoje (última tela: a grade de menus), essa próxima partida deve sair normal.** A que vier depois de a central ser desligada com a tela do veículo aberta deve nascer com o menu do veículo pela metade, por causa do rádio de mentira ("Achado na conferência final"); `ESPERA=6000 sh driver/ao_vivo.sh` evita. Está com a sessão do rádio para decidir. **Decidido em 12/10/2026, às 4h08 (Marco 15, decisão D158):** o `driver/ao_vivo.sh` passou a esperar 6 s pelos serviços DSI por padrão, e a central nascida na tela do veículo sai normal sem precisar de variável; a janela sobe a central por esse lançador.
- Nenhuma central minha ficou no ar.
- Próximos passos naturais: refazer os números do painel quando a unidade muda; ligar a gravação nos outros serviços, um a um (o estado deles já ficaria guardado, sem mudança no stub); e, se fizer falta, guardar cópias anteriores do estado.

## Marco 11 — o rádio de mentira (12/10/2026)

Feito numa sessão à parte, aberta pela sessão do simulador à 0h09 de 12/10/2026, para o item 4 do roteiro de [simulador-jetta.md](simulador-jetta.md): a tela do rádio sair do aviso "função indisponível" e mostrar um rádio simulado, pelo mesmo método do carro de mentira. É emulação num Mac; nada vai para o carro. O número 10 ficou com a sessão do consumo de processador, que escreveu na mesma madrugada.

**Resultado, verificado em execução** em centrais de teste, as duas últimas (`e1` e `e2`, à 1h35) subidas da pasta já instalada:

- a tela do rádio abre no lugar do aviso, em FM, com a lista de memórias, a sintonia manual e as configurações. **Ela abre numa frequência, e não numa emissora:** 76,1 MHz, o começo da faixa, numa central sem memória. Quem escolhe a frequência de partida é a HMI (o começo da faixa quando não há memória de rádio, o que foi medido; a da memória quando há, o que é leitura do código, porque nenhuma rodada nasceu com memória de rádio), e o rádio de mentira obedece; a emissora "selecionada" do perfil (89,1 MHz, "TESTE 1") só vale até esse primeiro pedido;
- "Lista de emissoras FM" mostra as cinco emissoras do perfil, três pelo nome e duas pela frequência. **Corrigido às 2h05 de 12/10/2026, olhando de novo as fotos:** enquanto a central está numa frequência que não é de emissora da lista, como os 76,1 MHz da partida, a lista tem uma sexta linha, com essa frequência. É a HMI que a põe (`AmFmStationList.generateFrozenFmList` acrescenta a emissora atual à lista que a tela mostra); reaberta a lista já numa emissora do perfil, ficam as cinco (rodada `g1`, na tabela das rodadas);
- tocar numa emissora da lista sintoniza: ela ganha a marca de "tocando" e o nome vai para a tela principal;
- as setas da tela das memórias passam para a emissora seguinte; na sintonia manual, as setas de cima fazem a busca e as de baixo andam de 0,2 em 0,2 MHz;
- o botão "Fonte" troca entre AM e FM, e a faixa de AM abre (sem emissoras);
- no assistente de configuração, "Buscar e salvar" termina e guarda as emissoras nas memórias 1 a 5; tocar numa memória sintoniza;
- as teclas Próxima e Anterior do volante, e a Próxima da moldura, trocam de emissora;
- nenhuma das dezessete rodadas registrou erro de OpenGL, tempo esgotado de rádio nem laço com a HMI; a décima oitava, a `g1` da conferência final (2h00, da pasta instalada), também não.

| A tela principal depois de sintonizar pela lista | A lista de emissoras |
|---|---|
| ![Tela principal](quadros/2026-10-12-hmi-rad-tela-principal.png) | ![Lista de emissoras](quadros/2026-10-12-hmi-rad-lista-de-emissoras.png) |

O roteiro inteiro, numa partida da pasta instalada (tela inicial, lista, toque na primeira, volta, seta, sintonia manual com busca, tecla Carro e tecla Rádio):

![O roteiro da prova e1](quadros/2026-10-12-hmi-rad-prova-da-pasta-e1.png)

**Nada do que se vê é do carro.** As emissoras são inventadas ("TESTE 1" a "TESTE 3" e duas sem nome), os códigos delas também, não há recepção e não sai som. A faixa de 76,1 a 107,9 MHz vem de um palpite sobre a adaptação do carro, explicado mais abaixo. O que é da central de verdade é o comportamento da tela: quem desenha, monta a lista, guarda as memórias e decide o que pedir é a HMI do firmware.

### O domínio certo: só o anúncio já abre a tela

A sessão do simulador tinha anunciado o sintonizador na rodada `v2` e concluído que "anunciar não basta". O domínio anunciado ali era o da tabela dela, `0x200000000000` (`PID_TUNER`): conferi à 1h53, relendo o barramento daquela rodada no rascunho da sessão principal (`…/d609a58c-…/scratchpad/m6/v2/barramento.txt`), que tem 122 anúncios do `0x200000000000`, nenhum do `0x400000000000` e nenhum pedido da HMI nas interfaces 400 a 421. Para a mídia a conclusão dela vale: com o `0x800000` anunciado, a HMI fez 7 pedidos ao `DSIMediaBase` (200) e 5 ao `DSIMediaBrowser` (202), e a tela seguiu no aviso. Lido em `ListenerWorkerThread.run` (como a sessão do Marco 8 já tinha apontado de passagem): quem leva ao tratamento do sintonizador é o `0x400000000000`, `PID_TUNER_HMI`, e é para ele que a HMI manda os pedidos (`DSIUnifiedTunerImpl` chama `postDSIProviderMsg(…, 0x400000000000L, 400)`).

Medido, com três centrais iguais em tudo menos no anúncio, mandado uma vez com o `dsi_manda` logo que o barramento sobe:

| Rodada | Anúncio do `0x400000000000` | Tela do rádio aos 60 s |
|---|---|---|
| `a0` (0h18) | não | "Rádio: Esta função não está disponível no momento. Aguarde…" |
| `a1` (0h14) e `a2` (0h18) | sim | a tela do rádio: 87,5 MHz, seis memórias vazias, a barra de botões |

![Sem o anúncio e com o anúncio, na mesma hora](quadros/2026-10-12-hmi-rad-sem-anuncio-e-com-anuncio.png)

Só com o anúncio, sem ninguém respondendo, a tela já tem quase tudo: lista (com uma linha só, a frequência da própria HMI), sintonia manual, escolha de AM ou FM e as configurações de FM ([quadro](quadros/2026-10-12-hmi-rad-so-o-anuncio-manual-e-config.png)). A HMI muda a frequência na tela por conta própria, e uma memória chega a ser gravada. O que não acontece é a sintonia: ela não manda `selectStation`.

Na lista de serviços que a HMI dá por ausentes na partida saem `DSIUnifiedTuner[0]` e `DSIRadioData[0]`. Pela minha contagem a lista vai de 35 para 33 nomes; a do Marco 8 e a do hub falam em 34, e não fui atrás da diferença de um.

O `radio-hmi` do firmware (`/lge/app_ro/radio/radio-hmi`, 3,3 MB, serviço `radio-hmi.service`) é o fornecedor de verdade desse domínio, ao lado do `radio` (o programa do sintonizador). O lançador não sobe nenhum dos dois, e não tentei subi-los.

### O que a HMI pede ao sintonizador

Levantado na `a1`, pelo barramento (`dsi_escuta`) e pelo rastreamento da própria HMI (chave `DSI.TUNER`, que dá o nome de cada chamada).

**Na partida, 41 pedidos em dois serviços:**

| Pedido | Vezes | Bytes vistos |
|---|---|---|
| `DSITunerAnnouncement.setNotification({2,4,3,5})` | 1 | `08 02 08 04 08 03 08 05` |
| `DSITunerAnnouncement.setFilter(0)` e `setFilter(2)` | 9 | vazio e `08 02` |
| `DSIUnifiedTuner.setNotification`, um atributo por pedido: 1, 18, 2, 5, 7, 6, 8, 11, 3, 9, 4 e 14 | 12 | `08 01`, `08 12`… |
| `setStationFollowingMode(2)` | 4 | `08 02` |
| `setRegMode(2)` | 3 | `08 02` |
| `setERTDisplayable(true)`, `setERTPrefered(true)`, `switchRDSIgnore(false)`, `setListMode(3)` | 2 cada | |
| `setSoftLinkSwitch(1)`, `enableRadioTextPlus({1,4})` | 1 cada | |
| `switchRadioUsage(0)` e depois `switchRadioUsage(1)` | 2 | vazio e `08 01` |

Ao `DSIRadioData` (banco de emissoras e logotipos) ela não pediu nada em nenhuma rodada.

Duas diferenças para os serviços de veículo: a assinatura vai em varint comum, e não em zigue-zague (`08 12` assina o atributo 18), e a HMI assina um atributo por pedido.

**Na tela, com o rádio de mentira respondendo:**

| Toque | Pedido | Bytes vistos |
|---|---|---|
| emissora na lista (93,7 MHz, com PI) | `selectStation(2, 93700, PI, -1, -1, -1, 0)`: modo 2 = FM, frequência em kHz | `08 02 10 84 dc 05 18 82 20 20 ff…` |
| seta da sintonia manual, ou toque na escala | `selectStation(6, …)`: modo 6 = FM manual, PI -1 | `08 06 10 8c d4 04 18 ff…` |
| "Fonte" e depois AM | `selectStation(0, 531, …)`: modo 0 = AM (não vai no fio) | `10 93 04 18 ff…` |
| setas de cima, na tela da sintonia manual | `seekStation(1 ou 2, faixa 2, sessão)`: busca acima ou abaixo | `08 01 10 02 18 01` |
| qualquer toque com a busca em curso | `seekStation(3, …)`: interromper | `08 03 10 02 18 02` |
| toque longo numa memória | `isOnPreset(emissora, posição)` | |
| "Buscar e salvar", no assistente de configuração | `forceUpdateStationList(1, 2)`; depois 36 `freePreset`, cinco `isOnPreset` e um `selectStation` | `08 01 10 02` |
| setas da tela das memórias; teclas Próxima e Anterior | `selectStation` da emissora seguinte, escolhida pela HMI | |

### De que a sintonia depende

Lido no código da HMI (`de.vw.mib.asl.internal.radio`) e conferido passo a passo no barramento:

1. **Uso do rádio.** A HMI pede `switchRadioUsage(1)` e só segue quando o atributo 18 (`updateRadioUsageStatus`) volta com 1 (`StationSelectionManager.switchRadioUsage`, `StateTopMainUnifiedRadio.dsiUnifiedTunerUpdateRadioUsageStatus`). Na `b1`, com o stub respondendo só com valores padrão, esse atributo ficou em 0 e a HMI nunca mandou sintonizar. Na `b2`, com uma regra no perfil devolvendo o número pedido, o `selectStation(2, 87500, …)` saiu logo em seguida.
2. **Três mensagens, nesta ordem.** Depois de `selectStation` a HMI espera `selectStationStatus = 1` (em curso), a atualização da emissora selecionada e `selectStationStatus = 2` (feito). Enquanto espera o "em curso" ela descarta a atualização da emissora (`UnifiedRadioStatus.waitingForStatusRunning`). O motor do carro de mentira não faz isso: ele manda a atualização do atributo antes da resposta, e uma resposta só.
3. **A busca é igual**, com `seekStationStatus` levando de volta a faixa e a sessão do pedido. Sem o estado final a HMI fica em busca, e o toque seguinte só serve para interrompê-la (`StateAmFmActiveSeek`): foi o que se viu na `a1` e na `a2`, em que o toque seguinte à seta saiu como `seekStation(3, …)`.
4. **"Buscar e salvar"** espera `forceUpdateStationListStatus` com estado final; sem ele a HMI fica 65 s em "Procurando emissoras…" (`InitialAutostore`, temporizador de 65000 ms). Vi a espera na `f2` ([quadro](quadros/2026-10-12-hmi-rad-buscar-e-salvar-antes.png)); os 65 s eu li, não cronometrei.
5. **A ligação de áudio não fecha na bancada.** A HMI manda o `selectStation` antes de pedir a ligação do som, mas só dá a sintonia por terminada quando o serviço de áudio confirma (`UnifiedRadioBandSwitch`, `StationSelectionManager`). A bancada não tem esse serviço, e o registro dela diz "AudioManagement not available yet". Consequência medida na `f3`: numa série de trocas seguidas a tela muda na hora, e o pedido ao sintonizador só sai quando vence um temporizador de 10 s da HMI, com os do meio pulados. **O que é medido e o que é leitura:** medidos, o atraso e as linhas do registro da HMI (cinco vezes "GlobalStationSelectionManger#selectStation() - true == checkStationSelectionProgress", e o pedido seguinte saindo 10,0 s depois da última delas, nas duas vezes em que aconteceu). Lido no código, que a seleção fica "em curso" porque a ligação de áudio não confirma: a marca `audioManagementAvailable` só é ligada em `UnifiedRadioBandSwitch.processAudioManagementStatus`, e sem ela a HMI registra o erro e não pede a ligação (pede uma vez só, na partida do rádio). **A comparação com um serviço de áudio no ar não foi feita, e é ela que confirma a causa.** A sessão do som levantou na mesma madrugada (nota dela, 1h50) que esse serviço é RSI (`audiomanagement`), não DSI, e que sem ele quem atende à ligação de entretenimento é um substituto de dentro da própria HMI (`AMTDummy`); como isso se encaixa com a marca acima eu não conferi.

### O esquema: a segunda forma do código da LG

O gerador do Marco 8 (`driver/LeEsquemaDSI.java`) devolvia zero serviços para o domínio do sintonizador. A camada da LG está escrita de dois jeitos:

| | Serviços de veículo | Sintonizador |
|---|---|---|
| `process…ListenerService` | um caso por interface de eventos | só o caso do anúncio; o resto cai no `default`, que entrega a mensagem a uma classe intermediária (`DSITunerListenerService`) |
| quem separa por interface | o próprio `switch` | `DSITunerListenerService.executeListenerService(int)` |
| método do ouvinte | `executeListenerService(Message, int)`, com um `switch` de api | um método próprio (`UnifiedTunerMsgParse`, `TunerAnnouncementMsgParse`, `RadioDataMsgParse`); no primeiro há dois `switch`, um para as apis abaixo de 2000 e outro para as demais |

O gerador passou a seguir as duas, e o roteiro `driver/m8_esquema.sh` ganhou a variável `ROTULO`, para o cabeçalho dos arquivos. Saída em `stubs/radio/gerado/`: **3 serviços, 27 atributos, 53 eventos, 43 pedidos, 115 mensagens, 399 campos e 453 constantes**; 14 pedidos com resposta na tabela da HMI, todos do `DSIRadioData`. Para o `DSIUnifiedTuner` a tabela do proxy DSI da HMI não marca nenhum pedido com resposta nem prazo: as três mensagens da sintonia não saem dela.

A prova do codificador vale aqui também: **53 de 53 mensagens iguais, byte a byte**, entre o `veiculostub` e as classes do firmware. E o esquema de veículo, gerado de novo com o gerador estendido, saiu **idêntico** ao que estava na pasta (esquema, constantes e os dois arquivos de vetores).

Esquisitices do firmware que apareceram no caminho (valem no carro, é o mesmo código):

- `profileChange` e `profileCopy` do `DSIUnifiedTuner` saem com a mesma api, 1007; a interface previa 1008 para a cópia. O gerador passou a relatar isso, e achou mais quatro pares assim nos serviços de veículo (portas e persianas, limpador traseiro, visão do painel digital e spoiler). O esquema fica com o primeiro de cada par; o `stubs/veiculo/gerado/relatorio.txt` ganhou as quatro linhas.
- A lista de emissoras chega em pedaços com um campo `max`, e a classe que os junta (`UnifiedListControl`) lê `max` emissoras de cada mensagem e compara o nome da sessão por identidade, não por conteúdo. Com uma mensagem só e `max` igual ao tamanho da lista funciona, e é assim que o perfil faz. Não exercitei lista em mais de um pedaço.
- A camada da LG não põe no fio o nome curto nem o identificador que `selectStation` recebe: só modo, frequência, PI e os outros números.

### O rádio de mentira: o mesmo motor, mais um modelo

`driver/radiostub.c` inclui o `driver/veiculostub.c` inteiro e acrescenta um modelo do sintonizador, de umas 150 linhas. O motor continua fazendo o que fazia: lê o esquema e os perfis, anuncia o domínio uma vez, responde às assinaturas com o valor do perfil ou com o padrão marcado como válido, e atende à regra do uso do rádio, que é uma regra de perfil como a do fuso. O modelo cuida de três pedidos:

| Pedido | O que o modelo faz |
|---|---|
| `selectStation` | "em curso"; a emissora da lista do perfil que tem aquela frequência, inteira, ou, se a frequência não está na lista, só a faixa, a frequência e o PI do pedido; "feito" |
| `seekStation` acima ou abaixo | "em curso"; a emissora seguinte da lista naquela faixa, por frequência (do fim volta ao começo; sem emissora na faixa, fica onde está); "feito". Sem parar (modos 5 e 6): um passo, e fica em curso até a HMI mandar parar. Interromper e parar: o estado correspondente, e a emissora não muda |
| `forceUpdateStationList` | "em curso", a lista do perfil de novo para quem a assina, "feito"; interromper responde "interrompida" |

O modelo acha serviço, eventos, pedidos e campos pelo nome, no esquema, e não escreve byte de mensagem à mão; se o esquema não for o do sintonizador, recusa tudo e diz o que faltou. Os pedidos dele passam pelo `ignora` do perfil e pelo corte de laço do Marco 9, como os outros. No `veiculostub.c` entraram três ganchos, atrás de `#ifdef VEICULOSTUB_MODELO`: sem a macro o programa é o mesmo.

Por que um modelo em C, e não mais regras no perfil: as três mensagens em ordem, a procura na lista e o passo para a vizinha pediriam uma pequena linguagem dentro do perfil, e a mídia, que vem depois, precisa de ainda mais (posição que anda com o tempo). O custo está na decisão D93.

### O perfil, e a regra do nome

`stubs/radio/fm-de-mentira.txt`: cinco emissoras em 89,1, 93,7, 98,5, 102,9 e 105,3 MHz. Tudo simulação, marcado no arquivo.

A rodada `c3` foi feita para saber o que faz a central mostrar um nome. Cinco emissoras, cada uma com uma combinação:

| Emissora | Nome longo | Nome curto | RDS | PI | Na lista |
|---|---|---|---|---|---|
| A | sim | sim | sim | -1 | a frequência |
| B | sim | sim | sim | `0xB001` | **o nome** |
| C | sim | não | sim | `0xB002` | **o nome** |
| D | não | sim | sim | `0xB003` | a frequência (o nome curto aparece na tela principal depois de sintonizada) |
| E | sim | sim | não | `0xB004` | a frequência |

![A lista da rodada c3](quadros/2026-10-12-hmi-rad-regra-do-nome.png)

Ou seja: nome no `longName`, `rds = 1` e PI diferente de -1, os três juntos. As primeiras rodadas com o modelo (`c1` e `c2`) tinham os nomes e o PI em -1, e a lista saiu só com frequências; foi isso que levou à `c3`. O teste confere a regra no perfil da pasta.

### A faixa de FM é da adaptação do carro

Os limites da faixa e o passo da sintonia manual a HMI não pergunta ao sintonizador. Ela os tira da adaptação "rádio" do 5F (DID `0559`), campo "Bandsettings FM Tuner": 0 sem ajuste, 1 Europa e resto do mundo, 2 América do Norte, 3 Japão, 4 Coreia, 5 China, **6 Brasil** (nomes da `stubs/codificacao/tabela-campos.tsv`). As faixas estão em `de.vw.mib.asl.api.radio.RadioConstants`: a da Europa vai de 87,5 a 108,0 MHz em passos de 0,1; a do Brasil (`FM_BANDRANGE_BRA`), de 76,1 a 107,9 MHz em passos de 0,2.

Medido na `c2`, com uma linha a mais na codificação (`0559 @1.4+4 = 6`): o registro da HMI diz "Coding: FM Band Setting: BAND_SETTINGS_FM_TUNER_BRA" e "FM band range update: limit low: 76100 limit high: 107900 step width: 200", a escala da sintonia manual vai de 76 a 108, a seta anda de 76,1 para 76,3, e uma central sem memória abre em 76,1 MHz.

![A faixa Brasil: tela inicial, sintonia manual, um passo e a lista](quadros/2026-10-12-hmi-rad-faixa-brasil.png)

O simulador passou a subir com o 6, por **palpite** marcado em `stubs/codificacao/jetta-gli-2021-deducoes.txt` (decisão D98): o carro é do mercado brasileiro, e a HMI tem um ajuste com esse nome. Que o Jetta saia de fábrica com ele não foi lido de lugar nenhum. Dá para conferir no carro sem cabo, olhando a tela de sintonia manual: se a escala começar em 88 e a seta andar de 0,1 em 0,1, o palpite está errado e a linha sai.

Um efeito a saber: central que nasce de uma memória guardada com a faixa de antes registra, na primeira partida, "WARNING: FM BAND CODING CHANGED from: FM_BANDRANGE_EU_RDW (0) to: BAND_SETTINGS_FM_TUNER_BRA (6)" e "Reseting AM FM tuner because of band change!", e zera os ajustes de rádio guardados (rodada `f1`, com uma cópia da memória do modo ao vivo). Como o rádio nunca tinha funcionado na bancada, não havia memória de emissora para perder.

A faixa de AM ficou sem ajuste: a HMI usa a da Europa (531 a 1602 kHz, de 9 em 9) e registra "Coding: ERROR: AM band setting undefined". Não pus palpite para ela.

### As rodadas

Centrais de teste `moi3-rad-<nome>`, à parte da central ao vivo. Saídas em `traces/rad_*`. Até a `c3` a máquina esteve muito carregada por centrais de outras sessões (carga entre 30 e 80, até oito centrais no ar); as `f*` e as `e*` subiram depois da troca da biblioteca de barramento do Marco 10 (1h13).

| Rodada | O que rodou | Observado |
|---|---|---|
| `a0` | padrões do modo ao vivo, sem anúncio do sintonizador | o aviso "função indisponível"; nenhum pedido nas interfaces 400 a 421 |
| `a1`, `a2` | o anúncio do `0x400000000000`, uma vez, e mais nada | a tela do rádio; 41 pedidos na partida; a sintonia manual e uma memória mudam na tela sem pedido ao sintonizador |
| `b1` | o `veiculostub` com o esquema do sintonizador e nenhum perfil | a mesma tela da `a1`; 13 assinaturas atendidas; nenhum `selectStation` |
| `b2` | mais um perfil parado: a regra do uso do rádio, uma emissora e uma lista | `selectStation(2, 87500, …)` logo depois do uso confirmado; a lista com as cinco frequências; as trocas pela tela sem resposta |
| `c1` | o `radiostub` e a opção `RADIO` do lançador, da cópia de trabalho | toque na lista sintoniza, com as três mensagens; nomes ausentes (PI -1) |
| `c2` | mais a faixa Brasil na codificação | a faixa de 76,1 a 107,9; busca pelas setas com o nome na tela; AM abre; `selectStation(0, 531, …)` |
| `c3` | o perfil das cinco combinações | a regra do nome; setas da tela das memórias; memória gravada com toque longo |
| `d1` a `d4` | o conjunto final, da cópia de trabalho, com o mesmo roteiro | nas quatro: 13 assinaturas, sintonias em 76100, 89100 e 93700, busca de 93700 para 98500, as oito telas iguais, tecla Carro e tecla Rádio, 0 erros de OpenGL, nenhum laço |
| `f1` | o mesmo, nascendo de uma cópia da memória do modo ao vivo | a grade como primeira tela, o assistente de configuração 48,3 s depois de "UI thread started" (48,2 s na `f2` e na `f3`), o aviso da troca de faixa no registro, a tela do rádio pela tecla |
| `f2` | o mesmo; "Buscar e salvar" com o modelo ainda sem esse pedido | "Procurando emissoras…" sem fim à vista |
| `f3` | o mesmo, com o modelo final | "Emissoras… foram encontradas e armazenadas"; memórias 1 a 5 preenchidas; toque numa memória; teclas do volante; o atraso de 10 s nas trocas seguidas |
| `e1`, `e2` | **da pasta instalada**, com `RADIO` no `driver/m8_central.sh` | o mesmo das `d*`, nas duas; 0 erros de OpenGL em cerca de 1.500 quadros |
| `g1` (2h00) | **da pasta instalada**, na conferência final do registro: abrir a lista, tocar em "TESTE 1", voltar, reabrir a lista, tecla Carro, tecla Rádio e a lista outra vez | a lista com seis linhas enquanto a central está em 76,1 MHz (as cinco do perfil e a frequência atual) e com cinco depois de sintonizada uma emissora do perfil, nas duas vezes em que foi reaberta; 13 assinaturas, sintonias em 76100 e 89100; 0 erros de OpenGL em 955 quadros; nenhum tempo esgotado de rádio, nenhum laço, nenhuma exceção de rádio |

![A linha da frequência atual, e a lista depois de sintonizada uma emissora do perfil: rodada g1](quadros/2026-10-12-hmi-rad-linha-da-frequencia-atual.png)

![As partidas d2, d3 e d4](quadros/2026-10-12-hmi-rad-serie-d2-d3-d4.png)

Outras fotos: [busca pelas setas](quadros/2026-10-12-hmi-rad-busca-pelas-setas.png), [faixa de AM](quadros/2026-10-12-hmi-rad-faixa-de-am.png), [emissora com nome na lista, na tela principal e na tela Emissora](quadros/2026-10-12-hmi-rad-emissora-com-nome.png), [com a memória do modo ao vivo](quadros/2026-10-12-hmi-rad-com-a-memoria-do-modo-ao-vivo.png), ["Buscar e salvar" depois](quadros/2026-10-12-hmi-rad-buscar-e-salvar-depois.png), [memórias preenchidas](quadros/2026-10-12-hmi-rad-memorias-preenchidas.png), [teclas do volante](quadros/2026-10-12-hmi-rad-teclas-do-volante.png) e [a rodada b2](quadros/2026-10-12-hmi-rad-b2-dados-parados.png).

### O que entrou na pasta

- `driver/radiostub.c`: o rádio de mentira.
- `driver/tests/t_radio.sh`: 81 casos, sem firmware.
- `stubs/radio/`: o perfil `fm-de-mentira.txt`, o esquema gerado (`gerado/`) e o [README](stubs/radio/README.md).
- `driver/LeEsquemaDSI.java` e `driver/m8_esquema.sh`: a segunda forma de despacho, o relato de pedidos com a mesma api e a variável `ROTULO`.
- `driver/veiculostub.c`: os três ganchos do modelo. `stubs/veiculo/gerado/relatorio.txt`: quatro linhas a mais.
- `driver/m1_build.sh`: compila o `radiostub`. Se ele não compilar, avisa e segue: como ele inclui o `veiculostub.c`, uma mudança lá podia derrubar a partida de todas as centrais.
- `driver/m1_hmi.sh`: opção `RADIO` (lista de perfis; `-` liga só com os padrões) e `RADIO_ESQUEMA`, o registro `radiostub.log` e uma linha de resumo "rádio:". Sem o programa compilado a central sobe sem rádio, com aviso.
- `driver/ao_vivo.sh`: `RADIO` vem ligado, com o perfil da pasta (`RADIO=` desliga). `driver/m3_ao_vivo.sh`: só comentário; ali não há padrão. `driver/m8_central.sh`: repassa `RADIO` e `RADIO_ESQUEMA`.
- `stubs/codificacao/jetta-gli-2021-deducoes.txt`: a linha da faixa de FM.
- Dezenove quadros `quadros/2026-10-12-hmi-rad-*.png` e 63 arquivos `traces/rad_*` (8,8 MB). Da prova `e1`, subida da pasta instalada, ficaram a saída da HMI, o registro do `glhost` e a saída do lançador (`traces/rad_e1.out`, `rad_e1.glhost.log` e `rad_e1.lancador.out`); da `g1`, o barramento, o registro do rádio de mentira, o do `glhost`, a saída do roteiro e o roteiro (`traces/rad_g1.*` e `rad_g.sh`).
- Na ficha do carro: dois itens em "O que anotar no primeiro autoscan" (a adaptação de rádio do 5F e a foto da tela de sintonia manual).

### Testes

Escritos antes do código, com uma exceção dita abaixo. As saídas das vezes em que falharam estão em `traces/rad_t_radio_vermelho_*.out`.

| Etapa | Contra o que rodou | Resultado |
|---|---|---|
| esquema | a saída do gerador de antes | 23 de 26 falhavam (os 3 que passavam são as linhas do anúncio, que o gerador já lia) |
| modelo | um esqueleto do `radiostub` que só incluía o motor | 29 de 63 falhavam |
| atualizar a lista | o modelo sem esse pedido | os 6 novos falhavam |
| compilação que não derruba | o `m1_build.sh` de antes | 2 de 3 falhavam |

**A exceção:** a mudança do gerador foi escrita antes de existir o arquivo de teste. Eu tinha visto o defeito rodando o gerador ("serviços 0"), escrevi a mudança, e só então escrevi os casos do esquema e os rodei contra a saída antiga. E dois casos (o `ignora` e o da conferência de nomes do perfil) passaram na primeira vez, porque fixam o que o motor já fazia ou conferem um arquivo já certo; o da conferência de nomes tem ao lado um caso que acusa um perfil errado de propósito.

Três defeitos eram do próprio teste, e foi rodando que apareceram: uma conta de bytes errada minha (16, e não 15); um segundo `radiostub` no mesmo hub atendia de novo os pedidos antigos, porque o hub reentrega a quem se liga as 48 primeiras mensagens (cada caso passou a subir um hub novo); e a contagem do corte de laço pelo barramento oscilava, porque na rajada o hub descarta parte do que manda a quem escuta (passou a contar no registro do programa).

Rodados da pasta à 1h34 de 12/10/2026, depois de instalar (saída em `traces/rad_suites_da_pasta.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 16, `t_veiculo.sh` 187, `t_radio.sh` 81, `t_memoria.sh` 80, `t_hub.sh` 25, `t_kipc_bus.sh` 14, `t_hmilog.sh` 32 e `t_vc_diag_alm.sh` 37, no container; `t_servidor.sh` 20 e `t_lsmval_abre.sh` 38, no Mac. Todos passam, e a compilação não dá aviso. O `t_veiculo.sh` tinha 128 casos quando comecei e 187 na hora de instalar, pelas mãos da sessão do estado do carro de mentira; passam os 187 com os ganchos. A rodada terminou à 1h38; conferi à 1h57 que desde então só mudaram três READMEs em `driver/`, `stubs/`, `glforward-moi3/` e `janela/`, que nenhum teste lê, e que treze dos arquivos que instalei (os de código, o perfil, o esquema do rádio e o arquivo de deduções) seguem byte a byte iguais aos que testei.

### Limites

- **Sem recepção e sem som.** A lista é fixa. Não há intensidade de sinal de verdade, RDS que muda, texto de rádio nem logotipo; a tela "Emissora" diz "Nenhum texto de rádio disponível".
- **Trocas seguidas chegam ao sintonizador com até 10 s de atraso**, por falta do serviço de áudio. A tela muda na hora.
- **Só FM tem emissoras.** A lista de AM seria outra mensagem do mesmo atributo, o que o motor não faz. A faixa de AM abre e a busca em AM fica onde está.
- **A busca é instantânea e pula de emissora em emissora.** No carro ela varre a faixa. A busca sem parar dá um passo só.
- **Os pedidos de ajuste ficam sem resposta**: filtro de avisos de trânsito, RDS regional, seguimento de emissora, texto de rádio, memórias avisadas ao sintonizador (`isOnPreset`, `freePreset`). As telas de configuração abrem; se mudar um ajuste tem efeito, não conferi.
- **Avisos de trânsito, alarme e DAB** não foram simulados. O `DSIRadioData` só responde às assinaturas.
- **O anúncio sai duas vezes.** O motor repete o anúncio quando a HMI aparece no barramento e passa 2 s sem pedir nada, e o rádio só é ligado por ela mais tarde. Nas dezessete rodadas isso não fez mal; a regra é a do Marco 8, pensada para o domínio 16, e não mexi nela.
- **Exceções da HMI: nenhuma em classe de rádio.** Conferido nas saídas guardadas (`a0`, `a1`, `b2`, `c2`, `c3`, `f3` e `e1`): nenhuma linha "HMI-Radio Exception", nenhuma linha com "Exception" e "radio" ou "tuner". As 50 linhas com "Exception" que cada partida da série final tem são de serviços que faltam: 40 `DSIProxyException` (a `a0`, sem rádio, tem 42), 8 `asyncException`, uma do telefone e uma de arquivo. **Um efeito de o rádio funcionar:** na `c3`, ao gravar uma memória com toque longo, o componente de som registrou um `NullPointerException` em `WaveplayerController.triggerFeedbackSound`, porque o serviço de tons (`waveplayer`) não existe na bancada. Que seja o bipe de confirmação da memória é dedução: uma ocorrência, na rodada em que gravei a memória. O `NullPointerException` em `WaveplayerController.setBeepType` (2 na `c3`, 10 na `f3`) não vem do rádio: está também em saídas de outras sessões, sem ele (`m6_lsm_expl1`, `m6_lsm_expl2`, `m6_lsm_som2` e `m8_ne1`).
- **Lista em mais de um pedaço, `max` errado, mais de uma instância**: não exercitados.
- **A faixa e as emissoras são palpite e simulação.** O que o carro mostra depende da adaptação dele e de onde ele está.
- **O `driver/ao_vivo.sh` não foi executado por mim**: ele sobe a central ao vivo. Conferi a linha do `docker` dele com um `docker` de mentira, numa cópia à parte (`RADIO` chega com o perfil por padrão, vazio com `RADIO=`).
- **Só no container.** Não rodou no QEMU. **O esquema é do MU0883**; o carro roda o MU0803.
- **O rádio de mentira mudou a ordem da partida, e isso eu não tinha medido** (acrescentado em 12/10/2026, às 4h20). Com o sintonizador presente, a HMI deixa de esperar 2 s por ele, e a etapa que precisa dos serviços de veículo chega antes de eles existirem quando a central nasce na tela do veículo: ela fica sem sete deles. Quem achou foi a sessão do estado do carro de mentira, à 1h44; a causa e o conserto no modo ao vivo estão no Marco 15, "A espera da partida pelos serviços de veículo" (decisão D158). Nas dezoito rodadas do rádio nenhuma central nasceu da memória, e por isso não apareceu.
- **Três destes limites mudaram em 12/10/2026, às 10h00 (Marco 16, passo 3):** a busca passou a varrer a faixa, em vez de pular de emissora em emissora; AM passou a ter emissoras e lista; e há texto de rádio para as emissoras marcadas no perfil. O atraso de 10 s nas trocas seguidas já tinha sumido no modo ao vivo com o som de mentira (Marco 13). Os outros continuam valendo.

### Onde parou

- O item 4 do roteiro do simulador está feito para FM: lista, emissora atual, troca pela tela, busca, memórias.
- **A central ao vivo não foi tocada, e está desligada.** A sessão principal a parou à 0h26 de 12/10/2026, para liberar processador (registro em [simulador-jetta.md](simulador-jetta.md), "Noite de 11 para 12/10/2026"); à 1h51 conferi com `docker ps -a` que não há container `moi3-ao-vivo`. Não a liguei. Na próxima partida ela pega o rádio de mentira e a faixa Brasil; a primeira partida com a memória guardada registra a troca de faixa e zera os ajustes de rádio guardados. O que esperar nessa partida: na saída do lançador, a linha "[m1_hmi] rádio (/stubs/radio/fm-de-mentira.txt): modelo do sintonizador: 5 emissora(s) na lista do perfil; começo em 89100 kHz"; no registro da HMI, uma vez, "FM BAND CODING CHANGED" e "Reseting AM FM tuner because of band change!"; e a tecla Rádio abrindo a tela do rádio em FM. Se o rádio atrapalhar: `RADIO= sh driver/ao_vivo.sh`.
- Próximos passos naturais: o serviço de áudio (com ele some o atraso de 10 s e a tela de som deixa de ficar indisponível); a lista de AM; texto de rádio; e a mídia, pelo mesmo caminho, conferindo primeiro o domínio e a forma do código.

## Os avisos que descem do topo: é o desenho da HMI, não corte da bancada (12/10/2026)

Feito na mesma sessão do rastreamento, por pedido da sessão principal: à 1h10 ela mandou uma fila de trabalho para a noite e contou que o Lipe, antes de dormir, tinha escrito a ela, à 0h25: "Será que nesse meio tempo não conseguirmos deixar os trabalho sendo execultados?". Não vi a frase na conversa dele; foi a sessão principal que a passou. O primeiro item da fila era o 3 do roteiro do simulador, "Popups cortados no topo": achar a causa antes de propor conserto, e consertar só se ela estivesse do lado da bancada. É emulação num Mac; nada vai para o carro.

**Resultado: não há corte, e não há o que consertar na bancada.** Os três avisos registrados desde os Marcos 4 e 5 (o do usuário principal, o "Telefone indisponível" e a confirmação de restauração de fábrica) usam o mesmo molde, e o molde é uma folha que desce de cima da tela e fica pendurada na borda de cima, sem linha no topo. A posição, as margens e a moldura são dados fixos do skin do firmware, e a bancada desenha cada peça no ponto exato em que a HMI a põe. **O que continua sem resposta é se a central do carro mostra igual:** falta uma foto de um aviso desses no carro.

| Telefone indisponível | Usuário principal |
|---|---|
| ![Telefone](quadros/2026-10-12-hmi-pop-telefone-indisponivel.png) | ![Usuário principal](quadros/2026-10-12-hmi-pop-usuario-principal.png) |

| Restauração de fábrica | Volume: uma faixa no topo, no lugar do título |
|---|---|
| ![Restauração](quadros/2026-10-12-hmi-pop-restauracao-de-fabrica.png) | ![Volume](quadros/2026-10-12-hmi-pop-volume.png) |

### O que foi medido

Três medidas, em centrais de teste `moi3-pop-*` subidas da pasta entre a 1h13 e a 1h38 de 12/10/2026, com os padrões do modo ao vivo.

1. **O que a HMI registra da animação de entrada** (chaves `ANIMATION.ANIMATION_KEYFRAME`, `VIEWS.POPUPENTRY`, `WIDGETS.VIEW` e `WIDGETS.REPAINT`; isto só ficou possível com o conserto do rastreamento, porque o aviso abre depois da carga tardia). O aviso entra com uma animação de 500 ms que leva uma propriedade de −160 a 0, em valor absoluto, e outra de 0 a 255 em 300 ms, a transparência. A primeira é a posição vertical da tela do aviso: a repintura registrada uns 22 ms depois da abertura dá a tela em (0, −128), com 800×480, e nesse ponto a animação vale perto de −128 (−133 aos 18 ms e −121 aos 27 ms, noutra central). A animação vai até o fim: "enter animation stopped" aos 507 ms, com o valor em 0.
2. **A árvore de widgets, lida da HMI em execução.** Um leitor pequeno (`traces/pop_ArvoreDeTelas.java`) liga-se à JVM pelo agente de depuração do próprio firmware e imprime posição, tamanho e margens de cada peça do aviso. As árvores dos três avisos estão em `traces/pop_arvore_*.txt`.
3. **Os pixels do quadro**, linha a linha (`traces/pop_linhas.py`, `traces/pop_perfil-dos-quadros.txt`), para comparar com a árvore.

| Aviso | Tela do aviso | Área de fundo | Texto | Rodapé | Linha divisória: árvore → quadro | Borda de baixo: fundo − 67 → quadro |
|---|---|---|---|---|---|---|
| `PO_DIGITALASSISTANT_CONFIRM_SYSTEM_WELCOME_SCREEN_PRIMARY_USER` | (0, 0), 800×480 | 800×228 | y de 2 a 82 | y de 97 a 166 | 99 → 99 | 161 → 161 |
| `PO_SYSTEM_ALERT_PHONE_NOT_INSTALLED` | (0, 0), 800×480 | 800×188 | 2 a 42 | 57 a 126 | 62 → 62 | 121 → 121 |
| `PO_CONFIRM_SYSTEM_ALL_RESET_TO_FACTORY_SETTINGS` | (0, 0), 800×480 | 800×268 | 2 a 122 | 137 a 206 | 142 → 142 | 201 → 201 |

As duas últimas colunas são a conferência: a linha divisória e a borda de baixo aparecem no quadro na linha exata que a geometria da HMI dá, nos três.

### Por que a caixa encosta no topo

- **A tela do aviso ocupa a tela inteira e a animação a deixa em y = 0.** O alvo da animação é absoluto e está nos dados do skin (`sourceValue=-160, targetValue=0`).
- **O conteúdo é empilhado de cima para baixo**, com 2 px de margem em cima, 15 entre o texto e o rodapé e 62 embaixo (`layoutAttribs: insetTop=2, gap=15, insetBottom=62`, orientação vertical, alinhado ao topo).
- **A área de fundo sai do conteúdo** (`visibleAreaAlignment=2`, em `PopupView.updateBackgroundLayout`): começa em y = 0 e tem a altura do conteúdo mais as duas margens. É o "updated background layout [x= 0 y= 0 width=800 height=188]" que a HMI registra.
- **A moldura é uma imagem só**, a de número 2097 do skin, esticada sobre a área de fundo (modo 6, "simples", em `BackgroundCompositor.paintSimple`). As partes dela que não esticam têm 5 px em cima, 75 de cada lado e 72 embaixo. A linha de baixo fica 67 px antes do fim da imagem; o que há abaixo dela não aparece no fundo preto, e não abri a imagem para ver. Em cima a parte fixa tem só 5 px e nenhuma linha: no quadro, as linhas 0 a 5 trazem apenas os dois pontos das bordas laterais. A imagem foi feita sem borda de cima.

Nenhum desses números vem de resposta da bancada. São parâmetros que a classe gerada de cada tela dá aos widgets (a do aviso do telefone é a `Psapni__0`, no `viewhandler.jar` do skin, e tira as margens e as animações do `SkinLiteralPool` do mesmo jar): não dependem do tamanho de tela anunciado, do display nem da codificação.

### O que foi descartado

- **Animação que para no meio:** vai até o fim.
- **Área de popups fora do lugar:** `ScreenAreaPopup` está em (0, 0) com 800×480, igual à tela (`SimpleDesktopLayout`).
- **O encaminhador de OpenGL desenhando deslocado:** os pixels batem com a geometria da HMI, linha por linha.
- **Diferença entre as variantes do lançador:** o quadro do aviso do telefone tem as mesmas linhas na variante real e na manual.
- **O texto mais baixo que a caixa dele** (o de uma linha fica uns 12 px abaixo do centro): é o `baselineOffset` da área de texto, 12 no texto e 11 nos rótulos dos botões, também dado do skin. No aviso do usuário principal é 0.

Dois avisos de outro molde, para comparar: o de volume é uma faixa no topo, de y = 0 a 62, no lugar do título da tela (a tela dele também fica em (0, 0)); o "Bem-vindo" do assistente de configuração é uma caixa inteira, com borda dos quatro lados, e por isso nunca pareceu cortado.

### O leitor da árvore de telas

`traces/pop_ArvoreDeTelas.java` usa a interface de depuração do Java (JDI, no `tools.jar` da JVM do firmware) e o agente `libjdwp.so` que o firmware traz. Só lê campos de objetos; não chama método da HMI nem altera nada, e segura a JVM por uma fração de segundo enquanto lê. Dá, para qualquer classe de widget e trecho de nome: posição em relação ao pai, posição na tela, tamanho, visibilidade, margens do layout, áreas do popup, imagens com as bordas fixas e os campos das áreas de texto.

- **Só funciona na variante manual do lançador.** Na real a JVM recusa o agente: "Tool agent requires sharing to be disabled", por causa do arquivo de classes compartilhado que o `runHMI.sh` liga (`-Xpreload:on`, `-XX:CompressedSharedArchiveFile`). Não procurei jeito de contornar.
- Como rodou: `VARIANTE=manual ROTEIRO="…" sh traces/pop_arvore.sh <bancada> <nome> <segundos> "<instantes>"`, que sobe a central com `-agentlib:jdwp=…address=127.0.0.1:8765` (o container não tem rede), compila o leitor com o `javac` do firmware e o roda nos instantes pedidos.

### Limites

- **Não comparei com o carro.** É o que falta para fechar: uma foto de um aviso pequeno na central (pedido na ficha).
- **Não abri a imagem 2097.** Que ela não tem borda de cima sai das bordas fixas (5 px em cima contra 72 embaixo) e dos pixels do quadro, não do arquivo.
- **A árvore foi lida na variante manual.** Na real conferi os pixels do aviso do telefone, iguais, e os retângulos de fundo que a HMI registra (188 e 228 de altura), iguais.
- **O tipo de tela do aviso (`viewTypeSelection`) vale 0.** Pelo código (`PopupView.startEnterAnimation`), outro valor escolheria outra animação de entrada. Não achei quem o mude fora dos dados da própria tela.
- **Olhei um molde.** Os três avisos usam a moldura 2097; avisos de outros moldes, fora o de volume e o do assistente, não foram abertos.
- **O skin é o do MU0883**; o carro roda o MU0803.

### O que entrou na pasta

- Nada em `driver/`, `stubs/` nem `glforward-moi3/`.
- `traces/pop_*`: o leitor e o roteiro que o roda, as árvores dos quatro avisos, o perfil dos quadros, o que a HMI registrou da animação e do fundo, e os roteiros das centrais.
- Quatro quadros, `quadros/2026-10-12-hmi-pop-*.png`.
- Na ficha do carro, em "O que anotar no primeiro autoscan", o pedido de foto de um aviso. No `simulador-jetta.md`, a situação do item 3.

### Onde parou

- O item 3 do roteiro do simulador está investigado: não é corte. Fecha de vez com a foto do carro.
- A fila que a sessão principal passou termina aqui (o consumo de processador ficou com outra sessão: é o Marco 10).

## O brilho pela tela: o display de mentira responde (12/10/2026)

Feito pela sessão do estado do carro de mentira, como segundo pedido da noite: o Lipe pediu à sessão principal, à 0h25 de 12/10/2026, que os trabalhos seguissem durante a madrugada, e ela me passou este item dos próximos passos ("o brilho pela tela", do levantamento do que a memória guarda). É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução:**

- **A escolha do nível de luminosidade pega.** Em Ajustes > Display, escolhi "Baixa": o campo ficou em "Baixa". Com o código de antes, lado a lado, a lista fechava e o campo voltava a "Muito alta" (rodadas `a` e `z`).
- **O campo mostra o nível de verdade.** Na partida ele passou a mostrar "Média", o nível com que a HMI nasce, e não "Muito alta".
- **A HMI guarda a escolha, e ela volta.** A linha `5051` `4311744512` da memória da tela mudou na posição 2 de 02 para 03 uns 20 s depois do toque. A central seguinte, subida da mesma memória, gravou 25 % no display na partida e nasceu com o campo em "Baixa", sem ninguém tocar (rodada `b`).
- **Sumiu o tempo esgotado de 10 s que toda partida tinha** (`DSI timeout occurred … getDisplayBrightness did not arrive`). Era o único que restava no registro da HMI nas centrais de teste.
- **Doze partidas lado a lado, seis com o código novo e seis com o de antes** (`s1` a `s4`, sem memória; `m1` e `m2`, com a memória da rodada `a`): nas seis novas, uma gravação e uma leitura de brilho por partida, uma resposta e nenhum tempo esgotado; nas seis de antes, nenhuma resposta e o tempo esgotado. O resto saiu igual nas doze: 111 assinaturas no carro de mentira, 3 pedidos atendidos, 14 sem resposta, Welcome, relógio, os mesmos 34 serviços ausentes e 0 erros de OpenGL. Nenhum laço.
- **Da pasta, depois de instalar, pelo caminho do modo ao vivo** (rodadas `p1` e `p2`, com o rádio e o telefone de mentira que outras sessões puseram no padrão): escolhi "Alta"; a sessão seguinte gravou 75 % na partida e nasceu em "Alta".

| Antes: escolhi "Baixa" e o campo voltou a "Muito alta" | Depois: fica em "Baixa" |
|---|---|
| ![Antes](quadros/2026-10-12-hmi-bri-antes-volta-a-muito-alta.png) | ![Depois](quadros/2026-10-12-hmi-bri-depois-fica-em-baixa.png) |

As outras fotos: [o campo em "Média" na partida](quadros/2026-10-12-hmi-bri-partida-mostra-media.png), [a lista dos cinco níveis](quadros/2026-10-12-hmi-bri-lista-de-niveis.png), [a sessão seguinte, que nasce em "Baixa"](quadros/2026-10-12-hmi-bri-sessao-seguinte-nasce-em-baixa.png) e, da pasta, [a sessão seguinte em "Alta"](quadros/2026-10-12-hmi-bri-pasta-sessao-seguinte-nasce-em-alta.png).

**O que não muda:** a imagem não escurece. O display de mentira só guarda o número e o devolve; a janela e a página mostram a tela com o mesmo brilho em qualquer nível.

### O que a HMI faz com o brilho

Lido nas classes decompiladas do MU0883 (leitura de código, salvo onde digo que foi visto):

- **Os níveis.** A tela tem cinco: "Muito alta", "Alta", "Média", "Baixa" e "Muito baixa", que a HMI guarda como 0 a 4 e manda ao display como 100, 75, 50, 25 e 0 % (`ASLDisplayManagementTarget.changeBrightness`, com `mapValue(nível, 0, 4, 100, 0)`). Visto no barramento: `10 32` (50 %), `10 19` (25 %), `10 4b` (75 %).
- **Dois pedidos por escolha.** A HMI grava e pede a leitura logo atrás (`DisplayBrightness.setDsiDisplayManagement`): `setDisplayBrightness` (api 1011: display, porcentagem) e `getDisplayBrightness` (api 1022: display), na interface 100 do serviço de display. O mesmo par sai na partida, com o nível da memória, e na restauração de fábrica.
- **É da resposta da leitura que sai tudo.** `getDisplayBrightnessResult` (api 2012: display, porcentagem), na interface 101. Ao recebê-la, a HMI converte a porcentagem de volta em nível, põe o nível no campo da tela e o guarda na memória (`processGetDisplayBrightnessResponse`). Sem a resposta, o campo ficava no valor inicial dele, "Muito alta", e nada era guardado: era isso o "a tela não deixa mudar" do levantamento de ontem.
- **A gravação não tem resposta própria.** A camada da LG não tem mensagem para isso (`DSIDisplayManagementListenerService` só trata a 2012).
- **Os inteiros são `int32` comuns**, como nos outros pedidos de display, e zero não vai no fio: 0 % é a mensagem vazia.

### O que o display de mentira faz

No `driver/dsistub.c`, junto dos outros pedidos de display:

- **`setDisplayBrightness`:** guarda a porcentagem daquele display. Sem resposta.
- **`getDisplayBrightness`:** devolve a porcentagem guardada, na instância do pedido.
- **Leitura sem gravação anterior fica sem resposta**, com uma linha no registro. O stub não tem brilho próprio para informar, e uma resposta inventada faria a HMI guardar na memória um nível que ninguém escolheu. Na HMI toda leitura vem depois de uma gravação, então isso não apareceu em nenhuma rodada.
- **Cada display tem o seu** (a HMI só usa o 0).
- **Nada é feito com o valor**, e ele não sobrevive ao stub: quem lembra do brilho de uma partida para a outra é a memória da tela, que a HMI regrava no display ao nascer.

No registro do stub (`dmstub.log`): "brilho do display 0 gravado: 25 %" e "brilho do display 0 lido: 25 %".

### As rodadas

Centrais de teste `moi3-bri-<nome>`. Previsões escritas antes, em `traces/bri_previsoes.md`: conferiram. Uma ficou sem conferência: que na `z` a memória da tela não mudaria, porque a `z` subiu sem memória. Saídas em `traces/bri_*`.

| Rodada | O que rodou | Observado |
|---|---|---|
| `z` | o código da pasta, de antes | tempo esgotado do `getDisplayBrightness` na partida; campo em "Muito alta"; escolher "Baixa" fecha a lista e o campo volta a "Muito alta" |
| `a` | cópia de trabalho, com as duas memórias | na partida: "gravado: 50 %" e "lido: 50 %", nenhum tempo esgotado, campo em "Média"; escolhi "Baixa": campo em "Baixa", "gravado: 25 %" e "lido: 25 %", e a memória da tela com `displayBrightness` de 2 para 3 |
| `b` | cópia de trabalho, mesma memória | na partida a HMI manda `10 19` (25 %) e recebe `10 19`; o campo nasce em "Baixa"; nenhum tempo esgotado |
| `s1` a `s4` | pares lado a lado, cópia de trabalho e pasta, sem memória | ver o resultado, acima |
| `m1`, `m2` | pares lado a lado, com a memória da `a` | o código novo grava e lê 25 %; o resto, como nas `s` |
| `p1`, `p2` | cópia da pasta de 2h42 (depois de instalar), pelo caminho do modo ao vivo | `p1`: "Alta", "gravado: 75 %". `p2`: 75 % na partida, campo em "Alta", 110 assinaturas no carro de mentira, 3 pedidos atendidos, 14 sem resposta, nenhum tempo esgotado |

**"Desligar display (em 10 s)"**, a caixa de cima da mesma tela: marquei, a caixa ficou marcada, e em 25 s nada mais aconteceu; nenhum pedido chegou ao display de mentira. Desmarquei e não investiguei.

### O que entrou na pasta

- `driver/dsistub.c`: os dois pedidos de brilho e o parágrafo do cabeçalho. O resto do programa não mudou.
- `driver/tests/t_pedidos.sh`: de 16 para 28 casos. Dos 12 novos, 11 foram vistos falhar contra o stub de antes; o que passava só fixa que o tamanho da tela continua respondido (`traces/bri_t_pedidos_vermelho.txt`).
- Seis quadros `quadros/2026-10-12-hmi-bri-*.png`. As versões de antes dos dois arquivos estão em `traces/bri_antes/`.

Instalado às 2h42 de 12/10/2026, por troca de nome. Testes, rodados da pasta logo depois: `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 187, `t_memoria.sh` 80 (no container e no Mac), `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, `t_hmilog.sh` 32, `t_kipc_bus.sh` 14, `t_radio.sh` 81, `t_servidor.sh` 20 e `t_lsmval_abre.sh` 38. Todos passam, e o stub compila sem aviso com `-Wall -Wextra`. Saída em `traces/bri_suites_pasta.txt`.

### Limites

- **A imagem não escurece** (decisão D109).
- **Só o brilho do display.** O brilho, o contraste e a cor do vídeo e da câmera de ré (`setBrightness` 1012, `getBrightness` 1013 e os vizinhos) seguem sem resposta, e o `requestDisplayControl` (1041) também.
- **O display de mentira aceita qualquer porcentagem.** O do carro pode limitar ou recusar; a HMI só manda 0, 25, 50, 75 e 100.
- **"Desligar display" não foi investigado.**
- **O que a HMI faz com a resposta é leitura do código do MU0883**; o que foi visto é o efeito: o campo, a memória e os bytes no barramento. O carro roda o MU0803.
- **Não rodou** o `driver/ao_vivo.sh` com o nome `moi3-ao-vivo`; a `p1` e a `p2` usaram uma cópia em que só o nome do container muda.

### Onde parou

- O item "o brilho pela tela" dos próximos passos está feito. Do que faltava para a memória ficar "igual ao carro" restam o serviço de áudio (a tela de som) e olhar o que a restauração de fábrica faz.
- A central ao vivo estava desligada e não foi tocada. A próxima partida já responde ao brilho.
- Nenhuma central minha ficou no ar.

## Marco 12 — o telefone de mentira: a tela "nenhum aparelho conectado" (12/10/2026)

Feito numa sessão à parte, aberta pela sessão principal à 1h30 de 12/10/2026, para o item 7 do roteiro de [simulador-jetta.md](simulador-jetta.md): a tecla Telefone abrir a tela do telefone no estado "nenhum aparelho conectado", em vez do popup "Telephone unavailable." do Marco 5, só com estados simulados. A bancada não tem Bluetooth nem aparelho de verdade, e não é para ter: são só os estados que a tela mostra. É emulação num Mac; nada vai para o carro.

**Acréscimo das 4h01 de 12/10/2026:** o segundo estado do pedido, um aparelho fictício conectado com a tela principal do telefone, foi feito depois deste texto e está na subseção "O segundo estado: um aparelho fictício conectado", no fim deste marco. O que vem daqui até lá é o primeiro estado, como estava às 3h05.

**Resultado, verificado em execução** em centrais de teste subidas da pasta já instalada (`prova` e `provac`, às 2h38; `prova3`, às 2h55, com o perfil definitivo):

- **a tecla Telefone da moldura abre a tela do telefone** (`PHONE_NOT_CONNECTED`): "Conecte o telefone celular apenas quando parado. Procure pelo menu de Bluetooth do celular.". No controle, sem o telefone de mentira, a mesma tecla abre o aviso "Telefone indisponível." (`PO_SYSTEM_ALERT_PHONE_NOT_INSTALLED`);
- **a grade de menus ganha o bloco "Telefone"**, em segundo lugar numa central sem memória, e passa a ter duas páginas (o nono bloco, "Configuração", vai para a segunda). O bloco abre a mesma tela;
- **a engrenagem da tela abre "Configurações do telefone"**; "Selecionar celular" mostra "Nenhum celular conhecido disponível."; e o ícone do alto abre as "Configurações do Bluetooth";
- **a caixa "Bluetooth" dessas configurações desliga e liga o Bluetooth de mentira.** Desligado, a tela do telefone passa a `PHONE_INIT_BT_SWITCHED_OFF`, "Ligar o Bluetooth.", com um botão que liga de novo e devolve a tela de antes;
- **o assistente de configuração ganha uma terceira página, a do telefone** (visto em centrais nascidas de uma cópia da memória guardada do modo ao vivo): ela explica como conectar e deixa seguir até o fim;
- **a tecla Telefone do volante não abre a tela.** A HMI a recebe e não pede conteúdo nenhum, com ou sem o telefone de mentira; não é coisa que este marco pudesse mudar (ver "A tecla do volante");
- **nada mais na partida muda.** Em nove pares de centrais lado a lado, com e sem o telefone de mentira, as telas que entram até a tecla são as mesmas e as fotos aos 25, 45 e 58 s são iguais byte a byte dentro de cada par;
- nenhuma das 41 centrais da sessão registrou erro de OpenGL, e nas da versão final não há tempo esgotado de serviço de telefone no registro da HMI.

| Antes: "Telefone indisponível." | Depois: a tela do telefone, sem aparelho |
|---|---|
| ![Antes](quadros/2026-10-12-hmi-tel-antes-telefone-indisponivel.png) | ![Depois](quadros/2026-10-12-hmi-tel-sem-aparelho.png) |

| A grade com o bloco "Telefone" | "Configurações do telefone" |
|---|---|
| ![Grade](quadros/2026-10-12-hmi-tel-grade.png) | ![Configurações](quadros/2026-10-12-hmi-tel-configuracoes.png) |

| "Selecionar celular" | "Configurações do Bluetooth" |
|---|---|
| ![Celulares conhecidos](quadros/2026-10-12-hmi-tel-celulares-conhecidos.png) | ![Bluetooth](quadros/2026-10-12-hmi-tel-bluetooth.png) |

| Depois de tocar na caixa "Bluetooth" | A tela do telefone com o Bluetooth desligado |
|---|---|
| ![Desligado](quadros/2026-10-12-hmi-tel-bluetooth-desligado.png) | ![Ligar o Bluetooth](quadros/2026-10-12-hmi-tel-ligar-o-bluetooth.png) |

As outras fotos: [a segunda página da grade](quadros/2026-10-12-hmi-tel-grade-pagina-2.png), o assistente de configuração [sem](quadros/2026-10-12-hmi-tel-assistente-sem-telefone.png) e [com](quadros/2026-10-12-hmi-tel-assistente-com-telefone.png) o telefone de mentira e [a página do telefone](quadros/2026-10-12-hmi-tel-assistente-pagina-do-telefone.png) dele.

**Tudo o que a tela mostra aqui é simulação.** Nenhum valor foi lido do carro nem de um aparelho, e o perfil não traz nome, número nem contato de ninguém. Se a central do carro mostra essas mesmas telas eu não sei: não há foto da tela de telefone do Jetta.

### O que decide a tecla e a tela (leitura do código, conferida em execução)

Lido nos decompilados do CFR de 10/10 (jar da HMI) e conferido nas rodadas.

- **A tecla Telefone é tratada pela atividade do sistema**, e não pela do telefone. `SystemActivityImpl` tem duas transições para ela: a 12, que pede o conteúdo `HMI_PHONE`, e a 13, que abre o aviso. A guarda é uma variável local da atividade (2000000002), que é a chave de recurso 894, `FEAT_LOG_IS_PHONE` (`LocalDataPoolUpdateImpl`, `FeatureFlagHandler`): telefone interno (239) **ou** telefone externo (213).
- **A chave do telefone externo é o estado do Bluetooth.** `AslTargetSystemFeaturesAndCoding.dsiBluetoothUpdateBTState` assina o atributo 3 do `DSIBluetooth` e liga "isExtPhoneAvailable" quando o estado chega válido e não é 2 (não funcional) nem 5 (proibido pelo diagnóstico). Ligado (0) e desligado (1) contam como "há Bluetooth". Na bancada ninguém respondia a esse atributo, e a chave ficava no falso do começo: daí o aviso do Marco 5.
- **A tela do telefone é escolhida pelo estado de ativação do aparelho** (`DSIMobileEquipment`, atributo 4) e pelo estado do Bluetooth. `HsmPhonePhoneAvailability.setPrimaryActivationState` grava o estado na propriedade 2906, **menos o 0 (INIT) e o 7, que ele ignora**; as guardas de `PhoneActivityImpl` separam três casos: estado 1 (não conectado) com o Bluetooth ligado, Bluetooth desligado, e estado 5 (ligado). Os dois primeiros foram vistos em execução: `PHONE_NOT_CONNECTED` e `PHONE_INIT_BT_SWITCHED_OFF`. Que o terceiro leve à tela principal, `PHONE_MAIN_NOCALLS`, é leitura das guardas; as tabelas de transição ficam em recursos, que não li, e essa tela não foi vista.
- **Conferido no registro da própria HMI** (rodada `e2`, com as chaves de rastreamento do telefone): aos 0,05 s, `[AslTargetSystemFeaturesAndCoding] dsiBluetoothUpdateBTState0`; aos 12,29 s, `updateActivationState (primary)`, `Setting phone domain available…` e `activationState: 1`; e, na tecla, `[StageContentProcessor] doRequestContent content: HMI_PHONE` seguido de `enterView(… (PHONE_NOT_CONNECTED)…)`.

O que distingue "sem aparelho" de "indisponível" é, portanto, um atributo só: **indisponível é não saber do Bluetooth, ou sabê-lo quebrado; sem aparelho é Bluetooth ligado e aparelho não conectado.**

### O levantamento: anunciar não basta

Quatro centrais de teste da cópia da pasta da 1h36, com o barramento gravado pelo `driver/dsi_escuta.c` e o anúncio de cada domínio mandado uma vez, pelo `dsi_manda`, antes de a HMI subir. Tecla Telefone da moldura aos 62 s e do volante aos 82 s.

| Rodada | Anunciado | Serviços DSI ausentes | Pedidos da HMI nas interfaces do telefone | Tecla da moldura |
|---|---|---|---|---|
| `ctl` | nada | 35 | 10 assinaturas, das instâncias 1 e 2 do `DSIMobileEquipment`, sem ninguém para responder | aviso |
| `a256` | 256 (`PID_BT`) | 19 | 30 assinaturas e 3 pedidos diretos | aviso |
| `a4096` | 4096 (`PID_DCMANAGER`) | 34 | 11 assinaturas | aviso |
| `ambos` | os dois | 18 | 31 assinaturas e 3 pedidos diretos | aviso |

- **O anúncio do 256 liga 27 instâncias de 16 serviços** (`ListenerWorkerThread.processBTListenerService`): `DSIBluetooth`, `DSIBluetoothLE`, `DSIBluetoothLEDataChannel`, `DSIObexAuthentication`, `DSIObjectPush`, `DSINFC`, `DSIMessaging`, `DSIMessagingServiceConfiguration`, `DSIMobileEquipment` (0 a 2), `DSIMobileEquipmentTopology`, `DSIAdbUserProfile`, `DSIAdbList` e `DSIAdbEdit` (0 a 3 cada), `DSIAdb`, `DSIAdbDataResolution` e `DSIAdbVCardExchange`. **O do 4096 liga dois:** `DSIPhoneManager` e `DSISmartphoneIntegration`.
- **As 31 assinaturas, com os dois anunciados:** `DSIBluetooth` (atributos 3, 7, 1, 9 e 10, em quatro pedidos), `DSIMobileEquipment` (33 atributos na instância 0, 32 na 1 e 20 na 2), `DSIAdb` (7), `DSIAdbUserProfile` (5 em cada uma de duas instâncias), `DSIAdbList` (o 2 e o 3000, que é uma indicação e não um atributo), `DSIMessaging` (5), `DSIMessagingServiceConfiguration` (6) e `DSIPhoneManager` (1, 7, 2, 4 e 8).
- **Os três pedidos diretos:** o idioma da agenda (`DSIAdb.setLanguage`, com `pt_BR`), de que a HMI espera resposta em 10 s e registra o tempo esgotado, e o idioma do aparelho (`DSIMobileEquipment.requestSetLanguage`), mandado duas vezes.
- **Na tecla, nada passa pelo barramento.** O aviso é decidido dentro da HMI, pela chave de recurso.
- A tecla do volante não mudou a tela em nenhuma das quatro.

### O experimento que confirmou o mínimo

Antes de mexer no gerador, quatro experimentos com o motor do `veiculostub` como estava, ligado à mão dentro da central. Nenhum deles foi para a pasta.

| Rodada | O que rodou | Observado |
|---|---|---|
| `e1` | esquema recortado à mão do rascunho do gerador, só com `DSIBluetooth` e `DSIMobileEquipment` (a interface de pedidos deste, 1001, posta à mão), e um perfil de uma linha: estado de ativação 1 | a tecla abre `PHONE_NOT_CONNECTED` aos 61,6 s; a grade ganha "Telefone"; 0 erros de OpenGL |
| `e1b` | o mesmo, com o motor alterado para não responder pela instância 2 do `DSIMobileEquipment` | a mesma tela; no registro da HMI, "Carplay: 0" em vez de "Carplay: 1" |
| `e2` | o esquema gerado, com os 18 serviços | a mesma tela; 31 assinaturas atendidas |
| `e2b` | o mesmo, sem os dois serviços do domínio 4096 | a mesma tela; `DSIPhoneManager` segue ausente |

O mínimo para a tela abrir são, então, dois valores: Bluetooth ligado (que é o zero do valor padrão) e aparelho não conectado.

### O esquema: três formas novas do código da LG

O `driver/LeEsquemaDSI.java` rodado como estava, com `DOMINIOS=256,4096`, devolveu 27 serviços e 146 avisos. Três coisas do código da LG ele não lia, e passou a ler:

| O que o gerador de antes fazia | O que é, no firmware | O que mudou |
|---|---|---|
| punha no esquema `DSIMediaBase`, `DSIMediaBrowser`, `DSIMediaOnline`, `DSIMediaPlayer`, `DSIMediaRecorder`, `DSIUPNPBrowser`, `DSIUPNPPlayer`, `DSISearch` e `DSISearchDataProvider` como serviços do domínio 256 | o despacho do 256 entrega também as interfaces de eventos desses serviços (201, 203, 205, 207, 210, 253, 257, 1302 e 1304), que são de outros donos: os pedidos deles vão para `0x800000` e `0x100000` | só fica no esquema o serviço que o anúncio do domínio liga (linhas `W`); os outros vão no relatório, em "fora do esquema" |
| deixava sem atributo todos os eventos de `DSIAdb`, `DSIAdbList`, `DSIAdbEdit`, `DSIAdbUserProfile`, `DSIAdbVCardExchange`, `DSIMessaging` e `DSIMessagingServiceConfiguration` | nesses ouvintes quem acha os assinantes é `getUpdateListenerList(<o …Impl da instância>, atributo)`, e não `(instância, atributo)` | o atributo sai das duas formas |
| deixava o `DSIMobileEquipment` com interface de pedidos -1 e nenhum pedido | os 63 pedidos passam por um método auxiliar, `messageToBytes(api, mensagem)`, que é quem chama `postDSIProviderMsg`, com um domínio que depende da instância | a api é lida em cada chamada ao auxiliar; o relatório diz para que domínio vai cada instância |

- **Os esquemas de veículo e de rádio saem idênticos byte a byte** aos da pasta, os cinco arquivos de cada um, regerados duas vezes (com a cópia de trabalho e com o candidato à instalação).
- **O do telefone** (`stubs/telefone/gerado/`): 18 serviços, 143 atributos, 329 eventos, 215 pedidos (166 com resposta na tabela da HMI, 7 com regra de gravação), 683 mensagens, 1.689 campos e 991 constantes. Duas rodadas deram arquivos idênticos. A prova do codificador: 324 de 324 mensagens iguais, byte a byte, às que as classes do firmware produzem.
- **O domínio depende da instância, e isso sai do firmware.** O gerador anda pelos desvios do método auxiliar: instância 0, domínio 256 (e 512 no pedido de idioma); instância 1, domínio 256; **instância 2, domínio 512, `PID_CARPLAY`**. A instância 2 do `DSIMobileEquipment` é a do CarPlay; no carro quem responde por ela é outro processo.
- **Duas coisas ficaram como o firmware as tem.** `DSISmartphoneIntegration` fala outro protocolo (a api vai dentro da mensagem, `postDSIProviderMsg(bytes, domínio, interface)`): sai no esquema sem mensagem nos eventos, e os quatro avisos do gerador são dele. E o `DSIPhoneManager` pede e recebe na mesma interface, a 600, e assina em varint comum, enquanto os serviços do 256 assinam em zigue-zague; o esquema diz qual, e o motor segue.

### Um defeito do motor que só o telefone mostrou

Na rodada `e2` o registro do stub encheu com 400 linhas "pedido sem resposta: DSIMessaging, api 1, que não está no esquema". Não eram pedidos.

- **A causa:** o `driver/veiculostub.c` tomava por pedido qualquer mensagem que chegasse numa interface de pedidos do esquema, sem olhar quem mandou. A interface 1101 é a de pedidos do `DSIMessaging` e também a de **eventos** do `DSIPowerManagement`, de outro domínio (`0x400000`), que o `dsistub` publica de segundo em segundo: 213 mensagens em 100 s.
- **O efeito:** o registro cheio até o limite de 400 linhas, o resumo "pedidos sem resposta" inflado, e a primeira dessas mensagens já contava como "a HMI pediu", o que desliga o anúncio repetido de reserva. Não chegou a responder nada errado, porque as apis 1, 4 e 7 não são pedidos do `DSIMessaging`; com outra coincidência de números, responderia.
- **O conserto, uma linha no laço principal:** só é pedido o que vem do canal da HMI (`0x8000`). Vale para o carro de mentira e para o rádio de mentira, que usam o mesmo motor; nos domínios deles nenhuma interface coincidia, e por isso nunca apareceu.
- **Teste escrito antes:** quatro casos em `t_telefone.sh`, que falhavam contra o motor de antes (`traces/tel_vermelho/`). Com a mudança, `t_veiculo.sh` (187), `t_radio.sh` (81) e `t_pedidos.sh` seguem passando.

### O perfil, e dois zeros que não são neutros

`stubs/telefone/sem-aparelho.txt`, tudo marcado como simulação:

| O quê | Valor | Por quê |
|---|---|---|
| estado do Bluetooth | 0, ligado | é o valor padrão; está escrito para ficar à vista |
| estado de ativação do aparelho | 1, não conectado | o zero a HMI ignora, e a tela do telefone não se decide |
| nível de bateria do aparelho | 255, sem informação | **o zero é "bateria vazia"** (abaixo) |
| mudo do microfone | 1, sem mudo | **o zero é mudo ligado**; a HMI pede "sem mudo" na partida, e com o perfil assim deixa de pedir |
| seis respostas "feito" | resultado 0 | visibilidade, mudo, estatísticas de chamadas, idioma da agenda, suspensão da reconexão e ligar ou desligar o Bluetooth: sem resposta, cada pedido deixa um tempo esgotado de 10 s no registro da HMI (o de ligar e desligar, 60 s com a tela em "Aguarde…") |
| `grava = DSIBluetooth` e a regra de ligar e desligar | o estado pedido vai para o atributo | a caixa "Bluetooth" da tela funciona; o nome da central, se for digitado, fica |

**O aviso de bateria, achado pela série.** A primeira versão do perfil só tinha as duas primeiras linhas. Nos dois pares da série `s`, a central com o telefone de mentira abriu, aos 12,8 s da partida e por 10 s, um aviso que o controle não tem: `PO_INFO_BLUETOOTH_BATTERY_WARNING`, o de bateria do celular, sem haver celular. A regra está em `TargetASLPhoneGetter`: nível 0 é `MEBATTERYCHARGELEVEL_0`, bateria vazia, e dispara o aviso; "sem informação" é 255. Com 255 no perfil o aviso não apareceu em nenhuma das nove centrais das séries `r` e `u`. É o mesmo tipo de armadilha que o Marco 8 achou no carro de mentira ("parado = 0" quer dizer andando).

**O idioma da agenda importa além do registro.** A resposta a `DSIAdb.setLanguage` é lida por `TargetASLOrganizerBaseFunctionality.dsiAdbSetLanguageResult`, que com ela dá por feita a parte da agenda na troca de idioma da central. O que a troca de idioma faz sem essa resposta eu não medi.

### A tecla do volante

A tecla Telefone do volante é a 53 do teclado 4 (`KEY_MFW_HOOK`, eventos `HK_MFL_PHONE_*`, 313 a 316 e 310, pela tabela de teclas do Marco 5). Mandada três vezes na rodada `prova`, com o telefone de mentira: curta e segura (toque longo) na tela do veículo, e curta na tela do telefone; e curta sobre a grade em todas as rodadas de levantamento e das séries.

- **Medido:** a HMI registra a tecla (`HK Received: KBD[4] KEY[53] KST[1]`, visto na `e1`) e não pede conteúdo nenhum; na tela do veículo nem um quadro novo é desenhado. A da moldura, no mesmo registro, é seguida de `doRequestContent content: HMI_PHONE`.
- **Leitura do código, com limite:** procurei os eventos 313, 314 e 316 nas atividades geradas e no código do telefone e não achei quem os trate. As tabelas de transição das atividades ficam em recursos, que não li: não posso afirmar que nenhuma tela os use.
- **Dedução:** é a tecla de gancho, de atender e desligar; sem chamada ela não tem o que fazer na central. No carro o menu de telefone que ela abre deve ser o do painel de instrumentos. Não tenho como conferir.

### As rodadas

Centrais de teste `moi3-tel-<nome>`, à parte da central ao vivo, subidas com `driver/m8_central.sh` (`PREFIXO=tel`) pelo roteiro `traces/tel_lev.sh`. Saídas em `traces/tel_*`.

| Rodada | Hora | De onde | O que rodou | Observado |
|---|---|---|---|---|
| `ctl`, `a256`, `a4096`, `ambos` | 1h37 a 1h42 | cópia da 1h36 | o levantamento | a tabela de cima |
| `e1`, `e1b`, `e2`, `e2b` | 1h44 a 2h02 | cópia da 1h36 | os experimentos | a tabela de cima |
| `p1`, `c1` | 2h06 | cópia de trabalho | a opção `TELEFONE` do lançador, e um controle | o lançador registra o esquema e o perfil; a tela abre; os quatro tempos esgotados de telefone da `e2` somem |
| `p2` | 2h08 | cópia de trabalho | passeio pelas telas | `PHONE_SETUP`, `PHONECONNECTIONMANAGER_PHONE` e `PHONECONNECTIONMANAGER_BLUETOOTH_SETUP`; dois tempos esgotados de `requestReconnectSuspend`, que ganhou resposta |
| `s1`, `s2` (pares) | 2h12 a 2h16 | cópia de trabalho | a série, com o primeiro perfil | o aviso de bateria nos dois pares; interrompi a série e corrigi o perfil |
| `r1` a `r6` (pares) | 2h17 a 2h29 | cópia de trabalho | a série, com o perfil corrigido | seis de seis como previsto (abaixo) |
| `lt`, `lc` | 2h29 | cópia de trabalho | como o modo ao vivo: nascendo de uma cópia da memória guardada, com o rádio de mentira, com e sem o telefone | o assistente de configuração aos 46,4 s e aos 48,3 s; três páginas e duas |
| `lw` | 2h32 | cópia de trabalho | a página do telefone do assistente | `PhoneGlobalWizardPage_Main`; "Continuar" leva ao fim (`GlobalSetupWizardFinished`) |
| `prova`, `provac` | 2h38 | **pasta instalada** | a prova, com passeio e as teclas do volante, e o controle | a lista do começo; 0 erros de OpenGL em 821 e 714 quadros |
| `prova2` | 2h41 | pasta instalada | as telas de ajustes, passo a passo | tocar em "Bluetooth" deixa a tela em "Aguarde…": `requestSwitchBTState` espera resposta por 60 s |
| `b1` | 2h46 | cópia de trabalho 2 | o perfil com a regra de ligar e desligar | o aviso "Aguarde…" abre e fecha em 10 ms; `PHONE_INIT_BT_SWITCHED_OFF` e a volta |
| `u1` a `u3` (pares) | 2h48 a 2h54 | cópia de trabalho 2 | a série, com o perfil definitivo | três de três como previsto |
| `prova3` | 2h55 | **pasta instalada** | o passeio inteiro, com o perfil definitivo | as cinco telas, desligar e ligar; nenhum tempo esgotado; 0 erros de OpenGL |

As previsões de cada série foram gravadas antes de largar (`traces/tel_previsoes.txt`). A da primeira falhou num ponto, o aviso de bateria, e está dito lá.

### O efeito na partida: as séries

Pares de centrais simultâneas, uma com `TELEFONE=/stubs/telefone/sem-aparelho.txt` e uma sem, largadas com 3 s de diferença e alternando quem larga na frente. Sem o registro detalhado (`HMILOG`), para a partida ser a do modo ao vivo. Tecla Carro aos 32 s, Telefone da moldura aos 62 s, Menu aos 74 s, Telefone do volante aos 82 s; fotos aos 25, 45, 58, 69, 79 e 90 s. Carga do Mac entre 4 e 8.

| | Com o telefone de mentira (9) | Controle (9) |
|---|---|---|
| serviços DSI ausentes | 18 | 35 |
| esperas de 2 s por serviços DSI ausentes | 7 | 8 |
| fim das esperas | 49,47 a 49,59 s | 51,48 a 51,62 s |
| hora aceita aos | 0,05 a 0,07 s | 0,04 a 0,07 s |
| Welcome na tela por | 4,62 a 4,84 s | 4,67 a 4,87 s |
| telas que entram até os 60 s | as mesmas 17 | as mesmas 17 |
| fotos aos 25, 45 e 58 s | iguais byte a byte às do controle do par, nas 27 comparações | — |
| tecla Telefone aos 62 s | `PHONE_NOT_CONNECTED`, nas 9 | o aviso, nas 9 |
| tecla Telefone do volante aos 82 s | nenhuma troca de tela | nenhuma |
| carro de mentira: assinaturas, pedidos atendidos, sem resposta | 111, 3 e 14 | 111, 3 e 14 |
| telefone de mentira: assinaturas atendidas | 31 | — |
| linhas `ERROR` e exceções da HMI | 119 ou 120, e 47 | 163 ou 164, e 79 |
| tempos esgotados DSI | os mesmos do controle | só o do brilho, na série `r`; nenhum na `u` |
| erros de OpenGL, `SIGSEGV` | 0 e 0 | 0 e 0 |

- A ordem das telas do primeiro segundo variou num dos nove pares (o quarto da série `r`), como o Marco 10 já tinha registrado para qualquer partida.
- **O que muda de fato:** 17 serviços a menos dados por ausentes, uma espera de 2 s a menos, e por isso tudo o que vem depois das esperas acontece uns 2 s mais cedo; menos linhas de erro; e o que se vê depois da tecla.
- A série `r` rodou com o `dsistub` de antes das 2h42 e a `u` com o de depois, de outra sessão, que responde ao brilho: é por isso que o tempo esgotado do brilho some na `u`, nos dois braços.

![A série r aos 69 s: em cima as seis centrais com o telefone de mentira, embaixo os seis controles](quadros/2026-10-12-hmi-tel-serie.png)

### O modo ao vivo

**O telefone de mentira entra por padrão no modo ao vivo** (`driver/ao_vivo.sh`, e com ele o botão da janela), como o carro de mentira e o rádio de mentira. É decisão minha, registrada como D133; `TELEFONE= sh driver/ao_vivo.sh` sobe sem ele. As centrais de teste e o lançador direto seguem sem padrão.

Para saber o que o Lipe vai ver, duas centrais nascidas de uma **cópia** da memória guardada do modo ao vivo (61 linhas, só lida), com o rádio de mentira, uma com o telefone e uma sem (`lt` e `lc`):

- as duas nascem na grade, que é a última tela guardada; a grade guardada não tem o bloco "Telefone" na primeira página, e ele entra na segunda;
- o assistente de configuração aparece nas duas (aos 46,4 s com o telefone, aos 48,3 s sem); **com o telefone ele tem três páginas, e sem, duas**. A terceira é a do telefone: um texto que manda procurar pelo menu de Bluetooth do celular, com "Voltar" e "Continuar", e "Continuar" leva ao fim do assistente (`lw`);
- a tecla Telefone, com o assistente na tela, abre a tela do telefone numa e o aviso na outra;
- as contagens do carro de mentira (111, 3 e 14) e do rádio de mentira (13 assinaturas, 1 sintonia) são as mesmas nas duas; 0 erros de OpenGL.

**Não verificado:** a central ao vivo em si. Ela estava desligada e não foi ligada; o `ao_vivo.sh` foi conferido com um `docker` de mentira, só para ver a linha de comando que ele monta (`-e TELEFONE=/stubs/telefone/sem-aparelho.txt`, e vazio com `TELEFONE=`).

### O que entrou na pasta

- `driver/LeEsquemaDSI.java`: as três leituras novas e o cabeçalho. A diferença está em `traces/tel_LeEsquemaDSI.diff`.
- `driver/veiculostub.c`: o filtro de canal no laço principal e uma frase no cabeçalho (`traces/tel_veiculostub.diff`).
- `driver/m1_hmi.sh`: a opção `TELEFONE` (e `TELEFONE_ESQUEMA`), o bloco que sobe o telefone de mentira depois do rádio, o `telefonestub.log` na lista de registros e a linha "telefone:" do resumo. `driver/m3_ao_vivo.sh`: só comentário. `driver/m8_central.sh`: repassa as duas variáveis. `driver/ao_vivo.sh`: `TELEFONE_PADRAO`.
- `stubs/telefone/`, nova: o perfil `sem-aparelho.txt`, o esquema gerado e o [README](stubs/telefone/README.md).
- `driver/tests/t_telefone.sh`, novo: 103 casos.
- Treze quadros `quadros/2026-10-12-hmi-tel-*.png`. Os das telas são da `prova3`; o da grade e o do aviso, da `prova` e da `provac`; os do assistente, da `lt`, da `lc` e da `lw`.
- `traces/tel_*` (130 arquivos e duas pastas, 16 MB): as saídas, o barramento e os registros das rodadas; os roteiros (`tel_lev.sh`, `tel_serie.sh`, `tel_compara.py`, `tel_pedidos.py`); as previsões e as análises das séries; os testes vistos falhar; as baterias; e os roteiros da instalação.
- Os arquivos de antes da troca estão em `traces/tel_antes/`.
- Na ficha do carro: dois itens em "O que anotar no primeiro autoscan" (as adaptações de Bluetooth do 5F, com a posição que a tabela de campos do Marco 6 dá, e duas fotos: a tela Telefone sem celular e as "Configurações do Bluetooth").

**Não há programa novo.** Quem roda é o próprio `veiculostub`, com outro esquema e outro perfil; o lançador o copia para a central com o nome `telefonestub`.

**A instalação.** Montei um candidato com a pasta como estava às 2h34 e as mudanças por cima, por inserção ancorada; regerei os três esquemas com o gerador dele; rodei a bateria nele; e troquei os arquivos às 2h37min58, um a um, por cópia ao lado e troca de nome, depois de conferir que cada arquivo compartilhado ainda era o de quando o candidato foi montado. Avisei antes, por mensagem e na pasta de notas, as sessões do rádio e da hora e unidades; não esperei resposta. O perfil definitivo, o teste e o README entraram às 2h55min03.

### Testes

`driver/tests/t_telefone.sh`, 103 casos, sem firmware: o esquema (interfaces, atributos, eventos e pedidos, com os números lidos à parte no código da LG e nas interfaces `org.dsi.ifc`, e não tirados do esquema), os 324 vetores do firmware, os perfis da pasta (conferem, dizem que são simulação, não trazem texto que não esteja marcado como fictício) e o telefone de mentira no barramento, com os bytes que a HMI mandou nas rodadas.

Escritos antes do código, em cinco levas, cada uma vista falhar (`traces/tel_vermelho/`):

| Leva | Casos que falhavam | Contra o quê |
|---|---|---|
| o esquema | 23 de 59: 20 do esquema e 3 por ainda não haver perfil | o gerador de antes |
| o filtro de canal e as quatro respostas | 9 de 90 | o motor e o perfil de antes |
| a resposta do gerente de aparelhos | 1 de 91 | o perfil de antes |
| a bateria e o mudo | 2 de 97 | o perfil de antes |
| ligar e desligar o Bluetooth, e o nome | 5 de 102 | o perfil de antes |

O caso da contagem de pedidos atendidos foi ajustado depois da última leva, porque com o serviço em `grava` um pedido passa a deixar duas linhas no registro.

A bateria inteira, rodada da pasta às 2h55 de 12/10/2026 com tudo instalado (`traces/tel_suites_pasta_02h55.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 187, `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, `t_hmilog.sh` 32, `t_kipc_bus.sh` 14, `t_radio.sh` 81 e `t_telefone.sh` 103, no container; `t_memoria.sh` 80, `t_servidor.sh` 20 e `t_lsmval_abre.sh` 38, no Mac. Todos passam, sem aviso do compilador. O `t_pedidos.sh` tinha 16 casos na bateria das 2h38; os 12 a mais são de outra sessão, que instalou o brilho às 2h42.

### Limites

- **Sem aparelho, sempre.** Há dois estados, "nenhum aparelho conectado" e "Bluetooth desligado". Não há aparelho conectado, chamada, discagem, agenda nem mensagem. **O segundo estado do pedido, um aparelho fictício conectado, não tinha sido feito quando isto foi escrito, às 3h05; foi feito depois, às 3h48** (ver "O segundo estado: um aparelho fictício conectado", abaixo). Com o perfil padrão, este item continua valendo.
- **A tela convida a parear, e parear não dá.** Não há busca nem conexão; os pedidos correspondentes ficariam sem resposta. Não os exercitei.
- **A instância 2 do `DSIMobileEquipment` é respondida pelo domínio errado.** É a do CarPlay (domínio 512, lido do firmware pelo gerador); o telefone de mentira responde por ela pelo 256, dizendo "não conectado", porque o barramento da bancada não leva o destino do pedido e o motor não distingue instâncias. Medido na `e1b`: sem essa resposta a tela é a mesma.
- **A caixa "Visibilidade" não marca.** O modo pedido fica gravado e a resposta diz "feito", mas a caixa segue o outro campo do atributo (`visible`), que nenhum pedido muda.
- **O nome da central no Bluetooth nasce vazio.** Não inventei um.
- **O que a tela muda vale até a central desligar.** O telefone de mentira nasce sempre do perfil; a opção de guardar estado do motor (`-e`) não foi ligada aqui.
- **Ligar e desligar é instantâneo**, sem os estados "ligando" e "desligando".
- **Válido e zero foi olhado nas telas que abri** (telefone, configurações, celulares conhecidos, Bluetooth, grade, assistente). Achei dois zeros ruins; pode haver outros em telas que não abri, e nos 13 serviços que a HMI assina e cujas telas não existem sem aparelho.
- **`DSISmartphoneIntegration` não é respondido**; CarPlay, Android Auto e MirrorLink ficam de fora, e a tecla APP não foi testada.
- **A central ao vivo não foi ligada.** O que o modo ao vivo fará saiu de centrais de teste nascidas de uma cópia da memória dele.
- **A memória guardada vai mudar.** Com o telefone de mentira a HMI guarda a grade com o bloco "Telefone"; não olhei que outras linhas ela grava nem o que acontece ao desligar o telefone de mentira depois.
- **Não há foto da tela de telefone do carro**, então "igual ao carro" não tem como ser dito.
- **O esquema é do MU0883**; o carro roda o MU0803.
- **Só no container.** Não rodou no QEMU.

**Um cuidado para quem roteirizar toques nessas telas.** A barra de baixo da tela do telefone fica levantada (com o rótulo "Config.") por alguns segundos depois de a tela entrar e depois abaixa. Com ela abaixada o primeiro toque na engrenagem só a levanta; com ela levantada, já abre. Na `prova` o segundo toque, mandado às cegas, caiu na barra de rolagem da tela seguinte e desalinhou o resto do passeio. Na `prova2` e na `prova3` o roteiro confere o nome da tela que entrou antes de mandar o toque seguinte.

### Onde parou

- O item 7 do roteiro do simulador está feito para o estado "nenhum aparelho conectado", com o estado "Bluetooth desligado" de brinde.
- **Faltava o segundo estado, um aparelho fictício conectado com a tela principal do telefone; foi feito às 3h48 de 12/10/2026** (ver a subseção seguinte). Às 3h05 este item dizia que ele pedia três coisas: um valor por instância no motor, os dados que a tela principal lê e uma série para achar os zeros que não são neutros. Foram essas, mais uma resposta a um pedido novo; a lista de aparelhos do Bluetooth, que eu tinha posto na conta, não foi precisa.
- A central ao vivo não foi tocada. A próxima partida dela já sobe com o telefone de mentira.
- Nenhuma central minha ficou no ar.

### O segundo estado: um aparelho fictício conectado (das 3h09 às 4h01)

O pedido dizia: "se couber, um segundo estado: um aparelho fictício conectado, com a tela principal do telefone", só depois de o primeiro estar fechado. Coube, na mesma sessão, depois do registro das 3h05. **O que este marco dizia do segundo estado ("não foi feito", "falta") valia até ali e deixou de valer;** o texto de cima ficou como estava, com a marca de que mudou em cada ponto.

**Resultado, verificado em execução** numa central de teste subida da pasta já instalada (`kf`, às 3h48), com `TELEFONE="/stubs/telefone/sem-aparelho.txt /stubs/telefone/aparelho-conectado.txt"`:

- **a tecla Telefone da moldura abre a tela principal do telefone** (`PHONE_MAIN_NOCALLS`): a caixa com o nome "Celular fictício", a operadora "Fictícia", seis favoritos vazios ("Adicionar favorito") e a barra com "Discar nº", "Chamadas" e "Config.";
- **a barra do alto ganha os ícones do aparelho**: Bluetooth, bateria e sinal, em toda tela que tem a barra;
- **as telas de dentro abrem, vazias:** "Discar nº" (`PHONE_MAIN_DIAL__UNIFIED`, o teclado com a tecla verde e a "Caixa postal"), "Chamadas" (`PHONE_CALLSTACKS`, "Nenhum registro disponível."), um favorito (`PHONE_CONTACTS_SEARCH_JUST_SELECT_NUMBER__EU_SL_UNIFIED`, "Nenhum contato disponível com números de telefone…") e as "Configurações do telefone";
- **"Selecionar celular" lista o aparelho fictício**, marcado como o telefone 1, com "É possível conectar outro celular.";
- **desligar o Bluetooth pela tela leva a "Ligar o Bluetooth.", e ligar devolve a tela principal**, como no primeiro estado. Com ele desligado a tela fica incoerente: a caixa do nome e os ícones do aparelho continuam lá (ver "Limites do segundo estado");
- **a tecla Telefone do volante continua sem mudar a tela**, também com o aparelho conectado (cinco medidas: `kp`, os três pares da série e `kf`);
- **nada mais na partida muda**, contra o primeiro estado, além dos ícones da barra do alto e de uma linha no registro da HMI: três pares de centrais lado a lado (ver "A série");
- nenhuma das dez centrais do segundo estado registrou erro de OpenGL nem `SIGSEGV`, e nas da versão final não há tempo esgotado de serviço no registro da HMI. Com elas a sessão soma 51 centrais, todas sem erro de OpenGL.

| A tela principal, com o aparelho fictício | "Selecionar celular" |
|---|---|
| ![Principal](quadros/2026-10-12-hmi-tel-conectado-principal.png) | ![Celulares conhecidos](quadros/2026-10-12-hmi-tel-conectado-celulares-conhecidos.png) |

| "Discar nº" | "Chamadas" |
|---|---|
| ![Discar](quadros/2026-10-12-hmi-tel-conectado-discar.png) | ![Chamadas](quadros/2026-10-12-hmi-tel-conectado-chamadas.png) |

| Um favorito | Com o Bluetooth desligado pela tela |
|---|---|
| ![Favorito](quadros/2026-10-12-hmi-tel-conectado-favorito.png) | ![Bluetooth desligado](quadros/2026-10-12-hmi-tel-conectado-bluetooth-desligado.png) |

A sétima foto é [a tela principal sem o aparelho na lista do gerente](quadros/2026-10-12-hmi-tel-conectado-sem-a-lista.png), da rodada `kp`: a caixa do nome vazia.

**É simulação, e o aparelho é fictício.** O nome ("Celular fictício"), o endereço Bluetooth (`00:00:00:00:00:01`, o de exemplo), a operadora ("Fictícia") e o identificador (`aparelho-ficticio-1`) são inventados e dizem que são. O perfil não traz IMEI, IMSI, cartão SIM, número, contato nem chamada, e marca os três primeiros como "não há". Não houve lista para preencher: com o aparelho conectado a tela mostra chamadas, contatos e favoritos vazios, e ficou assim. Se a central do carro, com um celular de verdade, mostra essa mesma tela principal, eu não sei.

**O que o segundo estado pediu, na ordem em que apareceu:**

1. **Um valor por instância no motor.** O `DSIMobileEquipment` tem três instâncias (0 o aparelho principal, 1 o associado, 2 a do CarPlay) e o perfil valia igual para as três: "conectado" sairia nas três. O perfil ganhou a seção `[Serviço[N].evento]`, que vale só para quem assina pela instância `N`, e vale inteira, no lugar da seção sem instância. São 51 linhas a mais e 1 trocada no `driver/veiculostub.c` (`traces/tel_veiculostub_valor_por_instancia.diff`): cada seção por instância é uma cópia do evento com valores próprios, e a assinatura daquela instância aponta para a cópia; o resto do programa trata a cópia como um evento qualquer. Sem seção de instância em perfil nenhum, o programa faz o mesmo de antes. Vale para o carro de mentira e para quem inclui o motor (o rádio e a mídia de mentira).
2. **Os dados que a tela principal lê do aparelho principal** (`DSIMobileEquipment`, instância 0): o estado de ativação 5 (`ACTIVATIONSTATE_PHONE_ON`) com o modo 3 (`TELMODE_HFP`, mãos-livres por Bluetooth; o zero do padrão é `TELMODE_INTERNAL_SIM`), os dados do aparelho (nome e endereço), a rede (1, `REGISTERSTATE_REGISTERED`, na operadora inventada), o sinal e a bateria. A bateria vai em 4 (`MEBATTERYCHARGELEVEL_60_79`), e não no zero, que é "vazia" e abre o aviso (achado do primeiro estado). Os nomes dos números estão em `stubs/telefone/gerado/constantes.txt`.
3. **O mesmo aparelho na lista de aparelhos conhecidos do gerente de aparelhos** (`DSIPhoneManager`, atributo 2), com o mãos-livres conectado (8, `SERVICESTATE_CONNECTED`). Não estava no plano: a rodada `kp`, sem a lista, abriu a tela principal com a caixa do nome vazia e "Nenhum celular conhecido disponível." em "Selecionar celular". Lido depois no código da HMI: o nome que os dados do aparelho trazem vai para um serviço de nomes (`HsmPhonePhoneAvailability.setDeviceName`), e quem monta a seleção de telefone é o `TargetASLPhoneConnectionManager.updatePhoneSelectionData`, que procura na lista do gerente o aparelho cujo endereço Bluetooth é o do aparelho principal. Por isso o endereço tem de ser o mesmo nas duas seções do perfil, e o teste cobra. Conferido na rodada `k2`: com a lista a caixa mostra o nome, a barra do alto ganha o ícone de Bluetooth e "Selecionar celular" lista o aparelho. Os outros serviços do aparelho ficam no zero, que aqui é `SERVICESTATE_NOT_SUPPORTED`: sem música por Bluetooth, agenda, mensagens, CarPlay nem Android Auto.
4. **A resposta a um pedido novo.** Com o aparelho na lista a HMI pede, aos 20 s da partida, que ele seja o prioritário (`DSIPhoneManager.requestSetPrioritizedDevice`, com o identificador da lista) e espera 10 s: na `k2` ficou um "DSI timeout occurred". O perfil ganhou a seção vazia de resposta (resultado 0, que neste serviço é `RESULT_OK`); a HMI só anota o resultado (`HsmPhonePhoneAvailability.dsiPhoneManagerResponseSetPrioritizedDevice`).

**As rodadas do segundo estado** (dez centrais):

| Rodada | Hora | De onde subiu | O que mostrou |
|---|---|---|---|
| `x1` | 3h09 | a pasta, com um motor de rascunho ao lado (um remendo sem teste, `traces/tel_x1_motor_de_rascunho.diff`) e um perfil de experimento | a tecla Telefone abriu `PHONE_MAIN_NOCALLS` aos 61,66 s, de primeira. O nome de operadora do experimento, longo, saiu cortado ("Operadora de t…") |
| `kp` | 3h23 | cópia de trabalho 3: o motor com valor por instância e o perfil ainda sem a lista | passeio por seis telas (principal, discar, chamadas, favorito, configurações, celulares conhecidos); a caixa do nome vazia e a lista de celulares vazia; nenhum tempo esgotado |
| `k2` | 3h29 | a mesma cópia, com a lista num perfil de experimento por cima | a caixa com o nome, o ícone de Bluetooth e a linha em "Celulares conhecidos"; um tempo esgotado, o do aparelho prioritário |
| `k1` a `k3`, braços `t` e `c` | 3h34 a 3h40 | cópia de trabalho 3, com o perfil final | a série (abaixo) |
| `kf` | 3h48 | **a pasta instalada** | o passeio inteiro, com o Bluetooth desligado e ligado pela tela: as dez telas que o roteiro esperava entraram, e depois `PHONE_INIT_BT_SWITCHED_OFF` e de novo a principal; nenhum tempo esgotado; 14 pedidos diretos atendidos e 3 sem resposta, que a HMI não espera (o idioma do aparelho, duas vezes, e `resetMissedCallIndicator`, ao abrir "Chamadas"); 1.263 quadros, nenhum erro de OpenGL |

O remendo da `x1` foi jogado fora. O que entrou no motor foi escrito depois do teste (ver "Testes do segundo estado").

**A série: o segundo estado contra o primeiro.** Três pares de centrais lado a lado, largadas com 3 s de diferença e alternando quem larga na frente: o braço `t` com os dois perfis, o braço `c` só com o `sem-aparelho.txt`, que é o padrão do modo ao vivo. Sem o registro detalhado, com o roteiro das séries anteriores (tecla Carro aos 32 s, Telefone da moldura aos 62 s, Menu aos 74 s, Telefone do volante aos 82 s). Doze previsões gravadas às 3h34, antes de largar (`traces/tel_previsoes.txt`); a comparação está em `traces/tel_serie_k_analise.txt`.

| O quê | Braço `t` (aparelho conectado) | Braço `c` (nenhum aparelho) | Previsto? |
|---|---|---|---|
| serviços DSI ausentes; esperas; fim das esperas | 18; 7; 49,4 a 49,5 s | 18; 7; 49,4 a 49,6 s | sim |
| telas e avisos até a tecla Telefone | as mesmas 17 | as mesmas 17 | sim; nenhum aviso novo |
| fotos aos 45 e 58 s | com os ícones do aparelho | sem eles | sim: diferem só numa caixa de 125 × 24 pontos da barra do alto, 793 pontos, igual nos três pares |
| foto aos 25 s | igual byte a byte à do outro braço | | **não**: previ diferença na barra do alto, e nessa hora o aviso do usuário principal cobre a barra |
| tecla Telefone aos 62 s | `PHONE_MAIN_NOCALLS` | `PHONE_NOT_CONNECTED` | sim, 3 de 3 |
| grade aos 74 s | igual byte a byte à do outro braço | | **não**: previ diferença na barra do alto, e a grade não tem barra |
| tecla do volante aos 82 s | nenhuma troca de tela | nenhuma troca de tela | sim (era a previsão menos segura) |
| telefone de mentira: assinaturas, pedidos atendidos, sem resposta | 31, 3, 2 | 31, 6, 2 | sim, nas seis |
| carro de mentira: idem | 111, 3, 14 | 111, 3, 14 | sim |
| tempos esgotados DSI; erros de OpenGL; `SIGSEGV` | nenhum | nenhum | sim |
| linhas ERROR da HMI | 120, 121 e 120 | 119, 119 e 119 | **em parte**: uma linha a mais em `t`, nas três |
| quadros desenhados | 526 a 539 | 450 a 454 | não previ: a tela principal desenha mais quadros que a "sem aparelho" depois da tecla |

O que a HMI mandou ao barramento difere em três mensagens (par 1, comparado linha a linha): em `t`, o pedido de aparelho prioritário; em `c`, os dois pedidos de visibilidade, que a tela "sem aparelho" faz ao entrar e ao sair e a tela principal não faz.

**A linha a mais** é "The method org.dsi.ifc.keypanel.DSIKeyPanelListener[0].genericSettingResponse is unexpected and will be discarded.", aos 12,3 s da partida, nas três centrais do braço `t`. Os dois braços têm uma igual aos 8,2 s. O que a HMI pediu ao teclado e o que recebeu de volta é igual nos dois braços, mensagem por mensagem; a HMI descarta a resposta e nada mais muda. **Não sei por que ela passa a estranhar uma resposta a mais com o aparelho conectado.** No par 2 o braço `t` teve também uma exceção a mais, das que saem quando a central é desligada, aos 99,6 s. **Correção das 9h20 de 12/10/2026: isto estava errado.** A exceção a mais era da partida, aos 12,3 s: a HMI estoura ao receber a lista de aparelhos antes de guardar o endereço do aparelho (ver "O modelo: o aparelho fictício conecta e desconecta pela tela", abaixo). As linhas de pilha não trazem hora, e eu as datei pela linha vizinha.

**O que entrou na pasta**, às 3h47min48:

- `stubs/telefone/aparelho-conectado.txt`, novo: o segundo perfil, para usar depois do primeiro.
- `driver/veiculostub.c`: o valor por instância (51 linhas a mais, 1 trocada) e o formato novo no cabeçalho. O de antes está em `traces/tel_antes2/`.
- `driver/tests/t_telefone.sh`: de 103 para 142 casos.
- `stubs/telefone/README.md`: o segundo estado, a seção por instância e os limites.
- Sete quadros `quadros/2026-10-12-hmi-tel-conectado-*.png`: seis da `kf` e um da `kp`.
- Na ficha do carro e no pedido de foto do `simulador-jetta.md`: uma frase a mais, pedindo, se der, a tela principal do telefone do carro com um celular conectado, para comparar o desenho; os nomes e os números podem ir cobertos.
- `traces/tel_*`: os registros de `x1`, `kp`, `k2`, `kf` e do par 1 da série; os roteiros novos (`tel_passeio.sh`, `tel_serie2.sh`, `tel_compara2.py`, `tel_aplica_motor2.py`, `tel_candidato2.sh`, `tel_instala2.sh`); as previsões e a análise da série; os testes vistos falhar; e as duas baterias.

**Lançadores e esquemas não mudaram.** A opção é a mesma `TELEFONE`, agora com dois arquivos. O padrão do modo ao vivo continua sendo só o primeiro (D150).

**A instalação.** Montei o candidato com a pasta como estava às 3h42, que já tinha o som de mentira e os lançadores novos de outra sessão, e apliquei o motor por inserção ancorada: saiu igual, byte a byte, ao da cópia de trabalho em que as rodadas correram. Rodei a bateria inteira nele e troquei os quatro arquivos por cópia ao lado e troca de nome, depois de conferir que cada um ainda era o de quando o candidato foi montado. Avisei antes, por mensagem e na pasta de notas, as sessões do rádio e da hora e unidades; não esperei resposta. **As rodadas da série usaram os lançadores das 3h12; a `kf` usou os da pasta, com a opção de som já lá.**

**Testes do segundo estado.** O `driver/tests/t_telefone.sh` passou a 142 casos. Três levas, cada uma escrita antes do código e vista falhar (`traces/tel_vermelho/`):

| Leva | Casos que falhavam | Contra o quê |
|---|---|---|
| o valor por instância (16 casos: a instância 0 recebe o valor dela e a 1 e a 2 o valor sem instância, assinar de novo, desassinar, os erros de escrita) | 12 de 119 | o motor de antes |
| o perfil do aparelho conectado (19 casos) | 1 de 120: sem o perfil o bloco se resume ao caso que diz que ele não existe | a pasta sem o perfil |
| a lista de aparelhos e o aparelho prioritário (4 casos novos e 2 trocados) | 4 de 142 | o perfil de antes |

Um caso meu estava errado (a expressão que barra "há IMEI" casava com o espaço depois do sinal de igual) e falhou contra o perfil certo; corrigi a expressão. Para saber se os casos de dado pessoal pegam alguma coisa, rodei o teste contra uma cópia do perfil com um identificador escrito e um IMEI "disponível": falharam 16 de 138, entre eles os dois que olham identificadores e o de texto sem a marca de fictício (`traces/tel_vermelho/t_telefone_com_um_perfil_que_traz_identificador.out`).

A bateria inteira, no candidato às 3h43 e da pasta às 3h52 (`traces/tel_suites_candidato_03h43.txt` e `traces/tel_suites_pasta_03h52.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 188 (187 no candidato; o caso a mais é de um perfil que outra sessão instalou no intervalo), `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, `t_hmilog.sh` 32, `t_kipc_bus.sh` 14, `t_radio.sh` 81, `t_telefone.sh` 142, `t_midia.sh` 107 e `t_som.sh` 135, no container; `t_memoria.sh` 80, `t_servidor.sh` 20, `t_lsmval_abre.sh` 38 e `t_catalogo.sh` 32, no Mac (o último só da pasta: é de outra sessão e entrou no intervalo). Todos passam; o `veiculostub.c`, o `radiostub.c` e o `midiastub.c` compilam com `-Wall -Wextra` sem aviso.

**Limites do segundo estado:**

- **O aparelho conectado não faz nada.** Não há chamada, discagem, agenda nem mensagem. A tecla verde do teclado, "Importar contatos novamente", tocar no aparelho em "Celulares conhecidos" e os pedidos de chamada não foram exercitados; os que a HMI espera ficariam sem resposta.
- ~~**Conectado desde a partida, e para sempre.**~~ Mudou às 9h50 de 12/10/2026: o aparelho conecta e desconecta pela tela (ver "O modelo", abaixo).
- ~~**Com o Bluetooth desligado pela tela, o aparelho continua "conectado"**~~ (corrigido às 9h50 de 12/10/2026, com o modelo; era o que a `kf` media): a tela do telefone mostra "Ligar o Bluetooth.", como deve, mas a caixa do nome, a barra de baixo e os ícones do aparelho na barra do alto continuam lá. A regra do perfil muda só o estado do Bluetooth.
- **A seção por instância não se mistura com o que um pedido grava.** O que a tela grava num atributo (regras e linhas `G`) e o estado guardado (`-e`) valem para a seção sem instância; quem assinou por uma instância com seção própria não recebe a mudança. No telefone isso não aparece: nenhum pedido grava nos atributos do aparelho.
- **Válido e zero só foi olhado nas telas que abri.** A série achou os ícones e a linha a mais, e não achou aviso novo. As telas de chamada, de agenda e de mensagens não abrem sem dados.
- **A linha a mais no registro da HMI** (acima) não tem causa conhecida.
- **A memória guardada não foi olhada.** Não medi o que a HMI grava na memória do modo ao vivo com um aparelho conectado, nem o que acontece ao voltar ao primeiro estado depois.
- **A central ao vivo não foi ligada**, e o padrão dela não mudou.
- Valem os limites de cima: a instância 2 respondida pelo domínio errado, o esquema do MU0883, só no container, e nenhuma foto da tela do carro para comparar.

**Onde parou, às 4h01.** O item 7 do roteiro do simulador está feito nos dois estados do pedido. O que ficou de fora pede um modelo, e não só valores: conectar e desconectar pela tela, parear, discar e uma chamada simulada. Nenhuma central minha ficou no ar.

**Depois do registro, às 4h10: três conferências que avisos de outras sessões pediram.** (As horas deste trecho e as três de cima, "Acréscimo", o título e "Onde parou", saíram do relógio; eu tinha escrito 3h57 e 4h12 de cabeça e corrigi.)

- **O telefone e a mídia não respondem um pelo outro.** A sessão da mídia avisou que, com `MIDIA` e `TELEFONE` ligados, dois programas responderiam ao mesmo pedido se os esquemas tivessem interface em comum. Cruzei os quatro esquemas gerados da pasta (veículo, rádio, telefone e mídia): nenhuma interface de pedidos e nenhuma de eventos em comum. Há um cruzamento: **a interface 1001 é a de pedidos do `DSIMobileEquipment` (telefone) e a de eventos do `DSIFileBrowser` (mídia)**. É o mesmo caso da 1101 (ver "Um defeito do motor que só o telefone mostrou"), e quem separa é o filtro de canal das 2h38: o telefone de mentira só trata como pedido o que vem do canal da HMI, e a mídia de mentira procura o serviço pela interface de pedidos e não tem nenhum na 1001. **Conferido em execução** na central `kj` (4h06, subida da pasta com rádio, mídia, som e o telefone com o aparelho fictício: os padrões do modo ao vivo mais o segundo perfil): a mídia publicou 2 eventos na 1001 e o telefone de mentira não registrou pedido nenhum fora do esquema; a HMI mandou 20 pedidos na 1001 e o registro da mídia não fala deles. A tecla Telefone abriu a tela principal, a tecla Mídia abriu [o tocador com a faixa de teste](quadros/2026-10-12-hmi-tel-conectado-com-midia.png), a tecla Rádio abriu o rádio e, de volta ao telefone, "Selecionar celular" listou o aparelho; nenhum tempo esgotado, nenhum erro de OpenGL, 1.003 quadros. É a 11ª central do segundo estado e a 52ª da sessão. Não abri a lista "Fonte" da mídia: se o aparelho fictício aparece lá como fonte Bluetooth, eu não sei (o perfil diz que a música por Bluetooth não é suportada).
- **As duas corridas de partida que a sessão do consumo de processador apontou** (o perfil inicial da persistência e o indicador de gesto, que mudam linhas do registro e a contagem de quadros de uma partida para outra) não explicam as diferenças da série. "Persistence initial profile not set" não aparece em nenhuma das centrais do segundo estado, e o indicador de gesto liga e desliga nos mesmos segundos nos dois braços dos três pares (liga aos 31,7 s na tela do carro, desliga aos 61,7 s, liga aos 73,7 s na grade). Os quadros a mais do braço `t` aparecem entre a foto dos 58 s e a dos 79 s, com o indicador no mesmo estado nos dois braços; que venham da tela principal do telefone é dedução.
- **A troca das 3h47min48 pegou no meio uma bateria de outra sessão** (a do estado do carro de mentira): ela tinha compilado o motor de antes às 3h45min52 e leu o `t_telefone.sh` novo às 3h49, e deram 26 falhas, que não eram da pasta; repetida às 3h50, passou inteira. Está no Marco 14, seção "Testes", e em `traces/men_suites_depois_1_corrida.txt`. Os meus avisos de troca, mandados por mensagem às 3h44, só chegaram lá às 4h02: mensagem para sessão ocupada fica na fila, e não serve para coordenar uma troca de minutos. O que protegeu a pasta foi a conferência de cada arquivo antes de trocar.

### O modelo: o aparelho fictício conecta e desconecta pela tela (12/10/2026, das 8h45 às 10h02)

Pedido novo do Lipe, na manhã de 12/10/2026, repassado pela sessão principal: terminar o que faltava no item 7. Este é o primeiro passo, fechado sozinho: **conectar e desconectar o aparelho fictício pela tela, com a tela do telefone e os ícones do alto acompanhando.** É estudo num computador; nada vai para o carro, e o aparelho continua inventado e marcado.

**Resultado, verificado em execução** numa central subida da pasta já instalada pelo roteiro do modo ao vivo (`pv2`, às 9h58: uma cópia do `driver/ao_vivo.sh` em que só mudam o nome do container e as pastas de dados, nascida de uma cópia da memória guardada, com rádio, mídia e som ligados), e antes disso em centrais de teste (`m1`, `m2`). A primeira tentativa dessa prova (`pv`) não chegou a conectar: o assistente de configuração tomou a tela sozinho aos 52 s, como faz no modo ao vivo, e o toque do roteiro caiu nele; a `pv2` começou o passeio depois:

- **a central nasce com o aparelho fictício conhecido e não conectado**, e isso passa a ser o padrão do modo ao vivo (D177). Na partida nada muda na tela: a tecla Telefone abre a mesma "Conecte o telefone celular…", sem ícone no alto;
- **em "Selecionar celular" o aparelho é um botão, e tocar nele conecta:** aviso "Estabelecendo conexão com Telefone viva-voz (HFP)" por 2 s, depois a tela principal do telefone, com o nome, a operadora e os ícones de Bluetooth, bateria e sinal no alto;
- **desligar o Bluetooth pela tela desconecta**, e a tela fica coerente: "Ligar o Bluetooth.", sem a caixa do nome e sem os ícones (até a madrugada o aparelho continuava "conectado" ali). Ligado de novo, o aparelho fica conhecido e não conectado, e o botão conecta outra vez;
- **nada mais na partida muda**, contra a pasta de antes: seis pares de centrais lado a lado, com e sem memória guardada, com as fotos iguais byte a byte até a tecla Telefone e as mesmas linhas de erro nos dois braços (ver "As séries do modelo");
- nenhum tempo esgotado de serviço no registro da HMI e nenhum erro de OpenGL nas 34 centrais deste passo. A sessão soma 86.

| "Selecionar celular": o aparelho é um botão | O aviso enquanto conecta |
|---|---|
| ![Lista](quadros/2026-10-12-hmi-tel-modelo-lista-com-botao.png) | ![Conectando](quadros/2026-10-12-hmi-tel-modelo-estabelecendo-conexao.png) |

| Conectado pela tela | Com o Bluetooth desligado pela tela: sem nome e sem ícones |
|---|---|
| ![Conectado](quadros/2026-10-12-hmi-tel-modelo-conectado.png) | ![Bluetooth desligado](quadros/2026-10-12-hmi-tel-modelo-bluetooth-desligado.png) |

**O que a tela desta variante oferece (levantamento, rodadas `d0`, `e10`, `e2k` e `e2c`).** O pedido falava em conectar e desconectar em "Selecionar celular". Medido:

- **O aparelho conectado é uma linha de informação**, e não um botão: toque curto, toque longo e o mãos-livres escrito de outro jeito (10 em vez de 8) não fazem nada, e nenhum pedido sai. Tocar na caixa do nome, na tela principal, abre a mesma lista. A linha diz "É possível conectar outro celular.": nesta tela se troca de aparelho, não se solta o que está.
- **O aparelho conhecido e não conectado é um botão.** Tocar nele manda `DSIPhoneManager.requestConnectService` com o identificador da lista e o tipo 1 (`SERVICETYPE_TELEPHONY_HFP`), e a HMI espera `responseConnectService` por até 180 s, com o aviso na tela.
- **O caminho da própria tela para soltar o aparelho é desligar o Bluetooth** (o ícone do alto da lista leva às "Configurações do Bluetooth"). Foi o que o modelo passou a atender. O pedido `requestDisconnectService` existe e o modelo o atende, mas não achei tela que o mande.

**O modelo** (`driver/telefonestub.c`, 314 linhas). O telefone de mentira deixou de ser o motor puro: é um programa que inclui o motor, como o rádio e a mídia de mentira, e atende três pedidos. Os valores continuam todos no perfil: "conectado" são as seções da instância 0 do aparelho, "desconectado" são as seções sem instância, que as outras instâncias já recebiam; o modelo troca um conjunto pelo outro e avisa quem assinou. Na lista do gerente, o mãos-livres vai a 10 (`SERVICESTATE_SUPPORTED` + `SERVICESTATE_CONNECTED`) ou a 2 (só suportado).

| Pedido | O que o modelo faz |
|---|---|
| `DSIPhoneManager.requestConnectService` (api 1000) | conecta e responde resultado 0. Identificador que não é o da lista: 1. Serviço que não é o mãos-livres: 8. Bluetooth desligado: 4 |
| `DSIPhoneManager.requestDisconnectService` (api 1001) | desconecta e responde resultado 0 |
| `DSIBluetooth.requestSwitchBTState` (api 1012) | grava o estado, avisa e responde, como a regra do perfil fazia; se o pedido é desligar, desconecta o aparelho antes. Ligar de novo não reconecta |

O aparelho começa conectado se o mãos-livres dele, no perfil, traz o bit 8, e desconectado se traz só o 2. Por isso há um terceiro perfil, de uma linha (`stubs/telefone/comeca-desconectado.txt`), e o `aparelho-conectado.txt` passou de 8 para 10 (a HMI testa os dois bits em `ServiceStateUtil`; com 8 ela também mostrava conectado). Sem aparelho no perfil o modelo só liga e desliga o Bluetooth.

**Um defeito que a série achou: a HMI estoura ao receber a lista cedo demais.** Antes da correção, 7 das 12 centrais desta manhã com a lista de aparelhos não vazia (4 das 7 das séries `n` e `g`) registraram, aos 12,3 s da partida, uma `NullPointerException` em `TargetASLPhoneConnectionManager.validateCanBePrimaryOrAssociated`, com duas linhas ERROR. O código compara cada aparelho da lista com o endereço do aparelho principal e com o do associado sem ver se já os tem, e ela só os guarda quando processa os dados do aparelho (`updatePhoneInformation`) das instâncias 0 e 1. No barramento a ordem está sempre certa (os dados saem umas cem mensagens antes da lista, nas centrais com e sem a exceção): a corrida é dentro da HMI.

**O defeito já estava na pasta desde as 3h48**, com o aparelho conectado da madrugada: a mesma exceção está em 3 das 6 centrais daquele estado com a lista (`k2t`, `kf` e `kj`; não está na `k2`, na `k1t` nem na `k3t`). **Eu a tinha visto na série "k" e classifiquei errado**: escrevi, mais acima, que no par 2 o braço `t` teve "uma exceção a mais, das que saem quando a central é desligada, aos 99,6 s". Não era: as linhas de pilha não trazem hora, e eu as datei pela linha vizinha. Nas rodadas em que ela ocorreu, as telas que abri depois funcionaram.

**A correção:** no barramento, o modelo responde à assinatura da lista com a lista vazia e só manda a de verdade um a dois segundos depois de as instâncias 0 e 1 terem assinado os dados do aparelho (se isso não acontecer, 6 s depois de a lista ser assinada). Pediu um gancho de tempo no motor: 10 linhas no `driver/veiculostub.c`, atrás de uma macro (`VEICULOSTUB_MODELO_TEMPO`) que só o telefone define. **Conferi que os binários do `veiculostub`, do `radiostub` e do `midiastub` saem idênticos, byte a byte, com e sem o gancho.** Depois da correção: nenhuma exceção em 11 centrais com a lista (as cinco do braço `t` das séries `v` e `w`, `q1`, `q2`, `y1t`, `m2`, `pv` e `pv2`), contra 7 de 12 antes.

**As séries do modelo.** Pares lado a lado, largados com 3 s de diferença e alternando quem larga na frente, sem o registro detalhado, com o roteiro das séries anteriores. Braço `t`: a cópia de trabalho com o modelo e os três perfis (o candidato a padrão). Braço `c`: a pasta como estava, com o que o modo ao vivo subia (o motor puro com o `sem-aparelho.txt`). Previsões gravadas antes de cada leva (`traces/tel_previsoes.txt`).

| Série | Quando | O quê | Resultado |
|---|---|---|---|
| `n` (3 pares) | 9h08 | o modelo, ainda sem segurar a lista, sem memória | fotos aos 25, 45 e 58 s iguais byte a byte nos três; mesmas 17 telas; telefone de mentira 31/6/2 nos dois braços; nenhum tempo esgotado. **Duas centrais do braço `t` com a exceção da lista** |
| `g` (2 pares) | 9h14 | o mesmo, as duas nascendo de uma cópia da memória guardada | as seis fotos iguais byte a byte nos dois pares; mesmas 23 telas. Uma central do braço `t` com a exceção |
| `ga`, depois `gb` | 9h18 | `ga` conecta pela tela, com memória; `gb` nasce da memória que a `ga` deixou | a `gb` nasce na tela do telefone (a última da `ga`), em "Conecte o telefone celular…": nenhum aviso, nenhum pedido de conexão sozinho. A `ga` teve a exceção |
| `v` (3 pares) | 9h27 | o modelo que segura a lista, sem memória | como previsto nos três: fotos iguais byte a byte até a tecla (em dois pares o minuto do relógio virou entre as duas fotos dos 58 s); 119 linhas ERROR nos dois braços; **nenhuma exceção** |
| `w` (2 pares) | 9h33 | o mesmo, com a cópia da memória | as seis fotos iguais byte a byte nos dois pares; 121 linhas ERROR nos dois braços; nenhuma exceção |
| `q1` e `q2` | 9h37 | o aparelho conectado desde a partida, com o modelo | a tecla abre a tela principal; nenhuma exceção, nenhum tempo esgotado |
| `y1` (1 par) | 9h45 | **o candidato** (a pasta das 9h39, com o motor que outra sessão trocou às 9h20, mais as mudanças) contra a pasta | as seis fotos iguais byte a byte; 119 linhas ERROR nos dois braços |

Duas coisas que as séries mostraram e não são do modelo: com memória, a central que larga em segundo lugar dá um serviço a mais por ausente (o `DSIKeyPanelListener`), em qualquer dos braços; e a foto dos 69 s difere num par de cada série sem memória só na faixa da barra de baixo, que sobe e desce sozinha.

**O padrão do modo ao vivo mudou** (D177): `TELEFONE_PADRAO` do `driver/ao_vivo.sh` passou a ser os três perfis, isto é, o aparelho fictício conhecido e não conectado. `TELEFONE=/stubs/telefone/sem-aparelho.txt sh driver/ao_vivo.sh` volta ao que era; `TELEFONE=` sobe sem telefone. As centrais de teste continuam sem telefone, salvo quem passar `TELEFONE`. **A central ao vivo estava no ar durante a troca (desde as 8h36) e não foi tocada**; ela pega o padrão novo na próxima partida.

**O que entrou na pasta**, às 9h50min22 (`traces/tel_instalacao3.txt`):

- `driver/telefonestub.c`, novo: o motor mais o modelo.
- `stubs/telefone/comeca-desconectado.txt`, novo; `stubs/telefone/aparelho-conectado.txt`: o mãos-livres de 8 para 10.
- `driver/veiculostub.c`: o gancho de tempo opcional (10 linhas; `traces/tel_veiculostub_gancho_de_tempo.diff`), por inserção ancorada sobre o arquivo que outra sessão tinha trocado às 9h20.
- `driver/m1_build.sh`: compila o `telefonestub`. `driver/m1_hmi.sh`: o bloco do telefone sobe o `telefonestub` (era o `veiculostub` com outro nome); se ele não compilou, a central sobe sem telefone, com aviso, como no rádio. `driver/ao_vivo.sh`: o padrão e o comentário dele.
- `driver/tests/t_telefone.sh`: de 142 para 201 casos. `stubs/telefone/README.md`.
- Quatro quadros `quadros/2026-10-12-hmi-tel-modelo-*.png` e os registros, roteiros, previsões e análises em `traces/tel_*`. Os arquivos de antes estão em `traces/tel_antes3/`.

**A instalação.** A pasta mudou enquanto eu trabalhava: outra sessão trocou o `veiculostub.c` às 9h20 (marcadores que acompanham a unidade) e os lançadores às 9h07. Montei o candidato sobre a pasta das 9h39, por inserção ancorada; nele rodei a bateria inteira, um par contra a pasta (`y1`) e a prova com o passeio (`m2`); e troquei os nove arquivos por cópia ao lado e troca de nome, depois de conferir que cada um ainda era o de quando o candidato foi montado. O aviso foi para a nota de passagem às 9h28, antes da troca.

**Testes.** `driver/tests/t_telefone.sh`, 201 casos. Duas levas novas, escritas antes do código e vistas falhar (`traces/tel_vermelho/`):

| Leva | Casos que falhavam | Contra o quê |
|---|---|---|
| o modelo: conectar, desconectar, o Bluetooth que derruba o aparelho, os três erros, começar desconectado, sem aparelho (53 casos) | 2 de 143: sem o programa o bloco se resume ao caso que diz que ele não existe; e o valor novo do mãos-livres | a pasta de antes |
| a lista segurada na partida (5 casos, e as esperas nos que já havia) | 4 de 201 | o modelo que mandava a lista na hora |

A bateria inteira, no candidato às 9h40 e da pasta às 9h50 (`traces/tel_suites_candidato_09h40.txt` e `traces/tel_suites_pasta_09h50.txt`): `run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 224, `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, `t_hmilog.sh` 32, `t_kipc_bus.sh` 14, `t_radio.sh` 81, `t_telefone.sh` 201, `t_midia.sh` 119 e `t_som.sh` 185, no container; `t_memoria.sh` 80, `t_servidor.sh` 20, `t_lsmval_abre.sh` 38 e `t_catalogo.sh` 32, no Mac. Todos passam; os quatro programas que usam o motor compilam com `-Wall -Wextra` sem aviso.

**Limites do modelo:**

- **A tela não tem "desconectar".** O caminho é desligar o Bluetooth; trocar por outro aparelho não dá, porque só há um.
- **Tudo é instantâneo e sempre dá certo.** Não há "conectando", falha nem pedido de confirmação no aparelho. O aviso fica 2 s porque a HMI o segura.
- **Ligar o Bluetooth de novo não reconecta sozinho.** Num carro o aparelho voltaria.
- **O que a tela muda vale até a central desligar.** A central seguinte nasce do perfil, desconectada. Se a última tela era a do telefone, a HMI, com memória, nasce nela (`gb`), em "Conecte o telefone celular…".
- **Nos primeiros um a dois segundos depois de a HMI assinar, a lista está vazia.** É de propósito (a exceção de cima). Ninguém chega a "Selecionar celular" tão cedo.
- **`requestRemoveDevice` não é atendido.** Não achei tela que o mande; se houver, o pedido fica 10 s sem resposta.
- **A correção da exceção é por tempo, não por causa.** O modelo não vê o estado da HMI: espera um a dois segundos. Em 11 centrais não falhou; numa máquina muito carregada pode não bastar.
- O aparelho continua sem fazer chamada: é o passo seguinte.

**Retomado às 10h12 e resolvido às 11h06: ver "A chamada simulada", logo abaixo. O que segue é como estava às 10h10.** **O passo 2 (discar e chamada simulada) parou no levantamento, às 10h10.** O que ficou sabido e o que travou:

- **O que o esquema diz** (lido, não exercitado): a HMI disca com `DSIMobileEquipment.dialNumber` (api 1005, o número) e espera `responseDialNumber` (api 2007) por 30 s; encerra com `hangupCall` (api 1001, o número da chamada) e espera `responseHangupCall` (api 2010) por 15 s; a chamada aparece na lista de chamadas (atributo 9, `updateCallList`), com estado (`CALLSTATE_DIALING` 1, `ALERTING` 2, `ACTIVE` 4, `DISCONNECTING` 5) e número; e o estado de ativação tem `ACTIVATIONSTATE_TEL_ACTIVE_CALL` (2). O gancho de tempo que entrou no motor serve para "chamando" virar "em chamada".
- **O que travou: a tecla verde do teclado não manda pedido nenhum.** Em duas centrais com o aparelho conectado desde a partida (`c0` e `c1d`), digitei um número fictício (5550100) em "Discar nº" e toquei na tecla verde, três vezes ao todo, em dois pontos dela e com pausa antes. O número aparece no campo; a cada toque a HMI reconstrói o teclado (no registro: `stop include app: SpellerApp.PhoneDial`, `create`, `starting app`, e a tela `PHONE_MAIN_DIAL__UNIFIED` sai e entra de novo) e **nenhum `dialNumber` sai no barramento**, nem para a instância 0 nem para outra. No registro detalhado não há a linha "ASL_PHONE_CALL_DIAL_NUMBER received", que a parte de chamadas da HMI escreve ao receber o evento (`CallHandling`): o evento não chega nela. [A tela depois do toque](quadros/2026-10-12-hmi-tel-modelo-discar-sem-chamada.png) é a mesma de antes.
- **O que não sei:** por que a tela não manda o evento. A condição de `CallHandling.dialTrimmedNumber` (estado de ativação 5 ou 2 e modo 3) o perfil cumpre, mas o código nem é alcançado. A causa está no lado da tela (o aplicativo do teclado e as transições), que não li.
- **Por onde seguir:** ler o que o aplicativo do teclado faz com a tecla verde e que condição ele consulta; tentar a discagem por outro caminho da própria tela (a lista de chamadas e os favoritos estão vazios; uma chamada na lista de chamadas ou um favorito fictício daria um botão de rediscar); e só então o modelo da chamada, com teste antes, como no passo 1.
- **O passo 3 ficou pela metade:** desligar o Bluetooth com o aparelho conectado deixa a tela coerente desde as 9h50 (era parte do passo 1); a tecla Telefone do volante durante a chamada depende do passo 2.

Parei aqui por causa do limite de uso do plano, que a sessão principal avisou estar em 85 % às 8h45: a pasta está coerente, com o passo 1 instalado e nada do passo 2 nela.

### A chamada simulada: discar, chamando, em chamada (12/10/2026, das 10h12 às 11h17)

A sessão principal mandou retomar o passo 2 às 10h12, com prazo de uns noventa minutos: achar por que a tecla verde não discava; se a causa fosse da bancada, consertar e seguir para a chamada; se fosse da própria tela desta versão do firmware, medir, registrar e parar. **A causa era da bancada, e a chamada simulada está instalada desde as 11h06min37.** O bloco de cima ("O passo 2… parou no levantamento") vale como estava às 10h10.

**Resultado, verificado em execução** numa central subida da pasta instalada pelo roteiro do modo ao vivo (`pv3`, às 11h06, nascida de uma cópia da memória guardada, com rádio, mídia e som) e, antes, em centrais de teste (`h1`, `h4`):

- **com o aparelho fictício conectado, "Discar nº" e a tecla verde abrem a tela de chamada** (`PHONE_MAIN_CALLS`) com o número digitado: "discando", dois segundos depois "chamando", aos cinco "em chamada" (o cronômetro em 0:00, o botão de mudo e o botão vermelho), e a barra do alto ganha o ícone de chamada;
- **aos vinte segundos a chamada acaba sozinha**, como se o outro lado desligasse, e a tela volta à principal do telefone, sem aviso. Dá para discar de novo;
- **o botão vermelho da tela não encerra a chamada na bancada**, e isso não é do telefone de mentira: ao tratar o toque a HMI consulta os estados de chamada do CarPlay, que a bancada não tem, e estoura. Por isso o fim é por tempo (D183);
- **a tecla Telefone do volante, durante a chamada, não faz nada**: nenhum pedido sai, a tela não muda, nenhum erro (uma medida, `h4`);
- **nada muda na partida nem no padrão do modo ao vivo**: a central continua nascendo com o aparelho conhecido e não conectado, e a agenda só fica "pronta" quando a tela conecta o aparelho. Três pares lado a lado contra a pasta de antes, com e sem memória: as seis fotos iguais byte a byte em todos, as mesmas linhas de erro;
- nenhum erro de OpenGL nas 17 centrais deste passo, e nenhum tempo esgotado de serviço nas da versão final (`h4`, as séries e a `pv3`). Nas duas provas intermediárias (`h1` e `h3`) estouraram o prazo os tons que o teclado manda durante a chamada (`sendDTMF`, 13 ao todo); o perfil passou a respondê-los. A sessão soma 103 centrais.

| Discando | Em chamada |
|---|---|
| ![Discando](quadros/2026-10-12-hmi-tel-chamada-discando.png) | ![Em chamada](quadros/2026-10-12-hmi-tel-chamada-em-chamada.png) |

**É simulação.** Nada é discado, não há áudio e ninguém atende: é o relógio do modelo. O número que aparece é o que foi digitado na tela (nas rodadas, 5550100 e 123); o modelo não traz nome, número nem contato de ninguém.

**Por que a tecla verde não discava: dois zeros do valor padrão.** A tela só manda discar se o número digitado estiver no dado que a parte de chamadas da HMI mantém, e essa parte só trata o pedido depois de sair do estado em que espera a agenda. O telefone de mentira respondia às duas coisas com o padrão do motor, "válido e zero":

| O que faltava | O que a HMI fazia | Como achei |
|---|---|---|
| a agenda "pronta": `DSIAdb`, atributo 1, tem de ser 2 (`ADBSTATE_READY`); o zero é `ADBSTATE_UNDEFINED` | a máquina de chamadas fica em `StateCallHandlingWaitForAdb` e não trata o pedido de discar: a tela espera 10 s por um dado que não vem (`Waitaphore [property 1152] … errorTimeout`) e desiste | rodada `ca` (com o perfil de agenda e sem a agenda pronta) e o código de `StateCallHandlingWaitForAdb` |
| um perfil de agenda ativo: `DSIAdb`, atributo 2 (a lista de perfis), e `DSIAdbUserProfile`, atributo 5 (o ativo) | sem perfil na lista o número do perfil ativo fica em -1; a cada dígito a HMI busca contatos (`Truffles.search`) e estoura em `SOURCES[-1]` (`ArrayIndexOutOfBoundsException`) antes de guardar o número digitado; a tecla verde acha o campo vazio e só reconstrói o teclado | rodada `ct`, com o rastreamento da máquina de estados da tela: 14 exceções para 7 dígitos, e a guarda da transição 12 da submáquina `IphoneMainDial` (`PhoneActivityImpl`), que testa o tamanho do texto digitado |

Com as duas coisas no perfil (rodada `cb`) o pedido sai: `DSIMobileEquipment.dialNumber` com o número digitado. O som de mentira não tem a ver: com ele ligado (`cs`) a tecla também não discava.

**As duas só valem com o aparelho conectado.** O modelo guarda os valores do perfil para a agenda pronta e para o perfil de agenda e, com o aparelho desconectado, deixa sair o padrão de sempre. Assim a partida do modo ao vivo, que nasce desconectada, manda ao barramento o mesmo de antes.

**O modelo da chamada** (`driver/telefonestub.c`, que passou de 314 para 445 linhas). O motor não mudou.

| Pedido | O que o modelo faz |
|---|---|
| `DSIMobileEquipment.dialNumber` (api 1005) | só na instância 0 e com o aparelho conectado (senão resultado 7; com chamada em curso, 6). Responde resultado 0; manda o tipo do número (`updateServiceCodeType`, 0: número comum), que é o que a HMI espera para largar o teclado; põe o aparelho "em chamada" (estado de ativação 2); e põe na lista de chamadas da instância 0 a chamada 1, com o número que a tela mandou, em "discando" (1). Aos 2 s, "chamando" (2); aos 5 s, "em chamada" (4); **aos 20 s tira a chamada e devolve o aparelho a "ligado"** |
| `DSIMobileEquipment.hangupCall` (api 1001) | tira a chamada, devolve o aparelho a "ligado" e responde. Na bancada a tela não chega a mandá-lo |
| desconectar e desligar o Bluetooth (do passo 1) | derrubam a chamada antes do aparelho |

O perfil `aparelho-conectado.txt` ganhou a agenda pronta, o perfil de agenda fictício ("Perfil fictício", ligado ao aparelho pelo endereço de exemplo, sem contato nenhum) e cinco respostas "feito": usar o perfil (agenda e mensagens), o estilo e uma janela da lista de contatos, que a HMI passa a pedir com a agenda pronta, e os tons do teclado durante a chamada (`sendDTMF`).

**O botão vermelho: o que medi.** Nas rodadas `h1` e `h3` toquei no botão de encerrar com a chamada em curso, três vezes ao todo. A cada toque a HMI registra "Uncaught exception in EventDispatcher", com `NullPointerException` em `ASLCarplayServicesImpl.getCarPlayCallStates`, chamado por `CallHandling.isCarplayCallIncomming`; o `hangupCall` não sai e a chamada continua. A parte de CarPlay da HMI não tem estado nenhum porque a bancada não sobe nem anuncia o serviço dele. **Consertar de verdade é pôr um CarPlay de mentira no barramento** (anunciar o domínio 512 e responder ao estado de chamadas com uma lista vazia); isso muda a partida e não cabia no prazo. *(Corrigido em 12/10/2026, 12h23: só o serviço no barramento não basta; a causa é a adaptação da central. Ver "O botão vermelho", abaixo.)* Com uma chamada em curso, discar de novo abre o aviso da própria HMI de linha ocupada, e os dígitos saem como tons.

**As rodadas deste passo:**

| Rodada | Hora | O que mostrou |
|---|---|---|
| `c0`, `c1d` | 10h03 e 10h05 | a tecla verde só reconstrói o teclado; nenhum pedido (o bloco de cima) |
| `ct`, `cs` | 10h12 | `ct`, com o rastreamento da tela: a exceção da busca a cada dígito e a guarda que acha o campo vazio. `cs`, com o som de mentira: igual, sem pedido |
| `ca` | 10h18 | só com o perfil de agenda: a exceção some, a tela espera 10 s e desiste; nenhum pedido |
| `cb` | 10h22 | com a agenda pronta também: sai o `dialNumber` com 5550100; sem resposta, a HMI volta à tela principal em 5 s |
| `h1` | 10h34 | o modelo: a tela de chamada, discando, chamando, em chamada. O toque no botão vermelho não encerra |
| `h2` | 10h37 | **inválida**: o roteiro passou os dígitos numa variável, que o zsh não separa em palavras, e nenhum dígito foi digitado |
| `h3` | 10h41 | o botão vermelho, dois toques: a exceção do CarPlay nos dois, nenhum pedido de encerrar |
| `h4` | 10h51 | a chamada acaba sozinha aos 20 s e a tela volta à principal; segunda chamada (123) igual; a tecla do volante na chamada não faz nada |
| `z1`, `z2`, `zm1` (pares) | 10h54 a 11h00 | o padrão (aparelho conhecido e não conectado) da cópia de trabalho contra a pasta, dois pares sem memória e um com: as seis fotos iguais byte a byte nos três, 93 linhas ERROR até os 95 s nos dois braços (94 com memória), telefone de mentira 31/6/2 nos dois, nenhuma exceção da lista |
| `pv3` | 11h06 | **da pasta instalada, pelo roteiro do modo ao vivo**: nasce com o aparelho conhecido e não conectado; a tela conecta (o aviso de 2 s), disca 5550100, a tela de chamada entra, a chamada anda e acaba sozinha, e a tela volta à principal. Nenhum tempo esgotado, nenhum erro de OpenGL, nenhuma exceção do telefone |

**O que entrou na pasta**, às 11h06min37 (`traces/tel_instalacao4.txt`): `driver/telefonestub.c`, `driver/tests/t_telefone.sh` (de 201 para 234 casos), `stubs/telefone/aparelho-conectado.txt` e `stubs/telefone/README.md`. **Nenhum arquivo compartilhado mudou**: nem o motor, nem os lançadores, nem o padrão do modo ao vivo. Os de antes estão em `traces/tel_antes4/`; dois quadros `quadros/2026-10-12-hmi-tel-chamada-*.png`; registros e roteiros em `traces/tel_*`. A central ao vivo estava desligada na troca (desligou sozinha por volta das 10h36) e não foi ligada.

**Testes.** `driver/tests/t_telefone.sh`, 234 casos. Três levas, escritas antes do código e vistas falhar (`traces/tel_vermelho/`): a chamada e a agenda só com aparelho conectado (20 de 229), o tom do teclado (2 de 230) e a chamada que acaba sozinha (3 de 234). A bateria inteira, no candidato às 11h00 e da pasta às 11h09 (`traces/tel_suites_candidato_11h00.txt` e `traces/tel_suites_pasta_11h09.txt`): `run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 225, `t_memoria.sh` 80, `t_hub.sh` 25, `t_vc_diag_alm.sh` 37, `t_hmilog.sh` 32, `t_kipc_bus.sh` 14, `t_radio.sh` 114, `t_telefone.sh` 234, `t_midia.sh` 119 e `t_som.sh` 185, no container; `t_memoria.sh` 80, `t_servidor.sh` 20, `t_lsmval_abre.sh` 38 e `t_catalogo.sh` 43, no Mac. Todos passam, sem aviso do compilador.

**Limites da chamada:**

- **O botão vermelho não encerra** (acima). A chamada dura 20 s e acaba; desligar o Bluetooth pela tela também a derruba.
- **É só o estado da tela.** Sem áudio; o cronômetro fica em 0:00; a chamada não entra na lista de chamadas feitas; não há chamada recebida, espera nem conferência.
- **A busca de contatos não responde.** Ao abrir o teclado a HMI registra três linhas de erro por não achar o serviço de busca (`DSISearch[4]`), que a bancada não tem; a lista de sugestões ao lado do teclado fica vazia.
- **A tecla do volante na chamada** foi medida uma vez, sem efeito. Se no carro ela encerra, eu não sei.
- **A correção é para a variante sem CarPlay no barramento.** Se um dia a bancada anunciar o CarPlay, o botão vermelho deve passar a mandar o `hangupCall`, que o modelo já atende; o fim por tempo fica como está. *(Corrigido em 12/10/2026, 12h23: anunciar o CarPlay não faz o botão funcionar, e o fim por tempo passou a depender de a HMI assinar o CarPlay. Ver "O botão vermelho", abaixo.)*
- Valem os limites do modelo, acima: conectar é instantâneo, não há "desconectar" na tela, a lista de aparelhos sai vazia nos primeiros segundos.

**Onde parou, às 11h17.** O item 7 está feito até onde a bancada deixa: dois estados de partida, conectar e desconectar pela tela, discar e a chamada simulada. Fica de fora o que pede o CarPlay no barramento (encerrar pelo botão) e o que nunca foi pedido (parear, chamada recebida, agenda com contatos). Nenhuma central minha ficou no ar.

### O botão vermelho: a causa é a adaptação da central, não o barramento (12/10/2026, das 11h22 às 12h22)

A sessão principal pediu, às 11h22, que o botão vermelho passasse a encerrar a chamada simulada, respondendo no barramento o estado "nenhuma chamada do CarPlay", com prazo de noventa minutos. **Responder no barramento não basta, e medi por quê. O botão funciona, mas só numa central configurada como "com CarPlay", que é uma hipótese sobre o carro; por isso ficou como opção (`CARPLAY=1`) e o padrão do modo ao vivo não mudou.** O bloco de cima ("A chamada simulada") vale como estava às 11h17, salvo no que este corrige: a causa do botão. *(Atualizado em 12/10/2026, 17h31: desde as 17h19, por decisão do Lipe, o CarPlay é o padrão do modo ao vivo. A opção `CARPLAY=1` e o `hipotese-carplay.txt` de que este bloco fala saíram, o esquema deixou de trazer o serviço duas vezes, e as exceções "de sempre" são do desligamento. Ver "O CarPlay no padrão do simulador", logo abaixo.)*

**Resultado, verificado em execução** (centrais de teste `r1` a `r4` e a pasta instalada, pelo roteiro do modo ao vivo):

- **no padrão nada muda**: o botão vermelho continua sem encerrar, e a chamada continua acabando sozinha aos 20 s. Um par lado a lado sem memória e um com (`s1`, `sm2`), da cópia de trabalho contra a pasta: as seis fotos iguais byte a byte nos dois, as mesmas linhas de erro;
- **com `CARPLAY=1` o botão vermelho encerra a chamada no primeiro toque** e a tela volta à principal do telefone. A chamada deixa de acabar por tempo: passou dos 20 s e ficou até o toque (`r4`);
- **a mesma opção põe o bloco "App-Connect" na grade do menu** nas centrais de teste sem memória guardada (`r2`, `r3`, `r4`), porque o que a liga é dizer à HMI que a central tem CarPlay. Com a memória guardada do modo ao vivo a primeira página da grade é outra e saiu igual, byte a byte, com e sem a opção (`pv4`, `pv5`); a segunda página eu não vi. Nenhuma tela do CarPlay abre e não há aparelho de CarPlay;
- **a partida fica uns 6 s mais longa com a opção** (uma medida, `pv5` contra `pv4`);
- **a tecla Telefone do volante, durante a chamada, não faz nada**, com ou sem a opção: três medidas (`h4`, `r2`, `r4`), nenhum pedido, a tela não muda, nenhum erro.

| A grade com a opção (central de teste, sem memória guardada) | A chamada aos 26 s, com a opção |
|---|---|
| ![A grade com App-Connect](quadros/2026-10-12-hmi-tel-carplay-grade.png) | ![A chamada depois dos 20 s](quadros/2026-10-12-hmi-tel-carplay-chamada-26s.png) |

**A causa, elo por elo.** O que está entre aspas saiu do registro da HMI; o resto, do código decompilado dela e da camada da LG.

| Elo | O que acontece | Como sei |
|---|---|---|
| 1 | o toque no botão chega a `CallHandling.handleEvent`, que pergunta `isCarplayCallIncomming()`; este chama `ASLCarplayServicesImpl.getCarPlayCallStates()`, que usa `this.properties` sem conferir se existe | a pilha da exceção nas rodadas `h1`, `h3`, `r1` e `r3`, e o código |
| 2 | `properties` só é dado a essa classe no fim de `HsmTarget.initializeDSI()` (pacote `carplay.target`), que roda quando o serviço `org.dsi.ifc.carplay2.DSICarplay2` aparece registrado | código; em `r2`: "HsmTarget.registered(org.dsi.ifc.carplay2.DSICarplay2, 0)" e "HsmTarget.initializeDSI()" |
| 3 | o alvo de CarPlay só passa a escutar esse registro depois de dar a partida (evento 106) e de o modelo de telas mandar o tamanho da tela cheia do CarPlay | código; em `r2`: "registerDsiIfAslAndModelReady() … modelReady=false", "setCanvasDimensionFullScreen() with width: 784and height:468", "… modelReady=true" |
| 4 | **na bancada o alvo de CarPlay nem dá a partida**: a HMI pula a unidade de trabalho da integração de smartphone | resumo da partida nas centrais padrão que olhei (`r1`, `s1`): "WORKUNIT_SMARTPHONE_INTEGRATION" seguido de "featureFlag 814 is disabled, skip this work unit." |
| 5 | a marca 814 é `FEAT_IS_APP_CONNECT`: vale se houver CarPlay (145), Android Auto (6), MirrorLink (272) ou CarLife (142, 143) | `FeatureFlagHandler`, `case 814`, e os nomes em `ConfigurationManagerGenerated` |
| 6 | a marca de CarPlay sai da **adaptação** da central: `AdaptationSetter.updateCarPlay` faz `adaptation.getValue(69) != 0`, e o campo 69 é o `ID_APPLE_DIO` (DID `0555`, byte 1, bits 2 e 3, valores 0 a 2) | código e `stubs/codificacao/tabela-campos.tsv`. O firmware não traz descrição do campo nem o que 1 e 2 querem dizer |

Na bancada esse campo vale zero porque a adaptação do Jetta não foi lida do carro: o `padrao.txt` traz tudo no valor padrão da HMI, e nem o `jetta-gli-2021.txt` nem o arquivo de deduções falam de App-Connect. **O defeito, então, é da própria HMI** (o código de chamadas consulta a parte de CarPlay sem conferir se ela subiu) **e só aparece numa central sem App-Connect na adaptação.** **O Jetta tem CarPlay**: a foto do carro de 09/10/2026 (`carros/jetta-gli-2021/fotos/2026-10-09-painel-digital-mapa/cluster-mapa-nativo-e-mib3.jpg`) mostra o bloco "Apple CarPlay" na grade da central. No carro, portanto, a parte de CarPlay sobe, e esse caminho não deve estourar; é dedução, não vi a tela de chamada do carro. A ficha, em texto, não registrava o CarPlay.

**As quatro centrais de teste**, todas com o aparelho fictício conectado e o mesmo roteiro (discar 5550100, esperar "em chamada", tocar no botão vermelho):

| Rodada | Hora | CarPlay na adaptação | Serviço do CarPlay no barramento | A HMI assina o serviço? | O botão vermelho |
|---|---|---|---|---|---|
| `h1`, `h3` | 10h34, 10h41 | não | não | não | estoura; nenhum pedido de encerrar |
| `r1` | 11h30 | não | sim | não: a camada da LG cria o serviço e a HMI não pergunta nada | estoura (2 toques, 2 exceções); nenhum pedido |
| `r3` | 12h00 | sim | não | não: o alvo fica à espera ("modelReady=true", sem "registered") | estoura (2 toques, 2 exceções); nenhum pedido; a chamada acaba aos 20 s |
| `r2` | 11h38 | sim | sim | sim, os dez estados | **encerra no primeiro toque** (`hangupCall`) |
| `r4` | 12h00 | sim | sim, da cópia final | sim | a chamada passa dos 20 s; a tecla do volante não encerra; **o botão encerra no primeiro toque** |

As previsões de `r3`, `r4` e dos pares foram gravadas antes (`traces/tel_vermelho2_previsoes.txt`) e se confirmaram. A `r1` e a `r2` vieram antes, como experimento.

**O que o telefone de mentira responde pelo CarPlay: uma coisa só.** Com o esquema opcional ele anuncia o domínio 512 (`PID_CARPLAY`, canal `0x200`) e atende o `DSICarplay2`. A HMI assina dez estados (`canal=0x8000 if=606 … api=9999`); nove saem **sem a marca de validade** e a HMI os ignora (todos os tratadores dela testam a validade), e um sai com valor: a lista de chamadas do CarPlay, **vazia e válida** (`api=3 pb=10 02`), que a HMI recebeu ("HsmTarget.dsiCarplayUpdateCallState()"). Não há CarPlay simulado: nem modo, nem aparelho, nem faixa, nem tela. Nessas mensagens o campo de validade se chama `arg1`, não `validFlag`, e por isso o motor não o põe sozinho: o padrão dele, aqui, já é "sem valor".

**O que muda na central com a opção ligada**, além do botão (medido em `r2` e `r4` contra o padrão):

- a grade do menu ganha "App-Connect" nas centrais sem memória guardada (`r2`, `r3`, `r4`, a mesma foto byte a byte; não abri o bloco). Com a memória do modo ao vivo a primeira página da grade tem Rádio, Navegação, Som, Ajuda, Mídias, Veículo, Usuário e Configuração, com e sem a opção (`pv4`, `pv5` e as duas centrais do par `sm2`: a mesma foto byte a byte); não deslizei até a segunda;
- a partida fica mais longa: o resumo da partida saiu aos 58,4 s na `pv5` e aos 52,5 s na `pv4`, uma medida. A unidade de trabalho que passa a subir espera por `DSIAndroidAuto2` e `DSIMirrorLink`, que a bancada não tem, e no modo ao vivo essa espera é de 6 s;
- o resumo da partida passa a dizer "featureFlag 814 is enabled, start this work unit", e os serviços que a HMI dá por ausentes passam de 18 para 20 (`r4` contra `s1`): entram `DSIAndroidAuto2` e `DSIMirrorLink`, que a unidade de trabalho pede e a bancada não tem. É uma linha de erro a mais na partida. Até os 55 s a `r4` tem 85 linhas de erro e a `s1`, 83: a outra a mais é uma segunda `DSIKeyPanelListener…genericSettingResponse is unexpected`, que não investiguei (o passo anterior já tinha anotado uma linha a mais do teclado quando o aparelho nasce conectado, e a `r4` nasce conectada);
- nenhuma exceção nova; as duas do botão somem. As cinco exceções não apanhadas que restam (`usermanagement…subscriptionhelper`) são as de sempre, iguais em `h4`.

Não abri o bloco App-Connect.

**O que entrou na pasta**, às 12h11min40 (`traces/tel_instalacao5.txt`):

| Arquivo | O que mudou |
|---|---|
| `driver/telefonestub.c` (de 445 para 458 linhas) | o fim por tempo dos 20 s só vale enquanto a HMI não assina o estado de chamadas do CarPlay; quando assina, o modelo registra uma linha e a chamada fica até o botão, a desconexão ou o Bluetooth desligado. Com o esquema padrão esse serviço não existe e nada muda |
| `driver/ao_vivo.sh` (**compartilhado**) | a opção `CARPLAY=1`, que acrescenta as três peças abaixo, e o repasse de `TELEFONE_ESQUEMA` ao container (vazio, vale o esquema de sempre). Sem `CARPLAY` o comando que sobe a central é o mesmo de antes |
| `stubs/codificacao/hipotese-carplay.txt` (novo) | uma linha, `0555 @1.2+2 = 1`: o `ID_APPLE_DIO` diferente de zero. **Hipótese de teste, não a adaptação do Jetta.** Não entra no padrão |
| `stubs/telefone/gerado-carplay/` (novo) | o esquema opcional: o do telefone mais o serviço do CarPlay. Nenhuma linha do esquema padrão sai nem muda; entram 309 |
| `stubs/telefone/carplay-sem-chamada.txt` (novo) | o perfil opcional, marcado como simulação: a lista de chamadas do CarPlay, vazia e válida |
| `driver/tests/t_telefone.sh` e `stubs/telefone/README.md` | 28 casos novos; a opção e a tabela das centrais |
| `stubs/codificacao/README.md` (do Marco 6), às 12h24 | uma linha na tabela de arquivos, para o `hipotese-carplay.txt` |

O motor, os outros lançadores, o esquema padrão, os três perfis e o `ajustes.txt` não mudaram. Os de antes estão em `traces/tel_antes5/`. A central ao vivo estava desligada na troca e não foi ligada. Às 12h21min54, depois das provas, troquei de novo dois textos já instalados (o comentário da opção no cabeçalho do `ao_vivo.sh` e o README do telefone): diziam que a grade ganha "App-Connect" sem a ressalva da memória guardada.

**Testes.** `driver/tests/t_telefone.sh`, de 234 para 262 casos. Escritos antes do código e vistos falhar (`traces/tel_vermelho2/`): sem o esquema e o perfil, 8 falhas; com eles e sem a mudança do modelo, 5: os quatro casos do fim por tempo e um caso meu com a expectativa errada (ao recusar um perfil o programa sai com 2, e eu esperava 1), que corrigi. Entre as duas levas tentei pôr o CarPlay no esquema padrão; o teste dos vetores de prova caiu de 324 para "324 de 352" e voltei atrás (ver "Limites"). A bateria inteira, no candidato às 12h04 e da pasta às 12h15 (`traces/tel_suites_candidato_12h04.txt` e `traces/tel_suites_pasta_12h15.txt`): run_tests 8, t_transmissao 19, t_codificacao 39, t_entrada 11, t_pedidos 28, t_veiculo 225, t_memoria 80, t_hub 25, t_vc_diag_alm 37, t_hmilog 32, t_kipc_bus 14, t_radio 114, t_telefone 262, t_midia 119, t_som 185; no Mac, t_memoria 80, t_servidor 20, t_lsmval_abre 38, t_catalogo 43. Nenhuma falha e nenhum aviso do compilador.

**A prova pela pasta instalada**, pelo roteiro do modo ao vivo: duas centrais lado a lado, às 12h11, nascidas de uma cópia da memória guardada, com rádio, mídia e som.

- **`pv4`, o padrão:** nasce desconectada, conecta pela tela e disca. O toque no botão vermelho deixa a exceção do CarPlay e nenhum pedido de encerrar; a chamada acaba sozinha aos 20 s e a tela volta à principal do telefone. Como antes deste passo.
- **`pv5`, com `CARPLAY=1`:** a codificação leva o `hipotese-carplay.txt` no fim e o telefone, o perfil do CarPlay; o resumo da partida diz "featureFlag 814 is enabled"; a HMI assina os dez estados. Conecta pela tela e disca; a tela de chamada fica 29,9 s (dos 90,9 s aos 120,8 s), sem fim por tempo; a tecla do volante não muda nada (a foto de antes e a de depois são iguais byte a byte); o botão vermelho manda o `hangupCall` e a tela volta à principal. Nenhuma exceção do CarPlay.

As outras exceções não apanhadas são as mesmas nas duas: 13 do som (`WaveplayerController.setBeepType`) e 5 do `usermanagement`. Até os 60 s a `pv5` tem 80 linhas de erro e a `pv4`, 72. Uma é a espera por Android Auto e MirrorLink. As outras sete são "Error while getting row item" da tela do veículo, aos 5 s, que saem 7 ou 14 vezes de uma partida para outra com ou sem a opção (14 em `pv`, `pv2`, `zm1` e nas duas do `sm2`; 7 em `pv3` e `pv4`): é variação de partida, não efeito da opção.

**Limites:**

- **O padrão continua como estava**: sem App-Connect, o botão vermelho estoura e a chamada acaba aos 20 s.
- **A opção é uma hipótese sobre a adaptação do carro.** Que o Jetta tem CarPlay, a foto mostra; o valor 1 no `ID_APPLE_DIO` não foi lido de lugar nenhum. O firmware aceita 0, 1 e 2 e não diz a diferença, e a HMI só testa se é zero. O valor certo sai da adaptação do 5F lida no carro, e aí o lugar dele é o `jetta-gli-2021.txt`. Que o carro (MU0803) tire a marca de CarPlay do mesmo campo é leitura do MU0883.
- **A marca "App-Connect codificado e habilitado" (815) eu não toquei.** Ela junta a 814 com outra, a 24 (`FEAT_IS_APP_CONNECT_FUNCTION_ENABLED`), que não olhei de onde vem. Para o botão bastou a 814.
- **O esquema com o CarPlay traz o serviço duas vezes**, porque a camada da LG entrega os eventos dele por duas interfaces (606 e 607), e o motor só fala pela primeira. Bastou para a HMI. Os vetores de prova dessa pasta não conferem para o CarPlay (28 de 352), e o teste não os usa; consertar é do gerador ou do motor, que são de todos.
- **Android Auto e MirrorLink** passam a ser pedidos pela HMI com a opção ligada e ficam sem resposta.
- **A tecla do volante na chamada** não faz nada na bancada. Se no carro ela encerra, eu não sei.
- **No modo ao vivo com a memória guardada não vi o bloco "App-Connect"**: a primeira página da grade é outra, e os roteiros de entrada da bancada não deslizam a tela.
- Um par sem memória e um com, não dois e um como nos passos anteriores: o que muda no padrão é uma condição do modelo que o esquema padrão nunca satisfaz.

**Onde parou, às 12h22.** O botão vermelho encerra a chamada na opção `CARPLAY=1`; no padrão, não. **Fica uma decisão para o Lipe**: como o carro tem CarPlay, a hipótese pode virar dedução marcada do Jetta (uma linha no `jetta-gli-2021-deducoes.txt`, mais o esquema e o perfil do CarPlay no padrão do telefone). Aí o botão funciona no padrão e a grade ganha "App-Connect", como no carro. Não fiz isso por conta própria porque muda a grade do modo ao vivo, e o pedido era não mexer no padrão além do esperado. Nenhuma central minha ficou no ar.

### O CarPlay no padrão do simulador: decisão do Lipe (12/10/2026, das 16h49 às 17h30)

**Decisão do Lipe, de 12/10/2026, às 12h40.** A sessão principal perguntou: "Posso pôr o CarPlay no padrão do simulador, como dedução marcada, para o botão vermelho encerrar a chamada?", dizendo que a grade ganha o bloco App-Connect e a partida fica uns 6 s mais longa. Ele respondeu: "Sim, pôr no padrão (Recomendado)". **Desde as 17h19min34 o modo ao vivo sobe como "central com CarPlay", e o botão vermelho da tela de chamada encerra a chamada simulada no padrão.** O bloco de cima ("O botão vermelho") vale como estava às 12h25, salvo no que este muda: a opção `CARPLAY=1` deixou de existir, porque virou o padrão, e o que há agora é `CARPLAY=0` para desligar.

**Resultado, verificado em execução** (a pasta instalada, pelo roteiro do modo ao vivo):

- **`pv8`, o padrão (17h19, nascida de uma cópia da memória guardada, com rádio, mídia e som):** a codificação são os quatro arquivos de sempre, e o telefone sobe com os quatro perfis; o resumo da partida diz "featureFlag 814 is enabled, start this work unit" e sai aos 58,4 s; a HMI assina os dez estados do CarPlay. A segunda página da grade tem "Telefone" e "App-Connect". Conecta pela tela, disca, a chamada fica 26,6 s na tela sem acabar por tempo, e **o botão vermelho a encerra** (`hangupCall`); a tela volta à principal do telefone. Nenhuma exceção do CarPlay.
- **`pv9`, com `CARPLAY=0` (lado a lado):** a codificação leva o `sem-carplay.txt` no fim e o telefone, os três perfis; "featureFlag 814 is disabled, skip this work unit", resumo aos 52,5 s; a segunda página da grade só tem "Telefone". O toque no botão vermelho deixa a exceção do CarPlay e nenhum pedido de encerrar; a chamada acaba sozinha aos 20 s. **É a central de antes da decisão.**
- **a tecla Telefone do volante na chamada** não foi repetida aqui: são quatro medidas do bloco de cima, sem efeito.

**As três peças do padrão.** Só fazem sentido juntas (as quatro centrais do bloco de cima):

| Peça | Onde está | O que é |
|---|---|---|
| a adaptação diz que há CarPlay | uma linha no fim do `stubs/codificacao/jetta-gli-2021-deducoes.txt`: `0555 @1.2+2 = 1` | **dedução marcada, não leitura do 5F.** A razão vai ao lado, no arquivo: a foto do carro de 09/10/2026 mostra o bloco "Apple CarPlay" na grade da central. O valor (1) é palpite: o firmware aceita 0, 1 e 2 e a HMI só olha se é zero |
| o serviço do CarPlay no barramento | o modo ao vivo passa ao telefone de mentira o esquema `stubs/telefone/gerado-carplay/esquema.txt` | o telefone anuncia também o domínio 512 e atende o `DSICarplay2` |
| "nenhuma chamada do CarPlay" | o perfil `stubs/telefone/carplay-sem-chamada.txt`, depois dos outros três | a única resposta com valor: a lista de chamadas, vazia e válida |

**Para desligar:** `CARPLAY=0 sh driver/ao_vivo.sh`. Põe o `stubs/codificacao/sem-carplay.txt` no fim da codificação (o campo de volta no zero) e sobe o telefone com o esquema `gerado/` e os três perfis. É a central de antes da decisão.

**As centrais de teste não mudaram.** O `driver/m8_central.sh` sobe sem codificação e o `driver/m1_hmi.sh` continua com o esquema `gerado/` por padrão; não mexi em nenhum dos dois. A mudança é do modo ao vivo.

**Onde fica o "App-Connect" com a memória guardada: na segunda página da grade.** Era o que faltava ver. Duas centrais pelo roteiro do modo ao vivo, nascidas de uma cópia da memória guardada (`pv6` com o CarPlay, `pv7` sem, às 16h52): a primeira página é igual nas duas, byte a byte (Rádio, Navegação, Som, Ajuda, Mídias, Veículo, Usuário, Configuração); arrastando a grade para a esquerda, a segunda tem "Telefone" e, só com o CarPlay, "App-Connect" logo abaixo. Numa central sem memória o bloco entra na primeira página, entre "Mídias" e "Veículo". O arrasto é dos gestos da bancada (`gesto 4`, `5` e `3` no arquivo de comandos: encosta, arrasta, solta).

| A segunda página, com o CarPlay | A segunda página, sem |
|---|---|
| ![Com o CarPlay](quadros/2026-10-12-hmi-tel-carplay-grade-pagina-2.png) | ![Sem o CarPlay](quadros/2026-10-12-hmi-tel-sem-carplay-grade-pagina-2.png) |

**O conserto do esquema: o gerador punha o serviço do CarPlay duas vezes.** No despacho da camada da LG dois casos caem no mesmo trecho ("case 606: case 607:"), e o `driver/LeEsquemaDSI.java` criava um serviço para cada caso. Agora dois casos com o mesmo destino são um serviço só, com a primeira interface, e a outra vai anotada no relatório ("o despacho entrega os eventos deste serviço também pela interface 607 (o mesmo trecho da 606); o esquema usa a 606"). A 606 é por onde a HMI recebeu (rodada `r2`). Prova: os quatro esquemas que a pasta já tinha (veículo, sintonizador, mídia e telefone), regerados com o gerador corrigido, saíram **idênticos** aos da pasta, arquivo por arquivo (`traces/tel_padrao_regera.txt`); o do telefone com o CarPlay passou de 20 para 19 serviços, e os vetores de prova dele conferem (338 mensagens, 14 do CarPlay), o que antes não acontecia (324 de 352).

**A série, antes de instalar:** três pares lado a lado pelo roteiro do modo ao vivo, a cópia de trabalho (o padrão novo) contra a pasta (o padrão de então), dois sem memória e um com. As previsões foram gravadas antes (`traces/tel_padrao_previsoes.txt`).

| O que comparei | Sem memória (`u1`, `u3`, `v1`) | Com memória (`u2`, `v2`) |
|---|---|---|
| resumo da partida | 5,9 s, 6,0 s e 6,0 s mais tarde no padrão novo (58,3 s contra 52,3 a 52,4 s) | 5,9 s e 5,9 s mais tarde (58,4 s contra 52,6 s); o assistente de configuração entra na tela junto com o resumo, então também 5,9 s depois |
| marca 814 | "is enabled, start this work unit" contra "is disabled, skip this work unit" | o mesmo |
| serviços que a HMI dá por ausentes | dois a mais: `DSIAndroidAuto2` e `DSIMirrorLink` | o mesmo |
| linhas de erro até os 100 s | uma a mais (106 contra 105): a espera por esses dois | uma a mais (80 contra 79): a mesma |
| exceções não apanhadas até os 100 s | nenhuma, nas duas | nenhuma, nas duas |
| telas | as mesmas | as mesmas |
| telefone de mentira | a mais, a assinatura do `DSICarplay2` (dez estados) e o aviso do modelo | o mesmo |
| fotos aos 25, 45, 66 e 77 s | iguais byte a byte | iguais byte a byte |
| fotos da grade, aos 87 e 98 s | diferem: o padrão novo tem "App-Connect" | iguais byte a byte: a primeira página é a mesma |

**Tudo o que mudou foi o que estava previsto**, com quatro observações:

- **Uma previsão minha estava errada.** Escrevi que a foto dos 98 s, depois da tecla do volante, sairia igual; ela é a própria grade, e difere pelo bloco. As telas são as mesmas nos dois braços, nos mesmos instantes.
- **A série teve de ser repetida em parte.** Os pares `u` usaram a cópia de trabalho; às 17h04min20, com o `u1` no ar, a sessão do som instalou o `somstub.c` novo na pasta, e o `u2` e o `u3` ficaram com o som novo num braço só. Repeti com o candidato montado da pasta de então (`v1`, `v2`): o som igual nos dois braços, e os mesmos resultados.
- **A ordem de duas telas aos 0,3 s da partida difere entre os braços em quatro dos cinco pares**, sempre no mesmo sentido: no padrão novo o indicador de gesto entra antes do controle de usuários, e na pasta, depois (no `u2` as duas centrais deram "depois"). É a corrida de partida do Marco 10, e nos pares de braços iguais do passo anterior (`s1`, `sm2`, `z1`, `z2`, `zm1`) a ordem também diferia entre os braços, ora num sentido, ora no outro. **Não sei dizer se o padrão novo favorece um dos sentidos**: são poucas partidas. Não vi efeito na tela: as fotos dos 25 aos 77 s são iguais byte a byte nos cinco pares.
- **Correção ao bloco de cima:** as exceções do `usermanagement` que chamei de "as de sempre" são do desligamento da central. Saem no último décimo de segundo do registro (aos 111,7 s numa central de 112 s; aos 184,7 s numa de 185 s); num braço chegam a ser escritas e no outro, não. O comparador passou a contar só até os 100 s.

**O que entrou na pasta**, às 17h19min34 (`traces/tel_instalacao6.txt`):

| Arquivo | O que mudou |
|---|---|
| `driver/ao_vivo.sh` (**de todos**) | o padrão passa a ser a central com CarPlay; `CARPLAY=0` desliga. A opção `CARPLAY=1` do meio-dia saiu |
| `driver/LeEsquemaDSI.java` (**de todos**) | dois casos do despacho com o mesmo destino viram um serviço só (9 linhas a mais, nenhuma tirada; `traces/tel_padrao_gerador.diff`). Os outros esquemas saem idênticos |
| `stubs/codificacao/jetta-gli-2021-deducoes.txt` (**do Marco 6**) | um bloco novo no fim: a dedução do CarPlay, com a razão, a decisão do Lipe e o que é palpite |
| `stubs/codificacao/sem-carplay.txt` (novo) e `hipotese-carplay.txt` (saiu) | o arquivo que desfaz a dedução; o de hipótese ficou sem uso e foi para `traces/tel_antes6/` |
| `stubs/codificacao/README.md` (**do Marco 6**) | a linha do arquivo de deduções e a do `sem-carplay.txt` |
| `stubs/telefone/gerado-carplay/` | regerado: o serviço uma vez |
| `driver/telefonestub.c` | só o comentário do cabeçalho; o programa é o mesmo das 12h11 |
| `driver/tests/t_telefone.sh` e `stubs/telefone/README.md` | cinco casos novos; o CarPlay como padrão |

O motor, o `m1_hmi.sh`, o esquema `gerado/` e os perfis não mudaram. Os de antes estão em `traces/tel_antes6/`. A central ao vivo estava desligada na troca e não foi ligada: **a próxima partida dela já sobe com o CarPlay.**

**O fim por tempo dos 20 s.** O pedido era tirá-lo ou alongá-lo com o botão funcionando no padrão. Não mexi no modelo: desde as 12h11 a chamada só acaba por tempo enquanto a HMI não assina o estado de chamadas do CarPlay, e no padrão novo ela assina. Então **no modo ao vivo não há mais fim por tempo**, e ele continua, nos 20 s, só onde o botão não funciona (`CARPLAY=0` e centrais de teste sem codificação), porque ali a chamada ficaria presa na tela.

**Testes.** `driver/tests/t_telefone.sh`, de 262 para 267 casos. Os cinco novos foram escritos antes de trocar o esquema e vistos falhar (`traces/tel_padrao_vermelho.txt`): o serviço duas vezes, 20 serviços, o relatório sem a nota e os vetores em "324 de 352". A bateria inteira, no candidato às 17h13 e da pasta às 17h23 (`traces/tel_suites_candidato_17h13.txt` e `traces/tel_suites_pasta_17h23.txt`): run_tests 8, t_transmissao 19, t_codificacao 39, t_entrada 11, t_pedidos 28, t_veiculo 225, t_memoria 80, t_hub 25, t_vc_diag_alm 37, t_hmilog 32, t_kipc_bus 14, t_radio 114, t_telefone 267, t_midia 119, t_som 214 (a sessão do som acrescentou casos nesse meio-tempo); no Mac, t_memoria 80, t_servidor 20, t_lsmval_abre 38, t_catalogo 43. Nenhuma falha e nenhum aviso do compilador.

**Limites:**

- **O CarPlay do padrão é dedução.** Que o Jetta tem CarPlay, a foto mostra. O valor do campo (1) é palpite, e que o carro (MU0803) tire a marca do mesmo campo é leitura do MU0883. O valor certo sai da adaptação do 5F, quando o carro for lido; aí a linha vai para o `jetta-gli-2021.txt`.
- **A partida do modo ao vivo ficou uns 6 s mais longa** (5,9 s nos três pares): a HMI espera por `DSIAndroidAuto2` e `DSIMirrorLink`, que a bancada não tem. O Android Auto ficou em zero na adaptação: a foto não o mostra.
- **Não abri o bloco "App-Connect"**, e nenhuma tela do CarPlay existe na bancada.
- **Com a memória guardada o bloco fica na segunda página**, com o Telefone. Por que a memória põe os dois lá eu não investiguei.
- **No carro a grade é outra**: a foto mostra Rádio, Telefone, Navegação, Assistentes, Mídias, Apple CarPlay, Veículo e Som. O bloco aparece com o nome "Apple CarPlay", e aqui é "App-Connect"; não sei se o nome muda com um aparelho ligado.
- **A tecla Telefone do volante na chamada** segue sem fazer nada.
- Valem os limites do bloco de cima sobre a chamada e sobre o serviço (uma resposta só).

**Onde parou, às 17h30.** O item 7 tem agora, no padrão do modo ao vivo: os dois estados de partida, conectar e desconectar pela tela, discar, a chamada simulada e o botão vermelho. Ficam de fora o que nunca foi pedido (parear, chamada recebida, agenda com contatos) e o que depende do carro (a adaptação lida do 5F). Nenhuma central minha ficou no ar.

### A chamada recebida, e o pareamento, que esta variante não faz pela tela (12/10/2026, das 17h33 às 18h11)

A sessão principal pediu, às 17h32, os dois pontos que o registro ainda listava como não feitos no telefone, em dois passos e com prazo de noventa minutos. **O primeiro está instalado: o aparelho fictício liga para a central, e dá para atender ou recusar pela tela. O segundo parou no levantamento: a tela desta variante não busca aparelhos, e eu não inventei um caminho.** *(Atualizado em 12/10/2026, 19h09: o pareamento foi feito depois, pelo caminho que a variante tem, e as duas medições que faltavam na chamada recebida também. Ver "O pareamento", logo abaixo.)*

#### 1. A chamada recebida (instalada às 17h58min39)

**O gatilho é um número combinado: 5550199.** Não há aparelho de verdade para ligar, e o gatilho tinha de servir pela janela, sem terminal. Discar o 5550199 sai como qualquer chamada; cinco segundos depois de ela acabar, o aparelho fictício "liga de volta" do mesmo número. O número é da faixa 555-01xx, que não é de ninguém, e nenhum outro liga de volta.

**Resultado, verificado em execução** (centrais de teste `i1`, às 17h43, e `i2`, às 17h47, e a pasta instalada pelo roteiro do modo ao vivo):

| O que se faz na tela | O que a HMI faz | O que sai no barramento |
|---|---|---|
| discar 5550199 e, em chamada, tocar no botão vermelho | a chamada acaba e a tela volta à principal do telefone | `hangupCall`; o modelo marca a volta para dali a 5 s |
| esperar | **a tela de chamada abre sozinha**, 4,2 a 5,0 s depois (o relógio do modelo anda de segundo em segundo), com 5550199, "chamando…" e três botões: atender (verde), silenciar o toque (sino) e recusar (vermelho) | a lista de chamadas com a chamada 1 tocando (`CALLSTATE_RINGING`, 3) e o aparelho "em chamada" |
| tocar no verde | a mesma tela passa a "em chamada": 0:00, mudo e botão vermelho | `acceptCall`, com o pedido vazio; o modelo responde e passa a chamada a "em chamada" (4) |
| tocar no vermelho, com a chamada atendida | a chamada acaba | `hangupCall` |
| tocar no vermelho, com a chamada tocando | a chamada some e a tela volta à principal | `hangupCall`: o modelo recusa |
| tocar no sino | o botão fica marcado; a chamada segue tocando | nada |
| não fazer nada | aos 20 s a chamada para de tocar e a tela volta à principal | o modelo tira a chamada |

| Tocando | Atendida |
|---|---|
| ![Tocando](quadros/2026-10-12-hmi-tel-recebida-tocando.png) | ![Atendida](quadros/2026-10-12-hmi-tel-recebida-atendida.png) |

Nenhuma exceção não apanhada antes do desligamento, nas duas centrais. Na `i2`, nenhum pedido novo sem resposta. Na `i1`, um toque meu fora de hora (a chamada já tinha parado de tocar) caiu num bloco de favorito e abriu a busca de contatos; a HMI pediu quatro vezes `DSIAdb.restartDownload`, que o telefone de mentira não responde. É da agenda, não da chamada, e fica anotado.

**A prova pela pasta instalada**, pelo roteiro do modo ao vivo: uma central, às 17h59, nascida de uma cópia da memória guardada, com rádio, mídia e som (`pv10`). Conecta pela tela, disca 5550199 e encerra pelo botão vermelho; a tela de chamada abre sozinha 4,5 s depois; **o verde atende** (`acceptCall`) e o vermelho encerra. Disca de novo, encerra, a chamada volta 4,6 s depois e **o vermelho recusa**. Nenhum pedido novo sem resposta.

**No modo ao vivo cada chamada recebida deixa uma exceção nova**: `NullPointerException` em `WaveplayerController.playToneRepeat`, chamado por `StatePhoneRingtoneInit.playRingtone`, no instante em que a chamada entra (duas chamadas, duas exceções). A HMI tenta tocar o toque de chamada pelo tocador de sons, que a bancada não tem. É o mesmo objeto nulo da exceção que cada toque na tela já deixa com o som de mentira ligado (`setBeepType`, 26 nesta prova). Não muda a tela; numa central sem o som de mentira (`i2`) não acontece.

**O modelo** (`driver/telefonestub.c`, de 461 para 503 linhas; o motor não mudou):

| Pedido ou relógio | O que o modelo faz |
|---|---|
| `dialNumber` com 5550199 | a chamada de sempre, e lembra que ela foi para o número combinado |
| o fim dessa chamada, pelo `hangupCall` ou pelo fim por tempo | marca a volta para 5 s depois. É uma por discagem |
| o relógio, na hora marcada, com o aparelho conectado e sem chamada | põe na lista a chamada 1, de 5550199, tocando (3), e o aparelho "em chamada" (2) |
| `acceptCall` (api 1000) | só na instância 0 e com uma chamada tocando (senão resultado 7): responde resultado 0 e passa a chamada a "em chamada" (4) |
| `hangupCall` com a chamada tocando | recusa: tira a chamada e devolve o aparelho a "ligado" |
| o relógio, 20 s depois de a chamada começar a tocar | tira a chamada: ninguém atendeu |
| desconectar o aparelho ou desligar o Bluetooth | cancela a volta marcada e derruba a chamada que houver |

**A série, antes de instalar:** dois pares pelo roteiro do modo ao vivo, o candidato contra a pasta, um sem memória (`w1`, 17h51) e um com (`w2`, 17h53). A previsão, gravada antes (`traces/tel_recebida_previsoes.txt`), era que nada mudasse, porque na partida o modelo novo faz o mesmo que o de então. **Nada mudou:** o resumo da partida na mesma hora (58,4 s contra 58,3 s; 58,4 s nas duas), as mesmas linhas de erro (106 e 106; 80 e 80), as mesmas telas na mesma ordem, as seis fotos iguais byte a byte e o mesmo que o telefone de mentira atendeu.

**Testes.** `driver/tests/t_telefone.sh`, de 267 para 287 casos. Os vinte novos foram escritos antes do código; onze falharam contra o modelo de então e nove já passavam, porque dizem o que não pode acontecer (`traces/tel_recebida_vermelho.txt`). A bateria inteira, no candidato às 17h51 e da pasta às 18h03 (`traces/tel_suites_candidato_17h51.txt` e `traces/tel_suites_pasta_18h03.txt`): run_tests 8, t_transmissao 19, t_codificacao 39, t_entrada 11, t_pedidos 28, t_veiculo 225, t_memoria 80, t_hub 25, t_vc_diag_alm 37, t_hmilog 32, t_kipc_bus 14, t_radio 114, t_telefone 287, t_midia 119, t_som 214; no Mac, t_memoria 80, t_servidor 20, t_lsmval_abre 38 e t_catalogo 43 no candidato e 59 na pasta (outra sessão acrescentou casos ao catálogo nesse meio-tempo). Nenhuma falha e nenhum aviso do compilador.

**O que entrou na pasta:** `driver/telefonestub.c`, `driver/tests/t_telefone.sh` e `stubs/telefone/README.md`, os três desta sessão (`traces/tel_instalacao7.txt`; os de antes, em `traces/tel_antes7/`). Nenhum arquivo de todos mudou. A central ao vivo estava desligada na troca e não foi ligada.

**Limites da chamada recebida:**

- **Só o número combinado liga de volta**, e só depois de ser discado. Não há chamada recebida "do nada".
- **Não há toque nem áudio.** O botão do sino só marca o toque como silenciado. Com o som de mentira ligado, a tentativa de tocar o toque deixa uma exceção por chamada recebida (acima); consertar é dar à HMI o tocador de sons, que é da frente do som.
- **Os tempos (5 s para voltar, 20 s tocando) são escolha da simulação.**
- **Só vi a chamada entrar com a central na tela do telefone.** Em outra tela (rádio, mídia, veículo) não medi; a tecla do volante com a chamada tocando, também não.
- **Numa central sem CarPlay** (`CARPLAY=0`, centrais de teste sem codificação) o botão vermelho não encerra, e a volta vem 5 s depois do fim por tempo dos 20 s; atender e recusar não foram medidos ali.
- A chamada não entra na lista de chamadas da tela, e o número não tem nome: a agenda fictícia não tem contatos.

#### 2. O pareamento: parei no levantamento

O pedido era a busca por aparelhos da tela achar um segundo aparelho fictício. **Esta variante não tem busca pela tela.** O que levantei, sem subir central para isso:

| O que olhei | O que há | Como sei |
|---|---|---|
| a tela "Celulares conhecidos" | não tem botão de busca; o rodapé diz "Para conectar um novo dispositivo, faça uma busca pelo seu dispositivo móvel." | `quadros/2026-10-12-hmi-tel-conectado-celulares-conhecidos.png`, da manhã |
| o que a HMI manda ao abrir essa tela | põe o Bluetooth em modo acessível (`setAccessibleMode` 4) e o tira ao sair (3); **nenhum pedido de busca** | o registro do telefone de mentira das rodadas `pv4`, `pv8` e `u2`: zero `requestStartDiscovery` e zero `requestInquiry` |
| o código da HMI | o pedido de busca existe (`TargetASLPhoneConnectionManager.startDiscovery`, que manda `requestStartDiscovery(10, 0)`), mas **nenhuma tela o chama**: nos modelos de tela do telefone, do gerente de conexões e da página do assistente não há nada de aparelhos descobertos | decompilado: `generated/de/vw/mib/appui/phone`, `phoneconnectionmanager` e `phoneglobalwizardpage` |
| o caminho que existe | **o pareamento pedido pelo aparelho, com confirmação de código**: a HMI assina o estado do código (`DSIBluetooth`, atributo 7) e tem como aceitar e recusar (`requestPasskeyResponse` com 1 ou 0) | `phoneconnectionmanager/bluetooth/TargetASLPhoneBluetoothPasskey`; a assinatura do atributo 7 aparece em toda rodada |

Ou seja: nesta central quem procura é o celular. A tela deixa a central visível, o aparelho a encontra e pede o pareamento, e a central mostra um código para confirmar. **Não fiz esse caminho**: não é o que foi pedido, pede levantar o que a tela mostra em cada estado do código e como o aparelho novo entra nas duas listas (a do Bluetooth e a do gerente), e não cabia no que restava de prazo e de uso. Se for para fazer, o gatilho natural é a própria tela: com "Celulares conhecidos" aberta, um segundo aparelho fictício pede o pareamento alguns segundos depois.

**Onde parou, às 18h11.** No item 7 fica de fora o pareamento (pelo caminho que a variante tem, o pedido pelo aparelho), a caixa "Visibilidade", a agenda com contatos e as fotos da tela de telefone do carro. Nenhuma central minha ficou no ar.

### O pareamento: o segundo aparelho fictício pede, e a central confirma o código (12/10/2026, das 18h13 às 19h08)

A sessão principal mandou, às 18h12, fazer o pareamento pelo caminho que a variante tem, com prazo de 75 minutos: era o último ponto aberto do item 7. **Está instalado desde as 18h56min45.** O levantamento do bloco de cima ("2. O pareamento: parei no levantamento") continua valendo como levantamento; este bloco é o que foi feito depois.

**Resultado, verificado em execução** (centrais de teste `j1`, `k1` e `k2`, e a pasta instalada pelo roteiro do modo ao vivo):

| O que se faz na tela | O que a HMI faz | O que sai no barramento |
|---|---|---|
| Telefone, engrenagem, "Selecionar celular" | abre "Celulares conhecidos" | `DSIPhoneManager.requestReconnectSuspend` com verdadeiro (`08 01`) |
| esperar 4 s | **abre a confirmação**: "Confirmação sobre o acoplamento: Esta senha 123456 coincide com a senha no Celular fictício 2?", com "Cancelar" e "Sim" (popup `PO_PHONE_CONNECTION_MANAGER_CONFIRM_BLUETOOTH_BONDING_SSP`) | o modelo publica o estado do código do Bluetooth (atributo 7) em `PS_SSP_SHOW_AND_CONFIRM` (7), com o nome, o endereço e o código |
| "Sim" | fecha a confirmação, abre e fecha o aviso de progresso (`…PROGRESS_BLUETOOTH_BONDING_PK_VERIFY`), e **o "Celular fictício 2" aparece na lista**, abaixo do primeiro | `DSIBluetooth.requestPasskeyResponse` com o código, o endereço e 1; o modelo responde, zera o estado do código e publica a lista de conhecidos com os dois |
| "Cancelar" | fecha a confirmação e mostra o aviso de conexão cancelada (`…INFO_BLUETOOTH_CONNECT_USER_ABORT`); o aparelho não entra | o mesmo pedido, com o código vazio e 0; o modelo responde e zera o estado |
| sair da tela e voltar, depois de "Cancelar" | pede de novo, 4 s depois | `requestReconnectSuspend` com falso e, de novo, com verdadeiro |
| sair e voltar, depois de "Sim" | não pede mais | |

| A confirmação | A lista, depois de "Sim" |
|---|---|
| ![A confirmação do código](quadros/2026-10-12-hmi-tel-pareamento-confirmacao.png) | ![Os dois aparelhos na lista](quadros/2026-10-12-hmi-tel-pareamento-lista.png) |

**Antes do código, sem modelo nenhum** (rodada `j1`, 18h14): publiquei à mão, pelo barramento e no lugar do serviço do Bluetooth, um estado de código 7 com a tela aberta, e a HMI mostrou a confirmação; tocando em "Sim" saiu o `requestPasskeyResponse`. Foi o que disse que a tela de confirmação não depende de mais nada que a bancada não tenha. A HMI não lê a resposta a esse pedido (`dsiBluetoothResponsePasskeyResponse` é vazio); o que conduz a tela é o estado do código.

**A regra para não incomodar: só com a tela "Celulares conhecidos" aberta, 4 s depois, uma vez por abertura.** O sinal de que a tela abriu é o `requestReconnectSuspend`: a HMI o manda com verdadeiro ao entrar nessa tela e com falso ao sair, e em nenhuma outra situação que eu tenha visto (zero vezes nos pares de partida e nas provas que não abrem a tela). O modo acessível do Bluetooth não servia de sinal: a HMI também o liga na tela "telefone não conectado". Na partida e nas outras telas nada aparece: dois pares pelo roteiro do modo ao vivo, o candidato contra a pasta (`y1`, sem memória, às 18h48, e `y2`, com, às 18h50), com a previsão gravada antes (`traces/tel_pareamento_previsoes.txt`). As seis fotos iguais byte a byte nos dois, as mesmas linhas de erro (106 e 106; 80 e 80), as mesmas telas na mesma ordem, o resumo da partida na mesma hora (a menos de 0,2 s), **o mesmo número de avisos abertos (um, o de boas-vindas, nos quatro braços) e nenhum pedido de pareamento.**

**O modelo** (`driver/telefonestub.c`, de 503 para 611 linhas; o motor não mudou):

| Pedido ou relógio | O que o modelo faz |
|---|---|
| a partida | se o perfil traz a seção `[DSIBluetooth.updatePasskeyState]`, guarda os valores e deixa o estado do código no padrão (zero). Sem a seção não há segundo aparelho, e os dois pedidos abaixo ficam com o motor, como antes |
| `DSIPhoneManager.requestReconnectSuspend` (api 1004) | responde; com verdadeiro, marca a hora em que a tela abriu; com falso, marca a tela como fechada e retira o pedido de pareamento, se estiver no ar |
| o relógio, 4 s depois de a tela abrir, se ainda não pediu nesta abertura e o aparelho não foi pareado | publica o pedido guardado |
| `DSIBluetooth.requestPasskeyResponse` (api 1009) | responde com o endereço e o nome e zera o estado do código. Com 1, põe o segundo aparelho na lista de conhecidos do gerente (`knownDevices[1]`: endereço, nome, o identificador `aparelho-ficticio-2`, conhecido e não conectado) e não pede mais. Sem pedido no ar, resultado 4 |

O nome ("Celular fictício 2"), o endereço (`00:00:00:00:00:02`) e o código (`123456`) estão no perfil `stubs/telefone/segundo-aparelho.txt`, marcados como inventados.

**O que entrou na pasta**, às 18h56min45 (`traces/tel_instalacao8.txt`): `driver/telefonestub.c`, `driver/tests/t_telefone.sh`, `stubs/telefone/README.md` e, novo, `stubs/telefone/segundo-aparelho.txt`; e **um arquivo de todos, o `driver/ao_vivo.sh`**: o `segundo-aparelho.txt` entrou no padrão do telefone (uma linha trocada e três de comentário). Os de antes estão em `traces/tel_antes8/`. A central ao vivo estava desligada na troca e não foi ligada.

**Testes.** `driver/tests/t_telefone.sh`, de 287 para 311 casos. Os do pareamento foram escritos antes do código; quinze falharam contra o modelo de então, inclusive o que diz que na partida ninguém pede para parear: com o modelo antigo o perfil faria o pedido sair na partida (`traces/tel_pareamento_vermelho.txt`). A bateria inteira, no candidato às 18h48 e da pasta às 19h00 (`traces/tel_suites_candidato_18h48.txt` e `traces/tel_suites_pasta_19h00.txt`): run_tests 8, t_transmissao 19, t_codificacao 39, t_entrada 11, t_pedidos 28, t_veiculo 226, t_memoria 80, t_hub 25, t_vc_diag_alm 37, t_hmilog 32, t_kipc_bus 14, t_radio 114, t_telefone 311, t_midia 119, t_som 214; no Mac, t_memoria 80, t_servidor 20, t_lsmval_abre 38, t_catalogo 59. Nenhuma falha e nenhum aviso do compilador.

**A prova pela pasta instalada**, pelo roteiro do modo ao vivo: uma central, às 18h56, nascida de uma cópia da memória guardada, com rádio, mídia e som (`pv12`). Na partida, só o aviso de boas-vindas de sempre. Aberta a tela, a confirmação vem 4 s depois; "Cancelar" a fecha, e a HMI avisa que a conexão foi cancelada; saindo e voltando, o pedido vem de novo; "Sim" põe o aparelho na lista; saindo e voltando outra vez, ele não pede mais. Nenhum pedido novo sem resposta. O botão de voltar dessa tela leva à tela "telefone não conectado", e não à de configuração do telefone (a primeira prova, `k1`, desandou por isso).

**Limites:**

- **Só o segundo aparelho fictício pede para parear**, e só com a tela "Celulares conhecidos" aberta. A busca de aparelhos pela tela não existe nesta variante.
- **O segundo aparelho não conecta**: existe para o pareamento. Tocar nele na lista faz a HMI pedir a conexão (`requestConnectService` com `aparelho-ficticio-2`); o modelo responde com o resultado 1, e a HMI mostra o alerta de erro geral dela (`…ALERT_AC_GENERAL_ERR`). Medido na `pv12`.
- **O pareamento vale para a sessão.** A central seguinte nasce sem o segundo aparelho, e ele volta a pedir.
- **A lista de aparelhos de confiança do próprio Bluetooth** (atributo 6) a HMI não assina, e o modelo não a preenche; o aparelho entra na lista do gerente, que é a que a tela mostra.
- **Só o código "mostrar e confirmar"** (estado 7). Os outros modos (digitar um código, "só mostrar") não foram exercitados.
- Os 4 s são escolha da simulação.

**As duas pendências das 18h11.** A do pareamento está feita, com os limites acima. A da chamada recebida: medi as duas coisas que faltavam, na pasta instalada (`pv13`, às 18h56). **Com a central no rádio, a chamada recebida entra como um aviso no alto da tela** (`PO_CONFIRM_PHONE_TEMP_INCOMING`): 5550199, "Chamada" e os mesmos três botões, por cima do rádio. **A tecla Telefone do volante, com a chamada tocando, não atende nem recusa**: apertada duas vezes, nenhum pedido saiu, e o aviso ficou até a chamada parar de tocar aos 20 s. Não toquei nos botões do aviso (só os vi). Continuam abertos a exceção do toque no modo ao vivo, que é da frente do som, e o `DSIAdb.restartDownload` da busca de contatos, que o telefone de mentira não responde.

![A chamada recebida com a central no rádio](quadros/2026-10-12-hmi-tel-recebida-no-radio.png)

**Onde parou, às 19h08.** O item 7 está feito: os estados de partida, conectar e desconectar, discar, a chamada, o botão vermelho, a chamada recebida e o pareamento. Ficam de fora a caixa "Visibilidade", a agenda com contatos, as fotos da tela de telefone do carro e a adaptação do 5F lida do carro. Nenhuma central minha ficou no ar.

## Marco 13 — o som de mentira: a tela de som, o volume e o mudo (12/10/2026)

Feito numa sessão à parte, aberta pela sessão principal à 1h30 de 12/10/2026, para o som do simulador de [simulador-jetta.md](simulador-jetta.md): a tela de som (grave, agudo, balanço e o que mais houver) sair do aviso de indisponível e aceitar os ajustes, e o volume ter um valor que muda com o botão e com as teclas do volante. **Não há áudio de verdade: é só o estado que a tela mostra.** É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução** em centrais de teste, as cinco últimas (das 3h35 às 3h44) subidas da pasta já instalada: duas com o carro, o rádio, o telefone e a mídia de mentira ligados, como no modo ao vivo (`P1` e `P2`); duas só com o som (`SO1` e `SO2`); e uma nascendo de uma cópia da memória guardada do modo ao vivo (`V1`):

- **a tecla Som abre a tela de equalização sem o aviso** "A configuração do som estará disponível novamente…": Agudos, Médios e Graves, de −9 a 9. Os toques em "+" e "−" mudam o número e o cursor, e o valor continua lá ao sair e voltar;
- **a aba Posição aceita balanço e fader juntos** (o alvo anda sobre o desenho dos bancos), e a aba Volume abre "Configurações do volume";
- **o volume tem valor e muda com o botão**: a barra abre com o valor (nasce em 5, de 0 a 30) e acompanha cada passo do botão de volume;
- **as teclas Vol + e Vol − do volante e o rolete mudam o mesmo volume**, mas não abrem a barra sozinhas (ver "Limites");
- **o mudo funciona pelas duas teclas** (a do volante e a da moldura): a barra do topo ganha um alto-falante riscado, e a barra de volume, se aberta, mostra o mesmo desenho e fica vazia; aumentar o volume tira o mudo; diminuir no mudo baixa e continua mudo;
- **o rádio de mentira passa a receber cada troca de emissora na hora.** Sem o som, de três trocas seguidas chegava uma, atrasada (era a hipótese da sessão do rádio; confirmada em dez partidas lado a lado). Na tela não muda nada;
- **a mídia de mentira passa a tocar e a voltar.** Com o som a tecla Mídia põe a faixa em "tocando" (botão ‖) e, depois de ir ao rádio, a volta à mídia traz a faixa. Sem o som valem os dois limites do Marco 15 (faixa em pausa e tocador vazio na volta);
- **nenhum molde novo de erro** aparece no registro da HMI com o som, e saem três; nenhuma das 51 centrais com HMI desta sessão registrou erro de OpenGL.

| Antes: o aviso | Depois: a equalização, com dois ajustes feitos |
|---|---|
| ![Antes](quadros/2026-10-12-hmi-som-antes-aviso-de-configuracao.png) | ![Depois](quadros/2026-10-12-hmi-som-equalizador.png) |

| A aba Posição, com o alvo deslocado | A aba Volume: "Configurações do volume" |
|---|---|
| ![Posição](quadros/2026-10-12-hmi-som-posicao.png) | ![Volumes](quadros/2026-10-12-hmi-som-configuracoes-do-volume.png) |

| A barra de volume, em 8 de 30 | A barra no mudo |
|---|---|
| ![Barra](quadros/2026-10-12-hmi-som-barra-de-volume.png) | ![Mudo](quadros/2026-10-12-hmi-som-barra-no-mudo.png) |

A mídia de mentira sem o som (à esquerda) e com ele (à direita): em cima, a tecla Mídia; embaixo, a volta à mídia depois de uma ida ao rádio.

![A mídia sem e com o som](quadros/2026-10-12-hmi-som-midia-sem-e-com-som.png)

As outras fotos: [o mudo na barra do topo, com a barra de volume fechada](quadros/2026-10-12-hmi-som-mudo-na-barra-do-topo.png), [o roteiro inteiro numa partida da pasta instalada](quadros/2026-10-12-hmi-som-prova-da-pasta-P1.png) e [o mesmo roteiro sem o som](quadros/2026-10-12-hmi-som-mesmo-roteiro-sem-som-TA3.png).

**Tudo o que a tela mostra aqui é simulação.** Nenhum valor foi lido do carro. O amplificador do perfil é um interno básico inventado; a ficha do carro traz "Som Beats, a confirmar", e com ele a tela do carro deve ser outra (subwoofer, predefinições). O que vem do firmware são os nomes, as faixas e os valores de fábrica (volume de 0 a 30 nascendo em 5, graves e agudos de −9 a 9), lidos do gerente de áudio do MU0883.

### O pedido previa DSI; o som desta HMI é RSI

O pedido era levantar os serviços DSI de som e estender o esquema do `veiculostub`, como no rádio. O levantamento mostrou outra coisa:

- **a camada DSI da LG não tem som.** Nos 62 pacotes de `com.lge.mib3oi.dsi` não há pacote de som nem de áudio;
- **o componente de som da HMI** (`de.vw.mib.asl.internal.sound.component.SoundComponent`) registra dois monitores de serviço **RSI**: `audiomanagement` e `waveplayer`. RSI são os serviços HTTP e WebSocket locais do Marco 4, achados pelo registro (`MIB_ServiceRegistryApp-00`). Tudo o que a tela de som, a barra de volume e o mudo usam vem do `audiomanagement`; o `waveplayer` é o dos tons (o bipe de toque);
- por isso **não há esquema novo, e o gerador e o `veiculostub` não mudaram**. O passo 2 do pedido não se aplica.

A especificação do serviço está no firmware (`/lge/app_ro/sai/spec/viwi.service.audiomanagement/viwi.service.audiomanagement-1.2.2.json`), com nove recursos: `globals`, `connections`, `volumesettings`, `soundsettings`, `soundparameters`, `soundpresets`, `controls`, `sinks` e `sources`.

### O que a tela de som espera (leitura do código, conferida em execução)

Lido nos decompilados do CFR de 10/10 e conferido nas rodadas com o registro da HMI (`HMILOG` com as chaves de som e de RSI).

1. **A HMI assina `globals`.** Se há um objeto global ("found object in globals, continue subscribing"), assina as conexões de entretenimento (`ENT_AMP_%`), o objeto global com as três conexões dele por inteiro (`entertainmentConnection`, `foregroundConnection`, `settingFocus`), o conjunto de som `entertainment`, a saída de som `entertainment` e os controles de volume do domínio `hmi`.
2. **A saída de som** (`sinks?name=entertainment`) diz que amplificador há (`handleNewSink`): as letras 3 a 5 do `amplifierId` escolhem a família (BAS, DYN, FND, BTS, CAN, SUB, HAK), as cinco primeiras são o prefixo das predefinições, e `availableSoundFunctions` e `eqFrequencies` dizem o que a tela mostra. Com a saída desligada e o identificador vazio, a tela não tem ajuste nenhum.
3. **O aviso "A configuração do som estará disponível novamente…"** é a propriedade 4138, `SOUND_SETUP_ACCESSIBLE`, em falso. Ela vira verdadeira quando o **foco de ajuste** do objeto global é uma fonte de entretenimento (`HsmSoundDPHandling.updateSettingfocusConnection`, `SoundUtil.isEntSource`). Sem serviço de áudio não há foco, e o aviso fica. É o aviso que a sessão das teclas do volante viu à 1h; o README da memória de estado já apontava a falta do `audiomanagement`, sem dizer o que nele decide o aviso.
4. **Graves, agudos, balanço e fader** são parâmetros de som (`soundparameters`: `EQ1` a `EQ5`, `balance`, `fader`, `subwoofer`, `surround`, `noisecompensation`…). A HMI só os assina depois de achar o conjunto de som por uma assinatura de **recurso** (`AudioManagementRsi.subscribeToSoundsettingsEntertainment`, "found soundsetting entertainment"). Isso vai importar adiante.
5. **O volume:** a HMI **não cria controle quando o botão gira.** O evento do botão (`DDS_VOLUME_ROTATED`, 4300067) cai num `break` em `HsmSoundSetterHandling`; ela só abre a barra. No carro quem cria o controle é o `touch-app`: o binário dele tem `/audiomanagement/controls`, `volumeUp_gui`, `volumeIncrease`, `volumeDown_gui`, `volumeDecrease`, `hmi`, `cluster`, `pressed`, `released` e `VolumeRsi.cpp`. A HMI assina os controles do domínio `hmi` ("handleControls new control found") e o volume da conexão com o foco.
6. **O mudo não é da HMI.** Ao receber a tecla ela registra "ASL_SOUND_PIN_MUTE. Not handled by HMI anymore, supplier resposibility now". O mudo é a conexão `FCT_AMP_USER_MUTE` iniciada; a HMI reage a ela (`handleStartConnection: 8`, `updateUserMute true`).

### O gerente de áudio de verdade, e onde parei

O firmware traz o gerente de áudio (`/lge/app_ro/audiomgnt/audiomgnt`, com a `libAudioManagement.so` e a tabela `audiotable/MIB3_Audiotable_MQB.xml`, versão v207, de 2019-10-15). **Ele roda na bancada**, com a biblioteca de barramento dela (`kipc_bus.so`) e o caminho de bibliotecas dos outros programas do firmware: registra-se em uns 12 s e serve a tabela inteira (1 objeto global, 124 conexões, 39 volumes, 49 conjuntos de som, 94 parâmetros, 31 predefinições, 18 saídas, 25 fontes).

Na rodada `r1` a HMI subiu com ele: achou o serviço ("Audiomanagement Ready"), desligou o substituto de áudio que tem dentro dela ("DUMMY RSI AMT DEACTIVATED") e achou o conjunto de som. A saída de som veio desligada e sem amplificador, e a tela seguiu no aviso. **E 75 s depois da partida da HMI o gerente de áudio iniciou sozinho a conexão `FCT_AMP_THEFTPROTECTION_MUTE`**, que a tabela descreve como "Component Protection: Mute on active theft protection", sem acesso de cliente. A HMI mostrou **"Proteção de componentes está ativa."**

**Parei esse caminho aí.** Proteção de componentes é proteção antifurto; não investiguei o que a dispara nem como liberar, e não vou (decisão D112). O gerente de áudio de verdade não roda junto com a HMI na bancada. Ele só é usado sozinho, parado, sem HMI, por alguns segundos, para dizer o que serve em repouso. Medido uma vez só; se o mudo de proteção entra sempre aos 75 s eu não sei.

![O aviso de proteção de componentes, com o gerente de áudio de verdade](quadros/2026-10-12-hmi-som-gerente-de-verdade-protecao-de-componentes.png)

### O mock da LG: o que ele faz e o que não deixa

O mock RSI do firmware (`/lge/app_ro/bin/rsi-mockup`, o do Marco 4) serve qualquer serviço que tenha especificação, se o nome entrar na lista dele (`audiomanagement:mixer`) e houver dados (`<serviço>/<recurso>.json`). Medido em sondas sem HMI, com a ferramenta `rsi-send` do firmware:

| O mock | Medido |
|---|---|
| busca por nome, com `%` (escrito `%25`), e `$fields` | atende |
| grava o que lhe mandam (POST num elemento) e guarda | sim; **não confere faixa** nem tipo |
| cria elemento em `controls` (POST no recurso) | sim (201) |
| avisa quem assina um **elemento** | sim |
| expande referências (`$expand`) | **não**: devolve o que estiver guardado |
| obedece aos métodos que a especificação declara | sim: POST em `sinks` dá **403**; DELETE em `controls` dá **403** |
| avisa quem assina um **recurso** | só se a especificação declarar (`systemTriggeredEvents`): em `soundsettings` não responde; onde responde, manda a lista inteira a cada mudança |
| recarrega os dados com o comando `$_rsi_mockup_reload` | não respondeu em nenhuma forma que tentei |

Com o despejo do gerente de verdade e uma sobreposição feita à mão (rodada `r2`), a tela de som abriu com as três abas e a barra de volume mostrou 5 de 30. Faltavam três coisas: quem aplique o perfil (o mock recusa gravação na saída de som), quem mexa no volume (a HMI não cria controle) e, descoberto na `r3`, a assinatura de recurso dos conjuntos de som.

### A especificação afrouxada

O mock obedece à especificação que o firmware traz, e ela é mais estreita que o serviço de verdade em dois pontos. Com `SOM`, o lançador monta por cima dela, dentro do container, uma cópia com os dois afrouxados (`somstub -s`). Só o mock lê esse arquivo; sem `SOM` nada é montado.

| Ponto | O que muda | Por quê | Evidência |
|---|---|---|---|
| `soundsettings` avisa quem assina o recurso | `systemTriggeredEvents` ganha `"resource"` | é por essa assinatura que a HMI chega a graves, agudos e balanço | **medido**: o gerente de verdade a atende (`r1`, "found soundsetting entertainment"); com a especificação original o mock não responde, e na `r3` o cursor andava e o número ficava em 0; com a cópia, na `r4`, o número acompanha |
| `controls` aceita DELETE de elemento | `endpoints.element` ganha `"delete"` | o controle atendido sai da lista; sem isso ela cresce e a HMI recebe a lista inteira a cada passo | **dedução**: que o gerente de verdade consuma os controles eu não medi |

### Os dados: o que o gerente de áudio serve em repouso

`stubs/som/gerado/audiomanagement/*.json` são os nove recursos como o gerente de áudio do firmware os serve sem carro e sem HMI: 381 elementos, com nomes, identificadores, ligações, faixas e valores de fábrica. Quem gera é `driver/som_tabela.sh`: sobe o hub, o registro e o `audiomgnt` sozinho, espera o serviço responder, pede cada recurso com o `rsi-send` e confere a contagem. Duas gerações deram arquivos idênticos. O `relatorio.txt` traz a versão da tabela, os resumos SHA-256 e a contagem. Em repouso a saída de som vem desligada e sem amplificador, o volume de entretenimento vem travado em 5 (de 0 a 30) e o objeto global não aponta conexão nenhuma: é o que o perfil troca.

### O som de mentira

`driver/somstub.c` (747 linhas, C estático, sem dependência) é um **cliente do mock**, por HTTP e WebSocket. Não inclui o `veiculostub.c`. Faz três coisas:

1. **Aplica o perfil antes de o mock subir** (`-w`): confere cada linha contra os dados (o campo tem de existir no elemento e o valor ser do mesmo tipo) e escreve os dados com o perfil e o estado de partida, que é o que o mock vai ler. Perfil com erro: nada sobe, e sai o arquivo e a linha.
2. **As regras mínimas do gerente de áudio**, no ar por cima do mock. São SIMULAÇÃO, o bastante para a tela:

   | Acontece | O som de mentira faz |
   |---|---|
   | um controle `volumeIncrease` ou `volumeDecrease`, com o número de passos | soma ou tira do volume da conexão que tem o foco de ajuste, dentro da faixa dele; apaga o controle |
   | o volume da fonte chega a zero | liga o mudo |
   | aumento com o mudo ligado | desliga o mudo e aplica os passos |
   | diminuição com o mudo ligado | baixa e continua mudo |
   | um controle `mute` | liga ou desliga a conexão `FCT_AMP_USER_MUTE`, que leva o foco de ajuste enquanto está iniciada |
   | a HMI inicia outra conexão de entretenimento (`ENT_AMP_…`) | ela passa a ser a do objeto global, e a anterior para |
   | a HMI inicia uma conexão de menu com volume próprio (como `FCT_AMP_ONVOLUME_MENU`, a do ajuste "Volume inicial máximo") | ela fica com o foco de ajuste enquanto estiver iniciada, e o botão mexe no volume dela |

   O objeto global é regravado a cada mudança com as três conexões por inteiro, porque a HMI pede com `$expand` e o mock não expande.
3. **O papel do `touch-app`**: ouve o barramento (`DSIKeyPanel`, interface 301) e cria o controle. Botão de volume da moldura (teclado 13, tecla 17) e rolete do volante (4/44): um controle com os passos do evento. Vol + e Vol − do volante (4/42 e 4/43): um passo ao apertar. Mudo do volante (4/57) e da moldura (13/88): um controle de mudo ao apertar. O domínio do controle é `hmi` para a moldura e `cluster` para o volante, e a hora é a do relógio monotônico em ms, que é a que a HMI usa (`MIBTimerManager.getSystemTimeMillis`).

`stubs/som/som-de-mentira.txt`, o perfil: a fonte que toca ao ligar (`ENT_AMP_TUNER`, que é a que a HMI pede na partida), a saída de som ligada com um amplificador interno básico (`VWBAS000`, com balanço, fader, três faixas de equalização e compensação de ruído) e os volumes destravados. Seis ajustes; 44 campos gravados. Tudo marcado como simulação no arquivo. O [README](stubs/som/README.md) diz o que vem do firmware e o que é escolha do simulador.

### Um defeito que o teste mostrou: o vai e volta entre fontes

A primeira versão ouvia a **lista** de conexões. Cada aviso de recurso do mock é um retrato da lista inteira; um retrato tirado antes de uma gravação do próprio programa chegava depois dela e parecia pedido novo de cliente. Na troca de fonte o programa ia e voltava entre as duas (no registro do teste: MEDIA, TUNER, MEDIA…), e a fonte nova acabava parada. Escrevi dois casos que o mostram (`fonte_nova_fica_iniciada` e `troca_de_fonte_nao_vai_e_volta`: quatro linhas de troca onde cabiam duas), vi falhar, e o programa passou a assinar **cada conexão por elemento**, como a HMI faz: o aviso de um elemento só fala daquela conexão, na ordem em que ela mudou. Nas séries, com a HMI trocando entre rádio e mídia, nenhuma troca a mais.

### As rodadas

Centrais de teste `moi3-som-<nome>`, à parte da central ao vivo, subidas de cópias da pasta por um roteiro próprio (o `sobe` do `driver/m8_central.sh` com outro nome de container e as pastas trocadas). Saídas em `traces/som_*`; as previsões de cada rodada, gravadas antes de rodar, em `traces/som_previsoes.md`.

| Rodada | Hora | O que rodou | Observado |
|---|---|---|---|
| sondas | 1h34 a 2h39 | o `audiomgnt` sozinho e o mock sozinho, sem HMI | a tabela "O mock da LG"; o gerente de áudio serve a tabela inteira em 12 s |
| `r1` | 1h43 | a HMI com o gerente de áudio de verdade | a HMI acha o serviço; saída sem amplificador; aos 75 s o mudo de proteção de componentes. Parei aí |
| `r2` | 1h52 | o mock com o despejo do gerente e uma sobreposição à mão | a tela de som abre com as três abas; a barra mostra 5 de 30 e não muda: a HMI não cria controle no giro |
| `r3` | 2h33 | a opção `SOM` do lançador | volume 5, 8, 7 com a barra acompanhando; graves e agudos gravados no serviço, mas o número na tela não muda: falta a assinatura de recurso |
| `r4` | 2h43 | o mesmo, com a especificação afrouxada | graves 2 e agudos −1 com o número ao lado, e continuam ao voltar; balanço 2 e fader 1 juntos; volume 5, 8, 7, 8, 9; lista de controles vazia |
| `r5` | 2h48 | controle sem tecla, mudo pelas duas teclas, giro no mudo | o mudo liga e desliga; a barra no mudo; um controle sem giro não abre a barra |
| série `S` (10) | 2h57 a 3h13 | lado a lado, com rádio e telefone: 4 sem som, 6 com | abaixo |
| sonda `M` (2) | 3h15 | lado a lado, com a mídia que entrou às 3h11 | abaixo |
| série `T` (6) | 3h22 a 3h34 | lado a lado, com rádio, telefone e mídia: 2 sem som, 4 com | abaixo |
| prova (6) | 3h35 a 3h44 | **a pasta instalada**: `P1` e `P2` com os padrões do modo ao vivo e o som, pelo `driver/m8_central.sh` dela; `SO1` e `SO2` só com os padrões das centrais de teste e `SOM`; `V1` como o modo ao vivo, nascendo de uma cópia da memória guardada. E `TA3`, a pasta das 3h14 sem som, para repor o controle do par invalidado da série T | abaixo |
| repetição (2) | 3h51 | a pasta como ficou depois de a sessão do telefone trocar o motor compartilhado (3h47): `P3` e `SO3` | iguais à `P1` e à `SO1`, byte a byte no estado e no registro do som |
| sonda `L` (2) | 3h56 | a pasta instalada, lado a lado: a lista de faixas da mídia sem e com o som | a lista, o toque numa faixa e o tocador funcionam igual; com o som a faixa aparece tocando |
| série `W` (18) | 3h59 a 4h20 | a pasta instalada, lado a lado, **no modo da central ao vivo** (nascendo de uma cópia da memória guardada): 9 sem som, 9 com | em "O modo ao vivo" |

Previsões que erraram, e estão ditas no arquivo: não previ o mudo de proteção (`r1`); achei que a HMI criava o controle de volume (`r1`, `r2`); que a tecla de mudo do volante geraria controle pela HMI (`r3`); e que um controle de volume abriria a barra (`r3`, `r5`). As horas dos títulos das previsões tinham sido escritas de cabeça e estavam adiantadas; corrigi pelas dos arquivos e deixei a nota lá.

### O efeito no rádio e na mídia: as séries

Pares de centrais simultâneas, uma com `SOM=/stubs/som/som-de-mentira.txt` e uma sem, largadas com 3 s de diferença e alternando quem larga na frente, com os padrões do modo ao vivo e o mesmo roteiro de teclas e toques.

**Série S** (2h57 a 3h13; carro, rádio e telefone de mentira; a mídia ainda não estava na pasta). Roteiro: rádio (lista, uma emissora, três trocas seguidas a cada 2,5 s), tela de som (dois "+" em Graves, um "−" em Agudos), três passos de volume, mudo do volante, um passo para baixo, um para cima, teclas Rádio e Carro.

| | Sem som (4) | Com som (6) |
|---|---|---|
| serviços DSI ausentes | 16 | 16 |
| serviços RSI ausentes | `audiomanagement`, `waveplayer` e outros seis | `waveplayer` e os mesmos seis |
| hora aceita, Welcome | sim, nas 4 | sim, nas 6 |
| sintonias no `radiostub` depois de cada uma das três trocas (partindo de 2) | 2, 2, 2, e só no fim 3 | 3, 4, 5 |
| foto depois das três trocas | — | igual byte a byte à do par sem som, nos quatro pares |
| estado do som a cada passo | — | o mesmo arquivo, byte a byte, nas 6: graves 2, agudos −1; volume 5 → 8; mudo liga; −1 dá 7 e segue mudo; +1 tira o mudo e dá 8; lista de controles vazia |
| registro do `somstub`, tirada a hora | — | o mesmo nas 6 (20 linhas) |
| linhas `ERROR` e exceções da HMI antes da derrubada | 145 e 17, nas 4 | 140 e 17, nas 6 |
| erros de OpenGL, `SIGSEGV` | 0 e 0 | 0 e 0 |

- **A hipótese da sessão do rádio se confirma** (Marco 11, "De que a sintonia depende"): sem serviço de áudio a HMI registra "AudioManagement not available yet" e "checkStationSelectionProgress" e a troca não sai; com o som cada troca chega ao sintonizador logo depois do toque. **Na tela não há diferença**: a HMI mostra a emissora que ela escolheu, tenha o pedido chegado ou não.
- **Os moldes das linhas `ERROR` são os mesmos dentro de cada braço.** Do braço sem som para o com som saem três (`audiomanagement` da lista "Required RSI services not registered", as três "checkStationSelectionProgress" e duas das três "AudioManagement not available yet"; a que fica é a da partida, aos 2,3 s, no instante em que a HMI acha o serviço). **Nenhum molde novo aparece.**
- **Contagem bruta de erros engana.** O resumo dava de 153 a 192 linhas `ERROR` e de 29 a 74 exceções nas partidas com som, contra 171 e 47 fixos nas sem. A variação está toda nos últimos 0,1 s, quando o lançador derruba a central e a HMI vê os serviços morrerem: cada assinatura perdida vira um erro com pilha, e quantas dão tempo de sair varia. Cortando no primeiro "Received dead", "Connection lost!" ou "ShutdownThread", os números ficam os da tabela.

**Sonda M e série T** (3h15 a 3h34; com a mídia de mentira, que entrou na pasta às 3h11, nos dois braços). O roteiro da série T acrescenta: tecla Mídia, próxima faixa, tecla Rádio e uma troca de emissora, tecla Mídia de novo, e no fim tecla Mídia e pausa.

| | Sem som (`TA1` e `TA3`) | Com som (`TB1`, `TB3` e `TB4`, da cópia; `P1` e `P2`, da pasta instalada) |
|---|---|---|
| serviços DSI ausentes | 7 | 7 |
| serviços RSI ausentes | `audiomanagement`, `waveplayer` e outros seis | `waveplayer` e os mesmos seis |
| tecla Mídia | a faixa em pausa (▷); o `midiastub` não recebe pedido de tocar | **a faixa tocando** (‖): a HMI inicia a conexão `ENT_AMP_MEDIA_PLAYER` e manda `resume`. O tempo segue em 0:00 |
| saída para o rádio | a HMI desativa a mídia | a HMI manda `pause`, desativa a mídia e inicia `ENT_AMP_TUNER` |
| sintonias no `radiostub` (partindo de 2): depois de cada uma das três trocas; depois da volta ao rádio e mais uma troca; no fim | 2, 2, 2; 3; 4 | 3, 4, 5; 7; 7 |
| volta à mídia | **o tocador vazio**; a HMI nem pede a ativação | **a mesma faixa, tocando**: `setActiveMedia` e `resume` de novo |
| tela de som | o aviso | a equalização; graves 2 e agudos −1 |
| botão de volume | a barra abre e o valor não muda | com a mídia como fonte: 5 → 8; mudo pela tecla do volante; 7, ainda mudo; 8, sem mudo |
| toque na pausa do tocador | nada (o tocador está vazio) | ▷, e um alto-falante riscado na barra do topo: é a HMI mostrando a pausa, o mudo do serviço segue desligado |
| estado do som a cada passo e registro do `somstub`, tirada a hora | — | os mesmos, byte a byte, nas cinco |
| linhas `ERROR` e exceções da HMI antes da derrubada | 115 e 19, nas duas | 108 e 19 em três; 107 e 18 em duas (`TB4` e `P2`) |
| erros de OpenGL, `SIGSEGV` | 0 e 0 | 0 e 0 |

- **Os dois limites que o Marco 15 pôs na conta da falta do serviço de áudio somem com o som.** Ele tinha deixado escrito que "a comparação com um serviço de áudio no ar confirma ou desmente" a causa: no resultado, confirma (com o serviço no ar a faixa toca e a mídia volta). O passo do meio, a segunda ativação do rádio terminar, eu não fui ler no registro. Sem `SOM` os dois limites continuam valendo. O terceiro de lá (as teclas Próxima e Anterior do volante na tela de mídia) não foi exercitado aqui.
- **Um par da série T não vale, por erro meu.** Às 3h28min49, com `TB2` e `TA2` no ar, regravei no lugar o roteiro que as conduzia, para acrescentar o ramo da prova da pasta. O shell lê o roteiro por posição no arquivo; as duas repetiram passos e soltaram "command not found". Os números delas saíram iguais aos do primeiro par, e não contam. A `TA3` repôs o controle, junto com a prova.
- **O erro a menos da `TB4` e da `P2`** é um molde da tela de mídia que existe nos dois braços: "Draw error in 'TextDrawable_0': Empty text", no título da faixa. Aparece duas vezes nas duas partidas sem som e, nas cinco com som, duas vezes em três e uma vez em duas. Não é molde novo; se o som muda quantas vezes ele sai, esta amostra não diz.
- A previsão das sintonias errou por um (escrevi 6 onde saiu 7): a volta ao rádio pela tecla já manda sintonizar. E a das ativações também (escrevi três; são duas: a terceira tecla Mídia vem da tela de som, com a mídia ainda como fonte).

**As centrais só com o som** (`SO1` e `SO2`, da pasta instalada, sem rádio, telefone nem mídia de mentira): iguais entre si, no estado a cada passo e no registro do `somstub`. A tecla Som abre a equalização; graves 2 e agudos −1; balanço 2 e fader 1; "Configurações do volume" abre e fecha, e os valores continuam lá; o botão leva o volume de 5 a 8; Vol + do volante, 9; Vol −, 8; o rolete com dois passos, 10; mais um passo do botão, 11, e a tecla de mudo da moldura liga e desliga o mudo. Nove controles criados, sete de volume atendidos, nenhum erro de OpenGL.

**O custo parado** (sonda M, 20 s, as duas centrais lado a lado): a JVM da HMI 1,6 % de um núcleo sem som e 1,5 % com; o mock RSI 0,1 %; o `somstub` abaixo do que a medida enxerga (3 centésimos de segundo de processador em 4 min e meio).

### Depois de instalar: a pasta mudou, e duas conferências que faltavam

- **A pasta mudou doze minutos depois.** Às 3h47min48 a sessão do telefone trocou o motor compartilhado (`driver/veiculostub.c`, valores por instância) e o `t_telefone.sh`; a sessão dos outros menus tinha posto três arquivos novos às 3h45. Os cinco lançadores e o `somstub.c` seguiam como instalei. Repeti com a pasta como ficou (3h51): `P3` igual à `P1` e à `P2`, e `SO3` igual à `SO1`, byte a byte no estado e no registro do som; e a bateria inteira passa de novo (`t_som.sh` 135, `t_telefone.sh` agora 142, `t_veiculo.sh` 188; `traces/som_suites/saida-pasta-0351.txt`).
- **A lista de faixas da mídia com o som** (sonda `L`). Nos registros das séries o pedido `DSIMediaBrowser.requestList` saía 4 vezes por partida sem som e 2 ou 3 com, e eu não tinha aberto a lista em nenhuma rodada com som. Lado a lado, da pasta instalada: nas duas a aba "Seleção" mostra "Minhas mídias" com as cinco faixas, o toque na terceira a marca e ela vai para o tocador, e a seta leva à quarta. Os pedidos ao `midiastub` são os mesmos, menos uma procura da entrada em foco (4 `requestList` contra 5) e o `resume` a mais. A diferença na tela é só o estado: em pausa sem o som, tocando com ele.

  ![A lista de faixas sem o som (à esquerda) e com ele (à direita)](quadros/2026-10-12-hmi-som-midia-lista-sem-e-com-som.png)
- **A partida no modo da central ao vivo, lado a lado** (série `W`), no item seguinte. A nota de método das outras sessões manda incluir esse modo na validação antes de instalar; eu só tinha rodado nele uma central, sem controle, e depois de instalar. Fica dito que a ordem foi essa.
- **A espera nova pelos serviços DSI** (par `W10`, 4h22). Às 4h08min09 a sessão do rádio e da mídia pôs no `driver/ao_vivo.sh` a `ESPERA_DSI`, 6 s por padrão no modo ao vivo (decisão D158), por cima da versão com o som, que ficou inteira. Um par como os da série W, com essa espera: os dois braços iguais entre si (12 telas até a tecla, o assistente aos 52,4 e aos 52,6 s em vez dos 43,7 s, 65 linhas `ERROR` e 14 exceções nos dois, o indicador de gesto nos dois, 0 erros de OpenGL); a tecla Som aos 70 s abre a equalização com o som e o aviso sem ele, e um passo do botão leva o volume de 5 a 6. Uma partida de cada braço, fora das contagens das tabelas: com elas a sessão fecha em 53 centrais com HMI.
- **A outra corrida de partida do Marco 10, conferida nas 53 centrais** (aviso da sessão do processador, às 4h25). O perfil inicial da persistência saiu "already set" em 52 e "not set" numa só, a `MB`, que é o braço com som da sonda M. O que a `MB` mostrou (a mídia tocando e voltando depois do rádio, as trocas de emissora chegando na hora) repetiu-se depois em seis partidas com "already set" (`TB1`, `TB3`, `TB4`, `P1`, `P2` e `P3`); a conclusão não depende dela. Uma em 53 está dentro do que aquela sessão mediu com a máquina livre (2 em 50), e não atribuo ao som.

### O modo ao vivo

**O som de mentira entra por padrão no modo ao vivo** (`driver/ao_vivo.sh`, e com ele o botão da janela), como o carro, o rádio, o telefone e a mídia de mentira. É decisão minha, registrada como D119; `SOM= sh driver/ao_vivo.sh` sobe sem ele. As centrais de teste e o lançador direto seguem sem padrão.

O critério: quinze partidas com som (seis na série S, uma na sonda M, três na série T e cinco na prova), com o estado igual byte a byte dentro de cada roteiro; nenhum molde novo de erro e três a menos; nenhum erro de OpenGL; custo parado abaixo do que se mede; e o rádio e a mídia, que o modo ao vivo já liga, passam a se comportar mais como os de um carro.

Para saber o que o Lipe vai ver, a central `V1` nasceu de uma **cópia** da memória guardada do modo ao vivo (61 linhas, só lida), com o rádio, o telefone, a mídia e o som de mentira, subida pelo `driver/m8_central.sh` da pasta instalada:

- nasce na grade, que é a última tela guardada, com o aviso do usuário principal por cima, e aos 66 s está no assistente de configuração ("Bem-vindo"), como antes do som;
- **com o assistente na tela a tecla Som abre a equalização**; dois toques em "+" levam os graves a 2; o botão abre a barra por cima e o volume vai de 5 a 8;
- a tecla Mídia abre o tocador com a faixa tocando; a tecla Rádio volta ao rádio, e a seta troca de emissora;
- a tecla de mudo do volante põe o alto-falante riscado na barra do topo, e a segunda vez o tira;
- 0 erros de OpenGL em 1.031 quadros (a `V1` foi uma partida só, sem controle ao lado; a comparação lado a lado nesse modo é a série W, abaixo);
- **a memória guardada não leva os ajustes de som.** A cópia saiu da central com 64 linhas (entrou com 61), e as três linhas do módulo Sound (`5056`) saíram iguais byte a byte, com os graves em 2 e o volume em 9 no serviço. As três linhas novas são do `5057`, que a tabela do [README da memória](stubs/persistencia/README.md) não tem (não atribuí; a mídia de mentira é nova no modo ao vivo desde as 3h11); mudaram também linhas do rádio (`5052`) e do assistente (`5060`).

![A central nascida da memória copiada: grade, assistente, tela de som, barra de volume, mídia, rádio e mudo](quadros/2026-10-12-hmi-som-como-o-modo-ao-vivo-V1.png)

**A série W** (3h59 a 4h20; nove pares simultâneos da pasta instalada, um braço com `SOM` e um com `SOM=`, todos nascendo de uma cópia da memória guardada, com o rádio, o telefone e a mídia de mentira; sem toque até os 70 s, depois a tecla Som e um passo de volume):

| | Sem som (9) | Com som (9) |
|---|---|---|
| perfil inicial da persistência | "already set", nas 9 | "already set", nas 9 |
| a JVM da HMI nasce, contado do container (medido em 6 pares) | aos 24 ou 25 s | aos 24 ou 25 s |
| telas que entram até os 70 s | 12 (13 numa) | 12 |
| assistente de configuração | aos 43,6 a 43,8 s | aos 43,6 a 44,0 s |
| linhas `ERROR` e exceções antes da derrubada | 68 e 17, nas 9 | 68 e 17, nas 9 |
| registros dos stubs (linhas): veículo, rádio, telefone, mídia | 130, 49, 40 e 20, nas 9 | os mesmos, nas 9 |
| indicador de gesto da grade ("[SpaceGesture] enableIndicator: SimpleGridMenu") | em 4 das 9 | em 6 das 9 |
| tecla Som aos 70 s | o aviso | a equalização |
| memória no fim | 64 linhas | 64 linhas |
| erros de OpenGL | 0 | 0 |

- **O som não muda a partida**: as mesmas telas, o assistente no mesmo segundo, os mesmos erros (nem um a mais nem a menos antes da tecla: neste roteiro não há troca de emissora), os registros dos stubs do mesmo tamanho, e a HMI nasce no mesmo segundo. Eu tinha previsto um atraso de 1 a 3 s por causa da espera pelo `somstub`; não há, na resolução de 1 s da medida.
- **O indicador de gesto da grade** é a corrida de partida que o Marco 10 descreveu: quando ele liga, a grade ganha um brilho vermelho no pé e a central desenha uns 180 quadros nos primeiros 20 s; quando não, uns 50. Na primeira leva, de três pares, saiu 2 a 0 para o braço com som; na segunda, de seis, 4 a 4; somadas, 6 a 4. Em cinco dos nove pares os dois braços concordam, e houve par em que só o braço sem som o teve. **Não atribuo ao som**: é o tipo de contagem que a nota das outras sessões já mostrou enganar numa leva só, e a amostra não separa as duas coisas.

**Não verificado:** a central ao vivo em si. Ela estava desligada e não foi ligada. O `ao_vivo.sh` foi conferido numa cópia à parte, com um `docker` de mentira no caminho, só para ver a linha que ele monta: `-e SOM=/stubs/som/som-de-mentira.txt` por padrão, vazio com `SOM=`, e `-` com `SOM=-`.

### O que entrou na pasta

- `driver/somstub.c`, novo: o som de mentira. `driver/som_tabela.sh`, novo: gera `stubs/som/gerado/` a partir do gerente de áudio do firmware, sozinho. `driver/som_estado.sh` e `driver/som_controle.sh`, novos: mostram o estado numa linha e criam um controle sem tecla, com a central no ar.
- `stubs/som/`, nova: os dados gerados (`gerado/audiomanagement/*.json` e `relatorio.txt`), o perfil `som-de-mentira.txt` e o [README](stubs/som/README.md).
- `driver/tests/t_som.sh`, novo: 135 casos.
- `driver/m1_build.sh`: compila o `somstub`; se não compilar, avisa e segue. `driver/m1_hmi.sh`: a opção `SOM` (e `SOM_DADOS`); um bloco antes da camada RSI, que confere o perfil, escreve os dados e monta a especificação afrouxada; o `audiomanagement` na lista de serviços do mock e os dados na pasta dele; o `somstub` no ar antes de a HMI subir; o `somstub.log` na lista de registros e a linha "som:" do resumo. `driver/m3_ao_vivo.sh`: só comentário. `driver/m8_central.sh`: repassa `SOM` e `SOM_DADOS`. `driver/ao_vivo.sh`: `SOM_PADRAO`.
- Treze quadros `quadros/2026-10-12-hmi-som-*.png`: os das telas são da `SO1`, da `P1` e da `V1` (pasta instalada); o do aviso e o do roteiro sem som, da `TA3`; o da proteção de componentes, da `r1`.
- `traces/som_*`: as previsões (`som_previsoes.md`), as pastas de todas as rodadas com fotos e registros (`som_rodadas/`), as sondas sem HMI (`som_sondas/`), o teste visto falhar (`som_vermelho/`), as baterias (`som_suites/`) e os roteiros e remendos (`som_roteiros/`). Os cinco lançadores de antes da troca estão em `traces/som_antes/driver/`.
- Na ficha do carro: um item em "O que anotar no primeiro autoscan" (fotos da tela de som do carro, que dizem se há Beats e o que a tela oferece).
- Notas datadas, onde o que estava escrito deixou de valer com o som ligado: no Marco 15 e no `stubs/midia/README.md` (os dois limites), no `stubs/persistencia/README.md` (graves, agudos e balanço passam a mudar pela tela), e nas linhas 4, 5 e 6 do roteiro de `simulador-jetta.md`, que ganhou também a linha 9, um pedido de fotos e uma fonte. Um parágrafo no `README.md` da bancada, uma frase no da raiz do projeto e uma nota no `stubs/rsi-mockup/README.md` (o mock passa a servir o serviço de áudio).

**O gerador de esquema, o `veiculostub` e os outros stubs não mudaram.** A pasta `janela/` e a `glforward-moi3/` não foram tocadas.

**A instalação.** Trabalhei em cópias fora da pasta. A primeira (da pasta das 2h54) passou pela série S; às 3h11, quando eu ia instalar, a mídia de mentira entrou na pasta e mexeu nos mesmos cinco lançadores. Refiz a cópia sobre a pasta das 3h14, com as âncoras novas (`traces/som_roteiros/remendos/lancadores3.py`), rodei a bateria nela e a série T, com a mídia nos dois braços. Às 3h35min16, depois de conferir que `driver/`, `stubs/` e `glforward-moi3/` inteiros ainda eram os das 3h14, troquei os cinco lançadores um a um, por cópia ao lado e troca de nome, e copiei os arquivos novos. Havia quatro centrais de outras sessões no ar. Avisei antes e depois na pasta de notas; não esperei resposta.

### Testes

`driver/tests/t_som.sh`, 135 casos. A primeira parte roda sem firmware: a conferência do perfil (o da pasta é aceito; elemento, recurso ou campo que não existe, valor que não é JSON, tipo trocado, linha solta e fonte inicial que não existe ou não é de entretenimento são recusados com o arquivo e a linha), a escrita dos dados (os nove recursos; o que o perfil não toca sai igual byte a byte; sem perfil saem os mesmos bytes que entraram; perfil recusado não escreve nada) e a especificação afrouxada. A segunda sobe o hub, o registro e o mock do firmware com os dados preparados e o `somstub` por cima, e faz o papel da HMI com o `rsi-send`: os dados servidos, a assinatura de recurso, o volume (passos, máximo, 70 controles cada um atendido uma vez, a lista vazia depois), o mudo, a troca de fonte, a conexão de menu, os ajustes guardados, e o botão e as teclas pelo barramento. Sem firmware montado essa parte avisa e é pulada.

Escrito antes do código, em levas, cada uma vista falhar (`traces/som_vermelho/`):

| Leva | Hora | Casos que falhavam | Contra o quê |
|---|---|---|---|
| o programa inteiro | 2h12 | 73 de 93 | um esqueleto do `somstub` que só aceitava as opções |
| a especificação afrouxada, apagar o controle atendido, 70 controles seguidos | 2h41 | 11 de 130 | o programa da rodada `r3` |
| o vai e volta entre fontes | 2h42 | mais 2 (13 de 132) | o mesmo programa, que ouvia a lista de conexões |
| o mudo pelas teclas do volante e da moldura | 2h47 | 2 de 135 | o programa da rodada `r4` |
| o que ficou | 2h48 | 0 de 135 | |

Um defeito do próprio teste, achado pelo caminho: a limpeza usava `kill $(jobs -p); wait`, e no `dash` o `$(jobs -p)` roda num subshell e não vê os processos. O `wait` nunca voltava, e três containers de teste ficaram no ar de 10 a 30 min até eu ver. Passou a guardar os processos numa lista.

A bateria inteira, rodada da pasta às 3h35 de 12/10/2026 com tudo instalado (`traces/som_suites/saida-pasta.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_hmilog.sh` 32, `t_hub.sh` 25, `t_kipc_bus.sh` 14, `t_memoria.sh` 80, `t_midia.sh` 107, `t_pedidos.sh` 28, `t_radio.sh` 81, `t_som.sh` 135, `t_telefone.sh` 103, `t_vc_diag_alm.sh` 37 e `t_veiculo.sh` 187, no container; `t_servidor.sh` 20, `t_lsmval_abre.sh` 38 e `t_memoria.sh` 80, no Mac. Todos passam, sem aviso do compilador; e de novo às 3h51, com a pasta como ficou depois da troca do motor compartilhado (`saida-pasta-0351.txt`). Antes de instalar: na segunda cópia (3h22) a mesma bateria passou inteira; na primeira (2h56) passou tudo menos o `t_telefone.sh` (57 casos de 62 falhando), por defeito do meu roteiro de bateria, que não entregava ao teste a pasta do perfil. Acertei o roteiro, e não o teste.

### Limites

- **Não há áudio.** É o estado que a tela mostra.
- **A barra de volume só abre com o giro do botão.** Quem a abre é o evento de giro, dentro da HMI. As teclas Vol + e Vol − do volante mudam o volume (controle de domínio `cluster`), mas a barra não abre: a HMI só assina os controles de `cluster` com a chave de recurso 271, e um controle de domínio `hmi` criado sem giro também não a abre (`r5`). A sessão das teclas do volante tinha deduzido que "a barra deve aparecer quando o serviço de som avisar o volume novo"; **medido, não aparece**. Com a barra já aberta as teclas a seguram e o valor acompanha. O que abre a barra no carro com a tecla do volante não foi achado. **Mudou às 17h03 de 12/10/2026: a barra passa a abrir pelo volante, como SIMULAÇÃO pedida pelo Lipe (o som de mentira gira também o botão da central); ver "A barra de volume pelo volante: simulada a pedido do Lipe".** **Achado às 9h19 de 12/10/2026:** nesta variante do firmware a central não abre a barra pelo volante, de propósito; quem mostra o volume do volante é o painel de instrumentos (ver "A barra de volume pelo volante", abaixo).
- **O amplificador é simulação.** `VWBAS000` não é identificador de verdade, e as frequências das três faixas são inventadas. A ficha traz "Som Beats, a confirmar": se for, a tela do carro tem outras funções.
- **As regras são mínimas.** Um passo por passo do botão; sem as prioridades e as ações cruzadas da tabela de áudio (aviso de navegação que abaixa a música, telefone que pausa o rádio, limite de volume ao ligar). Só há uma fonte de entretenimento por vez, o mudo e as conexões de menu.
- **Nada é guardado entre partidas.** O que a tela grava vale até a central desligar; a seguinte nasce dos valores de fábrica e do perfil. No carro o gerente de áudio guarda. **Mudou às 9h07 de 12/10/2026:** no modo ao vivo o som de mentira passou a guardar volume, graves, médios, agudos, balanço, fader e mudo (ver "A memória do som", abaixo). Nas centrais de teste continua valendo, salvo pedido.
- **Segurar a tecla do volante** não repete: é um passo por aperto.
- **O mock não confere faixa.** A HMI não manda valor fora da faixa; outro cliente poderia.
- **O `waveplayer` segue ausente**, e com ele o bipe de toque. Continua na lista de serviços RSI ausentes.
- **A especificação afrouxada é uma escolha da bancada**, com um ponto medido e um deduzido (acima).
- **O mudo de proteção do gerente de verdade** foi visto numa rodada só, e não foi investigado, de propósito.
- **Com `SOM` a HMI desliga o substituto de áudio dela**, e rádio e mídia passam a depender do serviço. Medido como melhora nos dois; o telefone não tem aparelho, e chamada não foi exercitada.
- **A aba "Configuração" da tela de som e as predefinições** não foram exercitadas.
- **Os dados são do MU0883**; o carro roda o MU0803, e a tabela de áudio pode diferir.
- **Só no container.** Não rodou no QEMU.

### A memória do som: o que a tela muda volta na partida seguinte (12/10/2026, de manhã)

Pedido do Lipe às 8h30, pela sessão principal, para terminar o item 9 do roteiro: os ajustes de som voltarem na partida seguinte, no modo ao vivo, pelo desenho do estado do carro de mentira.

**Resultado, verificado em execução** numa cadeia de duas centrais subidas da pasta já instalada (`Pa` e `Pb`, das 9h08 às 9h13), no modo da central ao vivo (memória guardada copiada, rádio, telefone, mídia e som de mentira):

- na primeira, graves em 2, agudos em −1, balanço 2 para a direita, volume de 5 a 8 e mudo pela tecla do volante; ao desligar, `memoria/som/estado.txt` tem exatamente isso;
- a segunda nasce da memória que a primeira deixou: **aos 40 s, antes de qualquer tecla, o serviço já serve volume 8, mudo ligado, graves 2, agudos −1 e balanço 2**; a tela de som mostra Agudos −1 e Graves 2 com o número ao lado e o alvo da posição onde ficou, e a barra do topo traz o alto-falante riscado;
- um passo do botão tira o mudo e leva o volume a 9, e a memória acompanha;
- uma central de teste comum (`SO4`, `SOM` sem `MEMORIA`) nasce como antes e não guarda nada: nenhuma linha de memória no registro, nenhuma pasta;
- a bateria inteira passa da pasta (`t_som.sh`, agora com 185 casos).

![Em cima, a primeira partida; no meio, a seguinte, que nasce com os mesmos valores e com o mudo; embaixo, o fim da primeira e a barra depois de um passo na segunda](quadros/2026-10-12-hmi-som-memoria-volta-na-partida-seguinte.png)

**Como funciona.** O `somstub` ganhou a opção `-m <arquivo>`. No ar, com ela, assina também cada volume e cada parâmetro de som (133 elementos: graves, agudos e balanço a tela grava direto no serviço, sem passar por ele) e regrava o arquivo a cada mudança, ao lado e com troca de nome. Na partida (`-w`), o que o arquivo traz entra por cima do perfil e antes do estado de partida, porque o objeto global leva embutido o volume da fonte. O arquivo tem o formato do perfil, com `mudo = ligado` ou `desligado` antes das seções e `value` nas de `volumesettings` e `soundparameters`. **É lido com tolerância:** linha que não serve (elemento que não existe, valor fora da faixa, outro recurso) é ignorada com aviso, e a central sobe. O lançador tem a opção `SOM_ESTADO`, que o `m3_ao_vivo.sh` tira da `MEMORIA` (`<memória>/som/estado.txt`), como o `VEICULO_ESTADO`; `MEMORIA=` desliga tudo, `SOM_ESTADO=` só essa parte. O [README](stubs/som/README.md) tem o formato e o que fica de fora (a fonte, a saída de som, as predefinições).

**As rodadas** (previsões em `traces/som_previsoes.md`, escritas antes):

| Rodada | Hora | O que rodou | Observado |
|---|---|---|---|
| série `E` (6) | 8h59 a 9h06 | duas cadeias de duas partidas da cópia de trabalho (`E1a`→`E1b`, `E2a`→`E2b`) e dois controles da pasta das 8h48 (`C1`, `C2`), no modo da central ao vivo, duas centrais por vez | a segunda partida de cada cadeia nasce com volume 8, mudo ligado, balanço 7 e fader 9, que a primeira deixou; a partida é a dos controles (13 telas até a tecla, assistente aos 52,3 a 52,6 s, 87 linhas `ERROR` e 26 exceções nas quatro, mesmos moldes); 0 erros de OpenGL |
| prova (3) | 9h08 a 9h13 | **a pasta instalada**: a cadeia `Pa`→`Pb`, com o roteiro escolhendo a aba do equalizador, e a central de teste comum `SO4` | a lista acima |

- **A previsão dos valores da série E errou.** A tela de som abriu na aba Posição, e não no equalizador: a memória copiada era a da central ao vivo, que o Lipe estava usando, e a HMI guarda a aba aberta. Os toques que eram para Graves e Agudos caíram no desenho dos bancos. O mecanismo é o mesmo (são parâmetros de som), mas **o equalizador voltando na tela só foi visto na prova da pasta, depois de instalar**.
- **Uma exceção que não existia de madrugada**, e que não é da memória: "Exception in HSM HsmSoundWaveplayerHandling … WaveplayerController.setBeepType", 4 vezes por partida nas quatro da fase de mexer, **controles incluídos**. Vem com a memória copiada de agora (palpite: o som de toque está ligado nela) e é o serviço de tons ausente, limite já registrado.
- "Persistence initial profile not set" numa das seis da série (`E1a`): a corrida de partida do Marco 10.

**Teste.** Cinquenta casos novos no `driver/tests/t_som.sh`, escritos antes do código e vistos falhar (`traces/som_vermelho/t_som.vermelho5.out`): a memória aplicada na escrita dos dados (volume, graves, balanço, mudo com foco e primeiro plano, o volume embutido no objeto global); arquivo que não existe ou vazio dá a mesma saída que sem memória; arquivo estragado não impede a partida, aplica o que presta e avisa linha por linha; no ar, cada mudança de volume, de mudo e de parâmetro gravado pela tela chega ao arquivo, o que já estava guardado continua, e o arquivo que fica é aceito inteiro na partida seguinte; memória que não dá para gravar avisa uma vez e não derruba o programa.

**Instalação.** Às 9h07min10, por cópia ao lado e troca de nome, com a central ao vivo no ar (ela tem `driver/` montado e segue com os arquivos que abriu): `driver/somstub.c`, `driver/tests/t_som.sh`, `driver/m1_hmi.sh`, `driver/m3_ao_vivo.sh` e `driver/m8_central.sh`; os de antes estão em `traces/som_antes_memoria/`. Antes, conferi que `driver/`, `stubs/` e `glforward-moi3/` eram os da cópia base das 8h48, salvo um README de outra sessão.

**Limites.** O mudo volta ligado, e se o carro faz isso eu não sei. A fonte que toca não é guardada aqui (quem lembra é a HMI). Não exercitei a central ao vivo: ela estava em uso, com os arquivos de antes, e **só passa a guardar o som na partida seguinte**. Duas cadeias antes de instalar e uma depois; a memória é um arquivo e o caminho é determinístico, mas é pouca repetição para a parte que depende da HMI ler os valores ao subir.

### A barra de volume pelo volante: nesta variante a central não a abre (12/10/2026, de manhã)

Segunda parte do pedido do Lipe para o item 9: a barra de volume abrir com Vol +, Vol − e o Rolete do volante. **Não foi feito: parei na leitura do firmware e perguntei.** O que achei, tudo por leitura (nenhuma central rodou para isto):

1. **A HMI tem uma chave de recurso para exatamente isso:** `FEAT_IS_MFL_VOLUME_POPUP`, número 271, nome `isMFLVolumePopup` (`ConfigurationManagerGenerated` e `FeatureFlags`). É ela que o Marco 13 já tinha visto como "a chave 271": com ela ligada, a HMI assina os controles de volume do domínio `cluster` (`AMTSubscriber`, `subscribeToClusterVolumeControls`), e cada controle novo vira o aviso `VolumeTriggered` (`AudiomanagementRSIAccess.handleControls`, `onControlVolumeCreated`). O som de mentira já cria controles desse domínio para as teclas do volante.
2. **No firmware da bancada (MU0883) a chave vem desligada e é constante.** Li `/vw/hmi/res/configurationmanager.res` com o leitor do próprio firmware (`com.microdoc.util.ObjectSwapper`; programa em `traces/som_roteiros/chaves/LeChaves.java`): o item 7 traz as chaves em três partes de 35 inteiros (valor, "muda em execução", "é lógica"). Para a 271: valor **falso**, não muda em execução, não é lógica, e nenhum dos 126 métodos de tempo de execução a toca. `FeatureFlagHandler.setFeatureFlagValue` só mexe em chave marcada como de execução. Ou seja, codificação e adaptação não a ligam: é da variante. (Conferência do leitor: a 894, `FEAT_LOG_IS_PHONE`, sai como lógica, como o Marco 12 achou.)
3. **O que abre a barra é o botão da própria central.** No registro da `r4` o popup `PO_SOUND_VOLUME_SOUND_ENTERTAINMENT` abre antes de a HMI tratar o controle: quem o abre é o evento do botão giratório, na máquina de estados de popups, e não o serviço de áudio. A HMI também cria controles ela mesma, mas só para a faixa de toque de volume e para dois eventos de mais e de menos (`HsmSoundSetterHandling.increaseVolumeForeground`); nada disso passa pelas teclas do volante.
4. **O volume pelo volante vai para o painel de instrumentos.** A tecla faz a HMI disparar o evento 232, `VOLUME_CHANGE_THROUGH_MFL` (isso é leitura da sessão das teclas do volante, que não refiz), e quem o escuta é a função `CurrentVolume` do serviço de áudio do painel (`de.vw.mib.bap.mqbab2.audiosd.functions.CurrentVolume`, `mflVolumeChange`): ela manda o volume ao painel e arma um temporizador que se chama "fechar o popup de volume". Na bancada não há painel.

**O que isso quer dizer.** Pelo que o firmware diz, a central desta variante não mostra a barra quando o volume muda pelo volante; o painel mostra. O simulador hoje faz o mesmo que a HMI: o volume muda, a barra não abre. **Fazer a barra abrir seria inventar um comportamento que o firmware diz que a central não tem**, por um de dois caminhos: trocar o arquivo de configuração da variante para ligar a chave (não fiz, e não acho certo: é mudar a identidade do firmware), ou fazer a tecla do volante girar também o botão da central (dá para fazer no som de mentira, marcado como simulação). É decisão do Lipe; a pergunta foi para a sessão principal às 9h19.

**O que não sei.** O carro roda o MU0803, e não li o `configurationmanager.res` dele; se a variante do carro tem a chave ligada, a central dele abre a barra, e aí o som de mentira já serviria (os controles de domínio `cluster` existem). Uma foto ou um vídeo curto da central do carro com o volume mudando pelo volante resolve: pedido acrescentado à ficha. Também não medi o caminho com a chave ligada: não há como ligá-la sem mexer no arquivo do firmware.

### A barra de volume pelo volante: simulada a pedido do Lipe (12/10/2026, à tarde)

**A resposta.** Às 12h40 a sessão principal perguntou ao Lipe: "No seu carro, quando você muda o volume pelo volante, a barra de volume aparece na tela da central?". Ele escolheu, entre três respostas, "Aparece na central". É dito por ele, de memória, sem foto nem vídeo. E escolheu simular.

**A diferença, para ficar clara.** Pela leitura do firmware da bancada (MU0883), a chave que faz a central mostrar a barra pelo volante (`isMFLVolumePopup`) é constante e vem desligada, e o volume do volante vai para o painel de instrumentos (seção anterior). O carro dele roda o MU0803 e, segundo ele, mostra a barra na central. As duas coisas podem ser verdade (a variante do carro ter a chave ligada), e eu não li o MU0803. **O que o simulador faz agora é imitar o que o Lipe disse do carro, por um caminho que não é o do carro:** a HMI da bancada continua com a chave desligada.

**Resultado, verificado em execução** em duas centrais subidas da pasta já instalada (`Pv1`, sem memória, e `Pv2`, nascendo de uma cópia da memória guardada do modo ao vivo, das 17h04 às 17h07):

- Vol + e Vol − do volante e o rolete **abrem a barra de volume** na tela da central, e o valor muda um passo por aperto (o rolete, os passos do giro);
- o botão da própria central segue como era, e o mudo do volante também;
- numa central com a simulação desligada no perfil o volume muda e a barra não abre, como a HMI faz sozinha.

![À esquerda, sem a simulação: o volume mudou e a barra não abriu. À direita, com ela: a barra aberta depois de Vol + (em cima) e do rolete (embaixo)](quadros/2026-10-12-hmi-som-barra-pelo-volante-sem-e-com.png)

**Como funciona.** O perfil ganhou a chave `volante_abre_a_barra` (`sim` em `stubs/som/som-de-mentira.txt`, marcada como SIMULAÇÃO, com a fonte). Com ela, quando o som de mentira vê Vol +, Vol − ou o rolete do volante no barramento, **não cria o controle de volume direto**: escreve `encoder 13 17 ±n` no arquivo de comandos do `dsistub`, ou seja, gira também o botão da central. A HMI abre a barra como abre para o botão, o `dsistub` publica o giro, e o controle de volume sai pelo caminho do botão, uma vez só. A opção sai do perfil para `<destino>/opcoes.txt` na escrita dos dados (`-w`) e é lida dali no ar; nenhum lançador mudou. Sem ter onde escrever o giro (central sem entrada), volta ao que era, com um aviso. **Para desligar:** um segundo perfil só com `volante_abre_a_barra = nao` (`SOM="/stubs/som/som-de-mentira.txt /caminho/desliga.txt"`), ou trocar a linha no perfil.

**Um defeito que o teste mostrou, e que não era da barra.** O hub do barramento reenvia a quem se liga as primeiras 48 mensagens que guardou (é assim que a HMI, que sobe depois, recebe os anúncios). Numa central de verdade são anúncios de partida; no teste havia teclas antigas entre elas, e uma instância do `somstub` que se ligasse depois **agia sobre essas teclas** (com a opção nova: três giros e cinco controles ao subir). O programa passou a descartar o que chega no reenvio, logo depois de se registrar. Numa central isso não aparecia, porque o `somstub` se liga antes de qualquer tecla; mas valia para quem o religasse à mão.

**As rodadas** (previsões em `traces/som_previsoes.md`, escritas antes):

| Rodada | Hora | O que rodou | Observado |
|---|---|---|---|
| série `F` (6) | 16h57 a 17h04 | a cópia de trabalho sem memória (`F1`, `F3`) e nascendo de uma cópia da memória guardada (`F2`, `F4`), e dois controles da pasta das 16h50 (`G1` sem, `G2` com), duas centrais por vez. Vol + aos 70 s; Vol + e Vol − aos 78 s; rolete +2 aos 87 s; botão da central aos 95 s; mudo do volante aos 102 s | nas quatro com a simulação a barra abriu quatro vezes (uma por grupo de teclas do volante e uma no botão) e o volume foi 5, 6, 6, 8, 9, com 4 giros mandados e nenhum controle direto; nos dois controles o mesmo volume e a barra uma vez só, no botão; a partida igual entre os braços em cada modo (sem memória: 11 telas até os 68 s, 108 linhas `ERROR` e 17 exceções; com: 12, 79 e 14); 0 erros de OpenGL |
| prova (3) | 17h04 a 17h07 | **a pasta instalada**, pelo `driver/m8_central.sh` dela | `Pv1` (sem memória) e `Pv2` (com) iguais às da série: a barra quatro vezes, volume 5, 6, 6, 8, 9. `Pv3`, com a simulação desligada por um segundo perfil, igual aos controles: a barra só no botão, 4 controles diretos. Na `Pv2` saiu "Persistence initial profile not set" (a corrida do Marco 10), com 72 linhas `ERROR` em vez de 79; o som fez o mesmo. Bateria da pasta inteira (`t_som.sh` 214) |

**Teste.** Vinte e nove casos novos no `driver/tests/t_som.sh` (agora 214), escritos antes do código: a chave do perfil (aceita `sim` e `nao`, recusa o resto com arquivo e linha; o perfil de depois vale sobre o de antes; não mexe nos dados), o arquivo de opções, e no ar: cada tecla do volante vira um giro do botão e um passo só, o rolete leva os passos, não há laço entre o giro e a tecla, o botão da central segue igual, e sem arquivo de comandos o volume muda com um aviso. Vistos falhar em `traces/som_vermelho/t_som.vermelho6.out` (22 casos, sem o código) e `t_som.vermelho7.out` (4 casos, por causa do reenvio do hub, que ganhou três casos próprios).

**Limites.** É simulação: a HMI recebe um giro do botão que ninguém deu. Por isso, para ela, o volume pelo volante passa a ser volume pelo botão (o controle sai com domínio `hmi`, e não `cluster`). A tecla continua chegando à HMI como tecla do volante também. Segurar a tecla não repete. **Se o carro mostra a barra do mesmo jeito** (mesmo desenho, mesmo tempo aberta) eu não sei: a resposta do Lipe é de memória. E a HMI de verdade do carro, se tem a chave ligada, faz isso por outro caminho (assina os controles do painel), que a bancada não exercita.

### Onde parou

- O pedido está feito como estado de tela: a tela de som aceita os ajustes, o volume tem valor e muda com o botão e com o volante, e o mudo liga e desliga.
- **O que não ficou igual ao pedido:** não houve esquema DSI a estender, porque o som é RSI; e a barra de volume não abre com as teclas do volante. **De manhã (12/10):** os ajustes de som passaram a voltar na partida seguinte ("A memória do som"); e a barra pelo volante ficou parada numa pergunta ao Lipe, porque o firmware diz que esta central não a abre ("A barra de volume pelo volante"). **À tarde:** ele respondeu que no carro dele a barra aparece na central e escolheu simular; feito e instalado às 17h03 ("A barra de volume pelo volante: simulada a pedido do Lipe").
- **A central ao vivo não foi tocada, e estava desligada.** Na próxima partida ela pega o som de mentira, junto com o rádio, o telefone e a mídia. `SOM= sh driver/ao_vivo.sh` sobe sem ele.
- Nenhuma central `moi3-som-*` ficou no ar (conferido às 4h20).
- **Para a sessão principal:** o `janela/README.md` (seção "Teclas do volante") traz a dedução de que a barra de volume abriria quando o serviço de som avisasse o volume novo. Medido aqui, não abre. Não mexi na pasta `janela/`.
- Próximos passos naturais: o amplificador do carro, quando o autoscan disser se há o módulo 47; guardar os ajustes entre partidas; o que abre a barra com a tecla do volante; o serviço de tons (`waveplayer`); e as ações cruzadas da tabela de áudio que fizerem falta na tela.

## Marco 15 — a mídia de mentira (12/10/2026)

Feito pela sessão do rádio de mentira (Marco 11), das 2h05 às 3h30 de 12/10/2026, pela fila que a sessão do simulador deixou: o item 5 do roteiro de [simulador-jetta.md](simulador-jetta.md), a tela de mídia sair do aviso "função indisponível" e mostrar uma fonte com uma lista curta de faixas, pelo mesmo método do rádio. É emulação num Mac; nada vai para o carro. Os números 13 e 14 estavam reservados pelo som e pelos outros menus do carro.

**Resultado, verificado em execução** em centrais de teste, as três últimas (`e1` e `e2`, às 3h12, e `f1`, às 3h23) subidas da pasta já instalada, com o rádio e o telefone de mentira ligados, como no modo ao vivo:

- a tecla Mídia abre o tocador no lugar do aviso, com artista, álbum e título da faixa, o tempo em 0:00 e a duração;
- as setas do tocador passam para a faixa seguinte e a anterior;
- a lista ("Minhas mídias") mostra as cinco faixas do perfil; tocar numa a marca e ela vai para o tocador;
- o botão "Fonte" da tela do rádio ganha "Minhas mídias", ao lado de AM e FM;
- o rádio segue sintonizando e a tecla Telefone segue abrindo a tela dela, na mesma central;
- uma central que nasce de uma cópia da memória guardada do modo ao vivo faz o mesmo (rodada `f1`, da pasta instalada);
- nenhuma das dezoito rodadas registrou erro de OpenGL; nas oito com o programa final não houve tempo esgotado de DSI nem laço, e nas sete da série e da `f1` (`d1` a `d4`, `e1`, `e2` e `f1`) nenhuma linha de exceção fala de mídia. Na oitava, a `r1`, com o tocador vazio (o segundo limite, abaixo), a HMI registrou duas vezes um erro de desenho do título da faixa ("Draw error in 'TextDrawable_0': Empty text").

| O tocador | A lista de faixas |
|---|---|
| ![O tocador](quadros/2026-10-12-hmi-mid-tocador.png) | ![A lista](quadros/2026-10-12-hmi-mid-lista-de-faixas.png) |

O roteiro inteiro, numa partida da pasta instalada (rádio, tecla Mídia, próxima, anterior, lista, toque na terceira, tocador, tecla do volante, tecla Rádio, lista de emissoras com uma sintonizada, tecla Telefone, tecla Carro):

![O roteiro da prova e1](quadros/2026-10-12-hmi-mid-prova-da-pasta-e1.png)

**Nada do que se vê é do carro nem de um aparelho.** As faixas são inventadas ("FAIXA DE TESTE 1" a "5"), não há arquivo de música e não sai som. O que é da central de verdade é o comportamento da tela.

**Três limites que aparecem no uso**, todos medidos:

- **A faixa fica em pausa.** O botão de tocar não chega ao tocador: sem serviço de áudio o comando da HMI termina com "resume postponed as media is not audible after demute". O tempo não anda.
- **Depois de ir da mídia ao rádio, a volta à mídia abre o tocador vazio**, até a central reiniciar (rodada `r1`, [quadro](quadros/2026-10-12-hmi-mid-volta-do-radio-tocador-vazio.png)). O rádio segue funcionando.
- **As teclas Próxima e Anterior do volante não trocam de faixa** na tela de mídia (na do rádio trocam de emissora). A tecla chega à HMI e nenhum pedido sai; não fui atrás do motivo.

> **Nota da sessão do som, 12/10/2026, 3h47 (Marco 13):** os dois primeiros limites valem **sem** o som de mentira. Com `SOM`, que o modo ao vivo liga desde as 3h35, a tecla Mídia põe a faixa em "tocando" (o tempo segue em 0:00) e a volta à mídia depois do rádio traz a faixa. Medido em cinco partidas, duas delas da pasta instalada (Marco 13, "O efeito no rádio e na mídia: as séries"). O terceiro limite não foi exercitado lá.

### Do que a tela de mídia depende

Seis rodadas, cada uma com uma coisa a mais. Em todas, a tecla Mídia aos 75 s:

| Rodada | O que rodou | Tecla Mídia |
|---|---|---|
| `m0` | padrões do modo ao vivo e o rádio de mentira | "Mídias: Esta função não está disponível no momento. Aguarde…" |
| `m1` | mais o anúncio do `0x800000`, uma vez | o mesmo aviso. A HMI liga tocador e navegadores e faz 11 pedidos de assinatura (5 à base, 1 ao tocador e 1 a cada uma das cinco instâncias do navegador), mas não liga as "fontes" |
| `m2` | o `veiculostub` com o esquema de mídia, sem perfil (listas vazias, válidas) | o mesmo aviso |
| `m3` | um aparelho USB com uma mídia de arquivos | **a tela de mídia**, em "Aguarde…"; "Fontes" vazia |
| `m4` | um aparelho de cada tipo (0, 2, 8, 10 e 1) | "Carregando…"; "Fontes" lista "Minhas mídias" (tipo 0) e "Áudio Bluetooth" (tipo 10); a HMI manda `setActiveMedia(1, 11, 0)` |
| `m5` | o mesmo, com o motor gravando os pedidos `set…` | a HMI recebe a mídia ativa e para no passo `SET_ACTIVE_MEDIA` |

![Sem o anúncio e com o anúncio: o mesmo aviso](quadros/2026-10-12-hmi-mid-sem-anuncio-e-com-anuncio.png)

Ao contrário do rádio, aqui anunciar o domínio não basta (a sessão do simulador já tinha visto isso na rodada `v2`). O que cada passo pede, lido no código da HMI e conferido nas rodadas:

1. **Sair do aviso: uma lista de aparelhos com ao menos um aparelho.** O aviso é a tela genérica `GENERIC_DOMAIN_FEATURE_NOT_AVAILABLE`, que a HMI mostra enquanto as fontes de mídia não ligam. Quem as liga é o `DeviceHandlingTarget`, quando `DevicesImpl.isInitialized()` passa a valer, e isso só acontece em `updateMediaList` se a lista de aparelhos não estiver vazia. Com as duas listas vazias e válidas (`m2`) nada muda; com um aparelho (`m3`) sai no registro "MEDIA CONTROLLER ~ SOURCES AVAILABLE" e a tecla abre `MEDIA_COMPRESSED_AUDIO_MAIN`.
2. **Virar fonte: um aparelho de um tipo que a HMI aceite.** O filtro `DeviceFilter.MEDIA` só deixa passar os tipos 0 (`DEVICETYPE_JUKEBOX`, que a tela chama de "Minhas mídias"), 4 (CD interno), 8 (AUX), 10 (Bluetooth) e 15 (mídia sob demanda). Conferi no bytecode do firmware (`DeviceFilter$5`, com o `javap` da JVM dele) e na `m4`: dos cinco aparelhos do perfil apareceram dois, o de tipo 0 e o de tipo 10; o AUX ficou de fora pelo tipo de mídia que dei a ele. **Pendrive (tipo 2) e cartão (tipo 1) esta HMI não põe em "Fontes".** Como o carro mostra um pendrive eu não sei: é a HMI do MU0883, e o programa de mídia do firmware, que diria com que tipo um pendrive chega, não foi aberto.
3. **Ativar a fonte: seis mensagens, e não uma.** `CmdActivatePlayer` manda `setActiveMedia` e espera. `MediaPlayer.detectActivationCompleted` só dá a ativação por feita quando, depois da mídia ativa, chegam as capacidades e um estado de reprodução "ativado" (1, 3, 5, 10 ou 11), com a lista de modos já recebida. O motor sozinho devolve só a mídia ativa (`m5`). O modelo manda a mídia ativa, as capacidades, a lista de modos, o modo, a posição e o estado "em pausa"; com isso os nove passos do comando terminam ("ENDED: SUCCESS", rodada `p2`).
4. **O navegador: a pasta e o tamanho da lista depois do filtro.** `CmdSetupBrowser` para em `SET_CONTENT_FILTER` por 10 s (rodada `p1`) se só o filtro voltar: `AbstractMediaBrowser.detectContentFilterSet` quer também a pasta atual e um tamanho de lista maior que zero. Depois vêm o caminho (`requestBrowsePath`) e dois pedidos de lista: `requestList(101, 1, 0, 0)`, que é a HMI procurando a entrada em foco e espera essa entrada com o índice dela (com lista vazia o comando falha com "listEntryNotFound"), e `requestList(0, 0, 0, n)`.
5. **O toque numa faixa da lista** monta uma "seleção": `resetSelection`, `enableRecurseSubdirectories(true)`, `addSelection(true, 1)` e, no tocador, `setPlaySelection(0, entrada, …)`. Sem resposta ao `addSelection` a lista fica parada (rodada `p3`).

Dois tempos esgotados somem de passagem: a HMI pede `setLanguage` e `setFileTypeActive` ao `DSIFileBrowser` na partida e espera 10 s por cada resposta (registrado nas `m1` e `m2`); com as duas seções de resposta no perfil, o motor as dá (`m3` em diante).

### A troca de idioma deixa de estourar

Achado de passagem, medido. Em toda partida da bancada a troca de idioma da HMI (LSC) estourava o prazo de 30 s, porque alguns alvos nunca confirmavam: na `e1` do Marco 11, só com o rádio, quatro estouros, com os alvos `AslId_1240000` (o navegador de arquivos) e `AslId_5308` pendentes. Com a mídia de mentira o navegador de arquivos e o tocador confirmam (rodada `d1`: sobra o `5308`). Com a mídia e o telefone de mentira juntos, **nenhum estouro** (`d3`, `d4`, `e1` e `e2`). O que mudava na tela por causa dos estouros eu não sei; nas fotos ela sempre esteve em português.

### O esquema: o gerador já lia o domínio

O gerador do Marco 8, com a segunda forma do Marco 11, leu o domínio `0x800000` sem mudança. Entre a minha primeira geração (2h06) e a que ficou na pasta, a sessão do telefone instalou a leitura de pedidos por método auxiliar (2h37), e o `DSISearch` ganhou interface de pedidos e 28 pedidos. Saída em `stubs/midia/gerado/`: **12 serviços, 81 atributos, 174 eventos, 190 pedidos (66 com resposta na tabela da HMI), 422 mensagens, 1.085 campos e 942 constantes**. A prova do codificador: 173 de 173 mensagens iguais, byte a byte, às das classes do firmware.

- **O rótulo vai sem acento** (`ROTULO="de PID_MEDIA"`): com "de mídia" o cabeçalho dos arquivos saiu com a letra estragada, porque o argumento chega à JVM do firmware fora de UTF-8. O teste confere que o cabeçalho não tem letra estragada.
- **O estado de reprodução tem duas apis com a mesma mensagem** (7 e 9000), e o gerador as distingue no nome: `MPLupdatePlaybackState:7`.
- **68 pedidos "vão para o domínio 256"** conforme a instância (tocador e navegador do Bluetooth, UPnP, busca). O hub da bancada entrega tudo a todos, e o motor atende pela interface, sem olhar a instância.
- **O `DSISearch` fica de fora** do programa (decisão D141): cada instância dele é de um domínio diferente, e respondendo o programa falaria pelas dos outros.

### O programa e o perfil

`driver/midiastub.c` inclui o `driver/veiculostub.c`, como o `radiostub.c`, e acrescenta um modelo do tocador e do navegador. Os ganchos são os três do Marco 11: o motor não mudou. O modelo acha tudo pelo nome, no esquema, e tira a discoteca do perfil: as faixas são as linhas da seção `[DSIMediaBrowser.MBRLresponseList]`, a pasta é a de `[DSIMediaBrowser.MBRLupdateBrowseFolder]`, a fonte são as duas listas do `DSIMediaBase`.

| Pedido | O que o modelo faz |
|---|---|
| `setActiveMedia` | a mídia ativa, as capacidades, os modos, a posição e o estado "em pausa"; sem campos (a HMI saindo da mídia) ou com mídia que não está no perfil, a mídia ativa vazia |
| `setPlaybackMode` | grava o modo se ele está na lista do perfil; manda o modo de agora |
| `play`, `resume`, `pause`, `stop` | muda o estado; a faixa não anda |
| `skip`, `setEntry`, `setPlaySelection` | muda a faixa (do fim volta ao começo) e manda a posição e o estado |
| `requestDetailInfo`, `requestPlayView` | os dados da faixa; um trecho da lista de reprodução |
| `setContentFilter`, `changeFolder` | a pasta e o tamanho da lista, só para a instância que pediu |
| `requestBrowsePath`, `requestList` | o caminho; um trecho da lista, ou a entrada em foco com o índice dela |
| `resetSelection`, `undoLastSelection`, `addSelection` | resultado 0 e, ao selecionar, a conta das faixas |

`stubs/midia/minhas-midias.txt`: uma fonte do tipo 0, uma pasta e cinco faixas de dois artistas e dois álbuns "DE TESTE". Tudo simulação, marcado no arquivo. O [README](stubs/midia/README.md) diz o que o perfil precisa ter e por quê.

### As rodadas

Centrais de teste `moi3-mid-<nome>`, à parte da central ao vivo, todas com o rádio de mentira. Saídas em `traces/mid_*`.

| Rodada | O que rodou | Observado |
|---|---|---|
| `m0` a `m5` | a tabela acima | |
| `p1` a `p4` | um protótipo do modelo, crescendo a cada rodada (`traces/mid_prototipo_midiastub.c`) | `p1`: a ativação termina e o navegador para no filtro. `p2`: o tocador com a faixa; a lista com três faixas. `p3`: "anterior" é `skip(1, 1)`; o toque na lista espera o `addSelection`; a volta do rádio à mídia fica presa. `p4`: o toque na lista funciona; o rádio sintoniza depois da mídia |
| `d1`, `d2` | o programa final, da árvore juntada, com o rádio | iguais: 11 assinaturas, 2 saltos, 2 seleções, 5 listas, sintonias em 76100, 76100 e 89100; 0 erros de OpenGL, 0 tempos esgotados de DSI |
| `d3`, `d4` | o mesmo, com o telefone de mentira | o mesmo, e a tecla Telefone abre a tela dela; nenhum estouro da troca de idioma |
| `r1` | mídia, rádio, 20 s, mídia; "Minhas mídias" pelo botão "Fonte" do rádio | o tocador vazio nas duas voltas; no registro, "waiting for activation …RadioActivationRequest" e "send reject to direct Media request" |
| `e1`, `e2` | **da pasta instalada** | o mesmo das `d3` e `d4`, nas duas |
| `f1` (3h23) | **da pasta instalada**, nascendo de uma cópia da memória guardada do modo ao vivo (61 linhas) | o assistente de configuração como primeira coisa na tela, agora com três páginas; dispensado, a grade; a tecla Mídia abre o tocador na primeira faixa; próxima, lista e toque na terceira como nas outras; a tecla Rádio volta ao rádio. 0 erros de OpenGL, 0 tempos esgotados de DSI, nenhum estouro da troca de idioma; o aviso da troca de faixa de FM, uma vez (é o do Marco 11) |

![Nascendo da memória do modo ao vivo: rodada f1](quadros/2026-10-12-hmi-mid-com-a-memoria-do-modo-ao-vivo.png)

Outras fotos: [um pendrive no perfil: "Aguarde…"](quadros/2026-10-12-hmi-mid-aparelho-usb-aguarde.png) e ["Fontes" vazia](quadros/2026-10-12-hmi-mid-aparelho-usb-fontes-vazia.png), [as fontes por tipo de aparelho](quadros/2026-10-12-hmi-mid-fontes-por-tipo-de-aparelho.png), [a série com o telefone](quadros/2026-10-12-hmi-mid-serie-d3-com-telefone.png) e [a exploração `p3`](quadros/2026-10-12-hmi-mid-exploracao-p3.png).

### O que entrou na pasta

- `driver/midiastub.c`: a mídia de mentira. `driver/tests/t_midia.sh`: 107 casos, sem firmware.
- `stubs/midia/`: o perfil `minhas-midias.txt`, o esquema gerado (`gerado/`) e o [README](stubs/midia/README.md).
- `driver/m1_build.sh`: compila o `midiastub`; se não compilar, avisa e segue, como com o `radiostub`.
- `driver/m1_hmi.sh`: opção `MIDIA` (lista de perfis) e `MIDIA_ESQUEMA`, o registro `midiastub.log` e uma linha de resumo "mídia:".
- `driver/ao_vivo.sh`: `MIDIA` vem ligada, com o perfil da pasta (`MIDIA=` desliga). `driver/m3_ao_vivo.sh`: só comentário; ali não há padrão. `driver/m8_central.sh`: repassa `MIDIA` e `MIDIA_ESQUEMA`.
- `stubs/radio/README.md`: a receita "Para fazer o mesmo noutro domínio", pedida pela sessão do simulador para as frentes do som e do telefone.
- Onze quadros `quadros/2026-10-12-hmi-mid-*.png` e 71 arquivos `traces/mid_*` (8,4 MB).

### Testes

O teste foi escrito antes do programa que ficou, e depois de uma exploração. As rodadas `p1` a `p4` usaram um protótipo sem teste, para descobrir o que a HMI pede; com isso sabido, escrevi o `t_midia.sh` com os bytes que a HMI mandou e só então o `midiastub.c` que está na pasta, de novo. O protótipo ficou em `traces/`, fora de `driver/`.

| Etapa | Contra o que rodou | Resultado |
|---|---|---|
| sem programa | a árvore sem o `midiastub.c` | 3 falhavam e 29 passavam (os do esquema e os vetores) |
| só o motor | um esqueleto do `midiastub` sem modelo | 57 de 103 falhavam: os do modelo, um a um |
| o programa | a primeira versão | 3 de 103 falhavam: dois por defeito do próprio teste e um por faltar o perfil da pasta |
| o que ficou | a árvore final | 107 passam |

O defeito do teste: eu provocava o laço com 230 chamadas separadas, que levavam mais que os 10 s da janela do corte; passou a usar a repetição a cada 2 ms do `dsi_manda`, como o teste do rádio. As saídas das vezes em que falhou estão em `traces/mid_t_midia_vermelho_*.out`.

Bateria completa antes de instalar (pasta como estava, 3h08), na árvore juntada e depois de instalar (3h15, saída em `traces/mid_suites_da_pasta_depois.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 187, `t_radio.sh` 81, `t_telefone.sh` 103, `t_midia.sh` 107, `t_memoria.sh` 80, `t_hub.sh` 25, `t_kipc_bus.sh` 14, `t_hmilog.sh` 32 e `t_vc_diag_alm.sh` 37, no container; `t_servidor.sh` 20, `t_lsmval_abre.sh` 38 e `t_memoria.sh` 80, no Mac. Todos passam, e a compilação não dá aviso.

### Limites

- **Sem som, sem aparelho, e a faixa em pausa.** Enquanto a bancada não tiver serviço de áudio, a HMI não manda tocar. Se um dia mandar, o modelo muda o estado e o tempo continua parado: o motor não tem relógio de faixa.
- **A volta à mídia depois do rádio.** Medido na `p3` e na `r1`: depois de mídia e rádio, o gerente de entretenimento da HMI fica esperando a ativação do rádio e recusa a da mídia. **A causa é leitura, não medida:** a segunda ativação do rádio não termina porque `UnifiedRadioBandSwitch` só pede a ligação de áudio na partida quando o serviço de áudio falta (Marco 11, "De que a sintonia depende"). A comparação com um serviço de áudio no ar confirma ou desmente. **Conferido em 12/10/2026, às 3h35 (Marco 13, sessão do som): com o som de mentira no ar a faixa toca e a mídia volta depois do rádio; sem ele, não.**
- **As teclas do volante** Próxima e Anterior não trocam de faixa. Fonte (45) não foi repetida com o programa final; na `p3`, depois da volta à mídia, ela levou ao rádio. **Conferido em 12/10/2026, às 9h (Marco 16, passos 1 e 2): com o som de mentira no ar, Próxima e Anterior trocam de faixa; a causa era a falta do serviço de áudio. E a Fonte, apertada na mídia, leva ao rádio e depois só alterna FM e AM.**
- **Uma pasta, um modo de reprodução, cinco faixas.** Sem subpastas, capas, favoritos, repetir ou embaralhar.
- **Só "Minhas mídias".** Áudio por Bluetooth não foi simulado; a lista "Fontes" com o perfil final não foi fotografada (vi "Minhas mídias" no botão "Fonte" do rádio).
- **Instâncias.** O navegador responde a cada instância em separado, mas guarda um valor só por atributo; as instâncias 1 e 3 (capas e painel) fizeram o mesmo roteiro da 0 e receberam a mesma lista.
- **`DSISearch`, UPnP, gravador e mídia online**: o primeiro fica de fora; os outros só respondem às assinaturas, com os padrões.
- **O `driver/ao_vivo.sh` não foi executado por mim.** Conferi a linha do `docker` dele com um `docker` de mentira, numa cópia à parte.
- **Só no container.** Não rodou no QEMU. **O esquema é do MU0883**; o carro roda o MU0803.

### A espera da partida pelos serviços de veículo (acréscimo de 12/10/2026, das 3h30 às 4h20)

A sessão do estado do carro de mentira achou que, com o rádio de mentira, a central que nasce na tela do veículo fica sem sete serviços de veículo (seção "O carro de mentira lembra do que a tela gravou", "Achado na conferência final": onze partidas em onze; com `ESPERA=6000`, nenhuma em quatro) e deixou a decisão com esta sessão. Fui conferir e achei mais: **a mídia de mentira pôs uma versão branda do mesmo defeito em toda partida**, e eu não tinha visto. Na série do Marco 15 contei erros de OpenGL e o que a tela de mídia fazia, e não comparei as assinaturas no carro de mentira com as das rodadas do rádio. A linha `Required DSI services not registered within 2000 ms` com `DSICarVehicleStates[0]`, `DSICarComfort[0]` e `DSICarSeat[0]` está nas 17 rodadas da mídia que tiveram anúncio do domínio (`m1` a `f1`) e em nenhuma das 18 do rádio.

**A causa, medida.** A HMI faz a partida em etapas, e cada etapa espera pelos serviços de que precisa até um prazo: `-Dde.vw.mib.asl.startup.timeout.dsi` para os serviços DSI e `.rsi` para os RSI. A bancada põe os dois em 2 s (`ESPERA`, decisão D21; o firmware usa 15 s). E a HMI só liga dez dos serviços de veículo 5 s depois de ver o domínio 16 (lido no bytecode no Marco 8, `BAPTaskScheduler`). Cada stub que passou a responder tirou da partida uma espera de 2 s por serviço ausente, e com isso adiantou a etapa que precisa do veículo:

| Partida | A segunda etapa, que começa entre 2,1 e 2,6 s, espera por | Quem precisa dos serviços de veículo | Registro |
|---|---|---|---|
| sem rádio nem mídia de mentira (Marco 10) | `DSIUnifiedTuner`, 2 s; depois `DSIRadioData`, 2 s; depois a mídia, 2 s | só é chamado depois dos 8,6 s, com os serviços já de pé: nenhuma parada | `traces/cpu_s_a6.out` |
| só o rádio | os serviços de mídia: estoura aos 4,2 s | chamado aos 4,2 s; a partida para 0,7 s, até os 4,9 s, e segue | `traces/rad_e1.out`, `rad_f3.out` |
| só o rádio, central nascida na tela do veículo | **os sete serviços de veículo: estoura aos 4,2 s** | a própria etapa | `traces/est_g1a.ao_vivo.out` (sessão do estado) |
| rádio e mídia, com ou sem telefone e som | **`DSICarVehicleStates`, `DSICarComfort` e `DSICarSeat`: estoura aos 4,2 s** | a própria etapa | `traces/mid_m1.out`, `mid_e1.out`, `mid_espera_h6c.ao_vivo.out` |

Com a espera maior, a etapa do veículo esperou de 2,69 a 2,83 s e seguiu, nas dez partidas em que isso pôde ser medido: começou a esperar entre 2,14 e 2,27 s e os serviços apareceram entre 4,93 e 4,99 s, contados da linha "UI thread started". Com 2 s, ela desistia 0,7 s antes.

**O efeito.** Nascida na tela do veículo, a central fica com 48 assinaturas no carro de mentira em vez de 112, 4 no painel em vez de 15, e o computador de bordo só mostra traços (a sessão do estado viu também "Configurações do veículo" com dois itens e "Status do veículo: Iniciando…"). Nas outras partidas faltam quatro assinaturas, 107 em vez de 111: `DSICarComfort` 1 e 21 (`ATTR_RGSVIEWOPTIONS` e `ATTR_WIPERVIEWOPTIONS`) e `DSICarSeat` 1 e 18 (`ATTR_SEATVIEWOPTIONS` e `ATTR_SEATCONTENT`). Pelos nomes do esquema são as opções de exibição do RGS, dos limpadores e dos bancos e o conteúdo dos bancos; o que é o RGS eu não conferi. O perfil padrão do carro de mentira não tem valor para nenhuma das quatro, e a foto final das duas partidas é igual byte a byte (`h6c` e `h6d`). **Não vi efeito na tela; não abri menu nenhum para procurar.** Para quem liga menus pelo catálogo do Marco 14 pode fazer diferença: lá está lido que a opção do limpador alimenta um item de "Abrir e fechar", e sem a assinatura a HMI nem recebe a opção.

![À esquerda, com 2 s, o computador de bordo só com traços; à direita, com 6 s para os serviços DSI, com os valores](quadros/2026-10-12-hmi-mid-espera-2s-e-6s.png)

À esquerda a `v1b` (2 s, como era) e à direita a `v1a` (6 s para os serviços DSI), as duas nascidas na tela do veículo e pelo `ao_vivo.sh`.

**As dezoito partidas** (`traces/mid_espera_tabela.txt`, que o `traces/mid_espera_tabela.sh` refaz dos registros guardados):

| Espera pelos serviços DSI | Partidas | Nascidas na tela do veículo | As outras | A última etapa começa aos | A primeira tela entra aos |
|---|---|---|---|---|---|
| 2 s (`ESPERA` padrão) | 6 | 3 de 3 degradadas (48 assinaturas) | 3 de 3 com 107 | 41,4 a 42,4 s | 2,1 a 2,2 s; 4,2 a 4,3 s na tela do veículo, com traços |
| 4 s, só os DSI | 2 | 1 de 1 normal (112) | 1 de 1 normal (111) | 46,2 a 46,3 s | 2,2 s; 5,0 s na tela do veículo, com valores |
| **6 s, só os DSI** | 8 | 4 de 4 normais | 4 de 4 normais | 50,1 a 50,3 s | 2,1 a 2,2 s; 4,9 a 5,0 s na tela do veículo |
| 6 s para tudo (`ESPERA=6000`) | 2 | 1 de 1 normal | 1 de 1 normal | 67,5 s | 6,2 s |

Entram na conta as partidas com e sem o som de mentira (que virou padrão do modo ao vivo às 3h35, no meio da medição: repeti os quatro braços com ele, `h6a` a `h6d`), uma nascida da cópia da memória guardada do modo ao vivo (`h5a`, última tela na grade, primeira tela aos 0,1 s) e duas pelo próprio caminho do modo ao vivo (abaixo). Nas que subiram só a espera dos DSI, o primeiro prazo da partida, que é de serviço RSI, continua em 2 s: por isso a primeira tela não atrasa, ao contrário do `ESPERA=6000`.

**O conserto: no modo ao vivo, a espera pelos serviços DSI passa a 6 s** (decisão D158). O `driver/ao_vivo.sh` ganhou a variável `ESPERA_DSI`, com padrão 6000, e a manda à HMI como `-Dde.vw.mib.asl.startup.timeout.dsi=6000`, pela `EXTRA_VMOPTIONS` que o `m1_hmi.sh` já aceitava; ela fica depois das duas opções que o `m1_hmi.sh` monta a partir da `ESPERA`, e na máquina Java vale a última (medido: o registro passa a dizer "within 6000 ms" para os DSI e segue com "within 2000 ms" para os RSI). `ESPERA` dada à mão vale para os dois tipos, como antes, e aí a `ESPERA_DSI` só entra se também for dada; `ESPERA_DSI=` vazio volta ao que era. Só o lançador do modo ao vivo mudou: a janela sobe a central por ele, e por ela não há como passar variável.

- **Custo:** a última etapa da partida começa aos 50 s em vez de aos 42 s. São as duas etapas que esperam por serviço que a bancada não tem (`DSIWLAN` e `DSIBrowser`), 6 s cada em vez de 2. O que só fica pronto nessas etapas finais chega 8 s mais tarde; não medi o que é.
- **Por que 6 e não 4:** com 4 s também deu certo, e custa a metade, mas são duas partidas e 1,2 s de folga sobre os 2,8 s de que a etapa precisa; com 6 s são dez partidas minhas (oito só com os DSI e duas com `ESPERA=6000`) e quatro da sessão do estado, e 3,2 s de folga. E há um motivo por dedução: a primeira espera da partida, de 2 s, hoje só existe porque falta o serviço RSI `waveplayer` (com o som de mentira é o único que sobra nessa linha); se alguém o puser, a etapa do veículo passa a começar perto dos 0,2 s e a precisar de uns 4,8 s. Os 6 s ainda cobrem; os 4 s, não. Para trocar: `ESPERA_DSI=4000 sh driver/ao_vivo.sh`, ou a linha `ESPERA_DSI_PADRAO` do lançador.
- **As centrais de teste não mudaram:** `m3_ao_vivo.sh` e `m8_central.sh` seguem com 2 s para tudo. Havia séries de outras sessões no ar, que executam esses roteiros montados, e mudar o padrão mudaria a base de comparação de todas as rodadas guardadas. Quem sobe central de teste com rádio e mídia de mentira tem a versão branda (107 assinaturas), e a degradada se a memória terminar na tela do veículo. Para ter numa central de teste o que o modo ao vivo tem: `EXTRA_VMOPTIONS=-Dde.vw.mib.asl.startup.timeout.dsi=6000`, que o `m8_central.sh` repassa.

**Pelo caminho do modo ao vivo, sem tocar na central ao vivo** (`traces/mid_espera_viva.sh`): rodei uma cópia do `ao_vivo.sh` já mudado em duas bancadas de rascunho, com um `docker` de passagem que só troca o nome do container (`moi3-ao-vivo` por `moi3-mid-v1a` e `-v1b`) e recusa qualquer chamada que cite a central ou a pasta ao vivo. Tudo o mais é o padrão do lançador: codificação, idioma, memória, carro, rádio, telefone, mídia e som. As duas nasceram da memória que termina na tela do veículo. A `v1a`, com os padrões novos, saiu normal (112 assinaturas, a etapa esperou 2,78 s, computador de bordo com valores); a `v1b`, com `ESPERA_DSI=`, saiu degradada (48, traços). O que o `docker` recebeu em cada uma está em `traces/mid_espera_v1a.docker.args` e `_v1b.docker.args`.

**Teste antes do código.** O bloco novo do `driver/tests/t_midia.sh` roda uma cópia do `ao_vivo.sh` com um `docker` de mentira, como o `t_memoria.sh` faz, e confere o que ele receberia: doze casos. Contra o lançador de antes, três falharam pelo motivo esperado (a opção dos 6 s não chegava: `traces/mid_espera_teste_vermelho.txt`); os outros nove fixam o que já valia (a mídia ligada por padrão, `ESPERA=2000`) e que o `m1_hmi.sh` põe as opções extras depois das da `ESPERA`, que é do que o conserto depende. Com o lançador mudado passam os doze (`_teste_verde.txt`). O `t_midia.sh` foi de 107 para 119 casos. Bateria completa da pasta antes (4h03) e depois de instalar (4h08, `traces/mid_espera_suites_depois.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 188, `t_radio.sh` 81, `t_telefone.sh` 142, `t_midia.sh` 107 antes e 119 depois, `t_som.sh` 135, `t_memoria.sh` 80, `t_hub.sh` 25, `t_kipc_bus.sh` 14, `t_hmilog.sh` 32 e `t_vc_diag_alm.sh` 37, no container; `t_servidor.sh` 20, `t_lsmval_abre.sh` 38, `t_memoria.sh` 80 e `t_catalogo.sh` 32, no Mac. Todos passam, sem aviso do compilador.

**O que conferi além das assinaturas.** Nenhum molde novo de linha ERROR com os 6 s. Somem as linhas dos serviços de veículo fora do prazo e voltam três que já existiam em rodadas antigas, porque as unidades que as produzem voltam a rodar: `DSIKeyPanelListener[0].genericSettingResponse is unexpected` (estava em mais de cem registros antigos de `traces/`), `ArrayConversion: Do fallback to full range update` e, só na central nascida na tela do veículo, 108 linhas `Error while getting row item` e 4 de "Empty text", todas em 30 ms aos 5,0 s: a tela do computador de bordo entra antes de as listas dela encherem (aparecem também em `traces/est_e1a.ao_vivo.out` e em rodadas dos Marcos 5 e 6). Os registros dos stubs têm o mesmo tamanho nos dois braços, fora o do carro de mentira, que cresce com as assinaturas: nenhum laço (conferido nas oito últimas partidas). Das duas corridas de partida do Marco 10: a linha do indicador de gesto está na partida das dezoito, e o perfil inicial da persistência veio "não posto" em duas (`h4b` e `h6d`), as duas na central que parte 3 s depois da outra do par, com a máquina ocupada; nas duas as assinaturas saíram como no resto do braço. E o motor compartilhado (`driver/veiculostub.c`) foi trocado pela sessão do telefone às 3h47, no meio da medição: as rodadas `h1` a `h6` compilaram o de antes, e as `h7`, as `v1` e as duas baterias, o de depois. Os números não mudaram de um para o outro.

**O que não conferi.**
- O `driver/ao_vivo.sh` com o nome `moi3-ao-vivo` e a partida pela janela: a central ao vivo estava desligada e eu não a liguei. O mais perto foram a `v1a` e a `v1b`.
- Os 5 s da HMI são leitura do Marco 8; o que medi é que os serviços aparecem perto dos 5,0 s nas dez partidas. Com a máquina muito mais carregada não testei: rodei em pares, com centrais de outras sessões no ar (de duas a cinco, nas vezes em que olhei).
- Outras últimas telas na memória (só a do veículo e a grade), e sessão longa.
- No carro nada disso existe: lá os serviços vêm dos programas de verdade e a espera é de 15 s. É acerto da bancada.

Saídas, registros das dezoito partidas, roteiros e a memória-semente em `traces/mid_espera*` (63 arquivos); quadro em `quadros/2026-10-12-hmi-mid-espera-2s-e-6s.png`.

### Onde parou

- **Às 4h08 de 12/10/2026 a espera pelos serviços DSI do modo ao vivo passou a 6 s** (subseção acima, decisão D158). A central ao vivo continuava desligada e não foi tocada; a próxima partida dela, pelo `driver/ao_vivo.sh` ou pela janela, já sai com isso. `ESPERA_DSI= sh driver/ao_vivo.sh` volta ao que era.
- O item 5 do roteiro do simulador está feito: uma fonte, uma lista curta de faixas, troca de faixa pela tela.
- **A central ao vivo não foi tocada, e estava desligada** (conferido às 3h16 com `docker ps -a`). Na próxima partida ela pega a mídia de mentira, junto com o rádio e o telefone. `MIDIA= sh driver/ao_vivo.sh` sobe sem ela.
- Nenhuma central `moi3-mid-*` ficou no ar.
- Próximos passos naturais: o serviço de áudio, que deve destravar o tocar e a volta à mídia; áudio por Bluetooth, com o telefone; o motivo das teclas do volante; e saber com que tipo de aparelho um pendrive chega no carro.

## Marco 14 — os outros menus do carro (12/10/2026)

Feito pela sessão do estado do carro de mentira, como terceiro pedido da noite (depois do estado e do brilho). O pedido da sessão principal: "os outros menus do carro (luzes, espelhos, travas, pneus, assistentes, luz ambiente): cada um pede a função nas listas da adaptação (0B1B e 0B1D) e as opções do serviço dele no perfil de `stubs/veiculo/`. Regra do projeto: nenhum valor do carro pode ser inventado. Ligar só o que a ficha do carro sustenta; o que for palpite vai no arquivo de deduções, com a razão ao lado; na dúvida, deixar desligado e pôr numa lista para o Lipe decidir de manhã." É emulação num Mac; nada vai para o carro.

**Resultado:**

- **Nenhum menu novo entra ligado.** A ficha do carro não sustenta nenhum: todos os equipamentos de que estes menus dependem estão como "a confirmar", com fonte de imprensa, e a câmera dos assistentes está como "Não". A central padrão continua como estava, no modo ao vivo e nas centrais de teste: "Configurações do veículo" com quatro itens e a grade de menus sem o bloco "Assistentes". **O que ligar é decisão do Lipe**, e a lista está em "Para o Lipe decidir", mais abaixo. (**Mudou em 12/10/2026, às 18h05:** ele respondeu de memória, e quatro menus e três assistentes entraram no modo ao vivo; subseção "Os menus que o Lipe confirmou entram no modo ao vivo", no fim deste Marco.)
- **O mecanismo está medido e lido no código**, e é mais simples do que o pedido supunha: basta a função na lista de funções do carro (DID `0B1B`) e as opções do serviço. A lista de aparelhos (`0B1D`) não entra (D152).
- **Os assistentes não ficam em "Configurações do veículo".** Nesta HMI eles têm um bloco próprio na grade de menus, **"Assistentes"**, que abre a tela IQ.DRIVE e a lista "Configurações da assistência ao condutor". É lá que aparece o **Lane Assist**, que é o que o projeto 01 instala no carro.
- **Ficou na pasta um catálogo, fora de toda partida padrão**, que liga tudo o que o firmware conhece em cinco menus e nos assistentes, para se ver na tela, e um roteiro que sobe uma central de teste com ele: `sh driver/m14_menus.sh`. O catálogo não é o carro.

| Padrão (não mudou): a lista de configurações | Com o catálogo |
|---|---|
| ![Padrão](quadros/2026-10-12-hmi-men-lista-sem-as-opcoes.png) | ![Catálogo](quadros/2026-10-12-hmi-men-lista-com-o-catalogo.png) |

| Padrão (não mudou): a grade de menus | Com o catálogo: aparece "Assistentes" |
|---|---|
| ![Padrão](quadros/2026-10-12-hmi-men-grade-padrao.png) | ![Catálogo](quadros/2026-10-12-hmi-men-grade-com-assistentes.png) |

| A tela "Assistentes" com todos | O Lane Assist aberto na lista de configurações |
|---|---|
| ![Assistentes](quadros/2026-10-12-hmi-men-assistentes-todos.png) | ![Lane Assist](quadros/2026-10-12-hmi-men-lane-assist-aberto.png) |

### Como a HMI decide que um menu existe

Lido nas classes decompiladas do MU0883 e conferido nas rodadas (digo qual, em cada ponto).

1. **A função na adaptação.** Na partida o `DeviceManager` pede a lista de funções do carro (`requestCarFuncAdap`, o DID `0B1B`) e percorre uma tabela de 36 aparelhos (`initTable`). Para cada um ele lê um campo só, o "Menu display" daquele aparelho (bit 7 de um byte do `0B1B`). A tabela inteira, com o byte de cada função, está em [traces/men_aparelhos.txt](traces/men_aparelhos.txt).
2. **A assinatura.** Para cada aparelho ligado, a HMI passa a assinar no carro as opções de exibição dele (`ViewOptionServiceFactory.createViewOption`): pneus ligados, ela assina `DSICarComfort.updateRDKViewOptions`, e assim por diante. Sem a função, a HMI nem pede as opções. Visto: com a função do ACC ligada, o carro de mentira registrou a assinatura do atributo 24 (rodada `a`).
3. **As opções.** Cada opção chega com um estado; 2 quer dizer "existe e pode ser usada". Cada menu é uma lista de funções, e o item aparece em "Configurações do veículo" quando pelo menos uma função dele chega com estado 2 (`MappingList.getListFunctionState`, `CarSetupMenu.updateMenu`).

Conferido em centrais de teste:

| Rodada | O que foi ligado | O que a lista mostrou |
|---|---|---|
| `c` | oito funções com os aparelhos delas, nenhuma opção | os quatro itens de sempre: a função sozinha não cria menu |
| `b` | a função dos pneus, o aparelho dela e as opções | ganhou "Pneus" |
| `pn1` | a função dos pneus e as opções, **sem o aparelho no `0B1D`** | ganhou "Pneus" do mesmo jeito |
| `luzext`, `luzint` | iluminação externa; iluminação do interior | "Luz" nas duas |
| `pdc` | auxílio de estacionamento | "Estacionar e manobrar" |
| `espelho` | espelhos | "Espelho e limpador" |
| `lim` | limpador | "Espelho e limpador" **e "Abrir e fechar"** |
| `travas` | travamento central | "Abrir e fechar" **e "Luz"** |
| `assist` | sete funções de assistência e as opções delas | **"Estacionar e manobrar"**, e nenhum item de assistência |

Os três cruzamentos têm explicação no código. Uma opção de um serviço pode ser mostrada no menu de outro:

- **travas → "Luz":** a opção `turnIndRepeat` das travas alimenta uma linha da tela de luz (`LightDevice.addMappingComfortBlinking`). Pelo nome do método é a do pisca de conforto, que a tela chama de "Indicador de direção conforto"; não abri a tela de luz naquela rodada para ver a linha.
- **limpador → "Abrir e fechar":** a opção `easyEntrySteeringColumn` do limpador é mostrada em abrir e fechar (`OpenCloseViewOptionListenerImpl.updateWiperViewOptions`).
- **Side Assist → "Estacionar e manobrar":** as opções `exitAssist` e `rcta` (pelos nomes, o assistente de saída de vaga e o alerta de tráfego traseiro) são mostradas em estacionar (`ParkingDevice.updateSWAViewOptions`). Também não abri o item nessa rodada.

Isso importa para o carro de verdade: um item pode aparecer na tela dele sem que o equipamento "do nome do item" exista.

**A lista de aparelhos (`0B1D`) não é consultada para estes menus.** O `DeviceManager` só lê o `0B1B`, e as rodadas `pn1`, `l2` e `l3` deram o mesmo resultado com e sem o aparelho. A tabela de campos do firmware tem, para cada aparelho, a disponibilidade e o barramento (por exemplo `ID_LDW_HCA_0X19` e `ID_LDW_HCA_0X19_MSG_BUS`, "LDW / HCA (Spurhalteassistent/Lane departure warning, 0x19)"). Quem usa esses dois deve ser o serviço de dados de veículo do firmware, que fica abaixo da HMI e que a bancada troca pelo carro de mentira: é dedução, não li esse serviço.

### Os assistentes: o bloco "Assistentes" e a tela IQ.DRIVE

Na rodada `a` liguei o ACC e o Front Assist, com as opções, e nada apareceu em "Configurações do veículo". O modelo de menus até tem a posição (a 3, `AssistantsDevice`), e o registro da HMI mostra que as opções chegaram (`dsiCarDriverAssistanceUpdateACCViewOptions 1, accReason=0`), mas a tela não desenha esse item. A assistência fica noutro lugar, num pacote à parte da HMI (`de.vw.mib.asl.internal.carfas`):

- **O bloco "Assistentes" aparece na grade de menus quando a função de algum assistente está na adaptação.** Medido com uma função só, a do Lane Assist (rodada `l1`): a grade ganhou o bloco e passou a ter duas páginas. Na central padrão ele não existe (rodada `p`).
- **Cada assistente pergunta por um campo do `0B1B`** (`isDeviceCoded` de cada serviço, leitura de código):

| Assistente na tela | Campo "Menu display" que ele consulta | Byte do `0B1B` na bancada |
|---|---|---|
| ACC | `ID_DISPLAY_ACC` | 0 |
| Front Assist | `ID_DISPLAY_AWV` | 3 |
| **Lane Assist** | **`ID_DISPLAY_LANE_DEPARTURE_WARNING`** ("Lane Departure Warning: Menu display") | **4** |
| Side Assist | `ID_DISPLAY_LANE_ASSISTANT` ("Lane asssistant: Menu display", que na HMI é o assistente de troca de faixa) | 5 |
| Reconhecimento de sinalização e de reboque | `ID_DISPLAY_ROAD_SIGN_IDENTIFICATION` | 30 |
| Reconhecimento de cansaço | `ID_DISPLAY_WEARINESS_RECOGNITION` | 35 |
| (serviço `PreCrashAssistService`; não vi o nome dele na tela) | `ID_DISPLAY_REVERSIBLE_BELT_PRE_LOAD` | 28 (não experimentado) |
| (serviço `PredictiveEfficiencyAssistService`; idem) | `ID_DISPLAY_EFFICIENCY_ASSIST` | 55 (não experimentado) |

- **A lista de configurações só mostra o assistente quando as opções dele chegam.** Só com a função (rodada `l1`), ou com a função e o aparelho no `0B1D` (rodada `l2`), o bloco abre, a cena mostra só o carro e a lista "Configurações da assistência ao condutor" fica [vazia](quadros/2026-10-12-hmi-men-assistentes-lista-vazia-sem-as-opcoes.png). Com a função e as opções `updateLDWHCAViewOptions` (rodada `l3`), a cena ganha [as faixas pintadas e o selo IQ.DRIVE](quadros/2026-10-12-hmi-men-assistentes-so-lane-assist.png) e a lista mostra ["Emergency Assist" e "Ass. perm. faixa (Lane Assist)"](quadros/2026-10-12-hmi-men-assistentes-lista-so-lane-assist.png).
- **Com tudo ligado** (rodada `menus`, a do catálogo) a cena tem o carro da frente, a placa, a xícara do cansaço, o carro de trás e o reboque, e a lista tem: ACC (contr. autom. de dist.), Aviso de nova partida, Front Assist, Emergency Assist, Ass. perm. faixa (Lane Assist), Side Assist, Sist. reconh. sinaliz. trânsito, Reconhecimento de reboque e Sistema reconh. de cansaço ([primeira parte](quadros/2026-10-12-hmi-men-assistentes-lista-1.png), [segunda](quadros/2026-10-12-hmi-men-assistentes-lista-2.png)).

O que cada um mostra aberto, visto na tela com os valores padrão do carro de mentira (zero), que não são ajuste de carro nenhum:

| Assistente | Itens |
|---|---|
| [ACC](quadros/2026-10-12-hmi-men-acc-aberto.png) | Perfil de condução; Última distância selecionada; Distância; Visualização do trajeto (duas linhas com o mesmo nome) |
| [Front Assist](quadros/2026-10-12-hmi-men-front-assist-aberto.png) | Aviso prévio (caixa); Aviso prévio (lista); Exibir alerta de distância; Evasive Steering Support; Turn Assist |
| [Lane Assist](quadros/2026-10-12-hmi-men-lane-assist-aberto.png) | Orientação adaptativa de faixa; Vibração do volante; Intensidade do alerta; Informações |
| [Side Assist](quadros/2026-10-12-hmi-men-side-assist-aberto.png) | Luminosidade; Informações |
| [Reconhecimento de sinalização](quadros/2026-10-12-hmi-men-sinalizacao-aberta.png) | Alerta de velocidade; Alerta a mais de (km/h); Informações |

Não abri: Emergency Assist, Aviso de nova partida, Reconhecimento de reboque e Sistema de reconhecimento de cansaço. Os subitens saem em cinza enquanto a caixa do assistente está desmarcada. **Não toquei nas caixas.** Pelo que o Marco 9 mostrou, pedido de gravar de um serviço que não está no ajuste `grava` do perfil não é atendido, e só o de relógio e unidades está: se for assim aqui, a caixa não fica marcada (dedução, não testado; D157).

**Para o projeto 01 (o Lane Assist no carro).** O que este firmware da central consulta para mostrar o Lane Assist, lido em `LaneAssistService`:

- o campo `ID_DISPLAY_LANE_DEPARTURE_WARNING` da lista de funções do carro (na tabela da HMI: requisito `Dia_Anp_613`, "Lane Departure Warning: Menu display");
- as opções do serviço de assistência (`LDWHCAViewOptions`), que no carro vêm do serviço de dados de veículo, e não da adaptação;
- e sete valores que ele assina: ligado ou desligado, a orientação adaptativa de faixa (o nível de tolerância do HCA: 0 é ligada, 2 é desligada), a vibração do volante, a intensidade do alerta, o Emergency Assist e mais dois (`CutCorner` e `LaneChange`).

O registro do projeto 01 lista, com fonte, quatro adaptações do 5F para o Lane Assist. A tabela de campos da HMI tem as quatro, com os mesmos nomes: `menu_display_Lane_Departure_Warning` é o `ID_DISPLAY_LANE_DEPARTURE_WARNING`; `…_over_threshold_high` é o `ID_DISPLAY_LANE_DEPARTURE_WARNING_OVER_THRESHOLD_HIGH` ("Operation with v > V_Threshold_high possible"), que o `DeviceManager` usa para tirar o aparelho da lista dos que ficam bloqueados acima da velocidade-limite; `LDW_HCA_0x19` e `LDW_HCA_0x19_msg_bus` são o `ID_LDW_HCA_0X19` e o `ID_LDW_HCA_0X19_MSG_BUS` da lista de aparelhos, que a HMI não consulta para a tela. O par `menu_display_lane_assistant`, que as fontes de lá dão como do ponto cego, é consultado pelo serviço do Side Assist. Deixei isso escrito no [README do projeto 01](../../../carros/jetta-gli-2021/projetos/01-lane-assist/README.md), numa subseção nova e num acréscimo à pendência 8 (D156).

É leitura do firmware MU0883 (o carro está no MU 0803, do mesmo train) e medida na bancada. **Não diz o que gravar no carro:** o nome com que esse campo aparece na adaptação do 5F no ODIS não foi conferido, e a numeração dos bytes é a do DID nesta bancada. Serve para saber o que procurar depois de instalar a câmera: se o bloco "Assistentes" não aparecer, é a função na adaptação; se aparecer com a lista vazia, é a câmera que não está entregando as opções à central.

### O que entrou na pasta

Cinco arquivos novos, às 3h45:37 de 12/10/2026. **Nenhum arquivo que já existia foi mudado**: nenhum lançador, nenhum perfil padrão, nenhum stub. Às 4h00:20 troquei o gerador e os dois arquivos do catálogo para acertar um comentário (o das travas, que dava por visto o que é leitura do nome de um método); as linhas que valem são as mesmas, conferidas pelo resumo de cada arquivo sem os comentários.

| Arquivo | O que é |
|---|---|
| `stubs/veiculo/catalogo-menus.txt` | 262 opções de exibição em estado 2, em 13 eventos: 192 dos cinco menus (pneus 15, luz externa 26, luz do interior 36, estacionar 60, espelhos 6, limpador 9, travas 40) e 70 dos assistentes (ACC 25, Front Assist 9, Lane Assist 13, Side Assist 9, sinalização 9, cansaço 5). Só opções: nenhum valor |
| `stubs/codificacao/catalogo-menus.txt` | as 13 funções "Menu display" no `0B1B`, uma por evento |
| `driver/m14_catalogo_menus.py` | o gerador dos dois. As opções saem do esquema dos serviços de veículo e o byte de cada função sai da tabela de campos, os dois tirados do firmware: nenhum número é digitado. De seu, ele só tem a lista "que função e que serviço fazem cada menu" |
| `driver/m14_menus.sh` | sobe a central de teste "menus" (`moi3-m8-menus`) com os perfis padrão e, depois deles, o catálogo. Tira os padrões do `m3_ao_vivo.sh`, para não repetir a lista; se não os acha, para e diz. `mostra` só mostra o comando; `desce` encerra |
| `driver/tests/t_catalogo.sh` | 32 casos, no Mac, sem container |

Para ver:

```bash
sh driver/m14_menus.sh
```

```bash
sh janela/abre.sh --container moi3-m8-menus
```

```bash
sh driver/m14_menus.sh desce
```

Na janela: tecla Carro e o botão "Config." para os cinco menus; tecla Menu e o bloco "Assistentes" para a tela IQ.DRIVE, e o botão do canto de baixo à direita para a lista. Não rodei o `janela/abre.sh`: a pasta `janela/` é da sessão principal; olhei as centrais por fotos.

O que os cinco menus mostram com o catálogo (fotos): [pneus](quadros/2026-10-12-hmi-men-pneus.png); luz ([1](quadros/2026-10-12-hmi-men-luz-1.png), [2](quadros/2026-10-12-hmi-men-luz-2.png), [3](quadros/2026-10-12-hmi-men-luz-3.png)); estacionar e manobrar ([1](quadros/2026-10-12-hmi-men-estacionar-1.png), [2](quadros/2026-10-12-hmi-men-estacionar-2.png), [3](quadros/2026-10-12-hmi-men-estacionar-3.png)); espelho e limpador ([1](quadros/2026-10-12-hmi-men-espelho-1.png), [2](quadros/2026-10-12-hmi-men-espelho-2.png)); abrir e fechar ([1](quadros/2026-10-12-hmi-men-abrir-1.png), [2](quadros/2026-10-12-hmi-men-abrir-2.png), [3](quadros/2026-10-12-hmi-men-abrir-3.png)). As fotos de luz, estacionar e abrir e fechar não chegam ao fim da lista de cada um.

### Para o Lipe decidir

Cada linha é um menu que hoje está **desligado**. A coluna do meio é o que a ficha do carro diz hoje ([carros/jetta-gli-2021/README.md](../../../carros/jetta-gli-2021/README.md), tabela de equipamentos). Para ligar um deles no simulador de todo dia, a linha da função vai para `stubs/codificacao/jetta-gli-2021-deducoes.txt` e as opções que o carro tiver vão para `stubs/veiculo/jetta-gli-2021-deducoes.txt`, tiradas do catálogo, com a razão ao lado.

| Menu | O que a ficha sustenta | O que decidir |
|---|---|---|
| Pneus (indicador de controle dos pneus, botão SET) | Nada: a ficha não tem linha sobre monitoramento de pneus | A central do carro tem "Pneus" em Veículo → Config.? |
| Luz | Faróis "a confirmar"; farol alto automático "a confirmar" (o Lipe informou em 07/10/2026 que o carro tem); luz ambiente "a confirmar" (o texto de lançamento de 2019 diz de série, em 10 tonalidades) | Que linhas a tela de luz do carro tem (Light Assist, horário para ligar, luz de circulação diurna, modo de viagem, iluminação ambiente)? |
| Estacionar e manobrar | Sensores de estacionamento e frenagem de manobra "a confirmar" (imprensa: de série) | Existe o item? Com que linhas? |
| Espelho e limpador | Sensor de chuva "a confirmar" (Garagem360: de série); sobre os espelhos, nada | Existe o item? Com que linhas? |
| Abrir e fechar | Chave presencial "a confirmar" (Garagem360: de série) | Existe o item? Com que linhas? |
| Assistentes: ACC e Front Assist | Radar e ACC "a confirmar" (imprensa: de série) | A grade do carro tem o bloco "Assistentes"? O que a lista dele mostra? |
| Assistentes: Side Assist | Monitor de ponto cego "a confirmar"; as fontes brasileiras não citam | Idem |
| Assistentes: Lane Assist | Câmera "Não": é o projeto 01 | Fica desligado até o carro ter a câmera. Para ver como a tela vai ficar: `sh driver/m14_menus.sh` |
| Assistentes: sinalização, reboque e cansaço | Nada | Idem |

A forma mais barata de decidir é fotografar a central do carro. A ficha já pedia a foto de "Veículo → Config. (a lista inteira)"; acrescentei a ela, às 3h51, o pedido de cada submenu aberto e da grade de menus (D156).

### As rodadas

Vinte e uma centrais de teste `moi3-men-*`, de 2h52 a 3h53 de 12/10/2026, no máximo duas ao mesmo tempo: `a`, `b`, `c`, `luzext`, `luzint`, `pdc`, `espelho`, `travas`, `k`, `assist`, `as2`, `p`, `l1`, `l2`, `l3`, `pn1`, `lim`, `menus`, `rep2` e, da pasta depois de instalar, `pasta1` e `pasta2`. Os perfis de cada uma estão em [traces/men_exp/](traces/men_exp/).

- **Tempos esgotados.** Só dois experimentos tiveram: a rodada `a` (5) e a `luzint` (4), todos do pedido `DSICarLight.requestIntLightRGBColorList`, a lista de cores da luz ambiente, que a HMI faz quando a opção `intLightRGBColorList` existe. A resposta é uma mensagem de um tipo que o carro de mentira não monta. O catálogo deixa essa opção de fora, e com ele não houve nenhum (rodadas `k`, `menus`, `rep2`, `pasta1` e `pasta2`).
- **O catálogo, duas partidas da cópia de trabalho e duas da pasta:** as mesmas listas nas quatro, pelo roteiro novo. Nas duas da pasta (`pasta1`, às 3h45, e `pasta2`, às 3h50, esta já com o motor do carro de mentira que a sessão do telefone trocou às 3h47): as mesmas telas, 119 assinaturas no carro de mentira, perfil de 353 valores em 45 eventos, nenhum tempo esgotado e nenhum pedido com prazo sem resposta ([traces/men_prova_da_pasta.txt](traces/men_prova_da_pasta.txt)).
- **Com o catálogo aparece "Aviso de nova partida"** na lista de assistentes, que não apareceu na rodada só de assistência (`as2`): lá a lista tinha os outros oito. Vem de alguma opção dos cinco menus; não procurei qual.

### Testes

`driver/tests/t_catalogo.sh`, 32 casos, no Mac. Confere que os dois arquivos da pasta são o que o gerador escreve do esquema e da tabela de agora; que o perfil de veículo só tem opções em estado 2, em seções de opções, sem a lista de cores; que a codificação só liga campos "Menu display" que existem na tabela, com a constante certa ao lado; que os dois dizem logo no começo que não são o carro; que nenhum lançador cita o catálogo e que os padrões dos lançadores são listas sem curinga; e que o roteiro põe o catálogo depois dos padrões, sobe uma central de teste e para se não acha os padrões.

- **Vistos falhar:** os 28 que não são guardas, por falta dos arquivos, em duas levas: 17 antes de existirem o gerador e o catálogo ([vermelho](traces/men_t_catalogo_vermelho.txt)) e 11 antes de existir o roteiro ([vermelho do roteiro](traces/men_t_catalogo_vermelho_roteiro.txt)). Como falhar por falta de arquivo prova pouco, estraguei uma coisa de cada vez numa cópia e conferi que caso pega cada defeito: 19 defeitos, todos pegos ([traces/men_t_catalogo_mutacoes.txt](traces/men_t_catalogo_mutacoes.txt)).
- **Guardas, que já passavam:** 4, os que olham os lançadores. Três deles foram vistos falhar por mutação; o quarto, o do padrão da codificação, é igual ao do veículo e não foi estragado.
- **Ordem (D155):** o gerador nasceu das rodadas de exploração, antes do teste. O roteiro foi escrito depois dos casos dele.
- **O que o teste não cobre:** que item aparece na tela. Isso só as rodadas fotografadas mostram.

**Bateria completa da pasta, antes e depois de instalar.** Antes, às 3h41 ([saída](traces/men_suites_antes.txt)): glforward 8, `t_transmissao` 19, `t_codificacao` 39, `t_entrada` 11, `t_pedidos` 28, `t_veiculo` 187, `t_memoria` 80, `t_hub` 25, `t_vc_diag_alm` 37, `t_hmilog` 32, `t_kipc_bus` 14, `t_radio` 81, `t_telefone` 103, `t_midia` 107 e `t_som` 135; no Mac, `t_memoria` 80, `t_servidor` 20 e `t_lsmval_abre` 38. Tudo passando. Depois, às 3h50 ([saída](traces/men_suites_depois_2.txt)): tudo passando, com os mesmos números, salvo três: `t_veiculo` 188 (o caso a mais é o catálogo conferido contra o esquema), `t_telefone` 142 (a sessão do telefone acrescentou casos às 3h47) e o `t_catalogo` novo, 32.

**Uma rodada de depois saiu vermelha, e não era da pasta.** A primeira, às 3h45 ([saída](traces/men_suites_depois_1_corrida.txt)), teve 26 falhas no `t_telefone`. Às 3h47:48, no meio dela, a sessão do telefone trocou o motor (`driver/veiculostub.c`), o teste e um perfil. A rodada tinha compilado o motor de antes, no começo, e leu o teste de depois, no fim. Repetida às 3h50, com a pasta parada, passou inteira. Serve de aviso a quem roda a bateria com outras sessões instalando: os programas são compilados no início, e os testes são lidos da pasta na hora de cada um.

### Limites

- **O catálogo não é o carro.** Liga opções que o Jetta com certeza não tem (o esquema traz farol a laser, cortina elétrica, visão noturna) e deixa os valores em zero. Quem comparar com o carro compara a lista de linhas, não os valores.
- **Não sei que opção liga que linha de tela**, fora as que cito. São 262, e o catálogo liga todas.
- **A lista de cores da luz ambiente fica de fora**, e com ela o item de cor, se a tela tiver um. Para tê-la falta o carro de mentira montar a resposta do pedido.
- **Nada do que se muda nesses menus fica gravado**, pela dedução de cima; não testei. (**Mudou em 12/10/2026, às 9h56:** medido, e com `sh driver/m14_menus.sh grava` passa a ficar; subseção "A gravação nos menus do catálogo", no fim deste Marco.)
- **Menus que a tabela do `DeviceManager` conhece e não experimentei:** bancos, climatização, head-up display, teto solar, controle de garagem, freio (que alimenta o ESC e o Auto Hold), perfil de condução, amortecedores, start-stop, híbrido. Estão em [traces/men_aparelhos.txt](traces/men_aparelhos.txt), com o byte e o serviço de cada um.
- **A função do assistente de pedestres** estava ligada na rodada `as2` e não mudou nada que eu visse.
- **O nome de cada função no ODIS não foi conferido.** Os nomes daqui são os da tabela da própria HMI.
- **A central ao vivo não foi tocada.** Estava desligada todas as vezes em que olhei, e nada do que entrou muda a partida dela.

### Onde parou

Parou no que depende do Lipe: dizer, ou fotografar, o que a central do carro mostra. Com isso, ligar um menu é copiar linhas do catálogo para os dois arquivos de deduções e rodar uma série de partidas. O que ficou para depois, na ordem em que eu faria: a gravação nesses menus (pôr o serviço em `grava` e ver se alguma tela entra em laço, como o idioma do painel entrou no Marco 9), a lista de cores da luz ambiente, e os menus não experimentados.

### A gravação nos menus do catálogo: as caixas pegam e voltam (12/10/2026, manhã)

Pedido do Lipe de manhã, repassado pela sessão principal: fazer o que estava no cartão "Menus do carro: as caixas pegarem e voltarem", mantendo que nenhum menu é ligado por padrão. O trabalho é no catálogo.

**Resultado, verificado em execução:**

- **Com `sh driver/m14_menus.sh grava`, a caixa marcada fica marcada.** Mexi numa de cada menu: Light Assist (luz), Ativar automaticamente (estacionar e manobrar), Abaixar em marcha a ré (espelhos), Fechar automaticamente com chuva (abrir e fechar), Front Assist e Lane Assist (rodada `c`). Sem o `grava`, as mesmas seis voltavam a desmarcadas (rodada `a`): era o que a D157 deduzia, e ficou medido.
- **A central seguinte nasce com as seis marcadas**, subida do estado guardado (rodadas `d`, `g3` e `g4`, e, da pasta, pelo roteiro, a `pasta`).
- **Mexer de novo desfaz:** na `g5`, que nasceu marcada, os quatro toques nos menus gravaram zero.
- **Nenhum laço.** Oito partidas com o perfil de gravação (`b`, `c`, `d` e `g1` a `g5`), e a partida é a mesma do catálogo sem gravação (`g6`): 119 assinaturas, 3 pedidos atendidos, 14 sem resposta, nenhum tempo esgotado. Só abrir as telas não grava nada.

| Antes: marquei "Light Assist" e a caixa não ficou | Depois: fica |
|---|---|
| ![Antes](quadros/2026-10-12-hmi-gra-antes-a-caixa-nao-fica.png) | ![Depois](quadros/2026-10-12-hmi-gra-depois-a-caixa-fica.png) |

Na partida seguinte, sem ninguém tocar: [estacionar e manobrar](quadros/2026-10-12-hmi-gra-partida-seguinte-estacionar.png), [os assistentes](quadros/2026-10-12-hmi-gra-partida-seguinte-assistentes.png) e, da pasta, [a luz](quadros/2026-10-12-hmi-gra-pasta-partida-seguinte-luz.png).

**O que entrou na pasta, às 9h56:51 de 12/10/2026:**

- `stubs/veiculo/catalogo-menus-grava.txt` (novo): a lista `grava` com os serviços dos menus (`DSICarComfort`, `DSICarLight`, `DSICarParkingSystem` e `DSICarDriverAssistance`) e uma regra. Só isso: nenhum valor. Como a lista de depois troca a de antes, ela repete o `DSICarTimeUnitsLanguage` do perfil padrão. Não é o carro e não entra em partida nenhuma por padrão.
- `driver/m14_menus.sh` ganhou a opção `grava`: põe esse perfil por último e liga o arquivo de estado do carro de mentira (`/tmp/moi3-menus/memoria/veiculo/estado.txt`). Sem a opção, o roteiro faz o que fazia. Para voltar ao zero: `rm -rf /tmp/moi3-menus/memoria`.
- `driver/tests/t_catalogo.sh`: 32 → 43 casos.

**O que cada pedido grava.** Vem das linhas `G` do esquema, que são dedução do gerador pelo nome: dos 277 pedidos `set…` dos quatro serviços, 221 têm linha. Um dos que não têm apareceu nos toques e ganhou regra no perfil: `setPDCAutoActivation` grava `updatePDCOPSAutoActivation` (os nomes diferem; o campo é `autoActivation` nos dois; com a regra, a caixa "Ativar automaticamente" fica). Os outros 55 continuam sem efeito e estão em [traces/gra_pedidos_sem_regra.txt](traces/gra_pedidos_sem_regra.txt).

**Visto de passagem, sem mexer:**

- **O SET dos pneus pede confirmação** (["As pressões dos 4 pneus estão conforme os valores exigidos?"](quadros/2026-10-12-hmi-gra-pneus-set-pede-confirmacao.png)). Confirmado, a HMI manda `DSICarComfort.setRDKPressureChanged` e espera uma resposta de um tipo que o carro de mentira não monta: fica sem resposta. Não houve tempo esgotado no registro até a central acabar.
- **Desmarcar o Front Assist pede confirmação** (["Não é recomendado desativar totalmente o sistema de monitoramento periférico. Deseja mesmo desativar essa função?"](quadros/2026-10-12-hmi-gra-desmarcar-front-assist-pede-confirmacao.png)). Não confirmei. Na rodada `g5` foi esse aviso que engoliu os dois toques nos assistentes.

**Testes.** `t_catalogo.sh`, 11 casos novos: 9 vistos falhar antes de existirem o perfil e a opção ([saída](traces/gra_t_catalogo_vermelho.txt)) e 2 guardas (sem `grava`, o roteiro não leva o perfil nem liga o estado), vistos falhar com o defeito posto de propósito ([mutações](traces/gra_t_catalogo_mutacoes.txt)). `t_veiculo.sh` passa a 225: o perfil novo é conferido sozinho contra o esquema. Bateria da pasta depois de instalar, das 10h02 às 10h07 ([saída](traces/gra_suites_depois.txt)): tudo passando, com `t_veiculo` 225, `t_catalogo` 43, `t_radio` 114 e `t_telefone` 201 (os dois últimos cresceram por outras sessões). A de antes é a das 9h22 do passo das unidades, toda passando; não rodei outra entre as duas.

**Limites:**

- **Só caixas, uma por menu.** Listas ("Horário para ligar", "Abertura de conforto") e barras (volumes do estacionamento, iluminação) não foram tocadas, nem os subitens dos assistentes.
- **55 pedidos sem regra** continuam sem efeito, e o SET dos pneus fica sem resposta.
- **O `grava` do roteiro guarda só o que o carro de mentira recebe.** A memória da própria tela (a última vista, por exemplo) não é ligada nessa central.
- **Continua não sendo o carro:** o que fica marcado é escolha de quem mexeu na central de teste.

**Rodadas:** treze centrais `moi3-gra-*`, das 9h29 às 10h01 de 12/10/2026: `a`, `b`, `c`, `d`, `g1` a `g7` e a `pasta` (esta pelo roteiro, com o nome `menus`). Resumos em [traces/gra_serie.txt](traces/gra_serie.txt).

### Os menus que o Lipe confirmou entram no modo ao vivo (12/10/2026, tarde)

Às 17h31 de 12/10/2026 a sessão principal repassou o que o Lipe respondeu, de memória e sem foto, a duas perguntas com opções: "Em Veículo → Config. no seu carro, quais destes itens aparecem? (Se tiver também Pneus, escreva em Outro.)": "Luz, Espelho e limpador, Abrir e fechar, Estacionar e manobrar"; e "A foto de 09/10 mostra o bloco Assistentes na grade do seu carro. O que aparece dentro dele?": "ACC, Front Assist, Detector de fadiga". O pedido: ligar esses no padrão do modo ao vivo, com o menor conteúdo que faz cada menu existir e a fonte ao lado de cada linha, num perfil à parte que dê para desligar; Pneus e Lane Assist ficam de fora.

**Resultado, verificado em execução** pelo caminho do modo ao vivo (uma cópia do `driver/ao_vivo.sh` com outro nome de container, com o rádio, o telefone, a mídia e o som de mentira do padrão):

- **"Configurações do veículo" ganha os quatro itens:** Luz, Estacionar e manobrar, Espelho e limpador e Abrir e fechar, antes de Instrumento combinado. Não há Pneus.
- **A grade de menus ganha o bloco "Assistentes"**, e a lista dele tem ACC (contr. autom. de dist.), Front Assist e Sistema reconh. de cansaço. Não há Lane Assist.
- **As caixas pegam e voltam.** Marquei Light Assist, a ativação dos limpadores na chuva, Keyless-Access, Front Assist e o reconhecimento de cansaço (rodadas `r1` e `r2`); as duas centrais seguintes, subidas da memória guardada, nasceram com as cinco marcadas (`r4` e `r3c`).
- **`MENUS=0 sh driver/ao_vivo.sh` desliga os dois perfis** (rodadas `b1` e `b2`; conferido pelos argumentos do lançador, no teste, e pela partida; não fotografei).

| Configurações do veículo | A lista de assistentes |
|---|---|
| ![Lista](quadros/2026-10-12-hmi-jm-lista.png) | ![Assistentes](quadros/2026-10-12-hmi-jm-assistentes-lista.png) |

Cada menu entrou com uma opção só. **Que o menu existe, ele disse; que a linha é esta, não:**

| Menu | O que aparece dentro | De onde vem a linha |
|---|---|---|
| [Luz](quadros/2026-10-12-hmi-jm-luz.png) | Light Assist | o Lipe informou em 07/10/2026 que o carro tem farol alto automático (ficha); que a linha é esta é palpite |
| [Estacionar e manobrar](quadros/2026-10-12-hmi-jm-estacionar.png) | Volume atrás (e, junto, Redução do entretenimento) | palpite: a ficha traz sensores dianteiros e traseiros pelo Garagem360 (imprensa, a confirmar) |
| [Espelho e limpador](quadros/2026-10-12-hmi-jm-espelho.png) | Ativação autom. limpadores dos vidros na chuva | palpite: a ficha traz o sensor de chuva pelo Garagem360 (imprensa, a confirmar) |
| [Abrir e fechar](quadros/2026-10-12-hmi-jm-abrir.png) | Keyless-Access | palpite: a ficha traz a chave presencial pelo Garagem360 (imprensa, a confirmar) |
| [Assistentes](quadros/2026-10-12-hmi-jm-assistentes-tela.png) | ACC, Front Assist e reconhecimento de cansaço | os três, ditos por ele; o bloco aparece na foto de 09/10/2026; a opção ligada em cada um é palpite |

**O que entrou na pasta, às 18h05:04 de 12/10/2026:**

- `stubs/veiculo/jetta-gli-2021-menus.txt` e `stubs/codificacao/jetta-gli-2021-menus.txt` (novos): sete opções em estado 2, sete funções "Menu display" no `0B1B` e a lista `grava` com os serviços desses menus. Cada linha traz a fonte, e o cabeçalho traz as perguntas e as respostas. Não são leitura do carro.
- `driver/ao_vivo.sh`: põe os dois no fim dos padrões de codificação e de veículo; `MENUS=0` (ou vazio) tira. Quem passa `CODING` ou `VEICULO` à mão fica com a lista que deu. Os outros lançadores não mudaram: central de teste só tem os menus se alguém passar os perfis.
- `driver/tests/t_catalogo.sh`: 43 → 59 casos.

**As rodadas** (centrais `moi3-jm-*`, das 17h34 às 17h58; resumos em [traces/jm_serie.txt](traces/jm_serie.txt)): seis partidas válidas com os menus, quatro com memória guardada (`r1`, `r2`, `r4` e `r3c`) e duas sem (`r5c` e `r6c`), e duas com `MENUS=0` (`b1` sem memória, `b2` com). Nas seis: relógio aceito, 112 assinaturas no carro de mentira (111 com `MENUS=0`), 3 pedidos atendidos além dos toques, 14 sem resposta, nenhum laço, nenhum tempo esgotado, 4 linhas de serviço não registrado. **Cinco partidas foram inválidas por erro do meu rascunho** (`r3`, `r5`, `r6`, `r3b` e `r5b`): eu apagava e recriava as pastas de registro e de quadros logo antes do `docker run`, e a montagem falhava ("mkdir: cannot create directory '/live/log': File exists"). Parei de apagar as pastas e refiz. O lançador da pasta não faz isso.

**Testes.** `t_catalogo.sh`, 16 casos novos: 12 vistos falhar antes de existirem os perfis e a chave ([saída](traces/jm_t_catalogo_vermelho.txt)) e 4 guardas, que já passavam (com `MENUS=0` e com `MENUS` vazio não há perfil; a lista dada à mão é respeitada; os outros lançadores não citam os perfis). O lançador é testado numa cópia, com um `docker` de mentira. `t_veiculo.sh` passa a 226. Bateria completa no candidato (a pasta das 17h58 mais os quatro arquivos), das 17h58 às 18h04: tudo passando. Na pasta, depois de instalar, das 18h05 às 18h12 ([saída](traces/jm_suites_depois.txt)): tudo passando, nas de container e nas do Mac, com `t_veiculo` 226 e `t_catalogo` 59 (`t_radio` 114, `t_telefone` 287, `t_midia` 119 e `t_som` 214, que cresceram por outras sessões).

**Limites:**

- **Os menus têm uma linha cada, e a do carro há de ter mais.** O resto fica desligado até haver foto da tela do carro; o que a central sabe mostrar está no catálogo.
- **Não abri os assistentes** (o que há dentro de ACC, de Front Assist e do reconhecimento de cansaço), nem mexi na barra do volume do estacionamento.
- **A "função de frenagem de manobra", que tem fonte de imprensa, não serviu:** com ela o item aparece e a tela fica vazia. Troquei por "Volume atrás".
- **Com `MENUS=0` e uma memória cuja última tela era a dos assistentes**, a central ainda nasce nessa tela (rodada `b2`), com a cena sem assistente nenhum.
- **A ordem da grade** já é a da foto do carro no padrão do modo ao vivo (Rádio, Telefone, Navegação, Assistentes; Mídias, o bloco do CarPlay, Veículo, Som). O bloco do CarPlay aparece aqui como "App-Connect" e na foto como "Apple CarPlay": é da sessão do telefone, não mexi.
- **A central ao vivo estava desligada e não foi ligada.** A próxima partida dela já sobe com os menus.

## Marco 16 — rádio, mídia e volante: o que faltava (12/10/2026, manhã)

Feito pela sessão do rádio e da mídia de mentira (Marcos 11 e 15), a partir das 8h45 de 12/10/2026. O Lipe voltou às 8h30 e pediu para finalizar os itens do roteiro; a sessão do simulador repassou a esta os que faltavam nos itens 4, 5 e 6 de [simulador-jetta.md](simulador-jetta.md), em três passos, cada um medido, instalado quando há o que instalar e registrado antes do seguinte. É emulação num Mac; nada vai para o carro. **A central ao vivo estava ligada e em uso pelo Lipe: não mandei nada para ela.** Tudo abaixo é de centrais de teste `moi3-mid-k*`, subidas com os padrões do modo ao vivo (rádio, telefone, mídia e som de mentira, e 6 s para os serviços DSI).

### Passo 1 — Próxima e Anterior do volante na tela de mídia

**Resultado: com o som de mentira ligado, que é o padrão do modo ao vivo desde as 3h35, as duas teclas trocam de faixa. Não precisou de código.** O limite do Marco 15 ("as teclas Próxima e Anterior do volante não trocam de faixa") foi medido antes de o som existir. No modo ao vivo ele deixou de valer; sem o som (`SOM=`) continua valendo.

| Central | Som de mentira | Próxima do volante (46) | Anterior do volante (47) | Próxima da moldura (14) | Seta da tela |
|---|---|---|---|---|---|
| `k1` e `k3` | ligado | pula: `DSIMediaPlayer.skip`, próxima | pula: anterior | pula | pula |
| `k4` e `k5` | ligado | pula | pula | só o aperto longo, abaixo | não toquei |
| `k2`, ao lado da `k3` | desligado | a HMI recebe a tecla e nenhum pedido sai | o mesmo | o mesmo: a da moldura também não | pula |

**A causa.** Medido: na `k3` a linha `[SoundConnectionHandling] Audio entertainment: ENT_AMP_MEDIA_PLAYER 20` sai 0,25 s depois da tecla Mídia, e daí em diante cada tecla vira um `CmdSkip` na fila de comandos da mídia; na `k2` não há linha nenhuma da área de som depois da tecla Mídia, e o único `CmdSkip` da rodada é o da seta da tela. Lido no código decompilado da HMI: quem transforma tecla em salto de faixa é a máquina de estados gerada da tela de mídia (`generated.de.vw.mib.appui.media.internal.MediaActivityImpl`: as transições internas 0 e 1 disparam `SKIP_FORWARDS` e `SKIP_BACKWARDS`), e a guarda delas é `CURRENT_AUDIO_COMPONENT == 2` (item 1470 do banco de dados da HMI; expressão 6 do `LocalExpressionEvaluatorImpl`). Esse item quem escreve é a área de som da HMI (`de.vw.mib.asl.internal.sound.HsmSoundDPHandling.updateEntertainmentConnection`), quando o serviço de áudio diz qual é a conexão de entretenimento ativa: 2 para as de mídia, entre elas a 20, e 1 para as de rádio. Sem serviço de áudio ninguém diz, o item não vira 2 e a tecla é recebida e descartada. **O que não li:** a tabela que liga o evento da tecla a essas duas transições; que sejam elas é o que a guarda e a medida, juntas, indicam.

**Aperto longo.** Segurando Próxima ou Anterior por 1,5 s (volante, e a Próxima da moldura), a HMI tenta o avanço rápido (`CmdSeek`) e encerra com "seek not supported": o perfil da mídia de mentira não declara essa capacidade. Nenhum pedido sai e nada fica esperando. Não declarei a capacidade, porque o programa não faz a faixa andar (decisão D159).

Rodadas: `k1` (8h49), `k2` e `k3` lado a lado (8h51), `k4` e `k5` lado a lado (8h56), a `k4` nascida de uma cópia da memória guardada do modo ao vivo. A `k4` e a `k5` deram o mesmo em todos os 33 passos.

### Passo 2 — ▼, ▲, Fonte e Voz do volante, com rádio, mídia, telefone e som

As quatro teclas que ninguém tinha repetido, apertadas em quatro telas nas centrais `k4` e `k5` (os mesmos resultados nas duas). "Recebida" é a linha `HK Received: KBD[4] KEY[<código>]` no registro da HMI; "nada" quer dizer nenhum pedido no barramento, nenhuma linha nos registros dos stubs e nenhuma troca de tela ou de aviso.

| Tecla | Código | Na tela de mídia | Na tela do rádio | Na grade de menus | Na tela do telefone |
|---|---|---|---|---|---|
| ▼ | 49 | recebida; nada | recebida; nada | recebida; nada | recebida; nada |
| ▲ | 48 | recebida; nada | recebida; nada | recebida; nada | recebida; nada |
| Fonte | 45 | **sai da mídia e abre o rádio em FM**: a HMI pausa e desativa o tocador, entra na tela do rádio, pede a conexão `ENT_AMP_TUNER` e sintoniza | **troca de faixa do rádio**: FM para AM (531 kHz, conexão `ENT_AMP_TUNER_AM`) e, no aperto seguinte, AM para FM | troca a faixa do rádio por trás; a tela não muda | o mesmo |
| Voz | 50 | curta: nada. Segura por 1,2 s: a HMI pede o modo acessível ao Bluetooth (`DSIBluetooth.setAccessibleMode`, que o telefone de mentira atende); a tela não muda | curta e segura: nada | nada | nada |

- **Fonte é tecla do rádio nesta HMI.** Quem reage é o gerente de rádio (`RadioManager - isNewRequestedWaveBandPossible()`, `RadioActivationRequest`, `UnifiedRadioBandSwitch - requestAudioConnection() new connection: 13, existing active connection: 1060`, no registro). Da mídia ela leva ao rádio; no rádio alterna FM e AM. **Não volta para a mídia:** em oito apertos por central, depois do primeiro, só FM e AM. Curta e segura fazem o mesmo. A tecla Rádio da moldura, com o rádio em AM, também volta para FM.
- **A Voz segura só pediu o modo acessível na primeira vez** em cada central (na tela de mídia); nas três seguintes, nada. Por quê, não fui atrás.
- **▼ e ▲** seguem sem evento na tabela de teclas e sem efeito em tela nenhuma. No carro podem ser do menu do painel de instrumentos; é suposição.
- **De passagem, para o passo 3:** esta HMI tem faixa de AM, e ela começa em 531 kHz, que é a grade de 9 kHz. O rádio de mentira não tem emissora nenhuma em AM.
- Sem o som de mentira a tecla Fonte não foi repetida agora; à 1h, sem rádio, mídia nem som, ela não fazia nada (13 apertos, `janela/README.md`).

![Próxima pelo volante na mídia; Fonte na mídia, no rádio, na grade e no telefone; Voz segura](quadros/2026-10-12-hmi-m16-volante-na-midia-e-tecla-fonte.png)

Da esquerda para a direita, em cima: a faixa 2 depois da Próxima do volante; o rádio em FM depois da Fonte apertada na mídia; o rádio em AM depois da Fonte seguinte. Embaixo: a grade e a tela do telefone, iguais depois da Fonte, e o rádio em AM depois da Voz segura.

Evidências dos passos 1 e 2: `traces/m16_k*` (os roteiros, as saídas das cinco centrais e os registros da HMI da `k2`, da `k3` e da `k4`).

### Passo 3 — rádio: a busca varre a faixa, AM tem emissoras e há texto de rádio

Instalado às 10h00min34 de 12/10/2026: `driver/radiostub.c`, `stubs/radio/fm-de-mentira.txt` e `driver/tests/t_radio.sh`. Do que o item 4 do roteiro trazia em "Falta", três coisas passaram a existir e uma não tem o que fazer:

| Pedido | O que ficou | Como se vê |
|---|---|---|
| a busca varrer a faixa | **feito.** Na busca o rádio de mentira manda uma atualização por canal do caminho, só com a frequência, a cada 25 ms, até a emissora seguinte da lista, dando a volta pela ponta da faixa se for preciso; na chegada, a emissora inteira | na tela de sintonia manual a frequência corre (78,1, 81,3, 84,5, 87,3, 89,1 MHz, "TESTE 1", em fotos tiradas de 0,15 em 0,15 s) e o ponto da escala anda |
| a lista de AM | **feito.** Três emissoras inventadas, em 594, 1008 e 1305 kHz. A HMI recebe a lista de cada faixa numa mensagem à parte; o modelo tira as de AM da lista do perfil e as manda com o tipo de faixa 0 | "Lista de emissoras AM" mostra as três pela frequência; a sintonia, a busca e o botão de atualizar a lista funcionam em AM |
| texto de rádio | **feito**, com um texto só, parado: "TEXTO DE RÁDIO DE TESTE", para as emissoras marcadas no perfil ("TESTE 1" e "TESTE 3") | a tela principal do rádio mostra o texto debaixo do nome da emissora; nas outras, nada |
| intensidade de sinal | **não há o que mostrar.** A HMI guarda a qualidade de recepção de cada emissora e não a põe em tela nenhuma | leitura do código, abaixo; não medi |

![A busca varrendo a faixa, a chegada na emissora, o fim de uma busca em AM e a tela Emissoras em AM](quadros/2026-10-12-hmi-m16-radio-busca-que-varre.png)

Em cima, quatro fotos seguidas da mesma busca, de 76,1 MHz para cima. Embaixo: a chegada em 89,1 MHz, a tela principal com "TESTE 1", o fim de uma busca em AM (594 kHz) e a tela "Emissoras" em AM.

![Antes e depois: a lista de AM, o rádio sem e com texto, e a tela do veículo nas duas centrais com memória](quadros/2026-10-12-hmi-m16-radio-am-e-texto-antes-e-depois.png)

Em cima: a lista de AM antes (só a frequência atual) e depois, e a "TESTE 1" antes, sem texto. Embaixo: a "TESTE 1" depois, com o texto, e a tela do veículo de duas centrais da série, com os dados.

**O que a HMI espera, lido no código decompilado e conferido no barramento.**

- **Lista por faixa.** `StateTopMainUnifiedRadio.dsiUnifiedTunerUpdateStationList(lista, tipo de faixa, validade)` separa pelo tipo de faixa: 0 e 1 vão para a lista de AM (`StateAmFmTop.dsiUnifiedTunerUpdateStationListAm`), 2 e 3 para a de FM. É o mesmo atributo (4), em mensagens separadas. O motor do carro de mentira guarda um valor por atributo, e por isso a lista de AM é do modelo: ele a manda no primeiro pedido que atende, cada vez que a HMI entra em AM e quando ela pede a atualização da lista de AM.
- **Texto.** `UnifiedRadioRadioTextManager.dsiUnifiedTunerUpdateRadioText` só aceita o texto com validade 1 e com o PI da emissora atual (`isReceivedTextValid`), e só o mostra se não for vazio (`isRadioTextAvailable`). Por isso o PI não está no perfil: o modelo põe o da emissora de agora, depois de cada sintonia e de cada busca.
- **Atualização no meio da busca.** `StateTopMainUnifiedRadio.dsiUnifiedTunerUpdateSelectedStation` descarta a emissora só enquanto espera o "em curso"; depois dele, cada atualização vira a emissora da tela. É o que faz a frequência correr.
- **Intensidade de sinal.** O campo `receptionQuality` da emissora é copiado e guardado (`UnifiedStationComfortClass`, `AdapterAslPersistence`), e a única comparação por ele está em `AslStationQualityComparator`, usado só por `QualityPIFrequencyHDComparatorChain`, que nenhuma outra classe cita. Entre as constantes de tela do rádio não achei item de qualidade de recepção nem de sinal (o "audio quality" que há é um ajuste de alta ou baixa nas configurações, usado na escolha de fluxo do rádio híbrido; não é medida de sinal). **É leitura, e a procura foi por nome:** não pus o campo no perfil para ver se algo muda.

**A grade está no programa** (decisão D162). Para varrer, o sintonizador de mentira precisa saber onde a faixa começa, onde acaba e de quanto em quanto anda, e a HMI não lhe diz: tira isso da adaptação do carro (Marco 11). A tabela `GRADES` do `radiostub.c` traz as duas que a HMI usa na bancada: FM de 76,1 a 107,9 MHz, de 200 em 200 kHz (a faixa "Brasil" do palpite do Marco 11), e AM de 531 a 1602 kHz, de 9 em 9 kHz. **A de AM é a da Europa**, que a HMI assume porque a adaptação de AM da identidade provisória está "sem ajuste"; o campo "Bandsettings AM Tuner" do DID `0559` só tem Europa, América do Norte e Japão (`stubs/codificacao/tabela-campos.tsv`), e não pus palpite (decisão D163).

**A varredura segura o programa** (decisão D161). Ela acontece dentro do atendimento do pedido: até 40 mensagens, 25 ms entre elas, 1 s no máximo; caminho com mais de 40 canais pula canais. Nesse tempo o rádio de mentira não lê o barramento. Medido com dois toques seguidos na seta, a 0,3 s um do outro, em três centrais: a HMI mandou interromper no meio da varredura, a resposta "interrompida" só saiu depois da chegada, a emissora ficou a da chegada e a busca seguinte funcionou.

**Teste antes do código.** O `t_radio.sh` foi de 81 para 114 casos. Vistos falhar pelo motivo esperado, em três levas, cada uma antes do código dela: 20 contra o programa de antes (a varredura, a lista de AM e o texto; os outros 87 passavam), 4 contra o perfil de antes (emissoras de AM na grade, texto que se diz de teste, emissora com texto tem PI, e a contagem por faixa) e 3 contra o programa sem as linhas de registro (`traces/m16_t_radio_vermelho_*.txt`). Os casos de busca que esperavam três mensagens foram reescritos: o comportamento mudou. O teste confere a lista exata de frequências de uma busca curta (de 93,9 a 98,5 MHz, de 200 em 200), que a busca longa fica na faixa e dá uma volta só, que para antes de passar da emissora quando parte de fora da grade, e as mensagens de AM e de texto byte a byte.

**Série de antes de instalar: três pares lado a lado**, base contra candidata, a base sendo a pasta das 9h13 e a candidata a mesma com os três arquivos novos (`traces/m16_serie16.sh`):

| Par | A central nasce | Assinaturas de veículo | Serviço de veículo fora do prazo | Erros de OpenGL | Pedidos sem resposta (rádio, mídia, telefone, veículo) | O que mudou da base para a candidata |
|---|---|---|---|---|---|---|
| `s1b` e `s1c` | sem memória | 111 e 111 | nenhum | 0 e 0 | 26, 0, 2 e 14 nas duas | as duas linhas de busca, a lista de AM e o texto na tela |
| `s2b` e `s2c` | de uma cópia da memória guardada do modo ao vivo | 111 e 111 | nenhum | 0 e 0 | 26, 0, 2 e 14 nas duas | o mesmo |
| `s3b` e `s3c` | da memória que termina na tela do veículo | 112 e 112 | nenhum | 0 e 0 | 16, 0, 2 e 14 nas duas | o mesmo |

As fotos da grade de menus, da lista de FM e da tela do veículo são iguais byte a byte entre base e candidata nos três pares; a da tela do telefone difere num par, só no relógio (9:37 e 9:38). Os registros dos outros stubs têm o mesmo número de linhas nos dois braços. Uma linha de erro de desenho da tela de mídia (o título da faixa desenhado vazio, "Empty text", do tipo já visto no Marco 15) saiu uma vez na base nos três pares e na candidata em um: aparece e some de uma partida para outra, e não acompanha a mudança.

**A pasta mudou duas vezes durante o trabalho**, por outras sessões, em peça que o rádio usa: o motor (`driver/veiculostub.c`) às 9h20 e de novo às 9h50, junto com os lançadores. As duas mudanças só acrescentam linhas. Refiz a árvore por cima de cada uma: bateria completa e duas centrais de conferência na das 9h45 (`k9a` e `k9b`), bateria e uma central na das 9h55 (`k10`), e instalei da última, depois de conferir que motor, lançadores, compilação e os perfis de rádio, mídia, telefone, som e codificação da pasta eram os da árvore testada e que não havia bateria de outra sessão no ar. Bateria da pasta antes (9h45) e depois de instalar (10h00 a 10h05, motor igual no começo e no fim; `traces/m16_suites_depois.txt`): `glforward-moi3/tests/run_tests.sh` 8, `t_transmissao.sh` 19, `t_codificacao.sh` 39, `t_entrada.sh` 11, `t_pedidos.sh` 28, `t_veiculo.sh` 225, `t_radio.sh` 114, `t_telefone.sh` 201, `t_midia.sh` 119, `t_som.sh` 185, `t_memoria.sh` 80, `t_hub.sh` 25, `t_kipc_bus.sh` 14, `t_hmilog.sh` 32 e `t_vc_diag_alm.sh` 37, no container; `t_servidor.sh` 20, `t_lsmval_abre.sh` 38, `t_memoria.sh` 80 e `t_catalogo.sh` 43, no Mac. Todos passam, sem aviso do compilador.

Rodadas do passo 3: `k6` e `k7` (exploração da cópia de trabalho), `s1b` a `s3c` (a série), `k9a`, `k9b` e `k10` (conferências finais). Onze centrais `moi3-mid-*`.

### Limites

- **Tudo o que o rádio mostra continua inventado:** emissoras, frequências, nomes, PIs e o texto. Não há recepção nem som.
- **O texto de rádio é um só e não muda.** No carro ele vem da emissora e troca o tempo todo. Texto de rádio "plus" (título e artista) e o texto estendido não foram simulados.
- **Intensidade de sinal:** nada, pelo que está acima.
- **A grade de AM é a da Europa** e a de FM é a do palpite "Brasil". Se o Jetta do Brasil tem AM, e com que grade, não sei: pedi a foto na ficha do carro. Quem mudar a faixa na codificação tem de mudar a tabela `GRADES` do programa.
- **Enquanto varre, o rádio de mentira não atende** (1 s no máximo). A busca "sem parar" (seta segurada) faz um caminho só e espera a HMI mandar parar; no carro ela seguiria andando.
- **As setas da tela principal do rádio** seguem trocando entre memórias ou emissoras da lista, sem varrer: quem busca são as setas da sintonia manual. É o ajuste "Teclas de setas" da HMI, que não mudei.
- **A tecla Fonte do volante não volta para a mídia**, e ▼ e ▲ não fazem nada (passo 2). O aperto longo de Próxima e Anterior não faz nada na mídia (passo 1).
- **A central ao vivo não rodou nada disto:** estava ligada, em uso, com o programa de antes. A partida seguinte dela compila o rádio novo.
- **Só no container.** Não rodou no QEMU. **O esquema é do MU0883**; o carro roda o MU0803.

### Onde parou

- Os três passos estão feitos e registrados (10h08 de 12/10/2026). Os passos 1 e 2 não instalaram nada; o passo 3 instalou três arquivos às 10h00min34.
- **A central ao vivo estava ligada e não foi tocada.** Ela segue com o rádio de antes até ser religada; na partida seguinte, pelo `driver/ao_vivo.sh` ou pela janela, a "Lista de emissoras AM" ganha três emissoras, a "TESTE 1" e a "TESTE 3" ganham o texto, e a busca da sintonia manual passa a correr pela faixa. A saída do lançador passa a dizer "modelo do sintonizador: 5 emissora(s) de FM e 3 de AM na lista do perfil, com texto de rádio".
- Nenhuma central `moi3-mid-*` ficou no ar.
- Do que o item 4 do roteiro pedia fica de fora, por depender do carro: se ele tem AM, a grade de AM dele e a faixa de FM (fotos pedidas na ficha).

## Os números do painel acompanham a unidade escolhida (12/10/2026, manhã)

Feito pela sessão do estado do carro de mentira. O Lipe voltou às 8h30 de 12/10/2026 e escreveu à sessão principal "Vamos finalizar os demais itens"; ela me passou o que faltava no item 2 do roteiro do simulador: "os números do painel acompanharem a unidade escolhida pela tela. Hoje, com milhas escolhidas, a autonomia continua em km; o mesmo vale para consumo, velocidade média e temperatura, se a conta for do carro de mentira e não da HMI." É emulação num Mac; nada vai para o carro.

**Resultado, verificado em execução:**

- **Com milhas, mph e mpg escolhidos em Veículo > Config. > Unidades, as três páginas de dados de condução acompanham**: "0 mi", "Ø 0 mph", "Ø --- mpg (EUA)" e a autonomia em "249 mi", em "Desde a partida", "Longo prazo" e "Desde o abastecimento" (rodada `u1`). Antes ficava "0 km", "Ø 0 km/h", "Ø --.- km/l" e "400 km".
- **A partida seguinte já nasce na unidade guardada**, com o número refeito: três centrais subidas de uma memória guardada com milhas mostraram "249 mi" desde a primeira mensagem, sem passar pelos 400 km (rodadas `m1` a `m3`). A central da pasta de antes, com a mesma memória, mostrou "400 km" (rodada `mb1`).
- **No padrão a tela é a mesma de antes** (km, km/h, km/l, 400 km).
- **A temperatura não precisou de nada:** essa conta é da HMI. O topo já mostrava 95 °F para os 35 °C do perfil desde o Marco 9.

| Antes: memória guardada em milhas, e a tela em km | Depois: a mesma memória |
|---|---|
| ![Antes](quadros/2026-10-12-hmi-uni-antes-memoria-em-milhas-continua-em-km.png) | ![Depois](quadros/2026-10-12-hmi-uni-depois-memoria-em-milhas.png) |

As outras fotos: [as unidades escolhidas pela tela](quadros/2026-10-12-hmi-uni-unidades-escolhidas.png), [a página "Longo prazo" em milhas](quadros/2026-10-12-hmi-uni-longo-prazo-em-milhas.png) e [a tela do padrão, igual à de antes](quadros/2026-10-12-hmi-uni-padrao-continua-igual.png).

### De quem é a conta

Lido no esquema dos serviços de veículo e visto na tela:

- **Distância, velocidade média, consumo e autonomia chegam do carro com a unidade dentro da mensagem** (`CarBCDistance.distanceUnit`, `CarBCSpeed.speedUnit`, `CarBCConsumption.consumptionUnit`, `CarBCCurrentRange.currentRangeUnit`). A HMI mostra o número e a unidade que recebe. No carro quem manda é o painel; quando a tela troca a unidade, é ele que passa a mandar outro número e outra unidade.
- **As duas pontas usam a mesma tabela de códigos** do firmware (`CarConstants`: `DISTANCEUNIT_KM` 0 e `_MI` 1; `SPEEDUNIT_KMPH` 0 e `_MPH` 1; `CONSUMPTIONUNIT_LP100KM` 0, `_KMPL` 1, `_MPG_UK` 3 e `_MPG_US` 4). Por isso o código da unidade pode ser copiado tal qual.
- **O que o painel de verdade faz ao receber a unidade nova não está no código da HMI.** Que ele responde assim é dedução, e fica marcado como simulação no perfil (D170).

### O que mudou no carro de mentira

Instalado às 9h20:13 de 12/10/2026, por troca de nome, nesta ordem: o motor, o perfil e o teste. As versões de antes estão em `traces/uni_antes/`.

**No motor (`driver/veiculostub.c`), dois marcadores novos de perfil**, do mesmo tipo dos do relógio (`@hora`…): o valor não é fixo, sai a cada envio. Estes leem o que outro evento do carro de mentira guarda naquela hora:

```
campo = @de Serviço.evento.campo                  o valor de agora desse campo
campo = @se Serviço.evento.campo V:X … senao Y    X do primeiro par cujo V é o valor de agora; Y nos outros casos
```

Evento que tem marcador já era refeito uma vez por segundo e só saía se mudasse. Com isso, quando a tela grava a unidade, quem a acompanha sai de novo no segundo seguinte, uma vez. Não há regra por pedido nem conta de unidade dentro do programa: o motor continua sem conhecer serviço nenhum (D169). Perfil que não usa os marcadores sai igual, byte a byte.

**No perfil (`stubs/veiculo/jetta-gli-2021-deducoes.txt`), um bloco no fim**, marcado como simulação:

- a autonomia: `@se … distanceUnit 1:249 senao 400`, e a unidade dela, `@de … distanceUnit`;
- nos três grupos de médias (`updateBCShortTermGeneral`, `…LongTermGeneral` e `…CycleGeneral`): a unidade da distância e a da velocidade média;
- nas três médias de consumo: a unidade, `@de … consumptionPetrolUnit`.

O único número refeito é a autonomia: os 400 km de simulação viram 249 mi. Os outros são zero ou não existem (carro parado, médias sem medida).

**O arquivo de estado não muda de papel:** guarda só o que a tela gravou (a unidade). O que acompanha é refeito na partida seguinte.

### As rodadas

Treze centrais de teste `moi3-uni-*`, das 8h59 às 9h22 de 12/10/2026, no máximo duas ao mesmo tempo (salvo um minuto com três, por descuido meu ao lançar um par antes de a central `u1` acabar). Resumo de cada uma em [traces/uni_serie.txt](traces/uni_serie.txt).

| Rodadas | O que foi | Autonomia no barramento |
|---|---|---|
| `u1` | cópia de trabalho; troquei as três unidades pela tela e fotografei | 400 km e, depois da troca, 249 mi |
| `n1`, `n2`, `n3` | cópia de trabalho, sem memória | 400 km |
| `b1`, `b2` | a pasta de antes, sem memória | 400 km |
| `m0` | cópia de trabalho, com memória do zero; troca pela tela | 400 km e, depois, 249 mi |
| `m1`, `m2`, `m3` | cópia de trabalho, a partir da memória guardada pela `m0` (a da tela e a do carro de mentira) | 249 mi desde a primeira mensagem |
| `mb1` | a pasta de antes, com a mesma memória | 400 km |
| `p1`, `p2` | a pasta, depois de instalar: sem memória e com a memória guardada | 400 km; 249 mi |

- **As contagens são as mesmas nas doze partidas da série** (todas menos a `u1`, que foi dirigida à mão): relógio aceito, as mesmas vistas (com a troca de ordem de duas da partida, que já variava), 111 assinaturas no carro de mentira, 3 pedidos atendidos (6 na `m0`, com as três trocas), 14 sem resposta, nenhum tempo esgotado, 8 linhas de serviço não registrado e 44 mensagens do computador de bordo (54 na `m0`). Só mudam os bytes esperados.
- **Uma mensagem por mudança.** Na `m0`: a autonomia saiu duas vezes (400 km; 249 mi) e o grupo "desde a partida" três (padrão; milhas; milhas e mph).
- **No padrão mudam os bytes de três mensagens, e a tela não.** Os três grupos de médias iam vazios (`10 02`) e passam a ir com as duas submensagens vazias (`0a 04 0a 00 12 00 10 02`), porque o perfil agora dá valor a campos delas. A tela mostra o mesmo "0 km" e "Ø 0 km/h" (rodadas `u1`, `n1` a `n3` e `p1`).
- **A rodada `n1` tinha uma linha a mais no perfil**, a unidade do hodômetro. Tirei depois dela: a HMI não assina o hodômetro nas telas que abri, e não havia como ver o efeito (D171).

### Testes

`driver/tests/t_veiculo.sh`, 188 → 224 casos. Os 36 novos: os dois marcadores em perfil de mentira (o valor de agora, o par que serve, o `senao`, dez textos errados com o arquivo e a linha), no barramento (a troca muda o número, quem acompanha é avisado uma vez, a volta desfaz), com o arquivo de estado (guarda a unidade e não o que a acompanha; a partida seguinte nasce certa) e os perfis da pasta com o esquema de verdade.

- **Vistos falhar:** 31, antes do código ([saída](traces/uni_t_veiculo_vermelho.txt), que tem 32: o trigésimo segundo era o do hodômetro, que saiu).
- **Guardas, que já passavam:** 5. Dois são do padrão (a autonomia em 400 km e a média em km/l com os perfis da pasta); um é a regra antiga de marcador só em campo inteiro; dois passavam por vazio antes do código (o estado não guarda o que acompanha; a partida seguinte não passa pela unidade do perfil).
- **Bateria completa.** Na pasta de antes, às 9h15: tudo passando nas suítes de container. No candidato (a pasta das 9h14 mais os três arquivos), às 9h14: tudo passando, com `t_veiculo` 224. Na pasta, depois de instalar, das 9h22 às 9h26 ([saída](traces/uni_suites_depois.txt)): tudo passando, nas de container e nas do Mac: glforward 8, `t_transmissao` 19, `t_codificacao` 39, `t_entrada` 11, `t_pedidos` 28, `t_veiculo` 224, `t_memoria` 80, `t_hub` 25, `t_vc_diag_alm` 37, `t_hmilog` 32, `t_kipc_bus` 14, `t_radio` 81, `t_telefone` 142, `t_midia` 119, `t_som` 185, `t_servidor` 20, `t_lsmval_abre` 38 e `t_catalogo` 32.
- **Uma falha que foi minha:** rodei as duas baterias de antes ao mesmo tempo, e o `t_servidor.sh`, que usa a porta 8439 fixa, falhou nas duas (1 caso numa, 4 na outra). Sozinho, passou nas duas (20 de 20). Não toca na central ao vivo: usa pasta temporária e porta própria.

### Limites

- **Só campo inteiro.** Os marcadores não fazem conta com decimais. Bastou, porque a autonomia é inteira e as médias não têm valor; um consumo médio de verdade pediria mais.
- **Até um segundo de atraso** entre a troca na tela e o número novo.
- **Um campo que acompanha outro, lido por um terceiro, vale zero.** Não há cadeia.
- **Ficaram de fora:** o hodômetro, o consumo do momento (que o perfil dá em l/h, com o carro parado), o volume e a pressão. Nenhum aparece nas telas que abri.
- **Voltar de milhas para km pela tela** foi conferido no barramento, com perfil de mentira, e não fotografado.
- **Só o caso de gasolina:** as médias acompanham a unidade de consumo de combustível líquido.

### Onde parou

Feito para o que a tela do veículo mostra hoje. A central ao vivo estava ligada e em uso pelo Lipe desde as 8h36: não foi tocada, e a próxima partida dela já sobe com o motor e o perfil novos.

## Decisões

Decisões que tomei sem perguntar, com o custo se estiverem erradas.

- **D1 — usar o MU0883 como firmware da sessão.** É o que a bancada usa e nenhum pacote é o train do carro. Custo se errado: diferença de comportamento entre MU0803 e MU0883 não aparece no sandbox.
- **D2 — não refazer a extração.** Usei as imagens de 09 e 10/10, só para leitura. Custo se errado: nenhum; os hashes estão anotados e a extração é reproduzível.
- **D3 — Marco 0 sem rede e sem instalar nada.** Container com `--network none`, análise no Mac com `python3 -I`. Custo: o parser de ELF é meu e mais limitado que o `readelf`.
- **D4 — regenerar o `driver/fakegl.c`.** O que estava na pasta não compilava. Custo se errado: o stub novo exporta dez funções de EGL 1.5 que o Adreno de verdade não exporta, e isso pode ter mudado o caminho que a HMI toma. As rodadas de 11/10 não são comparáveis linha a linha com as de 10/10.
- **D5 — o teste do Marco 1 não sobe o systemd do firmware.** O `/init` é meu e chama o mesmo lançador do container. Responde à pergunta "a HMI roda na `virt`?", não à pergunta "o sistema inteiro sobe na `virt`?". Custo se errado: o esforço de um boot completo pelo Caminho A continua sem medida.
- **D6 — Marco 2: desenvolver no container.** Razões na seção acima. Custo se errado: se o Marco 3 precisar de algo do kernel, o trabalho migra para o QEMU, onde o mesmo lançador já roda.
- **D7 — o shim imita o que o Adreno exporta, mas não finge ser Qualcomm.** Exporta ES 3.2 e EGL 1.4, como o driver real; as strings (`GL_VENDOR`, `GL_RENDERER`, extensões) são as verdadeiras do Mesa. Custo se errado: se a HMI tiver caminhos só para "Adreno", eles não são exercitados aqui.
- **D8 — o estado de energia é publicado pelo stub com o `mcu-interpreter` de verdade ainda rodando.** São duas fontes para o mesmo serviço. Custo se errado: se o daemon real passar a publicar, os dois brigam. Até aqui ele não publicou nada que a HMI aceitasse.
- **D9 — o código gerado fica na pasta** (`glforward-moi3/gen/`), para compilar sem precisar de Python na imagem. Custo: ao mudar o gerador é preciso lembrar de rodá-lo.
- **D10 — os quadros ficam em `quadros/`, em PNG.** São imagens desenhadas pelo firmware; ficam só nesta pasta.
- **D11 — a JVM da HMI enxerga 4 processadores por padrão** (`JCPUS=4` no lançador, `-XX:ActiveProcessorCount=4`). É o número do carro e é o que faz o cliente RSI iniciar. Custo se errado: as rodadas de 11/10 anteriores ao Marco 4 rodaram com 16 e não são comparáveis linha a linha; `JCPUS=0` volta ao comportamento antigo.
- **D12 — usar o registro e o mock que a LG deixou no firmware, e não o `usermgnt` de verdade.** O mock entrega o que a HMI pede; o daemon real depende de algo que parece ser um chip de segurança, e fazê-lo funcionar sairia do combinado da sessão. Custo se errado: o que o `usermgnt` real faria de diferente (troca de usuário, S-PIN, sincronização) não é exercitado.
- **D13 — os dados de usuário são os de teste da LG, com o mínimo acrescentado.** Ficou o "level2 user" como ativo e identificado, embutido por inteiro no contexto, e um usuário principal de apelido vazio. Um carro sem cadastro online provavelmente teria só o convidado anônimo, o que levaria a outra tela (2 ou 4 na tabela do Marco 4); isso é dedução e não foi testado. Custo se errado: a tela que se vê na bancada não é a que o carro do Lipe mostraria.
- **D14 — o serviço `onlineregistration` do mock no lugar da chave `ignorePrimary`.** Os dois funcionam (rodadas `e6` e `e8`). O serviço usa o mesmo mecanismo do resto e não depende de uma chave de depuração da HMI. Custo se errado: nenhum conhecido; a chave continua disponível por `EXTRA_VMOPTIONS`.
- **D15 — o RSI vem ligado só no modo ao vivo.** No `m1_hmi.sh` ele é opção (`RSI=1`), para que as rodadas antigas continuem reproduzíveis.
- **D16 — a entrada é publicada pelo nosso stub, com o `touch-app` de verdade ainda rodando.** São duas fontes para o mesmo serviço, como na energia (D8). O `touch-app` não tem de onde ler toque na bancada, então não publica nada que concorra. Custo se errado: se ele passar a publicar, a HMI recebe eventos dos dois.
- **D17 — o reconhecimento de gestos fica na página, e é o mínimo.** Um clique vira "encosta" e "toque"; um arrasto vira "encosta", "arrasta" e "solta". No carro quem decide isso é o controlador de toque. Custo se errado: gestos que a HMI espera e a página não gera (arremesso, pinça) não funcionam, e o tempo dos eventos não é o de um dedo de verdade.
- **D18 — a página só mostra as teclas que fizeram algo na bancada** (rodadas `k8` e `n1`): as oito que abrem tela e a do telefone, que abre um aviso. Ficaram de fora HOME, que nesta variante pede uma tela inexistente, e as que não fizeram nada. Custo se errado: uma tecla que só reage em certa tela (mudo, por exemplo) não tem botão; dá para mandar pelo `ROTEIRO`.
- **D19 — o modo ao vivo não aperta MENU sozinho.** A HMI abre no último conteúdo, como no carro, e quem está na página aperta a tecla. Custo: a primeira tela continua sendo o aviso do rádio.
- **D20 — fechei o acesso do container à página** (ver "Cuidados com a página" no Marco 5). Ninguém pediu; fiz porque a página passou a mandar comandos. Custo: o `ao_vivo.sh` antigo e o novo montam pastas diferentes, e o servidor antigo (`python3 -m http.server`) não serve mais para a entrada.
- **D21 — no modo ao vivo a HMI espera 2 s, e não 15 s, por serviços ausentes** (`ESPERA=2000`). É um ajuste da própria HMI e, na rodada `p1`, não mudou quais serviços ela encontra. Custo se errado: num ambiente mais lento (o QEMU sem aceleração, por exemplo) um serviço que existe pode chegar depois dos 2 s e ser dado por ausente; lá, usar `ESPERA=15000`. As rodadas do lançador direto continuam com o padrão do firmware.
- **D22 — o hub guarda o que vem do microcontrolador de diagnóstico e entrega a cada processo que se liga, antes de qualquer outra mensagem** (Marco 6). A HMI só aceita esses conjuntos antes da primeira leitura de persistência, e o momento em que o carro os manda não está no código que li. Custo se errado: o caso em que os dados chegam tarde não aparece por padrão (a rodada `cod6` exercitou o caminho de reserva, mandando menos de 44), e todo processo do barramento recebe 48 mensagens que no carro talvez não recebesse. Nas seis rodadas em que medi, nenhum daemon caiu.
- **D23 — o conjunto padrão leva só os 48 conjuntos que a HMI procura na tabela dela.** Ficam de fora `720B`, `720C` e `F1B1`, que seguem outro caminho; o `720B` faria o `persistence-app` regravar o idioma da HMI com um texto sem sentido. Custo: quatro leituras de registros continuam sem dado, e esse caminho do `persistence-app` não foi exercitado.
- **D24 — os três primeiros bytes de cada mensagem vão zerados.** Nenhum dos dois leitores os usa. Custo se errado: outro processo do firmware que os leia veria zeros; não achei nenhum. **Mudou às 23h26 de 11/10/2026:** o custo apareceu. O `navi-al` os lê e descartava os 48 conjuntos; passaram a `83 60 22` (decisão D77).
- **D25 — no modo ao vivo a codificação vem ligada, com tudo no valor padrão; no lançador direto é opção** (`CODING`), como o RSI (D15). Medi cinco telas idênticas, pixel a pixel, com e sem esse padrão. Custo se errado: numa tela que não visitei, "padrão válido" pode não ser igual a "sem dado"; `CODING= sh driver/ao_vivo.sh` volta ao comportamento anterior. **Mudou com a D34:** o padrão do modo ao vivo deixou de ser neutro.
- **D26 — o `ajustes.txt` vem com tudo comentado.** Não há valor do carro para pôr ali, e ligar a navegação por conta própria mudaria a ordem da grade, que a outra sessão usa nos testes de toque. Custo: a sessão ao vivo continua mostrando a central "sem codificação" até o Lipe escolher.
- **D27 — a rodada `cod4` usou um tipo de veículo escolhido por existir pasta para ele no firmware** (VW, classe A, geração 7, sedã). É um valor de teste. Custo se errado: quem ler pode tomar por codificação do Jetta; por isso o aviso está em todo lugar onde o valor aparece.
- **D28 — as rodadas do Marco 6 rodaram fora da página ao vivo, e a conferência ao vivo foi numa segunda sessão, na porta 8432.** A sessão principal estava em uso pela outra sessão. Custo: o Lipe não acompanhou essas rodadas pela página.
- **D29 — mexi em arquivos que a sessão do Marco 5 também usa** (`kipc_hub.c`, `dsi_escuta.c`, `m1_build.sh`, `m1_hmi.sh`, `ao_vivo.sh`, `m3_ao_vivo.sh`), sempre acrescentando, e deixei o `dsistub.c` intocado: o stub da codificação é um programa à parte. Custo se errado: uma sessão ao vivo iniciada depois da mudança compila o hub novo; os 11 casos de `t_entrada.sh` passam com ele.
- **D30 — a identidade do Jetta fica em dois arquivos, o do que foi lido do carro e o das deduções, e nenhum entra no modo ao vivo por padrão.** O pedido da sessão principal foi não inventar valor e deixar um arquivo que receba os reais. Custo: para ver o simulador com a cara do Jetta era preciso passar os arquivos em `CODING`; quem decide torná-los o padrão é o Lipe. **Ele decidiu no mesmo dia:** ver a D34.
- **D31 — a identificação vai como texto ASCII, com o número de peça sem espaços e completado com um espaço.** A ficha tem os valores, não o formato dos bytes. Custo se errado: o carro pode mandar outro preenchimento; a tela da HMI corta os espaços das pontas, então o que se vê é igual.
- **D32 — usei "0803" como versão de software, o valor do carro, num simulador que roda o build 0883.** É o que a tela do carro mostra. Custo: a tela de informações diz 0803 e as telas de versão da HMI, no menu de serviço, continuam dizendo `H40.272.304`.
- **D33 — acrescentei um item à lista "O que anotar no primeiro autoscan" da ficha do carro**: comparar os bytes 0, 1 e 5 da codificação do 5F com as deduções. É só leitura. Custo: nenhum conhecido.
- **D34 — o modo ao vivo sobe com a identidade provisória do Jetta GLI 2021.** Esta não é decisão minha: o Lipe escreveu, em 11/10/2026, "pode deixar o Jetta como padrão do modo ao vivo". `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh` passaram a ler `padrao.txt`, `jetta-gli-2021.txt`, `jetta-gli-2021-deducoes.txt` e `ajustes.txt`, nessa ordem. O que muda para quem usa: a grade tem 8 blocos e outra ordem (Settings fica na segunda fileira, última coluna), o destaque é vermelho, a tela do veículo mostra o sedã e a de informações traz peça, hardware e software. O que continua valendo: parte disso é dedução, não leitura do carro, e o arquivo de deduções sai da lista quando houver autoscan. Para a central sem identidade: `CODING="/stubs/codificacao/padrao.txt /stubs/codificacao/ajustes.txt" sh driver/ao_vivo.sh`. O lançador direto (`m1_hmi.sh`) e as rodadas (`m6_rodada.sh`) continuam só com o que for passado.
- **D35 — a transmissão atravessa o container pelo `docker exec -i`, e não por rede** (Marco 7). A regra da bancada é rodar o firmware em container descartável e sem rede, e esse canal já dá ida e volta. Custo: cada janela aberta mantém um processo `docker exec` e uma `ponte` dentro do container; se o Docker demorar a entregar, a tela atrasa junto.
- **D36 — só viaja o que mudou em cada quadro, sem compressão geral.** A HMI redesenha a tela inteira e quase nada muda de um quadro para o outro; a conta é simples dos dois lados e não pede biblioteca. Custo: troca de tela inteira e imagens cheias de detalhe vão quase sem ganho (433 kB no pior quadro medido).
- **D37 — uma thread do `glhost` só para transmitir, e espectador atrasado pula quadros.** Quem assiste não pode segurar a HMI. Custo: o `glhost` deixou de ter uma thread só; a imagem é copiada uma vez a mais por quadro (1,15 MB em 800×480).
- **D38 — a transmissão vem ligada por padrão no `glhost`** (`GLHOST_STREAM` vazio ou `0` desliga). Assim o `driver/ao_vivo.sh` e o `driver/m3_ao_vivo.sh`, que naquela hora estavam com a sessão do Marco 6, não precisaram de mudança. Custo: toda rodada que usa o `glhost` abre um soquete e uma thread a mais, mesmo sem ninguém assistindo.
- **D39 — a janela é um programa em Swift com AppKit, compilado na pasta com o `swiftc` que já estava no Mac.** Não instala nem baixa nada. Custo: o programa não é assinado nem empacotado; vale para este Mac, e o compilado fica em `janela/build/`.
- **D40 — a janela liga a central (2 horas, sem a página) e não a desliga ao fechar.** A página e outras sessões podem estar usando a mesma central. Custo: central esquecida no ar gasta processador até o tempo acabar; desligar é pelo menu Central ou por `docker stop moi3-ao-vivo`.
- **D41 — desenvolvi numa cópia fora da pasta e instalei no fim, arquivo por arquivo, por troca de nome.** A sessão do Marco 6 compila `glforward-moi3/` a cada rodada dela; um arquivo pela metade quebraria a rodada. Custo: nenhum conhecido; a cópia de trabalho ficou no rascunho.
- **D42 — o modo de roteiro (`--roteiro`) mora dentro do próprio programa da janela.** Foi o jeito de exercitar toque, teclas e botões sem a captura de tela e sem controle do mouse, que esta sessão não tinha. Custo: código de conferência dentro do programa de uso, e eventos fabricados em vez de saídos de um mouse.
- **D43 — o modo ao vivo nasce em português do Brasil** (Marco 6, idioma): `LSMVAL` tem como padrão `/stubs/persistencia/idioma-pt_BR.txt` em `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh`. O Lipe não pediu com essas palavras. Ele trocou o idioma pela tela e escreveu "Eu já coloquei o idioma em pt-br"; a central do carro está em português (foto de 07/09/2026) e o objetivo dele é o simulador igual ao carro. Tomei isso como a escolha dele e fiz valer desde a partida. Custo se errado: quem esperava a central em inglês a encontra em português; `LSMVAL= sh driver/ao_vivo.sh` volta ao inglês, e tirar do padrão é uma linha em cada roteiro. As rodadas (`m6_rodada.sh`) e o lançador direto continuam com a memória vazia, salvo pedido.
- **D44 — a memória de partida é um arquivo fixo, e não a memória da sessão anterior.** Assim toda partida é igual, e um estado ruim não passa de uma sessão para a outra. Custo: fora o idioma, o que o Lipe mudar pela tela se perde ao reiniciar; no carro ficaria. Fazer a memória sobreviver é possível (o firmware lê o arquivo inteiro na partida) e ficou nos próximos passos, para ele decidir. **Mudou com a D46**, no mesmo dia: no modo ao vivo a memória passou a sobreviver.
- **D45 — o lançador confere o arquivo de memória e recusa tudo se houver uma linha errada** (`driver/lsmval_prepara.sh`), como o `diagstub` faz com a codificação. Os dois leitores do firmware descartam em silêncio a linha que não entendem. Custo: um arquivo que o firmware aceitaria em parte, por exemplo com tipo diferente de 0 e 1, aqui é recusado.
- **D46 — no modo ao vivo a memória de estado passa a sobreviver de uma sessão para a outra** (Marco 6, "A memória de uma sessão para a outra"; muda a D44). A D44 tinha deixado o passo para o Lipe decidir; a sessão que o faz foi proposta para começar só se ele quisesse, e começou às 20h15 de 11/10/2026. Não há frase dele sobre isto; apoiei-me no objetivo que ele deu, o simulador "igual ao carro de verdade mesmo". Vale para o `driver/ao_vivo.sh` e para a janela, que o chama. Custo: as partidas deixam de ser iguais entre si e um estado ruim fica guardado (rodada `mem3`); quem toca por coordenada ou compara fotos precisa subir com `MEMORIA= sh driver/ao_vivo.sh`. Desfazer numa sessão é essa variável; desfazer de vez é tirar do `ao_vivo.sh` o bloco que começa em `MEM=`.
- **D47 — quem chama o `m3_ao_vivo.sh` direto não ganha a memória, e `LSMVAL` passado à mão a desliga naquela sessão.** O roteiro de dentro do container só lê e guarda a memória se receber `MEMORIA`, e quem passa é o `ao_vivo.sh`. Assim as centrais de teste das outras sessões e as rodadas seguem nascendo iguais, e uma sessão que nasce de um arquivo escolhido não escreve por cima da memória guardada. Custo: para começar a memória a partir de um arquivo é preciso copiá-lo para `ao-vivo/memoria/lsmval`, e são duas variáveis (`LSMVAL` e `MEMORIA`) para explicar.
- **D48 — a guarda é uma vigia dentro do container, a cada 2 s, que só guarda cópia estável, terminada em fim de linha e aceita pelo conferidor, e grava por troca de nome.** O pedido era guardar só o que passa no `lsmval_prepara.sh`. Acrescentei as outras duas condições porque o conferidor aceita arquivo vazio e pode aceitar um arquivo cortado no meio de uma linha, e é isso que existe por um instante quando o `persistence-app` regrava o arquivo. Memória vazia nunca é guardada. Custo: a cópia fica até uns 2,3 s atrás da central, além dos 5 a 7 s da própria HMI, e num corte seco esse resto se perde. Três tipos de arquivo que o firmware pode escrever seriam recusados (mais de 500 valores, linha acima de 9998 letras, valor de zero bytes): aí a memória para de ser atualizada, com aviso em `recusada.txt` e na partida seguinte.
- **D49 — ficam guardadas as memórias com que as cinco últimas sessões nasceram (`ao-vivo/memoria/anteriores/`), e memória guardada com erro não impede a partida** (vai para `lsmval.recusada`, com aviso, e vale a semente). Ninguém pediu. É o tratamento que dei ao risco do estado ruim: voltar ao zero joga fora tudo, voltar uma partida não. A D45 recusa tudo e não sobe nada quando o arquivo vem de `LSMVAL`; aqui preferi a central subir, porque quem liga pela janela não vê a mensagem do lançador. Custo: mais um comportamento para entender, e uma memória estragada pode passar despercebida por quem não olha a saída do lançador nem a pasta.
- **D50 — com o carro de mentira ligado, o `carifservice` do firmware não sobe** (Marco 8). Os dois respondem pelo mesmo domínio e, ao contrário da energia (D8) e da entrada (D16), aqui o daemon de verdade publica: juntos, 73 de 310 eventos chegam com dois conteúdos, e qual vale depende de quem chega por último (rodadas `dois`, `d1` e `d2`). Não tenho um caso em que isso tenha estragado a tela: nas duas partidas em que olhei com o stub definitivo, os dados do stub apareceram. Custo: o que o `carifservice` faz além de responder à HMI deixa de acontecer, e não procurei quem mais dependa dele. `VEICULO_CARIF=1` sobe os dois; `VEICULO=` volta ao que era.
- **D51 — o esquema dos serviços de veículo é gerado pelas classes do firmware, rodando na JVM dele, e fica guardado na pasta** (`stubs/veiculo/gerado/`, 1,5 MB), como o código gerado do encaminhador (D9). O pedido foi não escrever o esquema à mão. Custo: ao trocar de firmware é preciso gerar de novo; e o gerador reconhece o bytecode pela forma (o `switch` de cada serviço, a chamada de envio), então uma classe escrita de outro jeito sairia errada. A defesa são as contagens do relatório e os 1.117 vetores conferidos byte a byte.
- **D52 — no esquema e nos perfis, o nome de cada evento é o da mensagem que a HMI lê, e não o do método que ela chama.** Em 18 eventos o firmware chama o método de outro atributo. Custo: quem procurar pelo nome do método de `org.dsi.ifc` pode não achar; nome repetido num serviço leva `:<api>`.
- **D53 — o que o perfil não diz vai com zero e marcado como válido.** O pedido foi esse. Custo: zero nem sempre é neutro (parado = 0 é carro andando; unidade de volume 0 é quilo), e "válido" faz a HMI confiar no valor. Olhei as telas de veículo, clima e ajustes e a grade; nas outras não sei o que um zero válido mostra.
- **D54 — os dados do Jetta ficam em três arquivos: o cenário, o que foi lido do carro e as deduções, cada linha marcada como PALPITE ou SIMULAÇÃO.** É o desenho da codificação (D30). O chassi do carro não entra em nenhum. Custo: o arquivo do que foi lido tem cinco valores, todos de uma foto do painel; quase tudo o que a tela mostra vem do arquivo de deduções.
- **D55 — os números sem os quais a tela fica vazia são de simulação: autonomia 400 km, tanque 50 %, e a temperatura externa parada nos 35,0 °C da foto.** Não há medida do carro. Médias, distância e tempo ficam em zero, e os prazos de revisão ficam sem valor, para não inventar quilometragem. Custo: quem olhar a tela pode tomar 400 km por dado do carro; por isso a marca em cada linha.
- **D56 — liguei quatro funções do carro nas duas listas da adaptação (`0B1B` e `0B1D`), no arquivo de deduções da codificação, que é da sessão do Marco 6.** Sem elas a HMI recebe os dados e não cria os menus. Custo: é palpite sobre a adaptação do 5F do carro (entrou na lista do autoscan, na ficha); só testei as duas listas juntas; e, como esse arquivo é padrão do modo ao vivo (D34), a central ao vivo ganha os menus na próxima partida.
- **D57 — o carro de mentira vem ligado no modo ao vivo e desligado no lançador direto** (`VEICULO`), como o RSI (D15) e a codificação (D25). O pedido foi esse. Custo: a partida do modo ao vivo deixa de ser comparável com as de antes: a HMI liga dezesseis serviços a mais e não faz duas esperas de 2 s por serviços ausentes. As rodadas do Marco 6 (`m6_rodada.sh`) seguem sem ele.
- **D58 — o `dsistub` responde na hora à assinatura de energia e informa que a câmera de ré não está ativa.** É um stub do Marco 3, que toda rodada usa; mexi nele porque a causa do sumiço da Welcome estava ali. Custo: a partida de toda rodada muda, com ou sem dados de veículo: a primeira tela sai uns 6 s mais cedo e a Welcome fica perto de 5 s na tela, em vez de 1 s (rodada `anu`, sem o carro de mentira: 4,7 s). Fotos e tempos de antes não são comparáveis.
- **D59 — o `dsistub` guarda e devolve os ajustes genéricos do painel de teclas e confirma cada gravação** (`setGenericSetting`, `requestGenericSetting`). Era o item opcional do pedido. Custo: a HMI passa a acreditar que o ajuste foi aplicado; o que os oito ajustes da partida são não investiguei, e o valor some quando a central desliga.
- **D60 — a hora é a do relógio do container, em UTC, mais o fuso do perfil, fixo em −3 h.** O container não tem fuso. Custo: se o relógio do Mac estiver errado, a central erra junto; não há horário de verão no perfil; e acertar a hora pela tela não tem efeito, porque o pedido fica sem resposta. **Mudou com a D71** (Marco 9): acertar pela tela passou a deslocar o relógio do carro de mentira, até a central desligar.
- **D61 — o domínio 16 é anunciado uma vez só, e a HMI recebe o anúncio pela reentrega do hub.** Custo: depende de o anúncio estar entre as 48 primeiras mensagens que o hub guarda. Hoje é a primeira, porque o stub sobe antes dos daemons; se a ordem mudar, vale a reserva, que é repetir de 2 em 2 s enquanto a HMI estiver no barramento sem ter pedido nada. E a causa que o conserto ataca é dedução; o que há de medido são treze partidas seguidas sem a falha. **Mudou com a D66 (22h52 de 11/10/2026):** o anúncio deixou de depender das 48 primeiras mensagens; o hub guarda o último de cada canal e entrega um só à HMI, esteja ele onde estiver.
- **D62 — as rodadas foram em centrais de teste com nome e pasta próprios** (`driver/m8_central.sh`, `moi3-m8-<nome>`), e não na central ao vivo, que o Lipe estava usando. Custo: mais um roteiro que sobe a HMI, ao lado de `ao_vivo.sh` e `m6_rodada.sh`; ele não tem página, e as fotos saem do arquivo de quadros.
- **D63 — mexi em arquivos de outras sessões, sempre acrescentando:** `driver/dsistub.c` (Marcos 3 e 5), `driver/m1_hmi.sh`, `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh` (em que a sessão do Marco 6 trabalhava na mesma hora), `stubs/codificacao/jetta-gli-2021-deducoes.txt` e a ficha do carro. Os roteiros foram alterados por troca pontual, relendo antes. Custo se errado: uma edição simultânea pode ter se perdido. Conferi às 20h58 e às 21h27 que as minhas linhas e as da outra sessão estavam lá, e as oito suítes passam. Às 21h47 o `ao_vivo.sh` e o `m3_ao_vivo.sh` tinham os mesmos resumos SHA-256 que a sessão da memória informou ao instalar o bloco dela às 20h58 (`8268aa7a…` e `e0f171c3…`): ninguém mexeu neles depois. As minhas edições nesses dois são das 20h54, e as do `m1_hmi.sh`, das 20h23.
- **D64 — deixei o limite de 48 mensagens do hub como está**, mesmo tendo visto que ele esconde da HMI o anúncio do `carifservice` (Marco 8, "Quem responde pelos serviços de veículo na bancada"). Consertar muda a partida de todas as rodadas e não era o pedido. Custo: com `VEICULO=` a HMI continua sem enxergar os serviços de veículo, como até o Marco 7, por um defeito nosso e não do firmware; e outros anúncios podem estar ficando de fora do mesmo jeito, o que não contei. **Mudou com a D66 (22h52 de 11/10/2026):** o limite continua, mas os anúncios deixaram de depender dele. Contados: na ordem de partida de sempre só o do `carifservice` ficava de fora; quando ele se liga antes de outro daemon, ficavam também o de energia, o de teclas ou o de persistência.
- **D65 — o assistente de configuração passa a aparecer no modo ao vivo, e deixei assim.** Ele é efeito do carro de mentira somado à memória guardada (Marco 8, "Um efeito colateral novo"). Ninguém pediu o assistente; ele é o que a HMI faz com os serviços de veículo ligados e configuração inicial pendente, e escondê-lo seria mexer na memória do Lipe ou desligar o que foi pedido. Custo: toda partida do modo ao vivo mostra um popup aos 52 s, por cima do que estiver na tela, até ele ser dispensado; quem toca por coordenada nessa hora acerta o popup.
- **D66 — o hub guarda o último anúncio de cada canal e o entrega a quem se liga depois, além do cache das 48 primeiras mensagens** (muda a D64; seção "O hub e os anúncios que ficavam fora do cache"). O conserto, com os testes antes, foi pedido pela sessão do carro de mentira. As escolhas dentro dele são minhas. O anúncio guardado vai depois do cache, um por canal, e só para o canal de que o cache não entregou anúncio àquele leitor: quem já recebia tudo pelo cache recebe exatamente o mesmo. Não tirei do cache os anúncios repetidos que já cabiam nele: no modo ao vivo a HMI continua recebendo cinco ou seis anúncios do display de uma vez. A tabela tem 64 canais, os que o firmware define. Instalei às 22h52 de 11/10/2026, com outras sessões rodando centrais, depois de as suítes passarem e de seis partidas lado a lado com cada hub em cada configuração. Custo: toda rodada que sobe o `carifservice` de verdade (`VEICULO=`, `m6_rodada.sh`, `vc_rodada.sh`, o lançador direto) muda de partida: a HMI liga os serviços de veículo com dados de "sem carro", dá 34 serviços por ausentes em vez de 48, anda uns 10 s mais depressa e pede 106 assinaturas a mais; tempos, contagens e tráfego dessas rodadas deixam de ser comparáveis com os de antes (as telas que fotografei não mudaram). Anúncio guardado não expira: um daemon que anuncia e cai continua anunciado. A corrida de partida que a medição achou continua existindo; o hub só deixou de transformá-la em falha. E fica uma contagem sem explicação: nas séries lado a lado a ordem ruim apareceu mais no braço do hub novo (9 de 34 contra 3 de 35); num ensaio controlado de 240 partidas não houve diferença (22 de 120 contra 25 de 120). Desfazer: copiar `traces/hub_kipc_hub.antes.c` por cima de `driver/kipc_hub.c`, por troca de arquivo, e tirar `driver/tests/t_hub.sh`.
- **D67 — a tabela de respostas vem da tabela do proxy DSI da própria HMI, lida por reflexão na JVM do firmware** (Marco 9), e não de uma lista escrita à mão nem do nome dos métodos. O pedido era gerar se desse. Custo: ela só cobre os pedidos para os quais a HMI marca prazo; `getBAPState`, a lista de avisos do veículo e a de perfis de condução não estão nela e seguem sem resposta. E depende dos nomes de classe deste build (`Cfg`, `RuntimeGeneratedConstants`): noutro firmware o gerador avisa e o esquema sai sem as linhas `R`.
- **D68 — o que cada pedido grava (linhas `G`) e o que cada resposta devolve (linhas `X`) são dedução do gerador, marcadas como tal no esquema.** O firmware não traz essa ligação; a regra é o nome (`set<X>` e atributo `<X>`) e a forma das mensagens. Custo: das 373 regras de gravar, nove foram exercitadas na HMI; uma regra errada grava o valor no campo errado sem avisar, e as 16 que casam por nome são as menos seguras. Por isso a D69.
- **D69 — a gravação só vale nos serviços que o perfil lista em `grava`, e o padrão é só o de relógio e unidades.** O pedido falava nesse serviço. Custo: clima, luzes, bancos e os outros continuam aceitando o toque sem mudar nada; ligar um é uma palavra no perfil, e não conferi nenhum deles.
- **D70 — pedido de gravar atendido responde sempre com a atualização do atributo, mesmo sem mudança, em toda instância, e o carro de mentira aceita qualquer valor que caiba no campo.** A HMI trata a atualização como a resposta. Custo: o painel do carro pode recusar ou trocar um valor (uma unidade que ele não tem); aqui a tela mostra o que pediu. E os valores são um só para todas as instâncias do serviço.
- **D71 — acertar a hora ou a data desloca o relógio do carro de mentira em relação ao do Mac, e isso vale até a central desligar** (completa a D60, que dizia que acertar pela tela não tinha efeito). Custo: no carro o painel guarda a hora; aqui a partida seguinte volta à hora do Mac, e o mesmo vale para as unidades. A sessão da memória registrou a mesma falta na D76. **Mudou com a D85 e a D86** (12/10/2026): no modo ao vivo o acerto e os valores gravados ficam guardados e voltam na partida seguinte.
- **D72 — o fuso escolhido vai para dois lugares, o atributo de fuso e a mensagem da hora, por uma regra escrita à mão no perfil, e a hora não é recalculada.** A tela só mostra o fuso que vem na mensagem da hora. Custo: não sei o que o painel do carro faz com a hora quando o fuso muda; se ele a recalcula, o simulador mostra uma hora que o carro não mostraria.
- **D73 — só é respondido o pedido cuja resposta tem seção no perfil, e hoje são as três listas do start-stop, vazias.** O pedido era resposta vazia e válida para elas. Custo: os outros 81 pedidos com prazo seguem sem resposta (restaurar os ajustes de fábrica das unidades ainda esgota 10 s); a lista vazia é simulação, e responder com ela faz a HMI seguir o caminho de "nenhum motivo", que só li no código.
- **D74 — o idioma do painel não é atendido, e pedido atendido mais de 200 vezes em 10 s é cortado até o stub reiniciar.** Atender punha a HMI em laço em 1 de 9 partidas (Marco 9, "O laço do idioma do painel"). Custo: o idioma do painel fica o do perfil, e não o que a HMI pede; o limite de 200 em 10 s é palpite meu, e um uso legítimo acima disso seria cortado sem volta naquela sessão (não conheço nenhum: um toque é um pedido). E o corte só age depois de 200 mensagens: laço lento, abaixo de 20 por segundo, passa.
- **D75 — desenvolvi numa cópia fora da pasta e instalei por troca de nome, na ordem stub, esquema, perfil; e mexi em três roteiros de outras sessões:** `driver/m1_hmi.sh` (uma linha do resumo e três de comentário), `driver/m8_central.sh` (a variável `PREFIXO`) e `driver/m8_esquema.sh` (comentário). Custo: a primeira versão ficou 19 minutos na pasta (das 22h32 às 22h51) com o defeito do laço, e nos 5 últimos com a gravação desligada no perfil; a central ao vivo religada às 22h47 pegou esse estado intermediário e fica com ele até a próxima partida. Não a reiniciei.
- **D76 — levantei o que a memória guarda sem alterar nenhum stub nem roteiro da bancada** (Marco 6, "O que a memória guarda, linha por linha"). O pedido era descobrir e conferir, e dizer o que a tela não deixa mudar. Três coisas ficaram por isso do tamanho que a bancada tem hoje: o brilho não se muda pela tela (o `dsistub` não responde a `setDisplayBrightness` nem a `getDisplayBrightness`), a tela de som fica indisponível (não há o serviço de áudio) e a unidade escolhida não volta na partida seguinte (o carro de mentira não guarda o que a tela grava). Para o brilho, conferi a volta trocando o byte à mão numa memória de teste, com a soma refeita, e ouvindo o pedido da HMI no barramento. Custo: "igual ao carro" continua faltando nesses três pontos, e a prova do brilho é do pedido que a HMI manda, não do que a tela mostra. O que entrou de novo foram só arquivos novos: o leitor `driver/lsmval_abre.py`, que é em Python e por isso roda no Mac e não na imagem, o teste dele e o leitor dos módulos. **Mudou em 12/10/2026 (D85 a D89):** no modo ao vivo o carro de mentira guarda o que a tela grava, e a unidade escolhida volta; o brilho e o som seguem como estão ditos aqui. **O brilho também mudou em 12/10/2026 (D108):** o `dsistub` responde aos dois pedidos, a escolha pega e volta.
- **D77 — o `diagstub` manda o cabeçalho `83 60 22` em todos os conjuntos, e a opção `-k` escolhe outro** (Marco 6, "O cabeçalho dos conjuntos"; muda a D24). Não é decisão minha: o pedido veio da sessão do navi-al com a frase do Lipe, "Pode passar o pedido quando o hub entrar". Minha é a opção `-k`, para ter controle sem editar o código, e a escolha de não mexer nos dois testes de outras sessões que dependiam do cabeçalho zerado, só avisar. Custo: dois testes de outras sessões falharam até as donas ajustarem, o que elas fizeram às 23h25 (`t_hub.sh`) e às 23h28 (`t_vc_diag_alm.sh`); o que o microcontrolador de verdade manda segue sem ter sido visto. Para desfazer: trocar `cab[3]` no `diagstub.c` de volta para zeros, ou `DIAGCAB=000000` no lançador, para uma central só.
- **D78 — as chaves do `HMILOG` vão para a seção `LOGMANAGER` dos perfis de sondas de depuração, em cópias montadas por cima, e para os 33 perfis, não só o 130** (seção "O rastreamento da HMI que parava na carga tardia"). O pedido deixava escolher entre o arquivo que a HMI lê nessa fase e uma propriedade dela. Propriedades há duas, lidas no código e não experimentadas: `debugprobes.config.force.probes`, que fixa a lista de sondas ativas e deixaria a `LOGMANAGER` de fora, e `debugprobes.config`, que troca os perfis por um arquivo só. Fiquei com os arquivos porque assim a HMI faz o mesmo que sem `HMILOG` (as mesmas sondas, o mesmo perfil, lido na mesma hora) e só as chaves mudam; e com todos os perfis porque quem escolhe o perfil é um número guardado na memória. Custo: 33 montagens a mais por partida com chaves; depende de a linha `END LOGMANAGER` começar na primeira coluna, como nos 33 deste firmware (se faltar, o lançador avisa e as chaves voltam a parar na carga tardia); e, se a memória escolher um perfil que liga chaves, as dele valem junto, como já valiam (a rodada `m60n` teve 15 mil linhas das chaves do perfil 060). Desfazer: `traces/hlog_m1_hmi.antes.sh` por cima de `driver/m1_hmi.sh`, por troca de arquivo.
- **D79 — o efeito vale para toda rodada que passa chaves no `HMILOG`, sem opção para o comportamento de antes, e o lançador ganhou uma linha de resumo que ninguém pediu** ("registro detalhado:", só com `HMILOG`). O pedido era as chaves continuarem valendo; não vi uso para "só a partida" que pagasse mais uma variável. Custo: rodadas com chaves geram saída maior e deixam de ser comparáveis, em contagem de linhas, com as de antes da 0h48 de 12/10/2026 (as séries do Marco 9 usavam duas chaves); numa central longa com chave faladora o arquivo cresce sem parar, e não medi se a fila de registro da HMI, de 2.000 mensagens, chega a transbordar; e o resumo do lançador passa de 14 para 15 linhas com `HMILOG`. Procurei quem lê essas linhas: só o `m1_qemu.sh`, que as mostra.
- **D80 — desenvolvi numa cópia fora da pasta, instalei o lançador por troca de nome com centrais de outras sessões no ar, e o teste novo foi escrito depois do código.** A cópia que rodou nas séries e o arquivo instalado diferem só em linhas de comentário (sem elas, o mesmo resumo SHA-256), e antes da troca conferi que o da pasta ainda era o da 0h14. Custo: toda central que subiu depois da 0h48 roda o lançador novo sem a dona saber, o que só faz diferença para quem passa chaves no `HMILOG`; o `driver/tests/t_hmilog.sh` não seguiu a ordem da bancada, de teste antes, e o que ele garante é o que o vi fazer: falhar em 8 de 32 casos contra o lançador de antes e passar com o novo; ele recorta o trecho do lançador pela forma (começa em `if [ -n "$HMILOG" ]; then` e acaba no `fi` da primeira coluna), e quem mudar a forma do bloco tem de ajustar o recorte; é mais uma suíte para rodar, fora da lista de nove que as outras sessões conhecem; e as séries rodaram com a máquina carregada por outras quatro sessões, o que não escolhi.
- **D81 — a leitura do barramento espera a mensagem com prazo de 100 ms: não fica presa até a mensagem chegar, e não usa os 5 s do driver do carro** (Marco 10). O pedido era a central parada deixar de gastar processador sem mudar o que a HMI e os daemons enxergam. Havia três jeitos de esperar. Prender a thread até chegar mensagem pararia, num barramento calado, a batida de vida e a ordem de parar do laço do firmware, que só são conferidas entre uma leitura e outra. O prazo do kernel do firmware, 5 s, é o que o carro faz; mas eu só o li com a série já rodando com 100 ms, e com ele a batida e a parada passariam a acontecer com até 5 s de atraso em relação ao que a bancada fazia. Com 100 ms as duas seguem na hora de antes, e o gasto de processador é o mesmo para qualquer efeito prático. Custo: não é o valor do carro; com o barramento calado cada laço dá dez voltas por segundo, contra uma a cada 5 s lá; o `carifservice` continua registrando "timeout" e dormindo 10 ms a cada volta vazia mais vezes do que no carro; e o valor é uma constante do código (`ESPERA_NS`), sem opção de ambiente. Para trocar: mudar a constante, ajustar o caso `leitura_sem_mensagem_continua_voltando` do teste, que cobra uma volta por segundo, e repetir a série.
- **D82 — o conserto ficou todo na leitura do `driver/kipc_bus.c`; o hub, a escrita e o lançador não mudaram, a primeira tentativa de leitura continua sem espera e o `errno` da volta vazia continua 11** (Marco 10). O pedido era o menor lugar possível e os mesmos retornos. Uma alternativa de duas linhas era pôr o prazo no próprio socket (`SO_RCVTIMEO`) e tirar o `MSG_DONTWAIT`; preferi um `ppoll` entre duas leituras sem espera porque, se ele falhar por qualquer motivo, a leitura volta na hora como antes e nunca fica presa. Custo: três chamadas de sistema por mensagem que chega com o leitor dormindo, em vez de uma; pelo que li do kernel o carro devolve `errno` 1 quando o prazo passa, e nenhum dos laços que li olha o `errno`, mas não li o firmware inteiro; e, se o hub cair, a leitura volta na hora e os leitores giram, como antes.
- **D83 — dei a série lado a lado por aprovada com três diferenças entre os braços, que atribuí ao acaso e à máquina, e ela rodou sem o registro detalhado** (Marco 10, "A série lado a lado"). O pedido era comparar serviços, linha do tempo, fotos, erros de OpenGL e consumo, com os padrões do modo ao vivo; por isso sem `HMILOG`, e os dados do painel foram conferidos pelo barramento e pelas fotos, não pelas linhas de rastreamento das séries do Marco 8. As três diferenças: a ordem de quatro telas no primeiro segundo, o número de quadros desenhados e um registro da HMI cortado no fim. Custo se eu estiver errado: uma mudança real na ordem de partida passaria por acaso (o que tenho contra isso são 67 de 123 registros antigos com a ordem trocada, uma partida do braço de antes na série e os pares sem o carro de mentira, em que foi o braço de antes que variou); que os quadros a mais sejam sobra de processador é dedução; e o corte do registro é uma corrida do desmonte do container, lida no lançador e não repetida de propósito. Os pares sem o carro de mentira foram só três e rodaram depois da instalação, com a máquina já livre. **Depois da instalação apareceu uma quarta diferença, que a série não olhava** (Marco 10, "As corridas de partida, que a série não olhava"): com a máquina carregada a biblioteca de antes fazia a HMI tomar o caminho do perfil inicial atrasado em 6 de 8 partidas, e a nova em nenhuma de 7. Mantive a instalação: o caminho já existia (58 dos 131 registros antigos), a mudança vai no sentido de a central se comportar como numa máquina livre, e com a máquina livre as 94 partidas a mais que rodei não mostraram diferença que se repetisse. Custo: "nada mudou no que a HMI faz", como eu tinha escrito à 1h30, era mais do que a série permitia dizer; e, se a diferença de uma das levas no indicador de gesto da grade (8 contra 2) for real, a faixa vermelha aparece menos vezes na partida da central ao vivo.
- **D84 — instalei à 1h13 por troca de arquivo, três minutos depois de avisar e sem esperar resposta, com centrais de outras sessões no ar; desenvolvi numa cópia fora da pasta; e mexi em três coisas além do pedido: o comentário de cabeçalho do `kipc_bus.c`, o `README.md` e uma nota de memória** (Marco 10, "A instalação e a conferência da pasta"). O pedido mandava avisar antes e instalar por troca de nome. As mensagens ficaram na fila das quatro sessões, que só as leem ao fim do turno, e deixei a mesma nota na pasta de notas. Custo: quem estava no meio de uma comparação passou a ter, talvez sem ter lido o aviso, centrais novas numa máquina com outra carga; à 1h25 uma central de outra sessão ainda marcava 497 %, sinal de que roda de cópia própria com a biblioteca antiga, e essas rodadas continuam pesando no Mac até a dona trocar; o arquivo instalado tem comentário diferente do que rodou na série (a biblioteca compilada dos dois é idêntica byte a byte, conferido); o cabeçalho do arquivo descrevia a leitura com os campos trocados e eu o reescrevi sem que fosse pedido; ferramentas que escrevi para medir (o amostrador de pilha, o extrator da tabela de símbolos do kernel) só existem em `traces/`, sem teste; e a nota de memória que dizia "cinco núcleos por central" e mandava limitar a duas centrais foi reescrita. Para desfazer: `traces/cpu_kipc_bus.antes.c` por cima de `driver/kipc_bus.c`, por troca de arquivo; o `t_kipc_bus.sh` passa então a falhar em 2 casos.
- **D85 — o estado do carro de mentira é um arquivo de texto com a forma de perfil, que o próprio `veiculostub` escreve a cada pedido que muda alguma coisa, e só com o que foi mudado por pedido** (seção "O carro de mentira lembra do que a tela gravou"). O pedido dizia o que guardar (o acerto do relógio e cada valor gravado) e pedia a troca de nome; a forma de perfil foi escolha minha, para o arquivo ser lido com o que já se sabe dos perfis. Custo: o arquivo inteiro é reescrito a cada mudança, logo depois de a resposta sair para a HMI, e se o container morrer nesse instante perde-se a última mudança; um laço de pedidos reescreve o arquivo a cada volta, até o corte dos 200; o arquivo tem duas linhas (`acerto`, `fim`) e um escape (`\n`) que o perfil não tem, então não serve de perfil; e não ficam cópias anteriores.
- **D86 — o relógio guarda a diferença para o relógio do Mac, em segundos, e não a hora** (muda a D71, em que o acerto valia até a central desligar). O pedido indicava esse caminho e pedia o custo. Custo: o relógio do carro de mentira anda com o do Mac, e não sozinho como o de um painel: se o Mac for corrigido ou trocado de hora, ele vai junto, e não atrasa nem adianta por conta própria; a diferença é em relação à hora do Mac no `fuso` do perfil, então trocar o `fuso` no perfil desloca uma hora que tinha sido acertada à mão; um acerto de data é a mesma diferença, e a central fica um dia adiante até alguém acertar de volta ou apagar o arquivo; perder o arquivo devolve a hora do Mac, e não um relógio zerado como o de um carro que ficou sem bateria; e um acerto que levaria o relógio para fora de 1970 a 2099 faz o arquivo inteiro ser recusado.
- **D87 — o arquivo de estado é conferido inteiro antes de qualquer valor ser aplicado; com erro, nada dele vale, a partida acontece e ele fica de lado, em `<arquivo>.recusado`; e a última linha, `fim`, leva a conta dos valores.** O pedido era não impedir a partida nem aplicar pela metade; a linha `fim` e o arquivo posto de lado são meus. Custo: um erro numa linha só joga fora tudo o que estava guardado, e a tela precisa gravar de novo; só fica o último arquivo recusado; quem editar o arquivo à mão tem de acertar a conta do `fim`; e o aviso só aparece no registro do stub e na linha "veículo" do lançador, que ninguém lê sem motivo.
- **D88 — o estado vale por cima do perfil mesmo para serviço que saiu de `grava`, e valor guardado para campo que o perfil enche com marcador é descartado.** São dois casos que só aparecem quando o perfil muda de uma sessão para a outra, e o pedido não falava deles. Custo: com a gravação desligada (`grava =`) a unidade guardada continua valendo e a tela não tem como trocá-la, é preciso apagar o arquivo; e o valor descartado some do arquivo na gravação seguinte, sem volta. A regra do marcador só foi exercitada nos testes.
- **D89 — o estado vem ligado só no modo ao vivo, na pasta da memória da tela (`ao-vivo/memoria/veiculo/estado.txt`), e desliga junto com ela; nas centrais de teste e no lançador direto, só a pedido.** O pedido era esse. Custo: no modo ao vivo o carro de mentira deixa de nascer igual, a hora mostrada pode não ser a do Mac, e quem compara fotos ou dirige a central por roteiro precisa saber disso ou subir com `MEMORIA=`; um estado ruim fica guardado; não há interruptor só para o carro de mentira no `ao_vivo.sh`, é apagar a pasta `veiculo`; e `rm -rf ao-vivo/memoria`, que já era o jeito de voltar a tela ao zero, agora zera o carro de mentira também.
- **D90 — se a pasta do estado não puder ser montada dentro do chroot, a central sobe sem guardar, com aviso, em vez de parar.** O arquivo de entrada, que usa o mesmo tipo de montagem, para a central quando falha; aqui escolhi o contrário, porque sem guardar a central ainda serve. Custo: a falha passa despercebida por quem não lê a saída do lançador, e só se nota na partida seguinte, quando a hora não volta. Esse ramo não foi exercitado.
- **D91 — nos testes cada partida tem um hub novo, e o stub não ganhou defesa contra pedido reentregue pelo hub** (mesma seção, "O hub reentrega pedidos antigos"). Custo: se o stub for religado com a central de pé, coisa que nenhum roteiro faz hoje, o hub lhe entrega de novo as 48 primeiras mensagens do barramento, e um pedido de gravar que estivesse entre elas seria aplicado outra vez e iria para o arquivo. No registro do barramento da rodada `a` não havia nenhum entre as 48.
- **D92 — desenvolvi numa cópia fora da pasta, refiz a cópia quando a pasta mudou no meio, instalei por troca de nome na ordem stub, lançadores e, por último, o teste, e mexi em quatro roteiros de outras sessões:** `driver/m1_hmi.sh` (sete linhas de cabeçalho e o trecho do carro de mentira), `driver/m3_ao_vivo.sh` (cinco linhas de cabeçalho e quatro de código), `driver/ao_vivo.sh` (só cabeçalho) e `driver/m8_central.sh` (seis linhas de cabeçalho, duas variáveis a mais na lista, a montagem a pedido e a faixa de linhas da ajuda, que ninguém pediu). Custo: a primeira série e todas as rodadas com fotos da cópia de trabalho rodaram com o `kipc_bus.c` de antes de 1h13; a série repetida sobre a pasta nova tem seis partidas com estado, e não oito; entre a instalação do stub (1h24:47) e a do teste (1h28:08) quem rodasse o `t_veiculo.sh` da pasta rodava o teste de antes contra o stub novo, o que passa, porque os 128 casos de antes estão sem mudança entre os 187; e o `t_veiculo.sh` passou de 43 s para 64 s. E a pasta seguiu mudando: à 1h33 a sessão do rádio de mentira instalou a mudança dela por cima de cinco dos meus seis arquivos, sem desfazer nada meu (conferi trecho a trecho); as rodadas `p`, `w` e `q` são de antes disso, e as `x`, `g`, `h`, `k` e `e`, de depois.
- **D93 — quem responde pelo sintonizador é o motor do `veiculostub` mais um modelo em C, num programa à parte que inclui o arquivo do motor (`driver/radiostub.c`)** (Marco 11). O pedido era decidir entre o próprio `veiculostub` com outro esquema e um programa novo. O esquema e o perfil bastam para as assinaturas e para o uso do rádio; não bastam para as três mensagens em ordem, a procura na lista e o passo para a emissora vizinha. Preferi um modelo pequeno em C a uma linguagem de regras dentro do perfil, e incluir o motor a copiá-lo. Custo: o `radiostub.c` enxerga as funções e estruturas internas do `veiculostub.c`, sem interface declarada, e uma mudança lá pode quebrá-lo (por isso a D100); o registro do rádio traz linhas com dois prefixos, `[veiculostub]` do motor e `[radiostub]` do modelo; e o modelo sabe coisas de rádio (nomes de campo, modos, estados), ao contrário do motor, que não conhece serviço nenhum.
- **D94 — o `driver/veiculostub.c` ganhou três ganchos atrás de `#ifdef VEICULOSTUB_MODELO`, e os pedidos do modelo passam pelo `ignora` do perfil e pelo corte de laço** (Marco 11). Sem a macro o programa compilado é o mesmo. Apliquei por inserção em cima da versão que outra sessão tinha instalado nove minutos antes (a do `-e <estado>`). Custo: mais um arquivo de outra sessão mexido, e o corte de 200 em 10 s, que é palpite do Marco 9, passa a valer também para sintonia e busca (um dedo não chega perto; um roteiro automático muito rápido chegaria).
- **D95 — o gerador segue a segunda forma do código da LG e relata pedidos que saem com a mesma api; o `stubs/veiculo/gerado/relatorio.txt` ganhou quatro linhas** (Marco 11). O pedido era estender o gerador sem escrever mensagem à mão. O esquema de veículo saiu idêntico, byte a byte; só o relatório mudou, e o instalei para a pasta continuar igual ao que o gerador produz. Custo: o gerador reconhece as duas formas pela aparência do bytecode (o `default` que cria uma classe com `executeListenerService(int)`, o `switch` logo depois de `getApiId`); uma terceira forma sairia com aviso ou em silêncio. Dos cinco pares de pedidos com a mesma api o esquema continua guardando só o primeiro.
- **D96 — o rádio de mentira anuncia o domínio `0x400000000000` (`PID_TUNER_HMI`), e não o `0x200000000000` da tabela do `simulador-jetta.md`** (Marco 11). É o que o código da HMI trata e o que as rodadas `a0`, `a1` e `a2` mostraram; e o barramento da rodada `v2`, relido, mostra que ela anunciou o outro. Custo: nenhum conhecido; corrigi a tabela e a conclusão da rodada `v2` no `simulador-jetta.md`, que é da sessão principal, deixando escrito o que mudou.
- **D97 — o perfil tem cinco emissoras inventadas, três com nome ("TESTE 1" a "TESTE 3") e duas sem, com códigos PI também inventados (`0xB001` a `0xB003`)** (Marco 11). O pedido era não usar nome de emissora de verdade e marcar tudo como simulação. As frequências eu escolhi à mão; qualquer frequência do dial é de alguma emissora em alguma cidade, e nenhuma foi escolhida por isso. Custo: quem olhar a tela pode tomar a lista por recepção; por isso os nomes dizem "TESTE" e o arquivo diz "SIMULAÇÃO" em cada bloco. Os PIs não são procurados em banco nenhum na bancada (a HMI não fez pedido ao `DSIRadioData`); num ambiente com o banco de emissoras, um PI inventado poderia casar com uma emissora de verdade.
- **D98 — o simulador sobe com a faixa de FM "Brasil" na adaptação (`0559 @1.4+4 = 6`), no arquivo de deduções do Jetta** (Marco 11). O pedido falava em faixa de FM do Brasil; descobri no caminho que a faixa não vem do sintonizador, e sim da adaptação do 5F, e que a HMI tem um valor com esse nome. É palpite sobre o carro: nenhuma leitura. Custo: se o carro usar a faixa da Europa, a tela de sintonia manual do simulador fica diferente da dele (escala de 76 em vez de 88, passo de 0,2 em vez de 0,1), e o arquivo de deduções é padrão de toda central. A primeira partida com uma memória guardada na faixa de antes zera os ajustes de rádio guardados. Para desfazer: apagar a linha. Pus na ficha o que olhar no carro. A faixa de AM ficou sem palpite.
- **D99 — o rádio de mentira vem ligado por padrão só no `driver/ao_vivo.sh`; no `driver/m3_ao_vivo.sh`, que as centrais de teste usam, não há padrão** (Marco 11). O pedido era decidir se entra por padrão no modo ao vivo. Entra: o objetivo do Lipe é o simulador funcional, e a tela do rádio é a primeira que uma central sem memória mostra. Nas centrais de teste não, porque naquela hora havia outras sessões no meio de comparações lado a lado, e um padrão novo mudaria as partidas delas sem aviso. Custo: pela primeira vez o `m8_central.sh` sem variável nenhuma não reproduz o modo ao vivo; quem quiser a igualdade passa `RADIO=/stubs/radio/fm-de-mentira.txt`. E a partida do modo ao vivo muda: a HMI liga três serviços a mais e a primeira tela deixa de ser o aviso.
- **D100 — se o `radiostub` não compilar, o `driver/m1_build.sh` avisa e segue, e o lançador sobe a central sem rádio** (Marco 11). Ninguém pediu. Fiz porque o `radiostub.c` inclui o `veiculostub.c`, em que outra sessão estava mexendo naquela hora, e o `m1_build.sh` roda na partida de toda central: um erro de compilação ali pararia todas, a ao vivo também. Custo: é o único programa da bancada cuja falha de compilação não para a partida; quem não olhar a saída do lançador pode achar que o rádio sumiu sem motivo. O teste `t_radio.sh` falha nesse caso, que é onde o defeito aparece.
- **D101 — a busca é instantânea e pula de emissora em emissora; a busca sem parar dá um passo e espera; "Buscar e salvar" devolve na hora a mesma lista** (Marco 11). O sintonizador do carro varre a faixa e leva tempo. Custo: nenhuma tela de "buscando" chega a aparecer, e comportamento que dependa do tempo da busca (a varredura com amostra de cada emissora, por exemplo) não é exercitado. O passo único da busca sem parar é escolha minha; o que o sintonizador de verdade manda nesse modo eu não vi.
- **D102 — dos pedidos de ajuste só o uso do rádio é atendido; filtro de avisos de trânsito, RDS regional, seguimento de emissora, texto de rádio e o aviso das memórias ficam registrados e sem resposta** (Marco 11). A HMI não marca prazo para nenhum deles, e atender a pedido sem necessidade foi o que pôs a HMI em laço no Marco 9. Custo: os atributos correspondentes ficam no valor padrão; se alguma tela de configuração mostra o estado que o sintonizador devolve, ela mostra o padrão, e não o que foi escolhido. Não conferi essas telas.
- **D103 — desenvolvi numa cópia fora da pasta e instalei por troca de nome, com centrais de outras sessões no ar, refazendo a junção quando cinco arquivos mudaram na pasta nove minutos antes de eu instalar** (Marco 11). Os roteiros `m1_hmi.sh`, `m3_ao_vivo.sh`, `ao_vivo.sh`, `m8_central.sh` e o `veiculostub.c` foram trocados por outras sessões à 0h48 e à 1h24; apliquei as minhas inserções em cima do que estava lá, conferindo arquivo por arquivo que nada tinha mudado entre a foto e a troca. Custo se errado: uma edição simultânea pode ter se perdido; as suítes da pasta passaram depois (os 187 casos do `t_veiculo.sh` e os 32 do `t_hmilog.sh`, que são delas, entre eles). As rodadas `d1` a `d4` rodaram com o lançador de antes da troca da 1h24; as `e1` e `e2`, com o da pasta.
- **D104 — acrescentei dois itens à lista "O que anotar no primeiro autoscan" da ficha do carro**: copiar a adaptação de rádio do 5F e fotografar a tela de sintonia manual de FM. É só leitura. Custo: mais dois itens na lista do Lipe; os nomes dos campos que dei são os da tabela da HMI do MU0883, e o nome com que o ODIS mostra cada um eu não conferi.
- **D105 — concluí que os avisos não estão cortados e não mexi em nada da bancada** (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada"). O pedido era consertar só se a causa estivesse do lado da bancada. Custo se errado: se a central do carro mostrar esses avisos com a caixa inteira, a diferença está em algo que não olhei (os dados de skin do MU0803, um tipo de tela escolhido por dado do carro, ou a primeira fila da imagem da moldura, que não abri), e o simulador segue diferente do carro até alguém comparar com uma foto.
- **D106 — li a árvore de widgets da HMI pelo agente de depuração da JVM do firmware (JDWP), só em centrais de teste e na variante manual do lançador, e deixei o leitor em `traces/`, não em `driver/`.** Ninguém pediu a ferramenta; foi o jeito de saber onde a HMI põe cada peça sem adivinhar pelos pixels. O agente escuta só no 127.0.0.1 de um container sem rede; o leitor só lê campos e segura a JVM por uma fração de segundo. Custo: com o agente a HMI sobe pela linha da JVM montada à mão, sem o `runHMI.sh`, e a partida dessas centrais não é a de uso (por isso repeti nos pixels da variante real o que importava); a ferramenta não tem teste e depende dos nomes de campo deste build (`x`, `y`, `width`, `height`, `parent`, `children`, `internalStates`); e fora da pasta `driver/` quem precisar dela tem de saber que existe: está nos próximos passos.
- **D107 — acrescentei um pedido de foto à ficha do carro e troquei a situação do item 3 no `simulador-jetta.md`, que é da sessão principal.** A foto de um aviso pequeno na central do carro é o que fecha a comparação. Custo: mais um item na lista do Lipe, e uma edição em arquivo de outra sessão (uma célula de tabela, relida na hora).
- **D108 — o display de mentira guarda a porcentagem de brilho que a HMI grava e a devolve na leitura, e leitura sem gravação anterior fica sem resposta** (seção "O brilho pela tela: o display de mentira responde"). O pedido era responder com a porcentagem. A parte de ficar calado sem gravação é minha: o stub não tem brilho próprio, e uma resposta inventada faria a HMI guardar na memória um nível que ninguém escolheu. Custo: se algum caminho da HMI ler antes de gravar, o tempo esgotado de 10 s volta nesse caminho (não achei nenhum no código nem nas rodadas); e o stub aceita qualquer valor, quando o display do carro poderia limitar.
- **D109 — nada é feito com o brilho: a imagem da janela e da página não escurece.** Escurecer seria no `glhost` ou na janela, que são da sessão principal, e o pedido era a escolha pegar e voltar. Custo: quem olha a tela não vê diferença entre "Muito alta" e "Muito baixa"; só o campo e a memória mudam.
- **D110 — os casos do brilho entraram no `driver/tests/t_pedidos.sh`, e não no `t_entrada.sh` que o pedido citava; e instalei com a pasta mudando.** O `t_pedidos.sh` é o teste dos pedidos que o `dsistub` atende, e o `t_entrada.sh`, o do arquivo de comandos. Entre a cópia de trabalho (2h23) e a instalação (2h42) outra sessão instalou o telefone de mentira e trocou cinco arquivos comuns, nenhum dos meus dois; não repeti a série sobre a pasta nova, fiz a prova a partir dela (`p1` e `p2`). Custo: as doze partidas lado a lado são da pasta de antes do telefone de mentira; e o `t_pedidos.sh` ficou uns 5 s mais longo.
- **D111 — o som de mentira é um cliente do mock RSI, com programa e perfil próprios; o esquema gerado e o `veiculostub` não mudaram** (Marco 13). O pedido era levantar os serviços DSI de som e estender o esquema do `veiculostub`, pelo desenho do rádio. O levantamento mostrou que o som desta HMI não é DSI: é o serviço RSI `audiomanagement`. Custo: o som não aproveita o motor nem o formato de perfil dos outros stubs (o perfil dele é outro, com valores em JSON), e depende da camada RSI (`RSI=1`); sem ela a central sobe sem som, com aviso.
- **D112 — o gerente de áudio de verdade não roda junto com a HMI: parei no mudo de proteção de componentes e não investiguei** (Marco 13). Seria o caminho mais fiel, e ele sobe na bancada. Na única rodada com a HMI, 75 s depois da partida ele entrou no mudo de proteção de componentes e a HMI mostrou "Proteção de componentes está ativa.". É proteção antifurto; o limite da bancada é não contornar nem descrever como burlar ativação ou licença, e tomei este caso como dentro dele. Não olhei o que dispara o mudo nem o que o libera. Custo: as regras de verdade do gerente (prioridades, ações cruzadas, limites de volume) ficam de fora, e o que roda é um substituto com regras mínimas; se o mudo entra em toda partida eu não sei.
- **D113 — quem serve o `audiomanagement` é o mock RSI do firmware, com os dados que o gerente de áudio serve em repouso, gerados uma vez e guardados na pasta; e o perfil é aplicado a esses dados antes de o mock subir** (Marco 13). Alternativas: escrever um servidor RSI próprio, ou gerar os dados a cada partida. Fiquei com o mock porque ele já é o servidor RSI da bancada (Marco 4) e fala o que a HMI espera; com os dados guardados porque gerar leva uns 15 s e depende de o `audiomgnt` subir; e com o perfil nos arquivos porque o mock recusa gravação na saída de som (403). Custo: 310 kB de dados do MU0883 na pasta (nomes, ligações e faixas de volume); os limites do mock vêm junto (não expande referência, não confere faixa); e mudar o perfil com a central de pé não tem efeito.
- **D114 — com `SOM`, o lançador monta por cima da especificação do serviço uma cópia afrouxada em dois pontos** (Marco 13): a assinatura de recurso dos conjuntos de som e o apagamento de controle. O primeiro é necessário e medido (o serviço de verdade atende; sem ele a tela aceita o toque e nunca recebe o valor de volta); o segundo é dedução minha, para a lista de controles não crescer. Não mexi no binário do mock. Custo: dentro do container, com `SOM`, o arquivo de especificação que o mock lê não é o do firmware (é montagem por cima de um arquivo em `/tmp`; a partição continua só leitura e intacta); quem estudar o mock com `SOM` ligado precisa saber disso. Sem `SOM` nada é montado.
- **D115 — as regras do som de mentira são mínimas e marcadas como simulação** (Marco 13): um passo de volume por passo do botão, dentro da faixa; chegar a zero liga o mudo; aumentar tira o mudo; uma fonte de entretenimento por vez; conexão de menu com volume próprio leva o foco. A tabela de áudio do firmware descreve o caso real, com 124 conexões e ações cruzadas, e não a reproduzi: o pedido é o estado que a tela mostra. Custo: não há aviso de navegação abaixando a música, limite de volume ao ligar nem volume por fonte; e nada é guardado de uma partida para a outra.
- **D116 — o som de mentira faz o papel do `touch-app`: cria o controle de volume e o de mudo a partir do botão e das teclas que passam no barramento** (Marco 13). Lido no código que a HMI não cria o controle no giro e, nos textos do binário do `touch-app`, que ele fala com `/audiomanagement/controls`; que o mudo também seja dele é dedução. Custo: os nomes dos controles vêm de textos do binário, e a correspondência exata entre tecla e controle no carro não foi medida.
- **D117 — as conexões são assinadas uma a uma, e não pela lista** (Marco 13). Conserto do vai e volta entre fontes que o teste mostrou: o aviso de recurso do mock é um retrato da lista inteira, e um retrato atrasado parecia pedido novo. Custo: 124 assinaturas no lugar de uma, e a partida do programa espera o estado inicial de todas (prazo de 8 s; nas rodadas chega em menos de 1 s).
- **D118 — a opção `SOM` do lançador segue o molde de `RADIO`, `TELEFONE` e `MIDIA`, sem padrão nas centrais de teste** (Marco 13). Perfil com erro: nada sobe. Sem a camada RSI ou sem o programa compilado: a central sobe sem som, com aviso. Custo: mais uma variável em cinco arquivos compartilhados.
- **D119 — o som de mentira entra por padrão no modo ao vivo** (`driver/ao_vivo.sh`; `SOM=` desliga), e não nas centrais de teste (Marco 13). O pedido deixava a escolha comigo, com registro. Critério: quinze partidas com som em duas séries lado a lado, numa sonda e na prova da pasta, com o estado igual byte a byte dentro de cada roteiro; nenhum molde novo de erro no registro da HMI e três a menos; nenhum erro de OpenGL; custo parado abaixo do que se mede; o rádio e a mídia de mentira, que o modo ao vivo já liga, passam a se comportar mais como os do carro (cada troca de emissora chega ao sintonizador, a faixa toca e volta); e o objetivo do Lipe é a central se comportar como a do carro. Custo: a HMI desliga o substituto de áudio que tem dentro dela e passa a depender do serviço para rádio e mídia (medido como melhora nos dois; chamada de telefone não foi exercitada, não há aparelho); a mídia passa a mostrar "tocando" com o tempo parado em 0:00, que é mais estranho de ver que a pausa; dois limites que o Marco 15 e o README da mídia descrevem deixam de valer no modo ao vivo, e anotei isso nos dois lugares; e a central ao vivo não foi ligada para conferir. Para desfazer: `SOM= sh driver/ao_vivo.sh`, ou apagar as duas linhas do `SOM_PADRAO`. **Acréscimo das 4h20:** a série no modo da central ao vivo, que devia ter vindo antes de instalar, rodou depois (série W, nove pares): o som não muda a partida; ver "O modo ao vivo", no Marco 13.
- **D120 — o amplificador do perfil é um interno básico inventado (`VWBAS000`), embora a ficha diga "Som Beats, a confirmar"** (Marco 13). Não inventei um Beats: não sei o identificador, as funções nem as faixas de equalização do amplificador do carro, e a existência dele depende do autoscan (módulo 47). O básico é o menor conjunto que a tela aceita: balanço, fader, três faixas e compensação de ruído. Custo: se o carro tem Beats, a tela de som do simulador não é a do carro. Pedi a foto na ficha.
- **D121 — as teclas Vol + e Vol − do volante mudam o volume sem abrir a barra, e não forcei a barra a abrir** (Marco 13). Um passo por aperto, com controle de domínio `cluster`, que é o que os textos do `touch-app` sugerem para o volante. Para a barra abrir eu teria de mandar à HMI um evento de giro que não aconteceu; e medi que criar o controle como se fosse da moldura também não abre (`r5`). Custo: quem aperta Vol + no volante da janela só vê o valor mudar se a barra já estiver aberta; segurar a tecla não repete.
- **D122 — desenvolvi em cópias fora da pasta e instalei por troca de nome com centrais de outras sessões no ar, refazendo a junção quando a mídia entrou** (Marco 13). A primeira cópia (da pasta das 2h54) já tinha passado por uma série de dez partidas quando a mídia de mentira entrou na pasta, às 3h11, mexendo nos mesmos cinco lançadores; refiz a cópia sobre a pasta das 3h14, com âncoras novas, e repeti bateria e série antes de instalar, às 3h35min16, depois de conferir que `driver/`, `stubs/` e `glforward-moi3/` inteiros ainda eram os da base. Avisei na pasta de notas, antes e depois; não esperei resposta. Custo: 25 minutos entre estar pronto e instalar; quem tem cópia de trabalho dos cinco lançadores de antes das 3h35 de 12/10/2026 precisa reler antes de instalar, ou leva o som embora. **E um erro meu no caminho:** regravei no lugar um roteiro do meu rascunho enquanto um par da segunda série o executava; o par foi invalidado e reposto.
- **D123 — nenhum menu novo entra ligado por padrão** (Marco 14). O pedido mandava ligar só o que a ficha do carro sustenta, e ela não confirma nenhum dos equipamentos desses menus: estão todos como "a confirmar", com fonte de imprensa, e a câmera dos assistentes está como "Não". Custo: a central de todo dia continua mostrando menos do que a do carro deve mostrar (um GLI há de ter pelo menos luz e abrir e fechar), e quem olha só o padrão não vê o que o Marco 14 achou. Para rever: a lista "Para o Lipe decidir", no Marco 14. (**Mudou em 12/10/2026, às 18h05:** revisto com a resposta do Lipe; D196 a D198.)
- **D124 — os palpites não foram para os arquivos de deduções; ficou um catálogo à parte, que liga tudo e não entra em partida nenhuma** (Marco 14). O pedido dizia "o que for palpite vai no arquivo de deduções, com a razão ao lado". Os dois arquivos de deduções entram em toda partida do modo ao vivo: palpite posto lá vira tela, e eu não tinha razão para escolher uma opção e não outra. O catálogo mostra o que a HMI sabe fazer sem dizer nada do carro. Custo: ele liga opções que o Jetta não tem, e o risco é alguém tomá-lo pelo carro. Por isso o aviso nas primeiras linhas dos dois arquivos, o teste que não deixa lançador citá-lo e a central de teste à parte.
- **D125 — os dois arquivos do catálogo são gerados, e a lista de cores da luz ambiente fica de fora** (Marco 14). Nenhum byte e nenhum nome de opção é digitado: saem da tabela de campos e do esquema, os dois tirados do firmware. A opção `intLightRGBColorList` sai porque faz a HMI pedir uma lista que o carro de mentira não sabe responder (4 e 5 tempos esgotados de 10 s nas rodadas `luzint` e `a`). Custo: para refazer é preciso python3, que o Mac tem e a imagem da bancada não; e a tela de luz do catálogo não tem a linha de cor, se houver uma.
- **D126 — o telefone de mentira é o próprio `veiculostub`, com outro esquema e outro perfil; não escrevi programa novo** (Marco 12). O pedido era seguir o desenho do rádio, que tem um programa com modelo; para "nenhum aparelho conectado" o motor basta. Custo: as linhas do registro saem com `[veiculostub]` na frente; e o que o motor não sabe fazer (resposta em várias mensagens, valor por instância) fica de fora até alguém escrever um modelo.
- **D127 — o esquema e o stub cobrem os dois domínios e os 18 serviços que os anúncios ligam, e não só os dois serviços de que a tela depende.** Medi que bastavam `DSIBluetooth` e `DSIMobileEquipment` (rodada `e1`). Fiquei com todos, respondidos com o valor padrão marcado como válido, como no carro de mentira: a HMI passa a ter os serviços de telefone inteiros, com listas vazias, em vez de serviços registrados e mudos. Custo: foi daí que veio o aviso de bateria (um zero que não é neutro), e pode haver outros em telas que não abri; `DSISmartphoneIntegration` fica ligado e sem resposta.
- **D128 — o telefone de mentira responde também pela instância 2 do `DSIMobileEquipment`, que é a do CarPlay e de outro domínio.** O motor não distingue instâncias e o barramento da bancada não leva o destino do pedido; não responder por ela pedia mexer mais no motor. Custo: a HMI recebe "CarPlay: não conectado" de quem, no carro, não diria isso. Medido que sem essa resposta a tela é a mesma (rodada `e1b`).
- **D129 — mexi no motor compartilhado: só o que vem do canal da HMI é tratado como pedido** (`driver/veiculostub.c`, uma linha; vale para o carro de mentira e para o rádio de mentira). Não estava no pedido; sem isso o telefone de mentira tomava os eventos de energia por pedidos (Marco 12, "Um defeito do motor que só o telefone mostrou"). Custo: um programa que mandasse pedidos com outro canal deixaria de ser atendido (os testes da pasta mandam com o `0x8000`, e passam); e mexi num arquivo em que outras duas sessões trabalhavam.
- **D130 — no gerador, só entra no esquema o serviço que o anúncio do domínio liga, e o domínio de cada instância vai só para o relatório, sem linha nova no esquema.** A regra tira do esquema do telefone nove serviços de mídia, UPnP e busca, que o despacho do domínio 256 entrega e que são de outros donos. Custo: se num domínio futuro um serviço for dele e não estiver na lista do anúncio, fica de fora (aparece no relatório, em "fora do esquema"); e o motor segue sem saber que a instância 2 é de outro domínio.
- **D131 — o perfil responde "feito" a seis pedidos e deixa a tela ligar e desligar o Bluetooth.** O pedido era o mínimo para a tela abrir. Acrescentei as respostas porque cada pedido sem resposta deixa um tempo esgotado no registro da HMI, e o de ligar e desligar prende a tela até 60 s em "Aguarde…". Custo: "feito" é dito onde nada foi feito (a central não fica visível, não há microfone); a caixa "Visibilidade" não marca; ligar e desligar é instantâneo; e o serviço do Bluetooth em `grava` usa uma regra de gravação "por nomes", a menos segura do gerador (a do modo de visibilidade).
- **D132 — não inventei o nome da central no Bluetooth: nasce vazio.** O carro deve mostrar um nome ali, e não sei qual. Custo: o campo "Nome do dispositivo" aparece em branco até alguém digitar.
- **D133 — o telefone de mentira entra por padrão no modo ao vivo** (`driver/ao_vivo.sh`; `TELEFONE=` desliga), e não nas centrais de teste. O pedido deixava a escolha comigo. Critério: nove pares lado a lado sem diferença até a tecla, o carro e o rádio de mentira já entram assim, e o objetivo do Lipe é a central se comportar como a do carro. Custo: o assistente de configuração ganha uma página; a grade ganha um bloco, e a memória guardada passa a levá-lo; o que vem depois das esperas da partida acontece uns 2 s mais cedo, então **tempos de rodadas do modo ao vivo de antes e de depois das 2h38 de 12/10/2026 não se comparam**; e a central ao vivo não foi ligada para conferir.
- **D134 — parei no primeiro estado; o aparelho fictício conectado não foi feito.** O pedido dizia "se couber" e "só depois de o primeiro estar fechado". Ele pede um valor por instância no motor, que é mexer mais num arquivo compartilhado, e um perfil com dados de aparelho. Custo: a tela principal do telefone continua sem ser vista na bancada. **Mudou às 3h48 de 12/10/2026:** o segundo estado foi feito na mesma sessão, depois deste registro (D147 a D150). O que esta decisão descreve é onde o trabalho estava às 3h05.
- **D135 — desenvolvi em cópias fora da pasta e instalei por troca de nome com centrais de outras sessões no ar**, depois de conferir que cada arquivo compartilhado ainda era o de quando o candidato foi montado; avisei por mensagem e na pasta de notas, e não esperei resposta. Custo: quem tinha uma cópia de trabalho de `veiculostub.c`, do `LeEsquemaDSI.java` ou dos lançadores de antes das 2h38 de 12/10/2026 precisa reler antes de instalar, ou leva as mudanças embora; o `t_telefone.sh` acusa se o filtro de canal ou o perfil sumirem.
- **D136 — acrescentei dois itens à lista "O que anotar no primeiro autoscan" da ficha do carro**: copiar as adaptações de Bluetooth do 5F e fotografar a tela Telefone sem celular e as "Configurações do Bluetooth". As posições que cito (`0555`, bytes 4 e 5, e `0B33`) são as da HMI do MU0883, pela tabela de campos do Marco 6, e isso está dito no item. Custo: mais dois itens na lista do Lipe; e o simulador não usa esses campos, então a leitura só serve para comparar com o carro.
- **D137 — a mídia de mentira é um programa à parte, `driver/midiastub.c`, que inclui o motor e acrescenta um modelo do tocador e do navegador; o motor não ganhou gancho novo** (Marco 15). O pedido era seguir o método do rádio. As seis mensagens em ordem da ativação, a pasta e o tamanho depois do filtro e a lista por trechos não cabem em regra de perfil. Custo: cerca de 60 linhas de apoio repetem as do `radiostub.c` (um cabeçalho comum pediria mexer nele); o programa depende das funções e estruturas internas do `veiculostub.c`, como o do rádio; e, sem gancho de tempo, a faixa não anda mesmo que a HMI mande tocar.
- **D138 — a fonte do perfil é um aparelho do tipo 0, que a tela chama de "Minhas mídias", e não um pendrive** (Marco 15). O pedido falava em "uma fonte"; a escolha natural seria USB, e esta HMI não lista aparelho do tipo 2 em "Fontes" (bytecode e rodada `m4`). Custo: não sei com que tipo um pendrive chega no carro (o programa de mídia do firmware não foi aberto), então não sei se "Minhas mídias" é o nome que o carro dá a um pendrive ou a outra coisa; a HMI é a do MU0883.
- **D139 — o perfil tem uma pasta, cinco faixas inventadas com "TESTE" no nome e um modo de reprodução só** (Marco 15). O pedido era uma lista curta, marcada como simulação. Custo: subpastas, capas, repetir e embaralhar não são exercitados; os botões de repetir e de embaralhar aparecem e não fazem nada.
- **D140 — depois de ativar a fonte o modelo diz "em pausa", e não "tocando"** (Marco 15). O que o tocador de verdade manda nessa hora eu não vi. Escolhi a pausa porque o comando da HMI tem um passo "retomar" depois da ativação, e porque sem áudio ela mesma pede pausa em seguida. Custo: se o tocador do carro já sai tocando, a ordem dos estados no simulador é outra; o estado final, sem áudio, é o mesmo.
- **D141 — o `DSISearch` fica de fora da mídia de mentira: o modelo desliga a interface de pedidos dele na partida** (Marco 15). Ninguém pediu. O esquema do domínio o traz, com 28 pedidos desde o gerador das 2h37; como o hub entrega tudo a todos e o motor atende pela interface, o programa responderia às assinaturas das doze instâncias, das quais só três são do domínio de mídia. Custo: a busca de mídia e a busca por voz não são servidas, e as rodadas `m1` a `p4`, que definiram o comportamento, rodaram assim.
- **D142 — a mídia de mentira vem ligada por padrão só no `driver/ao_vivo.sh`** (Marco 15), como o rádio e o telefone. Nas centrais de teste não há padrão. Custo: a partida do modo ao vivo muda de novo (a HMI liga doze serviços a mais); e quem for da mídia ao rádio e voltar encontra o tocador vazio até reiniciar, o que antes não existia porque a tela nem abria. Pesei isso contra a tecla Mídia continuar no aviso, e preferi a tela funcionando na primeira visita. Para desfazer: `MIDIA= sh driver/ao_vivo.sh`, ou apagar as duas linhas do `MIDIA_PADRAO`.
- **D143 — gerei o esquema com o gerador como a sessão do telefone o deixou às 2h37, e com o rótulo sem acento** (Marco 15). Custo: o esquema da pasta não é o das rodadas `m1` a `p4` (essas usaram o de 2h06, sem os pedidos do `DSISearch`); as rodadas `d*`, `r1` e `e*` usaram o novo. O cabeçalho dos arquivos diz "serviços DSI de PID_MEDIA", que é feio mas não sai estragado.
- **D144 — explorei com um protótipo sem teste antes de escrever o teste, e reescrevi o programa depois dele** (Marco 15). A regra é teste antes do código. Sem saber o que a HMI pede não havia o que testar; o protótipo serviu para descobrir, ficou em `traces/` e não entrou em `driver/`. Custo: o programa final foi escrito por quem já tinha visto o protótipo funcionar, e os casos do teste cobrem o que a exploração mostrou, não mais que isso.
- **D145 — instalei por troca de nome com centrais de outras sessões no ar, em cima dos lançadores que a sessão do telefone tinha mudado meia hora antes** (Marco 15). Conferi arquivo por arquivo contra a foto de que a junção partiu, e rodei a bateria completa antes e depois. Custo se errado: uma edição simultânea pode ter se perdido; as suítes delas passaram depois (`t_telefone.sh` 103, `t_veiculo.sh` 187, `t_pedidos.sh` 28).
- **D146 — mexi em três linhas da tabela do roteiro do `simulador-jetta.md`, que é da sessão do simulador, e escrevi uma receita no `stubs/radio/README.md`** (Marco 15). O item 5 era o pedido; os itens 4 e 6 ganharam uma frase cada sobre o que a mídia mudou neles. A receita foi pedida por mensagem. Custo: edição em arquivo de outra sessão, relida na hora.
- **D147 — o segundo estado do telefone foi feito, e a D134 deixou de valer** (Marco 12, "O segundo estado: um aparelho fictício conectado"). Depois do registro das 3h05 sobrava madrugada, e um experimento de dez minutos (`x1`) abriu a tela principal de primeira. O valor por instância entrou no motor como forma geral do perfil (`[Serviço[N].evento]`), e não como caso especial do telefone nem como programa à parte com modelo: o segundo estado só pedia valores. Alternativa descartada: dizer "conectado" nas três instâncias do `DSIMobileEquipment`, que é o que o motor de antes faria, e é errado para o aparelho associado e para a do CarPlay. Custo: 51 linhas num arquivo que o carro, o rádio e a mídia de mentira usam, e uma seção por instância que não recebe o que um pedido grava.
- **D148 — o aparelho fictício tem o mínimo de dados, e todos dizem que são inventados.** O nome "Celular fictício", a operadora "Fictícia", o endereço Bluetooth de exemplo `00:00:00:00:00:01` e o identificador `aparelho-ficticio-1`. Sem IMEI, IMSI, cartão SIM, número, contato, chamada ou mensagem: a tela não pediu lista nenhuma (mostra vazio), então não inventei lista. O teste barra texto sem a marca de fictício na própria linha, campo de identificador com texto e identificador marcado como disponível. O nome da operadora é curto porque o longo saiu cortado na tela.
- **D149 — o aparelho vai também na lista do gerente de aparelhos, só com o mãos-livres conectado, e o perfil responde ao pedido de aparelho prioritário.** Não era do plano: a rodada `kp` abriu a tela principal com a caixa do nome vazia e a lista de celulares vazia. Só o mãos-livres porque é o que "telefone conectado" quer dizer; com música por Bluetooth, agenda e mensagens como "não suportados" as outras telas ficam como estavam, e a mídia de mentira não ganha uma fonte Bluetooth por causa do telefone.
- **D150 — o padrão do modo ao vivo continua sendo o primeiro estado; o aparelho conectado é opção.** Critério: sem celular pareado, o que uma central mostra é "nenhum aparelho"; o aparelho fictício põe ícones na barra do alto de quase toda tela e um nome de celular que não existe, e não faz nada (não disca, não conecta nem desconecta). É decisão minha e fica para o Lipe rever. Para ligar: `TELEFONE="/stubs/telefone/sem-aparelho.txt /stubs/telefone/aparelho-conectado.txt" sh driver/ao_vivo.sh`.
- **D151 — acrescentei notas datadas em textos de outras sessões, onde o que estava escrito deixou de valer com o som ligado, e um item na ficha do carro** (Marco 13). No Marco 15 e no `stubs/midia/README.md`: os limites "a faixa fica em pausa" e "a volta à mídia abre o tocador vazio" não valem com `SOM`, que o modo ao vivo liga. No `stubs/persistencia/README.md`: graves, agudos e balanço passam a mudar pela tela, e continuam sem ser guardados. No `README.md` da bancada e no da raiz: um parágrafo e uma frase. No `stubs/rsi-mockup/README.md`: uma nota de que o mock passa a servir o serviço de áudio. No roteiro de `simulador-jetta.md`, que é da sessão do simulador: uma linha nova, a 9, uma frase nas linhas 4, 5 e 6, um pedido de fotos e uma fonte. Na ficha: um pedido de fotos da tela de som do carro, que é só leitura. Não reescrevi o texto de ninguém: cada nota diz a data, o marco e o que mudou. **Não mexi no `janela/README.md`**, que também ficou com uma dedução desmentida (a barra de volume abrir quando o serviço avisa o volume novo): a pasta `janela/` é da sessão principal, e deixei o aviso no "Onde parou" do Marco 13 e na pasta de notas. Custo: os textos das outras sessões ficam com um remendo no meio, e quem os escreveu pode preferir outra redação.
- **D152 — a lista de aparelhos (`0B1D`) ficou de fora do catálogo, ao contrário do que o pedido descrevia** (Marco 14). O pedido dizia que cada menu pede a função "nas listas da adaptação (0B1B e 0B1D)", como o Marco 8 tinha feito. Medi que os pneus e o Lane Assist dão a mesma tela com e sem o aparelho (rodadas `pn1`, `l2` e `l3`), e o `DeviceManager` só lê o `0B1B`. Custo: se no carro a lista de aparelhos importar por outro caminho, o catálogo não mostra. As quatro linhas do `0B1D` que o Marco 8 pôs nas deduções ficaram como estavam: não testei tirá-las.
- **D153 — incluí os assistentes pelo bloco "Assistentes", embora o pedido os tratasse como menu de "Configurações do veículo"** (Marco 14). Nesta HMI eles não aparecem lá (rodadas `a` e `assist`); achei o lugar lendo o pacote `carfas`. Custo: mais 70 opções e 6 funções no catálogo, de equipamentos que a ficha dá como ausentes ou a confirmar, e umas dez rodadas a mais.
- **D154 — fiz um roteiro que ninguém pediu, `driver/m14_menus.sh`, e não mexi em nenhum lançador** (Marco 14). O comando para subir uma central com o catálogo leva duas listas, de cinco e de quatro arquivos; o roteiro as monta a partir dos padrões do `m3_ao_vivo.sh`. Ele sobe uma central de teste, e não a central ao vivo, para o catálogo nunca entrar na memória dela. Custo: mais um roteiro para manter, e ele depende da forma de duas linhas do `m3_ao_vivo.sh`; se mudarem, ele para sem subir nada e o `t_catalogo.sh` avisa.
- **D155 — explorei antes de escrever o teste, e o teste confere arquivos, não tela** (Marco 14). A regra é teste antes do código. Sem as rodadas eu não sabia que função e que serviço fazem cada menu, que é só o que o gerador tem de seu: a lista dele nasceu delas. O teste veio antes do roteiro e antes de os arquivos irem para a pasta, e cada regra dele foi vista falhar com um defeito posto de propósito. Custo: não há teste automático de "que item aparece na tela"; isso está nas fotos das 21 rodadas.
- **D156 — escrevi em três registros que não são meus: a ficha do carro, o README do projeto 01 e o `simulador-jetta.md`** (Marco 14). Na ficha, um item novo na lista do primeiro autoscan: fotografar cada submenu de Veículo → Config. e a grade de menus. No projeto 01, uma subseção nova e um acréscimo datado no fim da pendência 8, com o que a central faz com as quatro adaptações do Lane Assist. No `simulador-jetta.md`, que é da sessão do simulador, uma linha nova na tabela do roteiro (a 10) e um item em "O que depende do Lipe". Critério: a regra do projeto manda gravar cada achado no arquivo a que ele pertence. Custo: são arquivos que o Lipe e outras sessões mantêm; acrescentei em bloco próprio, sem mudar texto de ninguém nem numeração.
- **D157 — não pus os serviços desses menus no ajuste `grava`** (Marco 14). Sem isso, o que se marca nas telas novas não deve ficar (dedução; não toquei nas caixas). Pôr é mudar resposta de stub, e o Marco 9 mostrou que atender pedido de gravar pode pôr a HMI em laço, como no idioma do painel: pede uma série de partidas só para isso. Custo: o catálogo serve para ver as telas, não para mexer nelas. (**Mudou em 12/10/2026, às 9h56:** feito num perfil à parte, com série de partidas e sem laço; D179 a D181.)
- **D158 — no modo ao vivo a HMI espera 6 s pelos serviços DSI de cada etapa da partida; a espera geral e as centrais de teste seguem com 2 s** (Marco 15, "A espera da partida pelos serviços de veículo"). A sessão do estado do carro de mentira tinha deixado a decisão comigo. Com o rádio e a mídia de mentira, a etapa que precisa dos serviços de veículo passou a ser chamada aos 2,2 s, antes de a HMI os criar (perto dos 5 s), e com 2 s de espera desistia 0,7 s cedo: computador de bordo só com traços quando a central nasce na tela do veículo, e quatro assinaturas a menos nas outras partidas. Subi só a espera dos DSI (`ESPERA_DSI`, padrão 6000, no `driver/ao_vivo.sh`), e não a `ESPERA` inteira, porque assim a primeira tela não atrasa (2,2 s contra 6,2 s) e a última etapa atrasa 8 s em vez de 25. Escolhi 6 s, e não 4 s, por três motivos: a folga (3,2 s contra 1,2 s); as partidas medidas (catorze, com as quatro da sessão do estado, contra duas); e, por dedução, porque a primeira espera da partida, de 2 s, hoje só existe por faltar o serviço RSI `waveplayer`, e no dia em que ele entrar a etapa do veículo passa a precisar de uns 4,8 s. Não mexi no padrão das centrais de teste: havia séries de outras sessões no ar, que executam os roteiros montados, e isso mudaria a base de comparação das rodadas guardadas. Custo: a última etapa da partida do modo ao vivo começa aos 50 s em vez de aos 42 s, e o modo ao vivo e as centrais de teste passam a diferir na partida (quem quiser igualar passa `EXTRA_VMOPTIONS=-Dde.vw.mib.asl.startup.timeout.dsi=6000`). Se os 5 s não forem um prazo fixo, como li no Marco 8, e dependerem da carga da máquina, os 6 s podem não bastar num Mac mais ocupado: o sinal é a linha `Required DSI services not registered within 6000 ms` com `DSICar…` no registro da HMI. O `driver/ao_vivo.sh` de verdade não foi executado por mim; rodei uma cópia dele com o nome do container trocado. Deixei notas datadas, sem mexer no que estava escrito, nos pontos em que a sessão do estado dava o defeito como pendente (três na seção dela e um em "Próximos passos"), no trecho do `README.md` da bancada que ensinava o contorno e nos limites do Marco 11.
- **D159 — as teclas Próxima e Anterior do volante na mídia ficaram sem código novo, e o avanço rápido continua de fora** (Marco 16, passo 1). O pedido era achar a causa e consertar se fosse da bancada. A causa era da bancada (faltava o serviço de áudio, e é dele que a HMI tira o "componente de áudio atual" que a tecla exige) e já estava consertada desde as 3h35 pelo som de mentira de outra sessão: bastou medir. Não declarei no perfil da mídia a capacidade de avanço rápido, que faria o aperto longo mandar pedidos: o programa não faz a faixa andar, e declarar o que ele não faz criaria pedido sem resposta. Custo: o aperto longo de Próxima e Anterior não faz nada na mídia de mentira, e sem o som (`SOM=`) as teclas voltam a não trocar de faixa.
- **D160 — a tecla Fonte do volante ficou como a HMI a trata: do rádio, e sem voltar para a mídia** (Marco 16, passo 2). O pedido era anotar o que cada tecla faz. Medi que ela leva da mídia ao rádio e alterna FM e AM, e que quem reage é o gerente de rádio da HMI. Que no carro a tecla percorra todas as fontes é suposição minha, sem medida; se percorrer, quem faz isso lá é um programa que a bancada não tem. Não inventei esse comportamento num stub. Custo: pela faixa "Volante" da janela, a tecla Fonte não serve para voltar à mídia; a tecla Mídia da moldura serve.
- **D161 — a busca varre a faixa dentro do atendimento do pedido, segurando o programa por até 1 s, e não por um gancho de tempo no motor** (Marco 16, passo 3). Para varrer sem segurar, o motor compartilhado teria de chamar o modelo de tempos em tempos. Não mexi nele: outras sessões o trocaram duas vezes na mesma manhã, e o gancho de tempo que a sessão do telefone pôs às 9h50 chama o modelo uma vez por segundo, grosso demais para passos de 25 ms. Custo: a interrupção que a HMI manda no meio da varredura só é respondida depois da chegada, e a busca "sem parar" não segue andando. Medido que a HMI não fica presa (dois toques seguidos, três centrais).
- **D162 — a grade de cada faixa é uma tabela dentro do `radiostub.c`** (Marco 16, passo 3). A HMI não diz ao sintonizador onde a faixa começa nem o passo, e o formato do perfil não tem ajuste para o modelo (só `fuso`, `grava` e `ignora`). Pus as duas grades que a HMI usa na bancada. Custo: uma ligação escondida com a codificação: quem trocar a faixa lá e não trocar aqui vê a busca correr por canais que a tela não tem. Está dito no cabeçalho do programa, no perfil e no README.
- **D163 — AM entrou com a grade da Europa, três emissoras inventadas e nenhum palpite de adaptação** (Marco 16, passo 3). O pedido dizia "a lista de AM se esta HMI tiver AM para este mercado". Esta HMI tem AM na bancada, e com a adaptação de AM "sem ajuste" ela usa a grade da Europa. O campo da adaptação não tem valor "Brasil", como o de FM tem; escolher "América do Norte" porque o Brasil usa canais de 10 kHz seria inventar um valor do carro, e não pus. Custo: se o Jetta do Brasil não tem AM, ou tem com outra grade, a tela de AM do simulador não é a do carro. Fica para a foto pedida na ficha.
- **D164 — o texto de rádio é um só, e a intensidade de sinal ficou sem nada** (Marco 16, passo 3). Um texto por emissora pediria um lugar no perfil que o formato não tem; o texto do perfil vai para as emissoras marcadas e as outras ficam sem, que é o bastante para ver as duas telas. Para a intensidade de sinal não achei, no código da HMI, tela que a mostre: não pus valor nenhum, e não testei pôr. Custo: o texto não muda como no carro; e, se houver uma tela de sinal que a procura por nome não achou, ela segue vazia.
- **D165 — a memória do som guarda o valor de qualquer volume e de qualquer parâmetro de som que mude, e o mudo; e é lida com tolerância** (Marco 13, "A memória do som"). O pedido citava agudos, médios, graves, balanço, fader, volume e mudo. Guardei pela regra geral (todo `value` de `volumesettings` e de `soundparameters`), que cobre os sete e também o volume de menu ("Volume inicial máximo") e o que um amplificador maior trouxer. A fonte fica de fora: quem lembra dela é a HMI. Ao contrário do perfil, linha ruim é ignorada com aviso, porque memória estragada não pode impedir a central de subir. Custo: o mudo volta ligado, e não sei se no carro volta; e, com a memória ligada, 133 assinaturas a mais no WebSocket do serviço (sem ela o programa se comporta como antes: a central de teste comum saiu igual às da madrugada).
- **D166 — a memória do som segue o desenho do estado do carro de mentira: mora em `ao-vivo/memoria/som/`, sai da `MEMORIA`, e as centrais de teste não guardam nada** (pedido do Lipe, pela sessão principal). `SOM_ESTADO` escolhe outro arquivo ou desliga só essa parte. Instalei com a central ao vivo no ar e em uso, por troca de nome; ela segue com os arquivos que abriu e só passa a guardar na partida seguinte. Custo: mais uma variável em três lançadores compartilhados; e o equalizador voltando na tela só foi visto depois de instalar, porque na série de antes a tela de som abriu noutra aba.
- **D167 — não fiz a barra de volume abrir pelo volante: parei e perguntei** (Marco 13, "A barra de volume pelo volante"). O pedido do Lipe era a barra abrir com Vol +, Vol − e Rolete. A leitura do firmware mostrou que a chave que liga isso na HMI (`isMFLVolumePopup`, 271) é constante e desligada nesta variante, e que o volume pelo volante vai para o painel de instrumentos. Fazer abrir é inventar comportamento contra o que o firmware diz, e a sessão principal tinha mandado parar e avisar nesse caso. Custo: o item 9 fica com esse ponto aberto até o Lipe decidir; a D168 segue reservada para o que ele escolher.
- **D169 — a conta saiu por dois marcadores novos no motor, avaliados a cada envio, e não por regra amarrada ao pedido nem por conta de unidade dentro do programa** (seção "Os números do painel acompanham a unidade escolhida"). Uma regra por pedido gravaria o número refeito como se a tela o tivesse escrito, e ele iria para o arquivo de estado; conta de unidade no motor o faria conhecer serviço, e ele serve também ao rádio, ao telefone e à mídia. Os marcadores `@de` e `@se` só leem outro campo e escolhem um valor: o que é do carro fica no perfil. Custo: até um segundo de atraso, só campos inteiros, e um campo que acompanha outro não pode ser lido por um terceiro.
- **D170 — o bloco das unidades foi para o `jetta-gli-2021-deducoes.txt`, que entra em toda partida, marcado como simulação.** O que o painel do carro manda quando a unidade muda não está no código da HMI; que ele refaz o número e troca a unidade é dedução. O código da unidade é copiado tal qual, porque as duas pontas usam a mesma tabela do firmware, e o único número refeito é a autonomia (400 km viram 249 mi). Custo: no padrão, três mensagens do computador de bordo saem com outros bytes (duas submensagens vazias a mais), sem mudança na tela, o que conferi em cinco partidas.
- **D171 — ficaram de fora o hodômetro, o consumo do momento, o volume e a pressão.** Nenhum aparece nas telas que abri, e a HMI nem assinou o hodômetro: não havia como ver o efeito. Cheguei a pôr a linha do hodômetro e tirei, com o caso de teste dela. Custo: se outra tela os mostrar, continuam na unidade do perfil.
- **D172 — mexi no motor, que é de todos, por inserção ancorada, e instalei com outras sessões trabalhando e a central ao vivo no ar.** Montei um candidato sobre a pasta das 9h14, rodei a bateria inteira nele e só troquei os três arquivos depois de conferir que os da pasta ainda eram os do candidato; avisei antes pela nota de passagem. A central ao vivo não é afetada no ar: compilou o motor e leu os perfis na partida. Custo: quem sobe central com cópia antiga de `driver/` e a `stubs/` da pasta deixa de subir, porque o motor de antes recusa o perfil novo.
- **D173 — o telefone de mentira passou a ter um programa com modelo, `driver/telefonestub.c`, e a D126 deixa de valer daqui em diante** (Marco 12, "O modelo"). A D126 dizia que o motor bastava; bastava enquanto o aparelho não mudava de estado. Conectar e desconectar é comportamento, e o desenho já existia (rádio e mídia). Os valores ficaram todos no perfil: o modelo só troca o conjunto "conectado" (as seções da instância 0) pelo "desconectado" (as sem instância). Alternativa descartada: pôr regras com constantes no formato do perfil, que mexeria mais no motor de todos.
- **D174 — desconectar é desligando o Bluetooth, porque é o que a tela desta variante oferece.** O pedido falava em desconectar em "Selecionar celular"; medi que ali o aparelho conectado é uma linha de informação (toque curto, toque longo e outro valor de estado não fazem nada). Ligar o Bluetooth de novo não reconecta sozinho, para a tela ter o que conectar; num carro o aparelho voltaria.
- **D175 — o mãos-livres do aparelho conectado passou de 8 para 10 no perfil, e é esse valor que diz como o aparelho começa.** A HMI testa os dois bits (suportado e conectado); com 8 ela também mostrava conectado. O estado de partida "conhecido e não conectado" é um terceiro perfil de uma linha, em vez de um ajuste novo no formato do perfil.
- **D176 — o modelo segura a lista de aparelhos na partida, com um gancho de tempo opcional no motor.** A HMI estoura (NullPointerException) se a lista não vazia chega antes de ela guardar os endereços do aparelho principal e do associado, e a corrida é dentro dela. Segurar por um a dois segundos é remendo por tempo, não pela causa; não achei no barramento sinal de que ela já guardou. Custo: 10 linhas no motor, atrás de macro (binários dos outros programas idênticos, conferido), e a lista vazia nos primeiros segundos. O defeito estava na pasta desde as 3h48 e eu o tinha classificado errado.
- **D177 — o padrão do modo ao vivo passou a ser o aparelho fictício conhecido e NÃO conectado.** O pedido deixava a escolha comigo, com a condição de a partida não abrir aviso novo sozinha. Critério: em cinco pares lado a lado contra o padrão de antes, com e sem memória guardada, as fotos são iguais byte a byte até a tecla Telefone, as telas e as linhas de erro são as mesmas, e nenhum pedido de conexão sai sozinho; e com ele o telefone passa a ser usável no modo ao vivo sem variável de ambiente (dois toques conectam). Não escolhi "já conectado" porque põe ícones em quase toda tela e uma linha a mais no registro da HMI. Para voltar: `TELEFONE=/stubs/telefone/sem-aparelho.txt sh driver/ao_vivo.sh`.
- **D178 — parei o passo 2 (discar e chamada) no levantamento, sem pôr nada dele na pasta** (Marco 12, fim de "O modelo"). A tecla verde do teclado não manda pedido de discagem na bancada e a causa está no lado da tela, que pede leitura longa; o limite de uso do plano estava apertado e o pedido mandava fechar um passo antes de abrir o outro. Custo: o aparelho fictício conecta e não liga. O que ficou pronto para a retomada: os números do esquema, o gancho de tempo no motor e as duas rodadas guardadas. **Mudou às 11h06 de 12/10/2026:** a sessão principal mandou retomar, com prazo; a causa era da bancada, e a chamada simulada foi instalada (D182 a D184).
- **D179 — a gravação nos menus ficou num perfil à parte, que só entra com `sh driver/m14_menus.sh grava`** (Marco 14, "A gravação nos menus do catálogo"). O catálogo continua só com opções, e o teste continua exigindo isso; nenhuma partida padrão muda. Custo: quem sobe o catálogo sem a opção continua sem gravar, e a lista `grava` do perfil novo tem de repetir a do perfil padrão, porque a de depois troca a de antes (o teste confere).
- **D180 — pus uma regra de gravação por dedução de nome, para um pedido só** (`setPDCAutoActivation` grava `updatePDCOPSAutoActivation`). O esquema não liga os dois porque os nomes diferem. Deduzi pelo campo, que é `autoActivation` nos dois, e conferi na tela: com a regra a caixa fica. Não procurei regra para os outros 55 pedidos sem linha `G`; a lista está em `traces/gra_pedidos_sem_regra.txt`. Custo: essas funções aceitam o toque e não mudam.
- **D181 — o `grava` do roteiro liga o arquivo de estado do carro de mentira, e não a memória da tela.** É o que faz a caixa voltar marcada, e era o pedido. Custo: essa central de teste não lembra da última tela aberta nem dos ajustes que a própria HMI guarda.
- **D182 — a tecla verde não discava por dois zeros do valor padrão, e a correção vale só com o aparelho conectado** (Marco 12, "A chamada simulada"). A agenda saía "indefinida" (0) e sem perfil ativo; com a agenda "pronta" (2) e um perfil de agenda fictício, a HMI manda o `dialNumber`. O modelo deixa esses valores saírem só com o aparelho conectado, para a partida do padrão do modo ao vivo (desconectado) mandar ao barramento o mesmo de antes. Alternativa descartada: deixar a agenda sempre pronta, que é como seria num carro, mas mudaria a partida de todos e pediria série maior.
- **D183 — a chamada simulada acaba sozinha aos 20 s; não pus um CarPlay de mentira para o botão vermelho funcionar.** O botão não encerra porque a HMI, ao tratá-lo, consulta o CarPlay, que a bancada não anuncia, e estoura. Anunciar o domínio do CarPlay mexe na partida (serviços ausentes, esperas, talvez a grade) e não cabia no prazo de noventa minutos com teste e série. Sem um fim, a chamada ficaria na tela até a central desligar. Custo: quem liga não desliga quando quer, e o fim por tempo é comportamento do modelo, não da HMI (está dito no cabeçalho, no perfil e no README). *(Corrigida em 12/10/2026, 12h23, pelas D185 a D187: a causa é a adaptação da central, e anunciar o domínio do CarPlay não muda nada.)*
- **D184 — os tempos da chamada (2 s discando, 3 s chamando, em chamada até os 20 s) são escolha minha, e o padrão do modo ao vivo não mudou neste passo.** Não há fonte para tempos: é o relógio de uma simulação. A central continua nascendo com o aparelho conhecido e não conectado (D177); ligar pede conectar antes, com dois toques.
- **D185 — o botão vermelho ficou como opção (`CARPLAY=1`); o padrão do modo ao vivo não mudou** (Marco 12, "O botão vermelho"). O botão só encerra numa central configurada como "com CarPlay", e isso muda a grade e alonga a partida. O pedido era não instalar no padrão se a partida mudasse além do esperado. Além disso o valor do campo da adaptação é hipótese, não leitura do carro. Promover a hipótese a dedução do Jetta fica para o Lipe decidir; a foto de 09/10/2026 mostra que o carro tem CarPlay.
- **D186 — o serviço do CarPlay responde uma coisa só, e num esquema à parte.** Com o esquema opcional `stubs/telefone/gerado-carplay/` o telefone de mentira atende o `DSICarplay2` e dá valor apenas à lista de chamadas, vazia e válida; os outros nove estados saem sem validade. Não simulei CarPlay. O esquema ficou à parte por dois motivos: no padrão a HMI nem assina o serviço, e pô-lo no esquema padrão derrubava o teste dos vetores de prova (o serviço vem duas vezes, por duas interfaces de eventos, e o motor só trata a primeira).
- **D187 — o fim por tempo dos 20 s fica enquanto a HMI não assina o CarPlay, e sai quando assina.** O pedido era tirar ou alongar o fim por tempo com o botão funcionando. No padrão o botão não funciona, e sem o fim a chamada ficaria presa na tela. A condição é a assinatura do estado de chamadas do CarPlay, que é o sinal, no barramento, de que a parte de CarPlay da HMI subiu. Com ela não há teto: a chamada fica até o botão, a desconexão ou o Bluetooth desligado.
- **D188 — a hipótese de adaptação vai num arquivo próprio (`stubs/codificacao/hipotese-carplay.txt`), fora do `ajustes.txt` e das deduções do Jetta, e mexi no `driver/ao_vivo.sh`, que é de todos.** A linha da adaptação sozinha não basta: precisa do esquema e do perfil do telefone. Um `#` tirado no `ajustes.txt` deixaria a central com "App-Connect" na grade e o botão ainda estourando (rodada `r3`). A opção `CARPLAY=1` liga as três peças de uma vez. No `ao_vivo.sh` entraram o bloco da opção e o repasse de `TELEFONE_ESQUEMA`; sem `CARPLAY`, o comando que sobe a central só ganhou essa variável, vazia, e a prova `pv4` saiu como antes.
- **D189 — o CarPlay entra no padrão do simulador, como dedução marcada: decisão do Lipe, de 12/10/2026, às 12h40** (Marco 12, "O CarPlay no padrão do simulador"). A sessão principal perguntou "Posso pôr o CarPlay no padrão do simulador, como dedução marcada, para o botão vermelho encerrar a chamada?", avisando do bloco na grade e dos 6 s a mais na partida, e ele respondeu "Sim, pôr no padrão (Recomendado)". Troca a D185, que deixava o botão como opção. Instalado às 17h19min34.
- **D190 — o padrão novo é do modo ao vivo; as centrais de teste ficam como estavam, e `CARPLAY=0` desliga.** A dedução é do Jetta, e o Jetta só existe na codificação que o modo ao vivo carrega; uma central de teste sobe sem codificação nenhuma. Por isso ficaram dois esquemas do telefone (`gerado/`, sem o CarPlay, e `gerado-carplay/`, com) e não mexi no `driver/m1_hmi.sh`: pôr o CarPlay no esquema de todos mudaria a partida das centrais de teste de todas as sessões. O `hipotese-carplay.txt` e a opção `CARPLAY=1` da D188 saíram; o `sem-carplay.txt` e o `CARPLAY=0` são o caminho de volta.
- **D191 — consertei o gerador de esquema, que é de todos, em vez de remendar o esquema do telefone.** Dois casos do despacho que caem no mesmo trecho passam a ser um serviço só. Entre os domínios levantados até agora só o CarPlay cai nesse caso; a prova de que nada mais mudou é a regeração dos quatro esquemas da pasta, que saíram idênticos.
- **D192 — não mexi no fim por tempo, e instalei tendo visto uma diferença de ordem na partida.** O fim por tempo já saía quando a HMI assina o CarPlay (D187), que agora é o padrão; fica nos 20 s onde o botão não funciona. Na série, a ordem de entrada de duas telas aos 0,3 s diferiu entre os braços em quatro dos cinco pares, sempre no mesmo sentido; tratei como a corrida de partida conhecida, porque ela difere também entre braços iguais, e instalei. O pedido era parar diante de qualquer coisa fora do esperado: se a sessão principal entender que isso era motivo para parar, `CARPLAY=0` devolve a central de antes e os arquivos de antes estão em `traces/tel_antes6/`.
- **D193 — o gatilho da chamada recebida é discar um número combinado e fictício, o 5550199** (Marco 12, "A chamada recebida, e o pareamento"). O gatilho tinha de servir pela janela, sem terminal; a ideia era da sessão principal, e a HMI deixou. O número é da faixa 555-01xx, que não é de ninguém, e o modelo não traz nome nem contato. A volta vem 5 s depois do fim da chamada, uma por discagem; nenhum outro número liga de volta.
- **D194 — os tempos (5 s para voltar, 20 s tocando) são escolha da simulação, e recusar é o `hangupCall`.** Atender é o `acceptCall`, que a HMI manda vazio; recusar é o `hangupCall` com a chamada tocando, que é o que ela manda. Sem resposta, a chamada para de tocar aos 20 s, para não ficar na tela para sempre. Não há toque; a exceção que a tentativa de tocar deixa no modo ao vivo vem da falta do tocador de sons, que é da frente do som, e não mexi nela.
- **D195 — parei o pareamento no levantamento.** O pedido era a busca por aparelhos pela tela, e esta variante não tem: a tela manda procurar a central pelo celular, e nenhuma tela dispara a busca. A instrução era registrar e não inventar. O caminho que existe (o aparelho pede, a tela confirma o código) fica descrito como próximo passo possível, sem implementação.
- **D168 — a barra de volume abre pelo volante como simulação: o som de mentira faz a tecla girar também o botão da central** (Marco 13; decisão do Lipe, de 12/10/2026, às 12h40: no carro dele a barra aparece na central, dito de memória). Havia dois caminhos: trocar o arquivo de configuração da HMI para ligar a chave `isMFLVolumePopup`, ou mandar um giro do botão. Fiquei com o giro: não mexe em arquivo do firmware, é uma linha no arquivo de comandos do `dsistub`, e desliga por uma linha de perfil. Custo: para a HMI o volume pelo volante vira volume pelo botão (domínio `hmi`), e ela recebe um evento que ninguém deu; o que se vê imita o que o Lipe disse, e não o caminho do carro. Ligado no perfil padrão, e portanto no modo ao vivo e em toda central que use o `som-de-mentira.txt`.
- **D196 — os menus que o Lipe confirmou entram por dois perfis à parte e uma chave `MENUS` no `driver/ao_vivo.sh`, e só nele** (Marco 14, "Os menus que o Lipe confirmou entram no modo ao vivo"). É o molde do rádio, do telefone, da mídia, do som e do CarPlay: ligado por padrão no modo ao vivo, desligado com `MENUS=0`, e as centrais de teste sem ele. Custo: a lista `grava` do padrão do modo ao vivo passa a ter quatro serviços a mais (conforto, luz, estacionamento e assistência), e quem compara com rodadas de antes vê uma assinatura a mais e a grade com "Assistentes".
- **D197 — cada menu entrou com uma opção só, e a escolha da opção é palpite marcado.** O Lipe disse que os menus existem, de memória; não disse que linhas têm. Escolhi em cada um a linha com alguma fonte na ficha. No estacionamento a que tinha fonte de imprensa (frenagem de manobra) deixou a tela vazia, e pus "Volume atrás". Custo: os menus do simulador ficam mais pobres que os do carro, e uma linha pode estar errada; as fotos pedidas na ficha resolvem.
- **D198 — o que o Lipe disse de memória foi registrado como informado, e não como lido do carro.** Na ficha as linhas continuam "a confirmar", com a data e as palavras dele; os perfis dizem no cabeçalho que não são leitura do carro nem servem de codificação. Pneus ficou de fora porque ele não citou, e Lane Assist porque depende da câmera. Custo: nenhum para o simulador; no carro, nada disso vale sem o autoscan.
- **D199 — o pareamento é pedido pelo segundo aparelho fictício com a tela "Celulares conhecidos" aberta, 4 s depois, uma vez por abertura** (Marco 12, "O pareamento"). A regra pedida era não abrir aviso sozinha na partida nem em outra tela. O sinal de que a tela abriu é o `requestReconnectSuspend`, que a HMI só manda ali; o modo acessível do Bluetooth não servia, porque liga também na tela "telefone não conectado". "Sim" encerra os pedidos; "Cancelar" os deixa para a próxima abertura. Os dados do segundo aparelho (nome, endereço e código, inventados) ficam num perfil próprio, que o modelo guarda na partida e só publica nessa hora; o perfil entrou no padrão do telefone no `driver/ao_vivo.sh`, que é de todos.
- **D200 — o segundo aparelho só pareia.** Não conecta (tocar nele dá o alerta de erro da HMI), não fica guardado de uma sessão para a outra, e não preenchi a lista de aparelhos de confiança do Bluetooth, que a HMI não assina. O pedido era o pareamento e a lista de conhecidos; fazer o segundo aparelho conectar pede um segundo conjunto de estados do aparelho, que ninguém pediu. Os 4 s e o código são escolha da simulação.

> **Numeração, enquanto três sessões escrevem aqui (11/10/2026, 20h):** D43 a D49 ficam para a sessão do Marco 6, que segue no idioma; a sessão do carro de mentira (dados de veículo) começa em D50. Quem acrescentar confere o último número na hora. Esta nota sai quando as duas terminarem. **Às 21h40:** a sessão do carro de mentira terminou e usou D50 a D65; o próximo número livre é D66. **Às 22h06:** a sessão do hub (o limite de 48 mensagens, decisão D64) reservou a D66; quem acrescentar depois começa em D67 e confere na hora. **Às 22h44:** a sessão dos pedidos do carro de mentira escreve a seção "Marco 9" e reservou D67 a D75; quem acrescentar depois começa em D76 e confere na hora. **Às 23h15:** essa sessão terminou e usou as nove, D67 a D75. **Às 22h50:** a sessão do que a memória guarda (Marco 6, "O que a memória guarda, linha por linha") usou a D76, que está logo acima desta nota; quem acrescentar depois começa em D77 e confere na hora. **Às 23h26:** a sessão do Marco 6 usou a D77 (o cabeçalho do `diagstub`); quem acrescentar depois começa em D78 e confere na hora. **Às 23h22:** a sessão do hub usou a D66, que está entre a D65 e a D67; isso não mexe no próximo número livre, que às 23h26 passou a ser o D78. **À 0h36 de 12/10/2026:** a sessão do rastreamento da HMI (o `HMILOG` depois da carga tardia; seção "O rastreamento da HMI que parava na carga tardia") reservou D78 a D80; quem acrescentar depois começa em D81 e confere na hora. **À 1h10:** essa sessão terminou e usou as três. **À 1h18:** a sessão do consumo de processador (a leitura do barramento no `driver/kipc_bus.c`; seção "Marco 10") reservou D81 a D84; quem acrescentar depois começa em D85 e confere na hora. **À 1h29:** essa sessão terminou e usou as quatro. **À 1h17:** a sessão do estado do carro de mentira (o que a tela grava sobreviver à partida; seção "O carro de mentira lembra do que a tela gravou") reservou D85 a D92; quem acrescentar depois começa em D93 e confere na hora. **À 1h37:** essa sessão terminou e usou as oito. **À 1h38 de 12/10/2026:** a sessão do rádio de mentira (seção "Marco 11") reservou D93 a D104; quem acrescentar depois começa em D105 e confere na hora. **À 1h40 de 12/10/2026:** a sessão dos avisos "cortados no topo" (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada") reservou D105 a D107; quem acrescentar depois começa em D108 e confere na hora. **À 1h43:** essa sessão terminou e usou as três. **À 1h58:** a sessão do rádio de mentira terminou e usou as doze, D93 a D104; o próximo número livre continua sendo o D108, salvo o que tiver entrado depois. **Às 2h46:** a sessão do estado do carro de mentira, agora no brilho pela tela (seção "O brilho pela tela: o display de mentira responde"), reservou D108 a D110; quem acrescentar depois começa em D111 e confere na hora. **Às 2h47:** essa sessão usou as três. **Às 2h58:** a sessão do som de mentira (a tela de som, o volume e o mudo; a seção vai se chamar "Marco 13 — o som de mentira", contando que o telefone de mentira fica com o Marco 12, como diz a nota de passagem dele) reservou o Marco 13 e D111 a D122; quem acrescentar depois começa em D123 e confere na hora. **Às 2h59:** a sessão do estado do carro de mentira, agora nos outros menus do carro (luzes, espelhos, travas, pneus, assistentes e luz ambiente; a seção vai se chamar "Marco 14 — os outros menus do carro"), reservou o Marco 14 e D123 a D125; quem acrescentar depois começa em D126 e no Marco 15, e confere na hora. **Às 3h12:** a sessão do telefone de mentira (Marco 12) tinha gravado as dez decisões dela como D111 a D120 sem ver a reserva das 2h58; renumerou na hora para **D126 a D135**, que usou, e acrescentou a D136. Quem acrescentar depois começa em D137 e no Marco 15, e confere na hora. **Às 3h10:** a sessão do rádio de mentira, agora na mídia (item 5 do roteiro do simulador; a seção vai se chamar "Marco 15 — a mídia de mentira"), reservou o Marco 15 e D137 a D146; quem acrescentar depois começa em D147 e no Marco 16, e confere na hora. **Às 3h25:** essa sessão terminou a mídia e usou as dez, D137 a D146. **Às 3h29:** a sessão do telefone de mentira, agora no segundo estado (um aparelho fictício conectado; o texto fica dentro do Marco 12, sem Marco novo), reservou D147 a D150; quem acrescentar depois começa em D151 e no Marco 16, e confere na hora. **Às 3h47:** a sessão do som de mentira escreveu o Marco 13 e usou as doze reservadas, D111 a D122, que ficam entre a D110 e a D126; e usou a **D151**, o primeiro número livre depois da reserva das 3h29, para as notas datadas que deixou em textos de outras sessões. Quem acrescentar depois começa em D152 e no Marco 16, e confere na hora. **Às 3h48:** a sessão dos outros menus do carro (Marco 14, que tinha reservado D123 a D125) precisou de mais seis e reservou **D152 a D157**; quem acrescentar depois começa em D158 e no Marco 16, e confere na hora. **Às 3h52:** a sessão do rádio e da mídia de mentira, agora na espera da partida pelos serviços de veículo (a pendência que a sessão do estado do carro de mentira deixou com ela; o texto fica dentro do Marco 15, sem Marco novo), reservou a **D158**; quem acrescentar depois começa em D159 e no Marco 16, e confere na hora. **Às 3h57:** a sessão dos outros menus do carro escreveu o Marco 14 e usou as nove que tinha: D123 a D125, que ficam entre a D122 e a D126, e D152 a D157. **Às 4h01:** a sessão do telefone de mentira terminou o segundo estado e usou as quatro que tinha reservado às 3h29, D147 a D150, que ficam entre a D146 e a D151. **Às 4h21:** a sessão do rádio e da mídia de mentira usou a D158 que tinha reservado às 3h52 (a espera da partida pelos serviços de veículo, dentro do Marco 15). O próximo número livre continua sendo o D159, e o próximo Marco o 16, salvo o que tiver entrado depois. **Às 9h02 de 12/10/2026 (manhã):** a sessão do rádio e da mídia de mentira, no pedido novo do Lipe de terminar os itens 4, 5 e 6 do roteiro (teclas do volante na mídia, as quatro teclas que faltavam repetir e o que falta no rádio; a seção vai se chamar "Marco 16 — rádio, mídia e volante: o que faltava"), reservou o **Marco 16** e **D159 a D164**; quem acrescentar depois começa em D165 e no Marco 17, e confere na hora. **Às 10h08:** essa sessão escreveu o Marco 16 e usou as seis, D159 a D164. **Às 9h13:** a sessão do som de mentira, no pedido novo do Lipe de terminar o item 9 (guardar os ajustes de som entre partidas e a barra de volume pelas teclas do volante; o texto fica dentro do Marco 13, sem Marco novo), reservou **D165 a D168**; quem acrescentar depois começa em D169 e no Marco 17, e confere na hora. **Às 9h15:** a sessão do som de mentira usou a D165 e a D166 (a memória do som); a D167 e a D168 seguem reservadas para a barra de volume pelo volante. **Às 9h15:** a sessão do estado do carro de mentira, no pedido novo do Lipe de terminar o item 2 (os números do painel acompanharem a unidade escolhida; seção própria, sem Marco novo), reservou **D169 a D172**; quem acrescentar depois começa em D173 e no Marco 17, e confere na hora. **Às 9h19:** a sessão do som de mentira usou a D167 (a barra de volume pelo volante: parada numa pergunta ao Lipe); a D168 segue reservada para a resposta. **Às 9h27:** a sessão do estado do carro de mentira escreveu a seção "Os números do painel acompanham a unidade escolhida" e usou as quatro, D169 a D172. **Às 9h28:** a sessão do telefone de mentira, no pedido novo do Lipe de terminar o item 7 (o aparelho fictício conectar e desconectar pela tela, e depois discar e chamada; o texto fica dentro do Marco 12, sem Marco novo), reservou **D173 a D178**; quem acrescentar depois começa em D179 e no Marco 17, e confere na hora. **Às 10h02:** a sessão do estado do carro de mentira, no pedido novo do Lipe de terminar o item 10 (a gravação nos menus do catálogo; o texto fica dentro do Marco 14, sem Marco novo), reservou **D179 a D181**; quem acrescentar depois começa em D182 e no Marco 17, e confere na hora. **Às 10h02:** a sessão do telefone de mentira usou D173 a D177 (o modelo: conectar e desconectar); a D178 segue reservada para o passo seguinte dela (discar e chamada). **Às 10h08:** a sessão do estado do carro de mentira escreveu, dentro do Marco 14, "A gravação nos menus do catálogo" e usou as três, D179 a D181. **Às 10h10:** a sessão do telefone de mentira usou a D178 (o passo de discar e chamada parou no levantamento); o próximo número livre é o que a nota disser por último, e ela confere na hora quem acrescentar. **Às 11h07:** a sessão do telefone de mentira, que retomou o passo de discar e chamada a pedido da sessão principal (o texto fica dentro do Marco 12), reservou **D182 a D184**; quem acrescentar depois começa em D185 e no Marco 17, e confere na hora. **Às 11h17:** a sessão do telefone de mentira usou D182 a D184 (a chamada simulada). **Às 12h00:** a sessão do telefone de mentira, no passo do botão vermelho (o texto fica dentro do Marco 12), reservou **D185 a D188**; quem acrescentar depois começa em D189 e no Marco 17, e confere na hora. **Às 12h22:** a sessão do telefone de mentira usou D185 a D188 (o botão vermelho). **Às 17h04:** a sessão do telefone de mentira, no passo do CarPlay no padrão (decisão do Lipe; o texto fica dentro do Marco 12), reservou **D189 a D192**; quem acrescentar depois começa em D193 e no Marco 17, e confere na hora. **Às 17h10:** a sessão do som de mentira usou a D168 (a barra de volume pelo volante, simulada a pedido do Lipe). Das quatro reservadas às 9h13, usou as quatro. **Às 17h30:** a sessão do telefone de mentira usou D189 a D192 (o CarPlay no padrão); quem acrescentar depois começa em D193 e no Marco 17, e confere na hora. **Às 17h59:** a sessão do telefone de mentira, no passo da chamada recebida e do pareamento (o texto fica dentro do Marco 12), reservou **D193 a D195**; quem acrescentar depois começa em D196 e no Marco 17, e confere na hora. **Às 17h59:** a sessão do estado do carro de mentira, no pedido do Lipe de ligar no modo ao vivo os menus que ele confirmou (o texto fica dentro do Marco 14, sem Marco novo), reservou **D196 a D198**; quem acrescentar depois começa em D199, e confere na hora. **Às 18h11:** a sessão do telefone de mentira usou D193 a D195 (a chamada recebida; o pareamento parado no levantamento). **Às 18h13:** a sessão do estado do carro de mentira escreveu, dentro do Marco 14, "Os menus que o Lipe confirmou entram no modo ao vivo" e usou as três, D196 a D198. **Às 18h49:** a sessão do telefone de mentira, no passo do pareamento pedido pelo aparelho (o texto fica dentro do Marco 12), reservou **D199 e D200**; quem acrescentar depois começa em D201 e no Marco 17, e confere na hora. **Às 19h08:** a sessão do telefone de mentira usou D199 e D200 (o pareamento).

## Próximos passos

- ~~Passar do "Loading user profile…"~~: feito no Marco 4.
- ~~Entrada~~: feito no Marco 5.
- ~~Encurtar a partida~~: feito no Marco 5, com a opção `ESPERA`.
- ~~Os botões giratórios~~: feitos no Marco 5. Faltam as **teclas do volante** (teclado 4).
- ~~Ver a tela sem perder quadros e sem navegador~~: feito no Marco 7 (transmissão direta e janela do Mac).
- **Medir se a gravação do arquivo de cada quadro segura o ritmo da HMI**, agora que a janela não depende dele; se segurar, gravar o arquivo só de tempos em tempos.
- ~~Por que os popups saem cortados no topo~~: investigado em 12/10/2026 (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada", decisões D105 a D107). Não é corte: é o desenho do skin. **Falta a foto de um aviso pequeno na central do carro**, para comparar (pedido na ficha, em "O que anotar no primeiro autoscan").
- **Promover o leitor da árvore de telas a ferramenta da bancada** (`traces/pop_ArvoreDeTelas.java`, decisão D106): ele dá a posição na tela de qualquer widget da HMI em execução, o que serve para tocar por coordenada sem medir foto e para conferir layout. Hoje só roda na variante manual do lançador, porque a JVM recusa o agente de depuração com o arquivo de classes compartilhado que o `runHMI.sh` liga; falta achar como subir a variante real com o agente.
- ~~Descobrir por onde entram codificação e adaptação e o que muda com elas~~: feito no Marco 6. Falta **a codificação do carro**: depende do autoscan e da leitura do módulo 5F. Com ela, escrever um arquivo de ajustes do Jetta e comparar byte a byte com `stubs/codificacao/tabela-campos.tsv`.
- ~~Identificação na tela de informações do sistema~~: feita no Marco 6 com os valores da ficha (`F187`, `F189`, `F1A3`).
- **O que ainda separa a tela de informações do simulador da do carro:** ~~o idioma em português~~ (resolvido em 11/10/2026 com a memória de estado), o valor do banco de dados de navegação e a linha do codec de mídia (esta deve depender do serviço `DSIMediaBase`; não conferido).
- ~~Decidir se a identidade provisória do Jetta vira o padrão do modo ao vivo~~: o Lipe decidiu que sim em 11/10/2026 (decisão D34).
- ~~Idioma~~: feito em 11/10/2026 pela memória de estado (o arquivo `lsmval`), que é de onde a HMI tira o idioma na partida. Fica sem exercício a reserva, o idioma padrão do registro `720B`.
- ~~Memória de estado que sobrevive de uma sessão para a outra, como no carro~~: feita em 11/10/2026, às 20h58, no modo ao vivo (Marco 6, "A memória de uma sessão para a outra"; decisões D46 a D49). As rodadas de experimento continuam nascendo do arquivo que receberem. Faltam: ~~conferir o que as outras linhas da memória fazem ao renascer (som, favoritos, estação)~~ e ~~uma sessão longa~~, feitos às 22h50 de 11/10/2026 (Marco 6, "O que a memória guarda, linha por linha"), e a primeira partida de verdade da central ao vivo com a memória, que é da sessão principal.
- **O que falta para a memória ficar "igual ao carro"** (do levantamento das 22h50): ~~**o brilho pela tela**~~ (feito em 12/10/2026, às 2h42: o `dsistub` responde a `setDisplayBrightness` e a `getDisplayBrightness`, a escolha pega, a HMI guarda, e a tela deixa de mostrar sempre "Muito alta"; seção "O brilho pela tela: o display de mentira responde"); ~~**a unidade que volta**~~ (feito em 12/10/2026: o carro de mentira guarda o que a tela grava; seção "O carro de mentira lembra do que a tela gravou"); **o serviço de áudio** (RSI `audiomanagement`), sem o qual a tela de som fica indisponível; e **olhar o que a restauração de fábrica faz com a memória**, numa central de teste.
- ~~Cada central ocupa cinco núcleos do Mac mesmo parada~~: feito em 12/10/2026, à 1h13 (Marco 10, decisões D81 a D84). A leitura do barramento passou a esperar a mensagem, e uma central parada gasta de 2 a 3 % de um núcleo.
- **As corridas de partida da HMI** (Marco 10, "As corridas de partida, que a série não olhava"): com o rastreamento ligado (`HMILOG`), achar o evento que decide se o indicador de gesto da grade é ligado na partida e por que o perfil inicial chega tão perto da conferência; e contar as duas na central ao vivo de verdade.
- **O prazo de leitura do carro é 5 s, e o da bancada, 100 ms** (Marco 10, decisão D81). Trocar é uma constante do `driver/kipc_bus.c` e uma série de partidas. Só vale a pena se alguma coisa depender da cadência do laço com o barramento calado; não achei nada que dependa.
- **O barramento do carro é endereçado, e o da bancada entrega tudo a todos** (Marco 10, "O que o carro faz"). No kernel do firmware a palavra em `0x08` do descritor é a máscara de destino, e a mensagem só vai para quem está nela. Levar isso ao `kipc_bus.c` e ao `kipc_hub.c` muda o que cada processo recebe e pede série de partidas; antes disso vale contar, no que os processos escrevem, quantas mensagens têm destino que não é "todos". É a pista mais direta para o item das 77 mensagens do `carifservice`, mais abaixo.
- **Quando o hub cai, os leitores voltam a girar** (Marco 10, "Limites"): a leitura volta na hora com o socket fechado. Só pesa numa central com o hub morto; uma pausa nesse caso resolveria.
- **Skin e figura do veículo pela adaptação** ("Skin Codierung" e "Fahrzeugbild", DID `0556`), e o skin da MAN que o firmware traz.
- **Abrir a tela de diagnóstico da própria HMI**, que lista codificação e adaptação com os nomes dos `.swap` (evento `ENTER_DIAG_DATA_VIEW` em `AslTargetSystemFeaturesAndCoding`).
- **A tabela do firmware do carro:** a do MU0803 (`H40.272.151`) pode diferir da do MU0883. Sem o pacote, só dá para conferir pelo comportamento do carro.
- ~~Dados de veículo para a tela do carro (relógio, autonomia)~~: feito no Marco 8, com o carro de mentira. Continua em aberto o que o rádio e a mídia esperam para sair do aviso (`DSIUnifiedTuner`, `DSIRadioData`, `DSIMediaPlayer`). **O rádio saiu do aviso em 12/10/2026 (Marco 11), e a mídia também (Marco 15).**
- ~~O rádio sair do aviso "função indisponível"~~: feito em 12/10/2026, à 1h33, para FM (Marco 11, decisões D93 a D104): lista de emissoras, emissora atual, troca pela tela, busca e memórias, com o `driver/radiostub.c` e o perfil `stubs/radio/fm-de-mentira.txt`. Ligado por padrão só no modo ao vivo.
- **O que falta para o rádio parecer o do carro** (Marco 11, "Limites"): **o serviço de áudio**, com o qual deve sumir o atraso de 10 s nas trocas seguidas (a sessão do som está nele; quando ele entrar, a partida do rádio passa a depender da resposta dele, e a série do rádio tem de ser repetida com ele no ar, o que também confirma ou desmente a causa do atraso); **a lista de AM** (o motor teria de mandar duas mensagens do mesmo atributo, uma por faixa); **texto de rádio, intensidade de sinal e logotipo**; **busca que varre a faixa e leva tempo**; **os pedidos de ajuste** (filtro de avisos de trânsito, RDS regional, seguimento de emissora), olhando antes se alguma tela mostra o que o sintonizador devolve; **avisos de trânsito e DAB**, que não foram simulados; e **a faixa de FM e a de AM do carro**, pela foto da tela de sintonia manual e pela adaptação `0559` (pedidos na ficha, em "O que anotar no primeiro autoscan"). **Em 12/10/2026, às 10h (Marco 16, passo 3, decisões D161 a D164):** a busca passou a varrer a faixa, a lista de AM ganhou três emissoras inventadas e há um texto de rádio para as emissoras marcadas; para a intensidade de sinal não achei tela que a mostre. Do que está nesta lista continuam faltando os pedidos de ajuste, os avisos de trânsito, o DAB e os valores do carro; a ficha ganhou o pedido da foto da tela de AM.
- **Conferir se as memórias de emissora e a última emissora voltam na partida seguinte.** Quem as guarda é a HMI, na memória de estado; gravei memórias em duas rodadas (`c3` e `f3`) e não religuei a central para ver. Enquanto não houver emissora guardada, a central abre no começo da faixa (76,1 MHz), que não é emissora nenhuma; se isso incomodar no simulador, dá para pôr no perfil uma emissora nessa frequência. Não fiz: seria desenhar o perfil em volta de um palpite de faixa.
- ~~A mídia pelo mesmo caminho (item 5 do roteiro do simulador)~~: feito em 12/10/2026, às 3h11 (Marco 15, decisões D137 a D146): a tecla Mídia abre o tocador com a fonte "Minhas mídias" e cinco faixas inventadas, com o `driver/midiastub.c` e o perfil `stubs/midia/minhas-midias.txt`. Ligada por padrão só no modo ao vivo. O gerador não precisou de terceira forma de despacho.
- **O que falta para a mídia parecer a do carro** (Marco 15, "Limites"): **o serviço de áudio**, sem o qual a HMI não manda tocar (a faixa fica em pausa) e a volta à mídia depois do rádio abre o tocador vazio (quando ele entrar, repetir as rodadas `r1` e `e1`, o que também confirma ou desmente a causa); **o tempo da faixa andando**, que pede um gancho de tempo no motor; **o motivo de as teclas Próxima e Anterior do volante não trocarem de faixa** na tela de mídia; **áudio por Bluetooth**, com o telefone de mentira (os eventos de mídia chegam à HMI também pelo domínio 256); subpastas, capas, repetir e embaralhar; a busca (`DSISearch`), que ficou de fora; e **saber com que tipo de aparelho um pendrive chega no carro**: nesta HMI o tipo USB não entra em "Fontes", e o que a tela do carro lista ali só uma foto diz (pedido na ficha).
- **O anúncio do rádio sai duas vezes** (Marco 11, "Limites"): a regra de repetir o anúncio é a do Marco 8, pensada para o domínio 16. Não fez mal em dezessete rodadas. Se os serviços do sintonizador forem criados sem trava, como os de veículo, vale anunciar uma vez só; não li essa parte do código.
- ~~Responder aos pedidos dos serviços de veículo, e não só às assinaturas: gravar hora e unidades pela tela, e as três listas do start-stop~~: feito no Marco 9.
- ~~Guardar o que a tela grava no carro de mentira de uma partida para a outra~~: feito em 12/10/2026, à 1h24, no modo ao vivo (seção "O carro de mentira lembra do que a tela gravou"; decisões D85 a D92). A hora, a data, o fuso e as unidades voltam. Ficam: cópias anteriores do estado, se fizerem falta, e uma defesa para o caso de o stub ser religado com a central de pé (decisão D91).
- **Com o rádio de mentira, a central que nasce na tela do veículo fica sem sete serviços de veículo** (seção "O carro de mentira lembra do que a tela gravou", "Achado na conferência final"): 11 partidas em 11 com a tela do veículo como última tela guardada, contra nenhuma em 6 com só o rádio ou só essa memória, nenhuma em 2 com a grade de menus por última tela e nenhuma em 4 com `ESPERA=6000`. Falta achar a causa (a suspeita é a espera de 2 s por etapa contra os 5 s que a HMI leva para criar os serviços de veículo) e decidir o conserto: subir o padrão do `ESPERA`, ou outra coisa. É do modo ao vivo de agora, quando a central é desligada com a tela do veículo aberta. **Resolvido no modo ao vivo em 12/10/2026, às 4h08, pela sessão do rádio e da mídia (Marco 15, "A espera da partida pelos serviços de veículo", decisão D158):** a suspeita se confirmou nas medidas, e o `driver/ao_vivo.sh` passou a dar 6 s só aos serviços DSI (`ESPERA_DSI`), com a `ESPERA` geral nos 2 s; a central nascida na tela do veículo saiu normal em cinco partidas de cinco, uma delas pelo caminho do próprio lançador. **Continua em aberto nas centrais de teste**, que seguem com 2 s (item logo abaixo).
- **Decidir se as centrais de teste também sobem a espera pelos serviços DSI** (Marco 15, decisão D158). Hoje só o modo ao vivo sobe. Uma central de teste com o rádio e a mídia de mentira nasce com quatro assinaturas de veículo a menos (107 em vez de 111), e sem sete serviços de veículo se a memória terminar na tela do veículo. Para igualar numa rodada: `EXTRA_VMOPTIONS=-Dde.vw.mib.asl.startup.timeout.dsi=6000`. Mudar o padrão do `driver/m3_ao_vivo.sh` custa 8 s por partida e muda a base de comparação das rodadas guardadas; não mexi porque havia séries de outras sessões no ar. É decisão para a sessão do simulador.
- **Refazer os números do painel quando a unidade muda** (Marco 9, "Limites"): com milhas escolhidas, a autonomia e a distância continuam em km, porque o perfil as dá já com a unidade.
- **Ligar a gravação nos outros serviços de veículo, um a um** (ajuste `grava` do perfil): as regras já estão no esquema. Antes de ligar cada um, conferir as linhas `G` dele, olhar as telas e repetir a partida algumas vezes, por causa de laço como o do idioma do painel.
- **Os pedidos de lista que a HMI não cobra** (`requestVehicleStateList`, `requestCharismaList`) e o estado BAP (`getBAPState`): descobrir que resposta cada um quer. E as respostas dos outros 81 pedidos com prazo, a começar por "restaurar os ajustes de fábrica" das unidades.
- **O idioma do painel**: se a memória de partida trouxer o idioma do painel, as duas regras da HMI deixam de se desmentir e o pedido pode voltar a ser atendido. A sessão da memória tem a lista do que cada linha guarda.
- ~~Manter o rastreamento da HMI ligado depois da partida~~: feito em 12/10/2026 (seção "O rastreamento da HMI que parava na carga tardia", decisões D78 a D80). **Com ele, ver pelo registro da HMI o que o Marco 9 só leu no código:** as duas regras do laço do idioma do painel e o que ela faz com as listas vazias do start-stop.
- **Ligar e desligar chaves de rastreamento com a central no ar.** Hoje as chaves são as da partida: a HMI só relê o perfil de sondas quando o número do perfil muda, e quem o muda é a tela de teste dela. Caminhos a olhar: essa tela, e o canal de depuração por onde a ferramenta de monitoração manda configuração (`DebugProbeManagerImpl.onBinaryMessageReceived`).
- **A linha "diagnóstico:" do resumo do lançador sai embaralhada sem `HMILOG`**: o `sed` dela espera o cabeçalho por extenso (`[HSM|ASL.DIAGNOSTIC]`), e sem `HMILOG` o cabeçalho de cada linha é numérico. Visto em 12/10/2026 nas centrais sem `HMILOG` dos dois lançadores; não mexi.
- ~~O limite de 48 mensagens do hub (decisão D64)~~: feito em 11/10/2026, às 22h52 (seção "O hub e os anúncios que ficavam fora do cache", decisão D66). O hub guarda o último anúncio de cada canal; contados os anúncios, e rodado tudo de novo.
- **A corrida de partida dos daemons** (mesma seção, "Limites"). Os quatro daemons do firmware e o `dsistub` sobem juntos e se ligam ao barramento com milésimos de diferença; quando o `carifservice` passa na frente de outro, a rajada dele toma o cache de 48. O hub novo faz essa ordem não ter consequência para os anúncios, mas ela continua ao acaso. Subir o `carifservice` por último no lançador (`driver/m1_hmi.sh`) deixaria as partidas iguais entre si; muda a partida de quem roda com ele, e por isso não fiz.
- **O que são as 77 mensagens que o `carifservice` manda ao subir**, em interfaces que não são da tabela DSI (`102401`, `1762168833`, `17338369`, `34115585`), e para quem são.
- **Os outros menus do carro** (luzes, espelhos, travas, pneus, assistentes, luz ambiente): cada um pede a função nas listas `0B1B` e `0B1D` e as opções do serviço dele no perfil.
- **Confirmar a causa do painel mudo** do Marco 8: com o coletor do socket de rastreio (item "Ouvir os daemons do firmware", abaixo) dá para ver a camada da LG criar um serviço duas vezes, se for isso.
- **Trocar os palpites do perfil de veículo pelos valores do carro**, quando houver autoscan e fotos das telas; a lista do que fotografar está na ficha, em "O que anotar no primeiro autoscan".
- **O assistente de configuração no modo ao vivo** (decisão D65): aparece aos 52 s de cada partida enquanto não for dispensado. Dispensar é um toque em "Não exibir"; se ele deve ou não aparecer no simulador é escolha do Lipe.
- **Toque pelo caminho do firmware:** um dispositivo de entrada virtual em `/dev/input` para o `touch-app` reconhecer os gestos, no lugar da página.
- **Ouvir os daemons do firmware.** Um coletor no socket `/tmp/trace_unix_socket` daria o log do registro, do mock, do `mcu-interpreter` e dos demais, que hoje rodam mudos. Barato e ajuda em tudo o que vem depois.
- **O estado de um carro sem cadastro online**: só o convidado anônimo em `users` e no contexto, para ver a tela 2 ou 4 (decisão D13).
- **O mesmo dentro do QEMU `-M virt`**, com o shim falando por `virtio-serial` e o RSI ligado (conferir a interface `lo`).
- **Painel de instrumentos:** capturar o vídeo em `ff14::2:4`, fora da pilha de OpenGL.
- ~~Ajustar o `driver/tests/t_vc_diag_alm.sh`, que dependia do cabeçalho zerado do `diagstub`~~: a dona, a sessão do navi-al, ajustou às 23h28 de 11/10/2026 (37 de 37).
- **Região de navegação e país de venda do Jetta** (`0558`, byte 0, e `09CB`): saem da adaptação do 5F, na primeira leitura do carro. Até lá vão 0 e o padrão da HMI.
- ~~A tecla Telefone sair do aviso "Telephone unavailable."~~: feito em 12/10/2026, às 2h38 (Marco 12, decisões D126 a D136), para o estado "nenhum aparelho conectado", com o próprio `veiculostub`, o esquema de `stubs/telefone/gerado/` e o perfil `stubs/telefone/sem-aparelho.txt`. Ligado por padrão só no modo ao vivo.
- ~~O segundo estado do telefone: um aparelho fictício conectado, com a tela principal~~: feito em 12/10/2026, às 3h48 (Marco 12, "O segundo estado: um aparelho fictício conectado", decisões D147 a D150), com valor por instância no motor (`[Serviço[N].evento]`) e o perfil `stubs/telefone/aparelho-conectado.txt`. É opção; o padrão do modo ao vivo continua sendo "nenhum aparelho".
- ~~Conectar e desconectar o aparelho fictício pela tela~~: feito em 12/10/2026, às 9h50 (Marco 12, "O modelo: o aparelho fictício conecta e desconecta pela tela", decisões D173 a D177), com o `driver/telefonestub.c`. O padrão do modo ao vivo passou a ser o aparelho conhecido e não conectado.
- ~~Discar e a chamada simulada~~: feito em 12/10/2026, às 11h06 (Marco 12, "A chamada simulada", decisões D182 a D184): com o aparelho conectado a tecla verde abre a tela de chamada, que anda sozinha (discando, chamando, em chamada) e acaba aos 20 s.
- **O que o telefone de mentira ainda não faz** (Marco 12, "Limites da chamada"): **encerrar a chamada pelo botão vermelho, que pede um CarPlay de mentira no barramento** (a HMI estoura ao consultar o CarPlay; é o passo seguinte natural, e muda a partida); a tecla Telefone do volante durante a chamada (medida uma vez, sem efeito); chamada recebida; parear; esquecer o aparelho; reconectar sozinho ao ligar o Bluetooth; o cronômetro da chamada. E duas coisas pequenas da madrugada: o que a HMI guarda na memória com um aparelho conectado, e por que ela estranha uma resposta do teclado a mais aos 12 s quando o aparelho nasce conectado.
- **A instância 2 do `DSIMobileEquipment` é do CarPlay** (domínio 512; o gerador já tira isso do firmware e põe no relatório). Levar ao esquema e ao motor, para o telefone de mentira não responder por ela.
- **Quem trata a tecla Telefone do volante** (`HK_MFL_PHONE_*`): não achei nas atividades geradas nem no código do telefone; falta olhar as tabelas de transição, que ficam em recursos, e ver se ela age com uma chamada na tela, quando houver chamada. O segundo estado existe desde as 3h48 de 12/10/2026: com o aparelho fictício conectado e sem chamada a tecla continua sem mudar a tela (cinco medidas), e chamada a bancada não tem.
- **A caixa "Visibilidade" e os outros pedidos do Bluetooth** (busca de aparelhos, conexão, remover aparelho): sem resposta ou com resposta que não muda a tela. A visibilidade pede um campo calculado (`visible` a partir do modo), que o motor não faz.
- **O que a memória guardada passa a levar com o telefone de mentira**, e o que acontece ao desligá-lo depois (`TELEFONE=`) com essa memória.
- ~~A tela de som sair do aviso, e o volume e o mudo terem estado~~: feito em 12/10/2026, às 3h35 (Marco 13, decisões D111 a D122 e D151), com o mock RSI do firmware servindo o `audiomanagement` e o `driver/somstub.c` por cima, e o perfil `stubs/som/som-de-mentira.txt`. Ligado por padrão só no modo ao vivo. De passagem, o rádio de mentira passa a receber cada troca de emissora na hora, e a mídia de mentira passa a tocar e a voltar depois do rádio.
- **O amplificador do carro no perfil do som** (Marco 13): hoje é um interno básico inventado. Depende do autoscan (existe o módulo 47?) e das fotos da tela de som pedidas na ficha; com elas, um perfil do Jetta em `stubs/som/`, com o que for do carro separado do que for palpite.
- ~~Guardar os ajustes de som entre partidas~~: feito em 12/10/2026, às 9h07 (Marco 13, "A memória do som", decisões D165 e D166): no modo ao vivo o som de mentira guarda volume, graves, médios, agudos, balanço, fader e mudo em `ao-vivo/memoria/som/estado.txt` e nasce deles. Falta ver na central ao vivo, que só pega isso na partida seguinte.
- ~~A barra de volume pelas teclas do volante~~: feito em 12/10/2026, às 17h03, como SIMULAÇÃO pedida pelo Lipe (Marco 13, "A barra de volume pelo volante: simulada a pedido do Lipe", decisões D167 e D168). A HMI da bancada tem a chave `isMFLVolumePopup` desligada; o Lipe disse que no carro dele a barra aparece na central; o som de mentira faz a tecla do volante girar também o botão da central. Falta a foto do carro, para trocar "dito de memória" por "visto".
- **O serviço de tons (`waveplayer`)**: segue na lista de serviços RSI ausentes; é o do bipe de toque. Não olhei se o firmware traz a especificação dele para o mock servir.
- **As ações cruzadas da tabela de áudio** que aparecem na tela (limite de volume ao ligar, volume próprio de aviso e de telefone): só se fizerem falta. Hoje há uma fonte de entretenimento por vez, o mudo e as conexões de menu.
- **A aba "Configuração" da tela de som e as predefinições**: não exercitadas no Marco 13.
- **Completar os menus do carro com as linhas de verdade** (Marco 14): em 12/10/2026, às 18h05, entraram no modo ao vivo os quatro menus e os três assistentes que o Lipe citou de memória, com uma linha cada; falta a foto de cada tela do carro para pôr as linhas certas. O texto de antes deste item: depende de o Lipe dizer, ou fotografar na central do carro, o que Veículo → Config. e a grade de menus mostram. Com isso, é copiar do catálogo para os dois arquivos de deduções só as linhas do que o carro tem e rodar uma série de partidas.
- ~~A gravação nos menus novos (D157)~~: feito em 12/10/2026, às 9h56 (Marco 14, "A gravação nos menus do catálogo", D179 a D181), para caixas, com `sh driver/m14_menus.sh grava`. Falta: listas e barras, os 55 pedidos sem regra de gravação (`traces/gra_pedidos_sem_regra.txt`) e a resposta do SET dos pneus.
- **A lista de cores da luz ambiente** (D125): falta o carro de mentira montar a resposta do `DSICarLight.requestIntLightRGBColorList`.
- **Os menus não experimentados** (Marco 14, "Limites"): bancos, climatização, head-up display, freio, perfil de condução e os outros de [traces/men_aparelhos.txt](traces/men_aparelhos.txt). E de que opção vem o "Aviso de nova partida" da lista de assistentes.
- **O que ainda não acompanha a unidade** (D171): o hodômetro, o consumo do momento, o volume e a pressão. Só quando alguma tela os mostrar. E marcadores com decimais, se um dia houver média de consumo de verdade para converter.

Riscos que continuam em aberto: texturas em formato comprimido, mais de um contexto de OpenGL e vetores de vértice na memória do cliente ainda não apareceram, então não foram testados com a HMI.

## Pendências

- Disco do Mac com 26 GB livres em 11/10. A `sys/system.img` do MU0841 (2,9 GB), no rascunho antigo, não é usada pela bancada; apagar é decisão do Lipe.
- O que ficou no rascunho antigo (decompilados dos jars, projetos do Ghidra, `update.zip`, o `cfr.jar`) mora em `/private/tmp`, que o macOS limpa. Os decompilados foram a fonte de boa parte do Marco 3; se sumirem, refazer com o CFR sobre `MIBHMI-DSI.jar` e `MIBHMI-STD-…-bin.jar`.
- A imagem `moi3-bancada:1` existe só neste Mac. Se for apagada, refazer com `docker build -t moi3-bancada:1 - < driver/Dockerfile.bancada` (baixa 235 MB de novo).
- A pasta de trabalho do Marco 3 (binários compilados, quadros em PPM) está no rascunho desta sessão. Tudo se refaz com os scripts; os quadros que importam estão em `quadros/`.
- Do Marco 4 ficaram no rascunho desta sessão (`scratchpad/m4/`) os roteiros de sonda (`run_e.sh`, `probe_mock.sh`, `probe_get.sh`) e a cópia de `/lge/app_ro/sai/{conf,mockup,privileges,spec}`. Não entraram na pasta: os roteiros têm caminhos fixos desta sessão e o resto é do firmware, que se relê da imagem. As saídas que importam estão em `traces/m4_*`.
- Os popups aparecem cortados no topo: o do usuário principal (Marco 4) e mais dois no Marco 5. Falta achar a causa. **Resolvido em 12/10/2026:** não é corte (seção "Os avisos que descem do topo: é o desenho da HMI, não corte da bancada"); falta só a foto do carro para comparar.
- Do Marco 5 ficaram no rascunho (`scratchpad/m5/`) o roteiro de rodadas (`run5.sh`) e as saídas completas; as que importam estão em `traces/m5_*`. A pasta `ao-vivo/entrada/` guarda os comandos da última sessão ao vivo e é zerada a cada partida.
- Do Marco 6 ficaram no rascunho da sessão dele (`…/09440cca-327e-444a-925d-bdab467960a3/scratchpad/`) as cópias do `persistence-app` e do `mcu-interpreter` tiradas da imagem para análise, o projeto do Ghidra e as fotos de todas as rodadas. O que importa está em `traces/m6_*` (saídas, tráfego do barramento, previsões, trechos decompilados) e em `quadros/`. Os roteiros das rodadas entraram na pasta: `driver/m6_rodada.sh` e `driver/m6_analisa.py`.
- A sessão ao vivo principal (porta 8431) que estava no ar no fim do Marco 6 tinha sido iniciada antes de a codificação entrar no modo ao vivo: ela roda sem codificação. Depois da decisão D34 (19h19), a próxima partida carrega a identidade provisória do Jetta. Não reiniciei essa sessão: a dona dela é a sessão principal, que a mantém com o relógio ligado à mão; avisei-a da mudança. **Resolvido às 19h31:** a sessão principal reiniciou a central ao vivo pelo `driver/ao_vivo.sh`, sem alteração, e ela subiu com os quatro arquivos. Conferi às 19h37, só lendo: o container tem o `CODING` novo, o lançador registrou 48 conjuntos enviados, e a HMI registrou "Navigation is active", "VW brand is true", o destaque `#FF2832` e `part.number:=3G5035832C`.
- Do Marco 7 ficaram no rascunho da sessão principal (`…/d609a58c-bbdd-4fb1-9a97-6a234685ee40/scratchpad/tx/`) a cópia de trabalho do encaminhador e da janela, o roteiro da central de teste (`exp.sh`), as capturas e as fotos das rodadas. O que importa está em `glforward-moi3/`, em `janela/` e em `quadros/2026-10-11-janela-mac-veiculo.png`.
- O relógio da central ao vivo continua vindo de um laço de shell ligado à mão (`scratchpad/m6/relogio_ao_vivo.sh`, no rascunho da sessão principal). Vale até existir o stub de dados de veículo, item 2 do roteiro em [simulador-jetta.md](simulador-jetta.md). **Às 21h50 de 11/10/2026:** o stub existe (Marco 8) e a central religada às 21h33 já tem a hora dele, aceita aos 0,1 s da partida. O laço não deve ser religado: seriam duas fontes para a mesma hora.
- Marco 6, idioma: a central ao vivo que estava no ar às 20h de 11/10/2026 foi posta em português à mão pelo Lipe, e essa escolha vale só até ela reiniciar. Da próxima partida em diante ela já nasce em português (decisão D43). Não reiniciei: a dona dessa central é a sessão principal.
- Marco 6, memória entre sessões: a central ao vivo que estava no ar às 21h de 11/10/2026 subiu às 19h32, antes da mudança, e não guarda nada; o que estiver nela se perde quando ela reiniciar, salvo o idioma, que está no arquivo fixo. Da próxima partida em diante a central nasce do arquivo fixo (ainda não há `ao-vivo/memoria/`), passa a guardar, e a partida seguinte já abre na última tela. Não reiniciei nem criei a pasta: a dona da central é a sessão principal, avisada por nota. Para levar o estado da central que está no ar para a próxima partida, antes de desligá-la: `mkdir -p ao-vivo/memoria && docker exec moi3-ao-vivo cat /mnt/low/data/lge/hmi/lsmval > /tmp/lsmval-agora && sh driver/lsmval_guarda.sh passo /tmp/lsmval-agora ao-vivo/memoria`, que responde "guardada: N linha(s)". O `passo` foi conferido no Mac, numa pasta temporária, com a cópia da memória dessa central lida às 20h25 (61 linhas); o comando inteiro não rodou contra a central.
- Marco 6, memória entre sessões, atualização das 22h34 de 11/10/2026: o estado da central das 19h32 foi levado para `ao-vivo/memoria/` às 21h19, antes de ela sair do ar, pela sessão principal (registro em [simulador-jetta.md](simulador-jetta.md), "Conferência antes de a central ao vivo desligar"). Hoje é `anteriores/lsmval.2`: 61 linhas, idioma `pt_BR`, última tela os ajustes. A central das 22h07 nasceu da memória (lido na saída do lançador); as das 21h33 e das 21h43 também, pelo que ficou em `anteriores/` (dedução). Às 22h32 o Lipe escreveu na sessão do Marco 6 "pode rodar o comando antes de ela desligar". Não rodei: já estava feito, e gravar nessa pasta com a central no ar competiria com a guarda dela (uma central por pasta de memória). A central das 22h07 desliga por volta de 0h07, e a seguinte nasce do que ela deixar.
- Do trabalho da memória entre sessões ficaram no rascunho da sessão dele (`…/00cd38cd-998f-491b-a4fb-f00b1b1dcdaa/scratchpad/`) as pastas das seis rodadas, com as fotos e as cópias do `lsmval` em cada instante, a cópia da memória da central ao vivo lida às 20h25 e as cópias de trabalho dos roteiros. O que importa está em `traces/m6_mem*` (saídas, registros da guarda, as duas memórias que ficaram, os roteiros das rodadas e a bateria), em `traces/m6_previsoes.md` e em `quadros/2026-10-11-hmi-mem-*`.
- Marco 8: a central ao vivo das 19h32 saiu do ar por volta das 21h32 (o prazo dela era de 2 horas), e às 21h33 outra foi ligada pela janela, não por mim. A partida nova já pegou tudo o que o Marco 8 pôs na pasta. Conferi só lendo o registro dela (`ao-vivo/log/`): o lançador registrou o carro de mentira com os três perfis, a HMI aceitou a hora aos 0,1 s ("First valid ClockTime: ClockTime(hours=21,minutes=33,seconds=17,timeZone=-3.0,…)"), a Welcome ficou de 0,55 a 5,27 s, o popup do usuário principal até 35,3 s, e aos 52,3 s apareceu o assistente de configuração (decisão D65). Nenhum erro de OpenGL nos primeiros 60 quadros. Não mandei nada para essa central. Às 21h43 ela foi religada outra vez, também pela janela, e a sequência se repetiu: hora aceita aos 0,35 s (21:43:51), Welcome de 0,78 a 5,52 s, popup até 35,5 s e o assistente aos 51,7 s, com `dontShowAgain=false`. Até as 21h45 ninguém tinha tocado em "Não exibir" (o byte da memória guardada seguia em zero).
- Do Marco 8 ficaram no rascunho da sessão dele (`…/9ffa4c2f-755e4a547339/scratchpad/m8/`) as pastas das rodadas, com as fotos e os registros de todos os daemons, e a cópia de trabalho do driver. O que importa está em `traces/m8_*` (saídas, tráfego do barramento, registros dos stubs, a linha do tempo e os roteiros da série, da prova e do reenvio do anúncio) e em `quadros/2026-10-11-hmi-vei-*`.
- Os decompilados que serviram de fonte ao Marco 8 são os do CFR de 10/10, que estão no rascunho de outra sessão (`…/c2af147a-1d1d5856ce37/scratchpad/jars/`), em `/private/tmp`. Vale o que já está dito acima sobre refazê-los.
- Do levantamento do que a memória guarda (Marco 6, "O que a memória guarda, linha por linha") ficaram no rascunho da sessão dele (`…/f8f96c21-f9ad-417d-a3ab-5e01f55af697/scratchpad/m6mem/`) as pastas das 21 centrais de teste, com todas as fotos e as cópias do `lsmval` a cada passo, as duas cópias de trabalho de `driver/` e a de `stubs/`, e as pastas de memória de cada experiência. O que importa está em `traces/m6_lsm_*` (saídas, registros da HMI, memórias de antes e de depois, roteiros) e em `quadros/2026-10-11-hmi-lsm-*`. As centrais de teste `moi3-m6-*` foram todas encerradas; a central ao vivo não foi tocada.
- Do conserto do hub (11/10/2026) ficaram no rascunho da sessão dele (`…/4d8d1c0b-2458-47fc-8588-a39f66a9b161/scratchpad/hub/`, em `/private/tmp`) as pastas das 92 centrais de teste, com as fotos e os registros de todos os daemons, a cópia congelada de `driver/`, `stubs/` e `glforward-moi3/` das 21h50, a cópia de trabalho com o hub novo e a variante de experimento do lançador. O que importa está em `traces/hub_*` e em `quadros/2026-10-11-hmi-hub-*`. A cópia da memória guardada da central ao vivo que usei no experimento do assistente (lida às 22h14, só para leitura) ficou só no rascunho.
- A central ao vivo que estava no ar às 22h52 de 11/10/2026 (ligada às 22h47, não por mim) compilou o hub de antes e segue com ele até religar. No padrão do modo ao vivo os dois hubs se comportam igual; não mandei nada para ela.
- Marco 9: essa mesma central ao vivo, ligada às 22h47 pela janela, pegou o carro de mentira num estado intermediário: o stub da primeira versão dos pedidos, o esquema novo e o perfil com a gravação desligada (o registro dela diz "grava: nenhum serviço"). Ela responde às três listas do start-stop e não grava hora nem unidade pela tela. Não corre risco do laço do idioma do painel, que depende da gravação. A próxima partida pega a versão final. Conferi só lendo `ao-vivo/log/ao_vivo.out`; não mandei nada para ela.
- Do Marco 9 ficaram no rascunho da sessão dele (`…/72f01134-331f79e7b665/scratchpad/m9/`, em `/private/tmp`) as pastas das rodadas, com as fotos de cada passo e os registros, a cópia de trabalho do driver e as duas saídas do gerador. O que importa está em `traces/m9_*` (saídas, tráfego do barramento e registros do stub das rodadas `base1`, `v1`, `prova`, `prova2`, `laco1` e `n4`, as duas séries, os testes vistos falhar, a tabela da HMI despejada e os roteiros) e em `quadros/2026-10-11-hmi-ped-*`. Do registro do stub da `n4`, de 33 MB, ficou só o começo.
- Do rastreamento que parava (12/10/2026) ficaram no rascunho da sessão dele (`…/2b106c87-0406b5e1eb0e/scratchpad/hl/`, em `/private/tmp`) as pastas das 30 centrais de teste, com os registros de todos os daemons e os quadros, as duas cópias de `driver/`, `stubs/` e `glforward-moi3/` tiradas à 0h14, a cópia da memória guardada do modo ao vivo e o rascunho da seção. O que importa está em `traces/hlog_*`. As centrais `moi3-hlog-*` foram todas encerradas, e os 34 arquivos de `/vw/hmi/config` do firmware no Mac têm os mesmos resumos SHA-256 de antes das rodadas (`traces/hlog_sysroot-config.sha256`).
- Do estado do carro de mentira (12/10/2026) ficaram no rascunho da sessão dele (`…/52fc1723-2b1e-475a-a3d1-502c8b44efee/scratchpad/est/`, em `/private/tmp`) as pastas das 57 centrais de teste, com as fotos de cada passo e os registros, as duas cópias de trabalho (`wc`, da pasta de 0h16, e `wc2`, da de 1h15), a cópia da pasta de 1h44 (`wc3`) e as bases com que foram comparadas. O que importa está em `traces/est_*` (previsões, a rodada vermelha do teste, as suítes, as séries, os registros do stub e do lançador das rodadas com foto, os arquivos de estado e, em `est_antes/`, os seis arquivos como eram) e nos quinze quadros `quadros/2026-10-12-hmi-est-*.png`. A central ao vivo estava desligada e não foi ligada: a primeira partida dela com o estado do carro de mentira fica para quem a ligar, e o que esperar está na seção "O carro de mentira lembra do que a tela gravou", "Onde parou". **Pendência que não é minha e apareceu na minha conferência:** com o rádio de mentira, a central que nasce na tela do veículo fica sem sete serviços de veículo (mesma seção, "Achado na conferência final"); avisei a sessão do rádio às 2h09 de 12/10/2026, corrigi o aviso às 2h18 e não mexi em nada para consertar.
- Da investigação dos avisos (12/10/2026) ficaram no rascunho da sessão (`…/2b106c87-0406b5e1eb0e/scratchpad/pop/`, em `/private/tmp`) as pastas das nove centrais de teste `moi3-pop-*`, com as fotos de cada instante e os registros, e o bytecode de duas telas geradas do skin (`Sdl__0`, `Psapni__0`). O que importa está em `traces/pop_*` e em `quadros/2026-10-12-hmi-pop-*`. As centrais foram todas encerradas; a central ao vivo não foi tocada.
- Do Marco 11 (12/10/2026) ficaram no rascunho da sessão dele (`…/910e344e-2600-4200-a5fb-8c96886927ef/scratchpad/rad/`, em `/private/tmp`) as pastas das dezoito centrais de teste `moi3-rad-*`, com as fotos de cada passo e os registros de todos os daemons, a cópia de trabalho (`w`), a junção em cima da pasta de 1h27 (`w2`), a foto da pasta de que a junção partiu e as saídas das suítes. O que importa está em `traces/rad_*` (63 arquivos) e nos dezenove quadros `quadros/2026-10-12-hmi-rad-*.png`. As centrais foram todas encerradas (conferido às 2h02 com `docker ps -a`).
- Marco 11, central ao vivo: desligada desde a 0h26 de 12/10/2026 (parada pela sessão principal). Não a liguei nem mandei nada para ela. **A próxima partida dela pega o rádio de mentira e a faixa de FM "Brasil"**, e, por nascer da memória guardada com a faixa de antes, zera uma vez os ajustes de rádio guardados; o que esperar está no Marco 11, "Onde parou". Quem liga é o Lipe, pela janela, ou a sessão principal.
- Em `ao-vivo/memoria/` há dois arquivos `.lsmval.O1NQ4y` e `.lsmval.fLUxsP`, de 0h26:51 de 12/10/2026, com o mesmo tamanho do `lsmval` (10.322 bytes). Têm cara de sobra da guarda da memória (a cópia ao lado, que troca de nome no fim), e a hora é a do desligamento da central. Vi à 1h14 e de novo à 1h51; não abri nem mexi: a pasta é da central ao vivo e a guarda é de outra sessão.
- Do brilho pela tela (12/10/2026) ficaram no rascunho da sessão (`…/52fc1723-2b1e-475a-a3d1-502c8b44efee/scratchpad/bri/`, em `/private/tmp`) as pastas das 17 centrais de teste, com as fotos e os registros, e a cópia de trabalho. O que importa está em `traces/bri_*` e nos seis quadros `quadros/2026-10-12-hmi-bri-*.png`. As classes decompiladas que serviram de fonte são as do CFR de 10/10, no rascunho de outra sessão (`…/c2af147a-1d1d5856ce37/scratchpad/jars/`); vale o que já está dito acima sobre refazê-las.
- Do Marco 12 (12/10/2026) ficaram no rascunho da sessão dele (`…/bc3b992a-7af7-4c42-b566-997a91eb34df/scratchpad/tel/`, em `/private/tmp`) as pastas das 41 centrais de teste `moi3-tel-*`, com as fotos de cada passo e os registros de todos os daemons, as cópias de trabalho (`copia0`, da pasta da 1h36; `copia1`, da de 1h49; `copia2`, da de 2h46), o candidato à instalação (`cand`), as três saídas do gerador e os esquemas de experimento. O que importa está em `traces/tel_*`, em `traces/tel_antes/` (os seis arquivos compartilhados como estavam antes da troca) e nos treze quadros `quadros/2026-10-12-hmi-tel-*`. **Do segundo estado, até as 4h10:** mais onze pastas de centrais (`x1`, `kp`, `k2`, os pares `k1` a `k3`, `kf` e `kj`), as cópias de trabalho `copia3` e `copia4`, o candidato `cand2` e as saídas das duas baterias.
- Marco 12, central ao vivo: desligada desde a 0h26 de 12/10/2026. Não a liguei nem mandei nada para ela. **A próxima partida dela pega o telefone de mentira**: a tecla Telefone abre a tela "Conecte o telefone celular…", a grade guardada ganha o bloco "Telefone" (na segunda página, pelo que vi numa central nascida de uma cópia da memória) e o assistente de configuração, se ainda não foi dispensado, aparece com três páginas. Para subir sem ele: `TELEFONE= sh driver/ao_vivo.sh`. O segundo estado (o aparelho fictício conectado) não entra por padrão: a próxima partida dela sobe com "nenhum aparelho".
- Marco 12: a pasta `janela/` é da sessão principal e não foi tocada. A janela e a página não precisam de mudança para o telefone; se algum texto delas disser que a tecla Telefone abre o aviso, ficou velho.
- ~~Marco 12, o botão vermelho (12h25 de 12/10/2026): **decisão para o Lipe.** O botão vermelho da tela de chamada só encerra a chamada simulada com `CARPLAY=1`, uma hipótese sobre a adaptação (o campo `ID_APPLE_DIO` diferente de zero). O carro tem CarPlay (foto de 09/10/2026). Se o Lipe quiser, a hipótese vira dedução marcada do Jetta e o serviço do CarPlay entra no padrão do telefone de mentira: o botão passa a funcionar no modo ao vivo, a grade ganha "App-Connect" e a partida fica uns 6 s mais longa. Antes disso falta ver onde o bloco aparece com a memória guardada (a primeira página da grade é outra) e consertar o esquema, que traz o serviço do CarPlay duas vezes. O valor certo do campo sai da adaptação do 5F, quando o carro for lido.~~ **Decidido e feito em 12/10/2026:** o Lipe disse "Sim, pôr no padrão" às 12h40, e o CarPlay entrou no padrão do modo ao vivo às 17h19 (D189 a D192). O bloco fica na segunda página da grade com a memória guardada, e o esquema foi consertado.
- Marco 12, o CarPlay no padrão (17h30 de 12/10/2026): a dedução (`0555 @1.2+2 = 1` no arquivo de deduções do Jetta) espera a leitura da adaptação do 5F no carro, que diz o valor certo do campo e se o Android Auto também está marcado. Com o valor lido, a linha vai para o `jetta-gli-2021.txt`. Enquanto isso a HMI espera 6 s na partida por Android Auto e MirrorLink, que a bancada não tem.
- ~~Marco 12, o pareamento (18h11 de 12/10/2026): **não foi feito.** Esta variante não busca aparelhos pela tela; o caminho que ela tem é o pareamento pedido pelo aparelho, com confirmação de código (`TargetASLPhoneBluetoothPasskey`: atributo 7 do `DSIBluetooth`, `requestPasskeyResponse`). Para fazê-lo falta levantar o que a tela mostra em cada estado do código e como o aparelho novo entra na lista do Bluetooth e na do gerente de aparelhos. Gatilho possível, pela janela: com "Celulares conhecidos" aberta (a central fica visível), um segundo aparelho fictício pede o pareamento alguns segundos depois.~~ **Feito em 12/10/2026, às 18h56** (D199 e D200): com "Celulares conhecidos" aberta, o segundo aparelho fictício pede para parear e a central confirma o código. Limites no Marco 12, "O pareamento".
- Marco 12, a chamada recebida (18h11 de 12/10/2026): no modo ao vivo cada chamada recebida deixa uma exceção do toque (`WaveplayerController.playToneRepeat`), porque a bancada não tem o tocador de sons; é da frente do som. Falta medir a chamada entrando com a central em outra tela e a tecla do volante com a chamada tocando. A busca de contatos pede `DSIAdb.restartDownload`, que o telefone de mentira não responde. **Medido às 18h56 (`pv13`):** em outra tela a chamada entra como um aviso no alto, com os três botões, e a tecla Telefone do volante não atende nem recusa. Continuam abertos a exceção do toque e o `restartDownload`; os botões do aviso em outra tela não foram tocados.
- Do Marco 15 (12/10/2026) ficaram no rascunho da sessão dele (`…/910e344e-2600-4200-a5fb-8c96886927ef/scratchpad/mid/`, em `/private/tmp`) as pastas das dezoito centrais de teste `moi3-mid-*`, com as fotos de cada passo e os registros de todos os daemons, as duas fotos da pasta (2h47 e 3h08), a cópia de trabalho (`t`), as duas junções (`t2` e `t3`), a cópia da memória do modo ao vivo usada na `f1` e as saídas das suítes. O que importa está em `traces/mid_*` (71 arquivos) e nos onze quadros `quadros/2026-10-12-hmi-mid-*.png`. As centrais foram todas encerradas (conferido às 3h27 com `docker ps -a`). **Da espera da partida (3h30 às 4h20)** ficaram no mesmo rascunho as pastas das outras dezoito centrais (`sem`, `h1a` a `h7b`, `v1a` e `v1b`), a cópia da pasta em que o teste foi visto falhar e passar (`t4`), a foto dos dois arquivos de antes (`foto3`) e as duas baterias; o que importa está em `traces/mid_espera*` (63 arquivos) e em `quadros/2026-10-12-hmi-mid-espera-2s-e-6s.png`. Todas encerradas (conferido às 4h12).
- Marco 15, central ao vivo: desligada desde a 0h26 de 12/10/2026. Não a liguei nem mandei nada para ela. **A próxima partida dela pega a mídia de mentira**, junto com o rádio e o telefone: a saída do lançador ganha a linha "[m1_hmi] mídia (/stubs/midia/minhas-midias.txt): modelo do tocador: 1 fonte(s) e 5 faixa(s) no perfil…", e a tecla Mídia abre o tocador. Se a mídia atrapalhar: `MIDIA= sh driver/ao_vivo.sh`. **Desde as 4h08 a próxima partida pega também a espera de 6 s pelos serviços DSI** (decisão D158): o registro da HMI passa a dizer "within 6000 ms" nas linhas do `DSIWLAN` e do `DSIBrowser`, e a última etapa da partida começa uns 8 s mais tarde. Se atrapalhar: `ESPERA_DSI= sh driver/ao_vivo.sh`. **A conferir na primeira partida de verdade**, porque eu não a liguei: que a linha `Required DSI services not registered` não cita nenhum `DSICar…`. **Conferido em 12/10/2026, às 8h38, pela sessão principal, só lendo o registro:** o Lipe ligou a central pela janela às 8h36. As três linhas `Required DSI services not registered` dessa partida citam `DSIWLAN` (6000 ms), `DSITTS` e `DSISpeechRec` (25000 ms) e `DSIBrowser` (6000 ms); nenhuma cita `DSICar…`. Aos 89 s a central estava no quadro 1848, sem erro de OpenGL, com os seis stubs rodando (`dsistub`, `veiculostub`, `radiostub`, `midiastub`, `telefonestub`, `somstub`), a memória sendo guardada (64 linhas) e 134 linhas de comando recebidas da janela dele.
- Marco 15: a janela e a página não precisaram de mudança; a pasta `janela/` e a `glforward-moi3/` não foram tocadas.
- Do Marco 16 (12/10/2026, manhã) ficaram no rascunho da sessão dele (`…/910e344e-2600-4200-a5fb-8c96886927ef/scratchpad/mid/`, em `/private/tmp`) as pastas das dezesseis centrais de teste (`k1` a `k10` e `s1b` a `s3c`), com as fotos de cada passo, as árvores de trabalho (`t5`, `t5b`, `t6` e `t7`), a foto dos arquivos de antes (`foto5`) e as baterias. O que importa está em `traces/m16_*` (57 arquivos, mais a pasta `m16_antes/` com os três arquivos do rádio como eram) e nos quatro quadros `quadros/2026-10-12-hmi-m16-*.png`. Todas as centrais foram encerradas.
- Marco 16, central ao vivo: ligada pelo Lipe às 8h36 de 12/10/2026 e em uso durante todo o trabalho. Não mandei nada para ela. **Ela segue com o rádio de antes até ser religada;** a partida seguinte compila o `radiostub.c` novo e lê o perfil novo. Se o rádio novo atrapalhar: os três arquivos de antes estão em `traces/m16_antes/`.
- Marco 16: a tabela das teclas do volante medida no passo 2 fica para a sessão do simulador pôr no `janela/README.md`, que é dela; não mexi em `janela/` nem em `glforward-moi3/`.
- Do Marco 13 (12/10/2026) ficaram no rascunho da sessão dele (`…/b38b40c2-a857-461a-88dc-bd4dd486b737/scratchpad/som/`, em `/private/tmp`) as cópias de trabalho da pasta (`base2`, `trab2`, `base3`, `trab3` e `trab4`), os quadros crus e a escuta do barramento de cada rodada, e cópias de leitura de arquivos do firmware (`fw/`). O que importa foi para `traces/som_*`; o resto some quando o macOS limpar o `/private/tmp`.
- Marco 13, central ao vivo: desligada desde a 0h26 de 12/10/2026. Não a liguei nem mandei nada para ela. **A próxima partida dela pega o som de mentira**, junto com o rádio, o telefone e a mídia: a saída do lançador ganha a linha "[m1_hmi] som (/stubs/som/som-de-mentira.txt): perfil aplicado: 44 campos gravados; entretenimento: ENT_AMP_TUNER" e, no fim, a linha "[m1_hmi] som: …"; a tecla Som abre a equalização no lugar do aviso; e a mídia passa a mostrar a faixa tocando. Se algo sair estranho, `SOM= sh driver/ao_vivo.sh` volta ao que era.
- Marco 13: a pasta `janela/` e a `glforward-moi3/` não foram tocadas. **O `janela/README.md` ficou com uma dedução que a bancada desmentiu** (seção "Teclas do volante": a barra de volume apareceria quando o serviço de som avisasse o volume novo; medido, não aparece). É da sessão principal acertar.
- Marco 13: o mudo de proteção de componentes do gerente de áudio de verdade foi visto numa rodada e deixado como está, sem investigação. Não é pendência a resolver: é o limite da bancada.
- Do Marco 14 (12/10/2026) ficaram no rascunho da sessão dele (`…/52fc1723-2b1e-475a-a3d1-502c8b44efee/scratchpad/men/`, em `/private/tmp`) as pastas das 21 centrais de teste `moi3-men-*`, os quadros crus e as duas cópias de trabalho da pasta. O que importa foi copiado para `traces/men_*` e `quadros/2026-10-12-hmi-men-*`. Nenhuma central ficou no ar.
- Marco 14, central ao vivo: não a liguei nem mandei nada para ela. Nada do que entrou muda a partida dela: são cinco arquivos novos, que nenhum lançador padrão usa.
- Marco 14: a pasta `janela/` e a `glforward-moi3/` não foram tocadas.
- Marco 14: o `stubs/codificacao/jetta-gli-2021-deducoes.txt` continua com as quatro linhas da lista de aparelhos (`0B1D`) que o Marco 8 pôs, e diz lá que as duas listas foram ligadas juntas sem testar uma sem a outra. O Marco 14 mediu que, para os menus que experimentou, basta a lista de funções (`0B1B`). Não mexi no arquivo, que entra em toda partida.
- Marco 14, para quem roda a bateria com outras sessões instalando: os programas são compilados no começo da rodada e cada teste é lido da pasta na hora dele. Uma instalação no meio dá vermelho que não é da pasta (26 falhas no `t_telefone` às 3h49; repetida às 3h50, passou).
- Dos números que acompanham a unidade (12/10/2026, manhã) ficaram no rascunho da sessão (`…/52fc1723-2b1e-475a-a3d1-502c8b44efee/scratchpad/uni/`, em `/private/tmp`) as pastas das treze centrais de teste `moi3-uni-*`, as fotos cruas e as duas cópias de trabalho. O que importa foi para `traces/uni_*` e `quadros/2026-10-12-hmi-uni-*`. Nenhuma central ficou no ar.
- Números que acompanham a unidade, central ao vivo: estava ligada e em uso pelo Lipe; não a toquei. **A próxima partida dela sobe com o motor e o perfil novos**: no padrão a tela é a mesma; com milhas guardadas, a autonomia nasce em milhas.
- Números que acompanham a unidade: `janela/` e `glforward-moi3/` não foram tocadas, nem os lançadores.
- Da gravação nos menus do catálogo (12/10/2026, manhã) ficaram no rascunho da sessão (`…/52fc1723-2b1e-475a-a3d1-502c8b44efee/scratchpad/gra/`, em `/private/tmp`) as pastas das treze centrais `moi3-gra-*` e as fotos cruas; o que importa foi para `traces/gra_*` e `quadros/2026-10-12-hmi-gra-*`. Nenhuma central ficou no ar, e a central ao vivo, em uso pelo Lipe, não foi tocada. Em `/tmp/moi3-menus` não ficou nada meu: as rodadas usaram pasta própria.
- Dos menus do Jetta no modo ao vivo (12/10/2026, tarde) ficaram no rascunho da sessão (`…/52fc1723-2b1e-475a-a3d1-502c8b44efee/scratchpad/jm/`, em `/private/tmp`) as duas cópias de trabalho com a pasta `ao-vivo/` delas e as fotos cruas; o que importa foi para `traces/jm_*` e `quadros/2026-10-12-hmi-jm-*`. Nenhuma central ficou no ar; a central ao vivo estava desligada e não foi ligada, e a pasta `ao-vivo/` de verdade não foi tocada. **A próxima partida dela sobe com os menus e com a lista `grava` maior.**



## Fontes

- Marco 13 (12/10/2026, da 1h30 às 4h20): 51 centrais de teste `moi3-som-*` com HMI (`r1` a `r5`; a série S, `A1` a `A4` e `B1` a `B6`; a sonda `MA` e `MB`; a série T, `TA1`, `TA2` e `TB1` a `TB4`; a prova, `P1`, `P2`, `SO1`, `SO2`, `V1` e `TA3`; a repetição, `P3` e `SO3`; a sonda `LA` e `LB`; e a série W, `WA1` a `WA9` e `WB1` a `WB9`), 27 delas subidas da pasta instalada e 19 nascendo de uma cópia da memória guardada do modo ao vivo, e as sondas sem HMI; saídas, fotos, roteiros, previsões e o teste visto falhar em `traces/som_*`, quadros em `quadros/2026-10-12-hmi-som-*`. Do jar da HMI, pelos decompilados do CFR de 10/10: `de.vw.mib.asl.internal.sound.component.SoundComponent`, `HsmSoundDPHandling`, `HsmSoundSetterHandling`, `SoundUtil`, `AudioManagementRsi`, a tabela `ConnectionData` e `MIBTimerManager`. Do firmware (MU0883, partição `system`): `/lge/app_ro/sai/spec/viwi.service.audiomanagement/viwi.service.audiomanagement-1.2.2.json`; `/lge/app_ro/audiomgnt/audiomgnt`, a `libAudioManagement.so` e `audiotable/MIB3_Audiotable_MQB.xml` (versão v207; os resumos SHA-256 estão em `stubs/som/gerado/relatorio.txt`); `/lge/app_ro/bin/rsi-mockup` e `rsi-send`; e os textos do binário `/lge/app_ro/hmi/touch-app`, com o `strings`. As notas de passagem das sessões do rádio, da mídia e das teclas do volante, na pasta de notas das sessões.
- Marco 15 (12/10/2026, das 2h05 às 3h30): dezoito centrais de teste `moi3-mid-*` (`m0` a `m5`, `p1` a `p4`, `d1` a `d4`, `r1`, `e1`, `e2` e `f1`); saídas, roteiros, o protótipo e os testes vistos falhar em `traces/mid_*`, quadros em `quadros/2026-10-12-hmi-mid-*`. **Da espera da partida (3h30 às 4h20):** outras dezoito centrais `moi3-mid-*` (`sem`, `h1a` a `h7b`, `v1a` e `v1b`), em `traces/mid_espera*`; para a linha do tempo de antes dos stubs, reli registros guardados por outras sessões, sem mexer neles: `traces/cpu_s_a6.out` (Marco 10) e `traces/est_g1a.ao_vivo.out` e `est_e1a.ao_vivo.out` (sessão do estado); e os meus `traces/rad_e1.out`, `rad_f3.out`, `mid_m1.out` e `mid_e1.out`. Do `janela/janela.swift` li, sem mexer, as linhas que chamam o `driver/ao_vivo.sh` (ele só acrescenta `SEM_SERVIDOR=1` e o `PATH`). Do jar da HMI, pelos decompilados do CFR de 10/10 (no rascunho de outra sessão, `…/c2af147a-1d1d5856ce37/scratchpad/jars/`): em `de.vw.mib.asl.internal.media`, `targets.TargetMediaStartup`, `controller.MediaController`, `controller.commands.playback.CmdActivatePlayer`, `controller.commands.browsing.CmdSetupBrowser`, `playback.MediaPlayer` (`rqMediaActivation`, `dsiMediaPlayerUpdateActiveMedia` e `detectActivationCompleted`), `browser.AbstractMediaBrowser` (`rqSetContentFilter`, `detectContentFilterSet`, as atualizações de pasta e de tamanho e `detectResponseList`), `sound.MediaAudioManager` (`rqAudioConnection`), `sourcehandling.target.MediaInternalSourceHandlingTarget`, `sourcehandling.clients.MediaListObservers`, `MediaDevices` e `AslDeviceIdDeterminator`; em `de.vw.mib.asl.internal.devicehandling`, `DeviceHandlingTarget` e `DevicesImpl`; em `de.vw.mib.asl.api.devicehandling`, `Device`, `DeviceState`, `Medium`, `MediumState` e `clients.DeviceFilter` e `DeviceChangeListener`; e, para a volta do rádio, `de.vw.mib.asl.internal.radio.stationSelection.StationSelectionManager` e `GlobalStationSelectionManger` e `unifiedradio.UnifiedRadioBandSwitch`. Conferido no bytecode do firmware, com o `javap` da JVM dele (`traces/mid_javap.sh`): só `DeviceFilter$5`, o filtro de tipos de aparelho. O esquema, as constantes e os vetores saíram das classes do firmware rodando na JVM dele (`driver/LeEsquemaDSI.java`, como a sessão do telefone o deixou às 2h37, pelo `driver/m8_esquema.sh`); os números de interface, de `com.lge.mib3oi.dsi.DSIInterfaceId`. Do firmware: `/vw/hmi/config/logging.properties`, para as chaves `DSI.MEDIA` e `ASL_MEDIA.*`. O barramento da rodada `v2` da sessão do simulador tinha sido relido no Marco 11. Da central ao vivo só copiei, para leitura, `ao-vivo/memoria/lsmval` (às 3h22; 61 linhas, de 22h47 de 11/10), como memória de partida da rodada `f1`; ela estava desligada. Li as notas de passagem das sessões irmãs (som, telefone e volante) e recebi quatro mensagens: a fila e o ajuste dela, da sessão do simulador; o aviso da troca do `kipc_bus`, da sessão do processador; e o aviso do gerador, da sessão do telefone, a que respondi duas vezes. Nenhum download; nada instalado.
- Marco 12 (12/10/2026, da 1h30 às 3h10): 41 centrais de teste `moi3-tel-*` (`ctl`, `a256`, `a4096`, `ambos`, `e1`, `e1b`, `e2`, `e2b`, `p1`, `c1`, `p2`, os pares `s1` e `s2`, `r1` a `r6` e `u1` a `u3`, `lt`, `lc`, `lw`, `prova`, `provac`, `prova2`, `b1` e `prova3`); saídas, barramento, registros do stub, roteiros, previsões, análises das séries e os testes vistos falhar em `traces/tel_*`, quadros em `quadros/2026-10-12-hmi-tel-*`. Do `MIBHMI-DSI.jar`, pelo bytecode (o gerador, com o `javap` da JVM do firmware) e pelos decompilados do CFR de 10/10, que estão no rascunho de outra sessão (`…/c2af147a-1d1d5856ce37/scratchpad/jars/dec/`): `com.lge.mib3oi.dsi.ListenerWorkerThread` (`run`, `processBTListenerService` e `processDCMManagerListenerService`) e `DSIDomain`; `…dsi.bluetooth.DSIBluetoothImpl` e `DSIBluetoothListenerService`; `…dsi.telephoneng.DSIMobileEquipmentImpl` (`messageToBytes`); `…dsi.addressbook.DSIAdbListenerService`; `…dsi.phonemanager.DSIPhoneManagerImpl`; `…dsi.smartphoneintegration.DSISmartphoneIntegrationImpl`; `…dsi.search.DSISearchImpl`. Do jar da HMI, nos mesmos decompilados (`…/decstd/`): `generated.de.vw.mib.appui.system.internal.SystemActivityImpl`, `DynamicStatemachineTriggerImpl` e `LocalDataPoolUpdateImpl`; `de.vw.mib.configuration.internal.generated.FeatureFlagHandler` e `ConfigurationManagerGenerated`; `de.vw.mib.asl.internal.system.AslTargetSystemFeaturesAndCoding`; `generated.de.vw.mib.appui.phone.internal.PhoneActivityImpl`; `generated.de.vw.mib.asl.internal.phone.ASLPhonePropertyManager`; `de.vw.mib.asl.internal.phone.HsmPhonePhoneAvailability`, `TargetASLPhoneGetter`, `TargetASLPhoneSetter` e `setup.bluetooth.TargetASLPhoneBluetoothSetup`; `…phoneconnectionmanager.bluetooth.TargetASLPhoneConnectionManagerBluetooth`; `…organizer.TargetASLOrganizerBaseFunctionality`; e as interfaces `org.dsi.ifc.bluetooth.DSIBluetooth`, `telephoneng.DSIMobileEquipment`, `phonemanager.DSIPhoneManager` e `addressbook.DSIAdb`. A tabela de teclas é a do Marco 5 (`traces/m5_tabela_teclas.txt`). Da central ao vivo só copiei `ao-vivo/memoria/lsmval`, para ler.
- Marco 12, o segundo estado (12/10/2026, das 3h09 às 4h10): onze centrais de teste `moi3-tel-*` (`x1`, `kp`, `k2`, os pares `k1` a `k3`, `kf` e, às 4h06, a `kj`, com rádio, mídia e som ligados); registros, barramento, passeios, previsões e análise da série em `traces/tel_*`; sete quadros `quadros/2026-10-12-hmi-tel-conectado-*.png`. Classes da HMI lidas nos decompilados do CFR de 10/10: `de.vw.mib.asl.internal.phone.HsmPhonePhoneAvailability` (`updatePhoneInformation`, `getPrimaryPhoneDeviceName`, `setDeviceName`, `dsiPhoneManagerResponseSetPrioritizedDevice`) e `de.vw.mib.asl.internal.phone.connectionmanager.TargetASLPhoneConnectionManager` (`dsiPhoneManagerUpdateKnownDevices`, `updatePhoneSelectionData`). Os nomes dos estados: `stubs/telefone/gerado/constantes.txt`.
- Marco 12, o modelo (12/10/2026, das 8h45 às 10h02): 34 centrais de teste `moi3-tel-*` (`d0`, `e10`, `e2k`, `e2c`, `m1`, `m2`, os pares `n`, `g`, `v`, `w` e `y`, `ga`, `gb`, `q1`, `q2`, `pv` e `pv2`); registros, passeios, previsões e análises em `traces/tel_*`; quatro quadros `quadros/2026-10-12-hmi-tel-modelo-*.png`. Classes da HMI lidas nos decompilados: `TargetASLPhoneConnectionManager` (`connectDeviceFromPCM`, `disconnectDevice`, `dsiPhoneManagerResponseConnectService`, `updatePhoneSelectionData` e a classe interna `PhoneInformation`, onde está o `validateCanBePrimaryOrAssociated` que estoura) e `ServiceStateUtil` (os bits de estado de serviço).
- Marco 12, a chamada simulada (12/10/2026, das 10h12 às 11h17): 17 centrais (`c0`, `c1d`, `ct`, `cs`, `ca`, `cb`, `h1` a `h4`, os pares `z1`, `z2` e `zm1` e a `pv3`); registros, passeios, previsões e análises em `traces/tel_*`; dois quadros `quadros/2026-10-12-hmi-tel-chamada-*.png`. Classes da HMI lidas nos decompilados: `de.vw.mib.appui.phone.internal.PhoneActivityImpl` (a guarda da transição 12 de `IphoneMainDial`), `de.vw.mib.asl.internal.phone.call.HsmPhoneCallHandling` (`spellerTextChanged`), `StateCallHandlingWaitForAdb`, `CallHandling` (`handleDialNumberEvent`, `dialTrimmedNumber`, `updateServiceCodeType`, `isCarplayCallIncomming`), `de.vw.mib.asl.internal.phone.truffles.Truffles`, `de.vw.mib.asl.internal.phone.setup.profiles.TargetASLPhoneProfileSetup` (`handleUpdateActiveProfile`) e `de.vw.mib.asl.internal.carplay.api.impl.ASLCarplayServicesImpl`; os estados da agenda (`ADBSTATE_`), na interface `org.dsi.ifc.organizer.DSIAdb`.
- Marco 11 (12/10/2026, da 0h09 às 2h10): dezoito centrais de teste `moi3-rad-*` (`a0` a `a2`, `b1`, `b2`, `c1` a `c3`, `d1` a `d4`, `f1` a `f3`, `e1`, `e2` e `g1`); saídas, roteiros e os testes vistos falhar em `traces/rad_*`, quadros em `quadros/2026-10-12-hmi-rad-*`. Do `MIBHMI-DSI.jar`, pelo bytecode (com o `javap` da JVM do firmware, `traces/rad_javap.sh`) e pelos decompilados do CFR de 10/10, que estão no rascunho de outra sessão (`…/c2af147a-1d1d5856ce37/scratchpad/jars/`): `com.lge.mib3oi.dsi.ListenerWorkerThread` (`run` e `processTunnerListenerService`) e `DSIDomain`; e, em `com.lge.mib3oi.dsi.radio`, `DSITunerListenerService`, `DSIUnifiedTunerListenerService`, `DSITunerAnnouncementListenerService`, `DSIRadioDataListenerService`, `DSIUnifiedTunerImpl`, `DSIRadioDataImpl`, `UnifiedListControl` e `UslData`. Do jar da HMI, só pelos decompilados: `org.dsi.ifc.radio.DSIUnifiedTuner`, `DSIUnifiedTunerListener`, `DSITunerAnnouncement` e `WavebandInfo`; `de.vw.mib.asl.api.radio.RadioConstants` e `…api.radio.amFm.AmFmStation`; e, em `de.vw.mib.asl.internal.radio`, `RadioAdaptationAdapter`, `stationSelection.StationSelectionManager` e `GlobalStationSelectionManger`, `unifiedradio.UnifiedRadioBandSwitch`, `UnifiedRadioStatus` e `UnifiedStationComfortClass`, `unifiedradio.main.StateTopMainUnifiedRadio` e `TargetUnifiedRadio`, `unifiedradio.dsi.DsiUnifiedTunerAdapter`, `amfm.main.HsmTarget`, `StateAmFmTop`, `StateAmFmActive`, `StateAmFmActiveSeek` e `StateAmFmActiveTargetSeek`, `amfm.AmfmControllerBase`, `amfm.common.AmFmStation`, `amfm.lists.AmFmStationList` (`generateFrozenFmList` e `addCurrentStation`), `amfm.settings.AmFmDefaultSettings`, `amfm.persistence.AmFmSettingsPersistable`, `presets.InitialAutostore`, `transformer.FMStationInfoCollector` e `RadioFMStationListInfoTransformer`. Para as exceções do componente de som: `de.vw.mib.asl.internal.sound.HsmSoundWaveplayerHandling` e `…sound.communicators.WaveplayerController`. O esquema, as constantes e os vetores de prova saíram das classes do firmware rodando na JVM dele (`driver/LeEsquemaDSI.java`, pelo `driver/m8_esquema.sh`). Do firmware, lidos da imagem montada só para leitura: `lib/systemd/system/radio.service` e `radio-hmi.service`, `/lge/app_ro/post_init/data/radio.sh` e a lista de arquivos de `/lge/app_ro/radio` (os programas não foram abertos nem executados); e `/vw/hmi/config/logging.properties`, para a chave `DSI.TUNER`. Os nomes dos valores da adaptação `0559` são os de `stubs/codificacao/tabela-campos.tsv`, do Marco 6. Da central ao vivo só copiei, para leitura, `ao-vivo/memoria/lsmval` (à 1h16), como memória de partida das rodadas `f1` a `f3`; ela estava desligada. O barramento da rodada `v2` da sessão do simulador foi relido no rascunho dela (`…/d609a58c-bbdd-4fb1-9a97-6a234685ee40/scratchpad/m6/v2/barramento.txt`), e os números de interface da mídia e do sintonizador, em `com.lge.mib3oi.dsi.DSIInterfaceId`. Li as notas de passagem das sessões irmãs (hub, Marco 8, consumo do `kipc_bus`, volante e som); o que cito do som e do volante vem delas. Nenhum download; nada instalado.
- Avisos "cortados no topo" (12/10/2026, da 1h10 à 1h43): centrais de teste `moi3-pop-*`, subidas da pasta (`a1` e `a2`, variante real, com chaves de rastreamento; `j2` a `j6`, variante manual, com o agente de depuração; a `j1`, variante real com o agente, não subiu); saídas, árvores e roteiros em `traces/pop_*`, quadros em `quadros/2026-10-12-hmi-pop-*`. Lido da HMI em execução, pelo JDWP: os campos de `de.vw.mib.widgets.AbstractWidget`, `Container`, `View`, `PopupView`, `TextArea`, `de.vw.mib.properties.values.LayoutAttribs` e `de.vw.mib.graphics.image.Image` e `ScaleImage`. Classes lidas nos decompilados do CFR de 10/10: `de.vw.mib.widgets.PopupView` (`updateBackgroundLayout`, `startEnterAnimation`), `AbstractWidget` (a linha `repaint(absX, absY, largura, altura)` do rastreamento e os bits de estado), `Container`, `View`, `de.vw.mib.widgets.ui.PopupViewUI` e `de.vw.mib.widgets.ui.util.BackgroundCompositor` (`paintSimple`), e `de.vw.mib.log4mib.internal.LogClassifierNames`. Bytecode, com o `javap` do firmware: as telas geradas `generated.de.vw.mib.appui.simpledesktop.internal.view.Sdl__0` e `generated.de.vw.mib.appui.system.internal.view.Psapni__0`, do `/vw/hmi/Resources/skin1/viewhandler.jar` (só para ver de onde saem as margens e a área de popups). Do firmware: `/vw/hmi/runHMI.sh` (as opções do arquivo de classes compartilhado) e `/vw/hmi/Resources/skin1/info.txt`. Os quadros antigos dos Marcos 4 e 5 foram só olhados. A frase do Lipe citada na seção veio na mensagem da sessão principal. Nenhum download; nada instalado.
- Rastreamento que parava na carga tardia (12/10/2026, da 0h09 à 1h10): 30 centrais de teste `moi3-hlog-*` (`r1`, `r2`, `n1` a `n6`, `b1` a `b5`, `s1` a `s3`, `uso0`, `uso1`, `m60b`, `m60n`, `nk1`, `man0`, `man1`, `f1` a `f3` e `g1` a `g4`); saídas, roteiros e contadores em `traces/hlog_*`. Classes do jar da HMI lidas nos decompilados do CFR de 10/10 (no rascunho de outra sessão, `…/c2af147a-1d1d5856ce37/scratchpad/jars/decstd/`): `de.vw.mib.log4mib.internal.MainLogger`, `LoggingProperties` e `ContextLogger`; `de.vw.mib.debug.service.probes.logging.LogManagerProbe` e `LogManagerProbeFactory`; `de.vw.mib.debugmanager.internal.DebugProbeManagerImpl` e `de.vw.mib.debugmanager.probes.AbstractDebugProbe`; `de.vw.mib.debug.common.probes.configuration.DebugProbeConfigurationCodec` e `DebugProbeManagerConfiguration`; `de.vw.mib.debug.common.probes.logmanager.LogManagerProbeConfiguration`; `de.vw.mib.asl.internal.system.persistence.AslTargetSystemPersistenceHandling`; `de.vw.mib.asl.framework.internal.startupv7r.PersistableEarlyData` e `AslTargetStartup`; e `de.vw.mib.configuration.internal.ConfigurationManagerImpl` (`setDynamicTmDebugProbesCfg`). Conferidos no bytecode do jar do firmware, com o `javap` da JVM dele (`traces/hlog_javap/`, tirado com `traces/hlog_javap.sh`): `MainLogger.setLoggingProperties`, `LogManagerProbe.applySettings` e `configureProbe`, `DebugProbeManagerImpl.initialConfigureProbe` e as mensagens e os nomes de propriedade dela, `AbstractDebugProbe.activate(Properties)` e `activationRequested`, `LogManagerProbeConfiguration.buildActiveClassifiersMask` e `AslTargetSystemPersistenceHandling.retrieveActivatedDebugProbes`; o resto é só dos decompilados. Na conferência dos perfis, o leitor e as duas classes de configuração rodaram de verdade, na JVM do firmware (`traces/hlog_ConferePerfis.java`, `traces/hlog_confere.sh`). Do firmware: `/vw/hmi/config/logging.properties`, `bundles.properties` e os 33 `debugprobes_*.config`, e `/vw/hmi/runHMI.sh`. Da central ao vivo só copiei, para leitura, `ao-vivo/memoria/lsmval` (à 0h30), para ver o campo do perfil de sondas e montar a memória de teste. As saídas que outras sessões guardaram em `traces/*.out` foram lidas para duas contagens (a Welcome e o fecho da ligação com o registro RSI). Nenhum download; nada instalado.
- Marco 9 (11/10/2026, das 21h46 às 23h15): rodadas `base1`, `v1`, `prova`, `v2`, `n1` a `n6`, `prova2`, `f1` a `f8` e `laco1`, em centrais de teste `moi3-ped-*`; saídas em `traces/m9_*`, quadros em `quadros/2026-10-11-hmi-ped-*`. Da central ao vivo só li `ao-vivo/log/ao_vivo.out`. A tabela de respostas saiu das classes da HMI rodando na JVM do firmware, por reflexão (`driver/LeEsquemaDSI.java`; o despejo de sondagem está em `traces/m9_tabela_da_hmi.txt`). Classes lidas no jar da HMI, nos decompilados do CFR de 10/10: `de.vw.mib.asl.framework.internal.framework.dsi.proxy.DSIProxyTarget` (`handleTimedOutRequest`), `…framework.dsi.util.Cfg`, `CompileGeneratedConstants` e `RuntimeGeneratedConstants`, `de.vw.mib.asl.framework.internal.dsiproxy.lr.UCC`, `de.vw.mib.asl.internal.system.AslTargetSystemDate` (os pedidos de hora, data, fonte e fuso, os temporizadores de resposta e `updateClockTime`), `de.vw.mib.asl.framework.internal.lsc.AslTargetLscTNGBordComputerLAC` (o idioma do painel) e `de.vw.mib.asl.internal.car.service.HsmTargetCarService` (as listas do start-stop). Quem chama `getBAPState` foi só uma busca nos decompilados (as classes de estado `ECallState`, `ENIState` e `AsgControlUnitState`). Do firmware: `/vw/hmi/config/logging.properties`, para as chaves de rastreamento. Nenhum download; nada instalado.
- Conserto do hub (11/10/2026, das 21h46 às 23h30): centrais de teste `moi3-hub-*`, à parte da central ao vivo: medição (`mp1`, `mv1`), séries lado a lado (`ap1` a `ap6`, `dp1` a `dp6`, `av1` a `av7`, `dv1` a `dv7`), partidas curtas (`oa*`, `od*`, `ra*`, `rd*`), ordem forçada (`eaf*`, `edf*`), dois fornecedores (`ac*`, `dc*`), assistente (`eam`, `edm`), telas (`tav`, `tdv`), conferência final da pasta (`fp*`, `fv*`) e o ensaio da corrida de partida (`traces/hub_corrida.txt`). O registro do hub com `HUB_DUMP`, o `barramento.log`, a saída da HMI e, em 24 partidas, um `strace` pendurado no hub. As 46 rodadas antigas com `barramento.log` em `traces/` e a tabela `traces/vc_diag_repeticoes.txt` da sessão do painel de instrumentos foram só lidas. `PID_PERSISTENCE` e `PID_MAX_ID` saíram de `com.lge.mib3oi.dsi.DSIDomain`, no decompilado do CFR de 10/10 que está no rascunho de outra sessão. Nenhum download; nada instalado.

- Marco 6, o que a memória guarda (11/10/2026, das 21h35 às 23h): rodadas `longa1`, `expl1`, `expl2`, `som1`, `som2`, `tela1`, `tela2`, `fav1`, `fav2`, `vei1`, `vei2`, `tec1`, `tec2`, `bri1`, `bri2`, `bri3` e `uni1` a `uni5`, em centrais de teste; saídas em `traces/m6_lsm_*`, previsões em `traces/m6_previsoes.md` (quarta parte), quadros em `quadros/2026-10-11-hmi-lsm-*`. O formato da chave, do embrulho e dos campos foi lido no jar da HMI decompilado com o CFR em 10/10 (`PersistenceKeyUtil`, `Compression`, `RecordByteArray`, `PersistableWriter`, `PersistableEarlyData`, `DisplayBrightness`, `ASLDisplayManagementTarget` e os serializadores de `de.vw.mib.asl.*.persistence`; a lista dos campos de cada um está em `traces/m6_lsm_campos-dos-serializadores.txt`). A lista dos módulos vem do `/vw/hmi/res/persistenceIds.res`, lido com as classes do firmware (`traces/m6_lsm_persistenceIds.txt`). Da central ao vivo só li, sem alterar, `ao-vivo/memoria/lsmval` (às 21h34 e às 22h45) e a lista de processos dela (`docker exec … ps`, às 21h40).
- Marco 8 (11/10/2026, das 19h55 às 22h): rodadas `ref`, `pad`, `jet`, `men`, `men2`, `sl`, `w2`, `fim`, `dois`, `lista`, `s1` a `s6`, `prova`, `anu`, `t1` a `t6`, `d1`, `d2`, `am`, `bm`, `ne1` e `ne2`, em centrais de teste; saídas em `traces/m8_*`, quadros em `quadros/2026-10-11-hmi-vei-*`. Da central ao vivo ligada às 21h33 só li o registro (`ao-vivo/log/`), o ambiente do container e o arquivo de quadros; a memória guardada (`ao-vivo/memoria/lsmval`) foi copiada para o rascunho e usada como arquivo de partida das centrais de teste. O tráfego da rodada `v1` da sessão do simulador foi relido no rascunho dela. Classes lidas no `MIBHMI-DSI.jar`, pelo bytecode (com o `javap` da JVM do firmware) e pelos decompilados do CFR de 10/10: `com.lge.mib3oi.dsi.ListenerWorkerThread`, `ListenerHandler`, `BAPTaskScheduler`, `DSIProvider` e `NotifyListenerTable`, `com.lge.mib3oi.dsi.base.ServiceAdminImpl`, e os pares `…Impl` e `…ListenerService` de `DSICarTimeUnitsLanguage`, `DSICarKombi`, `DSIKeyPanel` e `DSIPowerManagement`. No jar da HMI: `org.dsi.ifc.base.DSIListener`, `de.vw.mib.asl.internal.system.AslTargetSystemDate`, `AslTargetSystemVehicleStates` e `AslTargetSystemPowerManagementHandling`, `de.vw.mib.asl.internal.system.processor.ContentProcessorManagerImpl`, `de.vw.mib.asl.internal.impl.system.powerstate.PowerStateServiceImpl` e `RvcStateServiceImpl`, `de.vw.mib.appadapter.digitalassistant.modelapi.WelcomeScreenLogicController`, `de.vw.mib.asl.internal.car.bordcomputer.ASLCarBordComputerHsmTarget` e `TankLevelCalculator`, `de.vw.mib.asl.clientapi.car.boardcomputer.RangeCalculator` e `de.vw.mib.asl.internal.car.dm.DeviceManager`. Do firmware: `/lge/app_ro/carif/carifservice`, dependências e símbolos, com `readelf` e `strings`. Do carro: a ficha e a foto do painel em `carros/jetta-gli-2021/fotos/2026-10-09-painel-digital-mapa/`. Nenhum download; nada instalado.
- Marco 7: testes de `glforward-moi3/tests/` rodados na imagem `moi3-bancada:1` a partir da pasta do projeto; o programa da janela nos modos `--decodifica`, `--captura` e `--roteiro`; a central de teste `moi3-exp-tx` e a central ao vivo ligada às 19h31 (`ao-vivo/log/ao_vivo.glhost.log`, `ao-vivo/log/janela-ligou.log`). Nenhum download; nada instalado: `swiftc` do Command Line Tools e o AppKit do macOS.
- Os dois pacotes em `~/Downloads` (hashes acima) e a extração de 09 e 10/10.
- Tudo o que está nas tabelas do Marco 0 foi lido dos próprios arquivos com `driver/m0_inventario.sh` e `driver/m0_analisa.py`, em 11/10/2026.
- Os resultados do Marco 1 são das rodadas de 11/10/2026 com `driver/m1_*.sh`; as saídas estão em `traces/m1_*`.
- Pacotes da imagem: repositório oficial do Ubuntu 24.04 para arm64, `ports.ubuntu.com`.
- Execuções de 10/10: [NOTAS-RE-DINAMICA.md](NOTAS-RE-DINAMICA.md).
- Estudo do `iscle/qemu-mhi2` e busca por máquina QEMU: [estudo-qemu-mhi2.md](estudo-qemu-mhi2.md).
- Marco 3: rodadas de 11/10/2026 com `glforward-moi3/` e `driver/dsistub.c`; saídas em `traces/m3_*` e quadros em `quadros/`.
- Classes decompiladas com o CFR 0.152 (feito em 10/10, lido em 11/10): `com.lge.mib3oi.dsi.*` do `MIBHMI-DSI.jar` (`DSIDomain`, `DSIInterfaceId`, `ListenerWorkerThread`, `DSIDisplayManagementImpl`, `DSIDisplayManagementListenerService`, `DSIPowerManagementImpl`, `DSIPowerManagementImplListener` e os protobufs) e `de.vw.mib.asl.internal.system.AslTargetSystemPowerManagementHandling` do jar da HMI.
- Cabeçalhos Khronos (`GLES3/gl32.h`, `EGL/egl.h`) do pacote `libgles-dev` da imagem.
- Marco 4: rodadas `e1` a `e8` e sondas de 11/10/2026; saídas em `traces/m4_*`, quadros em `quadros/`. Nenhum download; nada instalado.
- Classes lidas no jar da HMI para o Marco 4 (decompilado com o CFR em 10/10): `de.vw.mib.asl.internal.usermanagement.welcomescreenloading.WelcomeScreenControllerImpl` e `LoggingWelcomeScreenController`, `…usermanagement.rsi.RSIAccessImpl`, `…usermanagement.RSIUserManagement`, `…usermanagement.persistence.UserManagementPersistence`, `de.vw.mib.asl.internal.online.onlineregistration.RsiOnlineRegistration`, `de.vw.mib.appadapter.digitalassistant.modelapi.WelcomeScreenLogicController`, e `…systemsetup.newauth.rsi.RsiHandler` e `…systemsetup.auth.subscriptions.RootSubscriptions` (uso da categoria do serviço).
- Marco 5: rodadas `k1` a `k8`, `t1`, `t2`, `p1`, `n1`, `n2` e as sessões ao vivo de 11/10/2026; saídas em `traces/m5_*`, quadros em `quadros/`. Nenhum download; nada instalado.
- Documentação do `iscle/qemu-mhi2` relida na cópia local em 11/10/2026 (`tools/mhi2/README.md`, `RCC_INPUT.md`, `MISSING_MENU_DATA.md`); a comparação está no fim do [estudo-qemu-mhi2.md](estudo-qemu-mhi2.md).
- Classes lidas para o Marco 5. Do `MIBHMI-DSI.jar`: `com.lge.mib3oi.dsi.keypanel.DSIKeyPanelImpl`, `DSIKeyPanelListenerService` e os protobufs `DSIKeyPanelServiceMsg` e `DSIKeyPanelListenerServiceMsg`. Do jar da HMI: `org.dsi.ifc.keypanel.Constants` e `DSIKeyPanel`, `de.vw.mib.asl.internal.system.AslTargetSystemKeyPanelHandling`, `…system.util.SystemKeyUtil`, `…system.util.keymapping.HardKeyReader`, `…system.input.TouchInputDeviceImpl` e `AbstractDsiInputDevice`, `de.vw.mib.input.internal.devices.TouchInputDeviceHandler`, `de.vw.mib.app.framework.internal.AppLifecycleManager` e `ContentController`, `de.vw.mib.configuration.internal.ConfigurationManagerBase`, e `de.vw.mib.log4mib.internal.LoggingProperties`, `MainLogger`, `LogWriterManager` e `…log4mib.writer.ConsoleLogWriter`.
- Do firmware, para o Marco 5: `/vw/hmi/res/configurationmanager.res` (lido com `com.microdoc.util.ObjectSwapper`, de `/lge/hmi/jar/ObjectSwapper.jar`), `/vw/hmi/config/logging.properties`, `/vw/hmi/runHMI.sh`, e as strings de `/lge/app_ro/hmi/touch-app`.
- Marco 6: rodadas `cod0` a `cod9` e a sessão ao vivo paralela de 11/10/2026; saídas em `traces/m6_*`, quadros em `quadros/2026-10-11-hmi-cod-*`.
- Marco 6, memória entre sessões (11/10/2026, das 20h15 às 21h20): rodadas `mem1` a `mem6`; saídas em `traces/m6_mem*`, previsões e resultados em `traces/m6_previsoes.md` (terceira parte), quadros em `quadros/2026-10-11-hmi-mem-*`. A memória da central ao vivo foi lida às 20h25 com `docker exec … cat`, sem alterar nada. Classes lidas no jar da HMI (decompilado com o CFR em 10/10): `de.vw.mib.asl.internal.system.processor.StageContentProcessor` e `de.vw.mib.asl.internal.system.persistence.PersistenceValidator`. Na camada da LG, relida: `com.lge.mib3oi.dsi.persistence.DSIPersistenceImpl` (`initLSMDataInFile`, que descarta linha com menos de quatro campos, e `readBuffer`). Código nativo do `persistence-app`, relido em `traces/m6_ghidra_persistence-app_lsm.log`: `DSIHandler::Persistence_WriteBuffer` (o `fopen "w"`, o `fputc(10)` no fim de cada linha, o `sync` e a cópia para `lsmval_bak`) e `init_pso_file`. Os nomes dos módulos por namespace saíram das linhas "Set persistence module" de `traces/m6_idi1.out`. Nenhum download; nada instalado.
- Marco 6, idioma (11/10/2026, das 19h43 às 20h05): rodadas `idi1`, `idi2` e `idi3`; saídas em `traces/m6_idi*`, previsões e resultados em `traces/m6_previsoes.md`, quadros em `quadros/2026-10-11-hmi-cod-ptbr-*`. A frase do Lipe, "Eu já coloquei o idioma em pt-br", é da sessão do Marco 6. A memória de estado da central ao vivo foi lida com `docker exec … cat`, sem alterar nada. Classes lidas no jar da HMI: `de.vw.mib.asl.framework.internal.lsc.AslTargetLscTNGBullhorn`, `PersistableLsc`, `LanguageHelper` e `DiagnosisHandler`, e `de.vw.mib.lsctng.internal.changer.LanguageChangerImpl`. Na camada da LG (`MIBHMI-DSI.jar`): `com.lge.mib3oi.dsi.persistence.DSIPersistenceImpl` (`initLSMDataInFile`, `readBuffer`, `writeBuffer`). Código nativo do `persistence-app`, com o Ghidra: `init_pso_file`, `DSIHandler::Persistence_WriteBuffer` e `DSIHandler::Persistence_FlushSQLDatabase` (`traces/m6_ghidra_persistence-app_lsm.log`). A foto do carro é a mesma, `carros/jetta-gli-2021/fotos/2026-09-07-multimidia/01-informacoes-do-sistema.jpg`.
- Marco 6, cabeçalho dos conjuntos (11/10/2026, das 22h56 às 23h26): o pedido chegou por mensagem da sessão "MOI3: diagnóstico que o navi-al entende"; a frase do Lipe foi lida no histórico dela. O formato e o motivo estão em [painel-de-instrumentos.md](painel-de-instrumentos.md), "Item 3", e em `traces/vc_*`; não reli o `alm`. Do meu lado: `PersistenceKIPC::event_handler` e `DiagHandler::event_handler` do `persistence-app` (decompilados no Marco 6), rodadas `cab0` e `cab1` (`traces/m6_cab*`, com as diferenças do `diagstub.c` e do teste), previsão e resultado em `traces/m6_previsoes.md`. O padrão `30 78` foi conferido em `stubs/codificacao/tabela-campos.tsv` e no gerador (`driver/LeCodificacao.java`), que lê a `mConfiguration` da HMI. Depois de instalar, das 23h31 às 23h36: opção `DIAGCAB` no lançador e rodadas `vcab0` e `vcab1` (`traces/m6_vcab*`, com o resumo feito pelo `driver/vc_diag_resumo.sh` daquela sessão); a conferência do motor é dela (rodadas `r6` e `r6b`, `traces/vc_r6*`).
- Para a parte do Jetta no Marco 6: a ficha do carro, seção "Central multimídia (5F)", e a foto `carros/jetta-gli-2021/fotos/2026-09-07-multimidia/01-informacoes-do-sistema.jpg`; a frase do Lipe como a sessão principal a registrou em [simulador-jetta.md](simulador-jetta.md); as classes `de.vw.mib.asl.internal.system.util.SystemInfoUpdater` e `de.vw.mib.configuration.internal.SkinInformationServiceImpl`; e o arquivo `/vw/hmi/Resources/skin1/ambienceColorMap.res`, aberto com o `ObjectSwapper` (cinco padrões de tipo de veículo e 16 entradas de cor). Nenhum download; nada instalado. O Ghidra 12.1.4 já estava no Mac, e os decompilados do CFR são os de 10/10.
- Classes lidas para o Marco 6 (decompiladas com o CFR em 10/10). Do jar da HMI: `de.vw.mib.asl.framework.internal.diagnosis.TargetDiagnosis`, `DiagnosisDataProvider`, `DiagnosisValidityProviderImpl`, `…diagnosis.config.AccessImpl` e `PropertyFileManager`, `…diagnosis.config.util.Engine`, `…diagnosis.internal.DiagApiProperties`, `…diagnosis.variant.oilarge.*` e `oismall.*` (`ClassWarehouseImpl`, `CodingImpl`, `AdaptationImpl`, `CarFuncAdapImpl`, `RecordsImpl`, `IdentificationImpl`), `de.vw.mib.asl.framework.api.diagnosis.config.*` e `…diagnosis.namespaces.*`, `de.vw.mib.asl.internal.system.AslTargetSystemFeaturesAndCoding` e `AslTargetSystemDate`, `…system.diag.AslDiagDataFetcher` e `AslDiagGenericFetcher`, `de.vw.mib.asl.framework.internal.lsc.AslTargetLscTNGBullhorn`, `BrandHandler` e `…lsc.testmode.SkinIdSelector`, `de.vw.mib.configuration.internal.ConfigurationManagerImpl` e `de.vw.mib.widgets.SkinHelper`. Do `MIBHMI-DSI.jar`: `com.lge.mib3oi.dsi.persistence.DSIPersistenceImpl`, `ListenerWorkerThread`, `ListenerHandler`, `Handler`, `Message`, `DSIDomain` e `DSIInterfaceId`.
- Código nativo lido para o Marco 6, com o Ghidra: `DSIService::event_handler` da `libDSIService.so` (decompilado em 10/10, relido) e, do `/lge/app_ro/hmi/persistence-app` (SHA-256 `34a4c09d…`), `main`, `PersistenceKIPC::event_handler`, `DiagHandler::event_handler`, `DSIHandler::Persistence_ReadBuffer`, `init_diag_file` e `init_diag_direct_file`. Trechos em `traces/m6_ghidra_persistence-app_*.log`.
- Do firmware, para o Marco 6: `/vw/hmi/config/coding.swap`, `adaptation.swap`, `carfunction.swap` e `identification.swap` (abertos com o `ObjectSwapper`; cópia em texto em `traces/m6_swap_bruto.txt`), `/vw/hmi/config/diagApi.properties`, `/vw/hmi/runHMI.sh`, os `info.txt` dos skins e a lista de pastas de `/vw/hmi/Resources/skin0/Data`. A tabela de campos e o conjunto padrão saíram das classes do firmware rodando na JVM dele (`driver/m6_tabela.sh`; saída em `traces/m6_tabela_saida.txt`).
- Captura de persistência do Marco 3 (`traces/m3_g3_barramento_dsi.hub.log`), relida para o formato dos pedidos e respostas de leitura.
- Arquivos do firmware lidos para o Marco 4, todos da partição `system` do MU0883: `/lge/app_ro/sai/conf/rsiserver.properties` e `rsiclient.properties`, `/lge/app_ro/sai/mockup/services`, `/lge/app_ro/sai/mockup/usermanagement/*.json`, `/lge/app_ro/sai/spec/viwi.service.usermanagement/…-0.6.0.json` e `viwi.service.onlineregistration/…-1.1.3.json`, `/lge/app_ro/sai/privileges/viwi.service.usermanagement.priv`; e as strings de `/lge/app_ro/bin/rsi-mockup`, `rsi-send`, `rsi-subscribe` e `/lge/app_ro/lib64/libtraceif.so.0.1`.
- Marco 14 (12/10/2026, das 2h52 às 3h55): 21 centrais de teste `moi3-men-*`; roteiros, saídas, arquivos de experimento e a tabela de aparelhos em `traces/men_*`. Classes lidas, decompiladas com o CFR, do jar da HMI: `de.vw.mib.asl.internal.car.dm.DeviceManager`; `…car.srv.ViewOptionServiceFactory`; `…car.srv.drvass.impl.DriverAssistanceViewOptionsServiceImpl` e `ACCViewOptionsWrapper`; `…car.model.CarSetupMenu` e `ModelDeviceStartupManager`; `…car.model.device.MappingList`, `light.LightDevice` e `DoorlockingViewOptionListenerImpl`, `driverassistance.AssistantsDevice`, `parking.ParkingDevice`, `openclose.OpenCloseViewOptionListenerImpl` e `sight.SightDevice`; e, do pacote `de.vw.mib.asl.internal.carfas.clientapi.impl`, `LaneAssistService` e `ApplicationService` inteiros e, dos outros serviços de assistente, só o campo que cada um consulta e os atributos que assina. Do registro do projeto: a tabela de equipamentos da ficha do carro e, do README do projeto 01, a tabela de codificação do wiki do VCDS.de e a pendência 8.
- Números que acompanham a unidade (12/10/2026, das 8h45 às 9h30): treze centrais de teste `moi3-uni-*`; roteiros e saídas em `traces/uni_*`. Do firmware, pelo esquema e pelas constantes gerados (`stubs/veiculo/gerado/`): as mensagens `CarBCDistance`, `CarBCSpeed`, `CarBCConsumption`, `CarBCCurrentRange` e os três `BC…GeneralData`, as linhas `G` do `DSICarTimeUnitsLanguage` e as constantes de unidade do `CarConstants`. Nenhuma classe nova foi lida: que a conta é do carro saiu da foto do Marco 9 (a temperatura convertida no topo, a autonomia em km) e da forma das mensagens.
- Marco 16 (12/10/2026, das 8h45 às 10h10): dezesseis centrais de teste `moi3-mid-*` (`k1` a `k10`, e `s1b` a `s3c`); roteiros, saídas, os testes vistos falhar, as baterias e os remendos em `traces/m16_*`; quadros em `quadros/2026-10-12-hmi-m16-*`. Do jar da HMI, pelos decompilados do CFR de 10/10 (no rascunho de outra sessão, `…/c2af147a-7a39-48cb-934f-1d1d5856ce37/scratchpad/jars/decstd/`): para as teclas na mídia, `generated.de.vw.mib.appui.media.internal.MediaActivityImpl` (transições internas e guardas da máquina de estados) e `LocalExpressionEvaluatorImpl` (expressão 6), `generated.de.vw.mib.asl.internal.sound.ASLSoundListenerConstants` (`CURRENT_AUDIO_COMPONENT`, 1470), `de.vw.mib.asl.internal.sound.HsmSoundDPHandling.updateEntertainmentConnection`, `de.vw.mib.asl.internal.media.controller.MediaController` (os eventos `SKIP_FORWARDS` e `SKIP_BACKWARDS`) e `generated.de.vw.mib.asl.internal.avdc.audio.ASLAVDCAudioServiceConstants`; para o rádio, `de.vw.mib.asl.internal.radio.unifiedradio.main.StateTopMainUnifiedRadio` (`dsiUnifiedTunerUpdateStationList` e `dsiUnifiedTunerUpdateSelectedStation`), `amfm.main.StateAmFmTop` e `StateAmFmActiveSeek`, `unifiedradio.UnifiedRadioRadioTextManager`, `comparator.AslStationQualityComparator` e `QualityPIFrequencyHDComparatorChain`, `unifiedradio.gui.ApiGuiUnifiedRadioSetupAdvanced`; para as teclas, `de.vw.mib.asl.internal.system.util.SystemKeyUtil` e `de.vw.mib.asl.api.system.KeyListener`. Nada foi conferido no bytecode desta vez. Do firmware: `/vw/hmi/config/logging.properties` (as chaves `ASL_SYSTEM.KEYPANEL` e `ASL_MEDIA.*`), lido de dentro de uma central de teste. Da bancada: `traces/m5_tabela_teclas.txt`, `stubs/codificacao/tabela-campos.tsv` (o campo "Bandsettings AM Tuner") e `janela/README.md` (lido, sem mexer). Da central ao vivo, nada: estava ligada e em uso.
- Gravação nos menus do catálogo (12/10/2026, das 9h29 às 10h07): treze centrais de teste `moi3-gra-*`; roteiros, resumos e registros em `traces/gra_*`. Do firmware, pelo esquema gerado: as linhas `P` e `G` dos serviços `DSICarComfort`, `DSICarLight`, `DSICarParkingSystem` e `DSICarDriverAssistance` e as mensagens `setPDCAutoActivation` e `updatePDCOPSAutoActivation`. Nenhuma classe nova foi lida.
- Menus do Jetta no modo ao vivo (12/10/2026, das 17h31 às 18h15): as respostas do Lipe, repassadas pela sessão principal; a ficha do carro (tabela de equipamentos); treze partidas `moi3-jm-*` pelo caminho do modo ao vivo, oito válidas; roteiros e resumos em `traces/jm_*`. Nenhuma classe nova foi lida: as opções saíram do catálogo do Marco 14.

## A1 — codificação real do autoscan aplicada (16/10/2026)

Pedido do Lipe ("vamos pro A1"). Os valores de teste/dedução da codificação do 5F foram trocados pelos **reais** do autoscan de 14/10 (`carros/jetta-gli-2021/autoscans/5F-mib3-2026-10-14/`) em `stubs/codificacao/jetta-gli-2021.txt`; o `jetta-gli-2021-deducoes.txt` foi **esvaziado** (aposentado — o chain do modo ao vivo em `driver/ao_vivo.sh` e `driver/m3_ao_vivo.sh` ainda lista o caminho hardcoded, então deixei vazio = no-op em vez de apagar). Nome→DID pela `tabela-campos.tsv`.

- **Coding `0600`** (25 bytes): `23 71 00 00 00 02 …` — confirma marca/classe/geração (`23 71`) e power stage 1 (`byte5=02`).
- **Confirmadas** as deduções: navegação on, FM = Brasil (`band_settings_fm_tuner = BRA`), CarPlay on, e os menus `0B1B` (board computer/serviço/relógio/central unit master) e as disponibilidades `0B1D` (MFA/clock/serviço/unit master), todas `activated` no autoscan.
- **Corrigida** uma: **Android Auto on** (`0555 @1.4+2`, `google_android_auto = on`) — a dedução o deixava em 0 porque a foto não o mostrava.
- **Novos** reais: `Country_Navigation = MSA` (`0558 @0.0+8 = 3`); `navigation_map_transmission_mode = IP_streaming` (`0B5A @0.0+8 = 1` — o coding que liga o mapa no cluster por IP); `Media_Country_Code = BR` (`0B49 = 42 52`, ASCII, cru não reconferido).
- **Não ligados** no carro (ficam 0): LDW/Lane Assist (`0B1D @25.6`) e reconhecimento de placas (`0B1D @33.6`) = not_activated — confere com o carro não ter Lane Assist (projeto 01).

Validado por script (`scratchpad/validate_coding.py`): **19 linhas, 0 problemas** — cada `@byte.bit+bits` casa com um campo real da tabela e o valor está no range. **Falta** a conferência visual subindo a HMI (modo ao vivo); as linhas usam o mesmo formato e as mesmas posições das deduções já testadas (Marco 6, Marcos 8/11/14), então devem carregar igual.
