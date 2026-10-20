# janela — a central MOI3 numa janela do Mac

Um programa do Mac que mostra a tela da central do sandbox numa janela própria e manda toque, teclas e botões giratórios para ela. Feito em 11/10/2026, a pedido do Lipe ("faz a transmissão direta com a janela própria"). É estudo num computador: a HMI do firmware roda no container da bancada e nada vai para o carro.

## Abrir

```sh
sh referencia/ferramentas/mib3-sandbox/janela/abre.sh
```

Roda de qualquer pasta. Na primeira vez compila (uns 15 segundos); depois abre na hora. Não instala nem baixa nada: usa o `swiftc` do Command Line Tools e o que já vem no macOS. O programa fica em `build/Central MOI3.app` e também abre com dois cliques no Finder.

Se a central estiver desligada, a janela mostra o botão **Ligar a central**. Ele roda o `../driver/ao_vivo.sh` por 2 horas, sem a página do navegador. A primeira tela leva cerca de um minuto. Fechar a janela **não** desliga a central: para isso, menu Central → Desligar a central, ou `docker stop moi3-ao-vivo`.

A janela e a página do navegador (`../ao-vivo/`) funcionam ao mesmo tempo, na mesma central.

## O que tem na janela

| Onde | O que faz |
|---|---|
| Tela | clicar é tocar; arrastar é rolar |
| Teclas embaixo da tela | Menu, Rádio, Mídia, Telefone, Carro, Clima, Som, Navegação, Ajustes e, desde 12/10/2026, App (a tecla APP da moldura do Jetta). Ficam apertadas enquanto o mouse estiver apertado, para a HMI ver o toque longo |
| Botão redondo da esquerda | volume: rolar em cima gira, clicar aperta; o − e o + giram um passo |
| Botão redondo da direita | seleção: rolar em cima gira, clicar escolhe |
| Faixa "Volante", embaixo do aparelho | as onze teclas do volante: Vol −, Vol +, Rolete, Mudo, Anterior, Próxima, ▼, ▲, Fonte, Voz, Telefone. Ficam apertadas enquanto o mouse estiver apertado, como as da moldura. O que cada uma faz está em [Teclas do volante](#teclas-do-volante-12102026) |
| Rodapé | luz verde "Ao vivo", âmbar "Sem quadro novo há N s" (a HMI só desenha quando algo muda) ou vermelha sem ligação; tamanho da tela, quadros por segundo e número do quadro; o último comando mandado |

Teclado do Mac, com a janela na frente:

| Tecla | O que faz |
|---|---|
| 1 a 9 | as nove primeiras teclas da moldura, na ordem em que aparecem (a App, que é a décima, não tem atalho) |
| seta para baixo ou para a direita | gira a seleção um passo adiante |
| seta para cima ou para a esquerda | gira a seleção um passo para trás |
| Return | aperta o botão de seleção |
| − e = | volume para menos e para mais |
| ⌘S | salva a tela em PNG |
| ⌘L | liga a central (quando está desligada) |

As teclas do volante não têm atalho no teclado do Mac: só o mouse.

## Como funciona

```
janela (Mac)  ── docker exec -i moi3-ao-vivo ponte ──►  ponte (container)
   ▲ quadros pela saída padrão                            │ lê o soquete do glhost
   └ comandos pela entrada padrão ──────────────────────► └ acrescenta no arquivo que o dsistub acompanha
```

O container da bancada roda sem rede, e continua assim. O que atravessa é a entrada e a saída padrão de um `docker exec -i`. Do lado de dentro está a `../glforward-moi3/ponte.c`: ela copia para fora a transmissão dos quadros do glhost e grava no arquivo de comandos as linhas que chegam (`tecla`, `gesto`, `encoder`, o mesmo formato do `../driver/dsistub.c`).

A transmissão manda só o que mudou em cada quadro, no instante em que a HMI o desenha. O formato está descrito no `../glforward-moi3/host.c` e no [README do encaminhador](../glforward-moi3/README.md). A janela monta a imagem numa thread e entrega à tela; se a janela atrasar, os quadros se juntam e só o último é mostrado.

O visual (cores, teclas, botões, textos) é o mesmo da página `../ao-vivo/index.html`.

## Arquivos

| Arquivo | O que é |
|---|---|
| `janela.swift` | o programa inteiro: leitura dos quadros, ligação com a central, janela, modos sem janela e roteiro |
| `build.sh` | compila e monta `build/Central MOI3.app` |
| `abre.sh` | compila se preciso e abre |
| `build/` | o programa compilado (pode ser apagado; o `abre.sh` refaz) |

## Opções do programa

O programa compilado é `build/Central MOI3.app/Contents/MacOS/central-moi3`. Pelo `abre.sh`, as opções vão depois do nome do script.

| Opção | Para quê |
|---|---|
| `--container <nome>` | outra central que não a `moi3-ao-vivo` |
| `--ponte`, `--soquete`, `--entrada` | os caminhos de dentro do container, se mudarem (defaults no `janela.swift`) |
| `--docker <caminho>` | onde está o `docker`, se a janela não achar |
| `--pasta <mib3-sandbox>` | onde está a bancada, para o botão de ligar (a janela acha sozinha a partir de `build/`) |
| `--segundos <n>` | por quanto tempo a central fica no ar quando é ligada pela janela (default 7200) |
| `--decodifica <bruto> <ppm>` | sem janela: aplica uma transmissão gravada e grava a imagem final |
| `--captura <ppm> [segundos]` | sem janela: liga na central, lista cada quadro com a hora em que chegou e grava o último. Com `--manda "tecla 13 78 1;tecla 13 78 0"`, manda os comandos um segundo depois do primeiro quadro e diz em quanto tempo a tela respondeu |
| `--roteiro "<passos>"` | a janela se usa sozinha, pelos mesmos eventos que o mouse e o teclado geram. Passos, separados por `;`: `espera <s>`, `toque <x> <y>`, `arrasta <x0> <y0> <x1> <y1>`, `tecla <nome>`, `volante <nome>` (nomes na tabela das [teclas do volante](#teclas-do-volante-12102026)), `aperta volume\|selecao`, `passo volume\|selecao <+1\|-1>`, `rola volume\|selecao <dentes>`, `digita <tecla>`, `foto <png>`, `camadas <png>`, `quadro <png>`, `liga`, `desliga`, `sai` |

## Teclas do volante (12/10/2026)

Item 6 do roteiro do [simulador](../simulador-jetta.md), feito por uma sessão à parte na madrugada de 12/10/2026. A faixa "Volante" tem as onze teclas que a tabela de teclas da HMI traz para o volante (teclado 4, que as constantes do firmware chamam de `KBD_MFW`; a tabela está em `../traces/m5_tabela_teclas.txt`). Cada tecla sai como `tecla 4 <código> 1` ao apertar e `tecla 4 <código> 0` ao soltar. A página do navegador tem a mesma faixa.

**Atualização de 12/10/2026 (escrita às 4h43, no fechamento da noite, refeita às 10h12 com o que as sessões mediram de manhã, e de novo às 17h17, quando a barra de volume pelo volante entrou).** O que vem mais abaixo foi medido à 1h, quando a bancada não tinha rádio, mídia, telefone nem som. Desde as 4h08 o modo ao vivo sobe com os quatro, de mentira, e as sessões que os fizeram repetiram as onze teclas em centrais de teste. O que cada tecla faz agora (os marcos são do [modelagem-soc.md](../modelagem-soc.md)):

| Tecla | Código | O que faz com rádio, mídia, telefone e som de mentira ligados | Medido por |
|---|---|---|---|
| Vol −, Vol +, Rolete | 43, 42, 44 | **abrem a barra de volume e mudam o valor**, um passo por aperto (o Rolete, os passos do giro); segurar Vol + ou Vol − não repete. Isto é simulação, ligada por padrão desde as 17h03 de 12/10/2026: o Lipe disse que no carro dele a barra aparece na tela da central (de memória, sem foto), e o som de mentira faz o stub de entrada girar também o botão da central; cada tecla vira uma linha `encoder 13 17 ±n` a mais no arquivo de comandos. Pela leitura do firmware da bancada (MU0883) a central não abriria a barra pelo volante (a chave de recurso vem desligada; quem mostra é o painel): com `volante_abre_a_barra = nao` num segundo perfil do som o simulador volta a isso, e o volume muda sem a barra abrir | sessão do som, Marco 13 (decisões D165 a D168) |
| Mudo | 57 | liga e desliga o mudo; o alto-falante riscado aparece na barra do topo | sessão do som, Marco 13; e por mim, na partida de prova das 4h34 ([quadro](../quadros/2026-10-12-hmi-fecha-prova-de-partida.png), última tela) |
| Próxima, Anterior | 46, 47 | na tela do rádio trocam de emissora; na tela de mídia trocam de faixa, **desde que o som de mentira esteja ligado** (é o padrão do modo ao vivo). Sem o som não trocam: a tela de mídia só aceita essas teclas quando o serviço de áudio diz que a mídia é a fonte que toca. O aperto longo não faz nada | sessão do rádio, Marcos 11 e 16 |
| ▼, ▲ | 49, 48 | recebidas; nada, na mídia, no rádio, na grade de menus e na tela do telefone | sessão do rádio, Marco 16 |
| Fonte | 45 | é tecla do rádio nesta HMI: na tela de mídia sai da mídia e abre o rádio em FM; na tela do rádio alterna FM e AM; na grade e na tela do telefone troca a faixa do rádio por trás, sem mudar a tela. Não volta para a mídia. Curta e segura fazem o mesmo | sessão do rádio, Marco 16 |
| Voz | 50 | curta: nada. Segura por 1,2 s na tela de mídia: a HMI pede ao Bluetooth o modo acessível e a tela não muda (só na primeira vez em cada central). Nas outras telas, nada | sessão do rádio, Marco 16 |
| Telefone | 53 | não muda a tela, sem aparelho e com o aparelho fictício conectado. Durante uma chamada não foi medido: a bancada ainda não tem chamada | sessão do telefone, Marco 12 |

**Quais dessas teclas o volante do carro tem (12/10/2026, 17h29).** O Lipe informou, de memória, que no raio esquerdo há volume + e − e as teclas do piloto automático, e não marcou mudo nem telefone. Com o raio direito da foto de 09/10/2026 (voz, ▲, ▼, anterior, próxima, `OK` e `VIEW`), das onze teclas desta faixa o carro tem Vol −, Vol +, Voz, ▲, ▼, Anterior e Próxima. Mudo e Telefone ele não marcou; Fonte e Rolete não foram perguntados. As teclas do piloto automático não aparecem na tabela de teclas da HMI: não passam pela central. A faixa continua com as onze, porque a HMI aceita todas.

**A moldura do carro, na mesma foto:** RADIO, MEDIA, PHONE, NAV, APP, CAR e MENU, mais os dois botões giratórios. A janela tem Clima, Som e Ajustes a mais, que a HMI aceita e a moldura do carro não traz. **Desde 12/10/2026, 21h43, a janela e a página têm também a tecla App** (na tabela da HMI ela é a 114, `KEY_SMARTPHONE`; sai como `tecla 13 114 1` e `tecla 13 114 0`). Teste antes do código: o roteiro `tecla app` respondia "tecla desconhecida" e passou a mandar as duas linhas. Numa central de teste com os padrões de agora (a central marcada como tendo CarPlay), a tecla abre a tela "App-Connect: A função não está disponível." ([quadro](../quadros/2026-10-12-hmi-fecha-prova-grade-e-tecla-app.png)). A fileira de teclas da janela coube com a décima tecla; a da página não foi olhada num navegador, só o `t_servidor.sh` (20 casos, passando).

Tudo isso foi mandado pelo arquivo de comandos ou pelo roteiro das centrais de teste, não pelo mouse na janela. A frase "sozinha, nenhuma tecla do volante muda a tela" e a coluna "Sozinha, na bancada" da tabela de baixo valem para a bancada como estava à 1h, sem esses quatro.

**O que dava para ver à 1h de 12/10/2026.** Sozinha, nenhuma tecla do volante muda a tela nesta bancada: a HMI recebe todas, mas faltam o rádio, a mídia, o telefone e o som, que é com quem elas falam. O efeito aparece com a barra de volume aberta. Gire o botão de volume da moldura: a barra "Áudio" abre no alto da tela e fecha sozinha em 3 s. Com ela aberta:

- **Vol −, Vol + e Rolete** seguram a barra: ela passa a fechar 3 s depois de a tecla ser solta;
- **Mudo, Anterior, Próxima, Voz e Telefone** fecham a barra na hora;
- **▼, ▲ e Fonte** não mudam nada.

![A janela com a faixa do volante e a barra de volume aberta](../quadros/2026-10-12-janela-mac-volante-barra-de-volume.png)

| Tecla | No roteiro | Código e constante | O que a HMI tem para ela (lido no firmware) | Sozinha, na bancada | Com a barra de volume aberta |
|---|---|---|---|---|---|
| Vol − | `vol-` | 43, `KEY_MFW_VOLUME_DOWN` | nenhum evento na tabela; tratada no código: ao soltar, um passo de volume para menos | recebida; a tela não muda | a barra fecha 3,00 s depois de soltar (1 medida) |
| Vol + | `vol+` | 42, `KEY_MFW_VOLUME_UP` | o mesmo, para mais | recebida; a tela não muda (curta e segura por 2 s, na grade; curta, na tela de som) | a barra fecha 3,00 s depois de soltar (5 medidas) |
| Rolete | `rolete` | 44, `KEY_MFW_ROLLER_RIGHT` | nenhum evento na tabela; no código, apertar e soltar vale um passo de volume "zero" | recebida; a tela não muda | a barra fecha 3,00 s depois de soltar (1 medida) |
| Mudo | `mudo` | 57, `KEY_MFW_MUTE` | `HK_MUTE_PRESSED` e `HK_MUTE_RELEASED` (309, 308), os mesmos da tecla de mudo da moldura (88) | recebida; a tela não muda | a barra fecha no instante do aperto (5 medidas); reaberta depois, a barra está igual, sem sinal de mudo |
| Anterior | `anterior` | 47, `KEY_MFW_ARROW_A_DOWN` | `HK_PREV_*` (346, 347, 344, 348, 345), os mesmos da tecla 10 da moldura | recebida; nada na grade, no rádio e na mídia | fecha no instante do aperto (1 medida) |
| Próxima | `proxima` | 46, `KEY_MFW_ARROW_A_UP` | `HK_NEXT_*` (336, 337, 358, 338, 357), os mesmos da tecla 14 da moldura | recebida; nada na grade, no rádio e na mídia | fecha no instante do aperto (1 medida) |
| ▼ | `baixo` | 49, `KEY_MFW_ARROW_B_DOWN` | nenhum evento na tabela e nenhum tratamento no código que li | recebida; nada | nada: a barra fecha sozinha aos 3,00 s (1 medida) |
| ▲ | `cima` | 48, `KEY_MFW_ARROW_B_UP` | o mesmo | recebida; nada | nada: 3,00 s (1 medida) |
| Fonte | `fonte` | 45, `KEY_MFW_AUDIOSOURCE` | nenhum evento na tabela | recebida; a tela não muda (13 apertos: 5 na grade, 5 no rádio, 3 na mídia) | nada: 3,00 s (1 medida) |
| Voz | `voz` | 50, `KEY_MFW_PTT_ON` | `HK_PTT_*` (352, 353, 349, 354, 350) | recebida; a tela não muda. Segura por 1,2 s, a HMI tenta falar com o Bluetooth e registra erro | fecha no instante do aperto (1 medida) |
| Telefone | `telefone` | 53, `KEY_MFW_HOOK` | `HK_MFL_PHONE_*` (314, 313, 316, 310, 315) | recebida; a tela não muda (curta e segura por 2,2 s) | fecha no instante do aperto (1 medida) |

Controle: só com o botão de volume da moldura, a barra ficou aberta 3,00 s em quatro medidas e 3,01 s em uma.

**Como foi medido.** Numa central de teste (`moi3-vol-t1`, de 0h34 a 1h03 de 12/10/2026), subida da pasta com os padrões do modo ao vivo (identidade provisória do Jetta, carro de mentira, português) e com o registro de teclas da HMI ligado. Três levas pelo arquivo de comandos, depois o roteiro da janela e a página:

- "recebida" é a linha `HK Received: KBD[4] KEY[<código>] KST[<estado>]` no registro da HMI, que saiu para as onze teclas, ao apertar e ao soltar;
- "a tela não muda" é o número do quadro igual antes e depois, e nenhuma linha de tela ou de popup (`enterView`, `openPopupView`) no passo. Uma exceção: no passo do Vol + na tela de som o contador andou um quadro, sem nada no registro; o passo atravessou a virada do minuto e não fui atrás;
- os tempos da barra saem das horas de `openPopupView` e `closePopupView` do popup `PO_SOUND_VOLUME_SOUND_ENTERTAINMENT` no mesmo registro.

Nessas levas rádio e mídia estavam no aviso "função indisponível" (o rádio simulado é de outra sessão e ainda não estava na pasta). Os roteiros das levas, os resumos, o registro da HMI, o do `dsistub` e o do barramento estão em `../traces/vol_*`. A conta fecha: 142 linhas `tecla 4 …` no arquivo de comandos, 142 no `dsistub` e 71 apertos mais 71 solturas na HMI; nenhum comando recusado.

Essa central rodou com a biblioteca de barramento de antes de 12/10/2026, 01h13 (`../driver/kipc_bus.c` de 10/10, conferido pela data do arquivo às 01h12). A sessão do consumo de processador avisou que trocaria a biblioteca nessa hora e que, depois da troca, tempos, contagem de quadros e consumo não se comparam com os de antes. Os 3,00 s da barra de volume são de um relógio da própria HMI e não devem mudar, mas isso não foi medido com a biblioteca nova.

**O que é leitura e o que é dedução.**

- Lido no firmware (classes decompiladas em 10/10/2026): os nomes das teclas e dos teclados (`org.dsi.ifc.keypanel.Constants`), os nomes dos eventos (`ASLSystemHKEventConstants`), e o tratamento à parte das teclas 42, 43 e 44 em `AslTargetSystemKeyPanelHandling.processKeyEvent`: ao soltar, a HMI dispara o evento `VOLUME_CHANGE_THROUGH_MFL` (232); segurando mais de 0,8 s (o tempo longo da tabela para 42 e 43), um passo a cada 0,2 s. O mesmo arquivo aceita o giro do rolete (`encoder 4 44 <passos>`).
- Dedução minha, a partir do que foi medido: a barra de volume é segurada pelo evento 232 e fechada por qualquer tecla que tenha evento na tabela. Não li o código do popup.
- Dedução, sem como verificar aqui: no carro, quem muda o volume é o sistema de som, e a barra aparece porque ele avisa a HMI; por isso Vol − e Vol + sozinhas não abrem a barra na bancada, onde não há serviço de som. Anterior e Próxima devem trocar de faixa ou de emissora quando houver rádio ou mídia; Voz e Telefone dependem do telefone. Nada disso foi visto. **Medido depois, na mesma madrugada, pelas sessões do som, do rádio e do telefone:** a primeira parte não se confirmou na bancada (com o som de mentira avisando o volume novo, Vol − e Vol + mudam o volume e a barra continua fechada; o que acontece no carro segue sem verificação); Próxima e Anterior trocam de emissora e, com o som de mentira ligado, de faixa (medido na manhã de 12/10/2026; sem o som não trocavam); e a tecla Telefone não abre a tela do telefone. Ver a atualização no começo desta seção.
- **O volante do carro, até onde a foto mostra.** A [foto do painel de 09/10/2026](../../../../carros/jetta-gli-2021/fotos/2026-10-09-painel-digital-mapa/cluster-mapa-nativo-e-mib3.jpg) pega o raio direito do volante do Jetta: voz, seta para cima, `OK` com uma tecla de cada lado, `VIEW`, seta para baixo, faixa anterior e próxima faixa. Então o carro tem botão de voz, de seta para cima e para baixo, de faixa anterior e de próxima. `OK`, as duas teclas ao lado dele e `VIEW` não estão na tabela da HMI para o volante, e por isso não entraram na faixa. O raio esquerdo não aparece na foto: que o carro tenha Vol −, Vol +, Mudo, Telefone e Fonte é suposição, e o Rolete é o palpite mais fraco (é tecla de volante com rolete de volume). A [ficha](../../../../carros/jetta-gli-2021/README.md) ganhou a transcrição da foto em 12/10/2026.
- **Qual botão do carro manda qual código também é suposição.** Liguei pelo sentido: faixa anterior e próxima às teclas 47 e 46, que a tabela leva a `HK_PREV` e `HK_NEXT`; as setas às 49 e 48; a voz à 50. Mas as constantes do firmware têm outro par de setas, `KEY_MFW_UP` e `KEY_MFW_DOWN` (38 e 39), fora da tabela, e não sei qual dos dois pares o volante do Jetta manda. Na bancada não há como conferir.

**Observações soltas, sem causa atribuída.**

- Num dos cinco apertos de Fonte com a tela do rádio na frente, 2 ms depois de soltar a tecla o gerente de entretenimento da HMI registrou um pedido de ativação do rádio (`RadioActivationRequest`); nos outros quatro, nada. No registro inteiro esse pedido só aparece mais uma vez, na partida.
- O giro do rolete (`encoder 4 44 2` e `-2`, mandados pelo arquivo de comandos) sozinho não mudou a tela; com a barra aberta, ela ficou 4,66 s (1 medida). A janela e a página só oferecem o aperto do rolete, não o giro.

**Conferência da janela e da página (12/10/2026).**

- A janela compila sem aviso (`swiftc` 6.4). O modo `--decodifica` dá a mesma imagem de antes: aplicado à transmissão gravada no cenário 3 do `t_transmissao.sh`, o resultado é igual, byte a byte, ao quadro que o glhost gravou, ao do espectador de teste em C e ao do programa de antes desta mudança (SHA-256 `7c49967b8d9c…`). Transmissão cortada e lixo: código 2, sem arquivo de saída. O `t_transmissao.sh`, rodado dentro da central de teste para gerar a gravação, passou nas 19 conferências.
- Roteiro da janela na central de teste: Menu, as onze teclas do volante, Som, um passo do botão de volume, Vol + do volante, outro passo, Mudo do volante. As 32 linhas chegaram ao arquivo de comandos na ordem, iguais às que a janela disse ter mandado, e a HMI registrou cada tecla na mesma ordem. Nome errado no passo `volante` só avisa e segue.
- Página (cópia de trabalho, servida pelo `servidor_ao_vivo.py` numa porta de teste): as onze teclas clicadas gravaram 22 linhas `tecla 4 …` na ordem; ligada à central de teste, Som, volume, Vol + e Mudo deram o mesmo resultado da janela na HMI. Sem central, a faixa do volante some junto com a da moldura; em 1280 × 900 nada rola. O `../driver/tests/t_servidor.sh` passa nos 20 casos.
- A janela ficou 56 pontos mais alta, que é o que a faixa ocupa. Quem já tinha a janela guardada em outro tamanho ganha esses 56 pontos uma vez, para baixo, e a tela continua do tamanho que tinha (testado com ajustes de teste, sem tocar nos da janela do Lipe).
- Quadros: [a janela com a faixa](../quadros/2026-10-12-janela-mac-volante.png), [a página](../quadros/2026-10-12-pagina-volante-barra-de-volume.jpg) e [a barra de volume na tela da central](../quadros/2026-10-12-hmi-vol-barra-de-volume.png).

**Não verificado.** Mouse de verdade nas teclas do volante (só eventos do roteiro e cliques do navegador de teste). Tecla do volante segura pela janela ou pela página: o toque longo só foi mandado pelo arquivo de comandos. Qualquer tecla com rádio, mídia ou telefone funcionando (parte disso foi medida depois, sem o mouse de verdade: ver a atualização no começo desta seção).

## O que foi verificado em 11/10/2026

Tudo abaixo foi executado, numa central de teste à parte (`moi3-exp-tx`, subida pelo `m3_ao_vivo.sh` com o encaminhador novo) e depois na central ao vivo.

- **Leitura dos quadros.** O `--decodifica` aplicado à transmissão gravada no teste 3 da bancada (três quadros: tela cheia, um retângulo, nada novo) produz a mesma imagem, byte a byte, que o glhost gravou e que o espectador de teste em C montou. Transmissão cortada ou lixo: recusa e sai com código 2.
- **Do container ao Mac.** `--captura` recebeu o quadro idêntico ao `latest.ppm` da central. Ao sair, a ponte termina sozinha e o glhost registra a saída do espectador.
- **Entrada.** A tecla Menu mandada pela ponte abriu a grade de menus. Pela janela, com o roteiro: toque na tela (abriu "Vehicle"), clique nas teclas, teclado (9 abriu os ajustes, seta girou a seleção, = abriu o volume), rolagem e − / + nos botões redondos, clique no botão de seleção (entrou no item em foco) e arrasto (a lista rolou). As linhas chegaram ao arquivo de comandos na ordem.
- **Ritmo.** Em três trocas de tela seguidas a janela recebeu 229 quadros e trocou a imagem 229 vezes: nenhum quadro foi pulado. Na troca para a grade de menus, 40 quadros chegaram com 20 ms entre um e outro (50 por segundo), em média 37 kB cada; o mesmo quadro em arquivo tem 1,15 MB.
- **Ligação.** Com a central desligada a janela mostra o aviso e o botão; religada a central, passa por "A central está ligando" e volta sozinha ao vivo.

## Defeito corrigido em 11/10/2026, 22h10

Em uso, a central parecia travada: não reagia a nada que a janela mandava. A ponte abria o arquivo de comandos ao ligar, antes de o lançador pôr o arquivo de verdade naquele caminho, e ficava gravando num arquivo que ninguém lia. Dependia da ordem da partida, por isso funcionou em algumas partidas e em outras não. A ponte passou a abrir o arquivo a cada linha. Há teste para isso (`ponte_liga_cedo`, no `t_transmissao.sh`) e o caso foi reproduzido numa central de teste. Detalhes no Marco 7 do [modelagem-soc.md](../modelagem-soc.md).

Se a central no ar foi ligada antes da correção e não reage à janela: fechar e abrir a janela resolve, porque a ponte nova já acha o arquivo certo.

## Limites e o que não foi verificado

- A janela não sabe quando a central descartou uma linha. Nos primeiros segundos da partida ela mostra "Último: toque em…" mesmo que a central ainda não esteja pronta para entrada.

- **A imagem da janela na tela do Mac** não pôde ser fotografada por esta sessão (a captura de tela de fora foi negada). O que conferi por conta própria é o que a janela entrega para o sistema desenhar: a árvore de camadas, com a imagem da central na camada da tela (passo `camadas` do roteiro), e o desenho pelo caminho de impressão (passo `foto`). **Em 11/10/2026, às 22h09, o Lipe mandou uma captura da janela de verdade**: a tela da central, as teclas, os dois botões e o rodapé aparecem como no desenho.
- No roteiro, o mouse e o teclado são eventos fabricados dentro do próprio programa e entregues à janela; passam pelo mesmo código que os eventos de verdade, mas não saíram de um mouse.
- A rolagem com trackpad (em pontos) não foi exercitada pelo roteiro, só a de roda (em dentes).
- A janela e a página mandando comandos no mesmo instante gravam no mesmo arquivo por caminhos diferentes; não foi testado se as linhas podem se misturar. No uso normal só uma das duas manda.
- O programa não é assinado nem empacotado para distribuição: é compilado e usado neste Mac.
- **Um roteiro ficou parado, uma vez (12/10/2026, 0h44).** Em uma das sete execuções do roteiro com o programa das teclas do volante, o último passo `foto` não terminou em mais de 30 s e o limite de 90 s que eu tinha posto cortou o programa. As 26 linhas daquele roteiro já tinham chegado à central, na ordem. O Mac estava com carga acima de 100, com seis a oito centrais de outras sessões no ar. Não se repetiu nas cinco execuções seguintes. Não sei a causa, nem se o programa de antes faria o mesmo nessas condições.
