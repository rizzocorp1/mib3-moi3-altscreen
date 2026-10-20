#!/bin/sh
# Teste do vc_diag_alm (investigação do painel de instrumentos, item 3, 11/10/2026): ele manda de
# novo, com o cabeçalho que o navi-al (alm) entende, os conjuntos que o diagstub publicou no
# canal do microcontrolador de diagnóstico, sem mexer nos dados.
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_vc_diag_alm.sh <pasta com kipc_hub, diagstub, dsistub, dsi_escuta e vc_diag_alm>
# O dsi_escuta mostra cada mensagem desse canal assim:
#     canal=0x800000000000000 if=1901 cab=83 60 22 did=0558 dados=03 80 00 00
# "cab" são os três primeiros bytes da carga; o alm só aceita 83 60 22.
# Desde as 23h25 de 11/10/2026 o diagstub já manda 83 60 22 (mudança do Marco 6, a pedido deste
# item). Para ter o que trocar, o teste pede a ele o cabeçalho de antes com "-k 000000"; a parte 9
# confere que, com o diagstub de hoje, o programa não tem mais nada a fazer.
BIN=${1:-/work/bin}; T=/tmp/t_vc_diag_alm; SOCK=/run/kipc_hub.sock
ZERA="-k 000000"     # o diagstub com o cabeçalho zerado, como era
rm -rf $T; mkdir -p $T
rm -f $SOCK
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
C='canal=0x800000000000000 if=1901'
VELHO="$C cab=00 00 00"; NOVO="$C cab=83 60 22"
escuta() { "$BIN/dsi_escuta" $SOCK 1901 > "$1" 2>&1 & e=$!; sleep "${2:-0.4}"; kill $e 2>/dev/null; wait $e 2>/dev/null; }
tem()  { if grep -qE "$2" "$3"; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }
hub()  { rm -f $SOCK; "$BIN/kipc_hub" 2>$T/hub.log & HUB=$!
         i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done; }
fim_hub() { kill $HUB 2>/dev/null; wait $HUB 2>/dev/null; rm -f $SOCK; }

# os oito conjuntos que o alm usa e a bancada publica, mais dois que ele não usa (0B12 e F187)
cat > $T/base.txt <<'EOF'
0600 = 23 71 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0B12 = 05
0B5A = 01 00 00 00 00 00 00 02
0558 = 03 80 00 00
09CB = 30 78
243F = 00
0E38 = 00 00
0571 = C0 00
F187 = 33 47 35 30 33 35 38 33 32 43 20
0B1D = 00 01 02 03
EOF

# --- 1. controle: sem o programa, quem chega recebe tudo com 00 00 00
hub
"$BIN/dsistub" $SOCK 800 480 0 2>$T/dsistub.log & DSI=$!      # tráfego de outro canal, como na bancada
sleep 0.3
"$BIN/diagstub" $ZERA $SOCK $T/base.txt > $T/stub.log 2>&1
sleep 0.2; escuta $T/antes.log
tem controle_sem_o_programa_vem_zerado "^$VELHO did=0558 dados=03 80 00 00\$" $T/antes.log

