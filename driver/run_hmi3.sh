#!/bin/sh
LOW=/mnt/low; mkdir -p $LOW
mount -o loop,ro /scratch/newfw/system_new.img $LOW 2>/dev/null || { echo MOUNTFAIL; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d 2>/dev/null; done
mkdir -p $LOW/proc; mount -t proc proc $LOW/proc 2>/dev/null
mknod $LOW/dev/null c 1 3 2>/dev/null; mknod $LOW/dev/zero c 1 5 2>/dev/null; mknod $LOW/dev/urandom c 1 9 2>/dev/null
chmod 666 $LOW/dev/null $LOW/dev/zero $LOW/dev/urandom 2>/dev/null

# ---------- EXPERIMENTO 1: loadLibrary isolado ----------
cat > $LOW/tmp/T.java <<'EOJ'
public class T {
  public static void main(String[] a){
    System.out.println("java.library.path="+System.getProperty("java.library.path"));
    System.out.println("sun.boot.library.path="+System.getProperty("sun.boot.library.path"));
    try { System.loadLibrary("DebugSPINative"); System.out.println(">>> loadLibrary OK"); }
    catch(Throwable t){ System.out.println(">>> FALHOU: "+t); }
    try { System.load("/lge/hmi/libs/libDebugSPINative.so"); System.out.println(">>> load(absoluto) OK"); }
    catch(Throwable t){ System.out.println(">>> load(absoluto) FALHOU: "+t); }
  }
}
EOJ
cat > $LOW/tmp/exp1.sh <<'EOS'
export LD_LIBRARY_PATH=/lge/hmi/libs:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/usr/lib:/lib:/lib64
cd /tmp
/lge/hmi/jvm/bin/javac T.java 2>&1 || { echo "javac falhou (sem javac? usa classe pronta)"; }
/lge/hmi/jvm/bin/java -Djava.library.path=/lge/hmi/libs -cp /tmp T
EOS
echo "========== EXPERIMENTO 1: loadLibrary isolado =========="
chroot $LOW /bin/sh /tmp/exp1.sh 2>&1
echo
echo "========== EXPERIMENTO 2: HMI real + sun.boot.library.path =========="
mkdir -p $LOW/dev/dri; mknod $LOW/dev/dri/card0 c 226 0 2>/dev/null; mknod $LOW/dev/dri/renderD128 c 226 128 2>/dev/null; chmod 666 $LOW/dev/dri/* 2>/dev/null
mount --bind /scratch/vwroot $LOW/vw 2>/dev/null
mkdir -p $LOW/var/lib/hmi $LOW/var/cache/hmi $LOW/run/platform/weston 2>/dev/null
: > $LOW/dev/csr; : > $LOW/dev/css; chmod 666 $LOW/dev/csr $LOW/dev/css
cat > $LOW/tmp/launch3.sh <<'EOS'
export XDG_RUNTIME_DIR=/run/platform/weston
export LD_LIBRARY_PATH=/lge/hmi/libs:/lge/app_ro/lib64:/lge/hmi/jvm/lib/aarch64:/lge/app_rw/smartlink/dcmanager:/usr/lib:/lib:/lib64
export LD_PRELOAD=/lge/hmi/jvm/lib/aarch64/libjsig.so
CP=$(ls /lge/hmi/jar/*.jar /vw/hmi/lib/*.jar /vw/hmi/jar/*.jar 2>/dev/null | tr '\n' ':')
B=/vw/hmi
exec /lge/hmi/jvm/bin/java \
  -Dde.vw.mib.installation.directory=$B -Dde.vw.mib.configuration.directory=$B/config \
  -Dde.vw.mib.resources.directory=$B/Resources -Dvw.mib.resources=$B/res \
  -Dbundles.properties=$B/config/bundles.properties -Dsystem.properties=$B/config/system.properties \
  -Dlogging.properties=$B/config/logging.properties -Dthreads.properties=$B/config/threads.properties \
  -Djava.library.path=/lge/hmi/libs -Dsun.boot.library.path=/lge/hmi/jvm/lib/aarch64:/lge/hmi/libs \
  -Dviewhandler.format=jar -Dfile.encoding=UTF-8 -Dde.vw.mib.view.gfx.init.eager \
  -Xbootclasspath/a:$CP de.vw.mib.MIBMain
EOS
timeout 35 chroot $LOW /bin/sh /tmp/launch3.sh > /out/hmi3_out 2>&1
echo "  rc=$?  linhas=$(wc -l < /out/hmi3_out)"
grep -aE "DebugSPI|SPINative|UnsatisfiedLink|Starting SPI|bundle|DSI|Adreno|card0|Exception|FATAL|MIBMain|register|ViewHandler|loaded" /out/hmi3_out | head -40
