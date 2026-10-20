# stubs/persistencia — a memória de estado com que a central nasce

No carro, a central guarda o que você escolhe pela tela (o idioma, por exemplo) numa memória
que sobrevive ao desligar. Na bancada essa memória é um arquivo dentro do container
(`/data/lge/hmi/lsmval`), e some com ele. A bancada responde a isso de dois jeitos:

- **Os arquivos desta pasta** são a memória com que a central nasce quando não há nada guardado:
  a semente. Hoje, o idioma.
- **Desde a noite de 11/10/2026 a central do modo ao vivo lembra de uma sessão para a outra**,
  como no carro. Enquanto ela roda, a memória fica guardada em `ao-vivo/memoria/`, e a partida
  seguinte nasce dali. Está na seção "A memória de uma sessão para a outra", abaixo.

**Desde 12/10/2026 a mesma pasta guarda também o que a tela grava no carro de mentira** (a hora, a
data e o fuso acertados e as unidades escolhidas), em `ao-vivo/memoria/veiculo/estado.txt`. Isso
não é memória da central: no carro quem lembra é o painel. Está em
[../veiculo/README.md](../veiculo/README.md), "O estado".

**O que a memória guarda, linha por linha, e o que volta na partida seguinte** está na seção
"O que cada linha guarda", levantada na noite de 11/10/2026 em centrais de teste.

**Mudou em 11/10/2026, às 20h58:** aqui estava "tudo o que foi escolhido pela tela some quando a
central reinicia". No modo ao vivo isso deixou de valer.

É bancada de estudo: nada daqui vai para o carro, e nada daqui foi lido da memória do carro.

## O que tem aqui

| Arquivo | O que faz | De onde vem |
|---|---|---|
| `idioma-pt_BR.txt` | a central nasce em português do Brasil | é a linha que a própria HMI gravou quando o Lipe trocou o idioma pela tela da central ao vivo, em 11/10/2026 |

O modo ao vivo (`sh driver/ao_vivo.sh`) usa o `idioma-pt_BR.txt` como semente desde 11/10/2026:
é dele que a central nasce enquanto não há memória guardada. As rodadas de experimento
(`driver/m6_rodada.sh`), as centrais de teste e o lançador chamado direto (`driver/m1_hmi.sh`)
não usam a memória guardada: nascem do arquivo que receberem em `LSMVAL`, ou com a memória vazia.

## A memória de uma sessão para a outra

Vale para a central ligada pelo `driver/ao_vivo.sh` e pela janela do Mac, que chama o mesmo
roteiro. A memória de estado dela é copiada para `ao-vivo/memoria/lsmval` a cada mudança, e a
partida seguinte começa desse arquivo. A central passa a nascer como foi deixada: no mesmo
idioma, na última tela principal que estava aberta, com a grade de menus na ordem em que ficou e
com os ajustes do display como ficaram. A lista do que foi conferido está em "O que cada linha
guarda".

### O que muda na partida

Medido nas rodadas `mem1` a `mem5`, em 11/10/2026.

| Antes | Agora |
|---|---|
| toda partida abria o aviso "Rádio: esta função não está disponível no momento" | abre a última tela principal da sessão anterior: os ajustes, a grade de menus, a tela do veículo, a navegação |
| as partidas eram iguais entre si | cada partida depende de como a anterior terminou |
| a HMI gravava 57 valores a cada partida | nascendo com tudo, não grava nada enquanto ninguém muda nada |

| Sessão 1 termina nos ajustes | Sessão 2 nasce nos ajustes, sem ninguém apertar nada |
|---|---|
| ![Sessão 1 termina nos ajustes](../../quadros/2026-10-11-hmi-mem-sessao1-termina-nos-ajustes.png) | ![Sessão 2 nasce nos ajustes](../../quadros/2026-10-11-hmi-mem-sessao2-nasce-nos-ajustes.png) |

### Cuidados

- **Um estado ruim também fica guardado.** Exemplo medido (rodada `mem3`): desligar a central com
  a navegação aberta faz a seguinte nascer em "Iniciando navegação…", que na bancada não sai dali
  sozinha. Sai apertando outra tecla, e a tela nova passa a ser a guardada; ou voltando atrás,
  como está na tabela abaixo.
- **Nem toda tela é lembrada.** A HMI não guarda como última tela o clima, os usuários, a data e
  hora, as unidades e mais algumas (lista na classe `PersistenceValidator`). Nesses casos fica a
  tela principal de antes.
- **Na primeira sessão depois de voltar ao zero**, a HMI regrava "rádio" como última tela perto
  dos 2 min 20 s de ligada, quando acaba a troca de idioma da partida. Tela aberta antes disso e
  deixada lá não é lembrada; tela aberta depois, sim. Nas sessões que já nascem com memória isso
  não acontece. É comportamento do firmware (classe `StageContentProcessor`), não da bancada.
- **A HMI leva de 5 a 11 s para mandar guardar** o que foi mudado (de 5 a 7 s nas rodadas `mem`;
  de 6 a 11 s na sessão longa, com a máquina carregada). Desligar a central logo depois de
  mexer perde essa mudança.
- **Nem tudo o que se escolhe na tela chega à memória, na bancada.** Brilho, grave e agudo só
  valem quando um serviço confirma (o display, o áudio). Onde a bancada não tem quem confirme, a
  escolha não pega e nada é guardado. Lista em "O que a tela não deixa mudar na bancada".
  **Mudou em 12/10/2026:** para o brilho a bancada passou a ter quem confirme (o display de
  mentira), e a escolha pega e volta.
  **Mudou de novo às 3h35 de 12/10/2026 (Marco 13):** para grave, agudo, balanço e fader também
  há quem confirme, o som de mentira (`SOM`, que o modo ao vivo liga). A escolha **pega, mas não
  volta**: esses valores não passam por esta memória, e o som de mentira não guarda nada de uma
  partida para a outra. **(Desde as 9h07 de 12/10/2026 voltam, no modo ao vivo: quem guarda é o som de mentira, em `ao-vivo/memoria/som/estado.txt`; esta memória continua sem eles.)**
