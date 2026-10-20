/* Consultas de estado: cada pname devolve uma quantidade diferente de valores, e o
 * encaminhador não pode escrever além do que o GL de verdade escreveria. */
#include "t_common.h"

int main(void)
{
    GLint v[6], n = 0, ncf = 0, range[3] = { 0, 0, -7 }, prec[2] = { 0, -7 }, *cf;
    GLfloat f[5];
    GLboolean b[5];
    GLint64 big[2] = { 0, -7 };
    const char *ext0, *exts;
    int i;

    t_setup();
    glViewport(0, 0, T_W, T_H);

    for (i = 0; i < 6; i++) v[i] = -7;
    glGetIntegerv(GL_VIEWPORT, v);
    CHECK(v[0] == 0 && v[1] == 0 && v[2] == T_W && v[3] == T_H);
    CHECK_EQ(v[4], -7);                       /* não escreveu o quinto */
    glViewport(1, 2, 3, 4);
    glGetIntegerv(GL_VIEWPORT, v);
    CHECK(v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4);

    v[0] = v[1] = -7;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, v);
    CHECK(v[0] >= 2048);
    CHECK_EQ(v[1], -7);                       /* um valor só */

    v[2] = -7;
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, v);
    CHECK(v[0] >= T_W && v[1] >= T_H);
    CHECK_EQ(v[2], -7);                       /* dois valores */

    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    for (i = 0; i < 5; i++) f[i] = -7;
    glGetFloatv(GL_COLOR_CLEAR_VALUE, f);
    CHECK(f[0] == 0.25f && f[1] == 0.5f && f[2] == 0.75f && f[3] == 1.0f);
    CHECK(f[4] == -7.0f);

    for (i = 0; i < 5; i++) b[i] = 7;
    glGetBooleanv(GL_COLOR_WRITEMASK, b);
    CHECK(b[0] == GL_TRUE && b[1] == GL_TRUE && b[2] == GL_TRUE && b[3] == GL_TRUE);
    CHECK_EQ(b[4], 7);

    glGetInteger64v(GL_MAX_ELEMENT_INDEX, big);
    CHECK(big[0] > 0);
    CHECK_EQ(big[1], -7);

    /* lista de tamanho variável: formatos de textura comprimida */
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &ncf);
    CHECK(ncf >= 0 && ncf < 4096);
    cf = calloc((size_t)ncf + 1, sizeof *cf);
    cf[ncf] = -7;
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, cf);
    CHECK_EQ(cf[ncf], -7);
    if (ncf > 0) CHECK(cf[0] != 0);
    free(cf);

    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    CHECK(n > 0);
    ext0 = (const char *)glGetStringi(GL_EXTENSIONS, 0);
    CHECK(ext0 && strncmp(ext0, "GL_", 3) == 0);
    exts = (const char *)glGetString(GL_EXTENSIONS);
    CHECK(exts && ext0 && strstr(exts, ext0) != NULL);
    CHECK(glGetString(GL_SHADING_LANGUAGE_VERSION) != NULL);
    CHECK(glGetString(GL_VENDOR) == glGetString(GL_VENDOR));   /* ponteiro estável entre chamadas */

    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, prec);
    CHECK(range[0] > 0 && range[1] > 0 && prec[0] > 0);
    CHECK(range[2] == -7 && prec[1] == -7);

    CHECK_EQ(glIsEnabled(GL_BLEND), GL_FALSE);
    glEnable(GL_BLEND);
    CHECK_EQ(glIsEnabled(GL_BLEND), GL_TRUE);
    glDisable(GL_BLEND);

    glEnable(0x7777);                          /* enum inválido de propósito */
    CHECK_EQ(glGetError(), GL_INVALID_ENUM);
    CHECK_EQ(glGetError(), GL_NO_ERROR);
    return t_done("t_getters");
}
