#!/bin/sh
# Teste do rádio de mentira (Marco 11): o esquema dos serviços do sintonizador, gerado das classes
# do firmware, e o que o veiculostub faz com ele e com os perfis de stubs/radio/. É o mesmo
# programa do carro de mentira, com outro esquema e outro domínio: PID_TUNER_HMI, canal
# 0x400000000000, que é o que a HMI trata como sintonizador (ListenerWorkerThread.run).
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_radio.sh <pasta com kipc_hub, veiculostub, dsi_manda e dsi_escuta> [<pasta stubs/radio>]
# Quem faz o papel da HMI é o dsi_manda, com o canal 0x8000. Os bytes dos pedidos são os que a
# HMI mandou nas rodadas a1, a2 e seguintes de 12/10/2026 (traces/rad_*); os números de interface
# e de api esperados foram lidos à parte, no código da camada da LG (DSIUnifiedTunerImpl,
# DSITunerAnnouncementImpl e DSIRadioDataImpl) e nas interfaces org.dsi.ifc.radio.
BIN=${1:-/work/bin}; RAD=${2:-/stubs/radio}; T=/tmp/t_radio; SOCK=/run/kipc_hub.sock
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
# freqs <arquivo>: a frequência (campo 4 da emissora, em kHz) de cada atualização da emissora
# selecionada ("api=3") do arquivo, uma por linha, na ordem em que saíram
freqs() {
  grep ' api=3 ' "$1" | sed 's/.* pb=//' | awk '
    function h(x) { return (index("0123456789abcdef", substr(x, 1, 1)) - 1) * 16 + index("0123456789abcdef", substr(x, 2, 1)) - 1 }
    { n = split($0, b, " "); f = -1
      if (b[1] == "0a") { i = 3
        while (i <= n) { t = h(b[i]); i++; c = int(t / 8); w = t % 8
          if (w == 0) { v = 0; m = 1; while (i <= n) { x = h(b[i]); i++; v += (x % 128) * m; m *= 128; if (x < 128) break }; if (c == 4) { f = v; break } }
          else if (w == 2) i += 1 + h(b[i]); else break } }
      print f }'
}
# varredura <arquivo> <de> <até> <sentido>: confere as frequências do arquivo: todas dentro da faixa,
# andando no sentido dado (1 sobe, -1 desce), sem repetir, com no máximo uma volta pela ponta e no
# máximo 41 mensagens. Diz "ok" ou o defeito
varredura() {
  freqs "$1" | awk -v lo="$2" -v hi="$3" -v s="$4" '
    { f = $1; n++
      if (f < lo || f > hi) ruim = ruim " fora:" f
      if (n > 1) { if ((f - ant) * s < 0) voltas++; else if (f == ant) ruim = ruim " parada:" f }
      ant = f }
    END { if (voltas > 1) ruim = ruim " voltas:" voltas; if (n < 3) ruim = ruim " poucas:" n; if (n > 41) ruim = ruim " demais:" n; print (ruim == "" ? "ok" : ruim) }'
}

G=$RAD/gerado; E=$G/esquema.txt
DOM=70368744177664                                  # 0x400000000000, PID_TUNER_HMI

# ---- o esquema: os três serviços que o anúncio do domínio liga, com as interfaces do código da LG
tem esquema_sintonizador_unificado "^S DSIUnifiedTuner $DOM 400 401 radio\$" $E
tem esquema_avisos_de_transito "^S DSITunerAnnouncement $DOM 402 403 radio\$" $E
tem esquema_dados_de_radio "^S DSIRadioData $DOM 420 421 radio\$" $E
igual esquema_so_tem_os_tres_servicos "$(conta '^S ' $E)" 3
tem anuncio_liga_sintonizador_unificado "^W $DOM org\.dsi\.ifc\.radio\.DSIUnifiedTuner 0 0\$" $E
tem anuncio_liga_avisos_de_transito "^W $DOM org\.dsi\.ifc\.radio\.DSITunerAnnouncement 0 0\$" $E
tem anuncio_liga_dados_de_radio "^W $DOM org\.dsi\.ifc\.radiodata\.DSIRadioData 0 0\$" $E
# atributos: os 18 do sintonizador unificado e os 5 dos avisos (constantes ATTR_ das interfaces)
igual atributos_do_sintonizador_unificado "$(conta '^A DSIUnifiedTuner ' $E)" 18
igual atributos_dos_avisos "$(conta '^A DSITunerAnnouncement ' $E)" 5
tem atributo_emissora_selecionada '^A DSIUnifiedTuner 3 ATTR_SELECTEDSTATION$' $E
tem atributo_lista_de_emissoras '^A DSIUnifiedTuner 4 ATTR_STATIONLIST$' $E
tem atributo_uso_do_radio '^A DSIUnifiedTuner 18 ATTR_RADIOUSAGESTATUS$' $E
# eventos: as atualizações de atributo (api abaixo de 2000) e as respostas (o segundo switch do ouvinte)
tem evento_emissora_selecionada '^E DSIUnifiedTuner 3 3 3 updateSelectedStation DSIUnifiedTunerListenerMsg\.updateSelectedStation$' $E
tem evento_lista_de_emissoras '^E DSIUnifiedTuner 4 4 4 updateStationList DSIUnifiedTunerListenerMsg\.updateStationList$' $E
tem evento_resposta_de_sintonia '^E DSIUnifiedTuner 2000 - - selectStationStatus DSIUnifiedTunerListenerMsg\.selectStationStatus$' $E
tem evento_resposta_de_busca '^E DSIUnifiedTuner 2008 - - seekStationStatus DSIUnifiedTunerListenerMsg\.seekStationStatus$' $E
igual eventos_de_atributo_do_sintonizador_unificado "$(conta '^E DSIUnifiedTuner [0-9]+ [0-9]+ ' $E)" 18
# pedidos: as apis das chamadas a postDSIProviderMsg de cada …Impl
tem pedido_sintonizar '^P DSIUnifiedTuner 1000 selectStation DSIUnifiedTunerMsg\.selectStation$' $E
tem pedido_buscar '^P DSIUnifiedTuner 1014 seekStation DSIUnifiedTunerMsg\.seekStation$' $E
tem pedido_uso_do_radio '^P DSIUnifiedTuner 1019 switchRadioUsage DSIUnifiedTunerMsg\.switchRadioUsage$' $E
tem pedido_filtro_dos_avisos '^P DSITunerAnnouncement 1000 setFilter ' $E
tem pedido_logotipos '^P DSIRadioData 1001 requestRadioStationLogos ' $E
# a assinatura do sintonizador vai em varint comum, e não em zigue-zague como nos serviços de
# veículo: no barramento, "08 12" assina o atributo 18 (rodada a1)
tem assinatura_em_varint_comum '^P DSIUnifiedTuner 9999 setNotification DSIUnifiedTunerMsg\.setNotiUnifiedTuner$' $E
if [ "$(sed -n '/^M DSIUnifiedTunerMsg\.setNotiUnifiedTuner$/{n;p;}' $E 2>/dev/null)" = "C 1 attributes int32 rep" ]; then ok assinatura_campo_int32
else nok assinatura_campo_int32 "o campo da assinatura veio como: $(sed -n '/^M DSIUnifiedTunerMsg\.setNotiUnifiedTuner$/{n;p;}' $E 2>/dev/null)"; fi
# a frequência vai em kHz num inteiro de 64 bits sem zigue-zague: 100,0 MHz saiu "20 a0 8d 06" (rodada a2)
tem emissora_frequencia_int64 '^C 4 frequency int64 um$' $E