- **Na unidade quem manda é o carro.** As unidades ficam guardadas nesta memória, mas na partida
  a HMI segue o que o carro informa. Desde 12/10/2026, no modo ao vivo, o carro de mentira guarda
  o que a tela gravou nele e a unidade escolhida volta (rodada `v2`). **Mudou:** aqui estava "a
  unidade escolhida pela tela dura só até a central reiniciar", o que as rodadas `uni2` a `uni5`
  tinham mostrado; continua assim onde o carro de mentira não guarda o estado (centrais de teste
  sem pedido, `VEICULO=`).
- **Uma central por pasta.** Duas centrais guardando na mesma `ao-vivo/memoria/` escreveriam uma
  por cima da outra.

### Como ver, desligar e voltar atrás

Os comandos são dados a partir da pasta da bancada.

| Para | Faça |
|---|---|
| ver a memória guardada, com o nome de cada campo | `python3 -I driver/lsmval_abre.py ao-vivo/memoria/lsmval` |
| ver o que a última sessão mudou | `python3 -I driver/lsmval_abre.py ao-vivo/memoria/anteriores/lsmval.1 ao-vivo/memoria/lsmval` (com a central desligada ou ligada; a `lsmval.1` é a memória com que ela nasceu) |
| ver a memória crua | `cat ao-vivo/memoria/lsmval` (uma linha por valor) |
| ver o que a guarda fez na sessão | `cat ao-vivo/log/ao_vivo.memoria.log` |
| uma sessão que não lê nem guarda a memória, com a partida de antes | `MEMORIA= sh driver/ao_vivo.sh` (ou `MEMORIA=0`) |
| uma sessão que nasce de um arquivo escolhido | `LSMVAL=/stubs/persistencia/idioma-pt_BR.txt sh driver/ao_vivo.sh`. `LSMVAL` passado à mão vale sobre tudo: a memória guardada não é lida nem alterada |
| voltar ao zero | com a central desligada: `rm -rf ao-vivo/memoria`. A partida seguinte nasce do `idioma-pt_BR.txt` e volta a guardar. Desde 12/10/2026 isso zera também o carro de mentira (hora do Mac, unidades do perfil), que guarda o estado dele na subpasta `veiculo/` |
| voltar só o carro de mentira ao zero | com a central desligada: `rm -rf ao-vivo/memoria/veiculo` |
| ver o que o carro de mentira guardou | `cat ao-vivo/memoria/veiculo/estado.txt` (formato em [../veiculo/README.md](../veiculo/README.md), "O estado") |
| voltar a uma partida anterior | com a central desligada: `cp ao-vivo/memoria/anteriores/lsmval.1 ao-vivo/memoria/lsmval`. A `lsmval.1` é a memória de antes da última sessão; a `lsmval.2`, a de antes da penúltima; e assim até a `.5` |

Com a central ligada, apagar ou trocar o arquivo não adianta: em 2 s ela guarda de novo a
memória dela. A janela do Mac liga a central com tudo no padrão, portanto com a memória; para
uma sessão sem ela é preciso subir pelo terminal.

### Como funciona

- O `driver/ao_vivo.sh` monta a pasta `ao-vivo/memoria` no container e passa
  `MEMORIA=/live/memoria`. O `driver/m3_ao_vivo.sh`, quando recebe `MEMORIA`, chama o
  `driver/lsmval_guarda.sh` para escolher o arquivo de partida e para vigiar o `lsmval` da central.
  Sem `MEMORIA`, que é o caso de quem chama o `m3_ao_vivo.sh` direto, nada disso acontece.
- A vigia confere o arquivo a cada 2 s e só guarda a cópia que (1) ficou igual entre duas
  leituras com 0,3 s de intervalo, (2) não está vazia e termina em fim de linha e (3) passa pelo
  `lsmval_prepara.sh`. Grava ao lado e troca o nome. O motivo: o `persistence-app` regrava o
  arquivo inteiro quando a chave já existe, e há instantes em que ele está vazio ou pela metade.
- Cópia recusada não apaga a guardada. Fica a última boa, o motivo vai para o registro e para
  `ao-vivo/memoria/recusada.txt`, e a partida seguinte avisa.
- Memória guardada que não passe no conferidor na hora da partida (mexida à mão, por exemplo)
  vai para `ao-vivo/memoria/lsmval.recusada`, com aviso, e a central nasce da semente.
- Em `ao-vivo/memoria/anteriores/` fica a memória com que cada uma das cinco últimas sessões
  nasceu. Partida que encontra a memória igual à da partida anterior não ocupa lugar.
- Desde 12/10/2026 o `driver/m3_ao_vivo.sh`, quando recebe `MEMORIA`, também manda o carro de
  mentira guardar o estado dele em `<pasta>/veiculo/estado.txt` (variável `VEICULO_ESTADO`). Quem
  lê, confere e grava esse arquivo é o `driver/veiculostub.c`, e não o `lsmval_guarda.sh`: são
  duas memórias na mesma pasta, e uma não mexe na outra.
- Testes: `driver/tests/t_memoria.sh`, seções 6 a 10 (58 casos, no Mac e no container), e
  `driver/tests/t_lsmval_abre.sh` (o leitor; 38 casos, no Mac).

## Como usar

As opções vão com o caminho de dentro do container (a pasta `stubs/` aparece lá como `/stubs`):

```sh
sh driver/ao_vivo.sh                                               # nasce da memória guardada; sem ela, do idioma-pt_BR.txt
MEMORIA= sh driver/ao_vivo.sh                                      # não lê nem guarda a memória: nasce do idioma-pt_BR.txt
LSMVAL=/stubs/persistencia/idioma-pt_BR.txt sh driver/ao_vivo.sh   # nasce deste arquivo; a memória guardada fica como está
LSMVAL= sh driver/ao_vivo.sh                                       # memória vazia, em inglês; a guardada fica como está
LSMVAL=/stubs/persistencia/idioma-pt_BR.txt sh driver/m6_rodada.sh t /tmp/m6 stubs/codificacao/padrao.txt
```

