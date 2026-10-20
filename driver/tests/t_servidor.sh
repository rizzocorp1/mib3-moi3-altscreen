#!/bin/sh
# Teste do servidor do visualizador ao vivo (Marco 5): serve a página e recebe a entrada.
# Roda no Mac, de qualquer pasta:   sh referencia/ferramentas/mib3-sandbox/driver/tests/t_servidor.sh
# Sobe o servidor numa pasta temporária, faz os pedidos com curl e confere as respostas e o
# arquivo de comandos. PORTA muda a porta do teste (default 8439).
D=$(cd "$(dirname "$0")/.." && pwd)
T=$(mktemp -d); PORTA=${PORTA:-8439}; U=http://127.0.0.1:$PORTA; CMD=$T/entrada/comandos.txt
mkdir -p "$T/frames"; echo "<p>pagina</p>" > "$T/index.html"; echo "1 800 480" > "$T/frames/seq.txt"
python3 -I "$D/servidor_ao_vivo.py" "$PORTA" "$T" > "$T/servidor.log" 2>&1 & SRV=$!
trap 'kill $SRV 2>/dev/null; rm -rf "$T"' EXIT
i=0; while ! nc -z 127.0.0.1 "$PORTA" 2>/dev/null && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
codigo() { curl -s -o /dev/null -m 5 -w '%{http_code}' "$@"; }
conteudo() { cat "$CMD" 2>/dev/null; }
manda() { codigo -X POST -H 'X-Moi3-Entrada: 1' "$@"; }

igual pagina_servida "$(curl -s -m 5 $U/)" "<p>pagina</p>"
if curl -s -m 5 -D - -o /dev/null $U/frames/seq.txt | grep -qi '^Last-Modified:'; then ok quadro_vem_com_data
else nok quadro_vem_com_data "sem Last-Modified; a página usa esse cabeçalho para saber a idade do quadro"; fi
# sem isto o navegador guarda a página e segue mostrando a versão antiga depois de uma mudança
if curl -s -m 5 -D - -o /dev/null $U/ | grep -qi '^Cache-Control: no-cache'; then ok pagina_sempre_revalidada
else nok pagina_sempre_revalidada "a resposta não traz Cache-Control: no-cache"; fi

igual entrada_aceita "$(manda --data-binary "$(printf 'tecla 13 78 1\ntecla 13 78 0\n')" $U/entrada)" 204
igual entrada_gravada "$(conteudo)" "$(printf 'tecla 13 78 1\ntecla 13 78 0')"

antes=$(conteudo)
igual sem_cabecalho_recusada "$(codigo -X POST --data-binary 'tecla 13 78 1' $U/entrada)" 403
igual origem_de_outro_site_recusada "$(manda -H 'Origin: http://exemplo.invalid' --data-binary 'tecla 13 78 1' $U/entrada)" 403
igual linha_invalida_recusada "$(manda --data-binary "$(printf 'tecla 13 78 1\nrm -rf /\n')" $U/entrada)" 400
igual palavra_desconhecida_recusada "$(manda --data-binary 'energia 0' $U/entrada)" 400
igual corpo_vazio_recusado "$(manda --data-binary '' $U/entrada)" 400
igual corpo_grande_recusado "$(manda --data-binary "$(head -c 5000 /dev/zero | tr '\0' '1')" $U/entrada)" 413
igual outro_caminho_recusado "$(manda --data-binary 'tecla 13 78 1' $U/qualquer)" 404
igual recusas_nao_gravam_nada "$(conteudo)" "$antes"

igual botao_giratorio_aceito "$(manda --data-binary 'encoder 13 16 -1' $U/entrada)" 204
igual botao_giratorio_gravado "$(tail -n 1 "$CMD")" 'encoder 13 16 -1'
igual botao_giratorio_incompleto_recusado "$(manda --data-binary 'encoder 13 16' $U/entrada)" 400

igual gesto_aceito "$(manda --data-binary "$(printf 'gesto 4 400 240\ngesto 1 400 240 1')" $U/entrada)" 204
igual gesto_gravado "$(tail -n 2 "$CMD")" "$(printf 'gesto 4 400 240\ngesto 1 400 240 1')"
# a segunda linha chegou sem quebra de linha e tem de ganhar uma, senão o dsistub fica esperando
igual ultima_linha_termina_em_quebra "$(tail -c 1 "$CMD" | od -An -c | tr -d ' \n')" '\n'

igual fora_da_pasta_nao_sai "$(codigo --path-as-is "$U/../../../../etc/hosts")" 404

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- servidor ---"; tail -n 15 "$T/servidor.log"; }
[ $fail -eq 0 ]
