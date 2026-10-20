# stubs/radio — o rádio de mentira da bancada

Marco 11, 12/10/2026. No carro, a tela do rádio conversa com o sintonizador por um programa do
firmware, o `radio-hmi`. Na bancada não há sintonizador nem antena, e esse programa não sobe. O
`driver/radiostub.c` (o "rádio de mentira") ocupa o lugar dele: anuncia o domínio do sintonizador,
entrega a lista de emissoras e a emissora atual que estiverem nos arquivos desta pasta, e atende
aos pedidos de sintonizar e de buscar emissora.

**É estudo num computador. Nada daqui vai para o carro.** E é simulação do começo ao fim: as
emissoras são inventadas, não há recepção e não sai som.

O registro da investigação está em [../../modelagem-soc.md](../../modelagem-soc.md), seção do
Marco 11.

## Como usar

- **Modo ao vivo:** `sh driver/ao_vivo.sh` (e o botão da janela) já sobe com o rádio de mentira e o
  perfil `fm-de-mentira.txt`. A tela do rádio abre no lugar do aviso "função indisponível".
- **Sem o rádio de mentira**, como até o Marco 9: `RADIO= sh driver/ao_vivo.sh`. A tela do rádio
  volta ao aviso.
- **Só o anúncio e os valores padrão**, sem perfil: `RADIO=- sh driver/ao_vivo.sh`. A tela abre, em
  87,5 MHz (ou 76,1 MHz com a faixa "Brasil"), com a lista de emissoras vazia.
- **Outro perfil:** `RADIO="/stubs/radio/fm-de-mentira.txt /stubs/radio/meu.txt" sh driver/ao_vivo.sh`.
  Os caminhos são os de dentro do container; o arquivo de depois vale sobre o de antes, campo a
  campo. O perfil é lido na partida.
- **No lançador direto** (`driver/m1_hmi.sh`) e **nas centrais de teste** (`driver/m8_central.sh`,
  que chama o `driver/m3_ao_vivo.sh`) a opção é a mesma variável `RADIO`, e vem **desligada**:
  `RADIO=/stubs/radio/fm-de-mentira.txt sh driver/m8_central.sh sobe <nome> <pasta>`.
- **Ver o que um perfil publicaria**, sem subir nada (dentro do container):
  `radiostub -m /stubs/radio/gerado/esquema.txt <perfil>...`
- **Ver o que a tela pediu e o que foi feito:** o registro (`radiostub.log`) traz uma linha por
  pedido. Numa central de teste: `docker exec <container> cat /mnt/low/tmp/radiostub.log`. As
  linhas `[radiostub]` são do modelo do sintonizador; as `[veiculostub]` são do motor, que é o
  mesmo do carro de mentira.
- Linha errada no perfil, ou esquema que não é o do sintonizador: nada sobe, e a mensagem diz o
  arquivo e a linha, ou o que faltou.

## Os arquivos

| Arquivo | O que é | De onde veio |
|---|---|---|
| `fm-de-mentira.txt` | o perfil: cinco emissoras de FM inventadas (89,1, 93,7, 98,5, 102,9 e 105,3 MHz), três com nome ("TESTE 1" a "TESTE 3") e duas sem; desde 12/10/2026 (Marco 16), três de AM (594, 1008 e 1305 kHz) e um texto de rádio ("TEXTO DE RÁDIO DE TESTE", que acompanha a "TESTE 1" e a "TESTE 3"); a emissora em que o sintonizador diz estar ao ligar; som analógico; e a regra do uso do rádio | **SIMULAÇÃO**: nenhum valor lido do carro nem de um sintonizador |
| `gerado/esquema.txt` | o esquema dos três serviços do sintonizador (`DSIUnifiedTuner`, `DSITunerAnnouncement` e `DSIRadioData`): interfaces, números de api, 27 atributos, 53 mensagens de evento, 43 pedidos e os campos de 115 mensagens. Não editar à mão | gerado por `driver/m8_esquema.sh` das classes do firmware |
| `gerado/constantes.txt` | o que cada número quer dizer (faixas, modos, estados): 453 constantes | idem |
| `gerado/relatorio.txt` | contagens por serviço e as esquisitices do próprio firmware | idem |
| `gerado/vetores-perfil.txt` e `vetores-esperado.txt` | a prova do codificador: um perfil que dá valor a toda folha de todo evento e os bytes que as classes do firmware produzem para ele (53 mensagens) | idem |

