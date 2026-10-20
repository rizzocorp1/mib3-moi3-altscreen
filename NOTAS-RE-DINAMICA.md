# Notas de RE dinâmica — harness mib3-sandbox

**Escopo:** estudo/sandbox num PC (binários próprios, sem carro). Não toca secure boot/FDS/dm-verity.

> **Correção de 11/10/2026 — ler antes das seções de "teto" e "parede" abaixo.** Em várias seções deste arquivo está escrito que um cockpit renderizado no PC "não é alcançável" e que pixel é "parede de hardware". **Isso se mostrou errado.** Em 11/10 a HMI renderizou a tela "Welcome" em 800×480 no Mac, com um encaminhador de OpenGL: um shim com os nomes das libs Adreno, ligado contra a glibc 2.23 do firmware, que manda as chamadas para um Mesa fora do processo. O registro está em [modelagem-soc.md](modelagem-soc.md), Marco 3, e o código em [glforward-moi3/](glforward-moi3/README.md). Outros pontos corrigidos lá: a GPU vai pelo DRM `msm` (o kernel não tem kgsl); a HMI pede OpenGL ES 2.0 com a linha de partida do firmware (o "GLES 31 required" não reapareceu; é provável que viesse da opção `-Dde.vw.mib.graphics.gles.v3` usada naqueles testes, mas isso não foi conferido); a `glGetString` nula não reapareceu, nem com o stub regenerado nem com o encaminhador; e a geometria de 1920×720 usada no `dmstub` não é de nenhuma tela desta central (a variante `OISmall` é 800×480). As seções abaixo ficam como estavam, para histórico.

## Fase 0 — bancada (FEITO, 10/10/2026)
- Pasta dedicada criada; OrbStack/Docker 29.4 de pé (linux/arm64 **nativo** — ELF aarch64 roda sem emular CPU).
- Alvo 1 = `sdcm` (`/lge/app_rw/smartlink/dcmanager/sdcm`): **ELF aarch64 PIE, interpreter `/lib64/ld-linux-aarch64.so.1`, GNU/Linux 3.14, GLIBC_2.17, stripped.** NEEDED: libdl/librt/libpthread, libglib/gio/gobject-2.0, libstdc++/libgcc_s/libc, + **VW: `libdcmdsi` (DSI), `libdcminterface`, `libdcmnotification`, `libusbservice`, `libtraceif.so.0.1`**.
- **Lição de método:** sysroot manual por `debugfs` NÃO funciona (symlinks multilib Yocto viram arquivo 0-byte). O certo é **loop-mount read-only do `system.img` DENTRO do container** (os symlinks resolvem nativo) + chroot. Overlay nested (overlay-sobre-overlay do Docker) **falhou** aqui ("wrong fs type") → a camada gravável tem que vir por **tmpfs nos paths específicos**, não overlay.

## Fase 1 — LEDGER do cold-start (FEITO — entregável garantido)
`sdcm` rodou nativo no container (chroot no fs do firmware) e **abortou no ponto esperado** (`double free or corruption` depois de falhar ao criar `/data/lge/` em fs read-only — porque o overlay não subiu). O que ele pede ao subir (empírico, não estático):

| O que pede | Syscall | É o quê |
|---|---|---|
| **`/dev/csr`** (O_RDONLY) + **`/dev/css`** (O_WRONLY) | openat → ENOENT | **os nós kipc** (read/write). O alvo do MVP. |
| `/tmp/trace_unix_socket` | connect AF_UNIX → ENOENT (retry) | socket do **libtraceif** (log/trace VW) |
| `/data/lge/dcm/` + `dcm.known_devices` + `dcm.version` | mkdir/openat O_CREAT | estado do dcmanager (precisa ser **gravável**) |
| `/data/persist/count` | faccessat → ENOENT | contador |
| `/var/lib/nav/coredump` | aberto OK (forka um `du`) | — |
| `/dev/shm/sem.dcm.semaphore`, `/dev/shm/shmActiveStateLog` | abriu OK | shm (já funciona) |
| `/dev/tty` | ENXIO | tty (ignorável) |

**Confirma a previsão do Muro A:** os nós kipc `/dev/csr`+`/dev/css` e o trace socket. O path do kipc NÃO era runtime-só — é fixo `/dev/csr`/`/dev/css`.

## Plano do Muro A (workflow w61zdzg1c) — resumo
- **Pista I (fazer bootar):** cascata estrita **kipc → DSI → navsd/VIWI+BAP**. kipc é a camada mais baixa; interposição por **LD_PRELOAD de open/openat/ioctl/close** (passam por PLT; `recv_pkt/send_pkt` internos são `bl` direto, NÃO interponíveis). MVP = **`sdcm` + 1 stub crítico (kipc shim)**: `open(/dev/csr|/dev/css)` → fd falso real (socketpair/eventfd); `ioctl(fd, req, &MsgPkt)` → se Write lê o MsgPkt e ecoa, se Read bloqueia até ter msg. Isso tira o `open()=-1`, mata o loop "Fail to send KIPC message" e faz o binário emitir seus MsgPkts de boot.
- **Pista II (independente do barramento — vitória barata ~1 semana):** harness de **biblioteca** — `dlopen` da `libMIBNavSDClientLib` + `facade.so` autogerado dos 369 símbolos U (no-op) + `LD_PRELOAD` de `getDiagValue` preenchendo `value.data[8]` com byte7=(bit<<1), e um host que chama o ctor `FctFsgSetup` + `nvmDataChanged` com a key `{source=0x1000000, channel=0xB5A}`. **Prova de ponta a ponta a descoberta do coding (byte7/bit1 = VideoNavigation) SEM o stack kipc.**

## Próximos passos (MVP sdcm)
1. **Camada gravável:** tmpfs em `/data`, `/persist`, `/tmp`, `/var/lib/nav` dentro do chroot (overlay não rolou).
2. **kipc stub:** criar `/dev/csr`+`/dev/css` como FIFO/socket que aceitam+logam (ou LD_PRELOAD de open/ioctl) + `/tmp/trace_unix_socket` com um listener que só aceita.
3. Pré-criar `/data/lge/dcm`, `/data/persist`. Re-rodar → ver o `sdcm` passar do open do kipc e começar a emitir tráfego de boot.
4. Em paralelo: Pista II (BAP lib harness) como vitória barata que confirma o byte 7/bit 1.

## Iterações 2–4 — kipc shim e o wire do MsgPkt (10/10/2026)

**Fase 0 corrigida:** overlay nested do Docker não monta → camada gravável por **tmpfs** em `/data /persist /tmp /run /dev` + `/dev/shm`; nós `/dev/null,zero,urandom` via `mknod`; `/proc` montado.

**kipc ioctl = request `0`** (confirmado empiricamente — `ioctl(fd, _IOC(_IOC_NONE,0,0,0)=0, &MsgPkt)`). Bate com a previsão do Muro A ("literal 0 no disasm").

**kipc_shim.so** (LD_PRELOAD, sem libc, syscalls inline aarch64 — compila com `-nostdlib`): intercepta `open/openat/ioctl/close`, faz `ioctl(req=0)` nos nós kipc retornar 0 e **loga o MsgPkt**. Com ele o `sdcm` passa do open+ioctl e **emite suas mensagens de boot**. Wire capturado:

