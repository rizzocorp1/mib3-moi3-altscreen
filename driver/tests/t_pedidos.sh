#!/bin/sh
# Teste dos pedidos da HMI que o dsistub atende (Marco 8): a assinatura do estado de energia e os
# ajustes genéricos do painel de teclas. Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_pedidos.sh <pasta com kipc_hub, dsistub, dsi_manda e dsi_escuta>   (a que o m1_build.sh gera)
# Quem faz o papel da HMI é o dsi_manda, com o canal 0x8000.
#
# 1. Energia (domínio 0x400000, pedidos na interface 1100, eventos na 1101). A HMI assina com a
#    api 9999 e os atributos em zigue-zague (visto no barramento: "08 02" assina o 1, o estado de
#    energia; "08 06 08 08 08 0e" assina 3, 4 e 7; o 4 é o sinal de ignição). Um fornecedor DSI
#    responde à assinatura na hora, com o valor atual. O dsistub só publicava de segundo em
#    segundo: a HMI ficava até 1 s sem saber que a central estava ligada, e a tela Welcome, que
#    só aparece se o estado "ligado" chegar antes de o aplicativo dela subir, às vezes não vinha.
# 2. Painel de teclas (domínio 0x80000000000, pedidos na 300, eventos na 301). A HMI grava
#    ajustes com setGenericSetting (api 1005: teclado, chave, valor) e os lê com
#    requestGenericSetting (api 1009: teclado, chave); a resposta é genericSettingResponse
#    (api 2000: teclado, chave, valor). Inteiros int32 comuns. Sem resposta, a HMI registra um
#    erro de tempo esgotado a cada 10 s.
BIN=${1:-/work/bin}; T=/tmp/t_pedidos; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
rm -f $SOCK
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
tem() { if grep -qE -- "$2" "$3"; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }
conta() { grep -cE -- "$1" "$2"; }
hmi() { "$BIN/dsi_manda" $SOCK 0x8000 "$@"; }      # hmi <interface> <instância> <api> <protobuf>

"$BIN/kipc_hub" 2>$T/hub.log & HUB=$!
i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
"$BIN/dsi_escuta" $SOCK >> $T/escuta.log 2>&1 & ESC=$!
"$BIN/dsistub" $SOCK 800 480 1 2>$T/stub.log & STUB=$!
sleep 0.5

# ---- energia: resposta na hora. Quatro assinaturas, uma a cada 0,35 s, para cair em pontos
# diferentes do segundo: a publicação periódica sozinha só explicaria uma ou duas.
PW='canal=0x400000 if=1101 inst=0'
prova() {   # prova <atributo em zigue-zague> <api esperada>: quantas das 4 assinaturas tiveram resposta em 0,15 s
  hits=0; k=0
  while [ $k -lt 4 ]; do
    antes=$(conta "^$PW api=$2 " $T/escuta.log)
    hmi 1100 0 9999 "08 $1"; sleep 0.09
    depois=$(conta "^$PW api=$2 " $T/escuta.log)
    [ "$depois" -gt "$antes" ] && hits=$((hits+1))
    sleep 0.2; k=$((k+1))
  done
  echo $hits
}
h=$(prova 02 1)
if [ "$h" = 4 ]; then ok assinatura_do_estado_de_energia_responde_na_hora; else nok assinatura_do_estado_de_energia_responde_na_hora "só $h de 4 assinaturas tiveram resposta em 0,15 s"; fi
h=$(prova 08 4)
if [ "$h" = 4 ]; then ok assinatura_da_ignicao_responde_na_hora; else nok assinatura_da_ignicao_responde_na_hora "só $h de 4 assinaturas tiveram resposta em 0,15 s"; fi
tem estado_de_energia_ligado "^$PW api=1 pb=08 02 10 00 18 02\$" $T/escuta.log
tem ignicao_ligada "^$PW api=4 pb=0a 08 08 01 10 01 18 01 20 00 10 02\$" $T/escuta.log
# três atributos num pedido só, como a HMI manda (3, 4 e 7): o 4 é respondido
sleep 0.3; antes=$(conta "^$PW api=4 " $T/escuta.log)
hmi 1100 0 9999 "08 06 08 08 08 0e"; sleep 0.09
depois=$(conta "^$PW api=4 " $T/escuta.log)
sleep 0.3; a2=$(conta "^$PW api=4 " $T/escuta.log)
hmi 1100 0 9999 "08 06 08 08 08 0e"; sleep 0.09
d2=$(conta "^$PW api=4 " $T/escuta.log)
if [ "$depois" -gt "$antes" ] && [ "$d2" -gt "$a2" ]; then ok ignicao_no_meio_de_varios_atributos; else nok ignicao_no_meio_de_varios_atributos "sem resposta na hora ($antes→$depois, $a2→$d2)"; fi

