#!/bin/sh
# Teste do veiculostub (Marco 8): o "carro de mentira" que responde aos serviços DSI de veículo.
# Ele lê um esquema (que serviços existem, que mensagem atualiza cada atributo, que campos ela
# tem) e um ou mais perfis (os valores do carro); anuncia o domínio e, a cada atributo que a HMI
# assina, manda a atualização com o valor do perfil ou, sem perfil, o valor padrão marcado como
# válido. Desde o Marco 9 atende também os pedidos diretos: os que gravam um atributo e os que
# têm resposta própria (a parte "pedidos", mais abaixo). Roda na imagem moi3-bancada:1; não
# precisa do firmware.
#   t_veiculo.sh <pasta com kipc_hub, veiculostub, dsi_manda e dsi_escuta> [<pasta stubs/veiculo>]
# Quem faz o papel da HMI é o dsi_manda, com o canal 0x8000 (o remetente da HMI): a assinatura é
# a api 9999 na interface de pedidos do serviço, com os atributos em sint32 (zigue-zague), como
# foi visto no barramento em 11/10/2026 ("08 14 08 26" assina o 10 e o 19).
# Os bytes esperados foram calculados à parte, à mão; os dos "vetores" saíram das classes do
# próprio firmware (driver/m8_esquema.sh).
BIN=${1:-/work/bin}; VEI=${2:-/stubs/veiculo}; T=/tmp/t_veiculo; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
rm -f $SOCK
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
tem() { if grep -qE -- "$2" "$3"; then ok "$1"; else nok "$1" "não apareceu /$2/"; fi; }
naotem() { if grep -qE -- "$2" "$3"; then nok "$1" "apareceu /$2/: $(grep -E -- "$2" "$3" | head -n 1)"; else ok "$1"; fi; }
conta() { grep -cE -- "$1" "$2"; }
hmi() { "$BIN/dsi_manda" $SOCK 0x8000 "$@"; }      # hmi <interface> <instância> <api> <protobuf>

cat > $T/esq.txt <<'EOF'
# esquema de teste: um serviço no domínio 16 e outro no domínio da mídia
S SvcTeste 16 700 701 teste
A SvcTeste 1 ATTR_SIMPLES
A SvcTeste 2 ATTR_TIPOS
A SvcTeste 3 ATTR_LISTAS
A SvcTeste 4 ATTR_SEMFLAG
A SvcTeste 5 ATTR_HORA
A SvcTeste 6 ATTR_OUTRO
A SvcTeste 7 ATTR_CAMPOALTO
E SvcTeste 1 1 1 updateSimples TesteMsg.updateSimples
E SvcTeste 2 2 2 updateTipos TesteMsg.updateTipos
E SvcTeste 3 3 3 updateListas TesteMsg.updateListas
E SvcTeste 4 4 4 updateSemFlag TesteMsg.updateSemFlag
E SvcTeste 5 5 5 updateHora TesteMsg.updateHora
E SvcTeste 6 6 6 updateOutro TesteMsg.updateSimples
E SvcTeste 7 7 7 updateCampoAlto TesteMsg.updateCampoAlto
E SvcTeste 2000 - - responseAlgo TesteMsg.updateSimples
P SvcTeste 9999 setNotification TesteReq.setNotification
P SvcTeste 9998 clearNotification TesteReq.setNotification
P SvcTeste 1000 pedeAlgo TesteReq.pedeAlgo
S SvcMidia 8388608 710 711 midia
A SvcMidia 1 ATTR_FAIXA
E SvcMidia 1 1 1 updateFaixa TesteMsg.updateSimples
P SvcMidia 9999 setNotification TesteReq.setNotificationInt
M TesteMsg.updateSimples
C 1 valor sint32 um
C 2 validFlag sint32 um
M TesteMsg.updateTipos
C 1 a sint32 um
C 2 b int32 um
C 3 c uint32 um
C 4 d bool um
C 5 e float um
C 6 f double um
C 7 g string um
C 8 h msg um TesteMsg.Sub
C 9 i enum um
C 10 j sint64 um
C 11 k fixed32 um
C 12 validFlag sint32 um
M TesteMsg.Sub
C 1 x sint32 um
C 2 y sint32 um
M TesteMsg.updateListas
C 1 nums sint32 rep
C 2 pac sint32 pac
C 3 itens msg rep TesteMsg.Sub
C 4 validFlag sint32 um
M TesteMsg.updateSemFlag
C 1 valor sint32 um
M TesteMsg.updateHora
C 1 h sint32 um
C 2 m sint32 um
C 3 s sint32 um
C 4 ano sint32 um
C 5 mes sint32 um
C 6 dia sint32 um
C 7 validFlag sint32 um
M TesteMsg.updateCampoAlto
C 20 longe sint32 um
C 21 validFlag sint32 um
M TesteReq.setNotification
C 1 attributes sint32 rep
M TesteReq.setNotificationInt
C 1 attributes int32 rep
M TesteReq.pedeAlgo
C 1 n sint32 um
EOF

cat > $T/perfil.txt <<'EOF'
# perfil de teste
fuso = -3                      # horas a somar ao UTC nos valores @hora, @dia…

[SvcTeste.updateTipos]
a = -3
b = -1
c = 300
d = 1
e = 1.5
f = -2.25
g = "é\"x"                     # aspas dentro do texto vão com barra
h.x = 2
i = 3
j = -5000000000
k = 7

[SvcTeste.updateListas]
nums[0] = 1
nums[1] = -1
pac[0] = 3
pac[1] = 4
itens[0].x = 1
itens[1].y = 2

[SvcTeste.updateHora]
h = @hora
m = @minuto
s = @segundo
ano = @ano2000
mes = @mes
dia = @dia

[SvcTeste.updateOutro]
valor = 0x10
validFlag = 2                  # o perfil pode dizer que o dado é inválido

[SvcTeste.updateCampoAlto]
longe = 7

[SvcMidia.updateFaixa]
valor = 5
EOF

"$BIN/kipc_hub" 2>$T/hub.log & HUB=$!
i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
"$BIN/dsi_escuta" $SOCK >> $T/escuta.log 2>&1 & ESC=$!      # >>: o arquivo é zerado no meio do teste
"$BIN/veiculostub" $SOCK $T/esq.txt $T/perfil.txt > $T/stub.log 2>&1 & STUB=$!
sleep 1.4

E='canal=0x10 if=701'
tem anuncia_o_dominio '^canal=0x10 if=9000 inst=0 api=0 pb=$' $T/escuta.log
tem anuncia_cada_dominio_do_esquema '^canal=0x800000 if=9000 inst=0 api=0 pb=$' $T/escuta.log
naotem nada_sem_assinatura ' if=7[01]1 ' $T/escuta.log

# O anúncio sai UMA vez enquanto a HMI não aparece. A HMI liga dez dos serviços de veículo 5 s
# depois de cada anúncio, cada vez numa thread nova (BAPTaskScheduler), e a criação do serviço
# não é protegida (ServiceAdminImpl.startService confere e cria sem trava). O hub guarda as
# primeiras mensagens e as reentrega de uma vez a quem se liga: com um anúncio por segundo a
# HMI recebia uma rajada, as threads acordavam juntas e às vezes um serviço nascia duas vezes;
# o painel (DSICarKombi) ficava mudo em parte das partidas (rodadas men2 e sl de 11/10/2026).
sleep 2.2
n=$(conta '^canal=0x10 if=9000 ' $T/escuta.log)
if [ "$n" = 1 ]; then ok um_anuncio_so_antes_de_a_hmi_aparecer; else nok um_anuncio_so_antes_de_a_hmi_aparecer "$n anúncios do domínio 16 em 3,6 s"; fi
"$BIN/dsi_escuta" $SOCK > $T/tarde.log 2>&1 & e=$!; sleep 0.4; kill $e 2>/dev/null; wait $e 2>/dev/null
n=$(conta '^canal=0x10 if=9000 ' $T/tarde.log)
if [ "$n" = 1 ]; then ok quem_se_liga_depois_recebe_um_anuncio_so; else nok quem_se_liga_depois_recebe_um_anuncio_so "$n anúncios do domínio 16 na reentrega do hub"; fi
# A HMI aparece no barramento (uma mensagem com o canal dela) e não pede nada ao stub: passados
# 2 s ele anuncia de novo, espaçado, para o caso de o primeiro anúncio ter se perdido.
antes=$(conta '^canal=0x10 if=9000 ' $T/escuta.log)
hmi 1100 0 9999 "08 02"; sleep 3.5
depois=$(conta '^canal=0x10 if=9000 ' $T/escuta.log)
if [ $((depois - antes)) -ge 1 ] && [ $((depois - antes)) -le 2 ]; then ok volta_a_anunciar_se_a_hmi_nao_pede_nada
else nok volta_a_anunciar_se_a_hmi_nao_pede_nada "$((depois - antes)) anúncios novos em 3,5 s (esperava 1 ou 2)"; fi

hmi 700 0 9999 "08 02"; sleep 0.3
tem sem_perfil_responde_padrao_valido "^$E inst=0 api=1 pb=10 02\$" $T/escuta.log

hmi 700 0 9999 "08 04"; sleep 0.3
tem perfil_vale_sobre_o_padrao "^$E inst=0 api=2 pb=08 05 10 ff ff ff ff ff ff ff ff ff 01 18 ac 02 20 01 2d 00 00 c0 3f 31 00 00 00 00 00 00 02 c0 3a 04 c3 a9 22 78 42 02 08 04 48 03 50 ff c7 af a0 25 5d 07 00 00 00 60 02\$" $T/escuta.log

hmi 700 0 9999 "08 06 08 08"; sleep 0.3
tem varios_atributos_num_pedido_listas "^$E inst=0 api=3 pb=08 02 08 01 12 02 06 08 1a 02 08 02 1a 02 10 04 20 02\$" $T/escuta.log
tem varios_atributos_num_pedido_sem_validflag "^$E inst=0 api=4 pb=\$" $T/escuta.log

hmi 700 0 9999 "0a 02 0c 0e"; sleep 0.3          # os mesmos atributos empacotados num bloco
tem assinatura_empacotada "^$E inst=0 api=6 pb=08 20 10 04\$" $T/escuta.log
tem validflag_do_perfil "^$E inst=0 api=6 pb=08 20 10 04\$" $T/escuta.log
tem campo_de_numero_alto "^$E inst=0 api=7 pb=a0 01 0e a8 01 02\$" $T/escuta.log

hmi 700 2 9999 "08 02"; sleep 0.3
tem resposta_na_instancia_do_pedido "^$E inst=2 api=1 pb=10 02\$" $T/escuta.log

antes=$(conta ' if=701 ' $T/escuta.log)
hmi 700 0 9999 "08 c6 01"; sleep 0.3             # atributo 99: não existe
depois=$(conta ' if=701 ' $T/escuta.log)
if [ "$antes" = "$depois" ]; then ok atributo_sem_evento_nao_responde; else nok atributo_sem_evento_nao_responde "$antes mensagens antes, $depois depois"; fi
tem atributo_sem_evento_fica_registrado 'atributo 99' $T/stub.log

