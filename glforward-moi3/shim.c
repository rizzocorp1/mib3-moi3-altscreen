/* shim.c — lado da HMI do encaminhador de OpenGL do sandbox MOI3 (Marco 3, 11/10/2026).
 *
 * Vira libGLESv2_adreno.so (e o libEGL_adreno.so é uma casca que depende dele). A ponte JNI da
 * HMI (libjopengl_es3.so) liga contra esses dois nomes. Aqui nada é desenhado: cada chamada é
 * empacotada (ver proto.h) e mandada por socket Unix ao glhost, que roda Mesa llvmpipe fora do
 * chroot. O que volta (valores de retorno, textos, pixels) é copiado para a memória da HMI.
 *
 * É ligado contra a glibc 2.23 do PRÓPRIO firmware (build.sh), para carregar dentro da JVM sem
 * misturar glibc. Por isso usa só o básico da libc.
 *
 * Exporta o que o Adreno de verdade exporta: OpenGL ES 3.2 inteiro e EGL 1.4. As dez funções de
 * EGL 1.5 que a ponte importa não existem no driver real e também não existem aqui.
 *
 * Ambiente:  MOI3_GL_SOCKET  caminho do socket (default /tmp/moi3-gl.sock)
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
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include "proto.h"
#include "gen/ops_gen.h"

#define SZ(e) ((e) > 0 ? (size_t)(e) : (size_t)0)

static void LOG(const char *fmt, ...)
{
    char b[512];
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof b - 1) n = (int)sizeof b - 1;
    if (n > 0 && write(2, b, (size_t)n) < 0) { }
}

static void unimpl(const char *name, int *once)
{
    if (!*once) { *once = 1; LOG("[glfwd] NAO IMPLEMENTADA: %s\n", name); }
}

/* ------------------------------------------------------------------ transporte */
static int g_fd = -1, g_dead, g_tried;
static pthread_mutex_t L = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static uint8_t *tx, *rx, *rp;
static size_t tx_cap, tx_len, rq_start, rx_cap;
static uint8_t zero_reply[4096];

static void dead(const char *why)
{
    if (!g_dead) { g_dead = 1; LOG("[glfwd] sem glhost (%s): as chamadas GL viram no-op\n", why); }
}

static int io_write(const uint8_t *p, size_t n)
{
    while (n) {
        ssize_t r = send(g_fd, p, n, MSG_NOSIGNAL);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        p += r; n -= (size_t)r;
    }
    return 0;
}

static int io_read(uint8_t *p, size_t n)
{
    while (n) {
        ssize_t r = read(g_fd, p, n);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;
        p += r; n -= (size_t)r;
    }
    return 0;
}

static void flush(void)
{
    if (tx_len && !g_dead && io_write(tx, tx_len) < 0) dead("falha ao escrever");
    tx_len = 0;
}

static void ensure(size_t extra)
{
    if (tx_len + extra > tx_cap) {
        size_t c = tx_cap ? tx_cap : (size_t)1 << 20;
        while (c < tx_len + extra) c *= 2;
        tx = realloc(tx, c);
        tx_cap = c;
    }
}

static void W_u64(uint64_t v) { ensure(8); memcpy(tx + tx_len, &v, 8); tx_len += 8; }
static void W_f32(float f) { uint64_t v = 0; memcpy(&v, &f, 4); W_u64(v); }

static void W_blob(const void *p, size_t n)
{
    size_t pad = (8 - (n & 7)) & 7;
    if (!p) { W_u64(0); return; }
    W_u64(GLFWD_PRESENT | (uint64_t)n);
    ensure(n + pad);
    if (n) memcpy(tx + tx_len, p, n);
    memset(tx + tx_len + n, 0, pad);
    tx_len += n + pad;
}

static void W_str(const char *s) { W_blob(s, s ? strlen(s) + 1 : 0); }

static void raw_begin(uint32_t op) { rq_start = tx_len; W_u64(op); W_u64(0); }
static void raw_close(void) { uint64_t len = tx_len - rq_start - 16; memcpy(tx + rq_start + 8, &len, 8); }

static uint64_t R_u64(void)
{
    uint64_t v = 0;
    if (g_dead) return 0;
    memcpy(&v, rp, 8); rp += 8;
    return v;
}

static void R_copy(void *dst, size_t n)
{
    if (g_dead) { if (dst && n) memset(dst, 0, n); return; }
    if (dst && n) memcpy(dst, rp, n);
    rp += (n + 7) & ~(size_t)7;
}

/* manda o que está pendente e lê uma resposta inteira; o lock continua preso até W_done */
static void W_call(void)
{
    uint64_t len = 0;
    raw_close();
    flush();
    rp = zero_reply;
    if (g_dead) return;
    if (io_read((uint8_t *)&len, 8) < 0) { dead("falha ao ler"); return; }
    if (len + 8 > rx_cap) { rx_cap = len + 4096; rx = realloc(rx, rx_cap); }
    if (len && io_read(rx, len) < 0) { dead("falha ao ler"); return; }
    rp = rx;
}

static void W_done(void) { pthread_mutex_unlock(&L); }

static void W_end(void)
{
    raw_close();
    if (tx_len > (256u << 10)) flush();
    pthread_mutex_unlock(&L);
}

static void connect_once(void)
{
    struct sockaddr_un a;
    const char *path;
    int fd;
    if (g_tried) return;
    g_tried = 1;
    path = getenv("MOI3_GL_SOCKET");
    if (!path || !*path) path = "/tmp/moi3-gl.sock";
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0 || connect(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        if (fd >= 0) close(fd);
        LOG("[glfwd] não conectou em %s (errno %d)\n", path, errno);
        dead("socket");
        return;
    }
    g_fd = fd;
    raw_begin(OPX_HELLO);
    W_u64(GLFWD_MAGIC); W_u64(GLFWD_VERSION);
    W_call();
    if (R_u64() != 1) dead("o glhost recusou o protocolo");
    else LOG("[glfwd] ligado ao glhost em %s\n", path);
}

