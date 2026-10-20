#!/bin/sh
# gdb_incontainer.sh — igual ao rsi_incontainer.sh, mas roda o mss SOB gdb (imagem moi3-bancada:rsidbg)
# pra observar o data-plane: breakpoint em updateStreamsList (sempre chamado no fim de
# prepareStreamsList) imprimindo o TAMANHO da lista de streams. Diz, sem adivinhar, se o mss emite
# stream ou não com o seed atual. gdb roda FORA do chroot e ptraceia o processo (lê símbolos via
# /proc/PID/exe + .dynsym). Throwaway, sem rede.
set -e
W=/tmp/w; LOW=/mnt/low; O=/out
DLLP=/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif
SECS=${SECS:-25}
RSI_PORT=${RSI_PORT:-49600}
SEED=${SEED:-/c/seed_navi_kombi.json}
MSS=/lge/app_ro/graphics/multi-display/multi-screen-server
MSS_ARGS="${MSS_ARGS-}"

mkdir -p $W/fwlibs $W/bin $W/glfwd $LOW $O/frames
mount -o loop,ro,noload /fw/system.img $LOW
mount --bind /fw/vw $LOW/vw
mount -t tmpfs tmpfs $LOW/tmp
mount -t tmpfs tmpfs $LOW/run
cp -L $LOW/lib64/libc.so.6 $LOW/lib64/libpthread.so.0 $LOW/lib64/ld-linux-aarch64.so.1 $W/fwlibs/

# rede multicast no lo (bancada --network none): rota p/ o mss emitir o RTP e o rtp_rx capturar
ip link set lo up 2>/dev/null || true
ip -6 route replace ff00::/8 dev lo 2>/dev/null || ip -6 route add ff00::/8 dev lo 2>/dev/null || true

echo "[gdb] building glforward + compositor" >&2
FWLIBS=$W/fwlibs sh /out/glf-ext/build.sh $W/glfwd > $O/build.log 2>&1
gcc -static -O2 -Wall -o $W/bin/ivishare-stub /poc/compositor/ivishare-stub.c >> $O/build.log 2>&1

mkdir -p $LOW/tmp/fakegl $LOW/run/platform/weston
cp $W/glfwd/libGLESv2_adreno.so $W/glfwd/libEGL_adreno.so $LOW/tmp/fakegl/
gcc -shared -fPIC -O2 -Wall -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
    -mno-outline-atomics -nostdlib -Wl,-soname,libgbm.so -Wl,-rpath-link,$W/fwlibs \
    -o $LOW/tmp/fakegl/libgbm.so /poc/shims/fakegbm.c $W/fwlibs/libc.so.6 >> $O/build.log 2>&1
gcc -shared -fPIC -O2 -Wall -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
    -mno-outline-atomics -nostdlib -I/poc/shims -Wl,-soname,libOmxCore.so -Wl,-rpath-link,$W/fwlibs \
    -o $LOW/tmp/fakegl/libOmxCore.so /poc/shims/fakeomxcore.c $W/fwlibs/libc.so.6 $W/fwlibs/libpthread.so.0 >> $O/build.log 2>&1
gcc -shared -fPIC -O2 -Wall -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
    -mno-outline-atomics -nostdlib -Wl,-soname,fakedrm.so -Wl,-rpath-link,$W/fwlibs \
    -o $LOW/tmp/fakegl/fakedrm.so /poc/shims/fakedrm.c $W/fwlibs/libc.so.6 >> $O/build.log 2>&1

echo "[gdb] starting mock (VIWI) on 127.0.0.1:$RSI_PORT + compositor + glhost" >&2
python3 /c/rsi_mock.py $RSI_PORT -v ${SEED:+--seed $SEED} > $O/mock.log 2>&1 &
MOCK=$!
$W/bin/ivishare-stub $LOW/run/platform/weston/wayland-0 800 480 174 104 > $O/wlstub.log 2>&1 &
WL=$!
GLHOST_STREAM= $W/glfwd/glhost $LOW/tmp/moi3-gl-mss.sock 1280 480 $O/frames > $O/glhost.log 2>&1 &
GH=$!
# daemon encoder H.264 (host-side; o fakeomx encaminha NV12 por este socket e recebe H.264)
ENC_DUMP=/out/stream.h264 python3 /c/enc_daemon.py $LOW/tmp/moi3-enc.sock 1280 480 > $O/enc.log 2>&1 &
ENC=$!
# receptor multicast: sobe o lo e captura o RTP que o mss emite em [ff14::2:4]:42800
python3 /c/rtp_rx.py ff14::2:4 42800 $O/rtp.log > $O/rtp_rx.stderr 2>&1 &
RX=$!
sleep 1

cat > $LOW/tmp/run_mss.sh <<EOS
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/tmp/fakegl:$DLLP
export LD_PRELOAD=/tmp/fakegl/fakedrm.so
export MOI3_GL_SOCKET=/tmp/moi3-gl-mss.sock
export MOI3_ENC_SOCKET=/tmp/moi3-enc.sock
exec $MSS $MSS_ARGS
EOS

echo "[gdb] running mss under gdb for ${SECS}s (breakpoint updateStreamsList)" >&2
# gdb roda fora do chroot; --disable-randomization é default no gdb. Segue os execs chroot->sh->mss.
if [ -n "$STRACE" ]; then
  # diagnose do envio RTP: trace dos syscalls de rede do mss (sendto/sendmsg/bind/connect/setsockopt)
  timeout $SECS strace -f -e trace=%network -o $O/strace.log chroot $LOW /bin/sh /tmp/run_mss.sh > $O/gdb.out 2>&1 || true
else
  timeout $SECS gdb -q -batch -x /c/gdb_cmds.txt --args chroot $LOW /bin/sh /tmp/run_mss.sh > $O/gdb.out 2>&1 || true
fi

sleep 0.5
sleep 0.5
kill $MOCK $WL $GH $ENC $RX 2>/dev/null || true

{
  echo "=== gdb.out (decisao do mss: tamanho da lista de streams) ==="
  cat $O/gdb.out 2>/dev/null
  echo; echo "=== wlstub (grab de ivi?) ==="
  grep -aE "ivi_share|get_ivi_share|surface" $O/wlstub.log 2>/dev/null | head
  echo; echo "=== glhost (quadros) ==="
  grep -aE "resumo|quadros" $O/glhost.log 2>/dev/null | head
} > $O/resumo-gdb.txt 2>&1
echo "[gdb] done; veja /out/resumo-gdb.txt" >&2
cat $O/resumo-gdb.txt
