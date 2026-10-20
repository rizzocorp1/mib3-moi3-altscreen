#!/bin/sh
# Compila o encaminhador de OpenGL do sandbox MOI3 (Marco 3) dentro da imagem moi3-bancada:1.
#   build.sh [<saída>]          (default /work3/glfwd)
# Produz:
#   libGLESv2_adreno.so  o shim, ligado contra a glibc 2.23 DO FIRMWARE (pasta $FWLIBS), para
#                        carregar dentro da JVM da HMI sem misturar glibc
#   libEGL_adreno.so     casca vazia que depende do shim (a HMI pede os dois nomes)
#   glhost               o lado que desenha, ligado normalmente contra o Mesa do container
#   ponte                leva a transmissão dos quadros para fora do container (docker exec -i)
# $FWLIBS precisa ter libc.so.6, libpthread.so.0 e ld-linux-aarch64.so.1 copiados de /lib64 da
# partição system (cp -L). Os .inc de gen/ vêm de gen_glfwd.py, rodado no Mac.
set -e
OUT=${1:-/work3/glfwd}; SRC=$(dirname "$0"); FW=${FWLIBS:-/work3/fwlibs}
mkdir -p "$OUT"
[ -f "$FW/libc.so.6" ] || { echo "faltam as libs do firmware em $FW"; exit 1; }
gcc -shared -fPIC -O2 -Wall -Wno-unused-function -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
    -mno-outline-atomics -nostdlib -Wl,-soname,libGLESv2_adreno.so -Wl,-z,defs -Wl,-rpath-link,"$FW" \
    -o "$OUT/libGLESv2_adreno.so" "$SRC/shim.c" "$FW/libc.so.6" "$FW/libpthread.so.0"
echo 'static int moi3_glfwd_casca __attribute__((unused));' > /tmp/egl_casca.c
gcc -shared -fPIC -nostdlib -Wl,-soname,libEGL_adreno.so -Wl,--no-as-needed \
    -o "$OUT/libEGL_adreno.so" /tmp/egl_casca.c -L"$OUT" -l:libGLESv2_adreno.so
gcc -O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -o "$OUT/glhost" "$SRC/host.c" -lEGL -lGLESv2 -lpthread
gcc -O2 -Wall -o "$OUT/ponte" "$SRC/ponte.c"
ls -la "$OUT"