/* ------------------------------------------------------------------ estado por contexto */
struct mapping { GLuint buf; uint8_t *shadow; GLintptr off; GLsizeiptr len; GLbitfield access; };

#define NTGT 14
#define NVE 4096
struct cstate {
    GLint un_align, un_rowlen, un_imgh, un_skiprows, un_skippix, un_skipimg;
    GLint pk_align, pk_rowlen, pk_skiprows, pk_skippix;
    GLuint bound[NTGT];                       /* buffer ligado em cada alvo */
    GLuint vao;
    struct { GLuint key, ebo; } ve[NVE];      /* buffer de índices de cada VAO */
    struct mapping maps[32];
};
struct gctx { uint64_t h; struct cstate st; };

static struct gctx *g_ctxs[64];
static struct cstate st0 = { .un_align = 4, .pk_align = 4 };
static __thread uint64_t t_ctx, t_draw, t_read;
static __thread struct cstate *t_st;
static uint64_t h_ctx, h_draw, h_read;        /* o que o glhost tem como corrente */
#define ST (t_st ? t_st : &st0)

static struct cstate *state_of(uint64_t h)
{
    int i;
    for (i = 0; i < 64; i++) if (g_ctxs[i] && g_ctxs[i]->h == h) return &g_ctxs[i]->st;
    return &st0;
}

static void sync_ctx(void)
{
    if (!t_ctx || (t_ctx == h_ctx && t_draw == h_draw && t_read == h_read)) return;
    raw_begin(OPX_SWITCH);
    W_u64(t_ctx); W_u64(t_draw); W_u64(t_read);
    raw_close();
    h_ctx = t_ctx; h_draw = t_draw; h_read = t_read;
}

static void W_begin(uint32_t op)
{
    pthread_mutex_lock(&L);
    connect_once();
    if (op >= OP_GL_BASE) sync_ctx();
    raw_begin(op);
}

static int tslot(GLenum t)
{
    switch (t) {
    case GL_ARRAY_BUFFER: return 0;
    case GL_ELEMENT_ARRAY_BUFFER: return 1;
    case GL_PIXEL_PACK_BUFFER: return 2;
    case GL_PIXEL_UNPACK_BUFFER: return 3;
    case GL_COPY_READ_BUFFER: return 4;
    case GL_COPY_WRITE_BUFFER: return 5;
    case GL_TRANSFORM_FEEDBACK_BUFFER: return 6;
    case GL_UNIFORM_BUFFER: return 7;
    case GL_ATOMIC_COUNTER_BUFFER: return 8;
    case GL_DISPATCH_INDIRECT_BUFFER: return 9;
    case GL_DRAW_INDIRECT_BUFFER: return 10;
    case GL_SHADER_STORAGE_BUFFER: return 11;
    case GL_TEXTURE_BUFFER: return 12;
    }
    return 13;
}

static unsigned ve_find(struct cstate *s, GLuint vao)
{
    unsigned i = (vao * 2654435761u) % NVE, n;
    for (n = 0; n < NVE; n++, i = (i + 1) % NVE)
        if (s->ve[i].key == vao + 1 || s->ve[i].key == 0) return i;
    return NVE;
}

static GLuint ebo_get(struct cstate *s, GLuint vao)
{
    unsigned i = ve_find(s, vao);
    return (i < NVE && s->ve[i].key) ? s->ve[i].ebo : 0;
}

static void ebo_set(struct cstate *s, GLuint vao, GLuint ebo)
{
    unsigned i = ve_find(s, vao);
    if (i < NVE) { s->ve[i].key = vao + 1; s->ve[i].ebo = ebo; }
}

static void H_pixelstore(GLenum pname, GLint v)
{
    struct cstate *s = ST;
    switch (pname) {
    case GL_UNPACK_ALIGNMENT: s->un_align = v; break;
    case GL_UNPACK_ROW_LENGTH: s->un_rowlen = v; break;
    case GL_UNPACK_IMAGE_HEIGHT: s->un_imgh = v; break;
    case GL_UNPACK_SKIP_ROWS: s->un_skiprows = v; break;
    case GL_UNPACK_SKIP_PIXELS: s->un_skippix = v; break;
    case GL_UNPACK_SKIP_IMAGES: s->un_skipimg = v; break;
    case GL_PACK_ALIGNMENT: s->pk_align = v; break;
    case GL_PACK_ROW_LENGTH: s->pk_rowlen = v; break;
    case GL_PACK_SKIP_ROWS: s->pk_skiprows = v; break;
    case GL_PACK_SKIP_PIXELS: s->pk_skippix = v; break;
    }
}

static void H_bindbuffer(GLenum target, GLuint buffer)
{
    struct cstate *s = ST;
    s->bound[tslot(target)] = buffer;
    if (target == GL_ELEMENT_ARRAY_BUFFER) ebo_set(s, s->vao, buffer);
}

static void H_bindvao(GLuint vao)
{
    struct cstate *s = ST;
    s->vao = vao;
    s->bound[1] = ebo_get(s, vao);
}

