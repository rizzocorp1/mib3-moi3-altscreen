#!/usr/bin/env python3
"""gen_glfwd.py — gera as partes repetitivas do encaminhador de OpenGL ES 3.2 do sandbox MOI3.

Lê os protótipos do cabeçalho Khronos GLES3/gl32.h (358 funções) e escreve, em gen/:
  ops_gen.h      um código de operação por função GL + tabela de nomes
  shim_gen.inc   lado da HMI: cada função empacota os argumentos e manda pelo socket
  host_gen.inc   lado do glhost: desempacota, chama o GL de verdade (Mesa) e responde
  procs_gen.inc  tabela nome -> função, para o eglGetProcAddress
  exports.txt    nomes exportados (conferido pelo teste de exports)

Funções só com escalares saem automáticas. As que têm ponteiro precisam de uma regra em SPEC
(quantos bytes entram ou saem). As de MANUAL são escritas à mão em shim.c e host.c. O que
sobrar vira um stub que avisa uma vez ("NAO IMPLEMENTADA"), para a lista crescer por medição.

Uso (no Mac):  python3 -I gen_glfwd.py <.../GLES3/gl32.h> <pasta gen/>
"""
import os
import re
import sys

S4 = {"GLenum", "GLuint", "GLint", "GLsizei", "GLbitfield", "GLboolean", "GLfloat", "GLubyte", "GLbyte"}
S8 = {"GLintptr", "GLsizeiptr", "GLint64", "GLuint64", "GLsync"}


def IN(expr):
    return ("in", expr)


def OUT(expr):
    return ("out", expr)


STR = ("str",)
OFF = ("off",)

PN_TEX = "(pname==0x1004?4:1)*4"      # GL_TEXTURE_BORDER_COLOR tem 4 valores; o resto, 1
PN_ATTR = "(pname==0x8626?4:1)*4"     # GL_CURRENT_VERTEX_ATTRIB
NAMED = {"length": OUT("4"), "size": OUT("4"), "type": OUT("4"), "name": OUT("bufSize")}

