#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm 2>/dev/null
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null; chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom
mkdir -p $LOW/dev/dri; mknod $LOW/dev/dri/card0 c 226 0 2>/dev/null; mknod $LOW/dev/dri/renderD128 c 226 128 2>/dev/null; chmod 666 $LOW/dev/dri/* 2>/dev/null
mount --bind /scratch/vwroot $LOW/vw 2>/dev/null
mkdir -p $LOW/var/lib/hmi $LOW/var/lib/nav/coredump $LOW/var/cache/hmi $LOW/media/mp000 $LOW/media/mp001 $LOW/run/platform/weston $LOW/etc/gem/esd 2>/dev/null
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
cat > $LOW/tmp/launch2.sh <<'EOS'
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/lge/hmi/libs:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/lge/app_rw/smartlink/dcmanager:/usr/lib:/lib:/lib64
export LD_PRELOAD=/lge/hmi/jvm/lib/aarch64/libjsig.so
CP=$(ls /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar /vw/hmi/jar/*.jar 2>/dev/null | tr '\n' ':')
B=/vw/hmi
exec /lge/hmi/jvm/bin/java \
  -Dde.vw.mib.installation.directory=$B -Dde.vw.mib.configuration.directory=$B/config \
  -Dde.vw.mib.resources.directory=$B/Resources -Dvw.mib.resources=$B/res \
  -Dbundles.properties=$B/config/bundles.properties -Dsystem.properties=$B/config/system.properties \
  -Dlogging.properties=$B/config/logging.properties -Dthreads.properties=$B/config/threads.properties \
  -Djava.library.path=/lge/hmi/libs -Dviewhandler.format=jar -Dfile.encoding=UTF-8 -Dde.vw.mib.view.gfx.init.eager \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain
EOS
echo "[*] lancando HMI sem shared-archive, com /dev/dri fake (timeout 35s)..."
timeout 35 chroot $LOW /bin/sh /tmp/launch2.sh > /out/hmi2_out 2>&1
echo "  rc=$?  linhas=$(wc -l < /out/hmi2_out)"
echo "[*] a HMI carregou bundles / foi alem do GPU? (procura de.vw.mib / bundle / DSI / Adreno):"
grep -aE "de\.vw\.mib|[Bb]undle|DSI|dsi|Adreno|GSL|card0|Exception|FATAL|MIBMain|startup|ViewHandler|register" /out/hmi2_out | head -35