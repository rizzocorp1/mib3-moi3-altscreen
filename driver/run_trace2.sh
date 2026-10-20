#!/bin/sh
apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq strace >/dev/null 2>&1
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/tmp/m || { echo MOUNTFAIL; exit 1; }
# camada gravavel: tmpfs nos paths que o binario escreve
for d in data persist tmp run dev; do mkdir -p $LOW/$d 2>/dev/null; mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
# /dev minimo
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm 2>/dev/null
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
# nos kipc: por ora arquivos regulares (open sucede -> revela o ioctl REAL)
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
# trace socket do libtraceif: um listener que so aceita e descarta
( socat UNIX-LISTEN:$LOW/tmp/trace_unix_socket,fork - >/dev/null 2>&1 & ) 2>/dev/null
# dirs de estado
mkdir -p $LOW/data/lge/dcm $LOW/data/persist 2>/dev/null
echo "[*] rodando sdcm sob strace (timeout 20s)..."
timeout 20 strace -f -ff -e trace=%file,%desc,%network,%signal -s 400 -o /out/s2_trace \
  chroot $LOW /lge/app_rw/smartlink/dcmanager/sdcm >/out/s2_stdout 2>/out/s2_stderr
echo "rc=$?"
echo "[*] stderr (abort?):"; tail -6 /out/s2_stderr
echo; echo "[*] IOCTL real nos fds do kipc (/dev/csr|css):"
grep -hE "ioctl\(" /out/s2_trace* 2>/dev/null | grep -vE "ioctl\(-1," | head -15
echo; echo "[*] aberturas de csr/css (sucederam agora?):"
grep -hE "/dev/(csr|css)" /out/s2_trace* 2>/dev/null | head -10
echo; echo "[*] ate onde foi (tail do trace principal):"
M=$(ls -S /out/s2_trace* 2>/dev/null | head -1); tail -16 "$M"