# --- 2. depois do diagstub: troca o cabeçalho dos que o alm usa, com os mesmos dados
"$BIN/dsi_escuta" $SOCK 1901 > $T/aovivo.log 2>&1 & VIVO=$!   # leitor que já estava ligado
sleep 0.3
"$BIN/vc_diag_alm" -q 200 $SOCK > $T/prog.log 2>&1; rc=$?
if [ $rc -eq 0 ]; then ok programa_termina_bem; else nok programa_termina_bem "saiu com $rc: $(tail -n 2 $T/prog.log)"; fi
sleep 0.3; escuta $T/depois.log
tem pais_e_navegacao_com_cabecalho_novo "^$NOVO did=0558 dados=03 80 00 00\$" $T/depois.log
tem codificacao_com_cabecalho_novo "^$NOVO did=0600 dados=23 71 00 00 00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00\$" $T/depois.log
tem painel_com_cabecalho_novo "^$NOVO did=0b5a dados=01 00 00 00 00 00 00 02\$" $T/depois.log
tem pais_de_venda_com_cabecalho_novo "^$NOVO did=09cb dados=30 78\$" $T/depois.log
tem conjunto_de_1_byte_com_cabecalho_novo "^$NOVO did=243f dados=00\$" $T/depois.log
tem gnss_com_cabecalho_novo "^$NOVO did=0e38 dados=00 00\$" $T/depois.log
tem pso_com_cabecalho_novo "^$NOVO did=0571 dados=c0 00\$" $T/depois.log
tem aparelhos_com_cabecalho_novo "^$NOVO did=0b1d dados=00 01 02 03\$" $T/depois.log
tem o_que_o_alm_nao_usa_fica_como_veio "^$VELHO did=0b12 dados=05\$" $T/depois.log
tem identificacao_fica_como_veio "^$VELHO did=f187 dados=33 47 35 30 33 35 38 33 32 43 20\$" $T/depois.log
n=$(grep -c ' did=' $T/depois.log)
if [ "$n" = 10 ]; then ok nenhum_conjunto_duplicado; else nok nenhum_conjunto_duplicado "quem chega recebe $n, eram 10"; fi
ordem=$(grep -oE 'did=[0-9a-f]{4}' $T/depois.log | tr '\n' ' ')
if [ "$ordem" = "did=0600 did=0b12 did=0b5a did=0558 did=09cb did=243f did=0e38 did=0571 did=f187 did=0b1d " ]; then ok ordem_mantida
else nok ordem_mantida "veio '$ordem'"; fi
sleep 0.2; kill $VIVO 2>/dev/null; wait $VIVO 2>/dev/null
tem leitor_ja_ligado_recebe_a_nova "^$NOVO did=0558 dados=03 80 00 00\$" $T/aovivo.log
if grep -q "^$NOVO did=0b12" $T/aovivo.log; then nok leitor_ja_ligado_nao_recebe_o_resto "o 0B12 foi mandado de novo"; else ok leitor_ja_ligado_nao_recebe_o_resto; fi
tem registro_diz_o_que_trocou '0558 \(4 bytes\): 03 80 00 00' $T/prog.log
tem registro_resume 'recebidos 10 conjuntos; mandados de novo 8' $T/prog.log

# --- 3. de novo: não há o que trocar, e nada é mandado
"$BIN/dsi_escuta" $SOCK 1901 > $T/aovivo2.log 2>&1 & VIVO=$!
sleep 0.3
"$BIN/vc_diag_alm" -q 200 $SOCK > $T/prog2.log 2>&1; rc=$?
sleep 0.3; kill $VIVO 2>/dev/null; wait $VIVO 2>/dev/null
vivos=$(sed '1,10d' $T/aovivo2.log | grep -c ' did=')      # as 10 primeiras são a entrega de quem chega
if [ $rc -eq 0 ] && [ "$vivos" = 0 ] && grep -q 'mandados de novo 0' $T/prog2.log; then ok segunda_vez_nao_manda_nada
else nok segunda_vez_nao_manda_nada "rc=$rc, mandou $vivos: $(tail -n 1 $T/prog2.log)"; fi

# --- 4. valor de teste entra pelo diagstub, como sempre; o programa só acompanha
printf '0558 = 0D 80 00 00\n' > $T/troca.txt
"$BIN/diagstub" $ZERA $SOCK $T/troca.txt > $T/stub2.log 2>&1
sleep 0.2; escuta $T/troca1.log
tem diagstub_de_novo_volta_ao_cabecalho_zerado "^$VELHO did=0558 dados=0d 80 00 00\$" $T/troca1.log
"$BIN/vc_diag_alm" -q 200 $SOCK > $T/prog3.log 2>&1
sleep 0.2; escuta $T/troca2.log
tem programa_acompanha_o_valor_novo "^$NOVO did=0558 dados=0d 80 00 00\$" $T/troca2.log
tem so_o_que_mudou_e_mandado 'mandados de novo 1' $T/prog3.log
kill $DSI 2>/dev/null; fim_hub

# --- 5. ligado ANTES do diagstub (é como roda na bancada): espera a remessa e só então manda
hub
"$BIN/vc_diag_alm" -q 300 $SOCK > $T/prog4.log 2>&1 & P=$!
sleep 0.5
"$BIN/diagstub" $ZERA $SOCK $T/base.txt > $T/stub3.log 2>&1
wait $P; rc=$?
sleep 0.2; escuta $T/cedo.log
if [ $rc -eq 0 ]; then ok ligado_antes_termina_bem; else nok ligado_antes_termina_bem "saiu com $rc: $(tail -n 2 $T/prog4.log)"; fi
tem ligado_antes_troca_o_cabecalho "^$NOVO did=0558 dados=03 80 00 00\$" $T/cedo.log
tem ligado_antes_deixa_o_resto "^$VELHO did=0b12 dados=05\$" $T/cedo.log
n=$(grep -c ' did=' $T/cedo.log)
if [ "$n" = 10 ]; then ok ligado_antes_nao_duplica; else nok ligado_antes_nao_duplica "quem chega recebe $n, eram 10"; fi

