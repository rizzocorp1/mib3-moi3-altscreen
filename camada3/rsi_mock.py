#!/usr/bin/env python3
# rsi_mock.py — mock VIWI do barramento RSI da MOI3, para a bancada do C (camada 3).
#
# Cresceu de "só REST" para as TRÊS peças que o cliente RSI do mss (librsic.so / SAI) usa, mapeadas
# por RE (ver ../camada3-rsi-redirect.md "RE do cliente RSI do mss"):
#   (1) Service Registry  — o mss resolve o location "displaymanagement" -> host:port aqui.
#   (2) REST              — GET/POST/PUT/DELETE em /displaymanagement/{displays,renderingSurfaces,
#                           streamParameters}.
#   (3) WebSocket VIWI    — subscribe/unsubscribe + push de `data` quando um recurso muda.
#
# NÃO é o rsi-display-management nem o service-registry reais. É o alvo de desenvolvimento do
# produtor (redirect_producer.py) e, depois, o que vamos crescer/afinar empiricamente contra o mss
# real lendo os logs do librsic (`Creating WsClientImpl ws://host:port`, `subscribe event=...`).
#
# O ENVELOPE VIWI aqui é um palpite informado (modelado no VIWI / wzr1337/rsi.server) e está isolado
# em viwi_* abaixo para ser trocado quando virmos o que o mss manda de verdade. Todo frame WS e toda
# requisição são logados. Só stdlib do Python; localhost.
#
# Escopo: caminho de EXIBIÇÃO, bancada. Não derrota proteção nenhuma; nada vai pra veículo.
import base64, hashlib, json, struct, sys, threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

BASE = "/displaymanagement"
COLLECTIONS = ("displays", "renderingSurfaces", "streamParameters")
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"  # RFC6455
VERBOSE = "-v" in sys.argv

def log(*a):
    print("[rsi_mock]", *a, flush=True)

# ------------------------------------------------------------------ estado (REST) ----
class Store:
    def __init__(self):
        self.lock = threading.Lock()
        self.data = {c: {} for c in COLLECTIONS}  # collection -> { id -> element }

    def rid(self, el, coll):
        # id REST (o +0x8 do mss, usado no gate currentVisibleSurfaces), distinto do campo `name`.
        return el.get("id") or el.get("name") or f"{coll[:-1]}-{len(self.data[coll])}"

STORE = Store()

# ----------------------------------------------------------- conexões WebSocket ------
class WsConn:
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.sendlock = threading.Lock()
        self.subs = {}         # evento completo (com #_rsi_N) -> (collection, elem_id|None)

CONNS = set()
CONNS_LOCK = threading.Lock()

def ws_send_text(conn, text):
    data = text.encode()
    n = len(data)
    if n < 126:
        hdr = struct.pack("!BB", 0x81, n)
    elif n < 65536:
        hdr = struct.pack("!BBH", 0x81, 126, n)
    else:
        hdr = struct.pack("!BBQ", 0x81, 127, n)
    with conn.sendlock:
        conn.sock.sendall(hdr + data)

def ws_recv_frame(rfile):
    # retorna (opcode, payload_bytes) ou None no fim/close
    b = rfile.read(2)
    if len(b) < 2:
        return None
    b0, b1 = b[0], b[1]
    opcode = b0 & 0x0F
    masked = b1 & 0x80
    ln = b1 & 0x7F
    if ln == 126:
        ln = struct.unpack("!H", rfile.read(2))[0]
    elif ln == 127:
        ln = struct.unpack("!Q", rfile.read(8))[0]
    mask = rfile.read(4) if masked else b""
    payload = rfile.read(ln) if ln else b""
    if masked:
        payload = bytes(payload[i] ^ mask[i % 4] for i in range(ln))
    return opcode, payload

# ------------------------------------------------------- envelope VIWI (a confirmar) --
# Isolado aqui de propósito: quando virmos o frame real do mss, é só mexer nestas 3 funções.
def viwi_extract_event(msg):
    # do frame recebido, tira o "event" (ex. "/displaymanagement/displays#_rsi_1").
    if isinstance(msg, dict):
        v = msg.get("event")
        if isinstance(v, str):
            return v
    return None

