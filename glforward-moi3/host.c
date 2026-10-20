/* host.c — glhost: o lado que desenha do encaminhador de OpenGL do sandbox MOI3 (Marco 3).
 *
 * Roda FORA do chroot do firmware, sob a glibc nova, com Mesa llvmpipe (OpenGL ES 3.2 por
 * software, EGL sem janela). Escuta num socket Unix, recebe as chamadas que o shim empacotou
 * (proto.h), executa de verdade e responde. A cada troca de buffers lê o framebuffer, grava
 * o quadro em PPM e o transmite por um soquete a quem estiver assistindo (seção "transmissão
 * dos quadros", mais abaixo).
 *
 *   glhost <socket> [largura altura] [pasta dos quadros]
 *
 * Ambiente:  GLHOST_TRACE=1    registra cada operação
 *            GLHOST_NOCHECK=1  não confere glGetError depois de cada chamada
 *            GLHOST_KEEP=n     quantos quadros numerados guardar (default 40; latest.ppm sempre)
 *            GLHOST_STREAM=x   soquete da transmissão dos quadros (default /tmp/moi3-quadros.sock;
 *                              vazio ou 0 desliga)
 *
 * O cliente é o firmware (código que não é nosso): este processo deve rodar só dentro do
 * container descartável da bancada, sem rede.
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include "proto.h"
#include "gen/ops_gen.h"

#define SZ(e) ((e) > 0 ? (size_t)(e) : (size_t)0)
#define REP_CAP ((size_t)768 << 20)
#define NOPS (OP_GL_END + 1)

static int cfd = -1, g_trace, g_check = 1, g_keep = 40, g_bad;
static int g_w = 1280, g_h = 640;
static const char *g_frames = "/tmp";
static uint8_t *rq, *rp, *rend, *rep_base, *rep;
static size_t rq_cap, rep_len;
static uint8_t ibuf[1 << 20];
static size_t ib_len, ib_pos;
static unsigned long g_count[NOPS], g_errs[NOPS], g_total, g_frames_n;
static GLenum g_first_err[NOPS], g_pending_err;
static uint32_t g_op;

static EGLDisplay D;
static EGLContext g_cur_ctx = EGL_NO_CONTEXT;
struct surf { EGLSurface s; int w, h, idx; unsigned frames; };
static struct surf g_surf[64];
static EGLContext g_ctx[64];

static const char *opname(uint32_t op)
{
    static const char *const x[] = { "?", "HELLO", "EGL_CHOOSE", "EGL_GETCONFIGS", "EGL_GETCONFIGATTR",
        "EGL_CREATECTX", "EGL_DESTROYCTX", "EGL_CREATESURF", "EGL_DESTROYSURF", "EGL_MAKECURRENT",
        "EGL_SWAP", "EGL_QUERYSURF", "EGL_QUERYCTX", "SWITCH" };
    if (op >= OP_GL_BASE && op < OP_GL_END) return glfwd_gl_names[op - OP_GL_BASE];
    return op < sizeof x / sizeof *x ? x[op] : "?";
}

static void LOG(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}

/* ------------------------------------------------------------------ leitura do pedido */
static int readn(void *dst, size_t n)
{
    uint8_t *d = dst;
    while (n) {
        size_t k;
        if (ib_pos == ib_len) {
            ssize_t r = read(cfd, ibuf, sizeof ibuf);
            if (r < 0 && errno == EINTR) continue;
            if (r <= 0) return -1;
            ib_len = (size_t)r; ib_pos = 0;
        }
        k = ib_len - ib_pos < n ? ib_len - ib_pos : n;
        memcpy(d, ibuf + ib_pos, k);
        ib_pos += k; d += k; n -= k;
    }
    return 0;
}

static uint64_t R_u64(void)
{
    uint64_t v = 0;
    if (rp + 8 > rend) { g_bad = 1; return 0; }
    memcpy(&v, rp, 8); rp += 8;
    return v;
}

static float R_f32(void) { uint64_t v = R_u64(); float f; memcpy(&f, &v, 4); return f; }

static void *R_blob(size_t *n)
{
    uint64_t h = R_u64();
    size_t len = (size_t)(h & ~GLFWD_PRESENT), pad = (len + 7) & ~(size_t)7;
    void *p = rp;
    *n = 0;
    if (!(h & GLFWD_PRESENT)) return NULL;
    if (pad > (size_t)(rend - rp)) { g_bad = 1; return NULL; }
    rp += pad;
    *n = len;
    return p;
}

static const char *R_str(void) { size_t n; return R_blob(&n); }

/* ------------------------------------------------------------------ montagem da resposta */
static void *P_reserve(size_t n)
{
    size_t a = (n + 7) & ~(size_t)7;
    void *p = rep + rep_len;
    if (rep_len + a > REP_CAP - 4096) { LOG("[glhost] resposta grande demais em %s\n", opname(g_op)); exit(3); }
    memset(p, 0, a);
    rep_len += a;
    return p;
}

static void P_u64(uint64_t v) { memcpy(P_reserve(8), &v, 8); }

static void REPLY(void)
{
    uint64_t len = rep_len;
    const uint8_t *p = rep_base;
    size_t n = rep_len + 8;
    memcpy(rep_base, &len, 8);
    while (n) {
        ssize_t r = send(cfd, p, n, MSG_NOSIGNAL);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return;
        p += r; n -= (size_t)r;
    }
}

/* ------------------------------------------------------------------ transmissão dos quadros
 * Além do arquivo, cada quadro sai por um soquete Unix para quem quiser assistir ao vivo (a
 * janela do Mac, pela ponte.c). Uma thread cuida só disso: a de desenho entrega o quadro e
 * segue, de modo que espectador lento ou parado não segura a HMI; ele só perde os quadros do
 * meio do caminho.
 *
 * Cada mensagem tem 20 bytes de cabeçalho, em little-endian, e a carga:
 *   "MQ01"  número do quadro u32  largura u16  altura u16  marcas u16  reserva u16  bytes da carga u32
 *   carga: trechos [pular u32][copiar u32][copiar x 3 bytes RGB], contados em pixels, de cima
 *          para baixo e da esquerda para a direita
 * O espectador guarda a imagem e aplica os trechos nela: só viaja o que mudou desde o último
 * quadro que foi mandado para ele. Marca 1 (quadro-chave): zerar a imagem antes de aplicar; é
 * o primeiro quadro de cada espectador e o que segue uma mudança de tamanho. Quem liga com a
 * tela parada recebe na hora o último quadro desenhado.
 */
