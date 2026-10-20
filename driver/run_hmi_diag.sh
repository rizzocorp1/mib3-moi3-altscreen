#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
for d in tmp dev proc; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; chmod 666 $LOW/dev/null
echo "=== 1) o arquivo existe no chroot? ==="
ls -la $LOW/lge/hmi/libs/libDebugSPINative.so
echo "=== 2) java.library.path efetivo ==="
chroot $LOW /lge/hmi/jvm/bin/java -Djava.library.path=/lge/hmi/libs -XshowSettings:properties -version 2>&1 | grep -iA3 "library.path"
echo "=== 3) deps do libDebugSPINative.so (ldd no chroot) ==="
export LD_LIBRARY_PATH=/lge/hmi/libs:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64
chroot $LOW /bin/sh -c "LD_LIBRARY_PATH=$LD_LIBRARY_PATH ldd /lge/hmi/libs/libDebugSPINative.so 2>&1" | head -25
echo "=== 4) simbolo initSPINative presente? ==="
chroot $LOW /bin/sh -c "nm -D /lge/hmi/libs/libDebugSPINative.so 2>/dev/null | grep -i initSPINative" || /usr/bin/objdump -T $LOW/lge/hmi/libs/libDebugSPINative.so 2>/dev/null | grep -i "SPI\|JNI" | head
