#!/bin/sh
export DEBIAN_FRONTEND=noninteractive
echo "[*] instalando strace/file..."; apt-get update -qq >/dev/null 2>&1; apt-get install -y -qq strace file >/dev/null 2>&1
mkdir -p /mnt/low /mnt/up /mnt/work /mnt/merged /out
echo "[*] loop-mount ro do system.img..."
if mount -o loop,ro /scratch/newfw/system_new.img /mnt/low 2>/tmp/m1; then echo "  mount OK"; else echo "  MOUNT FAIL: $(cat /tmp/m1)"; exit 1; fi
if mount -t overlay overlay -o lowerdir=/mnt/low,upperdir=/mnt/up,workdir=/mnt/work /mnt/merged 2>/tmp/m2; then ROOT=/mnt/merged; echo "  overlay OK"; else ROOT=/mnt/low; echo "  overlay falhou ($(cat /tmp/m2)); usando low ro"; fi
mkdir -p $ROOT/proc $ROOT/sys $ROOT/dev $ROOT/tmp $ROOT/persist/sharedmemory $ROOT/data /out 2>/dev/null
mount -t proc proc $ROOT/proc 2>/dev/null; mount --bind /dev $ROOT/dev 2>/dev/null
echo "[*] file sdcm:"; file $ROOT/lge/app_rw/smartlink/dcmanager/sdcm
echo "[*] trace (deixa morrer, timeout 20s)..."
timeout 20 strace -f -ff -e trace=%file,%network,%signal,%desc -s 1024 -o /out/sdcm_trace \
  chroot $ROOT /lge/app_rw/smartlink/dcmanager/sdcm >/out/sdcm_stdout 2>/out/sdcm_stderr
echo "  rc=$?"
echo "[*] traces:"; ls -la /out/ | grep sdcm_trace | head
echo "[*] PRIMEIRAS falhas interessantes (ENOENT/ECONNREFUSED/open de nos):"
cat /out/sdcm_trace* 2>/dev/null | grep -iE "ENOENT|ECONNREFUSED|ENODEV|ENXIO|No such|kipc|/dev/|/persist/|socket\(|connect\(" | head -40