# ---- os vetores de prova: o veiculostub tem de serializar como as classes do firmware
if [ -f $G/vetores-perfil.txt ] && [ -f $G/vetores-esperado.txt ]; then
  "$BIN/veiculostub" -m $E $G/vetores-perfil.txt > $T/vetores.log 2>$T/vetores.err; rc=$?
  total=$(grep -c '^canal=' $G/vetores-esperado.txt)
  grep '^canal=' $G/vetores-esperado.txt | sort > $T/esperado.ord
  grep '^canal=' $T/vetores.log | sort > $T/obtido.ord
  iguais=$(comm -12 $T/esperado.ord $T/obtido.ord | wc -l | tr -d ' ')
  if [ $rc -eq 0 ] && [ "$total" -gt 40 ] && [ "$iguais" = "$total" ]; then ok "vetores_do_firmware ($iguais de $total mensagens iguais, byte a byte)"
  else nok vetores_do_firmware "rc=$rc; $iguais de $total iguais; primeira diferença: $(comm -23 $T/esperado.ord $T/obtido.ord | head -n 1 | cut -c1-160) / erro: $(tail -n 1 $T/vetores.err)"; fi
else
  nok vetores_do_firmware "não achei os vetores em $G"
fi

# ---- o rádio de mentira (radiostub): o motor do veiculostub mais o modelo do sintonizador.
# O perfil deste teste é pequeno e não é o da pasta: três emissoras, a do meio com RDS.
cat > $T/p.txt <<'EOF'
grava = DSIUnifiedTuner
[DSIUnifiedTuner.switchRadioUsage -> updateRadioUsageStatus]
deviceUsage = deviceUsage
[DSIUnifiedTuner.updateSelectedStation]
selectedStation.waveBand = 3
selectedStation.frequency = 89100
selectedStation.longName = "UM"
[DSIUnifiedTuner.updateStationList]
bandType = 2
max = 3
stationList[0].waveBand = 3
stationList[0].frequency = 89100
stationList[0].longName = "UM"
stationList[0].piSId = 4097
stationList[1].waveBand = 3
stationList[1].frequency = 93700
stationList[1].longName = "DOIS"
stationList[1].piSId = 4098
stationList[1].rds = 1
stationList[2].waveBand = 3
stationList[2].frequency = 98500
EOF
# as três emissoras, como as classes do firmware as escrevem (campos na ordem dos números)
UM="08 03 1a 02 55 4d 20 8c b8 05 28 81 20"                     # FM, "UM", 89100 kHz, PI 4097
DOIS="08 03 1a 04 44 4f 49 53 20 84 dc 05 28 82 20 58 01"       # FM, "DOIS", 93700 kHz, PI 4098, RDS
TRES="08 03 20 c4 81 06"                                        # FM, 98500 kHz, sem nome
M1="ff ff ff ff ff ff ff ff ff 01"                              # -1 num int32
R="canal=0x400000000000 if=401 inst=0"

# Cada radiostub sobe com um hub novo: o hub reentrega a quem se liga as 48 primeiras mensagens, e
# um segundo radiostub no mesmo hub atenderia de novo os pedidos antigos do teste.
sobe() {   # sobe <perfil> <registro do stub>: hub, escuta (em $T/escuta.log) e radiostub
  rm -f $SOCK
  "$BIN/kipc_hub" > $T/hub.log 2>&1 & HUB=$!
  i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
  "$BIN/dsi_escuta" $SOCK > $T/escuta.log 2>&1 & ESC=$!
  sleep 0.3
  "$BIN/radiostub" $SOCK $E "$1" > "$2" 2>&1 & STUB=$!
  sleep 1.2
}
desce() { kill $STUB $ESC $HUB 2>/dev/null; wait $STUB $ESC $HUB 2>/dev/null; rm -f $SOCK; }