static void H_delbuffers(GLsizei n, const GLuint *ids)
{
    struct cstate *s = ST;
    int i, k;
    for (i = 0; ids && i < n; i++) {
        for (k = 0; k < NTGT; k++) if (ids[i] && s->bound[k] == ids[i]) s->bound[k] = 0;
        if (ids[i] && ebo_get(s, s->vao) == ids[i]) ebo_set(s, s->vao, 0);
        for (k = 0; k < 32; k++)
            if (s->maps[k].shadow && s->maps[k].buf == ids[i]) { free(s->maps[k].shadow); s->maps[k].shadow = NULL; }
    }
}

static void H_delvaos(GLsizei n, const GLuint *ids)
{
    struct cstate *s = ST;
    int i;
    for (i = 0; ids && i < n; i++) {
        ebo_set(s, ids[i], 0);
        if (s->vao == ids[i]) { s->vao = 0; s->bound[1] = ebo_get(s, 0); }
    }
}

/* ------------------------------------------------------------------ EGL 1.4 */
#define DPY ((EGLDisplay)(uintptr_t)1)
static EGLint g_eglerr = EGL_SUCCESS;

static size_t attr_len(const EGLint *a)
{
    size_t n = 0;
    if (!a) return 0;
    while (a[n] != EGL_NONE && n < 126) n += 2;
    return n + 1;
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType id) { (void)id; return DPY; }
EGLDisplay eglGetCurrentDisplay(void) { return t_ctx ? DPY : EGL_NO_DISPLAY; }
EGLContext eglGetCurrentContext(void) { return (EGLContext)(uintptr_t)t_ctx; }
EGLSurface eglGetCurrentSurface(EGLint rd) { return (EGLSurface)(uintptr_t)(rd == EGL_READ ? t_read : t_draw); }
EGLint eglGetError(void) { EGLint e = g_eglerr; g_eglerr = EGL_SUCCESS; return e; }
EGLBoolean eglBindAPI(EGLenum api) { return api == EGL_OPENGL_ES_API ? EGL_TRUE : EGL_FALSE; }
EGLenum eglQueryAPI(void) { return EGL_OPENGL_ES_API; }
EGLBoolean eglSwapInterval(EGLDisplay d, EGLint i) { (void)d; (void)i; return EGL_TRUE; }
EGLBoolean eglWaitNative(EGLint e) { (void)e; return EGL_TRUE; }
EGLBoolean eglSurfaceAttrib(EGLDisplay d, EGLSurface s, EGLint a, EGLint v) { (void)d; (void)s; (void)a; (void)v; return EGL_TRUE; }
EGLBoolean eglBindTexImage(EGLDisplay d, EGLSurface s, EGLint b) { (void)d; (void)s; (void)b; g_eglerr = EGL_BAD_SURFACE; return EGL_FALSE; }
EGLBoolean eglReleaseTexImage(EGLDisplay d, EGLSurface s, EGLint b) { (void)d; (void)s; (void)b; g_eglerr = EGL_BAD_SURFACE; return EGL_FALSE; }
EGLBoolean eglCopyBuffers(EGLDisplay d, EGLSurface s, EGLNativePixmapType t) { (void)d; (void)s; (void)t; g_eglerr = EGL_BAD_NATIVE_PIXMAP; return EGL_FALSE; }
EGLSurface eglCreatePixmapSurface(EGLDisplay d, EGLConfig c, EGLNativePixmapType p, const EGLint *a) { (void)d; (void)c; (void)p; (void)a; g_eglerr = EGL_BAD_NATIVE_PIXMAP; return EGL_NO_SURFACE; }
EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay d, EGLenum t, EGLClientBuffer b, EGLConfig c, const EGLint *a) { (void)d; (void)t; (void)b; (void)c; (void)a; g_eglerr = EGL_BAD_PARAMETER; return EGL_NO_SURFACE; }
EGLBoolean eglReleaseThread(void) { t_ctx = t_draw = t_read = 0; t_st = NULL; return EGL_TRUE; }

EGLBoolean eglInitialize(EGLDisplay d, EGLint *major, EGLint *minor)
{
    (void)d;
    pthread_mutex_lock(&L);
    connect_once();
    pthread_mutex_unlock(&L);
    if (g_dead) { g_eglerr = EGL_NOT_INITIALIZED; return EGL_FALSE; }
    if (major) *major = 1;
    if (minor) *minor = 4;
    return EGL_TRUE;
}

EGLBoolean eglTerminate(EGLDisplay d)
{
    (void)d;
    pthread_mutex_lock(&L); flush(); pthread_mutex_unlock(&L);
    return EGL_TRUE;
}

const char *eglQueryString(EGLDisplay d, EGLint name)
{
    (void)d;
    switch (name) {
    case EGL_VENDOR: return "sandbox MOI3 (glfwd)";
    case EGL_VERSION: return "1.4 glfwd";
    case EGL_CLIENT_APIS: return "OpenGL_ES";
    case EGL_EXTENSIONS: return "EGL_KHR_create_context";
    }
    g_eglerr = EGL_BAD_PARAMETER;
    return NULL;
}

static EGLBoolean cfg_list(uint32_t op, const EGLint *al, EGLConfig *cfgs, EGLint size, EGLint *num)
{
    uint64_t ok, total, got, i;
    W_begin(op);
    if (op == OPX_EGL_CHOOSE) W_blob(al, attr_len(al) * 4);
    W_u64(cfgs && size > 0 ? (uint64_t)size : 0);
    W_call();
    ok = R_u64(); total = R_u64(); got = R_u64();
    for (i = 0; i < got; i++) {
        uint64_t h = R_u64();
        if (cfgs && (EGLint)i < size) cfgs[i] = (EGLConfig)(uintptr_t)h;
    }
    if (num) *num = cfgs ? (EGLint)got : (EGLint)total;
    W_done();
    if (!ok) g_eglerr = EGL_BAD_ATTRIBUTE;
    return ok ? EGL_TRUE : EGL_FALSE;
}