Para refazer a pasta `gerado/` (só lê o firmware, sem rede):

```bash
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v "$PWD/driver":/driver:ro -v "$PWD/stubs/radio/gerado":/out -e DOMINIOS=0x400000000000 -e ROTULO="do sintonizador" moi3-bancada:1 sh /driver/m8_esquema.sh
```

Com outras centrais no ar, o mais seguro é trocar `$PWD/stubs/radio/gerado` por uma pasta à parte e
depois trazer os arquivos com `mv`, um a um.

## Formato do perfil

É o do carro de mentira: ajustes (`grava`, `ignora`), seções `[Serviço.evento]` com
`caminho = valor`, e seções de regra `[Serviço.pedido -> evento]`. Está descrito em
[../veiculo/README.md](../veiculo/README.md). O que muda são os nomes, que aqui são os do
`gerado/esquema.txt` desta pasta. Para achar: `grep '^E DSIUnifiedTuner ' gerado/esquema.txt`.

Três coisas que este perfil precisa respeitar:

- **A frequência vai em kHz**: 89,1 MHz é `89100`.
- **`max` tem de ser o número de emissoras da lista.** A camada da LG lê "max" emissoras de cada
  mensagem (`UnifiedListControl.AddList`) e só entrega a lista à HMI quando a conta fecha. Pelo
  código, com `max` menor a lista sairia cortada e com `max` maior não sairia; os dois casos de
  erro não foram medidos. O teste confere que o perfil da pasta tem os dois números iguais.
- **Para a central mostrar o nome de uma emissora** são precisas três coisas juntas: o nome no
  `longName`, `rds = 1` e um `piSId` diferente de `-1`. Faltando uma, a lista mostra a frequência
  (medido na rodada `c3`). O teste `driver/tests/t_radio.sh` confere isso no perfil da pasta.

Desde 12/10/2026 (Marco 16), mais duas:

- **As emissoras de AM vão na mesma lista**, com `waveBand = 1`, depois das de FM. A HMI recebe a
  lista de cada faixa numa mensagem à parte; quem separa é o `radiostub`, na partida, e havendo
  alguma de AM ele acerta o `max` das duas listas. O `max` do perfil continua sendo o das de FM.
  As frequências de AM têm de cair na grade que a HMI usa (ver "A faixa de FM não está aqui").
- **O texto de rádio** é a seção `[DSIUnifiedTuner.updateRadioText]`, com `radioText.radioText`
  (o texto) e `radioText.source = 1` (FM). O PI não vai no perfil: o `radiostub` põe o da
  emissora de agora, porque a HMI só aceita texto com o PI dela. Recebe o texto a emissora que
  tem `radioText = 1` na lista, e ela precisa ter PI (o teste confere).

## O que o rádio de mentira faz

