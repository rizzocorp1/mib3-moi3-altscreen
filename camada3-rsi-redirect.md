# Camada 3 — serviço de redirect por RSI (opção "C")

Projeto do serviço que, na unidade, faz o mapa de um app (CarPlay/Android Auto) aparecer no
cluster — apontando o caminho de vídeo (o `multi-screen-server`) pro surface do app em vez do
`naviengine`. É o componente **mais próximo da arquitetura do carro** (o mesmo que o PoC do
`daChillout` faz: "custom RSI service, cropa 1280×480, empurra pro cluster"). **Interop, sem
exploit**: usa RSI/`displaymanagement`/`ivi_share` documentados. Desenvolvido e testado na bancada;
**rodar na unidade depende do pé de execução, que não desenvolvemos**.

**Estado (17/10/2026): o caminho está claro ponta a ponta.** O mecanismo para pôr o mapa de um app no cluster
está entendido e validado — pela nossa RE/bancada (seções abaixo) e por terceiros num carro real da mesma
família de unidade (MOI3 LG, ver "Corroboração externa"). O **único gate** é o foothold na unidade. O projeto o
mantém **fora do escopo**; as partes de contornar a assinatura, FAZIT e conversão de marca/região **não** são
registradas nem assistidas aqui. O que fica documentado é só a **arquitetura de vídeo** (surface, codec, tamanho,
AltScreen), que é o conhecimento reaproveitável.

### Resposta do James sobre o feed do encoder (17/10/2026)

Pergunta nossa (via drive2) sobre a atualização de quadros. Só a parte de **pipeline** (a parte de execução na
unidade fica fora, não registrada):
- **Ele não re-encoda pelo OMX.** O AltScreen do CarPlay já chega em **H.264** do iPhone; ele lê o `avcC` +
  quadros, converte **AVCC → Annex B**, embala em **MPEG-TS/RTP** e manda pro painel por Ethernet. Sem OMX.
- **O loop Empty/FillBuffer (OMX) ele NÃO resolveu** — disse que não tem solução completa; chegou no vídeo em
  movimento **por não re-encodar**. É o **mesmo muro** da nossa bancada (0 quadros H.264 pelo feed do OMX):
  contornado, não vencido.
- **Quadro estático inicial = lado do decoder (painel):** o `avcC` de reconfiguração reseta a espera por
  keyframe e descarta P-frames até o próximo **IDR**. (O travamento inicial não foi 100% atribuído só a isso.)
- Falso-positivo no debug dele: parada por **limite de tamanho de arquivo** na gravação (transmissor seguia,
  captura parou), não o pipeline.

**Leitura pra nós:** o "resto" (0 frames) era do caminho **OMX re-encode** (grab→Rgba2yuv→OMX→RTP), necessário só
pra **surface arbitrário** (mapa nativo RGBA). Pra fonte **já H.264** (CarPlay) dá pra **pular o OMX** e só
remuxar (AVCC→Annex B→MPEG-TS/RTP, vídeo padrão). O feed do OMX é o pedaço duro (ninguém fechou publicamente); a
rota prática pra H.264 o evita. Fecha conceitualmente nosso item aberto. `[WEB]` (relato do James)

### Bancada: o pipeline FECHA ponta a ponta (17/10/2026) — não é mais "0 quadros"

O run `rsi-enc-diag` (17/10 09:50, com o código dos 4 arquivos do commit `1426d30`) provou o transporte completo:
- **`stream.h264` = 581 KB, 420 frames** H.264 (x264 all-intra, crf28) no `enc.log`; decodificam em imagem
  reconhecível (`pat_*.png`, `demo_padrao_rtp.mp4`). O **feed do encoder funciona** (fakeomxcore→enc_daemon).
- O `glUniform program-not-linked` antigo (spike-13/14/15, 38 chamadas) **foi corrigido**: agora rodam ~11k
  chamadas, 398 `glDrawArrays`/`glFinish`.
- O `glRenderbufferStorage` "no renderbuffer bound" é **benigno**: o mss chama sem bindar RB (o Adreno tolera,
  o Mesa não), mas a emulação **render-to-external** usa uma textura 2D de backing como alvo do FBO, então os
  draws acontecem (nz=1152000 não-zero). O "0 quadros" no resumo do glhost é bug do contador, não do pipeline.
- **Shaders reais despejados:** `shader_1` = vertex passthrough; `shader_2` = conversão do mss
  (`samplerExternalOES`, remap de stride/scanline + swizzle BGRA — o "remap Venus", não YUV direto).

**Resíduo (acabamento):** o grab do `ivishare-stub` é em branco, então o conteúdo reconhecível vem de um
**PATTERN injetado no readback** (`MOI3_ENC_PATTERN` em `apply.py`, ramo das barras SMPTE). Pra conteúdo REAL
pelo pipeline inteiro: alimentar o `ivishare-stub` com uma imagem (frame de nav sintético) + readback real
(sem `MOI3_ENC_PATTERN`) + decodificar. A conversão real já foi provada isolada no `spike/moi3_shader_demo.c`
(frame sintético → NV12). `[EXEC]`

### FECHADO com conteúdo real — sem pattern (17/10/2026)

Acabamento feito. O `ivishare-stub` já serve **8 barras de cor** (XRGB8888, 1280×480, `criar_buffer()`). O
readback real devolvia RGBA cru (tamanho errado pro encoder — motivo do pattern); corrigido em
`glforward-ext/apply.py`: o ramo `OPX_READBACK` sem `MOI3_ENC_PATTERN` agora faz **RGBA→NV12 (BT.601 limitado,
flip V)**. Run `enc-real` (SEM pattern, SECS=20, imagem `moi3-bancada:rsidbg-enc`): `READBACK REAL->NV12
1280x480 sz=921600 glerr=0`, **`stream.h264` = 618 KB / 540 frames**, e o decode mostra **as barras exatas do
compositor** (vermelho/verde/azul/amarelo/ciano/magenta/branco/cinza) — conteúdo REAL, não o pattern SMPTE.

Cadeia provada num run só: `ivishare-stub (barras) → grab ivi_share → textura externa → shader real (Rgba2yuv)
→ FBO → readback RGBA→NV12 → fakeomx → enc_daemon x264 → H.264 → decode = as barras`. O `glRenderbufferStorage`
segue benigno. **Ressalvas:** conteúdo é imagem de teste (barras), estático, e o remap do shader saiu ~identidade
no llvmpipe nesta geometria (uniforms do carro seriam outros) — mas a **mecânica do pipeline está provada com
conteúdo real**. Comando de run adaptado do `camada3/run_rsi.sh` (imagem rsidbg-enc + `gdb_incontainer.sh`, sem
`MOI3_ENC_PATTERN`). Artefato: `scratchpad/enc-real/real_01.png`. `[EXEC]`

Estado no carro do Lipe (autoscan 14/10): a adaptação **`navigation_map_transmission_mode =
IP_streaming`** já está **ON** — a peça "#4 (cluster aceita o stream)" já existe.

## Corroboração externa — opção C feita num MOI3 LG (drive2.ru, visto 17/10/2026)

Post de um dono (Polo liftback, unidade **MOI3 LG `3G5035816D`** — mesma família da MIB3 LG do Jetta) relatando
**CarPlay no cluster AID**, e o mecanismo bate integralmente com o nosso modelo da camada 3:
- usou a **segunda tela (AltScreen) do CarPlay (tipo 111)** — o iPhone renderiza o mapa já no tamanho do cluster e manda um
  stream **H.264**; a unidade injeta esse vídeo no canal da **carta nativa**, tomando-o do `naviengine`;
- a surface é a **`navi-kombi-renderingsurface` (id interno 33)**, resolvida pela `displaymanagement` — a mesma
  peça que a nossa RE identificou;
- confirma os números que já tínhamos: **1280×480** e a referência ao **daChillout**; safe-area informada
  `X:480 Y:64 w:320 h:320` para a seta/rota não ficar sob os mostradores do AID;
- funcionou no carro, com Google Maps, Waze e Apple Maps, em três modos de layout;
- **Vetor de execução (revelado pelo James):** não modificou partições flash permanentes (`system.img`); utilizou uma
  **cópia modificada do binário do CarPlay na memória RAM** + uma **biblioteca compartilhada adicional (hook/handler)**,
  carregados **via pendrive USB pelo Red Engineering Menu (GEM)**. Ao reiniciar a central, as alterações somem e o sistema
  retorna ao estado original de fábrica (seguro contra brick);
- **Depuração do feed contínuo:** James relata que inicialmente obteve apenas o primeiro quadro estático antes de ajustar
  a alimentação contínua do fluxo de frames e a restauração do stream após eventos como engate da marcha a ré (câmera).

**Fonte:** <https://www.drive2.ru/l/745725070915144961/> (James11123, 17/10/2026). James construiu a própria solução
após recusar o preço de vendedores comerciais que vendiam patches travados ao VIN. Sem código aberto por enquanto
(está na 2ª versão alfa e pretende publicar quando tudo estiver estabilizado).

