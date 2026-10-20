#!/bin/sh
# Teste do leitor da memória de estado (driver/lsmval_abre.py, Marco 6, 11/10/2026): ele desfaz o
# embrulho de cada valor do lsmval (compressão ou soma), diz de quem é a chave e, nos valores
# identificados, o nome de cada campo; com dois arquivos, compara valor a valor.
# Não precisa do firmware nem do container. Precisa de python3, que a imagem moi3-bancada:1 não
# tem: lá o teste avisa e sai sem falhar. Roda no Mac, de qualquer pasta:
#   sh referencia/ferramentas/mib3-sandbox/driver/tests/t_lsmval_abre.sh
# O formato vem do jar da HMI: PersistenceKeyUtil (a chave), Compression e RecordByteArray (o
# embrulho) e os serializadores de cada módulo (os campos).
D=$(cd "$(dirname "$0")/.." && pwd); A="$D/lsmval_abre.py"
command -v python3 >/dev/null 2>&1 || { echo "PULOU: sem python3 nesta máquina (o leitor roda no Mac)"; exit 0; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
tem()  { if printf '%s\n' "$2" | grep -q -- "$3"; then ok "$1"; else nok "$1" "sem /$3/ em: $(printf '%s' "$2" | head -n 3)"; fi; }
nao_tem() { if printf '%s\n' "$2" | grep -q -- "$3"; then nok "$1" "apareceu /$3/"; else ok "$1"; fi; }
abre() { python3 -I "$A" "$@" 2>&1; }

# As linhas de teste são as que a própria HMI gravou (rodadas da quarta parte, 11/10/2026).
# 5051, persistable 1, usuário 1 (soma Adler): brilho 02, hora em standby ligada, última tela HMI_RADIO
SC='0 5051 4311744512 55 66 77 88 0 0 0 0 39 224 2 194 0 1 2 1 0 0 0 0 0 4 0 0 0 0 1 0 1 1 1 0 0 0 0 9 72 77 73 95 82 65 68 73 79 0 0 0 0 0 0 0 0 0'
# 5056 Sound, persistable 2, usuário 1 (soma Adler): tom do touchscreen desligado
SO='0 5056 8606711808 55 66 77 88 0 0 0 0 0 46 0 5 0 1 0 0 0 1 0 0 0 0 0 1 1 0 0'
# 5000/200, 6000/100 e o idioma: chaves antigas, sem módulo
ID='0 1101 10 112 116 95 66 82 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32 32'
SS='0 6000 100 0 6 0 0'
printf '%s\n%s\n%s\n%s\n' "$ID" "$SC" "$SO" "$SS" > "$T/a.txt"

# 1. De quem é cada chave
S1=$(abre "$T/a.txt"); rc=$?
igual abre_codigo "$rc" 0
tem chave_usuario   "$S1" '5051 SystemCommon.*persistable 1, usuário 1'
tem chave_do_som    "$S1" '5056 Sound.*persistable 2, usuário 1'
tem idioma          "$S1" 'idioma.*pt_BR'
tem soma_conferida  "$S1" 'soma certa'
nao_tem sem_soma_errada "$S1" 'SOMA ERRADA'

# 2. Os campos dos valores identificados
tem campo_brilho    "$S1" 'displayBrightness.*= 2'
tem campo_relogio   "$S1" 'showClock.*= sim'
tem campo_tela      "$S1" 'lastContentId.*= HMI_RADIO'
tem campo_tom       "$S1" 'touchscreenToneOn.*= não'
tem campo_eq        "$S1" 'eqMode.*= 1'

# 3. Chave comum, declaração e módulo desconhecido
printf '0 5056 12868124672 55 66 77 88 0 0 0 0 0 11 0 2 0 1 0 0 0 0\n0 5077 0 1 2 3\n0 4242 7 1 2 3\n' > "$T/b.txt"
S3=$(abre "$T/b.txt")
tem chave_comum      "$S3" '5056 Sound.*persistable 2, comum'
tem chave_declaracao "$S3" '5077 SimpleGridMenu.*declaração'
tem modulo_desconhecido "$S3" '4242 ?.*chave 7'

# 4. Soma errada aparece, e o valor não é dado por bom
printf '0 5056 8606711808 55 66 77 88 0 0 0 0 0 46 0 6 0 1 0 0 0 1 0 0 0 0 0 1 1 0 0\n' > "$T/c.txt"
S4=$(abre "$T/c.txt")
tem soma_errada "$S4" 'SOMA ERRADA'

# 5. Valor comprimido: gzip de (tamanho + dados), com o cabeçalho 11 22 33 44
python3 -I - "$T/g.txt" <<'PY'
import sys, gzip, io, struct
dados = bytes([0, 1]) + bytes(60)                      # versão 1 e 60 zeros: comprime
b = io.BytesIO(); g = gzip.GzipFile(fileobj=b, mode='wb', mtime=0); g.write(struct.pack('>i', len(dados)) + dados); g.close()
v = bytes([11, 22, 33, 44]) + b.getvalue()
open(sys.argv[1], 'w').write('0 5068 0 ' + ' '.join(str(x) for x in v) + '\n')
PY
S5=$(abre "$T/g.txt")
tem comprimido "$S5" 'comprimido'
tem comprimido_tamanho "$S5" 'conteúdo 62'

# 6. Comparação valor a valor: a ordem das linhas não conta; um byte trocado aparece com a
#    posição e o nome do campo
printf '%s\n%s\n%s\n%s\n' "$SS" "$SO" "$SC" "$ID" > "$T/a2.txt"
S6=$(abre "$T/a.txt" "$T/a2.txt"); rc=$?
igual ordem_nao_conta_codigo "$rc" 0
tem ordem_nao_conta "$S6" 'nenhum valor mudou'
# comentário e espaço a mais não são mudança (o lsmval_prepara.sh os tira antes de a central ler)
{ echo '# memória de teste'; printf '%s   # o idioma\n%s\n%s\n0  6000   100 0 6 0 0\n' "$ID" "$SC" "$SO"; } > "$T/a3.txt"
S6b=$(abre "$T/a.txt" "$T/a3.txt"); rc=$?
igual comentario_nao_conta_codigo "$rc" 0
tem comentario_nao_conta "$S6b" 'nenhum valor mudou'
SO1='0 5056 8606711808 55 66 77 88 0 0 0 0 0 47 0 6 0 1 0 0 0 1 0 0 0 0 0 1 1 0 1'
printf '%s\n%s\n%s\n%s\n' "$ID" "$SC" "$SO1" "$SS" > "$T/d.txt"
S7=$(abre "$T/a.txt" "$T/d.txt"); rc=$?
igual um_byte_codigo "$rc" 1
tem um_byte_quem    "$S7" '5056 Sound.*persistable 2, usuário 1'
tem um_byte_posicao "$S7" 'posição 14: 00 -> 01'
tem um_byte_campo   "$S7" 'touchscreenToneOn'
nao_tem um_byte_so_um "$S7" '5051'

# 7. Valor que entra e valor que sai
printf '%s\n%s\n' "$ID" "$SC" > "$T/e.txt"
S8=$(abre "$T/a.txt" "$T/e.txt")
tem valor_que_saiu "$S8" 'saiu.*5056 Sound'
S9=$(abre "$T/e.txt" "$T/a.txt")
tem valor_que_entrou "$S9" 'entrou.*6000'

# 8. Mudança de tamanho (a última tela muda o comprimento do valor): mostra o campo que mudou
SC2='0 5051 4311744512 55 66 77 88 0 0 0 0 29 9 2 39 0 1 2 1 0 0 0 0 0 4 0 0 0 0 1 0 1 1 1 0 0 0 0 7 72 77 73 95 67 65 82 0 0 0 0 0 0 0 0 0'
printf '%s\n%s\n%s\n%s\n' "$ID" "$SC2" "$SO" "$SS" > "$T/f.txt"
S10=$(abre "$T/a.txt" "$T/f.txt")
tem tamanho_mudou_campo "$S10" 'lastContentId: HMI_RADIO -> HMI_CAR'

# 9. Muitos bytes de uma vez (uma lista reordenada): resume as posições e mostra o campo inteiro
G1='0 5077 4311744512 55 66 77 88 0 0 0 0 0 249 0 28 0 1 0 0 0 2 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2'
G2='0 5077 4311744512 55 66 77 88 0 0 0 0 1 57 0 28 0 1 0 0 0 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1'
printf '%s\n' "$G1" > "$T/g1.txt"; printf '%s\n' "$G2" > "$T/g2.txt"
S11=$(abre "$T/g1.txt" "$T/g2.txt")
tem lista_resumo "$S11" '16 bytes, da posição 6 à 21'
tem lista_campo  "$S11" 'mainMenuEntryCioIds: 2 itens: 0101010101010101 0202020202020202 -> 2 itens: 0202020202020202 0101010101010101'
nao_tem lista_sem_posicao_a_posicao "$S11" 'posição 6: 01'
tem lista_soma_certa "$(abre "$T/g1.txt")" 'soma certa'

# 10. O arquivo da pasta e arquivo com erro
#    (a mensagem de erro traz o nome do arquivo, que também tem "idioma" e "pt_BR": por isso o
#    caso exige o código 0 e o texto que só a leitura certa produz)
SP=$(abre "$D/../stubs/persistencia/idioma-pt_BR.txt"); rc=$?
igual arquivo_da_pasta_codigo "$rc" 0
tem arquivo_da_pasta "$SP" 'idioma pt_BR, tipo de voz 32'
printf '0 5056 abc 1 2\n' > "$T/x.txt"
abre "$T/x.txt" > "$T/x.out"; rc=$?
igual linha_errada_codigo "$rc" 2
tem linha_errada_mensagem "$(cat "$T/x.out")" 'x.txt, linha 1'
abre "$T/nao-existe.txt" > "$T/y.out"; rc=$?
igual arquivo_ausente_codigo "$rc" 2

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ]
