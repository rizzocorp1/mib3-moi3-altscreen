#!/bin/sh
# rsi_incontainer.sh — roda DENTRO de moi3-bancada:rsi. Igual ao spike do mss, mas:
#   (1) sobe o mock VIWI (rsi_mock.py) no 127.0.0.1 do container (fora do chroot; python3 vive aqui),
#   (2) roda o mss SEM args de surface (modo RSI, não o atalho CLI),
#   (3) strace focado em REDE pra capturar onde o mss procura o registry e o que ele manda.
# Objetivo: ler o bootstrap do SR (primeiro connect) e o frame real do subscribe. Throwaway, sem rede.
#
# Mounts: /fw (sysroot ro), /glf (glforward-moi3), /poc (moi3-cluster-pipeline-re), /c (camada3), /out.
set -e
W=/tmp/w; LOW=/mnt/low; O=/out
DLLP=/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif
SECS=${SECS:-20}
RSI_PORT=${RSI_PORT:-8780}
SEED=${SEED:-/c/seed_navi_kombi.json}   # estado de boot pra semear o mock ("" = vazio)
MSS=/lge/app_ro/graphics/multi-display/multi-screen-server
# modo RSI = sem args de surface. Overridável por MSS_ARGS se quisermos testar variações.
MSS_ARGS="${MSS_ARGS-}"

mkdir -p $W/fwlibs $W/bin $W/glfwd $LOW $O/frames
mount -o loop,ro,noload /fw/system.img $LOW
mount --bind /fw/vw $LOW/vw
mount -t tmpfs tmpfs $LOW/tmp
mount -t tmpfs tmpfs $LOW/run
cp -L $LOW/lib64/libc.so.6 $LOW/lib64/libpthread.so.0 $LOW/lib64/ld-linux-aarch64.so.1 $W/fwlibs/

echo "[rsi] building glforward + compositor" >&2
FWLIBS=$W/fwlibs sh /out/glf-ext/build.sh $W/glfwd > $O/build.log 2>&1
gcc -static -O2 -Wall -o $W/bin/ivishare-stub /poc/compositor/ivishare-stub.c >> $O/build.log 2>&1
gcc -static -O2 -Wall -o $W/bin/tracecatch /poc/spike/tracecatch.c >> $O/build.log 2>&1

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

echo "[rsi] starting mock (VIWI) on 127.0.0.1:$RSI_PORT + compositor + glhost" >&2
# o mock roda FORA do chroot (python3 vive na imagem), mas no MESMO netns -> 127.0.0.1 compartilhado
python3 /c/rsi_mock.py $RSI_PORT -v ${SEED:+--seed $SEED} > $O/mock.log 2>&1 &
MOCK=$!
$W/bin/tracecatch $LOW/tmp/trace_unix_socket > $O/trace.log 2>&1 &
TR=$!
$W/bin/ivishare-stub $LOW/run/platform/weston/wayland-0 800 480 174 104 > $O/wlstub.log 2>&1 &
WL=$!
GLHOST_STREAM= $W/glfwd/glhost $LOW/tmp/moi3-gl-mss.sock 1280 480 $O/frames > $O/glhost.log 2>&1 &
GH=$!
sleep 1

echo "[rsi] running mss in RSI mode (MSS_ARGS='$MSS_ARGS') for ${SECS}s under strace" >&2
cat > $LOW/tmp/run_mss.sh <<EOS
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/tmp/fakegl:$DLLP
export LD_PRELOAD=/tmp/fakegl/fakedrm.so
export MOI3_GL_SOCKET=/tmp/moi3-gl-mss.sock
exec $MSS $MSS_ARGS
EOS
timeout $SECS strace -f -s 300 \
  -e trace=connect,socket,bind,sendto,recvfrom,sendmsg,recvmsg,getsockname,openat,execve,exit_group \
  -o $O/mss.strace chroot $LOW /bin/sh /tmp/run_mss.sh > $O/mss.out 2>&1 || true

sleep 0.5
kill $MOCK $WL $GH $TR 2>/dev/null || true

{
  echo "=== mss stdout/stderr (logs do librsic: 'Creating WsClientImpl ws://...', 'subscribe event=...') ==="
  cat $O/mss.out 2>/dev/null
  echo; echo "=== CONEXÕES que o mss tentou (onde ele procura o RSI/registry) ==="
  grep -E 'connect\(|AF_INET|AF_UNIX|sin_port|sun_path' $O/mss.strace 2>/dev/null | grep -vE 'ENOENT|wayland|weston|moi3-gl-mss' | head -40
  echo; echo "=== arquivos de config que o mss abriu (bootstrap do SR?) ==="
  grep -E 'openat\(' $O/mss.strace 2>/dev/null | grep -iE 'rsi|registry|service|displaymanag|\.conf|\.cfg|\.json|sai|viwi|etc/' | grep -v ENOENT | head -30
  echo; echo "=== o que o mock recebeu (requisições + frames WS) ==="
  cat $O/mock.log 2>/dev/null
} > $O/resumo-rsi.txt 2>&1
echo "[rsi] done; veja /out/resumo-rsi.txt" >&2
cat $O/resumo-rsi.txt