SPEC = {
    "glBindAttribLocation": {"name": STR},
    "glBufferData": {"data": IN("size")},
    "glBufferSubData": {"data": IN("size")},
    "glGetAttribLocation": {"name": STR},
    "glGetUniformLocation": {"name": STR},
    "glGetFragDataLocation": {"name": STR},
    "glGetUniformBlockIndex": {"uniformBlockName": STR},
    "glGetProgramResourceIndex": {"name": STR},
    "glGetProgramResourceLocation": {"name": STR},
    "glGetActiveAttrib": NAMED,
    "glGetActiveUniform": NAMED,
    "glGetTransformFeedbackVarying": NAMED,
    "glGetAttachedShaders": {"count": OUT("4"), "shaders": OUT("maxCount*4")},
    "glGetBufferParameteri64v": {"params": OUT("8")},
    "glGetFramebufferAttachmentParameteriv": {"params": OUT("4")},
    "glGetProgramiv": {"params": OUT("4")},
    "glGetShaderiv": {"params": OUT("4")},
    "glGetProgramInfoLog": {"length": OUT("4"), "infoLog": OUT("bufSize")},
    "glGetShaderInfoLog": {"length": OUT("4"), "infoLog": OUT("bufSize")},
    "glGetShaderSource": {"length": OUT("4"), "source": OUT("bufSize")},
    "glGetProgramPipelineInfoLog": {"length": OUT("4"), "infoLog": OUT("bufSize")},
    "glGetRenderbufferParameteriv": {"params": OUT("4")},
    "glGetShaderPrecisionFormat": {"range": OUT("8"), "precision": OUT("4")},
    "glGetQueryiv": {"params": OUT("4")},
    "glGetQueryObjectuiv": {"params": OUT("4")},
    "glDrawBuffers": {"bufs": IN("n*4")},
    "glGetIntegeri_v": {"data": OUT("4")},
    "glGetInteger64i_v": {"data": OUT("8")},
    "glGetBooleani_v": {"data": OUT("1")},
    "glVertexAttribI4iv": {"v": IN("16")},
    "glVertexAttribI4uiv": {"v": IN("16")},
    "glGetActiveUniformsiv": {"uniformIndices": IN("uniformCount*4"), "params": OUT("uniformCount*4")},
    "glGetActiveUniformBlockName": {"length": OUT("4"), "uniformBlockName": OUT("bufSize")},
    "glGetSynciv": {"length": OUT("4"), "values": OUT("bufSize*4")},
    "glGetProgramBinary": {"length": OUT("4"), "binaryFormat": OUT("4"), "binary": OUT("bufSize")},
    "glProgramBinary": {"binary": IN("length")},
    "glInvalidateFramebuffer": {"attachments": IN("numAttachments*4")},
    "glInvalidateSubFramebuffer": {"attachments": IN("numAttachments*4")},
    "glGetInternalformativ": {"params": OUT("bufSize*4")},
    "glDrawArraysIndirect": {"indirect": OFF},
    "glDrawElementsIndirect": {"indirect": OFF},
    "glGetFramebufferParameteriv": {"params": OUT("4")},
    "glGetProgramInterfaceiv": {"params": OUT("4")},
    "glGetProgramResourceName": {"length": OUT("4"), "name": OUT("bufSize")},
    "glGetProgramResourceiv": {"props": IN("propCount*4"), "length": OUT("4"), "params": OUT("bufSize*4")},
    "glGetProgramPipelineiv": {"params": OUT("4")},
    "glGetMultisamplefv": {"val": OUT("8")},
    "glGetTexLevelParameteriv": {"params": OUT("4")},
    "glGetTexLevelParameterfv": {"params": OUT("4")},
    "glGetnUniformfv": {"params": OUT("bufSize")},
    "glGetnUniformiv": {"params": OUT("bufSize")},
    "glGetnUniformuiv": {"params": OUT("bufSize")},
}
for _s in ("fv", "iv", "Iiv", "Iuiv"):
    SPEC["glTexParameter" + _s] = {"params": IN(PN_TEX)}
    SPEC["glGetTexParameter" + _s] = {"params": OUT(PN_TEX)}
    SPEC["glSamplerParameter" + _s] = {"param": IN(PN_TEX)}
    SPEC["glGetSamplerParameter" + _s] = {"params": OUT(PN_TEX)}
    SPEC["glGetVertexAttrib" + _s] = {"params": OUT(PN_ATTR)}
for _s in ("iv", "uiv", "fv"):
    SPEC["glClearBuffer" + _s] = {"value": IN("(buffer==0x1800?4:1)*4")}     # GL_COLOR
for _n in (1, 2, 3, 4):
    SPEC["glVertexAttrib%dfv" % _n] = {"v": IN("%d*4" % _n)}
    for _t in ("f", "i", "ui"):
        SPEC["glUniform%d%sv" % (_n, _t)] = {"value": IN("count*%d*4" % _n)}
        SPEC["glProgramUniform%d%sv" % (_n, _t)] = {"value": IN("count*%d*4" % _n)}
for _d, _k in (("2", 4), ("3", 9), ("4", 16), ("2x3", 6), ("3x2", 6), ("2x4", 8), ("4x2", 8), ("3x4", 12), ("4x3", 12)):
    SPEC["glUniformMatrix%sfv" % _d] = {"value": IN("count*%d*4" % _k)}
    SPEC["glProgramUniformMatrix%sfv" % _d] = {"value": IN("count*%d*4" % _k)}
for _o, _c, _p in (("Buffers", "n", "buffers"), ("Framebuffers", "n", "framebuffers"),
                   ("Renderbuffers", "n", "renderbuffers"), ("Textures", "n", "textures"),
                   ("Queries", "n", "ids"), ("VertexArrays", "n", "arrays"), ("Samplers", "count", "samplers"),
                   ("TransformFeedbacks", "n", "ids"), ("ProgramPipelines", "n", "pipelines")):
    SPEC["glGen" + _o] = {_p: OUT(_c + "*4")}
    SPEC["glDelete" + _o] = {_p: IN(_c + "*4")}