Para conferir um arquivo sem subir nada: `sh driver/lsmval_prepara.sh stubs/persistencia/idioma-pt_BR.txt`
mostra o que a central vai receber, ou o arquivo e a linha do erro. Serve também para a memória
guardada: `sh driver/lsmval_prepara.sh ao-vivo/memoria/lsmval`.

## O formato

Uma linha por valor guardado, com os números em decimal, igual ao arquivo `/data/lge/hmi/lsmval`
do firmware:

```
<tipo> <namespace> <chave> <valores...>
```

- `tipo`: 0 = uma sequência de bytes, cada um de 0 a 255; 1 = um número inteiro.
- `namespace` e `chave`: o endereço do valor. O idioma é o namespace 1101, chave 10.
- `#` começa um comentário; linha em branco não conta.

A linha do idioma tem 22 bytes: os cinco primeiros são o código do idioma em letras (`pt_BR` é
`112 116 95 66 82`), o sexto é o tipo de voz e o resto são espaços (32). Fonte: a classe
`de.vw.mib.asl.framework.internal.lsc.PersistableLsc` da HMI.

O lançador recusa o arquivo inteiro se houver uma linha errada (teste em
`driver/tests/t_memoria.sh`). O motivo: os dois programas do firmware que leem esse arquivo
descartam, sem avisar, a linha que não entendem, e a central subiria em inglês sem explicação.

### Como ler a chave e o valor

Lido no jar da HMI em 11/10/2026 e conferido nas memórias que ela gravou. O programa
`driver/lsmval_abre.py` faz essa leitura.

**A chave diz de quem é o valor.** Cada namespace de 5050 a 5095 é de um módulo da HMI (Sound,
SystemCommon…). Dentro do módulo, cada bloco de dados guardado tem um número (no código, o
"persistable"), e cada bloco tem até duas partes: a comum a todos os usuários e a de cada
usuário. A chave junta as duas coisas (`PersistenceKeyUtil.calcKey`):

```
chave = (número do bloco × 4294967296) + (perfil × 16777216)
```

| Chave | Em hexa | Quer dizer |
|---|---|---|
| `0` | `0x0` | a declaração do módulo: a descrição dos campos dele, regravada junto com os dados |
| `8573157376` | `0x01FF000000` | bloco 1, parte comum (perfil 255) |
| `4311744512` | `0x0101000000` | bloco 1, parte do usuário 1 |
| `8606711808` | `0x0201000000` | bloco 2, parte do usuário 1 |
| `12868124672` | `0x02FF000000` | bloco 2, parte comum |
| `21458059264` | `0x04FF000000` | bloco 4, parte comum |

Os namespaces mais antigos (1101, 5000, 5008, 6000) usam chaves pequenas (10, 100, 200, 300),
sem bloco nem perfil.

**O valor vai embrulhado** (`Compression` e `RecordByteArray.writeToDsi`). São três casos:

| Começa com | O que é | Quando |
|---|---|---|
| `55 66 77 88` (as letras `7BMX`) | 4 bytes de marca, 8 bytes com a soma de verificação (Adler-32) e depois o conteúdo | conteúdo de 5 bytes ou mais que não foi comprimido |
| `11 22 33 44` | 4 bytes de marca e o conteúdo comprimido (gzip de "tamanho em 4 bytes + conteúdo") | conteúdo de 50 bytes ou mais, quando comprimir encurta |
| qualquer outra coisa | o conteúdo cru | conteúdo de até 4 bytes |

Ao ler, a HMI confere a soma; com a soma errada o valor é descartado (`Compression.removeAdler`
devolve nulo). Isso é leitura de código: não rodei uma memória com soma errada. Nas memórias
abertas em 11/10/2026 todas as somas batem.

**O conteúdo** começa, quase sempre, com 2 bytes de versão (`00 01`) e segue com os campos na
ordem em que o módulo os grava. As posições desta página contam a partir do 0, já sem o
embrulho, que é como o `lsmval_abre.py` mostra.

## O que cada linha guarda

Levantado em 11/10/2026, das 21h35 às 23h, em centrais de teste (containers `moi3-m6-*`, com
uma pasta de memória por experiência; a central ao vivo não foi tocada). O método: mudar UMA coisa
pela tela, comparar a memória de antes com a de depois, e subir outra central a partir do que
ficou para ver se o valor volta. Previsões e resultados em `traces/m6_previsoes.md` (quarta
parte); saídas em `traces/m6_lsm_*`.

### Em poucas palavras

**A central lembra, de uma sessão para a outra** (conferido pela tela da sessão seguinte):

- o idioma e a última tela principal (já se sabia);
- a ordem dos blocos da grade de menus, depois de arrastar um bloco;
- "Som do touchscreen", "Exibir hora em modo de standby", "Sensor de aproximação" e "Gesto de
  mão", da tela Configurações > Display;
- os idiomas marcados em "Idiomas adicionais do teclado";
- a visão em que a tela do veículo abre (dados de condução ou status do veículo);
- o "Não exibir" do assistente de configuração (conferido pela sessão do carro de mentira).

**A unidade (°C ou °F) fica guardada aqui, mas quem a traz de volta é o carro.** A HMI guarda a
escolha e, na partida seguinte, segue o que o carro informa. Até 12/10/2026 o carro de mentira
nascia sempre do perfil e a unidade não voltava; desde então, no modo ao vivo, ele guarda o que a
tela gravou e a unidade volta.

**Não dá para mudar pela tela, na bancada**, e por isso não há o que voltar: grave, agudo e
balanço; o usuário; as estações de rádio. O nível de luminosidade estava nesta lista até
11/10/2026; desde 12/10 a escolha pega e volta, porque o display de mentira passou a responder. O motivo de cada um está em
"O que a tela não deixa mudar na bancada". O volume não fica nesta memória.
**Mudou às 3h35 de 12/10/2026 (Marco 13):** com o som de mentira, grave, agudo e balanço mudam
pela tela. Continuam sem ter o que voltar: numa central nascida de uma cópia da memória do modo ao
vivo (rodada `V1` daquele marco), com os graves levados a 2 e o volume a 9, as três linhas do
módulo Sound (`5056`) saíram iguais byte a byte às de entrada. **(Desde as 9h07 de 12/10/2026 voltam, no modo ao vivo: quem guarda é o som de mentira, em `ao-vivo/memoria/som/estado.txt`; esta memória continua sem eles.)**

