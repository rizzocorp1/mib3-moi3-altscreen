#!/bin/sh
# Teste do telefone de mentira (item 7 do roteiro do simulador, 12/10/2026): o esquema dos serviços
# de telefone e conectividade, gerado das classes do firmware, e o que o veiculostub faz com ele e
# com os perfis de stubs/telefone/. É o mesmo programa do carro de mentira, com outro esquema e
# outros domínios: PID_BT (256, canal 0x100: Bluetooth, aparelho, agenda e mensagens) e
# PID_DCMANAGER (4096, canal 0x1000: o gerente de aparelhos).
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_telefone.sh <pasta com kipc_hub, veiculostub, dsi_manda e dsi_escuta> [<pasta stubs/telefone>]
# Quem faz o papel da HMI é o dsi_manda, com o canal 0x8000. Os bytes dos pedidos são os que a HMI
# mandou nas rodadas "ambos" e "e1" de 12/10/2026 (traces/tel_*); os números de interface, de api e
# de atributo esperados foram lidos à parte, no código da camada da LG (DSIBluetoothImpl,
# DSIMobileEquipmentImpl, DSIPhoneManagerImpl, ListenerWorkerThread.processBTListenerService) e
# nas interfaces org.dsi.ifc (decompilados), e não tirados do esquema.
# Não há aparelho nem Bluetooth de verdade: são só os estados que a tela mostra.
BIN=${1:-/work/bin}; TEL=${2:-/stubs/telefone}; T=/tmp/t_telefone; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
rm -f $SOCK
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
tem() { if grep -qE -- "$2" "$3" 2>/dev/null; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }
naotem() { if grep -qE -- "$2" "$3" 2>/dev/null; then nok "$1" "apareceu /$2/: $(grep -E -- "$2" "$3" | head -n 1)"; else ok "$1"; fi; }
conta() { grep -cE -- "$1" "$2" 2>/dev/null; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperava $3, veio $2"; fi; }
hmi() { "$BIN/dsi_manda" $SOCK 0x8000 "$@"; }      # hmi <interface> <instância> <api> <protobuf>
campo1() { sed -n "/^M $1\$/{n;p;}" $E 2>/dev/null; }   # a primeira linha de campo de uma mensagem

G=$TEL/gerado; E=$G/esquema.txt; K=$G/constantes.txt

# ---- o esquema: os serviços que o anúncio de cada domínio liga, com as interfaces do código da LG
tem esquema_bluetooth '^S DSIBluetooth 256 1 2 bluetooth$' $E
# o DSIMobileEquipment manda os pedidos por um método auxiliar (messageToBytes), na interface 1001
tem esquema_aparelho '^S DSIMobileEquipment 256 1001 1002 telephoneng$' $E
tem esquema_agenda '^S DSIAdb 256 1201 1202 addressbook$' $E
tem esquema_lista_da_agenda '^S DSIAdbList 256 1207 1208 addressbook$' $E
tem esquema_perfil_da_agenda '^S DSIAdbUserProfile 256 1209 1210 addressbook$' $E
tem esquema_mensagens '^S DSIMessaging 256 1101 1102 messaging$' $E
tem esquema_ajustes_das_mensagens '^S DSIMessagingServiceConfiguration 256 1103 1104 messaging$' $E
# o gerente de aparelhos pede e recebe na mesma interface
tem esquema_gerente_de_aparelhos '^S DSIPhoneManager 4096 600 600 phonemanager$' $E
# o despacho do domínio 256 entrega também mídia, UPnP e busca, que são de outros donos (os
# pedidos deles vão para 0x800000 e 0x100000): não podem entrar como serviços deste esquema
naotem esquema_sem_servicos_de_midia '^S DSIMedia' $E
naotem esquema_sem_servicos_de_upnp '^S DSIUPNP' $E
naotem esquema_sem_servicos_de_busca '^S DSISearch' $E
# só entra serviço que o anúncio liga: 16 interfaces no domínio 256 e 2 no 4096
igual esquema_tem_os_18_servicos_que_os_anuncios_ligam "$(conta '^S ' $E)" 18
igual esquema_dominio_256_tem_16_servicos "$(conta '^S [A-Za-z]+ 256 ' $E)" 16
igual esquema_dominio_4096_tem_2_servicos "$(conta '^S [A-Za-z]+ 4096 ' $E)" 2
# o serviço do CarPlay (domínio 512) fica FORA do esquema padrão: está num esquema à parte, opcional
naotem esquema_padrao_sem_o_carplay '^S DSICarplay2 ' $E
semw=""
for s in $(grep '^S ' $E 2>/dev/null | cut -d' ' -f2); do grep -q "^W [0-9]* org\.dsi\.ifc\.[a-z0-9]*\.$s [0-9]" $E || semw="$semw $s"; done
igual todo_servico_do_esquema_e_ligado_por_um_anuncio "$semw" ""
tem anuncio_liga_o_bluetooth '^W 256 org\.dsi\.ifc\.bluetooth\.DSIBluetooth 0 0$' $E
tem anuncio_liga_o_aparelho_principal '^W 256 org\.dsi\.ifc\.telephoneng\.DSIMobileEquipment 0 0$' $E
tem anuncio_liga_o_aparelho_associado '^W 256 org\.dsi\.ifc\.telephoneng\.DSIMobileEquipment 1 0$' $E
tem anuncio_liga_o_aparelho_do_carplay '^W 256 org\.dsi\.ifc\.telephoneng\.DSIMobileEquipment 2 0$' $E
tem anuncio_liga_o_gerente_de_aparelhos '^W 4096 org\.dsi\.ifc\.phonemanager\.DSIPhoneManager 0 0$' $E
igual anuncio_do_256_liga_27_instancias "$(conta '^W 256 ' $E)" 27
igual anuncio_do_4096_liga_2_instancias "$(conta '^W 4096 ' $E)" 2
# atributos (constantes ATTR_ das interfaces org.dsi.ifc)
tem atributo_estado_do_bluetooth '^A DSIBluetooth 3 ATTR_BTSTATE$' $E
tem atributo_estado_de_ativacao '^A DSIMobileEquipment 4 ATTR_ACTIVATIONSTATE$' $E
tem atributo_dados_do_aparelho '^A DSIMobileEquipment 20 ATTR_PHONEINFORMATION$' $E
igual atributos_do_bluetooth "$(conta '^A DSIBluetooth ' $E)" 15
igual atributos_do_aparelho "$(conta '^A DSIMobileEquipment ' $E)" 70
tem atributo_aparelhos_conhecidos '^A DSIPhoneManager 2 ATTR_KNOWNDEVICES$' $E
tem atributo_estado_da_agenda '^A DSIAdb 1 ATTR_ADBSTATE$' $E
# eventos: a atualização de cada atributo, com a api de mesmo número
tem evento_estado_do_bluetooth '^E DSIBluetooth 3 3 3 updateBTState DSIBluetoothListenerMsg\.updateBTState$' $E
tem evento_estado_de_ativacao '^E DSIMobileEquipment 4 4 4 updateActivationState DSIMobileEquipmentListenerMsg\.updateActivationState$' $E
tem evento_aparelhos_conhecidos '^E DSIPhoneManager 2 2 2 updateKnownDevices DSIPhoneManagerListenerMsg\.updateKnownDevices$' $E
# na agenda e nas mensagens o ouvinte acha os assinantes por outro método (getUpdateListenerList
# com a instância do serviço, e não com o número dela); sem ler essa forma, as atualizações desses
# serviços saem sem atributo e nenhuma assinatura deles é respondida
tem evento_estado_da_agenda_e_de_atributo '^E DSIAdb 1 1 1 updateAdbState DSIAdbListenerMsg\.updateAdbState$' $E
tem evento_perfil_ativo_e_de_atributo '^E DSIAdbUserProfile 5 5 5 ' $E
tem evento_contas_de_mensagens_e_de_atributo '^E DSIMessaging 5 5 5 ' $E
tem evento_avisos_de_sms_e_de_atributo '^E DSIMessagingServiceConfiguration 11 11 11 ' $E
igual eventos_de_atributo_da_agenda "$(conta '^E DSIAdb [0-9]+ [0-9]+ ' $E)" 7
igual eventos_de_atributo_do_aparelho "$(conta '^E DSIMobileEquipment [0-9]+ [0-9]+ ' $E)" 70
# pedidos do aparelho: a api é o primeiro argumento de cada chamada ao método auxiliar
tem pedido_assinar_do_aparelho '^P DSIMobileEquipment 9999 setNotification DSIMobileEquipmentMsg\.setNotification$' $E
tem pedido_desassinar_do_aparelho '^P DSIMobileEquipment 9998 clearNotification DSIMobileEquipmentMsg\.clearNotification$' $E
tem pedido_atender '^P DSIMobileEquipment 1000 acceptCall DSIMobileEquipmentMsg\.acceptCall$' $E
tem pedido_discar '^P DSIMobileEquipment 1005 dialNumber DSIMobileEquipmentMsg\.dialNumber$' $E
tem pedido_idioma_do_aparelho '^P DSIMobileEquipment 1029 requestSetLanguage DSIMobileEquipmentMsg\.requestSetLanguage$' $E
tem pedido_estatisticas_de_chamadas '^P DSIMobileEquipment 1057 requestSetIntelligentCallStatisticsConfiguration ' $E
igual pedidos_do_aparelho "$(conta '^P DSIMobileEquipment ' $E)" 63
tem pedido_modo_visivel '^P DSIBluetooth 1000 setAccessibleMode DSIBluetoothMsg\.setAccessibleMode$' $E
tem pedido_idioma_da_agenda '^P DSIAdb 1000 setLanguage DSIAdbMsg\.setLanguage$' $E
# a assinatura do aparelho e do Bluetooth vai em zigue-zague ("08 06" assina o atributo 3); a do
# gerente de aparelhos, em varint comum ("08 01 08 07 08 02 08 04 08 08" são 1, 7, 2, 4 e 8)
igual assinatura_do_bluetooth_em_zigue_zague "$(campo1 'DSIBluetoothMsg\.setNotification')" "C 1 attributes sint32 rep"
igual assinatura_do_aparelho_em_zigue_zague "$(campo1 'DSIMobileEquipmentMsg\.setNotification')" "C 1 attributes sint32 rep"
igual assinatura_do_gerente_em_varint_comum "$(campo1 'DSIPhoneManagerMsg\.setNotification')" "C 1 attributes int32 rep"
# as constantes que o perfil usa (org.dsi.ifc.bluetooth.DSIBluetooth e telephoneng.DSIMobileEquipment)
tem constante_bluetooth_ligado '^K DSIBluetooth BTSTATE_ON 0$' $K
tem constante_bluetooth_nao_funcional '^K DSIBluetooth BTSTATE_NOT_FUNCTIONAL 2$' $K
tem constante_aparelho_nao_conectado '^K DSIMobileEquipment ACTIVATIONSTATE_NOT_ATTACHED 1$' $K
tem constante_aparelho_ligado '^K DSIMobileEquipment ACTIVATIONSTATE_PHONE_ON 5$' $K
tem constante_bateria_desconhecida '^K DSIMobileEquipment MEBATTERYCHARGELEVEL_UNKNOWN 255$' $K
tem constante_bateria_vazia '^K DSIMobileEquipment MEBATTERYCHARGELEVEL_0 0$' $K
tem constante_microfone_sem_mudo '^K DSIMobileEquipment MICMUTESTATE_OFF 1$' $K

# ---- os vetores de prova: o veiculostub tem de serializar como as classes do firmware
if [ -f $G/vetores-perfil.txt ] && [ -f $G/vetores-esperado.txt ]; then
  "$BIN/veiculostub" -m $E $G/vetores-perfil.txt > $T/vetores.log 2>$T/vetores.err; rc=$?
  total=$(grep -c '^canal=' $G/vetores-esperado.txt)
  grep '^canal=' $G/vetores-esperado.txt | sort > $T/esperado.ord
  grep '^canal=' $T/vetores.log | sort > $T/obtido.ord
  iguais=$(comm -12 $T/esperado.ord $T/obtido.ord | wc -l | tr -d ' ')
  if [ $rc -eq 0 ] && [ "$total" -gt 250 ] && [ "$iguais" = "$total" ]; then ok "vetores_do_firmware ($iguais de $total mensagens iguais, byte a byte)"
  else nok vetores_do_firmware "rc=$rc; $iguais de $total iguais; primeira diferença: $(comm -23 $T/esperado.ord $T/obtido.ord | head -n 1 | cut -c1-160) / erro: $(tail -n 1 $T/vetores.err)"; fi
else
  nok vetores_do_firmware "não achei os vetores em $G"
fi

# ---- os perfis da pasta: conferem, dizem que são simulação e não trazem dado de ninguém
achou=0
for p in "$TEL"/*.txt; do
  [ -f "$p" ] || continue
  achou=1; b=$(basename "$p")
  e=$E; [ "$b" = carplay-sem-chamada.txt ] && e=$TEL/gerado-carplay/esquema.txt   # o perfil do CarPlay pede o esquema opcional
  "$BIN/veiculostub" -n $e "$p" > $T/pp.log 2>&1
  if [ $? -eq 0 ]; then ok "perfil_da_pasta_confere: $b"; else nok "perfil_da_pasta_confere: $b" "$(tail -n 1 $T/pp.log)"; fi
  tem "perfil_da_pasta_diz_que_e_simulacao: $b" 'SIMULAÇÃO' "$p"
  # texto entre aspas (nome de aparelho, de contato, número) só com a marca de fictício na própria linha
  ruins=$(grep -n '=.*"' "$p" | grep -v '^[0-9]*:[[:space:]]*#' | grep -vi 'fict' | head -n 3 | tr '\n' '|')
  igual "perfil_da_pasta_todo_texto_e_marcado_como_ficticio: $b" "$ruins" ""
done
[ $achou = 1 ] || nok perfil_da_pasta_confere "não há perfil em $TEL"
P=$TEL/sem-aparelho.txt
tem perfil_sem_aparelho_diz_nao_conectado '^activationState\.telActivationState *= *1( |$)' $P
naotem perfil_sem_aparelho_nao_traz_texto '^[^#]*=.*"' $P

sobe() {   # sobe <registro do stub> [<perfil>...]: hub, escuta (em $T/escuta.log) e o stub com o esquema do telefone
  reg=$1; shift
  rm -f $SOCK
  "$BIN/kipc_hub" > $T/hub.log 2>&1 & HUB=$!
  i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
  "$BIN/dsi_escuta" $SOCK > $T/escuta.log 2>&1 & ESC=$!
  sleep 0.3
  "$BIN/${PROG:-veiculostub}" $SOCK $E "$@" > "$reg" 2>&1 & STUB=$!
  sleep 1.2
}
desce() { kill $STUB $ESC $HUB 2>/dev/null; wait $STUB $ESC $HUB 2>/dev/null; rm -f $SOCK; }
BT="canal=0x100 if=2 inst=0"; ME="canal=0x100 if=1002"; PM="canal=0x1000 if=600 inst=0"

if [ -f "$P" ] && [ -f "$E" ]; then
  # ---- o telefone de mentira no barramento, com o perfil da pasta: nenhum aparelho conectado
  sobe $T/stub.log "$P"
  igual anuncia_o_dominio_256_uma_vez "$(conta '^canal=0x100 if=9000 ' $T/escuta.log)" 1
  igual anuncia_o_dominio_4096_uma_vez "$(conta '^canal=0x1000 if=9000 ' $T/escuta.log)" 1

  # o estado do Bluetooth: "08 06" é o atributo 3, como a HMI mandou. A resposta tem de ser
  # "ligado" (0, que não vai no fio) e válida (validFlag 1 = "10 02"): é isso que tira a tecla
  # Telefone do aviso "Telefone indisponível." (AslTargetSystemFeaturesAndCoding.dsiBluetoothUpdateBTState)
  hmi 1 0 9999 "08 06"; sleep 0.3
  tem bluetooth_ligado_e_valido "^$BT api=3 pb=10 02\$" $T/escuta.log

  # o estado de ativação do aparelho: "08 08" é o atributo 4. Não conectado (1) e válido, na
  # instância que pediu: { 1: { 1: 1 }, 2: 1 } = "0a 02 08 02 10 02". O zero (INIT) a HMI ignora
  hmi 1001 0 9999 "08 08"; sleep 0.3
  tem aparelho_principal_nao_conectado "^$ME inst=0 api=4 pb=0a 02 08 02 10 02\$" $T/escuta.log
  hmi 1001 1 9999 "08 08"; sleep 0.3
  tem aparelho_associado_nao_conectado "^$ME inst=1 api=4 pb=0a 02 08 02 10 02\$" $T/escuta.log

  # a primeira assinatura do aparelho na rodada "ambos": dez atributos de uma vez, e uma atualização
  # para cada um, todas válidas
  n0=$(wc -l < $T/escuta.log)
  hmi 1001 0 9999 "08 08 08 28 08 04 08 10 08 12 08 18 08 1e 08 24 08 32 08 34"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$ME inst=0 " > $T/dez.log
  igual dez_atributos_do_aparelho_dez_atualizacoes "$(wc -l < $T/dez.log | tr -d ' ')" 10
  igual atualizacoes_do_aparelho_na_ordem_pedida "$(sed 's/.* api=\([0-9]*\) .*/\1/' $T/dez.log | tr '\n' ' ')" "4 20 2 8 9 12 15 18 25 26 "
  tem dados_do_aparelho_vazios_e_validos "^$ME inst=0 api=20 pb=10 02\$" $T/dez.log
  tem lista_de_chamadas_vazia_e_valida "^$ME inst=0 api=9 pb=10 02\$" $T/dez.log
  # zero nem sempre é neutro. O mudo do microfone em 0 é LIGADO (MICMUTESTATE_ON); a HMI pede
  # "desligado" (1) na partida, e o perfil diz o mesmo
  tem microfone_sem_mudo "^$ME inst=0 api=18 pb=08 02 10 02\$" $T/dez.log
  # e o nível de bateria em 0 é bateria VAZIA: com ele a HMI abre o aviso
  # PO_INFO_BLUETOOTH_BATTERY_WARNING aos 12,8 s da partida (série "s", par 1; a regra está em
  # TargetASLPhoneGetter). Sem aparelho o nível é 255, MEBATTERYCHARGELEVEL_UNKNOWN, que a HMI não
  # mostra. "08 0e" é o atributo 7; 255 em zigue-zague é 510, "fe 03"
  hmi 1001 0 9999 "08 0e"; sleep 0.3
  tem bateria_do_aparelho_desconhecida "^$ME inst=0 api=7 pb=08 fe 03 10 02\$" $T/escuta.log

  # o gerente de aparelhos: a assinatura da rodada "ambos", em varint comum (1, 7, 2, 4 e 8)
  n0=$(wc -l < $T/escuta.log)
  hmi 600 0 9999 "08 01 08 07 08 02 08 04 08 08"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$PM " > $T/pm.log
  igual gerente_responde_aos_cinco_atributos "$(sed 's/.* api=\([0-9]*\) .*/\1/' $T/pm.log | tr '\n' ' ')" "1 7 2 4 8 "
  tem gerente_sem_aparelhos_conhecidos "^$PM api=2 pb=10 02\$" $T/pm.log

  # agenda e mensagens: as assinaturas da rodada "ambos" também têm resposta
  hmi 1201 0 9999 "08 02"; hmi 1209 0 9999 "08 0a 08 08 08 06 08 04 08 02"; hmi 1101 0 9999 "08 08 08 0a 08 10 08 14"; hmi 1103 0 9999 "08 02 08 04 08 06 08 08 08 16 08 18"; sleep 0.4
  tem agenda_responde_ao_estado '^canal=0x100 if=1202 inst=0 api=1 pb=' $T/escuta.log
  igual perfil_da_agenda_responde_aos_cinco "$(conta '^canal=0x100 if=1210 inst=0 api=[1-5] ' $T/escuta.log)" 5
  igual mensagens_respondem_aos_quatro "$(conta '^canal=0x100 if=1102 inst=0 api=(4|5|8|10) ' $T/escuta.log)" 4
  igual ajustes_das_mensagens_respondem_aos_seis "$(conta '^canal=0x100 if=1104 inst=0 api=(1|2|3|4|11|12) ' $T/escuta.log)" 6
  naotem nenhum_atributo_assinado_ficou_sem_evento 'sem evento no esquema' $T/stub.log

  # quem assina de novo recebe de novo; desassinar não manda nada
  n0=$(wc -l < $T/escuta.log)
  hmi 1 0 9998 "08 06"; sleep 0.3
  igual desassinar_nao_manda_nada "$(tail -n +$((n0+1)) $T/escuta.log | grep -c '^canal=0x100 ')" 0

  # os quatro pedidos da partida e da entrada na tela de que a HMI espera resposta em 10 s (rodadas
  # "e1" e "e2", com os bytes que ela mandou): o perfil responde "feito" (resultado 0, que não vai no
  # fio). Sem resposta ficam quatro "DSI timeout occurred" no registro dela, e o do idioma da
  # agenda segura a troca de idioma da central (TargetASLOrganizerBaseFunctionality.dsiAdbSetLanguageResult)
  hmi 1 0 1000 "08 08"; hmi 1001 0 1028 "08 02"; hmi 1001 0 1057 "08 02 10 04 18 01"; hmi 1201 0 1000 "0a 05 70 74 5f 42 52"; sleep 0.4
  tem modo_visivel_respondido "^$BT api=2016 pb=\$" $T/escuta.log
  tem mudo_do_microfone_respondido "^$ME inst=0 api=2028 pb=\$" $T/escuta.log
  tem estatisticas_de_chamadas_respondidas "^$ME inst=0 api=2047 pb=\$" $T/escuta.log
  tem idioma_da_agenda_respondido '^canal=0x100 if=1202 inst=0 api=2000 pb=$' $T/escuta.log
  igual quatro_pedidos_respondidos "$(conta 'pedido atendido: .*: respondi ' $T/stub.log)" 4
  # o modo de visibilidade pedido fica gravado no atributo (o serviço do Bluetooth está em "grava")
  tem modo_visivel_gravado 'setAccessibleMode \(api 1000\): gravei updateAccessibleMode\.accessibleMode = 4' $T/stub.log
  # e um quinto, ao abrir e ao fechar a lista "Celulares conhecidos" (rodada "p2"): o gerente de
  # aparelhos suspende e retoma a reconexão automática
  hmi 600 0 1004 "08 01"; hmi 600 0 1004 ""; sleep 0.3
  igual suspender_reconexao_respondido_duas_vezes "$(conta "^$PM api=2004 pb=\$" $T/escuta.log)" 2

  # ligar e desligar o Bluetooth pela tela "Configurações do Bluetooth" (rodada "prova2"): o pedido
  # leva o estado pedido (1 = desligado, "08 02"; 0 = ligado, mensagem vazia). Sem resposta a HMI
  # fica até 60 s no aviso "Aguarde…". O perfil grava o estado pedido no atributo de estado do
  # Bluetooth, que volta para quem o assina, e responde "feito"
  hmi 1 0 9999 "08 06"; sleep 0.3
  n0=$(wc -l < $T/escuta.log)
  hmi 1 0 1012 "08 02"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$BT " > $T/desl.log
  tem desligar_o_bluetooth_muda_o_estado "^$BT api=3 pb=08 02 10 02\$" $T/desl.log
  tem desligar_o_bluetooth_e_respondido "^$BT api=2010 pb=\$" $T/desl.log
  n0=$(wc -l < $T/escuta.log)
  hmi 1 0 1012 ""; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$BT " > $T/lig.log
  tem ligar_o_bluetooth_volta_o_estado "^$BT api=3 pb=10 02\$" $T/lig.log
  tem ligar_o_bluetooth_e_respondido "^$BT api=2010 pb=\$" $T/lig.log
  # o nome da central no Bluetooth: o que a tela gravar volta no atributo (regra do esquema). O
  # texto do teste é "TESTE"
  hmi 1 0 9999 "08 12"; sleep 0.3
  hmi 1 0 1011 "0a 05 54 45 53 54 45"; sleep 0.3
  tem nome_do_bluetooth_gravado_pela_tela_volta "^$BT api=9 pb=0a 05 54 45 53 54 45 10 02\$" $T/escuta.log

  # o pedido de idioma do aparelho (rodada "e1") não tem resposta marcada pela HMI: fica registrado,
  # com o nome que o esquema dá
  hmi 1001 0 1029 "0a 05 70 74 5f 42 52"; sleep 0.3
  tem pedido_de_idioma_fica_registrado 'pedido sem resposta: DSIMobileEquipment\.requestSetLanguage \(api 1029' $T/stub.log
  naotem nenhum_pedido_fora_do_esquema 'que não está no esquema' $T/stub.log
  desce

  # ---- só a HMI pede. A interface de pedidos das mensagens (1101) é também a de EVENTOS do serviço
  # de energia, que é de outro domínio (0x400000), e na central o dsistub publica nela de segundo em
  # segundo (rodada "e2": 213 mensagens, e 400 linhas "pedido sem resposta: DSIMessaging, api 1, que
  # não está no esquema" no registro do stub). O que não vem do canal da HMI não é pedido: não fica
  # registrado, não é atendido e não conta como "a HMI já pediu", o que desligaria o anúncio de reserva
  sobe $T/stub2.log "$P"
  "$BIN/dsi_manda" $SOCK 0x400000 1101 0 1 "08 02 10 00 18 02"; "$BIN/dsi_manda" $SOCK 0x400000 1101 0 4 "0a 08 08 01 10 01 18 01 20 00 10 02"
  "$BIN/dsi_manda" $SOCK 0x400000 1101 0 9999 "08 08"; sleep 0.4
  naotem evento_de_outro_dominio_nao_vira_pedido 'DSIMessaging, api' $T/stub2.log
  naotem evento_de_outro_dominio_nao_vira_assinatura 'DSIMessaging: assinou' $T/stub2.log
  igual evento_de_outro_dominio_nao_tem_resposta "$(conta '^canal=0x100 if=1102 ' $T/escuta.log)" 0
  antes=$(conta '^canal=0x100 if=9000 ' $T/escuta.log)
  hmi 1100 0 9999 "08 02"; sleep 3.5        # a HMI aparece (assina energia, que não é deste stub) e não pede nada a ele
  depois=$(conta '^canal=0x100 if=9000 ' $T/escuta.log)
  if [ $((depois - antes)) -ge 1 ] && [ $((depois - antes)) -le 2 ]; then ok evento_de_outro_dominio_nao_conta_como_pedido_da_hmi
  else nok evento_de_outro_dominio_nao_conta_como_pedido_da_hmi "$((depois - antes)) anúncios novos em 3,5 s (esperava 1 ou 2)"; fi
  desce

  # ---- sem perfil nenhum (TELEFONE=-): só o anúncio e os padrões. O Bluetooth sai "ligado", mas
  # o estado de ativação sai 0 (INIT), que a HMI ignora: é o perfil que diz "não conectado"
  sobe $T/stub0.log
  hmi 1 0 9999 "08 06"; hmi 1001 0 9999 "08 08"; sleep 0.4
  tem sem_perfil_bluetooth_ligado "^$BT api=3 pb=10 02\$" $T/escuta.log
  tem sem_perfil_ativacao_em_zero "^$ME inst=0 api=4 pb=10 02\$" $T/escuta.log
  hmi 1001 0 9999 "08 0e"; sleep 0.3
  tem sem_perfil_bateria_em_zero "^$ME inst=0 api=7 pb=10 02\$" $T/escuta.log
  desce
else
  nok perfil_sem_aparelho_existe "não achei $P ou $E"
fi

# ---- valor por instância (o segundo estado, um aparelho fictício conectado, 12/10/2026). O
# DSIMobileEquipment tem três instâncias (0 o aparelho principal, 1 o associado, 2 a do CarPlay), e
# "conectado" só pode valer para uma. A seção [Serviço[N].evento] do perfil vale só para a
# instância N, inteira, no lugar da seção sem instância; as outras instâncias seguem com esta.
if [ -f "$E" ]; then
  cat > $T/pi.txt <<'EOF'
[DSIMobileEquipment.updateActivationState]
activationState.telActivationState = 1
[DSIMobileEquipment[0].updateActivationState]
activationState.telActivationState = 5
activationState.telMode = 3
[DSIMobileEquipment.updateSignalQuality]
signalQuality = 1
EOF
  "$BIN/veiculostub" -n $E $T/pi.txt > $T/pin.log 2>&1; igual perfil_com_instancia_confere "$?" 0
  tem perfil_com_instancia_conta_os_valores 'perfil: 4 valores em 3 eventos' $T/pin.log
  # -m mostra a seção sem instância como sempre, e a da instância à parte, dizendo qual é
  "$BIN/veiculostub" -m $E $T/pi.txt > $T/pim.log 2>/dev/null
  tem mostra_o_valor_sem_instancia '^canal=0x100 if=1002 inst=0 api=4 pb=0a 02 08 02 10 02$' $T/pim.log
  tem mostra_o_valor_da_instancia '^canal=0x100 if=1002 inst=0 api=4 pb=0a 04 08 0a 18 06 10 02 \(só a instância 0\)$' $T/pim.log
  sobe $T/stub3.log $T/pi.txt
  hmi 1001 0 9999 "08 08 08 36"; hmi 1001 1 9999 "08 08 08 36"; hmi 1001 2 9999 "08 08"; sleep 0.4
  # estado 5 (ligado) e modo 3 (HFP) só na instância 0: { 1: { 1: 5, 3: 3 }, 2: 1 }
  tem instancia_0_recebe_o_valor_dela "^$ME inst=0 api=4 pb=0a 04 08 0a 18 06 10 02\$" $T/escuta.log
  tem instancia_1_recebe_o_valor_sem_instancia "^$ME inst=1 api=4 pb=0a 02 08 02 10 02\$" $T/escuta.log
  tem instancia_2_recebe_o_valor_sem_instancia "^$ME inst=2 api=4 pb=0a 02 08 02 10 02\$" $T/escuta.log
  # evento sem seção de instância: todas as instâncias recebem o mesmo, como sempre
  igual evento_sem_instancia_vale_para_todas "$(conta "^$ME inst=[01] api=27 pb=08 02 10 02\$" $T/escuta.log)" 2
  # assinar de novo manda de novo o valor da instância; desassinar funciona nela
  n0=$(wc -l < $T/escuta.log)
  hmi 1001 0 9999 "08 08"; sleep 0.3
  igual instancia_0_assina_de_novo "$(tail -n +$((n0+1)) $T/escuta.log | grep -c "^$ME inst=0 api=4 pb=0a 04 08 0a 18 06 10 02\$")" 1
  hmi 1001 0 9998 "08 08"; sleep 0.2
  tem instancia_0_desassina 'DSIMobileEquipment: desassinou 4' $T/stub3.log
  desce
  # erros de escrita: nada sobe, e a mensagem diz a linha
  printf '[DSIMobileEquipment[x].updateActivationState]\nactivationState.telActivationState = 5\n' > $T/pe1.txt
  "$BIN/veiculostub" -n $E $T/pe1.txt > $T/pe1.log 2>&1; igual instancia_que_nao_e_numero_recusa "$?" 2
  tem instancia_que_nao_e_numero_diz_a_linha 'pe1.txt, linha 1: .*instância' $T/pe1.log
  printf '[DSIBluetooth[0].responseSwitchBTState]\n' > $T/pe2.txt
  "$BIN/veiculostub" -n $E $T/pe2.txt > $T/pe2.log 2>&1; igual instancia_em_resposta_recusa "$?" 2
  tem instancia_em_resposta_diz_por_que 'pe2.txt, linha 1: .*instância' $T/pe2.log
  printf '[DSIBluetooth[0].requestSwitchBTState -> updateBTState]\nbTState = btStateTarget\n' > $T/pe3.txt
  "$BIN/veiculostub" -n $E $T/pe3.txt > $T/pe3.log 2>&1; igual instancia_em_regra_recusa "$?" 2
  # e o perfil da pasta, sem seção de instância, segue igual: as três instâncias, "não conectado"
  if [ -f "$P" ]; then
    sobe $T/stub4.log "$P"
    hmi 1001 0 9999 "08 08"; hmi 1001 1 9999 "08 08"; hmi 1001 2 9999 "08 08"; sleep 0.4
    igual sem_secao_de_instancia_as_tres_iguais "$(conta "^$ME inst=[012] api=4 pb=0a 02 08 02 10 02\$" $T/escuta.log)" 3
    desce
  fi
fi

# ---- o segundo estado: um aparelho FICTÍCIO conectado (aparelho-conectado.txt), em camadas sobre o
# sem-aparelho.txt, que traz o Bluetooth ligado, as respostas e as regras. Só o aparelho principal
# (instância 0) fica conectado; o associado e a do CarPlay seguem "nenhum aparelho conectado". O
# mesmo aparelho vai na lista de aparelhos conhecidos do gerente de aparelhos: é dela que a HMI tira
# o nome da tela principal e a linha de "Celulares conhecidos" (rodadas "kp", sem a lista, e "k2").
C=$TEL/aparelho-conectado.txt
if [ -f "$C" ] && [ -f "$P" ] && [ -f "$E" ]; then
  "$BIN/veiculostub" -n $E "$P" "$C" > $T/cn.log 2>&1; igual conectado_em_camadas_confere "$?" 0
  # as seções do aparelho são todas da instância 0: nenhuma muda o que as outras instâncias recebem.
  # Fora elas, só as do gerente de aparelhos, que tem uma instância só
  igual conectado_aparelho_so_na_instancia_0 "$(conta '^\[DSIMobileEquipment[^.]*\.update' "$C")" "$(conta '^\[DSIMobileEquipment\[0\]\.update' "$C")"
  igual conectado_so_aparelho_gerente_e_agenda "$(conta '^\[' "$C")" "$(conta '^\[(DSIMobileEquipment\[0\]\.update|DSIMobileEquipment\.response|DSIPhoneManager\.|DSIAdb\.|DSIAdbUserProfile\.|DSIAdbList\.|DSIMessaging\.)' "$C")"
  # dado de ninguém: nada de IMEI, IMSI, cartão SIM nem número, nem como texto nem como "disponível" (2)
  naotem conectado_nao_traz_identificador_nem_numero '^[^#]*\.([A-Za-z]*(IMEI|IMSI|[Ss]im[Ii]d|[Nn]umber)[A-Za-z]*) *= *"' "$C"
  naotem conectado_nao_diz_que_ha_imei_imsi_ou_sim '^[^#]*telNetIdAvail(IMSI|SimId|NadIMEI|MeIMEI) *= *([02-9]|1[0-9])' "$C"
  # o endereço Bluetooth é o de exemplo, que não é de nenhum fabricante (prefixo 00:00:00)
  tem conectado_endereco_de_exemplo '^phoneInformation\.meBtAddress *= *"00:00:00:00:00:01"' "$C"
  # e é o mesmo nos dados do aparelho e na lista do gerente: a HMI liga os dois pelo endereço
  # (TargetASLPhoneConnectionManager.updatePhoneSelectionData)
  igual conectado_mesmo_endereco_nos_dois "$(conta '^[^#]*[Bb]t[A-Za-z]*Address *= *"00:00:00:00:00:01"' "$C")" 2

  sobe $T/stub5.log "$P" "$C"
  # atributos 4 (ativação), 20 (dados do aparelho), 24 (rede), 27 (sinal), 7 (bateria) e 18 (mudo)
  for i in 0 1 2; do hmi 1001 $i 9999 "08 08 08 28 08 30 08 36 08 0e 08 24"; done; sleep 0.5
  # instância 0: ligado (5 = ACTIVATIONSTATE_PHONE_ON) e em mãos-livres (3 = TELMODE_HFP)
  tem conectado_principal_ligado "^$ME inst=0 api=4 pb=0a 04 08 0a 18 06 10 02\$" $T/escuta.log
  # os dados: o endereço de exemplo (campo 5) e o nome "Celular fictício" (campo 6), e só eles como
  # disponíveis (campos 11 e 12 em 2; os de IMSI, SIM e IMEI, 7 a 10, em 1 = não há)
  tem conectado_nome_ficticio "^$ME inst=0 api=20 pb=0a 32 2a 11 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 31 32 11 43 65 6c 75 6c 61 72 20 66 69 63 74 c3 ad 63 69 6f 38 02 40 02 48 02 50 02 58 04 60 04 10 02\$" $T/escuta.log
  # a rede: registrado (1) na operadora "Fictícia"
  tem conectado_operadora_ficticia "^$ME inst=0 api=24 pb=0a 0d 08 02 12 09 46 69 63 74 c3 ad 63 69 61 10 02\$" $T/escuta.log
  tem conectado_sinal "^$ME inst=0 api=27 pb=08 06 10 02\$" $T/escuta.log
  # a bateria do aparelho fictício em 4 (60 a 79 %): não é o zero, que abre o aviso de bateria vazia
  tem conectado_bateria_nao_vazia "^$ME inst=0 api=7 pb=08 08 10 02\$" $T/escuta.log
  # o que o aparelho-conectado.txt não diz vem do sem-aparelho.txt, também na instância 0
  tem conectado_microfone_sem_mudo "^$ME inst=0 api=18 pb=08 02 10 02\$" $T/escuta.log
  # as outras instâncias: nenhum aparelho conectado, sem dados, sem rede, bateria desconhecida
  igual conectado_as_outras_nao_conectadas "$(conta "^$ME inst=[12] api=4 pb=0a 02 08 02 10 02\$" $T/escuta.log)" 2
  igual conectado_as_outras_sem_dados "$(conta "^$ME inst=[12] api=(20|24|27) pb=10 02\$" $T/escuta.log)" 6
  igual conectado_as_outras_bateria_desconhecida "$(conta "^$ME inst=[12] api=7 pb=08 fe 03 10 02\$" $T/escuta.log)" 2
  # o Bluetooth segue ligado, como no primeiro estado
  hmi 1 0 9999 "08 06"; hmi 600 0 9999 "08 02"; sleep 0.3
  tem conectado_bluetooth_ligado "^$BT api=3 pb=10 02\$" $T/escuta.log
  # o gerente de aparelhos lista o aparelho fictício: o endereço (campo 4), o nome (5), um
  # identificador inventado (6) e o mãos-livres suportado e conectado (campo 10 em 10 =
  # SERVICESTATE_SUPPORTED | SERVICESTATE_CONNECTED, "50 14"; a HMI testa os dois bits em
  # ServiceStateUtil). Os outros serviços ficam no zero, que aqui é SERVICESTATE_NOT_SUPPORTED: sem música
  # por Bluetooth, sem agenda, sem CarPlay nem Android Auto
  tem conectado_gerente_lista_o_aparelho "^$PM api=2 pb=0a 3d 22 11 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 31 2a 11 43 65 6c 75 6c 61 72 20 66 69 63 74 c3 ad 63 69 6f 32 13 61 70 61 72 65 6c 68 6f 2d 66 69 63 74 69 63 69 6f 2d 31 50 14 10 02\$" $T/escuta.log
  # com o aparelho na lista a HMI pede, aos 20 s da partida, que ele seja o prioritário, e espera a
  # resposta por 10 s (rodada "k2", com os bytes que ela mandou: o identificador da lista). O
  # perfil responde "feito" (0 = RESULT_OK, que não vai no fio)
  hmi 600 0 1010 "0a 13 61 70 61 72 65 6c 68 6f 2d 66 69 63 74 69 63 69 6f 2d 31"; sleep 0.3
  tem conectado_aparelho_prioritario_respondido "^$PM api=2008 pb=\$" $T/escuta.log
  naotem conectado_nenhum_pedido_sem_resposta 'pedido sem resposta' $T/stub5.log
  desce
else
  nok perfil_aparelho_conectado_existe "não achei $C"
fi

# ---- o modelo (driver/telefonestub.c, 12/10/2026): o motor mais o que CONECTA e DESCONECTA o
# aparelho fictício quando a tela pede. Os bytes dos pedidos são os que a HMI mandou nas rodadas
# "e2c" (conectar, ao tocar no aparelho em "Celulares conhecidos") e "prova2" (desligar o
# Bluetooth); os números de api e os resultados foram lidos em DSIPhoneManagerImpl e nas
# constantes RESULT_ e SERVICESTATE_ de org.dsi.ifc.phonemanager.DSIPhoneManager.
# Conectado: a instância 0 do aparelho recebe as seções [DSIMobileEquipment[0].…] do perfil e a
# lista do gerente diz mãos-livres 10. Desconectado: a instância 0 recebe o que as outras recebem
# (as seções sem instância) e a lista diz 2 (SERVICESTATE_SUPPORTED: conhecido, não conectado).
TB=$BIN/telefonestub; D=$TEL/comeca-desconectado.txt
ID="0a 13 61 70 61 72 65 6c 68 6f 2d 66 69 63 74 69 63 69 6f 2d 31"
ID1="61 70 61 72 65 6c 68 6f 2d 66 69 63 74 69 63 69 6f 2d 31"      # o identificador da lista
LISTA="0a 3d 22 11 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 31 2a 11 43 65 6c 75 6c 61 72 20 66 69 63 74 c3 ad 63 69 6f 32 13 61 70 61 72 65 6c 68 6f 2d 66 69 63 74 69 63 69 6f 2d 31"
LIG="0a 04 08 0a 18 06 10 02"; DESL="0a 02 08 02 10 02"
desde() { tail -n +$(($1+1)) $T/escuta.log; }
if [ -x "$TB" ] && [ -f "$C" ] && [ -f "$P" ] && [ -f "$D" ]; then
  "$TB" -n $E "$P" "$C" > $T/mn.log 2>&1; igual modelo_confere_o_perfil_conectado "$?" 0
  tem modelo_diz_que_comeca_conectado 'modelo do telefone: .*começa conectado' $T/mn.log
  "$TB" -n $E "$P" "$C" "$D" > $T/mnd.log 2>&1; igual modelo_confere_o_perfil_desconectado "$?" 0
  tem modelo_diz_que_comeca_desconectado 'modelo do telefone: .*começa desconectado' $T/mnd.log
  "$TB" -n $E "$P" > $T/mn0.log 2>&1; igual modelo_confere_sem_aparelho "$?" 0
  tem modelo_diz_que_nao_ha_aparelho 'modelo do telefone: nenhum aparelho' $T/mn0.log
  tem perfil_desconectado_diz_que_e_simulacao 'SIMULAÇÃO' "$D"

  # -- começa conectado (os dois perfis do segundo estado)
  PROG=telefonestub sobe $T/m1.log "$P" "$C"
  hmi 1001 0 9999 "08 08 08 28 08 30 08 36 08 0e"; hmi 1001 1 9999 "08 08 08 28"; hmi 600 0 9999 "08 02"; hmi 1 0 9999 "08 06"; sleep 0.5
  tem modelo_comeca_conectado "^$ME inst=0 api=4 pb=$LIG\$" $T/escuta.log
  # A LISTA É SEGURADA NA PARTIDA. A HMI guarda o endereço do aparelho principal e o do associado
  # quando processa os dados do aparelho das instâncias 0 e 1, e ao receber a lista compara cada
  # aparelho com esses endereços sem ver se já os tem: lista não vazia que chega antes dá
  # NullPointerException em TargetASLPhoneConnectionManager.validateCanBePrimaryOrAssociated (4 de
  # 7 partidas nas séries "n" e "g", e 3 de 5 nas do aparelho conectado da madrugada). Por isso o
  # modelo responde à assinatura da lista com a lista VAZIA e só manda a de verdade um a dois
  # segundos depois de as duas instâncias terem pedido os dados do aparelho
  tem lista_segurada_na_partida "^$PM api=2 pb=10 02\$" $T/escuta.log
  igual lista_segurada_ainda_nao_saiu "$(conta "^$PM api=2 pb=0a " $T/escuta.log)" 0
  sleep 2.3
  tem modelo_lista_conectada "^$PM api=2 pb=$LISTA 50 14 10 02\$" $T/escuta.log
  igual lista_sai_depois_dos_dados_das_duas_instancias "$(grep -n "^$ME inst=[01] api=20 \|^$PM api=2 pb=0a " $T/escuta.log | tail -n 1 | sed 's/^[0-9]*://; s/ pb=.*//')" "$PM api=2"
  tem lista_solta_fica_registrada 'soltei a lista de aparelhos' $T/m1.log
  # desligar o Bluetooth pela tela (o pedido da "prova2"): o aparelho cai junto. A instância 0
  # passa a receber "não conectado", sem dados, sem rede, sem sinal e bateria desconhecida; a
  # lista passa a "conhecido, não conectado"; o estado do Bluetooth vai a desligado; e o pedido é
  # respondido. Quem assinou pela instância 1 não recebe nada: o dela não mudou
  n0=$(wc -l < $T/escuta.log); hmi 1 0 1012 "08 02"; sleep 0.5
  desde $n0 > $T/d1.log
  tem bluetooth_desligado_derruba_o_aparelho "^$ME inst=0 api=4 pb=$DESL\$" $T/d1.log
  igual bluetooth_desligado_limpa_dados_rede_e_sinal "$(conta "^$ME inst=0 api=(20|24|27) pb=10 02\$" $T/d1.log)" 3
  tem bluetooth_desligado_bateria_desconhecida "^$ME inst=0 api=7 pb=08 fe 03 10 02\$" $T/d1.log
  tem bluetooth_desligado_lista_nao_conectada "^$PM api=2 pb=$LISTA 50 04 10 02\$" $T/d1.log
  tem bluetooth_desligado_muda_o_estado "^$BT api=3 pb=08 02 10 02\$" $T/d1.log
  tem bluetooth_desligado_e_respondido "^$BT api=2010 pb=\$" $T/d1.log
  igual bluetooth_desligado_nao_mexe_na_instancia_1 "$(conta "^$ME inst=1 " $T/d1.log)" 0
  # com o Bluetooth desligado não dá para conectar: resultado 4 (RESULT_ERROR_GENERAL, "08 08")
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1000 "$ID 10 02"; sleep 0.4
  desde $n0 > $T/d2.log
  tem conectar_com_bluetooth_desligado_da_erro "^$PM api=2000 pb=08 08\$" $T/d2.log
  igual conectar_com_bluetooth_desligado_nao_conecta "$(conta "^$ME inst=0 api=4 " $T/d2.log)" 0
  # ligar o Bluetooth de novo: o estado volta, e o aparelho fica conhecido e NÃO conectado (quem
  # conecta é a tela, tocando nele)
  n0=$(wc -l < $T/escuta.log); hmi 1 0 1012 ""; sleep 0.4
  desde $n0 > $T/d3.log
  tem bluetooth_ligado_de_novo "^$BT api=3 pb=10 02\$" $T/d3.log
  tem bluetooth_ligado_de_novo_e_respondido "^$BT api=2010 pb=\$" $T/d3.log
  igual bluetooth_ligado_nao_reconecta_sozinho "$(conta "^$ME inst=0 api=4 " $T/d3.log)" 0
  # conectar pela tela (o pedido da "e2c": o identificador e o tipo 1 = SERVICETYPE_TELEPHONY_HFP):
  # a instância 0 volta a receber as seções dela, a lista diz conectado e o pedido é respondido
  # com resultado 0 (RESULT_OK, que não vai no fio). A HMI espera essa resposta por 180 s, com o
  # aviso "Estabelecendo conexão…" na tela
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1000 "$ID 10 02"; sleep 0.5
  desde $n0 > $T/d4.log
  tem conectar_liga_o_aparelho "^$ME inst=0 api=4 pb=$LIG\$" $T/d4.log
  tem conectar_devolve_a_operadora "^$ME inst=0 api=24 pb=0a 0d 08 02 12 09 46 69 63 74 c3 ad 63 69 61 10 02\$" $T/d4.log
  tem conectar_devolve_a_bateria "^$ME inst=0 api=7 pb=08 08 10 02\$" $T/d4.log
  tem conectar_lista_conectada "^$PM api=2 pb=$LISTA 50 14 10 02\$" $T/d4.log
  tem conectar_e_respondido "^$PM api=2000 pb=\$" $T/d4.log
  igual conectar_responde_depois_de_avisar "$(tail -n 1 $T/d4.log | sed 's/ pb=.*//')" "$PM api=2000"
  # desconectar (requestDisconnectService, api 1001; a tela desta variante não o manda ao tocar no
  # aparelho conectado, que é só uma linha de informação, mas a HMI o usa ao trocar de aparelho)
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1001 "$ID 10 02"; sleep 0.5
  desde $n0 > $T/d5.log
  tem desconectar_derruba_o_aparelho "^$ME inst=0 api=4 pb=$DESL\$" $T/d5.log
  tem desconectar_lista_nao_conectada "^$PM api=2 pb=$LISTA 50 04 10 02\$" $T/d5.log
  tem desconectar_e_respondido "^$PM api=2001 pb=\$" $T/d5.log
  # quem assina com o aparelho desconectado recebe "não conectado" na instância 0
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 9999 "08 08"; sleep 0.3
  igual assinar_desconectado_recebe_nao_conectado "$(desde $n0 | grep -c "^$ME inst=0 api=4 pb=$DESL\$")" 1
  # aparelho que não é o da lista (identificador "x"): resultado 1, RESULT_ERROR_INVALID_DEVICE_ID
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1000 "0a 01 78 10 02"; sleep 0.4
  desde $n0 > $T/d6.log
  tem conectar_aparelho_desconhecido_da_erro "^$PM api=2000 pb=08 02\$" $T/d6.log
  igual conectar_aparelho_desconhecido_nao_conecta "$(conta "^$ME inst=0 api=4 " $T/d6.log)" 0
  # outro serviço que não o mãos-livres (9 = SERVICETYPE_AUDIO_SOURCE, "10 12"): resultado 8,
  # RESULT_ERROR_SERVICE_NOT_SUPPORTED ("08 10")
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1000 "$ID 10 12"; sleep 0.4
  tem conectar_outro_servico_nao_e_suportado "^$PM api=2000 pb=08 10\$" $T/escuta.log
  igual modelo_atendeu_os_pedidos "$(conta '\[telefonestub\] pedido atendido: ' $T/m1.log)" 7
  naotem modelo_nenhum_pedido_sem_resposta 'pedido sem resposta' $T/m1.log
  desce

  # -- começa desconectado (o terceiro perfil por cima): conhecido, não conectado, até a tela conectar
  PROG=telefonestub sobe $T/m2.log "$P" "$C" "$D"
  hmi 1001 0 9999 "08 08 08 28 08 0e"; hmi 600 0 9999 "08 02"; sleep 0.5
  tem comeca_desconectado_nao_conectado "^$ME inst=0 api=4 pb=$DESL\$" $T/escuta.log
  tem comeca_desconectado_sem_dados "^$ME inst=0 api=20 pb=10 02\$" $T/escuta.log
  tem comeca_desconectado_bateria_desconhecida "^$ME inst=0 api=7 pb=08 fe 03 10 02\$" $T/escuta.log
  # aqui a instância 1 não pediu os dados do aparelho: a lista fica segurada e sai sozinha 6 s
  # depois de assinada, para a tela não ficar sem ela se a HMI mudar a ordem
  sleep 2.5
  igual sem_os_dados_da_instancia_1_a_lista_espera "$(conta "^$PM api=2 pb=0a " $T/escuta.log)" 0
  sleep 5
  tem comeca_desconectado_lista_conhecido "^$PM api=2 pb=$LISTA 50 04 10 02\$" $T/escuta.log
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1000 "$ID 10 02"; sleep 0.5
  desde $n0 > $T/d7.log
  tem comeca_desconectado_e_conecta "^$ME inst=0 api=4 pb=$LIG\$" $T/d7.log
  tem comeca_desconectado_conecta_com_o_nome "^$ME inst=0 api=20 pb=0a 32 2a 11 30 30 3a" $T/d7.log
  tem comeca_desconectado_lista_conectada "^$PM api=2 pb=$LISTA 50 14 10 02\$" $T/d7.log
  tem comeca_desconectado_conectar_e_respondido "^$PM api=2000 pb=\$" $T/d7.log
  desce

  # -- sem aparelho nenhum (só o primeiro perfil, o padrão de antes): o modelo liga e desliga o
  # Bluetooth como o perfil fazia, e recusa conectar o que não conhece
  PROG=telefonestub sobe $T/m3.log "$P"
  hmi 1 0 9999 "08 06"; hmi 1001 0 9999 "08 08"; sleep 0.3
  n0=$(wc -l < $T/escuta.log); hmi 1 0 1012 "08 02"; sleep 0.4
  desde $n0 > $T/d8.log
  tem sem_aparelho_desliga_o_bluetooth "^$BT api=3 pb=08 02 10 02\$" $T/d8.log
  tem sem_aparelho_desligar_e_respondido "^$BT api=2010 pb=\$" $T/d8.log
  igual sem_aparelho_desligar_nao_mexe_no_aparelho "$(conta "^$ME " $T/d8.log)" 0
  hmi 1 0 1012 ""; sleep 0.3
  tem sem_aparelho_liga_o_bluetooth "^$BT api=3 pb=10 02\$" $T/escuta.log
  hmi 600 0 1000 "$ID 10 02"; sleep 0.3
  tem sem_aparelho_conectar_da_erro "^$PM api=2000 pb=08 02\$" $T/escuta.log
  # e o que o perfil já respondia continua respondido pelo motor
  hmi 1 0 1000 "08 08"; hmi 600 0 1004 "08 01"; sleep 0.3
  tem sem_aparelho_motor_segue_respondendo "^$BT api=2016 pb=\$" $T/escuta.log
  tem sem_aparelho_gerente_segue_respondendo "^$PM api=2004 pb=\$" $T/escuta.log
  desce
else
  nok modelo_do_telefone_existe "não achei $TB (driver/telefonestub.c compilado), $C ou $D"
fi

# ---- a chamada simulada (12/10/2026): discar pela tela, chamando, em chamada, encerrar. Os bytes
# do pedido de discar são os da rodada "cb" (o número fictício 5550100, digitado no teclado da
# tela); os números de api, os estados de chamada (CALLSTATE_DIALING 1, ALERTING 2, ACTIVE 4), o
# estado de ativação em chamada (ACTIVATIONSTATE_TEL_ACTIVE_CALL 2) e os resultados (RESULT_OK 0,
# RESULT_ERROR_NOT_ALLOWED 7) são constantes de org.dsi.ifc.telephoneng.DSIMobileEquipment.
# DUAS COISAS SEM AS QUAIS A HMI NEM MANDA DISCAR (rodadas "c0", "c1d", "ct", "ca" e "cb"):
#  - a agenda "pronta": 2 = ADBSTATE_READY. Com o zero (indefinida) a máquina de chamadas da HMI
#    fica em StateCallHandlingWaitForAdb e não trata o pedido de discar;
#  - um perfil de agenda ativo: sem ele o número do perfil ativo fica em -1 e a busca de contatos
#    que a HMI faz a cada dígito estoura (Truffles: SOURCES[-1]) antes de guardar o número
#    digitado; a tecla verde acha o campo vazio.
# As duas só valem com o aparelho conectado: desconectado, a agenda sai como sempre saiu.
DISCA="0a 07 35 35 35 30 31 30 30"; CH="4a 07 35 35 35 30 31 30 30 10 02"; EMCH="0a 04 08 04 18 06 10 02"
AG="canal=0x100 if=1202 inst=0"
if [ -x "$TB" ] && [ -f "$C" ] && [ -f "$P" ] && [ -f "$D" ]; then
  PROG=telefonestub sobe $T/m4.log "$P" "$C"
  hmi 1001 0 9999 "08 08 08 12 08 32"; hmi 1001 1 9999 "08 12"; hmi 1201 0 9999 "08 02 08 04"; hmi 1209 0 9999 "08 0a"; sleep 0.5
  tem agenda_pronta_com_o_aparelho_conectado "^$AG api=1 pb=08 04 10 02\$" $T/escuta.log
  tem agenda_com_o_perfil_ficticio "^$AG api=2 pb=0a .* 50 65 72 66 69 6c 20 66 69 63 74 c3 ad 63 69 6f .*10 02\$" $T/escuta.log
  tem perfil_de_agenda_ativo '^canal=0x100 if=1210 inst=0 api=5 pb=0a .* 50 65 72 66 69 6c 20 66 69 63 74' $T/escuta.log
  tem sem_chamada_a_lista_de_chamadas_e_vazia "^$ME inst=0 api=9 pb=10 02\$" $T/escuta.log
  # discar: a resposta (resultado 0), o tipo do número (0 = número comum, que é o que solta a tela),
  # o aparelho "em chamada" e a chamada na lista, discando (1), com o número que a tela mandou
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1005 "$DISCA"; sleep 0.5
  desde $n0 > $T/c1.log
  tem discar_e_respondido "^$ME inst=0 api=2007 pb=\$" $T/c1.log
  tem discar_diz_que_e_numero_comum "^$ME inst=0 api=25 pb=10 02\$" $T/c1.log
  tem discar_poe_o_aparelho_em_chamada "^$ME inst=0 api=4 pb=$EMCH\$" $T/c1.log
  tem discar_poe_a_chamada_na_lista "^$ME inst=0 api=9 pb=0a 0d 08 02 10 02 $CH\$" $T/c1.log
  igual discar_nao_mexe_na_instancia_1 "$(conta "^$ME inst=1 " $T/c1.log)" 0
  # o tempo: aos 2 s "chamando" (2), aos 5 s "em chamada" (4)
  sleep 2.6
  tem chamando_aos_dois_segundos "^$ME inst=0 api=9 pb=0a 0d 08 02 10 04 $CH\$" $T/escuta.log
  igual ainda_nao_em_chamada "$(conta "^$ME inst=0 api=9 pb=0a 0d 08 02 10 08 " $T/escuta.log)" 0
  sleep 3.2
  tem em_chamada_aos_cinco_segundos "^$ME inst=0 api=9 pb=0a 0d 08 02 10 08 $CH\$" $T/escuta.log
  # o teclado, durante a chamada, manda tons (sendDTMF, api 1007; o "5" da rodada "h1") e a HMI
  # espera a resposta por 10 s: o perfil responde "feito"
  hmi 1001 0 1007 "0a 01 35"; sleep 0.3
  tem tom_do_teclado_respondido "^$ME inst=0 api=2008 pb=\$" $T/escuta.log
  # com uma chamada em curso, discar de novo dá ocupado (6 = RESULT_ERROR_SERV_BUSY, "08 0c")
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1005 "$DISCA"; sleep 0.4
  tem discar_com_chamada_em_curso_da_ocupado "^$ME inst=0 api=2007 pb=08 0c\$" $T/escuta.log
  # encerrar (hangupCall, a chamada 1): a resposta, a lista vazia e o aparelho de volta a "ligado"
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1001 "08 02"; sleep 0.5
  desde $n0 > $T/c2.log
  tem encerrar_e_respondido "^$ME inst=0 api=2010 pb=\$" $T/c2.log
  tem encerrar_esvazia_a_lista "^$ME inst=0 api=9 pb=10 02\$" $T/c2.log
  tem encerrar_volta_o_aparelho_a_ligado "^$ME inst=0 api=4 pb=$LIG\$" $T/c2.log
  # desligar o Bluetooth no meio de uma chamada derruba a chamada junto com o aparelho
  hmi 1001 0 1005 "$DISCA"; sleep 0.4; n0=$(wc -l < $T/escuta.log); hmi 1 0 1012 "08 02"; sleep 0.5
  desde $n0 > $T/c3.log
  tem bluetooth_desligado_derruba_a_chamada "^$ME inst=0 api=9 pb=10 02\$" $T/c3.log
  tem bluetooth_desligado_em_chamada_derruba_o_aparelho "^$ME inst=0 api=4 pb=$DESL\$" $T/c3.log
  tem bluetooth_desligado_devolve_a_agenda_ao_padrao "^$AG api=1 pb=10 02\$" $T/c3.log
  # sem aparelho conectado não se disca: 7 = RESULT_ERROR_NOT_ALLOWED ("08 0e"), e nenhuma chamada
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1005 "$DISCA"; sleep 0.4
  desde $n0 > $T/c4.log
  tem discar_desconectado_nao_e_permitido "^$ME inst=0 api=2007 pb=08 0e\$" $T/c4.log
  igual discar_desconectado_nao_cria_chamada "$(conta "^$ME inst=0 api=9 " $T/c4.log)" 0
  # reconectar e ligar de novo: depois de o aparelho cair e voltar, discar funciona
  hmi 1 0 1012 ""; hmi 600 0 1000 "$ID 10 02"; sleep 0.4; n0=$(wc -l < $T/escuta.log); hmi 1001 0 1005 "$DISCA"; sleep 0.4
  tem discar_depois_de_reconectar "^$ME inst=0 api=9 pb=0a 0d 08 02 10 02 $CH\$" $T/escuta.log
  # A CHAMADA ACABA SOZINHA aos 20 s, como se o outro lado desligasse. Na bancada o botão vermelho
  # da tela não encerra: ao tratá-lo a HMI consulta os estados de chamada do CarPlay, que a bancada
  # não tem, e estoura (NullPointerException em ASLCarplayServicesImpl.getCarPlayCallStates;
  # rodadas "h1" e "h3"); o pedido hangupCall nem sai. Sem este fim a chamada ficaria para sempre
  sleep 12; igual chamada_ainda_de_pe_aos_doze_segundos "$(desde $n0 | grep -c "^$ME inst=0 api=9 pb=10 02\$")" 0
  sleep 9.5
  desde $n0 > $T/c6.log
  tem chamada_acaba_sozinha_aos_vinte_segundos "^$ME inst=0 api=9 pb=10 02\$" $T/c6.log
  tem chamada_acabada_volta_o_aparelho_a_ligado "^$ME inst=0 api=4 pb=$LIG\$" $T/c6.log
  tem chamada_acabada_fica_registrada 'acabou sozinha' $T/m4.log
  naotem chamada_nenhum_pedido_sem_resposta 'pedido sem resposta' $T/m4.log
  desce

  # -- começando desconectado (o padrão do modo ao vivo): a agenda sai como sempre saiu, indefinida
  # e sem perfil, e só fica pronta quando a tela conecta o aparelho
  PROG=telefonestub sobe $T/m5.log "$P" "$C" "$D"
  hmi 1201 0 9999 "08 02 08 04"; hmi 1209 0 9999 "08 0a"; sleep 0.4
  tem desconectado_a_agenda_fica_indefinida "^$AG api=1 pb=10 02\$" $T/escuta.log
  tem desconectado_nao_ha_perfil_de_agenda "^$AG api=2 pb=10 02\$" $T/escuta.log
  tem desconectado_nao_ha_perfil_ativo '^canal=0x100 if=1210 inst=0 api=5 pb=10 02$' $T/escuta.log
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1000 "$ID 10 02"; sleep 0.5
  desde $n0 > $T/c5.log
  tem conectar_deixa_a_agenda_pronta "^$AG api=1 pb=08 04 10 02\$" $T/c5.log
  tem conectar_traz_o_perfil_de_agenda "^$AG api=2 pb=0a " $T/c5.log
  desce
else
  nok chamada_modelo_existe "não achei $TB, $C ou $D"
fi

# ---- o serviço do CarPlay dizendo "nenhuma chamada" (12/10/2026, o passo do botão vermelho). O
# botão vermelho da tela de chamada só manda o hangupCall se a parte de CarPlay da HMI estiver de
# pé: ao tratar o toque ela consulta os estados de chamada do CarPlay. Essa parte só sobe com o
# CarPlay marcado na adaptação da central (ID_APPLE_DIO, DID 0555) E com este serviço no
# barramento (rodadas "r1", "r2" e "r3"). O telefone de mentira faz a parte dele: anuncia o
# domínio 512 e responde a UMA coisa, a lista de chamadas do CarPlay, vazia e válida. Os outros
# nove estados saem sem a marca de validade, e a HMI os ignora: não há CarPlay simulado.
# A assinatura é a da rodada "r2" (os atributos 1 4 10 5 6 7 8 11 2 3, na interface 606).
# O serviço fica num ESQUEMA À PARTE, opcional (gerado-carplay/: os dois domínios do telefone mais
# o 512, PID_CARPLAY, canal 0x200), porque só serve à hipótese "central com CarPlay"; o esquema
# padrão e o que o modo ao vivo sobe não mudam. Os pedidos do CarPlay vão pela interface 606 do
# domínio 512 (DSICarplay2Impl: postDSIProviderMsg(..., 512L, 606)); o despacho da LG entrega os
# eventos dele por DUAS interfaces (ListenerWorkerThread: "case 606: case 607:", o mesmo trecho).
# O gerador põe o serviço UMA vez no esquema, com a primeira, e anota a outra no relatório (até
# as 16h50 de 12/10/2026 punha duas, e os vetores de prova do CarPlay não conferiam).
K2=$TEL/carplay-sem-chamada.txt; E2=$TEL/gerado-carplay/esquema.txt
CP="canal=0x200 if=606 inst=0"; ASSCP="08 02 08 08 08 14 08 0a 08 0c 08 0e 08 10 08 16 08 04 08 06"
if [ -x "$TB" ] && [ -f "$C" ] && [ -f "$P" ] && [ -f "$K2" ] && [ -f "$E2" ]; then
  grep -v '^#' $E > $T/e1.txt; grep -v '^#' $E2 > $T/e2.txt
  igual esquema_com_carplay_traz_todo_o_esquema_padrao "$(diff $T/e1.txt $T/e2.txt | grep -c '^<')" 0
  igual esquema_com_carplay_so_acrescenta_o_carplay "$(diff $T/e1.txt $T/e2.txt | grep '^> [SW] ' | grep -vc 'DSICarplay2')" 0
  tem esquema_carplay_pedidos_na_606_do_dominio_512 '^S DSICarplay2 512 606 [0-9]+ carplay2$' $E2
  igual esquema_com_carplay_traz_o_servico_uma_vez "$(conta '^S DSICarplay2 ' $E2)" 1
  igual esquema_com_carplay_tem_19_servicos "$(conta '^S ' $E2)" 19
  tem relatorio_diz_da_segunda_interface_do_carplay 'também pela interface 607' $TEL/gerado-carplay/relatorio.txt
  # os vetores de prova do esquema com o CarPlay: os do telefone e os 14 do DSICarplay2
  G2=$TEL/gerado-carplay
  "$BIN/veiculostub" -m $E2 $G2/vetores-perfil.txt > $T/vetores2.log 2>$T/vetores2.err; rc=$?
  total=$(grep -c '^canal=' $G2/vetores-esperado.txt)
  grep '^canal=' $G2/vetores-esperado.txt | sort > $T/esperado2.ord; grep '^canal=' $T/vetores2.log | sort > $T/obtido2.ord
  iguais=$(comm -12 $T/esperado2.ord $T/obtido2.ord | wc -l | tr -d ' ')
  igual vetores_do_firmware_com_o_carplay "rc=$rc $iguais de $total" "rc=0 338 de 338"
  igual vetores_do_carplay_sao_14 "$(grep -c '^canal=0x200 ' $G2/vetores-esperado.txt)" 14
  tem anuncio_liga_o_servico_do_carplay '^W 512 org\.dsi\.ifc\.carplay2\.DSICarplay2 0 0$' $E2
  igual anuncio_do_512_liga_1_instancia "$(conta '^W 512 ' $E2)" 1
  # ATTR_CALLSTATE = 3 (org.dsi.ifc.carplay2.DSICarplay2); a validade é o segundo argumento do
  # updateCallState, que nesta interface se chama arg1 e não validFlag: o motor não a põe sozinho
  tem atributo_das_chamadas_do_carplay '^A DSICarplay2 3 ATTR_CALLSTATE$' $E2
  igual validade_das_chamadas_do_carplay_e_o_campo_2 "$(sed -n '/^M DSICarplay2ListenerMsg.updateCallState$/{n;n;p;}' $E2 2>/dev/null)" "C 2 arg1 sint32 um"
  # com o esquema padrão o perfil do CarPlay é recusado, com o arquivo e a linha
  "$BIN/telefonestub" -n $E "$P" "$C" "$K2" > $T/semesq.log 2>&1
  igual perfil_do_carplay_e_recusado_sem_o_esquema_dele "$([ $? -ne 0 ] && echo recusado) $(grep -c 'carplay-sem-chamada.txt' $T/semesq.log)" "recusado 1"
  E0=$E; E=$E2
  igual perfil_do_carplay_so_tem_a_lista_de_chamadas "$(grep -c '^\[' $K2) $(grep -c '^\[DSICarplay2\.updateCallState\]$' $K2)" "1 1"
  igual perfil_do_carplay_so_marca_a_validade "$(grep -v '^[[:space:]]*#' $K2 | grep -v '^\[' | grep -c '[^[:space:]]') $(grep -cE '^arg1 *= *1( |$)' $K2)" "1 1"
  PROG=telefonestub sobe $T/m6.log "$P" "$C" "$K2"
  igual anuncia_o_dominio_512_uma_vez "$(conta '^canal=0x200 if=9000 ' $T/escuta.log)" 1
  hmi 1001 0 9999 "08 08 08 12 08 32"; hmi 1201 0 9999 "08 02 08 04"; hmi 1209 0 9999 "08 0a"; sleep 0.3
  # antes de a HMI ouvir o CarPlay o modelo não diz nada dele
  naotem sem_a_assinatura_do_carplay_nada_e_registrado 'só acaba pelo botão' $T/m6.log
  hmi 606 0 9999 "$ASSCP"; sleep 0.5
  tem carplay_nenhuma_chamada_e_valida "^$CP api=3 pb=10 02\$" $T/escuta.log
  igual carplay_os_outros_nove_estados_saem_sem_validade "$(conta "^$CP api=(1|2|4|5|6|7|8|10|11) pb=\$" $T/escuta.log)" 9
  igual carplay_so_saem_os_dez_estados_pedidos "$(conta "^$CP api=" $T/escuta.log)" 10
  igual carplay_nada_sai_pela_outra_interface "$(conta '^canal=0x200 if=607 ' $T/escuta.log)" 0
  # com a HMI ouvindo o estado de chamadas do CarPlay o botão vermelho funciona (rodada "r2"):
  # a chamada simulada deixa de acabar sozinha aos 20 s e fica até o botão, a desconexão ou o
  # Bluetooth desligado
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1005 "$DISCA"; sleep 23
  desde $n0 > $T/c7.log
  tem com_o_carplay_ouvido_a_chamada_chega_a_em_chamada "^$ME inst=0 api=9 pb=0a 0d 08 02 10 08 $CH\$" $T/c7.log
  igual com_o_carplay_ouvido_a_chamada_nao_acaba_sozinha "$(grep -c "^$ME inst=0 api=9 pb=10 02\$" $T/c7.log)" 0
  naotem com_o_carplay_ouvido_nao_ha_fim_por_tempo 'acabou sozinha' $T/m6.log
  tem com_o_carplay_ouvido_fica_registrado 'só acaba pelo botão' $T/m6.log
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1001 "08 02"; sleep 0.5
  desde $n0 > $T/c8.log
  tem com_o_carplay_ouvido_o_botao_encerra "^$ME inst=0 api=9 pb=10 02\$" $T/c8.log
  tem com_o_carplay_ouvido_o_botao_e_respondido "^$ME inst=0 api=2010 pb=\$" $T/c8.log
  naotem carplay_nenhum_pedido_sem_resposta 'pedido sem resposta' $T/m6.log
  desce
  # sem o perfil do CarPlay a lista de chamadas dele sai como os outros estados, sem validade
  PROG=telefonestub sobe $T/m7.log "$P" "$C"
  hmi 606 0 9999 "$ASSCP"; sleep 0.4
  tem sem_o_perfil_o_carplay_nao_diz_nada "^$CP api=3 pb=\$" $T/escuta.log
  desce
  E=$E0
else
  nok carplay_perfil_existe "não achei $TB, $C, $K2 ou $E2"
fi

# ---- a chamada RECEBIDA (12/10/2026, o último passo do item 7). Não há aparelho de verdade para
# ligar; o gatilho é um número COMBINADO e fictício, o 5550199 (da faixa 555-01xx, que não é de
# ninguém): discá-lo sai como qualquer chamada e, 5 s depois de ela acabar, o aparelho fictício
# "liga de volta" do mesmo número. A chamada entra tocando (CALLSTATE_RINGING, 3), o aparelho fica
# "em chamada" (2); atender é o acceptCall (api 1000, resposta 2002: RESULT_OK 0), recusar é o
# hangupCall (api 1001). Se ninguém atende, ela para de tocar aos 20 s. Os tempos são escolha da
# simulação. Constantes de org.dsi.ifc.telephoneng.DSIMobileEquipment; os números de api, da
# camada da LG (linhas P e R do esquema).
VOLTA="0a 07 35 35 35 30 31 39 39"; CV="4a 07 35 35 35 30 31 39 39 10 02"
if [ -x "$TB" ] && [ -f "$C" ] && [ -f "$P" ]; then
  PROG=telefonestub sobe $T/m8.log "$P" "$C"
  hmi 1001 0 9999 "08 08 08 12 08 32"; sleep 0.3
  # um número comum não liga de volta
  hmi 1001 0 1005 "$DISCA"; sleep 0.4; hmi 1001 0 1001 "08 02"; sleep 0.4; n0=$(wc -l < $T/escuta.log); sleep 6.2
  igual numero_comum_nao_liga_de_volta "$(desde $n0 | grep -c "^$ME inst=0 api=9 ")" 0
  # o combinado sai como chamada comum
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1005 "$VOLTA"; sleep 0.4
  desde $n0 > $T/v0.log
  tem combinado_sai_como_chamada_comum "^$ME inst=0 api=9 pb=0a 0d 08 02 10 02 $CV\$" $T/v0.log
  hmi 1001 0 1001 "08 02"; sleep 0.4; n0=$(wc -l < $T/escuta.log)
  sleep 3; igual a_volta_nao_vem_antes_dos_cinco_segundos "$(desde $n0 | grep -c "^$ME inst=0 api=9 ")" 0
  sleep 3.2; desde $n0 > $T/v1.log
  tem chamada_recebida_entra_tocando "^$ME inst=0 api=9 pb=0a 0d 08 02 10 06 $CV\$" $T/v1.log
  tem chamada_recebida_poe_o_aparelho_em_chamada "^$ME inst=0 api=4 pb=$EMCH\$" $T/v1.log
  igual chamada_recebida_nao_mexe_na_instancia_1 "$(grep -c "^$ME inst=1 " $T/v1.log)" 0
  tem chamada_recebida_fica_registrada 'chamada recebida simulada' $T/m8.log
  # atender: a resposta (resultado 0) e a chamada em curso (4)
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1000 ""; sleep 0.4
  desde $n0 > $T/v2.log
  tem atender_e_respondido "^$ME inst=0 api=2002 pb=\$" $T/v2.log
  tem atender_poe_a_chamada_em_curso "^$ME inst=0 api=9 pb=0a 0d 08 02 10 08 $CV\$" $T/v2.log
  # encerrar a chamada atendida; a volta é uma só
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1001 "08 02"; sleep 0.4
  tem encerrar_a_chamada_atendida "^$ME inst=0 api=9 pb=10 02\$" $T/escuta.log
  n0=$(wc -l < $T/escuta.log); sleep 6.2
  igual a_volta_e_uma_so "$(desde $n0 | grep -c "^$ME inst=0 api=9 ")" 0
  # atender sem chamada tocando não é permitido (7 = RESULT_ERROR_NOT_ALLOWED, "08 0e")
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1000 ""; sleep 0.3
  desde $n0 > $T/v3.log
  tem atender_sem_chamada_nao_e_permitido "^$ME inst=0 api=2002 pb=08 0e\$" $T/v3.log
  # recusar: com a chamada tocando, o hangupCall a tira e o aparelho volta a "ligado"
  hmi 1001 0 1005 "$VOLTA"; sleep 0.3; hmi 1001 0 1001 "08 02"; sleep 5.9
  n0=$(wc -l < $T/escuta.log); hmi 1001 0 1001 "08 02"; sleep 0.4
  desde $n0 > $T/v4.log
  tem recusar_tira_a_chamada "^$ME inst=0 api=9 pb=10 02\$" $T/v4.log
  tem recusar_volta_o_aparelho_a_ligado "^$ME inst=0 api=4 pb=$LIG\$" $T/v4.log
  tem recusar_e_respondido "^$ME inst=0 api=2010 pb=\$" $T/v4.log
  # a chamada recebida que ninguém atende para de tocar aos 20 s
  hmi 1001 0 1005 "$VOLTA"; sleep 0.3; hmi 1001 0 1001 "08 02"; sleep 5.9; n0=$(wc -l < $T/escuta.log)
  sleep 12; igual chamada_recebida_ainda_toca_aos_doze_segundos "$(desde $n0 | grep -c "^$ME inst=0 api=9 pb=10 02\$")" 0
  sleep 9.5
  desde $n0 > $T/v5.log
  tem chamada_recebida_para_de_tocar_aos_vinte_segundos "^$ME inst=0 api=9 pb=10 02\$" $T/v5.log
  tem chamada_perdida_fica_registrada 'ninguém atendeu' $T/m8.log
  # desligar o Bluetooth antes da volta cancela a volta
  hmi 1001 0 1005 "$VOLTA"; sleep 0.3; hmi 1001 0 1001 "08 02"; sleep 0.3; hmi 1 0 1012 "08 02"; sleep 0.4
  n0=$(wc -l < $T/escuta.log); sleep 6.2
  igual bluetooth_desligado_cancela_a_volta "$(desde $n0 | grep -c "^$ME inst=0 api=9 ")" 0
  naotem chamada_recebida_nenhum_pedido_sem_resposta 'pedido sem resposta' $T/m8.log
  desce
else
  nok chamada_recebida_modelo_existe "não achei $TB, $C ou $P"
fi

# ---- o PAREAMENTO (12/10/2026, o último ponto do item 7). Esta variante não busca aparelhos pela
# tela: a tela "Celulares conhecidos" manda procurar a central pelo celular. O caminho que ela tem
# é o inverso: o aparelho pede o pareamento e a central confirma um código. O gatilho é a própria
# tela: ao abri-la a HMI manda requestReconnectSuspend com suspend = verdadeiro ("08 01") ao gerente
# de aparelhos, e ao fechá-la, com falso (pedido vazio); 4 s depois de aberta, um SEGUNDO aparelho
# FICTÍCIO (stubs/telefone/segundo-aparelho.txt) pede para parear: o estado do código (atributo 7
# do Bluetooth) vai a PS_SSP_SHOW_AND_CONFIRM (7), com o nome, o endereço e o código inventados.
# "Sim" e "Cancelar" são o requestPasskeyResponse (api 1009), com 1 ou 0 no fim; a HMI não lê a
# resposta (2008), só o estado, que volta a zero. Os bytes dos pedidos são os da rodada "j1".
S2=$TEL/segundo-aparelho.txt
NOME2="43 65 6c 75 6c 61 72 20 66 69 63 74 c3 ad 63 69 6f 20 32"; END2="30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 30 3a 30 32"
PEDE="0a 32 08 0e 12 13 $NOME2 1a 11 $END2 22 06 31 32 33 34 35 36 10 02"
ID2="61 70 61 72 65 6c 68 6f 2d 66 69 63 74 69 63 69 6f 2d 32"
BTE="canal=0x100 if=2 inst=0"
if [ -x "$TB" ] && [ -f "$C" ] && [ -f "$P" ] && [ -f "$D" ] && [ -f "$S2" ]; then
  PROG=telefonestub sobe $T/m9.log "$P" "$C" "$D" "$S2"
  hmi 1 0 9999 "08 0e"; hmi 600 0 9999 "08 02"; sleep 0.4      # o atributo 7 do Bluetooth e o 2 (a lista) do gerente
  tem na_partida_ninguem_pede_para_parear "^$BTE api=7 pb=10 02\$" $T/escuta.log
  sleep 5.2
  igual sem_a_tela_aberta_ninguem_pede "$(conta "^$BTE api=7 pb=0a " $T/escuta.log)" 0
  # abrir a tela: o pedido de suspender a reconexão é respondido, e o aparelho pede aos 4 s
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1004 "08 01"; sleep 0.4
  tem abrir_a_tela_e_respondido "^$PM api=2004 pb=\$" $T/escuta.log
  sleep 2.2; igual o_pedido_nao_vem_antes_dos_quatro_segundos "$(desde $n0 | grep -c "^$BTE api=7 ")" 0
  sleep 3
  desde $n0 > $T/p1.log
  tem segundo_aparelho_pede_para_parear "^$BTE api=7 pb=$PEDE\$" $T/p1.log
  tem pedido_de_pareamento_fica_registrado 'pede para parear' $T/m9.log
  sleep 6; igual o_pedido_e_um_por_abertura "$(desde $n0 | grep -c "^$BTE api=7 pb=0a ")" 1
  # "Cancelar": respondido, o estado do código volta a zero e o aparelho NÃO entra na lista
  n0=$(wc -l < $T/escuta.log); hmi 1 0 1009 "12 11 $END2"; sleep 0.5
  desde $n0 > $T/p2.log
  tem cancelar_e_respondido "^$BTE api=2008 pb=0a 11 $END2 12 13 $NOME2\$" $T/p2.log
  tem cancelar_volta_o_estado_do_codigo_a_zero "^$BTE api=7 pb=10 02\$" $T/p2.log
  igual cancelar_nao_poe_o_aparelho_na_lista "$(grep -c "$ID2" $T/escuta.log)" 0
  # fechar e reabrir a tela: pede de novo
  hmi 600 0 1004 ""; sleep 0.3; n0=$(wc -l < $T/escuta.log); hmi 600 0 1004 "08 01"; sleep 5.4
  desde $n0 > $T/p3.log
  tem reabrir_a_tela_pede_de_novo "^$BTE api=7 pb=$PEDE\$" $T/p3.log
  # fechar a tela com o pedido no ar retira o pedido
  n0=$(wc -l < $T/escuta.log); hmi 600 0 1004 ""; sleep 0.4
  desde $n0 > $T/p4.log
  tem fechar_a_tela_retira_o_pedido "^$BTE api=7 pb=10 02\$" $T/p4.log
  # "Sim": respondido, o estado volta a zero e o aparelho entra na lista de conhecidos do gerente,
  # conhecido e não conectado (2 = SERVICESTATE_SUPPORTED), depois do primeiro
  hmi 600 0 1004 "08 01"; sleep 5.4; n0=$(wc -l < $T/escuta.log)
  hmi 1 0 1009 "0a 06 31 32 33 34 35 36 12 11 $END2 18 02"; sleep 0.5
  desde $n0 > $T/p5.log
  tem sim_e_respondido "^$BTE api=2008 pb=0a 11 $END2 12 13 $NOME2\$" $T/p5.log
  tem sim_volta_o_estado_do_codigo_a_zero "^$BTE api=7 pb=10 02\$" $T/p5.log
  tem sim_poe_o_segundo_aparelho_na_lista "^$PM api=2 pb=0a .* $ID1 .* 0a .*$END2.* $NOME2 .* $ID2 .*10 02\$" $T/p5.log
  tem pareamento_fica_registrado 'pareei' $T/m9.log
  # pareado, não pede mais: nem nesta abertura nem na seguinte
  hmi 600 0 1004 ""; sleep 0.3; n0=$(wc -l < $T/escuta.log); hmi 600 0 1004 "08 01"; sleep 6
  igual pareado_nao_pede_mais "$(desde $n0 | grep -c "^$BTE api=7 pb=0a ")" 0
  # resposta de código sem pedido no ar: 4 = RESULT_ERROR_PAIRING_GENERAL ("18 08")
  n0=$(wc -l < $T/escuta.log); hmi 1 0 1009 "12 11 $END2"; sleep 0.4
  desde $n0 > $T/p6.log
  tem resposta_sem_pedido_da_erro "^$BTE api=2008 pb=.*18 08\$" $T/p6.log
  naotem pareamento_nenhum_pedido_sem_resposta 'pedido sem resposta' $T/m9.log
  desce
  # sem o perfil do segundo aparelho ninguém pede para parear
  PROG=telefonestub sobe $T/m10.log "$P" "$C" "$D"
  hmi 1 0 9999 "08 0e"; hmi 600 0 1004 "08 01"; sleep 6
  igual sem_o_segundo_aparelho_ninguem_pede "$(conta "^$BTE api=7 pb=0a " $T/escuta.log)" 0
  tem sem_o_segundo_aparelho_a_tela_e_respondida "^$PM api=2004 pb=\$" $T/escuta.log
  desce
else
  nok pareamento_perfil_existe "não achei $TB, $C, $D ou $S2"
fi

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- o que foi escutado (fim) ---"; tail -n 12 $T/escuta.log 2>/dev/null | cut -c1-200; echo "--- stub ---"; tail -n 12 $T/stub.log 2>/dev/null | cut -c1-200; }
[ $fail -eq 0 ]