#define TX_MAX 4
struct tx_viewer { int fd, w, h; uint32_t seq; uint8_t *prev, *out; size_t out_len, out_pos; };
static struct tx_viewer tx_v[TX_MAX];
static pthread_mutex_t tx_mu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t *tx_rgb;                     /* o último quadro desenhado, sob tx_mu */
static size_t tx_cap;
static int tx_w, tx_h, tx_ls = -1, tx_wake[2] = { -1, -1 };
static uint32_t tx_seq;

/* thread de desenho: deixa o quadro para a thread da transmissão e não espera por ninguém */
static void tx_publish(const uint8_t *rgb, int w, int h, uint32_t seq)
{
    size_t n = (size_t)w * (size_t)h * 3;
    if (tx_ls < 0) return;
    pthread_mutex_lock(&tx_mu);
    if (n > tx_cap) {
        uint8_t *p = realloc(tx_rgb, n);
        if (p) { tx_rgb = p; tx_cap = n; }
    }
    if (n <= tx_cap) { memcpy(tx_rgb, rgb, n); tx_w = w; tx_h = h; tx_seq = seq; }
    pthread_mutex_unlock(&tx_mu);
    if (write(tx_wake[1], "q", 1) < 0) { }  /* cano cheio: a thread já vai acordar */
}

static void tx_put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void tx_put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

static void tx_drop(struct tx_viewer *v, const char *why)
{
    LOG("[glhost] espectador saiu (%s)\n", why);
    close(v->fd); free(v->prev); free(v->out);
    memset(v, 0, sizeof *v);
    v->fd = -1;
}

/* monta em v->out a mensagem que leva o espectador de v->prev até cur, e atualiza v->prev */
static int tx_encode(struct tx_viewer *v, const uint8_t *cur, int w, int h, uint32_t seq)
{
    size_t n = (size_t)w * (size_t)h, i = 0, o = 20;
    unsigned flags = 0;
    if (!v->prev || v->w != w || v->h != h) {
        /* pior caso: todo pixel copiado (3 bytes) e um trecho (8 bytes) a cada 4 pixels */
        free(v->prev); free(v->out);
        v->prev = calloc(n, 3); v->out = malloc(20 + n * 5 + 16);
        if (!v->prev || !v->out) return -1;
        v->w = w; v->h = h; flags = 1;
    }
    while (i < n) {
        size_t s = i, l;
        while (i + 8 <= n && !memcmp(cur + 3 * i, v->prev + 3 * i, 24)) i += 8;
        while (i < n && !memcmp(cur + 3 * i, v->prev + 3 * i, 3)) i++;
        if (i == n) break;
        l = i;
        for (;;) {
            size_t g;
            while (i < n && memcmp(cur + 3 * i, v->prev + 3 * i, 3)) i++;
            for (g = i; g < n && g - i < 3 && !memcmp(cur + 3 * g, v->prev + 3 * g, 3); g++) { }
            if (g == n || g - i >= 3) break;
            i = g;                          /* buraco de 1 ou 2 pixels iguais: copiar sai mais barato que abrir outro trecho */
        }
        tx_put32(v->out + o, (uint32_t)(l - s)); tx_put32(v->out + o + 4, (uint32_t)(i - l));
        memcpy(v->out + o + 8, cur + 3 * l, 3 * (i - l));
        memcpy(v->prev + 3 * l, cur + 3 * l, 3 * (i - l));
        o += 8 + 3 * (i - l);
    }
    memcpy(v->out, "MQ01", 4);
    tx_put32(v->out + 4, seq); tx_put16(v->out + 8, (unsigned)w); tx_put16(v->out + 10, (unsigned)h);
    tx_put16(v->out + 12, flags); tx_put16(v->out + 14, 0); tx_put32(v->out + 16, (uint32_t)(o - 20));
    v->out_len = o; v->out_pos = 0; v->seq = seq;
    return 0;
}

