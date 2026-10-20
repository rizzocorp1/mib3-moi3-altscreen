#!/bin/sh
# Teste do trecho HMILOG do lançador (driver/m1_hmi.sh), de 12/10/2026: as chaves de rastreamento
# pedidas têm de entrar no logging.properties E na seção LOGMANAGER de todos os perfis de sondas
# de depuração (/vw/hmi/config/debugprobes_*.config). É dos perfis que a HMI tira as chaves na
# carga tardia, uns 6 s depois da partida; sem isso o rastreamento para ali (modelagem-soc.md,
# "O rastreamento da HMI que parava na carga tardia"). O que este teste fixa:
#   - cada chave válida vira "CHAVE = true" no logging.properties e no fim da seção LOGMANAGER de
#     cada perfil, e nada mais muda em nenhum dos arquivos;
#   - chave que o logging.properties não tem é avisada e não entra em lugar nenhum;
#   - perfil sem a seção LOGMANAGER fica como está, com aviso;
#   - com HMILOG=normal (sem chave) nenhum perfil é tocado; sem HMILOG, nada é tocado;
#   - o resumo "registro detalhado:" diz o que a HMI registrou ter aplicado.
# O teste roda o trecho TAL COMO ESTÁ no lançador: recorta o primeiro bloco que começa em
# 'if [ -n "$HMILOG" ]; then' e vai até o "fi" dele, e o roda numa árvore de mentira, com o mount
# trocado por uma função que anota o pedido e copia o arquivo por cima. Se o bloco mudar de
# forma, o primeiro caso falha e diz.
# Roda na imagem moi3-bancada:1 (o sed -i do lançador é o do Linux); não precisa do firmware nem
# de privilégio. Com o firmware montado em /fw (como nas suítes), confere também os 33 perfis de
# verdade.
#   t_hmilog.sh [lançador]        (default: o m1_hmi.sh da pasta deste teste)
D=$(cd "$(dirname "$0")/.." && pwd); L=${1:-$D/m1_hmi.sh}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
C=$T/low/vw/hmi/config

# O trecho do lançador e, à parte, o bloco do resumo (o último 'if [ -n "$HMILOG" ]' do arquivo)
sed -n '/^if \[ -n "\$HMILOG" \]; then$/,/^fi$/p' "$L" | sed '/^fi$/q' > "$T/trecho.sh"
awk '/^if \[ -n "\$HMILOG" \]; then$/ { n++; d = 1; b = "" } d { b = b $0 "\n" } /^fi$/ && d { d = 0; if (n > 1) fim = b } END { printf "%s", fim }' "$L" > "$T/resumo.sh"
grep '^c() {' "$L" > "$T/c.sh"

