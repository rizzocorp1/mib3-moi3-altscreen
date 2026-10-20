#!/bin/sh
# Teste do caminho de codificação e adaptação (Marco 6): o diagstub lê um arquivo de conjuntos e
# os publica no barramento como o microcontrolador faria (canal PID_DIAG_MCU); o kipc_hub guarda
# essas mensagens e as entrega a quem se ligar depois, ANTES de qualquer outra.
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_codificacao.sh <pasta com kipc_hub, diagstub, dsistub e dsi_escuta>   (a do m1_build.sh)
# Formato de cada mensagem no barramento: [canal:8][cabeçalho:3][identificador:2, o byte alto
# primeiro][dados]. O cabeçalho é 83 60 22, o que o navi-al (alm) do firmware exige; a HMI e o
# persistence-app não o leem. O dsi_escuta mostra assim:
#     canal=0x800000000000000 if=1901 cab=83 60 22 did=0600 dados=20 00 ...
BIN=${1:-/work/bin}; T=/tmp/t_codificacao; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
rm -f $SOCK          # um hub anterior deixa o arquivo do socket, e a espera abaixo o tomaria por pronto
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
DG='canal=0x800000000000000 if=1901 cab=83 60 22'
# escuta <arquivo> [segundos]: liga um leitor novo, deixa ouvir e desliga
escuta() { "$BIN/dsi_escuta" $SOCK > "$1" 2>&1 & e=$!; sleep "${2:-0.4}"; kill $e 2>/dev/null; wait $e 2>/dev/null; }
tem() { if grep -qE "$2" "$3"; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }

"$BIN/kipc_hub" 2>$T/hub.log & HUB=$!
i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
# tráfego comum antes de tudo: o dsistub anuncia o display a cada segundo e isso entra no cache
"$BIN/dsistub" $SOCK 800 480 0 2>$T/dsistub.log & DSI=$!
sleep 1.3
"$BIN/dsi_escuta" $SOCK 1901 > $T/aovivo.log 2>&1 & VIVO=$!     # leitor que já estava ligado
sleep 0.3

cat > $T/base.txt <<'EOF'
# codificação: 25 bytes
0600 = 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 01 02 03 04 05 06 07 08 09
0B12 = 05                  # conjunto de 1 byte: 6 bytes de carga, menos que o hub deixava passar
0b49 = 3078                # hexa colado e identificador em minúsculas também valem

0558 = 00 00 00 00
0600 @0.4+4 = 2            # um campo: byte 0, a partir do bit 4, 4 bits
0558 @1.7+1 = 1
0B49 @0.0+16 = 0x4252      # campo de mais de um byte: o mais significativo primeiro
EOF
"$BIN/diagstub" $SOCK $T/base.txt > $T/stub1.log 2>&1; rc=$?
if [ $rc -eq 0 ]; then ok stub_termina_bem; else nok stub_termina_bem "saiu com $rc: $(tail -n 2 $T/stub1.log)"; fi
tem resumo_diz_o_cabecalho 'cabeçalho 83 60 22$' $T/stub1.log
sleep 0.3
tem leitor_ja_ligado_recebe "^$DG did=0600 dados=20 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 01 02 03 04 05 06 07 08 09\$" $T/aovivo.log

escuta $T/tarde.log
tem leitor_que_chega_depois_recebe "^$DG did=0600 dados=20 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 01 02 03 04 05 06 07 08 09\$" $T/tarde.log
tem conjunto_curto_passa "^$DG did=0b12 dados=05\$" $T/tarde.log
tem ajuste_de_um_bit "^$DG did=0558 dados=00 80 00 00\$" $T/tarde.log
tem ajuste_de_dois_bytes "^$DG did=0b49 dados=42 52\$" $T/tarde.log

ordem=$(grep -oE 'did=[0-9a-f]{4}' $T/tarde.log | tr '\n' ' ')
if [ "$ordem" = "did=0600 did=0b12 did=0b49 did=0558 " ]; then ok ordem_do_arquivo; else nok ordem_do_arquivo "veio '$ordem'"; fi

# o que foi guardado chega antes do cache comum (o anúncio do display, interface 9000)
primeira=$(head -n 1 $T/tarde.log)
case "$primeira" in "$DG did=0600 "*) ok guardado_chega_antes_do_resto ;; *) nok guardado_chega_antes_do_resto "a primeira linha foi '$primeira'" ;; esac
tem cache_comum_continua_chegando '^canal=0x4000 if=9000 ' $T/tarde.log