# escritas à mão nos dois lados (shim.c e host.c)
MANUAL = {
    "glGetString", "glGetStringi", "glGetBooleanv", "glGetIntegerv", "glGetFloatv", "glGetInteger64v",
    "glShaderSource", "glTexImage2D", "glTexSubImage2D", "glTexImage3D", "glTexSubImage3D",
    "glCompressedTexImage2D", "glCompressedTexSubImage2D", "glCompressedTexImage3D",
    "glCompressedTexSubImage3D", "glReadPixels", "glReadnPixels", "glVertexAttribPointer",
    "glVertexAttribIPointer", "glDrawElements", "glDrawRangeElements", "glDrawElementsInstanced",
    "glDrawElementsBaseVertex", "glDrawRangeElementsBaseVertex", "glDrawElementsInstancedBaseVertex",
    "glMapBufferRange", "glUnmapBuffer", "glFlushMappedBufferRange", "glGetBufferPointerv",
    "glGetBufferParameteriv", "glGetUniformIndices", "glTransformFeedbackVaryings",
    "glGetActiveUniformBlockiv", "glGetError", "glFinish", "glFlush",
}
# resolvidas só no shim, sem ir ao host (depuração do KHR_debug)
LOCAL = {
    "glDebugMessageCallback", "glDebugMessageControl", "glDebugMessageInsert", "glPushDebugGroup",
    "glPopDebugGroup", "glObjectLabel", "glObjectPtrLabel", "glGetDebugMessageLog", "glGetObjectLabel",
    "glGetObjectPtrLabel", "glGetPointerv",
}
# o shim precisa acompanhar este estado para saber o tamanho dos dados do cliente
HOOKS = {
    "glPixelStorei": "H_pixelstore(pname, param);",
    "glBindBuffer": "H_bindbuffer(target, buffer);",
    "glBindBufferBase": "H_bindbuffer(target, buffer);",
    "glBindBufferRange": "H_bindbuffer(target, buffer);",
    "glBindVertexArray": "H_bindvao(array);",
    "glDeleteBuffers": "H_delbuffers(n, buffers);",
    "glDeleteVertexArrays": "H_delvaos(n, arrays);",
}


def parse(h):
    out = []
    for m in re.finditer(r"GL_APICALL\s+(.+?)\s*GL_APIENTRY\s+(\w+)\s*\(([^;]*)\);", h):
        ret, name, params = m.group(1).strip(), m.group(2), m.group(3).strip()
        ps = []
        if params != "void":
            for p in params.split(","):
                mm = re.match(r"(.*?)(\w+)$", p.strip())
                ps.append((mm.group(1).strip(), mm.group(2)))
        out.append((ret, name, ps))
    return out


def base(t):
    return t.replace("const", "").strip()


def is_scalar(t):
    return "*" not in t and base(t) in (S4 | S8)


def wr(t, n):
    b = base(t)
    if b == "GLfloat":
        return "W_f32(%s);" % n
    if b == "GLsync":
        return "W_u64((uint64_t)(uintptr_t)%s);" % n
    return "W_u64((uint64_t)%s);" % n


def rd(t, n):
    b = base(t)
    if b == "GLfloat":
        return "%s %s = R_f32();" % (b, n)
    if b == "GLsync":
        return "%s %s = (GLsync)(uintptr_t)R_u64();" % (b, n)
    return "%s %s = (%s)R_u64();" % (b, n, b)


