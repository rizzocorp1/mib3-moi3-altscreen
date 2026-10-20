#!/bin/sh
set -e
apt-get -qq update >/dev/null 2>&1
apt-get -qq install -y binutils unzip file >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
echo "=== 1) jar da camada DSI: loadLibrary + nomes de lib nativa ==="
mkdir -p /tmp/j
for f in $(ls $LOW/lge/hmi/jar/*DSI*.jar $LOW/lge/hmi/jar/*dsi*.jar $LOW/vw/hmi/jar/*DSI*.jar 2>/dev/null); do echo " jar: $(basename $f)"; done
f=$(ls $LOW/lge/hmi/jar/MIBHMI-DSI*.jar 2>/dev/null | head -1)
[ -n "$f" ] && unzip -oq "$f" -d /tmp/j
echo "--- strings de interesse nos .class (loadLibrary / libnativa / Native) ---"
grep -rhaoE "loadLibrary|lib[A-Za-z0-9_]+Native|lib[A-Za-z0-9_]+\.so|NativeDsi|DsiNative|dsipal|servicebroker|ServiceBroker" /tmp/j 2>/dev/null | sort | uniq -c | sort -rn | head -25
echo
echo "=== 2) libs nativas 'DSI/dsi/pal/broker' no firmware ==="
ls $LOW/lge/hmi/libs/ | grep -iE "dsi|pal|broker|ipc|service" | head -30
echo "--- NEEDED do carifservice (readelf real) ---"
readelf -d $LOW/lge/app_ro/carif/carifservice 2>/dev/null | grep NEEDED | head -30
echo
echo "=== 3) o que libEGL.so.1 / libGLESv2.so.2 REALMENTE são ==="
file $LOW/usr/lib/libEGL.so.1 $LOW/usr/lib/libGLESv2.so.2 2>/dev/null
ls -la $LOW/usr/lib/libEGL.so.1 $LOW/usr/lib/libGLESv2.so.2 2>/dev/null
echo "--- libjopengl_es3.so: NEEDED + se exige kgsl/adreno ---"
readelf -d $LOW/lge/hmi/libs/libjopengl_es3.so 2>/dev/null | grep NEEDED
echo "--- strings de device GPU que o stack exige ---"
strings $LOW/usr/lib/libEGL_adreno.so 2>/dev/null | grep -iE "/dev/kgsl|/dev/dri|/dev/ion|gbm|gralloc|surfaceless" | sort -u | head