# mandar de novo um identificador troca o valor guardado, sem duplicar
printf '0B12 = 07\n' > $T/troca.txt
"$BIN/diagstub" $SOCK $T/troca.txt > $T/stub2.log 2>&1
sleep 0.2; escuta $T/tarde2.log
n=$(grep -c 'did=0b12' $T/tarde2.log)
if [ "$n" = 1 ] && grep -q "^$DG did=0b12 dados=07\$" $T/tarde2.log; then ok reenvio_troca_o_valor
else nok reenvio_troca_o_valor "apareceu $n vez(es): $(grep 'did=0b12' $T/tarde2.log | tr '\n' ';')"; fi
tem os_outros_continuam_guardados "^$DG did=0558 dados=00 80 00 00\$" $T/tarde2.log

# dois arquivos: o segundo vale sobre o primeiro (é como se usa: o padrão e depois os ajustes)
printf '0B12 = 05\n0553 = 00 00 00\n' > $T/um.txt
printf '# ajustes\n0553 @2.0+8 = 0x7F\n0B12 = 09\n' > $T/dois.txt
"$BIN/diagstub" $SOCK $T/um.txt $T/dois.txt > $T/stub5.log 2>&1
sleep 0.2; escuta $T/tarde6.log
tem segundo_arquivo_ajusta_o_primeiro "^$DG did=0553 dados=00 00 7f\$" $T/tarde6.log
tem segundo_arquivo_troca_conjunto "^$DG did=0b12 dados=09\$" $T/tarde6.log
printf '0553 @9.0+1 = 1\n' > $T/dois-errado.txt
"$BIN/diagstub" $SOCK $T/um.txt $T/dois-errado.txt > $T/stub6.log 2>&1; rc=$?
if [ $rc -ne 0 ] && grep -q 'dois-errado.txt, linha 1' $T/stub6.log; then ok erro_diz_o_arquivo_e_a_linha
else nok erro_diz_o_arquivo_e_a_linha "rc=$rc: $(tail -n 1 $T/stub6.log)"; fi

# arquivo com erro: nada é enviado e o stub diz a linha
errado() {   # errado <nome> <conteúdo>
  printf '%b' "$2" > $T/errado.txt
  "$BIN/diagstub" $SOCK $T/errado.txt > $T/stub_err.log 2>&1; rc=$?
  sleep 0.2; escuta $T/tarde3.log 0.3
  if [ $rc -ne 0 ] && ! grep -q 'did=0777' $T/tarde3.log && grep -q 'linha' $T/stub_err.log; then ok "$1"
  else nok "$1" "rc=$rc; enviado: $(grep -c 'did=0777' $T/tarde3.log); stub: $(tail -n 1 $T/stub_err.log)"; fi
}
errado hexa_invalido_recusa_o_arquivo      '0777 = 01 02\n0778 = 0G\n'
errado hexa_impar_recusa_o_arquivo         '0777 = 01 02\n0778 = 012\n'
errado ajuste_sem_conjunto_recusa          '0777 = 01 02\n0779 @0.0+1 = 1\n'
errado ajuste_fora_do_conjunto_recusa      '0777 = 01 02\n0777 @2.0+1 = 1\n'
errado valor_que_nao_cabe_recusa           '0777 = 01 02\n0777 @0.0+2 = 4\n'
errado campo_que_passa_do_byte_recusa      '0777 = 01 02\n0777 @0.6+4 = 1\n'
errado linha_sem_sentido_recusa            '0777 = 01 02\nqualquer coisa\n'

"$BIN/diagstub" $SOCK $T/nao-existe.txt > $T/stub_ne.log 2>&1
if [ $? -ne 0 ]; then ok arquivo_ausente_da_erro; else nok arquivo_ausente_da_erro "saiu com 0"; fi

# Mensagem curta de OUTRO canal continua barrada pelo hub (a proteção de 10/10 contra o
# travamento da JVM). A opção -c do diagstub troca o canal, só para este teste: com
# 1 byte de dado a carga tem 6 bytes e não pode passar; com 25, tem 30 e passa.
"$BIN/dsi_escuta" $SOCK > $T/aovivo2.log 2>&1 & VIVO2=$!
sleep 0.3
printf '0777 = 01\n' > $T/curta.txt
printf '0778 = 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18\n' > $T/longa.txt
"$BIN/diagstub" -c 4000 $SOCK $T/curta.txt > $T/stub3.log 2>&1
"$BIN/diagstub" -c 4000 $SOCK $T/longa.txt > $T/stub4.log 2>&1
sleep 0.3
if grep -q '^canal=0x4000 bruto=' $T/aovivo2.log; then nok curta_de_outro_canal_barrada "o hub encaminhou: $(grep '^canal=0x4000 bruto=' $T/aovivo2.log | head -n 1)"
else ok curta_de_outro_canal_barrada; fi
# a longa aparece como mensagem comum; a "interface" são os 4 primeiros bytes da carga: o
# cabeçalho e o byte alto do identificador (83 60 22 07, lidos como número: 0x07226083)
tem longa_de_outro_canal_passa '^canal=0x4000 if=119693443 ' $T/aovivo2.log

