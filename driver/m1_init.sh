#!/bin/sh
# /init do initramfs do Marco 1 (sessão de modelagem do SoC, 11/10/2026).
# Roda dentro do QEMU -M virt com o kernel genérico do Ubuntu: monta a partição system do
# firmware (disco virtio, só leitura), a partição /vw e um disco de saída, chama o MESMO
# lançador usado no container (m1_hmi.sh) e desliga. Nada do firmware é alterado.
# Parâmetros pela linha do kernel: m1.variant= m1.secs= m1.tag= m1.wait= m1.mode=
/bin/busybox mkdir -p /proc /sys /dev /mnt/low /out /tmp
/bin/busybox --install -s /bin
export PATH=/bin
mount -t proc proc /proc; mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null
VARIANT=manual; SECS=180; TAG=qemu; WAIT=20; MODE=hmi
for w in $(cat /proc/cmdline); do
  case $w in
    m1.variant=*) VARIANT=${w#*=};; m1.secs=*) SECS=${w#*=};; m1.tag=*) TAG=${w#*=};;
    m1.wait=*) WAIT=${w#*=};; m1.mode=*) MODE=${w#*=};;
  esac
done
echo "[init] $(uname -srm) | cmdline: $(cat /proc/cmdline)"
echo "[init] cpu: $(grep -m1 -E 'CPU part' /proc/cpuinfo) | nproc=$(grep -c ^processor /proc/cpuinfo) | mem=$(grep MemTotal /proc/meminfo)"
for b in /sys/block/vd*; do
  [ -e "$b" ] || continue
  s=$(cat $b/serial 2>/dev/null); n=${b##*/}
  case "$s" in sys) SYS=/dev/$n;; vw) VW=/dev/$n;; out) OUTD=/dev/$n;; esac
done
echo "[init] discos: system=$SYS vw=$VW saida=$OUTD"
fim() { echo "[init] $1"; sync; umount /out 2>/dev/null; poweroff -f; }
mount -t ext4 $OUTD /out || echo "[init] sem disco de saída"
mount -t ext4 -o ro,noload $SYS /mnt/low || fim "ERRO: não montou a partição system"
mount -t ext4 -o ro,noload $VW /mnt/low/vw || fim "ERRO: não montou a partição /vw"
echo "[init] system montada: $(cat /mnt/low/etc/version 2>/dev/null) | glibc do firmware: $(ls /mnt/low/lib64/libc-*.so)"

# T2: o userspace do firmware executa neste kernel/plataforma?
echo "[init] T2 busybox do firmware: $(chroot /mnt/low /bin/busybox 2>&1 | head -1)"
echo "[init] T2 java do firmware:"; chroot /mnt/low /lge/hmi/jvm/bin/java -version 2>&1 | sed 's/^/    /'
[ "$MODE" = t2 ] && fim "modo t2: fim"

# T3: o mesmo lançador do container
LOW=/mnt/low BIN=/sandbox/bin OUT=/out TAG=$TAG VARIANT=$VARIANT SECS=$SECS WAIT=$WAIT sh /sandbox/m1_hmi.sh
dmesg | tail -40 > /out/$TAG.dmesg
fim "fim do Marco 1 ($TAG)"
