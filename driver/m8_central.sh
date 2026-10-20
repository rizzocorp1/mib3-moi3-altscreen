#!/bin/sh
# m8_central.sh — uma central de TESTE, à parte da central ao vivo (Marco 8, 11/10/2026).
# Sobe a HMI num container próprio (moi3-m8-<nome>), com quadros, registro e entrada numa pasta
# sua, e deixa mandar teclas e toques e tirar fotos pela linha de comando. Não tem página nem
# toca em ao-vivo/. Roda no Mac. É estudo num computador: nada vai para o carro.
#
#   sh driver/m8_central.sh sobe <nome> <pasta> [segundos]   sobe em segundo plano (default 600 s) e
#                                                            grava o barramento em <pasta>/log/barramento.log
#   sh driver/m8_central.sh tecla <pasta> <tecla> [teclado]  aperta e solta (teclado 13 = moldura da tela;
#                                                            78 Menu, 6 Carro, 79 Clima, 7 Ajustes, 15 Rádio…)
#   sh driver/m8_central.sh toque <pasta> <x> <y>            toca num ponto da tela (800×480)
#   sh driver/m8_central.sh manda <pasta> <comando…>         uma linha crua do arquivo de comandos (dsistub.c)
#   sh driver/m8_central.sh foto <pasta> <arquivo.png>       a tela de agora, em PNG (precisa do ffmpeg)
#   sh driver/m8_central.sh desce <nome>                     encerra
#
# Por dentro é o mesmo roteiro do modo ao vivo (m3_ao_vivo.sh), com os mesmos padrões dele. Do
# ambiente passam: VEICULO, VEICULO_CARIF, VEICULO_ESQUEMA, RADIO, RADIO_ESQUEMA, TELEFONE, TELEFONE_ESQUEMA, MIDIA, MIDIA_ESQUEMA, SOM, SOM_DADOS, SOM_ESTADO, CODING, LSMVAL, HMILOG, ROTEIRO,
# ESPERA, POWER, RSI e EXTRA_VMOPTIONS; o que não for dado fica com o padrão do m3_ao_vivo.sh.
# Para ver a mesma central numa janela: sh janela/abre.sh --container moi3-m8-<nome>
# PREFIXO troca o "m8" do nome do container (PREFIXO=ped dá moi3-ped-<nome>), para as rodadas de
# sessões diferentes não se confundirem; vale no sobe e no desce.
# Uma central de teste não lembra de nada: nasce igual toda vez, ao contrário da central ao vivo.
# Para ela lembrar, só a pedido: MEMORIA=/live/memoria liga as memórias do modo ao vivo (a da
# tela, lsmval, a do carro de mentira e, com SOM, a do som de mentira);
# VEICULO_ESTADO=/live/memoria/veiculo/estado.txt, só a do carro de mentira (o que a tela grava
# nele: hora acertada, unidades); SOM_ESTADO=/live/memoria/som/estado.txt, só a do som de mentira
# (volume, grave, médio, agudo, balanço, fader e o mudo). Com qualquer uma das três a
# pasta <pasta>/memoria vai montada em /live/memoria, e o sobe não a apaga: outra central subida
# na mesma <pasta> nasce do que a anterior deixou. Para voltar ao zero: rm -rf <pasta>/memoria.
B=$(cd "$(dirname "$0")/.." && pwd)
C=moi3-${PREFIXO:-m8}
cmd=$1; [ -n "$cmd" ] && shift
case "$cmd" in
sobe)
  NOME=$1; P=$2; SECS=${3:-600}
  [ -n "$NOME" ] && [ -n "$P" ] || { echo "uso: m8_central.sh sobe <nome> <pasta> [segundos]"; exit 2; }
  mkdir -p "$P/frames" "$P/log" "$P/entrada" && P=$(cd "$P" && pwd) || exit 2
  rm -f "$P/frames/latest.ppm" "$P/frames/seq.txt" "$P"/log/*; : > "$P/entrada/comandos.txt"
  set --
  for v in VEICULO VEICULO_CARIF VEICULO_ESQUEMA RADIO RADIO_ESQUEMA TELEFONE TELEFONE_ESQUEMA MIDIA MIDIA_ESQUEMA SOM SOM_DADOS SOM_ESTADO CODING LSMVAL HMILOG ROTEIRO ESPERA POWER RSI EXTRA_VMOPTIONS MEMORIA VEICULO_ESTADO; do
    eval "dado=\${$v+sim}"; [ -n "$dado" ] && set -- "$@" -e "$v"       # só o que foi dado, com o valor do ambiente
  done
  if [ -n "$MEMORIA$VEICULO_ESTADO$SOM_ESTADO" ]; then                            # a pasta das memórias, só a pedido (ver o cabeçalho)
    mkdir -p "$P/memoria" || exit 2
    set -- "$@" -v "$P/memoria":/live/memoria
  fi
  docker run -d --rm --privileged --network none --name "$C-$NOME" -e INPUT=/live/entrada/comandos.txt "$@" \
    -v "$B/sysroot/mu0883":/fw:ro -v "$B/driver":/driver:ro -v "$B/glforward-moi3":/glf:ro \
    -v "$B/stubs":/stubs:ro -v "$P/frames":/live/frames -v "$P/log":/live/log \
    -v "$P/entrada":/live/entrada moi3-bancada:1 sh /driver/m3_ao_vivo.sh "$SECS" >/dev/null || exit 1
  # escuta do barramento, de fora do chroot: o socket do hub fica no tmpfs de /mnt/low/run
  i=0; while ! docker exec "$C-$NOME" test -S /mnt/low/run/kipc_hub.sock 2>/dev/null && [ $i -lt 300 ]; do sleep 0.2; i=$((i+1)); done
  docker exec -d "$C-$NOME" sh -c 'exec /tmp/w/bin/dsi_escuta /mnt/low/run/kipc_hub.sock > /live/log/barramento.log 2>&1'
  echo "central de teste $C-$NOME no ar por $SECS s; pasta $P"
  ;;
tecla)
  P=$1; T=$2; K=${3:-13}
  [ -f "$P/entrada/comandos.txt" ] && [ -n "$T" ] || { echo "uso: m8_central.sh tecla <pasta> <tecla> [teclado]"; exit 2; }
  echo "tecla $K $T 1" >> "$P/entrada/comandos.txt"; sleep 0.2; echo "tecla $K $T 0" >> "$P/entrada/comandos.txt"
  ;;
toque)
  P=$1
  [ -f "$P/entrada/comandos.txt" ] && [ -n "$3" ] || { echo "uso: m8_central.sh toque <pasta> <x> <y>"; exit 2; }
  echo "gesto 4 $2 $3" >> "$P/entrada/comandos.txt"; sleep 0.15; echo "gesto 1 $2 $3 1" >> "$P/entrada/comandos.txt"
  ;;
manda)
  P=$1; shift
  [ -f "$P/entrada/comandos.txt" ] && [ -n "$1" ] || { echo "uso: m8_central.sh manda <pasta> <comando…>"; exit 2; }
  echo "$*" >> "$P/entrada/comandos.txt"
  ;;
foto)
  P=$1; A=$2
  [ -f "$P/frames/latest.ppm" ] && [ -n "$A" ] || { echo "sem quadro em $P/frames (a HMI já desenhou?)"; exit 2; }
  ffmpeg -loglevel error -y -i "$P/frames/latest.ppm" "$A" && echo "$A (quadro $(cut -d' ' -f1 "$P/frames/seq.txt" 2>/dev/null))"
  ;;
desce)
  [ -n "$1" ] || { echo "uso: m8_central.sh desce <nome>"; exit 2; }
  docker stop "$C-$1" >/dev/null && echo "$C-$1 encerrada"
  ;;
*)
  sed -n '2,27p' "$0"; exit 2
  ;;
esac
