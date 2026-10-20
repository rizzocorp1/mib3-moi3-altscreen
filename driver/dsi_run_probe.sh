#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
# LD_LIBRARY_PATH que cobre os dirs de lib do firmware
LLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/carif:/lge/app_ro/hmi"
echo "=========== EXECUTAR daemons DSI (raw, 8s cada) ==========="
for d in /lge/app_ro/hmi/persistence-app /lge/app_ro/hmi/mcu-interpreter /lge/app_ro/carif/carifservice; do
  echo; echo "####### $d #######"
  timeout 8 chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$LLP; exec $d" 2>&1 | head -25
  echo "   (rc efetivo acima; cortado em 25 linhas)"
done
