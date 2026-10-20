/* vc_diag_alm.c — manda de novo, com o cabeçalho que o navi-al (alm) entende, os conjuntos de
 * codificação e adaptação que o diagstub publicou (estudo no sandbox, sem carro).
 * Investigação do painel de instrumentos, item 3, 11/10/2026. É ferramenta de experimento: o
 * diagstub, que é do Marco 6, fica como está.
 *
 * SEM FUNÇÃO NAS CENTRAIS DESDE AS 23h26 DE 11/10/2026. A pedido deste item, a sessão do Marco 6
 * pôs o cabeçalho 83 60 22 no próprio diagstub (decisão D77 em ../modelagem-soc.md), e as rodadas
 * vc_r6 e vc_r6b mostraram o alm aceitando os conjuntos direto dele. Rodado hoje, este programa
 * diz "mandados de novo 0" e sai. Fica na pasta como registro do experimento e porque ainda
 * serve de contraprova: com "diagstub -k 000000" (o cabeçalho de antes) ele volta a ter o que
 * trocar, e é assim que o teste driver/tests/t_vc_diag_alm.sh o exercita. O que segue descreve
 * a bancada como era até aquela hora.
 *
 * O que foi lido no alm do MU0883 (/lge/app_ro/navi/navi-al/alm, função
 * KipcProvider::procDiagMsg; registro em ../painel-de-instrumentos.md):
 *  - as mensagens do canal do microcontrolador de diagnóstico (PID_DIAG_MCU) têm a carga
 *    [3 bytes][identificador: 2 bytes, o alto primeiro][dados]. O alm chama os três primeiros
 *    bytes de MainCmd, subCmd1 e subCmd2 e só segue adiante se forem 0x83, 0x60 e o serviço:
 *    0x22 para dado por identificador (os conjuntos de codificação e adaptação). Com outro
 *    valor ele registra "Data Type : Unknown !!!" e descarta a mensagem;
 *  - o diagstub manda esses três bytes zerados, porque a HMI e o persistence-app não os leem;
 *  - com o serviço 0x22 o alm trata nove identificadores: 0600, 0558, 0571, 09CB, 0B1D, 0B5A,
 *    0E38 e 243F, que a bancada publica, e o 04FB, que ela não publica.
 * O que o microcontrolador de verdade manda nesses bytes para os outros conjuntos não foi visto.
 *
 * O que este programa faz: liga-se ao hub como leitor e recebe os conjuntos que o hub tem
 * guardados (se ainda não há nenhum, espera o diagstub mandar). Quando a remessa acaba, manda de
 * novo os que o alm usa, com os MESMOS dados e o cabeçalho 83 60 22. O hub guarda um conjunto
 * por identificador, então a versão nova toma o lugar da antiga, na mesma posição, para quem
 * se ligar depois. Dado nenhum é mudado aqui: valor de teste entra pelos arquivos de
 * codificação do diagstub, como sempre.
 *
 * Estático (-static): roda fora ou dentro do chroot, sob qualquer glibc.
 * uso: vc_diag_alm [-a] [-q <ms>] [-e <ms>] <socket>
 *   -a   todos os conjuntos, e não só os que o alm usa (é o que aconteceria se o diagstub
 *        passasse a mandar o cabeçalho novo; serve para ver se a HMI se importa)
 *   -q   silêncio que marca o fim da remessa do diagstub (padrão 500 ms)
 *   -e   espera máxima pelo primeiro conjunto (padrão 15000 ms)
 * Sai com 0 se recebeu conjuntos (mesmo sem nada a trocar), 1 sem hub ou sem conjunto, 2 no uso.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#define PID_DIAG_MCU 0x0800000000000000ULL
#define MAXSETS 128
#define MAXDATA 4096
#define MAXMSG (0x6800 + 16)

/* os três bytes que o alm exige: MainCmd, subCmd1 e o serviço (dado por identificador) */
static const unsigned char CAB[3] = { 0x83, 0x60, 0x22 };
/* os identificadores que o alm trata com esse serviço (procDiagMsg) */
static const unsigned DO_ALM[] = { 0x0600, 0x0558, 0x0571, 0x09CB, 0x0B1D, 0x0B5A, 0x0E38, 0x243F, 0x04FB };

struct set { unsigned did; int n; unsigned char b[8 + 5 + MAXDATA]; };   /* a mensagem inteira */
static struct set sets[MAXSETS];
static int nsets;

static long agora_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
}

static int do_alm(unsigned did)
{
    size_t i;
    for (i = 0; i < sizeof DO_ALM / sizeof DO_ALM[0]; i++) if (DO_ALM[i] == did) return 1;
    return 0;
}