hmi 700 0 1000 "08 0e"; sleep 0.3                # pedido que não é assinatura
tem pedido_direto_fica_registrado 'SvcTeste.pedeAlgo' $T/stub.log
tem esquema_sem_tabela_de_respostas_nao_afirma_nada 'SvcTeste\.pedeAlgo .*não diz se a HMI espera resposta' $T/stub.log
hmi 700 0 1234 "-"; sleep 0.2
tem pedido_desconhecido_fica_registrado 'api 1234' $T/stub.log

hmi 710 0 9999 "08 01"; sleep 0.3                # neste serviço os atributos vão em int32 comum
tem outro_dominio_responde_com_o_canal_dele '^canal=0x800000 if=711 inst=0 api=1 pb=08 0a 10 02$' $T/escuta.log

# relógio: os valores @… acompanham a hora de verdade, no fuso do perfil, e a mensagem sai de
# novo quando muda. O que não muda não é repetido.
zz() { printf '%02x' $(($1 * 2)); }
hmi 700 0 9999 "08 0a"
a=$(( $(date -u +%s) - 3 * 3600 )); sleep 0.25
prim=$(grep -E "^$E inst=0 api=5 " $T/escuta.log | head -n 1)
achou=0
for d in 0 1 -1; do
  set -- $(date -u -d "@$((a + d))" "+%-H %-M %-S %y %-m %-d")
  esp="pb="; n=1
  for v in "$@"; do [ "$v" != 0 ] && esp="$esp$(printf '%02x' $((n * 8))) $(zz $v) "; n=$((n+1)); done
  esp="${esp}38 02"
  [ "$prim" = "$E inst=0 api=5 $esp" ] && achou=1
done
if [ $achou = 1 ]; then ok relogio_com_a_hora_do_fuso; else nok relogio_com_a_hora_do_fuso "veio '$prim'; a hora UTC-3 era $(date -u -d "@$a" "+%H:%M:%S de %d/%m/%y")"; fi
anuncios=$(conta '^canal=0x10 if=9000 ' $T/escuta.log)
sleep 2.4
agora=$(conta '^canal=0x10 if=9000 ' $T/escuta.log)
if [ "$anuncios" = "$agora" ]; then ok para_de_anunciar_depois_do_primeiro_pedido; else nok para_de_anunciar_depois_do_primeiro_pedido "$((agora - anuncios)) anúncios em 2,4 s, com a HMI já atendida"; fi
n=$(grep -E "^$E inst=0 api=5 " $T/escuta.log | sort -u | wc -l)
if [ "$n" -ge 3 ]; then ok relogio_sai_de_novo_quando_muda; else nok relogio_sai_de_novo_quando_muda "$n mensagens diferentes em 2,6 s"; fi
n=$(conta "^$E inst=0 api=2 " $T/escuta.log)
if [ "$n" = 1 ]; then ok o_que_nao_muda_nao_repete; else nok o_que_nao_muda_nao_repete "a mensagem dos tipos saiu $n vezes"; fi

hmi 700 0 9998 "08 0a"; sleep 1.3                # desassina o relógio
antes=$(conta "^$E inst=0 api=5 " $T/escuta.log); sleep 2.2
depois=$(conta "^$E inst=0 api=5 " $T/escuta.log)
if [ "$antes" = "$depois" ]; then ok desassinar_para_de_publicar; else nok desassinar_para_de_publicar "$antes antes, $depois depois"; fi

if kill -0 $STUB 2>/dev/null; then ok stub_continua_de_pe; else nok stub_continua_de_pe "morreu: $(tail -n 2 $T/stub.log)"; fi
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# ---- dois perfis: o de depois vale sobre o de antes, campo a campo
printf '[SvcTeste.updateTipos]\na = 9\n[SvcTeste.updateSimples]\nvalor = 4\n' > $T/perfil2.txt
"$BIN/veiculostub" -m $T/esq.txt $T/perfil.txt $T/perfil2.txt > $T/mostra.log 2>$T/mostra.err; rc=$?
if [ $rc -eq 0 ]; then ok modo_mostra_termina_bem; else nok modo_mostra_termina_bem "saiu com $rc: $(tail -n 1 $T/mostra.err)"; fi
tem segundo_perfil_troca_um_campo "^$E inst=0 api=2 pb=08 12 10 ff " $T/mostra.log
tem segundo_perfil_mantem_os_outros "^$E inst=0 api=2 pb=.* 5d 07 00 00 00 60 02\$" $T/mostra.log
tem segundo_perfil_acrescenta_evento "^$E inst=0 api=1 pb=08 08 10 02\$" $T/mostra.log
naotem evento_sem_atributo_e_sem_perfil_nao_aparece "^$E inst=0 api=2000 " $T/mostra.log

# ---- lista com buraco: o que falta no meio vai vazio
printf '[SvcTeste.updateListas]\nnums[2] = 9\nitens[1].y = 5\n' > $T/buraco.txt
"$BIN/veiculostub" -m $T/esq.txt $T/buraco.txt > $T/buraco.log 2>&1
tem lista_com_buraco "^$E inst=0 api=3 pb=08 00 08 00 08 12 1a 00 1a 02 10 0a 20 02\$" $T/buraco.log

# ---- perfil ou esquema com erro: nada sobe, e a mensagem diz o arquivo e a linha
errado() {   # errado <nome> <conteúdo do perfil>
  printf '%b' "$2" > $T/errado.txt
  "$BIN/veiculostub" -n $T/esq.txt $T/perfil.txt $T/errado.txt > $T/err.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q 'errado.txt, linha 2' $T/err.log; then ok "$1"
  else nok "$1" "rc=$rc: $(tail -n 1 $T/err.log)"; fi
}
errado servico_desconhecido_recusa      '# x\n[SvcNada.updateSimples]\nvalor = 1\n'
errado evento_desconhecido_recusa       '# x\n[SvcTeste.updateNada]\nvalor = 1\n'
errado campo_desconhecido_recusa        '[SvcTeste.updateSimples]\nnada = 1\n'
errado valor_fora_de_secao_recusa       '# x\nvalor = 1\n'
errado numero_invalido_recusa           '[SvcTeste.updateSimples]\nvalor = abc\n'
errado numero_grande_demais_recusa      '[SvcTeste.updateSimples]\nvalor = 3000000000\n'
errado negativo_em_campo_sem_sinal_recusa '[SvcTeste.updateTipos]\nc = -1\n'
errado booleano_invalido_recusa         '[SvcTeste.updateTipos]\nd = 2\n'
errado texto_sem_aspas_recusa           '[SvcTeste.updateTipos]\ng = abc\n'
errado valor_em_mensagem_recusa         '[SvcTeste.updateTipos]\nh = 1\n'
errado indice_em_campo_simples_recusa   '[SvcTeste.updateSimples]\nvalor[0] = 1\n'
errado lista_sem_indice_recusa          '[SvcTeste.updateListas]\nnums = 1\n'
errado marcador_desconhecido_recusa     '[SvcTeste.updateSimples]\nvalor = @nada\n'
errado marcador_em_texto_recusa         '[SvcTeste.updateTipos]\ng = @hora\n'
errado linha_sem_igual_recusa           '[SvcTeste.updateSimples]\nqualquer coisa\n'

"$BIN/veiculostub" -n $T/esq.txt $T/perfil.txt > $T/ok.log 2>&1
if [ $? -eq 0 ]; then ok perfil_certo_passa_na_conferencia; else nok perfil_certo_passa_na_conferencia "$(tail -n 1 $T/ok.log)"; fi
"$BIN/veiculostub" -n $T/esq.txt $T/nao-existe.txt > $T/ne.log 2>&1
if [ $? -ne 0 ]; then ok perfil_ausente_da_erro; else nok perfil_ausente_da_erro "saiu com 0"; fi
printf 'S SvcTeste 16 700 701 teste\nE SvcTeste 1 1 1 updateSimples TesteMsg.naoExiste\n' > $T/esq-errado.txt
"$BIN/veiculostub" -n $T/esq-errado.txt > $T/esqerr.log 2>&1; rc=$?
if [ $rc -ne 0 ] && grep -q 'esq-errado.txt, linha 2' $T/esqerr.log; then ok esquema_com_mensagem_que_nao_existe_recusa
else nok esquema_com_mensagem_que_nao_existe_recusa "rc=$rc: $(tail -n 1 $T/esqerr.log)"; fi
# com erro, o stub não chega a falar no barramento
: > $T/escuta.log
printf '[SvcTeste.updateSimples]\nnada = 1\n' > $T/errado.txt
"$BIN/veiculostub" $SOCK $T/esq.txt $T/errado.txt > $T/err2.log 2>&1; rc=$?
sleep 0.3
if [ $rc -ne 0 ] && ! grep -q 'if=9000' $T/escuta.log; then ok com_erro_nada_vai_ao_barramento
else nok com_erro_nada_vai_ao_barramento "rc=$rc; anúncios: $(conta 'if=9000' $T/escuta.log)"; fi

