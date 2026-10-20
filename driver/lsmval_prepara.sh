#!/bin/sh
# lsmval_prepara.sh <arquivo> — confere um arquivo de partida da memória de estado da central
# (Marco 6, opção LSMVAL do m1_hmi.sh) e o escreve na saída do jeito que o firmware lê.
# Roda no Mac ou no container; serve também para conferir um arquivo antes de subir a central:
#   sh driver/lsmval_prepara.sh stubs/persistencia/idioma-pt_BR.txt
# Uma linha por valor, com os números em decimal, como o persistence-app grava em
# /data/lge/hmi/lsmval:
#     <tipo> <namespace> <chave> <valores...>
#   tipo 0 = bytes, cada um de 0 a 255; tipo 1 = um número inteiro
# "#" começa um comentário; linha em branco não conta. Sobra de espaço, tabulação e fim de linha
# do Windows são aceitos e saem como UM espaço.
# Por que conferir: quem lê o arquivo na partida são o persistence-app (init_pso_file) e a HMI
# (DSIPersistenceImpl.initLSMDataInFile), e os dois descartam a linha que não entendem sem parar
# nem avisar na tela. A HMI separa os campos por um espaço só e recusa a linha que tiver dois
# seguidos; o persistence-app recusa namespace 0, guarda até 500 valores e lê até 9998 letras por
# linha. Aqui qualquer linha errada recusa o arquivo inteiro: sai o arquivo e a linha na saída de
# erro, nada na saída e o código 2.
[ $# -eq 1 ] || { echo "uso: lsmval_prepara.sh <arquivo>" >&2; exit 2; }
[ -f "$1" ] || { echo "$1: não achei o arquivo; nada foi gravado" >&2; exit 2; }
awk -v ARQ="$1" '
function erro(msg) {
  printf "%s, linha %d: %s; nada foi gravado\n", ARQ, NR, msg > "/dev/stderr"
  ruim = 1; exit
}
{ sub(/#.*/, ""); gsub(/[ \t\r]+/, " "); sub(/^ /, ""); sub(/ $/, "") }
$0 == "" { next }
{
  n = split($0, c, " ")
  if (n < 4) erro("esperava <tipo> <namespace> <chave> e pelo menos um valor")
  if (c[1] != "0" && c[1] != "1") erro("tipo " c[1] " desconhecido (0 = bytes, 1 = número inteiro)")
  if (c[2] !~ /^[0-9]+$/ || c[2] + 0 == 0) erro("namespace " c[2] " não vale (número maior que zero)")
  if (c[3] !~ /^-?[0-9]+$/) erro("chave " c[3] " não é um número")
  if (c[1] == "1") {
    if (n != 4 || c[4] !~ /^-?[0-9]+$/) erro("o tipo 1 leva um número inteiro só")
  } else {
    for (i = 4; i <= n; i++)
      if (c[i] !~ /^[0-9]+$/ || c[i] + 0 > 255) erro("byte " c[i] " não vale (decimal de 0 a 255)")
  }
  if (length($0) > 9998) erro("linha de " length($0) " letras; o firmware lê até 9998")
  k = c[2] " " c[3]
  if (k in visto) erro("namespace e chave repetidos (já estavam na linha " visto[k] ")")
  visto[k] = NR
  if (++q > 500) erro("passou de 500 valores, que é quanto o firmware guarda")
  linha[q] = $0
}
END {
  if (ruim) exit 2
  for (i = 1; i <= q; i++) print linha[i]
}' "$1"
