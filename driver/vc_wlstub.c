/* vc_wlstub.c — compositor Wayland de mentira, só para experimento (investigação do painel de
 * instrumentos, 11/10/2026).
 *
 * O motor de navegação do MOI3 (tsd.nav.mainapp.mib3) para na criação do componente de desenho do
 * mapa enquanto não consegue ligar no compositor da central (o Weston), no soquete
 * $XDG_RUNTIME_DIR/wayland-0. Na bancada não há Weston. Este programa ocupa o soquete e responde
 * o mínimo do protocolo para o cliente seguir adiante: anuncia wl_compositor, ivi_application e
 * wl_output, aceita a criação de superfícies e devolve os retornos de quadro. NÃO compõe nem
 * mostra nada: quem desenha é o encaminhador de OpenGL (glforward-moi3), que não passa pelo
 * Wayland. O protocolo ivi_share (por onde o multi-screen-server pega o quadro do painel) não é
 * oferecido de propósito.
 *
 *   vc_wlstub <soquete> [largura altura [mm_largura mm_altura]]
 *   na bancada: vc_wlstub /mnt/low/run/platform/weston/wayland-0 800 480 174 104
 *
 * Cada pedido que chega sai numa linha da saída padrão: cliente, objeto, tipo, operação.
 * Estático, para rodar fora do chroot do firmware. O cliente é código do firmware: rodar só no
 * container descartável, sem rede.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

enum { T_NADA, T_DISPLAY, T_REGISTRO, T_RETORNO, T_COMPOSITOR, T_IVIAPP, T_SAIDA, T_SUPERFICIE, T_REGIAO, T_IVISUP };
static const char *NOME[] = { "?", "wl_display", "wl_registry", "wl_callback", "wl_compositor", "ivi_application",
                              "wl_output", "wl_surface", "wl_region", "ivi_surface" };
#define MAXC 16
#define MAXO 4096
#define MAXP 64

struct cli {
    int fd, n;                          /* soquete e número do cliente */
    uint8_t tipo[MAXO];
    uint32_t pend[MAXP]; int npend;     /* retornos de quadro à espera do commit */
    uint8_t rx[65536]; size_t len;
    unsigned long pedidos;
};
static struct cli C[MAXC];
static int g_w = 800, g_h = 480, g_mmw = 174, g_mmh = 104;
static uint32_t g_serie = 1;
static double t0;

static double agora(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec + (double)t.tv_nsec / 1e9; }
#define LOG(...) do { printf("%8.3f ", agora() - t0); printf(__VA_ARGS__); printf("\n"); } while (0)

/* ---- montagem de eventos ---- */
static uint8_t tx[1024]; static size_t txl;
static void e_ini(void) { txl = 8; }
static void e_u32(uint32_t v) { memcpy(tx + txl, &v, 4); txl += 4; }
static void e_str(const char *s) { uint32_t n = (uint32_t)strlen(s) + 1, p = (n + 3) & ~3u; e_u32(n); memset(tx + txl, 0, p); memcpy(tx + txl, s, n); txl += p; }
static void e_fim(struct cli *c, uint32_t obj, uint32_t op)
{
    uint32_t w = ((uint32_t)txl << 16) | op;
    memcpy(tx, &obj, 4); memcpy(tx + 4, &w, 4);
    if (send(c->fd, tx, txl, MSG_NOSIGNAL) < 0) LOG("c%d envio falhou: %s", c->n, strerror(errno));
}
static void apaga_id(struct cli *c, uint32_t id) { e_ini(); e_u32(id); e_fim(c, 1, 1); if (id < MAXO) c->tipo[id] = T_NADA; }
static void retorno_feito(struct cli *c, uint32_t id) { e_ini(); e_u32(g_serie++); e_fim(c, id, 0); apaga_id(c, id); }

static uint32_t u32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static void novo(struct cli *c, uint32_t id, int tipo) { if (id < MAXO) c->tipo[id] = (uint8_t)tipo; }