| A HMI | O rádio de mentira | Quem faz |
|---|---|---|
| assina atributos (emissora selecionada, lista, estado do som, uso do rádio…) | manda cada um com o valor do perfil ou, sem perfil, o padrão marcado como válido | o motor |
| `switchRadioUsage(1)`: "vou usar o rádio" | grava e devolve o atributo de uso do rádio. A HMI só manda sintonizar depois disso | o motor, pela regra do perfil |
| `selectStation(modo, frequência, PI…)`: toque na lista, numa memória, na escala da sintonia manual, troca de faixa | responde "em curso", manda a emissora selecionada e responde "feito". Se a frequência está na lista do perfil, vai a emissora inteira, com nome; se não, só a faixa, a frequência e o PI do pedido | o modelo |
| `seekStation(acima ou abaixo, faixa, sessão)`: as setas, na tela de sintonia manual | responde "em curso", **varre a faixa** até a emissora seguinte da lista (Marco 16): uma atualização por canal do caminho, só com a frequência, a cada 25 ms, dando a volta pela ponta da faixa se for preciso; na chegada manda a emissora inteira e responde "feito". Na tela a frequência corre e o ponto da escala anda. Caminho com mais de 40 canais pula canais; sem emissora na faixa, dá a volta inteira e para onde estava | o modelo |
| depois de cada sintonia e de cada busca | o **texto de rádio** (Marco 16): o do perfil, com o PI da emissora de agora, se ela tem `radioText = 1` na lista; senão, o texto vazio. A tela principal mostra o texto debaixo do nome da emissora. Perfil sem a seção `[DSIUnifiedTuner.updateRadioText]`: nada é mandado | o modelo |
| assina a lista de emissoras, e depois entra em AM | a **lista de AM** (Marco 16) vai numa mensagem à parte, com o tipo de faixa 0: o modelo tira as emissoras de AM da lista do perfil na partida, e as manda no primeiro pedido que atende, cada vez que a HMI entra em AM e quando ela pede a atualização da lista de AM (o botão de atualizar da "Lista de emissoras AM") | o modelo |
| `seekStation(interromper ou parar)` | responde "interrompida" ou "parada"; a emissora não muda | o modelo |
| `forceUpdateStationList(começar, faixa)`: "Buscar e salvar", na página "Lista de memória" do assistente de configuração | responde "em curso", manda a lista de novo e responde "feito". A HMI guarda então as emissoras da lista nas memórias; sem a resposta ela ficaria 65 s em "Procurando emissoras…" | o modelo |
| os outros pedidos (`setFilter`, `setRegMode`, `setListMode`, `isOnPreset`…) | ficam registrados, sem resposta. A HMI não marca prazo para nenhum dos que mandou nas rodadas | o motor |

**A central abre numa frequência, e não numa emissora.** Quem escolhe a frequência de partida é a
HMI: a que estiver na memória dela (leitura do código; nas rodadas não houve memória de rádio) ou,
sem ela, o começo da faixa (76,1 MHz com a faixa "Brasil", 87,5 MHz com a da Europa; medido). Ela manda sintonizar e o rádio de mentira obedece; a
emissora "selecionada" do perfil (89,1 MHz, "TESTE 1") só vale até esse primeiro pedido. Enquanto
a central está numa frequência que não é de emissora da lista, a "Lista de emissoras FM" mostra
uma linha a mais, com essa frequência: é a HMI que a põe (`AmFmStationList.generateFrozenFmList`).
Reaberta a lista já numa emissora do perfil, ficam as cinco (medido na rodada `g1`).

As memórias ("Lista mem."), a ordem da lista, a escala da sintonia manual e a troca entre AM e FM
são da própria HMI: ela guarda as memórias, monta a lista com o que recebeu e só avisa o
sintonizador. As setas da tela das memórias e as teclas Próxima e Anterior (da moldura e do
volante) também são dela: ela escolhe a emissora seguinte da lista ou das memórias e manda
sintonizar.

## A faixa de FM não está aqui

Os limites da faixa e o passo da sintonia manual a HMI não pergunta ao sintonizador: tira da
adaptação do 5F (DID `0559`, "Bandsettings FM Tuner"). Sem ajuste ela usa a faixa da Europa, de
87,5 a 108,0 MHz em passos de 0,1. Com o valor 6, "Brasil", usa de 76,1 a 107,9 MHz em passos de
0,2 (`RadioConstants.FM_BANDRANGE_BRA`, no código da HMI). O simulador sobe com o 6, por **palpite**
marcado em [../codificacao/jetta-gli-2021-deducoes.txt](../codificacao/jetta-gli-2021-deducoes.txt),
e as cinco emissoras do perfil caem em canal das duas faixas.

