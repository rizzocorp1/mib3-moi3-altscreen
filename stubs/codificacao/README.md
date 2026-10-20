# stubs/codificacao — a codificação e a adaptação que a bancada entrega à HMI

Marco 6, 11/10/2026. No carro, a central recebe do microcontrolador, logo na partida, os conjuntos
de **codificação**, **adaptação**, **registros** e **identificação**. Na bancada não há
microcontrolador: o `driver/diagstub.c` faz esse papel e publica o que estiver nos arquivos desta
pasta. **É estudo num computador. Nada daqui vai para o carro, e nenhum valor desta pasta é a
codificação do Jetta**, que só se conhece com o autoscan. Do carro há só a identificação que as
telas da central mostraram (número de peça, hardware e software), em `jetta-gli-2021.txt`.

O registro da investigação, com as rodadas e as previsões, está em
[../../modelagem-soc.md](../../modelagem-soc.md), seção do Marco 6.

## Como usar

- **Modo ao vivo:** `sh driver/ao_vivo.sh` sobe com a **identidade provisória do Jetta GLI 2021**
  (ver "O Jetta" abaixo). Foi decisão do Lipe em 11/10/2026: "pode deixar o Jetta como padrão do
  modo ao vivo". Ele lê quatro arquivos, nesta ordem: `padrao.txt`, `jetta-gli-2021.txt`,
  `jetta-gli-2021-deducoes.txt` e `ajustes.txt`.
- **Para experimentar:** tire o `#` de uma linha do [ajustes.txt](ajustes.txt), ou escreva uma
  linha nova nele, e reinicie a sessão. Como ele é lido por último, vale sobre os outros. A HMI só
  aceita esses dados na partida, antes da primeira leitura de persistência; mudar com ela de pé
  não tem efeito.
- **A central sem identidade**, com tudo no valor padrão e a tela de antes do Marco 6:
  `CODING="/stubs/codificacao/padrao.txt /stubs/codificacao/ajustes.txt" sh driver/ao_vivo.sh`
- **Sem codificação nenhuma**, como até o Marco 5: `CODING= sh driver/ao_vivo.sh`.
- **Outros arquivos:** `CODING="/stubs/codificacao/padrao.txt /stubs/codificacao/meu.txt" sh driver/ao_vivo.sh`.
  Os caminhos são os de dentro do container; a pasta `stubs/` aparece lá como `/stubs`.
- **No lançador direto** (`driver/m1_hmi.sh`) a opção é a mesma variável `CODING`, e vem desligada.
- Linha errada em qualquer arquivo: nada sobe, e a mensagem diz o arquivo e a linha.

## Os arquivos

