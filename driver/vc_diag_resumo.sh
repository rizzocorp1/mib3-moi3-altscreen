#!/bin/sh
# vc_diag_resumo.sh <pasta de saída> <tag> [<tag>...] — resumo de rodadas do vc_rodada.sh no que
# toca ao diagnóstico que o navi-al (alm) recebe e ao que ele repassa ao motor de navegação:
# cabeçalho dos conjuntos no barramento, tipos de dado que o alm reconheceu, o que ele mandou ao
# motor, e o que o motor disse sobre região, submercado e banco de mapas.
# Investigação do painel de instrumentos, item 3 (11/10/2026). Só lê os arquivos da rodada.
# Aceita os nomes da pasta de saída (<tag>.alm.out) e os de traces/ (vc_<tag>.alm.out).
O=$1; [ -d "$O" ] && [ $# -ge 2 ] || { echo "uso: vc_diag_resumo.sh <pasta de saída> <tag> [<tag>...]"; exit 2; }
shift
conta() { grep -c -- "$1" "$2" 2>/dev/null || true; }
for T in "$@"; do
  P=$O/$T; [ -f "$P.alm.out" ] || P=$O/vc_$T
  [ -f "$P.alm.out" ] || { echo "===== $T: sem $O/$T.alm.out"; continue; }
  A=$P.alm.out; N=$P.navi.out; B=$P.barramento.log
  echo "===== $T"
  [ -f "$P.diagalm.log" ] && echo "vc_diag_alm: $(tail -n 1 "$P.diagalm.log")" || echo "vc_diag_alm: não rodou"
  echo "barramento, conjuntos de diagnóstico por cabeçalho: $(grep -o 'if=1901 cab=.. .. ..' "$B" | sort | uniq -c | sed 's/if=1901 //' | tr -s ' ' | tr '\n' ';')"
  echo "alm, 'Data Type : Unknown': $(conta 'Data Type : Unknown' "$A")  (por linha do fonte: $(grep -A1 'Data Type : Unknown' "$A" | grep 'MainCmd' | awk '{print $8}' | sort | uniq -c | tr -s ' ' | tr '\n' ';'))"
  echo "alm, tipos reconhecidos:"
  grep 'Data Type : ' "$A" | grep -v Unknown | sed 's/.*Data Type : /    /; s/ ~$//' | sort | uniq -c
  echo "alm, campos do 0558 (Function Configuration Navigation):"
  grep -A 11 'Data Type : Function Configuration Navigation' "$A" | grep 'procDiagMsg' | sed 's/.*procDiagMsg *[0-9]* *//' | grep -v 'Data Type' | tr -s ' ' | tr '\n' ';'; echo
  echo "alm, o que guardou dos outros conjuntos:"
  grep -h 'StubLoader : save\|\[CarCoding\]: rawdata\|Country of Sale : \|Map Switch : \|Fuel Type : \|pso: \|rollmode : \|Vze Activation : \|DeveloperTestMode\] : ' "$A" | sed 's/^AL MSG_. [0-9-]* [0-9:.]* [^ ]* [A-Za-z_]* *[0-9]* *//' | sort | uniq -c | sed 's/^/   /'
  echo "alm para o motor (chamadas ao serviço Diagnosis; cs=[0] é sucesso):"
  grep -oh 'm_proxyDiag->set[A-Za-z]*([^)]*) cs=\[[0-9]*\]' "$A" | sort | uniq -c | sed 's/^/   /'
  echo "motor, linhas sobre região, submercado e banco de mapas:"
  for k in 'unknown CountryCoding' 'Waiting for country coding info' 'Country coding info is available now' \
           'Country Coding was already set' 'Determined submarket path as' 'No submarket mapping' \
           'Setting countryOfSale' 'No mapping for devices market coding' 'DB root file does not exist' \
           'No compatible DAL' 'Cleanup of foreign submarket folder'; do
    c=$(conta "$k" "$N"); [ "$c" != 0 ] && echo "   $c x '$k': $(grep -m1 -- "$k" "$N" | cut -f4- | cut -c1-230)"
  done
  echo "motor, o que recebeu da codificação:"
  grep -h 'CodingModule : setCarType\|CodingModule : setSwitchMapEnabled\|CodingModule : setCountryCoding :\|CodingModule : setCountryOfSale\|CodingModule : setGnssReceiverCoding\|CodingModule : setFuelType\|CodingModule : setNavEnabled : to\|CodingModule : setPsdEnabled : to\|CodingModule : setTrafficEnabled\|CodingModule : setVz[aeo]Enabled' "$N" \
    | cut -f4- | sed 's/CodingModule : //' | sort | uniq -c | sort -rn | head -n 24 | sed 's/^/   /'
  echo "motor, linha 'R4N : nav' (a que o item 3 pede): $(grep -o 'R4N : nav : .*' "$N" | sort | uniq -c | tr -s ' ' | tr '\n' ';')"
  echo "navsystem pelo RSI no fim: $(grep -o '"name":"mainunit".*' "$P.rsi.txt" | grep -o '"status":"[A-Za-z0-9]*"' | head -n 1) $(grep -o '"databaseStatus":"[A-Za-z0-9]*"' "$P.rsi.txt" | tr '\n' ' ')"
  echo "serviços RSI do motor registrados: $(sed -n '/=== registro/,/=== GET/p' "$P.rsi.txt" | grep -E '^(navsystem|maprenderer|guidance|homeview|navdata|places|routing|traffic|tripview) ' | tr '\n' ';')"
  echo "motor, o que procurou em /var/lib/nav (strace): $(grep -o '"/var/lib/nav[^"]*"[^=]*= [-0-9A-Z a-z()]*' "$P.navi.strace" | sed 's/, [A-Z_|]*) *=/ =/; s/, [A-Z_|0-9]*, [A-Z_|0-9]*) *=/ =/' | sort | uniq -c | tr -s ' ' | head -n 12 | tr '\n' ';')"
  echo "de pé no fim: $(grep -E '^(alm|tsd.nav.mainapp|rsi-display-management|navsd_app|multi-screen-server|java|persistence-app): ' "$P.vivos.txt" | tr '\n' ' ')"
  [ -f "$P.roda.txt" ] && grep -h '\[m1_hmi\] diagnóstico\|\[m1_hmi\] tela Welcome\|\[m1_hmi\] falhas\|\[m1_hmi\] rc=' "$P.roda.txt" | cut -c1-330 | sed 's/^/HMI: /'
done
