#!/bin/sh
# Testes da transmissão direta dos quadros (glhost -> espectador) e da ponte que leva a
# transmissão para fora do container. Roda dentro da imagem moi3-bancada:1, sem firmware além
# das libs do shim.
#   t_transmissao.sh <pasta das libs sob teste> <glhost> [<ponte>]
# Os binários dos testes vêm de build_tests.sh ($TESTS_BIN, default /work3/tests). O que os
# espectadores receberam fica em $SAIDA (default /tmp/tq-out), para conferir de fora.
LIBS=$1; HOST=$2; PONTE=${3:-$(dirname "$HOST")/ponte}; T=${TESTS_BIN:-/work3/tests}
FR=/tmp/tq-frames; SOCK=/tmp/tq-gl.sock; QS=/tmp/tq-quadros.sock; O=${SAIDA:-/tmp/tq-out}
pass=0; fail=0
ok() { pass=$((pass+1)); echo "PASSOU $1"; }
ko() { fail=$((fail+1)); echo "FALHOU $1"; }
confere() {   # confere <nome> <esperado> <veio>
  if [ "$2" = "$3" ]; then ok "$1"; else ko "$1: esperado '$2', veio '$3'"; fi
}
igual() {     # igual <nome> <imagem recebida>: tem de ser, byte a byte, o quadro que o glhost gravou
  if cmp -s "$2" $FR/latest.ppm; then ok "$1"; else ko "$1: $2 difere de latest.ppm"; fi
}
sobe() {      # um glhost novo, com a transmissão em $QS
  rm -rf $FR $SOCK $QS; mkdir -p $FR $O
  GLHOST_STREAM=$QS "$HOST" $SOCK 1280 640 $FR >$O/glhost.log 2>&1 &
  HP=$!
  i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
}
desce() { kill $HP 2>/dev/null; wait $HP 2>/dev/null; }
desenha() { MOI3_GL_SOCKET=$SOCK LD_LIBRARY_PATH=$LIBS timeout 60 $T/"$@"; }
espera_linha() {   # espera_linha <arquivo> <texto>: até 8 s
  i=0; while ! grep -q "$2" "$1" 2>/dev/null && [ $i -lt 80 ]; do sleep 0.1; i=$((i+1)); done
}
quadros_de() { grep '^quadro' "$1" 2>/dev/null | tr '\n' ';'; }
rm -rf $O; mkdir -p $O

# 1. Quem já está ligado recebe o quadro assim que ele é desenhado, inteiro.
sobe
$T/le_quadros $QS $O/a.ppm 1 8000 >$O/a.log 2>&1 &
V=$!
espera_linha $O/a.log conectado
desenha t_clear >$O/t_clear.log 2>&1
wait $V
confere "primeiro_quadro: chega inteiro a quem já estava ligado" \
  "quadro 1 1280x640 chave=1 bytes=2457608;" "$(quadros_de $O/a.log)"
igual "primeiro_quadro: a imagem recebida é a que o glhost gravou" $O/a.ppm

# 2. Quem chega depois, com a tela parada e o cliente de OpenGL já fora, recebe o último quadro.
timeout 10 $T/le_quadros $QS $O/b.ppm 1 3000 >$O/b.log 2>&1
confere "quem_chega_depois: recebe o último quadro sem esperar desenho novo" \
  "quadro 1 1280x640 chave=1 bytes=2457608;" "$(quadros_de $O/b.log)"
igual "quem_chega_depois: a imagem recebida é a que o glhost gravou" $O/b.ppm
desce

# 3. Depois do primeiro quadro só viaja o que mudou: um retângulo de 40x30 custa 30 trechos
#    de 8 + 40*3 bytes; quadro igual ao anterior não leva carga nenhuma.
sobe
$T/le_quadros $QS $O/c.ppm 3 8000 $O/c.bin >$O/c.log 2>&1 &
V=$!
espera_linha $O/c.log conectado
desenha t_quadros >$O/t_quadros.log 2>&1
wait $V
confere "so_o_que_mudou: tamanho de cada quadro" \
  "quadro 1 1280x640 chave=1 bytes=2457608;quadro 2 1280x640 chave=0 bytes=3840;quadro 3 1280x640 chave=0 bytes=0;" \
  "$(quadros_de $O/c.log)"
igual "so_o_que_mudou: a imagem montada dos três quadros é a que o glhost gravou" $O/c.ppm
confere "so_o_que_mudou: o retângulo ficou no lugar (120,425)" "0 255 0" "$($T/ppmpix $O/c.ppm 120 425 2>&1)"
confere "so_o_que_mudou: fora do retângulo continua azul (120,400)" "0 0 255" "$($T/ppmpix $O/c.ppm 120 400 2>&1)"
cp $FR/latest.ppm $O/c.latest.ppm 2>/dev/null
desce