/* liga-se ao hub com o papel dado ('R' leitor, 'W' escritor); tenta por 1 s */
static int liga(const char *path, char papel)
{
    struct sockaddr_un a;
    char hs[5];
    int s, pid = getpid(), i;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    for (i = 0; ; i++) {
        s = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        if (s < 0) return -1;
        if (connect(s, (struct sockaddr *)&a, sizeof a) == 0) break;
        close(s);
        if (i >= 10) return -1;
        usleep(100000);
    }
    hs[0] = papel; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(s, hs, 5, 0);
    return s;
}

int main(int argc, char **argv)
{
    static unsigned char in[MAXMSG];
    const char *path;
    long quieto = 500, maximo = 15000, t0, ultimo = 0;
    int todos = 0, arg = 1, r, w, i, k, mandados = 0, ja = 0;
    for (; arg < argc && argv[arg][0] == '-'; arg++) {
        if (!strcmp(argv[arg], "-a")) todos = 1;
        else if (!strcmp(argv[arg], "-q") && arg + 1 < argc) quieto = atol(argv[++arg]);
        else if (!strcmp(argv[arg], "-e") && arg + 1 < argc) maximo = atol(argv[++arg]);
        else { arg = argc; break; }
    }
    if (argc - arg != 1 || quieto <= 0 || maximo <= 0) {
        fprintf(stderr, "uso: vc_diag_alm [-a] [-q <ms>] [-e <ms>] <socket>\n");
        return 2;
    }
    path = argv[arg];

    r = liga(path, 'R');
    if (r < 0) { fprintf(stderr, "[vc_diag_alm] sem hub em %s\n", path); return 1; }
    /* Recebe até a remessa acabar. Só os conjuntos de diagnóstico contam para o silêncio: o
     * resto do tráfego do barramento (o anúncio do display, por exemplo) não para nunca. */
    t0 = agora_ms();
    for (;;) {
        struct pollfd p;
        uint64_t ch;
        unsigned did;
        long agora = agora_ms(), falta = nsets ? ultimo + quieto - agora : t0 + maximo - agora;
        ssize_t n;
        if (falta <= 0) break;
        p.fd = r; p.events = POLLIN; p.revents = 0;
        k = poll(&p, 1, (int)falta);
        if (k < 0) { if (errno == EINTR) continue; break; }
        if (k == 0) continue;
        n = recv(r, in, sizeof in, 0);
        if (n <= 0) { fprintf(stderr, "[vc_diag_alm] o hub fechou a ligação\n"); break; }
        if (n < 8 + 5 || (size_t)n > sizeof sets[0].b) continue;
        memcpy(&ch, in, 8);
        if (ch != PID_DIAG_MCU) continue;
        did = (unsigned)in[11] << 8 | in[12];
        for (i = 0; i < nsets; i++) if (sets[i].did == did) break;
        if (i == nsets) { if (nsets >= MAXSETS) continue; nsets++; }
        sets[i].did = did; sets[i].n = (int)n;
        memcpy(sets[i].b, in, (size_t)n);
        ultimo = agora_ms();
    }
    close(r);
    if (!nsets) {
        fprintf(stderr, "[vc_diag_alm] nenhum conjunto de diagnóstico chegou em %ld ms; nada foi enviado\n", maximo);
        return 1;
    }

    w = liga(path, 'W');
    if (w < 0) { fprintf(stderr, "[vc_diag_alm] sem hub em %s\n", path); return 1; }
    for (i = 0; i < nsets; i++) {
        unsigned char *b = sets[i].b;
        int nd = sets[i].n - 13;
        if (!todos && !do_alm(sets[i].did)) continue;
        if (!memcmp(b + 8, CAB, 3)) { ja++; continue; }
        fprintf(stderr, "[vc_diag_alm] %04X (%d bytes):", sets[i].did, nd);
        for (k = 0; k < nd && k < 32; k++) fprintf(stderr, " %02X", b[13 + k]);
        fprintf(stderr, "%s  cabeçalho %02X %02X %02X -> %02X %02X %02X\n", nd > 32 ? " ..." : "",
                b[8], b[9], b[10], CAB[0], CAB[1], CAB[2]);
        memcpy(b + 8, CAB, 3);
        if (send(w, b, (size_t)sets[i].n, MSG_NOSIGNAL) < 0) {
            fprintf(stderr, "[vc_diag_alm] falha ao enviar %04X: %s\n", sets[i].did, strerror(errno));
            return 1;
        }
        mandados++;
    }
    fprintf(stderr, "[vc_diag_alm] recebidos %d conjuntos; mandados de novo %d com o cabeçalho %02X %02X %02X"
            " (%s; %d já estavam assim)\n", nsets, mandados, CAB[0], CAB[1], CAB[2],
            todos ? "todos" : "só os que o alm usa", ja);
    close(w);       /* o que já foi enviado fica na fila do hub, que lê tudo antes de fechar o lado dele */
    return 0;
}
