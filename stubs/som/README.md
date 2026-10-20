# stubs/som — o som de mentira da bancada

De 12/10/2026. Faz a tela de som da central (grave, médio, agudo, balanço, fader), a barra de volume
e o mudo terem estado no simulador. **Não há áudio: é só o que a tela mostra.** É estudo num
computador; nada daqui vai para o carro.

O registro da investigação está em [../../modelagem-soc.md](../../modelagem-soc.md), Marco 13.

## Como usar

- **Modo ao vivo:** `sh driver/ao_vivo.sh` já sobe com ele. A tecla Som abre o equalizador; o botão
  de volume, as teclas Vol − e Vol + do volante e as teclas de mudo (a do volante e a da moldura)
  mexem no volume.
- **O que muda no rádio e na mídia de mentira com ele** (medido): as trocas de emissora chegam ao
  rádio de mentira na hora (sem o som, de três trocas seguidas chega uma, atrasada; na tela não se
  vê diferença); e a mídia de mentira **toca** (o botão vira pausa; o tempo segue em 0:00) e
  **volta com a faixa** depois de uma ida ao rádio (sem o som a faixa fica em pausa e a volta abre
  o tocador vazio). Com a mídia em pausa a HMI põe um alto-falante riscado na barra do topo.
- **O volante abre a barra de volume** (desde as 17h03 de 12/10/2026; SIMULAÇÃO pedida pelo Lipe): Vol +,
  Vol − e o rolete do volante abrem a barra na tela e mudam o valor. Vem ligado no perfil
  (`volante_abre_a_barra = sim`). Para desligar, um segundo perfil com a linha
  `volante_abre_a_barra = nao`: `SOM="/stubs/som/som-de-mentira.txt /stubs/som/meu.txt"`.
- **Sem ele**, como até 11/10: `SOM= sh driver/ao_vivo.sh`. A tela de som volta ao aviso "A
  configuração do som estará disponível novamente…", e a barra de volume abre sem ninguém para
  mudar o valor.
- **O que a tela muda fica guardado, no modo ao vivo** (desde a manhã de 12/10/2026): volume,
  agudos, médios, graves, balanço, fader e o mudo voltam na partida seguinte. Ficam em
  `ao-vivo/memoria/som/estado.txt`, que o `somstub` regrava a cada mudança. `MEMORIA= sh
  driver/ao_vivo.sh` sobe sem memória nenhuma; `SOM_ESTADO= sh driver/ao_vivo.sh` desliga só a do
  som. Para voltar ao perfil, com a central desligada: `rm -rf ao-vivo/memoria/som`.
- **Numa central de teste** não vem ligado: `SOM=/stubs/som/som-de-mentira.txt sh driver/m8_central.sh sobe <nome> <pasta>`.
  Os caminhos são os de dentro do container; a pasta `stubs/` aparece lá como `/stubs`. Uma central
  de teste não guarda nada, e nasce igual toda vez; para ela guardar, só a pedido:
  `SOM_ESTADO=/live/memoria/som/estado.txt` (ou `MEMORIA=/live/memoria`, que liga as três
  memórias). A pasta `<pasta>/memoria` vai montada, e outra central subida na mesma `<pasta>` nasce
  do que a anterior deixou.
- **Outro perfil:** `SOM="/stubs/som/som-de-mentira.txt /stubs/som/meu.txt"`. O de depois vale sobre o
  de antes, campo a campo. O perfil é lido na partida; mudar o arquivo com a central de pé não tem
  efeito. `SOM=-` liga o serviço sem perfil: a tela segue no aviso, porque sem saída de som ligada
  a HMI não libera ajuste nenhum.
- **Ver o estado**, com a central no ar: `docker exec <container> sh /driver/som_estado.sh`. Uma
  linha: volume, mudo, grave, médio, agudo, balanço, fader, fonte e foco de ajuste.
- **Ver o que foi feito:** `docker exec <container> cat /mnt/low/tmp/somstub.log`. Uma linha por
  controle criado, por mudança de volume, de mudo e de fonte.
- **Mexer no volume sem tecla** (para experimentar): `docker exec <container> sh /driver/som_controle.sh volumeIncrease 2`.
- Perfil com erro: nada sobe, e a mensagem diz o arquivo e a linha.

## Por que não é como o carro de mentira

