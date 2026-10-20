# Painel de instrumentos (cluster) no MOI3 — como a central produz e manda o vídeo do mapa

Compilado em 11/10/2026 a pedido do Lipe (via a sessão "Modelagem SOC"), para o item 8 do roteiro em [simulador-jetta.md](simulador-jetta.md). É **síntese do que já está registrado na pasta**, com a fonte e o status de cada fato; aponta a seção de origem em vez de recopiar. Só leitura e registro — nada foi executado, baixado, nem tocado em `driver/`, `stubs/`, `modelagem-soc.md` ou READMEs.

**Convenção de status:** `[DESMONTAGEM]` verificado em binário/decompilado (REA/Ghidra); `[EXEC]` verificado rodando no sandbox; `[DEDUÇÃO]` inferência minha; `[FÓRUM]` relato de cena/campo; `[OUTRA PLATAFORMA]` fato de MHI2/MHI2Q/MPR3, **não** confirmado no MOI3.

**Aviso de fronteira (importante para o item 8):** a metade **receptora** do pipeline — quem assina o grupo multicast e rasteriza no painel — vive no **cluster/FPK, que é outra ECU**, cujo firmware **não está** nas imagens que temos. Tudo abaixo é o lado **produtor** (a central MOI3). Para um "Virtual Cockpit" no simulador, o nosso lado seria **escrever um receptor** que entra no grupo e decodifica — como o qemu-mhi2 faz, só que por IP, não MOST.

---

## 1. O fluxo `ff14::2:4` (porta, protocolo, empacotamento, codec, resolução/taxa, produtor)

- **Produtor (função):** `us_displaymanagement::createOrUpdateStreamParameters` **@0x214814**. Monta um **JSON** e faz **HTTP POST** a um microserviço **VIWI** (Volkswagen Infotainment Web Interface; Poco::Net HTTP + JSON na rede interna) no path `displaymanagement/.../streamParameters`. `[DESMONTAGEM]` — [mib3-firmware-pacote-moi3-lg.md](../../mib3-firmware-pacote-moi3-lg.md), §"RE do caminho de configuração de display" e §"Dissecação verificada por desmontagem".
- **Destino do stream (fixo no código, não é config):** `ip` = **`ff14::2:4`** (multicast IPv6), `port` = **42800** (imediato `mov w0,#0xa730`). Há **1 único literal `ff14:`** no `.so`; a string "42800" não existe → é destino compilado, não configurável. `[DESMONTAGEM]`
- **Protocolo/empacotamento:** vídeo IP em **grupo multicast IPv6**; a central renderiza o mapa e **transmite**, o cluster **assina o grupo e exibe**. Transporte provável **Ethernet AVB** (serviço `gptp`/gPTP presente para sincronismo AVB/TSN). O multicast+porta é `[DESMONTAGEM]`; "AVB" é `[DEDUÇÃO]`.
- **Strings/flags do caminho:** `FctVideoStreams`, `NAV_KOMBI_STREAMPARAMETERS_NAME`, `_ipStreamingEnabled: 0x`, mensagens de framerate/"stream parameters" no JSON. `[DESMONTAGEM]`
- **Codec:** **H.264 / OMX.qcom**, no binário `multi-screen-server`. A presença do H.264/OMX.qcom é `[DESMONTAGEM]` (string no binário); que seja **especificamente** o do mapa (e não só das câmeras) é `[DEDUÇÃO]` — ver dependências no item 4. *Corrigido em 11/10/2026 pela sessão do simulador: aqui estava "o decodificador do lado receptor". O binário é da central e o componente é um **codificador** (`OMX.qcom.video.encoder.avc`); ver "Conferência" no fim.*
- **Resolução/taxa:** **não extraí os literais** de largura/altura/fps do stream do cluster. O JSON de `createOrUpdateStreamParameters` carrega campos de framerate, mas não anotei os valores. **Não assumir 800×480:** esse é o tamanho da **tela central** variante `OISmall` (verificado no sandbox) e das telas AID em relatos de fórum — **não** um literal confirmado do stream do painel. `[DEDUÇÃO]`/`[FÓRUM]` — não misturar com o item do sandbox.
- **Conceito:** isto é o **"Video Navigation"** — a MIB3 LG **é a fonte do vídeo** do mapa do cluster (como o MHI2 no conceito, mas **por IP**, não MOST). `[DESMONTAGEM]` + reconciliação. *Corrigido em 11/10/2026 pela sessão do simulador: aqui estava "é sink de vídeo". As regras de rede do firmware têm a central como origem; ver "Conferência" no fim.*
- **Corroboração comportamental:** num reel da `vagcodingdf`, ao trocar a visualização do cluster **a seta de GPS não re-centraliza** — coerente com "superfície de vídeo preenchendo o retângulo do mapa", não "renderizador nativo desenhando a seta". `[FÓRUM]`/`[DEDUÇÃO]` — §"Resumo executivo" do doc de firmware.

## 2. Quem manda começar/parar — DSI/BAP, `setActiveDisplayable`, displayables

- **Serviço DSI do display:** `displaymanagement` (`DSIDisplayManagement`). Métodos vistos (protobuf LG): `com.lge.mib3oi.dsi.displaymanagement.proto.DSIDisplayManagementMsg$getAvailableDisplays`, `getDisplaySizeResult`, e o `getDisplayExtents(displayId)` que a HMI chama no startup. `[DESMONTAGEM]` + `[EXEC]` (o `driver/dmstub.c` responde esse método e a HMI registra o `DSIDisplayManagement`) — [NOTAS-RE-DINAMICA.md](NOTAS-RE-DINAMICA.md), §"EMULADOR DisplayManagement"; [modelagem-soc.md](modelagem-soc.md), Marco 4.
- **Gate do recurso (liga/desliga do mapa nativo):** BAP **`FsgSetup` Df44 Bit0 = `VideoNavigationSupported`** (membro `0x19a`). BAP é o protocolo de dados da VW. `[DESMONTAGEM]` + `[EXEC]` (ver item 3).
- **Caminho de rota (dados, não vídeo) para o cluster nativo:** `KombiMapRenderer`, `setRouteGuidanceMessage` → display "Instrument Cluster". O cluster **renderiza o mapa a partir de dados de rota** que o head unit envia. `[DESMONTAGEM]` — §"Comparação com o harman-f" do doc de firmware.
- **`setActiveDisplayable(displayID, displayable:i32)` e os displayables numerados** (93 `Displayable_External_Smarthphone`; cluster **26 Map_Cluster / 29 Guidance_Cluster / 40 Cluster_Map / 42 Cluster_Map_Route_Guidance / 60 Hud_Map**): **`[OUTRA PLATAFORMA]`** — são da RE do **Squallman/MPR3 (Audi Premium, P3695)**, mesma família MIB3-Linux, **arquitetura análoga**, mas **não são literais confirmados do MOI3/LG**. No MOI3 temos o nome `Displayable_External_Smarthphone` citado na RE, mas **não** enumerei os displayables do mapa do cluster. Tratar como referência cruzada até confirmar no firmware LG. — [coding-geral.md](../../coding-geral.md), §"Squallman/MPR3".
- **O que o painel pede à central:** ainda **não mapeado no MOI3** (é a metade receptora, outra ECU). No MPR3 o Squallman também lista o "cluster endpoint" como **UNKNOWN** — nesse ponto a **nossa RE está à frente** (temos o `ff14::2:4`). `[OUTRA PLATAFORMA]` + lacuna assumida.

## 3. O que precisa estar codificado/adaptado para a central mandar o vídeo

- **Mapa NATIVO no cluster (o "Video Navigation") — é CODING, e já está LIGADO no Jetta (confirmado pelo autoscan de 14/10/2026, ver §Conferência):** vem da adaptação **`DASHBOARD_DISPLAY_CONFIGURATION`** (nome interno `MIB3_KEY_ADAPTION_DASHBOARD_DISPLAY_CONFIGURATION`, **canal `0xB5A`**), **byte 7 / bit 1** (campo `switchMapSupported`) → seta o membro `0x19a` → `VideoNavigationSupported Df44 Bit0`. **Store único provado** no `.so`. `[DESMONTAGEM]` (unicidade) + `[EXEC]` (pôr `0x19a=1` faz o getter virar 1; =0, vira 0) — §"Dissecação verificada" e §"RESULTADOS EMPÍRICOS", pista II.
- **Outros codings lidos pelo SW:** `_iaa_function_coding`, `_pso_function_coding`, `SemidynamicRouteGuidanceAvailable Bit50`, `FunctionList`. É function-coding/function-list — o que o **ODIS-E lê e escreve**. `[DESMONTAGEM]`
- **Mapa CHEIO do app do telefone (Waze/Maps) — NÃO é coding:** exigiria (a) trocar os `false` **hardcoded** de `RouteGuidanceByMobileDevice` (`setBapEntity` Df47 Bit3/4/5) e (b) **substituir a fonte** que alimenta o multicast `ff14::2:4` pelo vídeo do CarPlay. Ambos são **patch de binário atrás da parede de assinatura/SMACK** — fora do escopo (o ponto de patch está mapeado; o furo de assinatura, não, e não é para fazer). `[DESMONTAGEM]` — §"Gate decompilado".

  **Lead comercial (14/10/2026) que desafia o "fora do escopo" acima.** Um serviço brasileiro, **Codifica Meu Volks**, vende "Integração CarPlay para AID e MIB3 com Multi Touch" (v1.1 Release Candidate), instalada **por pendrive** ("remova o pendrive e reinicie a MIB"); o instalador **desenha a própria tela no painel**. Num story de cliente (`naves.gli`), o AID aparece com um mapa de navegação com POIs comerciais — estética de **app de celular (provavelmente Waze/Google Maps)**, não o render nativo. O Lipe enviou o chassi **deste** carro (`3VW4L6BUXMM068933`, MIB3 LG, MU 0803) e o fornecedor respondeu que **dá para fazer** nele. Se o que aparece é mesmo o mapa do app, então a "camada 3" (mapa cheio no AID) existe de forma **comercial/privada** na MIB3 LG, por um caminho que esta RE **não mapeou** — o que supera o "não é para fazer" **para o serviço deles**, não para nós por conta própria. Em aberto, a apurar com o fornecedor antes de qualquer decisão: **o que exatamente faz** (o Waze no painel ou a navegação nativa), se **modifica o firmware**, se é **reversível**, o **risco de brick e a recuperação** (lembrar: o `R8030L`/MU0803 deste carro não está disponível publicamente e o boletim de recovery da VW não cobre este chassi), a **garantia** e o efeito de **atualizações futuras da VW**.

  **Confirmação fotográfica (16/10/2026, story do @codificameuvolks enviado pelo Lipe).** Foto do **instalador deles rodando num cluster AID de MIB3**: tela azul custom no painel com "**Aguardando Instalador…**", um **QR code** ("EXPANDA A VISUALIZAÇÃO PARA LER O QR CODE"), aviso "software **vinculado ao veículo / NÃO TENTE COPIAR**" em 5 idiomas, e rodapé "**Integração AID e Multi Touch para CarPlay em MIB3 — Versão 1.1 Beta 3**". Barra do cluster normal (P, trip 1344,4 km, 19308 km, 18:33, 24,0 °C). **O que prova:** (1) é **MIB3** (diz no rodapé); (2) o instalador **desenha uma tela custom inteira no AID** → eles têm **execução na unidade + controle total do surface do cluster** (o "pé de execução" que não desenvolvemos + o redirect de display que a RE mapeou); (3) é **CarPlay com multitouch no AID** (camada 3 + toque de volta), **produto versionado** (1.1 Beta 3) e **travado ao VIN** (anti-cópia). O "**Multi Touch**" bate com os requests **`redirect_touch_*` da `ivi_share`** que extraímos do binário — é o cluster devolvendo toque à fonte. Fluxo `[DEDUÇÃO]`: código custom sobe na unidade (método privado) → desenha o instalador no cluster → app de celular (via QR) conduz a integração. Tentei **ler** o QR do print (ler um QR público ≠ mexer no produto): ele está **cortado à direita** no story — falta parte dos módulos e um *finder pattern*, então é **indecodificável** (o próprio story avisa "EXPANDA A VISUALIZAÇÃO PARA LER O QR CODE"). Pelo "Aguardando Instalador…", o QR provavelmente **pareia o app instalador do celular** com a unidade `[DEDUÇÃO]`. O que **não** se faz é decifrar/crackear o **produto** (travado ao VIN, "não tente copiar" = linha de não furar proteção).