EGLBoolean eglChooseConfig(EGLDisplay d, const EGLint *al, EGLConfig *cfgs, EGLint size, EGLint *num)
{ (void)d; return cfg_list(OPX_EGL_CHOOSE, al, cfgs, size, num); }

EGLBoolean eglGetConfigs(EGLDisplay d, EGLConfig *cfgs, EGLint size, EGLint *num)
{ (void)d; return cfg_list(OPX_EGL_GETCONFIGS, NULL, cfgs, size, num); }

static EGLBoolean query(uint32_t op, uint64_t h, EGLint attr, EGLint *value)
{
    uint64_t ok, v;
    W_begin(op);
    W_u64(h); W_u64((uint64_t)attr);
    W_call();
    ok = R_u64(); v = R_u64();
    W_done();
    if (ok && value) *value = (EGLint)v;
    if (!ok) g_eglerr = EGL_BAD_ATTRIBUTE;
    return ok ? EGL_TRUE : EGL_FALSE;
}

EGLBoolean eglGetConfigAttrib(EGLDisplay d, EGLConfig c, EGLint a, EGLint *v) { (void)d; return query(OPX_EGL_GETCONFIGATTR, (uintptr_t)c, a, v); }
EGLBoolean eglQuerySurface(EGLDisplay d, EGLSurface s, EGLint a, EGLint *v) { (void)d; return query(OPX_EGL_QUERYSURF, (uintptr_t)s, a, v); }
EGLBoolean eglQueryContext(EGLDisplay d, EGLContext c, EGLint a, EGLint *v) { (void)d; return query(OPX_EGL_QUERYCTX, (uintptr_t)c, a, v); }

EGLContext eglCreateContext(EGLDisplay d, EGLConfig cfg, EGLContext share, const EGLint *al)
{
    uint64_t h;
    int i;
    (void)d;
    W_begin(OPX_EGL_CREATECTX);
    W_u64((uintptr_t)cfg); W_u64((uintptr_t)share); W_blob(al, attr_len(al) * 4);
    W_call();
    h = R_u64();
    if (h) {
        for (i = 0; i < 64 && g_ctxs[i]; i++) { }
        if (i < 64) {
            g_ctxs[i] = calloc(1, sizeof *g_ctxs[i]);
            g_ctxs[i]->h = h;
            g_ctxs[i]->st.un_align = g_ctxs[i]->st.pk_align = 4;
        }
    } else g_eglerr = EGL_BAD_CONFIG;
    W_done();
    return (EGLContext)(uintptr_t)h;
}

EGLBoolean eglDestroyContext(EGLDisplay d, EGLContext c)
{
    (void)d;
    W_begin(OPX_EGL_DESTROYCTX); W_u64((uintptr_t)c); W_end();
    return EGL_TRUE;
}

static EGLSurface mksurf(int kind, EGLConfig cfg, const EGLint *al)
{
    uint64_t h, w = 0, hh = 0;
    size_t i;
    for (i = 0; kind == 1 && al && al[i] != EGL_NONE; i += 2) {
        if (al[i] == EGL_WIDTH) w = (uint64_t)al[i + 1];
        if (al[i] == EGL_HEIGHT) hh = (uint64_t)al[i + 1];
    }
    W_begin(OPX_EGL_CREATESURF);
    W_u64((uint64_t)kind); W_u64((uintptr_t)cfg); W_u64(w); W_u64(hh);
    W_call();
    h = R_u64();
    W_done();
    if (!h) g_eglerr = EGL_BAD_ALLOC;
    return (EGLSurface)(uintptr_t)h;
}

EGLSurface eglCreateWindowSurface(EGLDisplay d, EGLConfig c, EGLNativeWindowType w, const EGLint *a) { (void)d; (void)w; (void)a; return mksurf(0, c, NULL); }
EGLSurface eglCreatePbufferSurface(EGLDisplay d, EGLConfig c, const EGLint *a) { (void)d; return mksurf(1, c, a); }

EGLBoolean eglDestroySurface(EGLDisplay d, EGLSurface s)
{
    (void)d;
    W_begin(OPX_EGL_DESTROYSURF); W_u64((uintptr_t)s); W_end();
    return EGL_TRUE;
}

EGLBoolean eglMakeCurrent(EGLDisplay d, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    uint64_t ok;
    (void)d;
    W_begin(OPX_EGL_MAKECURRENT);
    W_u64((uintptr_t)draw); W_u64((uintptr_t)read); W_u64((uintptr_t)ctx);
    W_call();
    ok = R_u64();
    if (ok) {
        t_ctx = h_ctx = (uintptr_t)ctx; t_draw = h_draw = (uintptr_t)draw; t_read = h_read = (uintptr_t)read;
        t_st = ctx ? state_of((uintptr_t)ctx) : NULL;
    } else g_eglerr = EGL_BAD_MATCH;
    W_done();
    return ok ? EGL_TRUE : EGL_FALSE;
}

EGLBoolean eglSwapBuffers(EGLDisplay d, EGLSurface s)
{
    uint64_t ok;
    (void)d;
    pthread_mutex_lock(&L);
    connect_once();
    sync_ctx();
    raw_begin(OPX_EGL_SWAP);
    W_u64((uintptr_t)s);
    W_call();
    ok = R_u64();
    W_done();
    return ok ? EGL_TRUE : EGL_FALSE;
}

EGLBoolean eglWaitGL(void) { glFinish(); return EGL_TRUE; }
EGLBoolean eglWaitClient(void) { glFinish(); return EGL_TRUE; }

/* ------------------------------------------------------------------ GL: consultas */
static const char *g_str[5];
static char **g_exts;
static GLuint g_next;

