#!/bin/sh
# vc_rodada_container.sh — investigação do painel de instrumentos ("Virtual Cockpit"), 11/10/2026.
# Roda DENTRO da imagem moi3-bancada:1, chamado pelo vc_rodada.sh. Sobe a HMI com o lançador da
# bancada (driver/m1_hmi.sh, usado como está) e, ao lado dela, os serviços de navegação e de
# vídeo do painel que o firmware traz, cada um sob strace, para ver até onde vão e por que param.
# É estudo num container descartável e sem rede; nada vai para o carro e nada aqui mexe em
# assinatura, dm-verity, secure boot nem em ativação de função.
#
# Saídas em /out, com o nome da rodada:
#   <tag>.out               a saída da HMI (do m1_hmi.sh)
#   <tag>.trace.log         o que os daemons mandaram ao soquete de log (vc_trace_escuta)
#   <tag>.barramento.log    as mensagens do barramento (dsi_escuta)
#   <tag>.<serviço>.out     saída padrão de cada serviço extra; <tag>.<serviço>.strace as chamadas
#   <tag>.nav.glhost.log    o glhost dedicado ao motor de navegação; quadros em frames_<tag>_nav
#   <tag>.wlstub.log        os pedidos Wayland que chegaram ao compositor de mentira
#   <tag>.diagalm.log       os conjuntos de codificação mandados de novo no formato do alm (DIAGALM)
#   <tag>.rsi.txt           o que o registro RSI e o serviço displaymanagement respondiam no fim
#   <tag>.vivos.txt         quem estava de pé no fim
#   foto_<tag>_<t>.ppm      a tela da HMI aos <t> segundos
# Do ambiente (tudo opcional):
#   SECS      duração da HMI (default 150)
#   SERVICOS  quais serviços extras sobem, nesta ordem (default "alm navi dispmgmt navsd mss"):
#             alm = navi-al, navi = tsd.nav.mainapp.mib3, dispmgmt = rsi-display-management,
#             navsd = navsd_app, mss = multi-screen-server
#   NAVPKG    pacote do motor (default eu/single; no carro quem escolhe é o post-init, pelos GPIOs)
#   GPIO      arquivos de /sys/devices/soc/soc:common_gpio_default a criar, "nome=valor ..."
#             (são os resistores de identificação da placa; sem isso o /sys do chroot fica vazio)
#   MAPA      se existir a pasta /mapa, ela é montada, só leitura, em /var/lib/nav/nav
#   WLSTUB    "largura altura mm_largura mm_altura": sobe o compositor Wayland de mentira
#             (vc_wlstub.c) no lugar do Weston, anunciando uma saída desse tamanho. Sem isso
#             não há compositor, como na bancada até aqui
#   DIAGALM   1 ou "todos": sobe o vc_diag_alm.c, que espera o diagstub publicar os conjuntos de
#             codificação e os manda de novo com o cabeçalho que o alm entende (83 60 22), com os
#             mesmos dados. 1 = só os oito conjuntos que o alm usa; todos = os 48. Sem isso os
#             conjuntos vão só com o cabeçalho zerado do diagstub, e o alm descarta todos.
#             SEM EFEITO desde as 23h26 de 11/10/2026: o diagstub passou a mandar 83 60 22 ele
#             mesmo (D77 em modelagem-soc.md); a ferramenta sobe, não acha o que trocar e sai
#   CODING, ROTEIRO, LSMVAL, HMILOG, ESPERA   como no m1_hmi.sh
#   FOTOS     segundos em que a tela é fotografada (default "70 100 140")
W=/tmp/w; TAG=${TAG:-vc}; SECS=${SECS:-150}; LOW=/mnt/low; O=/out
ESPERA=${ESPERA-2000}; HMILOG=${HMILOG-normal}; RSI=${RSI-1}
RSI_SERVICES=${RSI_SERVICES-usermanagement:auth onlineregistration:communication}
RSI_DATA=${RSI_DATA-/stubs/rsi-mockup}
SERVICOS=${SERVICOS-alm navi dispmgmt navsd mss}; NAVPKG=${NAVPKG:-eu/single}
FOTOS=${FOTOS-70 100 140}
export ESPERA HMILOG RSI RSI_SERVICES RSI_DATA CODING ROTEIRO LSMVAL
# chamadas que interessam: arquivos procurados, ligações, processos. Sem leitura nem escrita.
SYSC=openat,faccessat,readlinkat,execve,connect,bind,listen,exit_group,kill,tgkill,mkdirat,unlinkat,renameat,symlinkat,chdir
DLLP=/lge/app_ro/lib64:/usr/lib64:/usr/lib:/lib64:/lib:/lge/hmi/libs:/lge/app_ro/hmi:/lge/app_ro/carif
NAVAL=/lge/app_ro/navi/navi-al; NAVEG=/lge/app_ro/navi/naviengine

