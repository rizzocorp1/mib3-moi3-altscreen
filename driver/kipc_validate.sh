#!/bin/sh
apt-get -qq update >/dev/null 2>&1; apt-get -qq install -y gcc strace >/dev/null 2>&1
echo "=== build ==="
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c 2>&1 && echo "hub OK"
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c 2>&1 && echo "bus.so OK"
echo "   deps do bus.so (não pode exigir GLIBC_2.34):"
readelf -V /tmp/kipc_bus.so 2>/dev/null | grep -iE "GLIBC_2.3[4-9]" && echo "   !! ainda pede glibc nova" || echo "   ok, sem glibc 2.34+"
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
cp /tmp/kipc_hub $LOW/tmp/kipc_hub; cp /tmp/kipc_bus.so $LOW/tmp/kipc_bus.so
cp /usr/bin/strace $LOW/usr/bin/strace 2>/dev/null
echo "=== sobe o hub dentro do chroot ==="
chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
HUBPID=$!; sleep 1
LLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi"
echo "=== persistence-app sob o kipc_bus (8s) ==="
timeout 8 chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$LLP LD_PRELOAD=/tmp/kipc_bus.so; exec strace -f -e trace=openat,socket,connect,sendto,recvfrom,ioctl,write -s 30 /lge/app_ro/hmi/persistence-app" 2>&1 | grep -aE "kipc_bus|/dev/cs|socket\(AF_UNIX|connect\(|sendto|recvfrom|ENOTTY" | head -30
kill $HUBPID 2>/dev/null
echo "=== log do hub (registrou clientes? difundiu msgs?) ==="
cat $LOW/tmp/hub.log 2>/dev/null | head -30
