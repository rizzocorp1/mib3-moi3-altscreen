/* Testes do encaminhador de OpenGL do sandbox MOI3 (Marco 3, 11/10/2026).
 * Cada teste é um programa GLES 3 comum que liga contra libEGL_adreno.so/libGLESv2_adreno.so
 * (os nomes que a HMI pede). Rodando contra o stub de 10/10 eles FALHAM; contra o
 * encaminhador + glhost (Mesa llvmpipe) têm de passar. */
#ifndef T_COMMON_H
#define T_COMMON_H
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T_W 1280
#define T_H 640

static int g_fail, g_ok;
#define CHECK(c) do { if (c) g_ok++; else { g_fail++; \
    printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) do { long _a = (long)(a), _b = (long)(b); if (_a == _b) g_ok++; else { g_fail++; \
    printf("  FALHOU %s:%d: %s == %s (%ld != %ld)\n", __FILE__, __LINE__, #a, #b, _a, _b); } } while (0)

static EGLDisplay g_d;
static EGLSurface g_s;
static EGLContext g_c;
static EGLConfig g_cfg;

/* Sobe EGL + contexto ES 3 + superfície de janela, como um cliente comum faria. */
static int t_setup(void)
{
    EGLint ma = 0, mi = 0, n = 0;
    EGLint att[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                     EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                     EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8, EGL_NONE };
    EGLint ca[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    CHECK(g_d != EGL_NO_DISPLAY);
    CHECK(eglInitialize(g_d, &ma, &mi));
    CHECK(eglBindAPI(EGL_OPENGL_ES_API));
    CHECK(eglChooseConfig(g_d, att, &g_cfg, 1, &n) && n >= 1);
    g_c = eglCreateContext(g_d, g_cfg, EGL_NO_CONTEXT, ca);
    CHECK(g_c != EGL_NO_CONTEXT);
    g_s = eglCreateWindowSurface(g_d, g_cfg, (EGLNativeWindowType)0, NULL);
    CHECK(g_s != EGL_NO_SURFACE);
    CHECK(eglMakeCurrent(g_d, g_s, g_s, g_c));
    return g_fail;
}

static void expect_px_at(const char *f, int l, int x, int y, int r, int g, int b, int a)
{
    unsigned char p[4] = { 9, 9, 9, 9 };
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    if (abs(p[0] - r) <= 2 && abs(p[1] - g) <= 2 && abs(p[2] - b) <= 2 && abs(p[3] - a) <= 2) g_ok++;
    else { g_fail++; printf("  FALHOU %s:%d: pixel(%d,%d) = %d,%d,%d,%d; esperado %d,%d,%d,%d\n",
                            f, l, x, y, p[0], p[1], p[2], p[3], r, g, b, a); }
}
#define expect_px(x, y, r, g, b, a) expect_px_at(__FILE__, __LINE__, x, y, r, g, b, a)

static GLuint t_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    GLint ok = 0;
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512] = ""; GLsizei n = 0; glGetShaderInfoLog(s, sizeof log, &n, log);
               printf("  shader não compilou: %s\n", log); }
    CHECK(ok == GL_TRUE);
    return s;
}

static GLuint t_program(const char *vs, const char *fs)
{
    GLuint p = glCreateProgram();
    GLint ok = 0;
    glAttachShader(p, t_shader(GL_VERTEX_SHADER, vs));
    glAttachShader(p, t_shader(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512] = ""; GLsizei n = 0; glGetProgramInfoLog(p, sizeof log, &n, log);
               printf("  programa não ligou: %s\n", log); }
    CHECK(ok == GL_TRUE);
    return p;
}

static int t_done(const char *name)
{
    CHECK_EQ(glGetError(), GL_NO_ERROR);
    printf("%s %s: %d verificações ok, %d falhas\n", g_fail ? "FALHOU" : "PASSOU", name, g_ok, g_fail);
    return g_fail ? 1 : 0;
}
#endif
