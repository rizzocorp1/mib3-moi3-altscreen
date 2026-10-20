/* proto.h — protocolo do encaminhador de OpenGL do sandbox MOI3 (Marco 3, 11/10/2026).
 *
 * A HMI carrega o shim com os nomes libGLESv2_adreno.so/libEGL_adreno.so. O shim NÃO desenha:
 * empacota cada chamada e manda por um socket Unix para o glhost, que roda fora do chroot,
 * sob a glibc nova, com Mesa llvmpipe (OpenGL ES 3.2 por software).
 *
 * Pedido:    [u64 op][u64 len][payload de len bytes, sempre múltiplo de 8]
 * Resposta:  [u64 len][payload]        (só para as operações que devolvem algo)
 * Todo escalar ocupa 8 bytes. Um bloco de dados é [u64: bit 63 = ponteiro não nulo, resto =
 * tamanho][dados, completados até múltiplo de 8]. Os dois lados são aarch64 little-endian.
 * As funções GL usam os códigos OP_gl* de gen/ops_gen.h (a partir de 1000); EGL e auxiliares
 * usam os OPX_* abaixo.
 */
#ifndef GLFWD_PROTO_H
#define GLFWD_PROTO_H
#include <stdint.h>

#define GLFWD_MAGIC   0x3349464d4f4d4c47ULL   /* identifica o protocolo */
#define GLFWD_VERSION 1
#define GLFWD_PRESENT (1ULL << 63)

enum {
    OPX_HELLO = 1,          /* magic, versão                       -> ok, largura, altura da tela */
    OPX_EGL_CHOOSE,         /* atributos, máx                      -> ok, total, n, configs... */
    OPX_EGL_GETCONFIGS,     /* máx                                 -> ok, total, n, configs... */
    OPX_EGL_GETCONFIGATTR,  /* config, atributo                    -> ok, valor */
    OPX_EGL_CREATECTX,      /* config, contexto a compartilhar, atributos -> contexto (0 = falhou) */
    OPX_EGL_DESTROYCTX,     /* contexto                            (sem resposta) */
    OPX_EGL_CREATESURF,     /* tipo (0 janela, 1 pbuffer), config, largura, altura -> superfície */
    OPX_EGL_DESTROYSURF,    /* superfície                          (sem resposta) */
    OPX_EGL_MAKECURRENT,    /* draw, read, contexto                -> ok */
    OPX_EGL_SWAP,           /* superfície                          -> ok   (o glhost grava o quadro) */
    OPX_EGL_QUERYSURF,      /* superfície, atributo                -> ok, valor */
    OPX_EGL_QUERYCTX,       /* contexto, atributo                  -> ok, valor */
    OPX_SWITCH,             /* contexto, draw, read  (sem resposta: a thread que chama tem outro contexto) */
    OP_GL_BASE = 1000
};
#endif
