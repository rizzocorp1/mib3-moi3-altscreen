#!/bin/sh
# Teste do arquivo de partida da memória de estado (Marco 6, opção LSMVAL do m1_hmi.sh): o
# lsmval_prepara.sh confere o arquivo e o devolve do jeito que o firmware lê. Não precisa do
# firmware nem do container; roda no Mac ou na imagem, de qualquer pasta:
#   sh referencia/ferramentas/mib3-sandbox/driver/tests/t_memoria.sh
# O formato vem dos dois leitores do firmware: init_pso_file, do persistence-app, e
# DSIPersistenceImpl.initLSMDataInFile, da HMI. Uma linha por valor:
#     <tipo> <namespace> <chave> <bytes em decimal...>
# O leitor da HMI separa os campos por UM espaço e descarta, sem avisar, a linha que tiver dois
# seguidos; o persistence-app descarta namespace 0 e guarda no máximo 500 linhas de 9998 letras.
D=$(cd "$(dirname "$0")/.." && pwd); P="$D/lsmval_prepara.sh"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
# recusa <nome> <trecho esperado na mensagem>: o arquivo $T/e.txt tem de ser recusado por
# inteiro (nada na saída, código 2), com o arquivo e a linha do erro na mensagem
recusa() {
  sh "$P" "$T/e.txt" > "$T/e.out" 2> "$T/e.err"; rc=$?
  if [ $rc -ne 2 ]; then nok "$1" "saiu com $rc, esperava 2"
  elif [ -s "$T/e.out" ]; then nok "$1" "recusou, mas gravou: $(head -n 1 "$T/e.out")"
  elif ! grep -q "$2" "$T/e.err"; then nok "$1" "mensagem sem /$2/: $(cat "$T/e.err")"
  else ok "$1"; fi
}
PT='0 1101 10 112 116 95 66 82 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32'
TAB=$(printf '\t'); CR=$(printf '\r')

# 1. O arquivo que vai na pasta: comentários e linhas em branco saem, a linha de dados fica igual
igual arquivo_da_pasta "$(sh "$P" "$D/../stubs/persistencia/idioma-pt_BR.txt" 2>&1)" "$PT"

cat > "$T/a.txt" <<EOF
# comentário no começo
   # comentário recuado

$PT
0 5000 300 0 1 7 0      # comentário depois dos dados
1 6000 100 42
0 5052 98767470592 255 0 31
EOF
sh "$P" "$T/a.txt" > "$T/a.out" 2> "$T/a.err"; rc=$?
igual varias_linhas_codigo "$rc" 0
igual varias_linhas_saida "$(cat "$T/a.out")" "$PT
0 5000 300 0 1 7 0
1 6000 100 42
0 5052 98767470592 255 0 31"
igual sem_mensagem_quando_esta_certo "$(cat "$T/a.err")" ""

# 2. Espaços a mais, tabulação e fim de linha do Windows: a HMI recusaria; saem normalizados
printf '  0  1101${TAB}10   80 84 %s\n' "$CR" | sed "s/\${TAB}/$TAB/" > "$T/b.txt"
igual espacos_normalizados "$(sh "$P" "$T/b.txt" 2>&1)" "0 1101 10 80 84"

# 3. Só comentários: memória vazia, sem erro
printf '# nada\n\n' > "$T/c.txt"
sh "$P" "$T/c.txt" > "$T/c.out" 2>&1; rc=$?
igual so_comentarios_codigo "$rc" 0
igual so_comentarios_saida "$(cat "$T/c.out")" ""

