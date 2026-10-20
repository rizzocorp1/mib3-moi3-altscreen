#!/bin/sh
apt-get -qq update >/dev/null 2>&1; apt-get -qq install -y gcc >/dev/null 2>&1
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c || exit 1
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c || exit 1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm 2>/dev/null
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
mkdir -p $LOW/dev/dri; mknod $LOW/dev/dri/card0 c 226 0 2>/dev/null; mknod $LOW/dev/dri/renderD128 c 226 128 2>/dev/null; chmod 666 $LOW/dev/dri/* 2>/dev/null
mount --bind /scratch/vwroot $LOW/vw 2>/dev/null
mkdir -p $LOW/var/lib/hmi $LOW/var/cache/hmi $LOW/run/platform/weston 2>/dev/null
cp /tmp/kipc_hub $LOW/tmp/kipc_hub; cp /tmp/kipc_bus.so $LOW/tmp/kipc_bus.so
echo "[*] hub SEM daemons (HMI lê só o próprio eco)"
chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
sleep 1
cat > $LOW/tmp/launch5.sh <<'EOS'
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/lge/hmi/libs:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/lge/app_rw/smartlink/dcmanager:/usr/lib:/lib:/lib64
export LD_PRELOAD="/tmp/kipc_bus.so /lge/hmi/jvm/lib/aarch64/libjsig.so"
CP=$(ls /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar /vw/hmi/jar/*.jar 2>/dev/null | tr '\n' ':')
B=/vw/hmi
exec /lge/hmi/jvm/bin/java \
  -Dde.vw.mib.installation.directory=$B -Dde.vw.mib.configuration.directory=$B/config \
  -Dde.vw.mib.resources.directory=$B/Resources -Dvw.mib.resources=$B/res \
  -Dbundles.properties=$B/config/bundles.properties -Dsystem.properties=$B/config/system.properties \
  -Dlogging.properties=$B/config/logging.properties -Dthreads.properties=$B/config/threads.properties \
  -Djava.library.path=/lge/hmi/libs -Dsun.boot.library.path=/lge/hmi/jvm/lib/aarch64:/lge/hmi/libs \
  -Dviewhandler.format=jar -Dfile.encoding=UTF-8 -Dde.vw.mib.view.gfx.init.eager \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain
EOS
timeout 45 chroot $LOW /bin/sh /tmp/launch5.sh > /out/hmi5_out 2>&1
echo "  rc=$? linhas=$(wc -l < /out/hmi5_out)"
grep -aE "Required DSI|SIGSEGV|SIGABRT|Problematic|libjvm|libDSI|Starting .* bundles|UI thread|WorkUnit|Aborted" /out/hmi5_out | head -20
