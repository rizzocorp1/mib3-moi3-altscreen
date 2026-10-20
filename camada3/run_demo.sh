#!/bin/sh
# run_demo.sh — demo do C (RS própria) na bancada, só com o mock + produtor + verificador.
# Sobe o mock do DisplayManagement, o produtor cria a RS própria apontando pro surface de um app,
# e o verificador replica o prepareStreamsList do mss pra provar que o grab/destino saem certos.
# Não usa o binário do mss (isso é o próximo marco — ver README). Só stdlib do Python, localhost.
set -e
cd "$(dirname "$0")"
PORT="${PORT:-8780}"
APP_SURFACE="${APP_SURFACE:-1200}"   # id do surface IVI do app (fonte); 1000 é o do mapa nativo na bancada
DEST="${DEST:-ff14::2:4}"
RTP_PORT="${RTP_PORT:-42800}"
FPS="${FPS:-30}"

python3 rsi_mock.py "$PORT" & MOCK=$!
trap 'kill $MOCK 2>/dev/null || true' EXIT INT TERM

# espera o mock subir
python3 - "$PORT" <<'PY'
import socket, sys, time
port = int(sys.argv[1])
for _ in range(50):
    try:
        socket.create_connection(("127.0.0.1", port), 0.1).close(); sys.exit(0)
    except OSError:
        time.sleep(0.05)
sys.exit("mock não subiu")
PY

echo "== produtor: cria a RS própria (fonte surfaceid://$APP_SURFACE -> $DEST:$RTP_PORT) =="
python3 redirect_producer.py --base "http://127.0.0.1:$PORT" \
  --app-surface "$APP_SURFACE" --dest "$DEST" --port "$RTP_PORT" --fps "$FPS"

echo
python3 prepare_streams_check.py --base "http://127.0.0.1:$PORT" \
  --expect-source "$APP_SURFACE" --expect-dest "$DEST:$RTP_PORT"
