#!/bin/sh
apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq strace gcc socat >/dev/null 2>&1
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
echo "[*] compilando kipc_shim.so (sem libc)..."
gcc -shared -fPIC -nostdlib -ffreestanding -fno-stack-protector -o $LOW/tmp/kipc_shim.so /driver/kipc_shim.c 2>/tmp/gcc && echo "  build OK" || { echo "  BUILD FAIL:"; cat /tmp/gcc; }
echo "[*] rodando sdcm com LD_PRELOAD do shim (timeout 25s)..."
timeout 25 strace -f -ff -e trace=%file,%desc,%network,%signal -s 300 -o /out/s3_trace \
  chroot $LOW /usr/bin/env LD_PRELOAD=/tmp/kipc_shim.so /lge/app_rw/smartlink/dcmanager/sdcm >/out/s3_stdout 2>/out/s3_stderr
echo "  rc=$?"
echo; echo "[*] MsgPkts kipc capturados pelo shim (o wire do boot):"
grep -aE "kipc (msgpkt|ioctl)" /out/s3_stderr 2>/dev/null | head -25
echo; echo "[*] stderr (fim):"; grep -avE "kipc " /out/s3_stderr 2>/dev/null | tail -6
echo; echo "[*] o que o binario faz DEPOIS do ioctl passar (DSI/socket/write):"
M=$(ls -S /out/s3_trace* 2>/dev/null | head -1)
grep -aE "socket\(|connect\(|/dev/css|/dev/csr|write\(1|write\(2|getpid|clone|openat.*dsi" "$M" 2>/dev/null | tail -20
echo; echo "[*] tail do trace principal:"; tail -12 "$M"