- **MsgPkt A (boot, repetido no `/dev/css`):** `0c000000 .. .. .. 00680000 .. <ptr8B> .. .. 116a0000 ..`
  - word0 = `0x0c` (12 — tipo/id pequeno); word4 = **`0x6800`** (bufsize anunciado — bate com o plano); depois um **ponteiro** (buffer do payload); word10 = `0x6a11`.
- **MsgPkt B (registro de canal):** `01000000 .. 00002000 .. 11000000 2f637373(="/css") <ptr> ffff0000 ..`
  - type=1, size=`0x200000`, len=`0x11`, e o **nome do nó `/css`** embutido → é o binário **registrando seu endpoint por nome**.

**Onde trava:** o `sdcm` manda o MsgPkt mas, sem um broker que **responda** (o `/dev/csr` read devolve sucesso falso vazio), ele corrompe o heap (`double free`). **Lição:** NÃO zerar o buffer de leitura às cegas — 64 bytes **estoura a pilha** (`stack smashing`); o buffer do MsgPkt é menor.

**Próximo passo (o broker de verdade):** decompilar `send_pkt`/`recv_pkt`/`frame::Frame`/`CommunicationService::Open` pra obter o **struct MsgPkt** e a sequência de **handshake** (o que o binário espera de resposta). Só então o broker responde certo e o `sdcm` passa pro init do DSI. Esse é o trabalho profundo que os dois planos marcaram como "semanas, sem garantia".

## Protocolo kipc (struct MsgPkt) + mapa da Pista II (10/10/2026)

**Call-sites do ioctl do kipc** no `carplay`: 0x29b600, 0x29b798, 0x2a2d20 — todos `mov x1,#0x0` (req=0), x2 = &descritor, x0 = fd. **O MsgPkt é um DESCRITOR**, montado em pilha antes do ioctl (visto @0x2a2c40–0x2a2d20):
- `+0x00` (8B): flags/len (no boot do sdcm = 0x0c)
- `+0x08` (8B): valor
- `+0x10` (4B): **seq++** (contador incremental: `w23 = prev+1`)
- `+0x18` (8B): **ponteiro pro payload real** (a mensagem, ≤ `0x6800`)

Ou seja `ioctl(fd, 0, &descritor)`; o conteúdo está em `*(descritor+0x18)`. **Broker:** no write (css) ler o descritor→payload e rotear; no read (csr) escrever um descritor de resposta + preencher um payload. **Falta ainda:** o formato da RESPOSTA (o que o binário espera ler) — exige decompilar o handler de recv. É o trabalho que torna o broker "de semanas".

**Mapa da Pista II (libMIBNavSDClientLib_CarIF.so):**
- NEEDED: `libPoco{Foundation,Net,JSON}.so.45` + 6 `libMIB*_CarIF.so.00` (Mapping/Client/Bap/ServiceDB/Service/Util) — **todas no firmware** (extraíveis; evita compilar Poco).
- **369 símbolos U** (ServiceLocator, OEMInterface, MicroServiceInfo, Poco::NumberParser…) — a maioria resolve com as libs irmãs; o facade cobre só o resto.
- Alvos (dlsym-áveis, T): ctor `FctFsgSetup(INavSdInterface*)`@0x1417d0; `nvmDataChanged`@0x142578; `setBapEntity`@0x141350; getter `...VideoNavigationSupportedDf44Bit0`@0x1eff08; `NavSdDataHandler::getDiagValue`@0x1006f8.

**Muro honesto da Pista II (descoberto na execução):** (a) `getDiagValue` é **interno** à lib → não dá pra interpor por LD_PRELOAD; injetar o coding exige popular o datapool OU escrever direto no membro 0x19a do objeto construído. (b) O host custom (glibc 2.39 do container) **não casa** com a lib (glibc 2.17) no mesmo processo → precisa de **toolchain cruzado aarch64/glibc-2.17** pra compilar o host, OU rodar dentro do chroot com um gcc compatível. (c) `setBapEntity` faz uma **chamada virtual** (`this+0x100`→vtable+0x50) → o objeto tem que ser construído pelo ctor real (não dá pra forjar à mão). São esses 3 que fazem a Pista II ser "dias", não "horas".

## Estado consolidado (fim da sessão de execução)
**PRONTO e funcional:** bancada (OrbStack arm64 + loop-mount + tmpfs); `sdcm` **executa**; ledger da Fase 1; `kipc_shim.so` (open/ioctl interpostos) que faz os nós kipc responderem e **loga o MsgPkt**; struct do descritor MsgPkt documentado; mapa completo da Pista II.
**FALTA (multi-dia, sem garantia — como os planos previram):** o broker kipc com respostas corretas (handshake de recv) OU a Pista II com toolchain glibc-2.17 + ctor real. Qualquer um é uma empreitada dedicada.

## ✅ PISTA II CONCLUÍDA — prova dinâmica do coding do VideoNavigation (10/10/2026)

Rodando o **código real** da `libMIBNavSDClientLib_CarIF.so` num processo no sandbox (sem carro), provei de ponta a ponta:

```
switchMapSupported (membro 0x19a) = 1  →  VideoNavigation (FsgSetup Df44 Bit0) = 1
switchMapSupported (membro 0x19a) = 0  →  VideoNavigation (FsgSetup Df44 Bit0) = 0
```

Junto com a prova estática (instrução única que escreve 0x19a a partir de **byte 7 / bit 1 da adaptação `DASHBOARD_DISPLAY_CONFIGURATION`** em `nvmDataChanged`), a cadeia completa está confirmada **estática + dinamicamente**:
> **`DASHBOARD_DISPLAY_CONFIGURATION` byte7/bit1 → membro 0x19a → `setBapEntity` → `VideoNavigation` Df44 Bit0.**

**Como os 3 muros caíram:**
1. **ABI:** era falso alarme — **glibc é forward-compatible**; a 2.39 do container hospeda a lib 2.17. Basta copiar as libs do firmware (Poco + 15 `libMIB*` + wolfssl/sqlite… = ~969) num libdir SEM a família glibc/libstdc++ (resolve na do container). `dlopen` + `dlsym` funcionaram direto.
2. **ctor crashava** em `ServiceListener::addServiceToMonitor` (registro de serviço sem infra global) → é **cross-lib (PLT)** → **LD_PRELOAD** com um no-op (o nome mangled é identificador C válido). ctor completa e aloca o `Status` real em `obj+0x160`.
3. **`setBapEntity` crashava no fim** no push BAP virtual (`this+0x100`→vtable+0x50, sem IBapEntity real) — mas a **lógica dos bits roda ANTES**. Apontei `this+0x100` pra uma **vtable de no-ops** → o push vira no-op e o método completa.

**Artefatos:** `driver/host.c` (dlopen+ctor+setBapEntity+getter), `driver/preload_stubs.c` (no-op do addServiceToMonitor), `driver/run_pista2c.sh`. Reproduzível.

**O que isto significa pro carro:** confirma que o **mapa nativo no cluster** (VideoNavigation) é ligado pelo bit **byte7/bit1** dessa adaptação — o dado que você leva pro ODIS (falta só o rótulo humano do canal `0xB5A` no catálogo ODX). NÃO é o CarPlay (esse continua `Df47 Bit3` hardcoded false, provado estaticamente).

