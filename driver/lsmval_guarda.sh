#!/bin/sh
# lsmval_guarda.sh — a memória de estado da central de uma sessão para a outra (Marco 6, 11/10/2026).
# No carro, o que a central guarda em /data/lge/hmi/lsmval (o idioma, a última tela, os ajustes)
# sobrevive ao desligar. Na bancada /data é memória do container e some com ele. Este roteiro
# mantém uma cópia desse arquivo numa pasta que fica fora do container (no modo ao vivo,
# ao-vivo/memoria/ do Mac) e diz de que arquivo a sessão seguinte nasce. Quem o chama é o
# m3_ao_vivo.sh; roda no container e, nos testes (driver/tests/t_memoria.sh), também no Mac.
# Precisa do lsmval_prepara.sh na mesma pasta.
#
#   lsmval_guarda.sh partida <pasta> <semente>
#       escreve na saída o arquivo de partida da sessão: <pasta>/lsmval, se houver memória
#       guardada e ela passar pelo lsmval_prepara.sh; senão a <semente> (o arquivo fixo de
#       stubs/persistencia/, ou vazio). Memória guardada com erro vai para <pasta>/lsmval.recusada,
#       com aviso, e vale a semente. A memória aceita ganha uma cópia em
#       <pasta>/anteriores/lsmval.1, e as de antes passam a .2, .3, .4 e .5 (a sexta some): é a
#       memória com que cada uma das cinco últimas sessões nasceu, para poder voltar atrás.
#   lsmval_guarda.sh passo <lsmval da central> <pasta>
#       uma conferência: guarda a cópia em <pasta>/lsmval se ela mudou, e diz numa linha o que fez
#       (guardada: N linha(s) | igual | mudando | vazia | pela metade | recusada: … | sem arquivo).
#   lsmval_guarda.sh vigia <lsmval da central> <pasta> [intervalo em segundos, default 2]
#       repete o passo até ser encerrada; registra cada cópia guardada e cada recusa.
#
# O persistence-app do firmware acrescenta uma linha ao lsmval a cada valor novo e REGRAVA o
# arquivo inteiro (fopen "w") quando a chave já existe: há instantes em que o arquivo está vazio
# ou pela metade. Por isso o passo só guarda a cópia que
#   1. ficou igual entre duas leituras com 0,3 s de intervalo (ninguém estava gravando);
#   2. não está vazia e termina em fim de linha, que é como o firmware termina toda linha (um
#      arquivo cortado no meio de uma linha ainda pode parecer certo ao conferidor);
#   3. passa pelo lsmval_prepara.sh, que é o que a sessão seguinte vai exigir dela.
# E grava ao lado e troca o nome: a cópia guardada nunca fica pela metade, nem se o container
# morrer no meio. Cópia recusada não apaga a guardada: fica a última boa, e o motivo fica em
# <pasta>/recusada.txt até uma cópia voltar a passar (a partida seguinte avisa, se ainda estiver
# lá). Memória vazia nunca é guardada: com ela a sessão seguinte nasceria sem a semente, em inglês.
D=$(dirname "$0"); PAUSA=0.3
uso() {
  echo "uso: lsmval_guarda.sh partida <pasta> <semente> | passo <lsmval da central> <pasta> | vigia <lsmval da central> <pasta> [intervalo]" >&2
  exit 2
}
pasta() { [ -d "$1" ] || { echo "[memoria] $1 não é uma pasta: a memória não tem onde ficar" >&2; exit 2; }; }
hora() { date +%H:%M:%S; }