def main():
    protos = parse(open(sys.argv[1]).read())
    outdir = sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    names = [p[1] for p in protos]
    ops, shim, host, unimpl, auto, spec_n = [], [], [], [], 0, 0

    ops.append("/* GERADO por gen_glfwd.py a partir de GLES3/gl32.h. Não editar. */")
    ops.append("enum {")
    for i, n in enumerate(names):
        ops.append("  OP_%s = %d," % (n, 1000 + i))
    ops.append("  OP_GL_END = %d\n};" % (1000 + len(names)))
    ops.append("static const char *const glfwd_gl_names[] = {")
    ops.extend('  "%s",' % n for n in names)
    ops.append("};")

    shim.append("/* GERADO por gen_glfwd.py. Não editar. Lado da HMI. */")
    host.append("/* GERADO por gen_glfwd.py. Não editar. Lado do glhost: casos do switch(op). */")
    for ret, name, ps in protos:
        if name in MANUAL or name in LOCAL:
            continue
        sig = "%s %s(%s)" % (ret, name, ", ".join("%s %s" % p for p in ps) or "void")
        spec = SPEC.get(name, {})
        ptrs = [p for p in ps if not is_scalar(p[0])]
        if any(p[1] not in spec for p in ptrs) or (ret != "void" and not is_scalar(ret)):
            unimpl.append(name)
            r = "" if ret == "void" else " return 0;"
            shim.append('%s { static int once; unimpl("%s", &once);%s }' % (sig, name, r))
            continue
        if ptrs:
            spec_n += 1
        else:
            auto += 1
        outs = [p for p in ps if spec.get(p[1], ("",))[0] == "out"]
        sync = ret != "void" or bool(outs)

        s = [sig + " {"]
        if name in HOOKS:
            s.append("  " + HOOKS[name])
        s.append("  W_begin(OP_%s);" % name)
        hh = ["case OP_%s: {" % name]
        for t, n in ps:
            k = spec.get(n, ("scalar",))[0] if not is_scalar(t) else "scalar"
            if k == "scalar":
                s.append("  " + wr(t, n))
                hh.append("  " + rd(t, n))
            elif k == "in":
                s.append("  W_blob(%s, SZ(%s));" % (n, spec[n][1]))
                hh.append("  size_t n_%s; %s %s = (%s)R_blob(&n_%s); (void)n_%s;" % (n, t, n, t, n, n))
            elif k == "str":
                s.append("  W_str(%s);" % n)
                hh.append("  %s %s = (%s)R_str();" % (t, n, t))
            elif k == "off":
                s.append("  W_u64((uint64_t)(uintptr_t)%s);" % n)
                hh.append("  %s %s = (%s)(uintptr_t)R_u64();" % (t, n, t))
        if ret != "void":
            hh.append("  uint64_t *ret_ = (uint64_t *)P_reserve(8);")
        for t, n in outs:
            hh.append("  %s %s = (%s)P_reserve(SZ(%s));" % (t, n, t, spec[n][1]))
        call = "%s(%s);" % (name, ", ".join(p[1] for p in ps))
        if ret == "void":
            hh.append("  " + call)
        elif base(ret) == "GLsync":
            hh.append("  *ret_ = (uint64_t)(uintptr_t)" + call)
        else:
            hh.append("  *ret_ = (uint64_t)" + call)
        if sync:
            s.append("  W_call();")
            if ret != "void":
                cast = "(GLsync)(uintptr_t)" if base(ret) == "GLsync" else "(%s)" % ret
                s.append("  %s ret_ = %sR_u64();" % (ret, cast))
            for t, n in outs:
                s.append("  R_copy(%s, SZ(%s));" % (n, spec[n][1]))
            s.append("  W_done();")
            if ret != "void":
                s.append("  return ret_;")
            hh.append("  REPLY();")
        else:
            s.append("  W_end();")
        s.append("}")
        hh.append("} break;")
        shim.append("\n".join(s))
        host.append("\n".join(hh))

    procs = ["/* GERADO por gen_glfwd.py. Não editar. */"]
    procs.extend('  {"%s", (void *)%s},' % (n, n) for n in names)

    open(os.path.join(outdir, "ops_gen.h"), "w").write("\n".join(ops) + "\n")
    open(os.path.join(outdir, "shim_gen.inc"), "w").write("\n".join(shim) + "\n")
    open(os.path.join(outdir, "host_gen.inc"), "w").write("\n".join(host) + "\n")
    open(os.path.join(outdir, "procs_gen.inc"), "w").write("\n".join(procs) + "\n")
    open(os.path.join(outdir, "exports.txt"), "w").write("\n".join(names) + "\n")
    print("funções: %d | automáticas (só escalares): %d | por regra: %d | à mão: %d | locais: %d | sem implementação: %d"
          % (len(names), auto, spec_n, len(MANUAL), len(LOCAL), len(unimpl)))
    print("sem implementação:", ", ".join(unimpl))
    falta = [n for n in list(SPEC) + sorted(MANUAL | LOCAL) + list(HOOKS) if n not in names]
    if falta:
        print("AVISO: nomes na tabela que não existem no cabeçalho:", falta)


if __name__ == "__main__":
    main()