/* manda o que couber sem esperar; o resto fica para quando o soquete tiver espaço */
static void tx_flush(struct tx_viewer *v)
{
    while (v->out_pos < v->out_len) {
        ssize_t r = send(v->fd, v->out + v->out_pos, v->out_len - v->out_pos, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (r < 0 && errno == EINTR) continue;
        if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
        if (r <= 0) { tx_drop(v, "erro no envio"); return; }
        v->out_pos += (size_t)r;
    }
}

static void *tx_thread(void *arg)
{
    uint8_t *cur = NULL;                    /* cópia do último quadro, só desta thread */
    size_t cur_cap = 0;
    int cw = 0, ch = 0, i;
    uint32_t cseq = 0;
    sigset_t ss;
    (void)arg;
    sigemptyset(&ss); sigaddset(&ss, SIGTERM); sigaddset(&ss, SIGINT);
    pthread_sigmask(SIG_BLOCK, &ss, NULL);  /* o encerramento continua com a thread de desenho */
    for (;;) {
        struct pollfd p[2 + TX_MAX];
        int n = 2, who[TX_MAX];
        char junk[256];
        p[0].fd = tx_ls; p[0].events = POLLIN; p[0].revents = 0;
        p[1].fd = tx_wake[0]; p[1].events = POLLIN; p[1].revents = 0;
        for (i = 0; i < TX_MAX; i++) {
            if (tx_v[i].fd < 0) continue;
            p[n].fd = tx_v[i].fd; p[n].revents = 0;
            p[n].events = (short)(POLLIN | (tx_v[i].out_pos < tx_v[i].out_len ? POLLOUT : 0));
            who[n - 2] = i; n++;
        }
        if (poll(p, (nfds_t)n, -1) < 0) { if (errno == EINTR) continue; break; }
        if (p[1].revents & POLLIN) while (read(tx_wake[0], junk, sizeof junk) > 0) { }
        for (i = 2; i < n; i++) {           /* o espectador não manda nada: leitura vazia é a saída dele */
            struct tx_viewer *v = &tx_v[who[i - 2]];
            ssize_t r;
            if (!(p[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
            r = recv(v->fd, junk, sizeof junk, MSG_DONTWAIT);
            if (r == 0) tx_drop(v, "fechou");
            else if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) tx_drop(v, "erro");
        }
        if (p[0].revents & POLLIN) {
            int fd = accept(tx_ls, NULL, NULL);
            if (fd >= 0) {
                for (i = 0; i < TX_MAX && tx_v[i].fd >= 0; i++) { }
                if (i == TX_MAX) { close(fd); LOG("[glhost] espectador recusado: já há %d\n", TX_MAX); }
                else { tx_v[i].fd = fd; LOG("[glhost] espectador conectou\n"); }
            }
        }
        pthread_mutex_lock(&tx_mu);
        if (tx_seq != cseq) {
            size_t nb = (size_t)tx_w * (size_t)tx_h * 3;
            if (nb > cur_cap) {
                uint8_t *q = realloc(cur, nb);
                if (q) { cur = q; cur_cap = nb; }
            }
            if (nb <= cur_cap) { memcpy(cur, tx_rgb, nb); cw = tx_w; ch = tx_h; cseq = tx_seq; }
        }
        pthread_mutex_unlock(&tx_mu);
        for (i = 0; i < TX_MAX; i++) {
            struct tx_viewer *v = &tx_v[i];
            if (v->fd >= 0 && v->out_pos < v->out_len) tx_flush(v);
            /* só monta quadro novo para quem já recebeu o anterior inteiro: o atrasado pula quadros */
            if (v->fd >= 0 && v->out_pos == v->out_len && cseq && v->seq != cseq) {
                if (tx_encode(v, cur, cw, ch, cseq) < 0) tx_drop(v, "sem memória");
                else tx_flush(v);
            }
        }
    }
    return NULL;
}

static void tx_start(void)
{
    const char *path = getenv("GLHOST_STREAM");
    struct sockaddr_un a;
    pthread_t t;
    int i;
    if (!path) path = "/tmp/moi3-quadros.sock";
    if (!*path || !strcmp(path, "0")) return;
    for (i = 0; i < TX_MAX; i++) tx_v[i].fd = -1;
    unlink(path);
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    tx_ls = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (tx_ls < 0 || bind(tx_ls, (struct sockaddr *)&a, sizeof a) < 0 || listen(tx_ls, TX_MAX) < 0
        || pipe2(tx_wake, O_NONBLOCK | O_CLOEXEC) < 0 || pthread_create(&t, NULL, tx_thread, NULL) != 0) {
        LOG("[glhost] sem transmissão dos quadros em %s: %s\n", path, strerror(errno));
        if (tx_ls >= 0) close(tx_ls);
        tx_ls = -1;
        return;
    }
    LOG("[glhost] transmissão dos quadros em %s\n", path);
}

/* ------------------------------------------------------------------ quadro em arquivo */
static void dump_frame(struct surf *sf)
{
    static uint8_t *px, *rgb;
    static size_t cap;
    GLint rfb = 0, pbo = 0, al = 4, rl = 0, sr = 0, sp = 0;
    size_t n = (size_t)sf->w * (size_t)sf->h, i, lit = 0;
    char tmp[512], dst[512];
    FILE *f;
    int y;
    if (n * 4 > cap) { cap = n * 4; px = realloc(px, cap); rgb = realloc(rgb, n * 3); }
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &rfb);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pbo);
    glGetIntegerv(GL_PACK_ALIGNMENT, &al); glGetIntegerv(GL_PACK_ROW_LENGTH, &rl);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &sr); glGetIntegerv(GL_PACK_SKIP_PIXELS, &sp);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1); glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0); glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, sf->w, sf->h, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)rfb);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, (GLuint)pbo);
    glPixelStorei(GL_PACK_ALIGNMENT, al); glPixelStorei(GL_PACK_ROW_LENGTH, rl);
    glPixelStorei(GL_PACK_SKIP_ROWS, sr); glPixelStorei(GL_PACK_SKIP_PIXELS, sp);
    while (glGetError() != GL_NO_ERROR) { }
    for (y = 0; y < sf->h; y++) {             /* o GL entrega de baixo para cima */
        const uint8_t *s = px + (size_t)(sf->h - 1 - y) * (size_t)sf->w * 4;
        uint8_t *d = rgb + (size_t)y * (size_t)sf->w * 3;
        for (i = 0; i < (size_t)sf->w; i++) {
            d[3 * i] = s[4 * i]; d[3 * i + 1] = s[4 * i + 1]; d[3 * i + 2] = s[4 * i + 2];
            if (s[4 * i] > 8 || s[4 * i + 1] > 8 || s[4 * i + 2] > 8) lit++;
        }
    }
    sf->frames++; g_frames_n++;
    tx_publish(rgb, sf->w, sf->h, (uint32_t)g_frames_n);
    snprintf(tmp, sizeof tmp, "%s/.latest.tmp", g_frames);
    f = fopen(tmp, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", sf->w, sf->h);
        fwrite(rgb, 3, n, f);
        fclose(f);
        if ((int)g_frames_n <= g_keep) {
            snprintf(dst, sizeof dst, "%s/frame_%05lu_s%d.ppm", g_frames, g_frames_n, sf->idx);
            if (link(tmp, dst) < 0) { }
        }
        snprintf(dst, sizeof dst, "%s/latest.ppm", g_frames);
        if (rename(tmp, dst) < 0) { }
        /* contador para o visualizador ao vivo: ele só busca o quadro quando o número muda */
        snprintf(tmp, sizeof tmp, "%s/.seq.tmp", g_frames);
        f = fopen(tmp, "w");
        if (f) {
            fprintf(f, "%lu %d %d\n", g_frames_n, sf->w, sf->h);
            fclose(f);
            snprintf(dst, sizeof dst, "%s/seq.txt", g_frames);
            if (rename(tmp, dst) < 0) { }
        }
    }
    if (g_frames_n <= 60 || g_frames_n % 100 == 0)
        LOG("[glhost] quadro %lu (superfície %d, %dx%d): %.1f%% dos pixels acesos; %lu chamadas até aqui\n",
            g_frames_n, sf->idx, sf->w, sf->h, 100.0 * (double)lit / (double)n, g_total);
}

