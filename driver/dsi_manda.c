/* dsi_manda.c — põe uma mensagem DSI no barramento do sandbox. Marco 6, 11/10/2026.
 * Estudo num computador, sem carro. Par do dsi_escuta.c: um lê, o outro escreve.
 *
 * Serve para experimentar antes de escrever um stub: anunciar um domínio (a mensagem WELCOME,
 * interface 9000, que faz a HMI ligar os serviços daquele domínio) ou mandar um evento solto.
 *
 * Barramento (igual kipc_bus/kipc_hub): datagrama = [canal:8][interface:4][instância:4][api:4][protobuf].
 * Estático (-static): roda dentro do chroot sob qualquer glibc.
 *
 * uso: dsi_manda <socket> <canal> <interface> <instância> <api> [protobuf em hexa] [repetir a cada N ms]
 *   canal aceita decimal ou 0x…; o protobuf vai como "08 01 10 74" ou "08011074"; "-" = vazio
 *   exemplo: dsi_manda /run/kipc_hub.sock 16 9000 0 0 - 1000    anuncia o domínio 16 a cada segundo
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char **argv)
{
    unsigned char b[20 + 4096];
    struct sockaddr_un a;
    char hs[5];
    uint64_t chan;
    uint32_t ifid, inst, api;
    int s, pid = getpid(), n = 0, cada = 0;
    if (argc < 6) { fprintf(stderr, "uso: dsi_manda <socket> <canal> <interface> <instância> <api> [hexa|-] [ms]\n"); return 2; }
    chan = strtoull(argv[2], NULL, 0);
    ifid = (uint32_t)strtoul(argv[3], NULL, 0); inst = (uint32_t)strtoul(argv[4], NULL, 0); api = (uint32_t)strtoul(argv[5], NULL, 0);
    if (argc > 6 && strcmp(argv[6], "-")) {
        const char *p = argv[6];
        while (*p && n < 4096) {
            unsigned v;
            if (isspace((unsigned char)*p)) { p++; continue; }
            if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1]) || sscanf(p, "%2x", &v) != 1) {
                fprintf(stderr, "[dsi_manda] protobuf em hexa inválido perto de \"%s\"\n", p); return 2;
            }
            b[20 + n++] = (unsigned char)v; p += 2;
        }
    }
    if (argc > 7) cada = atoi(argv[7]);
    memcpy(b, &chan, 8); memcpy(b + 8, &ifid, 4); memcpy(b + 12, &inst, 4); memcpy(b + 16, &api, 4);
    s = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (s < 0) return 1;
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, argv[1], sizeof a.sun_path - 1);
    if (connect(s, (struct sockaddr *)&a, sizeof a) < 0) { fprintf(stderr, "[dsi_manda] sem hub em %s\n", argv[1]); return 1; }
    hs[0] = 'W'; hs[1] = (char)pid; hs[2] = (char)(pid >> 8); hs[3] = (char)(pid >> 16); hs[4] = (char)(pid >> 24);
    send(s, hs, 5, 0);
    do {
        if (send(s, b, (size_t)(20 + n), MSG_NOSIGNAL) < 0) return 1;
        if (cada > 0) usleep((useconds_t)cada * 1000);
    } while (cada > 0);
    usleep(50000);      /* dá tempo de o hub ler antes de a ligação fechar */
    return 0;
}
