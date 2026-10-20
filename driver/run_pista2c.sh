#!/bin/sh
apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq gcc >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
mkdir -p /fwlibs
find $LOW/lge $LOW/usr $LOW/lib $LOW/lib64 -maxdepth 5 -name 'lib*.so*' 2>/dev/null | while read f; do cp -an "$f" /fwlibs/ 2>/dev/null; done
cd /fwlibs
rm -f libc.so* libc-*.so libm.so* libm-*.so libpthread.so* libpthread-*.so libdl.so* libdl-*.so \
      librt.so* librt-*.so libstdc++.so* libgcc_s.so* ld-linux* libresolv* libnss_* libnsl* \
      libanl* libutil.so* libutil-*.so libcrypt.so* libcrypt-*.so libBrokenLocale* 2>/dev/null
gcc -O0 -o /tmp/host /driver/host.c -ldl 2>/dev/null
gcc -shared -fPIC -o /tmp/stubs.so /driver/preload_stubs.c 2>/dev/null
echo "[*] rodando host (stubs preload)..."
LD_LIBRARY_PATH=/fwlibs LD_PRELOAD=/tmp/stubs.so timeout 30 /tmp/host
echo "  rc=$?"