O pedido era achar os serviços DSI de som e estender o esquema do `veiculostub`. O levantamento
mostrou outra coisa: **o som desta HMI não passa pelo barramento DSI.** A camada DSI da LG não tem
pacote de som nem de áudio. O componente de som da HMI fala com dois serviços RSI (HTTP e WebSocket
locais, os do Marco 4): `audiomanagement` e `waveplayer`. Tudo o que a tela de som, a barra de
volume e o mudo usam vem do `audiomanagement`.

| Peça | O que é | Na bancada |
|---|---|---|
| `/lge/app_ro/audiomgnt/audiomgnt` | o gerente de áudio de verdade do firmware. Serve o `audiomanagement` a partir da tabela de áudio (`audiotable/MIB3_Audiotable_MQB.xml`, versão v207) | sobe e serve a tabela inteira, mas na única rodada com a HMI, **75 s depois da partida dela, entrou no mudo de proteção de componentes** (a conexão `FCT_AMP_THEFTPROTECTION_MUTE`) e a HMI mostrou "Proteção de componentes está ativa." Isso é proteção antifurto: **não foi investigado nem será contornado**. Ele só é usado parado, sem HMI, para gerar os dados (abaixo) |
| `/lge/app_ro/bin/rsi-mockup` | o mock RSI da LG, o mesmo do Marco 4 | **é quem serve o `audiomanagement` aqui**, com os dados desta pasta. Guarda o que lhe gravam e avisa quem assina. Não tem regra nenhuma |
| `driver/somstub.c` | o som de mentira: um cliente do mock | põe o que falta: o perfil, as regras mínimas do gerente de áudio e o papel do `touch-app` |

## Os arquivos

| Arquivo | O que é | De onde veio |
|---|---|---|
| `gerado/audiomanagement/*.json` | o que o gerente de áudio do firmware serve em repouso: 1 objeto global, 124 conexões, 39 volumes, 49 conjuntos de som, 94 parâmetros, 31 predefinições, 18 saídas, 25 fontes e a lista de controles, vazia. Nomes, identificadores, ligações, faixas e valores de fábrica. Não editar à mão | gerado por `driver/som_tabela.sh`, que sobe o `audiomgnt` sozinho por alguns segundos e pede cada recurso pela ferramenta `rsi-send` do firmware |
| `gerado/relatorio.txt` | a versão da tabela de áudio, os resumos SHA-256 e a contagem por recurso | idem |
| `som-de-mentira.txt` | o perfil: o que no carro viria do amplificador e da última sessão. **SIMULAÇÃO**, comentada linha a linha | escolha do simulador; nenhum valor lido do carro |

Para refazer a pasta `gerado/` (só lê o firmware, sem rede; gerar numa pasta à parte e trazer os
arquivos com `mv`, porque uma central que esteja subindo copia os dados na partida):

```bash
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v "$PWD/driver":/driver:ro -v /tmp/som-gerado:/out moi3-bancada:1 sh /driver/som_tabela.sh
```

Duas gerações seguidas deram arquivos idênticos.

## O que vem do firmware e o que é simulação

| O quê | De onde |
|---|---|
| que o som é RSI, e o que a HMI assina e grava | as classes da HMI (`de.vw.mib.asl.internal.sound.*`), nos decompilados de 10/10, conferido nas rodadas com o registro dela |
| o volume de entretenimento de 0 a 30, nascendo em 5; grave, médio, agudo, balanço, fader e subwoofer de −9 a 9, nascendo em 0; os outros 38 volumes e os demais parâmetros | **firmware**: é o que o `audiomgnt` serve, lido da tabela de áudio |
| os nomes e identificadores das 124 conexões (`ENT_AMP_TUNER`, `FCT_AMP_USER_MUTE`…) | **firmware**, idem. A HMI tem a mesma tabela (`ConnectionData`) |
| a saída de som ligada, com um amplificador interno básico e três faixas de equalização | **SIMULAÇÃO**, no perfil. No carro quem informa é o lado do amplificador. A ficha traz "Som Beats, a confirmar": se for, a tela do carro é outra |
| a fonte que toca ao ligar (`ENT_AMP_TUNER`) | o que a HMI pede na partida (visto no registro dela); no carro é a última fonte usada |
| um passo de volume por passo do botão; zero liga o mudo; aumentar no mudo desliga | **SIMULAÇÃO** nas regras do `somstub`. A tabela de áudio descreve o caso real com mais ações (`onMuteByVolumeDecrease`, `onVolumeIncrease`, `limitVolume`), que não foram reproduzidas |
| quem cria o controle de volume quando o botão gira | no carro, o `touch-app` (o binário dele tem `/audiomanagement/controls`, `volumeUp_gui`, `volumeIncrease`, `hmi`, `cluster`). A HMI não cria: nela o evento do botão cai num `break`. Aqui o `somstub` ouve o botão no barramento e cria o controle |
| quem trata a tecla de mudo | não é a HMI: ela registra "ASL_SOUND_PIN_MUTE. Not handled by HMI anymore, supplier resposibility now". Que seja o `touch-app` é dedução; aqui é o `somstub` |

