#!/bin/sh
apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq strace >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm 2>/dev/null
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null; chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom
# monta o /vw (particao HMI extraida)
mkdir -p $LOW/vw; mount --bind /scratch/vwroot $LOW/vw 2>/dev/null && echo "[*] /vw montado ($(ls $LOW/vw | wc -l) itens)"
# dirs que a HMI espera
mkdir -p $LOW/var/lib/hmi $LOW/var/lib/nav/coredump $LOW/var/cache/hmi $LOW/media/mp000 $LOW/media/mp001 $LOW/run/platform/weston $LOW/etc/gem/esd 2>/dev/null
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
echo "[*] java -version (a JVM roda?):"
chroot $LOW /lge/hmi/jvm/bin/java -version 2>&1 | head -4
echo; echo "[*] LANÇANDO a HMI (runHMI.sh) — timeout 30s, capturando ledger..."
cat > $LOW/tmp/launch.sh <<'EOS'
export XDG_RUNTIME_DIR=/run/platform/weston
export VMOPTIONS="-Dde.vw.mib.view.drawpointer=true"
export LD_PRELOAD_FORHMI=/lge/hmi/jvm/lib/aarch64/libjsig.so:/lge/hmi/libs/libImageLoader.so:/lge/hmi/libs/libDebugSPINative.so
cd /vw/hmi
exec /bin/sh /vw/hmi/runHMI.sh
EOS
timeout 30 chroot $LOW /bin/sh /tmp/launch.sh > /out/hmi_out 2>&1
echo "  rc=$?"
echo "[*] primeiras linhas da HMI:"; head -30 /out/hmi_out
echo "..."; echo "[*] ULTIMAS linhas (onde parou/falhou):"; tail -20 /out/hmi_out
