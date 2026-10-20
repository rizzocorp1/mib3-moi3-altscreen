#!/bin/sh
# ao_vivo.sh [segundos] — sobe a HMI do MOI3 no container e mostra a tela ao vivo no navegador,
# com toque e teclas: clicar na página vira toque na central.
# Roda no Mac, de qualquer pasta. É estudo num computador: nada vai para o carro.
#   sh referencia/ferramentas/mib3-sandbox/driver/ao_vivo.sh          15 minutos
#   sh referencia/ferramentas/mib3-sandbox/driver/ao_vivo.sh 3600     uma hora
# Ctrl-C encerra. De outro terminal: docker stop moi3-ao-vivo
# Precisa da imagem moi3-bancada:1 (driver/Dockerfile.bancada) e do firmware em sysroot/mu0883.
# Variáveis: PORTA (default 8431); SEM_SERVIDOR=1 não sobe o servidor da página nem abre o
# navegador (para quando outra coisa já serve a pasta ao-vivo/ com o servidor_ao_vivo.py);
# POWER=0 deixa a central desligada; RSI= (vazio) desliga a camada RSI e a HMI volta a parar em
# "Loading user profile…"; ROTEIRO marca entrada com hora (formato no m1_hmi.sh), por exemplo
# ROTEIRO="85 tecla 13 78 1;0.2 tecla 13 78 0" aperta MENU aos 85 s; ESPERA é quanto a HMI
# espera, em ms, por serviços ausentes na partida (default 2000; o do firmware é 15000; a espera
# pelos serviços DSI tem valor próprio, ver ESPERA_DSI mais abaixo);
# CODING são os arquivos de codificação e adaptação, com o caminho de dentro do container.
# Desde 11/10/2026, por decisão do Lipe, o default é a identidade provisória do Jetta GLI 2021:
# padrao.txt, jetta-gli-2021.txt (o que foi lido do carro), jetta-gli-2021-deducoes.txt (palpites
# marcados) e ajustes.txt, nessa ordem, todos de /stubs/codificacao. Para a central sem
# identidade: CODING="/stubs/codificacao/padrao.txt /stubs/codificacao/ajustes.txt". CODING= vazio
# sobe sem codificação nenhuma.
# LSMVAL é o arquivo com que a memória de estado da central começa (caminho de dentro do
# container). Desde 11/10/2026 o default é /stubs/persistencia/idioma-pt_BR.txt: a central já
# nasce em português do Brasil, o idioma que o Lipe escolheu pela tela e o da central do carro.
# LSMVAL= vazio sobe com a memória vazia, em inglês.
# MEMORIA: desde a noite de 11/10/2026 a central lembra de uma sessão para a outra, como no carro.
# Enquanto roda, a memória de estado dela fica guardada em ao-vivo/memoria/lsmval, e a sessão
# seguinte nasce desse arquivo, com o idioma, a última tela e os ajustes como ficaram; o arquivo
# de LSMVAL só vale enquanto não há memória guardada. Com isso as partidas deixam de ser iguais
# entre si, e um estado ruim também fica guardado. Para uma sessão que não lê nem guarda a
# memória: MEMORIA= (vazio) ou MEMORIA=0. LSMVAL passado à mão vale sobre tudo: a sessão nasce
# do arquivo pedido, e a memória guardada não é lida nem alterada. Voltar ao zero: com a central
# desligada, rm -rf ao-vivo/memoria. Voltar a uma partida anterior: copiar
# ao-vivo/memoria/anteriores/lsmval.N por cima de ao-vivo/memoria/lsmval (a .1 é a memória com
# que a última sessão nasceu; ficam cinco). Quem confere e guarda é o driver/lsmval_guarda.sh.
# Desde 12/10/2026 a mesma pasta guarda também o que a tela grava no carro de mentira (a hora
# acertada e as unidades), em ao-vivo/memoria/veiculo/estado.txt: no carro quem lembra disso é o
# painel. A sessão seguinte nasce com as unidades escolhidas e com o relógio adiantado do mesmo
# tanto em relação à hora do Mac. MEMORIA= (ou LSMVAL à mão) desliga isso também. Para voltar só o
# carro de mentira ao zero, com a central desligada: rm -rf ao-vivo/memoria/veiculo.
LSMVAL_PADRAO="/stubs/persistencia/idioma-pt_BR.txt"
# VEICULO são os perfis de dados de veículo que o "carro de mentira" publica (Marco 8), com o
# caminho de dentro do container. O default é o Jetta GLI 2021 parado: relógio de verdade no
# fuso de Brasília, carro parado, computador de bordo, unidades. Parte é palpite ou valor de
# simulação, marcado no próprio arquivo (stubs/veiculo/). VEICULO= vazio sobe sem ele: relógio
# "--:--" e a tela do veículo vazia, como até o Marco 7.
VEICULO_PADRAO="/stubs/veiculo/carro-parado.txt /stubs/veiculo/jetta-gli-2021.txt /stubs/veiculo/jetta-gli-2021-deducoes.txt"
# RADIO são os perfis do "rádio de mentira" (Marco 11), com o caminho de dentro do container. O
# default é a faixa de FM com cinco emissoras inventadas (stubs/radio/fm-de-mentira.txt): a tela do
# rádio abre, mostra a lista e troca de emissora. Não há recepção nem som. RADIO= vazio sobe sem
# ele, e a tela do rádio volta ao aviso "função indisponível", como até o Marco 9.
RADIO_PADRAO="/stubs/radio/fm-de-mentira.txt"
# TELEFONE são os perfis do "telefone de mentira" (Marco 12), com o caminho de dentro do container.
# O default, desde 12/10/2026, é um aparelho FICTÍCIO conhecido e NÃO conectado (os três perfis de
# stubs/telefone/, nesta ordem). A tecla Telefone abre a tela que convida a conectar o celular, a
# grade tem o bloco Telefone e o assistente de configuração tem a página do telefone, como antes;
# e "Selecionar celular" passa a mostrar o "Celular fictício": tocar nele conecta (a tela
# principal do telefone, os ícones no alto), e desligar o Bluetooth pela tela desconecta. Na
# partida nada muda na tela. Não há Bluetooth nem aparelho de verdade: o aparelho é inventado.
# TELEFONE=/stubs/telefone/sem-aparelho.txt volta ao estado sem aparelho nenhum, o default de até
# então. TELEFONE= vazio sobe sem telefone, e a tecla volta ao aviso "Telefone indisponível.".
# CARPLAY: desde 12/10/2026, por decisão do Lipe, o modo ao vivo sobe como "central com CarPlay",
# que é o que o carro tem (a foto de 09/10/2026 mostra o bloco "Apple CarPlay" na grade). São duas
# peças: a dedução do jetta-gli-2021-deducoes.txt (o campo ID_APPLE_DIO da adaptação diferente de
# zero; palpite marcado, não leitura do 5F) e o telefone de mentira anunciando o serviço do CarPlay
# e respondendo "nenhuma chamada" (esquema stubs/telefone/gerado-carplay/ e perfil
# carplay-sem-chamada.txt). Com isso a grade do menu tem o bloco "App-Connect" (com a memória
# guardada ele fica na segunda página, com o Telefone), o botão vermelho da tela de chamada
# encerra a chamada simulada e a partida leva uns 6 s a mais (a HMI espera por Android Auto e
# MirrorLink, que a bancada não tem). Não há CarPlay simulado: nenhum aparelho, nenhuma tela dele.
# CARPLAY=0 volta à central sem CarPlay, a de até então: põe stubs/codificacao/sem-carplay.txt no
# fim da codificação e sobe o telefone de mentira com o esquema e os perfis de antes.
# Desde a noite de 12/10/2026 entra também o segundo-aparelho.txt: um SEGUNDO aparelho fictício, que
# pede para parear 4 s depois de a tela "Celulares conhecidos" abrir (a central mostra o código para
# confirmar). Só com essa tela aberta: na partida e nas outras telas nada aparece.
TELEFONE_PADRAO="/stubs/telefone/sem-aparelho.txt /stubs/telefone/aparelho-conectado.txt /stubs/telefone/comeca-desconectado.txt /stubs/telefone/segundo-aparelho.txt"
# MIDIA são os perfis da "mídia de mentira" (Marco 15), com o caminho de dentro do container. O
# default é a fonte "Minhas mídias" com cinco faixas inventadas (stubs/midia/minhas-midias.txt): a
# tecla Mídia abre o tocador, com a faixa, a lista e a troca de faixa. Não há aparelho nem som, e
# a faixa fica em pausa. MIDIA= vazio sobe sem ela, e a tecla volta ao aviso "função indisponível",
# como antes do Marco 15.
MIDIA_PADRAO="/stubs/midia/minhas-midias.txt"
# SOM são os perfis do "som de mentira" (12/10/2026), com o caminho de dentro do container. O
# default é um amplificador interno básico (stubs/som/som-de-mentira.txt): a tecla Som abre a tela
# de equalização com grave, médio e agudo, a de posição e a de volumes, os ajustes pegam, e o
# volume muda com o botão, com as teclas do volante e com o mudo. Não há áudio: é só o estado que
# a tela mostra, e o amplificador é SIMULAÇÃO (a ficha traz "Som Beats, a confirmar"). Com ele a
# HMI passa a pedir a ligação de áudio de cada fonte ao serviço, e isso muda duas coisas medidas:
# as trocas de emissora chegam ao rádio de mentira na hora, e a mídia de mentira toca (o botão vira
# pausa; o tempo segue em 0:00) e volta com a faixa depois de uma ida ao rádio. O que o comentário
# da MIDIA diz logo acima ("a faixa fica em pausa") vale sem o som. SOM= vazio sobe sem ele: a tela
# de som volta ao aviso de configuração indisponível e a barra de volume fica sem dono.
SOM_PADRAO="/stubs/som/som-de-mentira.txt"
# ESPERA_DSI é quanto a HMI espera, em ms, pelos serviços DSI de cada etapa da partida; os serviços
# RSI seguem com a ESPERA. O default é 6000 (12/10/2026, Marco 15, decisão D158). Motivo, medido: a
# HMI só cria os serviços de veículo perto dos 5 s de partida. Desde que o rádio, o telefone e a
# mídia de mentira respondem, a etapa da partida que precisa desses serviços começa a esperar aos
# 2,2 s, e com os 2 s da ESPERA desistia 0,7 s antes de eles existirem. Quando a memória fazia a
# central nascer na tela do veículo, ela ficava sem sete serviços de veículo e o computador de
# bordo só mostrava traços; nas outras partidas faltavam quatro assinaturas (opções dos menus de
# limpadores e de bancos), sem efeito visto na tela. Custo: a última etapa da partida começa aos
# 50 s em vez de aos 42 s (as duas etapas que esperam por serviço que a bancada não tem, DSIWLAN e
# DSIBrowser, levam 6 s cada em vez de 2); a primeira tela aparece na mesma hora. ESPERA dada à mão
# vale para os dois tipos de serviço, como antes: aí a ESPERA_DSI só entra se também for dada.
# ESPERA_DSI= vazio volta ao que era. Com 4000 também deu certo, em duas partidas de duas, com a
# última etapa aos 46 s e 1,2 s de folga em vez de 3,2 s.
ESPERA_DSI_PADRAO=6000
CODING_PADRAO="/stubs/codificacao/padrao.txt /stubs/codificacao/jetta-gli-2021.txt /stubs/codificacao/jetta-gli-2021-deducoes.txt /stubs/codificacao/ajustes.txt"
if [ -z "${CARPLAY-1}" ] || [ "${CARPLAY-1}" = 0 ]; then    # CARPLAY=0 (ou vazio): a central sem CarPlay
  CODING_PADRAO="$CODING_PADRAO /stubs/codificacao/sem-carplay.txt"
