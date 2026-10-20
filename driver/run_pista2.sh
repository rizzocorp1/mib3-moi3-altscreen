#!/bin/sh
apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq gcc >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
mkdir -p /fwlibs
echo "[*] copiando TODAS as libs do firmware..."
find $LOW/lge $LOW/usr $LOW/lib $LOW/lib64 -maxdepth 5 -name 'lib*.so*' 2>/dev/null | while read f; do cp -an "$f" /fwlibs/ 2>/dev/null; done
echo "[*] removendo familia glibc/libstdc++ (resolve na do container)..."
cd /fwlibs
rm -f libc.so* libc-*.so libm.so* libm-*.so libpthread.so* libpthread-*.so libdl.so* libdl-*.so \
      librt.so* librt-*.so libstdc++.so* libgcc_s.so* ld-linux* libresolv* libnss_* libnsl* \
      libanl* libutil.so* libutil-*.so libcrypt.so* libcrypt-*.so libBrokenLocale* 2>/dev/null
echo "    /fwlibs agora: $(ls | wc -l) libs"
echo "[*] compilando host..."
gcc -O0 -o /tmp/host /driver/host.c -ldl 2>/tmp/gcc && echo "  build OK" || { echo BUILDFAIL; cat /tmp/gcc; exit 1; }
echo "[*] rodando host..."
LD_LIBRARY_PATH=/fwlibs timeout 30 /tmp/host
echo "  rc=$?"