| Arquivo | O que é | De onde veio |
|---|---|---|
| `padrao.txt` | os 48 conjuntos que a HMI procura na tabela dela, cada um com **todos os campos no valor padrão** | gerado por `driver/m6_tabela.sh` (programa `driver/LeCodificacao.java`), que lê os padrões das classes do próprio firmware. Não editar à mão |
| `ajustes.txt` | campos a mudar por cima de tudo, um por linha, com a fonte de cada um. Vem com tudo comentado | escrito aqui; os valores vêm da tabela abaixo |
| `jetta-gli-2021.txt` | o que se **sabe** do carro: a identificação que a tela "Informações do sistema" mostrou. Tem o lugar, vazio, da codificação e das adaptações, para quando houver autoscan | a ficha do carro (`carros/jetta-gli-2021/README.md`) |
| `jetta-gli-2021-deducoes.txt` | o que é **palpite**: tipo de veículo (sedã), cor de destaque vermelha e navegação; desde a tarde de 12/10/2026, por decisão do Lipe, também o CarPlay (`0555`, campo `ID_APPLE_DIO`), que a foto do carro mostra. Desde o Marco 8, também quatro funções do carro (computador de bordo, relógio, unidades e revisão) nas listas `0B1B` e `0B1D`: sem elas a HMI não cria os menus do carro, mesmo recebendo os dados de [../veiculo/](../veiculo/README.md). Desde o Marco 11 (12/10/2026), também a faixa de FM "Brasil" na adaptação de rádio (`0559 @1.4+4 = 6`): com ela a sintonia manual vai de 76,1 a 107,9 MHz, de 0,2 em 0,2, em vez de 87,5 a 108,0, de 0,1 em 0,1; apagar a linha volta à faixa da Europa (ver [../radio/](../radio/README.md)). Nenhum valor foi lido do carro; cada linha diz por que foi escolhida. Apagar quando houver autoscan | escolhidos nas rodadas `cod4`, `cod8a` e `cod3` para o simulador mostrar o que se vê no carro |
| `jetta-gli-2021-menus.txt` | as sete funções "Menu display" (`0B1B`) dos menus que o Lipe **disse de memória** que o carro tem (12/10/2026): luz, estacionamento, limpador, travamento, ACC, Front Assist e reconhecimento de cansaço. É o par do [../veiculo/jetta-gli-2021-menus.txt](../veiculo/jetta-gli-2021-menus.txt). Entra no padrão do modo ao vivo; `MENUS=0 sh driver/ao_vivo.sh` tira. Não é leitura do carro nem serve de codificação para ele | as respostas do Lipe; o byte de cada função é o da `tabela-campos.tsv`; a função escolhida para cada menu é palpite |
| `catalogo-menus.txt` | **catálogo, não é o carro** (Marco 14, 12/10/2026): as 13 funções "Menu display" da lista de funções do carro (`0B1B`) que fazem a HMI mostrar cinco menus de "Configurações do veículo" e os assistentes do bloco "Assistentes". **Não entra em partida nenhuma por padrão**; só na central de teste do `sh driver/m14_menus.sh`. É o par do [../veiculo/catalogo-menus.txt](../veiculo/catalogo-menus.txt) | gerado por `driver/m14_catalogo_menus.py`, com o byte de cada função tirado da `tabela-campos.tsv`. Não editar à mão |
| `sem-carplay.txt` | **desfaz a dedução do CarPlay** (telefone de mentira, Marco 12, 12/10/2026): uma linha, o campo `ID_APPLE_DIO` (DID `0555`, byte 1, bits 2 e 3) de volta no zero. Desde a tarde de 12/10/2026, por decisão do Lipe, o `jetta-gli-2021-deducoes.txt` marca esse campo com 1 (o carro tem CarPlay: foto de 09/10/2026; o valor é palpite, não leitura do 5F), o que faz a HMI subir a parte de integração de smartphone: a grade ganha "App-Connect", o botão vermelho da tela de chamada encerra a chamada simulada e a partida leva uns 6 s a mais. Este arquivo só entra com `CARPLAY=0 sh driver/ao_vivo.sh`, no fim da lista, e devolve a central de antes | escrito aqui; o campo vem da tabela abaixo (id 69), e o motivo, de `AdaptationSetter.updateCarPlay` |
| `tabela-campos.tsv` | **todos os 1.188 campos** que a HMI entende: conjunto, DID, byte, bit, tamanho, tipo, padrão, valores aceitos, nome, requisito, descrição, significado de cada valor e fonte | gerado pelo mesmo programa |

Para refazer `padrao.txt` e `tabela-campos.tsv` (só lê o firmware, sem rede):

```bash
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v "$PWD/driver":/driver:ro -v "$PWD/stubs/codificacao":/out moi3-bancada:1 sh /driver/m6_tabela.sh
```

## Formato

```
# comentário
0600 = 00 00 00 ...            conjunto inteiro: <DID em hexa> = <bytes em hexa>
0600 @0.4+4 = 2                um campo: <DID> @<byte>.<bit>+<bits> = <valor>
```