**Outro post do mesmo autor** (retrofit de Discover Pro MIB3, 23/04/2024,
<https://www.drive2.ru/l/673974515499287687/>): troca a central por uma MOI3 Discover Pro com **nav nativa HERE
no AID**, com part numbers e fiação (display 9.2" `655919606` de Scala/Kamiq, unidade `3G5035816*`/`3G1035816*`,
LVDS, OCU/Ethernet, programador CH341A). A "ativação total" é por **edição de EEPROM/FEC** + acesso online ODIS —
**defeat de proteção de componentes, FORA DO ESCOPO**, não operacionalizado aqui (cita uma "solução de EEPROM
pública gratuita" p/ MOI3 LG/Preh/Joynext, sem link; nenhum repo). Do lado legítimo, confirma que a **nav
nativa** entrega o mapa completo no AID (com guia de rota e sinais) — o mesmo que a ficha do Jetta já mostra.

**Busca por repo do daChillout (17/10/2026):** `github.com/dachillout` → 404; sem repositório público do código
do PoC do mapa. O público dele são as ferramentas web do **mqbtools.nl** (editores GCA/MCF — aparência/dados) e
os relatos no drive2; o método de execução/instalação na MOI3 ele **não publica** (ver survey de 15/10 no
[painel-de-instrumentos.md](painel-de-instrumentos.md)). Conclusão: **o código do redirect de mapa não está em
git aberto** — nem do daChillout, nem do James11123.

**Escopo:** depende do **mesmo pé de execução na unidade** que já marcamos fora do escopo (linha de cima). As
partes do post que tratam de contornar a assinatura da unidade, do FAZIT e de conversão de marca/região **não**
entram neste registro — são defeat de proteção de dispositivo, fora do que o projeto faz. O que corrobora aqui é
só a **arquitetura de vídeo** (qual surface, qual tamanho, qual codec), que confirma a camada 3.

## RE do `rsi-display-management` (16/10/2026, Ghidra 12.3 headless, binário do MU0883)

Binário extraído do `system.img` (`debugfs`), analisado no Ghidra 12.3. É o serviço RSI
**`RSIDisplayManagementService`** (Poco JSON, REST/VIWI), com **três recursos**, cada um com
`onCreate/onRead/onUpdate/onDelete/onPublish`:

| Recurso | Elemento (campos lidos no `fromJSON`) |
|---|---|
| `/display` | `DisplayElement`: `WIDTH`, `HEIGHT`, `CURRENT_VISIBLE_SURFACES` (+ link `renderingSurfaces`) |
| `/renderingSurfaces` | `RenderingSurfaceElement`: `TOP_LEFT_X`, `TOP_LEFT_Y`, `WIDTH`, `HEIGHT`, `DISPLAY`, `STREAM_PARAMETERS` |
| `/streamParameters` | `StreamParametersElement`: `IP`, `PORT`, `FRAMERATE`, `RESYNC` |

Endereços (projeto Ghidra `reabin`): `StreamParametersElement::fromJSON @0x115468`,
`RenderingSurfaceElement::fromJSON @0x11b4b8`, `DisplayElement::fromJSON @0x122bb0`;
`StreamParametersResource::onCreate @0x12cf48` / `onUpdate @0x12bae0`. Decompilação em
`scratchpad/rea-bins/decomp.txt` (42 funções). `[DESMONTAGEM]`

Strings de interesse: **`navi-kombi-renderingsurface`** (o identificador da renderingSurface do
mapa nativo no cluster), `KEY_CURRENT_VISIBLE_SURFACES`.

### Insight que define o C (corrigido em 16/10/2026 — RE do lado de consumo)

Minha dedução antiga dizia "a renderingSurface **não tem campo de fonte**, logo o redirect seria
no compositor". **Errado.** A RE do `multi-screen-server` (o lado que consome) mostra que a
renderingSurface **carrega, sim, a fonte**, num campo `name` com o esquema **`surfaceid://<id>`**:

- A `renderingSurface` é a REGIÃO DE SAÍDA (top-left + w/h) num **display**, ligada a um
  **`streamParameters`** (ip/porta/fps = o destino RTP) **e a uma FONTE** — o surface ivi a capturar.
- O `rsi-display-management` (serviço) só guarda o elemento e faz `dump`; **não chama o mss**. O
  **mss subscreve** o recurso RSI e reage: recompila a lista de streams e (re)dispara os grabs.
- Logo, o "redirect" **é setar a fonte na renderingSurface**: apontar o `name` da
  `navi-kombi-renderingsurface` para o `surfaceid://` do surface do app. Nada de mexer no compositor.

### Como o mss consome (RE 16/10/2026, Ghidra 12.1.4 via REA, binário do MU0883)

O mss tem um `MultiScreenController` que **espelha os 3 recursos RSI** (classes `Display`,
`RenderingSurface`, `StreamParameters`, cada uma com `fromJSON`) e os mantém em mapas. O coração é
**`MultiScreenController::prepareStreamsList`** (`@0x130028`), chamado quando um recurso muda. Para
cada `renderingSurface` do mapa ele:

1. resolve `getDisplayById(renderingSurface.display)` e
   `getStreamParametersById(renderingSurface.streamParameters)`;
2. só segue se a renderingSurface estiver listada no **display** (a lista em `Display+0x68`,
   `currentVisibleSurfaces`) — é o gate de visibilidade;
3. **extrai a fonte**: pega o campo `name` (`RenderingSurface+0x28`), normaliza (trim+lower,
   `getPreparedSurfaceId @0x12f278`) e compara os **12 primeiros chars** com `"surfaceid://"`
   (`@0x13ff90`, 12 chars exatos). Se casa, `substr(12)` → o `<id>` é o surface-fonte; senão, cai
   no `internalIdentifier` (`RenderingSurface+0x88`);
4. monta um `StreamInfo{ id, name, streamParametersId, sourceSurfaceId, ip, port, fps }`
   (ip/port/fps vêm do `StreamParameters`: `+0x70` IP, `+0x90` port, `+0x68` framerate) e, no fim,
   chama **`updateStreamsList`** (`@0x12f9e8`).

`updateStreamsList` → `ShareServer::startShareInternal` (`FUN_001175d0`, 838 linhas) →
`ivi_share_get_ivi_share_surface(<id>)` (o log diz `startShare [id = %d] ...`, **id inteiro**) →
`ShareContext::StreamerTask` encoda (shader RGBA→YUV + `OMX.qcom...avc`) e empurra RTP pro
`ip:port`. O `<id>` inteiro é o mesmo tipo de id do CLI da bancada (`-m 1000 30` = surface 1000).

Campos JSON confirmados no `RenderingSurface::fromJSON` (`@0x128168`): `name`, `internalIdentifier`,
`display`, `topLeftX`, `topLeftY`, `width`, `height`, `streamParameters`. `[DESMONTAGEM]`

### Quem publica a renderingSurface do mapa nativo (RE 16/10/2026, `libMIBNavSDClientLib_CarIF.so.00`)

O publicador é a classe **`Viwi::NavSd::us_displaymanagement`** (namespace VIWI = o REST/JSON da
VW sobre websocket). Constantes: `navi-kombi-display`, `navi-kombi-renderingsurface`,
`navi-kombi-streamparameters`. Sequência, toda no **registro/boot** (`registerDisplays @0x1dae40`),
**não em laço**:

1. `createNavKombiDisplay()` → cria o display `navi-kombi-display`.
2. **`createRenderingSurfaces()`** (`@0x1d7e38`) → **POST** em `.../displaymanagement/renderingSurfaces`
   com `name="navi-kombi-renderingsurface"`, `width/height` (do display kombi), `topLeftX/Y=0`,
   `display="navi-kombi-display"`. Guarda o header `Location` (a URI da RS) em `this+0x1a8`.
   **Repare:** o `name` do mapa nativo é `"navi-kombi-renderingsurface"`, **não** um `surfaceid://`.
3. `assingStreamingParametersToRenderingSurface(sp, rs)` [sic no binário] → liga o streamParameters.
4. **`assignRenderingSurfaceToMapRenderer()`** (`@0x1da3b8`) → pega o `MaprendererServiceHandler`,
   e **POSTa `{"renderingSurface":"<uri-da-RS>"}` no recurso do maprenderer** — é a associação que
   faz o renderizador nativo desenhar naquela RS (e o mss, no consumo, lê a fonte via
   `internalIdentifier`, já que o `name` não é `surfaceid://`). Depois chama **`startMapStreaming`**.

**Reconciliação (a pergunta que decidia o C):** quando o mapa aparece/some, o handler
`OnKombiMapRendererVisibilityChanged` (`@0x1dbbd4`) **não** recria a RS nem reescreve a fonte — ele
só chama **`ToggleStreamFramerate`** (`@0x1db808`), que faz **POST `{"framerate": N}` no
streamParameters**. O `N` vem de **`calculateFramerate`** (`@0x1d4a84`): **1 fps** (baseline/escondido)
ou **10 fps** (mapa plenamente ativo). **Nunca zera.** Ou seja: o carro **afunila** o stream nativo
pra 1 fps quando o mapa não está em foco, não o pausa. `[DESMONTAGEM]`

**Conclusões pro redirect:**
- A **fonte** da RS é ligada **uma vez**, no registro. Trocas de visibilidade só mexem no **fps** do
  streamParameters, não na fonte → um `onUpdate` nosso na fonte **cola** entre aparecer/sumir do mapa.
- **MAS** um reinício/reconexão do NavSd roda `registerDisplays` de novo e **reverte** pra associação
  nativa. E o `ToggleStreamFramerate` do carro pode baixar o fps da `navi-kombi-streamparameters` pra
  1 quando o nativo se julga "escondido" — se sequestrarmos a RS nativa, isso afunila o nosso stream.

## Dois caminhos pro C

1. **Direto (linha de comando do mss) — melhor pra teste:** `multi-screen-server <ip> <port> <-m/-u>
   <surface_id> <fps>` pega **um surface ivi por id** e transmite. É o que a bancada já exercita
   (`-m 1000 30`). C simples = grab do surface ivi do app → `ff14::2:4 : 42800`. Não depende da
   orquestração RSI nem sofre o afunilamento de fps do nativo.
2. **Orquestrado (RSI, como o daChillout) — entendido ponta a ponta:** duas sub-opções:
   - **Sequestrar a `navi-kombi-renderingsurface`:** `onUpdate` pondo a fonte no surface do app
     (`name=surfaceid://<id>` ou re-associando o maprenderer). Cola entre visibilidades, mas é
     revertido por um `registerDisplays` (reboot/reconexão do NavSd) e sofre o `ToggleStreamFramerate`.
   - **Criar uma RS + streamParameters SÓ nossos** (mesmo esquema do `createRenderingSurfaces`, com
     `name=surfaceid://<id-do-app>` e o `streamParameters` apontando pro RTP do cluster), deixando a
     nativa quieta. Mais robusto a reconciliação; é preciso garantir que só um stream alimente o
     endpoint do cluster (zerar/!desenhar o nativo, ou usar porta/endereço que o cluster escute).

## Bancada (primeiro incremento, 16/10/2026)

Começou a montagem do C (sub-opção **RS própria**) em [`camada3/`](camada3/): o **produtor**
(`redirect_producer.py`) cria `streamParameters` + `display` + `renderingSurface` nossas via REST,
com `name=surfaceid://<id-do-app>` e destino no RTP do cluster; um **mock** do `DisplayManagement`
(`rsi_mock.py`) serve de alvo; e um **verificador** (`prepare_streams_check.py`) replica fielmente o
`prepareStreamsList` do mss (RE `@0x130028`) para provar que o estado criado faz o mss grabear o
surface do app e transmitir. `sh camada3/run_demo.sh` roda tudo; confirmado
`fonte 1200 → ff14::2:4:42800 @ 30fps`. Detalhes e limites em [`camada3/README.md`](camada3/README.md).

Falta (próximo marco): dirigir o **binário real do mss** por RSI — hoje a bancada do mss roda em
modo CLI (`-m`). Precisa de RE curta do cliente `SAI::RsiClient` do mss (porta/descoberta via
service-registry + canal de subscribe/notify) e de subir um serviço que ele aceite (o
`rsi-display-management` real ou o mock evoluído para casar o protocolo de fio).

## RE do cliente RSI do mss — SAI / `librsic.so.0.1` (16/10/2026)

O mss não implementa o RSI; importa de **`librsic.so.0.1`** (SDK LG "SAI", build `librsic 1.0`, em
`/lge/app_ro/lib64/`), a mesma lib que a HMI e o `navsd_app` usam. Três mecanismos, todos `EXTERNAL`
no mss (REA/Ghidra 12.1.4, MU 0883):

- **Descoberta** — `SAI::RsiLookup::locationToRsiClientConnectOption(URI location)` resolve o
  *location* de um serviço (ex. `displaymanagement`) para `RsiClientConnectOption{host, port}` (port
  é uint16). Passa por um **Service Registry (SR)**: `WsSrObserver` assina o SR (log
  `Subscribed SR. condition=...`). Ou seja, o mss **não hardcoda** a porta do displaymanagement —
  pergunta ao registry.
- **REST** — `SAI::RsiClient` (`RsiClientGetRequest/PostRequest/DeleteRequest`,
  `setBody(JSON::Object)`) sobre HTTP (Poco::Net) no host:port resolvido, nos caminhos
  `/displaymanagement/{displays,renderingSurfaces,streamParameters}`.
- **Notificação** — `SAI::WsClient`/`WsClientImpl` por **WebSocket** `ws://host:port`. Mensagem é um
  JSON object (`WsClientMessage::parse` = Poco JSON). Tipos `TYPE_SUBSCRIBE`/`TYPE_UNSUBSCRIBE`;
  assinatura = `WsSubscribeCondition{event, update_limit, interval, authorization}` (auth default
  `NO_AUTH_TOKEN`); o server empurra o `data` na mudança e o `MultiScreenController` reage
  (`prepareStreamsList`). Logs úteis: `Creating WsClientImpl ws://%s:%hu`,
  `subscribe event=%s, update_limit=%d, interval=%d, authorization=%s`. `[DESMONTAGEM]`

É o protocolo **VIWI** da VW; análogo open-source: `wzr1337/rsi.server` + `wzr1337/rsi.serviceregistry`.

**O que o mock precisa ter pra dirigir o mss real:** (1) um **Service Registry** que o mss consulte
pra resolver `displaymanagement` → host:port (ou apontar o mss direto); (2) o **REST** dos 3
recursos; (3) um **WebSocket** VIWI (subscribe → ack; push de `data` na mudança). Plano: modelar pelo
VIWI/`rsi.server` e **fechar o envelope exato empiricamente** — subir o mss contra o mock e ler os
logs do `librsic` (`ws://host:port`, `subscribe event=...`, erros de parse), em vez de decompilar a
pilha VIWI inteira.

**Ainda em aberto (pro mock):** (a) o **bootstrap do SR** — onde o `RsiLookup` procura o registry
primeiro (porta fixa/known, config ou env); lê-se do primeiro `Creating WsClientImpl ws://...` ao
rodar o mss. (b) as chaves exatas do envelope e o nome do `event` que o mss assina para
`renderingSurfaces`.

### Bancada RSI — mss em modo RSI, empírico (16/10/2026)

Harness: `camada3/run_rsi.sh` + `camada3/rsi_incontainer.sh` (imagem `moi3-bancada:rsi` = bancada +
python3; o mock roda no mesmo container/netns, `--network none`, e o mss conecta no `127.0.0.1`).
Rodando o mss **sem args de surface** (modo RSI), sob `strace`:

- **O mss vai pra modo RSI sem args** (não só imprime Usage — tenta conectar).
- Ele conecta em **`127.0.0.1:49600`** (TCP) e fala **HTTP REST**: `GET /displaymanagement/displays`
  (e repete). Ou seja, o transporte RSI por baixo é **HTTP** (REST + WebSocket no mesmo porto). O
  **49600 é estável** entre execuções e **não aparece** em `rsiclient.properties`, `rsi-cdn.json` nem
  `rsiserver.properties` — é default do displaymanagement/`viwiproxy` (há um binário `viwiproxy` e o
  `MIB_ServiceRegistryApp-00` em `/lge/app_ro/sai/`). `[BANCADA]`
- Com o **mock no 49600**, o mss **conecta** (`connect = 0`) e o stack gráfico inicializa
  (`fakedrm`/`fakegbm`/`OMX_Init`) — chega longe. Mas ele fica **re-pedindo** `GET .../displays`
  porque nosso mock devolve lista **vazia**: falta semear o estado que o carro já tem no boot.
- Configs (`/lge/app_ro/sai/conf/`): `rsiserver.properties` (lado servidor) confirma **CORS**
  (`GET,HEAD,PUT,PATCH,POST,DELETE`, `allowOrigin=*`, expõe `Location`) e **WebSocket**
  (`server.websocket.enabled=true`, polling ~4 s); registry default `127.0.0.1:443`, clientid
  `VIWI_Transceive`. `rsiclient.properties` é só retry/ping. `rsi-cdn.json` é CDN de imagens
  (phone/media/radio), irrelevante. `[fw-static]`

**O gate do subscribe (RE + empírico).** O mss só assina depois que `MultiScreenController::check`
(`@0x12e980`) vê o serviço "up" via `Connector::checkConnection(PATH_TO_DISPLAYS)` (`@0x12c1e8`):
essa função faz o `GET /displaymanagement/displays`, parseia como **JSON object** e **retorna TRUE
só se houver a chave `"status"`** no objeto (testa existência, não o valor). Nossa resposta
`{"displays":[...]}` não tinha `status` → FALSE → nunca assinava. **Correção:** a resposta do GET
tem de ser um objeto com `"status"` (o mock passou a mandar `{"status":{...},"displays":[...]}`).
Com isso, `check` cria o `WsClient` (WS no mesmo host:port, path `/`) e **assina** displays,
renderingSurfaces e streamParameters com `NO_AUTH_TOKEN`. `[DESMONTAGEM]`

**Subscribe capturado (empírico, 16/10/2026).** Com o `status` na resposta, o mss abriu o WebSocket
(`GET /` + `Upgrade: websocket`) e mandou os 3 frames — o **envelope VIWI real**:

```
{"event":"/displaymanagement/displays#_rsi_1","type":"subscribe"}
{"event":"/displaymanagement/renderingSurfaces#_rsi_2","type":"subscribe"}
{"event":"/displaymanagement/streamParameters#_rsi_3","type":"subscribe"}
```

Ou seja: WS no path `/`, mensagem `{"event":"<path>#_rsi_N","type":"subscribe"}` (o `#_rsi_N` é o id
da assinatura). O **push de volta** (notificação) o mss processa em `MultiScreenController::onMessage`
(`@0x135078`): só mensagens com `type == WsClientMessage::FIELD_DATA`, `event ==` caminho do recurso
e payload **array** (coleção → `updateDisplays`/`updateRenderingSurfaces`/`updateStreamParameters`)
ou **objeto** (elemento → `update*Element`; streamParameters lê também `FIELD_TIMESTAMP`). `[DESMONTAGEM]`

**Fechamento do end-to-end (16/10/2026) — controle RSI OK, falta o data-plane.** Depois de uma série
de iterações empíricas contra o mss real, o **plano de controle está fechado**: o mss conecta, passa
o health-check, abre o WS, assina e **consome o modelo RSI inteiro**. O que destravou, em ordem (tudo
gravado no mock):

1. **Health-check:** o `GET /displaymanagement/displays` só é aceito se a resposta for um **objeto
   JSON com a chave `"status"`** (`Connector::checkConnection @0x12c1e8` só testa existência da chave).
   Sem ela, `MultiScreenController::check @0x12e980` nunca abre o WS nem assina.
2. **Roteamento do push:** o `WsSubscribeClient::broadcastMessage @0x160380` casa o dado recebido à
   assinatura pelo **session id**, que é o **fragmento `#_rsi_N`** do evento (o ctor
   `WsClientMessage @0x166580` parseia o `event` como URI, põe o fragmento no +0x158 e exige que
   comece com `SESSION_ID_PREFIX`; o caminho antes do `#` vai pro +0x50, comparado com PATH_TO_*).
   Então o `event` do push **tem de incluir o `#_rsi_N`** da assinatura.
3. **Shape do payload:** o `onMessage @0x135078` faz `getArray`/`getObject` na mensagem pela chave
   **`data`** — coleção → `data` = array; elemento → `data` = objeto. `type` = `"data"`.

Com isso o mss **cascateia**: assina as 3 coleções (`#_rsi_1..3`) e depois cada elemento
(`navi-kombi-display`/`-renderingsurface`/`-streamparameters`, `#_rsi_4..6`), e monta o pipeline
EGL/GL + o cliente `ivi_share`. `[BANCADA]`

### Contrato VIWI e data-plane do mss — RE completa (16/10/2026, REA/Ghidra 12.1.4, binário MU0883)

RE de ponta a ponta de `Display/RenderingSurface/StreamParameters::fromJSON`, `Display::extractIds
@0x125120`, `onMessage @0x135078`, `prepareStreamsList @0x130028`, `getPreparedSurfaceId @0x12f278`,
`updateStreamsList @0x12f9e8`, o ctor `MultiScreenController @0x12f4a8`, o `main @0x1106a0` e o
`startStream` (vtable do `IRSIEventHandler`, `@0x11b350`). Fecha o modelo que o **C** precisa emitir.

**Contrato VIWI (confirmado por chave→offset no `fromJSON`).** Associação entre recursos é **objeto
`{"id": "..."}`, não string** (o mss faz `getObject` → `.id`; string crua lança `NullPointerException`
dentro do ctor e o objeto nem entra na árvore). Campos:

| recurso | chave → forma | offset no objeto |
|---|---|---|
| RenderingSurface | `id` (string, obrigatória) | +0x8 (é a **chave** da árvore e o valor do gate) |
| | `name` (string; se começa com `surfaceid://` define a fonte) | +0x28 |
| | `uri` (string) | +0x48 |
| | `display` = **objeto** `{"id"}` → `.id` | +0x68 |
| | `internalIdentifier` (string; fonte-fallback) | +0x88 |
| | `streamParameters` = **objeto** `{"id"}` → `.id` | +0xa8 |
| | `topLeftX/topLeftY/width/height` (int, obrigatórias) | ints |
| Display | `id`,`name` (string) | +0x28, +0x48 |
| | `renderingSurfaces` = **array de objetos** `{"id"}` (`extractIds`) | vetor em **+0x68** |
| | `currentVisibleSurfaces` = **array de objetos** `{"id"}` (`extractIds`) | vetor em +0x80 |
| | `width`,`height` (int) | +0x98, +0x9c |
| StreamParameters | `id`,`name`,`uri` (string) | +0x8, +0x28, +0x48 |
| | `ip` (string — destino RTP; o gate exige não-vazio) | +0x70 |
| | `framerate`,`port` (int) | +0x68, +0x90 |

**O GATE é `display.renderingSurfaces`, NÃO `currentVisibleSurfaces`** (correção da hipótese antiga).
`prepareStreamsList`, pra cada RS: exige `display` e `streamParameters` não-vazios, resolve
`getDisplayById(rs.display.id)` e `getStreamParametersById(rs.streamParameters.id)`, e faz `find_if`
do **`rs.id`** dentro do vetor `display+0x68` = **`renderingSurfaces`** (e `sp.ip != ""`). A **fonte**
do grab é `getPreparedSurfaceId(rs.name)` (trim+lower): se começa com `surfaceid://` → fonte =
`name[12:]`; senão → `internalIdentifier`. Precondição de 3 árvores: `displays`, `renderingSurfaces`
e `streamParameters` todas não-vazias. `[DESMONTAGEM]`

**Root cause das rodadas vazias (exp1..9):** o seed modelava `display`/`streamParameters` como
**strings** e o display **não tinha `renderingSurfaces`** (só `currentVisibleSurfaces`, e como array de
strings). Resultado: `RenderingSurface::fromJSON` e `Display::fromJSON` lançavam exceção (objeto nunca
entrava na árvore) **e** o vetor do gate ficava vazio. Corrigido em `seed_navi_kombi.json` (associações
viraram `{"id"}`, `display.renderingSurfaces` passou a existir com o id da RS). `[BANCADA]`

**Prova do data-plane (oráculo fiel).** `prepare_streams_check.py` (réplica 1:1 da RE, agora com o gate
`renderingSurfaces` e associações `{"id"}`) sobre o seed corrigido **emite o stream esperado**:
`sourceSurfaceId=1000` (do `internalIdentifier`, pois `name` não é `surfaceid://`) → `ip=ff14::2:4`,
`port=42800`, `framerate=30`. Ou seja, **o dado e a lógica estão corretos**: `prepareStreamsList`
montaria o stream. `[BANCADA]`

**Pipeline completo do grab (RE).** `onMessage` → `updateDisplays/RenderingSurfaces/StreamParameters`
(populam as árvores + cascateiam subscribe de cada elemento) → `prepareStreamsList` → `updateStreamsList
@0x12f9e8`. Este, pra cada stream **novo** (diff por `StreamInfo::compare`) e com `0 < framerate`,
chama o handler `this+0x58` (o `IRSIEventHandler`, setado no ctor) via **vtable+0x10 = `startStream
@0x11b350`**, que loga `"START STREAMING"` e chama **`FUN_001175d0`** (`startShare` — o grab de
`ivi_share` + Rgba2yuv/OMX + RTP). O `main @0x1106a0` com `argc<6` sobe em **modo RSI** (porta
`0xc1c0`=49600) e cria esse handler ligado ao stream-server real (DRM/gbm/OMX/Wayland) — ou seja, o
handler **existe** no bench. `[DESMONTAGEM]`

**`uri` é OBRIGATÓRIO em todo objeto (achado do gdb, 16/10/2026).** O `*/fromJSON` lê os campos em
ordem: `id`, `name`, **`uri`**, depois `display`/`internalIdentifier`/ints/`streamParameters` (e, no
Display, `renderingSurfaces`/`currentVisibleSurfaces`). Se o objeto **não tem `uri`**, o parse **para
logo depois de `name`** e deixa todo o resto vazio (o objeto entra na árvore só com `id`+`name`). Com
isso o laço por-RS do `prepareStreamsList` pula a RS (display/sp não resolvem, `renderingSurfaces` do
display vazio) e **nenhum stream é emitido**. Então todo objeto POSTado precisa de `uri` e de campos
não-vazios.

**End-to-end FECHADO no mss (bench sob gdb, 16/10/2026).** Com o seed/produtor corrigidos (associações
`{"id"}`, `display.renderingSurfaces` com o id da RS, e **`uri` em todos**), rodando o mss **real** sob
gdb (imagem `moi3-bancada:rsidbg`, breakpoints em `updateStreamsList`/`prepareStreamsList`):

- `prepareStreamsList`: as 3 árvores cheias (`displays=1 RS=1 sp=1`) **e o objeto RS completo** —
  `display='navi-kombi-display'`, `internalId='1000'`, `sp='navi-kombi-streamparameters'`;
  `display.renderingSurfaces=['navi-kombi-renderingsurface']`; `sp.ip='ff14::2:4'`. `[BANCADA]`
- `updateStreamsList: stream_list_size = 1` — a lista deixou de ser vazia. `[BANCADA]`
- `wlstub`: **`ivi_share.get_ivi_share_surface id 7 for surface_id 1000`** → o mss **grabeou** o surface
  do kombi, recebeu `configure 1280x480 XR24` e registrou damage periódico. `[BANCADA]`
- `glhost`: **importou o buffer grabeado** (`import_000.ppm 1280x480`), rodou o shader de conversão
  (`IMG_CREATE XR24`, `glDrawArrays`, `output_draw_000.ppm`). `[BANCADA]`

Ou seja: subscribe → consume → **grab (ivi 1000)** → import → draw. Controle E data-plane provados
contra o binário real.

**Único resto (limitação do stub gráfico, não do mss/dado):** `glhost` fica em **0 quadros
codificados** por um `GL_INVALID_OPERATION` em `glUniform1i` ("program not linked") — o programa do
shader Rgba2yuv não linka no `glhost`/Mesa llvmpipe da bancada, então não há H.264/RTP. É assunto do
`glforward-moi3` (stub de GL), separado do RSI/C. O grab + import + draw já provam o caminho.

## H.264/RTP no bench — viabilidade (RE 16/10/2026, não bloqueia o C)

Investigado a fundo (gdb + RE de `Rgba2yuv::init @0x121ec8`, `convert @0x122548`, `fakeomxcore.c`).
**Não é um ajuste de shader; são dois blocos de infra nova.** O grab já é a prova do trabalho de C/RSI.

1. **Encoder é stub de 0 bytes.** O H.264 real é o **Qualcomm Venus** (`OMX.qcom.video.encoder.avc`,
   V4L2 `/dev/video33`) — só roda no SoC. Na bancada é o `fakeomxcore.c`, que na `FillBufferDone`
   devolve **`nFilledLen = 0`** (acusa o buffer, não codifica). Logo **não sai H.264 real** por mais
   que o resto funcione. Para RTP de verdade seria preciso **escrever um encoder de software** no
   `fakeomx` (ex.: openh264/x264 aarch64 estático) — componente novo.
2. **Convert (Rgba2yuv) não está nem rodando.** `program/vert/frag` são globais; `Rgba2yuv::init`
   cria o shader uma vez (`if (program==0)`). No run do bench **nenhuma op de `init`**
   (`glGenFramebuffers`/`glCreateProgram`/`glLinkProgram`…) chegou ao glhost → `init` não rodou →
   `program=0` → `convert` desenhou com program 0 (daí o `glUseProgram(0)` e o `glUniform` em
   program-not-linked). Além disso o shader exige **`GL_OES_EGL_image_external_essl3` + `samplerExternalOES`**
   (textura externa OES do EGLImage) — incerto se o Mesa **llvmpipe** da bancada compila isso mesmo
   com `init` rodando. Faltam RE: por que `convert` é chamado mas `init` não (ver `convert @0x122548`
   e o caller `FUN_001175d0`).

**O muro real (RE 17/10/2026): o handshake de buffer do encoder Venus, não o encoder.** Rodando o mss
em `--buf-memcpy` (sem GL) sob strace: ele leva o OMX até **Executing** (state 1→2→3, com todo
Set/GetParameter e os vendor params Qualcomm `0x7f000002/07/11/2a/39`), **importa 562 `gbm_bo`** (os
frames grabeados), mas **NUNCA aloca buffer OMX** (`UseBuffer`/`AllocateBuffer` = 0) **nem chama
`EmptyThisBuffer`** (0), e **não manda RTP**. O `fakeomxcore.c` até tem o caminho `want_idle`/
`buffers_ok` pra completar `Loaded→Idle` só com buffers alocados, mas o `SendCommand` o desvia e
completa na hora — porque (comentário no próprio arquivo) o mss **não aloca pelo caminho padrão**. O
mss usa o **caminho nativo/metadata da Qualcomm** (registra os `gbm_bo` como buffer do encoder via
vendor params), e não abre `/dev/video33`/`/dev/ion` (stub). Então ele fica travado no handshake antes
de alimentar o encoder.

**Handshake de buffer do encoder — MAPEADO (RE 17/10/2026, opção "mapear").** O objeto encoder
(`omx_encoder_create` → ctor `FUN_0011eb88`: handle 0x40 + estado 0x230 + 5 métodos) tem o método
principal **`0x11fe30`** (`método[0]`, via thunk `0x1214f8`), que faz TODO o OMX. Fluxo (por logs
`[omx_encoder]`):
1. `OMX_GetHandle("OMX.qcom.video.encoder.avc")`; configura porta IN (PortDefinition: NV12,
   `OMX_COLOR_FormatYUV420SemiPlanar`) e OUT (AVC); profile/level H264; frame-packing.
2. `GetParameter(PortDefinition)` lê `nBufferCountActual`; conta ponteiros não-nulos num struct
   **`UseBufferData`** (0x90 bytes passado ao `omx_encoder_create`, copiado em `estado+0x160`) →
   `uVar34`. `num_in_buffers = max(nBufferCountActual, uVar34)`.
3. `SendCommand(Idle)`; aloca buffers de entrada: **se `uVar34>0`** → `UseBuffer` com os buffers
   **pré-providos** (os `gbm_bo` grabeados, zero-copy); **se `uVar34==0`** → abre `/dev/ion` (ION
   alloc), e se falhar → `gbm_bo_create` + `Rgba2yuv::init` + mmap + `UseBuffer`. Depois `UseBuffer`
   na porta OUT.
4. `SendCommand(Executing)`; `FillThisBuffer` nos buffers de saída; por-frame: grab → convert →
   `EmptyThisBuffer(IN)`; o encoder devolve NAL via `FillBufferDone(OUT)` → o **callback `param_6`**
   do `omx_encoder_create` RTP-envia.

**É OMX PADRÃO — não metadata proprietário.** Buffer = `UseBuffer` com memória ION/gbm ou buffers
pré-providos (`UseBufferData`). Isso torna a emulação **viável** (o `fakeomx` já tem `UseBuffer`/
`EmptyThisBuffer`/`FillThisBuffer`).

**O que trava no bench:** o mss aloca **ZERO** buffer de entrada (0 `/dev/ion`, 0 `gbm_bo_create`,
0 `UseBuffer`, 0 ioctl no strace) → `num_in_buffers` resolve pra 0 (nem o caminho `UseBufferData` nem o
ION/gbm disparam) → chega a Executing sem buffer → nunca `EmptyThisBuffer`, nunca RTP. Falta pinar o
gate exato (1 run de gdb em `0x11fe30`, inspecionar `num_in_buffers`/`uVar34` após o `GetParameter`).

**GATE PINADO (gdb, 17/10/2026): era um bug de UM valor de enum OMX.** Quebrando em `omx_encoder_log`
(`0x11ee40`) sob gdb, o fluxo real apareceu com valores: `allocating Input buffers [num_in_buffers=0]`,
`frame_bytes=0`, `nBufferSize=0`, e depois `num_out_buffers=0`. Causa: o `fakeomxcore.c`/`omx_il.h`
definia **`OMX_IndexParamPortDefinition = 0x02000003`**, mas o mss usa **`0x02000001`** (confirmado:
ele faz `Get/SetParameter(0x2000001)` pra configurar as portas e ler w/h/count/size). Como não casava,
o `if (idx == OMX_IndexParamPortDefinition)` do `fakeomx` nunca rodava o `fill_portdef` → o mss lia
count/size = 0 → alocava zero buffers. (O encoder OMX nunca tinha sido exercitado — a bancada anterior
era a HMI, que não usa encoder; o valor errado passou batido.)

**Fix aplicado e CONFIRMADO** (`omx_il.h`: `0x02000003`→`0x02000001`): o `fakeomx` passou a casar
`PortDefinition`; o mss negocia IN=NV12 1280×480 sz 921600 / OUT=H264 sz 614400, faz `UseBuffer`×2 (IN)
+ `AllocateBuffer`×2 (OUT), `Idle`→`Executing`, e prima a saída (`FillThisBuffer`×2). **O handshake
destravou** — a parte "assustadora" era um bug trivial.

**Feed — modelo CORRIGIDO (RE estática completa, REA, 16/10/2026 — supera a tese do "`0x320` semáforo"
das notas "17/10", que eram gdb parcial + UTC).** O feed tem **três threads** sobre o mesmo stream-state
`S`, mais os callbacks OMX:
- **Produtor — `cb_damage` (`0x115860`)**, callback de frame do Wayland. Importa o `gbm_bo` e, **se**
  `frame(S+0x268)==ack(S+0x264)` (= o streamer já consumiu o último capturado), copia o frame pro
  *pass buffer* `S+0x3e8`(=1000) (memcpy em `--buf-memcpy`; GLES/`Rgba2yuv::convert`@S+0x78 senão),
  incrementa o produzido `S+0x3f8` e faz `S+0x268 = S+0x3f8`. A matemática das flags `pass`(S+0x260)/
  `sinal`(S+0x261): `iVar1=*(int*)(S+800)`; `iVar1==0`→pass=1,sinal=1; senão se `frame==ack`→pass=1,
  sinal=0; senão→pass=0,sinal=0. `cb_damage` só sinalizaria `S+0x248` no caso `iVar1==0`.
- **Pacer — `EventSourceTask` `FUN_00112640`** (AsyncTask #1 embutida em `S+0x230`). Dorme o período-alvo
  (`S+0x270`), mede FPS real vs alvo (`S+0x26c`), grava `S+0x320` em 1/2 e **`Condition::signal(S+0x248)`**
  no ritmo-alvo. **É o pacer, não o `cb_damage`, que acorda o feeder** em regime normal.
- **Feeder — `StreamerTask` `FUN_00113910`** (AsyncTask #2 embutida em `S+0x400`). Bloqueia em
  `Condition::wait(S+0x248)`. Ao acordar: trava `S+0x250`, confere o **gate** = `W(S+0x30)!=0` &&
  `passbuf(S+0x3e8)!=0` && `size(S+0x3f0)!=0` && `W->is_ready()`; passando, grava `ack S+0x264 = S+0x3f8`
  e chama `W->vtable[+0x18]` = **`FUN_0011f220`** (read_and_empty) com (passbuf, size, stride).
- **`FUN_0011f220`** copia o pass buffer pra um header de entrada livre (array `E+0x78`) e dispara a
  **`EmptyThisBuffer` física** `(**(code**)(*(E+0x20)+0x78))(…)`, faz `E+0x14++` e espera `E+0x200`.
  `EmptyBufferDone` (`FUN_0011ece0`) faz `E+0x11=1` + `signal(E+0x200)` → libera o feeder.
  `FillBufferDone` (`FUN_0011f130`) manda o NAL por `omx_encoder_create` callback (RTP) e re-enfileira
  a saída (`FillThisBuffer` = `*(E+0x20)+0x80`).

**`S+0x320` NÃO é semáforo — é estado de regulação de FPS de 3 valores {1,2,3}** (1=abaixo do alvo,
2=acima, 3=parada). Nasce **2** no `startShare`; os dois tasks (controlador bang-bang) o reescrevem em
1/2; teardown=3. **Ninguém escreve 0**, logo o ramo `iVar1==0` do `cb_damage` (free-run) é inalcançável em
regime, e `EmptyBufferDone`/`FillBufferDone` **não** tocam `S+0x320`. ⇒ **A tese anterior ("`0x320` tem de
cair a 0 pro feed começar") estava ERRADA**; o gdb antigo (preso em 1) só via o controlador decidindo
"abaixo do alvo" — normal, não é bloqueio. `ack=S+0x264`, `frame=S+0x268` ambos 0 no gdb = o feeder
**nunca executou o feed** (nunca gravou `ack=S+0x3f8`).

**`is_ready()` (`W->vtable+0x8` = `FUN_0011ed80`) = `E+0x10 && E+0x11`.** No init (`FUN_0011fe30`):
`E+0x18=1` cedo (linha 159; só zera no caminho de erro de HW), e no fim (809-811) `if(E+0x18){E+0x11=1;
E+0x10=1;}`. ⇒ **init completo ⇒ ready=true.** `EventHandler` (`FUN_0011f5f0`): `OMX_EventCmdComplete`/
`StateSet` grava `E+0x1c=estado` e `signal(E+0x220)` — a condição que o **init espera** (linhas 782/788)
antes de primar a saída. Então o init só avança até `FillThisBuffer` se o `fakeomx` entregar os
`EventCmdComplete` (Idle, Executing); como o bench já mostrou `FillThisBuffer×2`, o init **passou** as
esperas → `ready` deve estar true. `OMX_EventError` → `signal(E+0x200)`; erro crítico → `E+0x10=0`,`E+0x18=0`,`exit(1)`.

**Novo diagnóstico do bloqueio (a verificar no gdb, instrumentado pro modelo certo):** como o feed é
dirigido pelo **pacer sinalizando `S+0x248`** + o **gate do feeder**, o `EmptyThisBuffer=0` do bench vem
de uma destas, em ordem de suspeita: (1) o **pacer `FUN_00112640` não roda / não sinaliza** (as AsyncTasks
#1/#2 não foram iniciadas no bench); (2) o **gate do feeder falha** num operando — mais provável o
**pass buffer `S+0x3e8`/`S+0x3f0`** vazio (se o `cb_damage` não chegou a copiar) do que `is_ready`
(que o init deve ter ligado). Próximo gdb: breakpoint em `FUN_00113910` (entra? acorda?), `FUN_00112640`
(sinaliza?), ler os 4 operandos do gate quando o feeder acorda, e `FUN_0011f220` (chega?).

**O que falta até o RTP (inalterado no destino, corrigido no mecanismo):** `fakeomx` precisa se comportar
como encoder — entregar os `EventHandler(EventCmdComplete, StateSet, estado)` (destrava init→ready),
no `EmptyThisBuffer` chamar `EmptyBufferDone` (libera o input), codificar NV12→H.264 (openh264) e
devolver a saída com `nFilledLen>0`+flags via `FillBufferDone` → o callback manda RTP. **Não há "`0x320`
zerar"** no caminho. Multi-hora, infra de bancada, zero relação com o C/RSI (provado pelo grab).

**Destravamento do feed — gdb instrumentado no modelo certo (17/10/2026). Bloqueio rastreado em 3 níveis.**
Com breakpoints no laço do feeder (`FUN_00113910`: gate entry `0x113b14`, ramo not-ready `0x113e28`, feed
`0x113b7c`) e nos dois threads:
- **Threads vivos:** pacer (`FUN_00112640`) e feeder (`FUN_00113910`) RODAM; o feeder **acorda e reavalia o
  gate a cada sinal** (gate#1..8). Logo não era "pacer não sinaliza".
- **Nível 1 (passbuf):** `gbm_perform` era no-op (op 8 → `size` não calculava; op 6 → mmap não devolvia
  ponteiro) ⇒ `cb_damage` nunca copiava ⇒ `passbuf=0`. **CONSERTADO** em `shims/fakegbm.c`: `gbm_perform`
  op 8 retorna não-zero, op 6 mapeia `bo->size` legível (dma-buf real se o fd via `lseek` for ≥ `bo->size`,
  senão página anônima zerada = frame preto; `lseek` porque a glibc 2.23 do FW **não exporta `fstat`** — ele
  inlina pra `__fxstat` e dá `symbol lookup error`), op 7 `munmap`. Depois do fix: `passbuf≠0 size=2457600
  prod=1` no gdb. ✓
- **Nível 2 (is_ready):** com `W/passbuf/size` OK, o gate ainda falha em **`is_ready()` = `E+0x10 && E+0x11`
  = 0,0** (`E=*(W+0x30)`). `state(E+0x1c)=3` (Executing) prova que é o MESMO `E` que o EventHandler escreveu.
- **Nível 3 (E+0x18):** o init (`FUN_0011fe30`) seta `E+0x18=1` incondicional na entrada (linha 159) e no fim
  liga `E+0x10/0x11` só se `E+0x18`. Mas `E+0x18=0` em runtime ⇒ o init bateu no caminho de erro
  `LAB_00121038` (`omx_encoder_log "error [res=0x80001001]" ... init_omx:0x4d8`), que faz `E+0x10=0; E+0x18=0`
  sem `exit`. Esse erro está na **alocação do buffer de ENTRADA do encoder** (`omx_encoder_bo_malloc`), que
  tem dois caminhos conforme `E+0x210`: **(a) ION** — `open("/dev/ion")` + ioctls `0xc0204900/0xc0084902` —
  inexistente fora do SoC; **(b) `gbm_bo_create` + `Rgba2yuv::init`** — o `gbm_bo_create` o fakegbm dá, mas o
  `Rgba2yuv::init` é GL (o mesmo shader que falha no glhost/Mesa, `glUniform1i program-not-linked`). **Os dois
  caminhos esbarram em infra que o bench não emula (ION allocator OU GL).** Essa é a parede real do
  `--buf-memcpy` também: o encoder exige buffers de entrada ou via ION ou via GL/Rgba2yuv.

**Conclusão de escopo:** destravar o feed de verdade exige emular mais uma camada do stack Qualcomm — um
**/dev/ion fake** (open+ioctls ION_ALLOC/ION_MAP) OU fazer o `Rgba2yuv::init` (GL) funcionar no glhost/Mesa —
e só então o `fakeomx` codificar pra sair RTP. Cada conserto revela a próxima camada (índice OMX →
`gbm_perform` → ION/Rgba2yuv). Progresso real desta frente: pass buffer agora flui, feeder acorda, bloqueio
localizado na linha exata. Segue 100% infra de bancada, sem relação com o C/RSI (provado). **Achado extra:
em `--buf-memcpy` o glhost recebe 0 chamadas de GL** ⇒ nesse modo o init toma o caminho **ION**; a rota GL
só é exercida em **modo GLES** (sem `--buf-memcpy`).

**Decisão do Lipe (17/10/2026): rota GL (Rgba2yuv).** Entre fake /dev/ion, rota GL e parar, escolheu fazer o
`Rgba2yuv::init` (shader GL) linkar no glhost/Mesa, pra o caminho `gbm_bo_create`+`Rgba2yuv` do
`omx_encoder_bo_malloc` completar → `E+0x18=1` → `is_ready` → feed (depois ainda falta o fakeomx-encoder pro
RTP). Em andamento: rodar em modo GLES e capturar o erro exato do shader (glhost loga `SHADER/PROGRAMA NÃO
COMPILOU/LIGOU` com fonte e info log em `after_op`).

**Rota GL — progresso (17/10/2026, modo GLES, porta 49600). Dois marcos + a parede funda.**
- **MARCO: o `EmptyThisBuffer` dispara em modo GLES.** Com gdb no feeder: `is_ready(E+0x10,0x11)=1,1`,
  `E+0x18=1`, `state=3`, e `*** FED: read_and_empty` + `*** EmptyThisBuffer site`. Ou seja, o bloqueio do
  "`EmptyThisBuffer=0`" era **exclusivo do `--buf-memcpy`** (caminho ION, que a bancada não tem); em GLES o
  encoder fica ready e o feed roda. (Toda a RE do `0x320`/trigger perseguia o modo errado.) `passbuf=` handle
  de EGLImage, `size=8` (caminho z-copy).
- **MARCO: shader do Rgba2yuv compila/linka.** Causa do fail: o fragment shader (`#version 300 es`) tinha
  `precision highp uint;`, **inválido em GLSL ES 3.00** (o Adreno tolerava, o Mesa estrito rejeita). Conserto
  em `glforward-moi3/host.c` (handler `OP_glShaderSource`): branqueia qualquer linha com `precision`+`uint`
  antes de compilar. Depois: shader compila, programa linka, somem os erros de `glUseProgram/glUniform`.
- **PAREDE FUNDA (atual): render-to-external não existe no Mesa.** Com o shader OK, o FBO fica incompleto:
  `glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textarget=0x8d65=GL_TEXTURE_EXTERNAL_OES,
  tex=2)` → Mesa `GL_INVALID_ENUM: unknown textarget`. O Rgba2yuv converte **renderizando para dentro de uma
  textura EXTERNA (EGLImage)** = o buffer NV12 de saída do encoder (render-to-EGLImage, válido no Adreno). O
  **Mesa llvmpipe não aceita `GL_TEXTURE_EXTERNAL_OES` como alvo de FBO** (externa é só amostragem). FBO
  incompleto → `glDrawArrays` `GL_INVALID_FRAMEBUFFER_OPERATION` (663×) → 0 quadros. Também um
  `glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, …)` com "no renderbuffer bound" (secundário).
- **Para vencer:** o glhost tem de **emular render-to-external** — ao ver `glFramebufferTexture2D` com
  `GL_TEXTURE_EXTERNAL_OES`, substituir por uma `GL_TEXTURE_2D` RGBA normal como alvo, renderizar, e **copiar
  o resultado de volta** pro gbm bo/EGLImage que o encoder lê (hook pós-`glDrawArrays` já existe:
  `moi3_dump_fbo("draw")`). É a camada mais funda; e mesmo depois dela o `fakeomx` ainda precisa codificar NV12
  →H.264 pro RTP. Diag temporário em `host.c` (`handle_manual` loga args de `glRenderbufferStorage`/
  `glFramebufferTexture2D`) — reverter ou manter conforme a decisão de seguir.

**Lipe: "Vamos construir" (17/10/2026). Passo 1 da emulação — FEITO, conversão GL roda no Mesa.**
Implementado no `glforward-ext/apply.py` (não no `host.c` base; o estado de EGLImage vive lá): cada EGLImage
importada já tem uma `GL_TEXTURE_2D` de respaldo no glhost (criada do blob em `OPX_IMG_CREATE`); agora o
glhost guarda `g_img_tex[handle]` e, no `OPX_IMG_TARGET`, mapeia a textura externa ligada → respaldo
(`g_ext_backing[extTex]`, via `glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES)`). Um patch no handler gerado
de `glFramebufferTexture2D` troca `textarget=GL_TEXTURE_EXTERNAL_OES` pela `GL_TEXTURE_2D` de respaldo
(renderável) e guarda `g_fbo_out_handle`. O diag do `host.c` base foi revertido; o conserto do shader fica.
**Resultado (GLES, porta 49600):** somem os erros de `glFramebufferTexture2D`/`glDrawArrays`; log
`FBO render-to-external: ext 3 -> backing 2D N` a cada frame; **`dump draw: maxpx=255 nz=1152000 readerr=0`**
= o shader converteu RGBA→NV12 de verdade na textura de respaldo. `EmptyThisBuffer` segue disparando. Resta
o `glRenderbufferStorage`("no renderbuffer bound") — secundário (era depth/stencil; o color renderizou).
**É a conquista central da rota GL: a conversão funciona no Mesa.**

**Passo 2 (em construção): write-back.** O NV12 renderizado está na textura do glhost, não no bo que o
encoder lê — falta copiar de volta. Plano: op `OPX_READBACK` (glhost devolve `g_fbo_out_handle` + pixels via
`glReadPixels`), o shim mapeia `handle→(fd,w,h,stride)` no `eglCreateImageKHR` e, num ponto de sync
(`glFinish`), lê de volta e escreve no dma-buf do bo (mmap PROT_WRITE). Risco: layout/tamanho NV12 no alvo vs
o bo (cópia limitada ao tamanho do bo). Depois, o `fakeomx` ainda precisa ler o bo e codificar (openh264) →
`FillBufferDone` → RTP.

**Passo 2 — FEITO e VERIFICADO ponta a ponta (17/10/2026).** Implementado: op `OPX_READBACK` (glhost
`glReadPixels` do FBO atual → devolve `g_fbo_out_handle`+w+h+size+pixels); shim guarda `g_oimg[handle]=
{fd,w,h,stride}` no `eglCreateImageKHR` e, no `glFinish`, chama `moi3_writeback` (mmap PROT_WRITE do fd do bo
+ `R_copy`). **Prova:** `READBACK handle=1 1280x480 sz=2457600 glerr=0` + o `fakeomx` loga no input
`EmptyThisBuffer filled=942080 alloc=2457600 nz=432000` — o bo de entrada do encoder tem o NV12 convertido
(no z-copy o input do OMX É o bo; o mss não re-memcpy porque `E+0x210!=0`). **Cadeia inteira provada:
grab → convert GL no Mesa → write-back → input do encoder com conteúdo real.**

**Último passo (o encoder) — restrição achada: a saída OMX é 614400 bytes** (`AllocateBuffer port=1`), menor
que um frame NV12 (921600). Logo a saída tem de ser **comprimida**: I_PCM (H.264 cru, ~920 KB) NÃO cabe.
Opções: (a) encoder real (openh264/x264) no `fakeomx` — comprime de verdade, cabe, mas é ABI-difícil no shim
`-nostdlib` contra glibc 2.23 do FW (C++/libstdc++ pra openh264; x264 é C mas é um build grande cross pra o
sysroot); (b) **stream H.264 comprimido fixo** (SPS+PPS+IDR pré-computado de um frame sólido/pequeno, <614400)
emitido a cada `FillThisBuffer`/`EmptyThisBuffer` → prova o RTP ponta a ponta (conteúdo estático, não o frame
convertido); (c) dividir um frame maior em 2 buffers de saída (mais complexo). Diag no `fakeomxcore.c`
(`c_EmptyThisBuffer` loga nz do input) — manter ou reverter conforme a escolha.

**Lipe: "x264 via daemon no host" (17/10/2026). Encoder real — FEITO, H.264 chega ao sender RTP do mss.**
Implementado:
- **Imagem** `moi3-bancada:rsidbg-enc` (`camada3/Dockerfile.enc` = `FROM rsidbg` + `python3-av`; PyAV 11 +
  libx264). x264/openh264 não linkam no shim; o encoder real roda no container e o fakeomx encaminha.
- **Daemon** `camada3/enc_daemon.py` (PyAV/libx264, all-intra gop=1, zerolatency, crf28): unix socket,
  req `ENC0`+w+h+size+NV12 → resp nal+flags+H.264 Annex-B. Cada frame é um IDR independente.
- **fakeomx** (`shims/fakeomxcore.c`): worker reestruturado — J_FTB guarda os buffers de saída primados
  numa fila; J_ETB pega uma saída livre, chama `enc_frame` (socket → daemon → H.264), preenche a saída
  (`nFilledLen`, `nFlags`=ENDOFFRAME|SYNCFRAME), `FillBufferDone`, e `EmptyBufferDone` do input. Socket via
  FW libc (socket/connect/read/write existem; ao contrário de `fstat`). `MOI3_ENC_SOCKET` do ambiente.
- **Harness** (`gdb_incontainer.sh`): sobe o daemon em `$LOW/tmp/moi3-enc.sock` e passa
  `MOI3_ENC_SOCKET=/tmp/moi3-enc.sock` ao mss.
- **PROVA (GLES, gdb):** `enc.log` e o fakeomx logam `encoded frame #N -> ~195528 bytes (idr=1)` (frame #1
  375140 B), 240+ frames em 30s, sem crash; e o probe no **`FillBufferDone` do mss (0x11f130)** lê
  `nFilledLen=195528` (>0) a cada frame ⇒ o mss recebeu o H.264 real e invoca o sender RTP. Os NALs cabem
  nos 614400 do buffer de saída. **Cadeia completa: grab → convert GL (Mesa) → write-back → input NV12 →
  fakeomx → x264 → H.264 → FillBufferDone → RTP do mss.**

**RTP PROVADO NO FIO (strace, 17/10/2026) — OBJETIVO ALCANÇADO.** `strace -f -e trace=%network` do mss:
`sendto(9, "\x80\x21...", 1140, MSG_DONTWAIT, {AF_INET6, sin6_port=htons(42800), "ff14::2:4"}, 28) = 1140`
— **11670 chamadas** em 20s, cada `sendto` **retornando 1140 (sucesso)**, destino exato do seed
(`[ff14::2:4]:42800`), payload com cabeçalho **RTP v2** (`0x80`) PT=33 (MPEG-TS) ⇒ o mss embrulha o H.264 em
MPEG-TS sobre RTP. **A bancada emite H.264/RTP real.** (O receptor `rtp_rx.py` não capturou por nuance de
entrega multicast no `lo` isolado — rota `ff00::/8 dev lo` via `ip` da imagem enc; o `sendto=1140` no strace
é a prova direta do envio. Toggle `STRACE=1` no `gdb_incontainer.sh`.) Infra: imagem `rsidbg-enc` (+strace,
iproute2); `camada3/rtp_rx.py` (sobe lo via ioctl). **Cadeia inteira na bancada, do grab do kombi ao pacote RTP.**

**Confirmação visual + achado de fidelidade (17/10/2026).** Com `ENC_DUMP=/out/stream.h264` o daemon grava o
Annex-B; o PyAV decodifica como **H.264 válido 1280×480** — prova final de que é H.264 de verdade. **MAS o
conteúdo do frame está embaralhado** (ruído em grade, emenda vertical em x=640), não o cluster. Causa: **layout
NV12 no handoff GL→encoder**. O `moi3_writeback`/`OPX_READBACK` faz `glReadPixels(GL_RGBA)` do viewport inteiro
**1280×480 = 2457600 B**, e o daemon trata os primeiros 921600 B como NV12. Mas o Rgba2yuv empacota o NV12 num
alvo de dimensões diferentes (921600 B = 230400 px RGBA, ex. 1280×180 ou 640×360) e o `glReadPixels` inverte
na vertical → os bytes entregues ao x264 não são NV12 válido. **Plumbing PROVADO (H.264/RTP real no fio);
falta só a FIDELIDADE: ler de volta no tamanho/orientação certos do alvo do Rgba2yuv** (o fonte do shader foi
dumpado pelo glhost — dá pra deduzir o empacotamento). Refinamento, não bloqueio.

**Ajuste de layout → achado de fundo (17/10/2026): não há cluster na bancada; o shader não é NV12 padrão.**
Ao tentar corrigir a fidelidade: (1) **flip vertical** no `OPX_READBACK` (glReadPixels é bottom-up, o encoder
lê top-down) — não mudou o frame. (2) **Fonte do shader dumpado** (`/out/frames/shader_2.glsl`): o fragment
shader do Rgba2yuv **não faz RGB→YUV** — é um **remapeamento Venus-específico** (transpõe `texcoord`, reempaca
por `lstride`/`lscanl`, swizzla BGR, saída RGBA). Logo a saída dele NÃO é NV12 que o x264 entenda. (3) **O
grab é PRETO**: `import_000.ppm` tem R=0 (único), PNG ~1.9 KB — o `ivishare-stub` dá uma superfície VAZIA.
**A bancada não tem renderizador de cluster** — o cluster real é desenhado pela HMI/navegação do CARRO, não
aqui. O "ruído embaralhado" era o source preto pelo remap do shader, relido como NV12. **Conclusão: a bancada
prova o PIPELINE de streaming (grab→convert GL→encode x264→RTP), que está 100% provado; conteúdo real de
cluster é do carro, não da bancada.** Para um demo visual de fidelidade dá pra injetar um padrão de teste como
source e bypassar o remap Venus (encode direto RGBA→NV12 no daemon) — sintético, não o cluster real.
Diagnósticos deixados: dump de shader no `host.c`, flip no `OPX_READBACK` (correto de qualquer forma).

**Demo de padrão de teste (17/10/2026) — FEITO, conteúdo reconhecível fluindo até o RTP.** Toggle
`MOI3_ENC_PATTERN=1`: o `OPX_READBACK` do glhost gera um **NV12 reconhecível** (8 barras SMPTE + faixa de
gradiente no topo + barra branca varrendo por frame) no ponto onde o frame convertido iria — bypassa o remap
Venus e o grab preto. O padrão flui pelo pipeline real: glhost → write-back no bo → fakeomx → x264 (daemon) →
H.264 → `FillBufferDone` → RTP. Encode agora ~1.3 KB/frame (conteúdo estruturado comprime bem, vs ~195 KB do
ruído). O `stream.h264` decodifica (PyAV) em **barras de cor corretas** e a barra branca **se move entre
frames** (vídeo ao vivo, não estático). Prova a fidelidade do caminho de conteúdo do encoder+RTP. Artefato:
`demo_padrao_rtp.mp4` (90 frames, 30fps). Sintético, não o cluster real (que é do carro). Infra do demo no
`apply.py` (`OPX_READBACK`, ramo `MOI3_ENC_PATTERN`).

**Experimento 1 (17/10/2026) — REFUTADO.** Fiz o `fakeomx` devolver saída **não-zero** (8192 bytes +
flags `ENDOFFRAME|SYNCFRAME`) em vez de 0, pra testar se era só isso que travava o flow-control.
**Não destravou:** `EmptyThisBuffer` continuou 0. Conclusão (válida): o gatilho NÃO é "saída não-zero".
Explicação (agora resolvida pelo modelo corrigido acima): a saída (`FillBufferDone`) é o lado de SAÍDA;
o feed (`EmptyThisBuffer`) é dirigido pelo **pacer `FUN_00112640` sinalizando `S+0x248`** + o **gate do
feeder `FUN_00113910`**, nada a ver com o valor de `0x320`. Revertido (bancada limpa; só o fix do índice
OMX em `omx_il.h` fica). Nota: o mss **já cria o socket RTP** (`bind` AF_INET6 porta efêmera, destino
`ff14::2:4:42800`) — o data-plane de saída está montado, só falta ser alimentado.
**Próximo passo = gdb instrumentado pro modelo certo** (pacer roda e sinaliza? feeder acorda? qual
operando do gate falha? `FUN_0011f220` é alcançado?), e só então o `fakeomx`-encoder. Ponto de
retornos decrescentes para um objetivo 100% de bancada.
- Mapear os gatilhos de `registerDisplays` (só boot, ou também reconexão/evento BAP) pra dimensionar
  a janela em que o redirect "cola". Pistas: `FctActiveRgType::triggerKombiMapRendererVisibilityChanged`,
  `handleKombiMaprenderer`, BAP `FctVideoStreams` (bit 58).
- **Resolvido:** o gate de visibilidade do mss é `display.renderingSurfaces` (o id da RS tem de estar
  lá), não `currentVisibleSurfaces`. Pro C: o produtor precisa **acrescentar a RS própria ao
  `renderingSurfaces` do display-alvo**, além de criar a RS com `name="surfaceid://<app-surface>"`.

## Ferramentas (16/10/2026)

- **Ghidra 12.3 headless direto FUNCIONA** (`~/Downloads/ghidra_12.3_DEV/support/analyzeHeadless`):
  import+auto-análise 1x, depois `-process <bin> -postScript <script>.java -noanalysis` (rápido).
  Script **tem de ser Java** — este Ghidra não tem PyGhidra no headless (Python falha).
- **REA FUNCIONA** com o provider **Ghidra 12.1.4** (consertado em 16/10/2026). A variável
  `GHIDRA_INSTALL_DIR` estava faltando no `rea.env` do **`~/Library/Application Support/Claude/claude_desktop_config.json`** —
  é esse arquivo que o app de desktop lê pra subir o MCP, não o `~/.claude.json`. Depois de
  adicionar `GHIDRA_INSTALL_DIR=/opt/homebrew/opt/ghidra/libexec` (+ `JAVA_HOME`) ali e reiniciar
  o app, `open_binary` com `provider_id: ghidra` abre e `binary_overview`/`procedure_pseudo_code`
  funcionam. O provider **Hopper** segue barrado (permissão de Automação do macOS) — usar Ghidra.
  Detalhe da saga em `memory/re-firmware-mib3-ferramentas.md`.

---

## Engenharia Reversa Nativa via Ghidra / REA e Descoberta dos Símbolos Oficiais (18/10/2026)

Durante a análise dos binários oficiais extraídos do firmware MIB3 LG (`system.img` do train `MOI3_EU_VW_R8830L_MU0883` e `MOI3_EU_VW_E0803L`), foram analisadas as bibliotecas `/lib64/libAirPlay.so`, `/lib64/libAirPlaySupport.so`, `/lge/app_rw/smartlink/carplay/carplay` e `/lib64/libCarPlayMediaFramework.so`.

### 1. Assinatura Real de `AirPlayInfoArrayAddScreenDisplay` (`libAirPlay.so` @ `0x01f670`)
A descompilação revelou os 9 parâmetros exatos recebidos pela função da Apple implementada no AirPlay da VW/LG:
```c
void AirPlayInfoArrayAddScreenDisplay(
    CFMutableArrayRef *inArray,
    CFStringRef uuid,
    uint32_t features,
    uint32_t primaryInputDevice,
    int32_t maxFPS,
    int32_t widthPixels,
    int32_t heightPixels,
    int32_t widthPhysical,
    int32_t heightPhysical
);
```
- **Achado Crítico:** Nos protótipos iniciais, a chamada passava parâmetros em ordem incorreta (dimensões e taxas de quadros em posições trocadas), o que gerava um descriptor inválido para o iPhone.
- Para a tela secundária do AID do Jetta GLI (1280×480 a 60 FPS):
  - `uuid`: string única (ex: `"cluster-screen-uuid"`) — **não pode ser idêntica ao display principal**, sob pena de asserção de duplicidade no CoreFoundation da Apple.
  - `features`: `0x1`
  - `primaryInputDevice`: `0`
  - `maxFPS`: `60`
  - `widthPixels`: `1280`
  - `heightPixels`: `480`
  - `widthPhysical`: `300` (largura física aproximada em mm do painel digital)
  - `heightPhysical`: `112` (altura física aproximada em mm)

### 2. Pipeline de Vídeo e Hook de Streaming (`libAirPlaySupport.so`)
- A função central de entrega de pacotes de vídeo decodificados é:
  ```c
  OSStatus ScreenStreamProcessData(
      void *stream,
      const uint8_t *inData,
      size_t inLen,
      uint64_t inTimestamp,
      uint32_t inOptions,
      void *inCompletion,
      void *inContext
  );
  ```
- No fluxo oficial, `libCarPlayMediaFramework.so` (`CarPlayScreen::processData`) processa os NALUs AVCC recebidos do iPhone, adiciona o prefixo Annex B (`0x00 0x00 0x00 0x01`) e entrega ao player Cinemo para a tela principal (Display 0).
- Para a tela 1 (Cluster), nosso hook intercepta os pacotes quando `stream == g_cluster_stream`, converte o empacotamento AVCC para Annex B com SPS/PPS, empacota em MPEG-TS (188 bytes com PCR e PTS) e envia direto via socket RTP (Payload Type 33) para `[ff14::2:4]:42800` através da interface de rede do cluster (`eth0.5`).
- **Sem recodificação OMX:** Isso confirma 100% o relato do James11123 — o fluxo H.264 vem pronto do iPhone; não há necessidade de re-encodar no SoC, garantindo 60 FPS com latência mínima e sem sobrecarga de CPU/GPU.

---

## Causa Raiz do Incidente da Tela Preta e Reboot Imediato (18/10/2026)

No primeiro teste embarcado do AltScreen v4.0 no carro, ao conectar o iPhone a central multimídia apresentou tela preta e reiniciou.

### 1. Análise Forense dos Logs do Carro (`deploy_success.log`)
O log gravado no pendrive revelou:
```
root 923 ... /bin/bash /etc/vw_fs_reboot.sh /lge/app_rw/smartlink/carplay/carplay
[carplay] <defunct>
```
O watchdog de segurança do firmware da VW foi acionado!

### 2. O Mecanismo de Fail-Safe do Firmware (`systemd` + `vw_fs_reboot.sh`)
Investigando `/lib/systemd/system/carplay.service` no `system.img`:
```ini
[Unit]
Description=CarPlay Daemon
After=network.target

[Service]
ExecStartPre=+/bin/sh /lge/app_rw/smartlink/carplay/carplay.sh
ExecStart=/lge/app_rw/smartlink/carplay/carplay
Restart=on-failure
FailureAction=reboot-immediate
```
- A diretiva `FailureAction=reboot-immediate` instrui o `systemd` a reiniciar o hardware imediatamente caso o serviço falhe ou caso o `ExecStartPre` retorne qualquer código diferente de zero (`0`).
- O script `/etc/vw_fs_reboot.sh` é disparado pelo systemd antes do reboot:
  1. Executa `systemctl stop vw-hmi` (o que apaga instantaneamente a tela da central — a **tela preta** observada pelo usuário).
  2. Força o reboot via hardware watchdog para tentar recuperar o sistema de entretenimento.

### 3. A Falha Específica no Script Modificado
No script `/lge/app_rw/smartlink/carplay/carplay.sh` injetado pelo deploy:
```sh
umount -f /lge/app_rw/smartlink/carplay/carplay 2>/dev/null
```
- Quando o arquivo não estava montado com bind mount, o comando `umount` retornava código de saída `1` (erro).
- Como o script não tratava o retorno, o `carplay.sh` terminou com código de erro `1`.
- O `systemd` interpretou como falha do `ExecStartPre`, chamou `/etc/vw_fs_reboot.sh`, matou o HMI e rebootou o carro!

### 4. Regras Obrigatórias de Segurança para Scripts MIB3
1. **Terminação Segura:** Todo script executado como hook (`carplay.sh`, `fix_hw_partnumber.sh`) DEVE obrigatoriamente terminar com `exit 0`.
2. **Proteção de Comandos:** Todos os comandos que podem falhar devem ser protegidos com `|| true` (ex: `umount -f ... 2>/dev/null || true`).
3. **Restrição de Execução SMACK:**
   O arquivo `/etc/smack/accesses.d/smartlink.rules` define:
   `smartlink::app _ rw`
   O label `_` (usado no `/tmp`) **não possui permissão de execução (`x`)** para o domínio `smartlink::app`.
   Tentar executar qualquer binário ou carregar bibliotecas diretamente de `/tmp` causa terminação imediata pelo kernel Linux com violação de segurança SMACK.
   Portanto, binários e bibliotecas modificadas devem residir em `/lge/app_rw/smartlink/carplay/` e ter seus labels SMACK configurados explicitamente (`chsmack -a smartlink::app` / `carplay::app`).

---

## Chave Física de Segurança e Reversibilidade no Pendrive USB

Para garantir risco zero de brick ou instabilidade persistente:
- O script `fix_hw_partnumber.sh` na raiz do pendrive checa a presença de arquivos de controle:
  - **`REVERT.txt` presente:**
    - Desfaz qualquer bind mount ativo (`umount -f ... || true`).
    - Restaura as cópias originais limpas de fábrica de `carplay`, `carplay.sh`, `mdnsd` e `iAP2d`.
    - Remove bibliotecas injetadas e limpa `/tmp`.
    - Restaura os labels de SMACK originais de fábrica.
    - Reinicia os serviços do systemd de forma limpa.
  - **`ENABLE_ALTSCREEN.txt` presente:**

---

---

## Arquitetura de Teste One-Shot Auto-Desarmável (18/10/2026)

Em alinhamento direto com a prática do pesquisador James11123 (*"os arquivos são substituídos pelos originais de fábrica na inicialização"*), os testes do CarPlay AltScreen utilizam o padrão **One-Shot Auto-Desarmável**:

1. **A Realidade do GEM (`swdl`):**
   - No MIB3 LG, a conclusão de qualquer atualização de software no Red Menu (GEM) exige que o usuário clique em **"Resume"**.
   - O subsistema `swdl` da Volkswagen obrigatoriamente força um `reboot` de hardware no Resume. Não existe saída do GEM sem reboot.
   - Qualquer patch aplicado puramente em RAM volátil (`/tmp`) antes do Resume é descartado durante o reboot.

2. **O Mecanismo de Ingresso Único (`DO_TEST_ONCE`):**
   - O instalador USB `fix_hw_partnumber.sh` instala `altscreen_handler.so` em `/lge/app_rw/lib64/` e `carplay_altscreen` em `/lge/app_rw/smartlink/carplay/`.
   - Cria o arquivo-ingresso: `/lge/app_rw/smartlink/carplay/DO_TEST_ONCE`.
   - Cria o backup permanente `/lge/app_rw/smartlink/carplay/carplay_factory.sh`.

3. **Ciclo de Vida do Boot pós-Resume:**
   - No primeiro boot após o Resume, o script `/lge/app_rw/smartlink/carplay/carplay.sh` roda como root (`ExecStartPre`):
     ```sh
     FLAG="/lge/app_rw/smartlink/carplay/DO_TEST_ONCE"
     if [ -f "$FLAG" ] && [ -f "$TEST_BIN" ]; then
         # 1. Consome o ingresso na hora (deleta do disco)
         rm -f "$FLAG" 2>/dev/null || true
         # 2. Restaura o carplay.sh de fábrica no disco na hora
         cp -f "$FACTORY_SH" "$CURRENT_SH" 2>/dev/null || true
         # 3. Aplica o teste exclusivamente para esta sessão
         mount --bind "$TEST_BIN" "$TARGET" 2>/dev/null || true
     fi
     exit 0
     ```
   - **Garantia de Auto-Desarme:** O ingresso é consumido e o script de fábrica é restaurado no disco **no primeiro segundo do boot**, antes mesmo do processo do CarPlay inicializar!
   - Se ocorrer qualquer crash, erro ou se o carro reiniciar por qualquer motivo: no próximo boot o ingresso já não existe e o script no disco já é o de fábrica. A central acorda **100% original de fábrica**. Risco de bootloop: zero.

4. **Correção de UUID no AltScreen (`cluster-display-001`):**
   - A descompilação de `libCarPlayAppFramework.so` e `libAirPlay.so` revelou que passar o mesmo ponteiro `uuid` do Display 0 no `AirPlayInfoArrayAddScreenDisplay` provoca rejeição ou colisão de telas no CoreFoundation do iOS.
   - O `altscreen_handler.so` v4.1 cria um identificador exclusivo para a tela secundária via `CFStringCreateWithCString(NULL, "cluster-display-001", kCFStringEncodingUTF8)`, garantindo que o iPhone reconheça o Cluster AID como uma tela de navegação complementar independente.

