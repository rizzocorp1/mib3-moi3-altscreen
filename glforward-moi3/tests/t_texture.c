/* Envio de pixels: o encaminhador precisa calcular quantos bytes o cliente está passando
 * (formato, tipo, alinhamento de linha) e a textura tem de chegar inteira do outro lado. */
#include "t_common.h"

static const char *VS =
    "#version 300 es\n"
    "in vec2 aPos; out vec2 vUv;\n"
    "void main(){ vUv = aPos * 0.5 + 0.5; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
static const char *FS =
    "#version 300 es\n"
    "precision mediump float; in vec2 vUv; uniform sampler2D uTex; out vec4 o;\n"
    "void main(){ o = texture(uTex, vUv); }\n";

static void quad(void) { glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); }

int main(void)
{
    GLuint p, vao, vbo, tex[3];
    GLint w = 0;
    static const GLfloat v[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
    /* 2x2 RGBA: linha de baixo vermelho|verde, linha de cima azul|branco */
    static const unsigned char rgba[] = { 255, 0, 0, 255,  0, 255, 0, 255,
                                          0, 0, 255, 255,  255, 255, 255, 255 };
    static const unsigned char black[] = { 0, 0, 0, 255 };
    /* 3x2 RGB com alinhamento 4: cada linha tem 9 bytes + 3 de enchimento */
    static const unsigned char rgb_pad[] = { 255, 255, 0,  255, 255, 0,  255, 255, 0,  7, 7, 7,
                                             0, 255, 255,  0, 255, 255,  0, 255, 255 };
    /* 3x1 RGB com alinhamento 1 */
    static const unsigned char rgb_tight[] = { 255, 0, 255,  255, 0, 255,  255, 0, 255 };

    t_setup();
    p = t_program(VS, FS);
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glBindAttribLocation(p, 0, "aPos"); glLinkProgram(p);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glUseProgram(p);
    glUniform1i(glGetUniformLocation(p, "uTex"), 0);
    glViewport(0, 0, T_W, T_H);
    glClearColor(0.5f, 0.5f, 0.5f, 1);

    glGenTextures(3, tex);
    CHECK(tex[0] && tex[1] && tex[2] && tex[0] != tex[1]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    CHECK_EQ(w, 2);
    quad();
    expect_px(T_W / 4, T_H / 4, 255, 0, 0, 255);
    expect_px(3 * T_W / 4, T_H / 4, 0, 255, 0, 255);
    expect_px(T_W / 4, 3 * T_H / 4, 0, 0, 255, 255);
    expect_px(3 * T_W / 4, 3 * T_H / 4, 255, 255, 255, 255);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, black);
    quad();
    expect_px(3 * T_W / 4, 3 * T_H / 4, 0, 0, 0, 255);
    expect_px(T_W / 4, T_H / 4, 255, 0, 0, 255);

    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 3, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_pad);
    quad();
    expect_px(T_W / 2, T_H / 4, 255, 255, 0, 255);        /* linha de baixo: amarelo */
    expect_px(T_W / 2, 3 * T_H / 4, 0, 255, 255, 255);    /* linha de cima: ciano (depois do enchimento) */

    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, 3, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 3, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb_tight);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    quad();
    expect_px(T_W / 2, T_H / 2, 255, 0, 255, 255);

    glDeleteTextures(3, tex);
    CHECK_EQ(glIsTexture(tex[0]), GL_FALSE);
    CHECK(eglSwapBuffers(g_d, g_s));
    return t_done("t_texture");
}
