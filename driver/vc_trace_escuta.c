/* vc_trace_escuta.c — ouve o soquete de log dos daemons do firmware (investigação do painel de
 * instrumentos, 11/10/2026).
 *
 * Os daemons do MOI3 não escrevem log em arquivo nem na saída padrão: a libtraceif.so manda tudo
 * para um soquete Unix de fluxo (SOCK_STREAM), /tmp/trace_unix_socket, que no carro é atendido
 * pelo daemon de rastreio. Na bancada ninguém atendia, e os daemons rodavam mudos. Este programa
 * ocupa o soquete e copia o que chega para a saída padrão, um trecho por linha, com os bytes que
 * não são texto trocados por ponto. Não interpreta o formato (que não foi levantado) e não
 * responde nada.
 *
 *   vc_trace_escuta <caminho do soquete>      (na bancada: /mnt/low/tmp/trace_unix_socket)
 *
 * Linha de saída:  <segundos desde a partida> c<cliente> <bytes> | <texto>
 * Estático, para rodar fora do chroot do firmware. Só lê; o que chega é dado do firmware.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#define MAXC 96

static double agora(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    struct sockaddr_un a;
    struct pollfd p[MAXC + 1];
    int id[MAXC + 1], n = 1, prox = 1, i, ls;
    static unsigned char buf[65536];
    static char txt[65536 + 1];
    double t0 = agora();

    if (argc < 2) { fprintf(stderr, "uso: vc_trace_escuta <soquete>\n"); return 2; }
    signal(SIGPIPE, SIG_IGN);
    unlink(argv[1]);
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, argv[1], sizeof a.sun_path - 1);
    ls = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ls < 0 || bind(ls, (struct sockaddr *)&a, sizeof a) < 0 || listen(ls, 64) < 0) { perror("soquete"); return 2; }
    chmod(argv[1], 0777);
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("%8.3f -- escutando em %s\n", 0.0, argv[1]);
    p[0].fd = ls; p[0].events = POLLIN;
    for (;;) {
        if (poll(p, (nfds_t)n, -1) < 0) { if (errno == EINTR) continue; break; }
        if (p[0].revents & POLLIN) {
            int fd = accept(ls, NULL, NULL);
            if (fd >= 0) {
                if (n <= MAXC) { p[n].fd = fd; p[n].events = POLLIN; p[n].revents = 0; id[n] = prox; n++;
                                 printf("%8.3f c%d -- ligou\n", agora() - t0, prox); }
                else close(fd);
                prox++;
            }
        }
        for (i = 1; i < n; i++) {
            ssize_t r, k;
            if (!(p[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
            r = read(p[i].fd, buf, sizeof buf);
            if (r <= 0) {
                printf("%8.3f c%d -- saiu\n", agora() - t0, id[i]);
                close(p[i].fd);
                p[i] = p[n - 1]; id[i] = id[n - 1]; n--; i--;
                continue;
            }
            for (k = 0; k < r; k++) txt[k] = (buf[k] >= 32 && buf[k] < 127) ? (char)buf[k] : '.';
            txt[r] = 0;
            printf("%8.3f c%d %zd | %s\n", agora() - t0, id[i], r, txt);
        }
    }
    return 0;
}