# ---- câmera de ré: a HMI assina o atributo 7 (updateRVCActive) junto com 3 e 4, e segura TODAS
# as telas da partida até saber que a câmera não está ativa, ou até passarem 6 s
# (ContentProcessorManagerImpl, fase "early RVC"). O stub responde "não ativa, válido": 10 02.
# Só na assinatura: repetir de segundo em segundo faria a HMI reprocessar o aviso à toa.
hmi 1100 0 9999 "08 06 08 08 08 0e"; sleep 0.15
tem camera_de_re_nao_ativa_na_assinatura "^$PW api=7 pb=10 02\$" $T/escuta.log
n1=$(conta "^$PW api=7 " $T/escuta.log); sleep 2.2
n2=$(conta "^$PW api=7 " $T/escuta.log)
# (o n1 conta também as duas assinaturas de "3, 4 e 7" do caso de cima: uma resposta por assinatura)
if [ "$n1" = 3 ] && [ "$n2" = "$n1" ]; then ok camera_de_re_uma_vez_por_assinatura; else nok camera_de_re_uma_vez_por_assinatura "$n1 respostas para 3 assinaturas; $n2 passados 2 s"; fi

# ---- painel de teclas: ajustes genéricos
KP='canal=0x80000000000 if=301'
hmi 300 0 1005 "08 0d 10 6a 18 03"; sleep 0.25
tem gravar_ajuste_devolve_o_valor "^$KP inst=0 api=2000 pb=08 0d 10 6a 18 03\$" $T/escuta.log
n1=$(conta "^$KP inst=0 api=2000 pb=08 0d 10 6a 18 03\$" $T/escuta.log)
hmi 300 0 1009 "08 0d 10 6a"; sleep 0.25
n2=$(conta "^$KP inst=0 api=2000 pb=08 0d 10 6a 18 03\$" $T/escuta.log)
if [ "$n2" = $((n1+1)) ]; then ok ler_ajuste_devolve_o_valor_guardado; else nok ler_ajuste_devolve_o_valor_guardado "respostas com o valor: $n1 antes, $n2 depois"; fi
hmi 300 0 1009 "08 0d 10 70"; sleep 0.25
tem ajuste_nunca_gravado_vale_zero "^$KP inst=0 api=2000 pb=08 0d 10 70\$" $T/escuta.log
hmi 300 0 1005 "08 0d 10 6a 18 20"; sleep 0.2
hmi 300 0 1009 "08 0d 10 6a"; sleep 0.25
n=$(conta "^$KP inst=0 api=2000 pb=08 0d 10 6a 18 20\$" $T/escuta.log)
if [ "$n" = 2 ]; then ok gravar_de_novo_troca_o_valor; else nok gravar_de_novo_troca_o_valor "apareceu $n vez(es) com o valor novo"; fi
hmi 300 0 1009 "08 04 10 6a"; sleep 0.25       # mesma chave, outro teclado (4, o volante)
tem teclados_nao_se_misturam "^$KP inst=0 api=2000 pb=08 04 10 6a\$" $T/escuta.log
hmi 300 0 1005 "08 0d 10 68 18 ff ff ff ff ff ff ff ff ff 01"; sleep 0.25   # valor -1
tem valor_negativo "^$KP inst=0 api=2000 pb=08 0d 10 68 18 ff ff ff ff ff ff ff ff ff 01\$" $T/escuta.log
hmi 300 2 1005 "08 0d 10 6b 18 05"; sleep 0.25
tem resposta_na_instancia_do_pedido "^$KP inst=2 api=2000 pb=08 0d 10 6b 18 05\$" $T/escuta.log
# ---- brilho do display (12/10/2026; domínio 0x4000, pedidos na interface 100, respostas na 101).
# Na tela de ajustes a HMI manda setDisplayBrightness (api 1011: display, porcentagem) e, logo em
# seguida, getDisplayBrightness (api 1022: display). A resposta é getDisplayBrightnessResult (api
# 2012: display, porcentagem). Inteiros int32 comuns; zero não vai no fio (visto no barramento em
# 11/10/2026: "10 32" grava 50 %, "10 64" grava 100 %, e 0 % vai com a mensagem vazia). É da
# resposta que a HMI tira o nível que mostra na tela e que guarda na memória
# (ASLDisplayManagementTarget.processGetDisplayBrightnessResponse); sem ela a escolha não pegava
# e o registro ganhava um tempo esgotado de 10 s a cada partida. O display de mentira devolve o
# que a HMI gravou, e nada mais: a gravação não tem resposta própria (a camada da LG não tem
# mensagem para isso), e leitura sem gravação anterior fica calada, para a HMI não guardar um
# brilho que ninguém escolheu.
DM='canal=0x4000 if=101'
brilhos() { conta "^$DM inst=[0-9]* api=2012 " $T/escuta.log; }
n0=$(brilhos)
hmi 100 0 1022 "-"; sleep 0.25                   # getDisplayBrightness(0), antes de qualquer gravação
if [ "$(brilhos)" = "$n0" ] && grep -q 'brilho do display 0: leitura sem gravação anterior' $T/stub.log; then ok ler_o_brilho_sem_ter_gravado_fica_sem_resposta
else nok ler_o_brilho_sem_ter_gravado_fica_sem_resposta "$(( $(brilhos) - n0 )) resposta(s); no registro: $(grep -c 'leitura sem gravação' $T/stub.log)"; fi
hmi 100 0 1011 "10 4b"; sleep 0.25               # setDisplayBrightness(0, 75)
if [ "$(brilhos)" = "$n0" ] && grep -q 'brilho do display 0 gravado: 75 %' $T/stub.log; then ok gravar_o_brilho_nao_tem_resposta_propria
else nok gravar_o_brilho_nao_tem_resposta_propria "$(( $(brilhos) - n0 )) resposta(s); no registro: $(grep -c 'gravado: 75 %' $T/stub.log)"; fi
hmi 100 0 1022 "-"; sleep 0.25
tem ler_o_brilho_devolve_a_porcentagem_gravada "^$DM inst=0 api=2012 pb=10 4b\$" $T/escuta.log
hmi 100 0 1011 "10 19"; hmi 100 0 1022 "-"; sleep 0.25       # 25 %, e a leitura logo atrás, como a HMI faz
tem gravar_de_novo_troca_o_brilho "^$DM inst=0 api=2012 pb=10 19\$" $T/escuta.log
n=$(conta "^$DM inst=0 api=2012 pb=10 4b\$" $T/escuta.log)
if [ "$n" = 1 ]; then ok brilho_antigo_nao_volta; else nok brilho_antigo_nao_volta "a resposta de 75 % apareceu $n vez(es)"; fi
hmi 100 0 1011 "10 64"; hmi 100 0 1022 "-"; sleep 0.25       # 100 %
tem brilho_de_cem_por_cento "^$DM inst=0 api=2012 pb=10 64\$" $T/escuta.log
hmi 100 0 1011 "-"; hmi 100 0 1022 "-"; sleep 0.25           # 0 %: a mensagem vai vazia, e a resposta também
tem brilho_zero_vai_com_a_mensagem_vazia "^$DM inst=0 api=2012 pb=\$" $T/escuta.log
hmi 100 0 1011 "10 32"; sleep 0.1; hmi 100 2 1022 "-"; sleep 0.25    # 50 %, lido por outra instância
tem resposta_do_brilho_na_instancia_do_pedido "^$DM inst=2 api=2012 pb=10 32\$" $T/escuta.log
hmi 100 0 1011 "08 01 10 0a"; hmi 100 0 1022 "08 01"; sleep 0.25     # display 1 com 10 %
tem cada_display_tem_o_brilho_dele "^$DM inst=0 api=2012 pb=08 01 10 0a\$" $T/escuta.log
n1=$(conta "^$DM inst=0 api=2012 pb=10 32\$" $T/escuta.log)
hmi 100 0 1022 "-"; sleep 0.25
n2=$(conta "^$DM inst=0 api=2012 pb=10 32\$" $T/escuta.log)
if [ "$n2" = $((n1+1)) ]; then ok gravar_num_display_nao_muda_o_outro; else nok gravar_num_display_nao_muda_o_outro "respostas de 50 % para o display 0: $n1 antes, $n2 depois"; fi
n1=$(brilhos)
hmi 100 0 1022 "08 02"; sleep 0.25               # display 2: nunca gravado
if [ "$(brilhos)" = "$n1" ] && grep -q 'brilho do display 2: leitura sem gravação anterior' $T/stub.log; then ok display_nunca_gravado_fica_sem_resposta
else nok display_nunca_gravado_fica_sem_resposta "$(( $(brilhos) - n1 )) resposta(s)"; fi
# os pedidos de display que já eram atendidos continuam: o tamanho da tela (getDisplaySize 1043 -> 2029)
hmi 100 0 1043 "-"; sleep 0.25
tem tamanho_da_tela_continua_respondido "^$DM inst=0 api=2029 pb=08 00 10 a0 06 18 e0 03 20 00\$" $T/escuta.log
if kill -0 $STUB 2>/dev/null; then ok stub_continua_de_pe; else nok stub_continua_de_pe "morreu: $(tail -n 2 $T/stub.log)"; fi
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# ---- com a central "desligada" (estado de energia 0 = não publicar) a assinatura fica sem resposta
: > $T/escuta.log
"$BIN/dsistub" $SOCK 800 480 0 2>$T/stub0.log & STUB=$!
sleep 0.4
hmi 1100 0 9999 "08 02 08 08"; sleep 0.3
n=$(conta ' if=1101 ' $T/escuta.log)
if [ "$n" = 0 ]; then ok central_desligada_nao_responde_energia; else nok central_desligada_nao_responde_energia "$n mensagens de energia"; fi

kill $STUB $ESC $HUB 2>/dev/null; wait 2>/dev/null
rm -f $SOCK
echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- o que foi escutado (fim) ---"; tail -n 15 $T/escuta.log | cut -c1-160; echo "--- stub ---"; tail -n 8 $T/stub.log; }
[ $fail -eq 0 ]
