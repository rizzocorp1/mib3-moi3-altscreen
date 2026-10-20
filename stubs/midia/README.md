# stubs/midia — a mídia de mentira da bancada

Marco 15, 12/10/2026. No carro, a tela de mídia conversa com um programa do firmware que lê
pendrive, Bluetooth e as outras fontes, e que toca as faixas. Na bancada não há aparelho nem som, e
esse programa não sobe. O `driver/midiastub.c` (a "mídia de mentira") ocupa o lugar dele: anuncia
o domínio de mídia, diz que há uma fonte com algumas faixas, e atende aos pedidos de ativar a
fonte, trocar de faixa e listar as faixas.

**É estudo num computador. Nada daqui vai para o carro.** E é simulação do começo ao fim: as
faixas são inventadas, não há arquivo de música e não sai som.

O registro da investigação está em [../../modelagem-soc.md](../../modelagem-soc.md), seção do
Marco 15.

## Como usar

- **Modo ao vivo:** `sh driver/ao_vivo.sh` (e o botão da janela) já sobe com a mídia de mentira e o
  perfil `minhas-midias.txt`. A tecla Mídia abre o tocador no lugar do aviso "função indisponível".
- **Sem a mídia de mentira**, como antes do Marco 15: `MIDIA= sh driver/ao_vivo.sh`. A tecla Mídia
  volta ao aviso.
- **Outro perfil:** `MIDIA="/stubs/midia/minhas-midias.txt /stubs/midia/meu.txt" sh driver/ao_vivo.sh`.
  Os caminhos são os de dentro do container; o arquivo de depois vale sobre o de antes, campo a
  campo. O perfil é lido na partida.
- **No lançador direto** (`driver/m1_hmi.sh`) e **nas centrais de teste** (`driver/m8_central.sh`,
  que chama o `driver/m3_ao_vivo.sh`) a opção é a mesma variável `MIDIA`, e vem **desligada**:
  `MIDIA=/stubs/midia/minhas-midias.txt sh driver/m8_central.sh sobe <nome> <pasta>`.
- **A mídia de mentira mexe na partida da HMI** (medido em 12/10/2026, depois do Marco 15). Com
  ela e o rádio respondendo, a HMI deixa de esperar 2 s por eles, e a etapa que precisa dos
  serviços de veículo chega 0,7 s antes de eles existirem: a central nasce com quatro assinaturas
  de veículo a menos e, se nascer na tela do veículo, sem sete serviços e com o computador de
  bordo só em traços. **No modo ao vivo isso está resolvido desde as 4h08 de 12/10/2026:** o
  `driver/ao_vivo.sh` dá 6 s aos serviços DSI (`ESPERA_DSI`; vazio volta aos 2 s), ao custo de a
  última etapa da partida começar aos 50 s em vez de aos 42 s. **Numa central de teste, não:** para
  ter o mesmo, somar à linha acima `EXTRA_VMOPTIONS=-Dde.vw.mib.asl.startup.timeout.dsi=6000`.
  Medidas no `modelagem-soc.md`, Marco 15, "A espera da partida pelos serviços de veículo".
- **Ver o que um perfil publicaria**, sem subir nada (dentro do container):
  `midiastub -m /stubs/midia/gerado/esquema.txt <perfil>...`
- **Ver o que a tela pediu e o que foi feito:** o registro (`midiastub.log`) traz uma linha por
  pedido. Numa central de teste: `docker exec <container> cat /mnt/low/tmp/midiastub.log`. As
  linhas `[midiastub]` são do modelo do tocador e do navegador; as `[veiculostub]` são do motor,
  que é o mesmo do carro de mentira.
- Perfil sem fonte ou sem faixa, linha errada no perfil, ou esquema que não é o de mídia: nada
  sobe, e a mensagem diz o arquivo e a linha, ou o que faltou.

## Os arquivos

| Arquivo | O que é | De onde veio |
|---|---|---|
| `minhas-midias.txt` | o perfil: uma fonte, que a tela chama de "Minhas mídias", com uma pasta e cinco faixas inventadas ("FAIXA DE TESTE 1" a "5", de dois artistas e dois álbuns "DE TESTE"); o que o tocador diz saber fazer; e as duas respostas do navegador de arquivos | **SIMULAÇÃO**: nenhum valor lido do carro, de um pendrive ou de um celular |
| `gerado/esquema.txt` | o esquema dos doze serviços do domínio de mídia (`DSIMediaBase`, `DSIMediaPlayer`, `DSIMediaBrowser`, `DSIFileBrowser`, os de UPnP, a busca e outros): interfaces, números de api, 81 atributos, 174 mensagens de evento, 190 pedidos e os campos de 422 mensagens. Não editar à mão | gerado por `driver/m8_esquema.sh` das classes do firmware |
| `gerado/constantes.txt` | o que cada número quer dizer (tipos de aparelho e de mídia, estados, modos): 942 constantes | idem |
| `gerado/relatorio.txt` | contagens por serviço e as esquisitices do próprio firmware | idem |
| `gerado/vetores-perfil.txt` e `vetores-esperado.txt` | a prova do codificador: um perfil que dá valor a toda folha de todo evento e os bytes que as classes do firmware produzem para ele (173 mensagens) | idem |

