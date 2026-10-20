# stubs/telefone — o telefone de mentira da bancada

Marco 12, 12/10/2026. No carro, a tela do telefone conversa com dois grupos de programas do
firmware: o do Bluetooth (que cuida do aparelho pareado, da agenda e das mensagens) e o gerente de
aparelhos. Na bancada não há rádio Bluetooth nem celular, e esses programas não sobem. O "telefone
de mentira" ocupa o lugar deles diante da HMI: anuncia os dois domínios e responde com os estados
que estiverem nos arquivos desta pasta.

**É estudo num computador. Nada daqui vai para o carro.** E é simulação do começo ao fim: não há
Bluetooth, pareamento, chamada, agenda nem mensagem de verdade, e nenhum arquivo traz nome, número
ou contato de ninguém. São só os estados que a tela mostra. O perfil `aparelho-conectado.txt` traz
um aparelho **fictício**: o nome ("Celular fictício"), o endereço Bluetooth de exemplo e a
operadora ("Fictícia") são inventados e dizem que são; não há IMEI, IMSI, número nem contato.

O registro da investigação está em [../../modelagem-soc.md](../../modelagem-soc.md), seção do
Marco 12.

**Desde a manhã de 12/10/2026 o aparelho fictício conecta e desconecta pela tela.** Quem sobe é o
`driver/telefonestub.c`: o mesmo motor, mais um modelo pequeno. Em "Selecionar celular" o aparelho
conhecido e não conectado é um botão; tocar nele conecta (aviso "Estabelecendo conexão…" por 2 s,
depois a tela principal do telefone e os ícones no alto). Desligar o Bluetooth pela tela desconecta
(a tela do telefone mostra "Ligar o Bluetooth.", sem nome nem ícones); ligado de novo, o aparelho
fica conhecido e não conectado, à espera do toque.

**E disca, desde o fim da manhã de 12/10/2026.** Com o aparelho conectado, "Discar nº" e a tecla
verde abrem a tela de chamada com o número digitado: "discando", dois segundos depois "chamando",
aos cinco "em chamada". É o relógio do modelo: nada é discado, não há áudio e ninguém atende.
**O botão vermelho da tela encerra a chamada no modo ao vivo**, desde a tarde de 12/10/2026: por
decisão do Lipe o simulador passou a subir como "central com CarPlay", que é o que o carro tem, e
é isso que faz o botão funcionar (ver "O CarPlay no padrão", em "Como usar"). Desligar o Bluetooth
pela tela também derruba a chamada. Numa central sem CarPlay (`CARPLAY=0`, ou uma central de
teste sem codificação) o botão não encerra, e a chamada acaba sozinha aos 20 s.

**E recebe chamada, desde a noite de 12/10/2026.** Não há aparelho de verdade para ligar, então o
gatilho é um número combinado e fictício: **disque 5550199**, encerre a chamada pelo botão
vermelho e, cinco segundos depois, o aparelho fictício "liga de volta". A tela de chamada abre
sozinha com o número tocando e três botões: o verde atende (e a chamada vira uma chamada em curso,
que o botão vermelho encerra), o vermelho recusa, e o do sino só marca o toque como silenciado.
Se ninguém atende nem recusa, ela para de tocar aos 20 s.

## O que muda na tela

| | Sem o telefone de mentira | Com `sem-aparelho.txt` |
|---|---|---|
| tecla Telefone da moldura | popup "Telefone indisponível." | a tela do telefone: "Conecte o telefone celular apenas quando parado. Procure pelo menu de Bluetooth do celular." |
| grade de menus | oito blocos, uma página, sem telefone | ganha o bloco "Telefone" e passa a ter duas páginas |
| tecla Telefone do volante | a HMI recebe e a tela não muda | igual: a tela não muda |
| engrenagem da tela do telefone | — | "Configurações do telefone"; em "Selecionar celular", "Nenhum celular conhecido disponível."; e as "Configurações do Bluetooth" |
| caixa "Bluetooth" nessas configurações | — | desliga e liga o Bluetooth de mentira; desligado, a tela do telefone mostra "Ligar o Bluetooth." com o botão para ligar |
| assistente de configuração | duas páginas (hora e lista de memória) | ganha a terceira, do telefone, que só explica como conectar e deixa seguir |

Com o segundo perfil por cima, `aparelho-conectado.txt` (um aparelho **fictício** conectado):

| | Com `sem-aparelho.txt` | Com `sem-aparelho.txt` e `aparelho-conectado.txt` |
|---|---|---|
| tecla Telefone da moldura | a tela "Conecte o telefone celular…" (`PHONE_NOT_CONNECTED`) | a tela principal do telefone (`PHONE_MAIN_NOCALLS`): a caixa com o nome "Celular fictício", a operadora "Fictícia", seis favoritos vazios ("Adicionar favorito") e a barra com "Discar nº", "Chamadas" e "Config." |
| barra do alto da tela | sem ícones de telefone | os ícones de Bluetooth, da bateria do aparelho e do sinal |
| "Discar nº" | — | o teclado ("Introduzir número de telefone"), com a tecla verde e a "Caixa postal" |
| "Chamadas" | — | "Nenhum registro disponível." |
| um favorito | — | "Nenhum contato disponível com números de telefone…" e o botão "Importar contatos novamente" |
| "Selecionar celular", nas configurações | "Nenhum celular conhecido disponível." | uma linha, "Celular fictício", marcada como o telefone 1, e "É possível conectar outro celular." |
| tecla Telefone do volante | a tela não muda | igual: a tela não muda |