## Broker kipc — por que o sdcm não sobe só com LD_PRELOAD (10/10/2026)

Decompilei `kipc::CommunicationService::read` (`_ZN4kipc20CommunicationService4readERmRjPPc`): após o `ioctl(fd,0,desc)`, se `ret>=0` o binário lê **`desc+0x00`=tamanho recebido, `desc+0x10`=tipo, `desc+0x18`=ponteiro do payload** (o buffer de 0x6800, pré-zerado com `memset`). Para "sem mensagem" basta `desc+0x00=0` (NÃO zerar `+0x18` — vira crash). Corrigi o shim assim.

**Mas o `sdcm` continua dando double-free, e a RE mostra por quê — DOIS muros reais:**
1. **ioctl por syscall direto:** no trace, `ioctl(12,0,…)` aparece como **syscall real** (EBADF/ENOTTY), i.e. **não passa pela PLT** → o `LD_PRELOAD` do meu shim **não intercepta** esse caminho. Interceptá-lo exige **ptrace / seccomp-unotify** (um tracer que falsifica o ioctl) ou **patch no binário** (é cópia local de estudo, legítimo) — não dá por preload.
2. **Semântica do handshake:** mesmo interceptando tudo, "sem mensagem" não basta — o binário espera uma **resposta VÁLIDA** à sua registração/alive (ex.: confirmação do canal `/css`). Fabricar isso exige RE do protocolo DSI-sobre-kipc (o que cada msgid significa). É o "contrato em runtime" que o plano chamou de semanas.

**Conclusão honesta:** fazer o `sdcm` subir 100% precisa de (a) um interceptor de syscall por ptrace/seccomp (ou patch do binário) + (b) RE do handshake do protocolo. É um subprojeto dedicado de vários dias, com o caminho agora **precisamente mapeado** (não mais "hipótese"). O alvo do usuário — **provar o coding do VideoNavigation — foi atingido pela Pista II** (acima), que não depende disto.

## ✅ sdcm SOBE ALÉM DO KIPC — broker por ptrace (10/10/2026)

O muro #1 (ioctl por syscall direto, não-PLT) foi resolvido com um **broker por `ptrace`** (`driver/ptrace_broker.c`): intercepta TODO `ioctl` (não só os da PLT), e para `req==0` (kipc) **pula o syscall real** (via `NT_ARM_SYSTEM_CALL`=-1), devolve `0`, e zera `descritor+0x00`/`+0x10` (= "sem mensagem") com `process_vm_writev`. Roda o `sdcm` dentro do chroot `/mnt/low`.

**Resultado — o `sdcm` passou a subir e ficar VIVO e ESTÁVEL:**
- Rodou os timeouts inteiros (12–25s) **sem double-free, sem abort, sem segfault** (antes crashava em 1–2s).
- Depois de 14s: **estado S (vivo), 16 threads** em estados de serviço normais:
  - `gmain` em `ppoll` (loop de eventos **GLib** — o framework que a NEEDED mostrava), `pool` em `futex_wait` (**thread pool**),
  - threads em `do_select`, `epoll_pwait`, `ppoll` (loops de I/O), `nanosleep` (timers), `futex_wait` (locks/condvars).
- Ou seja: **daemon totalmente inicializado, em steady-state orientado a eventos.** Subiu muito além do kipc.

**Diagnóstico confirmado:** o double-free era do **caminho de erro do EBADF** — o ioctl kipc por syscall direto que o LD_PRELOAD não pegava retornava erro, e o binário corrompia o heap no cleanup. Interceptando por ptrace e devolvendo "sem mensagem" limpo, o erro some e o binário inicializa normal.

**Nota técnica:** o broker traça a thread principal; as workers são clones não-traçados (sem `PTRACE_O_TRACECLONE`) e rodam livres — mesmo assim estável, o que indica que o caminho que crashava era o da thread principal. Para um broker "de produção" (fazer o DSI handshake de verdade, não só "sem mensagem"), faltaria: `PTRACE_O_TRACECLONE` + responder mensagens válidas (RE do protocolo DSI). Mas o objetivo "**sdcm sobe além do kipc**" está **atingido e reproduzível** (`driver/run_ptrace2.sh`).

## RESUMO FINAL DA SESSÃO "manda bala"
1. **Bancada de RE dinâmica** funcional (OrbStack arm64 + loop-mount + tmpfs) — reutilizável. ✅
2. **`sdcm` da central EXECUTA** no sandbox. ✅
3. **Ledger** + **wire do kipc** + **struct do MsgPkt** + **protocolo de `CommunicationService::read`**. ✅
4. **PISTA II: prova dinâmica do coding do VideoNavigation** (byte7/bit1 → Df44Bit0), rodando o código real. ✅ ← alvo do Lipe
5. **`sdcm` sobe além do kipc** e roda estável com 16 threads (broker ptrace). ✅ ← cobrança do hook
6. **A HMI (`de.vw.mib`) sobe até a UI thread** no sandbox (SPI→autostart→managed bundles, ViewHandler instanciado, servidor RSI/VIWI vivo). ✅ Parede final = GPU Adreno (sem software-render) + barramento DSI (daemons `carifservice`/`persistence-app` + dados sintéticos) + cluster ser outra ECU. Cockpit renderizado no PC: **não alcançável**; camada Java da HMI: **roda de verdade**.

## ✅ A HMI (de.vw.mib) SOBE NO SANDBOX ATÉ A UI THREAD (10/10/2026)

Prova dinâmica de que a **própria HMI** (o framework `de.vw.mib`, não só o `sdcm`) inicializa
no sandbox. Lançada direto: `de.vw.mib.MIBMain` na JVM do firmware (`/lge/hmi/jvm`, OpenJDK
**1.8.0_242** Zulu aarch64), com os jars de `/lge/hmi/jar` + `/vw/hmi/{lib,jar}` no
`-Xbootclasspath/a`, recursos de `/vw` (partição HMI `OISmall_EU.zip`) bind-montados, e
`/dev/dri/card0`+`renderD128` fingidos. Script reprodutível: `driver/run_hmi3.sh`.

### O achado que destravou: `sun.boot.library.path`
`System.loadLibrary("DebugSPINative")` falhava com `no DebugSPINative in java.library.path`
mesmo com o arquivo presente (`/lge/hmi/libs/libDebugSPINative.so`, 88 KB) e
`-Djava.library.path=/lge/hmi/libs`. Causa provada: como o `MIBMain` entra via
**`-Xbootclasspath/a`**, as classes dos bundles são carregadas pelo **bootstrap classloader**,
que resolve nativos pelo **`sun.boot.library.path`** — não pelo `java.library.path`.
A lib exporta o símbolo JNI estático `Java_com_lge_mib3oi_spi_debug_DebugSPIImpl_initSPINative`
(confirmado por dump de `.dynsym` com `driver/elfsyms.py`), então bastava a JVM **achar e
carregar** o `.so`. Fix: `-Dsun.boot.library.path=/lge/hmi/jvm/lib/aarch64:/lge/hmi/libs`.
(LD_PRELOAD da lib "adiantava" pro erro `initSPINative()V` porque uma lib pré-carregada pelo
linker não entra na tabela de NativeLibrary da JVM — só `System.load/loadLibrary` registra.)

