#!/bin/sh
# Teste do canal de entrada do dsistub (Marco 5): comandos escritos num arquivo viram eventos do
# serviço DSIKeyPanel no barramento. Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_entrada.sh <pasta com kipc_hub, dsistub e dsi_escuta>   (a que o m1_build.sh gera)
# Cada caso escreve um comando, espera e confere a mensagem que o dsi_escuta viu na interface 301.
# Bytes esperados: protobuf com inteiros int32 comuns, como a HMI declara (conferido no
# DSIKeyPanelListenerServiceMsg decompilado). O campo de tempo varia, por isso entra como padrão.
BIN=${1:-/work/bin}; T=/tmp/t_entrada; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
printf 'tecla 1 1 1\n' > $T/cmd            # comando antigo: já estava no arquivo antes de o stub subir
pass=0; fail=0
"$BIN/kipc_hub" 2>$T/hub.log & HUB=$!
i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
"$BIN/dsi_escuta" $SOCK 301 > $T/escuta.log 2>&1 & ESC=$!
"$BIN/dsistub" $SOCK 800 480 0 $T/cmd 2>$T/stub.log & STUB=$!
sleep 0.6

TEMPO='([0-9a-f]{2} ){1,5}'
KP='canal=0x80000000000 if=301 inst=0'
confere() {   # confere <nome> <padrão que TEM de aparecer no que foi escutado>
  if grep -qE "$2" $T/escuta.log; then pass=$((pass+1)); echo "PASSOU $1"
  else fail=$((fail+1)); echo "FALHOU $1: não apareceu /$2/"; fi
}
conta() { grep -c "if=301" $T/escuta.log; }

printf 'tecla 1 116 1\n' >> $T/cmd; sleep 0.4
confere tecla_home_pressionada "^$KP api=25 pb=08 01 10 74 18 01 20 ${TEMPO}28 01\$"

printf 'tecla 1 116 0\n' >> $T/cmd; sleep 0.4
confere tecla_home_solta "^$KP api=25 pb=08 01 10 74 18 00 20 ${TEMPO}28 01\$"

printf 'gesto 4 400 240\n' >> $T/cmd; sleep 0.4
confere gesto_pressiona "^$KP api=20 pb=08 0d 10 04 18 01 20 00 28 90 03 30 f0 01 38 00 40 00 48 ${TEMPO}50 01\$"

printf 'gesto 1 400 240 1\n' >> $T/cmd; sleep 0.4
confere gesto_toque_com_contagem "^$KP api=20 pb=08 0d 10 01 18 01 20 00 28 90 03 30 f0 01 38 01 40 00 48 ${TEMPO}50 01\$"

# botão giratório: updateEncoder2 (api 23). Girar para trás é incremento negativo, que no int32
# do protobuf vai com o sinal estendido (dez bytes).
printf 'encoder 13 16 1\n' >> $T/cmd; sleep 0.4
confere botao_giratorio_para_frente "^$KP api=23 pb=08 0d 10 10 18 01 20 ${TEMPO}28 01\$"

printf 'encoder 13 17 -2\n' >> $T/cmd; sleep 0.4
confere botao_giratorio_para_tras "^$KP api=23 pb=08 0d 10 11 18 fe ff ff ff ff ff ff ff ff 01 20 ${TEMPO}28 01\$"

antes=$(conta)
printf 'tecla abc\nqualquer coisa\ntecla 1 2\ngesto 4 10\nencoder 13 16\n' >> $T/cmd; sleep 0.4
depois=$(conta)
if [ "$antes" = "$depois" ]; then pass=$((pass+1)); echo "PASSOU linha_invalida_nao_vira_mensagem"
else fail=$((fail+1)); echo "FALHOU linha_invalida_nao_vira_mensagem: $antes mensagens antes, $depois depois"; fi
printf 'tecla 1 13 1\n' >> $T/cmd; sleep 0.4
confere stub_segue_depois_de_linha_invalida "^$KP api=25 pb=08 01 10 0d 18 01 20 ${TEMPO}28 01\$"

antes=$(conta)
printf 'tecla 1 78' >> $T/cmd; sleep 0.4      # linha pela metade: ainda sem a quebra de linha
meio=$(conta)
printf ' 1\n' >> $T/cmd; sleep 0.4
if [ "$antes" = "$meio" ]; then pass=$((pass+1)); echo "PASSOU linha_incompleta_espera"
else fail=$((fail+1)); echo "FALHOU linha_incompleta_espera: saiu mensagem antes de a linha terminar"; fi
confere linha_completada_sai_inteira "^$KP api=25 pb=08 01 10 4e 18 01 20 ${TEMPO}28 01\$"

if grep -qE "^$KP api=25 pb=08 01 10 01 18 01 " $T/escuta.log; then
  fail=$((fail+1)); echo "FALHOU comando_antigo_nao_repete: o stub mandou o que já estava no arquivo"
else pass=$((pass+1)); echo "PASSOU comando_antigo_nao_repete"; fi

kill $STUB $ESC $HUB 2>/dev/null; wait 2>/dev/null
echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- o que foi escutado ---"; cat $T/escuta.log; echo "--- stub ---"; tail -n 12 $T/stub.log; }
[ $fail -eq 0 ]