## Como usar

Os três estados de partida, pela ordem dos arquivos em `TELEFONE` (os caminhos são os de dentro do
container):

| `TELEFONE` | Como a central nasce |
|---|---|
| `/stubs/telefone/sem-aparelho.txt` | nenhum aparelho: a tela "Conecte o telefone celular…", e "Selecionar celular" vazio |
| os dois primeiros mais `/stubs/telefone/comeca-desconectado.txt` | o aparelho fictício conhecido e não conectado: a mesma tela, e "Selecionar celular" com o botão "Celular fictício", que conecta |
| `/stubs/telefone/sem-aparelho.txt /stubs/telefone/aparelho-conectado.txt` | o aparelho fictício já conectado: a tela principal do telefone e os ícones no alto |

O padrão do modo ao vivo (`driver/ao_vivo.sh`) está dito no cabeçalho dele.

- **Numa central de teste:**
  `TELEFONE=/stubs/telefone/sem-aparelho.txt sh driver/m8_central.sh sobe <nome> <pasta>`.
  O caminho é o de dentro do container; a pasta `stubs/` aparece lá como `/stubs`.
- **No lançador direto** (`driver/m1_hmi.sh`) e no `driver/m3_ao_vivo.sh` a opção é a mesma variável
  `TELEFONE`, e vem **desligada**. O padrão do modo ao vivo (`driver/ao_vivo.sh`) está dito no
  cabeçalho dele.
- **Sem o telefone de mentira:** `TELEFONE=` (vazio). A tecla volta ao aviso.
- **Só o anúncio e os valores padrão**, sem perfil: `TELEFONE=-`. O Bluetooth sai "ligado", mas sem
  o estado de ativação do aparelho a HMI não decide a tela do telefone (o zero ela ignora).
- **Com o aparelho fictício conectado** (o segundo estado):
  `TELEFONE="/stubs/telefone/sem-aparelho.txt /stubs/telefone/aparelho-conectado.txt"`, nessa
  ordem. O segundo arquivo só acrescenta o aparelho; o Bluetooth ligado, as respostas e as regras
  vêm do primeiro. Vale para a central de teste e para o modo ao vivo
  (`TELEFONE="…" sh driver/ao_vivo.sh`); o padrão do modo ao vivo continua sendo só o primeiro.
- **Outro perfil:** `TELEFONE="/stubs/telefone/sem-aparelho.txt /stubs/telefone/meu.txt"`. O arquivo
  de depois vale sobre o de antes, campo a campo. O perfil é lido na partida.
- **Ver o que um perfil publicaria**, sem subir nada (dentro do container):
  `veiculostub -m /stubs/telefone/gerado/esquema.txt <perfil>...`
- **Ver o que a tela pediu e o que foi feito:** o registro (`telefonestub.log`) traz uma linha por
  assinatura e por pedido. Numa central de teste:
  `docker exec <container> cat /mnt/low/tmp/telefonestub.log`. As linhas saem com `[veiculostub]`
  na frente porque o programa é o mesmo do carro de mentira (ver abaixo).
- Linha errada no perfil: nada sobe, e a mensagem diz o arquivo e a linha.

### A chamada recebida: disque 5550199

| Passo, pela tela | O que acontece |
|---|---|
| conectar o "Celular fictício" (Telefone, engrenagem, "Selecionar celular", toque no aparelho) | a tela principal do telefone |
| "Discar nº", 5550199, tecla verde | uma chamada como as outras: discando, chamando, em chamada |
| botão vermelho | a chamada acaba; **5 s depois o aparelho fictício liga de volta** |
| a tela de chamada abre sozinha: 5550199, "chamando…" | três botões: atender (verde), silenciar o toque (sino) e recusar (vermelho) |
| atender | a chamada passa a "em chamada" (0:00, mudo, botão vermelho); o botão vermelho a encerra |
| recusar | a chamada some e a tela volta à principal do telefone |
| não fazer nada | ela para de tocar aos 20 s |

O 5550199 é da faixa 555-01xx, que não é de ninguém; nenhum outro número liga de volta. A volta é
uma por discagem, e desconectar o aparelho ou desligar o Bluetooth antes dos 5 s a cancela. Numa
central sem CarPlay, em que o botão vermelho não encerra, a volta vem 5 s depois do fim por tempo
dos 20 s. Não há toque de verdade nem áudio; os tempos (5 s e 20 s) são escolha da simulação.
Medido nas rodadas `i1` e `i2` de 12/10/2026 (`traces/tel_recebida_*`).

### Parear um segundo aparelho: abra "Celulares conhecidos" e espere

Esta central não busca aparelhos pela tela: "Celulares conhecidos" manda procurar a central pelo
celular. O que ela faz é confirmar o código de um aparelho que pede para parear, e é isso que o
telefone de mentira simula, com um segundo aparelho fictício (`segundo-aparelho.txt`).

