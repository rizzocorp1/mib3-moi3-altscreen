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
gcc -O0 -o /tmp/pb /driver/ptrace_broker.c 2>/tmp/gcc && echo "[*] build OK" || { echo BUILDFAIL; cat /tmp/gcc; exit 1; }
echo "[*] rodando sdcm sob ptrace_broker (12s) ..."
/tmp/pb >/out/pb_out 2>&1 &
PB=$!
sleep 8
# achar o pid do sdcm (filho do pb) e ver em que syscall/estado ele esta
SD=$(pgrep -P $PB 2>/dev/null | head -1); [ -z "$SD" ] && SD=$(pgrep -x sdcm 2>/dev/null | head -1)
echo "[*] estado do sdcm (pid=$SD) apos 8s:"
if [ -n "$SD" ]; then
  echo "  state=$(cat /proc/$SD/stat 2>/dev/null | awk '{print $3}')  wchan=$(cat /proc/$SD/wchan 2>/dev/null)"
  echo "  ultima syscall:"; cat /proc/$SD/syscall 2>/dev/null
  echo "  threads: $(ls /proc/$SD/task 2>/dev/null | wc -l)"
fi
sleep 5; kill $PB 2>/dev/null
echo "[*] pb_out (startup + progresso):"; head -8 /out/pb_out
