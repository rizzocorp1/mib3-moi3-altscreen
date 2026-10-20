# camada3 — bancada do C (redirect por RS própria)

Bancada do serviço da **opção "C"** (ver [`../camada3-rsi-redirect.md`](../camada3-rsi-redirect.md)):
fazer o mapa de um app (CarPlay/Android Auto) aparecer no cluster **criando uma renderingSurface
própria** no RSI `DisplayManagement` — com a fonte apontando pro surface IVI do app e o destino no
RTP do cluster — em vez de sequestrar a `navi-kombi-renderingsurface` nativa.

**Interop, caminho de exibição.** Nada aqui derrota Secure Boot / dm-verity / assinatura VW / SMACK.
Rodar numa unidade real depende de execução de código na ECU, que **está fora de escopo**. Isto é
bancada: só `python3` (stdlib) e `localhost`.

## Peças

| Arquivo | O que é |
|---|---|
| `rsi_mock.py` | Mock **VIWI** do barramento RSI, as 3 peças que o cliente do mss (librsic/SAI) usa: **(1) Service Registry** (resolve `displaymanagement` → host:port), **(2) REST** dos 3 recursos, **(3) WebSocket VIWI** (subscribe/unsubscribe + push de `data` na mudança). Loga todo frame WS e requisição. **Não** é o `rsi-display-management` real — é alvo de desenvolvimento e o que vamos afinar contra o mss. |
| `redirect_producer.py` | **O C**: cria, via REST, uma `streamParameters` + `display` + `renderingSurface` nossas, com `name=surfaceid://<id-do-app>` e o destino no cluster. |
| `prepare_streams_check.py` | Replica **fiel** do `MultiScreenController::prepareStreamsList` do mss (RE `@0x130028`, REA/Ghidra 12.1.4, MU 0883): prevê a lista de streams a partir do estado do serviço. É o teste do produtor. |
| `ws_selftest.py` | Autoteste do WebSocket do mock (sem o mss): conecta, assina, cria uma RS via REST e confere que chega o push `data`. Valida a máquina de subscribe/notify. |
| `run_demo.sh` | Sobe o mock, roda o produtor e o verificador, e confere que a fonte/destino saem certos. |

> **Envelope VIWI = palpite informado.** O formato do frame WS (`{"type":"subscribe","event":...}`, push `{"type":"data","event":...,"data":...}`) e o shape do registry estão modelados no VIWI (`wzr1337/rsi.server`) e **isolados** em `viwi_*`/`registry_services` no `rsi_mock.py`. Serão trocados pelo real quando virmos o que o mss manda — por isso o mock loga todo frame cru (`ws frame <-`).

## Rodar

```sh
sh run_demo.sh
# ou parametrizado:
APP_SURFACE=1200 DEST=ff14::2:4 RTP_PORT=42800 FPS=30 sh run_demo.sh
```

Saída esperada (resumo): o verificador imprime o `StreamInfo` que o mss montaria e confirma
`fonte 1200 -> ff14::2:4:42800 @ 30fps (grab ivi 1200, stream pro cluster)`.

## O que isto prova — e o que ainda não

**Prova:** o estado que o C cria no `DisplayManagement` é exatamente o que o `prepareStreamsList`
do mss consome para grabear o surface do app e transmitir pro cluster — validado contra a nossa RE
da função, sem precisar do binário. Exercita o **gate de visibilidade** (`currentVisibleSurfaces`) e
a extração de fonte via `surfaceid://`. Além disso, `ws_selftest.py` valida a **máquina de
subscribe/notify** do WebSocket VIWI (handshake → subscribe → ack → snapshot → push na mudança).

**Ainda não (próximo marco):** dirigir o **binário real do mss** por RSI. Hoje a bancada do mss
(`../../../../moi3-cluster-pipeline-re/spike/`) roda o mss em modo **CLI** (`-m <surface> <fps>`),
não RSI. A RE do cliente RSI do mss já foi feita (lib **`librsic.so.0.1`** / SDK "SAI" — ver
[`../camada3-rsi-redirect.md`](../camada3-rsi-redirect.md) "RE do cliente RSI do mss"): descoberta
por **Service Registry** (`RsiLookup`), **REST** (`RsiClient`) e **notificação por WebSocket VIWI**
(`WsClient`, subscribe/notify). O `rsi_mock.py` **já tem as três peças** (registry + REST + WS).

Estado do caminho pro mss real:
- **Container destravado (16/10):** `Dockerfile.rsi` gera `moi3-bancada:rsi` (a imagem da bancada +
  `python3`). O mock roda **dentro do container, em `--network none`, no `127.0.0.1`** — o loopback
  sobe mesmo sem rede; provado com o `ws_selftest.py` lá dentro. Então o mss (no mesmo container/netns)
  acha o mock no localhost. Build: `docker build -f Dockerfile.rsi -t moi3-bancada:rsi .`.
- **Falta:** o harness que roda o **mss em modo RSI** (sem os args de surface do CLI) contra o mock,
  sob `strace`, pra (a) ver o **bootstrap do SR** (primeiro `connect()`/`Creating WsClientImpl
  ws://host:port`) e (b) ler o **frame real do subscribe** — e então afinar o envelope VIWI do mock
  (hoje é palpite). É o loop empírico.

## Decisões de modelagem (da RE)

- **id REST ≠ `name`.** No mss, o id do recurso (o `+0x8`, usado no gate do `currentVisibleSurfaces`)
  é distinto do campo `name` (`+0x28`, de onde sai a fonte `surfaceid://`). O mock e o produtor
  modelam os dois separados.
- **Fonte.** `prepareStreamsList` tira a fonte de `name` quando começa com `surfaceid://` (12 chars,
  normalizado trim+lower), senão do `internalIdentifier`. O C usa o caminho `surfaceid://` (lido
  direto do JSON postado), não o `internalIdentifier` (que, no nativo, quem preenche é a associação
  ao maprenderer).
- **RS própria, não sequestro.** A fonte da RS nativa é ligada uma vez em `registerDisplays`;
  visibilidade só re-POSTa framerate (1/10 fps). Uma RS própria fica imune ao afunilamento e ao
  re-bind — ver [`../camada3-rsi-redirect.md`](../camada3-rsi-redirect.md) §6.