# ---- pedidos (Marco 9). Além de assinar, a HMI manda pedidos diretos, de três tipos:
#  - os que GRAVAM um atributo (set<X>): o carro de mentira guarda o valor e manda a atualização
#    do atributo, que é a resposta que a HMI espera desses (linhas G do esquema). Só nos
#    serviços que o perfil lista no ajuste "grava"; o perfil pode dar regras a mais, em seções
#    [Serviço.pedido -> evento];
#  - os que têm RESPOSTA própria e prazo (linhas R, da tabela do proxy DSI da HMI): a resposta
#    sai se o perfil tiver a seção dela, com os valores do perfil e, onde ele não diz nada, com
#    o que as linhas X mandam devolver do pedido (o número da transação, por exemplo);
#  - os que a HMI manda sem esperar nada: ficam só registrados.
# Campo cujo valor no perfil é um marcador (@hora…) não é trocado por um número fixo: o pedido
# acerta o relógio do carro de mentira, que segue andando a partir dali.
# Duas defesas contra laço, postas depois da rodada n4 de 11/10/2026, em que a HMI e o carro de
# mentira ficaram se respondendo no idioma do painel (setMenuLanguage 1, 26, 1, 26…, 222 mil
# vezes em 75 s): o ajuste "ignora" do perfil tira um pedido do atendimento, e o pedido atendido
# mais de 200 vezes em 10 s deixa de ser atendido até o programa reiniciar.
cat $T/esq.txt > $T/esqp.txt
cat >> $T/esqp.txt <<'EOF'
S SvcOutro 16 720 721 outro
A SvcTeste 8 ATTR_UNIDADE
A SvcTeste 9 ATTR_FUSO
A SvcTeste 10 ATTR_TEXTO
E SvcTeste 8 8 8 updateUnidade TesteMsg.updateUnidade
E SvcTeste 9 9 9 updateFuso TesteMsg.updateFuso
E SvcTeste 10 10 10 updateTexto TesteMsg.updateTexto
E SvcTeste 2001 - - responseLista TesteMsg.responseLista
E SvcTeste 2002 - - responseOutra TesteMsg.responseLista
E SvcTeste 2003 - - acknowledgeZera TesteMsg.acknowledgeZera
P SvcTeste 1001 setUnidade TesteReq.setUnidade
P SvcTeste 1002 setHora TesteReq.setHora
P SvcTeste 1003 setData TesteReq.setData
P SvcTeste 1004 setFuso TesteReq.setFuso
P SvcTeste 1005 requestLista TesteReq.requestLista
P SvcTeste 1006 requestOutra TesteReq.requestLista
P SvcTeste 1007 setZera -
P SvcTeste 1008 setTexto TesteReq.setTexto
P SvcTeste 1009 requestSequencia TesteReq.requestLista
P SvcTeste 1010 requestPerdida TesteReq.requestLista
P SvcTeste 1011 setIgnorada TesteReq.setUnidade
P SvcTeste 1012 setEmLaco TesteReq.setUnidade
P SvcMidia 1001 setFaixa TesteReq.setUnidade
R SvcTeste 1005 uma 10000 2001 responseLista
R SvcTeste 1006 uma 10000 2002 responseOutra
R SvcTeste 1007 uma 10000 2003 acknowledgeZera
R SvcTeste 1009 sequencia 10000 2001 responseLista
R SvcTeste 1009 sequencia 10000 2003 acknowledgeZera
R SvcTeste 1010 uma 10000 - responsePerdida
G SvcTeste 1001 8 posicao unidade unidade
G SvcTeste 1011 8 posicao unidade unidade
G SvcTeste 1012 8 posicao unidade unidade
G SvcTeste 1002 5 nomes h h
G SvcTeste 1002 5 nomes m m
G SvcTeste 1002 5 nomes s s
G SvcTeste 1003 5 nomes data.ano ano
G SvcTeste 1003 5 nomes data.mes mes
G SvcTeste 1003 5 nomes data.dia dia
G SvcTeste 1004 9 posicao offset fuso
G SvcTeste 1008 10 nomes texto caixa.texto
G SvcTeste 1008 10 nomes ligado caixa.ligado
G SvcMidia 1001 1 posicao unidade valor
X SvcTeste 1005 2001 info.inicio info.inicio
X SvcTeste 1005 2001 info.quantos info.quantos
X SvcTeste 1005 2001 info.transacao info.transacao
M TesteMsg.updateUnidade
C 1 unidade sint32 um
C 2 validFlag sint32 um
M TesteMsg.updateFuso
C 1 fuso float um
C 2 validFlag sint32 um
M TesteMsg.updateTexto
C 1 caixa msg um TesteMsg.Caixa
C 2 validFlag sint32 um
M TesteMsg.Caixa
C 1 texto string um
C 2 ligado bool um
M TesteMsg.responseLista
C 1 info msg um TesteMsg.Info
C 2 itens msg rep TesteMsg.Sub
M TesteMsg.Info
C 1 inicio sint32 um
C 2 quantos sint32 um
C 3 transacao sint32 um
M TesteMsg.acknowledgeZera
C 1 success bool um
M TesteReq.setUnidade
C 1 unidade sint32 um
M TesteReq.setHora
C 1 h sint32 um
C 2 m sint32 um
C 3 s sint32 um
M TesteReq.setData
C 1 data msg um TesteReq.Data
M TesteReq.Data
C 1 ano sint32 um
C 2 mes sint32 um
C 3 dia sint32 um
M TesteReq.setFuso
C 1 offset float um
M TesteReq.requestLista
C 1 info msg um TesteMsg.Info
M TesteReq.setTexto
C 1 texto string um
C 2 ligado bool um
EOF

cat > $T/perfilp.txt <<'EOF'
# perfil de teste dos pedidos
fuso = -3
grava = SvcOutro SvcTeste          # serviços em que o que a HMI grava é aceito
ignora = SvcTeste.setIgnorada      # menos este pedido: fica sem resposta

[SvcTeste.updateHora]
h = @hora
m = @minuto
s = @segundo
ano = @ano2000
mes = @mes
dia = @dia

[SvcTeste.updateUnidade]
unidade = 2

[SvcTeste.responseLista]           # a seção da resposta é o que faz o pedido ser respondido
info.quantos = 0                   # dito no perfil: vale sobre o que veio no pedido

[SvcTeste.acknowledgeZera]
success = 1

[SvcTeste.setUnidade -> updateSimples]     # regra a mais: este pedido grava também outro evento
valor = unidade

[SvcMidia.updateFaixa]
valor = 5
EOF

"$BIN/veiculostub" $SOCK $T/esqp.txt $T/perfilp.txt > $T/stubp.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log

# -- pedidos que gravam
hmi 700 0 9999 "08 10 08 02"; sleep 0.3          # assina a unidade (8) e o simples (1)
hmi 700 0 1001 "08 06"; sleep 0.3                # setUnidade(3)
tem gravar_manda_a_atualizacao_com_o_valor_novo "^$E inst=0 api=8 pb=08 06 10 02\$" $T/escuta.log
tem regra_do_perfil_grava_em_outro_evento "^$E inst=0 api=1 pb=08 06 10 02\$" $T/escuta.log
tem gravacao_fica_registrada 'pedido atendido: SvcTeste\.setUnidade .*gravei updateUnidade\.unidade = 3' $T/stubp.log
hmi 700 2 9999 "08 10"; sleep 0.3                # outra instância assina depois
tem quem_assina_depois_recebe_o_valor_gravado "^$E inst=2 api=8 pb=08 06 10 02\$" $T/escuta.log
hmi 700 0 1001 "08 08"; sleep 0.3                # setUnidade(4)
tem gravar_avisa_as_outras_instancias "^$E inst=2 api=8 pb=08 08 10 02\$" $T/escuta.log
antes=$(conta "^$E inst=0 api=8 pb=08 08 10 02\$" $T/escuta.log)
hmi 700 0 1001 "08 08"; sleep 0.3                # o mesmo valor: a HMI espera a atualização do mesmo jeito
depois=$(conta "^$E inst=0 api=8 pb=08 08 10 02\$" $T/escuta.log)
if [ $((depois - antes)) = 1 ]; then ok gravar_o_mesmo_valor_tambem_responde; else nok gravar_o_mesmo_valor_tambem_responde "$((depois - antes)) atualizações (esperava 1)"; fi
hmi 700 0 1001 "-"; sleep 0.3                    # setUnidade(0): no protobuf 3 o zero nem vai no fio
tem gravar_zero_tira_o_campo_do_fio "^$E inst=0 api=8 pb=10 02\$" $T/escuta.log
hmi 700 0 1011 "08 0a"; sleep 0.3                # setIgnorada(5): o perfil manda ignorar
naotem pedido_ignorado_nao_grava "^$E inst=[0-9]* api=8 pb=08 0a 10 02\$" $T/escuta.log
tem pedido_ignorado_fica_registrado 'pedido sem resposta: SvcTeste\.setIgnorada .*o perfil manda ignorar' $T/stubp.log
hmi 700 0 1004 "0d 00 00 00 c0"; sleep 0.3       # setFuso(-2.0), sem ninguém assinando o atributo 9
naotem gravar_sem_assinante_nao_manda_nada "^$E inst=[0-9]* api=9 " $T/escuta.log
hmi 700 0 9999 "08 12"; sleep 0.3
tem gravar_sem_assinante_guarda_o_valor "^$E inst=0 api=9 pb=0d 00 00 00 c0 10 02\$" $T/escuta.log
hmi 700 0 9999 "08 14"; sleep 0.2                # assina o texto (10)
hmi 700 0 1008 "0a 02 c3 a9 10 01"; sleep 0.3    # setTexto("é", ligado)
tem gravar_texto_e_booleano_em_mensagem_de_dentro "^$E inst=0 api=10 pb=0a 06 0a 02 c3 a9 10 01 10 02\$" $T/escuta.log
hmi 710 0 9999 "08 01"; sleep 0.3                # o serviço da mídia não está em "grava"
antes=$(conta ' if=711 ' $T/escuta.log)
hmi 710 0 1001 "08 06"; sleep 0.3
depois=$(conta ' if=711 ' $T/escuta.log)
if [ "$antes" = "$depois" ]; then ok servico_fora_de_grava_nao_muda; else nok servico_fora_de_grava_nao_muda "$antes mensagens antes, $depois depois"; fi
tem servico_fora_de_grava_fica_registrado 'pedido sem resposta: SvcMidia\.setFaixa .*"grava"' $T/stubp.log
hmi 700 0 1000 "08 0e"; sleep 0.2                # pedeAlgo: nem grava nem tem resposta na tabela da HMI
tem pedido_sem_prazo_de_resposta_fica_registrado_assim 'pedido sem resposta: SvcTeste\.pedeAlgo .*não marca prazo de resposta' $T/stubp.log

# -- relógio: gravar num campo que o perfil enche com marcador acerta o relógio
hoje() { set -- $(date -u -d "@$(( $(date -u +%s) - 3 * 3600 ))" "+%y %-m %-d"); echo "20 $(zz $1) 28 $(zz $2) 30 $(zz $3)"; }
hmi 700 0 9999 "08 0a"; sleep 0.3                # assina a hora (5)
H=$(hoje)
hmi 700 0 1002 "08 0e 10 10"; sleep 0.3          # setHora(7, 8, 0)
tem acertar_a_hora_muda_o_relogio "^$E inst=0 api=5 pb=08 0e 10 10 $H 38 02\$" $T/escuta.log
tem acerto_da_hora_fica_registrado 'pedido atendido: SvcTeste\.setHora .*acertei o relógio para 07:08:00' $T/stubp.log
sleep 2.4
tem relogio_acertado_continua_andando "^$E inst=0 api=5 pb=08 0e 10 10 18 04 $H 38 02\$" $T/escuta.log
hmi 700 0 1003 "0a 06 08 3c 10 04 18 06"; sleep 0.3      # setData(30, 2, 3): 3 de fevereiro de 2030
tem acertar_a_data_mantem_a_hora "^$E inst=0 api=5 pb=08 0e 10 10 18 [0-9a-f]{2} 20 3c 28 04 30 06 38 02\$" $T/escuta.log
hmi 700 0 1002 "08 32"; sleep 0.3                # setHora(25, 0, 0): hora que não existe
naotem hora_que_nao_existe_nao_acerta_o_relogio "^$E inst=0 api=5 pb=08 (32|0e 18) " $T/escuta.log
tem hora_que_nao_existe_fica_registrada 'SvcTeste\.setHora .*fora do intervalo' $T/stubp.log