# 4. Erros: recusa tudo e diz o arquivo e a linha
printf '%s\n0 1101 10\n' "$PT" > "$T/e.txt";            recusa sem_nenhum_byte 'e.txt, linha 2'
printf '%s\n# x\n2 1101 11 5\n' "$PT" > "$T/e.txt";     recusa tipo_desconhecido 'e.txt, linha 3'
printf '0 0 10 1 2\n' > "$T/e.txt";                     recusa namespace_zero 'e.txt, linha 1'
printf '0 1101 dez 1 2\n' > "$T/e.txt";                 recusa chave_nao_numerica 'e.txt, linha 1'
printf '0 1101 10 1 256\n' > "$T/e.txt";                recusa byte_acima_de_255 'e.txt, linha 1'
printf '0 1101 10 1 -3\n' > "$T/e.txt";                 recusa byte_negativo 'e.txt, linha 1'
printf '0 1101 10 0x70\n' > "$T/e.txt";                 recusa byte_em_hexa 'e.txt, linha 1'
printf '1 6000 100 1 2\n' > "$T/e.txt";                 recusa inteiro_com_dois_valores 'e.txt, linha 1'
printf '0 1101 10 1\n0 5000 1 2\n0 1101 10 3\n' > "$T/e.txt"
recusa mesma_chave_duas_vezes 'e.txt, linha 3'
grep -q 'linha 1' "$T/e.err" && ok repetida_diz_onde_estava || nok repetida_diz_onde_estava "$(cat "$T/e.err")"
awk 'BEGIN { for (i = 1; i <= 501; i++) print "0 5000 " i " 1" }' > "$T/e.txt"
recusa mais_de_500_linhas 'e.txt, linha 501'
awk 'BEGIN { printf "0 5000 1"; for (i = 1; i <= 5000; i++) printf " 7"; print "" }' > "$T/e.txt"
recusa linha_longa_demais 'e.txt, linha 1'
rm -f "$T/e.txt";                                       recusa arquivo_que_nao_existe 'e.txt'

# 5. Nos limites ainda passa: 500 linhas, e uma linha de 9998 letras
awk 'BEGIN { for (i = 1; i <= 500; i++) print "0 5000 " i " 1" }' > "$T/l.txt"
igual quinhentas_linhas "$(sh "$P" "$T/l.txt" 2>/dev/null | wc -l | tr -d ' ')" 500
awk 'BEGIN { printf "0 5000 1"; for (i = 1; i <= 4995; i++) printf " 7"; print "" }' > "$T/l.txt"
igual linha_no_limite "$(sh "$P" "$T/l.txt" 2>/dev/null | awk '{ print length($0) }')" 9998

# ---------------------------------------------------------------------------------------------
# Da seção 6 em diante: a memória de uma sessão para a outra (lsmval_guarda.sh e o ao_vivo.sh).
# No modo ao vivo a central guarda a memória de estado numa pasta do Mac enquanto roda, e a sessão
# seguinte nasce dela. Aqui quem faz o papel do firmware é o próprio teste, escrevendo no arquivo
# como o persistence-app escreve (Persistence_WriteBuffer): acrescenta uma linha a cada valor
# novo e regrava o arquivo inteiro, com fopen "w", quando a chave já existe.
G="$D/lsmval_guarda.sh"
SEM="$D/../stubs/persistencia/idioma-pt_BR.txt"     # a semente: o arquivo de partida fixo
M="$T/memoria"; C="$T/central"                      # a pasta do Mac e o /data/lge/hmi da central
L2='0 5000 300 0 1 7 0'; L3='0 5051 4311744512 55 66 77 88 0 72 77 73'
inode() { ls -i "$1" 2>/dev/null | awk '{ print $1 }'; }
sobras() { ls -A "$1" | grep -c '^\.lsmval\.'; }    # cópias de trabalho esquecidas na pasta
passo() { sh "$G" passo "$C/lsmval" "$M" 2>&1; }