### Até onde a HMI chegou (evidência do log, `traces/hmi3_out`)
1. `Starting SPI bundles ...` → `DebugSPI network log sink state: active` — **lib nativa SPI carregou e RODOU.**
2. `Adreno-GSL open(/dev/dri/card0) failed: errno 1` + `Failed to open GSL Library` — **GPU falha, mas NÃO é fatal**; a lógica segue.
3. `Starting autostart bundles ...` → `Starting managed bundles ...` — ciclo OSGi completo (SPI→autostart→managed).
4. `[WorkUnitStarter] Starting Work Unit WORKUNIT_ROOT__GENERIC`
5. `Instantiating class loader: loading CLASSes from viewhandler.jar` — **o ViewHandler (motor de UI) é instanciado.**
6. `UI thread started!` — **a thread de UI sobe.**
7. Linha de status RSI/VIWI populada e viva:
   `skin...OISmall2_EU; hmisoftware.version:=H40.272.304_OISmall2_EU;
   protocol.version:=1.2; vehicle.id:=0200000000000000; rsi.stats:{OpenSubs:1};
   free.memory:=136010744` — o servidor RSI/VIWI está respondendo.
8. Mesmo após a exceção DSI (abaixo) **continua**: `WORKUNIT_EARLY_PERSISTENCE`.

### A parede real: barramento de dados DSI (não o Java, não o coding)
`ERROR Required DSI services not registered within 15000 ms:`
`org.dsi.ifc.persistence.DSIPersistence[0]`, `DSIPowerManagement[0]`,
`DSICarTimeUnitsLanguage[0]`, `DSIGeneralVehicleStates[0]` →
`DSIProxyException: The DSI service instance is not available`.
A HMI é **cliente** desses serviços; quem os **publica** são daemons nativos C++ (achados por
grep no firmware):
- `/lge/app_ro/carif/carifservice` (unit `carifservice.service`) — gateway dos dados do veículo (vehicle states, power, car time/units/language);
- `/lge/app_ro/hmi/persistence-app` — `DSIPersistence`;
- `/lge/app_ro/hmi/mcu-interpreter` — power/MCU.
Esses daemons, por sua vez, falam com os **pares de hardware** (MCU, CAN) via kipc — que o PC
não tem. Destravar o DSI = rodar esses daemons com os mesmos stubs de kipc + **dados sintéticos
de veículo**, ou escrever um **emulador DSI** que publique as 4 interfaces. Grande, e mesmo
assim não gera pixel (ver abaixo).

### Teto honesto de "cockpit renderizado no PC" — 3 paredes de hardware/dados
1. **GPU Adreno (parede dura de render):** o render é OpenGL ES pelo stack **proprietário
   Qualcomm (Adreno-GSL)**, que abre `/dev/dri/card0` real. **Não há fallback de software**
   (não é Mesa/llvmpipe). Sem GPU Qualcomm = sem pixels do cockpit. O PC não tem.
2. **Barramento DSI (dados):** sem `carifservice`/`persistence-app`/`mcu-interpreter` + dados
   sintéticos, a HMI roda degradada, sem estado de veículo.
3. **O cluster (Active Info Display) é OUTRA ECU:** o firmware do painel **não está nesta
   imagem** (a central é o módulo 5F/MIB; o cluster é o módulo 17). Mesmo com HMI+GPU+DSI, o
   que renderiza o mapa no painel é a ECU do cluster, cujo binário não temos.

**Veredito:** a camada **Java/OSGi da HMI roda de verdade no sandbox** (bundles, ViewHandler,
UI thread, servidor RSI/VIWI) — isso está **provado e é reproduzível**. Um cockpit
**visualmente renderizado e funcional no PC** não é alcançável: barra na GPU Qualcomm (sem
software-render), no barramento DSI (precisa dos daemons + dados sintéticos) e no fato de o
painel ser uma ECU separada cujo firmware não está nesta imagem. O alvo central do projeto —
**provar o coding do VideoNavigation** — já estava atingido pela Pista II e **não depende de
nada disto**.

## ATAQUE À PAREDE DSI — barramento kipc em userspace + daemons reais (10/10/2026)

Resposta ao "faça tudo o que for necessário": em vez de *afirmar* que o DSI/GPU eram muros,
fui **executar e testar**. Construí um **barramento kipc em userspace** e rodei os daemons
DSI de verdade sobre ele.

### O que foi construído (reprodutível)
- **`driver/kipc_hub.c`** — broker: AF_UNIX SEQPACKET em `/run/kipc_hub.sock`; recebe de
  qualquer *writer* (`/dev/css`) e difunde para os *readers* (`/dev/csr`), com cache+replay
  dos anúncios (a HMI sobe ~15s depois dos daemons) e **exclusão por PID** (um emissor não
  recebe a própria mensagem). Estático (`-static`), roda sob qualquer glibc.
- **`driver/kipc_bus.c`** — `LD_PRELOAD` **sem libc** (syscalls inline aarch64): `open("/dev/csr|css")`
  vira socket pro hub; `ioctl(fd,0,desc)` vira send (writer: lê `desc+0x00`=len, `+0x10`=tipo,
  `+0x18`=payload) / recv (reader: copia payload pra `*(desc+0x18)`, seta len/tipo).
  Emula `errno` da libc (`__errno_location`) nas pass-throughs, senão a JVM trata `-errno`
  como fd válido e estoura. `run_hmi4.sh` (com daemons) / `run_hmi5.sh` (sem).

### Fatos PROVADOS (não inferidos)
1. **Os daemons DSI estão VIVOS no barramento.** `persistence-app`, `mcu-interpreter` e
   `carifservice` conectam `/dev/csr`+`/dev/css` ao hub e **trocam mensagens kipc reais**:
   um `WELCOME` de 26628 bytes (payload `0x6800`, tipo `0x14`/`0xc`) na subida, seguido de um
   fluxo de mensagens de 20 bytes (tipos `0x3`, `0x10`, `0x17`, `0x1f`, `0x2f`, `0xf`…) — o
   tráfego de registro/estado do DSI. Antes (sem o hub) eles giravam em `ENOTTY`; agora falam
   o barramento. **Os daemons rodam.**
2. **A HMI COEXISTE com o barramento vivo** (até a UI thread), desde que o shim use a
   convenção certa de "sem mensagem": **`-1`/`EAGAIN`**, não sucesso-vazio. Com sucesso-vazio a
   `libDSIService` cai num `terminate` recursivo; com `EAGAIN` a HMI vai a `managed bundles` →
   `UI thread started!` → timeout normal de 15s do DSI (igual ao run sem barramento).

### A parede, agora caracterizada por TESTE
Ao difundir as mensagens **reais dos daemons** para a HMI, a `libDSIService` **estoura a JVM**
(`SIGSEGV` em `libjvm.so+0x675118`, offset **fixo** em todos os runs — assinatura de operação
JNI de array com tamanho errado). Testado:
- **Não é overflow de tamanho:** o crash persiste mesmo encaminhando só mensagens pequenas
  (20 bytes); logo não é o `WELCOME` de 26 KB estourando buffer.
