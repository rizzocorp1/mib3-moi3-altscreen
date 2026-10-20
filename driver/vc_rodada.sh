#!/bin/sh
# vc_rodada.sh <tag> <pasta de saída> [arquivos de codificação...] — uma rodada da investigação
# do painel de instrumentos ("Virtual Cockpit"), 11/10/2026. Roda no Mac. Sobe, num container
# próprio (moi3-vc-<tag>, sem rede), a HMI e os serviços de navegação e de vídeo do painel que o
# firmware traz, e guarda as saídas na pasta dada. NÃO aparece na página nem na janela ao vivo.
#   sh driver/vc_rodada.sh a1 /tmp/vc stubs/codificacao/padrao.txt stubs/codificacao/jetta-gli-2021-deducoes.txt
# O que cada serviço é e o que sai de cada um está no cabeçalho do vc_rodada_container.sh.
# Variáveis (todas opcionais): SECS, SERVICOS, NAVPKG, GPIO, FOTOS, ROTEIRO, HMILOG, ESPERA,
# LSMVAL, ALMLOG, NAVWAIT, NAVW, NAVH, WLSTUB, DIAGALM; e
#   MAPA   pasta do Mac com o banco de mapas já no formato instalado (ROOT.NDS, LC/, WO/ ...);
#          entra só para leitura em /var/lib/nav/nav. Sem isso a pasta de mapas fica vazia.
#   DIAGALM  1 = os conjuntos de codificação que o navi-al usa vão também com o cabeçalho que ele
#          entende (vc_diag_alm.c); todos = todos os conjuntos. Sem isso, como na r1 e na r2.
#          SEM EFEITO desde as 23h26 de 11/10/2026: o diagstub já manda esse cabeçalho sozinho.
TAG=$1; OUT=$2
[ -n "$TAG" ] && [ -n "$OUT" ] || { echo "uso: vc_rodada.sh <tag> <pasta de saída> [arquivos de codificação...]"; exit 2; }
shift 2
B=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$OUT/codificacao-$TAG" && OUT=$(cd "$OUT" && pwd) || exit 2
CODING=; n=0
for f in "$@"; do
  n=$((n+1)); b=$n-$(basename "$f")
  cp "$f" "$OUT/codificacao-$TAG/$b" || exit 2
  CODING="$CODING /out/codificacao-$TAG/$b"
done
VMAPA=
if [ -n "$MAPA" ]; then
  [ -d "$MAPA" ] || { echo "MAPA não é uma pasta: $MAPA"; exit 2; }
  VMAPA="-v $(cd "$MAPA" && pwd):/mapa:ro"
fi
# NAV aos 80 s da partida da HMI (tecla 13 4, da tabela do Marco 5)
ROT=${ROTEIRO-"80 tecla 13 4 1;0.2 tecla 13 4 0"}
# shellcheck disable=SC2086
docker run --rm --privileged --network none --name "moi3-vc-$TAG" \
  -e TAG="$TAG" -e SECS="${SECS:-150}" -e CODING="$CODING" -e HMILOG="${HMILOG-normal}" -e ROTEIRO="$ROT" \
  -e FOTOS="${FOTOS-70 100 140}" -e LSMVAL="${LSMVAL-}" -e ESPERA="${ESPERA-2000}" \
  -e SERVICOS="${SERVICOS-alm navi dispmgmt navsd mss}" -e NAVPKG="${NAVPKG:-eu/single}" -e GPIO="${GPIO-}" \
  -e WLSTUB="${WLSTUB-}" -e ALMLOG="${ALMLOG:-1}" -e NAVWAIT="${NAVWAIT:-6}" -e NAVW="${NAVW:-1280}" -e NAVH="${NAVH:-480}" \
  -e DIAGALM="${DIAGALM-}" \
  -v "$B/sysroot/mu0883":/fw:ro -v "$B/driver":/driver:ro -v "$B/glforward-moi3":/glf:ro -v "$B/stubs":/stubs:ro \
  $VMAPA -v "$OUT":/out moi3-bancada:1 sh /driver/vc_rodada_container.sh
if command -v ffmpeg >/dev/null 2>&1; then
  for p in "$OUT"/foto_"$TAG"_*.ppm; do [ -f "$p" ] && ffmpeg -loglevel error -y -i "$p" "${p%.ppm}.png" && rm -f "$p"; done
fi
echo "saídas em $OUT: $(ls "$OUT" | grep -c "^$TAG\.\|^foto_${TAG}_") arquivos da rodada $TAG"