# 6. De que arquivo a central nasce: partida <pasta> <semente>
mkdir -p "$M" "$C"
igual partida_sem_memoria_usa_a_semente "$(sh "$G" partida "$M" "$SEM" 2>"$T/p.err")" "$SEM"
igual partida_sem_memoria_nao_avisa "$(cat "$T/p.err")" ""
printf '%s\n%s\n' "$PT" "$L2" > "$M/lsmval"
igual partida_com_memoria_usa_a_guardada "$(sh "$G" partida "$M" "$SEM" 2>"$T/p.err")" "$M/lsmval"
igual partida_com_memoria_nao_avisa "$(cat "$T/p.err")" ""
igual partida_deixa_copia_em_anteriores "$(cat "$M/anteriores/lsmval.1" 2>&1)" "$PT
$L2"
sh "$G" partida "$M" "$SEM" >/dev/null 2>&1
if [ -f "$M/anteriores/lsmval.1" ] && [ ! -e "$M/anteriores/lsmval.2" ]; then ok partida_repetida_nao_gira
else nok partida_repetida_nao_gira "anteriores: $(ls "$M/anteriores" 2>&1 | tr '\n' ' ')"; fi
# seis partidas, cada uma com a memória diferente da anterior: ficam as cinco últimas
for i in 1 2 3 4 5 6; do
  printf '%s\n0 5000 300 %s\n' "$PT" "$i" > "$M/lsmval"; sh "$G" partida "$M" "$SEM" >/dev/null 2>&1
done
igual anteriores_a_mais_nova_e_a_1 "$(tail -n 1 "$M/anteriores/lsmval.1" 2>&1)" "0 5000 300 6"
igual anteriores_a_mais_velha_e_a_5 "$(tail -n 1 "$M/anteriores/lsmval.5" 2>&1)" "0 5000 300 2"
igual anteriores_sao_cinco "$(ls "$M/anteriores" 2>/dev/null | wc -l | tr -d ' ')" 5
# memória guardada com erro (mexida à mão, por exemplo): sai do caminho e a central nasce da semente
printf '%s\n0 5000 300 999\n' "$PT" > "$M/lsmval"
igual partida_memoria_com_erro_cai_na_semente "$(sh "$G" partida "$M" "$SEM" 2>"$T/p.err")" "$SEM"
if grep -q 'linha 2' "$T/p.err" && grep -q 'lsmval.recusada' "$T/p.err"; then ok partida_diz_a_linha_e_onde_ficou
else nok partida_diz_a_linha_e_onde_ficou "$(cat "$T/p.err")"; fi
if [ ! -e "$M/lsmval" ] && [ -f "$M/lsmval.recusada" ]; then ok memoria_com_erro_sai_do_caminho
else nok memoria_com_erro_sai_do_caminho "na pasta: $(ls -A "$M" | tr '\n' ' ')"; fi
: > "$M/.lsmval.abc123"; : > "$M/.lsmval.zzz999"
sh "$G" partida "$M" "$SEM" >/dev/null 2>&1
igual partida_limpa_copias_esquecidas "$(sobras "$M")" 0
sh "$G" partida "$T/nao-existe" "$SEM" > "$T/p.out" 2> "$T/p.err"; rc=$?
igual partida_pasta_que_nao_existe_codigo "$rc" 2
igual partida_pasta_que_nao_existe_saida "$(cat "$T/p.out")" ""
rm -rf "$M"; mkdir -p "$M"
sh "$G" partida "$M" "" > "$T/p.out" 2>&1; rc=$?
igual partida_semente_vazia "$rc:$(cat "$T/p.out")" "0:"