mkdir -p $W/fwlibs $LOW $O/frames_$TAG
mount -o loop,ro,noload /fw/system.img $LOW && mount --bind /fw/vw $LOW/vw || { echo "não montou o firmware"; exit 1; }
cp -L $LOW/lib64/libc.so.6 $LOW/lib64/libpthread.so.0 $LOW/lib64/ld-linux-aarch64.so.1 $W/fwlibs/
sh /driver/m1_build.sh $W/bin > $W/build.log 2>&1 || { tail -5 $W/build.log; exit 1; }
FWLIBS=$W/fwlibs sh /glf/build.sh $W/glfwd >> $W/build.log 2>&1 || { tail -8 $W/build.log; exit 1; }
gcc -static -O2 -o $W/bin/vc_trace_escuta /driver/vc_trace_escuta.c || exit 1
gcc -static -O2 -Wall -o $W/bin/vc_wlstub /driver/vc_wlstub.c || exit 1
gcc -static -O2 -Wall -o $W/bin/vc_diag_alm /driver/vc_diag_alm.c || exit 1
rm -f $O/$TAG.out

espera_arquivo() { i=0; while [ ! -e "$1" ] && [ $i -lt ${2:-300} ]; do sleep 0.1; i=$((i+1)); done; [ -e "$1" ]; }
sob_strace() {   # sob_strace <nome> <roteiro dentro do chroot>
  strace -f -qq --seccomp-bpf -s 200 -e trace=$SYSC -o $O/$TAG.$1.strace chroot $LOW /bin/sh $2 > $O/$TAG.$1.out 2>&1 &
}

# 1. o soquete de log, assim que o /tmp do chroot existir (o m1_hmi.sh o monta logo no começo)
( while ! grep -q " $LOW/tmp tmpfs" /proc/mounts; do sleep 0.05; done
  exec $W/bin/vc_trace_escuta $LOW/tmp/trace_unix_socket > $O/$TAG.trace.log 2>&1 ) &

# 2. escuta do barramento
( espera_arquivo $LOW/run/kipc_hub.sock; sleep 0.3
  exec $W/bin/dsi_escuta $LOW/run/kipc_hub.sock > $O/$TAG.barramento.log 2>&1 ) &

# 2b. diagnóstico no formato do alm (só se pedido). Liga-se ao barramento antes de o diagstub
#     publicar (o m1_hmi.sh o roda 1 s depois do hub) e manda de novo logo em seguida, bem antes
#     de os daemons, o alm e a HMI se ligarem: quem chega depois já recebe a versão nova
if [ -n "$DIAGALM" ]; then
  ( espera_arquivo $LOW/run/kipc_hub.sock || exit 0
    A=; [ "$DIAGALM" = todos ] && A=-a
    exec $W/bin/vc_diag_alm $A $LOW/run/kipc_hub.sock > $O/$TAG.diagalm.log 2>&1 ) &
fi