# um_passo <lsmval da central> <pasta> <cópia de trabalho 1> <cópia de trabalho 2>: deixa em $R o que fez
um_passo() {
  R="sem arquivo"; [ -f "$1" ] || return 0
  cp "$1" "$3" 2>/dev/null || { R="não consegui gravar em $2"; return 0; }
  sleep $PAUSA
  cmp -s "$1" "$3" || { R="mudando"; return 0; }
  [ -s "$3" ] || { R="vazia"; return 0; }
  if [ "$(tail -c 1 "$3" | wc -l | tr -d ' ')" != 1 ]; then
    R="pela metade"; echo "o arquivo da central não termina em fim de linha: gravação cortada" > "$2/recusada.txt"; return 0
  fi
  if ! E=$(sh "$D/lsmval_prepara.sh" "$3" 2>&1 > "$4"); then
    E=$(printf '%s\n' "$E" | sed 's/^.*, linha /linha /; s/; nada foi gravado$//')
    R="recusada: $E"; printf '%s\n' "$E" > "$2/recusada.txt"; return 0
  fi
  rm -f "$2/recusada.txt"
  cmp -s "$4" "$2/lsmval" && { R="igual"; return 0; }
  chmod 644 "$4" 2>/dev/null
  if mv -f "$4" "$2/lsmval"; then R="guardada: $(wc -l < "$2/lsmval" | tr -d ' ') linha(s)"; else R="não consegui gravar em $2"; fi
}

case "$1" in
partida)
  [ $# -eq 3 ] || uso
  P=$2; SEM=$3; A="$P/anteriores"; pasta "$P"
  rm -f "$P"/.lsmval.*                                  # cópias de trabalho de uma sessão que morreu no meio
  [ -f "$P/recusada.txt" ] && echo "[memoria] a sessão anterior parou de guardar a memória ($(cat "$P/recusada.txt")); a guardada é a última cópia boa" >&2
  if [ ! -f "$P/lsmval" ]; then printf '%s\n' "$SEM"; exit 0; fi
  if ! E=$(sh "$D/lsmval_prepara.sh" "$P/lsmval" 2>&1 >/dev/null); then
    mv -f "$P/lsmval" "$P/lsmval.recusada"
    echo "[memoria] memória guardada recusada (${E%; nada foi gravado}). Ficou em $P/lsmval.recusada; a central nasce de ${SEM:-memória vazia}" >&2
    printf '%s\n' "$SEM"; exit 0
  fi
  if ! cmp -s "$P/lsmval" "$A/lsmval.1" && mkdir -p "$A"; then
    n=5; while [ $n -gt 1 ]; do
      [ -f "$A/lsmval.$((n-1))" ] && mv -f "$A/lsmval.$((n-1))" "$A/lsmval.$n"; n=$((n-1))
    done
    cp -p "$P/lsmval" "$A/lsmval.1"
  fi
  printf '%s\n' "$P/lsmval"
  ;;
passo)
  [ $# -eq 3 ] || uso
  pasta "$3"; T1=$(mktemp "$3/.lsmval.XXXXXX") && T2=$(mktemp "$3/.lsmval.XXXXXX") || exit 2
  trap 'rm -f "$T1" "$T2"' EXIT
  um_passo "$2" "$3" "$T1" "$T2"; echo "$R"
  ;;
vigia)
  [ $# -eq 3 ] || [ $# -eq 4 ] || uso
  pasta "$3"; INT=${4:-2}; T1=$(mktemp "$3/.lsmval.XXXXXX") && T2=$(mktemp "$3/.lsmval.XXXXXX") || exit 2
  trap 'rm -f "$T1" "$T2"; exit 0' TERM INT HUP
  echo "$(hora) vigia: $2 -> $3/lsmval, a cada $INT s (a hora é a do relógio de quem roda; no container, UTC)"
  ULT=
  while :; do
    um_passo "$2" "$3" "$T1" "$T2"
    case "$R" in
      guardada*) echo "$(hora) $R"; ULT= ;;
      igual) ULT= ;;
      recusada*|"pela metade"|"não consegui"*) [ "$R" = "$ULT" ] || echo "$(hora) $R; fica a última cópia boa"; ULT=$R ;;
    esac
    sleep "$INT" & wait $!                              # assim o encerramento não espera o intervalo
  done
  ;;
*) uso ;;
esac
