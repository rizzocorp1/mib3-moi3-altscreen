/* O ponto em que a HMI parava com o stub: compilar, ligar e PERGUNTAR ao programa quais são
 * os atributos e uniforms ativos (glGetActiveAttrib). Depois desenhar com VAO/VBO/EBO. */
#include "t_common.h"

static const char *VS =
    "#version 300 es\n"
    "in vec2 aPos; in vec3 aCol; uniform mat4 uMvp; out vec3 vCol;\n"
    "void main(){ vCol = aCol; gl_Position = uMvp * vec4(aPos, 0.0, 1.0); }\n";
static const char *FS =
    "#version 300 es\n"
    "precision mediump float; in vec3 vCol; uniform vec4 uTint; out vec4 o;\n"
    "void main(){ o = vec4(vCol, 1.0) * uTint; }\n";

int main(void)
{
    GLuint p, vao, vbo, ebo, bad;
    GLint n = 0, ok = 1, i, size, lp, lc, um, ut, seen_pos = 0, seen_col = 0, seen_mvp = 0, seen_tint = 0;
    GLenum type;
    GLsizei len;
    char name[64], log[256];
    static const GLfloat id[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    /* x,y, r,g,b: metade esquerda verde; quarto inferior direito vermelho */
    static const GLfloat v[] = {
        -1, -1, 0, 1, 0,   0, -1, 0, 1, 0,   -1, 1, 0, 1, 0,   0, 1, 0, 1, 0,
         0, -1, 1, 0, 0,   1, -1, 1, 0, 0,    0, 0, 1, 0, 0,   1, 0, 1, 0, 0 };
    static const GLushort idx[] = { 4, 5, 6, 6, 5, 7 };
    const char *bad_src = "#version 300 es\nvoid main(){ isto nao compila }\n";

    t_setup();
    p = t_program(VS, FS);

    glGetProgramiv(p, GL_ACTIVE_ATTRIBUTES, &n);
    CHECK_EQ(n, 2);
    for (i = 0; i < n; i++) {
        memset(name, 'x', sizeof name); len = -1; size = -1; type = 0;
        glGetActiveAttrib(p, (GLuint)i, sizeof name, &len, &size, &type, name);
        CHECK(len > 0 && len == (GLsizei)strlen(name));
        CHECK_EQ(size, 1);
        if (!strcmp(name, "aPos")) { seen_pos = 1; CHECK_EQ(type, GL_FLOAT_VEC2); }
        if (!strcmp(name, "aCol")) { seen_col = 1; CHECK_EQ(type, GL_FLOAT_VEC3); }
    }
    CHECK(seen_pos && seen_col);

    glGetProgramiv(p, GL_ACTIVE_UNIFORMS, &n);
    CHECK_EQ(n, 2);
    for (i = 0; i < n; i++) {
        memset(name, 'x', sizeof name); len = -1; size = -1; type = 0;
        glGetActiveUniform(p, (GLuint)i, sizeof name, &len, &size, &type, name);
        if (!strcmp(name, "uMvp"))  { seen_mvp = 1;  CHECK_EQ(type, GL_FLOAT_MAT4); }
        if (!strcmp(name, "uTint")) { seen_tint = 1; CHECK_EQ(type, GL_FLOAT_VEC4); }
    }
    CHECK(seen_mvp && seen_tint);

    lp = glGetAttribLocation(p, "aPos");  lc = glGetAttribLocation(p, "aCol");
    um = glGetUniformLocation(p, "uMvp"); ut = glGetUniformLocation(p, "uTint");
    CHECK(lp >= 0 && lc >= 0 && lp != lc);
    CHECK(um >= 0 && ut >= 0 && um != ut);
    CHECK_EQ(glGetUniformLocation(p, "naoExiste"), -1);

    /* shader quebrado: status falso e log não vazio */
    bad = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(bad, 1, &bad_src, NULL);
    glCompileShader(bad);
    glGetShaderiv(bad, GL_COMPILE_STATUS, &ok);
    CHECK_EQ(ok, GL_FALSE);
    log[0] = 0; len = 0;
    glGetShaderInfoLog(bad, sizeof log, &len, log);
    CHECK(len > 0 && strlen(log) == (size_t)len);
    glDeleteShader(bad);

    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof idx, idx, GL_STATIC_DRAW);
    CHECK(vao != 0 && vbo != 0 && ebo != 0 && vbo != ebo);
    glEnableVertexAttribArray((GLuint)lp);
    glVertexAttribPointer((GLuint)lp, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)0);
    glEnableVertexAttribArray((GLuint)lc);
    glVertexAttribPointer((GLuint)lc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat)));

    glViewport(0, 0, T_W, T_H);
    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(p);
    glUniformMatrix4fv(um, 1, GL_FALSE, id);
    glUniform4f(ut, 1, 1, 1, 1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)0);

    expect_px(T_W / 4, T_H / 2, 0, 255, 0, 255);          /* esquerda: verde (glDrawArrays) */
    expect_px(3 * T_W / 4, T_H / 4, 255, 0, 0, 255);      /* direita embaixo: vermelho (glDrawElements) */
    expect_px(3 * T_W / 4, 3 * T_H / 4, 0, 0, 255, 255);  /* direita em cima: fundo azul */

    glUniform4fv(ut, 1, (const GLfloat[]){ 0.0f, 0.0f, 0.0f, 1.0f });   /* tinta preta */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    expect_px(T_W / 4, T_H / 2, 0, 0, 0, 255);
    CHECK(eglSwapBuffers(g_d, g_s));
    return t_done("t_shader");
}
