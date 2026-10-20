#!/bin/sh
# Compila os testes do encaminhador (Marco 3) na imagem moi3-bancada:1.
#   build_tests.sh <pasta com libGLESv2_adreno.so e libEGL_adreno.so> [<saída>]
# Os testes ligam pelos sonames do Adreno, então o mesmo binário roda contra o stub ou contra
# o encaminhador: quem decide é o LD_LIBRARY_PATH na hora de rodar.
set -e
LIBS=$1; OUT=${2:-/work3/tests}; SRC=$(dirname "$0")
mkdir -p "$OUT"
for t in "$SRC"/t_*.c; do
  n=$(basename "$t" .c)
  gcc -O1 -Wall -Wno-unused-function -o "$OUT/$n" "$t" -L"$LIBS" -l:libGLESv2_adreno.so -l:libEGL_adreno.so
done
gcc -O1 -Wall -o "$OUT/ppmpix" "$SRC/ppmpix.c"
gcc -O1 -Wall -o "$OUT/le_quadros" "$SRC/le_quadros.c"
ls "$OUT"