## Formato do perfil

```
# comentário (só em linha própria)
entretenimento = ENT_AMP_TUNER        antes das seções: a fonte que toca ao ligar (vazio = nenhuma)
[sinks.entertainment]                 seção: <recurso>.<nome do elemento>; * vale para todos
amplifierId = "VWBAS000"              <campo> = <valor em JSON>: número, true/false, "texto", [lista]
```

- Recurso, nome e campo são os de `gerado/audiomanagement/`. Para achar:
  `grep -o '"name":"[^"]*"' gerado/audiomanagement/volumesettings.json`.
- O campo tem de existir no elemento e o valor tem de ser do mesmo tipo do que está lá.
- O perfil não é gravado pelo serviço: o mock recusa gravação na saída de som (403). Antes de o
  mock subir, o `somstub -w` escreve os dados já com o perfil, e é isso que o mock lê.

## O que o somstub faz, e o que ele não faz

- **Volume.** Cada controle (`volumeIncrease`, `volumeDecrease`, com o número de passos) muda o
  volume da conexão que tem o foco de ajuste, dentro da faixa dele. Depois de atendido o controle é
  apagado. Com uma conexão de menu iniciada (o ajuste "Volume inicial máximo" usa
  `FCT_AMP_ONVOLUME_MENU`), o botão mexe no volume dela.
- **Mudo.** O controle `mute` liga e desliga a conexão `FCT_AMP_USER_MUTE`, que fica com o foco de
  ajuste enquanto está iniciada. Chegar a zero liga o mudo; aumentar no mudo desliga.
- **Fonte.** Quando a HMI inicia outra conexão de entretenimento, ela passa a ser a do objeto
  global, e a anterior para. É o que acontece na tecla Mídia (`ENT_AMP_MEDIA_PLAYER`) e na volta
  ao rádio (`ENT_AMP_TUNER`). O volume é o mesmo para as duas (`VolumeEntertainment`).
- **Botão e teclas.** Botão de volume da moldura (teclado 13, tecla 17) e rolete do volante (4/44):
  um controle por evento, com os passos. Vol + e Vol − do volante (4/42 e 4/43): um passo ao
  apertar. Mudo do volante (4/57) e da moldura (13/88): um controle de mudo ao apertar.
- **A especificação afrouxada.** O mock obedece à especificação do serviço que o firmware traz, e
  ela é mais estreita que o serviço de verdade em dois pontos. O lançador monta por cima uma cópia
  com os dois afrouxados (`somstub -s`), que só o mock lê: a assinatura de **recurso** dos conjuntos
  de som (é por ela que a HMI chega a grave, agudo e balanço; o gerente de verdade a atende, medido)
  e o apagamento de controle (para a lista não crescer; que o gerente de verdade os consuma é
  dedução).

Não faz:

- **Prioridades e ações cruzadas** da tabela de áudio (avisos de navegação que abaixam a música,
  telefone que pausa o rádio). Só há uma fonte de entretenimento, o mudo e as conexões de menu.
- **Guardar de uma partida para a outra, numa central de teste.** Nela o que a tela grava vale até
  a central desligar, salvo pedido. **Mudou em 12/10/2026, de manhã:** no modo ao vivo passou a
  guardar (ver "A memória", abaixo); até então esta linha dizia que nada era guardado em modo nenhum.
- **Segurar a tecla do volante** para o volume ir subindo: é um passo por aperto.
- **Conferir faixa do que a tela grava** em grave, agudo e balanço: o mock guarda o que vier. A
  HMI não manda valor fora da faixa.
- **O serviço de tons (`waveplayer`)**: segue ausente, e com ele o bipe de toque.

## A memória

