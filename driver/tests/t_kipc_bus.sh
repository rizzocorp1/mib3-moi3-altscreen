#!/bin/sh
# Teste da leitura do barramento pelo kipc_bus.so (12/10/2026).
# O firmware lê o barramento num laço sem pausa: kipc::CommunicationService::read zera 26 kB,
# chama ioctl(fd, 0, descritor) e, se a volta não for 0, tenta de novo na hora
# (frame::Frame::event_checker_kipc). Ele conta com uma leitura que segura a thread até haver
# mensagem. Até 12/10/2026 a biblioteca voltava na hora com -1/EAGAIN, e cada processo que a
# carrega (a HMI e os quatro daemons do firmware) gastava um núcleo inteiro girando, mesmo com a
# central parada. A leitura passa a esperar a mensagem, com prazo. O que este teste fixa:
#   - um leitor parado quase não gasta processador, e a leitura continua voltando de vez em
#     quando sem mensagem (o laço do firmware só avalia a batida de vida dele quando ela volta);
#   - sem mensagem a volta é a de sempre: -1 com EAGAIN, e o descritor fica como estava;
#   - as mensagens chegam iguais, inteiras e na mesma ordem: uma rajada, mensagens espaçadas,
#     a de tamanho máximo;
#   - a mensagem acorda o leitor na hora em que chega: ele não fica esperando o prazo acabar;
#   - o que já existia continua igual: o que um escritor põe no barramento, e o ioctl de
#     qualquer descritor que não seja do barramento.
# Roda na imagem moi3-bancada:1; não precisa do firmware.
#   t_kipc_bus.sh <pasta com kipc_bus.so, kipc_hub e dsi_escuta>   (a do m1_build.sh)
# O cliente de teste é compilado aqui mesmo. Ele usa o barramento como o firmware usa: abre
# /dev/csr e /dev/css com open() e fala por ioctl(fd, 0, descritor), com a biblioteca carregada
# por LD_PRELOAD. O descritor é o MsgPkt: canal em 0x00, tamanho em 0x10, ponteiro em 0x18.
BIN=${1:-/work/bin}; T=/tmp/t_kipc_bus; SOCK=/run/kipc_hub.sock
rm -rf $T; mkdir -p $T
pass=0; fail=0
ok()  { pass=$((pass+1)); echo "PASSOU $1"; }
nok() { fail=$((fail+1)); echo "FALHOU $1: $2"; }
igual() { if [ "$2" = "$3" ]; then ok "$1"; else nok "$1" "esperado '$3', veio '$2'"; fi; }
campo() { sed -n "s/.*[ ^]$1=\([^ ]*\).*/\1/p" "$2" | head -n 1; }      # campo <nome> <arquivo de resumo>
hub() { rm -f $SOCK; "$BIN/kipc_hub" 2>"$1" & HUB=$!; i=0; while [ ! -S $SOCK ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done; }
fim_hub() { kill $HUB 2>/dev/null; wait $HUB 2>/dev/null; rm -f $SOCK; }

cat > $T/cliente.c <<'EOF'
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
struct msgpkt { uint64_t canal, id; uint32_t len, sobra; char *carga; };   /* 0x00, 0x08, 0x10, 0x18 */
static uint64_t ns(clockid_t c) { struct timespec t; clock_gettime(c, &t); return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
static void poe32(char *p, uint32_t v) { memcpy(p, &v, 4); }
/* a carga de teste: [interface:4][sequência:4][0:4] e depois, na interface 501, a hora do envio
 * em ns; o resto é enchimento que depende da sequência e da posição */
static void monta(char *b, uint32_t ifc, uint32_t seq, uint32_t tam) {
  uint32_t k;
  for (k = 0; k < tam; k++) b[k] = (char)((seq + k) & 0xff);
  poe32(b, ifc); poe32(b + 4, seq); poe32(b + 8, 0);
}
/* leitor <segundos parado> <segundos no máximo> <saída>: lê como o firmware lê, até chegar uma
 * mensagem da interface 999 ou o tempo acabar. Nos primeiros <segundos parado> o teste não manda
 * nada: ali se mede quantas vezes a leitura voltou vazia e quanto processador o laço gastou. */
static int leitor(double parado, double maximo, const char *saida) {
  static char carga[0x6800];
  char nome[512];
  struct msgpkt d;
  long vazias = 0, vazias_parado = -1, outras = 0, mexeu = 0, n = 0;
  uint64_t t0, c0, cpu_parado = 0, t_parado = 0;
  FILE *fm, *fa, *fr;
  int fd = open("/dev/csr", O_RDONLY, 0777);
  if (fd < 0) { fprintf(stderr, "cliente: open(/dev/csr) falhou (%s): a biblioteca não está carregada?\n", strerror(errno)); return 3; }
  snprintf(nome, sizeof nome, "%s.msgs", saida); fm = fopen(nome, "w");
  snprintf(nome, sizeof nome, "%s.atrasos", saida); fa = fopen(nome, "w");
  if (!fm || !fa) return 3;
  t0 = ns(CLOCK_MONOTONIC); c0 = ns(CLOCK_PROCESS_CPUTIME_ID);
  for (;;) {
    uint64_t t = ns(CLOCK_MONOTONIC) - t0, rx;
    uint32_t ifc = 0, seq = 0, k;
    int r;
    if (vazias_parado < 0 && t >= (uint64_t)(parado * 1e9)) { vazias_parado = vazias; cpu_parado = ns(CLOCK_PROCESS_CPUTIME_ID) - c0; t_parado = t; }
    if (t >= (uint64_t)(maximo * 1e9)) break;
    d.canal = 0x1234; d.id = 0; d.len = 0; d.sobra = 0; d.carga = carga;
    memset(carga, 0, sizeof carga);                    /* como o CommunicationService::read */
    errno = 0;
    r = ioctl(fd, 0, &d);
    if (r != 0) {
      vazias++;
      if (r != -1 || errno != EAGAIN) outras++;
      if (d.canal != 0x1234 || d.len != 0 || d.carga != carga) mexeu++;
      continue;
    }
    rx = ns(CLOCK_MONOTONIC);
    n++;
    if (d.len >= 8) { memcpy(&ifc, carga, 4); memcpy(&seq, carga + 4, 4); }
    if (ifc == 999) break;
    if (ifc == 501 && d.len >= 20) {                   /* com a hora do envio: só o que não varia */
      uint64_t tx; memcpy(&tx, carga + 12, 8);
      fprintf(fm, "canal=0x%llx len=%u if=501 seq=%u\n", (unsigned long long)d.canal, d.len, seq);
      fprintf(fa, "%lld\n", (long long)(rx - tx) / 1000);
    } else if (d.len <= 64) {
      fprintf(fm, "canal=0x%llx len=%u pb=", (unsigned long long)d.canal, d.len);
      for (k = 0; k < d.len; k++) fprintf(fm, k + 1 < d.len ? "%02x " : "%02x", (unsigned char)carga[k]);
      fprintf(fm, "\n");
    } else {
      unsigned long soma = 0;
      for (k = 0; k < d.len; k++) soma += (unsigned char)carga[k];
      fprintf(fm, "canal=0x%llx len=%u if=%u seq=%u soma=%lu\n", (unsigned long long)d.canal, d.len, ifc, seq, soma);
    }
  }
  fclose(fm); fclose(fa);
  if (vazias_parado < 0) { vazias_parado = vazias; cpu_parado = ns(CLOCK_PROCESS_CPUTIME_ID) - c0; t_parado = ns(CLOCK_MONOTONIC) - t0; }
  snprintf(nome, sizeof nome, "%s.resumo", saida); fr = fopen(nome, "w");
  if (!fr) return 3;
  fprintf(fr, " vazias_parado=%ld janela_ms=%llu cpu_us=%llu vazias=%ld outras=%ld mexeu=%ld mensagens=%ld\n", vazias_parado,
          (unsigned long long)(t_parado / 1000000), (unsigned long long)(cpu_parado / 1000), vazias, outras, mexeu, n);
  fclose(fr);
  return 0;
}
/* escritor <canal> <interface> <quantas> <intervalo ms> <tamanho>: escreve como o firmware escreve */
static int escritor(uint64_t canal, uint32_t ifc, int quantas, int intervalo, uint32_t tam) {
  static char carga[0x6800];
  struct msgpkt d;
  int i, erros = 0, fd = open("/dev/css", O_WRONLY, 0777);
  if (fd < 0) { fprintf(stderr, "cliente: open(/dev/css) falhou (%s)\n", strerror(errno)); return 3; }
  if (tam > sizeof carga) tam = sizeof carga;
  for (i = 0; i < quantas; i++) {
    monta(carga, ifc, (uint32_t)i, tam);
    d.canal = canal; d.id = 0; d.len = tam; d.sobra = 0; d.carga = carga;
    if (ifc == 501 && tam >= 20) { uint64_t tx = ns(CLOCK_MONOTONIC); memcpy(carga + 12, &tx, 8); }
    if (ioctl(fd, 0, &d) != 0) erros++;
    if (intervalo) usleep(intervalo * 1000);
  }
  close(fd);
  return erros ? 4 : 0;
}
/* outros: o ioctl e o open de quem não é do barramento têm de passar direto, com o errno da libc */
static int outros(void) {
  int p[2], n = -1, r, fd, ruim = 0;
  struct msgpkt d; memset(&d, 0, sizeof d);
  if (pipe(p) < 0 || write(p[1], "abcde", 5) != 5) return 3;
  r = ioctl(p[0], FIONREAD, &n);
  if (r != 0 || n != 5) { printf("FIONREAD num pipe: voltou %d com %d bytes\n", r, n); ruim = 1; }
  errno = 0; r = ioctl(-1, 0, &d);
  if (r != -1 || errno != EBADF) { printf("ioctl(-1, 0): voltou %d, errno %d\n", r, errno); ruim = 1; }
  errno = 0; r = ioctl(p[0], 0, &d);                  /* pedido 0 num descritor que não é do barramento */
  if (r != -1 || (errno != ENOTTY && errno != EINVAL)) { printf("ioctl(pipe, 0): voltou %d, errno %d\n", r, errno); ruim = 1; }
  errno = 0; fd = open("/nao/existe/csr-nao", O_RDONLY, 0);
  if (fd != -1 || errno != ENOENT) { printf("open de arquivo que não existe: voltou %d, errno %d\n", fd, errno); ruim = 1; }
  fd = open("/dev/csr", O_RDONLY, 0777);
  if (fd < 0) { printf("open(/dev/csr): %s\n", strerror(errno)); return 1; }
  close(fd);
  errno = 0; r = ioctl(fd, 0, &d);                    /* depois de fechado deixa de ser do barramento */
  if (r != -1 || errno != EBADF) { printf("ioctl no descritor já fechado: voltou %d, errno %d\n", r, errno); ruim = 1; }
  if (!ruim) printf("tudo certo\n");
  return ruim;
}
int main(int argc, char **argv) {
  if (argc >= 5 && !strcmp(argv[1], "leitor")) return leitor(atof(argv[2]), atof(argv[3]), argv[4]);
  if (argc >= 7 && !strcmp(argv[1], "escritor")) return escritor(strtoull(argv[2], 0, 0), (uint32_t)atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), (uint32_t)atoi(argv[6]));
  if (argc >= 2 && !strcmp(argv[1], "outros")) return outros();
  fprintf(stderr, "uso: cliente leitor|escritor|outros …\n"); return 2;
}
EOF
gcc -O2 -Wall -U_FORTIFY_SOURCE -o $T/cliente $T/cliente.c 2>$T/cliente-build.log || { echo "FALHOU compilacao_do_cliente_de_teste: $(head -n 3 $T/cliente-build.log | tr '\n' ' ')"; echo "RESUMO: 0 passaram, 1 falharam"; exit 1; }
C="env LD_PRELOAD=$BIN/kipc_bus.so $T/cliente"

# ---- 1. um leitor ligado a um barramento calado por 2 s; depois o tráfego.
hub $T/hub1.log
$C leitor 2 20 $T/l1 2>$T/l1.err & LEITOR=$!
"$BIN/dsi_escuta" $SOCK > $T/escuta.log 2>&1 & ESC=$!     # ouvinte direto, sem a biblioteca
sleep 2.5
$C escritor 0x20 500 200 0 24 2>$T/e1.err; e1=$?          # uma rajada de 200, sem intervalo
sleep 0.3
$C escritor 0x40 501 20 120 20 2>$T/e2.err                # 20 com a hora do envio, uma a cada 120 ms
$C escritor 0x80 502 3 50 26624 2>$T/e3.err               # 3 do tamanho máximo (0x6800)
$C escritor 0x20 999 1 0 12 2>$T/e4.err                   # a que encerra o leitor
i=0; while kill -0 $LEITOR 2>/dev/null && [ $i -lt 100 ]; do sleep 0.1; i=$((i+1)); done
if kill -0 $LEITOR 2>/dev/null; then kill $LEITOR 2>/dev/null; nok leitor_termina_com_a_ultima_mensagem "seguia no ar 10 s depois"; else ok leitor_termina_com_a_ultima_mensagem; fi
wait $LEITOR 2>/dev/null; sleep 0.2; kill $ESC 2>/dev/null; wait $ESC 2>/dev/null
R=$T/l1.resumo
[ -f $R ] || { echo " vazias_parado=0 janela_ms=1 cpu_us=0 vazias=0 outras=-1 mexeu=-1 mensagens=0" > $R; nok leitor_deixou_o_resumo "$(head -n 2 $T/l1.err | tr '\n' ' ')"; }

vz=$(campo vazias_parado $R); jan=$(campo janela_ms $R); cpu=$(campo cpu_us $R)
pct=$(awk -v c="$cpu" -v j="$jan" 'BEGIN { printf "%.2f", c / (j * 10) }')          # % de um núcleo
pors=$(awk -v v="$vz" -v j="$jan" 'BEGIN { printf "%.1f", v * 1000 / j }')          # voltas vazias por segundo
if awk -v p="$pct" 'BEGIN { exit !(p < 10) }'; then ok leitor_parado_quase_nao_gasta_processador
else nok leitor_parado_quase_nao_gasta_processador "gastou $pct % de um núcleo em $jan ms parado (limite: 10 %)"; fi
if awk -v s="$pors" 'BEGIN { exit !(s <= 100) }'; then ok leitor_parado_nao_fica_girando
else nok leitor_parado_nao_fica_girando "a leitura voltou vazia $vz vezes em $jan ms ($pors por segundo; limite: 100)"; fi
if awk -v s="$pors" 'BEGIN { exit !(s >= 1) }'; then ok leitura_sem_mensagem_continua_voltando
else nok leitura_sem_mensagem_continua_voltando "a leitura voltou vazia $vz vezes em $jan ms; tem de voltar ao menos uma vez por segundo"; fi
igual leitura_sem_mensagem_devolve_menos_um_e_EAGAIN "$(campo outras $R) voltas de outro jeito" '0 voltas de outro jeito'
igual leitura_sem_mensagem_nao_mexe_no_descritor "$(campo mexeu $R) vezes" '0 vezes'

# a rajada: as 200, iguais e na ordem. A carga esperada sai da mesma regra do cliente.
awk 'BEGIN { for (s = 0; s < 200; s++) { printf "canal=0x20 len=24 pb=f4 01 00 00 %02x 00 00 00 00 00 00 00", s; for (k = 12; k < 24; k++) printf " %02x", (s + k) % 256; printf "\n" } }' > $T/esperado-rajada.txt
sed -n '1,200p' $T/l1.msgs > $T/veio-rajada.txt
if [ $e1 = 0 ] && cmp -s $T/esperado-rajada.txt $T/veio-rajada.txt; then ok rajada_chega_inteira_e_na_ordem
else nok rajada_chega_inteira_e_na_ordem "vieram $(grep -c 'canal=0x20 len=24 ' $T/l1.msgs) das 200; primeira diferença: $(diff $T/esperado-rajada.txt $T/veio-rajada.txt | sed -n '2p;3p' | cut -c1-90 | tr '\n' ' ')"; fi
awk 'BEGIN { for (s = 0; s < 20; s++) print "canal=0x40 len=20 if=501 seq=" s }' > $T/esperado-espacadas.txt
sed -n '201,220p' $T/l1.msgs > $T/veio-espacadas.txt
if cmp -s $T/esperado-espacadas.txt $T/veio-espacadas.txt; then ok mensagens_espacadas_chegam_na_ordem
else nok mensagens_espacadas_chegam_na_ordem "vieram $(grep -c ' if=501 ' $T/l1.msgs) das 20, ou fora de ordem"; fi
# a de tamanho máximo: 26624 bytes; a soma esperada sai da mesma regra de enchimento
awk 'BEGIN { for (s = 0; s < 3; s++) { soma = 0; for (k = 12; k < 26624; k++) soma += (s + k) % 256; soma += 246 + 1 + s; print "canal=0x80 len=26624 if=502 seq=" s " soma=" soma } }' > $T/esperado-grandes.txt
sed -n '221,223p' $T/l1.msgs > $T/veio-grandes.txt
if cmp -s $T/esperado-grandes.txt $T/veio-grandes.txt; then ok mensagem_de_tamanho_maximo_chega_inteira
else nok mensagem_de_tamanho_maximo_chega_inteira "veio: $(tr '\n' ';' < $T/veio-grandes.txt | cut -c1-160)"; fi
igual leitor_recebe_cada_mensagem_uma_vez "$(campo mensagens $R) mensagens" '224 mensagens'

# a mensagem acorda o leitor: o atraso entre o envio e a entrega é de frações de milésimo de
# segundo, e não o prazo de espera da leitura. Conta a mediana, que não sofre com um soluço da máquina.
sort -n $T/l1.atrasos > $T/atrasos-ordenados.txt
med=$(sed -n '10p' $T/atrasos-ordenados.txt); max=$(tail -n 1 $T/atrasos-ordenados.txt)
if [ -n "$med" ] && [ "$med" -lt 20000 ]; then ok mensagem_acorda_o_leitor_sem_esperar_o_prazo
else nok mensagem_acorda_o_leitor_sem_esperar_o_prazo "atraso mediano de ${med:-?} µs entre o envio e a entrega (limite: 20000)"; fi
echo "MEDIDA leitor parado: $pct % de um núcleo e $pors voltas vazias por segundo, em $jan ms; atraso do envio à entrega: mediana $med µs, máximo $max µs (20 mensagens)"

# o que o escritor põe no barramento não muda: o ouvinte direto vê [canal][carga], como sempre
awk 'BEGIN { for (s = 0; s < 200; s++) { printf "canal=0x20 if=500 inst=%d api=0 pb=", s; for (k = 12; k < 24; k++) printf (k < 23 ? "%02x " : "%02x"), (s + k) % 256; printf "\n" } }' > $T/esperado-escuta.txt
grep '^canal=0x20 if=500 ' $T/escuta.log > $T/veio-escuta.txt
if cmp -s $T/esperado-escuta.txt $T/veio-escuta.txt; then ok escritor_poe_no_barramento_o_mesmo_de_sempre
else nok escritor_poe_no_barramento_o_mesmo_de_sempre "o ouvinte direto viu $(wc -l < $T/veio-escuta.txt | tr -d ' ') das 200; primeira diferença: $(diff $T/esperado-escuta.txt $T/veio-escuta.txt | sed -n '2p;3p' | cut -c1-90 | tr '\n' ' ')"; fi
fim_hub

# ---- 2. quem não é do barramento passa direto, com o errno da libc.
hub $T/hub2.log
igual ioctl_e_open_de_outros_descritores_passam_direto "$($C outros 2>/dev/null | tr '\n' ';')" 'tudo certo;'

# ---- 3. o hub cai com o leitor esperando: a leitura não pode prender a thread para sempre,
# porque o laço do firmware só olha a ordem de parar entre uma leitura e outra.
$C leitor 0.2 1.5 $T/l3 2>$T/l3.err & LEITOR=$!
sleep 0.5; fim_hub
i=0; while kill -0 $LEITOR 2>/dev/null && [ $i -lt 40 ]; do sleep 0.1; i=$((i+1)); done
if kill -0 $LEITOR 2>/dev/null; then kill -9 $LEITOR 2>/dev/null; nok leitor_nao_fica_preso_quando_o_hub_cai "seguia preso 3 s depois do prazo dele"; else ok leitor_nao_fica_preso_quando_o_hub_cai; fi
wait $LEITOR 2>/dev/null

echo "RESUMO: $pass passaram, $fail falharam"
[ $fail -eq 0 ] || { echo "--- resumo do leitor ---"; cat $T/l1.resumo; echo "--- erros do leitor ---"; head -n 5 $T/l1.err; echo "--- hub ---"; tail -n 4 $T/hub1.log; }
[ $fail -eq 0 ]