def parse_event(event):
    # "/displaymanagement/renderingSurfaces/navi-kombi-renderingsurface#_rsi_5"
    #   -> (coll="renderingSurfaces", elem="navi-kombi-renderingsurface"); coleção -> elem=None.
    # O #_rsi_N (fragmento, após '#') é o session id — o librsic o separa do caminho.
    path = (event or "").split("#", 1)[0].rstrip("/")
    if not path.startswith(BASE + "/"):
        return None, None
    rest = path[len(BASE) + 1:].split("/")
    coll = rest[0]
    if coll not in COLLECTIONS:
        return None, None
    return coll, (rest[1] if len(rest) > 1 and rest[1] else None)

def viwi_ack(event, code=200):
    # onMessage (@0x135078) ignora quem não é FIELD_DATA; o ack é para o WsSubscribeClient.
    return json.dumps({"type": "subscribe", "event": event, "code": code})

def viwi_data(event, collection, elem):
    # NOTIFICAÇÃO. RE: broadcastMessage (@0x160380) casa pelo SESSION ID (o "#_rsi_N" do fragmento),
    # então `event` TEM de incluir o #_rsi_N. onMessage (@0x135078): coleção -> getArray (data=array);
    # elemento -> getObject (data=objeto). A chave do payload é "data".
    with STORE.lock:
        data = STORE.data[collection].get(elem, {}) if elem else list(STORE.data[collection].values())
    return json.dumps({"type": "data", "event": event, "code": 200, "data": data})

def broadcast_change(collection):
    """empurra o estado de `collection` (e de cada elemento assinado dela) para os assinantes."""
    with CONNS_LOCK:
        targets = [(c, ev, el) for c in CONNS for ev, (cl, el) in c.subs.items() if cl == collection]
    for c, ev, el in targets:
        try:
            ws_send_text(c, viwi_data(ev, collection, el))
            log(f"push data -> {c.addr}: {ev}")
        except OSError:
            pass

# --------------------------------------------------------- registry (descoberta) -----
def registry_services(host, port):
    # lista de serviços que o mss pode resolver. Formato VIWI-ish (a confirmar contra o mss):
    # cada serviço aponta para o nosso próprio host:port (onde servimos REST+WS).
    return {"services": [{
        "name": "displaymanagement",
        "uri": f"{BASE}",
        "version": "1.0.0",
        "host": host, "port": port,
        "baseUri": f"http://{host}:{port}{BASE}",
        "wsUri": f"ws://{host}:{port}{BASE}",
    }]}

# ---------------------------------------------------------------- HTTP/WS handler ----
def split_path(path):
    p = path.split("?", 1)[0].rstrip("/")
    if not p.startswith(BASE + "/"):
        return None, None
    rest = p[len(BASE) + 1:].split("/", 1)
    coll = rest[0]
    eid = rest[1] if len(rest) > 1 and rest[1] else None
    return (coll if coll in COLLECTIONS else None), eid