const GLubyte *glGetString(GLenum name)
{
    int i = name == GL_VENDOR ? 0 : name == GL_RENDERER ? 1 : name == GL_VERSION ? 2 :
            name == GL_EXTENSIONS ? 3 : name == GL_SHADING_LANGUAGE_VERSION ? 4 : -1;
    const char *r;
    pthread_mutex_lock(&L);
    if (i < 0 || !g_str[i]) {
        uint64_t n;
        char *s = NULL;
        W_begin(OP_glGetString);
        W_u64(name);
        W_call();
        n = R_u64();
        if (n) { s = malloc(n); R_copy(s, n); }
        W_done();
        if (i < 0) { free(s); pthread_mutex_unlock(&L); return NULL; }
        g_str[i] = s;
    }
    r = g_str[i];
    pthread_mutex_unlock(&L);
    return (const GLubyte *)r;
}

const GLubyte *glGetStringi(GLenum name, GLuint index)
{
    const char *r = NULL;
    pthread_mutex_lock(&L);
    if (name == GL_EXTENSIONS && index < g_next && g_exts[index]) r = g_exts[index];
    else {
        uint64_t n;
        char *s = NULL;
        W_begin(OP_glGetStringi);
        W_u64(name); W_u64(index);
        W_call();
        n = R_u64();
        if (n) { s = malloc(n); R_copy(s, n); }
        W_done();
        if (s && name == GL_EXTENSIONS && index < 4096) {
            if (index >= g_next) {
                g_exts = realloc(g_exts, (index + 64) * sizeof *g_exts);
                memset(g_exts + g_next, 0, (index + 64 - g_next) * sizeof *g_exts);
                g_next = index + 64;
            }
            g_exts[index] = s;
        }
        r = s;
    }
    pthread_mutex_unlock(&L);
    return (const GLubyte *)r;
}

static void getv(uint32_t op, GLenum pname, void *out, size_t elem)
{
    uint64_t cnt;
    W_begin(op);
    W_u64(pname);
    W_call();
    cnt = R_u64();
    R_copy(out, (size_t)cnt * elem);
    W_done();
}

void glGetBooleanv(GLenum pname, GLboolean *data) { getv(OP_glGetBooleanv, pname, data, 1); }
void glGetIntegerv(GLenum pname, GLint *data) { getv(OP_glGetIntegerv, pname, data, 4); }
void glGetFloatv(GLenum pname, GLfloat *data) { getv(OP_glGetFloatv, pname, data, 4); }
void glGetInteger64v(GLenum pname, GLint64 *data) { getv(OP_glGetInteger64v, pname, data, 8); }

void glGetActiveUniformBlockiv(GLuint program, GLuint index, GLenum pname, GLint *params)
{
    uint64_t cnt;
    W_begin(OP_glGetActiveUniformBlockiv);
    W_u64(program); W_u64(index); W_u64(pname);
    W_call();
    cnt = R_u64();
    R_copy(params, (size_t)cnt * 4);
    W_done();
}

GLenum glGetError(void)
{
    GLenum e;
    W_begin(OP_glGetError);
    W_call();
    e = (GLenum)R_u64();
    W_done();
    return e;
}

void glFinish(void)
{
    W_begin(OP_glFinish);
    W_call();
    (void)R_u64();
    W_done();
}

void glFlush(void)
{
    W_begin(OP_glFlush);
    raw_close();
    flush();
    pthread_mutex_unlock(&L);
}

/* ------------------------------------------------------------------ GL: shaders */
static void strings_blob(GLsizei count, const GLchar *const *str, const GLint *len)
{
    size_t total = 0, o = 0;
    char *b;
    GLsizei i;
    for (i = 0; i < count; i++)
        total += (len && len[i] >= 0) ? (size_t)len[i] : (str[i] ? strlen(str[i]) : 0);
    b = malloc(total + 1);
    for (i = 0; i < count; i++) {
        size_t n = (len && len[i] >= 0) ? (size_t)len[i] : (str[i] ? strlen(str[i]) : 0);
        if (n) memcpy(b + o, str[i], n);
        o += n;
    }
    b[total] = 0;
    W_blob(b, total + 1);
    free(b);
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length)
{
    W_begin(OP_glShaderSource);
    W_u64(shader);
    strings_blob(count, string, length);
    W_end();
}

/* nomes separados por NUL num bloco só */
static void names_blob(GLsizei count, const GLchar *const *names)
{
    size_t total = 0, o = 0;
    char *b;
    GLsizei i;
    for (i = 0; i < count; i++) total += strlen(names[i]) + 1;
    b = malloc(total + 1);
    for (i = 0; i < count; i++) { size_t n = strlen(names[i]) + 1; memcpy(b + o, names[i], n); o += n; }
    W_blob(b, total);
    free(b);
}

void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const *uniformNames, GLuint *uniformIndices)
{
    W_begin(OP_glGetUniformIndices);
    W_u64(program); W_u64((uint64_t)uniformCount);
    names_blob(uniformCount, uniformNames);
    W_call();
    R_copy(uniformIndices, SZ(uniformCount * 4));
    W_done();
}

void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const *varyings, GLenum bufferMode)
{
    W_begin(OP_glTransformFeedbackVaryings);
    W_u64(program); W_u64((uint64_t)count); W_u64(bufferMode);
    names_blob(count, varyings);
    W_end();
}

