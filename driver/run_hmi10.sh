#!/bin/sh
# Ensemble com provedor fake de DisplayManagement (dmstub) pra destravar getDisplayExtents.
apt-get -qq update >/dev/null 2>&1; apt-get -qq install -y gcc >/dev/null 2>&1
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c || exit 1
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c || exit 1
gcc -static -O2 -o /tmp/dmstub /driver/dmstub.c || exit 1
echo "[build] hub + bus.so + dmstub OK"
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm 2>/dev/null
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
mkdir -p $LOW/dev/dri; mknod $LOW/dev/dri/card0 c 226 0 2>/dev/null; mknod $LOW/dev/dri/renderD128 c 226 128 2>/dev/null; chmod 666 $LOW/dev/dri/* 2>/dev/null
mount --bind /scratch/vwroot $LOW/vw 2>/dev/null
mkdir -p $LOW/var/lib/hmi $LOW/var/lib/nav/coredump $LOW/var/cache/hmi $LOW/media/mp000 $LOW/run/platform/weston 2>/dev/null
mkdir -p $LOW/data/lge/hmi $LOW/data/lge/persistency 2>/dev/null; : > $LOW/data/lge/hmi/lsmval 2>/dev/null
cp /tmp/kipc_hub $LOW/tmp/; cp /tmp/kipc_bus.so $LOW/tmp/; cp /tmp/dmstub $LOW/tmp/

echo "[*] sobe o hub"
chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
sleep 1
DLLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif"
echo "[*] sobe daemons DSI reais (persistence, mcu, carif, touch)"
for d in /lge/app_ro/hmi/persistence-app /lge/app_ro/hmi/mcu-interpreter /lge/app_ro/carif/carifservice /lge/app_ro/hmi/touch-app; do
  chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$DLLP LD_PRELOAD=/tmp/kipc_bus.so; exec $d" >$LOW/tmp/$(basename $d).log 2>&1 &
done
echo "[*] sobe o dmstub (provedor fake de DisplayManagement)"
chroot $LOW /tmp/dmstub /run/kipc_hub.sock 1920 720 >$LOW/tmp/dmstub.log 2>&1 &
sleep 4
echo "[*] lanca a HMI (70s)"
cat > $LOW/tmp/launch9.sh <<'EOS'
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
  -Dviewhandler.format=jar -Dfile.encoding=UTF-8 \
  -XX:ErrorFile=/tmp/hs_err.log \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain
EOS
timeout 70 chroot $LOW /bin/sh /tmp/launch9.sh > /out/hmi10_out 2>&1
echo "  rc=$? linhas=$(wc -l < /out/hmi10_out)"
echo "=== DisplayManagement registrou? (sumiu da lista?) ==="
grep -aE "Required DSI services not reg|DisplayManagement|getDisplayExtents|DisplayExtents|startService.*Display" /out/hmi10_out | grep -avE "Waiting for" | head -15
echo "=== dmstub log ==="; head -20 $LOW/tmp/dmstub.log 2>/dev/null
