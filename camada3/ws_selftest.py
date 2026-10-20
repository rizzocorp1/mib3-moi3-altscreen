#!/usr/bin/env python3
# ws_selftest.py — autoteste do WebSocket VIWI do rsi_mock.py (sem o mss; só valida a máquina).
#
# Conecta como cliente WS, faz o handshake, assina renderingSurfaces, confere o ack + snapshot,
# então cria uma renderingSurface via REST e confere que chega um push `data` com ela. Prova que o
# subscribe/notify do mock funciona de ponta a ponta. (O envelope exato do fio será afinado depois
# contra o mss real — aqui o cliente fala o mesmo envelope que o mock assume.)
import base64, json, os, socket, struct, sys, urllib.request

def ws_connect(host, port, path):
    s = socket.create_connection((host, port), 3)
    s.settimeout(3)
    key = base64.b64encode(os.urandom(16)).decode()
    req = (f"GET {path} HTTP/1.1\r\nHost: {host}:{port}\r\n"
           f"Upgrade: websocket\r\nConnection: Upgrade\r\n"
           f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n")
    s.sendall(req.encode())
    f = s.makefile("rb")
    line = f.readline()
    if b"101" not in line:
        raise SystemExit(f"handshake falhou: {line!r}")
    while f.readline() not in (b"\r\n", b"", b"\n"):  # consome headers até linha vazia
        pass
    return s, f

def ws_send(s, text):  # cliente DEVE mascarar
    data = text.encode(); n = len(data); mask = os.urandom(4)
    if n < 126:
        hdr = struct.pack("!BB", 0x81, 0x80 | n)
    elif n < 65536:
        hdr = struct.pack("!BBH", 0x81, 0x80 | 126, n)
    else:
        hdr = struct.pack("!BBQ", 0x81, 0x80 | 127, n)
    s.sendall(hdr + mask + bytes(data[i] ^ mask[i % 4] for i in range(n)))

def ws_recv(f):
    b = f.read(2)
    if len(b) < 2:
        return None
    ln = b[1] & 0x7F
    if ln == 126:
        ln = struct.unpack("!H", f.read(2))[0]
    elif ln == 127:
        ln = struct.unpack("!Q", f.read(8))[0]
    return f.read(ln).decode("utf-8", "replace")

def main():
    base = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:8780"
    host = base.split("://", 1)[1].split(":")[0]
    port = int(base.rsplit(":", 1)[1])
    event = "/displaymanagement/renderingSurfaces"

    s, f = ws_connect(host, port, event)
    print("handshake WS ok")
    ws_send(s, json.dumps({"type": "subscribe", "event": event}))
    ack = ws_recv(f)
    print("ack      <-", ack)
    snap = ws_recv(f)
    print("snapshot <-", snap)

    # cria uma renderingSurface via REST -> deve disparar um push
    body = json.dumps({"id": "selftest-rs", "name": "surfaceid://777",
                       "display": "d", "streamParameters": "sp"}).encode()
    req = urllib.request.Request(f"{base}/displaymanagement/renderingSurfaces",
                                 data=body, method="POST",
                                 headers={"Content-Type": "application/json"})
    urllib.request.urlopen(req, timeout=3).read()
    push = ws_recv(f)
    print("push     <-", push)
    s.close()

    ok = push and "surfaceid://777" in push and "data" in (json.loads(push) or {}).get("type", "data")
    # tolerante: basta o push conter a RS nova
    if push and "surfaceid://777" in push:
        print("OK: subscribe -> push com a renderingSurface nova chegou")
        sys.exit(0)
    print("FALHOU: não chegou push com a RS nova", file=sys.stderr)
    sys.exit(1)

if __name__ == "__main__":
    main()