if [ -x "$BIN/radiostub" ]; then
  "$BIN/radiostub" -n $E $T/p.txt > $T/n.log 2>&1; igual perfil_do_teste_confere "$?" 0
  sobe $T/p.txt $T/stub.log
  igual anuncia_o_dominio_uma_vez "$(conta '^canal=0x400000000000 if=9000 ' $T/escuta.log)" 1

  # assinaturas, com os bytes que a HMI mandou na rodada a1: "08 03" é o atributo 3, sem zigue-zague
  hmi 400 0 9999 "08 03"; hmi 400 0 9999 "08 04"; hmi 400 0 9999 "08 12"; sleep 0.4
  tem assinatura_manda_a_emissora_do_perfil "^$R api=3 pb=0a 0a 08 03 1a 02 55 4d 20 8c b8 05 10 01\$" $T/escuta.log
  tem assinatura_manda_a_lista_do_perfil "^$R api=4 pb=0a 0d $UM 0a 11 $DOIS 0a 06 $TRES 10 02 18 01 28 03\$" $T/escuta.log
  tem assinatura_do_uso_do_radio_comeca_em_zero "^$R api=18 pb=10 01\$" $T/escuta.log

  # uso do rádio: a HMI pede e só sintoniza depois de o atributo dizer 1 (StationSelectionManager)
  hmi 400 0 1019 "08 01"; sleep 0.3
  tem uso_do_radio_confirmado "^$R api=18 pb=08 01 10 01\$" $T/escuta.log

  # sintonizar pela lista: os bytes do toque na segunda emissora na rodada b2. A HMI espera, nesta
  # ordem, "em curso", a emissora selecionada e "feito" (StateTopMainUnifiedRadio: enquanto espera
  # o "em curso" ela descarta a atualização da emissora)
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "08 02 10 84 dc 05 18 82 20 20 $M1 28 $M1 30 $M1"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/sint.log
  igual sintonia_manda_tres_mensagens "$(wc -l < $T/sint.log | tr -d ' ')" 3
  igual sintonia_1_em_curso "$(sed -n 1p $T/sint.log)" "$R api=2000 pb=08 01"
  igual sintonia_2_a_emissora_da_lista "$(sed -n 2p $T/sint.log)" "$R api=3 pb=0a 11 $DOIS 10 01"
  igual sintonia_3_feito "$(sed -n 3p $T/sint.log)" "$R api=2000 pb=08 02"
  tem sintonia_fica_registrada 'sintonizei 93700 kHz' $T/stub.log

  # quem assina depois recebe a emissora de agora, não a do perfil
  hmi 400 1 9999 "08 03"; sleep 0.3
  tem assinatura_tardia_recebe_a_emissora_de_agora "^canal=0x400000000000 if=401 inst=1 api=3 pb=0a 11 $DOIS 10 01\$" $T/escuta.log
  hmi 400 1 9998 "08 03"; sleep 0.2

  # buscar para cima (seta >): bytes da rodada a1, com outra sessão. Da 93,7 vai para a 98,5,
  # VARRENDO a faixa (Marco 16): "em curso", uma atualização por canal no caminho (só faixa,
  # frequência e PI -1, como na sintonia manual), a emissora inteira na chegada, e "feito". A grade
  # é a que a HMI usa na bancada: FM de 76,1 a 107,9 MHz de 200 em 200 kHz
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 10 02 18 07"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/busca.log
  igual busca_1_em_curso_com_faixa_e_sessao "$(sed -n 1p $T/busca.log)" "$R api=2008 pb=08 01 10 02 18 07"
  igual busca_varre_de_200_em_200_ate_a_seguinte "$(freqs $T/busca.log | tr '\n' ' ')" "$(awk 'BEGIN { for (f = 93900; f <= 98500; f += 200) printf "%d ", f }')"
  igual busca_no_caminho_vai_so_a_frequencia "$(grep ' api=3 ' $T/busca.log | head -n 1)" "$R api=3 pb=0a 11 08 03 20 cc dd 05 28 $M1 10 01"
  igual busca_2_a_emissora_seguinte "$(grep ' api=3 ' $T/busca.log | tail -n 1)" "$R api=3 pb=0a 06 $TRES 10 01"
  igual busca_3_feito_com_faixa_e_sessao "$(tail -n 1 $T/busca.log)" "$R api=2008 pb=08 02 10 02 18 07"
  igual busca_manda_o_estado_duas_vezes "$(grep -c ' api=2008 ' $T/busca.log)" 2
  # da última, para cima, passa do fim da faixa, recomeça em 76,1 e para na primeira. O caminho é
  # longo (113 canais): o modelo pula canais para não passar de 40 mensagens
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 10 02 18 08"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/volta.log
  igual busca_para_cima_da_ultima_volta_a_primeira "$(grep ' api=3 ' $T/volta.log | tail -n 1)" "$R api=3 pb=0a 0d $UM 10 01"
  igual busca_que_da_a_volta_fica_na_faixa "$(varredura $T/volta.log 76100 107900 1)" ok
  igual busca_que_da_a_volta_passa_pela_ponta "$(freqs $T/volta.log | awk 'NR > 1 && $1 < ant { print (ant > 105000 && $1 < 78000) ? "sim" : "não: " ant " -> " $1 } { ant = $1 }')" sim
  # da primeira, para baixo, passa do começo da faixa e vai à última
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 02 10 02 18 09"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/baixo.log
  igual busca_para_baixo_da_primeira_vai_a_ultima "$(grep ' api=3 ' $T/baixo.log | tail -n 1)" "$R api=3 pb=0a 06 $TRES 10 01"
  igual busca_para_baixo_fica_na_faixa "$(varredura $T/baixo.log 76100 107900 -1)" ok
  tem busca_para_baixo_feito "^$R api=2008 pb=08 02 10 02 18 09\$" $T/baixo.log

  # interromper a busca (a HMI manda o modo 3 a qualquer toque durante a busca): responde
  # "interrompida" e a emissora não muda
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 03 10 02 18 0a"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/aborta.log
  igual busca_interrompida_responde_interrompida "$(cat $T/aborta.log)" "$R api=2008 pb=08 03 10 02 18 0a"
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 04 10 02 18 0b"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/para.log
  igual busca_parada_responde_parada "$(cat $T/para.log)" "$R api=2008 pb=08 04 10 02 18 0b"

  # busca sem parar (modo 5, com a seta segurada na sintonia manual): dá o passo e fica "em curso",
  # sem o "feito"; quem encerra é a HMI, mandando parar (StateAmFmActiveTargetSeek)
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 05 10 02 18 0f"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/semparar.log
  igual busca_sem_parar_fica_em_curso "$(sed -n 1p $T/semparar.log | cut -d' ' -f4-) / $(grep -c ' api=2008 ' $T/semparar.log) / $(tail -n 1 $T/semparar.log | cut -d' ' -f4-)" "api=2008 pb=08 01 10 02 18 0f / 1 / api=3 pb=0a 0d $UM 10 01"
  igual busca_sem_parar_tambem_varre "$(varredura $T/semparar.log 76100 107900 1)" ok

  # emissora selecionada fica só com a faixa, a frequência e o PI do pedido, sem nome
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "08 06 10 a0 8d 06 18 $M1 20 $M1 28 $M1 30 $M1"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/manual.log
  igual sintonia_manual_fora_da_lista "$(sed -n 2p $T/manual.log)" "$R api=3 pb=0a 11 08 03 20 a0 8d 06 28 $M1 10 01"
  igual sintonia_manual_em_curso_e_feito "$(sed -n 1p $T/manual.log | cut -d' ' -f4-) / $(sed -n 3p $T/manual.log | cut -d' ' -f4-)" "api=2000 pb=08 01 / api=2000 pb=08 02"
  # dali, buscar para cima passa do fim da faixa e cai na primeira; para baixo, na última
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 10 02 18 0c"; sleep 1.5
  igual busca_de_fora_da_lista_para_cima "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R api=3 " | tail -n 1)" "$R api=3 pb=0a 0d $UM 10 01"
  hmi 400 0 1000 "08 06 10 a0 8d 06 18 $M1 20 $M1 28 $M1 30 $M1"; sleep 0.3
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 02 10 02 18 0d"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/fora.log
  igual busca_de_fora_da_lista_para_baixo "$(grep ' api=3 ' $T/fora.log | tail -n 1)" "$R api=3 pb=0a 06 $TRES 10 01"
  # de 100,0 MHz para baixo, de 200 em 200, o caminho não cai em cima da 98,5: para antes dela
  igual busca_para_antes_de_passar_da_emissora "$(freqs $T/fora.log | tr '\n' ' ')" "$(awk 'BEGIN { for (f = 99800; f > 98500; f -= 200) printf "%d ", f; printf "98500 " }')"

  # AM (modo 0, que não vai no fio): 531 kHz. A lista do teste não tem emissora de AM
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "10 93 04 18 $M1 20 $M1 28 $M1 30 $M1"; sleep 0.4
  igual sintonia_em_am "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R api=3 ")" "$R api=3 pb=0a 10 08 01 20 93 04 28 $M1 10 01"
  # buscar numa faixa sem emissora na lista (AM, faixa 0): varre a faixa inteira (de 531 a
  # 1602 kHz, de 9 em 9), volta à frequência de partida, e a busca termina
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 18 0e"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/am.log
  igual busca_sem_emissora_na_faixa_fica_onde_esta "$(grep ' api=3 ' $T/am.log | tail -n 1)" "$R api=3 pb=0a 10 08 01 20 93 04 28 $M1 10 01"
  igual busca_sem_emissora_varre_a_faixa_inteira "$(varredura $T/am.log 531 1602 1) $(freqs $T/am.log | awk '{ if (($1 - 531) % 9) r = "fora do canal" } $1 > 1500 { alto = 1 } END { print r (alto ? "" : " não chegou ao fim da faixa") }')" "ok "
  tem busca_sem_emissora_na_faixa_termina "^$R api=2008 pb=08 02 18 0e\$" $T/am.log

  # atualizar a lista ("Buscar e salvar" do assistente de configuração; bytes da rodada f2): a HMI
  # fica em "Procurando emissoras…" até chegar o estado final (ou 65 s, InitialAutostore). O rádio
  # de mentira responde "em curso", manda a lista de novo a quem a assina e responde "feito"
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1011 "08 01 10 02"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/lista.log
  igual atualizar_lista_manda_tres_mensagens "$(wc -l < $T/lista.log | tr -d ' ')" 3
  igual atualizar_lista_1_em_curso "$(sed -n 1p $T/lista.log)" "$R api=2007 pb=08 01 10 02"
  igual atualizar_lista_2_a_lista_de_novo "$(sed -n 2p $T/lista.log)" "$R api=4 pb=0a 0d $UM 0a 11 $DOIS 0a 06 $TRES 10 02 18 01 28 03"
  igual atualizar_lista_3_feito "$(sed -n 3p $T/lista.log)" "$R api=2007 pb=08 02 10 02"
  # interromper a atualização (modo 2): responde "interrompida" e não manda a lista
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1011 "08 02 10 02"; sleep 0.4
  igual atualizar_lista_interrompida "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R ")" "$R api=2007 pb=08 03 10 02"
  # atualizar a lista de AM (faixa 0, que não vai no fio): a lista do perfil é de FM, então só os estados
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1011 "08 01"; sleep 0.4
  igual atualizar_lista_de_outra_faixa_so_responde "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R " | cut -d' ' -f4- | tr '\n' '/')" "api=2007 pb=08 01/api=2007 pb=08 02/"

  # o que o modelo não trata continua com o motor: fica registrado, como no carro de mentira
  hmi 400 0 1001 "08 02"; sleep 0.3
  tem pedido_fora_do_modelo_fica_registrado 'pedido sem resposta: DSIUnifiedTuner\.setStationFollowingMode ' $T/stub.log
  # este perfil não tem texto de rádio nem emissora de AM: o modelo não manda nem um nem outro
  igual perfil_sem_texto_nao_manda_texto_de_radio "$(grep -c "^$R api=5 " $T/escuta.log)" 0
  igual perfil_so_de_fm_nao_manda_lista_de_am "$(grep "^$R api=4 " $T/escuta.log | grep -vc ' 10 02 18 01 28 03$')" 0
  desce

  # ---- Marco 16: a lista de AM e o texto de rádio. O perfil tem as emissoras das duas faixas na
  # mesma lista, misturadas, e um "max" errado de propósito: quem separa e acerta é o modelo. A HMI
  # guarda uma lista por faixa e as recebe em mensagens separadas, com o tipo de faixa de cada uma
  # (StateTopMainUnifiedRadio.dsiUnifiedTunerUpdateStationList); o texto só vale se o PI for o da
  # emissora atual (UnifiedRadioRadioTextManager.isReceivedTextValid)
  cat > $T/p16.txt <<'EOF16'
