#!/bin/sh
# run_rsi.sh — experimento "mss em modo RSI contra o mock VIWI" (roda no host, Docker).
#   sh run_rsi.sh [output-dir]
# Usa a imagem moi3-bancada:rsi (bancada + python3). Firmware NÃO vai no repo; FW/GLF default apontam
# para este workspace. Container --privileged (loop-mount do system.img) e --network none.
set -e
CAM=$(cd "$(dirname "$0")" && pwd)                 # .../mib3-sandbox/camada3
SB=$(cd "$CAM/.." && pwd)                           # .../mib3-sandbox
VAG=$(cd "$SB/../../.." && pwd)                     # .../VAGCoding
POC=${POC:-$VAG/moi3-cluster-pipeline-re}          # shims/compositor/glforward-ext
FW=${FW:-$SB/sysroot/mu0883}                        # system.img + vw/
GLF=${GLF:-$SB/glforward-moi3}                      # OpenGL forwarder sources
OUT=${1:-/tmp/rsi-exp}
IMG=${IMG:-moi3-bancada:rsi}

[ -f "$FW/system.img" ] || { echo "sem firmware em $FW (set FW=...)"; exit 1; }
[ -f "$GLF/build.sh" ]  || { echo "sem glforward em $GLF (set GLF=...)"; exit 1; }
[ -f "$POC/glforward-ext/apply.py" ] || { echo "sem $POC/glforward-ext (set POC=...)"; exit 1; }
docker image inspect "$IMG" >/dev/null 2>&1 || {
  echo "imagem $IMG não existe — build: docker build -f $CAM/Dockerfile.rsi -t $IMG $CAM"; exit 1; }
mkdir -p "$OUT"

echo "patching glforward -> $OUT/glf-ext"
python3 -I "$POC/glforward-ext/apply.py" "$GLF" "$OUT/glf-ext" || { echo "glforward patch falhou"; exit 1; }

docker run --rm --privileged --network none --name moi3-rsi-exp \
  -e SECS="${SECS:-20}" -e RSI_PORT="${RSI_PORT:-8780}" -e MSS_ARGS="${MSS_ARGS-}" \
  -e SEED="${SEED:-/c/seed_navi_kombi.json}" \
  -v "$FW":/fw:ro -v "$GLF":/glf:ro -v "$POC":/poc:ro -v "$CAM":/c:ro -v "$OUT":/out \
  "$IMG" sh /c/rsi_incontainer.sh
echo "saída em $OUT"