| Passo, pela tela | O que acontece |
|---|---|
| Telefone, engrenagem, "Selecionar celular" | a tela "Celulares conhecidos" |
| esperar 4 s | abre a confirmação: "Esta senha 123456 coincide com a senha no Celular fictício 2?", com "Cancelar" e "Sim" |
| "Sim" | o "Celular fictício 2" entra na lista, abaixo do primeiro, e não pede mais |
| "Cancelar" | a HMI avisa que a conexão foi cancelada; ele não entra, e pede de novo na próxima vez que a tela abrir |
| sair da tela sem responder | o pedido some |

O pedido é um por abertura da tela e só acontece com ela aberta: na partida e nas outras telas
nada aparece. O nome, o endereço (`00:00:00:00:00:02`) e o código (`123456`) são inventados. **O
segundo aparelho só existe para o pareamento**: fica na lista como conhecido e não conecta. O
pareamento vale para a sessão: a central seguinte nasce sem ele. Medido nas rodadas `j1`, `k1` e
`k2` de 12/10/2026 (`traces/tel_pareamento_*`).

### O CarPlay no padrão do modo ao vivo: o botão vermelho encerra a chamada

Desde a tarde de 12/10/2026, por decisão do Lipe, `sh driver/ao_vivo.sh` sobe a central "com
CarPlay". São três peças, que só fazem sentido juntas:

| Peça | O que é |
|---|---|
| uma linha no `stubs/codificacao/jetta-gli-2021-deducoes.txt` | o campo `ID_APPLE_DIO` da adaptação (DID `0555`) diferente de zero. **É dedução marcada, não leitura do 5F**: o carro tem CarPlay (a foto de 09/10/2026 mostra o bloco "Apple CarPlay" na grade da central), mas o valor do campo nele não foi lido |
| o esquema `gerado-carplay/esquema.txt` no lugar do `gerado/esquema.txt` | o telefone de mentira anuncia também o domínio 512 e atende o serviço do CarPlay (`DSICarplay2`) |
| o perfil `carplay-sem-chamada.txt`, depois dos outros três | a única resposta com valor desse serviço: a lista de chamadas do CarPlay, vazia e válida |

O que isso muda na central:

- **o botão vermelho da tela de chamada encerra a chamada simulada**, e ela deixa de acabar por
  tempo: fica até o botão, a desconexão do aparelho ou o Bluetooth desligado;
- **a grade do menu tem o bloco "App-Connect"**. Numa central sem memória guardada ele aparece na
  primeira página, entre "Mídias" e "Veículo"; com a memória guardada do modo ao vivo fica na
  segunda página, logo abaixo de "Telefone" (deslize a grade para a esquerda);
- **a partida leva uns 6 s a mais**: a HMI passa a esperar por Android Auto e MirrorLink, que a
  bancada não tem.

**Não há CarPlay simulado**: nenhum aparelho, nenhuma tela dele. Os outros estados do serviço saem
sem a marca de validade e a HMI os ignora. Não abri o bloco "App-Connect".

**Para desligar:** `CARPLAY=0 sh driver/ao_vivo.sh`. A central volta a ser a de antes: sem
"App-Connect", com o botão vermelho sem encerrar e a chamada acabando sozinha aos 20 s. O
`CARPLAY=0` põe o `stubs/codificacao/sem-carplay.txt` no fim da codificação (o campo de volta no
zero) e sobe o telefone com o esquema `gerado/` e os três perfis.

Por que são precisas as duas coisas, a adaptação e o serviço:

| Central de teste | A HMI assina o CarPlay? | O botão vermelho |
|---|---|---|
| `h1`, `h3`: nem adaptação nem serviço | não | estoura, não encerra |
| `r1`: só o serviço no barramento | não | estoura, não encerra |
| `r3`: só a adaptação | não | estoura, não encerra |
| `r2`, `r4`: a adaptação e o serviço | sim, os dez estados | **encerra no primeiro toque** |

