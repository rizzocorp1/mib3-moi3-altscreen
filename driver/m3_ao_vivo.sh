#!/bin/sh
# Marco 3, ver ao vivo (11/10/2026). Roda DENTRO da imagem moi3-bancada:1, chamado pelo
# ao_vivo.sh. Compila tudo num diretório do próprio container (não depende de pasta de
# trabalho no Mac), sobe a HMI com o encaminhador de OpenGL e deixa os quadros em /live/frames,
# que o visualizador (ao-vivo/index.html) acompanha.
#   m3_ao_vivo.sh [segundos]      POWER=0 deixa a central "desligada"
# A camada RSI vem ligada: registro e mock de serviços do próprio firmware, com os serviços
# usermanagement e onlineregistration e os dados de /stubs/rsi-mockup. Sem ela a HMI fica parada
# em "Loading user profile…"; para ver essa tela de novo, rodar com RSI vazio (RSI= sh ao_vivo.sh).
# A entrada (Marco 5) vem do ambiente e segue para o m1_hmi.sh: INPUT é o arquivo de comandos
# que a página alimenta (/live/entrada/comandos.txt) e ROTEIRO, se houver, a entrada com hora.
# A espera da HMI por serviços ausentes vem reduzida a 2 s (ESPERA), para a partida levar 1 min
# em vez de 5; ESPERA=15000 volta ao padrão do firmware.
# A codificação e a adaptação (Marco 6) vêm de CODING. Desde 11/10/2026, por decisão do Lipe, o
# padrão é a identidade provisória do Jetta GLI 2021, quatro arquivos de /stubs/codificacao lidos
# em ordem: padrao.txt (todos os campos no valor padrão da HMI), jetta-gli-2021.txt (o que foi
# lido do carro), jetta-gli-2021-deducoes.txt (palpites marcados) e ajustes.txt (experimentos).
# Só com padrao.txt e ajustes.txt a tela fica igual à de antes do Marco 6 (medido: quadros
# idênticos). CODING vazio sobe sem codificação nenhuma.
# A memória de estado da central começa do arquivo LSMVAL. Desde 11/10/2026 o padrão é
# /stubs/persistencia/idioma-pt_BR.txt, com que a HMI já nasce em português do Brasil (o idioma
# que o Lipe escolheu pela tela e o da central do carro). LSMVAL vazio sobe em inglês.
# A memória entre sessões (11/10/2026, à noite): com MEMORIA=<pasta> a central lembra de uma
# sessão para a outra, como no carro. Ela nasce de <pasta>/lsmval, a memória que a sessão
# anterior deixou, e vai guardando a sua lá enquanto roda; LSMVAL passa a valer só enquanto não
# há memória guardada. Quem escolhe, confere e guarda é o lsmval_guarda.sh, e o registro dele
# fica em /live/log/ao_vivo.memoria.log. O ao_vivo.sh passa MEMORIA=/live/memoria, com a pasta
# ao-vivo/memoria do Mac montada ali. Sem MEMORIA (ou com 0), que é o caso de quem chama este
# roteiro direto, nada disso acontece: a central nasce de LSMVAL e nada fica guardado.
# Desde 12/10/2026 a mesma pasta guarda o que a tela grava no carro de mentira (a hora acertada e
# as unidades), em <pasta>/veiculo/estado.txt: no carro quem lembra disso é o painel, e não a
# central. A sessão seguinte nasce com as unidades escolhidas e com o relógio adiantado do mesmo
# tanto. VEICULO_ESTADO escolhe outro arquivo (caminho de dentro do container) e vale mesmo sem
# MEMORIA; vazio ou 0 desliga só essa parte. Quem lê, confere e grava é o próprio veiculostub.
# Desde a manhã de 12/10/2026 a mesma pasta guarda o que a tela muda no som de mentira (volume,
# grave, médio, agudo, balanço, fader e o mudo), em <pasta>/som/estado.txt: no carro quem lembra
# disso é o gerente de áudio. SOM_ESTADO escolhe outro arquivo e vale mesmo sem MEMORIA; vazio ou
# 0 desliga só essa parte. Quem lê e grava é o próprio somstub.
SECS=${1:-900}; W=/tmp/w
ESPERA=${ESPERA-2000}; export ESPERA
RSI=${RSI-1}
RSI_SERVICES=${RSI_SERVICES-usermanagement:auth onlineregistration:communication}
RSI_DATA=${RSI_DATA-/stubs/rsi-mockup}
export RSI RSI_SERVICES RSI_DATA
CODING=${CODING-/stubs/codificacao/padrao.txt /stubs/codificacao/jetta-gli-2021.txt /stubs/codificacao/jetta-gli-2021-deducoes.txt /stubs/codificacao/ajustes.txt}; export CODING
LSMVAL=${LSMVAL-/stubs/persistencia/idioma-pt_BR.txt}; export LSMVAL
# Os dados de veículo (Marco 8) vêm de VEICULO: os perfis que o "carro de mentira" (veiculostub)
# publica, lidos em ordem. O padrão é o Jetta GLI 2021 parado: carro-parado.txt (o cenário: hora
# de verdade, carro parado, freio puxado), jetta-gli-2021.txt (o que foi lido do carro) e
# jetta-gli-2021-deducoes.txt (palpites e valores de simulação, marcados). VEICULO vazio sobe
# sem o carro de mentira, com o carifservice do firmware, como até o Marco 7.
VEICULO=${VEICULO-/stubs/veiculo/carro-parado.txt /stubs/veiculo/jetta-gli-2021.txt /stubs/veiculo/jetta-gli-2021-deducoes.txt}; export VEICULO
# O rádio (Marco 11) vem de RADIO: os perfis que o "rádio de mentira" (radiostub) publica, por
# exemplo /stubs/radio/fm-de-mentira.txt (a faixa de FM com cinco emissoras inventadas, sem
# recepção nem som). AQUI NÃO HÁ PADRÃO: quem chama este roteiro direto, que é o caso das centrais
# de teste (m8_central.sh), sobe sem o rádio de mentira e com a tela do rádio no aviso "função
# indisponível", como até o Marco 9, salvo se passar RADIO. O padrão do modo ao vivo está no
# ao_vivo.sh, que passa a variável para cá.
# O telefone (Marco 12) vem de TELEFONE: os perfis que o "telefone de mentira" publica, por exemplo
# /stubs/telefone/sem-aparelho.txt (Bluetooth ligado e nenhum aparelho conectado: a tecla Telefone
# abre a tela que convida a conectar o celular). AQUI NÃO HÁ PADRÃO, como no rádio: quem chama este
# roteiro direto, que é o caso das centrais de teste (m8_central.sh), sobe sem o telefone de
# mentira e com a tecla no aviso "Telefone indisponível.", como até o Marco 11, salvo se passar
# TELEFONE. O padrão do modo ao vivo está no ao_vivo.sh, que passa a variável para cá.
# A mídia (Marco 15) vem de MIDIA: os perfis que a "mídia de mentira" (midiastub) publica, por
# exemplo /stubs/midia/minhas-midias.txt (a fonte "Minhas mídias" com cinco faixas inventadas, sem
# aparelho nem som). AQUI NÃO HÁ PADRÃO, como no rádio e no telefone: quem chama este roteiro
# direto, que é o caso das centrais de teste (m8_central.sh), sobe sem a mídia de mentira e com a
# tecla Mídia no aviso "função indisponível", como antes do Marco 15, salvo se passar MIDIA. O padrão
# do modo ao vivo está no ao_vivo.sh, que passa a variável para cá.
# O som (12/10/2026) vem de SOM: os perfis do "som de mentira", por exemplo
# /stubs/som/som-de-mentira.txt (um amplificador interno básico: a tela de som abre com grave,
# médio, agudo, balanço e fader, e o volume e o mudo passam a ter estado; não há áudio). AQUI NÃO
# HÁ PADRÃO, como no rádio, no telefone e na mídia: quem chama este roteiro direto, que é o caso das centrais
# de teste (m8_central.sh), sobe sem ele e com a tela de som no aviso "A configuração do som estará
# disponível novamente…", salvo se passar SOM. O padrão do modo ao vivo está no ao_vivo.sh.
mkdir -p $W/fwlibs /mnt/low /live/frames /live/log
mount -o loop,ro,noload /fw/system.img /mnt/low && mount --bind /fw/vw /mnt/low/vw || { echo "não montou o firmware"; exit 1; }
cp -L /mnt/low/lib64/libc.so.6 /mnt/low/lib64/libpthread.so.0 /mnt/low/lib64/ld-linux-aarch64.so.1 $W/fwlibs/
sh /driver/m1_build.sh $W/bin > $W/build.log 2>&1 || { tail -5 $W/build.log; exit 1; }
FWLIBS=$W/fwlibs sh /glf/build.sh $W/glfwd >> $W/build.log 2>&1 || { tail -8 $W/build.log; exit 1; }
echo "[ao_vivo] compilado; subindo a HMI por $SECS s (a primeira tela leva cerca de um minuto)"
# Memória entre sessões (ver MEMORIA, no cabeçalho). A vigia só começa depois que o lançador
# monta a memória da central (o tmpfs de /data): antes disso não há o que guardar.
MEMORIA=${MEMORIA-}; [ "$MEMORIA" = 0 ] && MEMORIA=
if [ -n "$MEMORIA" ]; then
  LSMVAL=$(sh /driver/lsmval_guarda.sh partida "$MEMORIA" "$LSMVAL") || exit 1
  echo "[ao_vivo] memória entre sessões em $MEMORIA: a central nasce de ${LSMVAL:-memória vazia}"
  ( while ! grep -q ' /mnt/low/data tmpfs ' /proc/mounts; do sleep 0.2; done
    exec sh /driver/lsmval_guarda.sh vigia /mnt/low/data/lge/hmi/lsmval "$MEMORIA" ) > /live/log/ao_vivo.memoria.log 2>&1 &
  VIGIA=$!