# 7. Um passo de guarda: passo <lsmval da central> <pasta>
igual passo_sem_arquivo_da_central "$(passo)" "sem arquivo"
printf '%s\n%s\n' "$PT" "$L2" > "$C/lsmval"
igual passo_guarda_a_primeira_copia "$(passo)" "guardada: 2 linha(s)"
igual guardada_igual_a_da_central "$(cat "$M/lsmval" 2>&1)" "$(cat "$C/lsmval")"
i1=$(inode "$M/lsmval")
igual passo_sem_mudanca_diz_igual "$(passo)" "igual"
igual passo_sem_mudanca_nao_regrava "$(inode "$M/lsmval")" "$i1"
echo "$L3" >> "$C/lsmval"                               # valor novo: o firmware acrescenta a linha
igual passo_guarda_a_linha_nova "$(passo)" "guardada: 3 linha(s)"
igual guardada_tem_a_linha_nova "$(tail -n 1 "$M/lsmval" 2>&1)" "$L3"
# troca de nome: o guardado passa a ser OUTRO arquivo; quem estava lendo o antigo o lê inteiro
i2=$(inode "$M/lsmval")
if [ -n "$i2" ] && [ "$i2" != "$i1" ]; then ok grava_por_troca_de_nome
else nok grava_por_troca_de_nome "o arquivo foi regravado no lugar (inode $i1 antes, $i2 depois)"; fi
igual passo_nao_deixa_copia_esquecida "$(sobras "$M")" 0
# estado que o conferidor recusa: a cópia boa fica, e o motivo fica anotado na pasta
antes=$(cat "$M/lsmval" 2>&1)
echo '0 5000 400 999' >> "$C/lsmval"
r=$(passo)
case "$r" in "recusada: linha 4: "*"byte 999"*) ok passo_recusa_e_diz_a_linha ;; *) nok passo_recusa_e_diz_a_linha "veio '$r'" ;; esac
igual recusada_nao_troca_a_guardada "$(cat "$M/lsmval" 2>&1)" "$antes"
if grep -q 'linha 4' "$M/recusada.txt" 2>/dev/null; then ok recusa_fica_anotada_na_pasta
else nok recusa_fica_anotada_na_pasta "sem o motivo em recusada.txt"; fi
sh "$G" partida "$M" "$SEM" >/dev/null 2>"$T/p.err"
if grep -q 'linha 4' "$T/p.err"; then ok partida_avisa_que_a_guarda_tinha_parado
else nok partida_avisa_que_a_guarda_tinha_parado "veio '$(cat "$T/p.err")'"; fi
printf '%s\n%s\n%s\n0 5000 400 9\n' "$PT" "$L2" "$L3" > "$C/lsmval"
igual passo_volta_a_guardar "$(passo)" "guardada: 4 linha(s)"
if [ ! -e "$M/recusada.txt" ]; then ok aviso_some_quando_volta_a_guardar
else nok aviso_some_quando_volta_a_guardar "recusada.txt continua na pasta"; fi
antes=$(cat "$M/lsmval" 2>&1)
# vazio: é o que se vê no instante seguinte ao fopen "w" do firmware. Nunca é guardado
: > "$C/lsmval"
igual passo_nao_guarda_arquivo_vazio "$(passo)" "vazia"
igual vazio_nao_troca_a_guardada "$(cat "$M/lsmval" 2>&1)" "$antes"
# cortado no meio da terceira linha: os números que sobraram ainda formam uma linha válida
printf '%s\n%s\n0 5051 4311744512 55 66' "$PT" "$L2" > "$C/lsmval"
igual passo_nao_guarda_arquivo_pela_metade "$(passo)" "pela metade"
igual pela_metade_nao_troca_a_guardada "$(cat "$M/lsmval" 2>&1)" "$antes"
# (já era assim) o conferidor sozinho aceita esse arquivo; por isso o passo exige o fim de linha
if sh "$P" "$C/lsmval" >/dev/null 2>&1; then ok conferidor_sozinho_aceita_a_metade
else nok conferidor_sozinho_aceita_a_metade "o lsmval_prepara.sh passou a recusar; rever o passo"; fi
# arquivo mudando durante o passo (o firmware no meio de uma gravação): fica para o passo seguinte
printf '%s\n%s\n' "$PT" "$L2" > "$C/lsmval"
( i=0; while [ $i -lt 40 ]; do echo "0 5090 $i 1" >> "$C/lsmval"; i=$((i+1)); sleep 0.05; done ) & ESC=$!
sleep 0.1
igual passo_nao_guarda_arquivo_mudando "$(passo)" "mudando"
igual mudando_nao_troca_a_guardada "$(cat "$M/lsmval" 2>&1)" "$antes"
wait $ESC
igual passo_guarda_quando_para_de_mudar "$(passo)" "guardada: 42 linha(s)"

