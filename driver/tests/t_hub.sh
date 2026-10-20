#!/bin/sh
# Teste do que o kipc_hub entrega a quem se liga depois (11/10/2026).
# O hub guarda as 48 primeiras mensagens comuns do barramento e as entrega a cada leitor novo,
# porque a HMI se liga depois dos daemons. Um anúncio de domínio (interface 9000, o que faz a HMI
# ligar os serviços daquele domínio) que saísse depois da 48ª mensagem nunca chegava a ela: era
# o caso do carifservice do firmware, que anuncia o domínio 16 como 61ª mensagem, depois de
# mais de cinquenta mensagens próprias (Marco 8). O hub passa a guardar também o último anúncio
# de cada canal e a entregá-lo ao leitor novo, depois do cache, quando o cache não entregou
# nenhum anúncio daquele canal. O que este teste fixa:
#   - anúncio que chega depois da 48ª mensagem é reentregue;
#   - uma vez por canal, e não uma por repetição: os stubs repetem o anúncio de segundo em
#     segundo, e uma rajada de anúncios do domínio 16 faz a HMI criar serviços em dobro;
#   - quem já recebia o anúncio pelo cache não passa a receber dois;
#   - o que já era reentregue continua chegando igual, na mesma ordem, antes dos anúncios;
#   - as proteções de sempre valem para o anúncio guardado: mensagem curta demais e mensagem
#     maior que HUB_MAXFWD não passam, e ninguém recebe de volta o que o próprio processo mandou.
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_hub.sh <pasta com kipc_hub, dsi_manda, dsi_escuta e diagstub>   (a do m1_build.sh)
# Um caso precisa de um cliente que escreve e lê com o mesmo pid e de mensagens montadas byte a
# byte; isso é feito com o perl da imagem (o pid do aperto de mão é declarado pelo cliente).
# O dsi_escuta mostra cada mensagem comum assim:
#     canal=0x10 if=9000 inst=0 api=0 pb=57 45 4c 43 4f 4d 45 00
BIN=${1:-/work/bin}; T=/tmp/t_hub; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
conta() { grep -cE -- "$1" "$2"; }
tem() { if grep -qE -- "$2" "$3"; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }
naotem() { if grep -qE -- "$2" "$3"; then nok "$1" "apareceu /$2/: $(grep -E -- "$2" "$3" | head -n 1)"; else ok "$1"; fi; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
manda() { "$BIN/dsi_manda" $SOCK "$@"; }           # manda <canal> <interface> <instância> <api> [protobuf]
# tarde <arquivo> [segundos]: liga um leitor novo, deixa ouvir e desliga
tarde() { "$BIN/dsi_escuta" $SOCK > "$1" 2>&1 & e=$!; sleep "${2:-0.4}"; kill $e 2>/dev/null; wait $e 2>/dev/null; }
hub() {   # hub <registro> [VARIÁVEL=valor…]: um hub novo, com o cache vazio
  r=$1; shift; rm -f $SOCK
  env "$@" "$BIN/kipc_hub" 2>"$r" & HUB=$!
  i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
}
fim_hub() { kill $HUB 2>/dev/null; wait $HUB 2>/dev/null; rm -f $SOCK; }

# cliente.pl <socket> <pid declarado> <mensagem em hexa, ou -> <segundos>: manda a mensagem como
# escritor e, sem fechar, liga-se como leitor com o MESMO pid; imprime em hexa o que chegar
cat > $T/cliente.pl <<'EOF'
use Socket;
my ($caminho, $pid, $hexa, $seg) = @ARGV;
sub liga {
    my $papel = shift;
    socket(my $s, AF_UNIX, SOCK_SEQPACKET, 0) or die "socket: $!";
    connect($s, pack_sockaddr_un($caminho)) or die "sem hub em $caminho: $!";
    send($s, $papel . pack('V', $pid), 0);
    return $s;
}
my $w = liga('W');
if ($hexa ne '-') { send($w, pack('H*', $hexa), 0); select(undef, undef, undef, 0.15); }
my $r = liga('R');
my $fim = time + $seg;
$| = 1;
while (1) {
    my $resta = $fim - time; last if $resta <= 0;
    my $rin = ''; vec($rin, fileno($r), 1) = 1;
    next unless select(my $rout = $rin, undef, undef, 0.1) > 0;
    my $n = recv($r, my $buf, 0x6900, 0);
    last unless defined $n && length $buf;
    print unpack('H*', $buf), "\n";
}
EOF
ANU10='canal=0x10 if=9000 inst=0 api=0 pb=57 45 4c 43 4f 4d 45 00'

# ---- 1. o cache cheio. Leitor ligado desde o começo, para conferir que ao vivo nada muda.
hub $T/hub1.log
"$BIN/dsi_escuta" $SOCK > $T/vivo.log 2>&1 & VIVO=$!
sleep 0.2
: > $T/esperado.txt
comum() { manda 0x20 500 0 "$1" aa; echo "canal=0x20 if=500 inst=0 api=$1 pb=aa" >> $T/esperado.txt; }
k=1; while [ $k -le 3 ]; do comum $k; k=$((k+1)); done
manda 0x40 9000 0 0 a1; echo 'canal=0x40 if=9000 inst=0 api=0 pb=a1' >> $T/esperado.txt   # um anúncio dentro do cache
while [ $k -le 47 ]; do comum $k; k=$((k+1)); done         # com o anúncio são 48: o cache encheu
manda 0x20 500 0 999 aa                                    # a 49ª comum: esta continua de fora
manda 0x10 9000 0 0 "57 45 4c 43 4f 4d 45 00"              # como o carifservice: o anúncio depois da 48ª
for v in 01 02 03 04 05; do manda 0x80 9000 0 0 $v; done   # um stub que repete o anúncio
manda 0x400000 9000 1129071959 4541775 -                   # como o mcu-interpreter: "WELCOME" logo depois da interface, sem instância nem api
manda 0x40 9000 0 0 a2                                     # o canal que já tem anúncio no cache anuncia de novo
sleep 0.2
tarde $T/tarde.log

head -n 48 $T/tarde.log > $T/tarde48.txt
if cmp -s $T/esperado.txt $T/tarde48.txt; then ok cache_das_48_primeiras_chega_igual_e_na_ordem
else nok cache_das_48_primeiras_chega_igual_e_na_ordem "primeira diferença: $(diff $T/esperado.txt $T/tarde48.txt | sed -n '2p;3p' | tr '\n' ' ')"; fi
naotem a_49a_mensagem_comum_continua_de_fora ' api=999 ' $T/tarde.log
tem anuncio_depois_da_48a_e_reentregue "^$ANU10\$" $T/tarde.log
igual anuncio_repetido_chega_uma_vez_so "$(conta '^canal=0x80 if=9000 ' $T/tarde.log)" 1
tem vale_o_ultimo_anuncio_do_canal '^canal=0x80 if=9000 inst=0 api=0 pb=05$' $T/tarde.log
tem anuncio_sem_instancia_nem_api_tambem_e_guardado '^canal=0x400000 if=9000 inst=1129071959 api=4541775 pb=$' $T/tarde.log
# o canal 0x40 já chega pelo cache: se viesse também o anúncio guardado, seriam dois de uma vez
igual canal_com_anuncio_no_cache_nao_vem_em_dobro "$(grep -E '^canal=0x40 if=9000 ' $T/tarde.log | tr '\n' ';')" 'canal=0x40 if=9000 inst=0 api=0 pb=a1;'
igual reentrega_e_o_cache_mais_um_anuncio_por_canal "$(wc -l < $T/tarde.log | tr -d ' ')" 51
# depois do cache, e na ordem em que cada canal anunciou pela primeira vez
igual anuncios_guardados_chegam_depois_do_cache "$(tail -n 3 $T/tarde.log | cut -d' ' -f1 | tr '\n' ' ')" 'canal=0x10 canal=0x80 canal=0x400000 '

tarde $T/tarde2.log
if cmp -s $T/tarde.log $T/tarde2.log; then ok cada_leitor_novo_recebe_a_mesma_reentrega; else nok cada_leitor_novo_recebe_a_mesma_reentrega "a segunda veio com $(wc -l < $T/tarde2.log | tr -d ' ') linhas"; fi

# quem já estava ligado recebeu cada mensagem uma vez, na hora: 48 + a 49ª + 1 + 5 + 1 + 1
igual leitor_ja_ligado_recebe_tudo_uma_vez "$(wc -l < $T/vivo.log | tr -d ' ') linhas, $(conta '^canal=0x80 if=9000 ' $T/vivo.log) do canal 0x80, $(conta ' api=999 ' $T/vivo.log) da 49ª" '57 linhas, 5 do canal 0x80, 1 da 49ª'

# o que vem do microcontrolador de diagnóstico continua chegando antes de tudo (Marco 6). Os três
# bytes de cabeçalho ("cab") são escolha do diagstub e não entram na conferência: o hub não os lê
printf '0600 = 01 02 03 04 05\n' > $T/cod.txt
"$BIN/diagstub" $SOCK $T/cod.txt > $T/diagstub.log 2>&1
sleep 0.2; tarde $T/tarde3.log
head -n 1 $T/tarde3.log > $T/tarde3-primeira.txt
tem diagnostico_continua_chegando_antes_de_tudo '^canal=0x800000000000000 if=1901 cab=[0-9a-f]{2} [0-9a-f]{2} [0-9a-f]{2} did=0600 dados=01 02 03 04 05$' $T/tarde3-primeira.txt
tail -n +2 $T/tarde3.log > $T/tarde3-resto.txt
if cmp -s $T/tarde.log $T/tarde3-resto.txt; then ok depois_do_diagnostico_vem_a_mesma_reentrega; else nok depois_do_diagnostico_vem_a_mesma_reentrega "vieram $(wc -l < $T/tarde3-resto.txt | tr -d ' ') linhas depois da primeira"; fi

# Ninguém recebe de volta o que o próprio processo mandou: vale para o cache e tem de valer para
# o anúncio guardado. O "processo" 4242 anuncia o canal 0x200 e depois se liga como leitor.
ANU200=0002000000000000282300000000000000000000
perl $T/cliente.pl $SOCK 4242 $ANU200 0.5 > $T/mesmo.log 2>&1
tem leitor_do_mesmo_processo_recebe_os_outros_anuncios '^100000000000000028230000000000000000000057454c434f4d4500$' $T/mesmo.log
naotem leitor_nao_recebe_de_volta_o_proprio_anuncio "^$ANU200\$" $T/mesmo.log
tarde $T/tarde4.log
tem outro_leitor_recebe_esse_anuncio '^canal=0x200 if=9000 inst=0 api=0 pb=$' $T/tarde4.log

# Mensagem com menos de 12 bytes de carga derruba a JVM da HMI (proteção de 10/10): o hub não a
# encaminha, e não pode guardá-la como anúncio. Canal 0x800 e interface 9000, com 11 bytes de
# carga (um a menos que o mínimo) e com 8 (só a interface e mais 4 bytes).
perl $T/cliente.pl $SOCK 4243 00080000000000002823000000000000000000 0 > /dev/null 2>&1
perl $T/cliente.pl $SOCK 4243 00080000000000002823000000000000 0 > /dev/null 2>&1
sleep 0.2; tarde $T/tarde5.log
if grep -q '^canal=0x800 ' $T/tarde5.log $T/vivo.log; then nok anuncio_curto_continua_barrado "passou: $(grep -h '^canal=0x800 ' $T/tarde5.log $T/vivo.log | head -n 1)"; else ok anuncio_curto_continua_barrado; fi
tem hub_segue_atendendo_depois_do_anuncio_curto '^canal=0x20 if=500 inst=0 api=1 pb=aa$' $T/tarde5.log
kill $VIVO 2>/dev/null; wait $VIVO 2>/dev/null
fim_hub

# ---- 2. HUB_MAXFWD (opção de teste de 10/10): mensagem maior que o limite não é encaminhada,
# e o anúncio que não é encaminhado também não fica guardado.
hub $T/hub2.log HUB_MAXFWD=30
manda 0x10 9000 0 0 "57 45 4c 43 4f 4d 45 00 01 02 03 04"   # 32 bytes: acima do limite
manda 0x20 9000 0 0 -                                       # 20 bytes: passa
sleep 0.2; tarde $T/maxfwd.log
naotem anuncio_acima_de_HUB_MAXFWD_nao_e_guardado '^canal=0x10 ' $T/maxfwd.log
tem anuncio_dentro_de_HUB_MAXFWD_passa '^canal=0x20 if=9000 inst=0 api=0 pb=$' $T/maxfwd.log
fim_hub

# ---- 3. com o cache por encher nada muda. É o caso do modo ao vivo, em que a HMI se liga
# antes da 48ª mensagem: ela recebe o que já recebia, anúncios repetidos inclusive, nem um a mais.
hub $T/hub3.log
: > $T/esperado3.txt
manda 0x10 9000 0 0 -; echo 'canal=0x10 if=9000 inst=0 api=0 pb=' >> $T/esperado3.txt
for k in 1 2 3; do
  manda 0x4000 9000 0 0 -; echo 'canal=0x4000 if=9000 inst=0 api=0 pb=' >> $T/esperado3.txt
  manda 0x400000 1101 0 1 "08 02 10 00 18 02"; echo 'canal=0x400000 if=1101 inst=0 api=1 pb=08 02 10 00 18 02' >> $T/esperado3.txt
done
sleep 0.2; tarde $T/parcial.log
if cmp -s $T/esperado3.txt $T/parcial.log; then ok com_o_cache_por_encher_a_reentrega_e_a_de_sempre
else nok com_o_cache_por_encher_a_reentrega_e_a_de_sempre "vieram $(wc -l < $T/parcial.log | tr -d ' ') linhas (esperava 7); anúncios do canal 0x4000: $(conta '^canal=0x4000 if=9000 ' $T/parcial.log)"; fi
fim_hub

# ---- 4. muitos canais. A tabela de anúncios tem tamanho fixo: comporta pelo menos 64 canais (o
# canal DSI é um bit de uma palavra de 64). Com mais canais do que cabe, o hub segue de pé, os
# 64 primeiros continuam reentregues e nenhum chega duas vezes.
hub $T/hub4.log
"$BIN/dsi_manda" $SOCK 0x20 500 0 1 aa 1 > /dev/null 2>&1 & ENCHE=$!     # uma mensagem comum por milissegundo
sleep 0.4; kill $ENCHE 2>/dev/null; wait $ENCHE 2>/dev/null
k=1; while [ $k -le 70 ]; do manda $((4294967296 + k)) 9000 0 0 -; k=$((k+1)); done
sleep 0.2; tarde $T/muitos.log 0.6
igual cache_cheio_antes_dos_muitos_canais "$(conta '^canal=0x20 if=500 ' $T/muitos.log)" 48
grep -oE '^canal=0x1000000[0-9a-f]{2} if=9000 ' $T/muitos.log | sort > $T/muitos-canais.txt
n=$(sort -u $T/muitos-canais.txt | wc -l | tr -d ' ')
if [ "$n" -ge 64 ] && [ "$n" = "$(wc -l < $T/muitos-canais.txt | tr -d ' ')" ]; then ok tabela_comporta_64_canais_um_anuncio_de_cada
else nok tabela_comporta_64_canais_um_anuncio_de_cada "$n canais diferentes em $(wc -l < $T/muitos-canais.txt | tr -d ' ') anúncios reentregues"; fi
faltam=0; k=1; while [ $k -le 64 ]; do grep -q "^canal=0x$(printf '%x' $((4294967296 + k))) if=9000 " $T/muitos.log || faltam=$((faltam+1)); k=$((k+1)); done
igual os_64_primeiros_canais_ficam_guardados "$faltam ausentes" '0 ausentes'
manda 0x20 500 0 2 bb; tarde $T/muitos2.log 0.6
if cmp -s $T/muitos.log $T/muitos2.log && kill -0 $HUB 2>/dev/null; then ok hub_segue_de_pe_com_a_tabela_cheia; else nok hub_segue_de_pe_com_a_tabela_cheia "a reentrega mudou ou o hub caiu: $(tail -n 2 $T/hub4.log | tr '\n' ' ')"; fi
fim_hub

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- leitor tardio (fim) ---"; tail -n 6 $T/tarde.log; echo "--- hub ---"; tail -n 6 $T/hub1.log; }
[ $fail -eq 0 ]