/* ------------------------------------------------------------------ EGL de verdade */
static void GL_APIENTRY mesa_dbg(GLenum src, GLenum type, GLuint id, GLenum sev, GLsizei len,
                                 const GLchar *msg, const void *user)
{
    static unsigned n;
    (void)src; (void)id; (void)len; (void)user;
    if (type != GL_DEBUG_TYPE_ERROR && sev != GL_DEBUG_SEVERITY_HIGH) return;
    if (++n <= 300) LOG("[mesa] %s (em %s)\n", msg, opname(g_op));
}

static void egl_up(void)
{
    EGLint ma = 0, mi = 0;
    setenv("LIBGL_ALWAYS_SOFTWARE", "1", 0);
    setenv("GALLIUM_DRIVER", "llvmpipe", 0);
    D = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
    if (D == EGL_NO_DISPLAY || !eglInitialize(D, &ma, &mi)) { LOG("[glhost] EGL sem janela não subiu\n"); exit(2); }
    eglBindAPI(EGL_OPENGL_ES_API);
    LOG("[glhost] EGL %d.%d (%s)\n", ma, mi, eglQueryString(D, EGL_VENDOR));
}

static struct surf *surf_of(uint64_t h)
{
    int i;
    for (i = 0; i < 64; i++) if (g_surf[i].s != EGL_NO_SURFACE && (uint64_t)(uintptr_t)&g_surf[i] == h) return &g_surf[i];
    return NULL;
}

static EGLContext ctx_of(uint64_t h)
{
    int i;
    for (i = 0; h && i < 64; i++) if (g_ctx[i] != EGL_NO_CONTEXT && (uint64_t)(uintptr_t)g_ctx[i] == h) return g_ctx[i];
    return EGL_NO_CONTEXT;
}

static int make_current(uint64_t hd, uint64_t hr, uint64_t hc)
{
    struct surf *d = surf_of(hd), *r = surf_of(hr);
    EGLContext c = ctx_of(hc);
    static int first = 1;
    if (!eglMakeCurrent(D, d ? d->s : EGL_NO_SURFACE, r ? r->s : EGL_NO_SURFACE, c)) {
        LOG("[glhost] eglMakeCurrent falhou: 0x%x\n", eglGetError());
        return 0;
    }
    g_cur_ctx = c;
    if (c != EGL_NO_CONTEXT && first) {
        first = 0;
        LOG("[glhost] GL_VERSION  = %s\n[glhost] GL_RENDERER = %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
    }
    if (c != EGL_NO_CONTEXT) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(mesa_dbg, NULL);
        while (glGetError() != GL_NO_ERROR) { }
    }
    return 1;
}

static void cfg_reply(EGLBoolean ok, EGLConfig *all, EGLint total, uint64_t max)
{
    uint64_t n = (uint64_t)total < max ? (uint64_t)total : max, i;
    P_u64(ok ? 1 : 0); P_u64((uint64_t)total); P_u64(n);
    for (i = 0; i < n; i++) P_u64((uint64_t)(uintptr_t)all[i]);
    REPLY();
}

