#!/bin/sh
# Teste do catálogo dos menus do carro (Marco 14, 12/10/2026): stubs/veiculo/catalogo-menus.txt,
# stubs/codificacao/catalogo-menus.txt, o gerador dos dois (driver/m14_catalogo_menus.py) e o
# roteiro que sobe uma central de teste com eles (driver/m14_menus.sh).
# O catálogo liga tudo o que a HMI sabe mostrar em alguns menus do carro. Não é o carro: por isso
# não pode entrar em partida nenhuma por padrão, não pode trazer valor de carro e cada número dele
# tem de sair do firmware (do esquema e da tabela de campos), não de quem escreveu.
# Não precisa do firmware nem do container. Precisa de python3, que a imagem moi3-bancada:1 não
# tem: lá o teste avisa e sai sem falhar. Roda no Mac, de qualquer pasta:
#   sh referencia/ferramentas/mib3-sandbox/driver/tests/t_catalogo.sh
# Que cada linha do perfil de veículo é um campo de verdade do esquema, quem confere é o
# t_veiculo.sh (perfil_da_pasta_confere), com o veiculostub, dentro do container.
D=$(cd "$(dirname "$0")/.." && pwd); B=$(cd "$D/.." && pwd)
G="$D/m14_catalogo_menus.py"; V="$B/stubs/veiculo/catalogo-menus.txt"; C="$B/stubs/codificacao/catalogo-menus.txt"
ESQ="$B/stubs/veiculo/gerado/esquema.txt"; TAB="$B/stubs/codificacao/tabela-campos.tsv"
command -v python3 >/dev/null 2>&1 || { echo "PULOU: sem python3 nesta máquina (o gerador do catálogo roda no Mac)"; exit 0; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
# vazio <nome> <saída>: passa se a conferência não achou nada; sem os arquivos do catálogo, falha
vazio() { if [ -n "$FALTA" ]; then nok "$1" "falta $FALTA"; elif [ -z "$2" ]; then ok "$1"; else nok "$1" "$(printf '%s' "$2" | head -n 3 | tr '\n' ';')"; fi; }
# as linhas que valem: sem comentário, sem espaço nas pontas, sem linha vazia
valem() { sed 's/#.*//; s/[[:space:]]*$//; s/^[[:space:]]*//' "$1" 2>/dev/null | grep -v '^$'; }

# ---- os três arquivos existem
FALTA=
for par in "gerador:$G" "catalogo_de_veiculo:$V" "catalogo_de_codificacao:$C"; do
  if [ -f "${par#*:}" ]; then ok "${par%%:*}_existe"; else nok "${par%%:*}_existe" "falta ${par#*:}"; FALTA="$FALTA $(basename "${par#*:}")"; fi
done

# ---- os dois arquivos são o que o gerador escreve do esquema e da tabela de campos de agora
python3 -I "$G" "$B" "$T" > "$T/gerador.out" 2>&1; rc=$?
if [ $rc -eq 0 ]; then ok gerador_roda; else nok gerador_roda "saiu com $rc: $(tail -n 2 "$T/gerador.out" | tr '\n' ' ')"; fi
if cmp -s "$T/veiculo.txt" "$V"; then ok veiculo_e_o_que_o_gerador_escreve
else nok veiculo_e_o_que_o_gerador_escreve "o arquivo da pasta difere do gerado: $(diff "$T/veiculo.txt" "$V" 2>&1 | head -n 2 | tr '\n' ' ')"; fi
if cmp -s "$T/codificacao.txt" "$C"; then ok codificacao_e_o_que_o_gerador_escreve
else nok codificacao_e_o_que_o_gerador_escreve "o arquivo da pasta difere do gerado: $(diff "$T/codificacao.txt" "$C" 2>&1 | head -n 2 | tr '\n' ' ')"; fi

# ---- o perfil de veículo só diz "esta opção existe": nenhum valor de carro
n=$(valem "$V" | grep -c '\.state = 2$')
if [ "$n" -ge 100 ]; then ok "veiculo_tem_opcoes ($n)"; else nok veiculo_tem_opcoes "só $n linhas '.state = 2'"; fi
vazio veiculo_so_tem_opcoes_de_exibicao_em_2 "$(valem "$V" | grep -v '^\[' | grep -v '^[A-Za-z0-9_.]*\.state = 2$')"
vazio veiculo_so_tem_secoes_de_opcoes "$(valem "$V" | grep '^\[' | grep -v '^\[DSI[A-Za-z]*\.update[A-Za-z0-9]*ViewOptions[A-Za-z0-9]*\]$')"
# com esta opção a HMI pede a lista de cores da luz ambiente, que o carro de mentira não responde
vazio veiculo_nao_liga_a_lista_de_cores "$(valem "$V" | grep -i 'RGBColorList')"
# toda seção e todo caminho existem no esquema (conferência sem o veiculostub: evento e primeiro campo)
vazio veiculo_cada_secao_e_um_evento_do_esquema "$(valem "$V" | grep '^\[' | tr -d '[]' | while IFS=. read -r s e; do grep -q "^E $s [0-9]* [0-9-]* [0-9-]* $e " "$ESQ" || echo "$s.$e"; done)"

# ---- a codificação só liga "função no menu" (0B1B) e "aparelho existe" (0B1D), com os bytes da tabela
n=$(valem "$C" | grep -c .)
if [ "$n" -ge 8 ]; then ok "codificacao_tem_linhas ($n)"; else nok codificacao_tem_linhas "só $n"; fi
vazio codificacao_so_liga_menu_e_aparelho "$(valem "$C" | grep -v '^0B1B @[0-9]*\.7+1 = 1$' | grep -v '^0B1D @[0-9]*\.6+2 = 1$')"
vazio codificacao_cada_linha_e_um_campo_da_tabela "$(valem "$C" | while read -r did pos ig val; do
  byte=${pos#@}; byte=${byte%%.*}; bit=${pos#*.}; bits=${bit#*+}; bit=${bit%%+*}
  if [ "$did" = 0B1B ]; then quer='Menu display'; else quer='Availability'; fi
  awk -F'\t' -v d="$did" -v by="$byte" -v bi="$bit" -v n="$bits" -v q="$quer" '$3==d && $5==by && $7==bi && $8==n && index($15,q){a=1} END{exit !a}' "$TAB" || echo "$did $pos: sem campo \"$quer\" na tabela"
done)"
# a constante escrita ao lado de cada linha é a do campo que a linha liga
vazio codificacao_a_constante_ao_lado_e_a_do_campo "$(grep '^0B1' "$C" 2>/dev/null | while read -r did pos ig val resto; do
  byte=${pos#@}; byte=${byte%%.*}; bit=${pos#*.}; bit=${bit%%+*}
  k=$(printf '%s' "$resto" | sed -n 's/^# *\(ID_[A-Z0-9_]*\).*/\1/p')
  t=$(awk -F'\t' -v d="$did" -v by="$byte" -v bi="$bit" '$3==d && $5==by && $7==bi {print $13}' "$TAB")
  [ -n "$k" ] && [ "$k" = "$t" ] || echo "$did $pos: ao lado '$k', na tabela '$t'"
done)"

# ---- os dois dizem logo no começo que não são o carro
for par in "veiculo:$V" "codificacao:$C"; do
  if head -n 4 "${par#*:}" 2>/dev/null | grep -q 'NÃO É O CARRO'; then ok "${par%%:*}_diz_que_nao_e_o_carro"; else nok "${par%%:*}_diz_que_nao_e_o_carro" "sem 'NÃO É O CARRO' nas quatro primeiras linhas"; fi
done

# ---- o catálogo não entra em partida nenhuma por padrão
FALTA=
# nenhum lançador o cita, e os padrões dos lançadores são listas de arquivos, sem curinga: um
# arquivo novo na pasta dos perfis não entra sozinho
# (o m14_menus.sh é o único roteiro que o usa, e só quando alguém o chama)
vazio nenhum_lancador_cita_o_catalogo "$(grep -l 'catalogo-menus' "$D"/*.sh "$D"/servidor_ao_vivo.py "$B"/janela/*.sh 2>/dev/null | grep -v '/m14_menus\.sh$')"
vazio padroes_dos_lancadores_sem_curinga "$(grep -n 'stubs/veiculo/\|stubs/codificacao/' "$D/m3_ao_vivo.sh" "$D/ao_vivo.sh" "$D/m1_hmi.sh" "$D/m8_central.sh" | grep -v '^[^:]*:[0-9]*:[[:space:]]*#' | grep '\*')"
n=$(grep -c '^VEICULO=${VEICULO-/stubs/veiculo/carro-parado.txt /stubs/veiculo/jetta-gli-2021.txt /stubs/veiculo/jetta-gli-2021-deducoes.txt}' "$D/m3_ao_vivo.sh")
if [ "$n" = 1 ]; then ok padrao_de_veiculo_e_a_lista_de_tres; else nok padrao_de_veiculo_e_a_lista_de_tres "o padrão de VEICULO do m3_ao_vivo.sh mudou: conferir se o catálogo continua de fora"; fi
n=$(grep -c '^CODING=${CODING-/stubs/codificacao/padrao.txt /stubs/codificacao/jetta-gli-2021.txt /stubs/codificacao/jetta-gli-2021-deducoes.txt /stubs/codificacao/ajustes.txt}' "$D/m3_ao_vivo.sh")
if [ "$n" = 1 ]; then ok padrao_de_codificacao_e_a_lista_de_quatro; else nok padrao_de_codificacao_e_a_lista_de_quatro "o padrão de CODING do m3_ao_vivo.sh mudou: conferir se o catálogo continua de fora"; fi

# ---- o perfil que aceita a gravação nos menus do catálogo (12/10/2026): também não é o carro
GV="$B/stubs/veiculo/catalogo-menus-grava.txt"; FALTA=
if [ -f "$GV" ]; then ok perfil_de_gravacao_existe; else nok perfil_de_gravacao_existe "falta $GV"; FALTA=catalogo-menus-grava.txt; fi
if head -n 4 "$GV" 2>/dev/null | grep -q 'NÃO É O CARRO'; then ok perfil_de_gravacao_diz_que_nao_e_o_carro; else nok perfil_de_gravacao_diz_que_nao_e_o_carro "sem 'NÃO É O CARRO' nas quatro primeiras linhas"; fi
# a lista "grava" de depois troca a de antes: a daqui tem de repetir a do perfil padrão
gp=$(valem "$B/stubs/veiculo/carro-parado.txt" | sed -n 's/^grava *= *//p'); gg=$(valem "$GV" | sed -n 's/^grava *= *//p')
vazio perfil_de_gravacao_repete_a_lista_do_padrao "$(for x in $gp; do printf ' %s \n' "$gg" | grep -q " $x " || echo "falta $x na lista grava"; done; [ -n "$gp" ] || echo "não achei a lista grava do carro-parado.txt")"
# só a lista e regras de gravação: nenhum valor, nenhuma seção de evento
vazio perfil_de_gravacao_nao_da_valor_a_nada "$(valem "$GV" | grep -v '^grava *=' | grep -v '^\[DSI[A-Za-z]*\.set[A-Za-z0-9]* -> update[A-Za-z0-9]*\]$' | grep -v '^[A-Za-z0-9_.]* = [A-Za-z][A-Za-z0-9_.]*$')"
FALTA=

# ---- o roteiro que sobe a central de teste com o catálogo
# Ele tira os padrões do m3_ao_vivo.sh e põe o catálogo depois deles; não repete a lista.
R="$D/m14_menus.sh"
if [ -f "$R" ]; then ok roteiro_existe; else nok roteiro_existe "falta $R"; FALTA=m14_menus.sh; fi
PC=$(sed -n 's/^CODING=${CODING-\(.*\)}; export CODING$/\1/p' "$D/m3_ao_vivo.sh")
PV=$(sed -n 's/^VEICULO=${VEICULO-\(.*\)}; export VEICULO$/\1/p' "$D/m3_ao_vivo.sh")
m=$(sh "$R" mostra 2>&1)
tem() { if [ -n "$FALTA" ]; then nok "$1" "falta $FALTA"; elif printf '%s\n' "$2" | grep -qF -- "$3"; then ok "$1"; else nok "$1" "sem '$3' em: $(printf '%s' "$2" | head -n 2 | tr '\n' ' ')"; fi; }
tem roteiro_poe_o_catalogo_depois_da_codificacao_padrao "$m" "CODING=\"$PC /stubs/codificacao/catalogo-menus.txt\""
tem roteiro_poe_o_catalogo_depois_do_veiculo_padrao "$m" "VEICULO=\"$PV /stubs/veiculo/catalogo-menus.txt\""
tem roteiro_sobe_a_central_de_teste_menus "$m" 'driver/m8_central.sh sobe menus '
vazio roteiro_nao_toca_na_central_ao_vivo "$(printf '%s\n' "$m" | grep 'ao_vivo\|moi3-ao-vivo\|ao-vivo/')"
# o comando que ele roda é o que ele mostra: com um m8_central.sh de mentira no lugar, o "sobe"
# chega lá com as duas listas no ambiente e os argumentos na ordem
if [ -f "$R" ]; then
  mkdir -p "$T/b/driver" "$T/pasta"; cp "$R" "$D/m3_ao_vivo.sh" "$T/b/driver/"
  printf '#!/bin/sh\necho "args: $*"; echo "CODING=$CODING"; echo "VEICULO=$VEICULO"; echo "ESTADO=$VEICULO_ESTADO"\n' > "$T/b/driver/m8_central.sh"
  s1=$(MENUS_PASTA="$T/pasta" sh "$T/b/driver/m14_menus.sh" 90 2>&1)
  tem roteiro_sobe_com_os_segundos_dados "$s1" "args: sobe menus $T/pasta 90"
  tem roteiro_sobe_com_a_codificacao_do_catalogo "$s1" "CODING=$PC /stubs/codificacao/catalogo-menus.txt"
  tem roteiro_sobe_com_o_veiculo_do_catalogo "$s1" "VEICULO=$PV /stubs/veiculo/catalogo-menus.txt"
  s2=$(MENUS_PASTA="$T/pasta" sh "$T/b/driver/m14_menus.sh" 2>&1)
  tem roteiro_sobe_por_meia_hora_sem_pedido "$s2" "args: sobe menus $T/pasta 1800"
  s3=$(sh "$T/b/driver/m14_menus.sh" desce 2>&1)
  tem roteiro_desce "$s3" 'args: desce menus'
  # "grava" (12/10/2026): a mesma central, com o perfil que aceita o que a tela grava nos menus e
  # com o arquivo de estado do carro de mentira, para o que foi marcado voltar na partida seguinte
  s5=$(MENUS_PASTA="$T/pasta" sh "$T/b/driver/m14_menus.sh" grava 90 2>&1)
  tem roteiro_grava_sobe_a_mesma_central "$s5" "args: sobe menus $T/pasta 90"
  tem roteiro_grava_poe_o_perfil_de_gravacao_por_ultimo "$s5" "VEICULO=$PV /stubs/veiculo/catalogo-menus.txt /stubs/veiculo/catalogo-menus-grava.txt"
  tem roteiro_grava_liga_o_arquivo_de_estado "$s5" "ESTADO=/live/memoria/veiculo/estado.txt"
  s6=$(MENUS_PASTA="$T/pasta" sh "$T/b/driver/m14_menus.sh" grava 2>&1)
  tem roteiro_grava_sobe_por_meia_hora_sem_pedido "$s6" "args: sobe menus $T/pasta 1800"
  vazio roteiro_sem_grava_nao_leva_o_perfil_de_gravacao "$(printf '%s\n' "$s1" | grep 'catalogo-menus-grava')"
  vazio roteiro_sem_grava_nao_liga_o_estado "$(printf '%s\n' "$s1" | grep '^ESTADO=.'; printf '%s\n' "$s1" | grep -q '^ESTADO=$' || echo "sem a linha ESTADO=")"
  tem roteiro_mostra_o_comando_do_grava "$(sh "$R" mostra grava 2>&1)" "VEICULO_ESTADO=/live/memoria/veiculo/estado.txt CODING="
  # se o padrão do m3_ao_vivo.sh mudar de forma, o roteiro para e diz por quê, em vez de subir sem ele
  grep -v '^VEICULO=${VEICULO-' "$D/m3_ao_vivo.sh" > "$T/b/driver/m3_ao_vivo.sh"
  s4=$(MENUS_PASTA="$T/pasta" sh "$T/b/driver/m14_menus.sh" 90 2>&1); rc=$?
  if [ $rc -ne 0 ] && ! printf '%s\n' "$s4" | grep -q '^args:'; then ok roteiro_para_se_nao_acha_o_padrao; else nok roteiro_para_se_nao_acha_o_padrao "saiu com $rc: $(printf '%s' "$s4" | head -n 2 | tr '\n' ' ')"; fi
else
  for n in roteiro_sobe_com_os_segundos_dados roteiro_sobe_com_a_codificacao_do_catalogo roteiro_sobe_com_o_veiculo_do_catalogo roteiro_sobe_por_meia_hora_sem_pedido roteiro_desce roteiro_grava_sobe_a_mesma_central roteiro_grava_poe_o_perfil_de_gravacao_por_ultimo roteiro_grava_liga_o_arquivo_de_estado roteiro_grava_sobe_por_meia_hora_sem_pedido roteiro_sem_grava_nao_leva_o_perfil_de_gravacao roteiro_sem_grava_nao_liga_o_estado roteiro_mostra_o_comando_do_grava roteiro_para_se_nao_acha_o_padrao; do nok $n "falta m14_menus.sh"; done
fi

# ---- os menus que o Lipe disse que o carro tem (12/10/2026, à tarde): dois perfis à parte, que
# entram no padrão do modo ao vivo e saem com MENUS=0. Cada linha diz de onde veio; nada é leitura
# do carro. Pneus e Lane Assist ficam de fora.
JV="$B/stubs/veiculo/jetta-gli-2021-menus.txt"; JC="$B/stubs/codificacao/jetta-gli-2021-menus.txt"; FALTA=
for par in "menus_do_jetta_veiculo:$JV" "menus_do_jetta_codificacao:$JC"; do
  if [ -f "${par#*:}" ]; then ok "${par%%:*}_existe"; else nok "${par%%:*}_existe" "falta ${par#*:}"; FALTA="$FALTA $(basename "${par#*:}")"; fi
done
vazio menus_do_jetta_dizem_que_nao_sao_leitura_do_carro "$(for f in "$JV" "$JC"; do head -n 6 "$f" 2>/dev/null | grep -q 'NÃO É LEITURA DO CARRO' || echo "$(basename "$f") sem o aviso"; done)"
# só opções em 2, a lista grava e regras de gravação; e toda linha que vale traz a fonte ao lado
vazio menus_do_jetta_so_tem_opcoes_e_a_lista_grava "$(valem "$JV" | grep -v '^\[' | grep -v '^[A-Za-z0-9_.]*\.state = 2$' | grep -v '^grava *=' | grep -v '^[A-Za-z0-9_.]* = [A-Za-z][A-Za-z0-9_.]*$')"
vazio menus_do_jetta_cada_linha_diz_a_fonte "$(grep -h '^[A-Za-z0-9_.]*\.state = 2\|^0B1' "$JV" "$JC" 2>/dev/null | grep -v '#.*\(dito pelo Lipe\|informou\|visto na foto\|palpite\)')"
gp=$(valem "$B/stubs/veiculo/carro-parado.txt" | sed -n 's/^grava *= *//p'); gg=$(valem "$JV" | sed -n 's/^grava *= *//p')
vazio menus_do_jetta_repetem_a_lista_grava_do_padrao "$(for x in $gp; do printf ' %s \n' "$gg" | grep -q " $x " || echo "falta $x na lista grava"; done; [ -n "$gp" ] || echo "não achei a lista grava do carro-parado.txt")"
vazio menus_do_jetta_cada_secao_e_um_evento_do_esquema "$(valem "$JV" | grep '^\[' | grep -v -- '->' | tr -d '[]' | while IFS=. read -r s e; do grep -q "^E $s [0-9]* [0-9-]* [0-9-]* $e " "$ESQ" || echo "$s.$e"; done)"
vazio menus_do_jetta_codificacao_so_liga_funcoes_da_tabela "$(valem "$JC" | while read -r did pos ig val; do
  byte=${pos#@}; byte=${byte%%.*}
  [ "$did" = 0B1B ] && [ "$pos" = "@$byte.7+1" ] && [ "$val" = 1 ] && awk -F'\t' -v by="$byte" '$3=="0B1B" && $5==by && $7==7 && $8==1 && index($15,"Menu display"){a=1} END{exit !a}' "$TAB" || echo "$did $pos = $val"
done)"
# o que ele não citou, ou que o carro não tem, não entra: pneus e a câmera (Lane Assist, sinalização)
vazio menus_do_jetta_sem_pneus_nem_lane_assist "$(grep -h -i 'ID_DISPLAY_RDK\|ID_DISPLAY_LANE\|ID_DISPLAY_ROAD_SIGN\|RDKViewOptions\|LDWHCAViewOptions\|SWAViewOptions\|TSDViewOptions' "$JV" "$JC" 2>/dev/null | grep -v '^#')"
n=$(valem "$JC" | grep -c '^0B1B'); if [ -z "$FALTA" ] && [ "$n" = 7 ]; then ok menus_do_jetta_sao_sete_funcoes; else nok menus_do_jetta_sao_sete_funcoes "esperava 7 (quatro menus e três assistentes), veio $n"; fi
FALTA=
# o lançador do modo ao vivo: os dois perfis no fim dos padrões; MENUS=0 ou vazio tira os dois.
# Roda uma CÓPIA do ao_vivo.sh numa bancada de mentira, com um "docker" que só anota o que recebeu.
BV="$T/bancada viva"; mkdir -p "$BV/driver" "$T/bin"; cp "$D/ao_vivo.sh" "$BV/driver/"
printf '#!/bin/sh\nfor a in "$@"; do printf "%%s\\n" "$a"; done > "$DOCKER_VIU"\n' > "$T/bin/docker"; chmod +x "$T/bin/docker"
aovivo() { rm -rf "$BV/ao-vivo" "$T/docker.args"; ( unset MENUS CODING VEICULO; export DOCKER_VIU="$T/docker.args" SEM_SERVIDOR=1 PATH="$T/bin:$PATH"; env "$@" sh "$BV/driver/ao_vivo.sh" 5 >/dev/null 2>&1 ); }
arg() { grep -- "^$1=" "$T/docker.args" 2>/dev/null | head -n 1; }
aovivo
case "$(arg VEICULO)" in *" /stubs/veiculo/jetta-gli-2021-deducoes.txt /stubs/veiculo/jetta-gli-2021-menus.txt") ok ao_vivo_poe_os_menus_no_fim_do_veiculo;; *) nok ao_vivo_poe_os_menus_no_fim_do_veiculo "$(arg VEICULO | cut -c1-200)";; esac
case "$(arg CODING)" in *" /stubs/codificacao/jetta-gli-2021-menus.txt"*) ok ao_vivo_poe_os_menus_na_codificacao;; *) nok ao_vivo_poe_os_menus_na_codificacao "$(arg CODING | cut -c1-200)";; esac
for v in 0 ""; do
  aovivo "MENUS=$v"
  if [ -s "$T/docker.args" ] && ! grep -q 'jetta-gli-2021-menus' "$T/docker.args"; then ok "ao_vivo_sem_os_menus_com_MENUS=$v"; else nok "ao_vivo_sem_os_menus_com_MENUS=$v" "$(grep -c 'jetta-gli-2021-menus' "$T/docker.args" 2>/dev/null) argumento(s) com o perfil"; fi
done
aovivo "VEICULO=/stubs/veiculo/carro-parado.txt"
if [ "$(arg VEICULO)" = "VEICULO=/stubs/veiculo/carro-parado.txt" ]; then ok ao_vivo_respeita_a_lista_dada_a_mao; else nok ao_vivo_respeita_a_lista_dada_a_mao "$(arg VEICULO | cut -c1-160)"; fi
# os outros lançadores não mudam: central de teste só tem os menus se alguém pedir
vazio centrais_de_teste_sem_os_menus_por_padrao "$(grep -l 'jetta-gli-2021-menus' "$D/m3_ao_vivo.sh" "$D/m1_hmi.sh" "$D/m8_central.sh" "$D/m14_menus.sh" 2>/dev/null)"

echo "t_catalogo: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
