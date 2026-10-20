/* ponte.c — leva a transmissão dos quadros para fora do container e traz a entrada para dentro.
 *
 *   ponte <soquete dos quadros> [<arquivo de comandos>]
 *
 * O container da bancada roda sem rede. O que atravessa é a entrada e a saída padrão de um
 * `docker exec -i`: a janela do Mac (janela/) chama a ponte assim e conversa com ela por dois
 * canos. A ponte copia o soquete dos quadros do glhost (formato no host.c) para a saída padrão
 * e, se houver arquivo de comandos, acrescenta nele cada linha válida que chega pela entrada
 * padrão. As linhas são as do driver/dsistub.c (tecla, gesto, encoder), com a mesma regra do
 * driver/servidor_ao_vivo.py; o resto é descartado.
 *
 * O arquivo de comandos é aberto a cada linha, pelo caminho, e nunca é criado aqui. A janela
 * liga logo que o glhost sobe, e só depois o lançador põe o arquivo de verdade naquele caminho
 * (por montagem). Uma ponte que abrisse o arquivo ao ligar ficava presa ao que havia ali antes
 * e a central não recebia nada da janela (defeito visto em 11/10/2026). Linha que chega sem
 * arquivo no lugar é descartada: a central ainda não está pronta para entrada.
 *
 * Termina quando a entrada padrão fecha (a janela foi embora) ou quando o glhost fecha o
 * soquete. Saída: 0 fim normal; 2 uso errado; 3 não achou o soquete dos quadros; 4 erro de
 * leitura ou de gravação na saída padrão.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

static int liga(const char *caminho)
{
    struct sockaddr_un a;
    int fd, i;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, caminho, sizeof a.sun_path - 1);
    for (i = 0; i < 30; i++) {                   /* até 3 s: o glhost sobe logo depois da compilação */
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0 && connect(fd, (struct sockaddr *)&a, sizeof a) == 0) return fd;
        if (fd >= 0) close(fd);
        usleep(100000);
    }
    return -1;
}

static int escreve(int fd, const uint8_t *p, size_t n)
{
    while (n) {
        ssize_t r = write(fd, p, n);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return -1;
        p += r; n -= (size_t)r;
    }
    return 0;
}

/* (tecla|encoder) com 3 números, ou gesto com 3 a 6; número = sinal opcional e 1 a 6 dígitos */
static int linha_valida(const char *s)
{
    int minimo = 3, maximo = 3, n = 0, d;
    if (!strncmp(s, "tecla", 5)) s += 5;
    else if (!strncmp(s, "encoder", 7)) s += 7;
    else if (!strncmp(s, "gesto", 5)) { s += 5; maximo = 6; }
    else return 0;
    while (*s == ' ') {
        s++;
        if (*s == '-') s++;
        for (d = 0; *s >= '0' && *s <= '9'; s++) d++;
        if (d < 1 || d > 6) return 0;
        n++;
    }
    return *s == 0 && n >= minimo && n <= maximo;
}

int main(int argc, char **argv)
{
    static uint8_t buf[1 << 18];
    char linha[96];
    size_t ll = 0;
    const char *comandos = argc > 2 ? argv[2] : NULL;
    int qs, longa = 0;
    if (argc < 2) { fprintf(stderr, "uso: ponte <soquete dos quadros> [<arquivo de comandos>]\n"); return 2; }
    signal(SIGPIPE, SIG_IGN);
    qs = liga(argv[1]);
    if (qs < 0) { fprintf(stderr, "ponte: sem soquete dos quadros em %s\n", argv[1]); return 3; }
    for (;;) {
        struct pollfd p[2] = { { qs, POLLIN, 0 }, { 0, POLLIN, 0 } };
        ssize_t r;
        if (poll(p, 2, -1) < 0) { if (errno == EINTR) continue; return 4; }
        if (p[1].revents & (POLLIN | POLLHUP | POLLERR)) {
            ssize_t i;
            r = read(0, buf, 4096);
            if (r < 0 && errno == EINTR) continue;
            if (r <= 0) return 0;                /* a janela fechou o cano */
            for (i = 0; i < r; i++) {
                if (buf[i] != '\n') {
                    if (ll < sizeof linha - 2) linha[ll++] = (char)buf[i]; else longa = 1;
                    continue;
                }
                linha[ll] = 0;
                if (!longa && comandos && linha_valida(linha)) {
                    int cmd = open(comandos, O_WRONLY | O_APPEND);   /* o arquivo que estiver lá agora */
                    if (cmd >= 0) {
                        linha[ll++] = '\n';      /* a linha inteira numa gravação só */
                        if (escreve(cmd, (const uint8_t *)linha, ll) < 0) { }
                        close(cmd);
                    }
                }
                ll = 0; longa = 0;
            }
        }
        if (p[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            r = read(qs, buf, sizeof buf);
            if (r < 0 && errno == EINTR) continue;
            if (r <= 0) return 0;                /* o glhost saiu */
            if (escreve(1, buf, (size_t)r) < 0) return 4;
        }
    }
}