/* ------------------------------------------------------------------ GL: pixels */
static size_t ncomp(GLenum f)
{
    switch (f) {
    case GL_RED: case GL_RED_INTEGER: case GL_ALPHA: case GL_LUMINANCE:
    case GL_DEPTH_COMPONENT: case GL_STENCIL_INDEX: return 1;
    case GL_RG: case GL_RG_INTEGER: case GL_LUMINANCE_ALPHA: case GL_DEPTH_STENCIL: return 2;
    case GL_RGB: case GL_RGB_INTEGER: return 3;
    }
    return 4;
}

static size_t pixbytes(GLenum f, GLenum t)
{
    switch (t) {
    case GL_UNSIGNED_BYTE: case GL_BYTE: return ncomp(f);
    case GL_UNSIGNED_SHORT: case GL_SHORT: case GL_HALF_FLOAT: case 0x8D61: return 2 * ncomp(f);
    case GL_UNSIGNED_INT: case GL_INT: case GL_FLOAT: return 4 * ncomp(f);
    case GL_UNSIGNED_SHORT_5_6_5: case GL_UNSIGNED_SHORT_4_4_4_4: case GL_UNSIGNED_SHORT_5_5_5_1: return 2;
    case GL_UNSIGNED_INT_2_10_10_10_REV: case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV: case GL_UNSIGNED_INT_24_8: return 4;
    case GL_FLOAT_32_UNSIGNED_INT_24_8_REV: return 8;
    }
    return 4 * ncomp(f);
}

/* quantos bytes o cliente está passando (ou vai receber), contando alinhamento e saltos */
static size_t img_bytes(int unpack, GLsizei w, GLsizei h, GLsizei d, GLenum f, GLenum t)
{
    struct cstate *s = ST;
    size_t bp = pixbytes(f, t), al, rl, stride, ih, img;
    size_t skr = (size_t)(unpack ? s->un_skiprows : s->pk_skiprows);
    size_t skp = (size_t)(unpack ? s->un_skippix : s->pk_skippix);
    size_t ski = (size_t)(unpack ? s->un_skipimg : 0);
    if (w <= 0 || h <= 0 || d <= 0) return 0;
    al = (size_t)(unpack ? s->un_align : s->pk_align);
    if (al != 1 && al != 2 && al != 4 && al != 8) al = 4;
    rl = (size_t)((unpack ? s->un_rowlen : s->pk_rowlen) > 0 ? (unpack ? s->un_rowlen : s->pk_rowlen) : w);
    stride = (rl * bp + al - 1) / al * al;
    ih = (size_t)((unpack && s->un_imgh > 0) ? s->un_imgh : h);
    img = stride * ih;
    return (ski + (size_t)d - 1) * img + (skr + (size_t)h - 1) * stride + (skp + (size_t)w) * bp;
}

/* dados de pixel: se há um PBO ligado o "ponteiro" é um deslocamento; senão vai o bloco */
static void W_pixels(const void *p, size_t n)
{
    if (ST->bound[3]) { W_u64(1); W_u64((uint64_t)(uintptr_t)p); }
    else { W_u64(0); W_blob(p, p ? n : 0); }
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const void *pixels)
{
    W_begin(OP_glTexImage2D);
    W_u64(target); W_u64((uint64_t)level); W_u64((uint64_t)internalformat); W_u64((uint64_t)width);
    W_u64((uint64_t)height); W_u64((uint64_t)border); W_u64(format); W_u64(type);
    W_pixels(pixels, img_bytes(1, width, height, 1, format, type));
    W_end();
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                     GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    W_begin(OP_glTexSubImage2D);
    W_u64(target); W_u64((uint64_t)level); W_u64((uint64_t)xoffset); W_u64((uint64_t)yoffset);
    W_u64((uint64_t)width); W_u64((uint64_t)height); W_u64(format); W_u64(type);
    W_pixels(pixels, img_bytes(1, width, height, 1, format, type));
    W_end();
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                  GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
    W_begin(OP_glTexImage3D);
    W_u64(target); W_u64((uint64_t)level); W_u64((uint64_t)internalformat); W_u64((uint64_t)width);
    W_u64((uint64_t)height); W_u64((uint64_t)depth); W_u64((uint64_t)border); W_u64(format); W_u64(type);
    W_pixels(pixels, img_bytes(1, width, height, depth, format, type));
    W_end();
}

void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                     const void *pixels)
{
    W_begin(OP_glTexSubImage3D);
    W_u64(target); W_u64((uint64_t)level); W_u64((uint64_t)xoffset); W_u64((uint64_t)yoffset);
    W_u64((uint64_t)zoffset); W_u64((uint64_t)width); W_u64((uint64_t)height); W_u64((uint64_t)depth);
    W_u64(format); W_u64(type);
    W_pixels(pixels, img_bytes(1, width, height, depth, format, type));
    W_end();
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                            GLsizei height, GLint border, GLsizei imageSize, const void *data)
{
    W_begin(OP_glCompressedTexImage2D);
    W_u64(target); W_u64((uint64_t)level); W_u64(internalformat); W_u64((uint64_t)width);
    W_u64((uint64_t)height); W_u64((uint64_t)border); W_u64((uint64_t)imageSize);
    W_pixels(data, SZ(imageSize));
    W_end();
}

void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                               GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
    W_begin(OP_glCompressedTexSubImage2D);
    W_u64(target); W_u64((uint64_t)level); W_u64((uint64_t)xoffset); W_u64((uint64_t)yoffset);
    W_u64((uint64_t)width); W_u64((uint64_t)height); W_u64(format); W_u64((uint64_t)imageSize);
    W_pixels(data, SZ(imageSize));
    W_end();
}

void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                            GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
    W_begin(OP_glCompressedTexImage3D);
    W_u64(target); W_u64((uint64_t)level); W_u64(internalformat); W_u64((uint64_t)width);
    W_u64((uint64_t)height); W_u64((uint64_t)depth); W_u64((uint64_t)border); W_u64((uint64_t)imageSize);
    W_pixels(data, SZ(imageSize));
    W_end();
}