- **É semântica/ABI:** minha entrega por difusão burra não honra (a) o **ABI de leitura exato**
  da `libDSIService` (de quais offsets do `desc` ela lê len/tipo — assumi o layout do `sdcm`,
  que pode divergir) nem (b) o **endereçamento/handshake do protocolo DSI** (as mensagens que
  difundo são peer→peer dos daemons, não as notificações registro→cliente que a HMI espera).

### Conclusão honesta (com evidência)
O **transporte de bytes do kipc é alcançável** — provei rodando os daemons reais num barramento
caseiro. O que falta pra "DSI funcional" é a **semântica do protocolo**: RE do caminho de
leitura da `libDSIService` (`Java_...dsi_Handler_initDsiNative`/receive — delimitado) **mais** o
formato/endereçamento das mensagens de anúncio de serviço e o handshake (o "contrato de runtime"
estimado em semanas). É um subprojeto real, agora com o ponto de ataque **exato** mapeado
(`libDSIService.so`, os tipos de mensagem observados, o ABI do `desc`). E mesmo resolvido, **não
gera pixel**: o render é hard-linkado ao Adreno (`libjopengl_es3.so` NEEDED `libEGL_adreno.so`/
`libGLESv2_adreno.so`), que exige o driver kgsl + GPU Qualcomm ausentes no PC.

**Saldo deste ataque:** saí de "afirmar os muros" para **derrubar os de transporte** (bus kipc
funcional, daemons vivos, HMI coexistindo até a UI thread) e **isolar por teste** o muro
remanescente (semântica DSI + GPU). O alvo central do projeto — o coding do VideoNavigation —
permanece provado e independente disto.

## ✅ DSI FUNCIONAL NO SANDBOX — serviços registram, cockpit compõe conteúdo (10/10/2026)

Depois de mapear a parede DSI, **venci a camada semântica o suficiente para a HMI registrar
serviços DSI reais e rodar seu pipeline de composição de tela.** Não é mais "transporte ok,
semântica não": a HMI está **funcional no nível de software**.

### A RE que destravou (Ghidra headless sobre libDSIService.so)
Decompilando o caminho de recepção (`event_checker_kipc` → `event_handler` → `sendDSIResponseUpdate`
→ JNI) descobri:
- **ABI do `MsgPkt` (confirmado pelo `kipc::MsgPkt::MsgPkt` e `CommunicationService::read`):**
  `word0 (0x00)=canal, word1 (0x08)=id, word2 (0x10)=len (uint), word3 (0x18)=ponteiro do payload`.
  Meu shim tinha **len e canal trocados** → a HMI lia tamanho errado → `NewByteArray` com
  tamanho ruim → `SIGSEGV libjvm+0x675118`. Corrigido o framing para `[canal:8][payload]`.
- **Convenção "sem mensagem":** read não-bloqueante devolve **`-1/EAGAIN`**, não sucesso-vazio
  (sucesso-vazio → `terminate` recursivo).
- **Guard anti-crash:** `event_handler` tira header de até `0xc` bytes e passa o resto a
  `NewByteArray(msgSize)`; mensagem com payload `< 0xc` vira `msgSize` negativo = SIGSEGV. O hub
  passou a **descartar mensagens curtas demais** (poluição de broadcast do carifservice).

### O que a HMI faz agora, PROVADO (com os daemons reais no barramento kipc)
Rodando `persistence-app` + `mcu-interpreter` + `touch-app` (+ carifservice) sob o `kipc_hub`:
- **Serviços DSI REGISTRAM de verdade** — a lista de "Required DSI services not registered" cai
  de **4 → 2**: `DSIPersistence` (persistence-app), `DSIPowerManagement` (mcu-interpreter) e
  `DSIKeyPanel` (touch-app) passam a registrados; a HMI inclusive **chama** o keypanel
  (`DSIKeyPanel[0].setGenericSetting` request/response).
- **A HMI roda o pipeline de composição de tela** (`ContentProcessorManagerImpl`), atribuindo
  conteúdo a TODAS as áreas: `ScreenAreaControlCenter=HMI_CONTROLCENTER`,
  `ScreenAreaStatusBarSEAT=HMI_SEAT_STATUSBAR`, `ScreenAreaParticleBackground=HMI_BACKGROUND`,
  `ScreenAreaMenuBarSEAT=HMI_SEAT_MENUBAR`, `ScreenAreaStage=HMI_RADIO/HMI_USER_PERCEIVED_OFF`,
  `ScreenAreaDigitalAssistant`.
- **App adapters vivos:** `RADIO_PERFORMANCE Radio button visible in grid menu`,
  `[ControlCenterAppAdapter] register NotificationAction` (Rádio + Control Center rodando).

### O gate final e a convergência com a GPU
A HMI trava esperando `(ASL)DisplayManagementService.getDisplayExtents(displayId=0)` — a
**geometria da tela**, provida por `DSIDisplayManagement`. Os daemons de display
(`graphics/multi-display/multi-screen-server`, `rsi-display-management`) **não registram** esse
serviço porque precisam do **compositor weston + DRM/Adreno** real pra enumerar o display. Ou
seja: o último gate da composição e o render final **convergem na mesma parede de hardware
Qualcomm** — alcançada agora **pelo topo da pilha** (a HMI PEDE a geometria que só o display real
entrega), não só por baixo (o `libjopengl_es3` hard-linkado ao Adreno).

### Veredito atualizado (por execução, não afirmação)
- **A interface do cockpit está FUNCIONAL no nível de software no sandbox:** boota, carrega
  bundles, registra serviços DSI reais via o barramento kipc que construí, compõe o conteúdo de
  todas as áreas de tela e roda os app adapters. Isso é reproduzível (`driver/run_hmi4.sh`,
  `run_hmi7.sh`, `run_hmi8.sh`).
- **Um cockpit visualmente RENDERIZADO não é alcançável no PC:** tanto a geometria do display
  (`DSIDisplayManagement` via weston/DRM) quanto o render (`libEGL_adreno`/`libGLESv2_adreno`)
  exigem o stack de display/GPU Qualcomm (weston+DRM+Adreno) ausente — testado, absoluto. E o
  painel (Active Info Display) é outra ECU, firmware fora desta imagem.
- Faltam ainda `DSICarTimeUnitsLanguage`/`DSIGeneralVehicleStates` (carifservice, precisa de CAN)
  e dados sintéticos — mas o **mecanismo está provado**: cada daemon/serviço a mais leva a HMI
  mais longe, e o teto é a GPU/display.

**Artefatos novos:** `driver/kipc_hub.c` (broker, com guard anti-crash + cache/replay + exclusão
por PID), `driver/kipc_bus.c` (LD_PRELOAD, framing MsgPkt correto), `driver/run_hmi{4,6,7,8}.sh`,
`ghidra_scripts/DumpDsi*.java`/`DumpAddr.java` (RE do caminho DSI).

## Parede do display: testada até o kernel (10/10/2026)

Para destravar o `getDisplayExtents` (e ver a HMI *calcular* o layout, mesmo sem pixel), tentei
subir o compositor por software:
- **weston headless:** o weston do firmware só traz `drm-backend.so`, `fbdev-backend.so`,
  `wayland-backend.so` — **não há `headless-backend.so`**. E o `weston.ini` usa ivi-shell com
  outputs `HDMI-A-1`/`DSI-1` sem geometria fixa (vem do conector DRM).