Para refazer a pasta `gerado/` (só lê o firmware, sem rede):

```bash
docker run --rm --privileged --network none -v "$PWD/sysroot/mu0883":/fw:ro -v "$PWD/driver":/driver:ro -v "$PWD/stubs/midia/gerado":/out -e DOMINIOS=0x800000 -e ROTULO="de PID_MEDIA" moi3-bancada:1 sh /driver/m8_esquema.sh
```

O rótulo vai sem acento de propósito: com acento, o cabeçalho dos arquivos gerados sai com a
letra estragada (o argumento chega à JVM do firmware fora de UTF-8). Com outras centrais no ar, o
mais seguro é trocar `$PWD/stubs/midia/gerado` por uma pasta à parte e depois trazer os arquivos
com `mv`, um a um.

## Formato do perfil

É o do carro de mentira: ajustes (`grava`, `ignora`), seções `[Serviço.evento]` com
`caminho = valor`, e seções de regra `[Serviço.pedido -> evento]`. Está descrito em
[../veiculo/README.md](../veiculo/README.md). O que muda são os nomes, que aqui são os do
`gerado/esquema.txt` desta pasta. Para achar: `grep '^E DSIMediaPlayer ' gerado/esquema.txt`.

O que este perfil precisa ter, e por quê:

- **Um aparelho de um tipo que a HMI aceite como fonte.** Sem aparelho nenhum na lista
  (`[DSIMediaBase.MBLupdateDeviceList]`) a HMI nem liga as fontes, e a tela fica no aviso. E ela só
  põe em "Fontes" os tipos 0 (o que a tela chama de "Minhas mídias"), 4 (CD interno), 8 (AUX),
  10 (Bluetooth) e 15 (mídia sob demanda): é o filtro `DeviceFilter.MEDIA` do código dela,
  conferido no bytecode. **Pendrive (tipo 2) e cartão (tipo 1) não aparecem em "Fontes" nesta HMI**
  (medido na rodada `m4`). O teste confere o tipo no perfil da pasta.
- **A mídia do aparelho** (`[DSIMediaBase.MBLupdateMediaList]`), de tipo 7 (arquivos), 9, 10, 20
  ou 27, com `flags` sem os bits de erro e de autenticação.
- **As faixas**, na seção `[DSIMediaBrowser.MBRLresponseList]`: é a lista que o navegador devolve,
  e é dela que o modelo tira tudo (título, artista e álbum em `extInfo`, duração em segundos).
  Cada faixa precisa de um `entryID` diferente: é por ele que a HMI pede a faixa.
- **A pasta**, na seção `[DSIMediaBrowser.MBRLupdateBrowseFolder]`. Há uma só.
- **As duas seções do navegador de arquivos** (`FBLsetLanguageResult` e
  `FBLsetFileTypeActiveResult`), vazias: sem elas a HMI registra dois tempos esgotados de 10 s na
  partida.

## O que a mídia de mentira faz

| A HMI | A mídia de mentira | Quem faz |
|---|---|---|
| assina atributos (aparelhos, mídias, capacidades, modos, pasta, tamanho da lista…) | manda cada um com o valor do perfil ou, sem perfil, o padrão marcado como válido | o motor |
| `setActiveMedia(aparelho, mídia)`: a tecla Mídia, ou a escolha em "Fontes" | manda, nesta ordem, a mídia ativa, as capacidades, os modos, a faixa de agora e o estado "em pausa". A HMI só dá a fonte por ativada depois de receber as capacidades e o estado **depois** da mídia ativa | o modelo |
| `setActiveMedia()` sem campos: a HMI saindo da mídia | manda a mídia ativa vazia | o modelo |
| `requestDetailInfo(entrada)` | os dados da faixa: título, artista, álbum, número e total | o modelo |
| `skip(0 ou 1, quantas)`: as setas do tocador e as teclas Próxima e Anterior | vai para a faixa seguinte ou a anterior (do fim volta ao começo) e manda a posição e o estado | o modelo |
| `play`, `resume`, `pause`, `stop` | muda o estado. A faixa não anda sozinha | o modelo |
| `setContentFilter`, `changeFolder` | manda a pasta do perfil e o tamanho da lista, só para a instância que pediu. A HMI só segue quando a pasta e um tamanho maior que zero chegam depois do filtro | o modelo |
| `requestBrowsePath`, `requestList` | o caminho da pasta; um trecho da lista de faixas, ou a entrada em foco com o índice dela | o modelo |
| `resetSelection`, `addSelection` e, no tocador, `setPlaySelection`: o toque numa faixa da lista | resultado 0, a conta das faixas, e a faixa tocada vira a faixa de agora | o modelo |
| `setPreferredLanguage`, `setBrowseMedia`, `setBrowseMode` | grava o atributo e avisa quem o assina | o motor, pela regra do esquema |
| os pedidos da busca (`DSISearch`, interface 1301) | nada: a busca fica de fora (ver "Limites") | ninguém |

