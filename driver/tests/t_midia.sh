#!/bin/sh
# Teste da mídia de mentira (Marco 15): o esquema dos serviços de mídia, gerado das classes do
# firmware, e o que o midiastub (o motor do veiculostub mais o modelo do tocador e do navegador)
# faz com ele e com os perfis de stubs/midia/. O domínio é PID_MEDIA, canal 0x800000.
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_midia.sh <pasta com kipc_hub, veiculostub, midiastub, dsi_manda e dsi_escuta> [<pasta stubs/midia>]
# Quem faz o papel da HMI é o dsi_manda, com o canal 0x8000. Os bytes dos pedidos são os que a HMI
# mandou nas rodadas m1 a m5 e p1 a p4 de 12/10/2026 (traces/mid_*); os números de interface e de
# api esperados foram lidos à parte, em com.lge.mib3oi.dsi.DSIInterfaceId e nas interfaces
# org.dsi.ifc.media; o que a HMI espera de cada pedido, em de.vw.mib.asl.internal.media.
BIN=${1:-/work/bin}; MID=${2:-/stubs/midia}; T=/tmp/t_midia; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
rm -f $SOCK
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
tem() { if grep -qE -- "$2" "$3" 2>/dev/null; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }
naotem() { if grep -qE -- "$2" "$3" 2>/dev/null; then nok "$1" "apareceu /$2/: $(grep -E -- "$2" "$3" | head -n 1)"; else ok "$1"; fi; }
conta() { grep -cE -- "$1" "$2" 2>/dev/null; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperava [$3], veio [$2]"; fi; }
hmi() { "$BIN/dsi_manda" $SOCK 0x8000 "$@"; }      # hmi <interface> <instância> <api> <protobuf>

G=$MID/gerado; E=$G/esquema.txt
DOM=8388608                                         # 0x800000, PID_MEDIA

# ---- o esquema: os serviços que o anúncio do domínio liga, com as interfaces do código da LG
tem esquema_base "^S DSIMediaBase $DOM 200 201 media\$" $E
tem esquema_navegador "^S DSIMediaBrowser $DOM 202 203 media\$" $E
tem esquema_tocador "^S DSIMediaPlayer $DOM 206 207 media\$" $E
tem esquema_navegador_de_arquivos "^S DSIFileBrowser $DOM 1000 1001 filebrowser\$" $E
tem anuncio_liga_a_base "^W $DOM org\.dsi\.ifc\.media\.DSIMediaBase 0 0\$" $E
tem anuncio_liga_o_tocador "^W $DOM org\.dsi\.ifc\.media\.DSIMediaPlayer 0 0\$" $E
igual anuncio_liga_seis_navegadores "$(conta "^W $DOM org\.dsi\.ifc\.media\.DSIMediaBrowser " $E)" 6
tem atributo_lista_de_midias '^A DSIMediaBase 3 ATTR_MEDIALIST$' $E
tem atributo_lista_de_aparelhos '^A DSIMediaBase 4 ATTR_DEVICELIST$' $E
tem atributo_midia_ativa '^A DSIMediaPlayer 8 ATTR_ACTIVEMEDIA$' $E
tem atributo_posicao '^A DSIMediaPlayer 11 ATTR_PLAYPOSITION$' $E
igual atributos_do_tocador "$(conta '^A DSIMediaPlayer ' $E)" 22
igual atributos_do_navegador "$(conta '^A DSIMediaBrowser ' $E)" 12
# o estado de reprodução tem duas apis com a mesma mensagem (7 e 9000); o gerador as distingue no nome
tem evento_estado_de_reproducao '^E DSIMediaPlayer 7 7 7 MPLupdatePlaybackState:7 DSIMediaPlayerListenerMsg\.MPLupdatePlaybackState$' $E
tem evento_midia_ativa '^E DSIMediaPlayer 8 8 8 MPLupdateActiveMedia DSIMediaPlayerListenerMsg\.MPLupdateActiveMedia$' $E
tem evento_detalhe_da_faixa '^E DSIMediaPlayer 3003 - - MPLresponseDetailInfo DSIMediaPlayerListenerMsg\.MPLresponseDetailInfo$' $E
tem evento_resposta_de_lista '^E DSIMediaBrowser 2000 - - MBRLresponseList DSIMediaBrowserListenerMsg\.MBRLresponseList$' $E
tem evento_resultado_de_selecao '^E DSIMediaBrowser 2001 - - MBRLselectionResult DSIMediaBrowserListenerMsg\.MBRLselectionResult$' $E
tem pedido_ativar_midia '^P DSIMediaPlayer 1012 setActiveMedia DSIMediaPlayerMsg\.MPsetActiveMedia$' $E
tem pedido_pular '^P DSIMediaPlayer 1011 skip DSIMediaPlayerMsg\.MPskip$' $E
tem pedido_detalhe '^P DSIMediaPlayer 1019 requestDetailInfo DSIMediaPlayerMsg\.MPrequestDetailInfo$' $E
tem pedido_lista '^P DSIMediaBrowser 1008 requestList DSIMediaBrowserMsg\.MBRrequestList$' $E
tem pedido_filtro '^P DSIMediaBrowser 1004 setContentFilter DSIMediaBrowserMsg\.MBRsetContentFilter$' $E
# a assinatura vai em varint comum: "08 04" assina o atributo 4 (rodada m1)
if [ "$(sed -n '/^M DSIMediaBaseMsg\.MBsetNotification$/{n;p;}' $E 2>/dev/null)" = "C 1 attributes int32 rep" ]; then ok assinatura_campo_int32
else nok assinatura_campo_int32 "o campo da assinatura veio como: $(sed -n '/^M DSIMediaBaseMsg\.MBsetNotification$/{n;p;}' $E 2>/dev/null)"; fi
# o tempo da faixa e os identificadores: inteiros sem zigue-zague (a faixa 101 saiu "08 65", rodada p1)
tem posicao_entrada_int64 '^C 1 entryID int64 um$' $E
naotem cabecalho_sem_letra_estragada "$(printf '\357\277\275')" $E

# ---- os vetores de prova: o veiculostub tem de serializar como as classes do firmware
if [ -f $G/vetores-perfil.txt ] && [ -f $G/vetores-esperado.txt ]; then
  "$BIN/veiculostub" -m $E $G/vetores-perfil.txt > $T/vetores.log 2>$T/vetores.err; rc=$?
  total=$(grep -c '^canal=' $G/vetores-esperado.txt)
  grep '^canal=' $G/vetores-esperado.txt | sort > $T/esperado.ord
  grep '^canal=' $T/vetores.log | sort > $T/obtido.ord
  iguais=$(comm -12 $T/esperado.ord $T/obtido.ord | wc -l | tr -d ' ')
  if [ $rc -eq 0 ] && [ "$total" -gt 150 ] && [ "$iguais" = "$total" ]; then ok "vetores_do_firmware ($iguais de $total mensagens iguais, byte a byte)"
  else nok vetores_do_firmware "rc=$rc; $iguais de $total iguais; primeira diferença: $(comm -23 $T/esperado.ord $T/obtido.ord | head -n 1 | cut -c1-160) / erro: $(tail -n 1 $T/vetores.err)"; fi
else
  nok vetores_do_firmware "não achei os vetores em $G"
fi

# ---- a mídia de mentira (midiastub): o motor do veiculostub mais o modelo do tocador e do navegador.
# O perfil deste teste é pequeno e não é o da pasta: uma fonte, uma pasta e três faixas.
cat > $T/p.txt <<'EOF'
grava = DSIMediaBase DSIMediaBrowser
[DSIMediaBase.MBLupdateDeviceList]
deviceList[0].deviceID = 1
deviceList[0].numSlots = 1
[DSIMediaBase.MBLupdateMediaList]
mediaList[0].deviceID = 1
mediaList[0].mediaID = 11
mediaList[0].mediaType = 7
mediaList[0].name = "M"
[DSIMediaPlayer.MPLupdateCapabilities]
capabilities.pause = 1
capabilities.play = 1
[DSIMediaPlayer.MPLupdatePlaybackModeList]
playbackModeList[0].modeID = 1
playbackModeList[1].modeID = 2
playbackModeList[1].modeFlag = 2
[DSIMediaPlayer.MPLupdatePlaybackMode]
playbackMode = 1
[DSIMediaBrowser.MBRLupdateBrowseFolder]
activeBrowsingFolder[0].entryID = 1
activeBrowsingFolder[0].title = "R"
activeBrowsingFolder[0].contentType = 4
[DSIMediaBrowser.MBRLresponseList]
entrylist[0].entryID = 101
entrylist[0].title = "UM"
entrylist[0].contentType = 1
entrylist[0].secLength = 180
entrylist[0].extInfo.artist = "A"
entrylist[0].extInfo.album = "B"
entrylist[1].entryID = 102
entrylist[1].title = "DOIS"
entrylist[1].contentType = 1
entrylist[1].secLength = 200
entrylist[2].entryID = 103
entrylist[2].title = "TRES"
entrylist[2].contentType = 1
entrylist[2].secLength = 240
EOF
# as três faixas e a pasta, como as classes do firmware as escrevem (campos na ordem dos números)
UM="08 65 1a 02 55 4d 20 01 28 b4 01 3a 06 0a 01 42 1a 01 41"   # 101, "UM", áudio, 180 s, álbum "B", artista "A"
DOIS="08 66 1a 04 44 4f 49 53 20 01 28 c8 01"                   # 102, "DOIS", áudio, 200 s
TRES="08 67 1a 04 54 52 45 53 20 01 28 f0 01"                   # 103, "TRES", áudio, 240 s
RAIZ="08 01 1a 01 52 20 04"                                     # a pasta: 1, "R", pasta
M1="ff ff ff ff ff ff ff ff ff 01"                              # -1 num int32
TOC="canal=0x800000 if=207 inst=0"; NAV="canal=0x800000 if=203"; BAS="canal=0x800000 if=201 inst=0"
ATIVA="08 01 10 0b 28 01"                                       # aparelho 1, mídia 11, válida
CAPAC="0a 04 30 01 38 01 10 01"                                 # pausa e tocar
MODOS="0a 02 10 01 0a 04 08 02 10 02 10 01"                     # modo 1; modo 2 com a marca 2
P101="08 65 18 a0 fe 0a 20 01"; P102="08 66 18 c0 9a 0c 20 01"; P103="08 67 18 80 d3 0e 20 01"   # faixa, 0 ms, duração em ms

# Cada midiastub sobe com um hub novo: o hub reentrega a quem se liga as 48 primeiras mensagens, e
# um segundo programa no mesmo hub atenderia de novo os pedidos antigos do teste.
sobe() {   # sobe <esquema> <perfil> <registro>: hub, escuta (em $T/escuta.log) e midiastub
  rm -f $SOCK
  "$BIN/kipc_hub" > $T/hub.log 2>&1 & HUB=$!
  i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
  "$BIN/dsi_escuta" $SOCK > $T/escuta.log 2>&1 & ESC=$!
  sleep 0.3
  "$BIN/midiastub" $SOCK "$1" "$2" > "$3" 2>&1 & STUB=$!
  sleep 1.2
}
desce() { kill $STUB $ESC $HUB 2>/dev/null; wait $STUB $ESC $HUB 2>/dev/null; rm -f $SOCK; }
desde() { tail -n +$(($1+1)) $T/escuta.log | grep "^$2" > "$3"; }     # desde <linha> <prefixo> <arquivo>
agora_() { wc -l < $T/escuta.log; }
# as assinaturas que a HMI faz na partida, com os bytes dela (rodada p3)
assina() {
  hmi 200 0 9999 "08 04"; hmi 200 0 9999 "08 02 08 03"
  hmi 206 0 9999 "08 07 08 08 08 12 08 15 08 16 08 01 08 05 08 06 08 09 08 0a 08 0b"
  hmi 202 0 9999 "08 08 08 01 08 02 08 03 08 04 08 05 08 07 08 06 08 0a 08 0b"
  hmi 202 3 9999 "08 08 08 01 08 02 08 03 08 04 08 05 08 07 08 06 08 0a 08 0b"
  sleep 0.5
}

if [ -x "$BIN/midiastub" ]; then
  "$BIN/midiastub" -n $E $T/p.txt > $T/n.log 2>&1; igual perfil_do_teste_confere "$?" 0
  tem partida_conta_as_faixas 'modelo do tocador: 1 fonte\(s\) e 3 faixa\(s\) no perfil' $T/n.log

  # --- A: anúncio e assinaturas
  sobe $E $T/p.txt $T/stub.log
  igual anuncia_o_dominio_uma_vez "$(conta '^canal=0x800000 if=9000 ' $T/escuta.log)" 1
  assina
  tem assinatura_manda_a_lista_de_aparelhos "^$BAS api=4 pb=0a 04 08 01 20 01 10 01\$" $T/escuta.log
  tem assinatura_manda_a_lista_de_midias "^$BAS api=3 pb=0a 09 10 01 28 0b 30 07 42 01 4d 10 01\$" $T/escuta.log
  tem assinatura_manda_as_capacidades "^$TOC api=10 pb=$CAPAC\$" $T/escuta.log
  tem assinatura_manda_os_modos "^$TOC api=5 pb=$MODOS\$" $T/escuta.log
  tem assinatura_manda_o_modo "^$TOC api=6 pb=08 01 10 01\$" $T/escuta.log
  # antes de a HMI ativar a fonte não há mídia ativa nem faixa: valem os padrões, marcados como válidos
  tem antes_de_ativar_nao_ha_midia_ativa "^$TOC api=8 pb=28 01\$" $T/escuta.log
  tem antes_de_ativar_o_estado_e_zero "^$TOC api=7 pb=10 01\$" $T/escuta.log
  tem antes_de_ativar_a_posicao_e_zero "^$TOC api=11 pb=20 01\$" $T/escuta.log

  # --- B: ativar a fonte. A HMI só dá a ativação por feita quando, DEPOIS da mídia ativa, chegam as
  # capacidades, os modos e um estado de reprodução "ativado" (MediaPlayer.detectActivationCompleted).
  # Os bytes são os do toque na tecla Mídia na rodada m4: setActiveMedia(1, 11, 0)
  n0=$(agora_); hmi 206 0 1012 "08 01 10 0b"; sleep 0.4; desde $n0 "$TOC " $T/b.log
  igual ativar_manda_seis_mensagens "$(wc -l < $T/b.log | tr -d ' ')" 6
  igual ativar_1_midia_ativa "$(sed -n 1p $T/b.log)" "$TOC api=8 pb=$ATIVA"
  igual ativar_2_capacidades "$(sed -n 2p $T/b.log)" "$TOC api=10 pb=$CAPAC"
  igual ativar_3_lista_de_modos "$(sed -n 3p $T/b.log)" "$TOC api=5 pb=$MODOS"
  igual ativar_4_modo "$(sed -n 4p $T/b.log)" "$TOC api=6 pb=08 01 10 01"
  igual ativar_5_posicao_na_primeira_faixa "$(sed -n 5p $T/b.log)" "$TOC api=11 pb=$P101"
  igual ativar_6_estado_em_pausa "$(sed -n 6p $T/b.log)" "$TOC api=7 pb=08 05 10 01"
  tem ativar_fica_registrado 'setActiveMedia.*ativei a mídia 11 do aparelho 1' $T/stub.log

  # --- C: o modo de reprodução. A HMI manda -1 ("assumir o modo do aparelho") ao ativar (rodada p1):
  # volta o modo de agora, e não o -1
  n0=$(agora_); hmi 206 0 1000 "08 $M1"; sleep 0.3; desde $n0 "$TOC " $T/c.log
  igual modo_assumir_devolve_o_modo_de_agora "$(cat $T/c.log)" "$TOC api=6 pb=08 01 10 01"
  n0=$(agora_); hmi 206 0 1000 "08 02"; sleep 0.3; desde $n0 "$TOC " $T/c.log
  igual modo_da_lista_e_gravado "$(cat $T/c.log)" "$TOC api=6 pb=08 02 10 01"
  n0=$(agora_); hmi 206 0 1000 "08 09"; sleep 0.3; desde $n0 "$TOC " $T/c.log
  igual modo_fora_da_lista_nao_muda "$(cat $T/c.log)" "$TOC api=6 pb=08 02 10 01"

  # --- D: os dados da faixa (requestDetailInfo), com os bytes da rodada p1: "08 65" é a entrada 101
  n0=$(agora_); hmi 206 0 1019 "08 65"; sleep 0.3; desde $n0 "$TOC " $T/d.log
  igual detalhe_da_primeira_faixa "$(cat $T/d.log)" "$TOC api=3003 pb=0a 14 0a 01 42 1a 01 41 40 01 48 65 70 03 8a 01 02 55 4d 90 01 01"
  n0=$(agora_); hmi 206 0 1019 "08 66"; sleep 0.3; desde $n0 "$TOC " $T/d.log
  igual detalhe_da_segunda_faixa "$(cat $T/d.log)" "$TOC api=3003 pb=0a 10 40 01 48 66 70 03 8a 01 04 44 4f 49 53 90 01 02"
  n0=$(agora_); hmi 206 0 1019 "08 e7 07"; sleep 0.3; desde $n0 "$TOC " $T/d.log
  igual detalhe_de_faixa_desconhecida_nao_responde "$(wc -l < $T/d.log | tr -d ' ')" 0
  tem detalhe_desconhecido_fica_registrado 'requestDetailInfo.*999.*não está na lista' $T/stub.log

  # --- E: andar pelas faixas. "próxima" é skip(0, 1) e "anterior" é skip(1, 1) (rodada p3)
  n0=$(agora_); hmi 206 0 1011 "10 01"; sleep 0.3; desde $n0 "$TOC " $T/e.log
  igual proxima_manda_posicao_e_estado "$(wc -l < $T/e.log | tr -d ' ')" 2
  igual proxima_1_posicao_da_segunda "$(sed -n 1p $T/e.log)" "$TOC api=11 pb=$P102"
  igual proxima_2_estado_continua_em_pausa "$(sed -n 2p $T/e.log)" "$TOC api=7 pb=08 05 10 01"
  n0=$(agora_); hmi 206 0 1011 "08 01 10 01"; sleep 0.3; desde $n0 "$TOC api=11 " $T/e.log
  igual anterior_volta_para_a_primeira "$(cat $T/e.log)" "$TOC api=11 pb=$P101"
  n0=$(agora_); hmi 206 0 1011 "08 01 10 01"; sleep 0.3; desde $n0 "$TOC api=11 " $T/e.log
  igual anterior_da_primeira_vai_para_a_ultima "$(cat $T/e.log)" "$TOC api=11 pb=$P103"
  n0=$(agora_); hmi 206 0 1011 "10 01"; sleep 0.3; desde $n0 "$TOC api=11 " $T/e.log
  igual proxima_da_ultima_vai_para_a_primeira "$(cat $T/e.log)" "$TOC api=11 pb=$P101"
  # tocar, pausar e parar mudam só o estado; a faixa não anda sozinha (não há som nem relógio de faixa)
  n0=$(agora_); hmi 206 0 1006 ""; sleep 0.3; desde $n0 "$TOC api=7 " $T/e.log
  igual tocar_poe_em_reproducao "$(cat $T/e.log)" "$TOC api=7 pb=08 03 10 01"
  n0=$(agora_); hmi 206 0 1008 ""; sleep 0.3; desde $n0 "$TOC api=7 " $T/e.log
  igual pausar_poe_em_pausa "$(cat $T/e.log)" "$TOC api=7 pb=08 05 10 01"
  n0=$(agora_); hmi 206 0 1007 ""; sleep 0.3; desde $n0 "$TOC api=7 " $T/e.log
  igual retomar_poe_em_reproducao "$(cat $T/e.log)" "$TOC api=7 pb=08 03 10 01"
  n0=$(agora_); hmi 206 0 1009 ""; sleep 0.3; desde $n0 "$TOC api=7 " $T/e.log
  igual parar_poe_em_parado "$(cat $T/e.log)" "$TOC api=7 pb=08 04 10 01"
  # escolher uma faixa pelo identificador (setEntry), com um tempo: a posição leva o tempo pedido
  n0=$(agora_); hmi 206 0 1005 "08 67 10 e8 07"; sleep 0.3; desde $n0 "$TOC api=11 " $T/e.log
  igual escolher_faixa_com_tempo "$(cat $T/e.log)" "$TOC api=11 pb=08 67 10 e8 07 18 80 d3 0e 20 01"

  # --- F: tocar numa faixa da lista. A HMI zera a seleção, seleciona tudo e manda tocar a partir da
  # entrada (rodada p4): resetSelection, enableRecurseSubdirectories(true), addSelection(true, 1) e,
  # no tocador, setPlaySelection(0, 103, false) = "10 67"
  n0=$(agora_); hmi 202 0 1003 ""; sleep 0.3; desde $n0 "$NAV " $T/f.log
  igual zerar_selecao_responde_vazio "$(cat $T/f.log)" "$NAV inst=0 api=2001 pb="
  n0=$(agora_); hmi 202 0 1001 "08 01 10 01"; sleep 0.3; desde $n0 "$NAV " $T/f.log
  igual selecionar_tudo_diz_quantas_ha "$(cat $T/f.log)" "$NAV inst=0 api=2001 pb=10 01 18 01 30 03 38 03"
  n0=$(agora_); hmi 206 0 1021 "10 66"; sleep 0.3; desde $n0 "$TOC " $T/f.log
  igual tocar_selecao_manda_tres_mensagens "$(wc -l < $T/f.log | tr -d ' ')" 3
  igual tocar_selecao_1_resposta "$(sed -n 1p $T/f.log)" "$TOC api=2005 pb="
  igual tocar_selecao_2_posicao_da_faixa "$(sed -n 2p $T/f.log)" "$TOC api=11 pb=$P102"
  igual tocar_selecao_3_estado "$(sed -n 3p $T/f.log)" "$TOC api=7 pb=08 04 10 01"
  n0=$(agora_); hmi 206 0 1021 "08 03 10 67 18 01"; sleep 0.3; desde $n0 "$TOC api=2005 " $T/f.log
  igual tocar_selecao_devolve_o_navegador "$(cat $T/f.log)" "$TOC api=2005 pb=08 03"

  # --- G: o navegador. Depois do filtro de conteúdo a HMI só segue quando chegam a pasta atual e
  # um tamanho de lista maior que zero (AbstractMediaBrowser.detectContentFilterSet). Cada
  # instância do navegador recebe só o que pediu: aqui a 3 (a do painel), com "08 07" (rodada p4)
  n0=$(agora_); hmi 202 3 1004 "08 07"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual filtro_manda_tres_mensagens "$(wc -l < $T/g.log | tr -d ' ')" 3
  igual filtro_1_o_filtro "$(sed -n 1p $T/g.log)" "$NAV inst=3 api=2 pb=08 07 10 01"
  igual filtro_2_a_pasta "$(sed -n 2p $T/g.log)" "$NAV inst=3 api=4 pb=0a 07 $RAIZ 10 01"
  igual filtro_3_o_tamanho_da_lista "$(sed -n 3p $T/g.log)" "$NAV inst=3 api=5 pb=08 03 18 01"
  # o caminho da pasta: requestBrowsePath(-1, 13, false), bytes da rodada p3
  n0=$(agora_); hmi 202 0 1027 "08 $M1 10 0d"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual caminho_e_a_pasta_do_perfil "$(cat $T/g.log)" "$NAV inst=0 api=2010 pb=0a 07 $RAIZ"
  # a lista: requestList(0, 0, 0, 3) = "20 03" traz as três; requestList(101, 1, 0, 0) = "08 65 10 01"
  # é a HMI procurando a entrada em foco, e a resposta é essa entrada com o índice dela
  n0=$(agora_); hmi 202 0 1008 "20 03"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual lista_inteira "$(cat $T/g.log)" "$NAV inst=0 api=2000 pb=0a 13 $UM 0a 0d $DOIS 0a 0d $TRES"
  n0=$(agora_); hmi 202 0 1008 "08 65 10 01"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual lista_entrada_em_foco_primeira "$(cat $T/g.log)" "$NAV inst=0 api=2000 pb=0a 13 $UM"
  n0=$(agora_); hmi 202 3 1008 "08 67 10 01"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual lista_entrada_em_foco_terceira_com_indice "$(cat $T/g.log)" "$NAV inst=3 api=2000 pb=0a 0d $TRES 10 02"
  n0=$(agora_); hmi 202 0 1008 "18 01 20 01"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual lista_janela_do_meio "$(cat $T/g.log)" "$NAV inst=0 api=2000 pb=0a 0d $DOIS 10 01"
  # trocar de pasta: só há uma, e a resposta é a pasta e o tamanho, para quem pediu
  n0=$(agora_); hmi 202 0 1007 "0a 07 $RAIZ"; sleep 0.3; desde $n0 "$NAV " $T/g.log
  igual trocar_pasta_manda_pasta_e_tamanho "$(cat $T/g.log | tr '\n' '|')" "$NAV inst=0 api=4 pb=0a 07 $RAIZ 10 01|$NAV inst=0 api=5 pb=08 03 18 01|"
  # os pedidos set que o motor grava sozinho: mídia e modo do navegador (regras do esquema)
  n0=$(agora_); hmi 202 0 1006 "08 01 10 0b"; sleep 0.3; desde $n0 "$NAV inst=0 " $T/g.log
  igual midia_do_navegador_e_gravada "$(cat $T/g.log)" "$NAV inst=0 api=3 pb=08 01 10 0b 18 01"
  # a vista de reprodução do tocador: requestPlayView(0, 0, 2, 5)
  n0=$(agora_); hmi 206 0 1013 "18 02 20 05"; sleep 0.3; desde $n0 "$TOC " $T/g.log
  igual vista_de_reproducao "$(cat $T/g.log)" "$TOC api=2004 pb=0a 13 $UM 0a 0d $DOIS 20 05"

  # --- H: desativar. Ao sair da mídia a HMI manda setActiveMedia sem campos (rodada p3)
  n0=$(agora_); hmi 206 0 1012 ""; sleep 0.3; desde $n0 "$TOC " $T/h.log
  igual desativar_manda_so_a_midia_vazia "$(cat $T/h.log)" "$TOC api=8 pb=28 01"
  # mídia que não está no perfil não é ativada: volta a mídia vazia, e fica registrado
  n0=$(agora_); hmi 206 0 1012 "08 04 10 0e"; sleep 0.3; desde $n0 "$TOC " $T/h.log
  igual midia_desconhecida_nao_ativa "$(cat $T/h.log)" "$TOC api=8 pb=28 01"
  tem midia_desconhecida_fica_registrada 'setActiveMedia.*não está na lista de mídias do perfil' $T/stub.log

  # --- I: a busca (DSISearch) fica de fora: as instâncias dela são de domínios diferentes, e o hub
  # entrega tudo a todos. Nem a assinatura nem os pedidos na interface 1301 têm resposta daqui
  n0=$(agora_); hmi 1301 2 9999 "08 02"; hmi 1301 2 1014 "08 1a"; sleep 0.4; desde $n0 "canal=0x800000 " $T/i.log
  igual busca_nao_e_respondida "$(wc -l < $T/i.log | tr -d ' ')" 0
  tem busca_de_fora_fica_registrada 'DSISearch fica de fora' $T/stub.log
  # o navegador de arquivos só responde ao idioma se o perfil tiver a seção da resposta (aqui não tem).
  # Os bytes são os de DSIFileBrowser.setLanguage("pt_BR") na rodada m1
  hmi 1000 0 1009 "0a 05 70 74 5f 42 52"; sleep 0.3
  naotem sem_secao_o_idioma_dos_arquivos_nao_responde 'if=1001 .*api=2010' $T/escuta.log
  desce

  # --- J: "ignora" e a defesa contra laço valem para os pedidos do modelo
  { sed 's/^grava = .*/&\nignora = DSIMediaPlayer.skip/' $T/p.txt; printf '[DSIFileBrowser.FBLsetLanguageResult]\n'; } > $T/pi.txt
  sobe $E $T/pi.txt $T/stub_i.log; assina
  hmi 206 0 1012 "08 01 10 0b"; sleep 0.3
  n0=$(agora_); hmi 206 0 1011 "10 01"; sleep 0.3; desde $n0 "$TOC " $T/j.log
  igual pedido_ignorado_nao_tem_resposta "$(wc -l < $T/j.log | tr -d ' ')" 0
  tem pedido_ignorado_fica_registrado 'pedido sem resposta: DSIMediaPlayer\.skip .*o perfil manda ignorar' $T/stub_i.log
  n0=$(agora_); hmi 206 0 1019 "08 65"; sleep 0.3; desde $n0 "$TOC " $T/j.log
  igual outro_pedido_segue_atendido_com_um_ignorado "$(wc -l < $T/j.log | tr -d ' ')" 1
  # com a seção no perfil, o motor responde ao idioma do navegador de arquivos devolvendo o que veio
  n0=$(agora_); hmi 1000 0 1009 "0a 05 70 74 5f 42 52"; sleep 0.3; desde $n0 "canal=0x800000 if=1001 " $T/j.log
  igual idioma_dos_arquivos_respondido_com_a_secao "$(cat $T/j.log)" "canal=0x800000 if=1001 inst=0 api=2010 pb=12 05 70 74 5f 42 52"
  desce
  sobe $E $T/p.txt $T/stub_l.log; assina
  # o mesmo pedido sem parar (o dsi_manda repete a cada 2 ms): 200 são atendidos e depois vem o corte.
  # Contado no registro do programa: na rajada o hub descarta parte do que manda a quem escuta
  "$BIN/dsi_manda" $SOCK 0x8000 206 0 1019 "08 65" 2 & LACO=$!
  sleep 2.5; kill $LACO 2>/dev/null; wait $LACO 2>/dev/null; sleep 0.3
  igual laco_e_cortado_depois_de_200 "$(grep -c 'pedido atendido: DSIMediaPlayer\.requestDetailInfo ' $T/stub_l.log)" 200
  tem laco_cortado_fica_registrado 'DSIMediaPlayer\.requestDetailInfo .*atendido mais de 200 vezes' $T/stub_l.log
  desce

  # --- K: perfil sem faixas ou sem fonte, e esquema que não é o de mídia: recusa, dizendo o que faltou
  grep -v '^entrylist' $T/p.txt > $T/sf.txt
  "$BIN/midiastub" -n $E $T/sf.txt > $T/sf.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q 'nenhuma faixa' $T/sf.log; then ok perfil_sem_faixas_e_recusado; else nok perfil_sem_faixas_e_recusado "rc=$rc: $(tail -n 1 $T/sf.log)"; fi
  grep -v '^deviceList\|^mediaList' $T/p.txt > $T/sm.txt
  "$BIN/midiastub" -n $E $T/sm.txt > $T/sm.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q 'nenhuma fonte' $T/sm.log; then ok perfil_sem_fonte_e_recusado; else nok perfil_sem_fonte_e_recusado "rc=$rc: $(tail -n 1 $T/sm.log)"; fi
  sed 's/entrylist\[2\].entryID = 103/entrylist[2].entryID = 101/' $T/p.txt > $T/sd.txt
  "$BIN/midiastub" -n $E $T/sd.txt > $T/sd.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q 'mesmo identificador' $T/sd.log; then ok faixas_com_o_mesmo_identificador_sao_recusadas; else nok faixas_com_o_mesmo_identificador_sao_recusadas "rc=$rc: $(tail -n 1 $T/sd.log)"; fi
  if [ -f "$MID/../radio/gerado/esquema.txt" ]; then
    "$BIN/midiastub" -n "$MID/../radio/gerado/esquema.txt" > $T/er.log 2>&1; rc=$?
    if [ $rc -ne 0 ] && grep -q 'não é o de mídia' $T/er.log; then ok esquema_de_outro_dominio_e_recusado; else nok esquema_de_outro_dominio_e_recusado "rc=$rc: $(tail -n 1 $T/er.log)"; fi
  else nok esquema_de_outro_dominio_e_recusado "não achei o esquema do rádio para a prova"; fi
else
  nok midiastub_compilado "não há $BIN/midiastub"
fi

# ---- o perfil da pasta: confere, tem uma fonte que a HMI lista, faixas com nome e tudo marcado
PF=$MID/minhas-midias.txt
if [ -f "$PF" ] && [ -x "$BIN/midiastub" ]; then
  "$BIN/midiastub" -n $E "$PF" > $T/pf.log 2>&1; igual perfil_da_pasta_confere "$?" 0
  "$BIN/midiastub" -m $E "$PF" > $T/pf.m 2>/dev/null
  # o tipo de aparelho tem de ser um dos que a HMI aceita como fonte (DeviceFilter.MEDIA: 0, 4, 8, 10, 15)
  tipos=$(sed -n 's/^deviceList\[[0-9]*\]\.deviceType *= *\([0-9]*\).*/\1/p' "$PF" | tr '\n' ' ')
  semtipo=$(( $(grep -c '^deviceList\[[0-9]*\]\.deviceID' "$PF") - $(grep -c '^deviceList\[[0-9]*\]\.deviceType' "$PF") ))
  ruim=0; for t in $tipos; do case $t in 0|4|8|10|15) ;; *) ruim=1 ;; esac; done
  if [ $ruim -eq 0 ] && [ "$(grep -c '^deviceList\[[0-9]*\]\.deviceID' "$PF")" -ge 1 ]; then ok "perfil_da_pasta_fonte_que_a_hmi_lista (tipos: ${tipos:-0}; $semtipo sem tipo, que vale 0)"
  else nok perfil_da_pasta_fonte_que_a_hmi_lista "tipos de aparelho: $tipos"; fi
  nf=$(grep -c '^entrylist\[[0-9]*\]\.entryID' "$PF"); nt=$(grep -c '^entrylist\[[0-9]*\]\.title' "$PF")
  if [ "$nf" -ge 3 ] && [ "$nf" = "$nt" ]; then ok "perfil_da_pasta_faixas_com_nome ($nf faixas)"; else nok perfil_da_pasta_faixas_com_nome "$nf faixas, $nt com nome"; fi
  tem perfil_da_pasta_marcado_como_simulacao 'SIMULAÇÃO' "$PF"
  tem perfil_da_pasta_responde_ao_idioma_dos_arquivos '^\[DSIFileBrowser\.FBLsetLanguageResult\]' "$PF"
