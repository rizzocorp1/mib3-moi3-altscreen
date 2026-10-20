/* diagstub.c — faz o papel do microcontrolador de diagnóstico no sandbox (estudo, sem carro).
 * Marco 6, 11/10/2026. Publica no barramento os conjuntos de codificação, adaptação, registros
 * e identificação que a HMI lê na partida, e termina.
 *
 * Como a central recebe esses dados (lido no código do firmware, ver ../modelagem-soc.md):
 *  - O microcontrolador manda um conjunto por mensagem, com o canal PID_DIAG_MCU
 *    (0x0800000000000000). A carga é [cabeçalho: 3 bytes][identificador: 2 bytes, o alto
 *    primeiro][dados]. O identificador é o DID do diagnóstico da unidade: 0x0600 é a
 *    codificação, 0xF187 o número de peça, e assim por diante.
 *  - Na HMI, a libDSIService entrega a carga inteira com a interface 1901 (DIAG_MCU_MSG) e a
 *    camada DSI da LG guarda cada conjunto numa tabela (DSIPersistenceImpl.ValuesFromDiag), mas
 *    SÓ enquanto a HMI ainda não fez nenhuma leitura de persistência. Depois ela responde as
 *    leituras de codificação, identificação, funções do carro e registros direto dessa tabela.
 *  - O persistence-app do firmware recebe as mesmas mensagens (DiagHandler::event_handler) e
 *    passa a responder pelo barramento as leituras que a HMI manda para lá.
 *  - O cabeçalho vai 83 60 22. A HMI e o persistence-app não leem esses bytes; quem lê é o
 *    navi-al do firmware (/lge/app_ro/navi/navi-al/alm, KipcProvider::procDiagMsg): ele só
 *    aceita a mensagem se vierem 0x83, 0x60 e o serviço, que para estes conjuntos é 0x22, e
 *    descarta o resto ("Data Type : Unknown !!!"). Lido no firmware pela sessão do painel de
 *    instrumentos (../painel-de-instrumentos.md, "Item 3"). O que o microcontrolador de
 *    verdade manda não foi visto. Até 11/10/2026 este stub mandava os três bytes zerados.
 *  - O alm copia a mensagem inteira para estruturas de tamanho fixo, sem conferir o tamanho:
 *    nunca mandar um conjunto maior que o de verdade. Os de stubs/codificacao/padrao.txt cabem.
 * Para os conjuntos chegarem antes da primeira leitura, o kipc_hub guarda o que vem neste canal
 * e entrega a cada processo que se liga, antes de qualquer outra mensagem.
 *
 * Arquivos de dados (um ou mais, lidos em ordem; o que vem depois vale sobre o que veio antes),
 * uma linha por item ('#' começa comentário):
 *   <id> = <bytes em hexa>              o conjunto inteiro; <id> é o DID em hexa
 *   <id> @<byte>.<bit>+<bits> = <valor> um campo de um conjunto já definido acima: começa no
 *                                       bit <bit> (0 = o menos significativo) do byte <byte>;
 *                                       até 8 bits cabem num byte só; 16, 24 e 32 bits pedem
 *                                       bit 0 e vão com o byte mais significativo primeiro.
 *                                       <valor> em decimal ou 0x hexa. É como o AccessImpl da
 *                                       HMI lê os campos.
 * Qualquer linha errada recusa tudo: nada é enviado e sai o arquivo e a linha do erro.
 * Os conjuntos vão na ordem em que aparecem pela primeira vez.
 *
 * Estático (-static): roda dentro do chroot sob qualquer glibc.
 * uso: diagstub [-c <canal em hexa>] [-k <cabeçalho>] <socket> <arquivo> [<arquivo>...]
 *      -c troca o canal; existe só para o teste do hub
 *      -k troca o cabeçalho: 3 bytes em hexa colado. 000000 é o de antes de 11/10/2026; serve
 *         para rodada de controle
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PID_DIAG_MCU 0x0800000000000000ULL
#define MAXSETS 128
#define MAXDATA 4096

struct set { unsigned did; int n; unsigned char d[MAXDATA]; };
static struct set sets[MAXSETS];
static int nsets;

static struct set *find(unsigned did)
{
    int i;
    for (i = 0; i < nsets; i++) if (sets[i].did == did) return &sets[i];
    return NULL;
}

static char *trim(char *s)
{
    char *e;
    while (isspace((unsigned char)*s)) s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

/* número sem sinal, decimal ou 0x hexa, ocupando a string inteira */
static int number(const char *s, unsigned long *v)
{
    char *end;
    if (!*s || *s == '-' || *s == '+') return 0;
    errno = 0;
    *v = strtoul(s, &end, 0);
    return !errno && !*end;
}