void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                               GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                               GLsizei imageSize, const void *data)
{
    W_begin(OP_glCompressedTexSubImage3D);
    W_u64(target); W_u64((uint64_t)level); W_u64((uint64_t)xoffset); W_u64((uint64_t)yoffset);
    W_u64((uint64_t)zoffset); W_u64((uint64_t)width); W_u64((uint64_t)height); W_u64((uint64_t)depth);
    W_u64(format); W_u64((uint64_t)imageSize);
    W_pixels(data, SZ(imageSize));
    W_end();
}

static void readpix(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, size_t n, void *pixels)
{
    W_begin(OP_glReadPixels);
    W_u64((uint64_t)x); W_u64((uint64_t)y); W_u64((uint64_t)w); W_u64((uint64_t)h); W_u64(format); W_u64(type);
    if (ST->bound[2]) {                       /* PBO de leitura: o destino fica no glhost */
        W_u64(1); W_u64((uint64_t)(uintptr_t)pixels);
        W_end();
        return;
    }
    W_u64(0); W_u64((uint64_t)n);
    W_call();
    R_copy(pixels, n);
    W_done();
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
{ readpix(x, y, width, height, format, type, img_bytes(0, width, height, 1, format, type), pixels); }

void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data)
{
    size_t n = img_bytes(0, width, height, 1, format, type);
    if (n > SZ(bufSize)) return;
    readpix(x, y, width, height, format, type, n, data);
}

/* ------------------------------------------------------------------ GL: vértices e desenho */
static void attrib_ptr(uint32_t op, GLuint index, GLint size, GLenum type, GLboolean norm, GLsizei stride, const void *p)
{
    static int warned;
    if (!ST->bound[0] && p && !warned) {
        warned = 1;
        LOG("[glfwd] AVISO: atributo %u com ponteiro do cliente (sem VBO): não suportado\n", index);
    }
    W_begin(op);
    W_u64(index); W_u64((uint64_t)size); W_u64(type); W_u64(norm); W_u64((uint64_t)stride);
    W_u64((uint64_t)(uintptr_t)p);
    W_end();
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
{ attrib_ptr(OP_glVertexAttribPointer, index, size, type, normalized, stride, pointer); }

void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer)
{ attrib_ptr(OP_glVertexAttribIPointer, index, size, type, 0, stride, pointer); }

/* índices: deslocamento no buffer de índices do VAO, ou memória do cliente se não há buffer */
static void W_indices(GLsizei count, GLenum type, const void *indices)
{
    size_t es = type == GL_UNSIGNED_INT ? 4 : type == GL_UNSIGNED_SHORT ? 2 : 1;
    if (ST->bound[1] || !indices) { W_u64(1); W_u64((uint64_t)(uintptr_t)indices); }
    else { W_u64(0); W_blob(indices, SZ(count) * es); }
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    W_begin(OP_glDrawElements);
    W_u64(mode); W_u64((uint64_t)count); W_u64(type); W_indices(count, type, indices);
    W_end();
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
    W_begin(OP_glDrawRangeElements);
    W_u64(mode); W_u64(start); W_u64(end); W_u64((uint64_t)count); W_u64(type); W_indices(count, type, indices);
    W_end();
}

void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount)
{
    W_begin(OP_glDrawElementsInstanced);
    W_u64(mode); W_u64((uint64_t)count); W_u64(type); W_u64((uint64_t)instancecount); W_indices(count, type, indices);
    W_end();
}

void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex)
{
    W_begin(OP_glDrawElementsBaseVertex);
    W_u64(mode); W_u64((uint64_t)count); W_u64(type); W_u64((uint64_t)basevertex); W_indices(count, type, indices);
    W_end();
}

void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex)
{
    W_begin(OP_glDrawRangeElementsBaseVertex);
    W_u64(mode); W_u64(start); W_u64(end); W_u64((uint64_t)count); W_u64(type); W_u64((uint64_t)basevertex);
    W_indices(count, type, indices);
    W_end();
}

void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex)
{
    W_begin(OP_glDrawElementsInstancedBaseVertex);
    W_u64(mode); W_u64((uint64_t)count); W_u64(type); W_u64((uint64_t)instancecount); W_u64((uint64_t)basevertex);
    W_indices(count, type, indices);
    W_end();
}

/* ------------------------------------------------------------------ GL: mapeamento de buffer
 * O GL devolve um ponteiro para a memória do buffer. Aqui a memória de verdade está no glhost,
 * então a HMI recebe uma cópia: preenchida com o conteúdo atual quando ele importa, e mandada
 * de volta no glUnmapBuffer (ou no glFlushMappedBufferRange). */
static struct mapping *map_find(struct cstate *s, GLuint buf)
{
    int k;
    for (k = 0; k < 32; k++) if (s->maps[k].shadow && s->maps[k].buf == buf) return &s->maps[k];
    return NULL;
}