else
  nok perfil_da_pasta_confere "não achei $PF ou o midiastub"
fi

# ---- o roteiro de compilação: se o midiastub não compilar, avisa e segue (ele inclui o veiculostub.c)
MB=$(dirname "$0")/../m1_build.sh
if [ -f "$MB" ]; then
  mkdir -p $T/fonte $T/saida; cp "$(dirname "$0")"/../*.c $T/fonte/ 2>/dev/null
  echo 'isto não compila' > $T/fonte/midiastub.c
  DRIVER=$T/fonte sh "$MB" $T/saida > $T/build.log 2>&1; rc=$?
  igual compilacao_segue_sem_o_midiastub "$rc" 0
  tem compilacao_avisa_do_midiastub 'AVISO: o midiastub não compilou' $T/build.log
  if [ ! -e $T/saida/midiastub ] && [ -x $T/saida/veiculostub ]; then ok compilacao_nao_deixa_binario_velho; else nok compilacao_nao_deixa_binario_velho "ficou um midiastub em $T/saida"; fi
else
  nok compilacao_segue_sem_o_midiastub "não achei $MB"
fi

# ---- o lançador do modo ao vivo: a mídia vem ligada, e a espera pelos serviços DSI sobe para 6 s.
# Roda uma CÓPIA do ao_vivo.sh numa bancada de mentira, com um "docker" que só anota o que recebeu
# (o jeito do t_memoria.sh): o docker de verdade não é chamado e a pasta ao-vivo/ de verdade não é
# tocada. Por que os 6 s: a HMI só cria os serviços de veículo 5 s depois de ver o carro de mentira
# no barramento; com o rádio, o telefone e a mídia de mentira a etapa da partida que precisa deles
# começa a esperar aos 2,2 s, e com 2 s de espera desistia 0,8 s cedo demais. A central nascia sem
# parte deles (rodadas h1 a h7 de 12/10/2026, traces/mid_espera*). A opção vai em EXTRA_VMOPTIONS e
# tem de chegar à máquina Java DEPOIS das duas que o m1_hmi.sh monta a partir da ESPERA: vale a última.
AV=$(dirname "$0")/../ao_vivo.sh; MH=$(dirname "$0")/../m1_hmi.sh
DSI=-Dde.vw.mib.asl.startup.timeout.dsi
if [ -f "$AV" ]; then
  BM=$T/bancada; mkdir -p $BM/driver $T/bin; cp "$AV" $BM/driver/
  printf '%s\n' '#!/bin/sh' 'for a in "$@"; do printf "%s\n" "$a"; done > "$DOCKER_VIU"' > $T/bin/docker; chmod +x $T/bin/docker
  aovivo() {        # aovivo [VARIAVEL=valor...]: roda a cópia só com essas variáveis
    rm -rf $BM/ao-vivo $T/docker.args
    ( unset MIDIA ESPERA ESPERA_DSI EXTRA_VMOPTIONS LSMVAL MEMORIA
      export DOCKER_VIU=$T/docker.args SEM_SERVIDOR=1 PATH="$T/bin:$PATH"
      env "$@" sh $BM/driver/ao_vivo.sh 5 >/dev/null 2>&1 )
  }
  viu() { grep -E -- "${1:-ESPERA|VMOPTIONS}" $T/docker.args 2>&1 | tr '\n' ' '; }
  arg() { if grep -qxF -- "$2" $T/docker.args 2>/dev/null; then ok "$1"; else nok "$1" "o docker não recebeu [$2]; recebeu: $(viu "$3")"; fi; }
  semdsi() { if [ -s $T/docker.args ] && ! grep -q -- 'timeout\.dsi' $T/docker.args; then ok "$1"; else nok "$1" "o docker recebeu: $(viu)"; fi; }
  aovivo
  arg ao_vivo_liga_a_midia_por_padrao "MIDIA=/stubs/midia/minhas-midias.txt" MIDIA
  arg ao_vivo_espera_6_s_pelos_servicos_dsi "EXTRA_VMOPTIONS=$DSI=6000"
  arg ao_vivo_espera_geral_segue_em_2_s "ESPERA=2000"
  aovivo MIDIA=
  arg ao_vivo_midia_vazia_desliga "MIDIA=" MIDIA
  aovivo ESPERA_DSI=4000
  arg ao_vivo_espera_dsi_a_mao "EXTRA_VMOPTIONS=$DSI=4000"
  aovivo ESPERA_DSI=
  semdsi ao_vivo_espera_dsi_vazia_volta_ao_que_era
  arg ao_vivo_espera_dsi_vazia_segue_com_a_geral "ESPERA=2000"
  aovivo ESPERA=15000
  semdsi ao_vivo_espera_a_mao_vale_para_as_duas
  arg ao_vivo_espera_a_mao_chega "ESPERA=15000"
  aovivo ESPERA=3000 ESPERA_DSI=9000
  arg ao_vivo_as_duas_esperas_a_mao_dsi "EXTRA_VMOPTIONS=$DSI=9000"
  arg ao_vivo_as_duas_esperas_a_mao_geral "ESPERA=3000"
else
  nok ao_vivo_espera_6_s_pelos_servicos_dsi "não achei $AV"
fi
if grep -q "^\[ -n \"\$ESPERA\" \] && EXTRA_VMOPTIONS=\"$DSI=\$ESPERA .*timeout\.rsi=\$ESPERA \$EXTRA_VMOPTIONS\"\$" "$MH" 2>/dev/null; then ok lancador_poe_as_opcoes_extras_depois_das_da_espera
else nok lancador_poe_as_opcoes_extras_depois_das_da_espera "a linha da ESPERA do m1_hmi.sh mudou: $(grep -n 'startup\.timeout' "$MH" 2>&1 | head -n 2)"; fi

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