- **Display virtual sem GPU (vkms/vfb):** `modprobe vkms` e `modprobe vfb` **falham** — o kernel
  do OrbStack (7.0.14-orbstack) é minimal e **não tem módulos carregáveis** (`vkms`/`vfb`/`drm`
  ausentes). Logo não há como criar um `/dev/dri/card` ou `/dev/fb0` virtual nesta máquina.
- Sem compositor, `rsi-display-management`/`multi-screen-server` não enumeram display e não
  registram `DSIDisplayManagement` → a HMI fica em `Waiting for getDisplayExtents`.

**Stub de protocolo como alternativa:** as mensagens DSI são **protobuf por serviço**
(`com.lge.mib3oi.dsi.displaymanagement.proto.DSIDisplayManagementMsg$getAvailableDisplays`,
`$setDimension`, `$Builder`…, em `MIBHMI-DSI.jar`). Stubar o `getDisplayExtents` exigiria
reconstruir o protobuf + o registro do serviço + a correlação request/response — um **emulador
DSI por interface** (o "contrato de runtime", semanas), e mesmo assim só levaria ao *cálculo de
layout*, nunca a pixel (render hard-linkado ao Adreno).

**Conclusão por execução (não afirmação):** tentei o caminho do weston/display virtual que eu
mesmo apontei; ele está barrado **no kernel da máquina** (sem DRM/vkms/vfb) e no weston do
firmware (sem headless). A geometria de tela e o render são genuinamente indisponíveis neste PC.
O cockpit está **funcional no nível de software** (serviços DSI registram, composição de conteúdo
e app adapters rodando); **pixel/layout com display é parede de hardware**, agora testada até o
ponto de tentar carregar o módulo de kernel.

## EMULADOR DisplayManagement + TESTE DE RENDER (10/10/2026)

A pedido do Lipe ("tentar o emulador DisplayManagement"), escrevi um provedor DSI fake
(`driver/dmstub.c`) e levei a HMI ao ponto de **tentar renderizar de verdade**.

### RE do protocolo (CFR sobre MIBHMI-DSI.jar + MIBHMI-STD-...-bin.jar)
- **Domínios DSI** (`com.lge.mib3oi.dsi.DSIDomain`): `PID_DISPLAY_MGR = 0x4000`, `PID_PERSISTENCE =
  0x10000000000000`, `PID_TOUCH = 0x80000000000`, `PID_MCU_INTERPRETER = 0x400000`… Cada processo
  manda no barramento com `word0 = seu PID` (= domainId visto no `event_handler`).
- **Registro de serviço** (`ListenerWorkerThread`): mensagem no domínio do serviço com
  `interfaceId = 9000` dispara `ServiceManager.DSIAdmin.startService(DSIXxx)`. Para o display:
  domínio `0x4000` + ifid `9000`.
- **Framing do payload** (domínio 0x4000): `[interfaceId:4][instanceId:4][apiId:4][protobuf...]`
  (confirmado pela `Message(int apiId, byte[] data, long domainId, int interfaceId, int instanceId)`).
- **getDisplayExtents** (ASL `GetDisplayExtentsOperation`): chama **`getDisplaySize`** (request
  apiId **1043**) e espera **`getDisplaySizeResult`** (response apiId **2029**, protobuf
  `displayID=1, width=2, height=3, result=4`) → `reportSuccess({width,height})`.

### Resultado (provado)
- O `dmstub` manda `[canal=0x4000][ifid=9000,inst=0]` → **a HMI REGISTRA o `DSIDisplayManagement`**
  (sai da lista de "not registered") e passa a **chamar `getDisplaySize(displayId=0)`**.
- O `dmstub` responde `[canal=0x4000][ifid=1,inst=0,api=2029][protobuf width/height]` → a resposta
  **chega ao método certo** `DSIDisplayManagementListener[0].getDisplaySizeResult(...)`. (Fica
  "unexpected and will be discarded" por falta do handshake de *subscription* — a notificação só é
  aceita de um subscriber ativo; refinável respondendo ao request específico, mas ver abaixo.)
- Com o display registrado, a HMI **avança até a inicialização de GL** e **crasha** em:
  `Create: NULL native display is unsupported, for now` → `SIGSEGV`/`Aborted`.

### TESTE DE RENDER — o muro agora é por execução, não por hard-link
A HMI **tenta criar o display EGL** e morre porque **não há display nativo** (wayland/weston). Isso
é o teste de render que faltava: não é mais "o `libjopengl_es3` é NEEDED do Adreno, logo é
impossível" — é a HMI **executando a criação do display e falhando** na ausência do compositor.
E já está provado (acima) que **não dá pra prover esse display neste PC**: o kernel do OrbStack não
tem `vkms`/`vfb`/`drm` (sem `/dev/dri` ou framebuffer virtual) e o weston do firmware não tem
backend headless. O único jeito de passar daqui seria **stubar o EGL/GLESv3** (fabricar
`libEGL_adreno`/`libGLESv2_adreno` falsos), que é reimplementar um driver GLES3 — e ainda assim
sem pixel real.

### Até onde chegou o software do cockpit (reproduzível: `driver/run_hmi9.sh`)
HMI boota → SPI/autostart/managed bundles → serviços DSI registrados (persistence, power, keypanel,
**displaymanagement** via dmstub) → pipeline de composição de áreas de tela → app adapters (Rádio,
Control Center) → `enterView(SimpleDesktopLayout)` / `init screenSimpleDesktopLayout` → **init de
GL** → crash no display nativo ausente. **Tudo o que não depende de GPU/compositor roda.**

## ✅ GL STUB — o pipeline de render da HMI EXECUTA (10/10/2026)

Último passo do "faça tudo necessário": em vez de só constatar o crash de EGL, escrevi um
**stub de EGL/GLESv3** e fiz o loop de render da HMI rodar.

### O que foi feito
- `driver/gen_fakegl.py` gera `driver/fakegl.c` a partir dos **391 símbolos** que o
  `libjopengl_es3.so` importa do Adreno (dump via `readelf`). 64 funções críticas escritas à
  mão (EGL com handles não-nulos: `eglGetDisplay/Initialize/ChooseConfig/CreateContext/
  CreateWindowSurface/MakeCurrent/SwapBuffers`; GL com queries sãs: `glGetString`, `glGetIntegerv`,
  `glGetShaderiv/Programiv=GL_TRUE`, `glCreateShader/Program`, `glGen*`, `glCheckFramebufferStatus=
  COMPLETE`, `eglGetProcAddress` com tabela de despacho); o resto é no-op retornando 0.
- Compilado em **dois `.so` com sonames `libEGL_adreno.so` e `libGLESv2_adreno.so`** (`-nostdlib`,
  sem libc) e posto **primeiro no `LD_LIBRARY_PATH`** da HMI → o loader resolve o `libjopengl_es3`
  contra os meus, e o driver Adreno real (+ o sub-driver wayland que abortava) nunca carrega.
  Reprodutível: `driver/run_hmi11.sh`.

### Resultado — PROVADO
- **A HMI não crasha mais** (rc=124, timeout limpo; antes abortava em ~200 linhas, agora roda
  **34 651 linhas** nos 70 s).