### As linhas conferidas pela tela

"Volta na partida?" quer dizer: uma central nova, nascida da memória que a anterior deixou,
mostrou o valor sem ninguém mexer.

| Linha (namespace e chave) | Dono | O que guarda | Volta na partida? | Rodada que mostrou |
|---|---|---|---|---|
| `1101` `10` | idioma (chave antiga, `LSC`) | o código do idioma em cinco letras e o tipo de voz | **sim**: a HMI registra "persisted language 'pt_BR' is valid" e as telas saem no idioma | `idi1`, `mem2` |
| `5051` `4311744512` | SystemCommon, bloco 1, usuário 1 | várias coisas na mesma linha: a última tela, os ajustes de Configurações > Display e os idiomas do teclado (posições na tabela seguinte) | **sim**, campo a campo | abaixo |
| `5056` `8606711808` | Sound, bloco 2, usuário 1 | modo do equalizador, posição do som e cinco liga-desliga. A posição 14 é "Som do touchscreen" (`touchscreenToneOn`) | **sim**: a caixa nasce marcada, e a HMI registra a leitura com o byte em 01 | `som1`, `som2`, `expl2` |
| `5077` `4311744512` | SimpleGridMenu, bloco 1, usuário 1 | a ordem dos oito blocos da grade de menus: uma lista de oito identificadores de 8 bytes, coluna por coluna | **sim**: a grade nasce na ordem nova, sem tecla nenhuma | `fav1`, `fav2`, `expl2` |
| `5069` `8573157376` | CarEarly, bloco 1, comum | a visão em que a tela do veículo abre. Posições 6 a 9 (`carMainViewId`): -1 nunca aberta, 0 dados de condução, 2 status do veículo; 10 a 13 (`vehicleSubViewId`); 30 a 33 (`drivingDataSubViewId`); 2 a 5, a cor da luz ambiente | **sim**: a central nasce em "Status do veículo", e não no computador de bordo | `vei1`, `vei2`, `expl2` |
| `5060` `0`, `8573157376` e `4311744512` | GlobalSetupWizard | as páginas do assistente de configuração e, na posição 26 da parte do usuário, o "Não exibir". As três linhas só passam a existir quando o assistente abre pela primeira vez | **sim**: depois de "Não exibir" o assistente não aparece na sessão seguinte | `ne1` e `ne2`, da sessão do carro de mentira (`modelagem-soc.md`, Marco 8); na `expl2` também não apareceu |

**A linha `5051` `4311744512`, posição por posição** (ordem do `SystemCommonSerializer`):

| Posição | Campo no código | Na tela | Volta na partida? | Rodada |
|---|---|---|---|---|
| 0 e 1 | versão | (sempre `00 01`) | | |
| 2 | `displayBrightness` | o nível de luminosidade, de 0 (mais claro) a 4 (mais escuro); nasce em 2 | **sim, desde 12/10/2026**: a HMI lê e pede ao display na partida (00 pede 100 %, 01 pede 75 %, 02 pede 50 %, 03 pede 25 %, 04 pede 0 %), o display de mentira responde, e o campo "Nível de luminosidade" nasce no nível guardado. Até 11/10 a tela não deixava mudar | `bri1` (tela), `bri2` e `bri3` (valor trocado à mão no arquivo), `som1`; de outra sessão, em 12/10: `a` e `b` ("Baixa" escolhida e de volta), `p1` e `p2` ("Alta") |
| 3 | `showClock` | "Exibir hora em modo de standby" | **sim**: a caixa nasce desmarcada | `tela1`, `tela2`, `expl2` |
| 4 | `offclockLayout` | (não achei na tela) | não conferido | |
| 5 a 12 | `bitfieldLanguagesThatWereSetBefore` | "Idiomas adicionais do teclado": um bit por idioma. O 26 é o português da partida; o 0, alemão; o 8, tcheco. Pelo nome do campo, idioma que já foi o da central também fica ligado aqui: na central ao vivo, que já esteve em inglês, o bit 2 está ligado (não testei trocar o idioma) | **sim**: "Alemão" nasce marcado | `tec1`, `tec2`, `expl2` |
| 13 | `acousticTouchscreenFeedback` | não é o "Som do touchscreen" (esse é do Sound, acima); ficou em 00 | não mudou em nenhuma rodada | |
| 14 | `proximityEnabled` | "Sensor de aproximação" | **sim**: nasce desmarcado | `expl1`, `expl2` |
| 15 | `acousticSensorButtonFeedback` | (não achei na tela) | não conferido | |
| 16 | `gestureActive` | "Gesto de mão" | **sim**: nasce desmarcado | `expl1`, `expl2` |
| 17 e 18 | `gestureInformationActive`, `gestureSoundeffectActive` | os dois "Feedback … de gestos de mão" (apagados enquanto o gesto está desligado) | não mudados | |
| 19 em diante | `lastContentId` | a última tela principal: 1 byte, o tamanho em 4 bytes e o nome (`HMI_GRIDMENU`, `HMI_CAR`, `HMI_TONE`…) | **sim**: a central nasce nela | `mem2`, `mem3`, `fav2`, `vei2`, `expl2` |
| depois do nome | `backgroundIndex`, `globalCoachmarkSetup`, lista de `coachmarks` | (não achei na tela) | não conferido | |

| Configurações > Display, como nasce | A grade arrastada volta na ordem nova |
|---|---|
| ![Ajustes do display](../../quadros/2026-10-11-hmi-lsm-ajustes-do-display.png) | ![Grade reordenada](../../quadros/2026-10-11-hmi-lsm-grade-volta-reordenada.png) |
| **"Som do touchscreen" volta marcado** | **A tela do veículo nasce em "Status do veículo"** |
| ![Som do touchscreen marcado](../../quadros/2026-10-11-hmi-lsm-som-do-touchscreen-volta.png) | ![Status do veículo](../../quadros/2026-10-11-hmi-lsm-veiculo-nasce-no-status.png) |