**Mas a busca precisa da grade, e ela está no programa** (desde 12/10/2026, Marco 16). Para varrer
a faixa, o `driver/radiostub.c` tem de saber onde ela começa, onde acaba e de quanto em quanto
anda, e a HMI não lhe diz. A tabela `GRADES` do programa traz as duas que a HMI usa na bancada:
FM de 76,1 a 107,9 MHz, de 200 em 200 kHz (a faixa "Brasil" do palpite acima), e AM de 531 a
1602 kHz, de 9 em 9 kHz (a da Europa, que a HMI assume porque a adaptação de AM da identidade
provisória está "sem ajuste": o campo "Bandsettings AM Tuner" do mesmo DID só tem Europa, América
do Norte e Japão, e não pus palpite). **Quem mudar a faixa na codificação muda a tabela junto**;
senão a busca corre por canais que a tela não tem. O teste confere as frequências do perfil contra
essas duas grades.

## Limites

- **Trocas seguidas de emissora chegam ao sintonizador com até 10 s de atraso.** A tela muda na
  hora. Medido na rodada `f3`: a HMI registra que há uma seleção em curso, e o pedido seguinte sai
  10 s depois da última troca, por um temporizador dela (`GlobalStationSelectionManger`); os do
  meio são pulados. **A causa é leitura do código, não medida:** a HMI só dá a sintonia por
  terminada quando o serviço de áudio confirma a ligação do som, e a bancada não tem esse serviço
  (no registro dela: "AudioManagement not available yet"). Quando houver serviço de áudio na
  bancada, repetir a rodada confirma ou desmente. Sem som não se nota; vê-se no `radiostub.log`.
- **Sem recepção e sem som.** A lista é fixa; não há RDS que muda nem logotipo de emissora. **O
  texto de rádio é um só, parado** (desde 12/10/2026, Marco 16): aparece debaixo do nome das
  emissoras marcadas no perfil, e nas outras não aparece nada. No carro ele vem da emissora e muda
  o tempo todo.
- **Intensidade de sinal: não há o que mostrar.** A HMI guarda a qualidade de recepção de cada
  emissora (`receptionQuality`), mas não a põe em tela nenhuma: no código dela a única comparação
  por qualidade está numa classe que ninguém usa (`QualityPIFrequencyHDComparatorChain`), e não há
  item de tela do rádio ligado a esse número. É leitura do código, não medida; o perfil não traz
  esse campo.
- **AM tem três emissoras, sem nome** (desde 12/10/2026, Marco 16). A lista de AM as mostra pela
  frequência, a sintonia e a busca funcionam. **A grade de AM é a da Europa** (531 a 1602 kHz, de
  9 em 9), que é a que a HMI assume com a adaptação de AM "sem ajuste"; não há palpite para ela.
  Se o Jetta do Brasil tem AM, e com que grade, não foi lido de lugar nenhum.
- **A busca varre a faixa de mentira** (desde 12/10/2026, Marco 16): a frequência corre na tela
  até a emissora seguinte da lista, em 1 s no máximo. No carro ela para onde há sinal. **Enquanto
  varre, o programa não atende outro pedido:** se a HMI manda interromper no meio (um segundo toque
  na seta), a resposta só sai depois da chegada, e a emissora fica a da chegada (medido: a busca
  seguinte funciona). A busca "sem parar" (seta segurada na sintonia manual) faz o mesmo caminho
  e espera a HMI mandar parar.
- **Nada sobrevive à partida seguinte do lado do sintonizador.** A emissora atual e as memórias
  quem guarda é a HMI, na memória de estado; o rádio de mentira nasce sempre do perfil.
