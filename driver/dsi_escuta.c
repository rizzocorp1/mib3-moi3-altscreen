/* dsi_escuta.c — escuta o barramento do sandbox e imprime cada mensagem DSI inteira, em hexa.
 * Marco 5, 11/10/2026. Estudo num computador, sem carro.
 *
 * Liga-se ao kipc_hub como leitor e mostra uma linha por mensagem:
 *     canal=0x80000000000 if=301 inst=0 api=25 pb=08 01 10 74 18 01 20 e8 07 28 01
 * Serve para os testes do dsistub e para olhar o tráfego sem o limite de 40 bytes do HUB_DUMP.
 * O hub reentrega a quem chega as 48 primeiras mensagens e, desde 11/10/2026, o último anúncio
 * (interface 9000) de cada canal que não esteja entre elas; tudo isso aparece aqui também.
 * As mensagens do microcontrolador de diagnóstico (Marco 6) saem como
 *     canal=0x800000000000000 if=1901 cab=83 60 22 did=0600 dados=20 00 ...
 * ("cab" são os três primeiros bytes da carga; o diagstub mandava 00 00 00 até 11/10/2026)
 *
 * Barramento (igual kipc_bus/kipc_hub): datagrama = [canal:8][interface:4][instância:4][api:4][protobuf].
 * Estático (-static): roda dentro do chroot sob qualquer glibc.
 *
 * uso: dsi_escuta [socket] [interface; 0 = todas]
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/run/kipc_hub.sock";
    uint32_t so = argc > 2 ? (uint32_t)atoi(argv[2]) : 0;
    static unsigned char in[0x6900];
    struct sockaddr_un a;
    char hs[5];
    int s = socket(AF_UNIX, SOCK_SEQPACKET, 0), pid = getpid();
    if (s < 0) return 1;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { fprintf(stderr, "[dsi_escuta] sem hub em %s\n", path); return 1; }
    hs[0] = 'R'; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(s, hs, 5, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    for (;;) {
        ssize_t n = recv(s, in, sizeof in, 0), i;
        uint64_t ch;
        uint32_t ifid, inst, api;
        if (n <= 0) break;
        if (n < 8) continue;
        memcpy(&ch, in, 8);
        /* Marco 6: o canal do microcontrolador de diagnóstico não leva cabeçalho DSI. A carga é
         * [3 bytes][identificador:2][dados], e a HMI a recebe inteira como interface 1901. */
        if (ch == 0x0800000000000000ULL) {
            if (so && so != 1901) continue;
            printf("canal=0x%llx if=1901 ", (unsigned long long)ch);
            if (n >= 13) {
                printf("cab=%02x %02x %02x did=%02x%02x dados=", in[8], in[9], in[10], in[11], in[12]);
                for (i = 13; i < n; i++) printf(i + 1 < n ? "%02x " : "%02x", in[i]);
            } else {
                printf("bruto=");
                for (i = 8; i < n; i++) printf(i + 1 < n ? "%02x " : "%02x", in[i]);
            }
            printf("\n");
            continue;
        }
        if (n < 20) {       /* curta demais para ter cabeçalho: o hub não devia ter encaminhado */
            if (so) continue;
            printf("canal=0x%llx bruto=", (unsigned long long)ch);
            for (i = 8; i < n; i++) printf(i + 1 < n ? "%02x " : "%02x", in[i]);
            printf("\n");
            continue;
        }
        memcpy(&ifid, in + 8, 4); memcpy(&inst, in + 12, 4); memcpy(&api, in + 16, 4);
        if (so && ifid != so) continue;
        printf("canal=0x%llx if=%u inst=%u api=%u pb=", (unsigned long long)ch, ifid, inst, api);
        for (i = 20; i < n; i++) printf(i + 1 < n ? "%02x " : "%02x", in[i]);
        printf("\n");
    }
    return 0;
}