### O que a tela não deixa mudar na bancada

| O que | O que acontece | Por quê | Rodada |
|---|---|---|---|
| grave, agudo, balanço (tecla TONE ou o bloco "Som") | a tela mostra "A configuração do som estará disponível novamente quando terminar de telefonar, de estacionar ou após o aviso." e nenhuma das quatro abas responde | a HMI fala com um serviço de áudio (RSI `audiomanagement`) que a bancada não tem. E esses valores não estão entre os campos que o módulo Sound guarda: no carro devem ficar com o serviço de áudio (leitura de código, não conferida). **Desde as 3h35 de 12/10/2026 a bancada tem esse serviço, de mentira (`SOM`, Marco 13):** a tela abre sem o aviso e os ajustes pegam até a central desligar; a memória segue sem mudar (rodada `V1` do Marco 13) | `expl1` |
| volume (botão giratório) | o popup de volume aparece; a memória não muda | não é campo de nenhum módulo desta memória. **Desde o Marco 13 (12/10/2026), com `SOM`, o volume tem valor e muda com o botão**, no serviço de áudio de mentira; a memória segue sem ele | `expl1` |
| nível de luminosidade | a lista abre com cinco níveis; escolher "Baixa" fecha a lista e o campo volta a "Muito alta"; a memória não muda | a HMI só guarda o brilho quando o serviço de display responde (`processGetDisplayBrightnessResponse`), e o `dsistub` não responde a `setDisplayBrightness` (1011) nem a `getDisplayBrightness` (1022). A HMI registra "DSI timeout … getDisplayBrightness did not arrive". O campo da tela vem dessa resposta, não da memória: por isso mostra "Muito alta" com 02 guardado | `expl1`, `bri1` |
| nível de luminosidade, **desde 12/10/2026** | **dá para mudar**: a escolha fica no campo, a HMI a guarda na posição 2 da linha `5051` `4311744512` (02 para 03 ao escolher "Baixa") e a central seguinte nasce nela. A imagem não escurece | o `driver/dsistub.c` passou a guardar a porcentagem que a HMI grava (`setDisplayBrightness`) e a devolvê-la na leitura (`getDisplayBrightness`), que é de onde a HMI tira o nível que mostra e guarda. A linha de cima fica como registro do que acontecia até 11/10 | outra sessão, 12/10: `z` (antes), `a`, `b`, `p1`, `p2`; `modelagem-soc.md`, "O brilho pela tela: o display de mentira responde" |
| trocar de usuário (Usuário > Outros) | aparecem "Convidado" e três usuários do mock; tocar em outro não troca | o mock de usuários não faz a troca. Por isso toda a memória da bancada é do usuário 1 | `expl1` |
| estações de rádio favoritas | o rádio fica no aviso "função indisponível" | não há sintonizador. As memórias de estação são do módulo AmFm (bloco 22: 36 lugares) | `longa1` |
| os "favoritos" do módulo FavoritesConfiguration (`5071`) | não achei tela para eles; puxar do topo não abre central de controle | são três listas de atalhos (`carBar`, `controlCenter`, `tileM`) de variantes com outra tela inicial; aqui ficam no valor padrão | `expl1` |

O que mais se parece com um favorito e dá para mudar é a ordem dos blocos da grade: segurar um
bloco por 2,5 s e arrastar. Foi o que as rodadas `fav1` e `fav2` usaram.

### As unidades: ficam guardadas, e quem as traz de volta é o carro

Quem manda nas unidades é o carro (a HMI registra "Unitmaster is true"). Escolher uma unidade na
tela é um pedido ao carro; a HMI só troca a tela e a memória quando o carro confirma.

| Rodada | Carro de mentira | O que aconteceu |
|---|---|---|
| `expl1`, `uni1` | o das 21h38, que só respondia a assinaturas | escolher °F não mudou a tela nem a memória; o carro registrou "pedido sem resposta: DSICarTimeUnitsLanguage.setTemperatureUnit" |
| `uni2` | o das 22h32 (Marco 9), que aceita os pedidos de gravação do relógio e das unidades | a tela passou a "Temperatura: °F", o carro registrou "pedido atendido … temperatureUnit = 1", e a linha `5051` `21458059264` mudou na posição 9, de 00 para 01 |
| `uni3`, `uni4`, `uni5` | o mesmo; as três nasceram da memória que a `uni2` deixou | nas três a HMI leu °F da memória, o carro informou °C (ele nasce do perfil), e em cerca de um minuto (aos 59, 68 e 68 s) a HMI regravou a linha com 00. A tela mostra °C, e a HMI não chegou a pedir °F ao carro |

| `uni2`: a tela aceita °F | `uni3`: a sessão seguinte nasce em °C |
|---|---|
| ![Unidades em °F](../../quadros/2026-10-11-hmi-lsm-unidades-fahrenheit.png) | ![Unidades de volta em °C](../../quadros/2026-10-11-hmi-lsm-unidades-volta-celsius.png) |

No carro de verdade quem lembra da unidade é o próprio carro (o painel de instrumentos), então
lá ela voltaria; isso é dedução, não medida. Para a bancada lembrar, o carro de mentira teria de
guardar o que a tela grava, e em 11/10/2026 ele não guardava.
A hora e a data são pedidos ao mesmo serviço; não tentei gravar.

**Mudou em 12/10/2026: no modo ao vivo a unidade volta.** O carro de mentira passou a guardar o
que a tela grava nele (`ao-vivo/memoria/veiculo/estado.txt`) e a nascer disso. Outra sessão
conferiu, pelo caminho do modo ao vivo, com as duas memórias:

| Rodada | O que aconteceu |
|---|---|
| `v1` | escolhi °F pela tela; a linha `5051` `21458059264` ficou com `temperatureUnit = 1` e o carro de mentira guardou `temperatureUnit = 1` no estado dele |
| `v2`, nascida do que a `v1` deixou | a HMI leu °F da memória, o carro informou °F, e aos 100 s a linha seguia com `temperatureUnit = 1`: a HMI não a regravou. O topo da tela mostrava 95 °F sem ninguém tocar |