void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    struct cstate *s = ST;
    GLuint buf = s->bound[tslot(target)];
    struct mapping *m = NULL;
    int k, need_old;
    pthread_mutex_lock(&L);
    if (!buf || length <= 0 || map_find(s, buf)) { pthread_mutex_unlock(&L); return NULL; }
    for (k = 0; k < 32 && !m; k++) if (!s->maps[k].shadow) m = &s->maps[k];
    if (!m) { pthread_mutex_unlock(&L); return NULL; }
    m->buf = buf; m->off = offset; m->len = length; m->access = access;
    m->shadow = calloc(1, (size_t)length);
    need_old = (access & GL_MAP_READ_BIT) ||
               !(access & (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
    if (need_old) {
        uint64_t ok;
        W_begin(OP_glMapBufferRange);
        W_u64(target); W_u64((uint64_t)offset); W_u64((uint64_t)length);
        W_call();
        ok = R_u64();
        R_copy(m->shadow, (size_t)length);
        W_done();
        if (!ok && !g_dead) { free(m->shadow); m->shadow = NULL; pthread_mutex_unlock(&L); return NULL; }
    }
    pthread_mutex_unlock(&L);
    return m->shadow;
}

GLboolean glUnmapBuffer(GLenum target)
{
    struct cstate *s = ST;
    struct mapping *m;
    pthread_mutex_lock(&L);
    m = map_find(s, s->bound[tslot(target)]);
    if (!m) { pthread_mutex_unlock(&L); return GL_FALSE; }
    if ((m->access & GL_MAP_WRITE_BIT) && !(m->access & GL_MAP_FLUSH_EXPLICIT_BIT)) {
        W_begin(OP_glUnmapBuffer);
        W_u64(target); W_u64((uint64_t)m->off); W_blob(m->shadow, (size_t)m->len);
        W_end();
    }
    free(m->shadow);
    m->shadow = NULL;
    pthread_mutex_unlock(&L);
    return GL_TRUE;
}

void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
    struct cstate *s = ST;
    struct mapping *m;
    pthread_mutex_lock(&L);
    m = map_find(s, s->bound[tslot(target)]);
    if (m && offset >= 0 && length > 0 && offset + length <= m->len) {
        W_begin(OP_glFlushMappedBufferRange);
        W_u64(target); W_u64((uint64_t)(m->off + offset)); W_blob(m->shadow + offset, (size_t)length);
        W_end();
    }
    pthread_mutex_unlock(&L);
}

void glGetBufferPointerv(GLenum target, GLenum pname, void **params)
{
    struct mapping *m = map_find(ST, ST->bound[tslot(target)]);
    (void)pname;
    if (params) *params = m ? m->shadow : NULL;
}

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
    struct mapping *m = map_find(ST, ST->bound[tslot(target)]);
    if (pname == GL_BUFFER_MAPPED) { if (params) *params = m ? GL_TRUE : GL_FALSE; return; }
    if (pname == GL_BUFFER_MAP_OFFSET) { if (params) *params = m ? (GLint)m->off : 0; return; }
    if (pname == GL_BUFFER_MAP_LENGTH) { if (params) *params = m ? (GLint)m->len : 0; return; }
    if (pname == GL_BUFFER_ACCESS_FLAGS) { if (params) *params = m ? (GLint)m->access : 0; return; }
    W_begin(OP_glGetBufferParameteriv);
    W_u64(target); W_u64(pname);
    W_call();
    R_copy(params, 4);
    W_done();
}

/* ------------------------------------------------------------------ GL: depuração (só local) */
void glDebugMessageCallback(GLDEBUGPROC cb, const void *u) { (void)cb; (void)u; }
void glDebugMessageControl(GLenum s, GLenum t, GLenum v, GLsizei c, const GLuint *i, GLboolean e) { (void)s; (void)t; (void)v; (void)c; (void)i; (void)e; }
void glDebugMessageInsert(GLenum s, GLenum t, GLuint i, GLenum v, GLsizei l, const GLchar *b) { (void)s; (void)t; (void)i; (void)v; (void)l; (void)b; }
void glPushDebugGroup(GLenum s, GLuint i, GLsizei l, const GLchar *m) { (void)s; (void)i; (void)l; (void)m; }
void glPopDebugGroup(void) { }
void glObjectLabel(GLenum i, GLuint n, GLsizei l, const GLchar *b) { (void)i; (void)n; (void)l; (void)b; }
void glObjectPtrLabel(const void *p, GLsizei l, const GLchar *b) { (void)p; (void)l; (void)b; }
GLuint glGetDebugMessageLog(GLuint c, GLsizei b, GLenum *s, GLenum *t, GLuint *i, GLenum *v, GLsizei *l, GLchar *m) { (void)c; (void)b; (void)s; (void)t; (void)i; (void)v; (void)l; (void)m; return 0; }
void glGetObjectLabel(GLenum i, GLuint n, GLsizei b, GLsizei *l, GLchar *lab) { (void)i; (void)n; if (l) *l = 0; if (lab && b > 0) lab[0] = 0; }
void glGetObjectPtrLabel(const void *p, GLsizei b, GLsizei *l, GLchar *lab) { (void)p; if (l) *l = 0; if (lab && b > 0) lab[0] = 0; }
void glGetPointerv(GLenum pname, void **params) { (void)pname; if (params) *params = NULL; }

/* ------------------------------------------------------------------ o resto é gerado */
#include "gen/shim_gen.inc"

static const struct { const char *n; void *p; } g_procs[] = {
#include "gen/procs_gen.inc"
    {"eglGetDisplay", (void *)eglGetDisplay}, {"eglInitialize", (void *)eglInitialize},
    {"eglChooseConfig", (void *)eglChooseConfig}, {"eglCreateContext", (void *)eglCreateContext},
    {"eglCreateWindowSurface", (void *)eglCreateWindowSurface}, {"eglMakeCurrent", (void *)eglMakeCurrent},
    {"eglSwapBuffers", (void *)eglSwapBuffers}, {"eglGetError", (void *)eglGetError},
    {NULL, NULL}
};

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *name)
{
    int i;
    for (i = 0; name && g_procs[i].n; i++)
        if (!strcmp(name, g_procs[i].n)) return (__eglMustCastToProperFunctionPointerType)g_procs[i].p;
    return NULL;
}