- **DID** é o identificador de dado do diagnóstico da unidade. O `persistence-app` do firmware
  registra a chegada de cada conjunto como "RDID" (leitura de dado por identificador). `0600` é a
  codificação; `F187`, o número de peça.
- No campo, `byte` conta do zero dentro do conjunto e `bit` é onde o campo começa, com 0 no bit
  menos significativo. Campos de até 8 bits ficam dentro de um byte. Campos de 16, 24 ou 32 bits
  começam no bit 0 e vão com o byte mais significativo primeiro. É como a HMI lê
  (`AccessImpl.getValue`, `getBoolean` e `getByteArray`).
- O valor é decimal ou `0x` hexa e tem de caber no campo.
- A ordem importa: linha de baixo vale sobre linha de cima, e arquivo de depois sobre o de antes.

## De onde sai cada informação da tabela

| Coluna | Fonte no firmware (MU0883, HMI H40.272.304) |
|---|---|
| byte, bit, tamanho, tipo, padrão, valores aceitos | a tabela interna de cada classe de acesso (`de.vw.mib.asl.framework.internal.diagnosis.variant.oilarge.CodingImpl`, `AdaptationImpl`, `CarFuncAdapImpl`, `RecordsImpl`, `IdentificationImpl`), lida do objeto construído, do jeito que o `AccessImpl` a percorre. A variante `oismall` tem tabelas idênticas (conferido pelo programa) |
| DID e tamanho de cada conjunto | as listas de chaves e comprimentos que cada classe passa ao `AccessImpl`; batem com `DiagApiProperties` |
| constante (`ID_…`) | as interfaces `de.vw.mib.asl.framework.api.diagnosis.config.CodingMib3`, `AdaptationMib3`, `CarFuncAdap`, `Records`, `Identification` |
| requisito (`Dia_Cod_…`, `Dia_Anp_…`), descrição e significado dos valores | os arquivos `coding.swap`, `adaptation.swap`, `carfunction.swap` e `identification.swap` de `/vw/hmi/config`, abertos com o `ObjectSwapper` do firmware. É o que a tela de diagnóstico da própria HMI mostra (`AslDiagGenericFetcher`). Os registros não têm `.swap` |

O programa confere o conjunto padrão: para cada um dos 1.188 campos, o valor lido do `padrao.txt`
como dado válido é igual ao que a HMI usa quando não recebe dado nenhum.

## Os conjuntos

| Conjunto | DID | Bytes | Campos | O que traz |
|---|---|---|---|---|
| codificação | `0600` | 25 | 24 | marca, classe, geração, carroceria, região, tração, lado do volante, tipo de veículo. A HMI só interpreta os 7 primeiros bytes |
| adaptação | 39 DIDs, de `04FE` a `2442` | 397 | 400 | funções ligadas ou desligadas: navegação (`0558`), som e microfones (`0550`), WLAN e App-Connect (`0555`), sintonizador (`0559`, `09EB`), câmera de ré (`0505`, `055C`), skin e avisos (`0556`), lista de funções do carro (`0B1D`) e outras |
| funções do carro | `0B1B` | 256 | 336 | o que aparece nos menus do carro: "ACC: Menu display", "Clock: Menu display" e assim por diante |
| registros | `7209`, `720D` | 144 + 34 | 158 | limites de velocidade para bloquear funções, tempos de exibição |
| identificação | `F187`, `F189`, `F18C`, `F191`, `F1A3` | 49 | 5 | número de peça, versão de software, número de série, número e versão de hardware |

Ficam **fora** do padrão, comentados no fim do `padrao.txt`: `720B` e `720C` (idiomas) e `F1B1`.
A HMI não procura esses na tabela dela; pede ao `persistence-app` pelo barramento. O `720B` faz o
`persistence-app` regravar o idioma da HMI, e o valor padrão é um texto sem sentido.

## Campos já experimentados

