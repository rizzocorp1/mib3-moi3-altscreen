#!/bin/bash
# HMI sob glibc 2.39 (forward-compat) + Mesa llvmpipe REAL (GLES 3.2 por software) no lugar do
# Adreno. Sem chroot: dirs do firmware bindados na raiz do container; glibc/libstdc++/Mesa são do
# container (24.04). Objetivo: contexto gráfico ligar de verdade e renderizar por software.
set -e
apt-get -qq update >/dev/null 2>&1
apt-get -qq install -y mesa-utils-extra libgles2 libegl1 libgl1-mesa-dri gcc >/dev/null 2>&1
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c
gcc -static -O2 -o /tmp/dmstub /driver/dmstub.c
echo "[build] kipc + dmstub OK; Mesa: $(EGL_PLATFORM=surfaceless eglinfo 2>/dev/null | grep -i 'ES profile version' | head -1)"
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW
# raiz híbrida: conteúdo do firmware em /lge e /vw; glibc/Mesa do container em /usr,/lib
mkdir -p /lge /vw
mount --bind $LOW/lge /lge
mount --bind /scratch/vwroot /vw
mkdir -p /fwusr /fwlib
mount --bind $LOW/usr/lib64 /fwusr
mount --bind $LOW/lib64 /fwlib
mkdir -p /data/lge/hmi /data/lge/persistency /persist /media/mp000 /run/platform/weston \
         /var/lib/hmi /var/lib/nav/coredump /var/cache/hmi /etc/gem/esd 2>/dev/null || true
: > /data/lge/hmi/lsmval || true
: > /dev/csr 2>/dev/null || true; : > /dev/css 2>/dev/null || true; chmod 666 /dev/csr /dev/css 2>/dev/null || true
# Mesa como se fosse o Adreno (libjopengl é NEEDED desses sonames)
mkdir -p /opt/fakegl
MESADIR=/usr/lib/aarch64-linux-gnu
ln -sf $MESADIR/libEGL.so.1    /opt/fakegl/libEGL_adreno.so
ln -sf $MESADIR/libGLESv2.so.2 /opt/fakegl/libGLESv2_adreno.so

echo "[*] hub"
/tmp/kipc_hub 2>/tmp/hub.log &
sleep 1
DLLP="/usr/lib/aarch64-linux-gnu:/lge/app_ro/lib64:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif:/fwusr:/fwlib"
echo "[*] daemons DSI (sob glibc 2.39, sem chroot)"
for d in /lge/app_ro/hmi/persistence-app /lge/app_ro/hmi/mcu-interpreter /lge/app_ro/carif/carifservice /lge/app_ro/hmi/touch-app; do
  LD_LIBRARY_PATH="$DLLP" LD_PRELOAD=/tmp/kipc_bus.so "$d" >/tmp/$(basename "$d").log 2>&1 &
done
echo "[*] dmstub (DisplayManagement)"
/tmp/dmstub /run/kipc_hub.sock 1920 720 >/tmp/dmstub.log 2>&1 &
sleep 4

echo "[*] HMI com Mesa software (GLES 3.2), 80s"
CP=$(ls /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar /vw/hmi/jar/*.jar 2>/dev/null | tr '\n' ':')
B=/vw/hmi
export EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH="/opt/fakegl:/usr/lib/aarch64-linux-gnu:/lge/hmi/libs:/vw/hmi/lib:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/lge/hmi/jvm/lib/aarch64/server:/lge/app_rw/smartlink/dcmanager:/fwusr:/fwlib"
export LD_PRELOAD="/tmp/kipc_bus.so"
timeout 80 /lge/hmi/jvm/bin/java \
  -Dde.vw.mib.installation.directory=$B -Dde.vw.mib.configuration.directory=$B/config \
  -Dde.vw.mib.resources.directory=$B/Resources -Dvw.mib.resources=$B/res \
  -Dbundles.properties=$B/config/bundles.properties -Dsystem.properties=$B/config/system.properties \
  -Dlogging.properties=$B/config/logging.properties -Dthreads.properties=$B/config/threads.properties \
  -Djava.library.path=/lge/hmi/libs -Dsun.boot.library.path=/lge/hmi/jvm/lib/aarch64:/lge/hmi/libs \
  -Dde.vw.mib.graphics.gles.v3 -Dviewhandler.format=jar -Dfile.encoding=UTF-8 \
  -XX:ErrorFile=/tmp/hs_err.log \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain > /out/hmi12_out 2>&1 || true
cp /tmp/hs_err.log /out/hs_err12.log 2>/dev/null || true
echo "  linhas=$(wc -l < /out/hmi12_out)"
echo "=== frame do crash ==="; grep -aE "Problematic frame|siginfo|^C  \[|^J |^V  \[|^j  |library" /tmp/hs_err.log 2>/dev/null | head -15
echo "=== contexto gráfico ligou? (glGetString real / version / sem NPE / sem 'Error while repainting') ==="
grep -aoE "OpenGL ES [0-9.]+[^\"]*|llvmpipe|Error while repainting screen|NullPointerException|queryFunctionality|GLES.*version|renderer.*llvm" /out/hmi12_out 2>/dev/null | sort | uniq -c | sort -rn | head -15
echo "=== marcos de render/view ==="
grep -anE "enterView|RepaintManager|beginFrame|swapBuffers|First view|GLESGraphics3D|Error while repainting|version string|eglInitialize|Mesa" /out/hmi12_out 2>/dev/null | grep -aviE "^\s*at " | head -15
echo "--- últimas 6 ---"; tail -6 /out/hmi12_out