else                                                         # o padrão: a central com CarPlay (ver CARPLAY, no cabeçalho)
  TELEFONE_PADRAO="$TELEFONE_PADRAO /stubs/telefone/carplay-sem-chamada.txt"
  TELEFONE_ESQUEMA=${TELEFONE_ESQUEMA-/stubs/telefone/gerado-carplay/esquema.txt}
fi
# Os menus do carro que o Lipe disse que a central dele tem (Marco 14, 12/10/2026): entram no fim
# dos padrões de codificação e de veículo. MENUS=0 (ou vazio) tira os dois. Quem dá CODING ou
# VEICULO à mão fica com a lista que deu.
if [ -n "${MENUS-1}" ] && [ "${MENUS-1}" != 0 ]; then
  CODING_PADRAO="$CODING_PADRAO /stubs/codificacao/jetta-gli-2021-menus.txt"
  VEICULO_PADRAO="$VEICULO_PADRAO /stubs/veiculo/jetta-gli-2021-menus.txt"
fi
SECS=${1:-900}; PORTA=${PORTA:-8431}
B=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$B/ao-vivo/frames" "$B/ao-vivo/log" "$B/ao-vivo/entrada"
rm -f "$B/ao-vivo/frames/latest.ppm" "$B/ao-vivo/frames/seq.txt"
: > "$B/ao-vivo/entrada/comandos.txt"
if [ -z "$SEM_SERVIDOR" ]; then
  if ! nc -z 127.0.0.1 "$PORTA" 2>/dev/null; then
    # -I: a pasta servida recebe arquivos do container; o Python não pode carregar módulo de lá
    python3 -I "$B/driver/servidor_ao_vivo.py" "$PORTA" "$B/ao-vivo" >/dev/null 2>&1 &
    SRV=$!
    trap 'kill $SRV 2>/dev/null' EXIT INT TERM
  fi
  echo "Tela ao vivo em http://127.0.0.1:$PORTA  (Ctrl-C encerra)"
  open "http://127.0.0.1:$PORTA" 2>/dev/null