# 4. Espectador que liga e não lê nada não segura o desenho nem os outros espectadores.
sobe
$T/le_quadros $QS --dorme 40000 >$O/d.log 2>&1 &
V=$!
espera_linha $O/d.log conectado
t0=$(date +%s)
desenha t_quadros muitos >$O/muitos.log 2>&1; rc=$?
t1=$(date +%s)
confere "espectador_parado: os 120 quadros são desenhados até o fim" "0" "$rc"
if [ $((t1 - t0)) -le 20 ]; then ok "espectador_parado: sem atraso ($((t1 - t0)) s)"; else ko "espectador_parado: levou $((t1 - t0)) s"; fi
timeout 10 $T/le_quadros $QS $O/e.ppm 1 3000 >$O/e.log 2>&1
confere "espectador_parado: outro espectador recebe o último quadro" \
  "quadro 120 1280x640 chave=1 bytes=2457608;" "$(quadros_de $O/e.log)"
igual "espectador_parado: a imagem do outro espectador é a que o glhost gravou" $O/e.ppm
kill $V 2>/dev/null; wait $V 2>/dev/null
desce

# 5. A ponte: os quadros saem pela saída padrão e as linhas válidas da entrada padrão vão para
#    o arquivo de comandos (o que não é comando de entrada fica de fora).
sobe
desenha t_clear >$O/t_clear2.log 2>&1
: > $O/cmd.txt
(printf 'tecla 13 78 1\nlixo; rm -rf /\ngesto 4 10 20\ntecla 13 78\nencoder 13 16 -2\ngesto 1 400 240 1\n'; sleep 2) \
  | timeout 10 "$PONTE" $QS $O/cmd.txt 2>$O/ponte.err | timeout 10 $T/le_quadros - $O/p.ppm 1 3000 >$O/p.log 2>&1
confere "ponte: o quadro atravessa pela saída padrão" \
  "quadro 1 1280x640 chave=1 bytes=2457608;" "$(quadros_de $O/p.log)"
igual "ponte: a imagem que atravessou é a que o glhost gravou" $O/p.ppm
confere "ponte: só os comandos de entrada chegam ao arquivo" \
  "$(printf 'tecla 13 78 1\ngesto 4 10 20\nencoder 13 16 -2\ngesto 1 400 240 1')" "$(cat $O/cmd.txt 2>&1)"
desce
timeout 20 "$PONTE" /tmp/tq-nao-existe.sock $O/cmd.txt </dev/null >/dev/null 2>$O/ponte2.err; rc=$?
confere "ponte: sem soquete dos quadros, sai com o código 3" "3" "$rc"

# 6. A ponte liga antes de o arquivo de comandos estar no lugar. É o que acontece na partida da
#    central: a janela liga cedo, e só depois o lançador põe o arquivo de verdade naquele caminho.
#    Ela não pode criar o arquivo nem ficar presa ao que havia no caminho quando ligou: cada linha
#    vai para o arquivo que estiver lá na hora em que a linha chega.
sobe
desenha t_clear >$O/t_clear3.log 2>&1
rm -f $O/cmd6.txt $O/fila6; mkfifo $O/fila6
timeout 20 "$PONTE" $QS $O/cmd6.txt < $O/fila6 >/dev/null 2>$O/ponte6.err &
PP=$!
exec 9> $O/fila6                      # a ponta de escrita: a ponte começa a rodar
sleep 0.7                             # tempo de ela ligar no soquete dos quadros
if [ -e $O/cmd6.txt ]; then ko "ponte_liga_cedo: não cria o arquivo de comandos"; else ok "ponte_liga_cedo: não cria o arquivo de comandos"; fi
echo "tecla 13 78 1" >&9; sleep 0.3   # sem arquivo, a linha não tem para onde ir
: > $O/cmd6-a.txt; mv $O/cmd6-a.txt $O/cmd6.txt             # o arquivo aparece
echo "tecla 13 6 1" >&9; sleep 0.3
confere "ponte_liga_cedo: a linha chega ao arquivo que apareceu depois" "tecla 13 6 1" "$(cat $O/cmd6.txt 2>&1)"
: > $O/cmd6-b.txt; mv $O/cmd6-b.txt $O/cmd6.txt             # outro arquivo toma o lugar, no mesmo caminho
echo "gesto 1 400 240 1" >&9; sleep 0.3
confere "ponte_liga_cedo: depois da troca, a linha vai para o arquivo novo" "gesto 1 400 240 1" "$(cat $O/cmd6.txt 2>&1)"
exec 9>&-
wait $PP 2>/dev/null
desce

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
