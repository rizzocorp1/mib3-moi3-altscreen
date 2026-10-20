/* Buffers: glMapBufferRange devolve memória do cliente, então o encaminhador tem de guardar
 * uma cópia e mandá-la na hora do unmap; e ler de volta quando o mapeamento é de leitura. */
#include "t_common.h"

static const char *VS =
    "#version 300 es\n"
    "in vec2 aPos; void main(){ gl_Position = vec4(aPos, 0.0, 1.0); }\n";
static const char *FS =
    "#version 300 es\n"
    "precision mediump float; uniform vec4 uCol; out vec4 o; void main(){ o = uCol; }\n";

int main(void)
{
    GLuint p, vao, vbo;
    GLint size = 0, mapped = 1;
    GLfloat *m;
    const GLfloat *r;
    static const GLfloat left[] = { -1, -1, 0, -1, -1, 1, 0, 1 };
    static const GLfloat right[] = { 0, -1, 1, -1, 0, 1, 1, 1 };

    t_setup();
    p = t_program(VS, FS);
    glBindAttribLocation(p, 0, "aPos"); glLinkProgram(p);
    glUseProgram(p);
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof left, NULL, GL_DYNAMIC_DRAW);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    CHECK_EQ(size, (GLint)sizeof left);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glViewport(0, 0, T_W, T_H);
    glClearColor(0, 0, 0, 1);

    /* escrita por mapeamento */
    m = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof left, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    CHECK(m != NULL);
    if (m) memcpy(m, left, sizeof left);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
    CHECK_EQ(mapped, GL_TRUE);
    CHECK_EQ(glUnmapBuffer(GL_ARRAY_BUFFER), GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform4f(glGetUniformLocation(p, "uCol"), 0, 1, 0, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    expect_px(T_W / 4, T_H / 2, 0, 255, 0, 255);
    expect_px(3 * T_W / 4, T_H / 2, 0, 0, 0, 255);

    /* leitura por mapeamento: tem de devolver o que foi escrito */
    r = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof left, GL_MAP_READ_BIT);
    CHECK(r != NULL);
    CHECK(r && memcmp(r, left, sizeof left) == 0);
    CHECK_EQ(glUnmapBuffer(GL_ARRAY_BUFFER), GL_TRUE);

    /* mapeamento parcial com deslocamento: troca só os dois últimos vértices */
    m = glMapBufferRange(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat), 4 * sizeof(GLfloat), GL_MAP_WRITE_BIT);
    CHECK(m != NULL);
    if (m) memcpy(m, right + 4, 4 * sizeof(GLfloat));
    CHECK_EQ(glUnmapBuffer(GL_ARRAY_BUFFER), GL_TRUE);
    /* e os dois primeiros por glBufferSubData */
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(GLfloat), right);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform4f(glGetUniformLocation(p, "uCol"), 1, 1, 0, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    expect_px(3 * T_W / 4, T_H / 2, 255, 255, 0, 255);
    expect_px(T_W / 4, T_H / 2, 0, 0, 0, 255);

    glDeleteBuffers(1, &vbo);
    CHECK_EQ(glIsBuffer(vbo), GL_FALSE);
    CHECK(eglSwapBuffers(g_d, g_s));
    return t_done("t_buffers");
}