- **Os pedidos de ajuste ficam sem resposta** (filtro de avisos de trânsito, RDS regional,
  seguimento de emissora, texto de rádio). As telas de configuração abrem; se mudar um ajuste tem
  efeito, não foi conferido.
- **`DSIRadioData` (banco de emissoras e logotipos) só responde às assinaturas.** A HMI não fez
  nenhum pedido a ele nas rodadas.
- **Avisos de trânsito (TP/TA), alarme e DAB** não foram simulados.
- **O esquema é do MU0883.** O carro roda o MU0803; os números podem diferir.
- **Só no container.** Não rodou no QEMU.

## Para fazer o mesmo noutro domínio

Receita escrita em 12/10/2026, a pedido da sessão principal, para as frentes de mídia, telefone e
som. O rádio foi o segundo domínio servido pelo mesmo motor; o primeiro é o de veículo.

1. **Conferir o domínio e se o anúncio basta.** Em `com.lge.mib3oi.dsi.ListenerWorkerThread.run`,
   ver qual domínio leva ao tratamento do serviço: o nome em `DSIDomain` engana (o do sintonizador
   é `PID_TUNER_HMI`, e não `PID_TUNER`). Numa central de teste, mandar só o anúncio, uma vez
   (`dsi_manda <socket> <domínio> 9000 0 0 -`), e olhar a tela e o barramento (`dsi_escuta`): se a
   HMI passa a pedir nas interfaces do serviço, o domínio está certo.
2. **Gerar o esquema numa pasta à parte**, com o comando de "Os arquivos" e
   `-e DOMINIOS=<domínio> -e ROTULO="<nome>"`. Ler o `relatorio.txt`: quantos serviços saíram, os
   pedidos "sem api ou interface constantes" (forma do código que o gerador não lê) e os que vão
   para outro domínio conforme a instância. O gerador (`driver/LeEsquemaDSI.java`) segue duas
   formas de despacho da camada da LG; uma terceira sai com zero serviços ou com aviso.
3. **Ver o que a HMI pede e do que a tela depende**: o barramento de uma rodada só com o anúncio,
   o rastreamento dela (`HMILOG` com a chave do serviço) e o código da área em
   `de.vw.mib.asl.internal`. No rádio a tela só sintoniza depois de um atributo voltar com 1, e
   espera três mensagens em ordem para cada sintonia.
4. **Decidir quem responde.** Se bastam valores parados e regras de perfil
   (`[Serviço.pedido -> evento]`), é o próprio `veiculostub` com o outro esquema:
   `veiculostub <socket> <esquema> <perfil>...`, um processo por esquema. Se a HMI espera mensagens
   em ordem ou um valor calculado, um programa à parte no molde do `driver/radiostub.c`:
   `#define VEICULOSTUB_MODELO`, `#include "veiculostub.c"` e três funções. `modelo_inicia()` acha
   no esquema, pelo nome, tudo o que vai usar, e devolve -1 dizendo o que faltou;
   `modelo_trata(pedido)` diz se o pedido é do modelo; `modelo_pedido(pedido, instância, bytes, n)`
   responde. Do motor o `radiostub.c` usa `acha_svc`, `acha_campo`, `acha_no`, `no`, `clona`,
   `libera`, `atribui`, `publica`, `le_folha`, `folha`, `poe_msg`, `manda`, `agora` e `inteiro`, e
   as listas `evts`, `reqs` e `subs`. Os pedidos do modelo passam pelo `ignora` do perfil e pelo
   corte de laço do motor.
5. **Teste antes do código**: copiar o `driver/tests/t_radio.sh`. Ele confere linhas do esquema
   gerado, a prova do codificador (os vetores) e o modelo, com os bytes que a HMI manda de verdade,
   subindo um hub novo por cenário (o hub reentrega a quem se liga as 48 primeiras mensagens, e um
   segundo programa no mesmo hub atenderia de novo os pedidos antigos).
