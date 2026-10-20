#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
echo "===== vw-hmi.service ====="; cat "$LOW/lib/systemd/system/vw-hmi.service" 2>/dev/null
echo; echo "===== ExecStart script(s) referenciados ====="
for s in $(grep -hoE "Exec[A-Za-z]*=[^ ]*\.sh|/[^ ]*hmi[^ ]*\.sh|/usr/bin/[a-z_]*hmi" "$LOW/lib/systemd/system/vw-hmi.service" 2>/dev/null | grep -oE "/[^ =]+"); do
  echo "--- $s ---"; cat "$LOW$s" 2>/dev/null | head -60; echo
done
echo "===== procurar o comando java/startscreen em /lge e /usr ====="
grep -rhI "bin/java\|java.library.path\|MIBHMI-DSI\|de.vw\|com.lge\|startscreen\|MainController\|HmiMain" $LOW/usr/bin $LOW/lge 2>/dev/null | grep -i java | head -10