static int handle_egl(uint32_t op)
{
    EGLConfig all[512];
    EGLint total = 0;
    switch (op) {
    case OPX_HELLO: {
        uint64_t magic = R_u64(), ver = R_u64();
        P_u64(magic == GLFWD_MAGIC && ver == GLFWD_VERSION); P_u64((uint64_t)g_w); P_u64((uint64_t)g_h);
        REPLY();
    } return 1;
    case OPX_EGL_CHOOSE: {
        size_t nb; const EGLint *al = R_blob(&nb);
        uint64_t max = R_u64();
        EGLint a[160], strict[160], n = 0, m = 0;
        size_t i;
        EGLBoolean ok;
        /* a HMI pede superfície de janela; aqui só existe pbuffer. Tamanhos de canal ficam. */
        for (i = 0; al && i + 1 < nb / 4 && al[i] != EGL_NONE && n < 150; i += 2) {
            EGLint k = al[i], v = al[i + 1];
            if (k == EGL_SURFACE_TYPE || k == EGL_NATIVE_RENDERABLE || k == EGL_NATIVE_VISUAL_TYPE ||
                k == EGL_NATIVE_VISUAL_ID || k == EGL_CONFIG_ID || k == EGL_MATCH_NATIVE_PIXMAP) continue;
            strict[m++] = k; strict[m++] = v;
            if (k == EGL_SAMPLES || k == EGL_SAMPLE_BUFFERS) continue;
            a[n++] = k; a[n++] = v;
        }
        strict[m++] = EGL_SURFACE_TYPE; strict[m++] = EGL_PBUFFER_BIT; strict[m] = EGL_NONE;
        a[n++] = EGL_SURFACE_TYPE; a[n++] = EGL_PBUFFER_BIT; a[n] = EGL_NONE;
        ok = eglChooseConfig(D, strict, all, 512, &total);
        if (!ok || total == 0) {
            ok = eglChooseConfig(D, a, all, 512, &total);
            LOG("[glhost] eglChooseConfig: nada com os atributos pedidos; sem multisample deu %d\n", (int)total);
        }
        cfg_reply(ok, all, total, max);
    } return 1;
    case OPX_EGL_GETCONFIGS: {
        uint64_t max = R_u64();
        cfg_reply(eglGetConfigs(D, all, 512, &total), all, total, max);
    } return 1;
    case OPX_EGL_GETCONFIGATTR: {
        EGLConfig c = (EGLConfig)(uintptr_t)R_u64();
        EGLint attr = (EGLint)R_u64(), v = 0;
        EGLBoolean ok = eglGetConfigAttrib(D, c, attr, &v);
        if (attr == EGL_SURFACE_TYPE) v |= EGL_WINDOW_BIT;
        P_u64(ok ? 1 : 0); P_u64((uint64_t)(int64_t)v);
        REPLY();
    } return 1;
    case OPX_EGL_CREATECTX: {
        EGLConfig c = (EGLConfig)(uintptr_t)R_u64();
        EGLContext share = ctx_of(R_u64()), ctx;
        size_t nb, i; const EGLint *al = R_blob(&nb);
        EGLint a[16], n = 0, major = 2, minor = 0;
        int k;
        for (i = 0; al && i + 1 < nb / 4 && al[i] != EGL_NONE; i += 2) {
            if (al[i] == EGL_CONTEXT_MAJOR_VERSION) major = al[i + 1];
            if (al[i] == EGL_CONTEXT_MINOR_VERSION) minor = al[i + 1];
        }
        a[n++] = EGL_CONTEXT_MAJOR_VERSION; a[n++] = major;
        a[n++] = EGL_CONTEXT_MINOR_VERSION; a[n++] = minor;
        a[n++] = EGL_CONTEXT_FLAGS_KHR; a[n++] = EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
        a[n] = EGL_NONE;
        ctx = eglCreateContext(D, c, share, a);
        if (ctx == EGL_NO_CONTEXT) { a[4] = EGL_NONE; ctx = eglCreateContext(D, c, share, a); }
        for (k = 0; ctx != EGL_NO_CONTEXT && k < 64; k++) if (g_ctx[k] == EGL_NO_CONTEXT) { g_ctx[k] = ctx; break; }
        LOG("[glhost] contexto ES %d.%d %s (compartilhando: %s)\n", major, minor,
            ctx != EGL_NO_CONTEXT ? "criado" : "FALHOU", share != EGL_NO_CONTEXT ? "sim" : "não");
        P_u64((uint64_t)(uintptr_t)ctx);
        REPLY();
    } return 1;
    case OPX_EGL_DESTROYCTX: {
        uint64_t h = R_u64();
        int k;
        for (k = 0; k < 64; k++) if (g_ctx[k] != EGL_NO_CONTEXT && (uint64_t)(uintptr_t)g_ctx[k] == h) {
            eglDestroyContext(D, g_ctx[k]); g_ctx[k] = EGL_NO_CONTEXT;
        }
    } return 1;
    case OPX_EGL_CREATESURF: {
        uint64_t kind = R_u64();
        EGLConfig c = (EGLConfig)(uintptr_t)R_u64();
        int w = (int)R_u64(), h = (int)R_u64(), k;
        EGLint a[5];
        struct surf *sf = NULL;
        if (kind == 0 || w <= 0 || h <= 0) { w = kind == 0 ? g_w : (w > 0 ? w : 1); h = kind == 0 ? g_h : (h > 0 ? h : 1); }
        a[0] = EGL_WIDTH; a[1] = w; a[2] = EGL_HEIGHT; a[3] = h; a[4] = EGL_NONE;
        for (k = 0; k < 64 && !sf; k++) if (g_surf[k].s == EGL_NO_SURFACE) sf = &g_surf[k];
        if (sf) {
            sf->s = eglCreatePbufferSurface(D, c, a);
            sf->w = w; sf->h = h; sf->idx = (int)(sf - g_surf); sf->frames = 0;
            LOG("[glhost] superfície %d (%s) %dx%d %s\n", sf->idx, kind ? "pbuffer" : "janela", w, h,
                sf->s != EGL_NO_SURFACE ? "criada" : "FALHOU");
        }
        P_u64(sf && sf->s != EGL_NO_SURFACE ? (uint64_t)(uintptr_t)sf : 0);
        REPLY();
    } return 1;
    case OPX_EGL_DESTROYSURF: {
        struct surf *sf = surf_of(R_u64());
        if (sf) { eglDestroySurface(D, sf->s); sf->s = EGL_NO_SURFACE; }
    } return 1;
    case OPX_EGL_MAKECURRENT: {
        uint64_t d = R_u64(), r = R_u64(), c = R_u64();
        P_u64((uint64_t)make_current(d, r, c));
        REPLY();
    } return 1;
    case OPX_SWITCH: {
        uint64_t c = R_u64(), d = R_u64(), r = R_u64();
        make_current(d, r, c);
    } return 1;
    case OPX_EGL_SWAP: {
        struct surf *sf = surf_of(R_u64());
        if (sf && g_cur_ctx != EGL_NO_CONTEXT) { dump_frame(sf); eglSwapBuffers(D, sf->s); }
        P_u64(sf ? 1 : 0);
        REPLY();
    } return 1;
    case OPX_EGL_QUERYSURF: {
        struct surf *sf = surf_of(R_u64());
        EGLint attr = (EGLint)R_u64(), v = 0;
        EGLBoolean ok = sf ? eglQuerySurface(D, sf->s, attr, &v) : EGL_FALSE;
        P_u64(ok ? 1 : 0); P_u64((uint64_t)(int64_t)v);
        REPLY();
    } return 1;
    case OPX_EGL_QUERYCTX: {
        EGLContext c = ctx_of(R_u64());
        EGLint attr = (EGLint)R_u64(), v = 0;
        EGLBoolean ok = c != EGL_NO_CONTEXT ? eglQueryContext(D, c, attr, &v) : EGL_FALSE;
        P_u64(ok ? 1 : 0); P_u64((uint64_t)(int64_t)v);
        REPLY();
    } return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ GL escrito à mão */
static int getv_count(GLenum p)
{
    GLint n = 1;
    switch (p) {
    case GL_ALIASED_LINE_WIDTH_RANGE: case GL_ALIASED_POINT_SIZE_RANGE: case GL_DEPTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS: case GL_MULTISAMPLE_LINE_WIDTH_RANGE: return 2;
    case GL_VIEWPORT: case GL_SCISSOR_BOX: case GL_COLOR_CLEAR_VALUE: case GL_BLEND_COLOR:
    case GL_COLOR_WRITEMASK: return 4;
    case GL_PRIMITIVE_BOUNDING_BOX: return 8;
    case GL_COMPRESSED_TEXTURE_FORMATS: glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n); return n;
    case GL_PROGRAM_BINARY_FORMATS: glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &n); return n;
    case GL_SHADER_BINARY_FORMATS: glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &n); return n;
    }
    return 1;
}

/* dados de pixel: bloco que veio no pedido, ou deslocamento num PBO */
static const void *R_pixels(void)
{
    size_t n;
    if (R_u64()) return (const void *)(uintptr_t)R_u64();
    return R_blob(&n);
}