grava = DSIUnifiedTuner
[DSIUnifiedTuner.switchRadioUsage -> updateRadioUsageStatus]
deviceUsage = deviceUsage
[DSIUnifiedTuner.updateSelectedStation]
selectedStation.waveBand = 3
selectedStation.frequency = 89100
selectedStation.longName = "UM"
selectedStation.piSId = 4097
[DSIUnifiedTuner.updateRadioText]
radioText.radioText = "TEXTO"
radioText.source = 1
[DSIUnifiedTuner.updateStationList]
bandType = 2
max = 9
stationList[0].waveBand = 1
stationList[0].frequency = 594
stationList[1].waveBand = 3
stationList[1].frequency = 89100
stationList[1].longName = "UM"
stationList[1].piSId = 4097
stationList[1].radioText = 1
stationList[2].waveBand = 3
stationList[2].frequency = 93700
stationList[2].longName = "DOIS"
stationList[2].piSId = 4098
stationList[2].rds = 1
stationList[3].waveBand = 1
stationList[3].frequency = 1008
EOF16
  UMT="08 03 1a 02 55 4d 20 8c b8 05 28 81 20 78 01"              # FM, "UM", 89100 kHz, PI 4097, com texto de rádio
  AM1="08 01 20 d2 04"                                            # AM, 594 kHz
  AM2="08 01 20 f0 07"                                            # AM, 1008 kHz
  LFM="api=4 pb=0a 0f $UMT 0a 11 $DOIS 10 02 18 01 28 02"         # a lista de FM: tipo 2, válida, max 2
  LAM="api=4 pb=0a 05 $AM1 0a 05 $AM2 18 01 28 02"                # a de AM: tipo 0 (não vai no fio), max 2
  TXT="api=5 pb=0a 0c 08 81 20 2a 05 54 45 58 54 4f 30 01 10 01"  # o texto, com o PI da "UM" e a origem FM
  "$BIN/radiostub" -n $E $T/p16.txt > $T/n16.log 2>&1; igual perfil_com_am_e_texto_confere "$?" 0
  sobe $T/p16.txt $T/stub16.log
  tem modelo_conta_as_emissoras_por_faixa '2 emissora\(s\) de FM e 2 de AM' $T/stub16.log
  hmi 400 0 9999 "08 03"; hmi 400 0 9999 "08 04"; hmi 400 0 9999 "08 05"; hmi 400 0 9999 "08 12"; sleep 0.4
  tem assinatura_da_lista_recebe_so_as_de_fm "^$R $LFM\$" $T/escuta.log
  tem assinatura_do_texto_recebe_o_da_emissora_do_perfil "^$R $TXT\$" $T/escuta.log
  hmi 400 0 1019 "08 01"; sleep 0.3
  # a primeira sintonia: as três mensagens de sempre, o texto (vazio: a "DOIS" não tem) e, por ser
  # o primeiro pedido que o modelo atende, a lista de AM
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "08 02 10 84 dc 05 18 82 20 20 $M1 28 $M1 30 $M1"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " | cut -d' ' -f4- > $T/s16.log
  igual sintonia_com_texto_e_am_manda_cinco "$(wc -l < $T/s16.log | tr -d ' ')" 5
  igual sintonia_16_as_tres_de_sempre "$(sed -n 1,3p $T/s16.log | tr '\n' '/')" "api=2000 pb=08 01/api=3 pb=0a 11 $DOIS 10 01/api=2000 pb=08 02/"
  igual sintonia_16_texto_vazio_para_emissora_sem_texto "$(sed -n 4p $T/s16.log)" "api=5 pb=0a 05 08 82 20 30 01 10 01"
  igual sintonia_16_primeiro_pedido_manda_a_lista_de_am "$(sed -n 5p $T/s16.log)" "$LAM"
  # a emissora com texto: o texto do perfil, com o PI dela; a lista de AM não vai de novo
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "08 02 10 8c b8 05 18 81 20 20 $M1 28 $M1 30 $M1"; sleep 0.4
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " | cut -d' ' -f4- > $T/s16b.log
  igual sintonia_16_emissora_com_texto "$(tr '\n' '/' < $T/s16b.log)" "api=2000 pb=08 01/api=3 pb=0a 0f $UMT 10 01/api=2000 pb=08 02/$TXT/"
  # atualizar a lista de AM (faixa 0) e a de FM (faixa 2): cada pedido recebe a lista da sua faixa
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1011 "08 01"; sleep 0.4
  igual atualizar_lista_de_am_manda_a_de_am "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R " | cut -d' ' -f4- | tr '\n' '/')" "api=2007 pb=08 01/$LAM/api=2007 pb=08 02/"
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1011 "08 01 10 02"; sleep 0.4
  igual atualizar_lista_de_fm_manda_a_de_fm "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R " | cut -d' ' -f4- | tr '\n' '/')" "api=2007 pb=08 01 10 02/$LFM/api=2007 pb=08 02 10 02/"
  # sintonizar em AM (modo 0, que não vai no fio) uma emissora da lista: ela inteira, o texto vazio
  # e, por ter entrado em AM, a lista de AM de novo
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "10 d2 04 18 $M1 20 $M1 28 $M1 30 $M1"; sleep 0.4
  igual sintonia_em_am_da_lista "$(tail -n +$((n0+1)) $T/escuta.log | grep "^$R " | cut -d' ' -f4- | tr '\n' '/')" "api=2000 pb=08 01/api=3 pb=0a 05 $AM1 10 01/api=2000 pb=08 02/api=5 pb=0a 02 30 01 10 01/$LAM/"
  # buscar para cima em AM: de 594 a 1008 kHz são 46 canais de 9 kHz, mais que 40: vai de 18 em 18
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 18 31"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/b16.log
  igual busca_em_am_varre_ate_a_seguinte "$(freqs $T/b16.log | tr '\n' ' ')" "$(awk 'BEGIN { for (f = 612; f <= 990; f += 18) printf "%d ", f; printf "1008 " }')"
  igual busca_em_am_chega_na_emissora "$(grep ' api=3 ' $T/b16.log | tail -n 1 | cut -d' ' -f4-)" "api=3 pb=0a 05 $AM2 10 01"
  igual busca_em_am_feito_e_depois_o_texto "$(tail -n 2 $T/b16.log | cut -d' ' -f4- | tr '\n' '/')" "api=2008 pb=08 02 18 31/api=5 pb=0a 02 30 01 10 01/"
  # o registro diz quando a lista de AM foi mandada e que texto acompanhou cada sintonia
  igual lista_de_am_fica_registrada "$(grep -c 'mandei a lista de AM (2 emissora(s)) a 1 assinatura(s)' $T/stub16.log)" 2
  tem texto_do_perfil_fica_registrado 'sintonizei 89100 kHz, emissora da lista UM; avisei 1 assinatura\(s\); texto de rádio: o do perfil' $T/stub16.log
  tem texto_vazio_fica_registrado 'sintonizei 93700 kHz, emissora da lista DOIS; avisei 1 assinatura\(s\); texto de rádio: vazio' $T/stub16.log
  if kill -0 $STUB 2>/dev/null; then ok modelo_com_am_e_texto_segue_no_ar; else nok modelo_com_am_e_texto_segue_no_ar "o radiostub saiu"; fi
  desce

  # sem lista nenhuma no perfil a busca não quebra: termina onde está
  printf '[DSIUnifiedTuner.updateSelectedStation]\nselectedStation.waveBand = 3\nselectedStation.frequency = 87500\n' > $T/vazio.txt
  sobe $T/vazio.txt $T/stub2.log
  hmi 400 0 9999 "08 03"; sleep 0.3
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 10 02 18 01"; sleep 1.5
  tail -n +$((n0+1)) $T/escuta.log | grep "^$R " > $T/semlista.log
  igual busca_sem_lista_termina_onde_esta "$(grep ' api=3 ' $T/semlista.log | tail -n 1) / $(tail -n 1 $T/semlista.log | cut -d' ' -f4-)" "$R api=3 pb=0a 06 08 03 20 cc ab 05 10 01 / api=2008 pb=08 02 10 02 18 01"
  if kill -0 $STUB 2>/dev/null; then ok busca_sem_lista_nao_derruba_o_programa; else nok busca_sem_lista_nao_derruba_o_programa "o radiostub saiu"; fi
  desce

  # os pedidos do modelo passam pelo "ignora" do perfil e pela defesa contra laço, como os do motor
  { cat $T/p.txt | sed 's/^grava = .*/&\nignora = DSIUnifiedTuner.seekStation/'; } > $T/ign.txt
  sobe $T/ign.txt $T/stub3.log
  hmi 400 0 9999 "08 03"; sleep 0.3
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1014 "08 01 10 02 18 21"; sleep 0.3
  igual busca_ignorada_pelo_perfil_nao_responde "$(tail -n +$((n0+1)) $T/escuta.log | grep -c "^$R ")" 0
  tem busca_ignorada_fica_registrada 'pedido sem resposta: DSIUnifiedTuner\.seekStation .*o perfil manda ignorar' $T/stub3.log
  n0=$(wc -l < $T/escuta.log)
  hmi 400 0 1000 "08 02 10 84 dc 05 18 82 20 20 $M1 28 $M1 30 $M1"; sleep 0.3
  igual sintonia_segue_atendida_com_a_busca_ignorada "$(tail -n +$((n0+1)) $T/escuta.log | grep -c "^$R ")" 3
  # o mesmo pedido de sintonia sem parar (o dsi_manda repete a cada 2 ms): com o de cima, 200 são
  # atendidos e depois vem o corte, como no carro de mentira
  "$BIN/dsi_manda" $SOCK 0x8000 400 0 1000 "08 02 10 84 dc 05 18 82 20 20 $M1 28 $M1 30 $M1" 2 & LACO=$!
  sleep 2.5; kill $LACO 2>/dev/null; wait $LACO 2>/dev/null; sleep 0.3
  # (contado no registro do stub: na rajada o hub descarta parte do que manda a quem escuta)
  igual laco_de_sintonia_cortado_em_200 "$(grep -c 'pedido atendido: DSIUnifiedTuner\.selectStation ' $T/stub3.log)" 200
  tem laco_de_sintonia_fica_registrado 'DSIUnifiedTuner\.selectStation .*atendido mais de 200 vezes' $T/stub3.log
  desce

  # esquema que não é o do sintonizador: recusa, e diz por quê (é o que o lançador roda com -n)
  printf 'S Outro 16 700 701 teste\n' > $T/outro.txt
  "$BIN/radiostub" -n $T/outro.txt > $T/outro.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q 'DSIUnifiedTuner' $T/outro.log; then ok esquema_sem_o_sintonizador_e_recusado
  else nok esquema_sem_o_sintonizador_e_recusado "rc=$rc: $(tail -n 1 $T/outro.log)"; fi

  # ---- os perfis da pasta: conferem com o esquema, e a lista é coerente com o que a camada da LG
  # exige (UnifiedListControl.AddList lê "max" emissoras da mensagem: max tem de ser o tamanho da lista)
  achou=0
  for p in $RAD/*.txt; do
    [ -f "$p" ] || continue
    achou=1
    "$BIN/radiostub" -n $E "$p" > $T/pp.log 2>&1
    if [ $? -eq 0 ]; then ok "perfil_da_pasta_confere: $(basename $p)"; else nok "perfil_da_pasta_confere: $(basename $p)" "$(tail -n 1 $T/pp.log)"; fi
  done
  [ $achou = 1 ] || nok perfil_da_pasta_confere "não há perfil em $RAD"
  P=$RAD/fm-de-mentira.txt
  # as emissoras do perfil, uma por linha: "<índice> <faixa> <frequência>" (faixa 1 ou 2 = AM)
  emissoras() { awk -F'[][.= ]+' '/^stationList\[[0-9]+\]\.waveBand/ { wb[$2] = $4 } /^stationList\[[0-9]+\]\.frequency/ { fr[$2] = $4 } END { for (i in fr) print i, wb[i] + 0, fr[i] }' "$1" 2>/dev/null | sort -n; }
  nfm=$(emissoras $P | awk '$2 != 1 && $2 != 2' | wc -l | tr -d ' ')
  nam=$(emissoras $P | awk '$2 == 1 || $2 == 2' | wc -l | tr -d ' ')
  maxp=$(sed -n 's/^max *= *\([0-9]*\).*/\1/p' $P 2>/dev/null | tail -n 1)
  # o "max" do perfil é o da lista de FM; a de AM é o modelo que separa, e ele acerta o max das duas
  if [ -n "$maxp" ] && [ "${nfm:-0}" -ge 3 ] && [ "$maxp" = "$nfm" ]; then ok "perfil_da_pasta_max_e_o_numero_de_emissoras_de_fm ($nfm de FM)"
  else nok perfil_da_pasta_max_e_o_numero_de_emissoras_de_fm "max=$maxp, emissoras de FM=$nfm"; fi
  # frequências de FM na faixa que a própria HMI chama de Brasil (RadioConstants.FM_BANDRANGE_BRA:
  # de 76,1 a 107,9 MHz, de 200 em 200 kHz), em kHz. Caem todas em décimo ímpar
  ruins=$(emissoras $P | awk '$2 != 1 && $2 != 2 && ($3 < 76100 || $3 > 107900 || (($3 - 76100) % 200) != 0) { print $3 }' | tr '\n' ' ')
  if [ -z "$ruins" ] && [ "${nfm:-0}" -gt 0 ]; then ok perfil_da_pasta_frequencias_na_faixa_de_fm_do_brasil; else nok perfil_da_pasta_frequencias_na_faixa_de_fm_do_brasil "fora da faixa ou do canal: $ruins"; fi
  # Marco 16: emissoras de AM na grade que a HMI usa na bancada (a da Europa, de 531 a 1602 kHz, de
  # 9 em 9, que ela assume com a adaptação de AM "sem ajuste"), e o texto de rádio
  ruins=$(emissoras $P | awk '($2 == 1 || $2 == 2) && ($3 < 531 || $3 > 1602 || (($3 - 531) % 9) != 0) { print $3 }' | tr '\n' ' ')
  if [ -z "$ruins" ] && [ "${nam:-0}" -ge 2 ]; then ok "perfil_da_pasta_emissoras_de_am_na_grade ($nam de AM)"; else nok perfil_da_pasta_emissoras_de_am_na_grade "emissoras de AM: $nam; fora da faixa ou do canal: $ruins"; fi
  txt=$(sed -n '/^\[DSIUnifiedTuner\.updateRadioText\]/,/^\[/s/^radioText\.radioText *= *"\(.*\)".*/\1/p' $P 2>/dev/null)
  case "$txt" in *TESTE*) ok perfil_da_pasta_texto_de_radio_que_se_diz_de_teste ;; *) nok perfil_da_pasta_texto_de_radio_que_se_diz_de_teste "o texto do perfil é [$txt]" ;; esac
  # o texto vai para a emissora marcada com radioText = 1, e a HMI só o aceita pelo PI dela
  comtexto=$(awk -F'[][.= ]+' '/^stationList\[[0-9]+\]\./ { if ($3 == "radioText" && $4 == 1) t[$2] = 1; if ($3 == "piSId" && $4 != -1) pi[$2] = 1 } END { for (i in t) { n++; if (!pi[i]) sempi = sempi " " i } print n + 0, sempi }' $P 2>/dev/null)
  case "$comtexto" in [1-9]*" ") ok "perfil_da_pasta_emissoras_com_texto_tem_pi (${comtexto% } com texto)" ;; *) nok perfil_da_pasta_emissoras_com_texto_tem_pi "com texto e sem PI (índices), ou nenhuma: [$comtexto]" ;; esac
  "$BIN/radiostub" -n $E $P > $T/ppf.log 2>&1
  tem perfil_da_pasta_o_modelo_separa_fm_de_am "$nfm emissora\(s\) de FM e $nam de AM na lista do perfil, com texto de rádio" $T/ppf.log
  # a lista da central só mostra o nome de uma emissora que tenha longName, rds = 1 e PI diferente
  # de -1 (medido na rodada c3 de 12/10/2026); emissora com nome a que falte um dos dois apareceria
  # pela frequência, sem aviso
  semnome() {   # semnome <perfil>: índices das emissoras com longName e sem RDS ou sem PI
    awk -F'[][.= ]+' '/^stationList\[[0-9]+\]\./ { i = $2; c = $3; v = $4
        if (c == "longName") nome[i] = 1; if (c == "rds" && v == 1) rds[i] = 1; if (c == "piSId" && v != -1) pi[i] = 1 }
      END { for (i in nome) if (!rds[i] || !pi[i]) printf "%s ", i }' "$1" 2>/dev/null
  }
  printf 'stationList[0].longName = "A"\nstationList[0].rds = 1\nstationList[0].piSId = -1\nstationList[1].longName = "B"\nstationList[1].rds = 1\nstationList[1].piSId = 7\nstationList[2].longName = "C"\nstationList[2].piSId = 8\n' > $T/nomes.txt
  igual conferencia_de_nomes_acusa_emissora_sem_pi_ou_sem_rds "$(semnome $T/nomes.txt | tr ' ' '\n' | sort | tr '\n' ' ' | sed 's/^ *//')" "0 2 "
  igual perfil_da_pasta_emissora_com_nome_tem_rds_e_pi "$(semnome $P)" ""
  tem perfil_da_pasta_tem_emissora_com_nome '^stationList\[[0-9]+\]\.longName' $P
  tem perfil_da_pasta_diz_que_e_simulacao 'SIMULAÇÃO' $P
  tem perfil_da_pasta_lista_e_de_fm '^bandType *= *2' $P
