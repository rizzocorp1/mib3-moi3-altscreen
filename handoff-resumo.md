# Handoff — mapa/nav de app no cluster (MIB3 MOI3 LG, Jetta GLI 2021)

Resumo condensado do estado do projeto para continuar em outro agente. Data: 17/10/2026.

## Carro / unidade
- Jetta GLI 2021, painel digital (AID / Active Info Display). Nav nativa já aparece no cluster.
- Central MIB3 **MOI3 LG** (família `3G5035832C`, SW **MU0803**; RE feita sobre o firmware **MU0883**).
- Coding 5F real (autoscan 14/10): `navigation_map_transmission_mode=IP_streaming`, `..._compression=H264`,
  `ability_switch_nav_maps=yes`, CarPlay + Android Auto ligados.

## Objetivo
Pôr o mapa/nav de um app (CarPlay / Android Auto) no cluster.

## Dois caminhos (RE concluída)

**1. Nativo / BAP → só resumo, nunca a seta.**
O recurso VIWI `appconnectrouteguidanceactive` carrega **só um boolean** `isSmartphoneRouteGuidanceActive`
(spec `.../sai/spec/viwi.service.appconnectrouteguidanceactive-0.1.0.json`). Da projeção chega apenas
**ETA + distância** (`FctSemidynamicRouteGuidance`, BAP bit50). A **seta de manobra** (`FctManeuverDescriptor`
bit23, `FctLaneGuidance`, `FctTurnToInfo`, `FctDistanceToNextManeuver`) é alimentada **só pelo DrivingContext da
nav nativa** (`GuidanceServiceHandler`/`SimpleNavigationServiceHandler`, `handleNativeDrivingContext`). A spec da
VW diz: um recurso para todas as techs (CarPlay/AA/MirrorLink/Baidu CarLife), usado para exibir o texto
"Smartphone navigation active" **no lugar do mapa** no FPK.
→ **Seta do celular no cluster = impossível por design.** Confirmado no carro: Google Maps e Apple Maps no
CarPlay = ETA/distância sim, seta não.

**2. Vídeo / camada 3 (opção "C") → é o caminho que dá o mapa cheio.**
O app renderiza no tamanho do cluster (CarPlay **AltScreen tipo 111** / AA), manda **H.264**, e o
`multi-screen-server` / RSI DisplayManagement injeta na surface **`navi-kombi-renderingsurface` (id 33)**,
**1280×480**, cedendo o canal do `naviengine`.

## Bench RE (feito e provado)
- `multi-screen-server` data-plane RE completo (REA/Ghidra 12.1.4). Contrato VIWI: associações = objetos `{id}`;
  **`uri` obrigatório**; gate de `prepareStreamsList` = **`display.renderingSurfaces`** (não
  `currentVisibleSurfaces`). Pipeline:
  `onMessage@0x135078 → prepareStreamsList@0x130028 → updateStreamsList@0x12f9e8 → startStream@0x11b350 →
  FUN_001175d0 (startShare: ivi grab + Rgba2yuv/OMX + RTP)`. `main@0x1106a0` com `argc<6` = modo RSI, porta
  49600 (`0xc1c0`).
- End-to-end sob gdb: subscribe → grab (ivi surface 1000) → import → shader convert, OK.
- `rsi-display-management` RE: recursos `/display`, `/renderingSurfaces`, `/streamParameters`
  (IP / PORT / FRAMERATE / RESYNC).
- **Feed contínuo do encoder (resolvido na bancada / alinhamento com James):**
  - *Modelo interno do feed (RE estática):* `S+0x320` é regulação de FPS `{1=abaixo, 2=acima, 3=parada}` (NÃO é semáforo); 3 threads — `cb_damage@0x115860` (produtor), `EventSourceTask FUN_00112640` (pacer, `Condition::signal(S+0x248)`), `StreamerTask FUN_00113910` (feeder) → `FUN_0011f220` → `EmptyThisBuffer`; `is_ready` liga no init `FUN_0011fe30`; precisa de `EventCmdComplete` via `EventHandler FUN_0011f5f0`.
  - *Destravado na bancada (17/10/2026):* Shader Rgba2yuv corrigido no glhost (removido `precision highp uint;`), render-to-external emulado via textura 2D de respaldo, write-back implementado via `OPX_READBACK`, daemon de encode x264 (`enc_daemon.py`) acoplado ao `fakeomx` via socket UNIX. Resultado: `FillBufferDone` com NALs H.264 comprimidos e envio RTP real comprovado no strace (`sendto = 1140` para `[ff14::2:4]:42800`, 11.670 pacotes em 20s).
  - *No carro real (foco com o James):* No hardware da central, o encoder é o Qualcomm Venus (`OMX.qcom.video.encoder.avc`). A pergunta ao James (atualização de quadros / taxa de atualização do AltScreen) foca em como a segunda tela do CarPlay entrega danos contínuos à surface IVI para alimentar o pacer/feeder em taxa estável sem sofrer o throttling de 1 fps do `ToggleStreamFramerate` do `NavSd`.

## NavSd RE (`libMIBNavSDClientLib_CarIF.so`)
- `AppConnectRouteGuidanceActiveServiceHandler::handleAppConnectNavigationsChanged@0x1ad2cc` (lê
  `appConnectNavigations[0].uri`, reassina no microserviço).
- `FctInfoStates::handleAppConnectNavigationsObjectChanged@0x269c8c` (lê `isSmartphoneRouteGuidanceActive`,
  string em `0x31a218`; emite BAP `InfoStates` estado 6).
- `FctActiveRgType::rsiMaprenderer2Bap@0x25d274` (`kombi`→3, `main-unit`→0; string `kombi` em `0x30da28`).

## Estado da camada 3
Display + conversão **provados na bancada**. Para rodar no carro falta: (a) serviço de redirect apontando o mss
ao surface do app via RSI; (b) **execução do serviço na unidade**; (c) o app desenhando o mapa; (d) coding do
painel aceitando o stream (já ON, `IP_streaming`).

## Cena pública
- **daChillout** (Amsterdam; site `mqbtools.nl`): PoC Android Auto no cluster (Cupra Formentor MOI3); writeups +
  ferramentas web (GCA/MCF). Não publica código do redirect. `github.com/dachillout` não existe.
- **James11123** (Polo liftback → Jetta MK7): PoC CarPlay AltScreen no MOI3 LG; está na 2ª versão alfa; disse que
  posta "quando tudo funcionar". Sem repo.
- **avissi**: guia de ativação FEC/SWaP (Preh/Joynext).
- **O código do redirect de mapa NÃO está em git aberto.**

## Repos / docs
- `github.com/lipedesign/moi3-cluster-pipeline-re`
- `github.com/lipedesign/mib3-moi3-lg-research`
- No projeto: `referencia/ferramentas/mib3-sandbox/{camada3-rsi-redirect.md, painel-de-instrumentos.md,
  modelagem-soc.md}`; pasta da bancada em `referencia/ferramentas/mib3-sandbox/` (driver/, glforward-moi3/,
  camada3/, janela/, stubs/).