static const void *R_indices(void) { return R_pixels(); }

/* separa nomes terminados em NUL de um bloco */
static const GLchar **split_names(const char *b, size_t nb, int count)
{
    static const GLchar *v[1024];
    size_t o = 0;
    int i;
    for (i = 0; i < count && i < 1024; i++) {
        v[i] = (b && o < nb) ? b + o : "";
        while (b && o < nb && b[o]) o++;
        o++;
    }
    return v;
}

static int handle_manual(uint32_t op)
{
    switch (op) {
    case OP_glGetString: case OP_glGetStringi: {
        GLenum name = (GLenum)R_u64();
        const GLubyte *s = op == OP_glGetString ? glGetString(name) : glGetStringi(name, (GLuint)R_u64());
        size_t n = s ? strlen((const char *)s) + 1 : 0;
        P_u64(n);
        if (n) memcpy(P_reserve(n), s, n);
        REPLY();
    } return 1;
    case OP_glGetBooleanv: case OP_glGetIntegerv: case OP_glGetFloatv: case OP_glGetInteger64v: {
        GLenum pname = (GLenum)R_u64();
        int cnt = getv_count(pname);
        size_t elem = op == OP_glGetBooleanv ? 1 : op == OP_glGetInteger64v ? 8 : 4;
        void *b;
        if (cnt < 1) cnt = 0;
        P_u64((uint64_t)cnt);
        b = P_reserve((size_t)cnt * elem + 64);
        if (op == OP_glGetBooleanv) glGetBooleanv(pname, b);
        else if (op == OP_glGetIntegerv) glGetIntegerv(pname, b);
        else if (op == OP_glGetFloatv) glGetFloatv(pname, b);
        else glGetInteger64v(pname, b);
        REPLY();
    } return 1;
    case OP_glGetActiveUniformBlockiv: {
        GLuint program = (GLuint)R_u64(), index = (GLuint)R_u64();
        GLenum pname = (GLenum)R_u64();
        GLint cnt = 1;
        if (pname == GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES)
            glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &cnt);
        if (cnt < 0) cnt = 0;
        P_u64((uint64_t)cnt);
        glGetActiveUniformBlockiv(program, index, pname, P_reserve((size_t)cnt * 4 + 16));
        REPLY();
    } return 1;
    case OP_glGetError: {
        GLenum e = g_pending_err ? g_pending_err : glGetError();
        g_pending_err = 0;
        P_u64(e);
        REPLY();
    } return 1;
    case OP_glFinish: glFinish(); P_u64(1); REPLY(); return 1;
    case OP_glFlush: glFlush(); return 1;
    case OP_glShaderSource: {
        GLuint shader = (GLuint)R_u64();
        size_t n; const GLchar *src = R_blob(&n);
        GLint len = n ? (GLint)n - 1 : 0;
        /* Mesa (strict GLSL ES 3.00) rejects "precision <qual> uint;" which the Adreno compiler the
         * mss was built for tolerates; blank any such line so the shader compiles. Only lines that
         * contain BOTH "precision" and "uint" are neutralised — normal uint use is untouched. */
        char *fixed = NULL;
        if (src && len > 0) {
            fixed = malloc((size_t)len + 1);
            if (fixed) {
                memcpy(fixed, src, (size_t)len); fixed[len] = 0;
                char *p = fixed, *pend = fixed + len;
                while (p < pend) {
                    char *nl = memchr(p, '\n', (size_t)(pend - p));
                    char *le = nl ? nl : pend;
                    size_t ll = (size_t)(le - p), i; int hasP = 0, hasU = 0;
                    for (i = 0; i + 9 <= ll; i++) if (!memcmp(p + i, "precision", 9)) { hasP = 1; break; }
                    for (i = 0; i + 4 <= ll; i++) if (!memcmp(p + i, "uint", 4))      { hasU = 1; break; }
                    if (hasP && hasU) { for (i = 0; i < ll; i++) if (p[i] != '\r') p[i] = ' '; }
                    p = nl ? nl + 1 : pend;
                }
                src = fixed;
            }
        }
        if (src) glShaderSource(shader, 1, &src, &len);
        { /* DIAG: dump the shader source so we can read the Rgba2yuv NV12 packing layout */
            char fn[64]; snprintf(fn, sizeof fn, "/out/frames/shader_%u.glsl", (unsigned)shader);
            FILE *sf = fopen(fn, "wb"); if (sf) { if (src) fwrite(src, 1, (size_t)len, sf); fclose(sf); }
        }
        free(fixed);
    } return 1;
    case OP_glGetUniformIndices: {
        GLuint program = (GLuint)R_u64();
        int count = (int)R_u64();
        size_t nb; const char *b = R_blob(&nb);
        glGetUniformIndices(program, count, split_names(b, nb, count), P_reserve(SZ(count * 4)));
        REPLY();
    } return 1;
    case OP_glTransformFeedbackVaryings: {
        GLuint program = (GLuint)R_u64();
        int count = (int)R_u64();
        GLenum mode = (GLenum)R_u64();
        size_t nb; const char *b = R_blob(&nb);
        glTransformFeedbackVaryings(program, count, split_names(b, nb, count), mode);
    } return 1;
    case OP_glTexImage2D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(), ifmt = (GLint)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(); GLint border = (GLint)R_u64();
        GLenum format = (GLenum)R_u64(), type = (GLenum)R_u64();
        glTexImage2D(target, level, ifmt, w, h, border, format, type, R_pixels());
    } return 1;
    case OP_glTexSubImage2D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(), x = (GLint)R_u64(), y = (GLint)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64();
        GLenum format = (GLenum)R_u64(), type = (GLenum)R_u64();
        glTexSubImage2D(target, level, x, y, w, h, format, type, R_pixels());
    } return 1;
    case OP_glTexImage3D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(), ifmt = (GLint)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(), d = (GLsizei)R_u64(); GLint border = (GLint)R_u64();
        GLenum format = (GLenum)R_u64(), type = (GLenum)R_u64();
        glTexImage3D(target, level, ifmt, w, h, d, border, format, type, R_pixels());
    } return 1;
    case OP_glTexSubImage3D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(), x = (GLint)R_u64(), y = (GLint)R_u64(), z = (GLint)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(), d = (GLsizei)R_u64();
        GLenum format = (GLenum)R_u64(), type = (GLenum)R_u64();
        glTexSubImage3D(target, level, x, y, z, w, h, d, format, type, R_pixels());
    } return 1;
    case OP_glCompressedTexImage2D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(); GLenum ifmt = (GLenum)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(); GLint border = (GLint)R_u64(); GLsizei sz = (GLsizei)R_u64();
        glCompressedTexImage2D(target, level, ifmt, w, h, border, sz, R_pixels());
    } return 1;
    case OP_glCompressedTexSubImage2D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(), x = (GLint)R_u64(), y = (GLint)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(); GLenum format = (GLenum)R_u64(); GLsizei sz = (GLsizei)R_u64();
        glCompressedTexSubImage2D(target, level, x, y, w, h, format, sz, R_pixels());
    } return 1;
    case OP_glCompressedTexImage3D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(); GLenum ifmt = (GLenum)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(), d = (GLsizei)R_u64(); GLint border = (GLint)R_u64();
        GLsizei sz = (GLsizei)R_u64();
        glCompressedTexImage3D(target, level, ifmt, w, h, d, border, sz, R_pixels());
    } return 1;
    case OP_glCompressedTexSubImage3D: {
        GLenum target = (GLenum)R_u64(); GLint level = (GLint)R_u64(), x = (GLint)R_u64(), y = (GLint)R_u64(), z = (GLint)R_u64();
        GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64(), d = (GLsizei)R_u64(); GLenum format = (GLenum)R_u64();
        GLsizei sz = (GLsizei)R_u64();
        glCompressedTexSubImage3D(target, level, x, y, z, w, h, d, format, sz, R_pixels());
    } return 1;
    case OP_glReadPixels: {
        GLint x = (GLint)R_u64(), y = (GLint)R_u64(); GLsizei w = (GLsizei)R_u64(), h = (GLsizei)R_u64();
        GLenum format = (GLenum)R_u64(), type = (GLenum)R_u64();
        if (R_u64()) { glReadPixels(x, y, w, h, format, type, (void *)(uintptr_t)R_u64()); return 1; }
        glReadPixels(x, y, w, h, format, type, P_reserve((size_t)R_u64() + 64));
        REPLY();
    } return 1;
    case OP_glVertexAttribPointer: case OP_glVertexAttribIPointer: {
        GLuint index = (GLuint)R_u64(); GLint size = (GLint)R_u64(); GLenum type = (GLenum)R_u64();
        GLboolean norm = (GLboolean)R_u64(); GLsizei stride = (GLsizei)R_u64();
        const void *p = (const void *)(uintptr_t)R_u64();
        if (op == OP_glVertexAttribPointer) glVertexAttribPointer(index, size, type, norm, stride, p);
        else glVertexAttribIPointer(index, size, type, stride, p);
    } return 1;
    case OP_glDrawElements: {
        GLenum mode = (GLenum)R_u64(); GLsizei count = (GLsizei)R_u64(); GLenum type = (GLenum)R_u64();
        glDrawElements(mode, count, type, R_indices());
    } return 1;
    case OP_glDrawRangeElements: {
        GLenum mode = (GLenum)R_u64(); GLuint s = (GLuint)R_u64(), e = (GLuint)R_u64();
        GLsizei count = (GLsizei)R_u64(); GLenum type = (GLenum)R_u64();
        glDrawRangeElements(mode, s, e, count, type, R_indices());
    } return 1;
    case OP_glDrawElementsInstanced: {
        GLenum mode = (GLenum)R_u64(); GLsizei count = (GLsizei)R_u64(); GLenum type = (GLenum)R_u64();
        GLsizei inst = (GLsizei)R_u64();
        glDrawElementsInstanced(mode, count, type, R_indices(), inst);
    } return 1;
    case OP_glDrawElementsBaseVertex: {
        GLenum mode = (GLenum)R_u64(); GLsizei count = (GLsizei)R_u64(); GLenum type = (GLenum)R_u64();
        GLint bv = (GLint)R_u64();
        glDrawElementsBaseVertex(mode, count, type, R_indices(), bv);
    } return 1;
    case OP_glDrawRangeElementsBaseVertex: {
        GLenum mode = (GLenum)R_u64(); GLuint s = (GLuint)R_u64(), e = (GLuint)R_u64();
        GLsizei count = (GLsizei)R_u64(); GLenum type = (GLenum)R_u64(); GLint bv = (GLint)R_u64();
        glDrawRangeElementsBaseVertex(mode, s, e, count, type, R_indices(), bv);
    } return 1;
    case OP_glDrawElementsInstancedBaseVertex: {
        GLenum mode = (GLenum)R_u64(); GLsizei count = (GLsizei)R_u64(); GLenum type = (GLenum)R_u64();
        GLsizei inst = (GLsizei)R_u64(); GLint bv = (GLint)R_u64();
        glDrawElementsInstancedBaseVertex(mode, count, type, R_indices(), inst, bv);
    } return 1;
    case OP_glMapBufferRange: {              /* o shim quer o conteúdo atual do trecho */
        GLenum target = (GLenum)R_u64(); GLintptr off = (GLintptr)R_u64(); GLsizeiptr len = (GLsizeiptr)R_u64();
        uint64_t *ok = P_reserve(8);
        void *dst = P_reserve(SZ(len)), *src = glMapBufferRange(target, off, len, GL_MAP_READ_BIT);
        if (src) { memcpy(dst, src, SZ(len)); glUnmapBuffer(target); *ok = 1; }
        REPLY();
    } return 1;
    case OP_glUnmapBuffer: case OP_glFlushMappedBufferRange: {   /* o shim devolve o que a HMI escreveu */
        GLenum target = (GLenum)R_u64(); GLintptr off = (GLintptr)R_u64();
        size_t n; const void *data = R_blob(&n);
        if (data && n) glBufferSubData(target, off, (GLsizeiptr)n, data);
    } return 1;
    case OP_glGetBufferParameteriv: {
        GLenum target = (GLenum)R_u64(), pname = (GLenum)R_u64();
        glGetBufferParameteriv(target, pname, P_reserve(8));
        REPLY();
    } return 1;
    }
    return 0;
}

