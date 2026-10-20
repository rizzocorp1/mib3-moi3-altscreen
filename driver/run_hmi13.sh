#!/bin/bash
# HMI sob glibc 2.27 (Ubuntu 18.04, forward-compat do firmware 2.23) + Mesa llvmpipe REAL.
# Salto de ABI menor que 2.39 -> menos risco de crash dos libs nativos do firmware.
apt-get -qq update >/dev/null 2>&1
apt-get -qq install -y mesa-utils-extra libgl1-mesa-dri gcc >/dev/null 2>&1 || true
apt-get -qq install -y libgles2 libegl1 >/dev/null 2>&1 || apt-get -qq install -y libgles2-mesa libegl1-mesa >/dev/null 2>&1 || true
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c
gcc -static -O2 -o /tmp/dmstub /driver/dmstub.c
echo "[build] glibc=$(ldd --version|head -1|grep -oE '[0-9]+\.[0-9]+$') Mesa=$(EGL_PLATFORM=surfaceless eglinfo 2>/dev/null | grep -i 'ES profile version' | head -1)"
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW
mkdir -p /lge /vw /fwusr /fwlib
mount --bind $LOW/lge /lge
mount --bind /scratch/vwroot /vw
mount --bind $LOW/usr/lib64 /fwusr
mount --bind $LOW/lib64 /fwlib
mkdir -p /data/lge/hmi /data/lge/persistency /persist /media/mp000 /run/platform/weston \
         /var/lib/hmi /var/lib/nav/coredump /var/cache/hmi /etc/gem/esd 2>/dev/null || true
: > /data/lge/hmi/lsmval 2>/dev/null || true
: > /dev/csr 2>/dev/null || true; : > /dev/css 2>/dev/null || true; chmod 666 /dev/csr /dev/css 2>/dev/null || true
mkdir -p /opt/fakegl
MESADIR=/usr/lib/aarch64-linux-gnu
ln -sf $MESADIR/libEGL.so.1    /opt/fakegl/libEGL_adreno.so
ln -sf $MESADIR/libGLESv2.so.2 /opt/fakegl/libGLESv2_adreno.so

/tmp/kipc_hub 2>/tmp/hub.log &
sleep 1
DLLP="/usr/lib/aarch64-linux-gnu:/lge/app_ro/lib64:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif:/fwusr:/fwlib"
for d in /lge/app_ro/hmi/persistence-app /lge/app_ro/hmi/mcu-interpreter /lge/app_ro/carif/carifservice /lge/app_ro/hmi/touch-app; do
  LD_LIBRARY_PATH="$DLLP" LD_PRELOAD=/tmp/kipc_bus.so "$d" >/tmp/$(basename "$d").log 2>&1 &
done
/tmp/dmstub /run/kipc_hub.sock 1920 720 >/tmp/dmstub.log 2>&1 &
sleep 4

CP=$(ls /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar /vw/hmi/jar/*.jar 2>/dev/null | tr '\n' ':')
B=/vw/hmi
export EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH="/opt/fakegl:/usr/lib/aarch64-linux-gnu:/lge/hmi/libs:/vw/hmi/lib:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/lge/hmi/jvm/lib/aarch64/server:/lge/app_rw/smartlink/dcmanager:/fwusr:/fwlib"
export LD_PRELOAD="/tmp/kipc_bus.so"
echo "[*] HMI (80s)"
timeout 80 /lge/hmi/jvm/bin/java \
  -Dde.vw.mib.installation.directory=$B -Dde.vw.mib.configuration.directory=$B/config \
  -Dde.vw.mib.resources.directory=$B/Resources -Dvw.mib.resources=$B/res \
  -Dbundles.properties=$B/config/bundles.properties -Dsystem.properties=$B/config/system.properties \
  -Dlogging.properties=$B/config/logging.properties -Dthreads.properties=$B/config/threads.properties \
  -Djava.library.path=/lge/hmi/libs -Dsun.boot.library.path=/lge/hmi/jvm/lib/aarch64:/lge/hmi/libs \
  -Dde.vw.mib.graphics.gles.v3 -Dviewhandler.format=jar -Dfile.encoding=UTF-8 \
  -XX:ErrorFile=/tmp/hs_err.log \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain > /out/hmi13_out 2>&1 || true
echo "  linhas=$(wc -l < /out/hmi13_out)"
cp /tmp/hs_err.log /out/hs_err13.log 2>/dev/null || true
echo "=== até onde / crash ==="
grep -anE "Starting .* bundles|UI thread|OpenGL ES [0-9]|llvmpipe|Error while repainting|queryFunctionality|ImageLoader|enterView|RepaintManager|Segmentation|UnsatisfiedLink|Exception in thread" /out/hmi13_out 2>/dev/null | grep -aviE "^\s*at " | head -20
echo "--- frame do crash (hs_err) ---"; grep -aE "Problematic frame|^C  \[|^V  \[|siginfo" /tmp/hs_err.log 2>/dev/null | head -8
echo "--- últimas 4 ---"; tail -4 /out/hmi13_out