# árvore de mentira: um logging.properties e três perfis no formato dos do firmware
arvore() {
  rm -rf "$T/low" "$T/antes" "$T/montagens.txt" "$T/vm.txt"; mkdir -p "$C" "$T/low/tmp"; : > "$T/montagens.txt"
  cat > "$C/logging.properties" <<'EOF'
# version of logger settings
logging.settings.version = 2
logging.console.enabled = true

# settings for context APPS
APPS.LIFECYCLE = false
APPS.MANAGER = false
DSI.CAR = false
DSI.CAR_TIME_UNITS_LANGUAGE = false
EOF
  # como o perfil 130 do firmware: seção LOGMANAGER só com comentários, e sem fim de linha no fim
  printf '%s\n' '# ----- CONFIGURATION OF PROBE MANAGER -----' 'BEGIN PROBES' '' 'sinks = TCP' \
    'probes = DSIEVENTS, RSIPROBE, LOGMANAGER' '' 'END PROBES' '' '# ----- CONFIGURATION OF LOGMANAGER PROBE -----' \
    'BEGIN LOGMANAGER' '' '#RSI.VIWI = true' '' 'END LOGMANAGER' '' 'BEGIN RSIPROBE' '' 'log.read.enabled = true' '' > "$C/debugprobes_130_DSI_+_RSI_only.config"
  printf 'END RSIPROBE' >> "$C/debugprobes_130_DSI_+_RSI_only.config"
  # como o 060: a seção já traz as chaves dela, que têm de continuar lá
  printf '%s\n' 'BEGIN PROBES' 'sinks = TCP' 'probes = DSIEVENTS, LOGMANAGER' 'END PROBES' 'BEGIN LOGMANAGER' '' \
    'CAR.ASL_CAR = true' 'DSI.CAR = true' '' 'END LOGMANAGER' 'BEGIN DATAPOOL' 'log.changerequests.enabled = false' 'END DATAPOOL' > "$C/debugprobes_060_CAR_Specific_Tracing.config"
  # um perfil sem a seção LOGMANAGER
  printf '%s\n' 'BEGIN PROBES' 'sinks = TCP' 'probes = DSIEVENTS' 'END PROBES' > "$C/debugprobes_999_sem_secao.config"
  cp -R "$C" "$T/antes"
}
# roda <HMILOG ou "-" para sem a variável>: o trecho num shell à parte, com o mount de mentira
roda() {
  { cat <<'EOF'
mount() { [ "$1" = -o ] && [ "$2" = bind ] && [ -f "$3" ] && [ -f "$4" ] || return 1; echo "$4" >> "$MONTAGENS"; cp "$3" "$4"; }
EXTRA_VMOPTIONS=-Dantes=1
EOF
    cat "$T/trecho.sh"; echo 'echo "$EXTRA_VMOPTIONS" > "$VM"'; } > "$T/roda.sh"
  if [ "$1" = - ]; then ( unset HMILOG; LOW=$T/low MONTAGENS=$T/montagens.txt VM=$T/vm.txt sh "$T/roda.sh" ) > "$T/saida.txt" 2>&1
  else LOW=$T/low MONTAGENS=$T/montagens.txt VM=$T/vm.txt HMILOG="$1" sh "$T/roda.sh" > "$T/saida.txt" 2>&1; fi
  RC=$?
}
# esperado <arquivo de antes> <chave>...: grava em $T/esperado.config o arquivo de antes com uma
# linha "CHAVE = true" por chave, nessa ordem, logo antes da linha END LOGMANAGER que fecha a seção
# BEGIN LOGMANAGER, e mais nada. O awk põe fim de linha no fim do arquivo; o perfil do firmware
# pode não ter, e a cópia tem de ficar igual a ele byte a byte
esperado() {
  a=$1; shift
  awk -v ch="$*" 'BEGIN { n = split(ch, k, " ") }
    /^BEGIN LOGMANAGER/ { dentro = 1 }
    /^END LOGMANAGER/ { if (dentro) for (i = 1; i <= n; i++) print k[i] " = true"; dentro = 0 }
    { print }' "$a" > "$T/esperado.awk"
  if [ -n "$(tail -c 1 "$a")" ]; then printf '%s' "$(cat "$T/esperado.awk")" > "$T/esperado.config"; else cp "$T/esperado.awk" "$T/esperado.config"; fi
}
# so_acrescentou <nome do caso> <arquivo de antes> <arquivo de depois> <chave>...
so_acrescentou() {
  caso=$1; d=$3; a=$2; shift 3
  esperado "$a" "$@"
  if cmp -s "$T/esperado.config" "$d"; then ok "$caso"; else nok "$caso" "$(diff "$T/esperado.config" "$d" | head -n 4 | tr '\n' '|')"; fi
}

# 1. O bloco existe, com a forma que o teste sabe recortar
if [ "$(head -n 1 "$T/trecho.sh")" = 'if [ -n "$HMILOG" ]; then' ] && [ "$(tail -n 1 "$T/trecho.sh")" = fi ] && [ "$(wc -l < "$T/trecho.sh")" -ge 8 ]; then ok trecho_recortado
else nok trecho_recortado "não achei em $L o bloco 'if [ -n \"\$HMILOG\" ]; then' … 'fi' ($(wc -l < "$T/trecho.sh") linhas)"; fi

