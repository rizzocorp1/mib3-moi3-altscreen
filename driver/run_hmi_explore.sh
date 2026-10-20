#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
echo "=== /lge/hmi (arvore) ==="
find $LOW/lge/hmi -maxdepth 2 2>/dev/null | sed "s#$LOW##" | head -40
echo; echo "=== binario java na jvm ==="
JV=$(find $LOW/lge/hmi/jvm -maxdepth 3 -name java -type f 2>/dev/null | head -1); echo "java = ${JV#$LOW}"
[ -n "$JV" ] && file "$JV"
echo; echo "=== quem LANCA a HMI? (units/scripts citando MIBHMI/hmi/jvm) ==="
grep -rslI "MIBHMI\|hmi/jvm\|lge/hmi\|dalvikvm\|j9\|bin/java" $LOW/etc $LOW/lge/hmi $LOW/usr/bin $LOW/sbin 2>/dev/null | sed "s#$LOW##" | head -15
echo; echo "=== scripts .sh dentro de /lge/hmi ==="
find $LOW/lge/hmi -maxdepth 3 -name '*.sh' 2>/dev/null | sed "s#$LOW##" | head
echo; echo "=== conteudo do 1o script de launch achado ==="
S=$(find $LOW/lge/hmi -maxdepth 3 -name '*.sh' 2>/dev/null | head -1)
[ -n "$S" ] && { echo "--- ${S#$LOW} ---"; head -30 "$S"; }
echo; echo "=== jars ==="; ls -la $LOW/lge/hmi/jar 2>/dev/null | awk '{print $5, $9}' | head