- Passou do `Create: NULL native display is unsupported` → `initializing app framework ... app
  framework initialized!`.
- **Entrou nas views reais do cockpit** (uma por área de tela): `enterView(SimpleDesktopLayout)`
  → `enterView(HMI_OFF_Colored_Screen, 'ScreenAreaStage')` → `enterView(SYSTEM_CONTROLCENTER_MAIN,
  'ScreenAreaControlCenter')` → `USERMANAGEMENT_CONTROL_CENTER_MAIN` → `SPACE_GESTURE_INDICATOR`.
- **O loop de repaint/render roda:** `RepaintManagerImpl.consumeEvent` → `RepaintEventImpl` →
  `ViewCompositorImpl.repaintViews` → `beginFrame`, **~4 500 ciclos de repaint** em 70 s.

### Por que não vira pixel (o teto, agora no nível do render)
O contexto gráfico não liga de verdade: `GLESGraphics3D.queryFunctionality` chama
`glGetString` e o wrapper JNI (`org.opengl.GLES20Impl.glGetString`) recebe `byte[]` nulo →
`NullPointerException` → cada frame loga "Error while repainting screen!" (2253x). Ou seja, o
**loop executa, mas o desenho em si falha** porque o stub não é um contexto GLES3 real — e, mesmo
que fosse, **não há framebuffer/GPU pra rasterizar nem pra ler de volta**. Afinar o stub até o
desenho "passar" é reimplementar um driver GLES3 inteiro, sem nenhum pixel no fim (sem a GPU
Qualcomm). Então o render **executa**; **imagem** exige o hardware que o PC não tem.

### Estado final do "simular a central" (tudo por execução)
boot → AGL userspace → HMI OSGi (SPI/autostart/managed) → **barramento kipc (meu)** → serviços DSI
reais (persistence, power, keypanel) + **DisplayManagement emulado (meu `dmstub`)** → composição
de conteúdo das áreas → **entra nas views** → **loop de render/repaint executando** (stub GL).
**Tudo que é software roda.** O único passo que resta — pixel na tela — é parede de hardware
(GPU/compositor Adreno ausente; kernel sem vkms/vfb/drm), e o painel ainda é outra ECU.

## Por que o contexto gráfico não liga — provado no nativo (10/10/2026)

Investigação exaustiva do `glGetString` que retorna `byte[]` nulo em `GLES20Impl.glGetString`
(NPE que impede `GLESGraphics3D.queryFunctionality` → `bind`):
- `org.opengl.GLES20Impl` **não existe em jar nenhum** da imagem — é gerada em runtime; e as libs
  `libjopengl_es3`/`libjopenglnw_es3` **não exportam** os métodos JNI (ligam por `RegisterNatives`).
- Decompilei (Ghidra) a função que chama o `glGetString` importado: **`invoke_glGetString`** em
  `libjopengl_es3` — é um **executor de command-buffer**: lê o enum de `*(buf+0x20)`, chama
  `glGetString` (meu stub → string válida), `strlen`, `malloc`+`memcpy` do resultado num slot
  (`buf+0x60/0x70`), retorna 0. **Com o stub, o executor funciona.**
- Conclusão: o GL binding da HMI é um **renderizador em command-stream** (a HMI enfileira comandos
  GL; um executor nativo os replica contra a GPU). O `byte[]` nulo no Java vem da **entrega do
  resultado pelo command-buffer**, que depende do pipeline de render estar de fato ligado (contexto
  válido + executor consumindo o buffer) — o que exige um contexto GL real, logo a GPU. Não é um
  símbolo a mais pra stubar: é a arquitetura do renderizador, feita pra hardware real.

**Teto provado em TRÊS níveis independentes:** (1) `libjopengl_es3` NEEDED hard-linkado ao
`libEGL_adreno`/`libGLESv2_adreno`; (2) em execução, `Create: NULL native display is unsupported`
(sem compositor); (3) no código nativo, o round-trip do command-buffer do `glGetString` depende de
contexto real. Os três apontam pro mesmo lugar: **a GPU/compositor Qualcomm, ausente no PC** (e o
kernel sem `vkms`/`vfb`/`drm`). O software roda até o loop de render; pixel é hardware.

## Render por SOFTWARE também bloqueado — o torniquete GLES 3.1 × glibc 2.23 (10/10/2026)

Antes de declarar o teto, testei o caminho que faltava: um GLES **de software real** (Mesa
llvmpipe) no lugar do stub no-op, que renderizaria na CPU pra um framebuffer offscreen (pixel
sem GPU). Resultado: **inviável nesta máquina, por um torniquete de versão**:
- **Firmware = glibc 2.23** (`/lib64/ld-2.23.so`; GLIBC_ máx 2.23). Uma lib carregada no processo
  da HMI precisa ser construída para glibc **≤ 2.23**.
- **A HMI exige GLES 3.1** (`GLESGraphicsManager`: "GLES 31 required"; lib `libjopengl_es3`).
- **GLES 3.1 por llvmpipe só existe em Mesa ≳ 18.1**, que requer glibc **≳ 2.27** (> 2.23).
- A única base glibc-2.23 (Ubuntu 16.04) traz Mesa ≤ ~18.0 → **GLES 3.0**, abaixo do exigido; e os
  pacotes arm64 dela estão EOL/404.
- Construir um Mesa ≥21 + LLVM do zero mirando glibc 2.23 é um subprojeto de **dias** (toolchain
  C++17 em base 2.23, build do LLVM, do Mesa), de alto risco, e mesmo assim esbarraria no binding
  GL em command-stream da HMI (feito pro Adreno) e na captura do framebuffer — e geraria, no
  melhor caso, **um quadro estático da central (não o painel, que é outra ECU)**.

**Exaustão comprovada.** Pixel exige GLES 3.1; GLES 3.1 (hardware Adreno OU software Mesa) não
roda no userspace glibc-2.23 do firmware sem o hardware Qualcomm real OU um build de toolchain
proibitivo. O software da central roda **até o loop de render**; a imagem é parede de
hardware/ABI, provada em quatro frentes: NEEDED Adreno, `NULL native display` em execução,
round-trip do command-buffer no nativo, e agora o torniquete GLES 3.1 × glibc 2.23.

## Render por software TESTADO por build/run — forward-compat glibc + Mesa (10/10/2026)

Respondendo à cobrança de "provar por build/test, não por inferência": construí e **rodei** o
caminho de GLES de software real.
- **Fatos provados (empíricos):** (a) Mesa 25 (Ubuntu 24.04) faz **OpenGL ES 3.2 por llvmpipe**
  (software, `EGL_PLATFORM=surfaceless`), ≥ os 3.1 que a HMI exige; (b) o Mesa exporta **os 391
  símbolos** que o `libjopengl_es3` importa (0 faltando — `comm` de `readelf`); (c) o `java` do
  firmware (compilado p/ glibc 2.23) **roda sob glibc 2.39 e 2.27** (forward-compat, `rc=0`).
- **Montei o run** (`driver/run_hmi12.sh`): sem chroot, dirs do firmware bindados na raiz, glibc/
  libstdc++/Mesa do container, Mesa linkado como `libEGL_adreno.so`/`libGLESv2_adreno.so`, serviços
  DSI + dmstub no barramento kipc, `-Dde.vw.mib.graphics.gles.v3`.