**QR decodificado (16/10/2026, 2º material — reel @codificameuvolks + @pilotsibranco "WAZE E GOOGLE MAPS DIRETO NO PAINEL").** Neste frame o QR aparece inteiro (3 finder patterns) e dá pra ler (cv2 QRCodeDetector, crop do canto direito do cluster + upscale 8×). Conteúdo: prefixo **`CMV11`** (Codifica Meu Volks, formato 11) + ~430 chars no alfabeto **Base45** (`0-9 A-Z $%*+-./:`, mesma codificação do DCC da UE) = um **token de pareamento/ativação** binário (compactado/assinado) do instalador. Confirma a dedução: o QR é o **handshake de instalação** (app lê o token → pareia com o serviço no cluster). **Só leitura** — não decifrei/reproduzi o payload (é VIN-lock + "NÃO TENTE COPIAR"; é o lado de acesso, fora do escopo). O carro é um VW R-Line BR com o mesmo cockpit MIB3.

  - **Leitura completa do QR (16/10/2026, 19h07; frame ampliado que o Lipe mandou na conversa, com a luz do TPMS acesa e 0 km/h no painel; não dá para dizer se é o mesmo reel do 2º material).** Lido de primeira com `zxing-cpp` na imagem inteira (QR com nível de correção L). Texto: **`CMV11Q:`** + **468** caracteres Base45 (475 no total; acima está "~430", que era contagem aproximada ou outro frame). **Base45 é embalagem, não cifra** (como o hexadecimal): desembalado, dá **312 bytes**, que começam com o texto **`CMVQ11E1`** (8 bytes) e seguem com **304 bytes de aparência aleatória** (entropia de 7,44 bits por byte, o esperado para dados aleatórios desse tamanho; não abre como zlib, deflate, gzip, lzma nem bz2 em nenhum deslocamento de 8 a 19). `[DEDUÇÃO]`: `CMV` = Codifica Meu Volks, `11` = versão 1.1 (bate com "Versão 1.1 Beta 3"), `E1` = provavelmente esquema de cifra 1. O corpo é **cifrado** (ou comprimido por método desconhecido): não dá para ler sem a chave, nem saber se tem o VIN dentro. O "(compactado/assinado)" acima era palpite; de fora só se vê um bloco opaco. **Parei aí**, na mesma linha do registro acima: não procurei chave, não tentei decifrar e não gerei QR. Impressão digital para comparar com QRs futuros (se outro carro ou outra sessão der outro valor, o token muda por carro ou por sessão): SHA-256 do texto lido = `dcbc13e2c38606c0d95af370f78fd097e96ed0de428801a9757f20158fe47153`.

  - **Passei o blob pelo REA (16/10/2026, a pedido do Lipe — "vamos usar o REA e saber mais").** O REA é a caixa de ferramentas de engenharia reversa (abre e disseca binários, firmware, apps). `open_binary` no blob **recusou**: *"unsupported binary format"* — ou seja, isto **não é programa nem pacote**, é só um bloco de dados, então não há o que o REA "abra" ou descompile aqui. O que o blob tem para contar, conta pela forma: os **304 bytes** cifrados são exatamente **19 × 16**, múltiplo limpo do bloco de **16 bytes do AES** (a cifra padrão); varrendo em blocos de 16 **não há nenhum bloco repetido**, o que é a cara de **AES encadeado (CBC)** ou com vetor inicial — e descarta o modo mais fraco (ECB), que deixaria repetições visíveis. Juntando com o `E1` do cabeçalho, a leitura `[DEDUÇÃO]` é: **token cifrado em AES**, provavelmente com os 16 primeiros bytes como vetor inicial e os 288 restantes (18 blocos) como corpo. **Sem a chave não há o que ler** — e procurar a chave é justamente quebrar o produto (VIN-lock, "NÃO TENTE COPIAR"), a linha que não se cruza. **Conclusão do exercício de RE:** o alvo certo para o REA seria o **instalador/app deles** (que teria a chave e o protocolo dentro), não este QR; e esse instalador **não temos** — e obtê-lo para destrinchar a proteção sai do escopo. O QR em si está esgotado: lido, caracterizado, e parado aí. (Nota: numa 2ª passada, o `inspect_artifact` do REA, sem alvo nosso válido, pegou um binário à toa de outra bancada — o `multi-screen-server` ELF do scratchpad — o que reforça que o token não entra no REA.)

  - **O que o código provavelmente é (16/10/2026, pesquisa a pedido do Lipe).** Cruzando a forma do token com o padrão de mercado, a leitura mais provável `[DEDUÇÃO]` é um **token de ativação/licença offline, amarrado ao VIN** — exatamente o que a tela anuncia ("Aguardando Instalador…", "vinculado ao veículo", "NÃO TENTE COPIAR"). Dois apoios: (1) **a embalagem é a do certificado de vacina da UE (DCC/HCERT)** — lá é `HC1:` + Base45( zlib( COSE/CBOR ) ); a Noruega usa `NO1:`, provando que o prefixo antes dos `:` é só um **marcador livre de emissor/versão**. O do CMV, `CMV11Q:` + Base45( `CMVQ11E1` + AES ), é o **mesmo transporte**, só trocando o miolo (a UE usa zlib+COSE, que começa com 0x78; o CMV usa um container próprio `CMVQ11E1` + bloco AES). Ou seja, quem montou **reaproveitou a ideia de transporte do DCC**. (2) O **padrão de indústria** de ativação por QR: token entregue por QR, **Base45 como codificação** e **AES como camada de sigilo**, com o vínculo feito por um **identificador do aparelho** conferido na ativação — aqui o identificador é o **VIN**. Pelo tamanho (288 bytes de corpo, 18 blocos), cabe dentro: VIN (ou hash), código do produto/versão, um nonce/sessão e provavelmente **uma assinatura/MAC** para a unidade validar que o token não foi forjado. **Nada disso se lê sem a chave**, e a chave vive no produto deles. **Fornecedor:** busca pública não achou documentação técnica do "Codifica Meu Volks" — é serviço de Instagram (`@codificameuvolks`), confirmado só por descrição de um vídeo de YouTube ("MIB3 instalada no Volkswagen Polo"); o formato do token **não está documentado em lugar nenhum público**. Fontes: [HCERT spec v1.0.7](https://github.com/ehn-dcc-development/hcert-spec/blob/53f7a2626f8b6853c04ef246954ca8b3b32af617/hcert_spec.md) (estrutura `HC1:`+Base45+zlib+COSE), [RFC 9285 Base45](https://datatracker.ietf.org/doc/rfc9285/), [padrão de ativação offline por QR](https://dev.to/olivier_moussalli_e3492f5/offline-license-activation-with-qr-codes-serving-air-gapped-environments-in-c-1c5k).

  - **Varredura do GitHub "para esta situação" (16/10/2026, `gh search` autenticado como `lipedesign`).** Pergunta: existe ferramenta que ajude a lidar com um token de QR cifrado como este? Resultado, honesto: **nenhuma ferramenta atravessa o AES**; o que existe cobre só o **transporte** (que já peelamos) ou serviria para **analisar o instalador deles** (que não temos). Três grupos: (1) **decodificadores de Base45/DCC** — [`manekinekko/digital-covid-certificate-decoder`](https://github.com/manekinekko/digital-covid-certificate-decoder) (29★), [`jeroentrappers/dart_base45`](https://github.com/jeroentrappers/dart_base45), [`ca-haddock/impfcert`](https://github.com/ca-haddock/impfcert) — levam do texto do QR aos bytes brutos e **param no mesmo bloco `CMVQ11E1`+AES**; úteis para confirmar o transporte, inúteis depois da cifra. (2) **Triagem de blob**: [`gchq/CyberChef`](https://github.com/gchq/CyberChef) (36k★, o "canivete" de encoding/entropia/"tentar todo codec") e o equivalente em Python [`securisec/chepy`](https://github.com/securisec/chepy) — fazem na interface o que fiz na mão (Base45→entropia→blocos de 16); **não quebram AES sem a chave**. (3) **Para o alvo certo (o instalador/app deles), se um dia existir em mãos**: o kit padrão de RE — Ghidra/radare2/REA no ELF, `jadx`/`apktool` se o instalador for um APP Android, `frida` para ver a chave em uso em runtime, `binwalk` para triar firmware. **Busca específica deu vazio** em `mib3 carplay`, `volkswagen mib coding`, `car coding activation token`, `mib3 flash tool swdl`, `vag swap component protection`: **ninguém publicou ferramenta para o formato do token CMV nem para tokens de ativação/component-protection da VW** — reforça a conclusão de 14/10 ("o método do Codifica Meu Volks não tem base pública"). Em `mib3 moi3` os únicos achados são o `n3oka` (aparência/dados, não grava) e os nossos dois repos.

  - **Extração funda do blob + a pergunta "dá pra quebrar a cifra?" (16/10/2026).** O Lipe pediu para extrair mais e, no meio, perguntou direto se dá para quebrar a cifra e, se der, fazer. **Análise estatística** dos 304 bytes: entropia 7,44 bits/byte, chi-quadrado **196,2** (bem abaixo do limite ~293, logo passa como aleatório em p=0,05), monobit **0,484** (0,5 = aleatório) — ou seja, **indistinguível de aleatório**. **Varredura de descompressão exaustiva** (zlib/deflate/gzip/lzma/bz2/zstd, em 24 deslocamentos, no corpo, no corpo XOR o magic e no corpo invertido): **zero acerto** — não é nenhum formato de compressão comum. **Varredura de texto claro**: nenhum VIN (`3VW`/`9BW`/`WVW`/… ausentes), nenhum JSON, nenhum DER/ASN.1 (`30 82`) — a parte cifrada começa já no byte 0, sem cabeçalho legível. Conclusão técnica: é um **bloco AES opaco**, provavelmente CBC (alinhado a 16, sem blocos repetidos). **Resposta à pergunta — NÃO, e não é questão de esforço:** um único bloco AES **sem a chave** é inquebrável por definição (espaço de 2¹²⁸); não há brecha a explorar num só texto cifrado, e a varredura prova que do nosso lado não sobra material. O **único** caminho que existiria seria **tirar a chave do instalador/app deles** (RE do binário) — que não temos, e que seria **contornar a trava de VIN / "NÃO TENTE COPIAR" para clonar um produto pago de terceiro**. Essa é a linha que **não se cruza** (não é sistema nosso; é defeat de proteção de produto comercial alheio). **Decisão registrada:** não tentar quebrar; o QR está esgotado como leitura. OSINT no fornecedor (vídeo do YouTube, reel `@pilotsibranco`) **não rendeu** detalhe técnico público novo — o `WebFetch` do YouTube só trouxe navegação, e a busca web caiu em fóruns genéricos de "Waze no Virtual Cockpit", nada do método deles.

  **Repositório publicado no GitHub (16/10/2026).** O código da bancada virou o repo público **[lipedesign/moi3-cluster-pipeline-re](https://github.com/lipedesign/moi3-cluster-pipeline-re)** (era `moi3-cluster-map-poc`; nome mudado a pedido do Lipe — "PoC" sugere funcionando no carro, o que não é o caso; é **RE + reprodução de bancada**, termo alinhado ao que a cena usa, ex.: `ludwig-v/...-reverse-engineering`). Decisões do Lipe: **público**, licença **MIT (código) + CC BY (docs)**, **sem** `Co-Authored-By` nos commits. **Antes de subir:** o shader `rgb_to_yuv` do binário foi **reescrito como BT.601 studio-swing padrão, nosso** (coeficientes derivados da norma ITU-R BT.601; os valores do binário são a mesma BT.601, logo nada se perde) — a fonte **verbatim** do binário **não** foi publicada; o shader codificador (passe B) e todo firmware/binário/sysroot ficam **de fora** (bring-your-own); README sem "PoC", com disclaimer "not affiliated" e nota de proteção. O repo de pesquisa **[mib3-moi3-lg-research](https://github.com/lipedesign/mib3-moi3-lg-research)** (já público) ganhou **destaque no topo do README** (callout + badge clicável) e uma linha no `open-questions.md` apontando pro repo novo (commit `e8f4704`). Conferência pré-publicação: 31 arquivos, 216K, **zero binário/firmware/imagem**, sweep sem "verbatim"/coeficiente do binário/dado pessoal. A cópia **local** de bancada (fora do repo, no scratchpad) segue usando o shader real do binário para a análise; só o repo **público** usa o nosso.

  - **Enriquecido no mesmo dia (16/10/2026).** Uma **revisão automática de segurança** achou um OOB read no `glforward-ext/apply.py` (handler `OPX_IMG_CREATE` usava `stride*h` sem conferir contra o tamanho do blob — a classe do SIGBUS de dims erradas que já nos mordeu); **corrigido** com validação de geometria overflow-safe. Além disso o repo ganhou: **Dockerfile** (ambiente de build/run, *test-buildado* num debian+Mesa, demo roda nele), **Getting Started** no README, **dois diagramas mermaid** (pipeline do carro + como a bancada substitui a Qualcomm), um **visual de resultado** (`docs/images/bt601-roundtrip.png` — round-trip BT.601 das barras, diferença **1 LSB**, gerado rodando o **shader reescrito** no container, o que de passagem **verificou** que ele compila/linka/roda — barra branca → Y=218, croma 128), **CONTRIBUTING.md** (créditos: lipedesign + Claude) e **SECURITY.md**, e **9 topics** no GitHub. **Co-Authored-By religado** a pedido do Lipe (reverteu o "sem" de antes — e é o que popula o grafo de *Contributors*): os commits dos dois repos foram **amendados** com `Co-Authored-By: Claude Opus 4.8` e **force-push** (repos de minutos, risco mínimo). Commits finais: novo **`3b7c71d`**, pesquisa **`537a17f`**. Reforça, agora **com foto e em MIB3**, a conclusão: a camada 3 na MOI3 LG é real em runtime (daChillout PoC + este produto comercial); o elo que não desenvolvemos é a **execução**, que este fornecedor tem — e eles já disseram ao Lipe que **dá no chassi dele**.

  **Varredura do GitHub pelo método (14/10/2026, depois do lead).** Busquei a possível base técnica do serviço: repositórios `mib3/moi3 carplay`, `carplay active info display`, `carplay virtual cockpit`, `mib3 multitouch`, `mib3 swdl/swup/patch/firmware tool`; e por **código** as strings internas do nosso firmware (`RouteGuidanceByMobileDevice`, `ff14::2:4`, `multi-screen-server`, `MIB3_KEY_ADAPTION_DASHBOARD_DISPLAY_CONFIGURATION`, `createOrUpdateStreamParameters`). **Nada para MIB3 LG (MOI3): zero repositório e zero ocorrência de código.** O único player LG continua sendo o `n3oka` (e só boot screen/gráficos). Em contraste, a cena de "CarPlay no Virtual Cockpit" está fervilhando — mas é **toda MHI2/MHI2Q (Audi/Harman; MOST/QNX/Tegra)**, não a nossa plataforma: vários repos novos e ativos (atualizados em 13–14/10/2026), p.ex. `yuedizhibo/MHI2Q-CarPlay-AltScreen` (397★), `Lanye-z/MHI2Q-CarPlay-MMI-Mirror` (358★), `kamgurgul/mib2q-carplay`, `stevenzhr/MU1320-CarPlay-RGI`, `chefranov/mhi2-au37x-carplay`, `aberbic/Audi-MHI2-...-CarPlay-AndroidAuto`, e o canivete `Mr-MIBonk/M.I.B.` (1153★). Conclusão: o método do Codifica Meu Volks **não tem base pública** — é privado; o GitHub não explica como eles fazem na LG.

  **Detalhe do n3oka (revisto em 14/10/2026 — 5 repos).** É o autor mais avançado em MIB3 **LG**, mas tudo dele é **camada de aparência/dados, não função nem gravação**: `MIB3-Boot-Screen-Simulator` (boot animation, lê até partições `.mbn/.img/.ext4`), `MIB3-Image-Resolver` (nomes/troca de imagens), `MIB3-3D-Viewer` (arquivos 3D MCF de unidade MOI3 LG, troca de texturas), `MQBevo-Dataset-Editor` e `A5-2Q0-5WA-Dataset-Editor` (datasets de veículo e da câmera 2Q0/5WA — este último interessa ao projeto 01/Lane Assist). O ponto decisivo: o próprio simulador diz **"never writes to the unit … how and whether you transfer images to a unit is your own responsibility"** — ou seja, ele **edita os arquivos no PC mas não grava na central**. Nem o player mais avançado de LG resolve publicamente o **gargalo da gravação na central assinada** — que é justamente o pulo do gato do Codifica Meu Volks.

  **Pesquisa em fóruns indianos (Team-BHP), 14/10/2026 — confirma o padrão e acrescenta a camada 2.** O Team-BHP (maior fórum VAG da Índia, cena forte de OBDeleven/VCDS em Skoda/VW) só tem retrofit de CarPlay em **MIB2** (ativação por dealer/coding); **nada de mapa cheio do app no cluster em MIB3**. Um relato (fórum SEAT) resume: o "workaround" do MIB2 (troca do `overall.nds`) **foi fechado na MIB3**. No cluster, o que aparece é o mapa **nativo** (girar o botão para "Navigation"); o Google Maps fica na tela central. **Achado novo e testável no carro:** vários donos de Skoda/VW com MIB3 relatam que as **setas de manobra (turn-by-turn) do telefone sobem para o cluster** — com **Apple Maps (CarPlay)** de forma consistente; com **Google Maps/Android Auto** os relatos divergem. Isso é a **camada 2** (setas do app no cluster), e parece ser **comportamento de fábrica do CarPlay** na MIB3, não coding. Por coding/OBDeleven, na MIB3 só aparecem temas do cluster, layouts e **"wireless CarPlay" (na central, não no cluster)**; coding de cluster está ficando travado ao VIN. Conclusão: reforça que o mapa **cheio** no AID (camada 3 — o que o story do Codifica Meu Volks mostra) **não é nativo, nem coding, nem público**; e abre uma verificação **de graça**: testar no carro se as setas do CarPlay já sobem ao cluster (camada 2). Fontes: [OBDeleven codes/adaptions VW-Skoda](https://www.team-bhp.com/forum/technical-stuff/265035-obdeleven-codes-adaptions-volkswagen-skoda-4.html), [Kodiaq facelift (mapa nativo no cluster)](https://www.team-bhp.com/forum/official-new-car-reviews/246986-2022-skoda-kodiaq-facelift-review-2-0l-petrol-dsg-116.html), [Octavia (setas do CarPlay no cluster)](https://www.team-bhp.com/forum/official-new-car-reviews/237784-skoda-octavia-2-0-tsi-dsg-review-37.html).

  **Repositório público criado (14/10/2026).** Por decisão do Lipe, a pesquisa de plataforma (arquitetura, estrutura do firmware e o pipeline do mapa no cluster) foi estruturada, sanitizada e publicada como repositório público no GitHub, sob a conta `lipedesign`, para fomentar a comunidade: **https://github.com/lipedesign/mib3-moi3-lg-research**. Contém `README.md`, `docs/architecture.md`, `docs/cluster-map.md`, `docs/firmware.md`, `docs/open-questions.md`, `CONTRIBUTING.md` e `LICENSE` (CC BY 4.0 nos docs + MIT no código). O conteúdo deriva desta pesquisa, mas **sanitizado**: sem VIN/placa/local/nome, sem o lead comercial, e com o contorno de proteções (secure boot/dm-verity/assinatura VW) explicitamente **fora do escopo**. Fonte local: pasta `mib3-moi3-lg-research/` na raiz do projeto. Commit assinado com o e-mail noreply do GitHub (não expõe e-mail pessoal). Varredura de sanitização feita no repo inteiro antes de publicar: zero dado identificável. **Expansão (15/10/2026):** acrescentados `docs/coding-model.md` (modelo de codificação/adaptação — DIDs, campos e a cadeia do mapa nativo `0B5A`→`0x19a`→`VideoNavigationSupported`), `docs/bench-emulation.md` (metodologia da bancada de estudo da HMI, read-only), `docs/landscape.md` (projetos públicos — a cena Harman MHI2 e as ferramentas MIB3 LG) e `docs/glossary.md`; mais diagramas Mermaid (pipeline do mapa, as 3 camadas, barramentos) e templates de issue/PR. Mesma varredura de sanitização e linha das proteções antes do push (commits `b04ce7c`, `3c57431`). **Acrescentado em seguida (commit `44a73df`):** `docs/hmi.md` (a HMI por dentro — app Java/OSGi, OpenGL/Weston, serviços DSI e VIWI/RSI, e como ela decide o que mostrar a partir da adaptação); uma tabela de trains/versões conhecidos no `firmware.md`; e diagramas Mermaid do software stack e da cadeia de boot. README repaginado (tagline, badges, diagrama "hero", índice em tabela). Total ~3.000 linhas, **9 documentos**. SECURITY.md foi descartado por decisão do Lipe (foco em conteúdo).

  **Em aberto, para retomar (15/10/2026) — nada disso toca a central; é contato/decisão/leitura:**
  - **Divulgar o repo.** Textos prontos (no chat): post longo para fórum/Reddit (inglês) e mensagem curta para grupo de Telegram gringo (abre com "Hey guys"). O Lipe posta quando quiser.
  - **Contato `omonob`.** Mensagem pronta para o autor do repo público `omonob/MHI2-Carplay-Maps` (cena Harman MHI2), perguntando se conhece algo equivalente para MIB3 LG. Aguardando o Lipe enviar.
  - **Serviço comercial que alega fazer o mapa do app no AID da MOI3 LG** (confirmou compatibilidade com o chassi): lista de perguntas preparada (o que exatamente faz, se modifica firmware, **reversibilidade**, **risco de brick/recovery** — lembrar que o `R8030L`/MU0803 não está disponível publicamente —, garantia, efeito de atualizações). Decisão e envio com o Lipe.
  - **Teste grátis no carro — "camada 2":** com CarPlay ligado, ver se as setas de manobra (turn-by-turn) do telefone sobem ao cluster (testar Apple Maps, Waze e Google Maps) e relatar. Fecha a dúvida da camada 2 com dado do carro.
  - Opcional: banner/logo no topo do README (capricho visual), não feito.
  - **Issue #1 do repo, "Why LG?" (aberta em 15/10/2026, 09h29 UTC, por `Mjukes`, que assina Mike)** — [link](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1). Pergunta por que o repo é só LG e não JOYNEXT, e se não funcionaria nas MOI3 de sufixo P. **Contexto conferido:** "Preh" e "JOYNEXT" são a mesma variante: a Preh Car Connect (ex-TechniSat Automotive, comprada em 2016) virou a divisão JOYNEXT da Joyson em 2020 ([NDS Association, 10/06/2020](https://nds-association.org/?p=1005)); o repo chama de "Preh" quase sempre e só a tabela de SoC do `architecture.md` diz "Preh / JOYNEXT". **O que o repo já sustenta para a resposta:** a análise de firmware é toda de trains LG (a central do Jetta é `R8030L`; os pacotes lidos são `R8410L` e `R8830L`); a Preh é outro hardware (Renesas R-Car M3, Yocto 4.14), então boot, proteções e o lado `/lge/...` do fluxo (codificador `OMX.qcom`, `multi-screen-server`) não valem para ela. **Deduções, não conferidas numa central P:** (a) o motor que desenha o mapa na LG é o `tsd.nav.mainapp.mib3`, software da própria Preh Car Connect/JOYNEXT; (b) a central anuncia o fluxo ao painel pela função BAP `VideoStreams` (tipo, IPv6, porta, URI), e BAP é protocolo da VW, então uma central P falando com o mesmo painel teria de usar a mesma função; (c) os nomes de adaptação (`Dashboard_Display_Configuration`, `0B5A`) são da VW e devem existir na P, talvez com outro byte/bit. Já é a pergunta "Preh vs LG" do `open-questions.md`. **Situação:** respondida em 15/10/2026, às 10h27 (horário de Brasília), com o sim do Lipe ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6193203787)), depois de uma conferência das afirmações. **O que a conferência confirmou:**
    - **No firmware MU0883 da bancada** (extraído do `system.img` com `debugfs`):
      - o motor `tsd.nav.mainapp.mib3` (pacote `NAVCOREV2_LG_MOI3_EU_RDW2_VW_MQB_SingleMap_sec_028.42.4`) traz um contato `@preh.de` numa mensagem de erro; a palavra "TechniSat" não aparece no binário;
      - a `libMIBNavSDClientLib_CarIF.so.00` tem a função BAP `Viwi::Bap::NavigationSd::VideoStreams`, com os campos `StreamType`, `Ipv6Address`, `UdpPort` e `Uri`;
      - na mesma biblioteca, o grupo está gravado só por extenso (`ff14:0000:0000:0000:0000:0000:0002:0004`), por isso a resposta pede para procurar `ff14:`;
      - o `multi-screen-server` usa o `OMX.qcom.video.encoder.avc`, e o `/etc/os-release` diz AGL 3.0.0 "chinook".
    - **Nos slides da PCAutomotive** (Black Hat EU-24): a central Preh é Renesas R-Car M3 com "Yocto Linux 4.14.75" e tem ligação "Eth Base-T1" com o painel de instrumentos.
    - **Na leitura do ODIS do 5F (14/10/2026):** `Dashboard_Display_Configuration` → `navigation_map_transmission_mode` = `IP_streaming`.

    **O que mudou do rascunho:**
    - saiu "TechniSat";
    - "firmware.md não vale" virou "a maior parte do firmware.md";
    - entrou a ligação Ethernet da Preh com o painel;
    - os nomes de codificação passaram a ser "dos dados de diagnóstico da VW";
    - a busca pedida passou a ser por `ff14:`.

    **Resposta do Mike (15/10/2026, 10h53 de Brasília, [comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6193752068)).** É relato dele, não conferido por nós.
    - **A central dele:** train `MOI3_EU_VWMQB_P33000P` (Preh, MU 0330 pela tabela do `firmware.md`). Ele não disse modelo, ano nem mercado.
    - **O que ele diz ver no painel:** o mapa de fábrica aparece. Aparecem também as setas da navegação de fábrica e as do Apple Maps e do Google Maps no iPhone. É dado da "camada 2" do `open-questions.md`, onde havia relatos conflitantes sobre o Google Maps.
    - **A adaptação `Dashboard_Display_Configuration` dele, comparada com a do Jetta** (leitura do ODIS de 14/10/2026, `5F-codificacao-e-adaptacoes.tsv`, linhas 1344 a 1353):
      - os mesmos 10 campos, na mesma ordem;
      - os mesmos valores em 9 deles: `IP_streaming`, `H264`, resolução 1, cruzamento `off` e 0, capa `bap`, ícone dinâmico `off`, foto de chamada `bap` e logo da rádio `bap`;
      - **a única diferença é `ability_switch_nav_maps`: `no` na dele e `yes` no Jetta.**
    - **Achado que isso provocou (conferido no binário MU0883, 15/10/2026).** O repo afirma, no `cluster-map.md` §4.1 e no `coding-model.md` §9 e §10, que esse bit é o que liga o mapa nativo no painel. O nome completo da função BAP que ele alimenta, lido nos símbolos da `libMIBNavSDClientLib_CarIF.so.00`, é `FsgSetup::...FunctionSupportActivationDeactivationOfTheVideoNavigationSupportedDf44Bit0`. Ou seja, o bit anuncia que a central aceita **ligar e desligar** a navegação em vídeo; ele não é o mapa em si. Isso bate com o nome do campo ("habilidade de trocar os mapas"), com a coluna "Meaning" do próprio `cluster-map.md` §4 ("enables moving the map between radio screen and cluster") e com a central do Mike, que mostra o mapa no painel com o bit em `no`. **A afirmação do repo foi além da prova e precisa ser corrigida.** O que manda o mapa para o painel é mais provavelmente `navigation_map_transmission_mode = IP_streaming`, como já diz o diagrama do README. Isso é dedução: falta conferir na central LG com o bit em `no`.
    - **Pergunta que fica para o Mike:** com o mapa no painel, a tela central mostra o mapa ao mesmo tempo? Dá para tirar o mapa do painel ou trazer de volta para a central? No Jetta, com `yes`, o mapa sai da central quando vai para o painel (relato do Lipe, ficha, 11/10/2026). O firmware LG tem os dois pacotes de navegação, `SingleMap` e `DualMap`.
    - **Diferença à parte, para conferir:** o `coding-model.md` diz que os campos de cruzamento ficam num DID separado, o `0B26`. Mas o ODIS mostra `crossroad_detail_map_*` dentro de `Dashboard_Display_Configuration`, no Jetta e no Mike, e também em `Headup_display_configuration`, este com compressão `RLE`.

    **Ainda em aberto:**
    - ~~responder ao Mike~~: respondido em 15/10/2026, às 11h13, com o sim do Lipe ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6194174481)). A resposta reconhece o erro do Map Switch, promete corrigir o repo dando o crédito a ele e faz as duas perguntas: se o mapa aparece na central e no painel ao mesmo tempo, e qual é o modelo, o ano e o mercado;
    - ~~corrigir no repo a afirmação sobre o Map Switch e acrescentar o dado do Mike~~: **enviado ao GitHub em 15/10/2026, com o sim do Lipe ("Sim faz"), no commit [`de9a0ec`](https://github.com/lipedesign/mib3-moi3-lg-research/commit/de9a0ec)**, exatamente o diff mostrado a ele. Mexe em quatro arquivos:
      - `cluster-map.md`: a §4.1 virou "The Map Switch chain — and what it does *not* prove", com nota de correção datada e crédito a @Mjukes; a tabela da §4.2 põe a central LG (MU 0803) ao lado da Preh do Mike (MU 0330), com os 10 campos; mudaram também a linha da camada 1, o dado da camada 2 e a tabela da §8;
      - `coding-model.md`: a §9 virou "The native cluster map — this **is** coding"; mudaram também a tabela da §5.1, a introdução e a tabela da §10;
      - `README.md`: saiu "One coding bit switches it on";
      - `open-questions.md`: três dados novos e duas perguntas novas (`ability_switch_nav_maps = no` numa LG; ligar o byte 0 ao `ipStreamingEnabled`).

      O papel do byte 0 (`navigation_map_transmission_mode`) como o que liga o mapa ficou marcado `[inferred]`;
    - sugestão: deixar claro no README que sufixo P = Preh = JOYNEXT.

    **Segunda resposta do Mike (15/10/2026, 11h34 de Brasília, [comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6194620481)).** É relato dele, não conferido por nós.
    - **O carro:** "2020 GTE", mercado do Reino Unido. Ele não disse qual modelo GTE.
    - **O teste:** ele mudou o `ability_switch_nav_maps` no carro dele.
      - Com `no`, o mapa aparece na central e no painel ao mesmo tempo, e o menu da navegação não tem opção de troca. Ele pode ter o mapa de fábrica no painel e o CarPlay na central, mas só um dos dois mapas tem rota ativa por vez.
      - Com `yes`, aparece o botão de troca: o mapa de fábrica vai para o painel e a central mostra só uma bússola, e vice-versa.
      - Ele não testou o CarPlay com `yes`.
    - **O que isso fecha:** o campo escolhe entre "um mapa, que se troca de tela" e "um mapa em cada tela". Confirma a correção do `de9a0ec` com um teste no carro, não só com o nome do símbolo.

    **Conferido no firmware LG MU0883 (15/10/2026).** O `/lge/app_ro/post_init/data/navigation.sh` escolhe na primeira partida o pacote do motor de navegação: `eu/single` ou `eu/dual` (e os de `nar`). Quem decide é o pino da placa `/sys/devices/soc/soc:common_gpio_default/grade_id` (`0x0` = single, `0x1` = dual), não a codificação. O pacote `single` tem um renderizador visível por vez, e o `dual` tem dois (tabela das janelas de desenho, mais abaixo neste arquivo). Numa LG de placa "single", o campo em `no` talvez não dê os dois mapas. O Jetta se comporta como `single`, mas o `grade_id` dele não foi lido.

    **Situação em 15/10/2026, 12h28 — publicados os dois, com o sim do Lipe ("Pode enviar o acréscimo ao repo e postar a resposta ao Mike"):**
    - **Acréscimo ao repo:** commit [`5514ad2`](https://github.com/lipedesign/mib3-moi3-lg-research/commit/5514ad2), o diff mostrado a ele. Tem o teste do Mike na §4.1 do `cluster-map.md` e no `coding-model.md` (§5.1 e §9); na §4.2, o carro dele e a escolha `SingleMap`/`DualMap` pelo `grade_id`; no `open-questions.md`, a pergunta reescrita.
    - **Terceira resposta ao Mike:** postada às 12h28 ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6195778818)), o texto aprovado.
    - **Ainda em aberto:**
      - acompanhar a issue;
      - ler o `grade_id` da central do Jetta, se um dia houver acesso de leitura à placa;
      - rastrear no código o caminho do byte 0 até o `ipStreamingEnabled`;
      - a sugestão do README (sufixo P = Preh = JOYNEXT).

    **Mais dois comentários do Mike (15/10/2026, 13h24 e 13h42 de Brasília).**
    - [13h24](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6196936461): pergunta se a central do Lipe é Discover Pro, o que explicaria a diferença dos mapas, e sugere mudar a adaptação no Jetta para ver se ele se comporta como o carro dele.
    - [13h42](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6197298446): "CarPlay map still shows when switch maps is set to ON". Com o campo em `yes`, o mapa do CarPlay continua aparecendo. Ele não disse em qual tela.

    **O que foi conferido para responder (15/10/2026):**
    - **Os dois pacotes de navegação da LG têm telas centrais de tamanhos diferentes.** O `SingleMap` (`VW_SINGLEMAP_EUROPE_EMBEDDED_MQB`) desenha o mapa da central em 800×480; o `DualMap` (`VW_DUALMAP_EUROPE_EMBEDDED_MQB`), em 1280×640. Nos dois, o painel é 1280×480 (`traces/vc_inv_config_motor.txt`).
    - **A central do Jetta é a variante "Small":** HMI `H40.272.151_OISmall2_EU`, lida nas fotos da tela (ficha). A HMI da bancada é a de 800×480.
    - **O wiki do VCDS.de diz** que a MOI3 existe em telas de 800x400 ou 1280x640 e é vendida como Discover Media ou Discover Pro (ficha, seção do wiki).

    **Dedução, não conferida:** na LG, os dois mapas vêm junto com o hardware de tela grande, a classe da Discover Pro. A central "Small" do Jetta recebe o pacote de um mapa só, seja qual for a adaptação. O nome comercial da central do Jetta (Media ou Pro) não foi conferido, e o `grade_id` dela não foi lido. O modelo do GTE do Mike também não é conhecido.

    **Sugestão do Mike de mudar a adaptação no Jetta:** é gravação no carro, então é decisão do Lipe e segue as regras de trabalho no carro do `CLAUDE.md`:
    - autoscan completo antes;
    - valor original anotado (`ability_switch_nav_maps` = `yes`, na leitura de 14/10/2026);
    - capô aberto por causa do firewall de diagnóstico.

    Nada foi proposto como feito.

    **Quarta resposta ao Mike:** postada em 15/10/2026, às 15h35, com o sim do Lipe ("Pode postar a resposta ao Mike como está"); [comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6199474852). O texto:
    - explica que a central do Jetta é a variante `OISmall2_EU`;
    - explica que o `SingleMap` (800×480) e o `DualMap` (1280×640) acompanham o tamanho da tela, com o pacote escolhido pelo `grade_id`;
    - dá como palpite que os dois mapas vêm com a tela grande, da classe da Discover Pro, e que o nome comercial da central do Jetta não foi confirmado;
    - não promete o teste no carro: diz "I'll report back if I try it";
    - registra o relato do CarPlay.

    Antes de postar, a issue foi conferida e não tinha comentário novo. Falta acompanhar a issue.

    **Seis comentários novos (lidos em 20/10/2026, 07h28; horas em Brasília).**
    - **17/10, 08h14, Mike** ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6233230439)): manda um reel do Instagram, "Have you seen this?". É o mesmo material do `@codificameuvolks` com o `@pilotsibranco` ("WAZE E GOOGLE MAPS DIRETO NO PAINEL DA VOLKSWAGEN!"). A legenda diz que a solução foi desenvolvida no Brasil pelo Henrique. O reel já tinha sido analisado em 16/10/2026, nas notas "QR decodificado" deste arquivo.
    - **19/10, 15h05, `jilleb`** ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6273881596)):
      - diz que solução fechada desse tipo "is bad news", porque pode comprometer a segurança e a privacidade do sistema e do laptop que a roda;
      - diz que tem feito muita pesquisa de MOI3 nas últimas semanas e que "it's time to release the MOI3 toolbox".
    - **Quem é o `jilleb`:**
      - autor do `jilleb/mib2-toolbox` (1.028 estrelas em 20/10/2026);
      - o nome no perfil do GitHub é "Chillout";
      - é o mesmo daChillout do drive2, autor com o Meich da prova de conceito do mapa do celular no painel da MOI3. Isso foi confirmado em 18/10/2026 em `referencia/coding-geral.md`, pelo post dele no drive2 sobre o mqbtools.nl.
    - **O `jilleb/mib3-toolbox` público ainda não tem o "MOI3 toolbox"** (conferido em 20/10/2026):
      - tem só templates do 010 Editor para os formatos GCA, MCF e OPS e um extrator de `.anim`;
      - o último push é de 24/08/2026.
    - **19/10, 15h19, Mike:** pede ao `jilleb` que arranje uma central JOYNEXT.
    - **19/10, 15h33, `jilleb`:** "I dont have the money to get more hardware". Isso indica que a central dele é LG, mas é dedução nossa.
    - **19/10, 16h03, Mike:** espera que o trabalho do `jilleb` inspire um dono de JOYNEXT. Conta que "o cara do Drive2" disse ter quebrado a MIB3 chinesa, ainda sem prova.
    - **19/10, 19h24, `Alherdc`** ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6278703634)): "Could you do mine for me? It's an LG 3G5035832 with the 8830 system."
      - A conta foi criada no mesmo dia e não tem repositórios.
      - Não está claro se o pedido é ao `jilleb` ou a nós.
      - É a mesma família de part number da central do Jetta (`3G5 035 832 C`) e o software MU0883.

    **Quinta resposta, postada em 20/10/2026, às 07h41, com o sim do Lipe ("Pode postar a resposta na issue como está")** ([comentário](https://github.com/lipedesign/mib3-moi3-lg-research/issues/1#issuecomment-6289068895)). Antes de postar, a issue foi conferida e não tinha comentário novo.
    - **Ao `jilleb`:**
      - concorda que ferramenta fechada pede confiança demais;
      - diz que a prova de conceito dele com o Meich já tem crédito no `cluster-map.md` §6 e no `landscape.md`;
      - oferece o `cluster-map.md`, o `coding-model.md` e o repo `moi3-cluster-pipeline-re`;
      - se oferece para pôr o link do toolbox no repo quando sair e para comparar a central dele com as MU 0803 e 0883.
    - **Ao `Alherdc`:** o repo é só pesquisa de leitura, nada nele instala ou muda coisa no carro, e não há serviço. Se o toolbox do `jilleb` sair aberto no GitHub, dá para ver o que ele faz antes de rodar.
    - **Ao Mike:** sim, já tinha visto; é um serviço pago aqui no Brasil e fechado, então não há o que conferir nem documentar.

    A resposta é neutra sobre o serviço brasileiro: não publica nada da análise do token do QR feita em 16/10, porque essa análise mexe no lado de acesso e de licença do produto de terceiro. Falta acompanhar a issue e o lançamento do MOI3 toolbox.

    **Contato com o `jilleb` (pedido do Lipe em 20/10/2026: "queria contatar ele").** Canais públicos que ele mesmo publica, conferidos em 20/10/2026:
    - **E-mail no perfil do GitHub:** `jjberends@gmail.com`. O perfil também diz "the Netherlands" e o nome "Chillout".
    - **A issue #1 do nosso repo:** ele já participa dela. A quinta resposta, das 07h41, menciona ele, então ele recebe aviso.
    - **drive2.ru,** como daChillout: é onde ele publica os posts de MOI3.
    - **Patreon** `jille` e PayPal `chillout1`: estão no `FUNDING.yml` do `mib3-toolbox`.
    - **Não usar:** a issue #1 do `mib3-toolbox` ("Contributions", 2022, sem resposta) e o `mib2-toolbox`, que fogem do assunto.

    **O "toolbox para MIB3" do GitHub dele** (`jilleb/mib3-toolbox`, criado em 2021) ainda é só de formatos de arquivo (GCA, MCF, OPS, `.anim`). O MOI3 toolbox anunciado na issue não está publicado.

    **Cuidados levados ao Lipe:**
    - Mandar do Gmail pessoal mostra o nome e o e-mail dele. O repo usa a persona `lipedesign` com e-mail noreply justamente para não expor isso.
    - Mandar mensagem em nome do Lipe só com o sim dele.
    - Rodar o toolbox no Jetta, quando sair, seria gravação na central e segue as regras de trabalho no carro.

    Rascunho de e-mail mostrado em 20/10/2026, nada enviado.

    **Rascunho criado na conta errada (20/10/2026).** O Lipe escolheu o e-mail e pediu o rascunho "no meu Gmail".
    - O único conector de Gmail que funciona nesta sessão criou o rascunho em **`felipe.paulino@12min.com`**, não no Gmail pessoal (id `r-6938714601427217921`, assunto "MOI3 (LG) research: lipedesign / mib3-moi3-lg-research", para `jjberends@gmail.com`). Nada foi enviado.
    - O outro conector de Gmail (plugin small-business) falhou na autenticação: "Incompatible auth server".
    - **Pedido ao Lipe:** apagar ele mesmo esse rascunho na conta da 12min (apagar fica com ele) e mandar do Gmail pessoal, colando o texto, ou ligar o Gmail pessoal nos conectores do claude.ai.
    - **O Lipe confirmou ("Não é nessa conta do 12min, é no outro Gmail").** Conferido em 20/10/2026:
      - o conector "Gmail" do claude.ai desta sessão está ligado à conta da 12min;
      - o conector de Gmail do plugin aparece como "connected", mas falha em toda chamada, com o mesmo erro de autenticação;
      - não há como criar o rascunho no Gmail pessoal daqui. O texto foi passado na conversa para ele colar.
    - **Mudança de canal (20/10/2026):** o Lipe decidiu mandar a mensagem pelo drive2 ("vou mandar uma mensagem para ele no Drive2").
      - O texto foi adaptado do e-mail: mais curto, em inglês (o perfil avisa que ele não entende russo), com os links do repo e da issue #1.
      - Quem envia é o próprio Lipe, pelo botão "Сообщение" do perfil [daChillout](https://www.drive2.ru/users/dachillout/). Daqui nada foi enviado.
      - **Enviada pelo Lipe em 20/10/2026.** A captura de tela que ele mandou às 18h38 mostra a conversa "Переписка с daChillout" (`drive2.ru/my/messages/3031776`), com a mensagem inteira e a marca "сейчас" ("agora"). O texto é o adaptado, sem mudanças. Falta acompanhar a resposta no drive2 e na issue #1.
    - **Busca por outros canais:** interrompida pelo Lipe no meio. As duas buscas na web que chegaram a rodar (`"jilleb" mib2 toolbox`; `"daChillout" OR "chillout1" MIB MQB`) voltaram vazias.

    **Redes sociais do `jilleb` (pedido do Lipe em 20/10/2026).** A busca ficou nos canais que ele mesmo liga ao trabalho, pelos apelidos `jilleb`, daChillout e Chillout. Não foi feita busca pelo nome real.
    - **drive2:** perfil [daChillout](https://www.drive2.ru/users/dachillout/).
      - Amsterdam, Holanda; carro atual Cupra Formentor VZ Performance eHybrid; carros anteriores Golf GTE e SEAT Leon FR ST.
      - 570 seguidores, 11 anos no site; visto "7 horas atrás" em 20/10/2026.
      - O perfil tem botão de **mensagem** e avisa que ele não entende russo: escrever em inglês.
      - O título do post da prova de conceito é "**MOI3 (LG)** Android Auto maps on dashboard". Logo, a central dele é LG.
    - **mqbtools.nl** ("MQB Tools by Jille"): só liga para o GitHub `jilleb` e para o PayPal `chillout1`.
    - **README do `mib2-toolbox`:** só PayPal e Patreon `jille`. A página do Patreon não carregou sem login.
    - **Busca na web** por `jilleb`/daChillout com Reddit, Instagram, YouTube e Discord: nada.
    - **Canais para falar com ele:** o e-mail do perfil do GitHub, a mensagem do drive2 e a issue #1.

  **ACHADO IMPORTANTE (15/10/2026) — a "camada 3" foi demonstrada em runtime, e corrige nosso veredito.** Post público de **`daChillout`** no **drive2.ru** (14/10/2026, "MOI3 Android Auto maps on dashboard"), num **Cupra Formentor VZ 2024 (mesma plataforma MOI3)**: proof-of-concept do mapa do **Android Auto (Waze / Google Maps)** aparecendo **no cluster**, funcionando. Pelo relato, o método **NÃO é patch de firmware assinado** (o que a gente supunha ser o único caminho): é um **serviço custom em runtime, rodando de `/tmp`** (reboot apaga, não toca a partição `system`/dm-verity), que usa o **RSI** (ele cita o `rsi-test`), captura o surface do Android Auto, **croppa para 1280×480** e empurra para o display do cluster. Isso usa **exatamente** os mecanismos que a nossa RE mapeou (RSI/VIWI, `multi-screen-server`, o stream **1280×480** do cluster) — ou seja, **valida a nossa engenharia reversa** e **corrige a conclusão**: a camada 3 é alcançável **em runtime**, não só por patch de firmware. O autor diz que vai **tornar open source** ("knowledge is power, shared knowledge is power multiplied"). Feito com `Meich`.
  - **Cena nova não mapeada:** há atividade de MOI3 no **drive2.ru** (comunidade russa) que não vimos (só varremos GitHub e Team-BHP). `daChillout` tem também um "GCA editor (For MIB2 and MOI3)".
  - **A nossa linha:** o mecanismo de **exibir** (serviço RSI em runtime, de `/tmp`, sem tocar no firmware assinado) não fura secure boot/dm-verity. O que o post **não** detalha — e que não desenvolvemos — é **como obter o acesso de execução** na unidade.
  - **A fazer:** acompanhar o open source; **atualizar o `cluster-map.md`** do repo (hoje diz que a camada 3 é "firmware-gated / out of scope") com honestidade — há um caminho em runtime demonstrado; possível contato com o autor (é colaborativo e pró-open-source).
  - **Vasculhamento do drive2 (15/10/2026):** `daChillout` é de **Amsterdam**, membro antigo da cena de multimídia VAG (não é russo; o drive2.ru é internacional). Mantém o site **mqbtools.nl** com ferramentas web, entre elas um **GCA editor** (edita arquivos GCA de **MIB2 e MIB3/MOI3**: layouts de botão, zonas de toque e de luz ambiente). Faz RE de formatos binários do MIB há anos e é conectado ao `jilleb/mib2-toolbox` (cita como via de upload no MIB2). Sobre **instalar no carro**, ele é explícito que não ensina ("I give you the saw, not how to build the house") — o método de execução/instalação na MOI3 ele **não publica**, o que mantém a nossa linha intacta. `Meich` é coautor do PoC do mapa. A cena vive no **drive2.ru**, um hub que não tínhamos mapeado (só varremos GitHub e Team-BHP).
  - **Mais da cena (15/10/2026, posts enviados pelo Lipe):** `aktec` (Tiguan 2G) é o dev de um projeto **App-Connect + AID** (CarPlay/Android Auto no painel) muito aguardado; em 14/10 anunciou que está **refazendo do zero** (o anterior era PoC) e escrevendo "para ambos os sistemas". `VoobsheNeOchem` (Golf Alltrack, **MIB2**) documentou o método **MIB2** já consolidado: mapas (Yandex/2GIS) no AID via **MQB Toolbox v4.2** (GitHub) + **Green Menu** (`mqbcoding/customization/carplay-maps-vc`) + cartão SD, com patch vendido por terceiro (versão paga ~399 ¥); relata bugs de imagem/codec. Outros nomes: `mrholodoff`, `vagiz86`. **Leitura:** para **MIB2** o caminho é toolbox + green menu (patch pronto, estabelecido); para **MOI3 LG** o caminho é o runtime-via-RSI do `daChillout` (a sair) e, possivelmente, o projeto do `aktec` se cobrir MOI3.
  - **Dois posts do drive2 (lidos em 16/10/2026, enviados pelo Lipe).** Nenhum é "Waze no cluster do Jetta pronto para usar": um é de **outra plataforma**, o outro é de **outra função**.
    - **[romanreall, "встроенные карты при Гугл картах"](https://www.drive2.ru/l/608560445594293216/)** (18/01/2022, Škoda Superb Mk3, **MIB2 High / QNX**): o truque de **navegação dupla** — reabilitar a nav **nativa** no cluster ao mesmo tempo que o Google Maps (Android Auto) roda na central, cada uma com destino próprio. Método: **MIB Toolbox** (`jilleb/mib2-toolbox`) por **cartão SD** + opção do GEM `mqbcoding-customization-navigation-ignore navigation` / "enable dual navigation" do **MIBbash**; recálculo de código pelo `mib-helper.com/im-so-xory` ou pelo próprio MIBbash. **É MIB2, não a nossa MIB3**, e o que sobe ao cluster é o **mapa NATIVO** (render do carro), não os pixels do app. É o mundo "sem ponto de proteção" (toolbox por SD) a que o Lipe se referia — exatamente a porta que a MIB3 LG fechou. Reforça a leitura do bullet de cima (VoobsheNeOchem, MIB2 = toolbox + green/GEM menu).
    - **[daChillout, "MOI3 MCF file reverse engineering — part 2 (Language file editing)"](https://www.drive2.ru/l/738361916421837584/)** (14/07/2026, Cupra Formentor VZ 2024, **MOI3 — a nossa plataforma**): **não é o PoC do mapa**; é RE do **formato MCF** ("MOI/MIB Container Format": header de 20 bytes + TOC + chunks `zlib` `78 DA` com CRC32). O `i18ndata.mcf` é a **tabela de tradução** de toda a UI, lida pela **camada Java** (classe `I18nDataReaderImpl`), tags `OBJA`, magic `"I18n"` `0x6E383149`, 92 áreas × 34 idiomas, string pool UTF-16LE + lookup `scalars[]` (dois saltos: `scalars[]`→índice→`texts[]`). Cita o motor de render nativo **Kanzi** (as variantes de MCF de cena 3D / atlas de imagem vão pro Kanzi; a de i18n não, é só Java). Ferramenta pública nova pra nossa plataforma: **`mqbtools.nl/mcf_universal/`** (editor de MCF; não aberto por mim). Nos comentários, daChillout confirma que ainda está **"working on a way to get things onto the MOI3 in an easy way"** — ou seja, o **método de execução/instalação na MOI3 segue não publicado** (nossa linha de escopo intacta); e que o MHI2 usa `.res` (mais simples, via Mib2toolbox).
    - **O que isso rende pra nós:** o **formato MCF + o engine Kanzi** são fatos novos e concretos da nossa unidade — úteis pra deixar o **MIB3 Simulator** mais fiel (ler textos/assets reais da UI MOI3), trabalho que é nosso e **sem exploit**. E fecha de novo o diagnóstico pro **Jetta** (MIB3 LG): o apoio de **execução** na unidade gateia **tudo** na MOI3 pública hoje — não só o mapa (camada 3), mas até editar texto/idioma por MCF precisa do mesmo "pôr arquivo na unidade" que o daChillout ainda não publicou. Na MIB2 isso é trivial (toolbox por SD); é a diferença das duas plataformas. O lado display/conversão da camada 3 nós já provamos na bancada; o elo que falta segue sendo o de execução, que não desenvolvemos.

  **Dissecação (15/10/2026) — o mecanismo do PoC cravado pelos binários.** Extraí do `system.img` (MU0883, via `debugfs` do e2fsprogs) e analisei por strings os binários de `/lge/app_ro/graphics/multi-display/` (o REA/Hopper esbarrou numa permissão de automação do macOS — "Not authorized to send Apple events to Hopper"; a análise saiu por CLI). Achados:
  - **`multi-screen-server`** é uma ferramenta **parametrizável**: `Usage: multi-screen-server <ip> <port> <-m/-u> <surface ID to share> <fps> [<surface2> <fps2>]`. Pega um **surface do Wayland por ID (via `ivi_share`)**, converte rgba→yuv (`Rgba2yuv`, shader EGL), codifica em **`OMX.qcom.video.encoder.avc`** (H.264, buffers `/dev/ion`) e transmite por **UDP na `eth0.5` (VLAN 5)**, multicast (`-m`) ou unicast (`-u`, desabilitado).
  - **`rsi-display-management`** é o serviço **RSI** que expõe `/displaymanagement/renderingSurfaces` e `/displaymanagement/streamParameters` com create/update/read — é o lado parametrizável do `createOrUpdateStreamParameters`.
  - **`rsi-test`** cria um surface próprio (`TriangleRenderer` — o "triângulo animado" que o `daChillout` citou) e o transmite em "RSI Testing Mode", com classes de stream `UDPStream`/`RTPStream`/`VideoStream`.
  - **Conclusão:** o caminho do mapa pro cluster é **configurável em runtime** — apontar o `multi-screen-server` para o surface de um **app** (em vez do surface do `naviengine` nativo), cropado a 1280×480, transmite esse app pro cluster pelos **mesmos serviços**, **sem patch de firmware**. É exatamente o que o PoC do `daChillout` faz. O passo de **display é mecanismo existente/legítimo**; o que fica fora (e não analisamos) é **como obter execução** na unidade. Binários em `scratchpad/fw-bins/`; análise estática, nada executado.

  **Dissecação do `libMIBNavSDClientLib_CarIF.so.00` (15/10/2026, 2,6 MB; strings + símbolos C++ demangled).** O lado de *controle* do stream do cluster:
  - **Fonte do cluster é parametrizada, não fixa:** o render é escolhido por uma **URI** (`_kombiMaprendererUri`, "new kombi mode", "unknown kombi maprenderer mode URI", `KombiMapRendererParams_t`) e pelo flag `_ipStreamingEnabled`. O `FctVideoStreams.cpp` implementa o stream, exposto por BAP como `FctVideoStreamsAvailable` **bit 58**.
  - **Camada 2 ganha prova de firmware:** `AppConnectRouteGuidanceActiveServiceHandler.cpp` + BAP `FctSemidynamicRouteGuidanceAvailable` **bit 50** + classe `SemidynamicRouteGuidance` (ETA, delay, time-to-destination). É o mecanismo pelo qual a central **recebe a orientação do App-Connect (CarPlay/Android Auto) e a repassa ao cluster como dados** (setas/ETA) — distinto do vídeo (camada 3). A string `"Cannot start / stop RouteGuidance as guidance from Smartphone integration is active"` mostra a central já coordenando navegação nativa × do smartphone.
  - **Leitura:** camada 2 = caminho **nativo por BAP** (bit 50, dados); camada 3 (vídeo) = `FctVideoStreams` (bit 58) + a **URI do kombi renderer** — e é essa URI/surface que o PoC do daChillout troca. Análise estática; nada executado; o acesso de execução segue fora do escopo.

  **Dissecação encerrada por aqui (15/10/2026, decisão do Lipe).** Mecanismo mapeado e publicado no repo. Para retomar a decompilação fina (condição exata do gate, crop 1280×480, `FctVideoStreams`): re-extrair os binários do `sysroot/mu0883/system.img` com o `debugfs` do e2fsprogs (`/opt/homebrew/opt/e2fsprogs/sbin/debugfs`), caminhos `/lge/app_ro/graphics/multi-display/{multi-screen-server,rsi-test,rsi-display-management}`, `/lge/app_ro/carif/navsd_app`, `/lge/app_ro/lib64/libMIBNavSDClientLib_CarIF.so.00`, e seguir por **`objdump`** (Apple LLVM desmonta o ELF ARM64 — testado). O **REA/Hopper não serve nesta máquina**: o Hopper está instalado e o `rea doctor` o dá como "ok", mas a ponte REA↔Hopper devolve `unreadable_output` em toda operação (e antes houve bloqueio de permissão de Automação do macOS). Ver [[re-firmware-mib3-ferramentas]].

  **PoC própria do pipeline (retomada em 15/10/2026, pedido do Lipe).** Depois do PoC do daChillout, o Lipe pediu uma PoC nossa do pipeline **na bancada**, alvo "pixel de ponta a ponta com o binário real da LG". Código em pasta separada `moi3-cluster-pipeline-re/` na raiz (candidata a repo no GitHub: só código nosso, firmware nunca junto, linha de proteção no README — reproduz a **exibição**, não o acesso/execução na unidade).
  - **Protocolo `ivi_share` extraído do binário** (`moi3-cluster-pipeline-re/tools/extract_wl_protocol.py` lê as `wl_interface` do `multi-screen-server`): `ivi_share.get_ivi_share_surface(new_id, uint surface_id)`; `ivi_share_surface` eventos `damage`(`hh` = **2 fds = dma-buf**), `configure`(`uuuuu`: w,h,stride,fmt,+1), `input_capabilities`, `share_surface_state`; requests `redirect_touch_*`. Documentado em `moi3-cluster-pipeline-re/docs/ivi-share-protocol.md`. `[DESMONTAGEM]`
  - **Spike (resultado): o `ivi_share` está resolvido; o muro é o GBM.** Compositor com `ivi_share` (`compositor/ivishare-stub.c`) + harness isolado (`spike/`, só o `mss` real, sem a HMI) rodando o `multi-screen-server`: ele **deixa de morrer no "ivi_share not supported"** — faz bind do `ivi_share`, cria o próprio `wl_surface`, segue vivo (valida o protocolo) —, mas trava **antes** de pedir o share surface, na init do **GBM da Qualcomm** (`msmgbm` → `open /dev/dri/card0` e `/dev/ion` falham; `gbm_create_device: Device creation failed`). Strace confirma: carrega a `libgbm.so` do firmware e tenta `/dev/dri/{card0,renderD128}` e `/dev/ion`, todos ausentes.
  - **Ambiente (OrbStack) sem atalho:** sem `/dev/dri`, `/dev/udmabuf`, `/dev/dma_heap`, sem módulos `vgem`/`vkms`; e a libgbm é `msm_gbm` (KGSL, do SoC), então nem um DRM genérico resolveria.
  - **Leitura / próximo:** o risco "protocolo ivi_share" (médio) está **eliminado**; o risco real é o **GBM/ION da Qualcomm**. Caminho = **shim de GBM ("fakegbm")** que aloca em memória + um EGL nosso que importa esses buffers (análogo ao `fakegl` que já funciona), depois o encoder `OMX.qcom` (shim) e o transporte. É o grosso do trabalho; merece um spec antes de implementar. Nada executado em carro; container descartável.
  - **Spec aprovado + M1 concluído (15/10/2026).** Spec do projeto completo em `moi3-cluster-pipeline-re/docs/design.md` (6 marcos risk-first, aprovado pelo Lipe). **M1 (camada de buffer) ✅:** o `fakegbm` (`shims/fakegbm.c` — libgbm.so nossa, buffers em `memfd`, linkada contra a glibc do firmware) substitui a libgbm da Qualcomm; os `ioctl` DRM (`GET_MAGIC`/`AUTH_MAGIC`) são **não-fatais**; e, passando o `surface_id` na **linha de comando** do `mss` (`-m 1000 30`, conforme o `Usage` do binário) em vez de subir o RSI, o `multi-screen-server` real **chega a `get_ivi_share_surface(1000)` e aceita o nosso `configure` (1280×480, XR24, stride 5120)**. Sai com `exit_group(1)` logo depois — falta o evento `damage`/buffer (M2). Os logs internos do `mss` saem por um soquete de trace que o harness ainda não coleta; capturar no M2. Harness em `spike/` (só o `mss`, container descartável).
  - **M2 (EGLImage import) — veredito: VIÁVEL, sem muro do Mesa (15/10/2026).** O risco que decidia o M2 era o Mesa llvmpipe (software) fazer o caminho de textura external. Provado que faz, com três testes em `moi3-cluster-pipeline-re/spike/`: `mesa_caps.c` → o Mesa 25.2 (llvmpipe) tem **`EGL_EXT_image_dma_buf_import`** e **`GL_OES_EGL_image_external_essl3`**; `mesa_import_test.c` → criar `EGLImage` de uma textura 2D preenchida (`EGL_GL_TEXTURE_2D`), amarrá-la como `GL_TEXTURE_EXTERNAL_OES` e amostrá-la por um shader `essl3` `samplerExternalOES` (o tipo exato do `Rgba2yuv`) devolve **8/8 barras corretas**; `mesa_exttex_test.c` → `glTexImage2D` direto em external dá `GL_INVALID_ENUM`, logo é preciso a `EGLImage` real (via `GL_TEXTURE_2D`), não há atalho. O `mss` pega `eglCreateImageKHR`/`glEGLImageTargetTexture2DOES` por **`eglGetProcAddress`** (objdump: não são símbolos linkados).
  - **Falta pro `mss` real importar (plumbing determinística, sem risco novo):** estender o `eglGetProcAddress` do shim ([shim.c:1031]) + as funções `eglCreateImageKHR`/`glEGLImageTargetTexture2DOES`/`eglGetPlatformDisplayEXT`; 2 OPX no `host.c` (criar a `EGLImage` dos pixels via `GL_TEXTURE_2D`; amarrar como external) seguindo o padrão `EGL_GL_TEXTURE_2D` já provado; e o compositor enviar o evento `damage` com o buffer (`sendmsg`+`SCM_RIGHTS`). Tudo mapeado. **Próximo risco real do projeto: o encoder `OMX.qcom.video.encoder.avc` (M4)** — o `mss` abre `/dev/video33` (Venus V4L2) e usa `libOmxCore`; será outro shim.
  - **Plumbing do M2 (15/10/2026) revelou o acoplamento M2↔M4.** Implementei o envio do `damage` com o buffer (compositor: `sendmsg`+`SCM_RIGHTS`, test pattern em `memfd`) e um coletor de trace (`spike/tracecatch.c`). Resultado: o `mss` recebe `get_ivi_share_surface`+`configure`+`damage`, mas **morre na inicialização do encoder** — carrega `libOmxCore.so`+`libOmxVenc.so`, o `OMX_GetHandle("OMX.qcom.video.encoder.avc")` abre **`/dev/video33`** (Venus V4L2), falha e faz `exit_group(1)` **antes** do loop de frames (nunca chama `gbm_bo_import` nem `eglCreateImageKHR`). Ou seja: o `mss` monta o encoder na partida, então **o import (M2) só acontece depois do encoder (M4) subir** — a ordem real é encoder primeiro. Pra ver o import no `mss` real é preciso antes um shim do encoder (`fakeomxcore`/`fakeomxvenc`, OMX IL, sem `/dev/video33`), o maior shim do projeto.
  - **M4 (encoder) + import integrados (15/10/2026): o `mss` real monta o transporte e recebe os frames; trava no handshake de buffers do encoder.** Construído: `fakeomxcore` (`moi3-cluster-pipeline-re/shims/{fakeomxcore.c,omx_il.h}` — libOmxCore OMX IL que substitui o encoder Qualcomm; o `OMX_GetHandle` dá o nosso componente, então `/dev/video33`/`libOmxVenc` nunca são tocados); o import `eglCreateImageKHR`→EGLImage(`GL_TEXTURE_2D`)→external no shim/host (`glforward-ext/apply.py`); e o compositor mandando `damage` a 30fps (`SCM_RIGHTS`). O `mss` real agora: passa da init do encoder, roda toda a config OMX (dezenas de Set/GetParameter), **cria o socket UDP IPv6 multicast** (o transporte), e **recebe o fluxo de `damage`** (416 frames, os fds chegam). **Bloqueio:** não fecha o loop de frames — o encoder usa **"use-buffer mode"** (buffers GBM/ION — string `"GBM buffers count in use-buffer mode"`) e um handshake OMX **assíncrono**; o stub síncrono trava o `Loaded→Idle` (o `mss` espera o `CmdComplete(Idle)` sem alocar/importar). Diagnóstico: o OMX **threaded** (feito, 15/10) corrigiu o state machine — o `mss` agora completa `Loaded→Idle→Executing` e entra no loop de frames —, mas o bloqueio persiste e está isolado: o `mss` **nunca aloca os input buffers do encoder nem importa** (`"input buffer or encoder pipeline not ready"` pra sempre), em qualquer timing de Idle (testados síncrono, async-adiado, async-imediato — `shims/fakeomxcore.c`). Está na **orquestração interna do `ShareServer::startShareInternal`** (quando criar os GBM input buffers e disparar o import — possivelmente um gatilho "START STREAMING" do RSI/displaymanagement que o modo standalone não gera), decifrável só **desmontando o binário** (RE de assembly), não por tentativa-e-erro. **Veredito do M4:** todos os riscos de descoberta caíram; o `mss` real roda contra os shims até Executing + monta o transporte multicast + recebe os frames; o último elo (puxar o frame→encodar→transmitir) é RE profunda do ShareServer, não mais descoberta. Nada em carro; container descartável.
  - **RE do ShareServer + o shader rodando sobre o frame real (15/10/2026) — e CORREÇÃO do veredito acima.** Fiz a RE (Ghidra 12.3) da máquina de estados inteira do pipeline de exibição, documentada em `moi3-cluster-pipeline-re/docs/shareserver-pipeline.md` (endereços/offsets; scripts `tools/ghidra_decomp_*.java`). O `ShareServer::startShareInternal` (`FUN_001175d0`) só **arma 4 callbacks** (`damage`/`configure`/`input_capabilities`/`share_surface_state`, struct `surface_listener_callbacks` @ `0x157b68`) e sai; o trabalho por frame mora nelas, dirigido pelo compositor. **O veredito do M4 (acima) estava parcialmente errado:** o `mss` *não* depende de um gatilho "START STREAMING" oculto nem de orquestração interna misteriosa — o `configure` sozinho monta o encoder (o estado nasce `=2`). O muro real da corrida anterior era **mundano: o harness estava desatualizado e nem compilava o `fakegbm`** (a libgbm real da Qualcomm carregava e morria no `/dev/dri`/`/dev/ion`). Com o harness atual, a RE em mãos, e três correções, o `mss` real percorreu o pipeline até o shader:
    - `fakegbm`: tratar o import type **`0x4fff`** (o `cb_damage` usa esse, não `0x5503`) com layout `{fd, fence, w, h, fourcc}` (`shims/fakegbm.c`);
    - `fakedrm` (**novo shim, LD_PRELOAD**, `shims/fakedrm.c`): o `mss` abre `/dev/dri/renderD128` e faz o auth DRM (`GET_MAGIC`/`AUTH_MAGIC`) por conta própria; sem um fd válido aí, o `cb_damage` rejeita todo frame (gate `*(int*)(param_2+0x58)<0`). O shim devolve um fd e finge o auth — sem GPU/DRM real;
    - `configure`: mapeamento empírico dos args — o `mss` usa o **2º uint = width**, **3º = height**, **5º = seletor de fourcc** (`2`→XR24); o 1º e 4º são ignorados (`compositor/ivishare-stub.c`).
    - **Resultado (spike-6):** GBM ✓ → `get_ivi_share_surface`+`configure 1280×480` ✓ → encoder `OMX.qcom.video.encoder.avc` → **Executing** ✓ → `damage`: `gbm_bo_import` (1280×480 XR24) ✓ → **`eglCreateImageKHR`(EGL_LINUX_DMA_BUF_EXT) do dma-buf** ✓ → **o shader `Rgba2yuv` (RGBA→YUV) RODOU** sobre o frame importado (glhost registra o draw: `glUseProgram`/`glBindTexture`/FBO/`glDrawArrays`/`glFinish`). **Parada atual:** segfault na limpeza pós-`convert` (o `cb_damage` chama `eglDestroyImageKHR` `ctx[0xc0]`, linha 475, antes do readback/`EmptyThisBuffer`). O que falta é robustez do `glforward-ext` (destruir/ler de volta o EGLImage) e a ponte final `OMX`→`TransportStream`; a rede é `--network none`, então o RTP não precisa sair para o critério "pixel de ponta a ponta". Spikes em `scratchpad/spike-{3..6}`; nada em carro; container descartável.
  - **Pixel convertido pelo shader do binário, na bancada (15/10/2026).** Como o binário não orquestra o shader sozinho em standalone (o `Rgba2yuv::init`, que compila o shader e cria o FBO de saída, só é chamado quando o encoder OMX aloca os input buffers — handshake que o modo isolado não dispara), rodamos o **shader exato do binário** sobre o frame real no Mesa (llvmpipe) da bancada. Harness `moi3-cluster-pipeline-re/spike/moi3_shader_demo.c`; shaders extraídos verbatim do `multi-screen-server` (vertex, "swizzle", e o YUV com `rgb_to_yuv` BT.601); quad e uniforms do binário. **Passe A** (função `rgb_to_yuv` por pixel): o round-trip YUV→RGB devolve as 8 barras exatas (vermelho → YUV (81,90,240) → RGB (255,0,0)). **Passe B** (shader YUV completo, com empacotamento NV12): compila e roda (basta remover a linha `precision highp uint;` que o Mesa ES recusa e o Adreno aceita), produz NV12 1280×480 (921600/921600 bytes não-zero) que decodificado reproduz as barras — é o buffer que entraria no encoder H.264. Detalhe de implementação: textura externa no Mesa via `GL_TEXTURE_2D`+`eglCreateImageKHR(EGL_GL_TEXTURE_2D)`+`glEGLImageTargetTexture2DOES`. Honestidade: GLSL/quad/uniforms são do binário (verbatim), nós executamos; o binário não orquestra em standalone (bloqueio no handshake de input-buffer do OMX, caminho "A" purista, em aberto). RE completa e detalhes em [../../../moi3-cluster-pipeline-re/docs/shareserver-pipeline.md](../../../moi3-cluster-pipeline-re/docs/shareserver-pipeline.md). Nada em carro; container descartável.
  - **PoC da camada 3 na bancada — lado display (15/10/2026).** Pedido do Lipe: provar a camada 3 (app de mapa full no painel) até onde é nosso, e só então o device real. Como o pipeline é agnóstico ao surface, troquei o padrão de teste por um **frame de navegação sintético 1280×480** (sem imagem de terceiros) e passei pelo shader REAL do binário (harness `moi3_shader_demo.c`): o mapa vira o buffer NV12 do cluster (passe B, 921600/921600 não-zero) e, decodificado, aparece no "cluster". Ou seja: o caminho de exibição carrega qualquer surface — inclusive um app de mapa — até o stream do cluster, usando a conversão do próprio binário. **Isso fecha o lado display+conversão da camada 3 na bancada.** O que falta é só do device real e NÃO passa por mim desenvolver: (1) o **serviço de redirect** apontar o `multi-screen-server` ao surface do app via RSI, (2) o **apoio de execução** na unidade (como o daChillout — método de acesso que não analisamos; fora de escopo desenvolver exploit/bypass das proteções), (3) o app (Android Auto/projeção) desenhando o mapa, e (4) a adaptação no painel aceitar o stream (coding BAP/DSI). Linha mantida: desenvolvo interop/display/coding e provo na bancada; não desenvolvo o acesso/execução na central. Imagens entregues ao Lipe: entrada (mapa) e saída (cluster).
  - **"MIB3 Simulator + AID Virtual" ao vivo (15/10/2026, pedido do Lipe).** Montado o rig que o Lipe desenhou: **MIB3 Simulator** = a bancada (container + binário `multi-screen-server` + shims fakegbm/fakeomxcore/fakedrm/glforward + compositor stub); **AID Virtual** = uma janela HTML 1280×480 (cluster) que exibe o stream. Como a orquestração própria do mss não roda em standalone (parede do OMX/RSI), a batuta do loop é nossa: geramos uma sequência de mapa de navegação animado (36 frames, sintético), passamos **cada frame pelo shader REAL do binário** (`moi3_shader_demo.c`, passe B → NV12), `ffmpeg` monta o NV12→H.264, e o AID Virtual toca em loop. Resultado: o cluster virtual mostra o mapa em movimento convertido pelo binário. Arquivos no scratchpad `shaderdemo/` (in_seq, nv12_seq, aid_live.mp4, aid_virtual.html). **Pendências pro "mais real":** (passo 2) feed ao vivo do **iPhone pelo cabo** (tela do Apple Maps via `devicectl`; CarPlay com chrome só com dongle Carlinkit — protocolo Apple/MFi, não se implementa); e, pro device real, o `multi-screen-server` orquestrando sozinho (apoio de execução na unidade, via RSI — fora do que eu desenvolvo como exploit, mas interop eu faço). Nada em carro; tudo bancada/Mac.
  - **CarPlay funcional no Mac: SIM, com dongle Carlinkit + software aberto (busca no GitHub, 16/10/2026, pedido do Lipe). Corrige o veredito anterior de "impossível".** A barreira MFi é real para um receptor **puro de software** — o [`rednblkx/AirLib`](https://github.com/rednblkx/AirLib) diz explícito "(mfi hardware required)". **Mas** o dongle **Carlinkit** (hardware ~R$150–300) **é** o MFi e faz o handshake com o iPhone, depois **joga o CarPlay em H.264 + áudio PCM pelo USB** para o host; libs abertas decodificam:
    - [`rhysmorgan134/node-CarPlay`](https://github.com/rhysmorgan134/node-CarPlay) (265★, Node/WebUSB) — roda no **macOS/Chrome**, renderiza o CarPlay no navegador com toque/áudio/mic; **saída H.264** (perfeito: é o formato do nosso stream do cluster → entra direto no AID Virtual/pipeline). Exige o dongle tipo **CPC200-Autokit ou CPC200-CCPA** (o que faz um host virar receptor); os adaptadores **"wired→wireless" NÃO funcionam** (README).
    - [`harrylepotter/carplay-receiver`](https://github.com/harrylepotter/carplay-receiver) (414★, Python/Linux, fork do `electric-monk/pycarplay`; vídeo por mpv/ffmpeg) e [`mzyy94/gocarplay`](https://github.com/mzyy94/gocarplay) (Go) — mesmo ecossistema de dongle.
    - [`ludwig-v/wireless-carplay-dongle-reverse-engineering`](https://github.com/ludwig-v/wireless-carplay-dongle-reverse-engineering) (938★) — a RE canônica do dongle (SoC i.MX6; segmentação U2W/U2AW/U2AC).
  - **Caminho dongle-less que NÃO é a nossa linha:** [`DiPlay`](https://github.com/shihabal3amri/DiPlay) (11k★), `ztecjo/nexuscp` ("embedded offline MFi"), `NineThree/HeadUnitWireless-CarPlay` ("bundled experimental authentication material") viram receptor **sem dongle** empacotando **material de autenticação MFi extraído** — ou seja, contornando a proteção da Apple. Registrado como existente; **não perseguir** (fura proteção, fora do escopo). O DiPlay (lido por inteiro em 16/10) é **APK Android para central BYD** (base: `xcertplay`, GPL-3.0; "install on the car, not the iPhone"), e o próprio README crava: *"not an Apple-certified product. The APK bundles an experimental accessory identity recovered from public Carlinkit firmware… A bundled private key is extractable."* — ou seja, a mágica "sem dongle" é uma **chave privada MFi extraída do firmware da Carlinkit**, embutida no app. **Irrelevante pra nós em dois planos independentes:** (1) **plataforma** — é APK Android para central BYD; não roda no Mac nem na nossa MIB3 (AGL Linux, não Android, não instala APK); (2) **proteção** — usa identidade MFi extraída (contorna a auth da Apple). Ironia útil: a versão limpa do mesmíssimo truque é só **usar o dongle Carlinkit** (que é o MFi de verdade) via node-CarPlay; o DiPlay só arranca a identidade de dentro do firmware do dongle pra não comprá-lo. Tem até um "CarPlay map on dashboard (experimental)", mas **específico de cluster BYD**, não VW.
  - **Leitura:** pro rig "MIB3 Simulator + AID Virtual", um dongle Carlinkit (~R$200) destrava **CarPlay E Android Auto reais no Mac** (o 5.0/U2AC faz os dois), em H.264, alimentáveis no AID Virtual — resolve o "CarPlay funcional" do passo 2 pela via limpa. **Verificado:** o método (dongle+lib), os repos e o requisito de modelo (CPC200-Autokit/CCPA). **Não verificado:** preço/disponibilidade no Brasil hoje e qual unidade exata em estoque bate com a lib (confirmar no README da node-CarPlay antes de comprar). Nada em carro.
  - **SEM dongle, pela via LIMPA (pesquisa 16/10/2026, pedido do Lipe "ver se tem como fazer sem"): o CarPlay Simulator da Apple — e o Lipe JÁ o tinha funcionando.** O **CarPlay Simulator** é um **app oficial de Mac** (pacote "Additional Tools for Xcode"; **a partir do Xcode 27 vem no Device Hub**) que, pela [sessão WWDC25 216](https://developer.apple.com/videos/play/wwdc2025/216/) e pelo [CarPlay Developer Guide (out/2026)](https://developer.apple.com/download/files/CarPlay-Developer-Guide.pdf), "recria a experiência completa de CarPlay por USB" com um **iPhone real conectado**. É exatamente o que o Lipe mostrou antes (a grade do CarPlay do iPhone dele via DeviceHub). **Legítimo, sem dongle, sem furar MFi** — a autenticação é do próprio iPhone + app da Apple. (O [9to5mac](https://9to5mac.com/?p=357022) confirma que o MFi "exige chip de autenticação", por isso o receptor em software puro não existe; o simulador da Apple é a exceção oficial de desenvolvimento.)
    - **O muro real nunca foi o CarPlay; foi CAPTURAR a janela** do CarPlay Simulator pra alimentar o rig. Na tentativa anterior, `simctl io screenshot` deu "Timeout waiting for screen surfaces" — mas isso é do **iOS Simulator**, não do app **CarPlay Simulator**; e faltou `pyobjc` pra listar janelas. É problema de **captura de tela do macOS**, resolvível com `screencapture -l<windowid>` / ScreenCaptureKit / `ffmpeg -f avfoundation` (pede permissão de Gravação de Tela uma vez).
    - **Plano sem custo:** abrir o CarPlay Simulator (Device Hub) com o iPhone + Apple Maps → capturar a janela → escalar/letterbox 1280×480 → shader real do binário → AID Virtual = "CarPlay do iPhone real no cluster", sem comprar nada. Comparado ao dongle: o dongle dá H.264 pronto (mais limpo de encaixar) e funciona sem Xcode; o simulador é grátis e já está no Mac, mas depende de captura de tela e do Xcode 27. Fontes: [createwithswift — Testing Apps with an iPhone and the CarPlay Simulator](https://www.createwithswift.com/testing-apps-with-an-iphone-and-the-carplay-simulator/), [medium — CarPlay Simulator on real device](https://medium.com/@paereson/how-to-open-carplay-simulator-on-real-device-in-xcode-14-6a8399564b8b).

## 3.1 `ability_switch_nav_maps` — troca do mapa NATIVO entre telas, não redirect de fonte (17/10/2026)

Investigado a pedido do Lipe como possível alavanca in-scope (via ODIS, sem foothold) pra pôr o mapa de um
app no cluster. **Não é.** É o liga/desliga da capacidade de **trocar o mapa de navegação NATIVO entre o
cluster (Active Info Display) e a tela da central** — com ON, o usuário ganha o botão de alternar, e vale o
"um mapa por vez". No Jetta está **`yes`** (autoscan 14/10, grupo `[VO]_Dashboard_Display_Configuration`).

- **Web:** fórum Briskoda (Audi MY18) — o mapa só aparece nas duas telas com esse canal ligado, e aí aparece o
  botão de swap; guia VAG-coding do Golf 8 (5H), grupo `Dashboard_Display_Configuration`, Adaptação 10.
- **Firmware (system.img MU0883, corrobora):** `Service::updateSwitchMapEnabled : add/remove
  ActiveSingleMapRenderer`, `Service::updateSwitchMapEnabled : state is changed`,
  `m_proxyDiag->setSwitchMapEnabled(%d)` — o DID da adaptação liga o **renderer de mapa único** que migra
  entre os displays.
- **Conclusão:** `ability_switch_nav_maps` controla **onde** o mapa NATIVO aparece (cluster vs central), não
  a FONTE do stream. **Não há alavanca de codificação pra pôr o mapa de um APP no cluster por esse canal** —
  o redirect de fonte segue precisando do RSI (foothold, fora do escopo). Outras adaptações de mapa lidas no
  carro (`navigation_map_transmission_mode=IP_streaming`, `..._compression_mode=H264`, `..._resolution=1`,
  `crossroad_detail_map_transmission_mode=off`, `nhtsa_...androidauto_no_video_playback=activated`) só dizem
  **como/se** transmite, não a fonte. Ponta ainda aberta: a **fonte do app** (ivi surface do CarPlay/AA).
- **Corroboração do comportamento (drive2, 21/01/2025):** `Ability_switch_nav_maps = NO` → o mapa aparece **ao
  mesmo tempo** no MIB e no AID; `= Yes` → ganha a opção de escolher **MIB ou AID** (um por vez). E `VZO = ON`
  mostra os **sinais de limite de velocidade** no mapa (funciona no Discover Pro, não no Discover Media). Fonte:
  um guia de **ativação de FEC/SWaP** da cena — licenciamento/proteção de componentes, **fora do escopo**; aqui
  aproveito só o fato de codificação, não o método de ativação. `[WEB]`

## 3.2 Nav do smartphone (CarPlay/AA) no cluster — caminho NATIVO existe; mapa gráfico precisa de foothold (17/10/2026)

Investigada a fonte do app (ponta #2, a pedido do Lipe). Dois caminhos, conclusões distintas:

- **Mapa GRÁFICO do app no cluster — precisa de foothold (fora do escopo).** O app (AA/CarPlay) renderiza na
  superfície de projeção **`mirrorlink`/`PROJECTION_SURFACE`** (sistema usa Weston **ivi-shell**; config de
  surfaces nomeados `cluster:cluster_instrument, hmi:hmi, media:media, mirrorlink:mirrorlink`). Pôr isso no
  cluster = o mss capturar essa surface e streamar (como o daChillout cropa a tela do app). Depende do
  redirect RSI (foothold). E é a **tela inteira do app**, não um mapa limpo de cluster.
- **Nav do smartphone no cluster (turn-by-turn) — caminho NATIVO, in-scope.** O NavSd
  (`libMIBNavSDClientLib_CarIF.so`) tem `AppConnectRouteGuidanceActiveServiceHandler`,
  `isSmartphoneRouteGuidanceActive`, `FctSemidynamicRouteGuidance` (distância/tempo/manobra por **BAP**) e liga
  `_appConnectNavigationObjectUri` ↔ `_kombiMaprendererUri`. Ou seja, o firmware **já sabe mostrar as direções
  do CarPlay/AA no cluster** (o recurso comum "direções no painel digital"). Strings: `Both route guidance from
  MIB and Smartphone integration are active`, `Cannot start / stop RouteGuidance as guidance from Smartphone
  integration is active`, `appconnectrouteguidanceactive.appConnectNavigations`.
- **Autoscan (14/10):** nenhuma adaptação gateia isso explicitamente — só `app_connect_over_wifi=yes` no 5F;
  módulo 17 com `navigation_compass=yes` e `Locking_display_content / Map locked=no`. Então é provavelmente
  **automático/negociado em runtime** quando a nav do smartphone roda, não um bit de codificação.
- **Teste no carro (só leitura, zero risco, a fazer):** rodar navegação do **Android Auto ou CarPlay** e olhar
  o **cluster** — aparece o turn-by-turn (seta de manobra, distância, "vire à direita")? Se sim, o recurso
  nativo já está vivo no Jetta (é o "mapa do app no painel" possível in-scope — guidance, não o mapa gráfico).
  Se não, o firmware suporta, mas algo em runtime/negociação o impede (aí vale cavar mais).

### RE no nível de instrução — a cadeia nativa provada (17/10/2026)

Antes a §3.2 estava só em string; agora está **decompilada**. Binário `libMIBNavSDClientLib_CarIF.so.00`
(sha256 `57d9285f…`), extraído do firmware da **bancada (MU0883)**; Ghidra 12.1.4. Endereços = file vaddr.
Ressalva de versão: o carro é **MU0803** — mecanismo esperado igual, build exato do carro não conferido byte a
byte. Três funções fecham o caminho "nav do smartphone → cluster", e ele é **BAP, independente do vídeo/RSI**:

1. **Assinatura — `AppConnectRouteGuidanceActiveServiceHandler::handleAppConnectNavigationsChanged`** (corpo
   `@0x1ad2cc`; thunk `@0x192bc0`). Recebe o array JSON `appConnectNavigations`. Lê `[0].uri` (chave `uri` =
   `MicroServiceInfo::KEY_URI`). Quando a URI muda: `unsubscribeAppConnectNavigationObject`, grava a URI nova em
   `this+0x2a0` (`_appConnectNavigationObjectUri`) e chama `TriggerMicroServiceRegistrationChanged()` — reassina
   no microserviço novo. Array vazio / não-objeto → unsubscribe. É a camada que descobre **onde** vive o objeto
   de nav do telefone e se pluga nele. `[DESMONTAGEM]`
2. **Payload → cluster — `FctInfoStates::handleAppConnectNavigationsObjectChanged`** (corpo `@0x269c8c`; thunk
   `@0x192ce0`). Do objeto assinado lê o **bool `isSmartphoneRouteGuidanceActive`** (string em `0x31a218`,
   confirmada byte a byte: `69 73 53 6d…` = `isSmartphoneRouteGuidanceActive\0`). Guarda o estado em `this+0x218`;
   só age na borda (mudou de valor). Quando vira **true**: cria um `Bap::NavigationSd::InfoStates::Status`,
   `setStates('\x06')` (**estado 6**) e emite pelo virtual `(this+0x100)->vt[0x50]` com `(status, 0, 0x7f, 0)`
   (`0x7f` = function id). Quando **false**: reseta pro status default (`this+0x158/0x160`) e emite. **É esta a
   função que acende no cluster, por BAP, o estado "navegação do smartphone ativa".** `[DESMONTAGEM]`
3. **Qual tela recebe — `FctActiveRgType::rsiMaprenderer2Bap`** (`@0x25d274`). Mapeia nome de surface RSI → enum
   BAP do renderer-alvo: `"kombi"` → **3**, `"main-unit"` → **0**, senão `0xFF` (a string `kombi` confirmada em
   `0x30da28`: `6b 6f 6d 62 69 00`). É a ponte RSI↔BAP de **qual display** recebe o mapa de active-RG, dentro do
   próprio NavSd. A classe `FctActiveRgType` traz ainda `switchActiveSingleMapRenderer`,
   `createVisibleKombiMaprenderer`/`delete…`/`toggleVisibleKombiMapRenderer`, `getVisibleKombiRendererUri`, e os
   microserviços `us_navcombi_synchronize_arrival_mode::updateKombiMode` e
   `us_navcombi_parametrize_renderer_instance::parametrizeKombiMapRenderer` — a gerência do renderer do cluster.
   `[DESMONTAGEM]`

- **O que isto muda:** o turn-by-turn do telefone no cluster **não depende do redirect RSI nem de foothold** — é
  feature nativa por BAP (`InfoStates` + `SemidynamicRouteGuidance` + `ActiveRgType`). O "gate" não é um bit de
  codificação óbvio: o bool `isSmartphoneRouteGuidanceActive` é **publicado em runtime** pelo serviço de projeção
  quando o app do telefone expõe route-guidance (Google Maps/Waze no Android Auto, Apple Maps no CarPlay). Por
  isso o **teste no carro** (rodar a nav do celular e olhar o cluster) segue sendo a prova decisiva — o firmware
  faz; falta ver se a projeção + o app publicam o campo neste Jetta. `[DEDUÇÃO]` sobre o gate de runtime.

### Resultado do teste no carro — ETA/distância SIM, setas NÃO (17/10/2026)

O Lipe rodou navegação pelo celular e olhou o cluster: **aparecem a hora de chegada e a distância ao destino;
as setas de manobra, não.** App/projeção confirmado em 17/10: **Google Maps no CarPlay** (iPhone).
Isto **confirma que o caminho nativo está vivo** e permite separar, no firmware, o que chega do que não chega.

- **O que aparece = `FctSemidynamicRouteGuidance`** (BAP **Bit50**): `NewTimeToDestination` (hora de chegada),
  `NewDistanceToDestination` (distância), `Delay`, `TrafficImpact`. É o resumo "lento" da viagem. Bate 1:1 com o
  relato. `[DESMONTAGEM]`
- **O que NÃO aparece = a família da manobra**, funções BAP distintas, cada uma com seu bit de disponibilidade:
  `FctManeuverDescriptor` (**Bit23**; a seta/pictograma — `setManeuver1/2/3`, `getDirection`, `getSidestreets`,
  `ZLevelGuidance`), `FctDistanceToNextManeuver` (**Bit18**; "em 300 m" + bargraph), `FctLaneGuidance` (faixa),
  `FctManeuverState` (**Bit55**). Suporte estendido em `FsgSetup` Df47 Bit4 (direção+ruas laterais) e Bit5
  (lista de manobras). `[DESMONTAGEM]`
- **Pista forte da causa:** a família da manobra é alimentada pelo *driving context* da **nav NATIVA** —
  handlers `FctManeuverDescriptorBase::handleNativeDrivingContext`, `handleSimpleNavDrContextChanged`,
  `SimpleNavigationServiceHandler::getNextManeuverInfo`. A ETA/distância vem pelo canal semidinâmico que a
  projeção preenche. Ou seja: pela projeção do celular chega o resumo (Bit50), mas a **manobra** (Bit23) parece
  casada ao contexto da nav nativa. `[DESMONTAGEM]` nos nomes; `[DEDUÇÃO]` no "só casa com nativa".
- **Hardware não é o limite:** o cluster é Digital Cockpit e já desenha seta com a nav nativa (ficha, linha do
  Painel de instrumentos). Então a seta sumida é dado que não chega, não tela que não sabe desenhar.
- **Duas causas candidatas, uma só dá trabalho:** (1) o app/projeção não publica a manobra pro cluster; ou (2)
  o carro não liga a manobra do smartphone ao `ManeuverDescriptor`. O teste de 17/10 usou **Google Maps no
  CarPlay** — a combinação que, pela web, menos manda a seta (o Google Maps no CarPlay costuma entregar só o
  resumo; a seta no cluster vem com mais confiança do **Apple Maps** no CarPlay, do **Waze ≥ 5.4.0**, ou do
  **Android Auto + Google Maps**). Nenhuma fonte é específica do Jetta/MIB3 — é relato cruzado de outras marcas
  + a base do iOS 13 (dual-display, 2019). `[WEB]`
- **Teste decisivo (a fazer, só leitura; iPhone, sem precisar de Android):** (a) **Apple Maps no CarPlay** —
  candidato nº 1 à seta; (b) **Waze no CarPlay** (conferir 5.4.0+); (c) ciclar o botão **VIEW** do volante com a
  rota rodando, caso a seta more noutro layout do cluster. Se *algum* mostrar a seta → é app (resolvido, sem
  mexer no carro). Se *nenhum* → o CarPlay deste carro não alimenta a manobra; restaria **Android Auto + Google
  Maps** (telefone Android) ou seria gate do carro, e aí vale RE/coding a fundo. Calibragem: a seta do CarPlay no
  cluster costuma ser **seta + distância**, não o pictograma gráfico cheio.
- **Resultado Apple Maps no CarPlay (17/10):** **a seta também NÃO apareceu.** Como o Apple Maps é o app que
  mais confiavelmente empurra a manobra via CarPlay e nem ele mostrou, a causa **não é o app** — é o caminho do
  CarPlay neste carro que não alimenta a seta, só o resumo (ETA/distância). (Se o ETA/distância continuaram
  aparecendo com o Apple Maps, não foi perguntado; presume-se que sim.)

### Por que a seta não vem do celular — arquitetura do firmware (17/10/2026)

Busca `DrivingContext` no NavSd: **toda** a orientação rica do cluster é alimentada por um objeto
**DrivingContext**, consumido via `handleNativeDrivingContext`/`handleDrivingContextChanged` por
`FctManeuverDescriptor` (seta), `FctLaneGuidance` (faixa), `FctTurnToInfo` ("vire em"),
`FctDistanceToNextManeuver` ("em 300 m"), `FctCompassInfo` (bússola) e `DistTimeToDestinationBase`. O produtor do
DrivingContext é `GuidanceServiceHandler`/`SimpleNavigationServiceHandler` (`handleDrivingContextChanged`,
`getDrivingContextCarUri`, `TriggerDrivingContextChanged`) — a **nav nativa**. `[DESMONTAGEM]`

- **O AppConnect (smartphone) NÃO produz DrivingContext.** Na varredura completa de `AppConnect`, os únicos
  consumidores do objeto de nav do celular são `AppConnectRouteGuidanceActiveServiceHandler` (assina a URI) e
  `FctInfoStates` (lê `isSmartphoneRouteGuidanceActive`). `GuidanceServiceHandler` **não** aparece ligado a
  AppConnect. Logo, o dado de manobra do celular — mesmo que exista no objeto — **não é consumido** por nenhuma
  função de seta. `[DESMONTAGEM]`
- **Conclusão (firmware MU0883; carro MU0803, mesma geração):** a seta/faixa/"vire em" no cluster são
  **exclusivas da nav nativa**. Pelo celular (qualquer projeção) chegam só o flag "navegando" e o semidinâmico
  (ETA/distância). **Não há codificação que ligue a manobra do smartphone ao cluster** neste firmware — é
  arquitetura, não um bit. Bate 1:1 com o teste (Google Maps e Apple Maps no CarPlay: ETA/distância sim, seta
  não). `[DESMONTAGEM]` + `[DEDUÇÃO]` na extrapolação MU0883→MU0803.
- **Android Auto:** mesma via AppConnect neste binário, então **provavelmente** também só ETA/distância, não a
  seta — salvo uma ponte AA→DrivingContext num binário de projeção que não foi examinado. Relatos de comunidade
  de "Google Maps no cockpit via AA" existem, mas em plataformas/firmwares possivelmente mais novos; neste MU08xx
  a ponte não está no NavSd. Não testável sem um telefone Android. `[DEDUÇÃO]`/`[WEB]`
- **O que DÁ seta no cluster, de fato:** a **navegação nativa embarcada** (mapa do SD/onboard) — que já desenha
  mapa, seta e faixa no Active Info Display deste Jetta (ficha, linha do Painel de instrumentos). A seta do
  CarPlay no painel é um recurso que este carro não entrega. `[DESMONTAGEM]` + ficha.

### Prova definitiva — a spec VIWI do recurso (17/10/2026)

Extraída do firmware (`/lge/app_ro/sai/spec/viwi.service.appconnectrouteguidanceactive/…-0.1.0.json`, MU0883). O
recurso `appconnectrouteguidanceactive.appConnectNavigations` tem **exatamente 4 campos**: `id`, `name`, `uri` e
**`isSmartphoneRouteGuidanceActive`** (boolean). **Não existe campo de manobra/seta/faixa/ETA** — só o flag. É a
interface oficial pela qual a nav do smartphone chega ao cluster, e ela **não carrega manobra, por contrato**.
`[DESMONTAGEM]` (spec do firmware)

Trechos da própria descrição VW na spec:
- *"There are no separate RSIs for the different App-Connect technologies like **CarPlay, Android Auto,
  MirrorLink or Baidu CarLife** but this one resource is used technology independently."* → **Android Auto usa o
  mesmíssimo recurso que o CarPlay.** A dúvida "e o AA?" está **resolvida: igual** — não precisa de telefone
  Android pra testar. Isto **eleva a fato** a dedução anterior (que dizia "provavelmente").
- *"The information regarding active smartphone route guidance is used to display a **'Smartphone navigation
  active' text instead of the map** in FPK."* → o comportamento **projetado** pela VW é texto no lugar do mapa no
  painel (FPK = Freiprogrammierbares Kombi = cluster digital). Nunca foi pra mostrar mapa/seta do celular.
- *"This resource is read-only for the client (only client is **NAVKombiClient in CarSSW**)."* → o único
  consumidor é o cliente de nav do cluster.

- **Sobre o ETA/distância que o Lipe vê:** NÃO vem deste recurso (é boolean-only). Chega por um canal separado da
  própria sessão de projeção (metadados de nav do CarPlay/AA, recurso iOS 13+), que o cluster exibe via
  `FctSemidynamicRouteGuidance` (BAP). Esse canal não foi traçado binário a binário, mas é irrelevante pra seta:
  **a seta não existe em nenhum dos dois caminhos do telefone.** `[DEDUÇÃO]`/`[WEB]` no canal de ETA.
- **Conclusão final:** pôr a **seta de manobra do celular no cluster é impossível neste carro** — por desenho da
  VW, não por falta de codificação. Vale para CarPlay **e** Android Auto (mesmo recurso). Seta no painel = só com
  a **nav nativa embarcada**. Tema encerrado do lado do que é viável no carro. `[DESMONTAGEM]`
- **Ressalva (17/10/2026):** o "impossível" acima vale para o caminho **NATIVO** (codificação, sem código na
  unidade). O mapa do celular **no cluster** É alcançável pelo caminho de **VÍDEO** (camada 3 / redirect da
  surface `navi-kombi`) — hoje **claro e validado em carro real** (MOI3 LG). Detalhe e a corroboração externa em
  [camada3-rsi-redirect.md](camada3-rsi-redirect.md). O gate desse caminho é o **foothold na unidade**, que o
  projeto mantém **fora do escopo** (as partes de bypass/FAZIT/marca não são assistidas).

## 4. De que o produtor depende

- **Motor de navegação:** `naviengine` (renderiza o mapa a partir de mapas/rota). `[DESMONTAGEM]` (nome do binário) / `[DEDUÇÃO]` (papel exato).
- **Codificador de vídeo:** **`multi-screen-server`** (H.264 / OMX.qcom). **Ressalva:** o encoder de vídeo + AVB do firmware servem primariamente o **sistema de câmeras** (`qcarcam`/`ais_server`, surround/rear) — que o mesmo caminho sirva o mapa do cluster é `[DEDUÇÃO]` parcial. `[DESMONTAGEM]` (binários existem) + `[DEDUÇÃO]`.
- **Middleware VIWI/RSI:** barramento REST/JSON interno. O firmware **traz o próprio registro** (`MIB_ServiceRegistryApp-00`, porta 443) e um **mock da LG** (`rsi-mockup`, porta 7770), com **81 specs** em `/lge/app_ro/sai/spec/`. A HMI acha serviços em `ws://127.0.0.1:443/api/v1/`. `[EXEC]` — [modelagem-soc.md](modelagem-soc.md), Marco 4; open source análogo: `wzr1337/rsi.server` + `wzr1337/rsi.serviceregistry`.
- **Serviço de display:** `displaymanagement` (DSI), `rsi-display-management`, `multi-screen-server` (enumeram display e registram `DSIDisplayManagement`). `[DESMONTAGEM]` — NOTAS, §DisplayManagement.
- **Transporte:** Ethernet + **gPTP/AVB** (`gptp`). `[DEDUÇÃO]` (o multicast é certo; "AVB" é inferência do `gptp` presente).

## 5. O que é MOI3 e o que é de outra plataforma (para NÃO misturar)

| Fato | Plataforma | Vale para o sandbox do Jetta? |
|---|---|---|
| `ff14::2:4:42800`, `createOrUpdateStreamParameters`, VIWI POST | **MOI3 (Jetta, MIB3 LG, Linux/AArch64 AGL)** | **Sim** — é o nosso caso, verificado. |
| Gate `Df44 Bit0` / adaptação `DASHBOARD_DISPLAY_CONFIGURATION` (0xB5A, byte7/bit1) | **MOI3** | **Sim** — verificado estático+dinâmico. |
| `setActiveDisplayable`, displayables 26/29/40/42/60/93 | **MPR3 (Audi Premium)** — Squallman | **Referência** (arquitetura análoga), não literal MOI3. |
| MOST150, `/dev/mlb/isoTX2`, MPEG-TS, type-111/AltScreen, `LD_PRELOAD` no `dio_manager`, contexts 70/77/80, layer 58 | **MHI2 / MHI2Q (QNX)** | **Não** — transporte e método diferentes; não existe no MOI3 (varredura por `type 111`/`AltScreen` deu zero). |
| `vc_render`, `KombiMapRenderer` (dados de rota→cluster nativo) | conceito; `KombiMapRenderer` citado no MOI3 | Parcial — o cluster nativo desenha de **dados**; o vídeo é o `ff14::2:4`. |

Regra: **transporte do MOI3 = IP multicast**; **MHI2 = MOST**. Número de displayable e `setActiveDisplayable` **são do Audi**, não do LG. Não assumir resolução do cluster a partir da tela central (`OISmall` 800×480).

## 6. Onde já está registrado (seções e links)

**Na pasta:**
- [mib3-firmware-pacote-moi3-lg.md](../../mib3-firmware-pacote-moi3-lg.md) — §"Resumo executivo"; §"CORREÇÃO 2 — EXISTE stream de vídeo IP pro cluster, e é coding-gated"; §"RE do caminho de configuração de display"; §"Dissecação verificada por desmontagem" (o decompile de `createOrUpdateStreamParameters`, ip/porta); §"Gate decompilado"; §"RESULTADOS EMPÍRICOS" (prova dinâmica). **← a prova principal.**
- [NOTAS-RE-DINAMICA.md](NOTAS-RE-DINAMICA.md) — §"EMULADOR DisplayManagement + TESTE DE RENDER" (dmstub, `getDisplayExtents`, `DSIDisplayManagement`, os daemons `multi-screen-server`/`rsi-display-management`).
- [coding-geral.md](../../coding-geral.md) — §"Squallman/MPR3" (os displayables e `setActiveDisplayable` do Audi, com a ressalva de plataforma); §"CarPlay no cluster como recurso OFICIAL da Apple".
- [harman-f-mhi2-altscreen-repo.md](../../harman-f-mhi2-altscreen-repo.md) — por que o caminho MHI2 (MOST/type-111) **não** transfere.
- [mapa-do-celular-no-painel-RESUMO.md](../../mapa-do-celular-no-painel-RESUMO.md) — o modelo de 3 camadas (nativa / setas / mapa cheio).

**De fora:** `wzr1337/rsi.server` + `wzr1337/rsi.serviceregistry` (VIWI/RSI open source); `github.com/Squallman/mpr3-carplay-vc` (RE do MPR3 Audi); `github.com/harman-f/mhi2_altscreen_carplay` (MHI2, para contraste).

---

**Resumo de uma linha para o item 8:** o MOI3 **produz** o mapa do painel como **vídeo H.264 em multicast IPv6 `ff14::2:4:42800`**, montado por `us_displaymanagement::createOrUpdateStreamParameters` (POST VIWI), ligado pelo coding `Df44 Bit0` (já ativo no Jetta); o **receptor** (quem decodifica e desenha) é **outra ECU fora das nossas imagens** — então um "Virtual Cockpit" no simulador seria **escrever um sink** que entra no grupo e decodifica, não reaproveitar um receptor do firmware.

---

## Conferência feita pela sessão do simulador (11/10/2026, noite)

Antes de usar o repasse, conferi o que dava para conferir só lendo a partição `system` do **MU0883** (o firmware da bancada), num container descartável com a imagem montada para leitura. Nada foi executado do firmware.

| Afirmação do repasse | O que achei no MU0883 | Situação |
|---|---|---|
| destino `ff14::2:4`, porta 42800 | `/lge/app_ro/network/network` (o programa que carrega as regras de rede) tem as regras `-s fd53:7cb8:383:3::73 -d ff14::2:4 -p udp --dport 42800 -j ACCEPT`, de entrada e de saída, e as mesmas para `fd53:7cb8:383:5::73` | **confirmado**, e com um dado a mais: é **UDP** |
| a central transmite, o painel recebe | o endereço de origem nessas regras é o da própria central (`::73`); `/lge/app_ro/ethernet/vlan-config.sh`, linha 57, põe a rota do grupo na VLAN 5: `ip -6 route add ff14::2:4 dev eth0.5 table local` (a central é `fd53:7cb8:383:5::73` nessa VLAN) | **confirmado**; por isso as duas correções acima |
| `us_displaymanagement`, `FctVideoStreams`, `NAV_KOMBI_STREAMPARAMETERS_NAME`, `ipStreamingEnabled` | são símbolos de `/lge/app_ro/lib64/libMIBNavSDClientLib_CarIF.so.00` (2,6 MB), no espaço de nomes `Viwi::NavSd`: `us_displaymanagement::NAV_KOMBI_STREAMPARAMETERS_NAME`, `DisplayManagementServiceHandler::ipStreamingEnabled()`, `FctVideoStreams::startup()` e `shutdown()` | **confirmado** que existem e onde. Não refiz a desmontagem da função nem o endereço `0x214814` |
| `multi-screen-server` com H.264 / OMX.qcom | `/lge/app_ro/graphics/multi-display/multi-screen-server` tem `OMX.qcom.video.encoder.avc`, o arquivo-fonte `multi-screen/encoder/Rgba2yuv.cpp`, mensagens `[omx_encoder]` que citam memória ION, e as opções `-m Multicast` e `-u Unicast (disabled)` | **confirmado**, como **codificador** por hardware |
| ordem de partida | `multi-display.service` roda o `multi-screen-server` e tem `After=naviengine.service`; o `naviengine.service` roda `/lge/app_ro/navi/naviengine/runNavi.sh` e tem `After=navi-al.service` | dado novo |
| adaptação do painel, canal `0xB5A`, byte 7 / bit 1 | a tabela de campos que o Marco 6 tirou das classes da HMI (`stubs/codificacao/tabela-campos.tsv`) tem o DID `0B5A` com `ID_ABILITY_SWITCH_NAV_MAPS` ("Map Switch") no byte 7, bit 1, e mais três campos que o repasse não cita: **`ID_NAVIGATION_MAP_TRANSMISSION_MODE`** no byte 0 (0 desligado, 1 transmissão por IP, 2 por MOST, 3 LVDS), `ID_NAVIGATION_MAP_COMPRESSION_MODE` no byte 1 e `ID_NAVIGATION_MAP_RESOLUTION` no byte 2 | o bit **confere**; o modo de transmissão é dado novo e é o primeiro candidato a ligar o vídeo |

**Uma ressalva sobre o texto de cima.** Onde ele diz que o campo "já está ligado no Jetta": isso não foi lido do carro, que ainda não tem autoscan nem leitura do 5F. É dedução do fato de o painel do carro mostrar o mapa (ficha do carro). Os valores do `0B5A` do Jetta são desconhecidos.

**Atualização (14/10/2026): lido do carro, e confere.** O primeiro autoscan do Jetta (ODIS-E 19.0.0, cópia de adaptação do 5F em [`carros/jetta-gli-2021/autoscans/5F-mib3-2026-10-14/`](../../../carros/jetta-gli-2021/autoscans/5F-mib3-2026-10-14/LEIA-ME.md)) trouxe o grupo `Dashboard_Display_Configuration` com os valores reais deste carro:

| Campo (sufixo que o ODIS-E mostra) | Valor no Jetta | Byte do `0B5A` na tabela do Marco 6 |
|---|---|---|
| `navigation_map_transmission_mode` | **IP_streaming** | byte 0 = 1 ("transmissão por IP") |
| `navigation_map_compression_mode` | **H264** | byte 1 |
| `navigation_map_resolution` | **1** | byte 2 |
| `ability_switch_nav_maps` | **yes** | byte 7 / bit 1 |
| `crossroad_detail_map_transmission_mode` | off | — |

Ou seja: o "primeiro candidato a ligar o vídeo" da linha de cima **está em IP no carro**, e o codec é **H.264** — exatamente o que o firmware MU0883 da bancada já mostrava (`ff14::2:4`, `multi-screen-server` H.264/OMX.qcom). A dedução vira leitura: o mapa nativo no painel sai da central como vídeo IP H.264, e isso agora é dado do próprio Jetta (MU 0803), não de proxy do GLI 2022 público. A ressalva acima fica como histórico.

**O que isso quer dizer para um "Virtual Cockpit" no simulador** (análise; nada foi tentado):

1. O codificador é por hardware (Qualcomm, com memória ION). No container ele não tem como funcionar. Então o caminho "entrar no grupo `ff14::2:4` e decodificar" só serve se antes alguém fizer o papel do codificador.
2. O nosso simulador tem um atalho que o do qemu-mhi2 não tinha: todo desenho de OpenGL já passa pelo `glhost`. Se o mapa do painel é desenhado por um processo que usa as mesmas bibliotecas de vídeo, dá para mostrá-lo numa segunda janela direto do `glhost`, sem H.264 e sem rede. Qual processo desenha esse mapa, e em que superfície, ainda não sei.
3. Antes de tudo isso vem o motor de navegação: `navi-al` e `naviengine` precisam subir, e precisam de mapas. Sem o pacote oficial de mapas não sei até onde eles vão.

---

## Investigação: o que falta para uma janela de painel no simulador (11/10/2026, noite)

Feita por uma sessão à parte, aberta pela sessão do simulador depois do pedido do Lipe ("Pode iniciar a sessão da navegação"). É o primeiro passo do item 8 do roteiro em [simulador-jetta.md](simulador-jetta.md): ver até onde sobem na bancada o motor de navegação e o programa de vídeo do painel, o que cada um exige e quem desenha o mapa do painel. **A janela não foi construída.** É estudo num container descartável e sem rede; nada vai para o carro.

**Marcas usadas abaixo:** `[EXEC]` visto em execução nas rodadas `r1` e `r2`; `[ESTÁTICO]` lido nos arquivos do firmware (MU0883), sem executar; `[DEDUÇÃO]` conclusão minha.

### Resposta curta

1. **Quem desenha o mapa do painel é o próprio motor de navegação da central**, o `tsd.nav.mainapp.mib3` (núcleo TSD, da Preh Car Connect). Ele tem duas janelas de desenho na configuração: `mainunit` (800×480, a tela do rádio) e **`kombi` (1280×480, até 12 quadros por segundo)**. Desenha com as mesmas duas bibliotecas de vídeo que o encaminhador da bancada já substitui. `[ESTÁTICO]`
2. **Dá para mostrar esse mapa numa segunda janela direto do `glhost`, sem H.264 e sem rede.** O `multi-screen-server` só pega o quadro pronto dessa janela (pelo Weston), converte, codifica e manda; ele não desenha nada. `[ESTÁTICO]` O motor carrega com o encaminhador no lugar das duas bibliotecas e este exporta as 127 funções de EGL e OpenGL que o motor importa. `[EXEC]` **Não vi o motor desenhar**: ele não chegou a criar janela nem a fazer chamada de vídeo.
3. **O motor sobe na bancada.** O `navi-al` sobe inteiro. O motor parava esperando o compositor da central (Weston); com um compositor de mentira, que só responde o aperto de mão do protocolo, ele cria os 34 componentes, registra o serviço `navsystem` e a HMI passa a conversar com ele. `[EXEC]`
4. **Onde ele para agora: ativação de navegação.** O motor fica no estado `ready4navSwap` e a tela da tecla NAV muda de "Starting navigation…" para "This function is unavailable.". Ele espera duas coisas: a função de navegação vinda do diagnóstico e o estado de ativação (licença) vindo do microcontrolador. O pedido desse estado passa por uma mensagem segura que depende de um chip da placa. **Não mexi nisso; é decisão do Lipe** (item 2 da lista do fim). `[EXEC]`
5. **Os mapas ainda não são o gargalo.** O motor nem procura o banco de mapas enquanto não recebe o país da codificação. O pacote oficial que o Lipe baixou (América do Sul) serve a este firmware, que já conhece o submercado `SAM`; não foi instalado nem testado. `[EXEC]` para a espera, `[ESTÁTICO]` para o resto.
6. **O caminho de vídeo de verdade não precisa ser refeito.** O `multi-screen-server` desiste logo no começo, porque precisa de uma extensão do Weston (`ivi_share`) e depois de GPU, codificador e memória da Qualcomm. `[EXEC]` para a desistência.

### 1. O que o firmware traz (inventário, leitura estática)

Refeito por [driver/vc_inventario.sh](driver/vc_inventario.sh); saídas em `traces/vc_inv_*`.

| Peça | Onde | O que faz e do que precisa |
|---|---|---|
| `navi-al` (`alm`) | `/lge/app_ro/navi/navi-al/` | Ponte entre o barramento da central (kipc) e o motor. Sobe um barramento D-Bus de sessão só para os dois (`dbus-launch > /tmp/dbus.conf`) e oferece ao motor, por CommonAPI, persistência, arquivos seguros, GPS, sensores, estado do veículo, rede, diagnóstico e ativação. Precisa de `/dev/csr` e `/dev/css` (kipc) e do registro RSI |
| motor (`tsd.nav.mainapp.mib3`, 71 MB) | `/lge/app_ro/navi/naviengine/package/<eu\|nar>/<single\|dual>/usr/bin/` | Quatro pacotes de 250 MB, com o **mesmo executável**; mudam a configuração e as imagens. Versões `NAVCOREV2_LG_MOI3_EU_RDW2_VW_MQB_SingleMap_sec_028.42.4` e `…_EU_RDW1_RDW2_VW_MQB_DualMap_sec_028.42.4`. Partida em `runNavi.sh`. Liga contra `libEGL_adreno.so`, `libGLESv2_adreno.so`, `libwayland-client`, `libwaylandegl`, `libilmClient`, SQLite do NDS, CommonAPI e as bibliotecas RSI |
| escolha do pacote | `/lge/app_ro/post_init/data/navigation.sh` | Na primeira partida, cria o link `/data/lge/navi/navicore` para um dos quatro pacotes conforme dois resistores de identificação da placa: `/sys/devices/soc/soc:common_gpio_default/market_id` (`0x1` = América do Norte, o resto = `eu`) e `grade_id` (`0x1` = `dual`, `0x0` = `single`). O mesmo roteiro cria `/data/lge/navi/{pso,pos,persist,cert}` e arruma a pasta de mapas |
| `rsi-display-management` | `/lge/app_ro/graphics/multi-display/` | Serve o RSI `displaymanagement`: telas (`displays`), superfícies de desenho (`renderingSurfaces`) e fluxos (`streamParameters`). Tem uma tabela `surfaces.map` com os nomes `navi`, `test` e `navi-kombi-renderingsurface` |
| `navsd_app` (`libMIBNavSDClientLib_CarIF`) | `/lge/app_ro/carif/` | A função BAP "NavSD", a que conversa com o painel. Lê a adaptação do painel (`MIB3_KEY_ADAPTION_DASHBOARD_DISPLAY_CONFIGURATION`: transmissão por IP, resolução, troca de mapa), cria no `displaymanagement` os elementos `navi-kombi-display`, `navi-kombi-renderingsurface` e `navi-kombi-streamparameters` (destino `ff14:0000:0000:0000:0000:0000:0002:0004`), liga a superfície ao renderizador `kombi` e anuncia o fluxo ao painel pela função BAP `VideoStreams` (tipo, endereço IPv6, porta UDP, URI) |
| `multi-screen-server` | `/lge/app_ro/graphics/multi-display/` | Ver a seção 2 |
| Weston, módulo `ivi-controller.so` | `/usr/lib64/weston/` | É quem implementa o `ivi_share` ("Buffer Sharing"): entrega a um segundo programa o buffer de uma superfície de outro |
| HMI, pacote `nav2` | jar da HMI | Espera o serviço RSI `navsystem` (recurso `systems`, elemento `mainunit`, estado `operable`) e depois `guidance`, `homeview`, `maprenderer`, `navdata`, `places`, `routing`, `traffic` e `tripview`. Assina `maprenderer/renderers` com `name=mainunit`. Estados que ela conhece: `starting`, `ready4navSwap`, `ready4navFod`, `operable` |

Ordem de partida no carro: `vw-hmi` → `navi-al` → `naviengine` → `navsd`, `multi-display`, `rsi-display-management` (`traces/vc_inv_unidades.txt`).

**As janelas de desenho do mapa** (`MapRendererConfig` em `etc/nav/standardConfig.cfg`; nomes dos campos tirados do descritor embutido no executável; [driver/vc_le_config.py](driver/vc_le_config.py), saída em `traces/vc_inv_config_motor.txt`):

| Pacote | `maxVisibleRendererInstances` | `mainunit` | `kombi` |
|---|---|---|---|
| `eu/single` | 1 | 800×480, 117 dpi, até 30 quadros/s, id 19 | 1280×480, 137 dpi, até 12 quadros/s, id 37 |
| `eu/dual` | 2 (`dualMap`) | 1280×640, 156 dpi, até 30 quadros/s | igual |

No pacote `single` só um renderizador fica visível por vez: o mapa está **ou** no rádio **ou** no painel (`ActiveSingleMapRenderer`, guardado na persistência; sem valor guardado, o código cai em "Kombi"). É o que a adaptação "Map Switch" (DID `0B5A`, byte 7, bit 1) habilita. A HMI da bancada é a de 800×480, que casa com o `single`; qual pacote a central do Jetta usa não foi lido. `[DEDUÇÃO]` para o Jetta.

**Acrescentado em 11/10/2026, às 21h05, com a resposta do Lipe.** Perguntei se, no carro, o mapa some da central quando está no painel. Ele respondeu: "Isso, quando o mapa está no Active Info Display ele não mostra na Mib3". É o comportamento do pacote `single` (um renderizador visível por vez), o mesmo que as rodadas `r1` e `r2` usaram. Continua sendo observação de uso: o `grade_id` da placa e a adaptação "Map Switch" do carro não foram lidos. Registrado também na [ficha do carro](../../../carros/jetta-gli-2021/README.md), linha do painel de instrumentos. Para o simulador isso quer dizer: a janela de painel e o mapa da tela do rádio são alternativas, não as duas ao mesmo tempo, e quem escolhe é o valor `ActiveSingleMapRenderer`, que o motor guarda na persistência.

**Outro dado que já estava na ficha e eu não tinha usado:** no tópico público de um GLI 2022 manual com MIB3, o 5F tem `Dashboard_Display_Configuration-navigation_map_transmission_mode` = `IP_streaming` e o painel tem `Switching_Networking` = `Ethernet` ([thread 33576](https://forums.ross-tech.com/index.php?threads/33576/)); num Jetta 2024 do Canadá o mesmo canal vale `OFF` ([thread 40425](https://forums.ross-tech.com/index.php?threads/40425/)). É o campo do byte 0 do `0B5A`. O valor 1 (transmissão por IP) que usei na `r2` é, portanto, o de um carro parecido; o do Jetta do Lipe segue sem leitura. Fonte: ficha do carro, seção "Rede, proteção de componentes e imobilizador", que cita os dois tópicos; não reli os tópicos nesta sessão.

**Os mapas.** `[ESTÁTICO]`

- Ficam fora da partição `system`: `/var/lib/nav` é a partição `mmcblk0p51` do eMMC (`fstab`). O post-init arruma o banco em `/var/lib/nav/nav/EUROPE/` (ou `NAR`) com `3D`, `FB`, `FTS`, `LC`, `SDS`, `WO`, `ROOT.NDS` e `package.db`.
- O formato é NDS (bancos SQLite). O motor traz o SQLite do NDS (`libext.sqlite.nds.so`), o leitor `libtsd.nav.middleware.nds.2.5.2.so` e o gerente de atualização da HERE (`libndsdm.so`).
- A configuração lista 14 submercados com os países de cada um: `EUROPE`, `NAR`, `SA` (África austral), **`SAM` (AR BR CR GF GP CO PA PY PE UY VE MX MQ)**, `NA`, `AUS`, `AGCC`, `CHILE`, `ISR`, `ASIA`, `TUR`, `INDIA`, `KOR`, `JPN`. O submercado sai do país da codificação ("Setting countryOfSale configuration to the value from coding", "Determined submarket path as").
- Quando não acha o banco, o código tem as mensagens "Mib3MapDataService::asyncStart(): DB root file does not exist: ", "No compatible DAL was found" e o registro de falha `DataBaseMissing`. Não vi essas mensagens saírem, porque o motor não chegou a esse ponto (seção 3).

### 2. Quem desenha o mapa do painel, e de onde o vídeo tira os quadros

```
navsd_app  ── cria tela, superfície e fluxo ──►  rsi-display-management (RSI displaymanagement)
     │                                                │ id da superfície IVI, tamanho        │ lista de fluxos
     │ liga a superfície ao renderizador "kombi"      ▼                                      ▼
     └────────────────────────────►  motor (tsd.nav.mainapp.mib3)                 multi-screen-server
                                       desenha o mapa por libEGL_adreno e           pede ao Weston o buffer
                                       libGLESv2_adreno numa janela Wayland  ──►    da superfície (ivi_share),
                                       IVI de 1280×480                  Weston      converte para NV12 num shader
                                                                                    (Rgba2yuv), codifica em H.264
                                                                                    (OMX.qcom, memória ION),
                                                                                    empacota em MPEG-TS e manda
                                                                                    por UDP em eth0.5
```

- **O motor desenha.** Classes `tsd.nav.mapdisplay.mib3.display.WaylandDisplay` e `WaylandWindowSurface(wl_display*, wl_compositor*, ivi_application*, uint32_t, uint32_t, uint32_t)`; importa `wl_egl_window_create`, `eglCreateWindowSurface`, `eglSwapBuffers` e 108 funções de OpenGL, todas do ES 3.0 (`traces/vc_inv_importados.txt`). O tamanho e o número da superfície IVI ele pede por RSI ao `displaymanagement` (mensagem "Window parameters for renderingSurface are: width = , hight = , WindowId = "). `[ESTÁTICO]`
- **O `multi-screen-server` só transporta.** É cliente Wayland: pede `get_ivi_share_surface(<id>)`, recebe a cada quadro o buffer da superfície (evento `damage`), passa por um shader (modos `--buf-gles-copy`, o padrão, `--buf-rgba2yuv`, `--buf-memcpy`, `--buf-usebuffer`), entrega ao `OMX.qcom.video.encoder.avc` e manda o resultado em MPEG-TS (classe `TransportStream`, com tabelas PAT e PMT) por `sendto()`, num soquete preso à interface `eth0.5`. Padrões do texto de ajuda: 4 Mbit/s e período de quadro-chave 4. O serviço sobe sem argumentos e pega tela, superfície, IP, porta e taxa de quadros do RSI `displaymanagement`. Abre `/dev/dri/renderD128` (GBM) e `/dev/ion`. `[ESTÁTICO]`, strings em `traces/vc_inv_multi-screen-server.strings`
- **Então o `Rgba2yuv` não é um renderizador:** a entrada dele é a textura do quadro que o motor já desenhou. Não há nada do mapa que só exista depois do codificador.

### 3. As rodadas (verificado em execução)

Roteiros novos, que usam o lançador da bancada como está: [driver/vc_rodada.sh](driver/vc_rodada.sh) (no Mac) e `driver/vc_rodada_container.sh`. Cada rodada sobe a HMI num container próprio (`moi3-vc-<tag>`, sem rede) e, ao lado, `alm`, o motor, `rsi-display-management`, `navsd_app` e `multi-screen-server`, cada um sob `strace`. O motor e o `multi-screen-server` ganham um `glhost` só deles. A central ao vivo não foi tocada. 150 s cada, identidade provisória do Jetta, tecla NAV aos 80 s.

| Rodada | O que mudou | Observado |
|---|---|---|
| `r1` | base: sem compositor, `0B5A` zerado | **`alm` sobe inteiro.** O motor cria 13 componentes e **para em "Create component: NAV1_MAPDISPLAY started"**: 14.319 tentativas de ligar em `/run/platform/weston/wayland-0`, todas sem sucesso. Não registra serviço RSI nenhum. O `multi-screen-server` fica na mesma espera (13.826 tentativas). `rsi-display-management` registra `displaymanagement` (porta 49600) com as três listas vazias. Os seis processos ficam de pé até o fim. Tecla NAV: "Starting navigation…" |
| `r2` | compositor de mentira ([driver/vc_wlstub.c](driver/vc_wlstub.c)) e `0B5A` com byte 0 = 1 (transmissão por IP) e byte 7, bit 1 = 1 (troca de mapa) | **O motor cria os 34 componentes (em 61 ms), registra `navsystem` (porta 49503) com 0,4 s de vida e entra em `ready4navSwap` com 1,2 s.** A HMI vê o serviço, liga nele e assina o estado. O `navsd_app` liga no `navsystem` e no `displaymanagement` e não cria nada. O `multi-screen-server` liga no compositor, pede `wl_compositor` e `ivi_application` e sai 1 ms depois; o código dele tem a mensagem "ERROR: ivi_share not supported", que não vi sair porque ele roda mudo. Tecla NAV: **"This function is unavailable."** |

| `r1`, tecla NAV | `r2`, tecla NAV |
|---|---|
| ![Sem o motor](quadros/2026-10-11-hmi-vc-nav-sem-motor.png) | ![Motor de pé, sem ativação](quadros/2026-10-11-hmi-vc-nav-motor-sem-ativacao.png) |

O mapa atrás de "Starting navigation…" é uma imagem parada da própria HMI, não é o motor.

**O `navi-al` (`alm`).** Com `LOG_STDOUT=1` (variável que o próprio `runNavi-al.sh` define como 0) ele escreve o registro em claro. Registra todos os serviços CommonAPI, liga no kipc e no registro RSI. Dois pontos:

- Ele recebe os conjuntos de codificação que o `diagstub` publica, mas responde "Data Type : Unknown !!!" a todos. Ele **lê os três primeiros bytes da carga** (chama de `MainCmd`, `subCmd1` e `subCmd2`), que a bancada manda zerados. Para a HMI e o `persistence-app` esses bytes não importam (Marco 6); para o `alm`, importam. Por isso o motor recebe país 0 ("unknown CountryCoding", 735 vezes na `r2`) e função de navegação desligada.
- Ele pede o estado de ativação ("[SWaP] request status step=[0]") e falha em "[NAVSECMSG] could not open": o arquivo que não existe é `/sys/devices/soc/75ba000.spi/spi_master/spi12/spi12.0/challenge_gen`, o mesmo que o `usermgnt` de verdade procurava no Marco 4.

**O motor na `r2`.** Linhas do registro dele (`traces/vc_r2.navi.out`):

- "WaylandDisplay :: Display is ready for usage." Ele usou `wl_compositor` e `ivi_application`; não pediu `wl_output`.
- "FEC: activation modus obtained 0 (0 - undefined, 1 - SWaP, 2 - FoD)", "R4N : nav : uds = Disabled, fec = Undefined", "IsOperable: change the RSI status of the navsystem to Ready4navSwap". São duas entradas: `uds`, a função de navegação que vem do diagnóstico pelo `alm`, e `fec`, o estado de ativação (licença). O código fala em sinal "combined UDS / FEC".
- `GET /navsystem/systems`: `"name":"mainunit"`, `"status":"ready4navSwap"`, `"databaseStatus":"updating"`, `"softwareVersion":"NavCoreV2-028"`.
- "Waiting for country coding info to become available": o serviço de dados de mapa espera o país antes de abrir o banco. O motor não abriu nada em `/var/lib/nav/nav`.
- Posição de partida 52,4321 N, 10,7975 L (a da configuração, em Wolfsburg), "Vehicle is off road".
- Não registrou `maprenderer` nem os outros sete serviços que a HMI espera, e o `glhost` dele não recebeu cliente: **nenhuma chamada de vídeo do motor foi vista.**

**O pedido de vídeo com `0B5A` byte 0 = 1.** O `navsd_app` liga em três lugares: registro (443), `navsystem` (49503) e `displaymanagement` (49600). No `navsystem` ele assina `systems/?name=mainunit` e o campo `status`. As listas do `displaymanagement` continuam vazias. Minha leitura: ele espera o motor ficar `operable` antes de criar a tela, a superfície e o fluxo do painel. `[DEDUÇÃO]` para o motivo; `[EXEC]` para as ligações e as listas vazias. Para quem ele pede, quando pedir: o RSI `displaymanagement`, servido pelo `rsi-display-management`. A criação do fluxo em si não foi observada.

**O soquete de log.** Os daemons mandam o registro para `/tmp/trace_unix_socket`, um soquete de fluxo. Com alguém escutando ([driver/vc_trace_escuta.c](driver/vc_trace_escuta.c)), a HMI manda o registro inteiro dela (texto em UTF-16, quadros "FPT-MES"); os daemons nativos mandam só um cumprimento de 52 bytes e mais nada. Deve faltar uma resposta do daemon de rastreio; não investiguei. O que funcionou para ver os nativos foi a saída padrão: `LOG_STDOUT=1` no `alm`, e o motor, que já escreve nela.

### 4. Dá para mostrar o mapa do painel numa segunda janela direto do `glhost`?

Sim, em princípio, e é o caminho que recomendo. O que sustenta:

| Ponto | Situação |
|---|---|
| o mapa do painel é desenhado por OpenGL, pelas duas bibliotecas que o encaminhador substitui | `[ESTÁTICO]` |
| o encaminhador exporta tudo o que o motor importa: 127 funções de EGL e OpenGL, nenhuma faltando (`traces/vc_inv_shim_cobre_o_motor.txt`) | `[EXEC]` (conferência de símbolos) |
| o motor carrega e roda com o encaminhador no lugar das duas bibliotecas | `[EXEC]` |
| um segundo `glhost`, com soquete e pasta de quadros próprios, sobe ao lado do da HMI | `[EXEC]`; ficou sem cliente |
| a janela de 1280×480 chega ao `glhost` e vira quadro | **não verificado**: o motor não criou janela |
| o `glhost` dá conta do OpenGL ES 3.0 do motor (blocos de uniformes, instâncias, texturas 3D, objetos de sincronia, multiamostragem). A HMI usa só o ES 2.0 | **não verificado** |
| o que o motor pede por `eglGetProcAddress` | **não verificado** |

Com isso, o codificador por hardware, o `multi-screen-server`, o `ivi_share`, a VLAN 5 e o grupo `ff14::2:4` ficam fora do caminho da janela.

### 5. O que falta para a janela de painel, em ordem

| # | O quê | Tamanho | Depende de |
|---|---|---|---|
| 1 | **Compositor de mentira como peça da bancada.** Hoje é um experimento de 200 linhas que só atende o aperto de mão. Falta virar opção do lançador e passar por uma sessão com janela criada | pequeno | o lançador é de outra sessão |
| 2 | **Decisão sobre a ativação.** O motor não passa de `ready4navSwap` sem a função de navegação (`uds`) e, talvez, sem o estado de ativação (`fec`). O `fec` vem por mensagem segura com desafio de um chip da placa. **Fronteira: não imitei nem contornei.** O `uds` é codificação comum e depende do item 3; só depois dele dá para saber se basta. **Atualizado em 11/10/2026, 21h50: não basta.** Com "uds = Enabled" o motor segue em `ready4navSwap` (seção "Item 3", mais abaixo) | decisão | **Lipe** |
| 3 | **Diagnóstico no formato que o `alm` entende.** Descobrir os três bytes de cabeçalho (`MainCmd`, `subCmd1`, `subCmd2`) com que o microcontrolador manda cada conjunto, e pôr no `diagstub`. Sem isso o motor não recebe país nem função de navegação. **Atualizado em 11/10/2026, 21h50: os três bytes são `83 60 22`, e foram testados com uma ferramenta à parte** (seção "Item 3", mais abaixo). Falta só a mudança de uma linha no `diagstub`. **Feito às 23h26 de 11/10/2026** pela sessão do Marco 6 (D77 em `modelagem-soc.md`) e conferido com o `navi-al` e o motor no ar nas rodadas `r6` e `r6b`: item fechado | ~~médio~~ feito | ~~Marco 6 (dono do `diagstub`)~~ |
| 4 | **Mapas.** Pôr o pacote do Lipe no formato instalado e montar em `/var/lib/nav/nav`. Para o estado de São Paulo bastam a raiz, a visão geral e uma região: cerca de 2 GB dos 9,6 GB. O nome da pasta de submercado só aparece no registro depois do item 3. **Atualizado em 11/10/2026, 21h50: com a região de teste 3 (MSA) a pasta é `SAM`, e a raiz que o motor procura é `/var/lib/nav/nav/SAM/ROOT.NDS`** (seção "Item 3", mais abaixo) | pequeno a médio | item 3; disco (32 GB livres) |
| 5 | **Superfícies de desenho.** Fazer existir o `renderingSurface` de cada janela e ligá-lo ao renderizador. No carro quem cria o do painel é o `navsd_app`, quando o painel pede por BAP. Na bancada: simular o painel no BAP (grande) ou criar os elementos à mão no RSI `displaymanagement` (médio) | médio a grande | itens 2 a 4 |
| 6 | **`glhost` e encaminhador.** (a) janela com o tamanho da janela nativa: hoje toda janela nasce do tamanho do `glhost`; (b) um fluxo de quadros por superfície, se o motor usar as duas no mesmo processo: hoje todas caem no mesmo fluxo; (c) o que aparecer de ES 3.0 não exercitado; (d) para o mapa do rádio, compor o mapa por baixo da HMI, o que pede transparência no fluxo (hoje é RGB) | médio | item 5 para testar |
| 7 | **Segunda janela no Mac**, 1280×480, lendo o segundo soquete de transmissão | pequeno | item 6 |
| 8 | **Posição simulada.** O motor nasce em Wolfsburg. Caminhos a estudar: GPS e sensores pelo `alm` (mensagens kipc) ou a rota de demonstração que a configuração cita (`demoroute/route.gpx`) | médio; não investigado | itens 2 a 4 |

**O que depende do Lipe:** a decisão do item 2; o pacote de mapas (já está em `~/Downloads`); e, quando houver leitura do módulo 5F, os valores do `0B5A` e o país. Uma observação no carro que ajuda desde já: quando o mapa está no painel, o mapa some da tela da central? Se some, a central do Jetta é do tipo "um mapa por vez" (`single`). **Respondido pelo Lipe em 11/10/2026, às 21h05: some.** Ver o acréscimo na seção 1.

### 6. Fronteiras e o que não foi verificado

- **Ativação de navegação: não toquei.** Não imitei o chip, não fabriquei resposta de ativação, não mexi na configuração do motor (há nela uma seção de simulação, que só li pelo nome). Se o item 2 for adiante, é por decisão explícita do Lipe.
- **Nada foi contornado.** Nenhum binário foi alterado; a partição `system` entra só para leitura. O compositor de mentira não quebra proteção nenhuma: ele faz o papel de um programa que falta na bancada.
- **Nenhum mapa foi instalado nem aberto pelo motor.** O pacote é de atualização, com uma lista assinada (`PACKAGE.SLIST`); no carro quem instala e confere é o gerente de atualização do motor. Arrumar os arquivos à mão no formato instalado é experimento a fazer, não fato.
- **Só o pacote `eu/single` foi rodado.** Ninguém leu `market_id`, `grade_id` nem `variant_id` nas rodadas; o link do pacote foi criado pelo roteiro, como o post-init faria.
- **O compositor de mentira não foi exercitado com janela.** O motor só fez o aperto de mão.
- **O `rsi-display-management` e o `navsd_app` rodaram mudos.** O que sei deles vem das ligações e do que o RSI respondia.
- O motor registra a cada 0,2 s o erro do país desconhecido; não medi se isso pesa.
- **As rodadas usaram o lançador como ele estava na hora** (`driver/m1_hmi.sh` com data de 20h23; `r1` às 20h33, `r2` às 20h40). Outra sessão estava mudando esse arquivo no mesmo período. Se o lançador mudar de novo, vale repetir a `r1` antes de comparar.
- Foram duas rodadas, uma de cada tipo. Não repeti nenhuma para ver se o resultado se mantém.

### 7. O que muda no texto de cima

| Onde | Estava | O que achei |
|---|---|---|
| seção 1, "Protocolo/empacotamento" | transporte provável Ethernet AVB | o programa manda MPEG-TS por `sendto()` num soquete UDP preso a `eth0.5`; entre as dependências dele não há biblioteca de AVB `[ESTÁTICO]` |
| seção 1, "Resolução/taxa" | literais não extraídos | a janela `kombi` da configuração do motor é 1280×480, até 12 quadros/s. A resolução pedida ao painel ainda passa pelo byte 2 do `0B5A`, que não segui |
| seção 2, "Caminho de rota (dados, não vídeo)" | o painel desenharia o mapa a partir de dados de rota | no MOI3 o renderizador `kombi` fica dentro do motor da central; `KombiMapRendererParams` são os parâmetros dessa instância `[ESTÁTICO]` |
| seção 4, "Motor de navegação" | papel exato do `naviengine` como dedução | é ele que desenha os dois mapas `[ESTÁTICO]` |
| resumo de uma linha e "Conferência", item 2 | a janela seria um receptor que entra no grupo e decodifica; o atalho pelo `glhost` era hipótese | o atalho se sustenta (seção 4 desta investigação) |

### 8. O pacote de mapas do Lipe

Em 11/10/2026 o Lipe ofereceu o pacote nesta sessão ("estou baixando … e logo disponibilizo para você usar") e avisou: "MRM4_SAM_16PR169_CT3WS-251H0_Offline_Update está em Downloads". Página de origem, pela captura que ele mandou: South America, atualizado em 15/06/2026, arquivo tar, cobrindo Argentina, Bahamas, Brasil, Colômbia, Costa Rica, Guiana Francesa, Guadalupe, Martinica, México, Panamá, Paraguai, Peru, Porto Rico, Uruguai e Venezuela.

- `~/Downloads/MRM4_SAM_16PR169_CT3WS-251H0_Offline_Update.tar`, 9.627.217.920 bytes. **Só listei o conteúdo e extraí os arquivos de descrição** (168 arquivos, 696 kB, no rascunho da sessão). Nada foi instalado.
- 85 pacotes (`traces/vc_mapa_sam_pacotes.txt`): raiz `MIB31_MRM1_SAM.ROOT` (67 MB), visão geral `SAMWO` (1,35 GB), 28 regiões de mapa `SAMLC.UR…` (5,4 GB), 28 de busca `SAMFTS` (1,4 GB), 25 de modelos 3D `SAMCM` (278 MB), voz `SAMSDS` (1,1 GB) e a lista assinada `PACKAGE.SLIST`.
- Cada `PACKAGE.CFG` diz o destino de cada arquivo no banco instalado (por exemplo `LC/UR22301/ROUTE.NDS`). Pelo centro de cada região: `UR22301` (503 MB) é o estado de São Paulo, `UR22302` Minas Gerais, `UR222xx` Argentina, `UR224xx` Colômbia, `UR230xx` México.
- Há em `~/Downloads` uma pasta mais antiga, já extraída (`MRM4_SA_15PR154_CT3WS-251E0_Offline_Update`, 1,5 GB, de setembro). Olhei só para entender o formato: `ROOT.NDS` e `PRODUCT.NDS` são SQLite comum; os blocos grandes são SQLite com páginas comprimidas (cabeçalho "ZV-zlib").

### 9. Como reproduzir, e o que entrou na pasta

```bash
# inventário (só leitura)
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v /tmp/vc:/out \
  -v "$PWD/driver":/driver:ro moi3-bancada:1 sh /driver/vc_inventario.sh
python3 -I driver/vc_le_config.py /tmp/vc/vc_inv_standardConfig_eu_single.cfg

# r1 (base) e r2 (compositor de mentira e 0B5A em transmissão por IP)
J="stubs/codificacao/padrao.txt stubs/codificacao/jetta-gli-2021.txt stubs/codificacao/jetta-gli-2021-deducoes.txt"
sh driver/vc_rodada.sh r1 /tmp/vc $J
WLSTUB="800 480 174 104" sh driver/vc_rodada.sh r2 /tmp/vc $J traces/vc_painel_ip.txt
```

Arquivos novos, todos desta investigação (nenhum arquivo de outra sessão foi alterado, e a central ao vivo não foi tocada): `driver/vc_inventario.sh`, `driver/vc_le_config.py`, `driver/vc_rodada.sh`, `driver/vc_rodada_container.sh`, `driver/vc_trace_escuta.c`, `driver/vc_wlstub.c`; dois quadros em `quadros/2026-10-11-hmi-vc-nav-*.png`; e as saídas em `traces/vc_*` (15 MB): `vc_inv_*` do inventário, `vc_r1.*` e `vc_r2.*` das rodadas (`navi.out` e `navi.strace` do motor, `alm.out` do `navi-al`, `rsi.txt` com o que o RSI respondia, `wlstub.log`, `trace.log` do soquete de log), `vc_painel_ip.txt` (o ajuste de teste do `0B5A`) e `vc_mapa_sam_pacotes.txt`.

---

## Item 3: o diagnóstico no formato que o `navi-al` entende (11/10/2026, noite)

Feito por uma sessão à parte, aberta pela sessão da investigação acima. É o item 3 da lista da seção 5: descobrir com que cabeçalho o `navi-al` (`alm`) aceita os conjuntos de codificação e adaptação, e ver na bancada se, com ele, o motor de navegação recebe a região e a função de navegação. Estudo em containers descartáveis e sem rede; nada vai para o carro, nenhum binário foi alterado e a central ao vivo não foi tocada. **O `diagstub` não foi alterado**: é do Marco 6, e a nota para ele está na seção 7 daqui.

**Marcas:** `[EXEC]` visto em execução nas rodadas `r2b` a `r5` e nas oito repetições (`r5b`, `r5c`, `a1` a `a3`, `b1` a `b3`); `[ESTÁTICO]` lido no código do firmware (MU0883), sem executar; `[DEDUÇÃO]` conclusão minha.

### Resposta curta

1. **O formato.** O `alm` só aceita uma mensagem do canal do microcontrolador de diagnóstico se os três primeiros bytes da carga forem `83`, `60` e o serviço. Para os conjuntos de codificação e adaptação o serviço é `22`. O identificador (DID) e os dados ficam onde a bancada já os põe; só mudam os três bytes que hoje vão zerados. `[ESTÁTICO]` e `[EXEC]`

   ```
   [canal: 8 bytes = 0x0800000000000000][83][60][22][DID: 2 bytes, o alto primeiro][dados]
   ```

2. **Com o cabeçalho certo o `alm` reconhece 8 dos 48 conjuntos da bancada**, e o motor passa a receber a função de navegação: a linha dele muda de "uds = Disabled" para **"R4N : nav : uds = Enabled, fec = Undefined"**. `[EXEC]`: nas 8 rodadas com o cabeçalho novo saiu "Enabled", nas 4 sem ele saiu "Disabled"
3. **Com a região de teste (MSA) o motor para de reclamar do país**: nenhuma linha "unknown CountryCoding" (eram 721 no controle). Ele traduz a região para o país de venda **`BRA`**, escolhe o submercado **`SAM`** e procura a raiz do banco de mapas em **`/var/lib/nav/nav/SAM/ROOT.NDS`**, que não existe. **Nenhum mapa foi instalado.** `[EXEC]`: igual nas 7 rodadas com o cabeçalho novo e a região 3
4. **Parada na fronteira.** Com `uds = Enabled` o estado do motor continua `ready4navSwap` e a tecla NAV continua em "This function is unavailable.". O que segue indefinido na linha é o `fec`, o lado da ativação. Parei aí: não li, não imitei e não contornei esse lado. A decisão do item 2 da lista continua com o Lipe. `[EXEC]` para o estado
5. **A HMI lê os mesmos valores com qualquer cabeçalho.** Com os 48 conjuntos no formato novo ela registrou as mesmas 104 leituras, nas 6 rodadas em que isso foi feito, e em 5 delas mostrou as mesmas telas. Na sexta (`r5c`) a tecla NAV não teve efeito, por uma corrida de partida da bancada que não passa pelo diagnóstico (seção 4). `[EXEC]`

### 1. O formato, lido no `alm`

`/lge/app_ro/navi/navi-al/alm` (649.400 bytes, md5 `c77a430214ff734a8552a9c29c7d9ba5`), função `KipcProvider::procDiagMsg(int, char const*)`, no endereço `0x31928` (o símbolo é exportado; 5.252 bytes). Lida no desmontado (`objdump`) e no decompilado do Ghidra 12.1.4 que já estava no Mac. O construtor do `KipcProvider` registra um tratador por remetente do barramento; o do remetente `0x0800000000000000` chama essa função. Tudo `[ESTÁTICO]`, salvo onde diz o contrário.

| Byte da carga | Nome no registro do `alm` | O que ele aceita | Se for outra coisa |
|---|---|---|---|
| 0 | `MainCmd` | `0x83` | "Data Type : Unknown !!!", linhas 396 e 397 do `kipcProvider.cpp` (é o que saía até aqui) |
| 1 | `subCmd1` | `0x60` | o mesmo |
| 2 | `subCmd2` | `0x22`, `0x31` ou `0x19` | o mesmo |
| 3 e 4 | `rdid1`, `rdid2` | o identificador, com o byte alto primeiro | "Unknown" de novo, mas em outras linhas: 368 e 369 (serviço `22`), 381 e 382 (`31`), 391 e 392 (`19`) |
| 5 em diante | | os dados | |

| `subCmd2` | Identificador | Tipo no registro ("Data Type") | O que o `alm` faz com ele |
|---|---|---|---|
| `22` | `0558` | Function Configuration Navigation | guarda os 4 bytes e repassa ao motor região, navegação, mapas, trânsito e mais sete funções (seção 2) |
| `22` | `0600` | Fuel Type e CarCoding | repassa marca, classe, geração e carroceria (`setCarType`) e o tipo de tração; grava os 25 bytes na persistência dele |
| `22` | `0B5A` | Map Switch | repassa a troca de mapa (`setSwitchMapEnabled`) |
| `22` | `0B1D` | Vze Activation | repassa o reconhecimento de placas (`setVzeEnabled`) |
| `22` | `09CB` | Country of Sale | guarda 2 bytes para o motor (`setCountryOfSale`) |
| `22` | `0E38` | Navigation GNSS Receiver Setting | guarda 2 bytes para o motor (`setGnssReceiverCoding`) |
| `22` | `0571` | PSO (Persistence) | grava os 2 bytes na persistência dele |
| `22` | `243F` | DeveloperTestMode | liga ou desliga um modo de teste do `alm` |
| `22` | `04FB` | RollMode | repassa o modo de rolo (`setKonzernrollenmodusEnabled`). A bancada não publica este |
| `31` | `0252` | Navigation System Calibration Reset | só age se os dados forem `01 00`: chama a rotina de zerar a calibração (`callStubWithCalibrationReset`; não li além dela) |
| `19` | `0A03` | Navigation database not available status | chama a rotina que informa o estado da falha de banco de dados ausente (`procDtcMsg`; não li além dela) |

- **`22`, `31` e `19` parecem ser os serviços do diagnóstico UDS** (ler dado por identificador, rotina e leitura de falhas): o `alm` chama os bytes 3 e 4 de "rdid", os identificadores do `22` são os DIDs da unidade, e o `0252` e o `0A03` são uma rotina e um pedido de falha. `[DEDUÇÃO]` O que `83` e `60` querem dizer eu não sei.
- **O que o microcontrolador de verdade manda não foi visto.** O que está acima é o que o `alm` exige. Que o microcontrolador mande os conjuntos da partida exatamente assim é dedução; captura de um carro não existe.
- **A função não confere o tamanho da mensagem.** Ela copia a mensagem inteira para uma estrutura de tamanho fixo, e o `StubLoader` faz o mesmo adiante. Os tamanhos que a bancada usa (os da tabela da HMI) cabem em todas, quase sem sobra: 9 bytes de mensagem para 16 de espaço no `0558`, 30 para 32 no `0600`, 261 para 264 no `0B1D`, 7 para 8 no `09CB`. Uma mensagem maior que o conjunto de verdade estouraria a pilha do `alm`. Por isso a ferramenta de teste não inventa tamanho: repete o conjunto como veio.

### 2. Os campos, e de que DID vem cada um

Posições lidas em `procDiagMsg` e nas funções do `StubLoader` (`libStubLoader.so`, md5 `87afc43540084ecbeed796f43c976da4`: `sendConfigNaviMsg`, `setCarCoding`, `setMapSwitch`, `setVze`, `setCountryOfSale`, `sendGnssSysMsg`, `sendRollmodeData`). A coluna da HMI vem de `stubs/codificacao/tabela-campos.tsv` (Marco 6). "Byte" é o byte dos dados do conjunto, como nos arquivos de codificação; bit 0 é o menos significativo.

| Campo | DID | Byte e bit | Nome no `alm` | Nome na tabela da HMI | O que o motor recebe | Situação |
|---|---|---|---|---|---|---|
| **região da navegação** | `0558` | byte 0 inteiro | `country` | `ID_COUNTRY_NAVIGATION`, "Ländervariante Funktion Navigation" | `setCountryCoding(n)`; acima de 18 vira 0 | `[EXEC]` com 0 e com 3 |
| **função de navegação** | `0558` | byte 1, bit 7 | `nav` | `ID_NAVIGATION_SYSTEM`, "Funktion Navigation" | `setNavEnabled`; é o `uds` da linha "R4N" | `[EXEC]` |
| estilo de mapa | `0558` | byte 2 inteiro | `maps` | `ID_MAP_STYLE` | `setMapStyle` | `[ESTÁTICO]`; 0 nas rodadas |
| trânsito | `0558` | byte 3, bit 7 | `traffic` | não está na tabela | `setTrafficEnabled` | `[ESTÁTICO]`; 0 nas rodadas |
| dados preditivos (PSD) | `0558` | byte 1, bit 3 | `psd` | não está na tabela | `setPsdEnabled` | `[ESTÁTICO]`; 0 nas rodadas |
| LGI, TPEG, ARD | `0558` | byte 1, bits 0, 1 e 2 | `LGI`, `TPEG`, `ARD` | `ID_LGI`, `ID_TPEG_AVAILABILITY`, `ID_ADVANCED_RANGE_DISPLAY` | `setLgiEnabled`, `setTpegEnabled`, `setArdEnabled` | `[ESTÁTICO]` |
| avisos de placas | `0558` | byte 1, bits 4 e 5 | `vzo`, `vza` | `ID_VZO`, `ID_VZA` | `setVzoEnabled`, `setVzaEnabled` | `[ESTÁTICO]` |
| antena de GNSS | `0558` | byte 1, bit 6 | `gaa` | não está na tabela | `setGnssAntennaAvail` | `[ESTÁTICO]` |
| troca de mapa | `0B5A` | byte 7, bit 1 | `Map Switch` | `ID_ABILITY_SWITCH_NAV_MAPS` | `setSwitchMapEnabled` | `[EXEC]`: byte `02` virou "true" |
| tipo de veículo | `0600` | byte 0: marca (bits 4 a 7) e classe (0 a 3); byte 1: geração (4 a 7) e carroceria (0 a 3) | `CarCoding` | `ID_BRAND`, `ID_CAR_CLASS`, `ID_CAR_GENERATION`, `ID_CAR_BODYSTYLE` | `setCarType` | `[EXEC]`: `23 71` virou marca 2, classe 3, geração 7, carroceria 1 |
| tipo de tração | `0600` | byte 3, bits 4 a 7 | `Fuel Type` | `ID_CAR_TYPE_OF_DRIVETRAIN` | `setFuelTypeCoding` | `[ESTÁTICO]`; 0 nas rodadas |
| país de venda | `09CB` | bytes 0 e 1 | `Country of Sale` | `ID_MEDIA_COUNTRY_CODE` | `setCountryOfSale`, quando o motor pedir | `[EXEC]` só até o `alm`, que guardou `30 78`; o motor não pediu (seção 4) |
| reconhecimento de placas | `0B1D` | byte 33, bits 6 e 7 | `Vze Activation` | `ID_TRAFFIC_SIGN_RECOGNITION_0X21` | `setVzeEnabled` | `[EXEC]` com 0 |
| receptor de GNSS | `0E38` | byte 0, bits 0 a 7: `beidou`, `exgps1`, `exgps2`, `compas`, `glonas`, `galile`, `gps`, `hwrecp`; byte 1, bits 4 a 7: `gnrate` | os da lista | bit 2 `ID_EXTERNAL_GPS_1`, bit 3 `ID_COMPASS`, bit 4 `ID_GLONASS` | `setGnssReceiverCoding` | `[ESTÁTICO]`; zeros nas rodadas |
| PSO e IAA | `0571` | byte 0, bits 7 e 6 | `pso`, `iaa` | `ID_PSO_FUNCTION`, `ID_IAA_FUNCTION` | fica na persistência do `alm` | `[EXEC]`: `C0` virou 1 e 1 |
| modo de teste | `243F` | byte 0 | `DeveloperTestMode` | `ID_DEVELOPER_TESTMODE_ACTIVATED` | não vai ao motor | `[EXEC]` com 0 |
| modo de rolo | `04FB` | byte 0 igual a 1 | `rollmode` | não está na tabela | `setKonzernrollenmodusEnabled` | `[ESTÁTICO]`; a bancada não publica |

**O "país" é uma região.** O byte 0 do `0558` não é um código de país: é a variante de região da função de navegação. Os nomes, pela tabela da HMI: 0 nenhuma, 1 EU, 2 NAR, 3 MSA, 4 Korea, 5 China, 6 Japan, 7 AsiaPacific, 8 Australia, 9 South Afrika, 10 NEAST, 11 NMAfrica, 12 MEAST, 13 MSA 2, 14 India, 15 Israel, 16 Taiwan, 17 China SVW, 18 China FAW-VW. O motor tem a própria lista (um enum `CountryCoding` embutido no executável), igual até o 15; do 16 ao 18 ele diz Chile, Russia e Serbia. `[ESTÁTICO]` Só o 0 e o 3 foram exercitados.

**Como o valor chega ao motor.** `[EXEC]` e `[ESTÁTICO]` O motor registra um serviço "Diagnosis" no barramento D-Bus da navegação, e o `alm` é cliente dele. Para cada função o motor pede o valor por difusão ("Get CountryCoding", "Get Navigation System Enabled"…), e o `alm` responde com o que tem guardado do último conjunto que tratou. Como ele descartava todos, respondia zero: região 0 e navegação desligada. A região o motor pedia de novo a cada 0,2 s (720 pedidos no controle) e a cada resposta registrava "unknown CountryCoding". Se o `0558` chega depois de o motor já ter perguntado, o `alm` manda sozinho os campos que mudaram (lido, não exercitado: na bancada os conjuntos chegam ao `alm` 8 s antes de o motor subir).

### 3. O que a bancada manda e o que o `alm` usa

- O `diagstub` publica **48 conjuntos**, todos com os três bytes zerados. O `alm` descarta os 48. `[EXEC]`, `r2b`
- Dos 48, o `alm` usa **8**: `0600`, `0B5A`, `0558`, `0B1D`, `09CB`, `243F`, `0E38` e `0571`. O nono que ele conhece, `04FB`, não está em `padrao.txt` nem na tabela da HMI.
- Os outros 40 o `alm` não trata de jeito nenhum: com o cabeçalho certo continuam "Unknown", só que por identificador desconhecido. `[EXEC]`, `r5`
- Os identificadores e os tamanhos da bancada já são os que o `alm` espera. Nada muda nos dados.
- Nos arquivos de hoje o `0558` sai `00 80 00 00`: função de navegação ligada (palpite que já estava em `jetta-gli-2021-deducoes.txt`) e região 0. A região do Jetta não é conhecida: é o campo "Ländervariante Funktion Navigation" da adaptação do 5F, a ler do carro.

### 4. As rodadas

**A ferramenta de teste.** [driver/vc_diag_alm.c](driver/vc_diag_alm.c), nova. Liga-se ao barramento como leitor, espera o `diagstub` publicar e manda de novo os conjuntos com o cabeçalho `83 60 22` e **os mesmos dados**. O hub guarda um conjunto por identificador, então a versão nova toma o lugar da antiga para quem se liga depois (os daemons, o `alm`, a HMI). Sem opção ela troca só os 8 que o `alm` usa; com `-a`, os 48. Não muda dado nenhum: valor de teste entra pelos arquivos de codificação do `diagstub`, como sempre. Teste sem firmware em [driver/tests/t_vc_diag_alm.sh](driver/tests/t_vc_diag_alm.sh): escrito antes do programa, falhou em 24 de 34 casos e hoje passam os 34. O teste do Marco 6 (`t_codificacao.sh`) continua em 26 de 26.

**Como entrou nas rodadas.** `driver/vc_rodada.sh` e `driver/vc_rodada_container.sh` ganharam a opção `DIAGALM` (`1` ou `todos`); sem ela rodam como na `r1` e na `r2`. O resto é igual à `r2`: 150 s, compositor de mentira, tecla NAV aos 80 s, fotos aos 70, 100 e 140 s.

**Os dados.** Os mesmos quatro arquivos da `r2`, na cópia que ela guardou às 20h40. O arquivo de deduções do Jetta ganhou linhas depois (campos do `0B1B` e do `0B1D`, do Marco 8), e a bancada também mudou (`dsistub.c`, `veiculostub.c`); por isso há um controle novo, a `r2b`, e as comparações são com ele. Os 48 conjuntos, como foram publicados, estão em `traces/vc_diag_base.txt` (conferido: publica os mesmos bytes). O valor de teste da região está em `traces/vc_nav_regiao_teste.txt`: `0558 @0.0+8 = 3`. **São valores padrão, palpites e valores de teste; nenhum foi lido do carro.**

As previsões foram escritas antes de rodar, em `traces/vc_diag_previsoes.md`. Rodadas entre 21h40 e 21h47 de 11/10/2026, uma de cada.

| Rodada | O que mudou | Previsão | Observado |
|---|---|---|---|
| `r2b` | nada: a `r2` de novo, na bancada de hoje | igual à `r2` | igual: 48 "Unknown" no `alm`; `setNavEnabled(0)`; `setCountryCoding(0)` 720 vezes; no motor, 721 "unknown CountryCoding", "uds = Disabled, fec = Undefined", `ready4navSwap`. A `r2` se repete |
| `r3` | `DIAGALM=1`: os 8 conjuntos também com `83 60 22` | "Unknown" cai para 40; oito tipos reconhecidos; `setNavEnabled(1)`; motor em "uds = Enabled"; região continua 0 e o motor continua reclamando; tipo de veículo e troca de mapa chegam; palpite de que o estado não muda; HMI igual | **tudo confirmado.** 40 "Unknown"; os oito conjuntos reconhecidos (nove nomes, porque o `0600` dá dois); `setNavEnabled(1)`; **"R4N : nav : uds = Enabled, fec = Undefined"**; 713 "unknown CountryCoding"; `setCarType : brand=2,class=3,generation=7,bodyStyle=1` (era 255 em tudo); `setSwitchMapEnabled : to value : true`; estado `ready4navSwap`. HMI: as 104 linhas de leitura de diagnóstico e as três fotos idênticas às do controle |
| `r4` | `r3` e a região de teste: byte 0 do `0558` = 3 (MSA) | `setCountryCoding(3)`; some o "unknown CountryCoding"; aparece o submercado, palpite `SAM`; o motor procura a raiz do banco e não acha; estado igual ao da `r3` | **confirmado.** O motor pediu a região uma vez e recebeu 3: "Received market coding: 3", "Country coding info is available now", **"Setting countryOfSale configuration to the value from coding: BRA"**, **"Determined submarket path as SAM"**, `baseCountry: BR`; nenhum "unknown CountryCoding". Depois: **"DB root file does not exist: /lge/app_ro/navi/naviengine/navicore/var/lib/nav/map/SAM/ROOT.NDS"**. Estado `ready4navSwap`, linha "R4N" igual à da `r3`. Fotos iguais |
| `r5` | os dados da `r4`, com `DIAGALM=todos`: os 48 conjuntos com `83 60 22` | HMI idêntica à `r4`; os 40 "Unknown" mudam de linha; motor igual | **confirmado.** HMI: as 104 linhas e as três fotos idênticas às da `r4`. `alm`: 40 "Unknown" nas linhas 368 e 369, com "MainCmd(0x83) subCmd1(0x60) subCmd2(0x22)". Motor: as mesmas linhas da `r4`, `SAM` e o mesmo caminho |

Em todas as rodadas (estas quatro e as oito repetições de baixo) ficaram de pé até o fim o `alm`, o motor, o `rsi-display-management`, o `navsd_app`, a HMI e os daemons da bancada (inclusive o `persistence-app`); o `multi-screen-server` sai logo no começo, como na `r2`. Nenhuma queda. O tráfego do barramento tem os mesmos canais no controle e na `r3`; mudam os 8 conjuntos a mais e a contagem das mensagens periódicas que o próprio `alm` publica (5.761 e 5.726).

**Repetições (das 21h57 às 22h11, depois das quatro rodadas).** Outra sessão registrou no mesmo dia que duas partidas iguais desta bancada podem dar telas diferentes, e que causa só se atribui com repetição. Por isso repeti, com os dados da `r4`: a `r5` mais duas vezes (`r5b`, `r5c`) e três pares simultâneos, um sem a ferramenta (`a1` a `a3`, os 48 conjuntos com `00 00 00`) e um com `DIAGALM=todos` (`b1` a `b3`). A tabela das 12 rodadas está em `traces/vc_diag_repeticoes.txt`.

| O que se olhou | Sem o cabeçalho novo (`r2b`, `a1`, `a2`, `a3`) | Com ele (`r3`, `r4`, `r5`, `r5b`, `r5c`, `b1`, `b2`, `b3`) |
|---|---|---|
| "Unknown" no `alm` | 48 nas 4 | 40 nas 8 |
| linha "R4N : nav" do motor | "uds = Disabled" nas 4 | "uds = Enabled" nas 8 |
| região | o motor pediu de 662 a 1.277 vezes e recebeu 0 | com região 0 (`r3`): 713 pedidos; com região 3 (as outras 7): 1 pedido, resposta 3, `SAM` e o mesmo caminho |
| estado do `navsystem` | `ready4navSwap` nas 4 | `ready4navSwap` nas 8 |
| leituras de diagnóstico da HMI (104 linhas) | iguais entre si para os mesmos dados | **as mesmas das rodadas sem o cabeçalho**, para os mesmos dados |
| telas aos 70, 100 e 140 s | iguais nas 4 | iguais em 7; **a `r5c` diverge** |

**A `r5c` divergiu, e não foi pelo diagnóstico.** Nela a HMI registrou "Required DSI services not registered within 2000 ms: org.dsi.ifc.keypanel.DSIKeyPanel[0]", não configurou o painel de teclas (nenhum dos 8 ajustes que manda nas outras rodadas) e ignorou a tecla NAV: as fotos de 100 e 140 s ficaram iguais à de 70 s. O que mudou na partida: o `touch-app` do firmware, que é quem anuncia o serviço de teclas (canal `0x80000000000`, interface 9000), ligou-se ao barramento por último, depois dos outros três daemons e até do `alm`. O anúncio dele foi a 90ª mensagem do barramento fora do canal de diagnóstico; nas outras 11 rodadas é a 4ª, a 5ª ou a 6ª. O hub só guarda as 48 primeiras para entregar a quem se liga depois, e as mensagens do canal `0x10` sozinhas ocupam mais de 40 desses lugares logo na partida; então na `r5c` a HMI não recebeu o anúncio. `[EXEC]` para tudo isso. O `touch-app` se atrasou antes de se ligar ao barramento, isto é, antes de receber qualquer conjunto; os conjuntos de diagnóstico ficam num guardado à parte no hub, e a ferramenta termina antes de o primeiro daemon se ligar. Não vejo por onde o cabeçalho causaria o atraso. `[DEDUÇÃO]` Em números: 1 partida divergente em 12, e ela caiu no braço do cabeçalho novo (1 em 6 com os 48; 0 em 6 nas outras). Doze rodadas não bastam para descartar por contagem; o que sustenta a leitura é o mecanismo. Havia de 3 a 4 containers de outras sessões no ar na hora, além dos meus dois. É uma corrida da bancada (os quatro daemons sobem juntos e a fila de anúncios do hub tem 48 lugares), fora deste item: fica como pendência para quem cuida do lançador e do hub. **Resolvido no hub às 22h52 de 11/10/2026** (nota da sessão do hub): ele passou a guardar o último anúncio de cada canal e a entregá-lo a quem se liga depois. Em partidas em que o `touch-app` voltou a se ligar depois da rajada do canal `0x10` (`dv1` e `dv6`, anúncio na 80ª e na 86ª mensagem), a HMI recebeu o anúncio e as teclas funcionaram; com o hub de antes, na mesma hora, uma partida minha repetiu a falha da `r5c` e terminou de tela preta. A corrida em si continua existindo. Detalhes em [modelagem-soc.md](modelagem-soc.md), "O hub e os anúncios que ficavam fora do cache" (decisão D66). As leituras de diagnóstico da `r5c` são as mesmas das outras.

**Duas coisas que a previsão não trazia:**

- **O motor tira o país da região, sozinho.** Com região 3 ele mesmo decidiu `BRA` e o submercado `SAM`. O "Country of Sale" do `09CB` não entrou nisso: o `alm` o guardou (`30 78`, o valor padrão da bancada), mas o motor não o pediu em nenhuma rodada. Que MSA cobria o Brasil era dedução pelo nome; agora foi visto. `[EXEC]`
- **Com a região o motor vai mais longe.** Nas 7 rodadas com a região 3 ele registrou mais dois serviços RSI além do `navsystem`: `places` e `phonemecontentnetwork`. `maprenderer` e os outros que a HMI espera continuam ausentes. `[EXEC]`

**A pasta de mapas que o motor procura** (registro e `strace` da `r4`). `[EXEC]` O caminho passa por um link do pacote: `…/navicore/var/lib/nav/map` aponta para `/var/lib/nav/nav` (lido na imagem). Então:

| O que | Caminho |
|---|---|
| raiz do banco | `/var/lib/nav/nav/SAM/ROOT.NDS` (procurou e não achou) |
| lista de pacotes do gerente de atualização | `/var/lib/nav/nav/SAM/package.db` |
| voz e teclado | `/var/lib/nav/nav/SAM/SDS` e `/var/lib/nav/nav/SAM/IME` |
| informação do banco | `/var/lib/nav/nav/dbinfo.txt` (procurou e não achou) |
| pasta temporária do gerente de atualização | `/var/lib/nav/tempUM` (criou ali `nds.dm` e `nds.nal`) |

Sem a raiz, o motor registra a falha de banco ausente, fecha o gerente de atualização e segue de pé. O post-init do firmware só arruma `EUROPE` e `NAR`; para `SAM` a pasta teria de vir pronta. Que as subpastas sejam as mesmas da Europa (`3D`, `FB`, `FTS`, `LC`, `SDS`, `WO`) é `[DEDUÇÃO]`. Um cuidado para o item 4: o motor tem uma rotina com a mensagem "Cleanup of foreign submarket folder" (`[ESTÁTICO]`; não saiu em nenhuma rodada). Mapa montado na bancada tem de ser só para leitura, como o `vc_rodada_container.sh` já faz.

### 5. Onde parei

Com `uds = Enabled` (as 8 rodadas com o cabeçalho novo) o motor continua em `ready4navSwap`, o `GET /navsystem/systems` devolve `"status":"ready4navSwap"` e a tecla NAV mostra "This function is unavailable." (nas 7 em que a tecla funcionou), igual ao controle. Na linha "R4N" o que segue indefinido é o `fec`. **Não investiguei o lado `fec`**: não li o código dele, não imitei o chip, não fabriquei resposta de ativação e não mexi na configuração do motor. Seguir adiante nisso é a decisão do item 2 da lista, que é do Lipe.

O que este item responde para aquela decisão: **a função de navegação pela codificação não basta para o motor sair de `ready4navSwap`.** O "talvez" do item 2 deixou de ser talvez.

### 6. O que não foi verificado

- **O que o microcontrolador de verdade manda.** O formato é o que o `alm` aceita, lido no código. Não há captura de um carro.
- **O significado de `83` e `60`**, e se o microcontrolador usa `22` em todos os conjuntos da partida.
- **Os campos que ficaram em zero**: `maps`, `traffic`, `psd`, LGI, TPEG, ARD, `vzo`, `vza`, `gaa`, os bits do `0E38` e o tipo de tração. A posição de cada um foi lida, não exercitada.
- **`04FB`, a rotina `0252` e o pedido `0A03`**: lidos, não enviados.
- **Outras regiões.** Só 0 e 3. Que a 13 (MSA 2) ou a 16 (Chile, para o motor) levem a outro submercado não foi visto.
- **O país de venda do `09CB`.** O motor não o pediu; não sei quando pede nem o formato que espera. O valor padrão da bancada é `30 78`.
- **A HMI fora das três telas fotografadas.** A comparação foi nas 104 linhas de leitura de diagnóstico e nas fotos aos 70, 100 e 140 s. O que o `persistence-app` respondeu não foi comparado mensagem por mensagem; ele ficou de pé e o número de leituras sem dado da HMI foi o mesmo (5).
- **Repetição.** A `r3` (cabeçalho novo com região 0) foi feita uma vez só. O resto tem repetição: 4 rodadas sem o cabeçalho novo, 7 com ele e a região 3. São 12 rodadas ao todo, o que é pouco para afirmar por contagem que o cabeçalho não mexe na partida da HMI: uma delas (`r5c`) divergiu, e a explicação que dou é pelo mecanismo (seção 4).
- **A HMI depois de uma partida longa.** Nada foi olhado além de 150 s.
- **A região do Jetta.** Não foi lida do carro. O 3 é valor de teste.
- **Só o MU0883**, pacote `eu/single`. O carro roda o MU0803.
- **O modo ao vivo.** A ferramenta só rodou nas rodadas `vc_`; a central ao vivo segue com o cabeçalho zerado.

### 7. Nota para o Marco 6: o que mudaria no `diagstub`

Não alterei `driver/diagstub.c`, `driver/kipc_hub.c` nem os arquivos de `stubs/codificacao/`. O que as rodadas sustentam:

1. **Uma linha.** Em `main()`, onde hoje está `b[8] = b[9] = b[10] = 0;`, passar a `b[8] = 0x83; b[9] = 0x60; b[10] = 0x22;`, para todos os conjuntos. Com isso o `alm` passa a tratar os 8 que conhece, e a HMI e o `persistence-app` seguem como estão: em 6 rodadas os 48 foram com o cabeçalho novo, e nas 6 a HMI registrou as mesmas 104 leituras das rodadas sem ele. Em 5 das 6 as telas também foram as mesmas; a sexta (`r5c`) divergiu por uma corrida de partida que explico na seção 4 e que não passa pelo diagnóstico. **Antes de trocar de vez, vale repetir algumas partidas olhando a tela**, porque 12 rodadas é pouco.
2. **O teste.** `driver/tests/t_codificacao.sh` compara a linha do `dsi_escuta` com `cab=00 00 00` (variável `DG`); passaria a `cab=83 60 22`.
3. **Os comentários.** O cabeçalho do `diagstub.c` e o item 5 de "Por onde os dados entram" em `modelagem-soc.md` dizem que os três bytes não são lidos por ninguém e que não se sabe o que o microcontrolador põe ali. O `alm` os lê e exige `83 60 22`; o que o microcontrolador manda continua sem ter sido visto.
4. **Nada muda no hub nem no formato dos arquivos.** O hub guarda por identificador (bytes 3 e 4 da carga), que não mudam.
5. **Tamanho.** Nunca mandar conjunto maior que o de verdade: o `alm` copia a mensagem inteira para estruturas de tamanho fixo. Os tamanhos de `padrao.txt` cabem.
6. **Depois da mudança**, o `vc_diag_alm` e a opção `DIAGALM` deixam de ser necessários; a ferramenta diz "mandados de novo 0" e sai.
7. **A decidir, não fiz:** se a região entra nos arquivos do Jetta. Hoje o `0558` tem a navegação ligada e a região 0, e com região 0 o motor fica pedindo o país cinco vezes por segundo. Uma linha `0558 @0.0+8 = 3` em `jetta-gli-2021-deducoes.txt` seria palpite (o carro é brasileiro; a região de verdade está na adaptação do 5F). A HMI também lê esse byte (`ID_COUNTRY_NAVIGATION`); nas três telas fotografadas nada mudou.
8. **Para olhar:** o padrão `30 78` de `09CB`, `0B48` e `0B49` é o texto "0x" em ASCII. Pode ser resto da geração do `padrao.txt`. Com o cabeçalho novo o `alm` guarda o do `09CB` como país de venda. **Respondido pela sessão do Marco 6 às 23h30 de 11/10/2026: não é resto da geração.** É o valor padrão que a tabela da própria HMI traz para esses três campos de bytes (`mConfiguration` da `AdaptationImpl`), o que ela devolve sem dado; o gerador só copia. A minha suspeita estava errada.

### 8. O que muda no texto de cima

| Onde | Estava | O que achei |
|---|---|---|
| investigação, resposta curta, item 4 | o motor espera a função de navegação do diagnóstico e o estado de ativação | a primeira agora chega (`uds = Enabled`) e o estado não muda |
| investigação, seção 1, "Os mapas" | o submercado sai do país da codificação | sai da **região** (byte 0 do `0558`), que o motor converte em país de venda e submercado: 3 deu `BRA` e `SAM` |
| investigação, seção 3, o `navi-al` | ele lê os três primeiros bytes, que a bancada manda zerados | os valores que ele aceita são `83 60` e `22`, `31` ou `19` |
| investigação, seção 5, item 2 | só depois do item 3 dá para saber se o `uds` basta | não basta |
| investigação, seção 5, item 3 | descobrir os três bytes e pôr no `diagstub` | descobertos e testados; falta a mudança no `diagstub`, que é do Marco 6 |
| investigação, seção 5, item 4 | o nome da pasta de submercado só aparece depois do item 3 | `SAM`: `/var/lib/nav/nav/SAM/ROOT.NDS` |

### 9. Como reproduzir, e o que entrou na pasta

```bash
# teste da ferramenta, sem firmware
docker run --rm --network none -v "$PWD/driver":/driver:ro moi3-bancada:1 sh -c \
  'sh /driver/m1_build.sh /work/bin >/dev/null && gcc -static -O2 -Wall -o /work/bin/vc_diag_alm /driver/vc_diag_alm.c && sh /driver/tests/t_vc_diag_alm.sh /work/bin'

# as quatro rodadas (os arquivos vão um a um: no zsh uma variável com a lista não é separada)
WLSTUB="800 480 174 104"                 sh driver/vc_rodada.sh r2b /tmp/vc traces/vc_diag_base.txt
WLSTUB="800 480 174 104" DIAGALM=1       sh driver/vc_rodada.sh r3  /tmp/vc traces/vc_diag_base.txt
WLSTUB="800 480 174 104" DIAGALM=1       sh driver/vc_rodada.sh r4  /tmp/vc traces/vc_diag_base.txt traces/vc_nav_regiao_teste.txt
WLSTUB="800 480 174 104" DIAGALM=todos   sh driver/vc_rodada.sh r5  /tmp/vc traces/vc_diag_base.txt traces/vc_nav_regiao_teste.txt
sh driver/vc_diag_resumo.sh /tmp/vc r2b r3 r4 r5
```

**Estas linhas valeram até as 23h26 de 11/10/2026.** Depois disso o `diagstub` manda `83 60 22` sozinho (D77) e o hub é outro (D66): a primeira linha deixou de ser um controle, `DIAGALM` não muda mais nada, e as quatro dão hoje o resultado da `r5` (ou o da `r3`, sem o arquivo da região). Para refazer o controle com os bytes zerados é preciso o `diagstub` de antes (`traces/m6_cab.diagstub.antes.c`) ou a opção `-k 000000` dele, que o lançador não repassa; a sessão do Marco 6 fez o dela com uma cópia de `driver/`. O que vale hoje, para ver o `alm` e o motor recebendo os conjuntos, é a linha da `r6`:

```bash
WLSTUB="800 480 174 104" sh driver/vc_rodada.sh r6 /tmp/vc traces/vc_diag_base.txt traces/vc_nav_regiao_teste.txt
sh driver/vc_diag_resumo.sh /tmp/vc r6
```

As quatro rodadas originais usaram os quatro arquivos da `r2` em vez do `vc_diag_base.txt`; os bytes publicados são os mesmos. As oito repetições já rodaram por estas linhas: `r5b` e `r5c` são a da `r5` com outra etiqueta; `a1` a `a3` e `b1` a `b3` são a da `r5` sem e com `DIAGALM=todos`, um par de cada vez, os dois ao mesmo tempo.

Arquivos novos: `driver/vc_diag_alm.c`, `driver/tests/t_vc_diag_alm.sh`, `driver/vc_diag_resumo.sh` (resumo de uma rodada: cabeçalhos no barramento, tipos reconhecidos pelo `alm`, o que foi ao motor e o que o motor disse de região e mapas). Alterados: `driver/vc_rodada.sh` e `driver/vc_rodada_container.sh`, que ganharam a opção `DIAGALM` (sem ela rodam como antes). Em `traces/` (31 MB): `vc_diag_previsoes.md`, `vc_diag_base.txt`, `vc_nav_regiao_teste.txt`, `vc_diag_resumo.txt`, `vc_diag_repeticoes.txt` (a tabela das 12 rodadas, mais a `r6` e a `r6b`), `vc_diag_fotos_md5.txt` e, das rodadas `r2b`, `r3`, `r4`, `r5`, `r5c` e `r6`, `vc_<tag>.alm.out`, `.alm.strace`, `.navi.out`, `.navi.strace`, `.barramento.log`, `.diagalm.log`, `.diagstub.log`, `.hub.log`, `.dmstub.log`, `.rsi.txt`, `.vivos.txt`, `.roda.txt`, `.vc.log`, `.wlstub.log` e `.out`. Das outras sete repetições ficaram só os registros pequenos (`.hub.log`, `.dmstub.log`, `.rsi.txt`, `.vivos.txt`, `.roda.txt`, `.diagalm.log`); os números delas estão na tabela. Nenhum arquivo de outra sessão foi alterado.

**Pendência que não é deste item**, para quem cuida do lançador e do hub: a corrida da `r5c`. Os quatro daemons do firmware sobem ao mesmo tempo, e o hub guarda só os 48 primeiros anúncios para quem chega depois. Quando o `touch-app` se liga depois da rajada do canal `0x10`, o anúncio do serviço de teclas fica de fora, a HMI não acha o `DSIKeyPanel` e as teclas param de funcionar naquela partida. Aconteceu em 1 de 12 rodadas, com outros containers no ar. Não mexi.

**Atualização das 22h45 de 11/10/2026.** Essa corrida já está com outra sessão, que cuida do hub (nota preliminar `hub-anuncios.md`, na pasta de notas da sessão principal; o registro oficial dela vai para `modelagem-soc.md`, decisão D66). Ela chegou ao mesmo mecanismo por outro caminho e viu o defeito em 4 de 8 partidas sem o carro de mentira, com seis ou sete centrais no ar. Duas consequências para este item: (1) pelo critério dela, a `r5c` é esse defeito e não serve de evidência de outra coisa, o que bate com a leitura da seção 4; (2) o conserto muda a partida de toda rodada que sobe o `carifservice` de verdade, inclusive as do `vc_rodada.sh`. Depois dele, qualquer comparação pede um controle novo: os números de partida das rodadas daqui (serviços ausentes, esperas) deixam de servir de régua. O que foi visto no `alm` e no motor não depende disso. Às 22h43 o `diagstub` continuava mandando os três bytes zerados e o hub da pasta ainda era o antigo.

**Decisão do Lipe, 22h46 de 11/10/2026:** "Pode passar o pedido quando o hub entrar". O pedido é o da seção 7 (a linha do `diagstub`), e vai para a sessão do Marco 6, dona do arquivo, depois que o hub novo estiver na pasta. A ordem é essa porque a conferência da troca precisa de um controle rodado com o hub novo. A sessão deste item ficou de vigia no `driver/kipc_hub.c` e na nota da sessão do hub; se ela não estiver mais no ar quando o hub entrar, o pedido é o que está escrito na seção 7.

**Pedido passado às 22h56 de 11/10/2026.** O hub novo entrou às 22h52 (decisão D66 em `modelagem-soc.md`; o arquivo de antes ficou em `traces/hub_kipc_hub.antes.c`). A parte do hub que guarda os conjuntos de diagnóstico não mudou. Antes de passar o pedido rodei, com o hub novo e o `diagstub` ainda como estava: `t_codificacao.sh` (26 de 26), `t_vc_diag_alm.sh` (34 de 34) e `t_hub.sh` (25 de 25). O pedido foi entregue à sessão do Marco 6 com a frase do Lipe, a linha a trocar, o teste e os comentários que mudam junto, e o aviso de que a conferência pede controle rodado no hub novo. A troca em si é dela; até ela registrar, o `diagstub` segue com os três bytes zerados.

**Trocado às 23h25 de 11/10/2026, pela sessão do Marco 6.** O `driver/diagstub.c` passou a mandar `83 60 22` em todos os conjuntos, e ganhou a opção `-k <cabeçalho>` (3 bytes em hexa colado; `-k 000000` é o de antes, para rodada de controle). O `driver/tests/t_codificacao.sh` dela foi ajustado e cresceu para 39 casos. O que conferi logo depois, num container descartável: `t_codificacao.sh` 39 de 39 e `t_hub.sh` 25 de 25. O meu teste (`t_vc_diag_alm.sh`) caiu em 10 de 34 casos, como era de esperar, porque pressupunha o cabeçalho zerado; ajustei-o para pedir `-k 000000` onde precisa do cabeçalho antigo e acrescentei três casos que conferem que, com o `diagstub` de hoje, a ferramenta diz "mandados de novo 0". Agora passam os 37. **A partir daqui `vc_diag_alm` e `DIAGALM` não têm mais função**, e os comandos da seção 9 mudam de sentido: sem `-k 000000` não existe mais a rodada de controle com os bytes zerados, e o lançador ainda não repassa essa opção. O registro dela está em `modelagem-soc.md`, subseção "O cabeçalho dos conjuntos: `83 60 22`" e decisão D77. Lá está também a conferência que ela fez antes de trocar (rodadas `m6_cab0` e `m6_cab1`, lado a lado no hub novo, sem o `navi-al`): a HMI leu as mesmas 104 linhas, o `persistence-app` deu as mesmas respostas e as cinco fotos saíram idênticas.

**Conferência final, 23h31 a 23h34 de 11/10/2026: o `alm` aceita os conjuntos vindos direto do `diagstub` novo.** `[EXEC]` Era o que a sessão do Marco 6 não tinha conferido e deixou comigo. Duas rodadas iguais e simultâneas (`r6` e `r6b`), com os dados da `r4`, **sem a ferramenta e sem `DIAGALM`**, já no hub novo. Previsão escrita antes, em `traces/vc_diag_previsoes.md`; tudo confirmado nas duas:

- o lançador diz "enviados 48 conjuntos … cabeçalho 83 60 22", e no barramento os 48 saem uma vez só, todos com `cab=83 60 22`;
- `alm`: os 8 conjuntos reconhecidos, 40 "Unknown" na linha 368, "country : 3" e "nav : 0x1";
- motor: `setNavEnabled(1)`, `setCountryCoding(3)` uma vez, "R4N : nav : uds = Enabled, fec = Undefined", país de venda `BRA`, submercado `SAM`, a mesma raiz de banco procurada e não achada, os serviços `navsystem`, `places` e `phonemecontentnetwork`, estado `ready4navSwap`;
- HMI: as mesmas 104 leituras de diagnóstico da `r4`, as três fotos idênticas às dela (inclusive a de "This function is unavailable."), os 8 ajustes do painel de teclas e a tecla NAV com efeito nas duas.

Com isso o item 3 da lista fica fechado na bancada: o motor recebe a região e a função de navegação pelo caminho normal, sem ferramenta à parte. O que não muda: ele continua em `ready4navSwap` (seção 5).

## Levantamento de projetos públicos de Waze/Maps no painel (13/10/2026)

### Varredura nova — MIB3 + CarPlay no GitHub (18/10/2026)

Busca de repositórios no GitHub (pedido do Lipe). **Para MIB3/MOI3 (LG, Linux — a plataforma do Jetta): NADA
público sobre CarPlay no cluster, exceto os repos do próprio Lipe** (`lipedesign/mib3-moi3-lg-research` e
`lipedesign/moi3-cluster-pipeline-re` — os únicos resultados de "MIB3 cluster"). `MIB3 CarPlay`, `MIB3 AID
navigation`, `MIB3 Android Auto cluster`, `MHI3 carplay` = **0**. Outros MIB3 no GitHub são tools de
**aparência/dados/ativação**: `jilleb/mib3-toolbox`, `n3oka/MIB3-Image-Resolver` + `MIB3-3D-Viewer`,
`pschlan/vw-mib3-map-validator`, `SERGIOCORVALIS/activator_mib3-shcoda` (ativação FEC). Nenhum de CarPlay-no-cluster.

**Cena ATIVA e aberta existe — mas só em MHI2/MHI2Q (Harman/QNX/Tegra, Audi/VW/Skoda), OUTRA central, não a LG
MOI3.** Repos de CarPlay **AltScreen tipo 111** no Virtual Cockpit: `yuedizhibo/MHI2Q-CarPlay-AltScreen` (419★),
`omonob/MHI2-Carplay-Maps` (143★; já citado na tabela abaixo), `harman-f/mhi2_altscreen_carplay` (133★),
`kamgurgul/mib2q-carplay`, `Lanye-z/MHI2Q-CarPlay-MMI-Mirror` (361★), `aberbic/Audi-MHI2-...` (Tegra MU1438),
`stevenzhr/MU1320-CarPlay-RGI` (turn-by-turn + HUD), `eupn/audi_tt_8s_mu1339_altscreen111_carplay`.

**Conclusão:** o conceito AltScreen-111 está **aberto e maduro no Harman**; na **MOI3 LG não há nada público** —
por isso o lado on-car é fronteira (James em privado; Gemini/Lipe construindo). Plataformas diferentes
(QNX/Tegra/ARMv7 × Linux/MSM8996/aarch64): **não é drop-in**, serve de referência de arquitetura, não código que
roda na central do Jetta. `[WEB]`

### Atualização da varredura (20/10/2026) — surgiu um repo MOI3 LG

Ordenando por atualização recente, a cena está **muito ativa** e apareceu um item **direto da plataforma do
Jetta** (que não existia em 18/10):
- **`githubAll3/moi3_LG_vc`** (atualizado ~15 h atrás, 0★): "CarPlay AltScreen altshim.c:278-290, 374-382 on the
  AID (virtual cockpit)". **Primeiro repo público de CarPlay-no-cluster para MOI3 LG** além dos do próprio Lipe —
  a plataforma exata do carro. É o lado on-car (shim/injeção do AltScreen); **não aberto nem analisado por mim**.
- Novos em MIB2/MHI2 (últimas 24 h): `1mikew/mst2-altscreen-carplay` (MIB2 Standard/MST2, QNX, WIP),
  `OMKE/wireless-carplay-virtual-cockpit` (MHI2Q via dongle CarPlay sem fio).
- Harman/MHI2 segue fervendo: `yuedizhibo/MHI2Q-CarPlay-AltScreen` (435★), `omonob/MHI2-Carplay-Maps` (154★),
  `kamgurgul/mib2q-carplay` (77★), `harman-f/mhi2_altscreen_carplay` (138★) — vários atualizados em horas.

**Leitura:** até 18/10 a MOI3 LG não tinha nada público; em 2 dias surgiu o `moi3_LG_vc`. A fronteira se move
rápido — vale o Lipe/Gemini olharem esse repo (é a plataforma deles). Do meu lado, catalogo a existência; não
abro/analiso o payload on-car. `[WEB]`

A pedido do Lipe ("veja se não tem coisas novas sobre o Waze / Maps na MIB3"), uma varredura na web e na API do GitHub em 13/10/2026. Os sete repositórios abaixo foram confirmados por `api.github.com/repos/<dono/nome>` nesta data (existem, não estão arquivados; data do último push e estrelas anotadas). **Nenhum é da plataforma do Jetta.**

**O achado que importa — e por que não é o nosso carro.** O [harman-f/mhi2_altscreen_carplay](https://github.com/harman-f/mhi2_altscreen_carplay) (126 estrelas; push de 13/10/2026) provou, em 29/09/2026, **Waze, Google Maps e Apple Maps rodando como vídeo no Virtual Cockpit** de um Škoda de referência. É, muito provavelmente, o que o grupo de Telegram "VcMostRenderMQB" acompanha (o nome do grupo cita o *MOST render*). Mas o caminho dele é **CarPlay Auxiliary/ScreenAlt → H.264 → MPEG-TS → MOST150 → Virtual Cockpit**, na plataforma **MHI2** (fornecedor Harman, sistema QNX, SoC Tegra, firmware `MHI2_ER_SKG13_P4526_MU1440`). O Jetta GLI 2021 é **MIB3 LG (MOI3)**: outro fornecedor (LG, não Harman), outro sistema (Linux, não QNX) e, o ponto decisivo, o vídeo do painel sai por **rede IP (multicast `ff14::2:4`)**, não por MOST150 (ver seção 1 e seção 5 acima). O método do harman-f não atravessa para o nosso carro: não há MOST, não há QNX, não há Tegra, e a pilha de CarPlay é outra. O próprio README avisa que não é pacote de pendrive, que exige SSH já existente na unidade e que pode inutilizar a central se mal aplicado.

**Os outros seis, todos de outra plataforma:**

| Repositório | Estrelas | Último push | Plataforma | O que faz |
|---|---|---|---|---|
| [luka-dev/mib2q-carplay-rgi](https://github.com/luka-dev/mib2q-carplay-rgi) | 309 | 25/09/2026 | MHI2Q (Audi) | seta de manobra 3D sobre o mapa do painel, na navegação por CarPlay (Apple/Google/AMap, não Waze) |
| [OnSerious/mib2q-carplay](https://github.com/OnSerious/mib2q-carplay) | 0 | 13/10/2026 | MHI2Q (Audi) | CarPlay e Android Auto no Virtual Cockpit; no Android Auto mostra o vídeo do mapa (Maps/Waze) |
| [Mr-MIBonk/mib2q-carplay](https://github.com/Mr-MIBonk/mib2q-carplay) | 1 | 09/10/2026 | MHI2Q (Audi) | guia de rota do CarPlay no painel |
| [wasimlhr/mib2q-android-auto-cluster](https://github.com/wasimlhr/mib2q-android-auto-cluster) | 7 | 01/10/2026 | MIB2 (B9 SQ5, `MU0918`) | Android Auto no painel, com Maps e Waze |
| [fifthBro/mh2p-cluster](https://github.com/fifthBro/mh2p-cluster) | 82 | 21/07/2026 | Porsche PCM5 / MH2P | leva Maps e Waze (Android Auto e CarPlay) ao painel; em beta |
| [jilleb/mib2-toolbox](https://github.com/jilleb/mib2-toolbox) | 1011 | 24/08/2026 | MIB2-HIGH (QNX) | caixa de ferramentas da MIB2; o issue #95 ("What about MIB3?") não tem resultado |

**Para o MIB3 LG especificamente, segue "escuridão total" no mundo público.** As fontes de 2026 dizem que não há root, kit de mod nem firmware remendado público para nenhuma MIB3; o único utilitário público de "MIB3 challenge" no GitHub só gera a resposta do login de engenharia e não instala nada. O issue #95 da mib2-toolbox, sobre uma MIB3 LG, resume: "até agora, absolutamente nada, escuridão total".

**O que isso muda para nós:**
- Não há nada novo e aplicável ao Jetta para pôr Waze/Maps no painel. O que existe de novo e impressionante é da família MHI2/MIB2 (Harman/QNX) ou Porsche, não da LG.
- **Nenhum pacote desses repositórios pode ser instalado na central do Jetta:** são de outra plataforma, e aplicar um deles aqui arrisca inutilizar a central. (Mesmo aviso que já está na ficha para o [omonob/MHI2-Carplay-Maps](https://github.com/omonob/MHI2-Carplay-Maps).)
- Para o nosso estudo o valor é de método, não de pacote: o harman-f documenta o caminho "vídeo do CarPlay → codec → transporte → Virtual Cockpit" de ponta a ponta, que é a mesma ideia do nosso item 8, só que sobre MOST em vez do IP `ff14::2:4`. Continua sendo estudo num computador; nada vai para o carro, e a linha de navegação do simulador segue como estava.

### Segunda varredura, pela API de busca do GitHub (13/10/2026)

Busca por `MIB3` e `MOI3` ordenada pelos mais recentes (não só pela web). **Para Waze/Maps no painel, continua não havendo nada na nossa plataforma.** Mas apareceram repositórios novos específicos de MIB3 LG (a nossa), nenhum de navegação ou cluster — são de personalização e de perícia. Todos confirmados na API nesta data:

| Repositório | Estrelas | Último push | O que é |
|---|---|---|---|
| [jilleb/mib3-toolbox](https://github.com/jilleb/mib3-toolbox) | 61 | 24/08/2026 | caixa de ferramentas dedicada à MIB3 (separada da de MIB2). O README ainda está praticamente vazio (14 caracteres): é mais casca do que conteúdo por enquanto |
| [n3oka/MIB3-Boot-Screen-Simulator](https://github.com/n3oka/MIB3-Boot-Screen-Simulator) | 0 | 13/10/2026 | toca a animação de abertura da MOI3 (MQ2 LG, MQB LG) no PC, 1:1. Freeware, Windows |
| [n3oka/MIB3-3D-Viewer](https://github.com/n3oka/MIB3-3D-Viewer) | 4 | 07/08/2026 | editor e visualizador dos arquivos 3D (`.MCF`) da MIB3/MHI3 |
| [n3oka/MIB3-Image-Resolver](https://github.com/n3oka/MIB3-Image-Resolver) | 5 | 15/07/2026 | resolve e nomeia imagens dos dados da MIB3 |
| [nicolasnt3-a11y/MIB3tosaurus](https://github.com/nicolasnt3-a11y/MIB3tosaurus) | 1 | 15/09/2025 | perícia: extrai texto, JSON e coordenadas GPS de coredumps e logs de MIB3 OI e plota num mapa OpenStreetMap. Não é navegação do carro |
| [jugih-official/mib3-dl](https://github.com/jugih-official/mib3-dl) | 0 | 25/07/2026 | converte vídeo para MP4 que a MIB3/MOI3 toca |

**Fora do nosso caminho, por serem de ativação/FEC** (o limite de proteção que não perseguimos): [jiangbo2571/MH2P_MIB3_challenge-tools](https://github.com/jiangbo2571/MH2P_MIB3_challenge-tools) (8★) só gera a resposta do login de engenharia e não instala nada; e um "activator_mib3" apareceu na lista. Anoto que existem e paro aí.

**Conclusão das duas varreduras:** o ecossistema público de MIB3 LG está crescendo (caixa de ferramentas, simulador de boot, visualizador de 3D, parser de perícia), mas **nada toca Waze/Maps nem o painel**. O que existe para pôr mapa no Virtual Cockpit segue sendo MHI2/MIB2 (Harman/QNX/MOST) ou Porsche, plataformas que não são a do Jetta. Nada mudou a conclusão da seção anterior.

### Aprofundamento no jilleb/mib3-toolbox (13/10/2026)

Aberto por dentro pela API. É pequeno e específico: criado em 27/06/2021, licença MIT, 61★, 7 forks, um só ramo (`main`), **sem releases nem wiki**, 60 KB no total. O nome engana: não instala função nenhuma. É um **repositório de engenharia reversa de formatos de arquivo da HMI**, do mesmo autor ("Jille / MQBTools"). Tem quatro arquivos de conteúdo:

- **`Tools/extract-anim.py`** — descompacta os arquivos de imagem de abertura da MIB3 (`startScript_x.anim`): lê o cabeçalho, a lista de deslocamentos e extrai cada quadro (zlib, BGRA → PNG, com a correção de R/B). É o que lê a animação de boot do nosso próprio firmware; combina com o [n3oka/MIB3-Boot-Screen-Simulator](https://github.com/n3oka/MIB3-Boot-Screen-Simulator), que a toca.
- **`Research/010 Editor templates/gca_format.bt`** — template do 010 Editor para o `.gca` ("GenericCarAdjuster"): os layouts de botão, imagem e fundo dos widgets da HMI (clima, bancos, ajustes do carro). **A metade MIB3 é transcrição byte a byte da desserialização decompilada de `MIBHMI-STD-H42.325.89-bin.jar`** (classes `GenericCarAdjusterConfigData`, `GenericCarAdjusterConstants`, `AbstractConfigData`). É exatamente o método da nossa bancada (decompilar o jar da HMI e ler os leitores), só que num build mais novo que o nosso (H42 contra o H40.272.304 do MU0883; o carro é H40.272.151 do MU0803): o formato deve ser próximo, não garantido igual.
- **`mcf_format.bt`** — template do contêiner `.mcf` (21 tipos de bloco): `car_lhd.mcf` (cena 3D, 996 blocos) e `images.mcf` (gráficos 2D, 1643 imagens). Traz a estrutura dos blocos zlib e a fórmula do CRC32 conferida em 1007+ blocos. É onde moram o carro 3D e as imagens que a tela mostra — o sedã prateado e os bancos que vemos no simulador saem de arquivos desse tipo.
- **`ops_format.bt`** — template do `.ops` (Optical Parking System): a geometria do assistente de estacionamento (correção de espelhamento da câmera, faixas de distância, fundo da vista de cima, posições do carro/reboque). Tamanho fixo de 8244 bytes, tudo int16; magia `CAFE`, a mesma do `.gca`.

**O que serve para nós:** é material de estudo alinhado ao da bancada. O `extract-anim.py` lê a tela de boot do Jetta; os templates de `.mcf` identificam os assets 3D e 2D que a HMI desenha (inclusive os que o simulador já mostra); o `.ops` casa com o menu "Estacionar e manobrar". **O que não tem:** nada de Waze, Maps, navegação nem cluster — consistente com as duas varreduras acima. Nada aqui vai para o carro; é leitura de formato.


### O autor n3oka: ferramentas de MIB3 MOI3 LG e de datasets (13/10/2026)

A pedido do Lipe, uma olhada no perfil [n3oka](https://github.com/n3oka) (5 repositórios, todos de VW/MIB3, binários Windows em release; a maioria freeware mas de código fechado, "use por sua conta e risco"). **Nenhum é de navegação, Waze ou cluster**, mas vários são da nossa plataforma (MOI3 LG) ou do nosso projeto:

| Repositório | Release | O que faz |
|---|---|---|
| [A5-2Q0-5WA-Dataset-Editor](https://github.com/n3oka/A5-2Q0-5WA-Dataset-Editor) | v2.10 (15/07/2026) | edita datasets das câmeras frontais **2Q0 e 5WA** — a do projeto 01. Detalhe e ressalvas no [README do projeto 01](../../../carros/jetta-gli-2021/projetos/01-lane-assist/README.md), "Editor pronto da comunidade" |
| [MIB3-3D-Viewer](https://github.com/n3oka/MIB3-3D-Viewer) | v17.07.19 (07/08/2026) | abre e edita os arquivos 3D `.MCF` da MIB3; **já funciona com `.MCF` de unidade MOI3 LG e troca texturas** — a nossa unidade. É o carro 3D e os gráficos que a HMI (e o simulador) mostram |
| [MIB3-Image-Resolver](https://github.com/n3oka/MIB3-Image-Resolver) | v1.0.0 (15/07/2026) | lê as imagens dos dados da MIB3, descobre e atribui os nomes, liga imagem a veículo e a cena, e troca imagens de mesma resolução |
| [MIB3-Boot-Screen-Simulator](https://github.com/n3oka/MIB3-Boot-Screen-Simulator) | v1.0.1 (13/10/2026) | toca 1:1 a animação de abertura da MOI3 (MQ2 LG, MQB LG, GP), com as sequências tiradas do renderizador da unidade (`early-ethcam`, `early-cvbs`); troca imagens, gera série de logo e exporta GIF. Tem wiki embutida |
| [MQBevo-Dataset-Editor](https://github.com/n3oka/MQBevo-Dataset-Editor) | v1.01-beta (02/08/2026) | editor de datasets de veículos MQB evo (mais geral, não específico do nosso carro) |

**Para nós:** o editor da câmera 2Q0/5WA é o mais direto (projeto 01); os de `.MCF`, de imagens e de boot são de **customização gráfica da nossa própria unidade MOI3 LG** e servem de estudo dos mesmos assets que o simulador desenha. Tudo é binário fechado de fonte não confiável: só na VM Windows, nunca no Mac, e a confiança é decisão do Lipe. Nada vai para o carro sem as regras de trabalho do projeto.


### MIB3tosaurus (perícia) e jugih-official (bancada e mídia) — 13/10/2026

A pedido do Lipe, mais dois. Nenhum é de Waze, Maps ou cluster; os dois rendem por outros motivos.

**[nicolasnt3-a11y/MIB3tosaurus](https://github.com/nicolasnt3-a11y/MIB3tosaurus)** (perícia; Python, GUI e linha de comando; criado em 28/08/2025, 1★). Lê **extrações lógicas (coredumps, logs) de unidades MIB3 OI** — a nossa família — exportadas pelo **"Infotainment recorder" do VCDS**, e gera um relatório HTML: varre tudo recursivamente (estilo `strings`), acha blocos JSON mesmo corrompidos, **extrai coordenadas GPS**, organiza por data num tabelão (pandas), **plota os pontos num mapa OpenStreetMap (Folium)** e ainda recupera as imagens das pastas `Coredump` com miniaturas. O "mapa" dele é isso: o histórico de posições que a central guardou, plotado depois — não é navegação do carro. **O que vale saber para nós:** a MIB3 **registra histórico de localização, horários e imagens** em log e coredump, e isso sai por um export de diagnóstico comum; é tema de privacidade (não publicar uma extração dessas) e mostra o VCDS Infotainment recorder como caminho de dados (estamos focados no ODIS, então aqui é só informação).

**[jugih-official](https://github.com/jugih-official)** ("Jukka, físico e engenheiro", 6 repositórios). Dois interessam:
- **[can-emulator](https://github.com/jugih-official/can-emulator)** (mexido em 13/10/2026) — Arduino Uno + MCP2515 que **emula a ignição/CAN do MQB para teste de bancada**. É a contraparte de hardware do nosso simulador (que é só software): serve para alimentar e acordar um módulo MQB na bancada — por exemplo a própria câmera do projeto 01 ou uma central — fora do carro. Fica anotado como opção se um dia quisermos bancada física.
- **[mib3-dl](https://github.com/jugih-official/mib3-dl)** — converte qualquer vídeo para um **MP4 que a MIB3/MOI3 toca** (o perfil de codec/contêiner que o tocador da unidade aceita), com baixador do Yle Areena embutido. É para pôr vídeo real na central de verdade; não tem a ver com a "mídia de mentira" do simulador.

Com isso, as buscas cobrem o campo público de MIB3 LG: **nada de Waze/Maps no painel**, e o que existe é ferramenta de gráfico, dataset, boot, perícia e bancada.


### Varredura ampla e o grupo do Telegram (13/10/2026, continuação)

Fui aos outros repositórios dos mesmos autores, a tópicos e a mais termos. **A conclusão só ficou mais forte: toda a cena de "mapa no Virtual Cockpit" é Harman (MIB2/MIB2.5/MHI2/MHI2Q/MH2P), sobre QNX e transporte MOST/AID — nenhum é MIB3 LG.** O que apareceu de relevante:

- **O grupo de Telegram do Lipe ("VcMostRenderMQB") é o projeto [OneB1t/VcMOSTRenderMqb](https://github.com/OneB1t/VcMOSTRenderMqb)** (193★): "escrever dados ou um stream VNC no Virtual Cockpit do MQB". O próprio README traz o link do grupo. **Pré-requisitos: MIB2 com acesso SSH, Virtual Cockpit e servidor VNC no celular**; instala pela [mib2-toolbox](https://github.com/jilleb/mib2-toolbox) e usa um patch que sobe o MOST de 10 para 20 FPS. É QNX/MIB2/MOST — a razão de não valer para o Jetta (MIB3 LG, vídeo por rede IP) é essa.
- **A cena de CarPlay/Android Auto no cluster, toda Harman** (as maiores, por estrelas): [yuedizhibo/MHI2Q-CarPlay-AltScreen](https://github.com/yuedizhibo/MHI2Q-CarPlay-AltScreen) (382★), [Lanye-z/MHI2Q-CarPlay-MMI-Mirror](https://github.com/Lanye-z/MHI2Q-CarPlay-MMI-Mirror) (356★), [Mr-MIBonk/M.I.B._More-Incredible-Bash](https://github.com/Mr-MIBonk/M.I.B._More-Incredible-Bash) (1148★, o "canivete" da MIB 2.x Harman), [chefranov/mhi2-au37x-carplay](https://github.com/chefranov/mhi2-au37x-carplay) (76★), [kamgurgul/mib2q-carplay](https://github.com/kamgurgul/mib2q-carplay) (66★) e o [harman-f/mhi2_altscreen_carplay](https://github.com/harman-f/mhi2_altscreen_carplay) já citado.
- **O mais "pronto para usar", e ainda assim fora da nossa plataforma:** [MacsInSpace/mhi2-android-auto-cluster-map](https://github.com/MacsInSpace/mhi2-android-auto-cluster-map) põe o mapa do Android Auto (Maps/Waze) no cluster da VW **instalando por cartão SD, sem toolbox, notebook nem VNC** — mas é para **Harman "Discover Pro" MIB2.5 High**, não LG.
- **Da nossa câmera (projeto 01):** [Mr-MIBonk/2Q0980653x-Tools](https://github.com/Mr-MIBonk/2Q0980653x-Tools) (45★), com template do formato da nossa 2Q0 980 653 D FW 1421. Detalhe e ressalvas no README do projeto 01.
- **Bancada física:** além do [jugih-official/can-emulator](https://github.com/jugih-official/can-emulator), há o [fifthBro/mh2p-testbench-cansim](https://github.com/fifthBro/mh2p-testbench-cansim) (simulador de CAN da VAG em Arduino, para operar módulo + gateway na bancada).
- **De coding, podem servir (MQB em geral, não MIB3):** [jilleb/MQB-FPA](https://github.com/jilleb/MQB-FPA) (141★, pesquisa de perfis de condução) e [jilleb/odis2vcp](https://github.com/jilleb/odis2vcp) (35★, converte datasets XML do ODIS para o formato VCP).

**Cobertura:** foram duas rodadas de busca (web + API), os repositórios dos nove autores da cena e treze termos. Para o MIB3 LG (a nossa), nada de Waze/Maps/cluster; o ecossistema público da nossa unidade é de gráfico, dataset, boot, perícia e bancada, já listado acima. Se quiser monitorar o surgimento de algo novo, o lugar é o grupo OneB1t e os autores n3oka e jilleb.


### Busca focada em MIB3 (13/10/2026): datasets de som da nossa unidade

A pedido do Lipe ("foca em achar coisas da MIB3"), rodei mais 19 termos só de MIB3/MOI3/LG, os forks do mib3-toolbox e os autores que faltavam. A busca por nome está **saturada** (os tópicos `mib3` e `moi3` têm zero repositórios — a comunidade não marca assim —, e os sete forks do mib3-toolbox não têm conteúdo novo). O ecossistema público da nossa unidade é pequeno e já está mapeado nas seções acima. **Uma coisa nova e na nossa plataforma apareceu:**

- **Formato dos datasets de som/equalização da MIB3, documentado.** O [NumberOneBot/mqb-mib-sound-datasets](https://github.com/NumberOneBot/mqb-mib-sound-datasets) (templates do 010 Editor para os datasets dentro do contêiner **ZDC** de unidades MIB1/2/**3**) e o [jfalmeida/mib3_sound_dataset](https://github.com/jfalmeida/mib3_sound_dataset) trazem o layout dos datasets de som do **5F da MIB3**, nos endereços **`0x007201`, `0x007204` e `0x007206`** — a configuração do sistema de som e o ajuste fino do equalizador para os alto-falantes instalados. **É direto da nossa central (5F MIB3)** e toca o item em aberto do "Som Beats, a confirmar" na ficha e o "som de mentira" do simulador (Marco 13): quando a leitura do 5F do Jetta chegar, dá para ler esses datasets com esses templates e comparar o amplificador de verdade com o que o simulador inventou.
- **Parser de dataset para ODIS-E (XML):** [OneB1t/MQBDatasetParser](https://github.com/OneB1t/MQBDatasetParser) (C#) lê o dataset da câmera (A5 Front Assist) exportado pelo ODIS-E. É da câmera — deixo só anotado, por ora fora de foco.
- **CAN e bancada (MQB em geral, não MIB3):** [mrfixpl/MQB-sniffer](https://github.com/mrfixpl/MQB-sniffer) (96★, registro de IDs de CAN do Golf MK7/MQB) e mais dois emuladores de CAN em Arduino para bancada, [JeniCzech92/MQBClusterMileageRoller](https://github.com/JeniCzech92/MQBClusterMileageRoller) e [kostaszaf/can-gateway-sniffer](https://github.com/kostaszaf/can-gateway-sniffer), somando ao can-emulator e ao mh2p-testbench-cansim já citados.

**Conclusão da busca de MIB3:** confirmado que não há, no público, nada de Waze/Maps nem de render no cluster para a MIB3 LG. O que é da nossa unidade e vale guardar: as ferramentas de gráfico/imagem/3D/boot do n3oka, os templates de formato do jilleb/mib3-toolbox, estes **templates de dataset de som do 5F** (o achado novo), o parser de perícia MIB3tosaurus e o conversor de vídeo mib3-dl. A parte de ativação/FEC (challenge, activator) fica de fora por ser o limite de proteção.


### Referências técnicas de arquitetura da MIB3 (13/10/2026) — e uma dúvida a verificar

Fora do GitHub (busca na web, e os PDFs lidos com `pdftotext`), apareceram **duas fontes oficiais/independentes sobre a arquitetura da MIB3** — o mais útil de tudo, porque é sobre o carro de verdade, não sobre ferramenta:

- **SSP eSelf-Study Program 970893, "Third-generation modular infotainment matrix"** (documento de treinamento da VW/Audi, público na NHTSA: [PDF](https://static.nhtsa.gov/odi/tsbs/2021/MC-10189329-0001.pdf)). É centrado na Audi, mas descreve a MIB3 em geral. O que importa, confirmado no texto:
  - A central é o `J794` ("Control unit 1 for information electronics"). Há três versões: **Basic, High e Premium**; a Basic se distingue por **não ter o LVDS para o painel `J285`**; High/Premium têm (conector branco).
  - **A MIB3 não tem antena de GPS na central:** o módulo de emergência/conectividade `J949` recebe o GPS e **manda a posição para a `J794` por Ethernet**. A central calcula a navegação, mas recebe o GPS pela rede.
  - No **Audi virtual cockpit**, o mapa de navegação e os cruzamentos vão ao painel `J285` **por LVDS**, e os menus/lista por **MOST**. Isso é Audi/MLB.
  - **Importante para o nosso item 8:** esse caminho LVDS/MOST é o da Audi. A nossa unidade **VW MQB LG (MU0883)** manda o vídeo do mapa do Active Info Display por **Ethernet, multicast `ff14::2:4`** (medido no firmware, seção 1 acima). Ou seja, não se contradizem: é implementação diferente por fabricante/plataforma, e a nossa é a de Ethernet.
- **Black Hat EU-24, "Over-the-Air: Compromise of Modern Volkswagen Group Vehicles"** ([slides](https://i.blackhat.com/EU-24/Presentations/EU-24-Parnishchev-OverTheAirVW.pdf)). Confirma, de fonte independente: a VW não fabrica a MIB3, **encomenda de três fornecedores — Preh Car Connect, LG e Aptiv** — e a palestra (12 vulnerabilidades) é **só sobre a MIB3 da Preh**, não a da LG. Reforça por que a LG é "escuridão total" no público: a pesquisa de segurança publicada mirou a Preh. A arquitetura que eles desenham é o mesmo esqueleto: Ethernet **Base-T1** ligando gateway, telemática, infotainment e painel, com o conector de tela por **LVDS** na unidade Preh.

**A lista "Preh" da Black Hat NÃO muda o nosso fabricante — isso a ficha já resolveu.** A lista de part numbers da palestra (de `/etc/swup/tnr/tnrref.csv`) inclui `3G5035832[A|C|D|E|F|G]`, e a nossa central é `3G5 035 832 C`; o [mib-helper.com](https://mib-helper.com/index.php?train=MOI3_EU_VWMQB_P27800P) também marca "Hardware vendor: Preh" para um train MOI3. **Mas isso é compatibilidade ampla de part number, não identificação de hardware.** A [ficha do carro](../../../carros/jetta-gli-2021/README.md), seção "Leitura do firmware", já cravou **LG** pela voz do software: o `main.mnf` do pacote MOI3 lista as variantes compatíveis e todas terminam em `-VW-MQB-LG`, e o hardware é **Qualcomm Snapdragon com AGL Linux**, não o Renesas da Preh. Ou seja, as fontes novas reforçam que a LG é um fabricante à parte e pouco pesquisado (a Black Hat mirou só a Preh), e não abalam a identificação da nossa central. Corrijo aqui um susto que eu mesmo tinha escrito: não é "dúvida a conferir no autoscan"; está resolvido pelo firmware. (A etiqueta física no autoscan será só confirmação redundante.)
- **Recurso de firmware:** o [mib-helper.com](https://mib-helper.com) é um banco de trains de firmware da MIB, inclusive MOI3 (ex.: `MOI3_EU_VW_R7570L`, `MOI3_EU_VWMQB_P27800P`), com o último update oficial de cada um. Útil para conferir versões; não traz o protocolo do vídeo.
- **Extrato completo do SSP 970893** (lido por inteiro em 13/10/2026, com o que interessa ao projeto, separando o que é geral do que é só da Audi): [../../mib3-ssp970893-arquitetura.md](../../mib3-ssp970893-arquitetura.md). Traz os endereços de diagnóstico (5F e 0075), a topologia de som (A2B, 5F master), o rádio (três sintonizadores, busca de fundo), e o GPS por Ethernet via J949.


### Mais documentos achados (13/10/2026): SSP do Jetta e boletins de MIB3

Continuando a busca por documentos (não repositórios), dois conjuntos novos e oficiais, além do SSP 970893 já extraído:

- **SSP 588 "The Jetta 2019"** (VW, [vag-technique.fr](https://www.vag-technique.fr/ext_files/ssp/vw/SSP_588_The_Jetta_2019.pdf)). É o self-study da VW do **nosso carro**, mas da geração anterior de infotainment: o texto diz "The New Jetta uses the modular Infotainment matrix (MIB) **generation 2**". Logo, a parte de rádio/navegação é MIB2 (Composition Color/Media, Discover Media, BeatsAudio), **não a nossa MIB3**. O que vale e é de carro (não da geração da central): confirma o **BeatsAudio** (amplificador `J525` = endereço 47, 300 W, 6 alto-falantes + subwoofer), o **Active Info Display** de 10,25" (2D/3D, navegação e mídia), a proteção de componentes via **FAZIT** (chave reconhecida → módulo bloqueia função; habilitar online pelo tester), e a lista de módulos no padrão VW com os números J (ex.: `J794` = central/5F, `J525` = som digital/47, `J285` = painel, `J533` = gateway, `J685` = tela, `J949` = eCall/conectividade). Útil de cruzamento com o SSP 970893 (que é Audi): os números J são os mesmos, as marcas mudam.
- **Boletins técnicos (TSB) da VW para a MIB3, na NHTSA** (oficiais, públicos, legíveis com `pdftotext`). Tratam da nossa central e vários citam o Jetta:
  - **91-23-03 — MIB3 Various Infotainment Concerns** → atualiza para o software **0841** e exige **carregador de 70 A**; lista o Jetta. [PDF](https://static.nhtsa.gov/odi/tsbs/2023/MC-10239652-0001.pdf). (Anotado no projeto 01.)
  - **91-22-02 — MIB3 Infotainment System Reset** [PDF](https://static.nhtsa.gov/odi/tsbs/2022/MC-10214462-0001.pdf).
  - **MIB3 Wireless CarPlay Connectivity Issues** [PDF](https://static.nhtsa.gov/odi/tsbs/2022/MC-10220334-0001.pdf).
  - **91-21-02 — MIB3 Composition & Discover Media** [PDF](https://static.nhtsa.gov/odi/tsbs/2021/MC-10200561-0001.pdf).
  - Recall **22V514** (tela/câmera de ré preta na MIB3) cobre o Jetta 2021 — já citado na ficha.

**Conclusão da busca por documentos:** o melhor continua sendo o **SSP 970893** (arquitetura da MIB3, já extraído em [../../mib3-ssp970893-arquitetura.md](../../mib3-ssp970893-arquitetura.md)). O SSP 588 serve ao carro (Beats, AID, FAZIT, lista de módulos), lembrando que o infotainment dele é MIB2. Os TSBs são de serviço (níveis de software, problemas conhecidos, o carregador de 70 A). Nada de Waze/Maps/cluster — isso não muda.


### Continuação da busca por documentos (13/10/2026): SSP de VW e a pesquisa da Preh

Fui atrás de um SSP de VW específico da MIB3 (ex.: Golf 8) e do artigo de segurança. Achados:

- **Não há SSP de VW de MIB3 fácil de achar no público.** O Golf 8 (o irmão VW mais próximo do Jetta com MIB3) não apareceu como self-study baixável; o manual de arquitetura da MIB3 continua sendo o **SSP 970893 da Audi** (já extraído). De VW só vêm documentos de geração anterior como pano de fundo: SSP 890153 (MIB II), SSP 970143 (MIB1) e um SSP de barramentos (CAN/LIN/MOST).
- **A pesquisa de segurança da MIB3 é da Preh, não da LG — e os detalhes confirmam que são hardwares diferentes.** A palestra da Black Hat EU-24 é da **PCAutomotive** (Danila Parnishchev e equipe). A unidade que eles abriram é a **Preh**, montada sobre **SoC Renesas R-Car M3, 64 GB eMMC e 32 MB de flash SPI**. A **nossa é LG, Qualcomm Snapdragon com AGL Linux** (ficha): SoC diferente, sistema diferente. Acharam 21 vulnerabilidades (9 publicadas em 2023, 12 na Black Hat 2024; CVE-2023-28902 a 28912 e 29113): um ataque inicial por Bluetooth deixa código residente, e daí dá para controlar o infotainment por um túnel de DNS, ler GPS e velocidade, gravar conversas e tirar fotos da tela; **não passaram do gateway para freio/direção**. Estimaram 1,4 milhão de carros. Referências: [slides](https://i.blackhat.com/EU-24/Presentations/EU-24-Parnishchev-OverTheAirVW.pdf), [advisory da PCAutomotive](https://pcacybersecurity.com/resources/advisory/vulnerabilities-in-vw-mib3-infotainment-2), [cobertura](https://securityaffairs.com/172024/hacking/volkswagen-group-infotainment-unit-flaws.html). Para nós é só contexto (é Preh): reforça que a LG é um fabricante à parte e pouco pesquisado, e completa a privacidade do carro (a MIB3 guarda GPS, velocidade e som — o que o [MIB3tosaurus](https://github.com/nicolasnt3-a11y/MIB3tosaurus) extrai).
- **"Coding list Golf 8" do n3oka** — lista de codificações e retrofits do VW Golf 8 (Mk8), partilhada em jan/2023 na comunidade golf8gti e atualizada até ago/2025 ("not for commercial use"). **Não li o PDF inteiro** (as cópias estão atrás de verificação anti-robô do Cloudflare no manuals.plus, que não contorno, e no Scribd); o que segue vem da descrição e do [tópico do fórum do OBDeleven](https://forum.obdeleven.com/thread/18043/coding-list-golf-8), **então sei os assuntos, não os bytes**. É organizada por função e por módulo, cada item marcado testado/não testado/sem função, com o módulo, a versão de software testada e os valores. Cobre: iluminação (luz diurna, funções de luz no 09, barra de luz, módulo USB); dinâmica (distribuição de torque na tela, secagem do disco de freio no 03, start-stop); retrofits (câmera de ré, engate de reboque, TPMS/sensor de pressão dos pneus, hotspot WiFi); som (subwoofer Harman/Kardon mais alto); infotainment/painel MIB3 (logo de boot, cronômetro de volta no head-up — anotado "sem função"); e avisos em alguns itens (erro não apagável; desativar a atribuição automática de chave depois de certas codificações). **Para nós:** é Golf 8, mesma plataforma (MQB) e mesma geração de central (MIB3) do Jetta, então a codificação se parece muito, mas é outro modelo — módulos, versões e valores podem diferir; serve de cruzamento, não de receita. Para registrar valor é preciso ler o PDF; fica como fonte a abrir quando formos codificar.

**Onde a busca fecha.** Documentos: SSP 970893 (arquitetura MIB3, o principal), SSP 588 (carro, MIB2), os TSBs de serviço de MIB3 e o SSP 571 (Polo 2018, para o Active Info Display). Pesquisa de segurança: só Preh. Coding: a lista do Golf 8 do n3oka, a buscar. **Nada de Waze/Maps/cluster** para a MIB3 LG — confirmado por GitHub, tópicos, strings de firmware, autores da cena e documentos oficiais.

## Camada 3 (opção "C"): como o mss consome a renderingSurface — RE fechada (16/10/2026)

Com o **REA + Ghidra 12.1.4** funcionando (conserto do dia, ver `memory/re-firmware-mib3-ferramentas.md`), fiz o RE do lado que **consome** a renderingSurface no `multi-screen-server` (binário do MU0883). Resposta à pergunta que estava em aberto no projeto do C:

- A `renderingSurface` **tem campo de fonte**: o campo JSON **`name`** no esquema **`surfaceid://<id>`**. O mss (`MultiScreenController::prepareStreamsList @0x130028`) extrai o `<id>` (strip dos 12 chars de `surfaceid://`, fallback `internalIdentifier`), casa com `streamParameters` (ip/porta/fps) e `display`, e dispara `ivi_share_get_ivi_share_surface(<id>)` → encode → RTP. O `<id>` é o inteiro do grab (o mesmo `1000` do `-m 1000` da bancada).
- Isso **corrige** a dedução antiga ("renderingSurface não tem fonte, redirect seria no compositor"), que eu tinha tirado só do lado serviço (`rsi-display-management`). O redirect do C **é** setar `name=surfaceid://<surface-do-app>` na `navi-kombi-renderingsurface` via RSI.
- Detalhe completo, endereços e os dois caminhos do C em [`camada3-rsi-redirect.md`](camada3-rsi-redirect.md).

**Lado publicador fechado (`libMIBNavSDClientLib_CarIF.so.00`, mesma sessão).** Quem publica a RS do mapa nativo é a classe **`Viwi::NavSd::us_displaymanagement`** (VIWI = REST/JSON da VW). No **boot** (`registerDisplays`): cria `navi-kombi-display` → `createRenderingSurfaces` (POST com `name="navi-kombi-renderingsurface"`, w/h, `display`) → `assignRenderingSurfaceToMapRenderer` (POSTa `{"renderingSurface":"<uri>"}` no maprenderer) → `startMapStreaming`. **A fonte é ligada uma vez, no registro.** Trocar visibilidade do mapa **não** reescreve a fonte: o `OnKombiMapRendererVisibilityChanged` só chama `ToggleStreamFramerate`, que POSTa `{"framerate":N}` no streamParameters, com `N` = **1 fps** (escondido) ou **10 fps** (ativo) — `calculateFramerate` **nunca zera**. Logo um `onUpdate` de redirect **cola** entre aparecer/sumir, mas é **revertido** por um reinício do NavSd (roda `registerDisplays` de novo) e sofre o afunilamento de fps se sequestrar a RS nativa. Recomendação registrada no doc do C: pra robustez, criar RS+streamParameters próprios em vez de sequestrar a nativa; pra teste, o grab direto do mss (`-m <surface> <fps>`).