# 2. Duas chaves válidas e uma que não existe
arvore; roda "normal DSI.CAR_TIME_UNITS_LANGUAGE NAO.EXISTE APPS.LIFECYCLE"
igual chaves_sai_com_zero "$RC" 0
igual chaves_no_logging_properties "$(grep ' = true$' "$C/logging.properties" | grep -v '^logging\.' | tr '\n' ';')" "APPS.LIFECYCLE = true;DSI.CAR_TIME_UNITS_LANGUAGE = true;"
igual logging_properties_so_troca_as_chaves "$(diff "$T/antes/logging.properties" "$C/logging.properties" | grep -c '^[<>]')" 4
so_acrescentou perfil_130_ganha_as_chaves "$T/antes/debugprobes_130_DSI_+_RSI_only.config" "$C/debugprobes_130_DSI_+_RSI_only.config" DSI.CAR_TIME_UNITS_LANGUAGE APPS.LIFECYCLE
so_acrescentou perfil_060_guarda_as_dele "$T/antes/debugprobes_060_CAR_Specific_Tracing.config" "$C/debugprobes_060_CAR_Specific_Tracing.config" DSI.CAR_TIME_UNITS_LANGUAGE APPS.LIFECYCLE
if cmp -s "$T/antes/debugprobes_999_sem_secao.config" "$C/debugprobes_999_sem_secao.config"; then ok perfil_sem_secao_fica_igual; else nok perfil_sem_secao_fica_igual "o arquivo mudou"; fi
if grep -q 'debugprobes_999_sem_secao.config.*LOGMANAGER' "$T/saida.txt"; then ok perfil_sem_secao_e_avisado; else nok perfil_sem_secao_e_avisado "sem aviso: $(tr '\n' '|' < "$T/saida.txt")"; fi
if grep -q 'chave desconhecida NAO.EXISTE' "$T/saida.txt"; then ok chave_desconhecida_e_avisada; else nok chave_desconhecida_e_avisada "sem aviso: $(tr '\n' '|' < "$T/saida.txt")"; fi
igual chave_desconhecida_nao_entra "$(grep -rl 'NAO.EXISTE' "$T/low" | wc -l | tr -d ' ')" 0
igual montou_o_logging_e_dois_perfis "$(sed "s|$C/||" "$T/montagens.txt" | tr '\n' ';')" "logging.properties;debugprobes_060_CAR_Specific_Tracing.config;debugprobes_130_DSI_+_RSI_only.config;"
case "$(cat "$T/vm.txt")" in
  *-Dde.vw.mib.log4mib.console.traces.enabled=true*-Dantes=1*) ok liga_o_console_e_guarda_as_opcoes ;;
  *) nok liga_o_console_e_guarda_as_opcoes "EXTRA_VMOPTIONS ficou '$(cat "$T/vm.txt")'" ;;
esac

# 3. Só "normal": o console é ligado, o logging.properties vai igual e nenhum perfil é tocado
arvore; roda normal
igual normal_sai_com_zero "$RC" 0
igual normal_so_monta_o_logging "$(sed "s|$C/||" "$T/montagens.txt" | tr '\n' ';')" "logging.properties;"
if diff -r "$T/antes" "$C" > /dev/null; then ok normal_nao_muda_arquivo_nenhum; else nok normal_nao_muda_arquivo_nenhum "$(diff -rq "$T/antes" "$C" | tr '\n' '|')"; fi

# 4. Só chave que não existe: avisa, e é como "normal"
arvore; roda "NAO.EXISTE"
igual desconhecida_so_monta_o_logging "$(sed "s|$C/||" "$T/montagens.txt" | tr '\n' ';')" "logging.properties;"
if diff -r "$T/antes" "$C" > /dev/null; then ok desconhecida_nao_muda_arquivo_nenhum; else nok desconhecida_nao_muda_arquivo_nenhum "$(diff -rq "$T/antes" "$C" | tr '\n' '|')"; fi

# 5. Sem HMILOG, e com HMILOG vazio: nada é montado, nada muda, as opções da JVM ficam como estavam
for v in - ""; do
  n=sem_hmilog; [ "$v" = - ] || n=hmilog_vazio
  arvore; roda "$v"
  igual ${n}_nao_monta_nada "$(wc -l < "$T/montagens.txt" | tr -d ' ')" 0
  if diff -r "$T/antes" "$C" > /dev/null && [ ! -e "$T/low/tmp/logging.properties" ] && [ ! -e "$T/low/tmp/hmilog" ]; then ok ${n}_nao_muda_nem_cria_arquivo; else nok ${n}_nao_muda_nem_cria_arquivo "$(diff -rq "$T/antes" "$C" | tr '\n' '|') $(ls "$T/low/tmp")"; fi
  igual ${n}_nao_mexe_nas_opcoes_da_jvm "$(cat "$T/vm.txt")" "-Dantes=1"
  igual ${n}_nao_imprime_nada "$(wc -c < "$T/saida.txt" | tr -d ' ')" 0