A hora, a data e o fuso acertados pela tela também voltam, e eles não passam por esta memória:
ficam só no estado do carro de mentira. Nas centrais de teste e com o lançador direto nada disso
vale, salvo pedido. Registro: [../../modelagem-soc.md](../../modelagem-soc.md), seção "O carro de
mentira lembra do que a tela gravou"; uso: [../veiculo/README.md](../veiculo/README.md), "O estado".

### As outras linhas

Ninguém mudou estas pela tela. "Lida na partida" quer dizer que, numa central nascida das 58
linhas, a HMI não registrou o aviso "Could not load declaration data for module…" para o módulo
(numa central sem memória ela registra 26 desses avisos; com as 58 linhas, 3, dos três blocos
que a memória não tem: GlobalSetupWizard/1, Homescreen2/2 e WebAppManagement/12). Para 32 das 58
linhas ela registra também o valor que recebeu, e ele é igual ao do arquivo (rodada `som2`). O
que cada campo faz ao voltar não foi conferido.

| Linha | Dono | O que guarda (lido no código) | Na bancada |
|---|---|---|---|
| `5051` `21458059264` | SystemCommon, bloco 4, comum | as unidades: nove números (distância, temperatura, volume, consumo de gasolina, de gás, elétrico, pressão, velocidade, peso) | lida na partida, com valor registrado, e logo regravada com o que o carro informa: é a cópia que a HMI tem das unidades do carro. Com o carro de mentira, consumo e pressão passam de 0 a 1 (era a linha que "mudava com o carro de mentira"). Ver "As unidades", acima |
| `5051` `8573157376` | SystemCommon, bloco 1, comum | teclado (`spellerLayout`), `menuMode`, modo dia e noite, manobrista, bloqueio do banco de trás e uma lista de ajustes de teclado | lida, com valor registrado; não mudou em nenhuma rodada |
| `5051` `12868124672` | SystemCommon, bloco 2, comum | as características do painel de toque (resolução 800×480, sensor, teclas) | lida, com valor registrado; não mudou |
| `5000` `200` | chave antiga `EARLY_SHARED` (`PersistableEarlyData`) | uma cópia antiga de vários ajustes: brilho, relógio, último contexto, skin, idioma a carregar cedo. Na primeira partida a posição 43 (`sportHMIEnabled`) passa a 01 perto dos 96 s | lida, com valor registrado. Não acompanha a tela: desmarcar a hora em standby mudou o `5051` e não esta linha |
| `5000` `300` | chave antiga `EARLY_ENGINETYPES` | o tipo de motor: `07 00` | lida, com valor registrado |
| `6000` `100` | escrita por `PersistableEarlyData.writeLastModeToSouthSide` | 4 bytes: o último contexto de tela e o de áudio (`00 06 00 00`), para o lado da LG | não vi a HMI ler |
| `5008` `100` | chave antiga do telefone (`PhonePersistence`) | 8 bytes; não abri | não vi a HMI ler |
| `5056` `12868124672` | Sound, bloco 2, comum | `lsmTabbar`: a aba aberta na tela de som | lida; as abas não respondem na bancada (**respondem desde o Marco 13, com o som de mentira**; se a aba aberta volta na partida seguinte não foi visto) |
| `5052` (oito linhas) | AmFm, blocos 1, 17, 22 e 28 | ajustes do rádio, estação atual, 36 memórias de estação em três bancos e a lista fixa. A declaração é a maior linha da memória | lidas, com valor registrado; o rádio não funciona na bancada |
| `5058` (três linhas) | Speech, bloco 2 | o tipo de voz e sete liga-desliga do comando de voz | lidas; "Comando de voz" fica apagado na tela |
| `5059` (três linhas) | InstrumentCluster, blocos 1 e 2 | o que o painel de instrumentos sabe mostrar | lidas |
| `5061` (três linhas) | EntertainmentManager, bloco 1 | o contexto de entretenimento (rádio ou mídia) | lidas, com valor registrado |
| `5065` (três linhas) | Homescreen, bloco 1 | o relógio e os blocos da tela inicial, que esta variante não tem | lidas |
| `5068` (duas linhas) | Car, bloco 1 | o que o carro tem (aquecimentos, luz ambiente, pressão dos pneus) e as últimas temperaturas | lidas |
| `5071` (três linhas) | FavoritesConfiguration, bloco 1 | as três listas de atalhos, no padrão | lidas |
| `5078` (três linhas) | Iaa, bloco 1 | se mostra a tela de boas-vindas | lidas, com valor registrado |
| `5079` (três linhas) | WebAppManagement, bloco 14 | os aplicativos web conhecidos e o último usado | lidas |
| `5083` (duas linhas) | DigitalKey, bloco 1 | o último cartão: o texto `deactivated` | lidas |
| `5086` (duas linhas) | CarFas, bloco 1 | a última visão dos assistentes de condução | lidas |
| `5092` (três linhas) | OffClock, bloco 1 | o desenho do relógio de standby | lidas, com valor registrado |
| `5050` (duas linhas) | SystemEarly, bloco 1 | quatro marcadores do comando de voz | lidas, com valor registrado |

Fora das duas tabelas ficam cinco linhas que só acompanham as outras: as declarações (chave `0`)
do SystemCommon, do Sound, do SimpleGridMenu e do CarEarly, e a parte comum do SimpleGridMenu,
que só tem a versão. Somando: 5 linhas conferidas pela tela, 48 nesta tabela e essas 5 são as 58
de uma central recém-nascida; com as 3 do assistente, 61.

### Quantas linhas, e de que tamanho

- **Uma central recém-nascida tem 58 linhas** aos 65 s de ligada (1 da semente e 57 que a HMI
  grava). **Passa a 61 quando o assistente de configuração abre** pela primeira vez (as três do
  `5060`). É o caso da central ao vivo.
