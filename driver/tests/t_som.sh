#!/bin/sh
# Teste do som de mentira (driver/somstub.c e stubs/som/), de 12/10/2026.
#
# O som desta HMI vem pelo serviço RSI "audiomanagement". Na bancada quem o serve é o mock RSI da
# LG (/lge/app_ro/bin/rsi-mockup), com os dados de stubs/som/gerado/ (o que o gerente de áudio do
# firmware serve em repouso). O mock guarda o que lhe gravam e avisa quem assina, mas não tem
# regra nenhuma, e recusa gravação na saída de som (403, como manda a especificação do serviço).
# O somstub põe o que falta:
#   1. o perfil (a saída de som ligada, o amplificador, a fonte que toca ao ligar), aplicado aos
#      arquivos de dados antes de o mock subir (somstub -w), e a especificação do serviço
#      afrouxada em dois pontos em que o mock é mais estreito que o serviço de verdade (somstub -s):
#      a assinatura de recurso dos conjuntos de som, sem a qual a HMI nunca assina grave, agudo e
#      balanço, e o apagamento dos controles atendidos;
#   2. as regras mínimas do gerente de áudio: um controle de volume muda o volume do foco de
#      ajuste, um controle de mudo liga e desliga o mudo do usuário, e a conexão de entretenimento
#      que a HMI inicia passa a ser a do objeto global;
#   3. o papel do touch-app: no carro é ele quem cria o controle de volume quando o botão gira
#      (visto no binário: "/audiomanagement/controls", "volumeUp_gui", "volumeIncrease"). Aqui o
#      botão vem do dsistub, pelo barramento, e o somstub cria o controle. O mesmo vale para a
#      tecla de mudo, que a HMI deixou de tratar.
#
#   4. a memória (-m <arquivo>, de 12/10/2026, de manhã): o que a tela mudou (volumes, parâmetros
#      de som e o mudo) é regravado no arquivo a cada mudança e, na partida seguinte, aplicado por
#      cima do perfil. Linha estragada do arquivo é ignorada com aviso: a central sobe do mesmo jeito.
#
#   5. o volante abrir a barra de volume (volante_abre_a_barra = sim no perfil; SIMULAÇÃO, de
#      12/10/2026, à tarde). A HMI deste firmware só abre a barra com o giro do botão da central;
#      com a opção, a tecla de volume do volante faz o somstub mandar o dsistub girar também o
#      botão (uma linha "encoder 13 17 ±n" no arquivo de comandos dele). O valor muda um passo
#      só, pelo caminho do botão. Sem a opção, fica como antes: o volume muda e a barra não abre.
#
# Parte 1, sem firmware: o somstub confere o perfil contra os dados gerados (-n), escreve os dados
# com o perfil aplicado (-w), com e sem memória (-m), e afrouxa uma especificação de mentira (-s).
# Parte 2, com o firmware em /fw (como nas suítes): sobe hub, registro e mock numa montagem
# própria e faz o papel da HMI com o rsi-send do firmware. Sem /fw a parte 2 é pulada.
#   t_som.sh <pasta com kipc_hub, kipc_bus.so, dsistub e somstub> [pasta stubs/som]
BIN=${1:-/work/bin}; D=$(cd "$(dirname "$0")/.." && pwd); SOM=${2:-$D/../stubs/som}
T=/tmp/t_som; LOW=$T/low; FW=${FW:-/fw}
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
PIDS=
limpa() {      # os processos de fundo vão por número: no dash, $(jobs -p) roda num subshell e não vê nenhum
  [ -n "$PIDS" ] && { kill $PIDS 2>/dev/null; sleep 0.3; kill -9 $PIDS 2>/dev/null; wait 2>/dev/null; }
  for m in proc dev/shm data persist tmp run dev var media ""; do umount "$LOW/$m" 2>/dev/null; done
}
trap limpa EXIT
limpa; rm -rf $T; mkdir -p $T $LOW
G=$SOM/gerado; P=$SOM/som-de-mentira.txt

# ---------------------------------------------------------------- parte 1: -n, sem firmware
[ -x "$BIN/somstub" ] || nok somstub_compilado "não achei $BIN/somstub"
out=$("$BIN/somstub" -n "$G" "$P" 2>&1); rc=$?
igual perfil_da_pasta_e_aceito "$rc" 0
case "$out" in *"9 recursos"*"elementos"*) ok confere_diz_o_que_leu ;; *) nok confere_diz_o_que_leu "saída: $out" ;; esac