A parte de CarPlay da HMI só começa a subir se a adaptação disser que há CarPlay (sem isso a HMI
pula na partida a unidade de trabalho da integração de smartphone: "featureFlag 814 is disabled,
skip this work unit"), e só termina de subir quando o serviço `DSICarplay2` aparece no barramento.
O código de chamadas do telefone consulta essa parte sem conferir se ela subiu.

Numa central de teste (`driver/m8_central.sh`) nada disso entra sozinho: ela sobe sem codificação
e com o esquema `gerado/`. Para tê-la com CarPlay, as três peças vão à mão: `CODING` com os
arquivos do modo ao vivo, `TELEFONE_ESQUEMA=/stubs/telefone/gerado-carplay/esquema.txt` e
`TELEFONE` com o `carplay-sem-chamada.txt` no fim. Com o esquema `gerado/` o programa recusa esse
perfil e diz o arquivo e a linha.

## Os arquivos

| Arquivo | O que é | De onde veio |
|---|---|---|
| `sem-aparelho.txt` | o perfil: Bluetooth ligado e nenhum aparelho conectado; a resposta "feito" a seis pedidos de que a HMI espera resposta; e a regra que deixa a tela ligar e desligar o Bluetooth | **SIMULAÇÃO**: nenhum valor lido do carro nem de um aparelho |
| `aparelho-conectado.txt` | o segundo perfil, para usar depois do primeiro: um aparelho **fictício** conectado como aparelho principal (estado de ativação, dados do aparelho, operadora, sinal e bateria, só na instância 0), o mesmo aparelho na lista de aparelhos conhecidos do gerente de aparelhos e a resposta ao pedido de aparelho prioritário | **SIMULAÇÃO**, e os textos são **fictícios**: nenhum dado é de aparelho ou de pessoa de verdade |
| `comeca-desconectado.txt` | o terceiro perfil, para usar depois dos outros dois: uma linha só, que tira o bit de "conectado" do mãos-livres do aparelho na lista. O modelo lê isso e começa com o aparelho conhecido e não conectado | **SIMULAÇÃO** |
| `gerado/esquema.txt` | o esquema dos 18 serviços que os anúncios dos dois domínios ligam: interfaces, números de api, 143 atributos, 329 mensagens de evento, 215 pedidos e os campos de 683 mensagens. Não editar à mão | gerado por `driver/m8_esquema.sh` das classes do firmware |
| `gerado/constantes.txt` | o que cada número quer dizer (estados do Bluetooth, estados de ativação, resultados): 991 constantes | idem |
| `gerado/relatorio.txt` | contagens por serviço, para que domínio vai cada instância do `DSIMobileEquipment`, os nove serviços que ficaram de fora e os avisos | idem |
| `gerado/vetores-perfil.txt` e `vetores-esperado.txt` | a prova do codificador: um perfil que dá valor a toda folha de todo evento e os bytes que as classes do firmware produzem para ele (324 mensagens) | idem |
| `segundo-aparelho.txt` | o perfil do pareamento, que o modo ao vivo põe depois dos três: um segundo aparelho **fictício** (nome, endereço e código inventados) que pede para parear. O modelo guarda esses valores e só os manda 4 s depois de a tela "Celulares conhecidos" abrir | **SIMULAÇÃO** |
| `carplay-sem-chamada.txt` | o quinto perfil, que o modo ao vivo põe depois dos outros três (sai com `CARPLAY=0`): a lista de chamadas do CarPlay, vazia e válida. Pede o esquema `gerado-carplay/` | **SIMULAÇÃO** |
| `gerado-carplay/` | o esquema que o modo ao vivo usa: tudo o que o `gerado/` tem, mais o serviço do CarPlay (`DSICarplay2`, domínio 512, `PID_CARPLAY`, canal `0x200`): 19 serviços, 154 atributos, 347 mensagens de evento, 235 pedidos. Nenhuma linha do `gerado/` sai nem muda; entram 257. A camada da LG aceita os eventos desse serviço por duas interfaces (606 e 607, o mesmo trecho do despacho); o esquema traz o serviço uma vez, com a 606, que é por onde a HMI recebeu (rodada `r2`), e o relatório anota a outra. Os vetores de prova conferem (338 mensagens) | gerado por `driver/m8_esquema.sh`, com `DOMINIOS=256,4096,512` e `ROTULO="de telefone, conectividade e CarPlay"` |

Para refazer a pasta `gerado/` (só lê o firmware, sem rede):

```bash
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v "$PWD/driver":/driver:ro -v "$PWD/stubs/telefone/gerado":/out -e DOMINIOS=256,4096 -e ROTULO="de telefone e conectividade" moi3-bancada:1 sh /driver/m8_esquema.sh
```

Com outras centrais no ar, o mais seguro é trocar `$PWD/stubs/telefone/gerado` por uma pasta à
parte e depois trazer os arquivos com `mv`, um a um.

## O programa: o motor mais um modelo

Até as 9h de 12/10/2026 quem rodava era o próprio `driver/veiculostub.c`, o motor do carro de
mentira, com o esquema e o perfil desta pasta, copiado para a central com o nome `telefonestub`.
Desde então roda o `driver/telefonestub.c`: inclui o motor (como o rádio e a mídia de mentira
fazem) e acrescenta o que não cabe em valor fixo, que é o aparelho mudar de estado quando a tela
pede. O modelo atende três pedidos:

| Pedido da HMI | Quando ela manda | O que o modelo faz |
|---|---|---|
| `DSIPhoneManager.requestConnectService` (identificador, tipo de serviço) | ao tocar no aparelho em "Selecionar celular" (rodada `e2c`); espera a resposta por 180 s, com o aviso "Estabelecendo conexão…" | a instância 0 do aparelho passa a receber as seções `[DSIMobileEquipment[0].…]` do perfil; na lista o mãos-livres vai a 10; responde resultado 0. Aparelho que não é o da lista: 1. Serviço que não é o mãos-livres: 8. Bluetooth desligado: 4 |
| `DSIPhoneManager.requestDisconnectService` | a tela desta variante não manda ao tocar no aparelho conectado (a linha dele é só informação) | a instância 0 volta a receber o que as outras recebem (as seções sem instância); na lista o mãos-livres vai a 2; responde resultado 0 |
| `DSIBluetooth.requestSwitchBTState` (o estado pedido) | a caixa "Bluetooth" das configurações e o botão "Ligar o Bluetooth" | grava o estado, avisa e responde, como a regra do perfil fazia; se o pedido é desligar, desconecta o aparelho antes (e a chamada, se houver). Ligar de novo não reconecta |
| `DSIMobileEquipment.dialNumber` (o número) | a tecla verde do teclado "Discar nº" (rodada `cb`); espera `responseDialNumber` por 30 s | só na instância 0 e com o aparelho conectado: responde resultado 0, diz que é número comum (`updateServiceCodeType`, que é o que solta o teclado), põe o aparelho "em chamada" (estado de ativação 2) e põe na lista de chamadas a chamada 1, com o número que a tela mandou: "discando" (1), aos 2 s "chamando" (2), aos 5 s "em chamada" (4), **e aos 20 s a tira, sozinho**. Sem aparelho conectado: resultado 7. Com chamada em curso: 6 |
| `DSIMobileEquipment.hangupCall` | o botão vermelho da tela de chamada (com o CarPlay na adaptação; sem ele a tela não chega a mandar, ver os limites) e o botão de recusar da chamada que toca | tira a chamada da lista, devolve o aparelho a "ligado" (5) e responde. Se a chamada era para o número combinado 5550199, marca a volta para dali a 5 s |
| `DSIMobileEquipment.acceptCall` | o botão verde da chamada que toca (rodada `i2`; a HMI manda o pedido vazio); espera `responseAcceptCall` por 10 s | só na instância 0 e com uma chamada tocando (senão resultado 7): responde resultado 0 e passa a chamada a "em chamada" (4) |

O aparelho é o primeiro da lista de aparelhos conhecidos do perfil; ele começa conectado se o
mãos-livres dele traz o bit 8 (`SERVICESTATE_CONNECTED`) e desconectado se traz só o 2
(`SERVICESTATE_SUPPORTED`). Sem aparelho no perfil o modelo só liga e desliga o Bluetooth. Os
resultados são constantes `RESULT_` de `org.dsi.ifc.phonemanager.DSIPhoneManager`, em
`gerado/constantes.txt`.

O que segue é do motor. O estado "nenhum aparelho conectado" não pede nada que o motor não soubesse:
anunciar os domínios, responder a cada assinatura com o valor do perfil (ou com o padrão, marcado
como válido), responder a um pedido com uma mensagem só e gravar num atributo o que um pedido
manda. O aparelho conectado pediu uma coisa a mais, que entrou no motor em 12/10/2026: **valor por
instância**, porque "conectado" só pode valer para o aparelho principal.

## Formato do perfil

É o do carro de mentira: seções `[Serviço.evento]` com `caminho = valor`, e a seção de um evento de
resposta, mesmo vazia, faz o pedido dele ser respondido; o ajuste `grava` e a seção de regra
`[Serviço.pedido -> evento]` fazem um pedido mudar um atributo. Está descrito em
[../veiculo/README.md](../veiculo/README.md). O que muda são os nomes, que aqui são os do
`gerado/esquema.txt` desta pasta. Para achar: `grep '^E DSIMobileEquipment ' gerado/esquema.txt`.

Uma forma a mais, que nasceu aqui e vale para qualquer perfil do motor: **a seção por instância**,
`[Serviço[N].evento]`. Ela vale só para quem assina o atributo pela instância `N` do serviço, e
vale inteira, no lugar da seção sem instância (as duas não se misturam campo a campo). As outras
instâncias seguem com a seção sem instância ou, sem ela, com o valor padrão. Só serve a atualização
de atributo: a resposta de um pedido já sai na instância do pedido, e seção de regra não leva
instância. O que um pedido grava (linhas `G` e regras) e o arquivo de estado continuam valendo para
a seção sem instância. `veiculostub -m` mostra as seções por instância à parte, com
"(só a instância N)" no fim da linha.

## Os serviços, e quem é dono de cada um

| Domínio | Serviços que o anúncio liga | No carro |
|---|---|---|
| 256, `PID_BT` (canal `0x100`) | `DSIBluetooth`, `DSIBluetoothLE`, `DSIBluetoothLEDataChannel`, `DSIObexAuthentication`, `DSIObjectPush`, `DSINFC`, `DSIMobileEquipment` (instâncias 0, 1 e 2), `DSIMobileEquipmentTopology`, `DSIMessaging`, `DSIMessagingServiceConfiguration`, `DSIAdb`, `DSIAdbList`, `DSIAdbEdit` e `DSIAdbUserProfile` (0 a 3 cada), `DSIAdbDataResolution`, `DSIAdbVCardExchange` | o processo do Bluetooth |
| 4096, `PID_DCMANAGER` (canal `0x1000`) | `DSIPhoneManager`, `DSISmartphoneIntegration` | o gerente de aparelhos |

Três coisas que o esquema mostra e que valem para quem escrever outro perfil:

- **O `DSIMobileEquipment` tem três instâncias, e a 2 não é do Bluetooth.** A 0 é o aparelho
  principal, a 1 o associado e a 2 a do CarPlay: os pedidos da 2 a camada da LG manda para o
  domínio 512 (`PID_CARPLAY`), o que o `gerado/relatorio.txt` traz, lido do código. O telefone de
  mentira responde às três pelo domínio 256, porque o barramento da bancada não leva o destino do
  pedido. Com "nenhum aparelho" nas três isso não se nota (medido: com a 2 sem resposta a tela é a
  mesma). O perfil do aparelho conectado usa seções da instância 0, e a 1 e a 2 seguem "nenhum
  aparelho conectado".
- **O gerente de aparelhos assina em varint comum**, e os serviços do domínio 256, em zigue-zague.
  O esquema diz qual, e o motor segue.
- **`DSISmartphoneIntegration` está no esquema pela metade.** Ele fala outro protocolo (a api vai
  dentro da mensagem), que o gerador não lê: os eventos saem sem mensagem e nada dele é
  respondido. É o serviço do CarPlay, do Android Auto e do MirrorLink.

## O que o perfil `sem-aparelho.txt` diz, e por quê

| O quê | Valor | O que a HMI faz com isso |
|---|---|---|
| estado do Bluetooth (`DSIBluetooth`, atributo 3) | 0, ligado, válido | liga a chave de recurso "isExtPhoneAvailable": a tecla Telefone passa a pedir a tela do telefone, e o bloco entra na grade. Com 2 (não funcional) ou 5 ela mostra "Telefone indisponível." (`AslTargetSystemFeaturesAndCoding.dsiBluetoothUpdateBTState`) |
| estado de ativação do aparelho (`DSIMobileEquipment`, atributo 4) | 1, não conectado, válido | com o Bluetooth ligado, a tela do telefone é `PHONE_NOT_CONNECTED`. O zero (INIT) ela ignora (`HsmPhonePhoneAvailability.setPrimaryActivationState`) |
| nível de bateria do aparelho (atributo 7) | 255, sem informação | nada. Com o zero do valor padrão, que é bateria vazia, ela abre o aviso de bateria do celular 13 s depois da partida (`TargetASLPhoneGetter`; visto na série `s`) |
| mudo do microfone (atributo 18) | 1, sem mudo | é o que ela pede na partida; o zero é mudo ligado |
| cinco respostas | resultado 0, "feito" | some o "DSI timeout occurred" de 10 s de cada um: visibilidade do Bluetooth, mudo do microfone, estatísticas de chamadas, idioma da agenda e suspensão da reconexão automática |
| ligar e desligar o Bluetooth (`requestSwitchBTState`) | o estado pedido vai para o atributo 3, e a resposta diz "feito" | a caixa "Bluetooth" muda; desligado (1), a tela do telefone é `PHONE_INIT_BT_SWITCHED_OFF`, "Ligar o Bluetooth.", e o botão dela liga de novo. Sem a resposta a HMI ficava até 60 s no aviso "Aguarde…" |
| `grava = DSIBluetooth` | o que a tela grava nesse serviço é aceito | além do estado: o nome da central no Bluetooth (o que for digitado volta para a tela) e o modo de visibilidade, pelas regras `G` do esquema |
| todo o resto | zero, lista vazia ou texto vazio, válido | listas de chamadas, favoritos, contatos, mensagens e aparelhos conhecidos vazios |

## O que o perfil `aparelho-conectado.txt` diz, e por quê

Tudo aqui é simulação, e os textos são fictícios. As seções do aparelho são da instância 0.

| O quê | Valor | O que a HMI faz com isso |
|---|---|---|
| estado de ativação do aparelho principal (`DSIMobileEquipment[0]`, atributo 4) | 5, ligado e pronto (`ACTIVATIONSTATE_PHONE_ON`), em modo 3, mãos-livres por Bluetooth (`TELMODE_HFP`) | com o Bluetooth ligado, a tecla Telefone abre a tela principal, `PHONE_MAIN_NOCALLS` |
| dados do aparelho (atributo 20) | o nome "Celular fictício" e o endereço de exemplo `00:00:00:00:00:01`, os dois marcados como disponíveis; IMSI, cartão SIM e IMEI marcados como "não há" | a HMI guarda o endereço como o do aparelho principal e é por ele que acha o aparelho na lista do gerente (`HsmPhonePhoneAvailability`, `TargetASLPhoneConnectionManager.updatePhoneSelectionData`) |
| rede (atributo 24) | 1, registrado, na operadora "Fictícia" | o texto grande da tela principal. Nome com mais de uns doze caracteres sai cortado (visto na rodada `x1`) |
| sinal (atributo 27) e bateria (atributo 7) | 3 e 4 (60 a 79 %) | os ícones da barra do alto. A bateria não pode ficar no zero, que é "vazia" e abre o aviso de bateria |
| lista de aparelhos conhecidos (`DSIPhoneManager`, atributo 2) | um aparelho: o mesmo endereço, o mesmo nome, o identificador inventado `aparelho-ficticio-1` e o mãos-livres suportado e conectado (10 = `SERVICESTATE_SUPPORTED` + `SERVICESTATE_CONNECTED`; até as 9h de 12/10/2026 era só o 8, que a HMI também mostrava como conectado) | a caixa com o nome na tela principal, o ícone de Bluetooth na barra do alto e a linha de "Celulares conhecidos". Sem a lista a caixa fica vazia e a tela diz "Nenhum celular conhecido disponível." com o aparelho conectado (rodada `kp`) |
| resposta ao pedido de aparelho prioritário (`requestSetPrioritizedDevice`) | resultado 0, "feito" | com o aparelho na lista a HMI faz o pedido aos 20 s da partida e espera 10 s; ela só anota o resultado (rodada `k2`, onde faltou a resposta e ficou um "DSI timeout occurred") |
| os outros serviços do aparelho na lista | zero, que aqui é "não suportado" | o aparelho fictício não tem música por Bluetooth, agenda, mensagens, CarPlay nem Android Auto |
| a agenda "pronta" (`DSIAdb`, atributo 1) | 2, `ADBSTATE_READY` (o zero do padrão é "indefinida") | **sem isto a HMI não disca**: a máquina de chamadas dela fica à espera da agenda (`StateCallHandlingWaitForAdb`) e nem trata o pedido. Com o modelo, vale só com o aparelho conectado |
| um perfil de agenda ativo (`DSIAdb`, atributo 2, e `DSIAdbUserProfile`, atributo 5) | o perfil 0, "Perfil fictício", ligado ao aparelho pelo endereço de exemplo; nenhum contato | **sem isto a HMI também não disca**: o número do perfil ativo fica em -1, a busca de contatos que ela faz a cada dígito estoura e o número digitado não é guardado; a tecla verde acha o campo vazio (rodada `ct`). Também só com o aparelho conectado |
| cinco respostas a mais | "feito" | com a agenda pronta a HMI pede para usar o perfil (agenda e mensagens), o estilo da lista de contatos e uma janela dela; e, durante a chamada, o teclado manda tons (`sendDTMF`). Sem resposta, cada um deixava um "DSI timeout occurred" |

## Limites

- **O aparelho conectado não faz nada.** Com `aparelho-conectado.txt` a tela principal abre, e as
  de dentro também (discar, chamadas, favoritos, configurações), todas vazias. Não há chamada,
  discagem, agenda nem mensagem: a tecla verde do teclado, "Importar contatos novamente" e
  qualquer pedido de chamada ficam sem resposta, e não os exercitei.
- **Conectar e desconectar é instantâneo e sempre dá certo.** Não há os estados "conectando" e
  "desconectando", nem falha, nem pedido de confirmação no aparelho. O aviso "Estabelecendo
  conexão…" fica 2 s porque a HMI o segura esse tempo.
- **A tela não tem "desconectar".** Nesta variante o aparelho conectado é uma linha de informação
  em "Celulares conhecidos" (toque curto e longo não fazem nada; medido). O caminho da própria
  tela para soltar o aparelho é desligar o Bluetooth. Trocar por outro aparelho não dá: só há um.
- **Ligar o Bluetooth de novo não reconecta sozinho.** Num carro o aparelho voltaria; aqui ele
  fica conhecido e não conectado, para a tela conectar.
- **O que a tela muda vale até a central desligar:** a central seguinte nasce do perfil.
- **Na partida a lista de aparelhos sai vazia por um a dois segundos.** A HMI estoura
  (`NullPointerException`) se a lista não vazia chega antes de ela guardar o endereço do aparelho;
  o modelo responde à assinatura com a lista vazia e manda a de verdade depois de as instâncias
  0 e 1 terem pedido os dados do aparelho (ou 6 s depois de assinada). Com `-n` e `-m` a lista
  sai como é. É remendo por tempo, não pela causa.
- **Sem CarPlay na adaptação o botão vermelho da tela de chamada não encerra a chamada.** É o
  caso do `CARPLAY=0` e das centrais de teste sem codificação. Ao tratar o toque a HMI consulta
  os estados de chamada do CarPlay e estoura (`NullPointerException` em
  `ASLCarplayServicesImpl.getCarPlayCallStates`); o pedido de encerrar nem sai, e a chamada acaba
  sozinha aos 20 s. Medido nas rodadas `h1`, `h3`, `r1` e `r3`. É defeito da HMI, que só aparece
  numa central sem App-Connect.
- **O CarPlay do padrão é dedução, não leitura do carro.** Que o Jetta tem CarPlay, a foto mostra;
  o valor do campo da adaptação (1) é palpite, e o Android Auto ficou de fora.
- **A tecla Telefone do volante, durante a chamada, não faz nada** (rodadas `h4`, `r2`, `r4` e
  `pv5`, com e sem CarPlay): nenhum pedido sai, a tela não muda e a HMI não registra erro. Não sei
  se é assim no carro.
- **A chamada é só o estado da tela.** Nada é discado, não há áudio, ninguém atende; o número que
  aparece é o que foi digitado. O contador da tela fica em 0:00. Não há espera, conferência nem
  lista de chamadas feitas (a chamada simulada não fica no histórico).
- **A chamada recebida só vem do número combinado**, depois de discá-lo. Não há toque de verdade;
  o botão do sino marca o toque como silenciado e não manda pedido nenhum ao telefone. Com a
  central em outra tela (medido no rádio) a chamada entra como um aviso no alto, com os mesmos
  três botões; a tecla Telefone do volante não a atende nem recusa. No modo ao vivo cada chamada
  recebida deixa uma exceção do toque, porque a bancada não tem o tocador de sons.
- **Com uma chamada em curso, discar de novo** abre o aviso da própria HMI de linha ocupada.
- ~~Desligar o Bluetooth pela tela com o aparelho conectado deixa o estado incoerente~~: era assim
  até as 9h de 12/10/2026 (rodada `kf`: o nome e os ícones do aparelho continuavam na tela). Com
  o modelo o aparelho cai junto com o Bluetooth, e a tela "Ligar o Bluetooth." fica sem nome e sem
  ícones (rodada `m1`).
- **A seção por instância não recebe o que um pedido grava.** O que a tela grava num atributo e o
  estado guardado valem para a seção sem instância. Aqui não aparece: nenhum pedido grava nos
  atributos do aparelho.
- **Uma linha a mais no registro da HMI:** com o aparelho conectado ela registra, aos 12 s da
  partida, que uma resposta do teclado (`genericSettingResponse`) é inesperada e a descarta. O que
  ela pediu e o que recebeu é igual ao do primeiro estado; a causa não foi achada.
- **Sem o segundo perfil há dois estados,** "nenhum aparelho conectado" e "Bluetooth desligado", e
  a tela troca entre eles.
- **O que a tela muda vale até a central desligar.** O Bluetooth volta ligado e o nome volta vazio
  na partida seguinte: o telefone de mentira nasce sempre do perfil.
- **O pareamento é o pedido pelo aparelho, e só o do segundo aparelho fictício.** Esta variante não
  busca aparelhos pela tela: ao abrir "Celulares conhecidos" a HMI só põe o Bluetooth em modo
  acessível, e nenhuma tela dela dispara a busca. O segundo aparelho, pareado, fica na lista e não
  conecta: tocar nele dá erro. A lista de aparelhos de confiança do próprio Bluetooth (atributo 6)
  a HMI não assina, e o modelo não a preenche. O pareamento não fica guardado de uma sessão para
  a outra.
- **A caixa "Visibilidade" não marca.** A HMI pede o modo visível ao abrir a tela e ao tocar na
  caixa; o modo fica gravado e a resposta diz "feito", mas a caixa segue o outro campo do atributo
  (`visible`), que nenhum pedido muda.
- **O nome da central no Bluetooth nasce vazio.** Não inventei um; o que for digitado na tela fica.
- **Ligar e desligar é instantâneo**, sem os estados "ligando" e "desligando" de um rádio de verdade.
- **A tecla Telefone do volante não abre a tela.** A HMI recebe a tecla (`HK_MFL_PHONE_*`) e não
  faz nada com ela nesta variante, com ou sem o telefone de mentira.
- **A instância 2 do `DSIMobileEquipment` é respondida pelo domínio errado** (ver acima).
- **CarPlay, Android Auto e MirrorLink ficam de fora** (`DSISmartphoneIntegration`, domínio 512).
- **Os eventos de mídia por Bluetooth não são deste esquema.** O despacho do domínio 256 entrega
  também as interfaces de mídia, de UPnP e de busca; os donos desses serviços são outros.
- **O esquema é do MU0883.** O carro roda o MU0803; os números podem diferir.
- **Só no container.** Não rodou no QEMU.

## De onde sai cada informação

| Informação | Fonte |
|---|---|
| os dois domínios e os serviços que cada anúncio liga | bytecode de `com.lge.mib3oi.dsi.ListenerWorkerThread` (`run`, `processBTListenerService` e `processDCMManagerListenerService`) |
| api de cada evento, mensagem e atributo | o `switch` de `executeListenerService` de cada `…ListenerService`; na agenda e nas mensagens o atributo sai de `getUpdateListenerList(<…Impl>, atributo)` |
| api e interface de cada pedido | as chamadas a `postDSIProviderMsg` de cada `…Impl`; no `DSIMobileEquipment`, as chamadas ao método auxiliar `messageToBytes(api, mensagem)` |
| para que domínio vai cada instância do `DSIMobileEquipment` | os desvios de `messageToBytes`, seguidos pelo gerador |
| campos e codificação de cada mensagem | as classes protobuf do firmware, por reflexão, com a codificação provada pelo `Builder` |
| que pedidos têm resposta, qual é ela e o prazo | a tabela do proxy DSI da própria HMI (como no Marco 9) |
| o que decide a tecla Telefone e a tela "sem aparelho" | código da HMI, lido nos decompilados: `AslTargetSystemFeaturesAndCoding`, `SystemActivityImpl` (transições 12 e 13 e a chave de recurso 894), `HsmPhonePhoneAvailability` e `PhoneActivityImpl`; conferido nas rodadas `e1`, `e1b` e `e2` |
| o que a tela principal lê com um aparelho conectado | os nomes dos estados, em `gerado/constantes.txt`; de onde sai o nome do aparelho e como a HMI o liga à lista do gerente, em `HsmPhonePhoneAvailability` e `TargetASLPhoneConnectionManager` (decompilados); conferido nas rodadas `x1`, `kp` e `k2` e na série `k` |
