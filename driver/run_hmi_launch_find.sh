#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
for f in /etc/init.d/jdkenv.sh /etc/smack/conf/hmi.sh /etc/early_init.conf; do
  echo "===== $f ====="; cat "$LOW$f" 2>/dev/null | head -50; echo
done
echo "===== procurando a invocacao do java/HMI em todo /etc e /lge ====="
grep -rslI "bin/java\|MIBHMI\|\-cp \|classpath\|MainClass\|hmi.*main\|StartHMI\|smartlink" $LOW/etc $LOW/lge 2>/dev/null | sed "s#$LOW##" | grep -viE "\.jar$|\.so$" | head -15
echo; echo "===== systemd units citando hmi ====="
grep -rslI "hmi\|HMI" $LOW/lib/systemd/system $LOW/etc/systemd 2>/dev/null | sed "s#$LOW##" | head
echo; echo "===== a classe main dentro do jar (MANIFEST) ====="
mkdir -p /tmp/j; cd /tmp/j; unzip -p "$LOW/lge/hmi/jar/MIBHMI-DSI.jar" META-INF/MANIFEST.MF 2>/dev/null | head -20
