#!/bin/sh
# som_controle.sh <volumeIncrease|volumeDecrease|mute> [passos] [hmi|cluster] — cria um controle de
# volume no serviço de áudio da central, como o touch-app faz no carro, sem passar por tecla
# nenhuma (roda dentro do container de uma central; usa o rsi-send do firmware). 12/10/2026.
#   docker exec <container> sh /driver/som_controle.sh volumeIncrease 2
LOW=${LOW:-/mnt/low}; T=${1:?uso: som_controle.sh <volumeIncrease|volumeDecrease|mute> [passos] [hmi|cluster]}
case "$T" in volumeIncrease) N=volumeUp_gui ;; volumeDecrease) N=volumeDown_gui ;; mute) N=muteButtonId_tbd ;; *) echo "tipo desconhecido: $T"; exit 2 ;; esac
# a hora é a do relógio monotônico, em ms, como a HMI e o somstub usam
H=$(awk '{ printf "%d", $1 * 1000 }' /proc/uptime)
chroot $LOW /lge/app_ro/bin/rsi-send post /audiomanagement/controls \
  "{\"name\":\"$N\",\"domain\":\"${3:-hmi}\",\"buttonstate\":\"pressed\",\"time\":\"$H\",\"type\":\"$T\",\"value\":${2:-1}}" 2>&1 | head -n 2 | tr '\n' ' '; echo