# 3. os serviços extras, depois que o barramento existe
( espera_arquivo $LOW/run/kipc_hub.sock || exit 0
  T0=$(date +%s)
  # os resistores de identificação da placa (só se pedido)
  if [ -n "$GPIO" ]; then
    mount -t tmpfs tmpfs $LOW/sys && G=$LOW/sys/devices/soc/soc:common_gpio_default && mkdir -p $G
    for kv in $GPIO; do echo "${kv#*=}" > "$G/${kv%%=*}"; done
    echo "[vc] GPIO: $(cd $G && grep . * | tr '\n' ' ')"
  fi
  # o que o post-init do firmware (post_init/data/navigation.sh) deixaria pronto
  mkdir -p $LOW/data/lge/navi/pso $LOW/data/lge/navi/pos $LOW/data/lge/navi/persist $LOW/data/lge/navi/cert \
           $LOW/var/lib/logging $LOW/var/lib/nav/tempUM $LOW/var/lib/nav/nav $LOW/var/run/services
  cp $LOW$NAVEG/navi_TS.pem $LOW/data/lge/navi/cert/
  ln -s $NAVEG/package/$NAVPKG $LOW/data/lge/navi/navicore
  if [ -d /mapa ]; then mount --bind /mapa $LOW/var/lib/nav/nav && mount -o remount,ro,bind $LOW/var/lib/nav/nav
    echo "[vc] mapa montado em /var/lib/nav/nav: $(ls $LOW/var/lib/nav/nav | tr '\n' ' ')"; fi
  # compositor Wayland de mentira (só se pedido): sem ele o motor fica tentando ligar no Weston
  if [ -n "$WLSTUB" ]; then
    $W/bin/vc_wlstub $LOW/run/platform/weston/wayland-0 $WLSTUB > $O/$TAG.wlstub.log 2>&1 &
    echo "[vc] compositor de mentira em /run/platform/weston/wayland-0 ($WLSTUB)"
  fi
  # espera a camada RSI (registro na 443), que o m1_hmi.sh sobe 1 s depois do barramento
  espera_arquivo $LOW/tmp/rsi-mockup.log 200; sleep 2

  for s in $SERVICOS; do case $s in
  alm)
    # como o runNavi-al.sh: barramento D-Bus de sessão próprio e o alm
    chroot $LOW /bin/sh -c 'dbus-launch > /tmp/dbus.conf 2> /tmp/dbus-launch.err'
    if [ ! -s $LOW/tmp/dbus.conf ]; then
      echo "[vc] dbus-launch não respondeu: $(cat $LOW/tmp/dbus-launch.err | head -3)"
      chroot $LOW /bin/sh -c 'a=$(dbus-daemon --session --fork --print-address 2>/tmp/dbus-daemon.err); echo "DBUS_SESSION_BUS_ADDRESS=$a" > /tmp/dbus.conf'
    fi
    echo "[vc] dbus: $(cat $LOW/tmp/dbus.conf | tr '\n' ' ')"
    cat > $LOW/tmp/vc_alm.sh <<EOS
cd $NAVAL
. /tmp/dbus.conf
export DBUS_SESSION_BUS_ADDRESS DBUS_SESSION_BUS_PID
export LD_LIBRARY_PATH=$NAVAL/lib64:/usr/lib64
export COMMONAPI_CONFIG=$NAVAL/commonapi.ini COMMONAPI_DBUS_CONFIG=$NAVAL/commonapi-dbus.ini
export LOG_STDOUT=${ALMLOG:-1} LD_PRELOAD=/tmp/kipc_bus.so
exec ./alm
EOS
    sob_strace alm /tmp/vc_alm.sh; sleep ${NAVWAIT:-6} ;;
  navi)
    # como o runNavi.sh, com duas diferenças da bancada: as duas bibliotecas de vídeo são o
    # encaminhador (em /tmp/fakegl) e o motor fala com um glhost só dele
    mkdir -p $O/frames_${TAG}_nav
    GLHOST_STREAM=/tmp/moi3-quadros-nav.sock $W/glfwd/glhost $LOW/tmp/moi3-gl-nav.sock ${NAVW:-1280} ${NAVH:-480} \
      $O/frames_${TAG}_nav > $O/$TAG.nav.glhost.log 2>&1 &
    cat > $LOW/tmp/vc_navi.sh <<EOS
[ -f /tmp/dbus.conf ] && . /tmp/dbus.conf
export DBUS_SESSION_BUS_ADDRESS DBUS_SESSION_BUS_PID
export DISABLE_FENCE_PROC_LIST=tsd.nav.mainapp.mib3
cd $NAVEG/navicore/usr/bin || { echo "[vc] sem $NAVEG/navicore/usr/bin"; exit 97; }
export LD_LIBRARY_PATH=/tmp/fakegl:/usr/ext-library/lib64:$NAVAL/lib64:/usr/lib64:/lib64:\${PWD}/../lib64
export XDG_RUNTIME_DIR=/run/platform/weston
export COMMONAPI_CONFIG=$NAVAL/commonapi.ini COMMONAPI_DBUS_CONFIG=$NAVAL/commonapi-dbus.ini
export CURL_CA_BUNDLE=/data/lge/navi/cert/navi_TS.pem
export TSD_COMMON_CONFIG=$NAVEG/nav.prio.cfg
export MOI3_GL_SOCKET=/tmp/moi3-gl-nav.sock
exec ./tsd.nav.mainapp.mib3 --ro-path $NAVEG/navicore/etc/nav
EOS
    sob_strace navi /tmp/vc_navi.sh; sleep 2 ;;
  dispmgmt)
    cat > $LOW/tmp/vc_dispmgmt.sh <<EOS
export XDG_RUNTIME_DIR=/run/platform/weston LD_LIBRARY_PATH=$DLLP
exec /lge/app_ro/graphics/multi-display/rsi-display-management
EOS
    sob_strace dispmgmt /tmp/vc_dispmgmt.sh; sleep 2 ;;
  navsd)
    cat > $LOW/tmp/vc_navsd.sh <<EOS
