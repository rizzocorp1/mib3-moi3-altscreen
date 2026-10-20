#!/bin/sh
# m6_rodada.sh <tag> <pasta de saída> [arquivos de codificação...] — uma rodada de experimento
# do Marco 6: sobe a HMI num container próprio, aperta teclas com hora marcada, tira fotos da tela
# e grava o tráfego do barramento. Roda no Mac. NÃO aparece na página ao vivo: é uma sessão à parte.
#   sh driver/m6_rodada.sh ref /tmp/m6 stubs/codificacao/padrao.txt
#   sh driver/m6_rodada.sh nav /tmp/m6 stubs/codificacao/padrao.txt meu-ajuste.txt
#   python3 -I driver/m6_analisa.py /tmp/m6 nav ref        compara as duas
# Sem arquivo de codificação a HMI sobe como até o Marco 5. Os arquivos são copiados para a pasta
# de saída e lidos de lá. Variáveis:
#   SECS     duração (default 140)
#   ROTEIRO  teclas e toques com hora, como no m1_hmi.sh. Default: MENU aos 85 s, Ajustes aos 97,
#            Carro aos 109 e NAV aos 123 (por último, porque deixa um popup por cima do que vier)
#   FOTOS    segundos, desde a partida da HMI, em que a tela é fotografada (default 60 93 105 119 132)
#   HMILOG   registro detalhado da HMI (default normal; é o que mostra os bytes recebidos)
#   LSMVAL   memória de estado de partida, com o caminho de dentro do container (por exemplo
#            /stubs/persistencia/idioma-pt_BR.txt); sem isso nasce vazia, como no m1_hmi.sh
#   DIAGCAB  cabeçalho dos conjuntos, 3 bytes em hexa colado (default: o do diagstub, 83 60 22);
#            DIAGCAB=000000 é o de antes de 11/10/2026, para rodada de controle
# As fotos saem em PNG se houver ffmpeg no Mac; senão ficam em PPM.
TAG=$1; OUT=$2
[ -n "$TAG" ] && [ -n "$OUT" ] || { echo "uso: m6_rodada.sh <tag> <pasta de saída> [arquivos de codificação...]"; exit 2; }
shift 2
B=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$OUT/codificacao-$TAG" && OUT=$(cd "$OUT" && pwd) || exit 2
CODING=; n=0
for f in "$@"; do
  n=$((n+1)); b=$n-$(basename "$f")
  cp "$f" "$OUT/codificacao-$TAG/$b" || exit 2
  CODING="$CODING /out/codificacao-$TAG/$b"
done
ROT=${ROTEIRO-"85 tecla 13 78 1;0.2 tecla 13 78 0;11.8 tecla 13 7 1;0.2 tecla 13 7 0;11.8 tecla 13 6 1;0.2 tecla 13 6 0;13.8 tecla 13 4 1;0.2 tecla 13 4 0"}
docker run --rm --privileged --network none --name "moi3-m6-$TAG" \
  -e TAG="$TAG" -e SECS="${SECS:-140}" -e CODING="$CODING" -e HMILOG="${HMILOG-normal}" -e ROTEIRO="$ROT" \
  -e FOTOS="${FOTOS-60 93 105 119 132}" -e EXTRA_VMOPTIONS="${EXTRA_VMOPTIONS-}" -e LSMVAL="${LSMVAL-}" \
  -e DIAGCAB="${DIAGCAB-}" \
  -v "$B/sysroot/mu0883":/fw:ro -v "$B/driver":/driver:ro -v "$B/glforward-moi3":/glf:ro -v "$B/stubs":/stubs:ro \
  -v "$OUT":/out moi3-bancada:1 sh /driver/m6_rodada_container.sh
if command -v ffmpeg >/dev/null 2>&1; then
  for p in "$OUT"/foto_"$TAG"_*.ppm; do [ -f "$p" ] && ffmpeg -loglevel error -y -i "$p" "${p%.ppm}.png" && rm -f "$p"; done
fi
echo "saídas em $OUT: $(ls "$OUT" | grep -c "^$TAG\.\|^foto_${TAG}_") arquivos da rodada $TAG"