# -- pedidos com resposta própria
hmi 700 0 1005 "0a 06 08 02 10 06 18 0a"; sleep 0.3      # requestLista(início 1, 3 itens, transação 5)
tem pedido_com_resposta_no_perfil_e_respondido "^$E inst=0 api=2001 " $T/escuta.log
tem resposta_devolve_a_transacao_do_pedido "^$E inst=0 api=2001 pb=.* 18 0a\$" $T/escuta.log
tem valor_do_perfil_vale_sobre_o_do_pedido "^$E inst=0 api=2001 pb=0a 04 08 02 18 0a\$" $T/escuta.log
tem resposta_fica_registrada 'pedido atendido: SvcTeste\.requestLista .*respondi responseLista' $T/stubp.log
hmi 700 3 1005 "0a 02 18 0c"; sleep 0.3                  # outra instância, transação 6, sem início
tem resposta_sai_na_instancia_do_pedido "^$E inst=3 api=2001 " $T/escuta.log
tem resposta_nao_guarda_o_pedido_anterior "^$E inst=3 api=2001 pb=0a 02 18 0c\$" $T/escuta.log
hmi 700 0 1007 "-"; sleep 0.3                            # setZera: pedido sem mensagem
tem resposta_a_pedido_sem_mensagem "^$E inst=0 api=2003 pb=08 01\$" $T/escuta.log
hmi 700 0 1006 "0a 02 18 0a"; sleep 0.3                  # requestOutra: o perfil não tem a seção da resposta
naotem resposta_sem_secao_no_perfil_nao_sai "^$E inst=0 api=2002 " $T/escuta.log
tem resposta_que_falta_fica_registrada 'pedido sem resposta: SvcTeste\.requestOutra .*espera responseOutra' $T/stubp.log
antes=$(conta "^$E inst=[0-9]* api=200[0-9] " $T/escuta.log)
hmi 700 0 1009 "0a 02 18 0a"; sleep 0.3                  # requestSequencia: a HMI espera duas respostas, em ordem
hmi 700 0 1010 "0a 02 18 0a"; sleep 0.3                  # requestPerdida: a resposta não tem api na camada da LG
depois=$(conta "^$E inst=[0-9]* api=200[0-9] " $T/escuta.log)
if [ "$antes" = "$depois" ]; then ok resposta_que_nao_sabe_montar_nao_e_inventada; else nok resposta_que_nao_sabe_montar_nao_e_inventada "$((depois - antes)) respostas a mais"; fi
tem resposta_em_sequencia_fica_registrada 'pedido sem resposta: SvcTeste\.requestSequencia .*sequencia' $T/stubp.log
tem resposta_sem_api_fica_registrada 'pedido sem resposta: SvcTeste\.requestPerdida .*responsePerdida' $T/stubp.log
# laço: o mesmo pedido, sem parar (o dsi_manda repete a cada 2 ms). O stub atende 200 e corta.
L="^$E inst=0 api=8 pb=08 0e 10 02\$"
"$BIN/dsi_manda" $SOCK 0x8000 700 0 1012 "08 0e" 2 & LACO=$!       # setEmLaco(7), centenas de vezes
sleep 2.5; kill $LACO 2>/dev/null; wait $LACO 2>/dev/null; sleep 0.3
n=$(conta "$L" $T/escuta.log)
if [ "$n" = 200 ]; then ok pedido_em_laco_e_cortado_em_200; else nok pedido_em_laco_e_cortado_em_200 "$n atualizações em 2,5 s de pedidos seguidos (esperava 200)"; fi
tem laco_cortado_fica_registrado 'SvcTeste\.setEmLaco .*em laço' $T/stubp.log
hmi 700 0 1012 "08 0e"; sleep 0.3                # o pedido cortado segue cortado
n2=$(conta "$L" $T/escuta.log)
if [ "$n" = "$n2" ]; then ok pedido_cortado_fica_sem_resposta; else nok pedido_cortado_fica_sem_resposta "$((n2 - n)) atualizações depois do corte"; fi
hmi 700 0 1001 "08 0c"; sleep 0.3                # os outros pedidos seguem atendidos: setUnidade(6)
tem outro_pedido_segue_atendido_depois_do_corte "^$E inst=0 api=8 pb=08 0c 10 02\$" $T/escuta.log
if kill -0 $STUB 2>/dev/null; then ok stub_continua_de_pe_depois_dos_pedidos; else nok stub_continua_de_pe_depois_dos_pedidos "morreu: $(tail -n 2 $T/stubp.log)"; fi
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# -- "grava" de um perfil de depois troca a lista do de antes
printf 'grava = SvcMidia\n' > $T/perfilg.txt
"$BIN/veiculostub" $SOCK $T/esqp.txt $T/perfilp.txt $T/perfilg.txt > $T/stubg.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log
hmi 710 0 9999 "08 01"; hmi 700 0 9999 "08 10"; sleep 0.3
hmi 710 0 1001 "08 06"; hmi 700 0 1001 "08 06"; sleep 0.3
tem grava_do_perfil_de_depois_vale '^canal=0x800000 if=711 inst=0 api=1 pb=08 06 10 02$' $T/escuta.log
naotem grava_do_perfil_de_antes_deixa_de_valer "^$E inst=0 api=8 pb=08 06 10 02\$" $T/escuta.log
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# -- perfil ou esquema com erro nos pedidos: nada sobe, e a mensagem diz o arquivo e a linha
"$BIN/veiculostub" -n $T/esqp.txt $T/perfilp.txt > $T/okp.log 2>&1
if [ $? -eq 0 ]; then ok perfil_com_pedidos_passa_na_conferencia; else nok perfil_com_pedidos_passa_na_conferencia "$(tail -n 1 $T/okp.log)"; fi
# os dois ajustes vazios esvaziam as listas: é como um perfil de depois desliga o que o de antes
# ligou (a rodada laco1 de 11/10/2026 tirou assim o "ignora" do perfil da pasta)
printf 'grava =\nignora =\n' > $T/perfilv.txt
"$BIN/veiculostub" -n $T/esqp.txt $T/perfilp.txt > $T/cheio.log 2>&1
"$BIN/veiculostub" -n $T/esqp.txt $T/perfilp.txt $T/perfilv.txt > $T/vazio.log 2>&1
tem ajustes_do_perfil_saem_no_resumo 'grava: SvcTeste SvcOutro; ignora: SvcTeste\.setIgnorada$' $T/cheio.log
tem ajustes_vazios_esvaziam_as_listas 'grava: nenhum serviço; ignora: nenhum pedido$' $T/vazio.log
erradop() {   # erradop <nome> <conteúdo do perfil>
  printf '%b' "$2" > $T/errado.txt
  "$BIN/veiculostub" -n $T/esqp.txt $T/perfilp.txt $T/errado.txt > $T/err.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q 'errado.txt, linha 2' $T/err.log; then ok "$1"
  else nok "$1" "rc=$rc: $(tail -n 1 $T/err.log)"; fi
}
erradop grava_com_servico_desconhecido_recusa         '# x\ngrava = SvcNada\n'
erradop ignora_com_pedido_desconhecido_recusa         '# x\nignora = SvcTeste.setNada\n'
erradop ignora_sem_servico_recusa                     '# x\nignora = setUnidade\n'
erradop regra_com_pedido_desconhecido_recusa          '# x\n[SvcTeste.setNada -> updateSimples]\nvalor = unidade\n'
erradop regra_com_evento_desconhecido_recusa          '# x\n[SvcTeste.setUnidade -> updateNada]\nvalor = unidade\n'
erradop regra_com_campo_do_pedido_desconhecido_recusa '[SvcTeste.setUnidade -> updateSimples]\nvalor = nada\n'
erradop regra_com_tipos_diferentes_recusa             '[SvcTeste.setFuso -> updateSimples]\nvalor = offset\n'
erradop regra_para_campo_que_e_mensagem_recusa        '[SvcTeste.setTexto -> updateTexto]\ncaixa = texto\n'
esqerrado() {   # esqerrado <nome> <linha a mais no fim do esquema>
  cat $T/esqp.txt > $T/esq-e.txt; printf '%s\n' "$2" >> $T/esq-e.txt
  n=$(grep -c '' $T/esq-e.txt)
  "$BIN/veiculostub" -n $T/esq-e.txt > $T/esqerr.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -q "esq-e.txt, linha $n:" $T/esqerr.log; then ok "$1"
  else nok "$1" "rc=$rc: $(tail -n 1 $T/esqerr.log)"; fi
}
esqerrado esquema_com_gravacao_de_pedido_que_nao_existe_recusa 'G SvcTeste 1999 8 posicao unidade unidade'
esqerrado esquema_com_gravacao_em_evento_que_nao_existe_recusa 'G SvcTeste 1001 99 posicao unidade unidade'
esqerrado esquema_com_gravacao_de_campo_que_nao_existe_recusa  'G SvcTeste 1001 8 posicao nada unidade'
esqerrado esquema_com_gravacao_de_tipos_diferentes_recusa      'G SvcTeste 1004 8 posicao offset unidade'
esqerrado esquema_com_resposta_que_nao_existe_recusa           'R SvcTeste 1001 uma 10000 2999 responseNada'
esqerrado esquema_com_tipo_de_resposta_desconhecido_recusa     'R SvcTeste 1001 talvez 10000 2001 responseLista'
esqerrado esquema_com_devolucao_de_campo_que_nao_existe_recusa 'X SvcTeste 1005 2001 info.nada info.inicio'

# ---- o estado: o que a tela grava sobrevive à partida seguinte (12/10/2026)
# No carro quem guarda a hora acertada e as unidades escolhidas é o painel, e elas voltam quando o
# carro é ligado de novo. Com "-e <arquivo>" o carro de mentira faz o mesmo: a cada pedido que
# grava, escreve nesse arquivo o acerto do relógio e os valores gravados, e a partida seguinte os
# aplica por cima dos perfis. O arquivo tem a forma de um perfil (seções e "caminho = valor") e
# duas linhas só dele: "acerto = <segundos>", quanto o relógio foi adiantado em relação à hora do
# computador no fuso do perfil (guarda a diferença, e não a hora: o relógio do painel segue
# andando com a central desligada), e "fim = <n>", a última, com a conta dos valores, que é como
# se reconhece um arquivo cortado. Arquivo com erro não impede a partida nem é aplicado pela
# metade: vale só o perfil, o motivo sai no registro e o arquivo fica de lado, em
# <arquivo>.recusado. A gravação é ao lado, com troca de nome. Sem "-e" nada é guardado: é o caso
# de todos os testes acima.
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
ED=$T/estado; EST=$ED/estado.txt; rm -rf $ED; mkdir -p $ED
semc() { grep -v '^#' "$1" 2>/dev/null; }                 # o estado sem as linhas de comentário
no_estado() { if [ -f $EST ] && grep -qE -- "$2" $EST; then ok "$1"; else nok "$1" "não está no estado /$2/: $(semc $EST | tr '\n' '|' | cut -c1-200)"; fi; }
fora_do_estado() { if [ -f $EST ] && ! grep -qE -- "$2" $EST; then ok "$1"; else nok "$1" "está no estado, ou não há estado, /$2/: $(semc $EST | tr '\n' '|' | cut -c1-200)"; fi; }
inode() { ls -i "$1" 2>/dev/null | cut -d' ' -f1; }
# Cada partida tem um hub novo, como na bancada (central nova, container novo). O hub reentrega a
# quem se liga as 48 primeiras mensagens que viu, e entre elas estão pedidos de gravar das sessões
# de antes deste teste: num hub velho o stub recém-nascido os receberia de novo e regravaria o
# estado sozinho. Na bancada isso não acontece: o stub é o primeiro a se ligar ao hub.
hub_novo() {
  kill $ESC $HUB 2>/dev/null; wait $ESC $HUB 2>/dev/null; rm -f $SOCK
  "$BIN/kipc_hub" 2>>$T/hub.log & HUB=$!
  i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
  "$BIN/dsi_escuta" $SOCK >> $T/escuta.log 2>&1 & ESC=$!
}

