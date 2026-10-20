#!/bin/sh
apt-get -qq update >/dev/null 2>&1; apt-get -qq install -y gcc >/dev/null 2>&1
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c || exit 1
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c || exit 1
echo "[build] hub + bus.so OK"
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
cp /tmp/kipc_hub $LOW/tmp/kipc_hub; cp /tmp/kipc_bus.so $LOW/tmp/kipc_bus.so

echo "[*] sobe o hub"
chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
sleep 1
# dados que a DSIPersistenceImpl abre (senao FileNotFound); /data e tmpfs gravavel
mkdir -p $LOW/data/lge/hmi $LOW/data/lge/persistency 2>/dev/null
: > $LOW/data/lge/hmi/lsmval 2>/dev/null
DLLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif:/lge/app_ro/graphics:/lge/app_ro/graphics/multi-display"
echo "[*] sobe os 3 daemons DSI sob o barramento"
for d in /lge/app_ro/hmi/persistence-app /lge/app_ro/hmi/mcu-interpreter /lge/app_ro/carif/carifservice /lge/app_ro/hmi/touch-app /lge/app_ro/graphics/multi-display/multi-screen-server /lge/app_ro/graphics/multi-display/rsi-display-management; do
  chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$DLLP LD_PRELOAD=/tmp/kipc_bus.so; exec $d" >$LOW/tmp/$(basename $d).log 2>&1 &
done
sleep 5
echo "[*] daemons vivos:"; chroot $LOW /bin/sh -c 'ls /proc | grep -E "^[0-9]+$" | wc -l' 
echo "[*] lanca a HMI no mesmo barramento (60s)"
cat > $LOW/tmp/launch4.sh <<'EOS'
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
  -XX:ErrorFile=/tmp/hs_err.log \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain
EOS
timeout 60 chroot $LOW /bin/sh /tmp/launch4.sh > /out/hmi8_out 2>&1
echo "  rc=$? linhas=$(wc -l < /out/hmi8_out)"
cp $LOW/tmp/hs_err.log /out/hs_err.log 2>/dev/null
echo "=== hs_err: stack NATIVO do SIGSEGV (quem chamou libjvm+0x675118) ==="
grep -aE "^(C|J|j|V|v)  |Problematic|siginfo|Stack:|Java frames|libDSI|libjvm|RegisterNatives|ByteArray" $LOW/tmp/hs_err.log 2>/dev/null | head -30
echo
echo "=== HMI: os serviços DSI registraram? (vs. timeout) ==="
grep -aE "Required DSI|not registered|DSIProxyException|service.*regist|registered service|DSI.*ready|Starting .* bundles|UI thread|WorkUnit" /out/hmi8_out | head -30
echo
echo "=== HUB: clientes e difusão ==="
grep -aE "cliente|reentregou|msg#" $LOW/tmp/hub.log | head -25
echo "=== daemons: primeiras linhas ==="
for d in persistence-app mcu-interpreter carifservice; do echo "--- $d ---"; head -4 $LOW/tmp/$d.log 2>/dev/null; done