fi
# O que a tela grava no carro de mentira (ver o cabeçalho): junto da memória, salvo outro pedido.
VEICULO_ESTADO=${VEICULO_ESTADO-${MEMORIA:+$MEMORIA/veiculo/estado.txt}}
[ "$VEICULO_ESTADO" = 0 ] && VEICULO_ESTADO=; [ -n "$VEICULO" ] || VEICULO_ESTADO=
export VEICULO_ESTADO
[ -n "$VEICULO_ESTADO" ] && echo "[ao_vivo] o carro de mentira guarda o que a tela gravar em $VEICULO_ESTADO$([ -f "$VEICULO_ESTADO" ] && echo ", e nasce do que já está lá")"
# O que a tela muda no som de mentira (ver o cabeçalho): junto da memória, salvo outro pedido.
SOM_ESTADO=${SOM_ESTADO-${MEMORIA:+$MEMORIA/som/estado.txt}}
[ "$SOM_ESTADO" = 0 ] && SOM_ESTADO=; [ -n "$SOM" ] || SOM_ESTADO=
export SOM_ESTADO
[ -n "$SOM_ESTADO" ] && echo "[ao_vivo] o som de mentira guarda o que a tela mudar em $SOM_ESTADO$([ -f "$SOM_ESTADO" ] && echo ", e nasce do que já está lá")"
LOW=/mnt/low BIN=$W/bin OUT=/live/log TAG=ao_vivo VARIANT=real SECS=$SECS \
GLDIR=$W/glfwd GLHOST=$W/glfwd/glhost GLFRAMES=/live/frames GLHOST_KEEP=0 GLW=800 GLH=480 \
DMW=800 DMH=480 DSISTUB=1 POWER=${POWER:-1} sh /driver/m1_hmi.sh
RC=$?
# A central parou: encerra a vigia e guarda a memória como ela ficou.
if [ -n "$MEMORIA" ]; then
  kill $VIGIA 2>/dev/null; wait $VIGIA 2>/dev/null
  echo "$(date +%H:%M:%S) fim da sessão: $(sh /driver/lsmval_guarda.sh passo /mnt/low/data/lge/hmi/lsmval "$MEMORIA" 2>&1)" >> /live/log/ao_vivo.memoria.log
fi
exit $RC