| Campo | DID, byte, bits | Valores | Efeito visto na bancada | Rodada |
|---|---|---|---|---|
| `ID_NAVIGATION_SYSTEM` — `Dia_Anp_5153` "Funktion Navigation" | `0558`, byte 1, bit 7 | 0 nicht aktiviert, 1 aktiviert | grade ganha "Navigation"; tecla NAV abre "Starting navigation…"; ícone novo na barra do veículo | cod3 |
| `ID_BRAND` — `Dia_Cod_230` | `0600`, byte 0, bits 4 a 7 | 0 a 12; 2 = VW | registro: "VW brand is true", some o aviso do skin. Tela igual | cod2 |
| `ID_CAR_CLASS` — `Dia_Cod_244` | `0600`, byte 0, bits 0 a 3 | 1 a 9; 3 = A | junto com marca, geração e carroceria: muda a figura do carro (Golf → sedã) | cod4 |
| `ID_CAR_GENERATION` — `Dia_Cod_246` | `0600`, byte 1, bits 4 a 7 | 1 a 9 e 16 | idem | cod4 |
| `ID_CAR_BODYSTYLE` — `Dia_Cod_248` | `0600`, byte 1, bits 0 a 3 | 0 a 9; 1 = Stufenheck (sedã) | idem | cod4 |
| `ID_CLOCK_0X11` — `Dia_Anp_2187` "Clock (0x11) Availability" | `0B1D`, byte 17, bits 6 e 7 | 0 not activated, 1 activated | registro: "Clock is true". Tela igual | cod5 |
| `ID_VW_SPARE_PART_NUMBER`, `ID_VW_ECU_HARDWARE_VERSION_NUMBER`, `ID_VW_APPLICATION_SOFTWARE_VERSION_NUMBER` — `Dia_Ident_7`, `_11`, `_110` | `F187` (11 bytes), `F1A3` (3), `F189` (4) | texto; com byte fora da faixa de `0` a `z`, espaço, hífen e ponto a HMI mostra "DIAG ERROR" (é o caso do zero do padrão) | a tela "System information" troca "DIAG ERROR" pelo número de peça, pelo hardware e pelo software | cod7 |
| `ID_VEHICLE_TYPE_POWER_STAGE_1` — `Dia_Cod_308` | `0600`, byte 5, bit 1 | 0 ou 1 | a cor de destaque da HMI passa de azul a vermelho (`#FF2832`) em todas as telas | cod8a |

Os nomes dos valores de marca, classe, geração e carroceria são as constantes `BRAND_*`,
`CAR_CLASS_*`, `CAR_GENERATION_*` e `CAR_BODYSTYLE_*` da interface `CodingMib3`.

## O Jetta

Em 11/10/2026 o Lipe pediu que o simulador seja o Jetta GLI 2021 dele. O carro ainda não tem
autoscan, então a identidade fica em dois arquivos, para o palpite nunca se misturar com o dado:

- [jetta-gli-2021.txt](jetta-gli-2021.txt): o que foi **lido do carro**. Hoje, só a identificação
  da tela "Informações do sistema" (número de peça `3G5035832C`, hardware `H56`, software `0803`).
  A codificação e as adaptações têm o lugar marcado, vazio.
- [jetta-gli-2021-deducoes.txt](jetta-gli-2021-deducoes.txt): o que foi **escolhido** para o
  simulador ficar parecido com o carro, com a razão de cada linha. Não é codificação do carro.

Com os dois (rodadas `cod8` e `cod9`) o simulador mostra o sedã na tela do veículo, o destaque em
vermelho, o bloco "Navigation" e a tela de informações com as mesmas quatro primeiras linhas da
foto da central do carro. Quadros em `quadros/2026-10-11-hmi-cod-jetta-*.png`.

O que continua diferente do carro nessa tela, e por quê: o valor do banco de dados de navegação
(o carro mostra 20.3; o simulador, "N/A") e a linha do codec de mídia (deve depender do serviço
de mídia, que a bancada não tem; não conferido).

