#!/bin/sh
# Marco 1 (sessão de modelagem do SoC, 11/10/2026): sobe o userspace do MOI3 numa máquina
# genérica  qemu-system-aarch64 -M virt  com kernel mainline (o genérico do Ubuntu 24.04).
# Roda DENTRO da imagem moi3-bancada:1. O OrbStack não expõe /dev/kvm, então é TCG (lento).
# Uso (host):
#   docker run --rm --privileged --network none -v <sysroot/mu0883>:/fw:ro -v <trabalho>:/work \
#     -v <driver>:/driver:ro moi3-bancada:1 sh /driver/m1_qemu.sh [variante] [secs] [tag] [modo]
# Saída em /work/out/<tag>.* e o console do QEMU em /work/out/<tag>.qemu.log
set -e
VARIANT=${1:-manual}; SECS=${2:-180}; TAG=${3:-qemu_$VARIANT}; MODE=${4:-hmi}
K=$(ls /boot/vmlinuz-*-generic | head -1)
mkdir -p /work/out
[ -x /work/bin/kipc_hub ] || sh /driver/m1_build.sh /work/bin >/work/build_tools.log 2>&1

# /vw vira uma imagem ext4 (o guest não enxerga pastas do host); feita uma vez
if [ ! -f /work/vw.img ]; then
  echo "[m1_qemu] criando vw.img a partir de /fw/vw ..."
  mke2fs -q -t ext4 -L vw -d /fw/vw /work/vw.img 900M
fi
rm -f /work/out.img; mke2fs -q -t ext4 -L out /work/out.img 64M

# initramfs: busybox estático + as ferramentas + o lançador
R=/tmp/initrd; rm -rf $R; mkdir -p $R/bin $R/sandbox
cp /bin/busybox $R/bin/busybox; ln -s busybox $R/bin/sh
cp -r /work/bin $R/sandbox/bin
cp /driver/m1_hmi.sh $R/sandbox/m1_hmi.sh
cp /driver/m1_init.sh $R/init; chmod 755 $R/init
( cd $R && find . | cpio -o -H newc 2>/dev/null | gzip -1 > /work/initrd.gz )
echo "[m1_qemu] kernel=$K initrd=$(wc -c < /work/initrd.gz) bytes | variante=$VARIANT secs=$SECS tag=$TAG modo=$MODE"

T0=$(date +%s)
timeout $((SECS + 420)) qemu-system-aarch64 -M virt -cpu cortex-a57 -smp 4 -m 4096 \
  -nographic -no-reboot -nic none -kernel "$K" -initrd /work/initrd.gz \
  -append "console=ttyAMA0 panic=-1 loglevel=4 m1.variant=$VARIANT m1.secs=$SECS m1.tag=$TAG m1.mode=$MODE $KARGS" \
  -drive if=none,id=d0,file=/fw/system.img,format=raw,readonly=on -device virtio-blk-device,drive=d0,serial=sys \
  -drive if=none,id=d1,file=/work/vw.img,format=raw,readonly=on -device virtio-blk-device,drive=d1,serial=vw \
  -drive if=none,id=d2,file=/work/out.img,format=raw -device virtio-blk-device,drive=d2,serial=out \
  > /work/out/$TAG.qemu.log 2>&1 || echo "[m1_qemu] qemu saiu com rc=$?"
echo "[m1_qemu] QEMU terminou em $(( $(date +%s) - T0 ))s"

mkdir -p /mnt/out && mount -o loop /work/out.img /mnt/out && cp /mnt/out/* /work/out/ 2>/dev/null; umount /mnt/out 2>/dev/null || true
grep -a "^\[init\]\|^\[m1_hmi\]\|^    \|org.dsi.ifc" /work/out/$TAG.qemu.log | cut -c1-300