hub_novo
"$BIN/veiculostub" -e $EST $SOCK $T/esqp.txt $T/perfilp.txt > $T/stube.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log
if kill -0 $STUB 2>/dev/null && [ ! -e $EST ]; then ok sem_nada_gravado_nao_ha_estado; else nok sem_nada_gravado_nao_ha_estado "o stub não subiu, ou o arquivo já existe: $(tail -n 1 $T/stube.log | cut -c1-200)"; fi
tem estado_que_ainda_nao_existe_sai_no_resumo 'estado: nada guardado ainda em .*estado\.txt$' $T/stube.log
hmi 700 0 9999 "08 10 08 02 08 12 08 14 08 0a"; sleep 0.3      # assina a unidade (8), o simples (1), o fuso (9), o texto (10) e a hora (5)
hmi 700 0 1001 "08 06"; sleep 0.3                # setUnidade(3): grava a unidade e, pela regra do perfil, o simples
igual gravar_escreve_o_estado "$(semc $EST)" '[SvcTeste.updateUnidade]
unidade = 3
[SvcTeste.updateSimples]
valor = 3
fim = 2'
tem gravacao_do_estado_fica_registrada 'pedido atendido: SvcTeste\.setUnidade .*estado guardado' $T/stube.log
i1=$(inode $EST)
hmi 700 0 1001 "08 08"; sleep 0.3                # setUnidade(4)
igual gravar_de_novo_troca_o_valor_no_estado "$(semc $EST)" '[SvcTeste.updateUnidade]
unidade = 4
[SvcTeste.updateSimples]
valor = 4
fim = 2'
i2=$(inode $EST)
if [ -n "$i1" ] && [ -n "$i2" ] && [ "$i1" != "$i2" ]; then ok estado_e_gravado_por_troca_de_nome; else nok estado_e_gravado_por_troca_de_nome "o arquivo era o '$i1' e é o '$i2': não houve troca"; fi
igual gravacao_do_estado_nao_deixa_sobra "$(ls -A $ED | tr '\n' ' ')" "estado.txt "
hmi 700 0 1001 "08 08"; sleep 0.3                # o mesmo valor: a HMI recebe a resposta, mas não há o que regravar
if [ -n "$i2" ] && [ "$(inode $EST)" = "$i2" ]; then ok gravar_o_mesmo_valor_nao_regrava_o_estado; else nok gravar_o_mesmo_valor_nao_regrava_o_estado "o arquivo era o '$i2' e é o '$(inode $EST)'"; fi
hmi 700 0 1001 "-"; sleep 0.3                    # setUnidade(0): o zero fica escrito, para valer sobre o 2 do perfil
no_estado zero_gravado_fica_no_estado '^unidade = 0$'
antes=$(cksum < $EST 2>/dev/null)
hmi 700 0 1011 "08 0a"; sleep 0.3                # setIgnorada(5): o perfil manda ignorar
hmi 710 0 1001 "08 06"; sleep 0.3                # setFaixa(3), num serviço que não está em "grava"
hmi 700 0 1002 "08 32"; sleep 0.3                # setHora(25, 0, 0): hora que não existe
if [ -f $EST ] && [ "$(cksum < $EST)" = "$antes" ]; then ok o_que_nao_e_gravado_nao_muda_o_estado; else nok o_que_nao_e_gravado_nao_muda_o_estado "$(semc $EST | tr '\n' '|' | cut -c1-200)"; fi
hmi 700 0 1004 "0d 00 00 00 c0"; sleep 0.3       # setFuso(-2.0)
no_estado numero_decimal_fica_no_estado '^fuso = -2$'
hmi 700 0 1008 "0a 06 61 22 62 5c 0a 63 10 01"; sleep 0.3      # setTexto: a"b\, quebra de linha, c; ligado
no_estado texto_fica_no_estado_numa_linha_so '^caixa\.texto = "a\\"b\\\\\\nc"$'
H=$(hoje)
a=$(( $(date -u +%s) - 3 * 3600 ))
hmi 700 0 1002 "08 0e 10 10"; sleep 0.3          # setHora(7, 8, 0)
ac=$(sed -n 's/^acerto = //p' $EST 2>/dev/null)
esp=$(( 7 * 3600 + 8 * 60 - a % 86400 ))
if [ -n "$ac" ] && [ $((ac - esp)) -ge -2 ] && [ $((ac - esp)) -le 2 ]; then ok acertar_o_relogio_guarda_a_diferenca_e_nao_a_hora
else nok acertar_o_relogio_guarda_a_diferenca_e_nao_a_hora "acerto = '$ac'; esperava perto de $esp s (07:08:00 menos a hora de agora no fuso)"; fi
fora_do_estado relogio_acertado_nao_vira_valor_fixo_no_estado '^(h|m|s) = '
TUDO='acerto = N
[SvcTeste.updateUnidade]
unidade = 0
[SvcTeste.updateSimples]
valor = 0
[SvcTeste.updateFuso]
fuso = -2
[SvcTeste.updateTexto]
caixa.texto = "a\"b\\\nc"
caixa.ligado = 1
fim = 5'
igual estado_com_tudo_o_que_foi_gravado "$(semc $EST | sed 's/^acerto = .*/acerto = N/')" "$TUDO"
kill -9 $STUB 2>/dev/null; wait $STUB 2>/dev/null         # a central morre sem aviso: o que foi gravado já está no arquivo
"$BIN/veiculostub" -e $EST -n $T/esqp.txt $T/perfilp.txt > $T/eok.log 2>&1; rc=$?
if [ $rc -eq 0 ] && grep -qE 'estado: 5 valor\(es\) e acerto de [+-][0-9]+ s no relógio, de .*estado\.txt$' $T/eok.log; then ok estado_guardado_passa_na_conferencia
else nok estado_guardado_passa_na_conferencia "rc=$rc: $(tail -n 1 $T/eok.log | cut -c1-300)"; fi

# -- a partida seguinte: o que ficou no arquivo vale sobre o perfil, e o relógio seguiu andando
i3=$(inode $EST)
hub_novo
"$BIN/veiculostub" -e $EST $SOCK $T/esqp.txt $T/perfilp.txt > $T/stube2.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log
hmi 700 0 9999 "08 10 08 12 08 14 08 0a"; sleep 0.4      # assina a unidade (8), o fuso (9), o texto (10) e a hora (5)
tem partida_aplica_o_valor_guardado "^$E inst=0 api=9 pb=0d 00 00 00 c0 10 02\$" $T/escuta.log
tem zero_guardado_vale_sobre_o_perfil "^$E inst=0 api=8 pb=10 02\$" $T/escuta.log          # o perfil diz 2
tem partida_aplica_o_texto_guardado "^$E inst=0 api=10 pb=0a 0a 0a 06 61 22 62 5c 0a 63 10 01 10 02\$" $T/escuta.log
tem partida_aplica_o_acerto_do_relogio "^$E inst=0 api=5 pb=08 0e 10 1[02] (18 [0-9a-f]{2} )?$H 38 02\$" $T/escuta.log
tem estado_aplicado_sai_no_resumo 'estado: 5 valor\(es\) e acerto de [+-][0-9]+ s no relógio, de .*estado\.txt$' $T/stube2.log
if [ -n "$i3" ] && [ "$(inode $EST)" = "$i3" ]; then ok partida_nao_regrava_o_estado; else nok partida_nao_regrava_o_estado "o arquivo era o '$i3' e é o '$(inode $EST)'"; fi
hmi 700 0 1001 "08 0c"; sleep 0.3                # setUnidade(6) na segunda sessão: soma ao que veio do arquivo
igual o_que_veio_do_estado_continua_guardado "$(semc $EST)" "$(printf '%s\n' "$TUDO" | sed "s/^acerto = N\$/acerto = $ac/; s/^\(unidade\|valor\) = 0\$/\1 = 6/")"
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null
# a terceira partida, sem barramento: -m mostra o que seria mandado, já com o estado
"$BIN/veiculostub" -e $EST -m $T/esqp.txt $T/perfilp.txt > $T/mostra-e.log 2>/dev/null
tem modo_mostra_aplica_o_estado "^$E inst=0 api=8 pb=08 0c 10 02\$" $T/mostra-e.log

# -- arquivo de estado com erro: a conferência passa (a partida acontece), nada dele vale e o
# aviso diz o arquivo e a linha. Cada arquivo começa com duas linhas certas: se o estado fosse
# aplicado pela metade, a unidade sairia 7, e não a 2 do perfil.
"$BIN/veiculostub" -m $T/esqp.txt $T/perfilp.txt 2>/dev/null | grep -v ' api=5 ' > $T/so-perfil.txt     # sem a hora, que muda
estado_ruim() {   # estado_ruim <nome> <linha do erro> <trecho do motivo> <conteúdo>
  printf '%b' "$4" > $T/ruim.txt
  "$BIN/veiculostub" -e $T/ruim.txt -n $T/esqp.txt $T/perfilp.txt > $T/ruim.log 2>&1; rc=$?
  "$BIN/veiculostub" -e $T/ruim.txt -m $T/esqp.txt $T/perfilp.txt 2>/dev/null | grep -v ' api=5 ' > $T/com-ruim.txt
  if [ $rc -ne 0 ]; then nok "$1" "o estado impediu a conferência (rc=$rc): $(tail -n 1 $T/ruim.log | cut -c1-200)"
  elif ! [ -s $T/com-ruim.txt ] || ! cmp -s $T/so-perfil.txt $T/com-ruim.txt; then nok "$1" "parte do estado foi aplicada: $(diff $T/so-perfil.txt $T/com-ruim.txt | head -n 3 | tr '\n' ' ')"
  elif ! grep -qE "estado: RECUSADO \(.*ruim\.txt, linha $2: .*$3" $T/ruim.log; then nok "$1" "aviso sem a linha $2 ou sem /$3/: $(tail -n 1 $T/ruim.log | sed 's/.*; estado: //' | cut -c1-200)"
  else ok "$1"; fi
}
BOM='[SvcTeste.updateUnidade]\nunidade = 7\n'
estado_ruim estado_com_linha_sem_igual_e_recusado          3 "falta o '='"        "${BOM}qualquer coisa\nfim = 1\n"
estado_ruim estado_com_servico_desconhecido_e_recusado     3 'serviço'            "${BOM}[SvcNada.updateSimples]\nvalor = 1\nfim = 2\n"
estado_ruim estado_com_evento_desconhecido_e_recusado      3 'evento'             "${BOM}[SvcTeste.updateNada]\nvalor = 1\nfim = 2\n"
estado_ruim estado_com_campo_desconhecido_e_recusado       3 'campo desconhecido' "${BOM}nada = 1\nfim = 2\n"
estado_ruim estado_com_valor_que_nao_e_numero_e_recusado   3 'número inteiro'     "${BOM}unidade = abc\nfim = 2\n"
estado_ruim estado_com_numero_grande_demais_e_recusado     3 '32 bits'            "${BOM}unidade = 3000000000\nfim = 2\n"
estado_ruim estado_com_texto_em_campo_inteiro_e_recusado   3 'aspas'              "${BOM}unidade = \"x\"\nfim = 2\n"
estado_ruim estado_com_marcador_e_recusado                 3 'marcador'           "${BOM}unidade = @hora\nfim = 2\n"
estado_ruim estado_com_ajuste_de_perfil_e_recusado         1 'só tem o acerto'    "grava = SvcMidia\n${BOM}fim = 1\n"
estado_ruim estado_com_regra_e_recusado                    3 'regra'              "${BOM}[SvcTeste.setUnidade -> updateSimples]\nvalor = unidade\nfim = 1\n"
estado_ruim estado_com_acerto_que_nao_e_numero_e_recusado  1 'acerto'             "acerto = cedo\n${BOM}fim = 1\n"
estado_ruim estado_com_acerto_fora_do_calendario_e_recusado 1 '1970 a 2099'       "acerto = 99999999999\n${BOM}fim = 1\n"
estado_ruim estado_sem_a_linha_fim_e_recusado              2 'cortado'            "${BOM}"
estado_ruim estado_com_fim_que_nao_bate_e_recusado         3 'fim'                "${BOM}fim = 5\n"
estado_ruim estado_com_linha_depois_do_fim_e_recusado      4 'depois do fim'      "${BOM}fim = 1\nunidade = 9\n"
estado_ruim estado_cortado_no_meio_de_uma_linha_e_recusado 3 'seção'              "${BOM}[SvcTeste.updateFu"
estado_ruim estado_vazio_e_recusado                        0 'cortado'            ""