O idioma deixou de ser diferença às 20h de 11/10/2026: a central nasce em português. **Corrigido:**
aqui estava que o idioma vinha "de outro registro, não exercitado". Ele não é codificação nem
adaptação: a HMI o tira da memória de estado, e o registro `720B` só vale quando ela não tem
idioma. Os arquivos disso ficam em [../persistencia/](../persistencia/README.md).

Os dois arquivos entram no modo ao vivo por padrão desde 11/10/2026, por decisão do Lipe. O que
isso quer dizer: a tela que o simulador mostra **não é mais a da central sem codificação**, e
parte do que se vê (o sedã, o vermelho, a navegação) é palpite, não leitura do carro. Quando o
autoscan chegar, os valores reais entram em `jetta-gli-2021.txt` e o arquivo de deduções sai da
lista de `driver/ao_vivo.sh` e de `driver/m3_ao_vivo.sh`.

### Os outros menus do carro: o que cada função liga (Marco 14, 12/10/2026)

Em 12/10/2026 o Marco 14 mediu o que faz um menu do carro existir, e o resultado acerta o que
esta pasta dizia:

- **Só a lista de funções do carro (`0B1B`) decide.** Para cada aparelho de uma tabela dela, a
  HMI lê um campo, o "Menu display" (bit 7 de um byte), e só então pede ao carro as opções
  daquele menu. **A lista de aparelhos (`0B1D`) não é consultada** para isso: os pneus e o Lane
  Assist deram a mesma tela com e sem o aparelho (rodadas `pn1`, `l2` e `l3`). O Marco 8 tinha
  ligado as duas listas juntas, sem testar uma sem a outra (está dito no
  `jetta-gli-2021-deducoes.txt`), e a tabela de arquivos daqui fala das duas. As quatro linhas
  do `0B1D` que o arquivo de deduções tem continuam lá: não testei tirá-las.
- **A função sozinha não cria o menu.** Faltam as opções do serviço, que vêm do carro e ficam em
  [../veiculo/](../veiculo/README.md).
- **Que função liga que menu** está em [../../traces/men_aparelhos.txt](../../traces/men_aparelhos.txt):
  os 36 aparelhos da tabela da HMI, com o byte de cada função e o serviço de opções que ela faz
  a HMI assinar. É leitura de código; o que foi visto na tela está no
  [modelagem-soc.md](../../modelagem-soc.md), Marco 14.

O [catalogo-menus.txt](catalogo-menus.txt) liga 13 dessas funções de uma vez, para se ver o que
a HMI sabe mostrar. **Não é o Jetta** e fica fora de toda partida padrão: a ficha do carro não
confirma nenhum dos equipamentos desses menus. Quando um deles se confirmar, a linha da função
sai do catálogo e vai para o `jetta-gli-2021-deducoes.txt` (ou para o `jetta-gli-2021.txt`, se
vier da leitura das adaptações do carro).

## Como achar um campo

A tabela é texto separado por tabulação. Para ver tudo o que fala de câmera, por exemplo:

```bash
grep -i -E "rvc|camera|rear view" stubs/codificacao/tabela-campos.tsv | cut -f1,3,5,7,8,10,13,15,16
```

As colunas pedidas ali são conjunto, DID, byte, bit, tamanho, padrão, constante, descrição e
significado dos valores.

## Limites

- O padrão é o que a HMI assume sem dado. **Um carro de verdade tem outros valores.** Quando
  houver autoscan do Jetta, a codificação longa da unidade 5F pode ser comparada com a tabela;
  até lá, o que o carro tem é desconhecido.
- Só vale para este firmware (MU0883, variante OISmall). Outro train pode ter outra tabela.
- A bancada entrega os dados que a HMI pede; não mexe em assinatura, proteção de componentes nem
  em nada do carro.