perfil() { printf '%s\n' "$@" > $T/p.txt; "$BIN/somstub" -n "$G" $T/p.txt > $T/p.out 2>&1; echo $?; }
recusa() {   # recusa <nome> <texto que tem de aparecer> <linhas do perfil…>
  nome=$1; texto=$2; shift 2
  rc=$(perfil "$@")
  if [ "$rc" != 0 ] && grep -q -- "$texto" $T/p.out; then ok "$nome"; else nok "$nome" "rc=$rc, saída: $(head -c 300 $T/p.out)"; fi
}
recusa elemento_que_nao_existe_e_recusado_com_a_linha 'p.txt:2' '[sinks.entertainment]' '[volumesettings.VolumeQueNaoExiste]' 'value = 3'
recusa recurso_que_nao_existe_e_recusado 'p.txt:1' '[amplificadores.entertainment]' 'x = 1'
recusa campo_que_o_elemento_nao_tem_e_recusado 'p.txt:2' '[volumesettings.VolumeEntertainment]' 'valor = 3'
recusa valor_que_nao_e_json_e_recusado 'p.txt:2' '[sinks.entertainment]' 'amplifierId = VWBAS000'
recusa tipo_trocado_e_recusado 'p.txt:2' '[volumesettings.VolumeEntertainment]' 'value = "oito"'
recusa linha_solta_e_recusada 'p.txt:1' 'isto não é nada'
recusa fonte_inicial_que_nao_existe_e_recusada 'p.txt:1' 'entretenimento = ENT_AMP_QUE_NAO_EXISTE'
recusa fonte_inicial_que_nao_e_de_entretenimento_e_recusada 'p.txt:1' 'entretenimento = FCT_AMP_USER_MUTE'
igual perfil_vazio_e_aceito "$(perfil '# só comentário' '')" 0
igual estrela_vale_para_todo_elemento "$(perfil '[volumesettings.*]' 'locked = false')" 0
recusa estrela_com_campo_que_falta_e_recusada 'p.txt:2' '[volumesettings.*]' 'trancado = false'
mkdir -p $T/sem && cp "$G"/*.json $T/sem/ 2>/dev/null; rm -f $T/sem/sinks.json
"$BIN/somstub" -n $T/sem "$P" > $T/sem.out 2>&1; rc=$?
if [ $rc != 0 ] && grep -q 'sinks' $T/sem.out; then ok dados_sem_um_recurso_sao_recusados; else nok dados_sem_um_recurso_sao_recusados "rc=$rc $(head -c 200 $T/sem.out)"; fi

# -w: os dados com o perfil e o estado de partida, que é o que o mock vai ler
cat > $T/perfil-de-teste.txt <<'FIM'
# perfil só deste teste
entretenimento = ENT_AMP_TUNER
[sinks.entertainment]
connectStatus = "connected"
audioStatus = "audible"
amplifierId = "T1BASXYZ"
availableSoundFunctions = ["balance","fader","eq1","eq3","eq5"]
eqFrequencies = [80,900,11000]
[volumesettings.VolumeEntertainment]
value = 8
[volumesettings.*]
locked = false
FIM
"$BIN/somstub" -w $T/prep "$G" $T/perfil-de-teste.txt > $T/w.out 2>&1; rc=$?
igual escreve_os_dados_com_o_perfil "$rc" 0
igual escreve_os_nove_recursos "$(ls $T/prep/audiomanagement 2>/dev/null | wc -l | tr -d ' ')" 9
linha() { grep "\"name\":\"$2\"" "$T/prep/audiomanagement/$1.json" 2>/dev/null | head -n 1; }      # linha <recurso> <nome>
tem() { case "$2" in *"$3"*) ok "$1" ;; *) nok "$1" "não achei $3 em: $(printf '%s' "$2" | head -c 160)" ;; esac; }
tem escrito_saida_ligada "$(linha sinks entertainment)" '"connectStatus":"connected"'
tem escrito_amplificador "$(linha sinks entertainment)" '"amplifierId":"T1BASXYZ"'
tem escrito_funcoes_de_som "$(linha sinks entertainment)" '"availableSoundFunctions":["balance","fader","eq1","eq3","eq5"]'
tem escrito_volume_inicial "$(linha volumesettings VolumeEntertainment)" '"value":8,"valueMax":30'
igual escrito_estrela_destrava_todos "$(grep -c '"locked":true' $T/prep/audiomanagement/volumesettings.json 2>/dev/null)" 0
tem escrito_fonte_iniciada "$(linha connections ENT_AMP_TUNER)" '"status":"started"'
tem escrito_outra_fonte_parada "$(linha connections ENT_AMP_MEDIA_PLAYER)" '"status":"stopped"'
gl=$(cat $T/prep/audiomanagement/globals.json 2>/dev/null)
tem escrito_global_com_entretenimento "$gl" '"entertainmentConnection":{"id":"646fda12-f747-5a7c-afc1-d917b971003c","name":"ENT_AMP_TUNER"'
tem escrito_global_com_foco "$gl" '"settingFocus":{"id":"646fda12-f747-5a7c-afc1-d917b971003c","name":"ENT_AMP_TUNER"'
tem escrito_global_com_primeiro_plano "$gl" '"foregroundConnection":{"id":"646fda12-f747-5a7c-afc1-d917b971003c","name":"ENT_AMP_TUNER"'
# a HMI pede o global com $expand e o mock não expande: a conexão vai embutida por inteiro, com o
# volume, a saída já ligada e os parâmetros de som
tem escrito_global_com_o_volume_embutido "$gl" '"name":"VolumeEntertainment"'
tem escrito_global_com_o_valor_do_perfil "$gl" '"value":8,"valueMax":30'
tem escrito_global_com_a_saida_ligada "$gl" '"amplifierId":"T1BASXYZ"'
tem escrito_global_com_os_parametros "$gl" '"name":"balance","uri":"\/audiomanagement\/soundparameters\/'
if cmp -s "$G/audiomanagement/soundparameters.json" $T/prep/audiomanagement/soundparameters.json; then ok o_que_o_perfil_nao_toca_sai_igual_byte_a_byte; else nok o_que_o_perfil_nao_toca_sai_igual_byte_a_byte "soundparameters.json mudou"; fi
"$BIN/somstub" -w $T/prep2 "$G" $T/perfil-de-teste.txt > /dev/null 2>&1
if [ -d $T/prep2 ] && diff -r $T/prep $T/prep2 > /dev/null 2>&1; then ok escrever_duas_vezes_da_o_mesmo; else nok escrever_duas_vezes_da_o_mesmo "as duas pastas diferem"; fi
# a prova do leitor e do escritor de JSON: sem perfil, sai o que entrou, byte a byte, nos nove arquivos
: > $T/vazio.txt; "$BIN/somstub" -w $T/igual "$G" $T/vazio.txt > /dev/null 2>&1
dif=0; for f in "$G"/audiomanagement/*.json; do cmp -s "$f" "$T/igual/audiomanagement/$(basename "$f")" || dif=$((dif+1)); done
igual sem_perfil_escreve_os_mesmos_bytes_que_leu "$dif arquivos diferentes de $(ls "$G"/audiomanagement/*.json 2>/dev/null | wc -l | tr -d ' ')" "0 arquivos diferentes de 9"
"$BIN/somstub" -w $T/ruim "$G" $T/p.txt > /dev/null 2>&1; rc=$?        # o p.txt ficou com o último perfil recusado
if [ $rc != 0 ] && [ ! -d $T/ruim ]; then ok perfil_recusado_nao_escreve_nada; else nok perfil_recusado_nao_escreve_nada "rc=$rc, pasta criada: $(ls $T/ruim 2>/dev/null)"; fi
"$BIN/somstub" $T/prep $T/perfil-de-teste.txt > $T/u.out 2>&1; rc=$?
igual no_ar_nao_aceita_perfil "$rc" 2
"$BIN/somstub" -w $T/da-pasta "$G" "$P" > $T/w2.out 2>&1; rc=$?
igual perfil_da_pasta_escreve "$rc" 0
tem perfil_da_pasta_liga_a_saida "$(grep '"name":"entertainment"' $T/da-pasta/audiomanagement/sinks.json 2>/dev/null)" '"connectStatus":"connected"'

# volante_abre_a_barra: chave do perfil, antes das seções; o -w a leva para <destino>/opcoes.txt, que o programa lê no ar
tem perfil_da_pasta_liga_a_barra_pelo_volante "$(cat $T/da-pasta/opcoes.txt 2>/dev/null)" 'volante_abre_a_barra = sim'
if grep -B8 '^volante_abre_a_barra' "$P" | grep -q 'SIMULAÇÃO'; then ok perfil_da_pasta_diz_que_a_barra_pelo_volante_e_simulacao; else nok perfil_da_pasta_diz_que_a_barra_pelo_volante_e_simulacao "sem a palavra perto da chave"; fi
tem perfil_sem_a_chave_deixa_desligado "$(cat $T/prep/opcoes.txt 2>/dev/null)" 'volante_abre_a_barra = nao'
printf '%s\n' 'volante_abre_a_barra = sim' > $T/pv1.txt; printf '%s\n' 'volante_abre_a_barra = nao' > $T/pv2.txt
"$BIN/somstub" -w $T/prepv1 "$G" $T/perfil-de-teste.txt $T/pv1.txt > /dev/null 2>&1; rc=$?
if [ $rc = 0 ] && grep -q '^volante_abre_a_barra = sim$' $T/prepv1/opcoes.txt 2>/dev/null; then ok chave_do_volante_sim_e_aceita; else nok chave_do_volante_sim_e_aceita "rc=$rc; $(cat $T/prepv1/opcoes.txt 2>/dev/null)"; fi
"$BIN/somstub" -w $T/prepv2 "$G" $T/perfil-de-teste.txt $T/pv1.txt $T/pv2.txt > /dev/null 2>&1; rc=$?
if [ $rc = 0 ] && grep -q '^volante_abre_a_barra = nao$' $T/prepv2/opcoes.txt 2>/dev/null; then ok perfil_de_depois_desliga_a_chave_do_volante; else nok perfil_de_depois_desliga_a_chave_do_volante "rc=$rc; $(cat $T/prepv2/opcoes.txt 2>/dev/null)"; fi
recusa chave_do_volante_com_valor_estranho 'p.txt:1' 'volante_abre_a_barra = talvez'
if diff -r $T/prep/audiomanagement $T/prepv1/audiomanagement > /dev/null 2>&1; then ok chave_do_volante_nao_mexe_nos_dados; else nok chave_do_volante_nao_mexe_nos_dados "os dados mudaram"; fi

# -m: a memória. O que a tela mudou na sessão anterior entra por cima do perfil, antes de o mock subir.
cat > $T/mem.txt <<'FIM'
# memória do som de mentira
mudo = ligado
[soundparameters.EQ1]
value = 3
[soundparameters.balance]
value = -2
[volumesettings.VolumeEntertainment]
value = 11
FIM
"$BIN/somstub" -w $T/prepm -m $T/mem.txt "$G" $T/perfil-de-teste.txt > $T/wm.out 2>&1; rc=$?
igual memoria_e_aceita_na_escrita "$rc" 0
linham() { grep "\"name\":\"$2\"" "$T/prepm/audiomanagement/$1.json" 2>/dev/null | head -n 1; }
tem memoria_traz_o_volume "$(linham volumesettings VolumeEntertainment)" '"value":11,"valueMax":30'
tem memoria_traz_o_grave "$(linham soundparameters EQ1)" '"value":3,'
tem memoria_traz_o_balanco "$(linham soundparameters balance)" '"value":-2,'
tem memoria_traz_o_mudo "$(linham connections FCT_AMP_USER_MUTE)" '"status":"started"'
glm=$(cat $T/prepm/audiomanagement/globals.json 2>/dev/null)
tem memoria_mudo_leva_o_foco "$glm" '"settingFocus":{"id":"646fda12-f747-5a7c-afc1-d917b9710002","name":"FCT_AMP_USER_MUTE"'
tem memoria_mudo_leva_o_primeiro_plano "$glm" '"foregroundConnection":{"id":"646fda12-f747-5a7c-afc1-d917b9710002","name":"FCT_AMP_USER_MUTE"'
tem memoria_mudo_nao_troca_a_fonte "$glm" '"entertainmentConnection":{"id":"646fda12-f747-5a7c-afc1-d917b971003c","name":"ENT_AMP_TUNER"'
# o volume que vai embutido no objeto global (a HMI o lê de lá na partida) tem de ser o da memória
tem memoria_volume_vai_embutido_no_global "$glm" '"value":11,"valueMax":30'
tem memoria_diz_o_que_aplicou "$(cat $T/wm.out)" 'memória aplicada: 3 valores e o mudo (ligado)'
# o que a memória não toca sai como sem ela
if cmp -s $T/prep/audiomanagement/sinks.json $T/prepm/audiomanagement/sinks.json && cmp -s $T/prep/audiomanagement/soundsettings.json $T/prepm/audiomanagement/soundsettings.json; then ok memoria_nao_mexe_no_resto; else nok memoria_nao_mexe_no_resto "saída de som ou conjuntos de som mudaram"; fi
# arquivo que ainda não existe, ou vazio: a central nasce do perfil, igual a sem -m
"$BIN/somstub" -w $T/prepn -m $T/nao-existe.txt "$G" $T/perfil-de-teste.txt > $T/wn.out 2>&1; rc=$?
if [ $rc = 0 ] && diff -r $T/prep $T/prepn > /dev/null 2>&1; then ok memoria_que_nao_existe_nasce_do_perfil; else nok memoria_que_nao_existe_nasce_do_perfil "rc=$rc ou as pastas diferem"; fi
: > $T/mem-vazia.txt; "$BIN/somstub" -w $T/prepv -m $T/mem-vazia.txt "$G" $T/perfil-de-teste.txt > /dev/null 2>&1; rc=$?
if [ $rc = 0 ] && diff -r $T/prep $T/prepv > /dev/null 2>&1; then ok memoria_vazia_nasce_do_perfil; else nok memoria_vazia_nasce_do_perfil "rc=$rc ou as pastas diferem"; fi
# arquivo estragado não impede a central de subir: o que presta entra, o resto é ignorado com aviso
cat > $T/mem-ruim.txt <<'FIM'
mudo = talvez
linha solta
[volumesettings.NaoExiste]
value = 3
[soundparameters.EQ1]
value = "alto"
value = 99
locked = true
[sinks.entertainment]
amplifierId = "TROCADO"
[connections.ENT_AMP_TUNER]
status = "stopped"
[soundparameters.EQ5]
value = -1
[volumesettings.VolumeEntertainment]
value = 31
FIM
"$BIN/somstub" -w $T/prepr -m $T/mem-ruim.txt "$G" $T/perfil-de-teste.txt > $T/wr.out 2>&1; rc=$?
igual memoria_estragada_nao_impede_a_partida "$rc" 0
linhar() { grep "\"name\":\"$2\"" "$T/prepr/audiomanagement/$1.json" 2>/dev/null | head -n 1; }
tem memoria_estragada_aplica_o_que_presta "$(linhar soundparameters EQ5)" '"value":-1,'
tem memoria_fora_da_faixa_e_ignorada "$(linhar soundparameters EQ1)" '"value":0,'
tem memoria_volume_fora_da_faixa_e_ignorado "$(linhar volumesettings VolumeEntertainment)" '"value":8,"valueMax":30'
tem memoria_nao_mexe_na_saida_de_som "$(linhar sinks entertainment)" '"amplifierId":"T1BASXYZ"'
tem memoria_nao_mexe_nas_conexoes "$(linhar connections ENT_AMP_TUNER)" '"status":"started"'
tem memoria_mudo_estragado_fica_desligado "$(linhar connections FCT_AMP_USER_MUTE)" '"status":"stopped"'
igual memoria_estragada_avisa_cada_linha "$(grep -c 'memória: ignorei mem-ruim.txt:' $T/wr.out)" 9
tem memoria_estragada_diz_o_motivo "$(cat $T/wr.out)" 'mem-ruim.txt:7: 99 está fora da faixa de EQ1 (-9 a 9)'
tem memoria_estragada_diz_o_que_aplicou "$(cat $T/wr.out)" 'memória aplicada: 1 valor'

# -s: a especificação afrouxada (aqui uma de mentira, com a forma da do firmware)
cat > $T/espec.json <<'FIM'
{"name":"audiomanagement","description":"de mentira","resources":{
 "controls":{"description":"c","endpoints":{"element":{},"resource":{"get":{},"post":{"parameters":{"x":1}}}},"systemTriggeredEvents":["resource"]},
 "soundsettings":{"description":"s","endpoints":{"element":{"get":{}},"resource":{"get":{}}},"systemTriggeredEvents":["element"]},
 "sinks":{"description":"k","endpoints":{"element":{"get":{}},"resource":{"get":{}}},"systemTriggeredEvents":["resource","element"]}}}
FIM
"$BIN/somstub" -s $T/espec.json $T/espec2.json > $T/s.out 2>&1; rc=$?
igual afrouxa_a_especificacao "$rc" 0
e2=$(cat $T/espec2.json 2>/dev/null)
tem conjuntos_de_som_passam_a_avisar_o_recurso "$e2" '"soundsettings":{"description":"s","endpoints":{"element":{"get":{}},"resource":{"get":{}}},"systemTriggeredEvents":["element","resource"]}'
tem controles_passam_a_aceitar_apagar "$e2" '"controls":{"description":"c","endpoints":{"element":{"delete":{}},"resource":{"get":{},"post":{"parameters":{"x":1}}}},"systemTriggeredEvents":["resource"]}'
tem o_resto_da_especificacao_fica_igual "$e2" '"sinks":{"description":"k","endpoints":{"element":{"get":{}},"resource":{"get":{}}},"systemTriggeredEvents":["resource","element"]}'
tem afrouxar_diz_o_que_mudou "$(cat $T/s.out)" 'soundsettings passa a avisar assinatura de recurso; controls passa a aceitar DELETE de elemento'
"$BIN/somstub" -s $T/espec2.json $T/espec3.json > $T/s2.out 2>&1
if cmp -s $T/espec2.json $T/espec3.json; then ok afrouxar_duas_vezes_nao_muda_mais_nada; else nok afrouxar_duas_vezes_nao_muda_mais_nada "a segunda passada mudou o arquivo"; fi
printf '{"name":"outro","resources":{}}\n' > $T/espec-ruim.json
"$BIN/somstub" -s $T/espec-ruim.json $T/espec4.json > $T/s3.out 2>&1; rc=$?
if [ $rc != 0 ] && [ ! -f $T/espec4.json ]; then ok especificacao_estranha_e_recusada; else nok especificacao_estranha_e_recusada "rc=$rc"; fi

# ---------------------------------------------------------------- parte 2: com o firmware
if [ ! -f "$FW/system.img" ]; then
  echo "PULOU a parte com o mock RSI: não há firmware em $FW"
  echo "RESUMO: $pass passaram, $fail falharam (parte 2 pulada)"; [ $fail -eq 0 ]; exit
fi
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
mount -o loop,ro,noload "$FW/system.img" $LOW || { nok monta_o_firmware "mount falhou"; echo "RESUMO: $pass passaram, $fail falharam"; exit 1; }
for d in data persist tmp run dev var media; do mount -t tmpfs tmpfs $LOW/$d; done
mkdir -p $LOW/dev/shm; mount -t tmpfs tmpfs $LOW/dev/shm; mount -t proc proc $LOW/proc
mknod -m 666 $LOW/dev/null c 1 3; mknod -m 666 $LOW/dev/zero c 1 5; mknod -m 666 $LOW/dev/urandom c 1 9; mknod -m 666 $LOW/dev/random c 1 8
cp "$BIN/kipc_hub" "$BIN/kipc_bus.so" "$BIN/somstub" "$BIN/dsistub" $LOW/tmp/
chroot $LOW /tmp/kipc_hub 2>$LOW/tmp/hub.log &
PIDS="$PIDS $!"
sleep 1
SAIM=$LOW/lge/app_ro/sai/mockup
# a central "nasce de uma sessão anterior": a memória traz os médios em 4, e os dados do mock saem dela
printf '%s\n' '# memória do som de mentira' 'mudo = desligado' '[soundparameters.EQ3]' 'value = 4' > $T/mem0.txt
"$BIN/somstub" -w $T/prep -m $T/mem0.txt "$G" $T/perfil-de-teste.txt > $T/w0.out 2>&1 || nok memoria_prepara_os_dados_do_mock "$(tail -n 2 $T/w0.out)"
mkdir -p $LOW/tmp/mem && cp $T/mem0.txt $LOW/tmp/mem/estado.txt
MEM=$LOW/tmp/mem/estado.txt
memv() { sed -n "/^\[$1\.$2\]\$/{n;s/^value = //p;}" $MEM 2>/dev/null; }      # memv <recurso> <nome>: o valor guardado
memmudo() { sed -n 's/^mudo = //p' $MEM 2>/dev/null; }
mkdir -p $LOW/tmp/rsi-mockup $LOW/tmp/som && cp -r $SAIM/. $LOW/tmp/rsi-mockup/ && cp -r $T/prep/audiomanagement $LOW/tmp/rsi-mockup/
cp -r $T/prep/audiomanagement $LOW/tmp/som/
printf '\n%s\n' audiomanagement:mixer >> $LOW/tmp/rsi-mockup/services
mount -o bind $LOW/tmp/rsi-mockup $SAIM
ESPEC=$(ls $LOW/lge/app_ro/sai/spec/viwi.service.audiomanagement/viwi.service.audiomanagement-*.json | head -n 1)
chroot $LOW /tmp/somstub -s "${ESPEC#$LOW}" /tmp/som/especificacao.json > $T/s-fw.out 2>&1; rc=$?
igual afrouxa_a_especificacao_do_firmware "$rc" 0
tem especificacao_do_firmware_precisava_dos_dois "$(cat $T/s-fw.out)" 'soundsettings passa a avisar assinatura de recurso; controls passa a aceitar DELETE de elemento'
mount -o bind $LOW/tmp/som/especificacao.json "$ESPEC" || nok monta_a_especificacao "mount falhou"
chroot $LOW /bin/sh -c "export LD_PRELOAD=/tmp/kipc_bus.so; exec /lge/app_ro/sai/MIB_ServiceRegistryApp-00" >$LOW/tmp/registry.log 2>&1 &
PIDS="$PIDS $!"
sleep 3
chroot $LOW /lge/app_ro/bin/rsi-mockup >$LOW/tmp/rsi-mockup.log 2>&1 &
PIDS="$PIDS $!"
sleep 2

# quem faz o papel da HMI: o rsi-send do firmware
get()  { chroot $LOW /lge/app_ro/bin/rsi-send get "$1" 2>/dev/null | sed -n 2p; }
post() { chroot $LOW /lge/app_ro/bin/rsi-send post "$1" "$2" 2>/dev/null | sed -n 1p; }
campo()  { printf '%s' "$1" | grep -o "\"$2\":\(\"[^\"]*\"\|\[[^]]*\]\|[-0-9a-z.]*\)" | head -n 1 | cut -d: -f2- | tr -d '"'; }
dentro() { campo "$(printf '%s' "$1" | sed -n "s/.*\"$2\":{//p")" "$3"; }      # dentro <json> <objeto> <campo>
GLO=/audiomanagement/globals/111ee2c9-d0b3-5d9d-8768-e1a12706b3fa
el()  { get "/audiomanagement/$1?name=$2"; }                                   # el <recurso> <nome>
vol() { campo "$(el volumesettings ${1:-VolumeEntertainment})" value; }
con() { campo "$(el connections $1)" status; }
foco() { dentro "$(get "$GLO")" settingFocus name; }
ID() { campo "$(el $1 $2)" id; }
campo_de() { campo "$(el $1 $2)" $3; }                                      # campo_de <recurso> <nome> <campo>
dentro_global() { dentro "$(get "$GLO")" $1 $2; }                            # dentro_global <objeto> <campo>
conta_controles() { get '/audiomanagement/controls?$limit=1000' | grep -o '"id"' | wc -l | tr -d ' '; }
espera() {   # espera <nome do caso> <esperado> <comando…>: o comando tem de dar o esperado em até 4 s
  nome=$1; esp=$2; shift 2; k=0; v=
  while [ $k -lt 25 ]; do v=$("$@"); [ "$v" = "$esp" ] && break; sleep 0.15; k=$((k+1)); done
  igual "$nome" "$v" "$esp"
}
controle() {  # controle <tipo> <passos> [domínio]: o que o touch-app e a HMI gravam em controls
  post /audiomanagement/controls "{\"name\":\"x_gui\",\"domain\":\"${3:-hmi}\",\"buttonstate\":\"pressed\",\"time\":\"$(date +%s%N | cut -c1-13)\",\"type\":\"$1\",\"value\":$2}" > /dev/null
}

: > $LOW/tmp/cmd
chroot $LOW /tmp/dsistub /run/kipc_hub.sock 800 480 1 /tmp/cmd > $LOW/tmp/dsistub.log 2>&1 &
PIDS="$PIDS $!"
sleep 0.3
chroot $LOW /tmp/somstub -m /tmp/mem/estado.txt /tmp/som > $LOW/tmp/somstub.log 2>&1 &
STUB=$!; PIDS="$PIDS $!"
k=0; while ! grep -q 'no ar\|ERRO' $LOW/tmp/somstub.log && [ $k -lt 100 ]; do sleep 0.1; k=$((k+1)); done
if grep -q 'no ar na porta 7770' $LOW/tmp/somstub.log; then ok somstub_acha_o_servico_e_fica_no_ar; else nok somstub_acha_o_servico_e_fica_no_ar "$(tail -n 2 $LOW/tmp/somstub.log)"; fi
tem memoria_no_ar_diz_onde_guarda "$(cat $LOW/tmp/somstub.log)" 'memória: guardo em /tmp/mem/estado.txt o que a tela mudar (1 valor de antes)'

# ---- 1. o mock serve os dados preparados: é isto que a HMI encontra ao subir
igual perfil_liga_a_saida_de_som "$(campo_de sinks entertainment connectStatus)" connected
igual perfil_da_o_amplificador "$(campo_de sinks entertainment amplifierId)" T1BASXYZ
igual perfil_da_as_funcoes_de_som "$(campo_de sinks entertainment availableSoundFunctions)" '[balance,fader,eq1,eq3,eq5]'
igual perfil_da_as_frequencias "$(campo_de sinks entertainment eqFrequencies)" '[80,900,11000]'
igual perfil_da_o_volume_inicial "$(vol)" 8
igual estrela_destrava_todos_os_volumes "$(get '/audiomanagement/volumesettings?$limit=100' | grep -o '"locked":true' | wc -l | tr -d ' ')" 0
igual o_que_o_perfil_nao_diz_fica_como_o_firmware_da "$(campo_de volumesettings VolumeEntertainment valueMax)" 30
igual fonte_inicial_fica_iniciada "$(con ENT_AMP_TUNER)" started
igual memoria_da_partida_e_servida "$(campo_de soundparameters EQ3 value)" 4
g=$(get "$GLO")
igual global_aponta_a_fonte_de_entretenimento "$(dentro "$g" entertainmentConnection name)" ENT_AMP_TUNER
igual global_aponta_o_primeiro_plano "$(dentro "$g" foregroundConnection name)" ENT_AMP_TUNER
igual global_aponta_o_foco_de_ajuste "$(dentro "$g" settingFocus name)" ENT_AMP_TUNER
igual conexao_embutida_no_global_esta_iniciada "$(dentro "$g" entertainmentConnection status)" started
# a HMI pede o global com $expand: a conexão embutida traz o volume e a saída por inteiro, e já com os valores de agora
ge=$(printf '%s' "$g" | sed -n 's/.*"entertainmentConnection":{//p')
igual volume_embutido_no_global_tem_a_faixa "$(dentro "$ge" volumesetting valueMax)" 30
igual volume_embutido_no_global_tem_o_valor_de_agora "$(dentro "$ge" volumesetting value)" 8
igual saida_embutida_no_global_esta_ligada "$(dentro "$ge" sink connectStatus)" connected
igual outras_fontes_seguem_paradas "$(con ENT_AMP_MEDIA_PLAYER)" stopped

# a HMI acha o conjunto de som "entertainment" por assinatura de RECURSO; sem a especificação afrouxada o mock não responde
chroot $LOW /lge/app_ro/bin/rsi-subscribe '/audiomanagement/soundsettings?name=entertainment&$fields=name' > $T/sub_ss.txt 2>&1 &
PIDS="$PIDS $!"
k=0; while ! grep -q '"type":"data"' $T/sub_ss.txt && [ $k -lt 30 ]; do sleep 0.1; k=$((k+1)); done
tem assinatura_de_recurso_dos_conjuntos_de_som_responde "$(grep '"type":"data"' $T/sub_ss.txt | head -n 1)" '"name":"entertainment"'

# ---- 2. controle de volume: muda o volume da conexão que tem o foco de ajuste
controle volumeIncrease 1;  espera um_passo_para_cima 9 vol
espera memoria_guarda_o_volume_a_cada_passo 9 memv volumesettings VolumeEntertainment
controle volumeIncrease 3;  espera tres_passos_para_cima 12 vol
controle volumeDecrease 2;  espera dois_passos_para_baixo 10 vol
controle volumeIncrease 1 cluster; espera controle_do_painel_tambem_vale 11 vol
k=0; while [ $k -lt 5 ]; do controle volumeIncrease 5; k=$((k+1)); done
espera volume_para_no_maximo 30 vol
controle volumeIncrease 2; sleep 0.6; igual volume_nao_passa_do_maximo "$(vol)" 30
# muitos controles seguidos: cada um é atendido uma vez só, e depois de atendido sai da lista
k=0; while [ $k -lt 35 ]; do controle volumeDecrease 1; controle volumeIncrease 1; k=$((k+1)); done
sleep 1.5; igual setenta_controles_cada_um_uma_vez "$(vol)" 30
espera controles_atendidos_saem_da_lista 0 conta_controles
igual outro_volume_nao_muda "$(vol VolumeNavigation)" 10
controle volumeDecrease 5; controle volumeDecrease 5; controle volumeDecrease 5; controle volumeDecrease 5
espera volume_desce 10 vol
espera memoria_acompanha_o_volume 10 memv volumesettings VolumeEntertainment
igual memoria_ainda_sem_mudo "$(memmudo)" desligado

# ---- 3. mudo do usuário (conexão FCT_AMP_USER_MUTE)
igual mudo_comeca_desligado "$(con FCT_AMP_USER_MUTE)" stopped
controle mute 0
espera controle_de_mudo_liga_o_mudo started con FCT_AMP_USER_MUTE
espera memoria_guarda_o_mudo_ligado ligado memmudo
espera mudo_leva_o_foco_de_ajuste FCT_AMP_USER_MUTE foco
igual mudo_nao_mexe_no_volume "$(vol)" 10
igual mudo_nao_para_a_fonte "$(con ENT_AMP_TUNER)" started
igual mudo_nao_troca_a_fonte_do_global "$(dentro "$(get "$GLO")" entertainmentConnection name)" ENT_AMP_TUNER
controle mute 0
espera segundo_controle_de_mudo_desliga stopped con FCT_AMP_USER_MUTE
espera memoria_guarda_o_mudo_desligado desligado memmudo
espera foco_volta_para_a_fonte ENT_AMP_TUNER foco
controle mute 0; espera mudo_de_novo started con FCT_AMP_USER_MUTE
controle volumeIncrease 1
espera aumentar_o_volume_tira_o_mudo stopped con FCT_AMP_USER_MUTE
espera aumentar_no_mudo_sobe_um_passo 11 vol
espera foco_volta_depois_de_aumentar ENT_AMP_TUNER foco
controle mute 0; espera mudo_outra_vez started con FCT_AMP_USER_MUTE
controle volumeDecrease 1; sleep 0.6
igual diminuir_no_mudo_continua_mudo "$(con FCT_AMP_USER_MUTE)" started
igual diminuir_no_mudo_baixa_o_volume "$(vol)" 10
controle mute 0; espera sai_do_mudo stopped con FCT_AMP_USER_MUTE
# a HMI também liga e desliga o mudo por conta própria, gravando o estado da conexão
post /audiomanagement/connections/$(ID connections FCT_AMP_USER_MUTE) '{"status":"started","postIdentifier":"t-1"}' > /dev/null
espera mudo_pedido_pela_hmi_leva_o_foco FCT_AMP_USER_MUTE foco
espera memoria_guarda_o_mudo_pedido_pela_hmi ligado memmudo
post /audiomanagement/connections/$(ID connections FCT_AMP_USER_MUTE) '{"status":"stopped","postIdentifier":"t-2"}' > /dev/null
espera mudo_solto_pela_hmi_devolve_o_foco ENT_AMP_TUNER foco
# descer até zero entra no mudo
controle volumeDecrease 5; controle volumeDecrease 5
espera volume_chega_a_zero 0 vol
espera volume_zero_liga_o_mudo started con FCT_AMP_USER_MUTE
controle volumeIncrease 2
espera sair_do_zero_tira_o_mudo stopped con FCT_AMP_USER_MUTE
espera sair_do_zero_sobe_os_passos 2 vol
espera memoria_guarda_o_volume_depois_do_zero 2 memv volumesettings VolumeEntertainment
espera memoria_guarda_o_fim_do_mudo desligado memmudo

# ---- 4. troca de fonte: a HMI inicia outra conexão de entretenimento
post /audiomanagement/connections/$(ID connections ENT_AMP_MEDIA_PLAYER) '{"status":"started","postIdentifier":"t-3"}' > /dev/null
espera fonte_nova_vira_a_do_global ENT_AMP_MEDIA_PLAYER dentro_global entertainmentConnection name
espera fonte_nova_leva_o_foco ENT_AMP_MEDIA_PLAYER foco
espera fonte_antiga_para stopped con ENT_AMP_TUNER
sleep 0.8; igual fonte_nova_fica_iniciada "$(con ENT_AMP_MEDIA_PLAYER)" started
igual fonte_nova_guarda_o_que_a_hmi_gravou "$(campo "$(el connections ENT_AMP_MEDIA_PLAYER)" postIdentifier)" t-3
controle volumeIncrease 1; espera volume_segue_valendo_na_fonte_nova 3 vol
post /audiomanagement/connections/$(ID connections ENT_AMP_TUNER) '{"status":"started","postIdentifier":"t-4"}' > /dev/null
espera volta_para_o_radio ENT_AMP_TUNER foco
espera midia_para stopped con ENT_AMP_MEDIA_PLAYER
# conexão de menu (o ajuste "volume inicial máximo"): enquanto está iniciada o foco é dela
post /audiomanagement/connections/$(ID connections FCT_AMP_ONVOLUME_MENU) '{"status":"started","postIdentifier":"t-5"}' > /dev/null
espera conexao_de_menu_leva_o_foco FCT_AMP_ONVOLUME_MENU foco
igual conexao_de_menu_nao_para_a_fonte "$(con ENT_AMP_TUNER)" started
controle volumeIncrease 1
espera no_menu_o_botao_mexe_no_volume_do_menu 12 vol VolumeEntertainmentMaxOnvol
igual no_menu_o_volume_de_entretenimento_fica "$(vol)" 3
post /audiomanagement/connections/$(ID connections FCT_AMP_ONVOLUME_MENU) '{"status":"stopped","postIdentifier":"t-6"}' > /dev/null
espera fim_do_menu_devolve_o_foco ENT_AMP_TUNER foco

# ---- 5. ajustes de som: quem guarda é o mock; o somstub não pode atrapalhar
BAL=$(ID soundparameters balance)
post /audiomanagement/soundparameters/$BAL '{"value":-4}' > /dev/null
igual balanco_gravado_pela_tela_fica "$(campo "$(el soundparameters balance)" value)" -4
post /audiomanagement/soundparameters/$(ID soundparameters EQ1) '{"value":6}' > /dev/null
controle volumeIncrease 1; espera volume_depois_dos_ajustes 4 vol
igual grave_continua_valendo "$(campo "$(el soundparameters EQ1)" value)" 6
igual balanco_continua_valendo "$(campo "$(el soundparameters balance)" value)" -4
# o que a tela grava direto no serviço (grave, agudo, balanço) também vai para a memória
espera memoria_guarda_o_balanco -4 memv soundparameters balance
espera memoria_guarda_o_grave 6 memv soundparameters EQ1
espera memoria_guarda_o_volume_dos_ajustes 4 memv volumesettings VolumeEntertainment
igual memoria_mantem_o_que_veio_de_antes "$(memv soundparameters EQ3)" 4
# volume de menu gravado direto pela tela (o ajuste "Volume inicial máximo"), sem controle
post /audiomanagement/volumesettings/$(ID volumesettings VolumeEntertainmentMaxOnvol) '{"value":14}' > /dev/null
espera memoria_guarda_volume_gravado_pela_tela 14 memv volumesettings VolumeEntertainmentMaxOnvol

# ---- 6. o papel do touch-app: botão de volume e teclas do volante, vindos do barramento
printf 'encoder 13 17 2\n' >> $LOW/tmp/cmd;  espera botao_de_volume_dois_passos 6 vol
printf 'encoder 13 17 -1\n' >> $LOW/tmp/cmd; espera botao_de_volume_para_tras 5 vol
printf 'encoder 13 16 3\n' >> $LOW/tmp/cmd;  sleep 0.6; igual botao_de_selecao_nao_mexe_no_volume "$(vol)" 5
printf 'tecla 4 42 1\n' >> $LOW/tmp/cmd; sleep 0.1; printf 'tecla 4 42 0\n' >> $LOW/tmp/cmd
espera vol_mais_do_volante 6 vol
printf 'tecla 4 43 1\n' >> $LOW/tmp/cmd; sleep 0.1; printf 'tecla 4 43 0\n' >> $LOW/tmp/cmd
espera vol_menos_do_volante 5 vol
printf 'encoder 4 44 1\n' >> $LOW/tmp/cmd;   espera rolete_do_volante 6 vol
printf 'tecla 13 78 1\n' >> $LOW/tmp/cmd; printf 'tecla 13 78 0\n' >> $LOW/tmp/cmd; sleep 0.6
igual outra_tecla_nao_mexe_no_volume "$(vol)" 6
# a tecla de mudo: a HMI não a trata ("ASL_SOUND_PIN_MUTE. Not handled by HMI anymore, supplier resposibility now"),
# então quem cria o controle de mudo é quem faz o papel do touch-app. Vale a do volante (4/57) e a da moldura (13/88)
printf 'tecla 4 57 1\n' >> $LOW/tmp/cmd; sleep 0.1; printf 'tecla 4 57 0\n' >> $LOW/tmp/cmd
espera tecla_de_mudo_do_volante_liga_o_mudo started con FCT_AMP_USER_MUTE
sleep 0.5; igual soltar_a_tecla_nao_desfaz_o_mudo "$(con FCT_AMP_USER_MUTE)" started
printf 'tecla 13 88 1\n' >> $LOW/tmp/cmd; sleep 0.1; printf 'tecla 13 88 0\n' >> $LOW/tmp/cmd
espera tecla_de_mudo_da_moldura_desliga_o_mudo stopped con FCT_AMP_USER_MUTE
igual mudo_pelas_teclas_nao_mexe_no_volume "$(vol)" 6
printf 'tecla 13 17 1\n' >> $LOW/tmp/cmd; printf 'tecla 13 17 0\n' >> $LOW/tmp/cmd; sleep 0.6
igual apertar_o_botao_de_volume_nao_e_mudo "$(con FCT_AMP_USER_MUTE)" stopped

# ---- 7. o registro diz o que foi feito, e o programa segue de pé
if kill -0 $STUB 2>/dev/null; then ok somstub_segue_de_pe; else nok somstub_segue_de_pe "morreu: $(tail -n 3 $LOW/tmp/somstub.log)"; fi
L=$LOW/tmp/somstub.log
if grep -q 'perfil aplicado: .* entretenimento: ENT_AMP_TUNER' $T/w.out; then ok registro_diz_o_perfil; else nok registro_diz_o_perfil "sem a linha: $(cat $T/w.out)"; fi
if grep -q 'entretenimento ENT_AMP_TUNER, foco de ajuste ENT_AMP_TUNER, volume VolumeEntertainment em 8 (faixa 0 a 30), mudo desligado' $L; then ok registro_diz_o_estado_de_partida; else nok registro_diz_o_estado_de_partida "$(head -n 2 $L)"; fi
if grep -q 'VolumeEntertainment: 8 -> 9' $L; then ok registro_diz_o_volume; else nok registro_diz_o_volume "sem a linha"; fi
if grep -q 'mudo do usuário: ligado' $L && grep -q 'mudo do usuário: desligado' $L; then ok registro_diz_o_mudo; else nok registro_diz_o_mudo "sem as linhas"; fi
if grep -q 'entretenimento: ENT_AMP_MEDIA_PLAYER' $L; then ok registro_diz_a_troca_de_fonte; else nok registro_diz_a_troca_de_fonte "sem a linha"; fi
if grep -q 'botão de volume' $L; then ok registro_diz_o_botao; else nok registro_diz_o_botao "sem a linha"; fi
# cada troca pedida é uma troca feita: o programa não pode ir e voltar entre as fontes (ele ia, quando ouvia a lista inteira)
igual troca_de_fonte_nao_vai_e_volta "$(grep -c '^\[somstub\] entretenimento: ' $L)" 2

# ---- 7b. a memória que ficou serve de entrada para a partida seguinte, inteira e sem linha ignorada
espera memoria_tem_o_volume_final "$(vol)" memv volumesettings VolumeEntertainment
cp $MEM $T/mem-final.txt 2>/dev/null
if [ ! -e $MEM.novo ]; then ok memoria_nao_deixa_arquivo_pela_metade; else nok memoria_nao_deixa_arquivo_pela_metade "ficou $MEM.novo"; fi
tem memoria_diz_que_e_simulacao "$(head -n 3 $T/mem-final.txt)" 'SIMULAÇÃO'
"$BIN/somstub" -w $T/prepz -m $T/mem-final.txt "$G" $T/perfil-de-teste.txt > $T/wz.out 2>&1; rc=$?
igual memoria_final_e_aceita "$rc" 0
igual memoria_final_sem_linha_ignorada "$(grep -c 'memória: ignorei' $T/wz.out)" 0
linhaz() { grep "\"name\":\"$2\"" "$T/prepz/audiomanagement/$1.json" 2>/dev/null | head -n 1; }
tem partida_seguinte_traz_o_grave "$(linhaz soundparameters EQ1)" '"value":6,'
tem partida_seguinte_traz_o_balanco "$(linhaz soundparameters balance)" '"value":-4,'
tem partida_seguinte_traz_os_medios "$(linhaz soundparameters EQ3)" '"value":4,'
tem partida_seguinte_traz_o_volume "$(linhaz volumesettings VolumeEntertainment)" "\"value\":$(vol),\"valueMax\":30"
tem partida_seguinte_traz_o_volume_de_menu "$(linhaz volumesettings VolumeEntertainmentMaxOnvol)" '"value":14,'

# ---- 8. sem o mock no ar o somstub desiste e diz por quê (não fica preso)
( chroot $LOW /tmp/somstub -p 7999 -e 2 /tmp/som > $LOW/tmp/somstub2.log 2>&1; echo $? > $T/rc2 ) &
PIDS="$PIDS $!"
k=0; while [ ! -f $T/rc2 ] && [ $k -lt 60 ]; do sleep 0.1; k=$((k+1)); done
if [ -f $T/rc2 ] && [ "$(cat $T/rc2)" != 0 ] && grep -q 'não achei o serviço' $LOW/tmp/somstub2.log; then ok sem_servico_desiste_e_avisa; else nok sem_servico_desiste_e_avisa "rc=$(cat $T/rc2 2>/dev/null) $(head -c 200 $LOW/tmp/somstub2.log)"; fi

# ---- 9. se o mock não está servindo os dados da pasta, o somstub recusa: não inventa estado
mkdir -p $LOW/tmp/outra && cp -r "$G"/audiomanagement $LOW/tmp/outra/ && printf '[{"id":"x","name":"y"}]\n' > $LOW/tmp/outra/audiomanagement/sources.json
( chroot $LOW /tmp/somstub -e 3 -b - /tmp/outra > $LOW/tmp/somstub3.log 2>&1; echo $? > $T/rc3 ) &
PIDS="$PIDS $!"
k=0; while [ ! -f $T/rc3 ] && [ $k -lt 80 ]; do sleep 0.1; k=$((k+1)); done
if [ -f $T/rc3 ] && [ "$(cat $T/rc3)" != 0 ] && grep -q 'não está servindo os dados' $LOW/tmp/somstub3.log; then ok dados_diferentes_dos_do_servico_sao_recusados; else nok dados_diferentes_dos_do_servico_sao_recusados "rc=$(cat $T/rc3 2>/dev/null) $(head -c 200 $LOW/tmp/somstub3.log)"; fi

# ---- 10. memória que não dá para gravar: o programa avisa uma vez e segue de pé
kill $STUB 2>/dev/null; sleep 0.5
chroot $LOW /tmp/somstub -b - -m /proc/nao-da/estado.txt /tmp/som > $LOW/tmp/somstub4.log 2>&1 &
STUB4=$!; PIDS="$PIDS $!"
k=0; while ! grep -q 'no ar\|ERRO' $LOW/tmp/somstub4.log && [ $k -lt 100 ]; do sleep 0.1; k=$((k+1)); done
post /audiomanagement/soundparameters/$(ID soundparameters EQ5) '{"value":2}' > /dev/null
post /audiomanagement/soundparameters/$(ID soundparameters EQ5) '{"value":3}' > /dev/null
k=0; while ! grep -q 'não gravei a memória' $LOW/tmp/somstub4.log && [ $k -lt 40 ]; do sleep 0.1; k=$((k+1)); done
igual memoria_que_nao_grava_avisa_uma_vez "$(grep -c 'não gravei a memória' $LOW/tmp/somstub4.log)" 1
if kill -0 $STUB4 2>/dev/null; then ok memoria_que_nao_grava_nao_derruba; else nok memoria_que_nao_grava_nao_derruba "morreu: $(tail -n 2 $LOW/tmp/somstub4.log)"; fi

# ---- 11. o volante abre a barra (SIMULAÇÃO): com a opção, a tecla de volume do volante vira giro do botão da central
kill $STUB4 2>/dev/null; sleep 0.5
printf '%s\n' 'volante_abre_a_barra = sim' > $LOW/tmp/som/opcoes.txt
giros() { grep -c '^encoder 13 17 ' $LOW/tmp/cmd; }
G0=$(giros)
chroot $LOW /tmp/somstub -c /tmp/cmd /tmp/som > $LOW/tmp/somstub5.log 2>&1 &
STUB5=$!; PIDS="$PIDS $!"
k=0; while ! grep -q 'ouvindo o botão\|ERRO' $LOW/tmp/somstub5.log && [ $k -lt 100 ]; do sleep 0.1; k=$((k+1)); done      # só depois disso o barramento é ouvido
tem volante_no_ar_diz_que_a_simulacao_esta_ligada "$(cat $LOW/tmp/somstub5.log)" 'o volante abre a barra de volume: SIMULAÇÃO ligada pelo perfil'
# O hub reenvia a quem se liga as primeiras mensagens que guardou. Neste teste há teclas antigas entre elas (as da seção 6):
# não são teclas de agora, e quem chega depois não pode agir sobre elas (agia: três giros e cinco controles a mais ao subir).
VA=$(vol); sleep 1.5
igual quem_chega_depois_nao_repete_teclas_antigas "$(giros)" $G0
igual quem_chega_depois_nao_mexe_no_volume "$(vol)" "$VA"
tem quem_chega_depois_diz_que_descartou "$(cat $LOW/tmp/somstub5.log)" 'guardada(s) pelo hub'
V0=$(vol)
printf 'tecla 4 42 1\ntecla 4 42 0\n' >> $LOW/tmp/cmd
espera volante_mais_gira_o_botao_da_central $((G0+1)) giros
tem volante_mais_manda_um_passo_para_cima "$(tail -n 1 $LOW/tmp/cmd)" 'encoder 13 17 1'
espera volante_mais_sobe_um_passo_so $((V0+1)) vol
sleep 0.8; igual volante_mais_nao_sobe_dois "$(vol)" $((V0+1))
printf 'tecla 4 43 1\ntecla 4 43 0\n' >> $LOW/tmp/cmd
espera volante_menos_gira_o_botao_para_baixo "encoder 13 17 -1" tail -n 1 $LOW/tmp/cmd
espera volante_menos_desce_um_passo $V0 vol
printf 'encoder 4 44 2\n' >> $LOW/tmp/cmd
espera rolete_gira_o_botao_com_os_passos "encoder 13 17 2" tail -n 1 $LOW/tmp/cmd
espera rolete_sobe_os_passos $((V0+2)) vol
sleep 0.8; igual volante_nao_entra_em_laco "$(giros)" $((G0+3))
igual volante_cria_controles_como_o_botao "$(grep -c 'botão de volume: .* -> controle' $LOW/tmp/somstub5.log)" 3
igual volante_nao_cria_controle_direto "$(grep -c 'do volante: .* -> controle' $LOW/tmp/somstub5.log)" 0
tem volante_registro_diz_o_que_fez "$(cat $LOW/tmp/somstub5.log)" 'tecla de volume do volante: +1 -> giro do botão da central'
# o botão da própria central segue como era: um passo, sem linha a mais no arquivo de comandos
printf 'encoder 13 17 1\n' >> $LOW/tmp/cmd
espera botao_da_central_segue_como_era $((V0+3)) vol
sleep 0.5; igual botao_da_central_nao_gera_outro_giro "$(giros)" $((G0+4))
# sem ter onde escrever o giro, o volante volta ao que era: o volume muda, com um aviso só
kill $STUB5 2>/dev/null; sleep 0.5
chroot $LOW /tmp/somstub -c /proc/nao-da/cmd /tmp/som > $LOW/tmp/somstub6.log 2>&1 &
STUB6=$!; PIDS="$PIDS $!"
k=0; while ! grep -q 'ouvindo o botão\|ERRO' $LOW/tmp/somstub6.log && [ $k -lt 100 ]; do sleep 0.1; k=$((k+1)); done      # só depois disso o barramento é ouvido
V1=$(vol); G1=$(giros)
printf 'tecla 4 42 1\ntecla 4 42 0\n' >> $LOW/tmp/cmd; espera volante_sem_arquivo_de_comandos_muda_o_volume $((V1+1)) vol
printf 'tecla 4 42 1\ntecla 4 42 0\n' >> $LOW/tmp/cmd; espera volante_sem_arquivo_de_comandos_segue_mudando $((V1+2)) vol
igual volante_sem_arquivo_de_comandos_avisa_uma_vez "$(grep -c 'não escrevi no arquivo de comandos' $LOW/tmp/somstub6.log)" 1
igual volante_sem_arquivo_de_comandos_nao_gira "$(giros)" $G1
# e sem a opção no perfil o programa nem tenta: é o comportamento dos casos da seção 6, acima
kill $STUB6 2>/dev/null

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- somstub ---"; tail -n 25 $LOW/tmp/somstub.log | cut -c1-200; }
[ $fail -eq 0 ]