# -- no barramento: o estado recusado fica de lado, a central nasce do perfil e volta a guardar
printf "${BOM}lixo\n" > $EST; rm -f $EST.recusado
hub_novo
"$BIN/veiculostub" -e $EST $SOCK $T/esqp.txt $T/perfilp.txt > $T/stube3.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log
if [ ! -e $EST ] && grep -q 'lixo' $EST.recusado 2>/dev/null; then ok estado_recusado_fica_de_lado; else nok estado_recusado_fica_de_lado "na pasta: $(ls -A $ED | tr '\n' ' ')"; fi
hmi 700 0 9999 "08 10"; sleep 0.3
tem estado_recusado_nao_impede_a_partida "^$E inst=0 api=8 pb=08 04 10 02\$" $T/escuta.log      # a unidade 2 do perfil, e não a 7 do arquivo
tem estado_recusado_fica_registrado 'estado: RECUSADO \(.*estado\.txt, linha 3: ' $T/stube3.log
hmi 700 0 1001 "08 06"; sleep 0.3                # setUnidade(3)
no_estado depois_de_recusar_volta_a_guardar '^unidade = 3$'
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# -- sem onde gravar (a pasta não existe): o pedido é atendido do mesmo jeito, e o registro avisa
hub_novo
"$BIN/veiculostub" -e $T/nao-existe/estado.txt $SOCK $T/esqp.txt $T/perfilp.txt > $T/stube4.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log
hmi 700 0 9999 "08 10"; sleep 0.2
hmi 700 0 1001 "08 06"; sleep 0.3                # setUnidade(3)
tem sem_onde_guardar_o_pedido_e_atendido "^$E inst=0 api=8 pb=08 06 10 02\$" $T/escuta.log
tem sem_onde_guardar_fica_registrado 'não consegui guardar o estado' $T/stube4.log
if kill -0 $STUB 2>/dev/null; then ok sem_onde_guardar_o_stub_continua_de_pe; else nok sem_onde_guardar_o_stub_continua_de_pe "morreu: $(tail -n 1 $T/stube4.log | cut -c1-200)"; fi
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# -- o perfil mudou de uma sessão para a outra: valor guardado para um campo que o perfil agora
# enche com marcador (@hora…) não congela o relógio. É descartado, com aviso, e o resto vale.
hf=$(( ( $(date -u +%-H) + 26 ) % 24 )); [ $hf -eq 0 ] && hf=1      # uma hora que não é a de agora no fuso do perfil
printf '[SvcTeste.updateHora]\nh = %s\n[SvcTeste.updateUnidade]\nunidade = 7\nfim = 2\n' $hf > $T/marc.txt
"$BIN/veiculostub" -e $T/marc.txt -m $T/esqp.txt $T/perfilp.txt > $T/marc.log 2>$T/marc.err
tem estado_com_campo_do_relogio_aplica_o_resto "^$E inst=0 api=8 pb=08 0e 10 02\$" $T/marc.log
if grep -qE "^$E inst=0 api=5 pb=" $T/marc.log && ! grep -qE "^$E inst=0 api=5 pb=08 $(zz $hf) " $T/marc.log; then ok estado_com_campo_do_relogio_nao_congela_a_hora
else nok estado_com_campo_do_relogio_nao_congela_a_hora "a hora saiu $hf, a do arquivo: $(grep -E "^$E inst=0 api=5 " $T/marc.log | head -n 1)"; fi
tem estado_com_campo_do_relogio_avisa 'estado: 1 valor\(es\) \(1 descartado' $T/marc.err

# ---- as centrais de teste nascem iguais, salvo pedido (driver/m8_central.sh)
# Só o modo ao vivo guarda o estado por padrão, junto da memória da tela. Numa central de teste,
# MEMORIA ou VEICULO_ESTADO (caminhos de dentro do container, em /live/memoria) fazem a pasta
# <pasta>/memoria ir montada lá; sem elas, nada muda. Roda uma CÓPIA do roteiro, com um "docker"
# de mentira que só anota a chamada: o docker de verdade não é chamado.
DRV=$(cd "$(dirname "$0")/.." && pwd)
BM=$T/bancada; mkdir -p $BM/driver $T/bin; cp "$DRV/m8_central.sh" $BM/driver/
cat > $T/bin/docker <<'EOF'
#!/bin/sh
if [ "$1" = run ]; then for a in "$@"; do printf '%s\n' "$a"; done > "$DOCKER_VIU"; fi
exit 0
EOF
chmod +x $T/bin/docker
central() {   # central [VARIÁVEL=valor…]: sobe a cópia com essas variáveis; a chamada do docker fica em $T/docker.args
  rm -rf $BM/c $T/docker.args
  ( unset MEMORIA VEICULO_ESTADO; export DOCKER_VIU=$T/docker.args PATH="$T/bin:$PATH"
    env "$@" sh $BM/driver/m8_central.sh sobe x $BM/c 5 >/dev/null 2>&1 )
}
arg() { grep -qxF -- "$1" $T/docker.args 2>/dev/null; }
central
if [ -s $T/docker.args ] && ! grep -qi 'memoria\|VEICULO_ESTADO' $T/docker.args && [ ! -e $BM/c/memoria ]; then ok central_de_teste_sem_pedido_nasce_como_antes
else nok central_de_teste_sem_pedido_nasce_como_antes "docker recebeu: $(grep -i 'memoria\|estado' $T/docker.args 2>&1 | tr '\n' ' '); pasta: $(ls -d $BM/c/memoria 2>&1)"; fi
central VEICULO_ESTADO=/live/memoria/veiculo/estado.txt
if arg VEICULO_ESTADO && arg "$BM/c/memoria:/live/memoria" && [ -d $BM/c/memoria ] && ! arg MEMORIA; then ok central_de_teste_guarda_o_estado_do_carro_se_pedido
else nok central_de_teste_guarda_o_estado_do_carro_se_pedido "docker recebeu: $(grep -i 'memoria\|estado' $T/docker.args 2>&1 | tr '\n' ' ')"; fi
central MEMORIA=/live/memoria
if arg MEMORIA && arg "$BM/c/memoria:/live/memoria" && [ -d $BM/c/memoria ]; then ok central_de_teste_com_as_duas_memorias_se_pedido
else nok central_de_teste_com_as_duas_memorias_se_pedido "docker recebeu: $(grep -i 'memoria\|estado' $T/docker.args 2>&1 | tr '\n' ' ')"; fi