/* devolve NULL se a linha vale; senão, o que há de errado nela */
static const char *parse_line(char *line)
{
    char *eq, *lhs, *rhs, *at;
    unsigned long did;
    struct set *s;
    if ((eq = strchr(line, '#'))) *eq = 0;
    line = trim(line);
    if (!*line) return NULL;
    if (!(eq = strchr(line, '='))) return "falta o '='";
    *eq = 0;
    lhs = trim(line); rhs = trim(eq + 1);
    if ((at = strchr(lhs, '@'))) *at++ = 0;
    lhs = trim(lhs);
    if (!*lhs || strlen(lhs) > 4 || strspn(lhs, "0123456789abcdefABCDEF") != strlen(lhs)) return "identificador tem de ser hexa, até 4 dígitos";
    did = strtoul(lhs, NULL, 16);
    if (!at) {                                    /* conjunto inteiro */
        unsigned char buf[MAXDATA];
        int n = 0, hi = -1;
        for (; *rhs; rhs++) {
            int v;
            if (isspace((unsigned char)*rhs)) continue;
            if (!isxdigit((unsigned char)*rhs)) return "dado que não é hexa";
            v = isdigit((unsigned char)*rhs) ? *rhs - '0' : (tolower((unsigned char)*rhs) - 'a' + 10);
            if (hi < 0) hi = v;
            else { if (n >= MAXDATA) return "conjunto grande demais"; buf[n++] = (unsigned char)(hi << 4 | v); hi = -1; }
        }
        if (hi >= 0) return "número ímpar de dígitos hexa";
        if (n == 0) return "conjunto vazio";
        if (!(s = find((unsigned)did))) {
            if (nsets >= MAXSETS) return "conjuntos demais";
            s = &sets[nsets++];
            s->did = (unsigned)did;
        }
        s->n = n;
        memcpy(s->d, buf, (size_t)n);
        return NULL;
    } else {                                      /* um campo: @byte.bit+bits = valor */
        unsigned long byte, bit, bits, val;
        char *dot, *plus;
        at = trim(at);
        if (!(dot = strchr(at, '.')) || !(plus = strchr(dot, '+'))) return "campo tem de ser @byte.bit+bits";
        *dot++ = 0; *plus++ = 0;
        if (!number(trim(at), &byte) || !number(trim(dot), &bit) || !number(trim(plus), &bits)) return "campo tem de ser @byte.bit+bits, com números";
        if (!number(rhs, &val)) return "valor tem de ser um número (decimal ou 0x hexa)";
        if (!(s = find((unsigned)did))) return "campo de um conjunto que ainda não foi definido";
        if (bit > 7) return "bit vai de 0 a 7";
        if (bits >= 1 && bits <= 8) {
            unsigned m;
            if (bit + bits > 8) return "campo de até 8 bits não pode passar do fim do byte";
            if (byte >= (unsigned long)s->n) return "byte fora do conjunto";
            if (val >> bits) return "valor não cabe no campo";
            m = (unsigned)(((1u << bits) - 1) << bit);
            s->d[byte] = (unsigned char)((s->d[byte] & ~m) | ((unsigned)val << bit));
        } else if ((bits == 16 || bits == 24 || bits == 32) && bit == 0) {
            unsigned long nb = bits / 8, k;
            if (byte + nb > (unsigned long)s->n) return "campo fora do conjunto";
            if (bits < 32 && (val >> bits)) return "valor não cabe no campo";
            if (val > 0xffffffffUL) return "valor não cabe no campo";
            for (k = 0; k < nb; k++) s->d[byte + k] = (unsigned char)(val >> (8 * (nb - 1 - k)));
        } else return "tamanho de campo: 1 a 8 bits, ou 16, 24 ou 32 começando no bit 0";
        return NULL;
    }
}

