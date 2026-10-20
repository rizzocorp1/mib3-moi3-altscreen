/* O mínimo: EGL sobe, a superfície tem o tamanho da tela de 8", limpar e ler de volta
 * devolve a cor pedida, e a troca de buffers funciona. */
#include "t_common.h"

int main(void)
{
    EGLint w = 0, h = 0;
    const char *ver;
    t_setup();
    CHECK(eglQuerySurface(g_d, g_s, EGL_WIDTH, &w) && w == T_W);
    CHECK(eglQuerySurface(g_d, g_s, EGL_HEIGHT, &h) && h == T_H);
    ver = (const char *)glGetString(GL_VERSION);
    CHECK(ver && strstr(ver, "OpenGL ES 3"));
    CHECK(glGetString(GL_RENDERER) != NULL);
    glViewport(0, 0, T_W, T_H);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    expect_px(T_W / 2, T_H / 2, 255, 0, 0, 255);
    expect_px(0, 0, 255, 0, 0, 255);
    expect_px(T_W - 1, T_H - 1, 255, 0, 0, 255);
    CHECK(eglSwapBuffers(g_d, g_s));
    return t_done("t_clear");
}