No carro quem lembra do volume e do equalizador é o gerente de áudio. Aqui é um arquivo de texto,
com o formato do perfil, que o `somstub` regrava a cada mudança (ao lado, com troca de nome) e que,
na partida seguinte, entra por cima do perfil, antes de o mock subir (opção `-m` do `somstub`;
no lançador, `SOM_ESTADO`, que o modo ao vivo tira da `MEMORIA`).

```
# Memória do som de mentira (SIMULAÇÃO): o que a tela mudou e volta na partida seguinte.
mudo = desligado
[volumesettings.VolumeEntertainment]
value = 9
[soundparameters.EQ1]
value = 2
```

- **O que guarda:** o valor de qualquer volume (`volumesettings`) e de qualquer parâmetro de som
  (`soundparameters`: `EQ1` é graves, `EQ3` médios, `EQ5` agudos, `balance`, `fader`…) que tenha
  mudado, pelo botão, pelo volante ou pela tela; e o mudo do usuário. O que já estava guardado
  continua guardado, mude ou não na sessão.
- **O que não guarda:** a fonte que toca (quem lembra dela é a HMI), a saída de som e o
  amplificador (são do perfil), e as predefinições.
- **Arquivo estragado não impede a central de subir.** Linha que não serve (elemento que não
  existe, valor fora da faixa, outro recurso, texto solto) é ignorada, com um aviso no
  `somstub.log` que diz o arquivo e a linha; o resto entra. O lançador mostra quantas ignorou.
- **Dá para editar à mão**, com a central desligada. Com ela no ar o `somstub` regrava o arquivo
  na mudança seguinte, com o que ele tem na tabela.
- **É SIMULAÇÃO.** Se no carro o mudo volta ligado depois de desligar e ligar a ignição eu não
  sei; aqui volta. E a tela de som abre na aba em que foi deixada: essa parte é da memória da
  própria HMI (`lsmval`), não desta.

Verificado em 12/10/2026 numa central da pasta instalada, nascida da memória que a anterior
deixou: aos 40 s, antes de qualquer tecla, o serviço já servia volume 8, mudo ligado, graves 2,
agudos −1 e balanço 2; a tela de som mostrou os números, o alvo da posição e o alto-falante
riscado ([foto](../../quadros/2026-10-12-hmi-som-memoria-volta-na-partida-seguinte.png)).

## Limites

- **A barra de volume só abre com o giro do botão.** Vol + e Vol − do volante mudam o volume, mas
  não abrem a barra sozinhos: quem a abre é o evento de giro, dentro da HMI. Com a barra aberta
  elas a seguram e o valor acompanha. O que abre a barra no carro com a tecla do volante não foi
  achado. **Achado em 12/10/2026, de manhã:** a HMI tem uma chave de recurso para isso
  (`isMFLVolumePopup`, 271), e no firmware da bancada ela é constante e vem desligada: esta
  central não abre a barra pelo volante, de propósito; quem mostra o volume do volante é o painel
  de instrumentos, que a bancada não tem. Fazer a barra abrir seria inventar; ficou para o Lipe
  decidir (Marco 13, "A barra de volume pelo volante"). **Decidido e feito em 12/10/2026, à
  tarde:** o Lipe disse que no carro dele (MU0803) a barra aparece na central (de memória, sem
  foto) e escolheu simular. Com `volante_abre_a_barra = sim` no perfil, o `somstub`, ao ver a
  tecla de volume do volante, escreve `encoder 13 17 ±n` no arquivo de comandos do `dsistub`: gira
  também o botão da central, a HMI abre a barra como abre para o botão, e o volume muda um passo.
  É simulação por um caminho que não é o do carro; a HMI da bancada segue com a chave desligada.
  Sem a opção fica como a HMI faz sozinha: o volume muda e a barra não abre.
- **O amplificador é simulação.** `VWBAS000` não é identificador de verdade; as frequências das três
  faixas são inventadas.
- **Com o som a HMI desliga o substituto de áudio que tem dentro dela** ("DUMMY RSI AMT
  DEACTIVATED") e passa a pedir ao serviço a ligação de áudio de cada fonte. Rádio e mídia foram
  medidos assim, e melhoram; chamada de telefone não foi exercitada (não há aparelho).
- **A aba "Configuração" da tela de som e as predefinições** não foram exercitadas.
- **Os dados são do MU0883**; o carro roda o MU0803. A tabela de áudio pode diferir.
- **Só no container.** Não rodou no QEMU.