int main(int argc, char **argv)
{
    const char *path;
    uint64_t chan = PID_DIAG_MCU;
    char line[3 * MAXDATA + 256], hs[5];
    struct sockaddr_un a;
    int i, s, pid = getpid(), total = 0, arg = 1;
    unsigned char cab[3] = { 0x83, 0x60, 0x22 };    /* o que o alm do firmware aceita; ver acima */
    while (arg < argc && argv[arg][0] == '-') {
        const char *v = arg + 1 < argc ? argv[arg + 1] : "";
        if (!strcmp(argv[arg], "-c") && *v) chan = strtoull(v, NULL, 16);
        else if (!strcmp(argv[arg], "-k")) {
            unsigned k[3];
            if (strlen(v) != 6 || strspn(v, "0123456789abcdefABCDEF") != 6 || sscanf(v, "%2x%2x%2x", &k[0], &k[1], &k[2]) != 3) {
                fprintf(stderr, "[diagstub] -k pede 3 bytes em hexa colado, como 836022; nada foi enviado\n"); return 2;
            }
            cab[0] = (unsigned char)k[0]; cab[1] = (unsigned char)k[1]; cab[2] = (unsigned char)k[2];
        } else { arg = argc; break; }               /* opção desconhecida: cai no uso, abaixo */
        arg += 2;
    }
    if (argc - arg < 2) { fprintf(stderr, "uso: diagstub [-c canal] [-k cabeçalho] <socket> <arquivo> [<arquivo>...]\n"); return 2; }
    path = argv[arg++];
    for (; arg < argc; arg++) {
        const char *file = argv[arg];
        FILE *f = fopen(file, "r");
        int ln = 0;
        if (!f) { fprintf(stderr, "[diagstub] não abri %s: %s; nada foi enviado\n", file, strerror(errno)); return 2; }
        while (fgets(line, sizeof line, f)) {
            const char *err;
            ln++;
            if (!strchr(line, '\n') && !feof(f)) { fprintf(stderr, "[diagstub] %s, linha %d: linha longa demais; nada foi enviado\n", file, ln); return 2; }
            if ((err = parse_line(line))) { fprintf(stderr, "[diagstub] %s, linha %d: %s; nada foi enviado\n", file, ln, err); return 2; }
        }
        fclose(f);
    }

    s = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    if (s < 0 || connect(s, (struct sockaddr *)&a, sizeof a) < 0) { fprintf(stderr, "[diagstub] sem hub em %s\n", path); return 1; }
    hs[0] = 'W'; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(s, hs, 5, 0);
    for (i = 0; i < nsets; i++) {
        static unsigned char b[8 + 5 + MAXDATA];
        int k;
        memcpy(b, &chan, 8);
        b[8] = cab[0]; b[9] = cab[1]; b[10] = cab[2];
        b[11] = (unsigned char)(sets[i].did >> 8); b[12] = (unsigned char)sets[i].did;
        memcpy(b + 13, sets[i].d, (size_t)sets[i].n);
        if (send(s, b, (size_t)(13 + sets[i].n), MSG_NOSIGNAL) < 0) { fprintf(stderr, "[diagstub] falha ao enviar %04X: %s\n", sets[i].did, strerror(errno)); return 1; }
        total += sets[i].n;
        fprintf(stderr, "[diagstub] %04X (%d bytes):", sets[i].did, sets[i].n);
        for (k = 0; k < sets[i].n && k < 32; k++) fprintf(stderr, " %02X", sets[i].d[k]);
        fprintf(stderr, sets[i].n > 32 ? " ...\n" : "\n");
    }
    fprintf(stderr, "[diagstub] enviados %d conjuntos, %d bytes de dados, canal 0x%llx, cabeçalho %02X %02X %02X\n",
            nsets, total, (unsigned long long)chan, cab[0], cab[1], cab[2]);
    close(s);       /* o que já foi enviado fica na fila do hub, que lê tudo antes de fechar o lado dele */
    return 0;
}