# --- 6. -a: todos os conjuntos (o que mudaria se o diagstub mandasse o cabeçalho novo)
"$BIN/vc_diag_alm" -a -q 200 $SOCK > $T/prog5.log 2>&1
sleep 0.2; escuta $T/todos.log
tem todos_inclui_o_que_o_alm_nao_usa "^$NOVO did=0b12 dados=05\$" $T/todos.log
tem todos_inclui_a_identificacao "^$NOVO did=f187 dados=33 47 35 30 33 35 38 33 32 43 20\$" $T/todos.log
if grep -q "^$VELHO" $T/todos.log; then nok todos_nao_deixa_nenhum_zerado "$(grep -c "^$VELHO" $T/todos.log) ficaram"; else ok todos_nao_deixa_nenhum_zerado; fi
tem todos_so_manda_os_que_faltavam 'mandados de novo 2' $T/prog5.log
fim_hub

# --- 7. sem conjunto nenhum: desiste no prazo e diz por quê
hub
"$BIN/vc_diag_alm" -e 400 $SOCK > $T/prog6.log 2>&1; rc=$?
if [ $rc -eq 1 ] && grep -q 'nenhum conjunto' $T/prog6.log; then ok sem_conjunto_desiste; else nok sem_conjunto_desiste "rc=$rc: $(tail -n 1 $T/prog6.log)"; fi
sleep 0.2; escuta $T/vazio.log 0.3
if grep -q ' did=' $T/vazio.log; then nok sem_conjunto_nao_manda_nada "mandou $(grep -c ' did=' $T/vazio.log)"; else ok sem_conjunto_nao_manda_nada; fi
fim_hub

# --- 9. com o diagstub de hoje (83 60 22 de fábrica) não sobra nada para trocar
hub
"$BIN/diagstub" $SOCK $T/base.txt > $T/stub4.log 2>&1
sleep 0.2
"$BIN/vc_diag_alm" -a -q 200 $SOCK > $T/prog9.log 2>&1; rc=$?
sleep 0.2; escuta $T/hoje.log
if [ $rc -eq 0 ] && grep -q 'mandados de novo 0' $T/prog9.log; then ok diagstub_de_hoje_nao_deixa_nada_a_trocar
else nok diagstub_de_hoje_nao_deixa_nada_a_trocar "rc=$rc: $(tail -n 1 $T/prog9.log)"; fi
if grep -q "^$VELHO" $T/hoje.log; then nok diagstub_de_hoje_manda_o_cabecalho_novo "$(grep -c "^$VELHO" $T/hoje.log) vieram zerados"
else ok diagstub_de_hoje_manda_o_cabecalho_novo; fi
n=$(grep -c "^$NOVO did=" $T/hoje.log)
if [ "$n" = 10 ]; then ok diagstub_de_hoje_entrega_os_dez; else nok diagstub_de_hoje_entrega_os_dez "quem chega recebe $n com o cabeçalho novo, eram 10"; fi
fim_hub

# --- 8. sem hub e sem argumento
"$BIN/vc_diag_alm" $SOCK > $T/prog7.log 2>&1; rc=$?
if [ $rc -eq 1 ] && grep -q 'sem hub' $T/prog7.log; then ok sem_hub_da_erro; else nok sem_hub_da_erro "rc=$rc: $(tail -n 1 $T/prog7.log)"; fi
"$BIN/vc_diag_alm" > $T/prog8.log 2>&1; rc=$?
if [ $rc -eq 2 ]; then ok sem_argumento_mostra_o_uso; else nok sem_argumento_mostra_o_uso "rc=$rc"; fi

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- programa ---"; cat $T/prog.log; echo "--- depois ---"; cat $T/depois.log; echo "--- hub ---"; tail -n 8 $T/hub.log; }
[ $fail -eq 0 ]