static void pedido(struct cli *c, uint32_t obj, uint32_t op, const uint8_t *a, size_t n)
{
    int t = obj < MAXO ? c->tipo[obj] : T_NADA;
    c->pedidos++;
    switch (t) {
    case T_DISPLAY:
        if (op == 0 && n >= 4) { LOG("c%d wl_display.sync", c->n); retorno_feito(c, u32(a)); return; }
        if (op == 1 && n >= 4) {
            uint32_t r = u32(a);
            novo(c, r, T_REGISTRO);
            LOG("c%d wl_display.get_registry -> anuncio wl_compositor, ivi_application, wl_output", c->n);
            e_ini(); e_u32(1); e_str("wl_compositor"); e_u32(3); e_fim(c, r, 0);
            e_ini(); e_u32(2); e_str("ivi_application"); e_u32(1); e_fim(c, r, 0);
            e_ini(); e_u32(3); e_str("wl_output"); e_u32(2); e_fim(c, r, 0);
            return;
        }
        break;
    case T_REGISTRO:
        if (op == 0 && n >= 16) {
            uint32_t nome = u32(a), sl = u32(a + 4), pad = (sl + 3) & ~3u, ver, id;
            char itf[64] = "";
            if (8 + (size_t)pad + 8 > n) break;
            memcpy(itf, a + 8, sl < sizeof itf ? sl : sizeof itf - 1);
            ver = u32(a + 8 + pad); id = u32(a + 12 + pad);
            LOG("c%d wl_registry.bind %s versão %u -> objeto %u", c->n, itf, ver, id);
            if (nome == 1) novo(c, id, T_COMPOSITOR);
            else if (nome == 2) novo(c, id, T_IVIAPP);
            else if (nome == 3) {
                novo(c, id, T_SAIDA);
                e_ini(); e_u32(0); e_u32(0); e_u32((uint32_t)g_mmw); e_u32((uint32_t)g_mmh); e_u32(0);
                e_str("moi3-bancada"); e_str("vc_wlstub"); e_u32(0); e_fim(c, id, 0);            /* geometry */
                e_ini(); e_u32(3); e_u32((uint32_t)g_w); e_u32((uint32_t)g_h); e_u32(60000); e_fim(c, id, 1); /* mode */
                if (ver >= 2) { e_ini(); e_u32(1); e_fim(c, id, 3); e_ini(); e_fim(c, id, 2); }   /* scale, done */
            }
            return;
        }
        break;
    case T_COMPOSITOR:
        if (op == 0 && n >= 4) { novo(c, u32(a), T_SUPERFICIE); LOG("c%d wl_compositor.create_surface -> objeto %u", c->n, u32(a)); return; }
        if (op == 1 && n >= 4) { novo(c, u32(a), T_REGIAO); LOG("c%d wl_compositor.create_region -> objeto %u", c->n, u32(a)); return; }
        break;
    case T_IVIAPP:
        if (op == 0 && n >= 12) {
            novo(c, u32(a + 8), T_IVISUP);
            LOG("c%d ivi_application.surface_create id IVI %u, superfície %u -> objeto %u", c->n, u32(a), u32(a + 4), u32(a + 8));
            return;
        }
        break;
    case T_SUPERFICIE: {
        static const char *o[] = { "destroy", "attach", "damage", "frame", "set_opaque_region", "set_input_region",
                                   "commit", "set_buffer_transform", "set_buffer_scale", "damage_buffer" };
        if (op == 3 && n >= 4) { novo(c, u32(a), T_RETORNO); if (c->npend < MAXP) c->pend[c->npend++] = u32(a); }
        if (op == 6) { int i; for (i = 0; i < c->npend; i++) retorno_feito(c, c->pend[i]); c->npend = 0; }
        if (c->pedidos < 400 || op == 0) LOG("c%d wl_surface(%u).%s", c->n, obj, op < 10 ? o[op] : "?");
        if (op == 0) apaga_id(c, obj);
        return;
    }
    case T_REGIAO: if (op == 0) apaga_id(c, obj); return;
    case T_IVISUP: LOG("c%d ivi_surface(%u).%s", c->n, obj, op == 0 ? "destroy" : "?"); if (op == 0) apaga_id(c, obj); return;
    case T_SAIDA: return;
    }
    LOG("c%d pedido sem tratamento: objeto %u (%s) operação %u, %zu bytes de argumento", c->n, obj, NOME[t], op, n);
}

int main(int argc, char **argv)
{
    struct sockaddr_un a;
    struct pollfd p[MAXC + 1];
    int ls, i, prox = 1;
    if (argc < 2) { fprintf(stderr, "uso: vc_wlstub <soquete> [largura altura [mm_largura mm_altura]]\n"); return 2; }
    if (argc >= 4) { g_w = atoi(argv[2]); g_h = atoi(argv[3]); }
    if (argc >= 6) { g_mmw = atoi(argv[4]); g_mmh = atoi(argv[5]); }
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IOLBF, 0);
    t0 = agora();
    unlink(argv[1]);
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, argv[1], sizeof a.sun_path - 1);
    ls = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ls < 0 || bind(ls, (struct sockaddr *)&a, sizeof a) < 0 || listen(ls, 8) < 0) { perror("soquete"); return 2; }
    chmod(argv[1], 0777);
    for (i = 0; i < MAXC; i++) C[i].fd = -1;
    LOG("escutando em %s; saída anunciada %dx%d, %dx%d mm", argv[1], g_w, g_h, g_mmw, g_mmh);
    for (;;) {
        int n = 1;
        p[0].fd = ls; p[0].events = POLLIN;
        for (i = 0; i < MAXC; i++) if (C[i].fd >= 0) { p[n].fd = C[i].fd; p[n].events = POLLIN; n++; }
        if (poll(p, (nfds_t)n, -1) < 0) { if (errno == EINTR) continue; break; }
        if (p[0].revents & POLLIN) {
            int fd = accept(ls, NULL, NULL);
            for (i = 0; i < MAXC && fd >= 0; i++) if (C[i].fd < 0) {
                memset(&C[i], 0, sizeof C[i]);
                C[i].fd = fd; C[i].n = prox++; C[i].tipo[1] = T_DISPLAY;
                LOG("c%d ligou", C[i].n); fd = -1;
            }
            if (fd >= 0) close(fd);
        }
        for (n = 1; n < MAXC + 1 && n < (int)(sizeof p / sizeof p[0]); n++) {
            struct cli *c = NULL;
            ssize_t r;
            if (!(p[n].revents & (POLLIN | POLLHUP | POLLERR))) continue;
            for (i = 0; i < MAXC; i++) if (C[i].fd == p[n].fd) c = &C[i];
            if (!c) continue;
            r = recv(c->fd, c->rx + c->len, sizeof c->rx - c->len, 0);
            if (r <= 0) { LOG("c%d saiu depois de %lu pedidos", c->n, c->pedidos); close(c->fd); c->fd = -1; continue; }
            c->len += (size_t)r;
            for (;;) {
                uint32_t obj, w, tam;
                if (c->len < 8) break;
                obj = u32(c->rx); w = u32(c->rx + 4); tam = w >> 16;
                if (tam < 8 || tam > sizeof c->rx) { LOG("c%d mensagem inválida (tamanho %u)", c->n, tam); c->len = 0; break; }
                if (c->len < tam) break;
                pedido(c, obj, w & 0xffff, c->rx + 8, tam - 8);
                memmove(c->rx, c->rx + tam, c->len - tam); c->len -= tam;
            }
        }
    }
    return 0;
}