export LD_LIBRARY_PATH=$DLLP LD_PRELOAD=/tmp/kipc_bus.so
exec /lge/app_ro/carif/navsd_app
EOS
    sob_strace navsd /tmp/vc_navsd.sh; sleep 1 ;;
  mss)
    mkdir -p $O/frames_${TAG}_mss
    GLHOST_STREAM= $W/glfwd/glhost $LOW/tmp/moi3-gl-mss.sock 1280 480 $O/frames_${TAG}_mss > $O/$TAG.mss.glhost.log 2>&1 &
    cat > $LOW/tmp/vc_mss.sh <<EOS
export XDG_RUNTIME_DIR=/run/platform/weston LD_LIBRARY_PATH=/tmp/fakegl:$DLLP
export MOI3_GL_SOCKET=/tmp/moi3-gl-mss.sock
exec /lge/app_ro/graphics/multi-display/multi-screen-server
EOS
    sob_strace mss /tmp/vc_mss.sh; sleep 1 ;;
  *) echo "[vc] serviço desconhecido: $s" ;;
  esac; done

  # perto do fim: o que o RSI responde e quem está de pé
  while [ $(( $(date +%s) - T0 )) -lt $((SECS - 12)) ]; do sleep 1; done
  C="env LD_LIBRARY_PATH=/usr/ext-library/lib64 /usr/ext-library/bin/curl -s -m 5"
  { echo "=== registro (nome porta)"
    chroot $LOW $C http://127.0.0.1:443/ | tr '{' '\n' | sed -n 's/.*"name":"\([^"]*\)","port":\([0-9]*\).*/\1 \2/p'
    for svc in displaymanagement navsystem maprenderer; do
      p=$(chroot $LOW $C http://127.0.0.1:443/ | tr '{' '\n' | sed -n 's/.*"name":"'$svc'","port":\([0-9]*\).*/\1/p' | head -1)
      [ -n "$p" ] || { echo "=== $svc: não registrado"; continue; }
      echo "=== GET $svc (porta $p)"; chroot $LOW $C http://127.0.0.1:$p/$svc/; echo
      for r in displays renderingSurfaces streamParameters systems renderers; do
        echo "=== GET /$svc/$r"; chroot $LOW $C "http://127.0.0.1:$p/$svc/$r"; echo
      done
    done; } > $O/$TAG.rsi.txt 2>&1
  for d in persistence-app mcu-interpreter carifservice touch-app MIB_ServiceRegistryApp-00 rsi-mockup dsistub java \
           dbus-daemon alm tsd.nav.mainapp rsi-display-management navsd_app multi-screen-server; do
    echo "$d: $(ps -eo args | grep -v grep | grep -v strace | grep -c "$d")"; done > $O/$TAG.vivos.txt
  ps -eo pid,ppid,etime,args | grep -v "ps -eo" >> $O/$TAG.vivos.txt
  cp $LOW/tmp/dbus-launch.err $O/$TAG.dbus-launch.err 2>/dev/null
  ls -laR $LOW/data/lge/navi $LOW/var/lib/logging $LOW/var/run/services $LOW/var/log 2>/dev/null > $O/$TAG.arquivos-navi.txt
) > $O/$TAG.vc.log 2>&1 &

# 4. fotos da tela da HMI
( while [ ! -f $O/$TAG.out ]; do sleep 0.2; done; t0=$(date +%s)
  for t in $FOTOS; do
    while [ $(( $(date +%s) - t0 )) -lt $t ]; do sleep 0.25; done
    cp $O/frames_$TAG/latest.ppm $O/foto_${TAG}_$t.ppm 2>/dev/null
  done ) &

LOW=$LOW BIN=$W/bin OUT=$O TAG=$TAG VARIANT=real SECS=$SECS \
GLDIR=$W/glfwd GLHOST=$W/glfwd/glhost GLFRAMES=$O/frames_$TAG GLHOST_KEEP=0 GLW=800 GLH=480 \
DMW=800 DMH=480 DSISTUB=1 POWER=${POWER:-1} INPUT= sh /driver/m1_hmi.sh
sleep 1
# guarda o último quadro de cada glhost extra e limpa as pastas de quadros
for g in nav mss; do [ -f $O/frames_${TAG}_$g/latest.ppm ] && cp $O/frames_${TAG}_$g/latest.ppm $O/foto_${TAG}_${g}_ultimo.ppm; done
ls $O/frames_${TAG}_nav 2>/dev/null | wc -l | sed "s/^/[vc] arquivos em frames_${TAG}_nav: /"
rm -rf $O/frames_$TAG $O/frames_${TAG}_nav $O/frames_${TAG}_mss
cat $O/$TAG.vc.log
