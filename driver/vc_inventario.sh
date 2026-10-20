#!/bin/sh
# vc_inventario.sh — inventário SÓ-LEITURA do que o firmware traz para a navegação e para o vídeo
# do painel de instrumentos (investigação de 11/10/2026). Roda DENTRO da imagem moi3-bancada:1,
# sem rede; monta a partição system só para leitura e não executa nada do firmware.
#   docker run --rm --privileged --network none -v <sysroot/mu0883>:/fw:ro -v <saída>:/out \
#     -v <driver>:/driver:ro moi3-bancada:1 sh /driver/vc_inventario.sh
# Saídas em /out (prefixo vc_inv_): unidades systemd, roteiros de partida, dependências e
# símbolos importados, e as strings dos dois programas de vídeo.
L=/mnt/low; O=/out; mkdir -p $L $O
mount -o loop,ro,noload /fw/system.img $L || { echo "não montou"; exit 1; }
cd $L || exit 1
N=lge/app_ro/navi; G=lge/app_ro/graphics; P=$N/naviengine/package
{ for u in navi-al naviengine navsd simplenav multi-display rsi-display-management gnssmgr-csr displaymgr weston post-init; do
    echo "===== lib/systemd/system/$u.service"; cat lib/systemd/system/$u.service; echo; done
  echo "===== onde são habilitadas"
  find etc/systemd lib/systemd -type l \( -name 'navi*' -o -name 'navsd*' -o -name 'multi-display*' -o -name 'rsi-display*' \
       -o -name 'gnssmgr*' -o -name 'simplenav*' \) -printf '%p -> %l\n'
  echo "===== etc/fstab"; cat etc/fstab; } > $O/vc_inv_unidades.txt
{ for f in $N/navi-al/runNavi-al.sh $N/navi-al/prework_navi-al.sh $N/naviengine/runNavi.sh $N/naviengine/nav.prio.cfg \
           $N/gnssmgr-csr/runGnssmgr-csr.sh lge/app_ro/post_init/data/navigation.sh; do
    echo "===== $f"; cat $f; echo; done
  for v in eu/single eu/dual nar/single nar/dual; do echo "===== $P/$v/navi_version: $(cat $P/$v/navi_version)"; done
  echo "===== o binário é o mesmo nas quatro variantes?"; md5sum $P/*/*/usr/bin/tsd.nav.mainapp.mib3
  echo "===== standardConfig.cfg"; md5sum $P/*/*/etc/nav/standardConfig.cfg; cat $P/eu/single/etc/nav/standardConfig.md5; echo
  echo "===== tamanho de cada pacote"; du -sh $P/*/*; } > $O/vc_inv_roteiros.txt
B=$P/eu/single/usr/bin/tsd.nav.mainapp.mib3
{ for b in $B $N/navi-al/alm $G/multi-display/multi-screen-server $G/multi-display/rsi-display-management \
           $G/displaymgr/bin/displaymgr lge/app_ro/carif/navsd_app lge/app_ro/lib64/libMIBNavSDClientLib_CarIF.so.00 \
           usr/lib64/weston/ivi-controller.so usr/lib64/libwaylandegl.so; do
    echo "===== $b ($(stat -c %s $b) bytes) NEEDED"; readelf -d $b | sed -n 's/.*Shared library: \[\(.*\)\]/  \1/p'; done
  echo "===== quem cita ivi_share"; grep -l -r ivi_share lge/app_ro usr/bin usr/lib64 lge/app_rw 2>/dev/null; } > $O/vc_inv_dependencias.txt
und() { readelf -W --dyn-syms "$1" | awk '$7=="UND" {print $8}' | sort; }
{ echo "===== motor: EGL"; und $B | grep '^egl' | tr '\n' ' '; echo
  echo "===== motor: Wayland e IVI"; und $B | grep -E '^(wl_|ivi|ilm)' | tr '\n' ' '; echo
  echo "===== motor: OpenGL ($(und $B | grep -c '^gl[A-Z]') funções)"; und $B | grep '^gl[A-Z]' | tr '\n' ' '; echo
  M=$G/multi-display/multi-screen-server
  echo "===== multi-screen-server: EGL, GL, Wayland, DRM, GBM, OMX"; und $M | grep -E '^(egl|gl[A-Z]|wl_|drm|gbm|OMX)' | tr '\n' ' '; echo; } > $O/vc_inv_importados.txt
strings -n 4 $G/multi-display/multi-screen-server | grep -v '^_Z' > $O/vc_inv_multi-screen-server.strings
strings -n 4 $G/multi-display/rsi-display-management | grep -v '^_Z' > $O/vc_inv_rsi-display-management.strings
cp $P/eu/single/etc/nav/standardConfig.cfg $O/vc_inv_standardConfig_eu_single.cfg
cp $P/eu/dual/etc/nav/standardConfig.cfg $O/vc_inv_standardConfig_eu_dual.cfg
cd /; umount $L; ls -la $O | sed 's/^/  /'
