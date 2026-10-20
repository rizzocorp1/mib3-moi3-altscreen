#!/bin/sh
apt-get -qq update >/dev/null 2>&1; apt-get -qq install -y strace >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
cp /usr/bin/strace $LOW/usr/bin/strace 2>/dev/null
LLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi"
echo "=== strace do persistence-app (ele registra serviço no kipc /dev/css?) ==="
timeout 8 chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$LLP; exec strace -f -e trace=openat,open,ioctl,write,read,connect,socket -s 60 /lge/app_ro/hmi/persistence-app" 2>&1 | grep -aE "csr|css|kipc|dsi|DSI|socket|connect|ioctl|register|/run/|\.sock|ENOENT.*persist" | head -40
echo "   rc=$?"
