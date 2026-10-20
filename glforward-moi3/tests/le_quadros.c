/* le_quadros — espectador de teste da transmissão direta do glhost (formato no host.c).
 *
 *   le_quadros <soquete|-> <saída.ppm> [quadros=1] [espera_ms=3000] [bruto.bin]
 *   le_quadros <soquete> --dorme <ms>
 *
 * Liga no soquete dos quadros (ou lê a entrada padrão, com "-"), aplica cada mensagem numa
 * imagem e grava o resultado em PPM quando chegou a quantidade pedida ou quando passou
 * espera_ms sem quadro novo. Imprime uma linha por quadro:
 *   quadro <seq> <largura>x<altura> chave=<0|1> bytes=<carga>
 * Com bruto.bin, guarda também os bytes como vieram (para testar outro decodificador).
 * --dorme liga e não lê nada: é o espectador parado, que não pode segurar o glhost.
 * Saída: 0 se gravou ao menos um quadro; 1 se não veio nenhum; 2 se o formato veio errado.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

static FILE *g_raw;

/* lê exatamente n bytes; -1 no fim do fluxo, -2 se passou o tempo sem chegar nada */
static int le(int fd, void *dst, size_t n, int espera_ms)
{
    uint8_t *d = dst;
    while (n) {
        struct pollfd p = { fd, POLLIN, 0 };
        ssize_t r;
        int k = poll(&p, 1, espera_ms);
        if (k < 0 && errno == EINTR) continue;
        if (k == 0) return -2;
        if (k < 0) return -1;
        r = read(fd, d, n);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return -1;
        if (g_raw) fwrite(d, 1, (size_t)r, g_raw);
        d += r; n -= (size_t)r;
    }
    return 0;
}

static uint32_t u32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static unsigned u16(const uint8_t *p) { return p[0] | p[1] << 8; }

static int liga(const char *caminho)
{
    struct sockaddr_un a;
    int fd, i;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, caminho, sizeof a.sun_path - 1);
    for (i = 0; i < 50; i++) {                       /* o glhost pode estar subindo */
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0 && connect(fd, (struct sockaddr *)&a, sizeof a) == 0) return fd;
        if (fd >= 0) close(fd);
        usleep(100000);
    }
    return -1;
}

int main(int argc, char **argv)
{
    uint8_t h[20], *img = NULL, *carga = NULL;
    size_t cap = 0;
    int fd, quero, espera, w = 0, hh = 0, vieram = 0, rc;
    FILE *f;
    if (argc < 3) { fprintf(stderr, "uso: le_quadros <soquete|-> <saida.ppm> [quadros] [espera_ms] [bruto.bin]\n"); return 2; }
    fd = strcmp(argv[1], "-") ? liga(argv[1]) : 0;
    if (fd < 0) { printf("sem soquete %s\n", argv[1]); return 1; }
    printf("conectado\n"); fflush(stdout);
    if (!strcmp(argv[2], "--dorme")) { usleep((useconds_t)atoi(argc > 3 ? argv[3] : "1000") * 1000); return 0; }
    quero = argc > 3 ? atoi(argv[3]) : 1;
    espera = argc > 4 ? atoi(argv[4]) : 3000;
    if (argc > 5) g_raw = fopen(argv[5], "wb");
    while (vieram < quero && (rc = le(fd, h, 20, espera)) == 0) {
        uint32_t seq = u32(h + 4), len = u32(h + 16), p = 0;
        int fw = (int)u16(h + 8), fh = (int)u16(h + 10), chave = (int)(u16(h + 12) & 1);
        size_t n = (size_t)fw * (size_t)fh, pos = 0;
        if (memcmp(h, "MQ01", 4) || !n) { printf("cabecalho errado\n"); return 2; }
        if (chave || fw != w || fh != hh) {
            if (!chave) { printf("mudou de tamanho sem quadro-chave\n"); return 2; }
            free(img); img = calloc(n, 3); w = fw; hh = fh;
        }
        if (len > cap) { cap = len; carga = realloc(carga, cap); }
        if (len && le(fd, carga, len, espera)) { printf("carga curta\n"); return 2; }
        while (p < len) {
            uint32_t pula, copia;
            if (len - p < 8) { printf("trecho cortado\n"); return 2; }
            pula = u32(carga + p); copia = u32(carga + p + 4); p += 8;
            if (pos + pula + copia > n || (size_t)copia * 3 > len - p) { printf("trecho fora da imagem\n"); return 2; }
            pos += pula;
            memcpy(img + pos * 3, carga + p, (size_t)copia * 3);
            pos += copia; p += copia * 3;
        }
        vieram++;
        printf("quadro %u %dx%d chave=%d bytes=%u\n", seq, fw, fh, chave, len); fflush(stdout);
    }
    if (g_raw) fclose(g_raw);
    if (!vieram) { printf("nenhum quadro\n"); return 1; }
    f = fopen(argv[2], "wb");
    if (!f) { printf("não gravou %s\n", argv[2]); return 1; }
    fprintf(f, "P6\n%d %d\n255\n", w, hh);
    fwrite(img, 3, (size_t)w * (size_t)hh, f);
    fclose(f);
    return 0;
}
