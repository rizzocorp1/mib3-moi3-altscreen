#!/bin/bash
# Compila altscreen_handler.so estritamente contra as bibliotecas do Yocto / GLIBC 2.17/2.27 do MOI3 LG.
# Garante que dlsym seja resolvido a partir de libdl.so.2 e não de libc.so.6.
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$DIR/../../../../.." && pwd)"

docker run --rm \
  -v "$REPO_ROOT":/workspace \
  -w /workspace/referencia/ferramentas/mib3-sandbox/deploy-usb/payload \
  moi3-bancada:rsidbg sh -c "
SYSROOT=/workspace/referencia/ferramentas/mib3-sandbox/sysroot/u18_glibc_aarch64
aarch64-linux-gnu-gcc -shared -fPIC -O2 -Wall -Wextra altscreen_handler.c -o altscreen_handler.so \
  -nostdlib \
  -Wl,-rpath-link,\$SYSROOT \
  \$SYSROOT/libdl.so.2 \$SYSROOT/libpthread.so.0 \$SYSROOT/libc.so.6 \$SYSROOT/ld-linux-aarch64.so.1
echo '[OK] altscreen_handler.so compilado com sucesso!'
"
