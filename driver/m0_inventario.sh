#!/bin/sh
# Marco 0 (sessão de modelagem do SoC, 11/10/2026): inventário SÓ-LEITURA do system.img do MOI3.
# Roda DENTRO de um container ubuntu puro (não instala nada, não usa rede): loop-mount
# ro,noload do system.img e despeja em /out
#   system_listing.tsv  tipo, modo, dono, tamanho, caminho, alvo do symlink (árvore inteira)
#   system_heads.bin    os 20 primeiros bytes de cada arquivo regular (censo de ELF no host)
#   subset.tar          subconjunto gráfico/boot/systemd pra análise estática no host
# O volume do firmware entra :ro, então a imagem original não tem como ser alterada.
# Uso (host):
#   docker run --rm --privileged --network none \
#     -v <scratch-do-firmware>:/scratch:ro -v <saida>:/out -v <driver>:/driver:ro \
#     ubuntu:24.04 sh /driver/m0_inventario.sh /scratch/newfw/system_new.img
IMG=${1:-/scratch/newfw/system_new.img}
LOW=/mnt/low; mkdir -p $LOW /out
mount -o loop,ro,noload "$IMG" $LOW 2>/tmp/m.err || { echo "MOUNTFAIL: $(cat /tmp/m.err)"; exit 1; }
echo "[mount] $(grep " $LOW " /proc/mounts)"
echo "[df]    $(df -h $LOW | tail -1)"

echo "[1/3] listagem completa"
find $LOW -xdev -printf '%y\t%m\t%U:%G\t%s\t%P\t%l\n' > /out/system_listing.tsv
echo "  entradas=$(wc -l < /out/system_listing.tsv)"

echo "[2/3] cabeças (20 bytes) de cada arquivo regular"
find $LOW -xdev -type f -size +19c -print0 | xargs -0 head -v -c 20 > /out/system_heads.bin
echo "  bytes=$(wc -c < /out/system_heads.bin)"

echo "[3/3] subconjunto gráfico/boot/systemd"
cd $LOW || exit 1
{
  find . -xdev \( -type f -o -type l \) \( \
       -name '*adreno*' -o -name '*gsl*' -o -name 'eglSubDriver*' -o -name 'libEGL*' \
    -o -name 'libGLES*' -o -name 'libgbm*' -o -name 'libdrm*' -o -name 'libwayland*' \
    -o -name 'libweston*' -o -name 'libllvm*' -o -name 'libC2D2*' -o -name 'libCB*' \
    -o -name 'libOpenCL*' -o -name 'libq3d*' -o -name 'libjopengl*' -o -name 'libImageLoader*' \
    -o -name 'libivi*' -o -name 'libilm*' -o -name 'weston*' -o -name 'ld-*.so' \
    -o -name 'libc-*.so' -o -name 'libc.so.6' -o -name 'ld-linux-aarch64.so.1' \) -print
  for d in usr/lib/weston usr/lib64/weston etc/xdg lib/systemd/system etc/systemd lib/udev etc/udev \
           lge/hmi/libs lge/app_ro/graphics etc/modules-load.d etc/modprobe.d etc/tmpfiles.d \
           usr/lib/tmpfiles.d etc/smack etc/profile.d; do
    [ -e "$d" ] && find "$d" -xdev \( -type f -o -type l \) -print
  done
  for f in etc/os-release etc/version etc/fstab etc/passwd etc/group etc/ld.so.conf etc/profile \
           etc/environment etc/early_init.conf etc/prework_systemd.sh etc/machine-id; do
    [ -e "$f" ] && echo "./$f"
  done
} | sed 's|^\./||' | sort -u > /tmp/subset.lst
echo "  arquivos=$(wc -l < /tmp/subset.lst)"
tar -cf /out/subset.tar --numeric-owner -T /tmp/subset.lst 2>/tmp/tar.err
echo "  subset.tar=$(wc -c < /out/subset.tar) bytes; avisos tar=$(wc -l < /tmp/tar.err)"
head -3 /tmp/tar.err
cp /tmp/subset.lst /out/subset.lst
cd /; umount $LOW
echo "[ok] fim"