- **Resultado (ttestado):** a HMI sobe (log4mib, SPI), carrega o `DebugSPINative` (com `/fwusr`
  pro `liblttng-ust`), e então **os libs nativos do firmware crasham sob a glibc estrangeira**:
  `UnsatisfiedLinkError: ... ImageLoader` seguido de **SIGSEGV** (glibc 2.39). Com glibc 2.27
  (18.04) o ambiente arm64 ficou instável (illegal instruction até em coreutils).
- **Conclusão empírica:** o firmware é integrado demais à sua glibc 2.23 — rodá-lo sob uma glibc
  nova o bastante p/ hospedar um Mesa com GLES 3.1 **quebra os próprios libs nativos do firmware**
  (ImageLoader/graphics). Não é inferência de versão: **buildei o Mesa certo e rodei**; o firmware
  não sobrevive ao ambiente que o Mesa 3.1 exige.

**Os dois caminhos reais pro pixel estão testados e bloqueados:** (1) GPU Adreno real — ausente no
PC (e kernel sem vkms/vfb/drm); (2) Mesa software com GLES 3.1 — exige glibc > a do firmware, e o
firmware crasha nessa glibc. O software do cockpit roda até o loop de render; **pixel é parede de
hardware/ABI, agora provada por build e execução, não só por versão.**

## CAMINHOS POSSÍVEIS PRA PIXEL — pesquisa multi-fonte (10/10/2026)

Varredura (web/GitHub/Reddit/X, 6 ângulos). **Correção importante de um erro meu:** eu tinha
concluído "render impossível" porque testei Mesa **pré-compilado moderno** sob glibc 2.39
(forward-compat) → os libs nativos do firmware crasharam. A pesquisa mostra o jeito CERTO, que eu
**não** tinha tentado — e com ele o pixel no Mac é alcançável. Três famílias de caminho:

### A) Pixel no Mac por SOFTWARE (o que eu fiz errado — e o certo)
- O "sem GPU" é, no fundo, só "sem Adreno", que é **substituível por um rasterizador de CPU**.
  **Mesa llvmpipe** faz GLES 3.2 por software; **SwiftShader+ANGLE ("SwANGLE")** faz GLES 3.1 por
  CPU, certificado Khronos. Ambos geram `libEGL.so`/`libGLESv2.so` drop-in.
- **O pulo do gato (que eu errei):** NÃO procurar binário pronto nem rodar o firmware sob glibc
  nova — e sim **COMPILAR o Mesa/SwANGLE dentro de um ambiente glibc 2.23** (container/chroot
  Ubuntu 16.04, que já traz 2.23), conferir o piso com `objdump -T libEGL.so | grep GLIBC_`, e
  dropar no chroot do firmware com o soname **`libEGL_adreno.so`/`libGLESv2_adreno.so`** (o
  `libjopengl` carrega por nome). Render headless: EGL surfaceless + pbuffer + `glReadPixels` → PNG.
  Assim o firmware fica na glibc 2.23 dele (não crasha) e o GLES roda em CPU (sem GPU). Lento,
  porém imagem de verdade. Precedente de build de Mesa p/ glibc antiga: `sg8010/uos20-mesa-gbm`.
  Fontes: https://docs.mesa3d.org/opengles.html · https://mesamatrix.net/ ·
  https://github.com/google/swiftshader/blob/main/docs/ANGLE.md ·
  https://anholt.pages.freedesktop.org/mesa/egl.html (surfaceless) ·
  https://fryzekconcepts.com/notes/freedreno_journey.html (troca por soname) ·
  https://github.com/sg8010/uos20-mesa-gbm
- **Precedente direto + contato:** `iscle/qemu-mhi2` (+ forks `Mr-MIBonk`, `chefranov` com
  `--headless`) — alguém **boota a HMI de uma central VW/Audi num Mac** e usa Mesa llvmpipe
  (GLES por software) no host pro pixel. Ressalva: é **MHI2 = QNX + Tegra3/NVIDIA**, não o nosso
  MIB3 LG (AGL/Adreno/glibc 2.23) — serve como arquitetura a copiar e autor ativo (out/2026) pra
  contatar. https://github.com/iscle/qemu-mhi2 · https://github.com/Mr-MIBonk/qemu-mhi2
- Ressalva do host Mac: ANGLE→Metal só faz GLES 3.0; GLES 3.1 no Mac = via Vulkan/MoltenVK ou por
  CPU (llvmpipe/SwANGLE). Por isso o caminho CPU é o mais previsível aqui.

### B) Pixel "autêntico" com Adreno DE VERDADE (fora do Mac)
- `freedreno`/`turnip` (driver Adreno aberto do Mesa) e o blob `libEGL_adreno` **só produzem pixel
  sobre silício Adreno real + kernel kgsl** — não têm fallback por software. Logo, no Mac, não.
- A cena madura que junta Adreno+glibc: **Winlator + Turnip + Zink** (userland glibc em proot num
  Snapdragon). E o padrão **Scutum** (bridge glibc↔driver por socket Unix) pra encaminhar as
  chamadas GLES a um Adreno real. Caminho real: rodar o stack aarch64 do MIB3 num **aparelho/placa
  Snapdragon** (o MIB3 LG é Snapdragon 820A/Adreno 530), onde o `libEGL_adreno` original funciona.
  https://winlator.com/ · https://github.com/LexusYTG/Scutum · https://docs.mesa3d.org/drivers/freedreno.html

### C) A bancada REAL (como as lojas de coding "veem" de verdade)
- Não é simulação: liga-se a **central MIB3 física** na mesa e captura-se o vídeo. A bancada que a
  VW/fornecedores usam é a **Digiteq MQB37W** (MIB3 + cluster + volante + gateway + CAN). Versão
  pobre: chicote + gateway + **emulador de rede CAN** (há kits no Brasil — Mercado Livre/Reiaudio).
  A central não liga sem **wake-up na CAN**. O pixel sai pelo **LVDS/APIX**, capturado por frame
  grabber (**Goepel Video Dragon**, receptor APIX ADV7782).
  https://www.digiteqautomotive.com/en/products/test-benches/mqb37w-test-bench ·
  https://www.goepel.com/en/automotive-test-solutions/hardware/bus-interfaces/lvds
- A cena "CarPlay no Virtual Cockpit" (kamgurgul, harman-f, omonob, OneB1t…) **não renderiza num
  PC**: roda no head unit REAL via SSH; o pixel vem do telefone (H.264 do CarPlay) ou da GPU do
  carro empurrada pro cluster por MOST150/LVDS. Útil como RE da display policy Java, não pra pixel
  no PC. `OneB1t/VcMOSTRenderMqb` é um renderizador GLES que escreve no Virtual Cockpit (QNX/Win).

**Veredito corrigido:** pixel **é alcançável no Mac** por GLES de software **compilado contra a
glibc 2.23** (caminho A) — eu tinha falhado por compilar/rodar errado, não por impossibilidade.
Pixel acelerado "de verdade" = Adreno real (B) ou bancada física (C). O MIB3 LG em si nunca foi
emulado publicamente; seríamos os primeiros.
