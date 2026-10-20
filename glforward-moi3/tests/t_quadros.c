/* Produz quadros para os testes da transmissão direta (t_transmissao.sh).
 *   t_quadros          três quadros com pausa entre eles: a tela toda azul; um retângulo verde
 *                      de 40x30 pixels; nada novo
 *   t_quadros muitos   120 quadros de tela cheia, um atrás do outro, cada um de uma cor
 */
#include "t_common.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    int i;
    t_setup();
    glViewport(0, 0, T_W, T_H);
    if (argc > 1 && !strcmp(argv[1], "muitos")) {
        for (i = 0; i < 120; i++) {
            glClearColor(i & 1 ? 1.0f : 0.0f, 0.5f, i & 1 ? 0.0f : 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            CHECK(eglSwapBuffers(g_d, g_s));
        }
        return t_done("t_quadros muitos");
    }
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK(eglSwapBuffers(g_d, g_s));
    usleep(300000);
    glEnable(GL_SCISSOR_TEST);
    glScissor(100, 200, 40, 30);                 /* contado de baixo: linhas 410 a 439 da imagem */
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    CHECK(eglSwapBuffers(g_d, g_s));
    usleep(300000);
    CHECK(eglSwapBuffers(g_d, g_s));
    usleep(300000);
    return t_done("t_quadros");
}