/* shader que não compila ou programa que não liga: mostrar o motivo e o fonte ajuda muito */
static void after_op(uint32_t op, uint64_t arg0)
{
    GLint ok = 1, n = 0;
    static char log[8192];
    static unsigned shown;
    if (op == OP_glCompileShader) {
        glGetShaderiv((GLuint)arg0, GL_COMPILE_STATUS, &ok);
        if (ok || ++shown > 40) return;
        glGetShaderInfoLog((GLuint)arg0, sizeof log, &n, log);
        LOG("[glhost] SHADER %u NÃO COMPILOU: %s\n", (unsigned)arg0, log);
        glGetShaderSource((GLuint)arg0, sizeof log, &n, log);
        LOG("[glhost] fonte do shader %u (%d bytes):\n%s\n[glhost] fim do fonte\n", (unsigned)arg0, n, log);
    } else if (op == OP_glLinkProgram) {
        glGetProgramiv((GLuint)arg0, GL_LINK_STATUS, &ok);
        if (ok || ++shown > 40) return;
        glGetProgramInfoLog((GLuint)arg0, sizeof log, &n, log);
        LOG("[glhost] PROGRAMA %u NÃO LIGOU: %s\n", (unsigned)arg0, log);
    }
    while (glGetError() != GL_NO_ERROR) { }
}