done

# 6. O resumo do fim da rodada: o que a HMI registrou ter aplicado (a última troca é a que vale)
resumo() {   # resumo <HMILOG ou "-">: roda o bloco do resumo sobre $T/out/t.out
  { cat "$T/c.sh" "$T/resumo.sh"; } > "$T/roda.sh"
  if [ "$1" = - ]; then ( unset HMILOG; OUT=$T/out TAG=t sh "$T/roda.sh" ) 2>&1; else OUT=$T/out TAG=t HMILOG="$1" sh "$T/roda.sh" 2>&1; fi
}
mkdir -p "$T/out"
printf '%s\n' '100  TRACE [HSM|DSI.CAR] a' '200 NORMAL [Lateload|LOGGING.LOGMANAGER] Changed logging properties to: {}' \
  '300  TRACE [HSM|DSI.CAR] b' '400 NORMAL [HSM|LOGGING.LOGMANAGER] Changed logging properties to: {DSI.CAR=true}' '500   INFO [HSM|X.Y] TRACE no meio do texto' > "$T/out/t.out"
igual resumo_diz_as_chaves_aplicadas "$(resumo "normal DSI.CAR")" "[m1_hmi] registro detalhado: 2 linhas TRACE; chaves que a HMI aplicou na carga tardia: {DSI.CAR=true}"
printf '%s\n' '100  TRACE [HSM|DSI.CAR] a' > "$T/out/t.out"
case "$(resumo "normal DSI.CAR")" in
  "[m1_hmi] registro detalhado: 1 linhas TRACE; chaves que a HMI aplicou na carga tardia: não houve troca"*) ok resumo_sem_troca ;;
  *) nok resumo_sem_troca "veio '$(resumo "normal DSI.CAR")'" ;;
esac
igual resumo_so_com_hmilog "$(resumo -)" ""

# 7. Os perfis de verdade, quando o firmware está montado em /fw: todos ganham as chaves no fim
#    da seção LOGMANAGER e mais nada muda (o 070, que o firmware traz sem o END PROBES, também)
F=/fw/vw/hmi/config
if [ -f "$F/logging.properties" ] && ls "$F"/debugprobes_*.config > /dev/null 2>&1; then
  rm -rf "$T/low" "$T/antes" "$T/montagens.txt"; mkdir -p "$C" "$T/low/tmp"; : > "$T/montagens.txt"
  cp "$F/logging.properties" "$F"/debugprobes_*.config "$C/"; cp -R "$C" "$T/antes"
  total=$(ls "$T/antes"/debugprobes_*.config | wc -l | tr -d ' ')
  roda "normal DSI.CAR_TIME_UNITS_LANGUAGE APPS.LIFECYCLE"
  igual firmware_sai_com_zero_sem_aviso "$RC:$(wc -c < "$T/saida.txt" | tr -d ' ')" "0:0"
  igual firmware_monta_todos_os_perfis "$(grep -c 'debugprobes_' "$T/montagens.txt")" "$total"
  ruins=
  for a0 in "$T/antes"/debugprobes_*.config; do
    b=$(basename "$a0")
    esperado "$a0" DSI.CAR_TIME_UNITS_LANGUAGE APPS.LIFECYCLE
    cmp -s "$T/esperado.config" "$C/$b" && [ "$(diff "$a0" "$C/$b" | grep -c '^>')" = 2 ] || ruins="$ruins $b"
  done
  igual firmware_cada_perfil_so_ganha_as_chaves "$ruins" ""
  igual firmware_logging_properties_so_troca_as_chaves "$(diff "$T/antes/logging.properties" "$C/logging.properties" | grep '^>' | tr '\n' ';')" "> APPS.LIFECYCLE = true;> DSI.CAR_TIME_UNITS_LANGUAGE = true;"
else
  echo "(sem o firmware em /fw: os casos dos 33 perfis de verdade não rodaram)"
fi

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