# 8. A vigia repete o passo enquanto a central roda: vigia <lsmval da central> <pasta> [intervalo]
rm -rf "$M" "$C"; mkdir -p "$M" "$C"
sh "$G" vigia "$C/lsmval" "$M" 0.2 > "$T/v.log" 2>&1 & V=$!
chegou() { i=0; while [ $i -lt 60 ] && ! cmp -s "$C/lsmval" "$M/lsmval"; do sleep 0.1; i=$((i+1)); done; cmp -s "$C/lsmval" "$M/lsmval"; }
sleep 0.5                                               # a central ainda não criou o arquivo: a vigia espera
printf '%s\n' "$PT" > "$C/lsmval"
if chegou; then ok vigia_guarda_quando_a_central_aparece; else nok vigia_guarda_quando_a_central_aparece "6 s sem guardar"; fi
echo "$L2" >> "$C/lsmval"
if chegou; then ok vigia_guarda_a_linha_nova; else nok vigia_guarda_a_linha_nova "6 s sem guardar"; fi
printf '%s\n0 5000 300 0 1 9 9\n' "$PT" > "$C/lsmval"     # chave que já existia: o arquivo é regravado inteiro
if chegou; then ok vigia_guarda_a_regravacao; else nok vigia_guarda_a_regravacao "6 s sem guardar"; fi
echo '0 5000 400 999' >> "$C/lsmval"; sleep 2             # recusado por vários intervalos seguidos
igual vigia_avisa_da_recusa_uma_vez_so "$(grep -c 'recusada' "$T/v.log")" 1
igual vigia_registra_cada_copia "$(grep -c 'guardada' "$T/v.log")" 3
kill $V 2>/dev/null; wait $V 2>/dev/null
igual vigia_encerrada_nao_deixa_copia_esquecida "$(sobras "$M")" 0

# 9. Duas sessões seguidas, sem firmware: o que a primeira deixou é o que a segunda recebe
rm -rf "$M" "$C"; mkdir -p "$M" "$C"
nasce() { sh "$P" "$(sh "$G" partida "$M" "$SEM" 2>/dev/null)" > "$C/lsmval"; }   # o que o m1_hmi.sh faz com LSMVAL
nasce
igual sessao_1_nasce_da_semente "$(cat "$C/lsmval")" "$PT"
echo "$L2" >> "$C/lsmval"; echo "$L3" >> "$C/lsmval"      # a HMI manda guardar dois valores
passo >/dev/null
fim1=$(cat "$C/lsmval"); rm -f "$C/lsmval"                # a central desliga: /data é memória e some
nasce
igual sessao_2_nasce_com_o_que_a_1_deixou "$(cat "$C/lsmval")" "$fim1"