fi
# Memória entre sessões (ver MEMORIA, no cabeçalho): a pasta ao-vivo/memoria vai montada em
# /live/memoria e o m3_ao_vivo.sh faz a central nascer dela e ir guardando. Com LSMVAL passado à
# mão, ou com MEMORIA vazio ou 0, MEM fica vazio: a pasta não é criada, montada nem alterada.
MEM="$B/ao-vivo/memoria"
if [ -n "${LSMVAL+x}" ] || [ -z "${MEMORIA-1}" ] || [ "${MEMORIA-1}" = 0 ]; then MEM=; else mkdir -p "$MEM"; fi
# Espera pelos serviços DSI (ver ESPERA_DSI, no cabeçalho). Vai como opção da máquina Java: o
# m1_hmi.sh a deixa depois das duas que monta a partir da ESPERA, e na máquina Java vale a última.
if [ -z "${ESPERA_DSI+x}" ] && [ -z "${ESPERA+x}" ]; then ESPERA_DSI=$ESPERA_DSI_PADRAO; fi
# O container só enxerga as três pastas de dados (quadros, log e entrada), não a página.
docker run --rm --privileged --network none --name moi3-ao-vivo -e POWER="${POWER:-1}" -e RSI="${RSI-1}" \
  -e ROTEIRO="${ROTEIRO-}" -e INPUT=/live/entrada/comandos.txt -e ESPERA="${ESPERA-2000}" \
  -e CODING="${CODING-$CODING_PADRAO}" -e LSMVAL="${LSMVAL-$LSMVAL_PADRAO}" \
  -e VEICULO="${VEICULO-$VEICULO_PADRAO}" -e RADIO="${RADIO-$RADIO_PADRAO}" \
  -e TELEFONE="${TELEFONE-$TELEFONE_PADRAO}" -e TELEFONE_ESQUEMA="${TELEFONE_ESQUEMA-}" \
  -e MIDIA="${MIDIA-$MIDIA_PADRAO}" \
  -e SOM="${SOM-$SOM_PADRAO}" \
  -e EXTRA_VMOPTIONS="${ESPERA_DSI:+-Dde.vw.mib.asl.startup.timeout.dsi=$ESPERA_DSI}" \
  -e MEMORIA="${MEM:+/live/memoria}" ${MEM:+-v} ${MEM:+"$MEM:/live/memoria"} \
  -v "$B/sysroot/mu0883":/fw:ro -v "$B/driver":/driver:ro -v "$B/glforward-moi3":/glf:ro \
  -v "$B/stubs":/stubs:ro -v "$B/ao-vivo/frames":/live/frames -v "$B/ao-vivo/log":/live/log \
  -v "$B/ao-vivo/entrada":/live/entrada moi3-bancada:1 sh /driver/m3_ao_vivo.sh "$SECS"