# ---- o esquema de verdade (gerado das classes do firmware) e os vetores de prova
G=$VEI/gerado
if [ -f $G/esquema.txt ]; then
  "$BIN/veiculostub" -n $G/esquema.txt > $T/real.log 2>&1; rc=$?
  if [ $rc -eq 0 ] && grep -q '16 serviços' $T/real.log; then ok esquema_do_firmware_carrega
  else nok esquema_do_firmware_carrega "rc=$rc: $(tail -n 1 $T/real.log)"; fi
  # O perfil de prova dá valor a toda folha de todo evento; os bytes esperados são os que as
  # classes do firmware serializaram. O stub tem de produzir os mesmos, mensagem por mensagem.
  "$BIN/veiculostub" -m $G/esquema.txt $G/vetores-perfil.txt > $T/vetores.log 2>$T/vetores.err; rc=$?
  total=$(grep -c '^canal=' $G/vetores-esperado.txt)
  grep '^canal=' $G/vetores-esperado.txt | sort > $T/esperado.ord
  grep '^canal=' $T/vetores.log | sort > $T/obtido.ord
  iguais=$(comm -12 $T/esperado.ord $T/obtido.ord | wc -l)
  if [ $rc -eq 0 ] && [ "$total" -gt 1000 ] && [ "$iguais" = "$total" ]; then ok "vetores_do_firmware ($iguais de $total mensagens iguais, byte a byte)"
  else nok vetores_do_firmware "rc=$rc; $iguais de $total iguais; primeira diferença: $(comm -23 $T/esperado.ord $T/obtido.ord | head -n 1 | cut -c1-160) / erro: $(tail -n 1 $T/vetores.err)"; fi

  # pelo barramento, com o esquema de verdade: assinar os 24 atributos do serviço de relógio
  # e conferir que cada atualização chega com os bytes do vetor
  : > $T/escuta.log
  "$BIN/veiculostub" $SOCK $G/esquema.txt $G/vetores-perfil.txt > $T/stub2.log 2>&1 & STUB=$!
  sleep 1.2
  pb=""; a=1; while [ $a -le 24 ]; do pb="$pb 08 $(printf '%02x' $((a * 2)))"; a=$((a+1)); done
  hmi 813 0 9999 "$pb"; sleep 0.6
  grep '^canal=0x10 if=814 inst=0 api=[0-9]* ' $G/vetores-esperado.txt | grep -v ' api=2000 ' | sort > $T/esp814.ord
  grep '^canal=0x10 if=814 ' $T/escuta.log | sort > $T/obt814.ord
  n=$(wc -l < $T/esp814.ord); iguais=$(comm -12 $T/esp814.ord $T/obt814.ord | wc -l)
  if [ "$n" = 24 ] && [ "$iguais" = 24 ]; then ok relogio_do_firmware_pelo_barramento
  else nok relogio_do_firmware_pelo_barramento "$iguais de $n atualizações iguais às do vetor"; fi
  kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

  # os pedidos de verdade: o esquema do firmware, os perfis da pasta e os bytes que a HMI mandou
  # na central de teste de 11/10/2026 (rodada base1), ao acertar a hora, a data, o fuso e as
  # unidades pela tela e ao pedir, na partida, as três listas de motivos do start-stop
  "$BIN/veiculostub" -n $G/esquema.txt > $T/real.log 2>&1
  if grep -qE '\([1-9][0-9]* com resposta, [1-9][0-9]* que gravam\)' $T/real.log; then ok esquema_do_firmware_traz_respostas_e_gravacoes
  else nok esquema_do_firmware_traz_respostas_e_gravacoes "$(tail -n 1 $T/real.log)"; fi
  : > $T/escuta.log
  "$BIN/veiculostub" $SOCK $G/esquema.txt $VEI/carro-parado.txt $VEI/jetta-gli-2021.txt $VEI/jetta-gli-2021-deducoes.txt > $T/stub3.log 2>&1 & STUB=$!
  sleep 1.2
  U='canal=0x10 if=814 inst=0'
  hmi 813 0 9999 "08 04 08 06 08 0e 08 16 08 18 08 1a"; sleep 0.4      # data (2), hora (3), fuso (7), idioma (11), temperatura (12), distância (13)
  hmi 813 0 1001 "08 2c 10 6a"; sleep 0.3                  # setClockTime(22, 53, 0)
  tem hora_acertada_pela_tela "^$U api=3 pb=0a 09 08 2c 10 6a 25 00 00 40 c0 10 02\$" $T/escuta.log
  hmi 813 0 1000 "0a 06 08 34 10 14 18 18"; sleep 0.3      # setClockDate(26, 10, 12)
  tem data_acertada_pela_tela "^$U api=2 pb=0a 06 08 34 10 14 18 18 10 02\$" $T/escuta.log
  hmi 813 0 1010 "08 02"; sleep 0.3                        # setTemperatureUnit(1): °F
  tem temperatura_trocada_pela_tela "^$U api=12 pb=08 02 10 02\$" $T/escuta.log
  hmi 813 0 1011 "08 02"; sleep 0.3                        # setDistanceUnit(1): milhas
  tem distancia_trocada_pela_tela "^$U api=13 pb=08 02 10 02\$" $T/escuta.log
  hmi 813 0 1005 "0d 00 00 00 c0"; sleep 0.3               # setClockTimeZoneOffset(-2.0)
  tem fuso_trocado_pela_tela "^$U api=7 pb=0d 00 00 00 c0 10 02\$" $T/escuta.log
  # a tela tira o fuso de dentro da mensagem da hora (AslTargetSystemDate.updateClockTime), não do atributo 7
  tem fuso_trocado_aparece_na_mensagem_da_hora "^$U api=3 pb=0a 0[9b] 08 2c 10 6a (18 [0-9a-f]{2} )?25 00 00 00 c0 10 02\$" $T/escuta.log
  # setMenuLanguage(1): a HMI manda na partida. Atendê-lo põe a HMI em laço em parte das partidas
  # (rodada n4), então o perfil da pasta manda ignorar: o idioma do painel fica o do perfil (26)
  hmi 813 0 1007 "08 02"; sleep 0.3
  naotem idioma_do_painel_fica_o_do_perfil "^$U api=11 pb=08 02 10 02\$" $T/escuta.log
  tem idioma_do_painel_ignorado_fica_registrado 'pedido sem resposta: DSICarTimeUnitsLanguage\.setMenuLanguage .*o perfil manda ignorar' $T/stub3.log
  V='canal=0x10 if=818 inst=0'
  hmi 817 0 1003 "0a 04 18 06 20 02"; hmi 817 0 1004 "0a 04 18 06 20 02"; hmi 817 0 1005 "0a 04 18 06 20 02"; sleep 0.4
  tem start_stop_motivos_de_nao_desligar_lista_vazia "^$V api=2002 pb=0a 02 20 02\$" $T/escuta.log
  tem start_stop_motivos_de_religar_lista_vazia "^$V api=2003 pb=0a 02 20 02\$" $T/escuta.log
  tem start_stop_motivos_de_nao_religar_lista_vazia "^$V api=2004 pb=0a 02 20 02\$" $T/escuta.log
  hmi 804 0 1060 "08 01"; sleep 0.2                        # setHMIIsReady: aviso da HMI, sem resposta
  tem aviso_da_hmi_sem_prazo_de_resposta 'pedido sem resposta: DSICarComfort\.setHMIIsReady .*não marca prazo de resposta' $T/stub3.log
  kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

  # o estado com o esquema e os perfis de verdade: a hora acertada, as unidades e o fuso que a
  # tela gravou (os mesmos bytes da rodada base1) ficam no arquivo e voltam na partida seguinte.
  # A hora não vai como número: o que fica é o acerto, e o relógio segue andando.
  ER=$T/estado-real.txt; rm -f $ER
  hub_novo
  "$BIN/veiculostub" -e $ER $SOCK $G/esquema.txt $VEI/carro-parado.txt $VEI/jetta-gli-2021.txt $VEI/jetta-gli-2021-deducoes.txt > $T/stub4.log 2>&1 & STUB=$!
  sleep 1.2
  hmi 813 0 1001 "08 2c 10 6a"; sleep 0.2                  # setClockTime(22, 53, 0)
  hmi 813 0 1010 "08 02"; sleep 0.2                        # setTemperatureUnit(1): °F
  hmi 813 0 1011 "08 02"; sleep 0.2                        # setDistanceUnit(1): milhas
  hmi 813 0 1005 "0d 00 00 00 c0"; sleep 0.2               # setClockTimeZoneOffset(-2.0)
  hmi 813 0 1007 "08 02"; sleep 0.3                        # setMenuLanguage(1): ignorado pelo perfil, não vai para o estado
  kill -9 $STUB 2>/dev/null; wait $STUB 2>/dev/null
  igual estado_com_o_esquema_de_verdade "$(semc $ER | sed 's/^acerto = .*/acerto = N/')" 'acerto = N
