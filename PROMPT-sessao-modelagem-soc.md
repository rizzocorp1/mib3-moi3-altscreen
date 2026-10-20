# Prompt para a sessão dedicada — sandbox/emulador do MOI3 (modelagem do SoC)

> Gerado em 11/10/2026. Cole o bloco abaixo como **primeira mensagem** de uma sessão nova dedicada. Ele é auto‑contido: dá o alvo, o que já foi decidido, a ordem de trabalho (viabilidade primeiro) e os limites. Os detalhes técnicos estão nos docs citados — a sessão deve lê‑los antes de agir.

---

## Contexto

Quero montar um **sandbox/emulador de pesquisa** para o firmware **MOI3** do MIB3 **LG/Qualcomm** do meu **Jetta GLI 2021** — para **estudo e engenharia reversa**, não para deploy no carro. O objetivo imediato desta sessão é **modelar/rodar o SoC** o suficiente para a HMI do MOI3 subir num ambiente controlado no meu Mac (Apple Silicon).

Já fizemos todo o levantamento numa sessão anterior; **leia primeiro estes registros** na pasta `referencia/ferramentas/mib3-sandbox/` (são a fonte — não re‑descubra o que já está lá):

- `estudo-qemu-mhi2.md` — estudo a fundo do `iscle/qemu-mhi2` (QEMU full‑system que emula um MIB3 Harman/Tegra), a receita de **GL‑forward**, a **identificação do SoC** e a **busca no GitHub** (não existe máquina QEMU pronta pro MSM8996; blocos reaproveitáveis listados).
- `glforward-moi3/` — esboço dos **shims Adreno** (`gl_shim_moi3.c` + `README.md`): encaminhar cada chamada GLES2/EGL por socket Unix para um `glhost` Mesa surfaceless. Resolve a parte **gráfica**.
- `NOTAS-RE-DINAMICA.md` — RE dinâmica do MOI3 (onde penamos com ABI/glibc e libs GL falsas; onde o SoC aparece nos traces).

## Alvo (pinado, com fonte nos docs acima)

- **SoC:** Snapdragon **820A = MSM8996 / APQ8096AU** (automotivo). **CPU Kryo (aarch64)**. **GPU Adreno 530**, driver **kgsl**.
- **OS:** **AGL / Linux**, kernel da linha **4.4.115** (boot Qualcomm XBL/ABL no hardware).
- **Referência pública do SoC:** **DragonBoard 820c (DB820c)** — device tree **mainline** (`msm8996.dtsi` + `apq8096-db820c.dts`); kernel quase‑mainline do **postmarketOS `msm8996-mainline`**. A Adreno 530 tem driver aberto **freedreno/Turnip** no Mesa.
- **Firmware:** vem do **pacote de atualização MOI3** (train `MOI3_EU_VW_R8030L`, PN `3G5 035 832 C`) — extrair o SWUP e montar rootfs; **secure boot NÃO precisa ser satisfeito na emulação** (QEMU/container não impõem assinatura — e **não é para contornar assinatura**, ver limites abaixo).
- **Cluster:** vídeo do painel é **H.264 em multicast IP `ff14::2:4`** (porta 42800) — capturar/decodificar à parte, fora da pilha GL.

## O que já está decidido

1. **A peça central é fazer um Linux/aarch64 com cara de MSM8996 rodar a HMI** — não emular o SoC bit a bit. A GPU é resolvida por **shim (GL‑forward)** ou por **virtio‑gpu/Venus**, **não** por emular a Adreno.
2. **Não existe máquina QEMU pronta pro MSM8996** (confirmado na busca). Então o trabalho é **adaptar mainline**, não modelar do zero.
3. Há **dois caminhos** em aberto, e a sessão deve **decidir entre eles por viabilidade**, não por preferência:
   - **Caminho A — QEMU full‑system.** Bootar um kernel numa máquina QEMU e subir o userspace do MOI3 por rootfs.
   - **Caminho B — container‑direto.** Rodar o userspace/HMI num container aarch64, sem QEMU; a parte gráfica sai pelo GL‑forward (shims Adreno → `glhost` Mesa). O obstáculo conhecido aqui é a **parede de ABI/glibc** (o MOI3 é AGL, glibc específico) — já batemos nela antes.

## Ordem de trabalho (viabilidade primeiro — faça nesta ordem, pare e reporte a cada marco)

**Marco 0 — extrair o firmware.** Confirmar que dá pra abrir o pacote MOI3 e montar o rootfs (squashfs/ext4/UBI conforme o layout), listar a árvore da HMI e as libs Adreno que ela carrega (`libGLESv2_adreno`, `libEGL_adreno`, kgsl). Sem alterar os arquivos originais de pesquisa.

**Marco 1 — o teste barato que decide A×B.** Antes de qualquer machine model: tentar bootar um **kernel mainline aarch64** na máquina genérica **`qemu-system-aarch64 -M virt`** (CPU `cortex-a57`/`max`) com um rootfs mínimo, e ver **se o userspace do MOI3 roda numa plataforma `virt`** (sem devices MSM8996‑específicos). Se a HMI não exigir hardware Qualcomm‑específico, isso rebaixa "modelar o SoC" para "rodar na `virt`" — muito menos esforço. **Esse é o primeiro experimento.**

**Marco 2 — ramificar conforme o Marco 1:**
- Se a HMI **tolera `virt`** → seguir por aí: kernel mainline (base no `msm8996-mainline`/DB820c só para drivers que faltarem), rootfs do MOI3, GPU por **GL‑forward** (reusar `glforward-moi3/`) ou **virtio‑gpu + Venus/virglrenderer**.
- Se a HMI **exige devices MSM8996** (kgsl, clock/pinctrl/SMMU Qualcomm, etc.) → avaliar o custo de **device models no QEMU** usando o DT do DB820c como mapa, **ou** cair para o **Caminho B (container‑direto)** resolvendo a ABI/glibc com um sysroot AGL casado.

**Marco 3 — gráfico.** Pôr um frame da HMI na tela: completar os entry points dos shims Adreno (mecânico, copiar de `gl_shim.c` do fork, transporte por socket já trocado), adaptar o `glhost` para `accept()` num socket Unix, e mostrar o PPM num viewer. Cluster depois, pela captura do multicast.

A cada marco: **registre** o que funcionou, o que não funcionou e a fonte de cada decisão nos docs da pasta (regra do projeto). Diga sempre o que foi **verificado em execução** e o que é **dedução**.

## Limites (inegociáveis)

- **É emulação/análise, não deploy.** Nada aqui vai para o carro.
- **Não contornar, quebrar nem descrever como burlar** a assinatura de firmware, o dm‑verity ou o secure boot da VW/Qualcomm. O ponto é que **o ambiente emulado simplesmente não exige assinatura** (QEMU/container não verificam) — não que a gente derrote a verificação do hardware. Se a tarefa escorregar para "como assinar/burlar", **pare e me avise**.
- **Precisa do `.7z`/pacote oficial do firmware** que eu forneço; não baixar firmware de fonte duvidosa.
- Responder em **pt‑BR** e seguir as regras do `CLAUDE.md` do projeto (registro obrigatório, uma peça de cada vez nos arquivos).

## Entregável desta sessão

Uma **decisão fundamentada A×B** (com o resultado do Marco 1) e, no melhor caso, a **HMI do MOI3 subindo** num ambiente controlado com **pelo menos um frame renderizado**. Se travar antes, um relatório claro de **onde** travou e **qual** é o próximo experimento.
