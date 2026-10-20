#!/usr/bin/env python3
# rtp_rx.py — captura o RTP que o mss emite na bancada e extrai o MPEG-TS pra decodificar depois.
#
# O stream vai p/ [ff14::2:4]:42800 (streamParameters do seed), RTP v2 PT=33 (MPEG-TS). Com
# --network none só há lo; o sendto do mss vai pro lo (rota ff00::/8 dev lo). Em vez de entrar no
# grupo multicast (entrega no lo é instável), capturo RAW (AF_PACKET no lo) — vê o pacote de qualquer
# jeito —, filtro UDP dst 42800, tiro o cabeçalho RTP (12 bytes) e junto o TS em /out/captured.ts.
#   rtp_rx.py [porta] [saida.ts] [log]
import socket, struct, sys, fcntl

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 42800
TSF  = sys.argv[2] if len(sys.argv) > 2 else "/out/captured.ts"
LOGF = sys.argv[3] if len(sys.argv) > 3 else "/out/rtp.log"

log = open(LOGF, "w")
def L(*a): print("[rtp_rx]", *a, file=log, flush=True); print("[rtp_rx]", *a, file=sys.stderr, flush=True)

def bring_lo_up():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        ifr = struct.pack("16sH14s", b"lo", 0, b"\0" * 14)
        cur = struct.unpack("16sH14s", fcntl.ioctl(s, 0x8913, ifr))[1]
        fcntl.ioctl(s, 0x8914, struct.pack("16sH14s", b"lo", cur | 0x1 | 0x1000, b"\0" * 14))
        s.close(); L("lo up")
    except Exception as e:
        L("nao subiu lo:", e)

bring_lo_up()
ETH_P_IPV6 = 0x86DD
s = socket.socket(socket.AF_PACKET, socket.SOCK_DGRAM, socket.htons(ETH_P_IPV6))
s.bind(("lo", 0))
s.settimeout(1.0)
ts = open(TSF, "wb")
L("capturando RAW no lo, UDP dst %d -> %s" % (PORT, TSF))
npkt = 0; nts = 0; marker_frames = 0
while True:
    try:
        data = s.recv(65535)
    except socket.timeout:
        continue
    except Exception as e:
        L("erro:", e); break
    if len(data) < 48:            # IPv6(40) + UDP(8)
        continue
    if data[6] != 17:             # next header != UDP
        continue
    udp = data[40:]
    dport = struct.unpack("!H", udp[2:4])[0]
    if dport != PORT:
        continue
    pl = udp[8:]                  # payload UDP = RTP
    if len(pl) < 12 or (pl[0] >> 6) != 2:   # RTP v2
        continue
    npkt += 1
    if pl[1] & 0x80: marker_frames += 1      # marker = fim de frame
    tsbytes = pl[12:]             # tira o cabeçalho RTP -> MPEG-TS (começa em 0x47)
    ts.write(tsbytes); nts += len(tsbytes)
    if npkt <= 8 or npkt % 500 == 0:
        L("pkt #%d %dB pt=%d seq=%d ts0=%#x (frames~%d, TS %dB)" %
          (npkt, len(pl), pl[1] & 0x7f, struct.unpack("!H", pl[2:4])[0],
           tsbytes[0] if tsbytes else 0, marker_frames, nts))
ts.close()