[DSICarTimeUnitsLanguage.updateTemperatureUnit]
temperatureUnit = 1
[DSICarTimeUnitsLanguage.updateDistanceUnit]
distanceUnit = 1
[DSICarTimeUnitsLanguage.updateClockTimeZoneOffset]
clockTimeZoneOffset = -2
[DSICarTimeUnitsLanguage.updateClockTime]
time.timeZone = -2
fim = 4'
  hub_novo
  : > $T/escuta.log
  "$BIN/veiculostub" -e $ER $SOCK $G/esquema.txt $VEI/carro-parado.txt $VEI/jetta-gli-2021.txt $VEI/jetta-gli-2021-deducoes.txt > $T/stub5.log 2>&1 & STUB=$!
  sleep 1.2
  hmi 813 0 9999 "08 06 08 0e 08 18 08 1a"; sleep 0.4      # hora (3), fuso (7), temperatura (12), distância (13)
  tem partida_seguinte_com_a_hora_acertada "^$U api=3 pb=0a 0[9b] 08 2c 10 6[ac] (18 [0-9a-f]{2} )?25 00 00 00 c0 10 02\$" $T/escuta.log
  tem partida_seguinte_com_a_temperatura_escolhida "^$U api=12 pb=08 02 10 02\$" $T/escuta.log
  tem partida_seguinte_com_a_distancia_escolhida "^$U api=13 pb=08 02 10 02\$" $T/escuta.log
  tem partida_seguinte_com_o_fuso_escolhido "^$U api=7 pb=0d 00 00 00 c0 10 02\$" $T/escuta.log
  kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

  # os perfis que a bancada usa têm de passar na conferência com o esquema de verdade
  for p in $VEI/*.txt; do
    [ -f "$p" ] || continue
    "$BIN/veiculostub" -n $G/esquema.txt "$p" > $T/p.log 2>&1
    if [ $? -eq 0 ]; then ok "perfil_da_pasta_confere: $(basename $p)"; else nok "perfil_da_pasta_confere: $(basename $p)" "$(tail -n 1 $T/p.log)"; fi
  done
else
  nok esquema_do_firmware_carrega "não achei $G/esquema.txt"
fi

# ------------------------------------------------------------------ campos que acompanham outro campo
# (12/10/2026, item 2 do roteiro do simulador) No carro, distância, autonomia e consumo vêm do
# painel com a unidade dentro da mensagem: quando a tela troca a unidade, o painel passa a mandar
# outro número e outra unidade. No perfil isso se escreve com dois marcadores, avaliados a cada
# envio, que leem o que outro evento do carro de mentira guarda naquela hora:
#   @de Serviço.evento.campo                  o valor de agora desse campo
#   @se Serviço.evento.campo V:X … senao Y    X do par cujo V é o valor de agora; Y nos outros casos
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null
cat > $T/esqu.txt <<'FIM'
S SvcUni 16 720 721 uni
A SvcUni 1 ATTR_UNIDADE
A SvcUni 2 ATTR_TEXTO
E SvcUni 1 1 1 updateUnidade UniMsg.updateUnidade
E SvcUni 2 2 2 updateTexto UniMsg.updateTexto
P SvcUni 9999 setNotification UniReq.setNotification
P SvcUni 1001 setUnidade UniReq.setUnidade
S SvcPainel 16 730 731 painel
A SvcPainel 1 ATTR_ALCANCE
A SvcPainel 2 ATTR_MEDIA
E SvcPainel 1 1 1 updateAlcance UniMsg.updateAlcance
E SvcPainel 2 2 2 updateMedia UniMsg.updateMedia
P SvcPainel 9999 setNotification UniReq.setNotification
G SvcUni 1001 1 posicao unidade unidade
M UniMsg.updateUnidade
C 1 unidade sint32 um
C 2 validFlag sint32 um
M UniMsg.updateTexto
C 1 texto string um
C 2 validFlag sint32 um
M UniMsg.updateAlcance
C 1 alcance msg um UniMsg.Alcance
C 2 validFlag sint32 um
M UniMsg.Alcance
C 1 valor sint32 um
C 2 unidade sint32 um
M UniMsg.updateMedia
C 1 media float um
C 2 unidade sint32 um
C 3 validFlag sint32 um
M UniReq.setNotification
C 1 attributes sint32 rep
M UniReq.setUnidade
C 1 unidade sint32 um
FIM
cat > $T/uni.txt <<'FIM'
grava = SvcUni
[SvcUni.updateUnidade]
unidade = 0
[SvcPainel.updateAlcance]
alcance.valor = @se SvcUni.updateUnidade.unidade 1:249 senao 400     # 400 na unidade 0; 249 na unidade 1
alcance.unidade = @de SvcUni.updateUnidade.unidade
[SvcPainel.updateMedia]
unidade = @de SvcUni.updateUnidade.unidade
FIM
printf '[SvcUni.updateUnidade]\nunidade = 1\n' > $T/uni1.txt
printf '[SvcUni.updateUnidade]\nunidade = 7\n' > $T/uni7.txt
PA="canal=0x10 if=731"
"$BIN/veiculostub" -n $T/esqu.txt $T/uni.txt > $T/uni-n.log 2>&1; rc=$?
igual perfil_que_acompanha_confere "$rc" 0
"$BIN/veiculostub" -m $T/esqu.txt $T/uni.txt > $T/uni-m0.log 2>&1
tem se_sem_par_que_sirva_vale_o_senao "^$PA inst=0 api=1 pb=0a 03 08 a0 06 10 02\$" $T/uni-m0.log
tem de_com_zero_nao_poe_nada_no_fio "^$PA inst=0 api=2 pb=18 02\$" $T/uni-m0.log
"$BIN/veiculostub" -m $T/esqu.txt $T/uni.txt $T/uni1.txt > $T/uni-m1.log 2>&1
tem se_escolhe_pelo_valor_de_agora "^$PA inst=0 api=1 pb=0a 05 08 f2 03 10 02 10 02\$" $T/uni-m1.log
tem de_copia_o_valor_de_agora "^$PA inst=0 api=2 pb=10 02 18 02\$" $T/uni-m1.log
"$BIN/veiculostub" -m $T/esqu.txt $T/uni.txt $T/uni7.txt > $T/uni-m7.log 2>&1
tem se_com_valor_fora_dos_pares_vale_o_senao "^$PA inst=0 api=1 pb=0a 05 08 a0 06 10 0e 10 02\$" $T/uni-m7.log
uni_ruim() {      # uni_ruim <nome> <trecho do motivo> <linha 3 do perfil, dentro de [SvcPainel.updateAlcance]>
  printf '[SvcPainel.updateAlcance]\nalcance.valor = 1\n%s\n' "$3" > $T/uni-ruim.txt
  "$BIN/veiculostub" -n $T/esqu.txt $T/uni-ruim.txt > $T/uni-ruim.log 2>&1; rc=$?
  if [ $rc -ne 0 ] && grep -qE -- "uni-ruim.txt, linha 3: .*$2" $T/uni-ruim.log; then ok "$1"; else nok "$1" "saiu com $rc: $(tail -n 1 $T/uni-ruim.log | cut -c1-160)"; fi
}
uni_ruim de_servico_que_nao_existe 'serviço' 'alcance.unidade = @de SvcNada.updateUnidade.unidade'
uni_ruim de_evento_que_nao_existe 'evento' 'alcance.unidade = @de SvcUni.updateNada.unidade'
uni_ruim de_campo_que_nao_existe 'campo' 'alcance.unidade = @de SvcUni.updateUnidade.nada'
uni_ruim de_campo_que_nao_e_inteiro 'inteiro' 'alcance.unidade = @de SvcUni.updateTexto.texto'
uni_ruim de_sem_o_campo 'Serviço\.evento\.campo' 'alcance.unidade = @de SvcUni.updateUnidade'
uni_ruim de_com_sobra 'sobra' 'alcance.unidade = @de SvcUni.updateUnidade.unidade 3'
uni_ruim se_sem_senao 'senao' 'alcance.valor = @se SvcUni.updateUnidade.unidade 1:249'
uni_ruim se_com_par_mal_escrito 'V:X' 'alcance.valor = @se SvcUni.updateUnidade.unidade 1=249 senao 400'
uni_ruim se_com_valor_que_nao_e_numero 'V:X' 'alcance.valor = @se SvcUni.updateUnidade.unidade 1:muito senao 400'
uni_ruim se_com_sobra_depois_do_senao 'sobra' 'alcance.valor = @se SvcUni.updateUnidade.unidade 1:249 senao 400 500'
printf '[SvcPainel.updateMedia]\nunidade = 1\nmedia = @de SvcUni.updateUnidade.unidade\n' > $T/uni-ruim.txt
"$BIN/veiculostub" -n $T/esqu.txt $T/uni-ruim.txt > $T/uni-ruim.log 2>&1; rc=$?
if [ $rc -ne 0 ] && grep -q 'uni-ruim.txt, linha 3: .*só vale em campo inteiro' $T/uni-ruim.log; then ok acompanhar_so_em_campo_inteiro; else nok acompanhar_so_em_campo_inteiro "saiu com $rc: $(tail -n 1 $T/uni-ruim.log | cut -c1-160)"; fi

# no barramento: a tela troca a unidade e os outros eventos acompanham, uma vez só
hub_novo
"$BIN/veiculostub" $SOCK $T/esqu.txt $T/uni.txt > $T/stubu.log 2>&1 & STUB=$!
sleep 1.2
: > $T/escuta.log
hmi 730 0 9999 "08 02 08 04"; hmi 720 0 9999 "08 02"; sleep 0.4
tem assinatura_recebe_o_valor_da_unidade_de_agora "^$PA inst=0 api=1 pb=0a 03 08 a0 06 10 02\$" $T/escuta.log
sleep 2.3
igual sem_mudanca_nao_repete "$(conta "^$PA " $T/escuta.log)" 2
: > $T/escuta.log
hmi 720 0 1001 "08 02"; sleep 2.3                # setUnidade(1)
tem troca_de_unidade_muda_o_numero "^$PA inst=0 api=1 pb=0a 05 08 f2 03 10 02 10 02\$" $T/escuta.log
tem troca_de_unidade_muda_a_unidade_dos_outros "^$PA inst=0 api=2 pb=10 02 18 02\$" $T/escuta.log
igual quem_acompanha_e_avisado_uma_vez "$(conta "^$PA " $T/escuta.log)" 2
: > $T/escuta.log
hmi 720 0 1001 "08 00"; sleep 2.3                # setUnidade(0)
tem voltar_a_unidade_volta_o_numero "^$PA inst=0 api=1 pb=0a 03 08 a0 06 10 02\$" $T/escuta.log
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# com o arquivo de estado: guarda-se a unidade, e não o que a acompanha; a partida seguinte nasce certa
UEST=$T/uni-estado.txt; rm -f $UEST
hub_novo
"$BIN/veiculostub" -e $UEST $SOCK $T/esqu.txt $T/uni.txt > $T/stubu2.log 2>&1 & STUB=$!
sleep 1.2
hmi 720 0 9999 "08 02"; hmi 720 0 1001 "08 02"; sleep 0.6
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null
if grep -q '^unidade = 1$' $UEST 2>/dev/null; then ok estado_guarda_a_unidade; else nok estado_guarda_a_unidade "$(grep -v '^#' $UEST 2>/dev/null | tr '\n' '|')"; fi
if grep -q 'alcance\|SvcPainel' $UEST 2>/dev/null; then nok estado_nao_guarda_o_que_acompanha "$(grep -v '^#' $UEST | tr '\n' '|')"; else ok estado_nao_guarda_o_que_acompanha; fi
hub_novo
: > $T/escuta.log
"$BIN/veiculostub" -e $UEST $SOCK $T/esqu.txt $T/uni.txt > $T/stubu3.log 2>&1 & STUB=$!
sleep 1.2
hmi 730 0 9999 "08 02"; sleep 0.4
tem partida_seguinte_nasce_na_unidade_guardada "^$PA inst=0 api=1 pb=0a 05 08 f2 03 10 02 10 02\$" $T/escuta.log
naotem partida_seguinte_nao_passa_pela_unidade_do_perfil "^$PA inst=0 api=1 pb=0a 03 08 a0 06 10 02\$" $T/escuta.log
kill $STUB 2>/dev/null; wait $STUB 2>/dev/null

# os perfis da pasta, com o esquema de verdade: a autonomia e as unidades do computador de bordo
# acompanham o que a tela escolheu em Unidades (os bytes foram calculados à mão)
if [ -f $VEI/gerado/esquema.txt ]; then
  PJ="$VEI/carro-parado.txt $VEI/jetta-gli-2021.txt $VEI/jetta-gli-2021-deducoes.txt"
  printf '[DSICarTimeUnitsLanguage.updateDistanceUnit]\ndistanceUnit = 1\n[DSICarTimeUnitsLanguage.updateSpeedUnit]\nspeedUnit = 1\n[DSICarTimeUnitsLanguage.updateConsumptionPetrolUnit]\nconsumptionPetrolUnit = 4\n' > $T/milhas.txt
  "$BIN/veiculostub" -m $VEI/gerado/esquema.txt $PJ > $T/jetta-km.log 2>&1
  "$BIN/veiculostub" -m $VEI/gerado/esquema.txt $PJ $T/milhas.txt > $T/jetta-mi.log 2>&1
  K="^canal=0x[0-9a-f]+ if=[0-9]+ inst=0"
  tem jetta_no_padrao_a_autonomia_e_400_km "$K api=8 pb=0a 05 08 02 10 a0 06 10 02\$" $T/jetta-km.log
  tem jetta_com_milhas_a_autonomia_e_249_mi "$K api=8 pb=0a 07 08 02 10 f2 03 18 02 10 02\$" $T/jetta-mi.log
  tem jetta_no_padrao_a_media_de_consumo_e_em_km_por_litro "$K api=11 pb=0a 02 18 02 10 02\$" $T/jetta-km.log
  tem jetta_com_mpg_a_media_de_consumo_acompanha "$K api=11 pb=0a 02 18 08 10 02\$" $T/jetta-mi.log
  tem jetta_com_mpg_a_media_de_longo_prazo_acompanha "$K api=14 pb=0a 02 18 08 10 02\$" $T/jetta-mi.log
  tem jetta_com_mpg_a_media_desde_o_abastecimento_acompanha "$K api=17 pb=0a 02 18 08 10 02\$" $T/jetta-mi.log
  tem jetta_com_milhas_a_distancia_desde_a_partida_e_em_milhas "$K api=13 pb=0a 08 0a 02 18 02 12 02 18 02 10 02\$" $T/jetta-mi.log
  tem jetta_com_milhas_a_distancia_de_longo_prazo_e_em_milhas "$K api=16 pb=0a 08 0a 02 18 02 12 02 18 02 10 02\$" $T/jetta-mi.log
  tem jetta_com_milhas_a_distancia_desde_o_abastecimento_e_em_milhas "$K api=19 pb=0a 08 0a 02 18 02 12 02 18 02 10 02\$" $T/jetta-mi.log
else
  nok esquema_do_firmware_para_as_unidades "não achei $VEI/gerado/esquema.txt"
fi

kill $ESC $HUB 2>/dev/null; wait 2>/dev/null
rm -f $SOCK
echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- o que foi escutado (fim) ---"; tail -n 15 $T/escuta.log | cut -c1-200; echo "--- stub ---"; tail -n 12 $T/stub.log; echo "--- stub dos pedidos ---"; tail -n 12 $T/stubp.log 2>/dev/null | cut -c1-200; }
[ $fail -eq 0 ]