static void dispatch(uint32_t op)
{
    if (handle_egl(op) || handle_manual(op)) return;
    switch (op) {
#include "gen/host_gen.inc"
    default:
        LOG("[glhost] operação desconhecida %u\n", op);
    }
}

static void stats(void)
{
    unsigned long best;
    uint32_t i, k, top;
    static char used[NOPS];
    LOG("[glhost] resumo: %lu chamadas, %lu quadros\n[glhost] mais chamadas:", g_total, g_frames_n);
    memset(used, 0, sizeof used);
    for (k = 0; k < 45; k++) {
        best = 0; top = 0;
        for (i = 0; i < NOPS; i++) if (!used[i] && g_count[i] > best) { best = g_count[i]; top = i; }
        if (!best) break;
        used[top] = 1;
        LOG(" %s=%lu", opname(top), best);
    }
    for (i = 0, k = 0; i < NOPS; i++) if (g_count[i]) k++;
    LOG("\n[glhost] operações distintas usadas: %u\n[glhost] todas:", k);
    for (i = 0; i < NOPS; i++) if (g_count[i]) LOG(" %s", opname(i));
    LOG("\n");
    for (i = 0; i < NOPS; i++)
        if (g_errs[i]) LOG("[glhost] erro GL em %s: %lu vezes (primeiro 0x%04x)\n", opname(i), g_errs[i], g_first_err[i]);
}

static void serve(void)
{
    uint64_t hdr[2];
    ib_len = ib_pos = 0;
    while (readn(hdr, 16) == 0) {
        uint32_t op = (uint32_t)hdr[0];
        size_t len = (size_t)hdr[1];
        uint64_t arg0 = 0;
        if (len > rq_cap) { rq_cap = len + 4096; rq = realloc(rq, rq_cap); }
        if (len && readn(rq, len) < 0) break;
        rp = rq; rend = rq + len; rep_len = 0; g_bad = 0; g_op = op;
        if (len >= 8) memcpy(&arg0, rq, 8);
        if (g_trace) LOG("[op] %s (%zu bytes)\n", opname(op), len);
        dispatch(op);
        if (op < NOPS) g_count[op]++;
        g_total++;
        if (g_bad) LOG("[glhost] pedido truncado em %s\n", opname(op));
        if (op >= OP_GL_BASE && g_cur_ctx != EGL_NO_CONTEXT) {
            if (op == OP_glCompileShader || op == OP_glLinkProgram) after_op(op, arg0);
            if (g_check && op != OP_glGetError) {
                GLenum e = glGetError();
                if (e != GL_NO_ERROR) {
                    if (!g_errs[op]++) { g_first_err[op] = e; LOG("[glhost] erro GL 0x%04x em %s\n", e, opname(op)); }
                    if (!g_pending_err) g_pending_err = e;
                }
            }
        }
    }
    stats();
}

/* a bancada encerra o glhost com SIGTERM: mostrar o resumo antes de sair */
static void on_term(int sig)
{
    (void)sig;
    stats();
    _exit(0);
}

int main(int argc, char **argv)
{
    struct sockaddr_un a;
    int ls, i;
    if (argc < 2) { fprintf(stderr, "uso: glhost <socket> [largura altura] [pasta dos quadros]\n"); return 2; }
    if (argc >= 4) { g_w = atoi(argv[2]); g_h = atoi(argv[3]); }
    if (argc >= 5) g_frames = argv[4];
    g_trace = getenv("GLHOST_TRACE") != NULL;
    g_check = getenv("GLHOST_NOCHECK") == NULL;
    if (getenv("GLHOST_KEEP")) g_keep = atoi(getenv("GLHOST_KEEP"));
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, on_term);
    signal(SIGINT, on_term);
    mkdir(g_frames, 0777);
    for (i = 0; i < 64; i++) { g_surf[i].s = EGL_NO_SURFACE; g_ctx[i] = EGL_NO_CONTEXT; }
    rep_base = mmap(NULL, REP_CAP, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (rep_base == MAP_FAILED) { perror("mmap"); return 2; }
    rep = rep_base + 8;
    egl_up();
    tx_start();

    unlink(argv[1]);
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, argv[1], sizeof a.sun_path - 1);
    ls = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ls < 0 || bind(ls, (struct sockaddr *)&a, sizeof a) < 0 || listen(ls, 4) < 0) { perror("socket"); return 2; }
    chmod(argv[1], 0777);
    LOG("[glhost] escutando em %s; tela %dx%d; quadros em %s\n", argv[1], g_w, g_h, g_frames);
    for (;;) {
        cfd = accept(ls, NULL, NULL);
        if (cfd < 0) { if (errno == EINTR) continue; break; }
        LOG("[glhost] cliente conectou\n");
        serve();
        close(cfd);
        if (getenv("GLHOST_ONESHOT")) break;
    }
    return 0;
}