6. **Compilar**: uma linha no `driver/m1_build.sh`, no molde da do `radiostub`: se não compilar,
   avisa e segue, para um erro ali não parar a partida de todas as centrais.
7. **Lançador**: um bloco no `driver/m1_hmi.sh` no molde do "Rádio de mentira (Marco 11)": a
   variável da opção, a cópia do esquema e dos perfis para `$LOW/tmp/<nome>`, a conferência com
   `-n`, a partida do programa antes da HMI, o registro na lista de arquivos copiados no fim e uma
   linha de resumo. Depois, o nome da variável na lista do `driver/m8_central.sh` e, só se for
   para vir ligado no modo ao vivo, o padrão no `driver/ao_vivo.sh`.
8. **Série de partidas antes de instalar**, com fotos, e a bateria inteira da pasta antes e depois.
   Os roteiros são executados montados por centrais no ar: trocar arquivo só por `mv` de uma cópia
   ao lado, ou com a ferramenta de edição.
9. **Na série, conferir também o que não é do domínio novo** (acrescentado em 12/10/2026, às 4h20,
   depois de o rádio e a mídia terem tropeçado nisso). Um serviço a mais respondendo tira da partida
   da HMI a espera de 2 s por ele, e as etapas seguintes chegam mais cedo; foi assim que a central
   passou a nascer sem serviços de veículo. Em cada partida da série, olhar no registro da HMI as
   linhas `Required DSI services not registered` (nenhuma pode citar `DSICar…` nem outro serviço
   que a bancada serve) e contar as assinaturas no carro de mentira (`grep -c ': assinou'` no
   registro do `veiculostub`: 111, ou 112 se a central nasce na tela do veículo). E incluir na
   série uma partida nascida da memória que termina na tela do veículo. O modo ao vivo dá 6 s aos
   serviços DSI por causa disso; numa central de teste, o mesmo sai com
   `EXTRA_VMOPTIONS=-Dde.vw.mib.asl.startup.timeout.dsi=6000`. Medidas e explicação:
   [modelagem-soc.md](../../modelagem-soc.md), Marco 15, "A espera da partida pelos serviços de
   veículo".

## De onde sai cada informação

| Informação | Fonte |
|---|---|
| o domínio do sintonizador e os serviços que o anúncio liga | bytecode de `com.lge.mib3oi.dsi.ListenerWorkerThread` (`run` e `processTunnerListenerService`): o domínio é `PID_TUNER_HMI`, `0x400000000000` |
| interface de eventos de cada serviço e o método que recebe a mensagem | `com.lge.mib3oi.dsi.radio.DSITunerListenerService.executeListenerService(int)` |
| api de cada evento, mensagem e atributo | os dois `switch` de `UnifiedTunerMsgParse`, e os de `TunerAnnouncementMsgParse` e `RadioDataMsgParse` |
| api e interface de cada pedido | as chamadas a `postDSIProviderMsg` de `DSIUnifiedTunerImpl`, `DSITunerAnnouncementImpl` e `DSIRadioDataImpl` |
| campos e codificação de cada mensagem | as classes protobuf do firmware, por reflexão, com a codificação provada pelo `Builder` |
| a ordem das três mensagens na sintonia, na busca e na atualização da lista, e a espera pelo uso do rádio | código da HMI (`de.vw.mib.asl.internal.radio`: `StationSelectionManager`, `StateTopMainUnifiedRadio`, `UnifiedRadioBandSwitch`, `StateAmFmActiveSeek`, `StateAmFmActiveTargetSeek` e `InitialAutostore`), lido nos decompilados e conferido no barramento das rodadas `b2`, `c1`, `c2`, `c3` e `f3` |
| a regra do nome na lista | medida na rodada `c3` |
| faixas de FM e de AM | `de.vw.mib.asl.api.radio.RadioConstants` e `stubs/codificacao/tabela-campos.tsv` |
