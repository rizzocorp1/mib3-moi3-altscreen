/* Renderização fora da tela: FBO com textura, blit e leitura do FBO. A HMI compõe as áreas
 * de tela assim. */
#include "t_common.h"

int main(void)
{
    GLuint fbo, tex, rbo;
    GLint cur = -1, type = 0;
    unsigned char px[4 * 4 * 4];
    static const GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
    static const GLenum inval[] = { GL_DEPTH_ATTACHMENT };
    int i, all = 1;

    t_setup();
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 64, 64);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 64, 64);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    CHECK_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur);
    CHECK_EQ(cur, (GLint)fbo);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    CHECK_EQ(type, GL_TEXTURE);
    glDrawBuffers(1, bufs);
    glViewport(0, 0, 64, 64);
    glClearColor(1, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, inval);

    /* leitura de um bloco 4x4 do FBO */
    memset(px, 9, sizeof px);
    glReadPixels(10, 10, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, px);
    for (i = 0; i < 16; i++)
        if (px[4 * i] != 255 || px[4 * i + 1] != 255 || px[4 * i + 2] != 0 || px[4 * i + 3] != 255) all = 0;
    CHECK(all);

    /* blit do FBO para um canto da tela */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, T_W, T_H);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, 64, 64, 100, 100, 300, 300, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    expect_px(200, 200, 255, 255, 0, 255);
    expect_px(50, 50, 0, 0, 0, 255);
    expect_px(350, 350, 0, 0, 0, 255);

    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &tex);
    CHECK(eglSwapBuffers(g_d, g_s));
    return t_done("t_fbo");
}
