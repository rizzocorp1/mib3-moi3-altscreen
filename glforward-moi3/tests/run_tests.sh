#!/bin/sh
# Roda os testes do encaminhador de OpenGL (Marco 3) dentro da imagem moi3-bancada:1.
#   run_tests.sh <pasta das libs sob teste> [<binário do glhost>]
# Sem glhost, as libs são usadas sozinhas (é como se roda contra o stub de 10/10, que TEM de falhar).
# Com glhost, cada teste ganha um glhost novo escutando num socket Unix.
# Os binários dos testes vêm de build_tests.sh ($TESTS_BIN, default /work3/tests).
LIBS=$1; HOST=$2; T=${TESTS_BIN:-/work3/tests}; FR=/tmp/tframes; SOCK=/tmp/t-gl.sock
pass=0; fail=0
for t in t_clear t_shader t_texture t_buffers t_fbo t_getters $EXTRA_TESTS; do
  rm -rf $FR $SOCK; mkdir -p $FR
  if [ -n "$HOST" ]; then
    "$HOST" $SOCK 1280 640 $FR >/tmp/host_$t.log 2>&1 &
    HP=$!
    i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
  fi
  MOI3_GL_SOCKET=$SOCK LD_LIBRARY_PATH=$LIBS timeout 60 $T/$t >/tmp/$t.log 2>&1; rc=$?
  if [ -n "$HOST" ]; then sleep 0.2; kill $HP 2>/dev/null; wait $HP 2>/dev/null; fi
  tail -n 25 /tmp/$t.log
  if [ $rc -eq 0 ]; then pass=$((pass+1)); else
    fail=$((fail+1)); echo "  ($t saiu com rc=$rc)"
    [ -n "$HOST" ] && tail -n 6 /tmp/host_$t.log | sed 's/^/  glhost: /'
  fi
  if [ $t = t_clear ]; then
    px=$($T/ppmpix $FR/latest.ppm 640 320 2>&1)
    if [ "$px" = "255 0 0" ]; then pass=$((pass+1)); echo "PASSOU quadro_em_arquivo: latest.ppm (640,320) = $px"
    else fail=$((fail+1)); echo "FALHOU quadro_em_arquivo: esperado '255 0 0', veio '$px'"; fi
    # o visualizador ao vivo acompanha seq.txt: "<número do quadro> <largura> <altura>"
    sq=$(cat $FR/seq.txt 2>/dev/null)
    if [ "$sq" = "1 1280 640" ]; then pass=$((pass+1)); echo "PASSOU contador_de_quadros: seq.txt = $sq"
    else fail=$((fail+1)); echo "FALHOU contador_de_quadros: esperado '1 1280 640', veio '$sq'"; fi
  fi
done
echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