# O cabeçalho: 83 60 22 por padrão. A opção -k manda outro (3 bytes em hexa colado), para
# rodada de controle e para quem precisa do formato de antes de 11/10/2026, que era zerado.
Z='canal=0x800000000000000 if=1901'
printf '0A01 = 11 22\n' > $T/cab.txt
"$BIN/diagstub" -k 000000 $SOCK $T/cab.txt > $T/stub7.log 2>&1; rc=$?
sleep 0.2; escuta $T/tarde7.log 0.3
if [ $rc -eq 0 ]; then ok cabecalho_a_pedido_termina_bem; else nok cabecalho_a_pedido_termina_bem "saiu com $rc: $(tail -n 2 $T/stub7.log)"; fi
tem cabecalho_zerado_a_pedido "^$Z cab=00 00 00 did=0a01 dados=11 22\$" $T/tarde7.log
tem resumo_diz_o_cabecalho_pedido 'cabeçalho 00 00 00$' $T/stub7.log
tem os_outros_seguem_com_o_padrao "^$DG did=0558 dados=00 80 00 00\$" $T/tarde7.log
# mandar de novo o mesmo identificador com outro cabeçalho troca a mensagem guardada inteira
"$BIN/diagstub" -k 83602E $SOCK $T/cab.txt > $T/stub8.log 2>&1
sleep 0.2; escuta $T/tarde8.log 0.3
n=$(grep -c 'did=0a01' $T/tarde8.log)
if [ "$n" = 1 ] && grep -q "^$Z cab=83 60 2e did=0a01 dados=11 22\$" $T/tarde8.log; then ok reenvio_troca_tambem_o_cabecalho
else nok reenvio_troca_tambem_o_cabecalho "apareceu $n vez(es): $(grep 'did=0a01' $T/tarde8.log | tr '\n' ';')"; fi
# -k que não seja 3 bytes em hexa: nada é enviado
printf '0A02 = 01 02\n' > $T/cab2.txt
for k in 8360 83602G 8360221 0x8360; do
  "$BIN/diagstub" -k "$k" $SOCK $T/cab2.txt > $T/stub9.log 2>&1; rc=$?
  sleep 0.2; escuta $T/tarde9.log 0.3
  if [ $rc -ne 0 ] && ! grep -q 'did=0a02' $T/tarde9.log && grep -q -- '-k' $T/stub9.log; then ok "cabecalho_invalido_recusa_$k"
  else nok "cabecalho_invalido_recusa_$k" "rc=$rc; enviado: $(grep -c 'did=0a02' $T/tarde9.log); stub: $(tail -n 1 $T/stub9.log)"; fi
done
"$BIN/diagstub" -x 1 $SOCK $T/cab2.txt > $T/stub10.log 2>&1
if [ $? -ne 0 ] && grep -q '^uso:' $T/stub10.log; then ok opcao_desconhecida_mostra_o_uso; else nok opcao_desconhecida_mostra_o_uso "$(tail -n 1 $T/stub10.log)"; fi
# -c e -k juntos, nas duas ordens: no outro canal a "interface" volta a ser 00 00 00 07
"$BIN/diagstub" -c 4000 -k 000000 $SOCK $T/longa.txt > $T/stub11.log 2>&1
"$BIN/diagstub" -k 000001 -c 4000 $SOCK $T/longa.txt > $T/stub12.log 2>&1
sleep 0.3
tem canal_e_cabecalho_juntos '^canal=0x4000 if=117440512 ' $T/aovivo2.log
tem cabecalho_e_canal_juntos '^canal=0x4000 if=117506048 ' $T/aovivo2.log
escuta $T/tarde5.log 0.3
if grep -q 'did=0777\|did=0778' $T/tarde5.log; then nok outro_canal_nao_e_guardado "apareceu como codificação"; else ok outro_canal_nao_e_guardado; fi
# Depois de tantos clientes que entram e saem, o hub tem de seguir atendendo. Antes de 11/10/2026
# um cliente novo herdava o estado do que tinha ocupado a vaga, e o hub travava ou o derrubava.
tem hub_segue_atendendo_depois_de_clientes_curtos "^$DG did=0600 " $T/tarde5.log

kill $VIVO $VIVO2 $DSI $HUB 2>/dev/null; wait 2>/dev/null
rm -f $SOCK
echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- leitor tardio ---"; cat $T/tarde.log; echo "--- stub ---"; cat $T/stub1.log; echo "--- hub ---"; tail -n 8 $T/hub.log; }
[ $fail -eq 0 ]