## Limites

- **Sem som e sem aparelho.** Não há arquivo de música; as faixas são linhas de um perfil.
- **A faixa fica em pausa.** Sem o serviço de áudio a HMI nem manda tocar: o comando de retomar
  termina com "resume postponed as media is not audible after demute" (medido). O tempo fica em
  0:00, com a duração da faixa ao lado.
- **Depois de ir da mídia ao rádio, a volta à mídia abre o tocador vazio**, até a central
  reiniciar: o gerente de entretenimento da HMI fica esperando a ativação do rádio, que não termina
  sem o serviço de áudio (medido na rodada `p3`; a causa é leitura do código). O rádio segue
  funcionando.

  > **Nota de 12/10/2026, 3h50, da sessão do som (Marco 13):** estes dois limites valem **sem** o
  > som de mentira (`SOM=`). Com ele, que o modo ao vivo liga desde as 3h35 de 12/10/2026, a HMI
  > pede a ligação de áudio ao serviço, manda `resume` e a tela mostra a faixa **tocando** (o tempo
  > continua em 0:00: este programa não faz a faixa andar); ao ir para o rádio ela manda `pause`; e
  > a volta à mídia traz a faixa, tocando. Com a faixa em pausa a HMI põe um alto-falante riscado
  > na barra do topo. Medido em cinco partidas lado a lado com duas sem som, duas delas da pasta
  > instalada. Detalhes em [../som/README.md](../som/README.md) e no Marco 13 do `modelagem-soc.md`.

- **Teclas Próxima e Anterior, do volante e da moldura** (medido em 12/10/2026, às 9h, Marco 16):
  com o som de mentira elas trocam de faixa na tela de mídia; sem ele (`SOM=`) a HMI as recebe e
  descarta, porque só as aceita quando o serviço de áudio diz que a mídia é a fonte que toca. As
  setas da tela trocam nos dois casos. **O aperto longo não faz nada:** a HMI tentaria o avanço
  rápido, mas o perfil não declara essa capacidade, e o programa não faz a faixa andar. A tecla
  Fonte do volante, apertada na mídia, leva ao rádio e não volta.
- **Uma pasta só, sem subpastas, capas nem listas de reprodução.** Repetir e embaralhar aparecem
  e não mudam nada: o perfil tem um modo só.
- **Só a fonte "Minhas mídias".** Pendrive e cartão esta HMI não lista em "Fontes"; áudio por
  Bluetooth depende do telefone e não foi simulado.
- **A busca (`DSISearch`) fica de fora.** O esquema do domínio a traz, mas cada instância dela
  pertence a um domínio diferente, e o hub da bancada entrega tudo a todos: respondendo, este
  programa falaria pelas instâncias dos outros.
- **Nada sobrevive à partida seguinte do lado do tocador.** A última fonte quem guarda é a HMI.
- **O esquema é do MU0883.** O carro roda o MU0803; os números podem diferir.
- **Só no container.** Não rodou no QEMU.

## De onde sai cada informação

| Informação | Fonte |
|---|---|
| o domínio e os serviços que o anúncio liga | `com.lge.mib3oi.dsi.DSIDomain` (`PID_MEDIA`, `0x800000`) e as linhas `W` do esquema; conferido no barramento da rodada `m1` |
| interfaces, apis, campos e codificação | o gerador (`driver/LeEsquemaDSI.java`), das classes do firmware, com a codificação provada pelos vetores |
| do que a tela precisa para sair do aviso | `de.vw.mib.asl.internal.devicehandling.DevicesImpl` e `DeviceHandlingTarget`, e as rodadas `m1` a `m3` |
| quais tipos de aparelho viram fonte | bytecode de `de.vw.mib.asl.api.devicehandling.clients.DeviceFilter$5`, e a rodada `m4` |
| a ordem das mensagens na ativação | `de.vw.mib.asl.internal.media.playback.MediaPlayer` (`rqMediaActivation`, `detectActivationCompleted`) e `…controller.commands.playback.CmdActivatePlayer`, e as rodadas `m5` e `p1` |
| o que o navegador espera | `de.vw.mib.asl.internal.media.browser.AbstractMediaBrowser` e `…commands.browsing.CmdSetupBrowser`, e as rodadas `p2` a `p4` |