- **Usar não cria linha.** Mudar um ajuste regrava uma linha que já existe. Na sessão longa
  (`longa1`: 35 minutos, 14 trocas de tela, uma a cada 140 s) a memória ficou em 58 linhas do
  começo ao fim, entre 10.007 e 10.047 bytes, com a maior linha sempre em 1.929 letras (a
  declaração do AmFm). A guarda fez 17 cópias, nenhuma recusada, e terminou com "fim da sessão:
  igual". Entre uma tecla e outra nenhum valor mudou sozinho.
- **O teto, pelo código da HMI:** o `/vw/hmi/res/persistenceIds.res` lista 36 módulos, com 80
  linhas possíveis para as declarações e as partes comuns e mais 25 por usuário; há 44 chaves
  antigas declaradas e a do `6000`. Com os 7 lugares de usuário que o código prevê (`MAX_SLOT`),
  dá perto de 300 linhas. A lista está em `traces/m6_lsm_persistenceIds.txt`. É conta feita sobre
  o código, não medida: na bancada só existe o usuário 1.

## Como o firmware usa essa memória

Lido no código; a coluna da direita diz como foi conferido.

| Quem | O que faz | Conferido |
|---|---|---|
| `persistence-app`, função `init_pso_file` | na partida, lê o arquivo para uma tabela de até 500 valores; se o arquivo não existe, cria vazio | decompilado (`traces/m6_ghidra_persistence-app_lsm.log`) |
| HMI, `DSIPersistenceImpl.initLSMDataInFile` | quando nasce, lê o mesmo arquivo para uma tabela própria | lido no código da HMI |
| HMI, `DSIPersistenceImpl.readBuffer` | enquanto não houve nenhuma gravação, responde as leituras a partir dessa tabela, sem passar pelo barramento; depois da primeira gravação, pergunta ao `persistence-app` | lido no código |
| HMI, `AslTargetLscTNGBullhorn.processAslEventPowerOn` | na partida pede o idioma guardado (1101, chave 10) e confere se a variante tem esse idioma. Sem valor guardado, usa o idioma padrão do registro de diagnóstico `720B` | rodada `idi1`: a HMI registrou "persisted language 'pt_BR' is valid" e as telas saíram em português |
| HMI, `StageContentProcessor` (módulo `SystemCommon`, namespace 5051) | a cada troca de tela principal manda guardar o nome dela (`HMI_GRIDMENU`, `HMI_SYSTEMSETUP`, `HMI_NAVI`…) e, na partida, reabre a que estiver guardada. Sem nada guardado registra "No last mode persisted, use default!" e abre o rádio | lido no código; rodadas `mem1` a `mem3`: a HMI registrou "Last content in ScreenAreaStage: HMI_SYSTEMSETUP" e abriu os ajustes sozinha |
| HMI, `PersistableWriter.writeDirtyPersistable` | quando um bloco muda, grava três linhas de uma vez: a parte comum, a parte do usuário (se o bloco tem) e a declaração do módulo | lido no código; nas rodadas só a linha cujo conteúdo mudou sai diferente no arquivo |
| HMI, `DisplayBrightness.loadFromSharedPersistence` | na partida lê o brilho guardado (0 a 4) e pede ao serviço de display a porcentagem correspondente (`setDisplayBrightness`, api 1011) | escuta do barramento: com 02 guardado o pedido leva 50; com 04, 0 (campo vazio); com 00, 100 (rodadas `som1`, `bri2` e `bri3`) |
| HMI, carga dos módulos na partida | para cada bloco que não acha na memória registra "Could not load declaration data for module…" | 26 avisos numa central sem memória (`expl1`, `longa1`), 3 numa nascida das 58 linhas, 2 numa nascida de 61 (`expl2`) |
| `persistence-app`, `Persistence_WriteBuffer` | a cada valor que a HMI manda guardar, acrescenta a linha ao arquivo (ou regrava o arquivo inteiro, se a chave já existia), chama `sync` e copia para `lsmval_bak` | decompilado; na rodada `mem1` o arquivo foi de 1 para 58 linhas entre os 30 e os 65 s |

**O nome que a HMI dá a cada namespace.** Com o registro detalhado ligado (`HMILOG=normal`) ela
registra "Set persistence module >nome< associated with namespace N". Na rodada `idi1`
(`traces/m6_idi1.out`):

| Namespace | Módulo | Namespace | Módulo |
|---|---|---|---|
| 5050 | SystemEarly | 5068 | Car |
| 5051 | SystemCommon (tem a última tela e os ajustes do display) | 5069 | CarEarly |
| 5052 | AmFm | 5071 | FavoritesConfiguration |
| 5056 | Sound | 5077 | SimpleGridMenu |
| 5058 | Speech | 5078 | Iaa |
| 5059 | InstrumentCluster | 5079 | WebAppManagement |
| 5060 | GlobalSetupWizard | 5082 | Homescreen2 |
| 5061 | EntertainmentManager | 5083 | DigitalKey |
| 5065 | Homescreen | 5086 | CarFas |
| | | 5092 | OffClock |

São 19 dos 36 módulos que o `persistenceIds.res` lista (os outros 17, como Media, Navi, Online
e WebRadio, não se registram na bancada). **Corrigido em 11/10/2026:** aqui estava "os namespaces
5000, 5008 e 6000 … não sei de quem são". São chaves do jeito antigo de guardar
(`PersistenceNamespace`): 1101 é `LSC` (idioma), 5000 é `EARLY`, 5008 é `PHONE`; o 6000 não está
na tabela de nomes e é escrito direto pelo `PersistableEarlyData`. Estão na tabela "As outras
linhas".

## Como fazer um arquivo para outra escolha

Para outro idioma, basta trocar os cinco primeiros bytes da linha pelo código dele em letras
(`en_GB`, `de_DE`…). A HMI só aceita idioma que a variante tenha; se não tiver, registra
"persisted language … is INVALID" e sobe em inglês.

Para qualquer outra escolha, o caminho é o que deu origem ao arquivo do idioma: mudar pela tela
da central ao vivo e copiar a linha que a HMI gravou. Desde 11/10/2026 ela está em
`ao-vivo/memoria/lsmval`; com a central ligada também dá para ler direto dela:

```sh
docker exec moi3-ao-vivo cat /mnt/low/data/lge/hmi/lsmval
```

Para achar qual linha uma escolha mexe, comparar a memória de antes e a de depois com o leitor,
que desfaz o embrulho e diz a posição e o nome do campo:

```sh
python3 -I driver/lsmval_abre.py ao-vivo/memoria/anteriores/lsmval.1 ao-vivo/memoria/lsmval
```

O `diff` comum não serve bem para isso: a ordem das linhas muda de uma sessão para a outra, e um
valor comprimido muda inteiro quando um byte dele muda.

Mexer num byte à mão exige refazer o embrulho (a soma, ou a compressão). Foi feito uma vez, para
o brilho, numa central de teste (rodadas `bri2` e `bri3`); o programa usado está em
`traces/m6_lsm.lsm_troca.py.txt`.

## O que não foi conferido

- **O efeito das linhas da tabela "As outras linhas".** Sei o que guardam pelo código e que a HMI
  as lê na partida; não mudei nenhuma delas nem olhei o que fazem ao voltar.
- **Os campos sem tela** da linha `5051` `4311744512` (posições 4, 13, 15, 17, 18 e o que vem
  depois do nome da tela) e o `drivingDataSubViewId` do CarEarly, que passou de 0 a 1 sozinho na
  segunda vez que a tela do veículo abriu, na sessão longa.
- **O brilho pela tela.** A escolha não pega na bancada. Conferi só a volta: a HMI pede ao display
  a porcentagem do valor guardado, com o valor trocado à mão no arquivo. Para a tela funcionar o
  `dsistub` teria de responder aos pedidos 1011 e 1022; não mexi nele.
- **Som (grave, agudo, balanço), usuário e estações** não dão para mudar pela tela (tabela acima);
  o que aconteceria com eles numa partida seguinte não foi visto. Em particular, onde o carro
  guarda grave, agudo, balanço e volume é dedução. **Som: mudou em 12/10/2026 (Marco 13).** Com o
  som de mentira eles mudam pela tela e, medido numa partida, não vão para esta memória; no
  simulador ninguém os guarda, e a partida seguinte nasce do perfil. Onde o carro os guarda
  continua sendo dedução. **(Desde as 9h07 de 12/10/2026 voltam, no modo ao vivo: quem guarda é o som de mentira, em `ao-vivo/memoria/som/estado.txt`; esta memória continua sem eles.)**
- **Hora e data pela tela.** São pedidos ao carro, como as unidades; não tentei gravar. Das
  unidades só mudei a temperatura.
- **Mais de um usuário.** A troca de usuário não funciona na bancada; nenhuma linha de perfil 2
  foi vista. O teto de linhas é conta sobre o código.
- **Redefinir para configurações de fábrica.** A confirmação abre; cancelei. Não sei o que a
  restauração faz com a memória.
- **Trocar o idioma pela tela** não foi repetido nestas rodadas; a linha e a volta vêm das
  rodadas `idi1` e `mem2`.
- **Valor com soma errada.** Pelo código a HMI o descarta; não rodei.
- **O `driver/ao_vivo.sh` com o nome de verdade e a partida pela janela do Mac.** A central ao
  vivo estava no ar e não foi reiniciada por esta frente. O que rodou foi uma cópia do roteiro
  instalado, com outro nome de container e outra pasta `ao-vivo/` (rodadas `mem5` e `mem6`), e
  as centrais de teste `moi3-m6-*`, pelo `m3_ao_vivo.sh` com `MEMORIA`. **Mudou depois:** a
  central ao vivo foi religada pela janela na noite de 11/10/2026. A das 22h47 nasceu da memória
  (61 linhas, na grade de menus, em português), com a guarda viva e sem recusa; conferido em
  12/10, à 0h10, só lendo a saída do lançador, o registro da HMI e a pasta `ao-vivo/memoria/`.
- **Uma cópia recusada, com o firmware.** Em nenhuma rodada o `lsmval` da central deixou de
  passar pelo conferidor, nem na sessão de 35 minutos; a recusa só foi exercitada nos testes. Há
  três casos em que o conferidor recusaria um arquivo que o próprio firmware escreveu: mais de 500
  valores (o `persistence-app` acrescenta a linha mesmo com a tabela cheia), linha com mais de
  9998 letras (valor acima de uns 2.500 bytes) e valor de zero bytes (linha de três campos, que
  o `persistence-app` aceita e a HMI descarta). Em nenhum deles a memória guardada se perde, mas
  ela para de ser atualizada, com aviso. Não vi nenhum dos três, e o código diz por quê: o teto
  da HMI fica perto de 300 linhas; os 36 blocos cujo serializador achei no jar começam todos por
  2 bytes de versão, então não vejo de onde viria um valor de zero bytes (de outros oito blocos,
  de módulos que a bancada não registra, como Navi e Online, não achei o serializador); e a
  maior linha tem 1.929 letras. O que pode crescer são as listas: as 36
  memórias de estação do AmFm (2.642 bytes antes de comprimir, hoje todos zerados), a lista de
  aplicativos web e os ajustes de teclado. Com o rádio funcionando e as memórias cheias a linha
  pode passar de 9998 letras; na bancada não há como encher.
- **O corte seco.** Se o container morre de repente, vale a última cópia guardada: perde-se o que
  a HMI mandou guardar nos 2 a 3 s anteriores. Medi o `docker stop` (rodadas `mem2` e `expl1`), em
  que a central ainda roda 10 s e nada se perde; a morte instantânea não foi exercitada.
- **O tipo de voz** (sexto byte da linha do idioma): ficou como a HMI gravou, um espaço.
- **A página ao vivo serve a pasta `ao-vivo/` inteira para este computador** (127.0.0.1), só
  leitura. A memória fica legível em `/memoria/lsmval`, como já ficavam `log/` e `entrada/`. É
  leitura do `driver/servidor_ao_vivo.py`; não fiz o pedido.
