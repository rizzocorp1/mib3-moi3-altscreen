#!/bin/sh
# Marco 6 (11/10/2026): uma rodada de experimento com fotos da tela e escuta do barramento.
# Roda DENTRO da imagem moi3-bancada:1, chamado pelo m6_rodada.sh. É como o m3_ao_vivo.sh, mas
# grava tudo em /out, com o nome da rodada:
#   <tag>.out             a saída da HMI
#   <tag>.barramento.log  todas as mensagens do barramento (dsi_escuta), para o m6_analisa.py
#   foto_<tag>_<t>.ppm    a tela aos <t> segundos da partida da HMI (FOTOS="60 93 105")
#   <tag>.vivos.txt       quais processos estavam de pé na hora da última foto
# Do ambiente: TAG, SECS, CODING, ROTEIRO, FOTOS, HMILOG (default normal), ESPERA (default 2000),
# LSMVAL (memória de estado de partida).
SECS=${SECS:-140}; W=/tmp/w; TAG=${TAG:-rodada}
ESPERA=${ESPERA-2000}; HMILOG=${HMILOG-normal}
RSI=${RSI-1}
RSI_SERVICES=${RSI_SERVICES-usermanagement:auth onlineregistration:communication}
RSI_DATA=${RSI_DATA-/stubs/rsi-mockup}
export ESPERA HMILOG RSI RSI_SERVICES RSI_DATA CODING ROTEIRO LSMVAL
mkdir -p $W/fwlibs /mnt/low /out/frames_$TAG
mount -o loop,ro,noload /fw/system.img /mnt/low && mount --bind /fw/vw /mnt/low/vw || { echo "não montou o firmware"; exit 1; }
cp -L /mnt/low/lib64/libc.so.6 /mnt/low/lib64/libpthread.so.0 /mnt/low/lib64/ld-linux-aarch64.so.1 $W/fwlibs/
sh /driver/m1_build.sh $W/bin > $W/build.log 2>&1 || { tail -5 $W/build.log; exit 1; }
FWLIBS=$W/fwlibs sh /glf/build.sh $W/glfwd >> $W/build.log 2>&1 || { tail -8 $W/build.log; exit 1; }
rm -f /out/$TAG.out
# escuta do barramento, de fora do chroot: o socket do hub fica no tmpfs de /mnt/low/run
( i=0; while [ ! -S /mnt/low/run/kipc_hub.sock ] && [ $i -lt 300 ]; do sleep 0.1; i=$((i+1)); done
  sleep 0.3; exec $W/bin/dsi_escuta /mnt/low/run/kipc_hub.sock > /out/$TAG.barramento.log 2>&1 ) &
# fotos: o relógio começa quando o lançador cria a saída da HMI
( while [ ! -f /out/$TAG.out ]; do sleep 0.2; done; t0=$(date +%s)
  for t in $FOTOS; do
    while [ $(( $(date +%s) - t0 )) -lt $t ]; do sleep 0.25; done
    cp /out/frames_$TAG/latest.ppm /out/foto_${TAG}_$t.ppm 2>/dev/null
  done
  for d in persistence-app mcu-interpreter carifservice touch-app MIB_ServiceRegistryApp-00 rsi-mockup dsistub java; do
    echo "$d: $(ps -eo args | grep -v grep | grep -c "$d")"; done > /out/$TAG.vivos.txt ) &
LOW=/mnt/low BIN=$W/bin OUT=/out TAG=$TAG VARIANT=real SECS=$SECS \
GLDIR=$W/glfwd GLHOST=$W/glfwd/glhost GLFRAMES=/out/frames_$TAG GLHOST_KEEP=0 GLW=800 GLH=480 \
DMW=800 DMH=480 DSISTUB=1 POWER=${POWER:-1} INPUT= sh /driver/m1_hmi.sh
rm -rf /out/frames_$TAG
