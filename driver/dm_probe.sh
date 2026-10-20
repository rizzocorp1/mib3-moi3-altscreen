#!/bin/sh
apt-get -qq update >/dev/null 2>&1; apt-get -qq install -y gcc strace >/dev/null 2>&1
gcc -static -O2 -o /tmp/kipc_hub /driver/kipc_hub.c 2>/dev/null
gcc -O2 -ffreestanding -fno-builtin -fno-stack-protector -shared -fPIC -o /tmp/kipc_bus.so /driver/kipc_bus.c 2>/dev/null
LOW=/mnt/low; mkdir -p $LOW; mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
for n in null:1:3 zero:1:5 urandom:1:9; do mknod $LOW/dev/$(echo $n|cut -d: -f1) c $(echo $n|cut -d: -f2) $(echo $n|cut -d: -f3) 2>/dev/null; done
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
cp /tmp/kipc_hub $LOW/tmp/; cp /tmp/kipc_bus.so $LOW/tmp/; cp /usr/bin/strace $LOW/usr/bin/ 2>/dev/null
mkdir -p $LOW/run/platform/weston
echo "=== weston.ini (geometria configurada?) ==="
cat $LOW/etc/xdg/weston/weston.ini 2>/dev/null | head -40
echo
echo "=== rsi-display-management NEEDED ==="
chroot $LOW /bin/sh -c 'LD_LIBRARY_PATH=/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lge/app_ro/graphics/multi-display readelf -d /lge/app_ro/graphics/multi-display/rsi-display-management 2>/dev/null' | grep NEEDED | head -20
echo
chroot $LOW /tmp/kipc_hub 2>/tmp/h.log & sleep 1
echo "=== rsi-display-management sob strace (o que abre/precisa; 6s) ==="
LLP="/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/app_ro/graphics/multi-display:/lge/app_ro/graphics"
timeout 6 chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$LLP LD_PRELOAD=/tmp/kipc_bus.so XDG_RUNTIME_DIR=/run/platform/weston; exec strace -f -e trace=openat,connect,stat,access -s 50 /lge/app_ro/graphics/multi-display/rsi-display-management" 2>&1 | grep -aE "wayland|weston|/dev/(fb|dri|drm)|\.so.*ENOENT|ENOENT.*(conf|ini|display|screen)|WAYLAND|socket|connect\(" | head -30
echo "--- stdout do daemon (erros) ---"
timeout 6 chroot $LOW /bin/sh -c "export LD_LIBRARY_PATH=$LLP LD_PRELOAD=/tmp/kipc_bus.so XDG_RUNTIME_DIR=/run/platform/weston; exec /lge/app_ro/graphics/multi-display/rsi-display-management" 2>&1 | head -15
