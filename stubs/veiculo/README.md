# stubs/veiculo — os dados de veículo que a bancada entrega à HMI

Marco 8, 11/10/2026. No carro, a central recebe do resto do carro o relógio, o computador de bordo,
os estados (parado, freio, marcha), as unidades e o clima. Quem traduz isso para a HMI é um
programa do firmware, o `carifservice`. Na bancada não há carro: o `driver/veiculostub.c` (o "carro
de mentira") ocupa o lugar dele e publica o que estiver nos arquivos desta pasta.

Desde o Marco 9 (11/10/2026) ele também atende pedidos da tela: acertar a hora, a data e o fuso
e trocar as unidades em "Configurações do veículo" passa a ter efeito.

Desde 12/10/2026, no modo ao vivo, o que a tela grava nele sobrevive à partida seguinte, como no
carro, em que quem guarda a hora e as unidades é o painel: fica em
`ao-vivo/memoria/veiculo/estado.txt`. Está na seção "O estado", abaixo. **Mudou:** aqui estava
"até a central desligar"; isso continua valendo só para as centrais de teste e para o lançador
direto.

**É estudo num computador. Nada daqui vai para o carro.** O que é do Jetta está separado em dois
arquivos: o que foi lido do carro e o que é palpite ou valor de simulação.

O registro da investigação está em [../../modelagem-soc.md](../../modelagem-soc.md), seções do Marco 8
(os dados) e do Marco 9 (os pedidos).

## Como usar

- **Modo ao vivo:** `sh driver/ao_vivo.sh` já sobe com o Jetta GLI 2021 parado. Lê três perfis, nesta
  ordem: `carro-parado.txt`, `jetta-gli-2021.txt` e `jetta-gli-2021-deducoes.txt`.
- **Para experimentar:** mude um valor num perfil (ou escreva um perfil novo e ponha no fim da
  lista) e reinicie a sessão. O perfil é lido na partida; mudar o arquivo com a central de pé não
  tem efeito. No modo ao vivo, o que a tela já gravou vale sobre o perfil: se o valor mudado for
  a hora ou uma unidade que alguém escolheu pela tela, apague antes o estado (seção "O estado")
  ou suba com `MEMORIA=`.
- **Sem o carro de mentira**, como até o Marco 7: `VEICULO= sh driver/ao_vivo.sh`. O relógio volta a
  "--:--" e a tela do veículo fica vazia.
- **Só o valor padrão**, sem perfil nenhum: `VEICULO=- sh driver/ao_vivo.sh`.
- **Outros perfis:** `VEICULO="/stubs/veiculo/carro-parado.txt /stubs/veiculo/meu.txt" sh driver/ao_vivo.sh`.
  Os caminhos são os de dentro do container; a pasta `stubs/` aparece lá como `/stubs`.
- **No lançador direto** (`driver/m1_hmi.sh`) a opção é a mesma variável `VEICULO`, e vem desligada.
- **Numa central de teste**, sem tocar na central ao vivo: `sh driver/m8_central.sh sobe <nome> <pasta>`.
  Ela não guarda o que a tela grava, salvo pedido (seção "O estado").
- **Voltar o carro de mentira ao zero** no modo ao vivo (hora do Mac, unidades do perfil): com a
  central desligada, `rm -rf ao-vivo/memoria/veiculo`.
- **Ver o que um perfil publicaria**, sem subir nada (dentro do container):
  `veiculostub -m /stubs/veiculo/gerado/esquema.txt <perfil>...`
- Linha errada em qualquer perfil: nada sobe, e a mensagem diz o arquivo e a linha.
- **Ver o que a tela pediu e o que foi feito:** o registro do stub (`veiculostub.log`) traz uma
  linha por pedido, "pedido atendido" (com o que foi gravado ou respondido) ou "pedido sem
  resposta" (com o motivo). Numa central de teste:
  `docker exec <container> cat /mnt/low/tmp/veiculostub.log`.
- **Desligar a gravação** (a tela volta a aceitar o toque sem mudar nada, como até o Marco 8): um
  perfil a mais, no fim da lista, com a linha `grava =` (vazia). As listas do start-stop
  continuam respondidas; para tirá-las também, é apagar as três seções do `carro-parado.txt`.

Os menus do carro dependem também da codificação: as funções têm de estar ligadas nas duas listas
da adaptação (`0B1B` e `0B1D`), o que está em
[../codificacao/jetta-gli-2021-deducoes.txt](../codificacao/jetta-gli-2021-deducoes.txt). Sem isso
a HMI recebe os dados e não cria os menus.

## Os arquivos

| Arquivo | O que é | De onde veio |
|---|---|---|
| `carro-parado.txt` | o cenário: hora e data de verdade no fuso de Brasília, carro parado, freio de estacionamento puxado. Vale para qualquer carro. Desde o Marco 9 diz também o que o carro de mentira faz com os pedidos: aceita o que a tela grava no relógio e nas unidades, ignora o idioma do painel e responde às três listas do start-stop com listas vazias. O que a tela grava não vem para este arquivo: fica no arquivo de estado (seção "O estado") | escolha do simulador |
| `jetta-gli-2021.txt` | o que foi **lido do carro**: unidades (km, km/h, °C), relógio de 24 h, câmbio em P | a ficha e a foto do painel de 09/10/2026 |
| `jetta-gli-2021-deducoes.txt` | o que **não** foi lido: que menus e dados o painel oferece (PALPITE) e os números que a tela mostra (SIMULAÇÃO: autonomia, tanque). Cada linha diz qual dos dois | escolhidos nas rodadas do Marco 8 |
| `jetta-gli-2021-menus.txt` | os menus que o Lipe **disse de memória** que o carro tem (12/10/2026, 17h31): quatro de Veículo → Config. (luz, estacionar e manobrar, espelho e limpador, abrir e fechar) e três assistentes (ACC, Front Assist, detector de fadiga), cada um com uma opção só, e a lista `grava` com os serviços deles. Entra no padrão do modo ao vivo; `MENUS=0 sh driver/ao_vivo.sh` tira. Não é leitura do carro | as respostas do Lipe e a ficha; a escolha de cada linha é palpite, com a fonte ao lado |
| `catalogo-menus.txt` | **catálogo, não é o carro** (Marco 14, 12/10/2026): 262 opções de exibição em "existe" (estado 2), as de cinco menus de "Configurações do veículo" (pneus, luz, estacionar e manobrar, espelho e limpador, abrir e fechar) e as dos assistentes do bloco "Assistentes". Só opções, nenhum valor. **Não entra em partida nenhuma por padrão**; só na central de teste do `sh driver/m14_menus.sh` | gerado por `driver/m14_catalogo_menus.py` do `gerado/esquema.txt`. Não editar à mão |
| `catalogo-menus-grava.txt` | par do catálogo (12/10/2026): põe os serviços dos menus (conforto, luz, estacionamento e assistência) na lista `grava`, mais uma regra para um pedido que o esquema não liga a atributo. Com ele o que se marca nos menus do catálogo fica. Só a lista e a regra, nenhum valor. **Não entra em partida nenhuma por padrão**; só com `sh driver/m14_menus.sh grava` | escrito à mão; a regra é dedução pelo nome, conferida na tela |
| `gerado/esquema.txt` | o esquema dos 16 serviços de veículo: interfaces, números de api, 957 atributos, 1.119 mensagens de evento, 647 pedidos e os campos de 2.362 mensagens. Desde o Marco 9, também os 84 pedidos de que a HMI espera resposta (linhas `R`), o que 373 pedidos `set…` gravam (linhas `G`) e o que as respostas devolvem do pedido (linhas `X`). Não editar à mão | gerado por `driver/m8_esquema.sh` das classes do firmware; as linhas `G` e `X` são dedução do gerador |
| `gerado/constantes.txt` | o que cada número quer dizer (unidades, estados, tipos): 3.812 constantes | idem |
| `gerado/relatorio.txt` | contagens por serviço e as esquisitices do próprio firmware (atualização entregue pelo método errado, por exemplo) | idem |
| `gerado/vetores-perfil.txt` e `vetores-esperado.txt` | a prova do `veiculostub`: um perfil que dá valor a toda folha de todo evento e os bytes que as classes do firmware produzem para ele | idem |

Para refazer a pasta `gerado/` (só lê o firmware, sem rede; não rodar com uma central subindo,
porque ela copia o esquema na partida):

```bash
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v "$PWD/driver":/driver:ro -v "$PWD/stubs/veiculo/gerado":/out moi3-bancada:1 sh /driver/m8_esquema.sh
```

Com outras centrais no ar, o mais seguro é trocar `$PWD/stubs/veiculo/gerado` por uma pasta à
parte e depois trazer os arquivos com `mv`, um a um. Foi assim no Marco 9.

## Formato do perfil

```
# comentário
fuso = -3                                   ajuste, antes da primeira seção: horas a somar ao UTC
grava = DSICarTimeUnitsLanguage             ajuste: serviços em que o que a tela grava é aceito
ignora = Serviço.pedido                     ajuste: pedidos que ficam sem resposta de qualquer jeito
[DSICarTimeUnitsLanguage.updateClockTime]   seção: <serviço>.<evento>, com os nomes do esquema
time.hours = @hora                          <caminho> = <valor>; o caminho desce pelas mensagens com "."
lista[0].campo = 2                          campo repetido pede índice
[Serviço.pedido -> evento]                  seção de regra: o que o pedido grava além do que o esquema diz
time.timeZone = offset                      <caminho no evento> = <campo do pedido>, do mesmo tipo
```

- **Serviço, evento e campos** são os do `gerado/esquema.txt`: linha `E` para o evento, `M` e `C` para
  a mensagem e os campos dela. Para achar: `grep '^E DSICarKombi ' gerado/esquema.txt`.
- **Valores:** inteiro (decimal ou `0x`), `0`/`1`/`true`/`false`, decimal para `float` e `double`,
  texto entre aspas. O que cada número quer dizer está no `gerado/constantes.txt`.
- **Marcadores**, só em campos inteiros: `@hora`, `@minuto`, `@segundo`, `@dia`, `@mes`, `@ano` e
  `@ano2000` (o ano menos 2000, que é como a HMI recebe a data). A mensagem que tem marcador sai
  de novo toda vez que muda.
- **Marcadores que acompanham outro campo** (12/10/2026), também só em campos inteiros. O valor
  sai, a cada envio, do que outro evento do carro de mentira guarda naquela hora:
  `campo = @de Serviço.evento.campo` (o valor de agora desse campo) e
  `campo = @se Serviço.evento.campo V:X … senao Y` (X do primeiro par cujo V é o valor de agora; Y
  nos outros casos). É como os números do computador de bordo acompanham a unidade escolhida na
  tela: a tela grava a unidade, e no segundo seguinte a autonomia e as unidades saem de novo, uma
  vez. O bloco fica no fim do `jetta-gli-2021-deducoes.txt`, marcado como simulação: o que o
  painel de verdade faz não foi medido. O que acompanha não vai para o arquivo de estado, e um
  campo que acompanha outro, lido por um terceiro, vale zero.
- **O que não está no perfil** vai com o valor padrão: tudo zero, marcado como válido. Zero nem
  sempre é neutro: "parado = 0" quer dizer carro andando, por isso o `carro-parado.txt` existe.
- **Arquivo de depois vale sobre o de antes**, campo a campo.
- Evento cujo nome se repete no serviço leva `:<api>` no fim (`updateHUDConfigurationMode:117`).

### Os pedidos (Marco 9)

A HMI não só assina: ela manda pedidos. O esquema diz de que tipo é cada um, e o perfil diz o que
o carro de mentira faz com ele.

- **Pedido que grava** (`setTemperatureUnit`, `setClockTime`…): se o serviço está em `grava`, o
  valor toma o lugar do valor do perfil e a atualização do atributo volta para a HMI. O que cada
  pedido grava está nas linhas `G` do esquema:
  `grep '^G DSICarTimeUnitsLanguage ' gerado/esquema.txt`. `grava` leva um ou mais nomes de
  serviço; a linha de um perfil de depois troca a lista inteira, e vazia desliga.
- **Relógio:** o pedido que grava num campo com marcador (`@hora`, `@dia`…) acerta o relógio do
  carro de mentira, que segue andando dali. Não vira número fixo.
- **Pedido com resposta própria** (as listas do start-stop): a resposta sai se o perfil tiver a
  seção do evento dela, mesmo vazia. Leva os valores da seção e, onde ela não diz nada, o que as
  linhas `X` do esquema mandam devolver do pedido (o número da transação). Para saber que
  pedidos têm resposta e qual é: `grep '^R DSICarEco ' gerado/esquema.txt`.
- **Regra a mais**, em seção `[Serviço.pedido -> evento]`: serve para o que o esquema não deduz.
  A que existe hoje leva o fuso escolhido para dentro da mensagem da hora, que é de onde a tela
  o tira.
- **`ignora`** tira pedidos do atendimento, um por um (`Serviço.pedido`). Existe por causa do
  idioma do painel: atendido, ele pôs a HMI em laço em 1 de 9 partidas.
- **Corte de laço:** pedido atendido mais de 200 vezes em 10 s deixa de ser atendido até a
  central reiniciar, com aviso no registro do stub.
- Só o que o esquema liga é atendido. O resto fica no registro como "pedido sem resposta", com o
  motivo: a resposta que a HMI espera e o prazo, ou "a HMI não marca prazo de resposta para ele".

**Antes de pôr outro serviço em `grava`:** as linhas `G` são dedução do gerador, não leitura do
firmware. Conferir as do serviço (as da regra `nomes` primeiro), olhar as telas dele e repetir a
partida algumas vezes. Só o serviço de relógio e unidades foi conferido.

### O estado: o que a tela grava fica guardado (12/10/2026)

No carro quem guarda a hora acertada e as unidades escolhidas é o painel de instrumentos, e elas
voltam quando o carro é ligado de novo (isto é dedução, não foi medido no carro). O carro de
mentira faz o mesmo quando recebe um arquivo de estado: a cada pedido que muda alguma coisa ele
escreve ali o que a tela gravou, e na partida seguinte aplica isso por cima dos perfis.

| Onde | O que acontece |
|---|---|
| modo ao vivo (`sh driver/ao_vivo.sh`, a janela) | **ligado**, junto da memória da tela. O arquivo é `ao-vivo/memoria/veiculo/estado.txt` |
| `MEMORIA= sh driver/ao_vivo.sh`, ou `LSMVAL` passado à mão | desligado nessa sessão: o carro de mentira nasce dos perfis e nada é guardado |
| central de teste (`driver/m8_central.sh`) | desligado. `VEICULO_ESTADO=/live/memoria/veiculo/estado.txt sh driver/m8_central.sh sobe <nome> <pasta>` liga só o carro de mentira; `MEMORIA=/live/memoria …` liga as duas memórias, como no modo ao vivo. O estado fica em `<pasta>/memoria/`, e outra central subida na mesma `<pasta>` nasce dele |
| lançador direto (`driver/m1_hmi.sh`) | desligado; liga com `VEICULO_ESTADO=<arquivo>` |

Para o dia a dia, a partir da pasta da bancada:

| Para | Faça |
|---|---|
| ver o que está guardado | `cat ao-vivo/memoria/veiculo/estado.txt` |
| voltar o carro de mentira ao zero (hora do Mac, unidades do perfil) | com a central desligada: `rm -rf ao-vivo/memoria/veiculo` |
| voltar tudo ao zero, a tela e o carro de mentira | com a central desligada: `rm -rf ao-vivo/memoria` |
| uma sessão que não lê nem guarda nada | `MEMORIA= sh driver/ao_vivo.sh` |
| ver o que o stub fez | no registro dele (`veiculostub.log`): cada pedido que mudou o arquivo termina em "estado guardado", e a linha de resumo da partida diz "estado: N valor(es) e acerto de … s no relógio", "estado: nada guardado ainda" ou "estado: RECUSADO (…)" |

Com a central ligada, apagar ou trocar o arquivo não adianta: ela o escreve de novo na próxima
mudança, com tudo o que tem na memória dela.

**O arquivo** tem a forma de um perfil. Este é o de uma central em que acertei a hora e a data
e escolhi °F e o fuso −2 h pela tela (rodada `v2`):

```
# (quatro linhas de comentário, que o programa escreve)
acerto = 90081
[DSICarTimeUnitsLanguage.updateTemperatureUnit]
temperatureUnit = 1
[DSICarTimeUnitsLanguage.updateClockTimeZoneOffset]
clockTimeZoneOffset = -2
[DSICarTimeUnitsLanguage.updateClockTime]
time.timeZone = -2
fim = 3
```

- **`acerto`**: os segundos que o relógio está adiantado em relação à hora do Mac no `fuso` do
  perfil (negativo: atrasado). Fica a diferença, e não a hora: o relógio do painel segue andando
  com a central desligada, e este volta com a hora acertada mais o tempo que passou. A data usa o
  mesmo número: um dia adiante são 86.400 s a mais.
- **As seções e os valores** são os de um perfil, e só entra o que algum pedido gravou. Texto vai
  entre aspas, numa linha só, com `\"`, `\\` e `\n`.
- **`fim = <n>`**: a última linha, com a conta dos valores. Sem ela, ou com a conta errada, o
  arquivo é tido por cortado.
- **Não entram:** a hora como número, o que o perfil manda ignorar (o idioma do painel), o que
  vem de serviço fora de `grava` e as respostas das listas do start-stop.

**As regras:**

- O arquivo é escrito ao lado e trocado de nome: nunca fica pela metade, nem se o container
  morrer no meio. Pedido que repete o valor que já estava não regrava nada; a partida também não.
- Na partida o arquivo é conferido inteiro contra o esquema antes de qualquer valor ser aplicado.
  **Com erro em qualquer linha, ou cortado, nada dele vale e a central sobe do mesmo jeito**, só
  com os perfis: o motivo sai no registro, com o arquivo e a linha, e o arquivo vai para
  `estado.txt.recusado`. O próximo pedido que gravar começa um arquivo novo.
- O estado vale mesmo para serviço que saiu de `grava`: é a memória do painel. Para o carro de
  mentira esquecer, é apagar o arquivo.
- Valor guardado para um campo que o perfil de agora enche com marcador (`@hora`…) é descartado,
  com aviso. Só acontece quando o perfil muda de uma sessão para a outra.
- Uma central por pasta de memória: duas gravando na mesma pasta escreveriam uma por cima da outra.

**O custo de guardar a diferença:** o relógio do carro de mentira anda com o do Mac. Se o Mac for
corrigido ou trocado de hora, ele vai junto; se o `fuso` do perfil mudar, uma hora acertada à mão
desloca junto; e ele não atrasa nem adianta sozinho, como o relógio de um painel.

Registro e rodadas: [../../modelagem-soc.md](../../modelagem-soc.md), seção "O carro de mentira
lembra do que a tela gravou" (decisões D85 a D92).

### O catálogo dos menus: ver o que a HMI sabe mostrar (Marco 14, 12/10/2026)

Os outros menus do carro (pneus, luz, estacionar e manobrar, espelho e limpador, abrir e fechar,
e os assistentes) estão **desligados** no simulador: a ficha do carro não confirma nenhum dos
equipamentos deles. Para ver como a HMI os mostraria há um catálogo à parte:

```bash
sh driver/m14_menus.sh
```

Sobe a central de teste "menus" (`moi3-m8-menus`) com os perfis padrão e, depois deles, o
`catalogo-menus.txt` daqui e o par dele na codificação. Para olhar: `sh janela/abre.sh --container
moi3-m8-menus`; para encerrar: `sh driver/m14_menus.sh desce`. Os cinco menus ficam em Veículo >
Config.; os assistentes, no bloco "Assistentes" da grade de menus.

**O catálogo não é o carro.** Ele diz "existe" para todas as opções que o firmware conhece em
cada menu, inclusive as que o Jetta com certeza não tem, e não dá valor a nada: as caixas e as
listas aparecem no valor padrão do carro de mentira, que é zero.

Como um menu passa a existir (medido no Marco 14):

1. a função dele está na lista de funções do carro da adaptação (lado da codificação,
   [../codificacao/](../codificacao/README.md)); só então a HMI assina as opções;
2. pelo menos uma opção do menu chega com estado 2.

Uma opção de um serviço pode aparecer no menu de outro: a `turnIndRepeat` das travas é uma linha
da tela de luz, a `easyEntrySteeringColumn` do limpador é uma linha de abrir e fechar, e as
`exitAssist` e `rcta` do Side Assist são linhas de estacionar e manobrar.

**Para ligar um menu de verdade**, quando o equipamento se confirmar: copiar do catálogo para o
`jetta-gli-2021-deducoes.txt` só as opções que o carro tem, com a razão ao lado, pôr a função no
arquivo de deduções da codificação e rodar uma série de partidas antes de deixar no padrão.

O que o catálogo deixa de fora: a opção `intLightRGBColorList` da luz ambiente. Com ela a HMI
pede a lista de cores (`DSICarLight.requestIntLightRGBColorList`), e a resposta é de um tipo de
mensagem que o carro de mentira não monta: foram quatro ou cinco tempos esgotados de 10 s por
sessão nas rodadas que a ligaram. E, com o `sh driver/m14_menus.sh` simples, nada do que se
muda nesses menus fica: o ajuste `grava` do padrão só tem o serviço de relógio e unidades.
**Para mexer e ver ficar** (desde as 9h56 de 12/10/2026): `sh driver/m14_menus.sh grava`. Entra por
último o `catalogo-menus-grava.txt`, com os quatro serviços dos menus na lista `grava`, e o arquivo
de estado do carro de mentira fica ligado: a caixa marcada volta marcada na central seguinte.
Conferido com uma caixa de cada menu e dois assistentes, em oito partidas, sem laço. Dos 277
pedidos `set…` desses serviços, 221 têm no esquema o que gravam; os outros não mudam nada, salvo a
"ativação automática" do estacionamento, que ganhou regra no perfil.

## De onde sai cada informação do esquema

| Informação | Fonte no firmware (MU0883, HMI H40.272.304) |
|---|---|
| que serviços são do domínio 16 e a interface de eventos de cada um | bytecode de `com.lge.mib3oi.dsi.ListenerWorkerThread` (`run` e `processBAPListenerService`), lido com o `javap` da JVM do firmware |
| api de cada evento, mensagem que a HMI lê e atributo de entrega | o `switch` de `executeListenerService` de cada `…ListenerService` |
| api, interface e domínio de cada pedido | as chamadas a `postDSIProviderMsg` de cada `…Impl` |
| campos de cada mensagem | as constantes `…_FIELD_NUMBER` e os campos das classes protobuf (`GeneratedMessageLite`, sem descritor) |
| codificação de cada campo (zigue-zague ou não, repetido um a um ou empacotado) | **provada**: o programa monta a mensagem com o `Builder` do firmware, serializa e olha os bytes |
| nomes dos atributos e das constantes | as interfaces `org.dsi.ifc.*` do jar da HMI |
| que pedidos têm resposta, qual é ela, de que tipo e com que prazo (linhas `R`) | a tabela do proxy DSI da própria HMI: as chamadas de `de.vw.mib.asl.framework.internal.framework.dsi.util.Cfg` que o `DSIProxyTarget` usa para registrar "DSI timeout occurred", feitas por reflexão; a api de cada resposta vem do tratamento que a camada da LG dá a cada método do ouvinte |
| o que cada pedido `set…` grava (linhas `G`) e o que cada resposta devolve (linhas `X`) | **dedução do gerador**, pelo nome do atributo e pela forma das mensagens; o firmware não traz essa ligação |

Nada veio de decompilador: o programa (`driver/LeEsquemaDSI.java`) é compilado e roda na JVM do
firmware. Duas rodadas seguidas geraram arquivos idênticos, no Marco 8 e de novo no Marco 9.

## Limites

- **Dos pedidos, só o relógio, as unidades e as três listas do start-stop** (Marco 9). Até o
  Marco 8 nenhum pedido tinha resposta. Hoje a hora, a data, o fuso e as unidades gravados pela
  tela têm efeito, e as listas do start-stop voltam vazias. Os outros serviços aceitam o toque
  e nada muda; os outros 81 pedidos com prazo (restaurar ajustes de fábrica, por exemplo) seguem
  estourando o tempo no registro da HMI; e os pedidos sem prazo (a lista de avisos do veículo,
  a de perfis de condução, o estado BAP) seguem sem resposta.
- **O que a tela grava só sobrevive à partida no modo ao vivo** (desde 12/10/2026; seção "O
  estado"). Nas centrais de teste e no lançador direto o carro de mentira nasce de novo dos
  perfis e da hora do Mac, salvo pedido. **Mudou:** aqui estava "vale até a central desligar",
  para todos os casos.
- **Do estado, foram conferidos pela tela** a hora, a data, o fuso e três unidades (temperatura,
  distância e velocidade). A fonte de referência, os formatos e as outras unidades usam a mesma
  regra e não foram tocados. Não há cópias anteriores do estado, como há da memória da tela.
- **Trocar a unidade não refaz os números do painel.** A autonomia, a distância e o consumo
  seguem na unidade que o perfil dá. A temperatura do topo muda porque a HMI faz a conta.
- **O fuso escolhido não mexe na hora.** O que o painel do carro faz nesse caso não está no
  código da HMI.
- **O idioma do painel não segue o que a HMI pede.** Fica o do perfil de deduções.
- **O anúncio do domínio sai uma vez só**, e chega à HMI pela reentrega do `kipc_hub`. O motivo
  está no cabeçalho do `driver/veiculostub.c`: repetido de segundo em segundo, ele fazia o painel
  ficar mudo em parte das partidas.
- **Menus feitos:** computador de bordo, relógio, unidades e revisão. Luzes, espelhos, travas,
  pneus, assistentes e luz ambiente continuam fora: cada um pede opções e valores do serviço dele.
- **O assistente de configuração.** Com os serviços de veículo ligados a HMI avalia, uns 52 s
  depois da partida, se mostra o assistente "Bem-vindo". Ele aparece quando a memória de estado
  diz que há configuração inicial por fazer, que é o caso da memória guardada do modo ao vivo.
  Um toque em "Não exibir" fica gravado na memória e ele não volta (conferido em centrais de
  teste). Sem memória, como nas centrais de teste, ele não aparece.
- **O esquema é do MU0883.** O carro roda o MU0803; os números podem diferir.
- **A foto do painel não mostra tudo:** a unidade de consumo e as opções de menu do painel são
  palpite. O chassi do carro não é usado.
