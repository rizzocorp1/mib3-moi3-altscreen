#!/bin/sh
apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq gcc socat procps >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm 2>/dev/null
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
( socat UNIX-LISTEN:$LOW/tmp/trace_unix_socket,fork - >/dev/null 2>&1 & ) 2>/dev/null
mkdir -p $LOW/data/lge/dcm $LOW/data/persist 2>/dev/null
gcc -O0 -o /tmp/pb /driver/ptrace_broker.c 2>/dev/null && echo "[*] build OK"
/tmp/pb >/out/pb_out 2>&1 &
PB=$!
sleep 14
SD=$(pgrep -P $PB 2>/dev/null | head -1)
echo "[*] sdcm pid=$SD apos 14s — AINDA VIVO? estado=$(cat /proc/$SD/stat 2>/dev/null|awk '{print $3}')"
echo "[*] threads do sdcm (nomes) e o que fazem:"
for t in $(ls /proc/$SD/task 2>/dev/null); do
  nm=$(cat /proc/$SD/task/$t/comm 2>/dev/null); wc=$(cat /proc/$SD/task/$t/wchan 2>/dev/null)
  printf "    %-18s wchan=%s\n" "$nm" "$wc"
done | sort | uniq -c | sort -rn | head -20
sleep 3; kill $PB 2>/dev/null
echo "[*] pb_out:"; grep -aE "VIVO|exit|killed|tracer" /out/pb_out | tail -5
echo "[*] algum crash no periodo todo?"; grep -aiE "double free|abort|segmentation" /out/pb_out || echo "    NENHUM crash"