else
  nok radiostub_existe "não achei $BIN/radiostub (o m1_build.sh o compila de driver/radiostub.c)"
fi

# ---- o radiostub inclui o veiculostub.c. Se uma mudança lá o quebrar, o m1_build.sh não pode
# levar junto as centrais que não pediram rádio: avisa, não deixa binário velho e segue
DRV=$(cd "$(dirname "$0")/.." && pwd)
if command -v gcc >/dev/null 2>&1 && [ -f "$DRV/m1_build.sh" ]; then
  mkdir -p $T/drv && cp "$DRV"/*.c "$DRV"/m1_build.sh $T/drv/ 2>/dev/null
  echo 'int isto_nao_compila(void) { return nao_existe; }' >> $T/drv/radiostub.c
  DRIVER=$T/drv sh $T/drv/m1_build.sh $T/bin > $T/build.log 2>&1; rc=$?
  igual build_segue_sem_o_radiostub "$rc" 0
  if [ -x $T/bin/veiculostub ] && [ -x $T/bin/kipc_hub ] && [ ! -e $T/bin/radiostub ]; then ok build_sem_radiostub_deixa_o_resto_e_nenhum_binario_velho
  else nok build_sem_radiostub_deixa_o_resto_e_nenhum_binario_velho "$(ls $T/bin 2>/dev/null | tr '\n' ' ')"; fi
  tem build_avisa_que_o_radiostub_nao_compilou 'AVISO: o radiostub não compilou' $T/build.log
else
  echo "PULOU: sem gcc ou sem m1_build.sh ao lado deste teste (o caso do radiostub que não compila)"
fi

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- o que foi escutado (fim) ---"; tail -n 12 $T/escuta.log 2>/dev/null | cut -c1-200; echo "--- radiostub ---"; tail -n 12 $T/stub.log 2>/dev/null | cut -c1-200; }
[ $fail -eq 0 ]
