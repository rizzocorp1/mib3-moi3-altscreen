#!/usr/bin/env python3
# enc_daemon.py — encoder H.264 do lado host da bancada MOI3.
#
# x264/openh264 não linkam no shim -nostdlib do fakeomx (glibc 2.23 do FW), então o encoder real roda
# AQUI (container Ubuntu, via PyAV/libx264) e o fakeomx só encaminha: recebe NV12 por um unix socket,
# codifica um frame e devolve H.264 Annex-B. All-intra (gop=1) + zerolatency: cada frame é um IDR
# independente (SPS+PPS+IDR), robusto para o envio RTP por-frame do mss e folgado dentro dos 614400 B
# do buffer de saída OMX. Throwaway, sem rede (socket unix em mount compartilhado).
#
#   enc_daemon.py <socket> [w] [h]
#
# Protocolo (little-endian): req = "ENC0"(4) w(4) h(4) size(4) + size bytes NV12 (Y de w*h, depois UV
# intercalado de w*(h/2)). resp = nal_size(4) flags(4) + nal_size bytes (flags bit0 = IDR). nal_size=0
# quando o encoder não emitiu pacote para esse frame.
import sys, os, socket, struct, av

SOCK = sys.argv[1] if len(sys.argv) > 1 else "/tmp/moi3-enc.sock"
DUMP = os.environ.get("ENC_DUMP")        # se setado, grava o H.264 Annex-B emitido (p/ decodificar e conferir)
_dumpf = open(DUMP, "wb") if DUMP else None

def log(*a): print("[enc]", *a, file=sys.stderr, flush=True)

def recvn(c, n):
    b = bytearray()
    while len(b) < n:
        d = c.recv(n - len(b))
        if not d: return None
        b += d
    return bytes(b)

def sendall(c, b):
    c.sendall(b)

def fill_plane(plane, data, wbytes, rows):
    """Copia data (empacotado, wbytes por linha) para o plano, respeitando o line_size do libav."""
    ls = plane.line_size
    if ls == wbytes and len(data) >= wbytes * rows:
        plane.update(bytes(data[:wbytes * rows]))
        return
    buf = bytearray(ls * rows)
    for r in range(rows):
        src = data[r * wbytes:(r + 1) * wbytes]
        buf[r * ls:r * ls + len(src)] = src
    plane.update(bytes(buf))

def make_encoder(w, h):
    cc = av.CodecContext.create("libx264", "w")
    cc.width = w; cc.height = h
    cc.pix_fmt = "yuv420p"
    cc.gop_size = 1                       # all-intra: cada frame é IDR
    cc.framerate = 30
    cc.time_base = "1/30"
    cc.options = {"tune": "zerolatency", "crf": "28", "repeat-headers": "1"}
    return cc

def encode_frame(cc, buf, w, h):
    f = av.VideoFrame(w, h, "nv12")
    ysz = w * h
    fill_plane(f.planes[0], buf, w, h)                 # Y
    fill_plane(f.planes[1], buf[ysz:], w, h // 2)      # UV intercalado (w bytes/linha, h/2 linhas)
    f = f.reformat(format="yuv420p")
    out = bytearray(); flags = 0
    for pkt in cc.encode(f):
        out += bytes(pkt)
        if pkt.is_keyframe: flags |= 1
    return bytes(out), flags

def main():
    if os.path.exists(SOCK):
        try: os.unlink(SOCK)
        except OSError: pass
    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(SOCK); srv.listen(1)
    log("escutando em", SOCK)
    cc = None; cw = ch = 0; n = 0
    while True:
        c, _ = srv.accept()
        log("cliente conectou")
        try:
            while True:
                hdr = recvn(c, 16)
                if not hdr: break
                magic, w, h, size = struct.unpack("<4sIII", hdr)
                if magic != b"ENC0" or size == 0 or size > 64 * 1024 * 1024:
                    log("req inválida magic=%r w=%d h=%d size=%d" % (magic, w, h, size)); break
                buf = recvn(c, size)
                if buf is None: break
                if cc is None or cw != w or ch != h:
                    cc = make_encoder(w, h); cw, ch = w, h
                    log("encoder %dx%d (x264 all-intra, crf28, zerolatency)" % (w, h))
                try:
                    nal, flags = encode_frame(cc, buf, w, h)
                except Exception as e:
                    log("erro no encode:", e); nal, flags = b"", 0
                sendall(c, struct.pack("<II", len(nal), flags) + nal)
                if _dumpf and nal: _dumpf.write(nal); _dumpf.flush()
                n += 1
                if n <= 5 or n % 60 == 0:
                    log("frame #%d -> %d bytes nal (idr=%d)" % (n, len(nal), flags & 1))
        except Exception as e:
            log("conexão caiu:", e)
        finally:
            c.close()

if __name__ == "__main__":
    main()