# 10. O ao_vivo.sh monta a pasta da memória e liga a guarda. Roda uma CÓPIA do roteiro numa
# bancada de mentira, com um "docker" que só anota o que recebeu. O docker de verdade não é
# chamado e a pasta ao-vivo/ de verdade não é tocada: o roteiro acha a bancada pelo lugar em que
# ele mesmo está. O nome da pasta tem espaço de propósito.
BM="$T/bancada de mentira"; mkdir -p "$BM/driver" "$T/bin"
cp "$D/ao_vivo.sh" "$BM/driver/"
cat > "$T/bin/docker" <<'EOF'
#!/bin/sh
for a in "$@"; do printf '%s\n' "$a"; done > "$DOCKER_VIU"
EOF
chmod +x "$T/bin/docker"
# aovivo [VARIAVEL=valor...]: roda a cópia com essas variáveis; LSMVAL e MEMORIA só as que vierem aqui
aovivo() {
  rm -rf "$BM/ao-vivo" "$T/docker.args"
  ( unset LSMVAL MEMORIA; export DOCKER_VIU="$T/docker.args" SEM_SERVIDOR=1 PATH="$T/bin:$PATH"
    env "$@" sh "$BM/driver/ao_vivo.sh" 5 >/dev/null 2>&1 )
}
tem_arg() { grep -qxF -- "$1" "$T/docker.args" 2>/dev/null; }
monta_memoria() { grep -q ':/live/memoria$' "$T/docker.args" 2>/dev/null; }
desligada() {     # desligada <nome>: a guarda não vai ligada, e a pasta não é montada nem criada
  if [ -s "$T/docker.args" ] && ! grep -q '^MEMORIA=.' "$T/docker.args" && ! monta_memoria && [ ! -e "$BM/ao-vivo/memoria" ]; then ok "$1"
  else nok "$1" "docker recebeu: $(grep -i 'memoria' "$T/docker.args" 2>&1 | tr '\n' ' '); pasta: $(ls -d "$BM/ao-vivo/memoria" 2>&1)"; fi
}
aovivo
if tem_arg "MEMORIA=/live/memoria"; then ok ao_vivo_liga_a_guarda
else nok ao_vivo_liga_a_guarda "docker recebeu: $(grep -i 'memoria' "$T/docker.args" 2>&1 | tr '\n' ' ')"; fi
if tem_arg "$BM/ao-vivo/memoria:/live/memoria"; then ok ao_vivo_monta_a_pasta_da_memoria
else nok ao_vivo_monta_a_pasta_da_memoria "docker recebeu: $(grep -i 'memoria' "$T/docker.args" 2>&1 | tr '\n' ' ')"; fi
if [ -d "$BM/ao-vivo/memoria" ]; then ok ao_vivo_cria_a_pasta_da_memoria; else nok ao_vivo_cria_a_pasta_da_memoria "não criou"; fi
if tem_arg "LSMVAL=/stubs/persistencia/idioma-pt_BR.txt"; then ok ao_vivo_segue_mandando_a_semente
else nok ao_vivo_segue_mandando_a_semente "docker recebeu: $(grep 'LSMVAL' "$T/docker.args" 2>&1)"; fi
# (já era assim) o resto da chamada não muda
falta=
for a in "$BM/ao-vivo/frames:/live/frames" "$BM/ao-vivo/log:/live/log" "$BM/ao-vivo/entrada:/live/entrada" \
         "INPUT=/live/entrada/comandos.txt" moi3-ao-vivo moi3-bancada:1 /driver/m3_ao_vivo.sh 5; do
  tem_arg "$a" || falta="$falta [$a]"
done
igual ao_vivo_resto_da_chamada_igual "$falta" ""
aovivo MEMORIA=;                      desligada memoria_vazia_desliga
aovivo MEMORIA=0;                     desligada memoria_zero_desliga
aovivo LSMVAL=/stubs/outro.txt;       desligada lsmval_a_mao_desliga_a_guarda
if tem_arg "LSMVAL=/stubs/outro.txt"; then ok lsmval_a_mao_vale_sobre_tudo
else nok lsmval_a_mao_vale_sobre_tudo "docker recebeu: $(grep 'LSMVAL' "$T/docker.args" 2>&1)"; fi
aovivo LSMVAL=;                       desligada lsmval_vazio_a_mao_desliga_a_guarda
if tem_arg "LSMVAL="; then ok lsmval_vazio_a_mao_segue_vazio
else nok lsmval_vazio_a_mao_segue_vazio "docker recebeu: $(grep 'LSMVAL' "$T/docker.args" 2>&1)"; fi

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