class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *a):
        if VERBOSE:
            log("http", self.command, self.path)

    # -- helpers REST --
    def _json(self, code, obj, extra=None):
        body = json.dumps(obj).encode()
        if VERBOSE:
            log(f"RESP {code} ({len(body)}b): {body[:300].decode('utf-8','replace')}")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        for k, v in (extra or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def _read(self):
        n = int(self.headers.get("Content-Length", 0))
        return json.loads(self.rfile.read(n) or b"{}") if n else {}

    # -- WebSocket --
    def _is_ws(self):
        up = (self.headers.get("Upgrade") or "").lower()
        return "websocket" in up and self.headers.get("Sec-WebSocket-Key")

    def _ws_handshake(self):
        key = self.headers["Sec-WebSocket-Key"]
        accept = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
        resp = ("HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                f"Sec-WebSocket-Accept: {accept}\r\n\r\n")
        self.connection.sendall(resp.encode())

    def _ws_loop(self):
        self._ws_handshake()
        conn = WsConn(self.connection, self.client_address)
        with CONNS_LOCK:
            CONNS.add(conn)
        log(f"ws connect {conn.addr}")
        try:
            while True:
                fr = ws_recv_frame(self.rfile)
                if fr is None:
                    break
                opcode, payload = fr
                if opcode == 0x8:            # close
                    break
                if opcode == 0x9:            # ping -> pong
                    with conn.sendlock:
                        self.connection.sendall(struct.pack("!BB", 0x8A, len(payload)) + payload)
                    continue
                if opcode not in (0x1, 0x2):
                    continue
                raw = payload.decode("utf-8", "replace")
                log(f"ws frame <- {conn.addr}: {raw}")   # <<< o que o mss manda (empírico)
                try:
                    msg = json.loads(raw)
                except ValueError:
                    continue
                self._ws_handle(conn, msg)
        finally:
            with CONNS_LOCK:
                CONNS.discard(conn)
            log(f"ws disconnect {conn.addr}")
            self.close_connection = True

    def _ws_handle(self, conn, msg):
        mtype = (msg.get("type") or "").lower() if isinstance(msg, dict) else ""
        event = viwi_extract_event(msg)
        coll, elem = parse_event(event)
        if "unsub" in mtype:
            conn.subs.pop(event, None)
            log(f"unsubscribe {event} <- {conn.addr}")
            ws_send_text(conn, viwi_ack(event))
            return
        if "sub" in mtype or event:
            if coll:
                conn.subs[event] = (coll, elem)   # chave = evento completo (com session id)
            log(f"subscribe {event} (coll={coll} elem={elem}) <- {conn.addr}")
            ws_send_text(conn, viwi_ack(event))
            if coll:
                ws_send_text(conn, viwi_data(event, coll, elem))   # snapshot
            return
        log(f"ws msg (sem handler) <- {conn.addr}: {msg}")

    # -- métodos HTTP --
    def do_GET(self):
        if VERBOSE:
            log(f"REQ GET {self.path} | hdrs: " +
                "; ".join(f"{k}={v}" for k, v in self.headers.items()))
        if self._is_ws():
            return self._ws_loop()
        host = self.headers.get("Host", "127.0.0.1").split(":")[0]
        port = self.server.server_address[1]
        p = self.path.split("?", 1)[0].rstrip("/")
        # registry: respondemos a alguns caminhos plausíveis + logamos pra ver o que o mss pede
        if "registr" in p.lower() or p.endswith("/services") or p in ("", "/"):
            log("registry GET", self.path)
            return self._json(200, registry_services(host, port))
        coll, eid = split_path(self.path)
        if not coll:
            log("GET desconhecido:", self.path)
            return self._json(404, {"error": "unknown resource", "path": self.path})
        with STORE.lock:
            if eid is None:
                # RE: Connector::checkConnection exige um objeto com a chave "status" no GET
                # (só testa existência) pra considerar o serviço "up" e então ASSINAR. @0x12c1e8
                return self._json(200, {"status": {"code": 200, "message": "OK"},
                                        coll: list(STORE.data[coll].values())})
            el = STORE.data[coll].get(eid)
        return self._json(200, el) if el else self._json(404, {"error": "not found"})

    def do_POST(self):
        coll, _ = split_path(self.path)
        if not coll:
            return self._json(404, {"error": "unknown resource", "path": self.path})
        el = self._read()
        with STORE.lock:
            k = STORE.rid(el, coll)
            el["id"] = k
            el.setdefault("name", k)
            STORE.data[coll][k] = el
        self._json(201, el, {"Location": f"{BASE}/{coll}/{k}"})
        broadcast_change(coll)

    def do_PUT(self):
        coll, eid = split_path(self.path)
        if not coll or not eid:
            return self._json(404, {"error": "need /collection/id"})
        patch = self._read()
        with STORE.lock:
            el = STORE.data[coll].get(eid, {"id": eid, "name": eid})
            el.update(patch)
            STORE.data[coll][eid] = el
        self._json(200, el)
        broadcast_change(coll)

    do_PATCH = do_PUT

    def do_DELETE(self):
        coll, eid = split_path(self.path)
        if not coll or not eid:
            return self._json(404, {"error": "need /collection/id"})
        with STORE.lock:
            STORE.data[coll].pop(eid, None)
        self._json(204, {})
        broadcast_change(coll)

def load_seed(path):
    with open(path) as fh:
        seed = json.load(fh)
    with STORE.lock:
        for coll in COLLECTIONS:
            for el in seed.get(coll, []):
                k = STORE.rid(el, coll)
                el["id"] = k
                el.setdefault("name", k)
                STORE.data[coll][k] = el
    log("seed:", ", ".join(f"{c}={len(STORE.data[c])}" for c in COLLECTIONS), f"(de {path})")

def main():
    args = sys.argv[1:]
    port, seed = 8780, None
    i = 0
    while i < len(args):
        if args[i] == "--seed" and i + 1 < len(args):
            seed = args[i + 1]; i += 2; continue
        if args[i].isdigit():
            port = int(args[i])
        i += 1
    if seed:
        load_seed(seed)
    srv = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    log(f"VIWI mock em http://127.0.0.1:{port}  (REST {BASE}/*, registry, e WS no mesmo porto)")
    log(f"recursos: {', '.join(COLLECTIONS)}")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()
