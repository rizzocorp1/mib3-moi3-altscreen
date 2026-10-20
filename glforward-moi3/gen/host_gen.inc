/* GERADO por gen_glfwd.py. Não editar. Lado do glhost: casos do switch(op). */
case OP_glActiveTexture: {
  GLenum texture = (GLenum)R_u64();
  glActiveTexture(texture);
} break;
case OP_glAttachShader: {
  GLuint program = (GLuint)R_u64();
  GLuint shader = (GLuint)R_u64();
  glAttachShader(program, shader);
} break;
case OP_glBindAttribLocation: {
  GLuint program = (GLuint)R_u64();
  GLuint index = (GLuint)R_u64();
  const GLchar * name = (const GLchar *)R_str();
  glBindAttribLocation(program, index, name);
} break;
case OP_glBindBuffer: {
  GLenum target = (GLenum)R_u64();
  GLuint buffer = (GLuint)R_u64();
  glBindBuffer(target, buffer);
} break;
case OP_glBindFramebuffer: {
  GLenum target = (GLenum)R_u64();
  GLuint framebuffer = (GLuint)R_u64();
  glBindFramebuffer(target, framebuffer);
} break;
case OP_glBindRenderbuffer: {
  GLenum target = (GLenum)R_u64();
  GLuint renderbuffer = (GLuint)R_u64();
  glBindRenderbuffer(target, renderbuffer);
} break;
case OP_glBindTexture: {
  GLenum target = (GLenum)R_u64();
  GLuint texture = (GLuint)R_u64();
  glBindTexture(target, texture);
} break;
case OP_glBlendColor: {
  GLfloat red = R_f32();
  GLfloat green = R_f32();
  GLfloat blue = R_f32();
  GLfloat alpha = R_f32();
  glBlendColor(red, green, blue, alpha);
} break;
case OP_glBlendEquation: {
  GLenum mode = (GLenum)R_u64();
  glBlendEquation(mode);
} break;
case OP_glBlendEquationSeparate: {
  GLenum modeRGB = (GLenum)R_u64();
  GLenum modeAlpha = (GLenum)R_u64();
  glBlendEquationSeparate(modeRGB, modeAlpha);
} break;
case OP_glBlendFunc: {
  GLenum sfactor = (GLenum)R_u64();
  GLenum dfactor = (GLenum)R_u64();
  glBlendFunc(sfactor, dfactor);
} break;
case OP_glBlendFuncSeparate: {
  GLenum sfactorRGB = (GLenum)R_u64();
  GLenum dfactorRGB = (GLenum)R_u64();
  GLenum sfactorAlpha = (GLenum)R_u64();
  GLenum dfactorAlpha = (GLenum)R_u64();
  glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
} break;
case OP_glBufferData: {
  GLenum target = (GLenum)R_u64();
  GLsizeiptr size = (GLsizeiptr)R_u64();
  size_t n_data; const void * data = (const void *)R_blob(&n_data); (void)n_data;
  GLenum usage = (GLenum)R_u64();
  glBufferData(target, size, data, usage);
} break;
case OP_glBufferSubData: {
  GLenum target = (GLenum)R_u64();
  GLintptr offset = (GLintptr)R_u64();
  GLsizeiptr size = (GLsizeiptr)R_u64();
  size_t n_data; const void * data = (const void *)R_blob(&n_data); (void)n_data;
  glBufferSubData(target, offset, size, data);
} break;
case OP_glCheckFramebufferStatus: {
  GLenum target = (GLenum)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glCheckFramebufferStatus(target);
  REPLY();
} break;
case OP_glClear: {
  GLbitfield mask = (GLbitfield)R_u64();
  glClear(mask);
} break;
case OP_glClearColor: {
  GLfloat red = R_f32();
  GLfloat green = R_f32();
  GLfloat blue = R_f32();
  GLfloat alpha = R_f32();
  glClearColor(red, green, blue, alpha);
} break;
case OP_glClearDepthf: {
  GLfloat d = R_f32();
  glClearDepthf(d);
} break;
case OP_glClearStencil: {
  GLint s = (GLint)R_u64();
  glClearStencil(s);
} break;
case OP_glColorMask: {
  GLboolean red = (GLboolean)R_u64();
  GLboolean green = (GLboolean)R_u64();
  GLboolean blue = (GLboolean)R_u64();
  GLboolean alpha = (GLboolean)R_u64();
  glColorMask(red, green, blue, alpha);
} break;
case OP_glCompileShader: {
  GLuint shader = (GLuint)R_u64();
  glCompileShader(shader);
} break;
case OP_glCopyTexImage2D: {
  GLenum target = (GLenum)R_u64();
  GLint level = (GLint)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  GLint border = (GLint)R_u64();
  glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
} break;
case OP_glCopyTexSubImage2D: {
  GLenum target = (GLenum)R_u64();
  GLint level = (GLint)R_u64();
  GLint xoffset = (GLint)R_u64();
  GLint yoffset = (GLint)R_u64();
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
} break;
case OP_glCreateProgram: {
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glCreateProgram();
  REPLY();
} break;
case OP_glCreateShader: {
  GLenum type = (GLenum)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glCreateShader(type);
  REPLY();
} break;
case OP_glCullFace: {
  GLenum mode = (GLenum)R_u64();
  glCullFace(mode);
} break;
case OP_glDeleteBuffers: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_buffers; const GLuint * buffers = (const GLuint *)R_blob(&n_buffers); (void)n_buffers;
  glDeleteBuffers(n, buffers);
} break;
case OP_glDeleteFramebuffers: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_framebuffers; const GLuint * framebuffers = (const GLuint *)R_blob(&n_framebuffers); (void)n_framebuffers;
  glDeleteFramebuffers(n, framebuffers);
} break;
case OP_glDeleteProgram: {
  GLuint program = (GLuint)R_u64();
  glDeleteProgram(program);
} break;
case OP_glDeleteRenderbuffers: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_renderbuffers; const GLuint * renderbuffers = (const GLuint *)R_blob(&n_renderbuffers); (void)n_renderbuffers;
  glDeleteRenderbuffers(n, renderbuffers);
} break;
case OP_glDeleteShader: {
  GLuint shader = (GLuint)R_u64();
  glDeleteShader(shader);
} break;
case OP_glDeleteTextures: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_textures; const GLuint * textures = (const GLuint *)R_blob(&n_textures); (void)n_textures;
  glDeleteTextures(n, textures);
} break;
case OP_glDepthFunc: {
  GLenum func = (GLenum)R_u64();
  glDepthFunc(func);
} break;
case OP_glDepthMask: {
  GLboolean flag = (GLboolean)R_u64();
  glDepthMask(flag);
} break;
case OP_glDepthRangef: {
  GLfloat n = R_f32();
  GLfloat f = R_f32();
  glDepthRangef(n, f);
} break;
case OP_glDetachShader: {
  GLuint program = (GLuint)R_u64();
  GLuint shader = (GLuint)R_u64();
  glDetachShader(program, shader);
} break;
case OP_glDisable: {
  GLenum cap = (GLenum)R_u64();
  glDisable(cap);
} break;
case OP_glDisableVertexAttribArray: {
  GLuint index = (GLuint)R_u64();
  glDisableVertexAttribArray(index);
} break;
case OP_glDrawArrays: {
  GLenum mode = (GLenum)R_u64();
  GLint first = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  glDrawArrays(mode, first, count);
} break;
case OP_glEnable: {
  GLenum cap = (GLenum)R_u64();
  glEnable(cap);
} break;
case OP_glEnableVertexAttribArray: {
  GLuint index = (GLuint)R_u64();
  glEnableVertexAttribArray(index);
} break;
case OP_glFramebufferRenderbuffer: {
  GLenum target = (GLenum)R_u64();
  GLenum attachment = (GLenum)R_u64();
  GLenum renderbuffertarget = (GLenum)R_u64();
  GLuint renderbuffer = (GLuint)R_u64();
  glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
} break;
case OP_glFramebufferTexture2D: {
  GLenum target = (GLenum)R_u64();
  GLenum attachment = (GLenum)R_u64();
  GLenum textarget = (GLenum)R_u64();
  GLuint texture = (GLuint)R_u64();
  GLint level = (GLint)R_u64();
  glFramebufferTexture2D(target, attachment, textarget, texture, level);
} break;
case OP_glFrontFace: {
  GLenum mode = (GLenum)R_u64();
  glFrontFace(mode);
} break;
case OP_glGenBuffers: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * buffers = (GLuint *)P_reserve(SZ(n*4));
  glGenBuffers(n, buffers);
  REPLY();
} break;
case OP_glGenerateMipmap: {
  GLenum target = (GLenum)R_u64();
  glGenerateMipmap(target);
} break;
case OP_glGenFramebuffers: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * framebuffers = (GLuint *)P_reserve(SZ(n*4));
  glGenFramebuffers(n, framebuffers);
  REPLY();
} break;
case OP_glGenRenderbuffers: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * renderbuffers = (GLuint *)P_reserve(SZ(n*4));
  glGenRenderbuffers(n, renderbuffers);
  REPLY();
} break;
case OP_glGenTextures: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * textures = (GLuint *)P_reserve(SZ(n*4));
  glGenTextures(n, textures);
  REPLY();
} break;
case OP_glGetActiveAttrib: {
  GLuint program = (GLuint)R_u64();
  GLuint index = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLint * size = (GLint *)P_reserve(SZ(4));
  GLenum * type = (GLenum *)P_reserve(SZ(4));
  GLchar * name = (GLchar *)P_reserve(SZ(bufSize));
  glGetActiveAttrib(program, index, bufSize, length, size, type, name);
  REPLY();
} break;
case OP_glGetActiveUniform: {
  GLuint program = (GLuint)R_u64();
  GLuint index = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLint * size = (GLint *)P_reserve(SZ(4));
  GLenum * type = (GLenum *)P_reserve(SZ(4));
  GLchar * name = (GLchar *)P_reserve(SZ(bufSize));
  glGetActiveUniform(program, index, bufSize, length, size, type, name);
  REPLY();
} break;
case OP_glGetAttachedShaders: {
  GLuint program = (GLuint)R_u64();
  GLsizei maxCount = (GLsizei)R_u64();
  GLsizei * count = (GLsizei *)P_reserve(SZ(4));
  GLuint * shaders = (GLuint *)P_reserve(SZ(maxCount*4));
  glGetAttachedShaders(program, maxCount, count, shaders);
  REPLY();
} break;
case OP_glGetAttribLocation: {
  GLuint program = (GLuint)R_u64();
  const GLchar * name = (const GLchar *)R_str();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetAttribLocation(program, name);
  REPLY();
} break;
case OP_glGetFramebufferAttachmentParameteriv: {
  GLenum target = (GLenum)R_u64();
  GLenum attachment = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
  REPLY();
} break;
case OP_glGetProgramiv: {
  GLuint program = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetProgramiv(program, pname, params);
  REPLY();
} break;
case OP_glGetProgramInfoLog: {
  GLuint program = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLchar * infoLog = (GLchar *)P_reserve(SZ(bufSize));
  glGetProgramInfoLog(program, bufSize, length, infoLog);
  REPLY();
} break;
case OP_glGetRenderbufferParameteriv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetRenderbufferParameteriv(target, pname, params);
  REPLY();
} break;
case OP_glGetShaderiv: {
  GLuint shader = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetShaderiv(shader, pname, params);
  REPLY();
} break;
case OP_glGetShaderInfoLog: {
  GLuint shader = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLchar * infoLog = (GLchar *)P_reserve(SZ(bufSize));
  glGetShaderInfoLog(shader, bufSize, length, infoLog);
  REPLY();
} break;
case OP_glGetShaderPrecisionFormat: {
  GLenum shadertype = (GLenum)R_u64();
  GLenum precisiontype = (GLenum)R_u64();
  GLint * range = (GLint *)P_reserve(SZ(8));
  GLint * precision = (GLint *)P_reserve(SZ(4));
  glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
  REPLY();
} break;
case OP_glGetShaderSource: {
  GLuint shader = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLchar * source = (GLchar *)P_reserve(SZ(bufSize));
  glGetShaderSource(shader, bufSize, length, source);
  REPLY();
} break;
case OP_glGetTexParameterfv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLfloat * params = (GLfloat *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetTexParameterfv(target, pname, params);
  REPLY();
} break;
case OP_glGetTexParameteriv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetTexParameteriv(target, pname, params);
  REPLY();
} break;
case OP_glGetUniformLocation: {
  GLuint program = (GLuint)R_u64();
  const GLchar * name = (const GLchar *)R_str();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetUniformLocation(program, name);
  REPLY();
} break;
case OP_glGetVertexAttribfv: {
  GLuint index = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLfloat * params = (GLfloat *)P_reserve(SZ((pname==0x8626?4:1)*4));
  glGetVertexAttribfv(index, pname, params);
  REPLY();
} break;
case OP_glGetVertexAttribiv: {
  GLuint index = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ((pname==0x8626?4:1)*4));
  glGetVertexAttribiv(index, pname, params);
  REPLY();
} break;
case OP_glHint: {
  GLenum target = (GLenum)R_u64();
  GLenum mode = (GLenum)R_u64();
  glHint(target, mode);
} break;
case OP_glIsBuffer: {
  GLuint buffer = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsBuffer(buffer);
  REPLY();
} break;
case OP_glIsEnabled: {
  GLenum cap = (GLenum)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsEnabled(cap);
  REPLY();
} break;
case OP_glIsFramebuffer: {
  GLuint framebuffer = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsFramebuffer(framebuffer);
  REPLY();
} break;
case OP_glIsProgram: {
  GLuint program = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsProgram(program);
  REPLY();
} break;
case OP_glIsRenderbuffer: {
  GLuint renderbuffer = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsRenderbuffer(renderbuffer);
  REPLY();
} break;
case OP_glIsShader: {
  GLuint shader = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsShader(shader);
  REPLY();
} break;
case OP_glIsTexture: {
  GLuint texture = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsTexture(texture);
  REPLY();
} break;
case OP_glLineWidth: {
  GLfloat width = R_f32();
  glLineWidth(width);
} break;
case OP_glLinkProgram: {
  GLuint program = (GLuint)R_u64();
  glLinkProgram(program);
} break;
case OP_glPixelStorei: {
  GLenum pname = (GLenum)R_u64();
  GLint param = (GLint)R_u64();
  glPixelStorei(pname, param);
} break;
case OP_glPolygonOffset: {
  GLfloat factor = R_f32();
  GLfloat units = R_f32();
  glPolygonOffset(factor, units);
} break;
case OP_glReleaseShaderCompiler: {
  glReleaseShaderCompiler();
} break;
case OP_glRenderbufferStorage: {
  GLenum target = (GLenum)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glRenderbufferStorage(target, internalformat, width, height);
} break;
case OP_glSampleCoverage: {
  GLfloat value = R_f32();
  GLboolean invert = (GLboolean)R_u64();
  glSampleCoverage(value, invert);
} break;
case OP_glScissor: {
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glScissor(x, y, width, height);
} break;
case OP_glStencilFunc: {
  GLenum func = (GLenum)R_u64();
  GLint ref = (GLint)R_u64();
  GLuint mask = (GLuint)R_u64();
  glStencilFunc(func, ref, mask);
} break;
case OP_glStencilFuncSeparate: {
  GLenum face = (GLenum)R_u64();
  GLenum func = (GLenum)R_u64();
  GLint ref = (GLint)R_u64();
  GLuint mask = (GLuint)R_u64();
  glStencilFuncSeparate(face, func, ref, mask);
} break;
case OP_glStencilMask: {
  GLuint mask = (GLuint)R_u64();
  glStencilMask(mask);
} break;
case OP_glStencilMaskSeparate: {
  GLenum face = (GLenum)R_u64();
  GLuint mask = (GLuint)R_u64();
  glStencilMaskSeparate(face, mask);
} break;
case OP_glStencilOp: {
  GLenum fail = (GLenum)R_u64();
  GLenum zfail = (GLenum)R_u64();
  GLenum zpass = (GLenum)R_u64();
  glStencilOp(fail, zfail, zpass);
} break;
case OP_glStencilOpSeparate: {
  GLenum face = (GLenum)R_u64();
  GLenum sfail = (GLenum)R_u64();
  GLenum dpfail = (GLenum)R_u64();
  GLenum dppass = (GLenum)R_u64();
  glStencilOpSeparate(face, sfail, dpfail, dppass);
} break;
case OP_glTexParameterf: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLfloat param = R_f32();
  glTexParameterf(target, pname, param);
} break;
case OP_glTexParameterfv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_params; const GLfloat * params = (const GLfloat *)R_blob(&n_params); (void)n_params;
  glTexParameterfv(target, pname, params);
} break;
case OP_glTexParameteri: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint param = (GLint)R_u64();
  glTexParameteri(target, pname, param);
} break;
case OP_glTexParameteriv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_params; const GLint * params = (const GLint *)R_blob(&n_params); (void)n_params;
  glTexParameteriv(target, pname, params);
} break;
case OP_glUniform1f: {
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  glUniform1f(location, v0);
} break;
case OP_glUniform1fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniform1fv(location, count, value);
} break;
case OP_glUniform1i: {
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  glUniform1i(location, v0);
} break;
case OP_glUniform1iv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glUniform1iv(location, count, value);
} break;
case OP_glUniform2f: {
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  GLfloat v1 = R_f32();
  glUniform2f(location, v0, v1);
} break;
case OP_glUniform2fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniform2fv(location, count, value);
} break;
case OP_glUniform2i: {
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  GLint v1 = (GLint)R_u64();
  glUniform2i(location, v0, v1);
} break;
case OP_glUniform2iv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glUniform2iv(location, count, value);
} break;
case OP_glUniform3f: {
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  GLfloat v1 = R_f32();
  GLfloat v2 = R_f32();
  glUniform3f(location, v0, v1, v2);
} break;
case OP_glUniform3fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniform3fv(location, count, value);
} break;
case OP_glUniform3i: {
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  GLint v1 = (GLint)R_u64();
  GLint v2 = (GLint)R_u64();
  glUniform3i(location, v0, v1, v2);
} break;
case OP_glUniform3iv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glUniform3iv(location, count, value);
} break;
case OP_glUniform4f: {
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  GLfloat v1 = R_f32();
  GLfloat v2 = R_f32();
  GLfloat v3 = R_f32();
  glUniform4f(location, v0, v1, v2, v3);
} break;
case OP_glUniform4fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniform4fv(location, count, value);
} break;
case OP_glUniform4i: {
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  GLint v1 = (GLint)R_u64();
  GLint v2 = (GLint)R_u64();
  GLint v3 = (GLint)R_u64();
  glUniform4i(location, v0, v1, v2, v3);
} break;
case OP_glUniform4iv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glUniform4iv(location, count, value);
} break;
case OP_glUniformMatrix2fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix2fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix3fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix3fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix4fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix4fv(location, count, transpose, value);
} break;
case OP_glUseProgram: {
  GLuint program = (GLuint)R_u64();
  glUseProgram(program);
} break;
case OP_glValidateProgram: {
  GLuint program = (GLuint)R_u64();
  glValidateProgram(program);
} break;
case OP_glVertexAttrib1f: {
  GLuint index = (GLuint)R_u64();
  GLfloat x = R_f32();
  glVertexAttrib1f(index, x);
} break;
case OP_glVertexAttrib1fv: {
  GLuint index = (GLuint)R_u64();
  size_t n_v; const GLfloat * v = (const GLfloat *)R_blob(&n_v); (void)n_v;
  glVertexAttrib1fv(index, v);
} break;
case OP_glVertexAttrib2f: {
  GLuint index = (GLuint)R_u64();
  GLfloat x = R_f32();
  GLfloat y = R_f32();
  glVertexAttrib2f(index, x, y);
} break;
case OP_glVertexAttrib2fv: {
  GLuint index = (GLuint)R_u64();
  size_t n_v; const GLfloat * v = (const GLfloat *)R_blob(&n_v); (void)n_v;
  glVertexAttrib2fv(index, v);
} break;
case OP_glVertexAttrib3f: {
  GLuint index = (GLuint)R_u64();
  GLfloat x = R_f32();
  GLfloat y = R_f32();
  GLfloat z = R_f32();
  glVertexAttrib3f(index, x, y, z);
} break;
case OP_glVertexAttrib3fv: {
  GLuint index = (GLuint)R_u64();
  size_t n_v; const GLfloat * v = (const GLfloat *)R_blob(&n_v); (void)n_v;
  glVertexAttrib3fv(index, v);
} break;
case OP_glVertexAttrib4f: {
  GLuint index = (GLuint)R_u64();
  GLfloat x = R_f32();
  GLfloat y = R_f32();
  GLfloat z = R_f32();
  GLfloat w = R_f32();
  glVertexAttrib4f(index, x, y, z, w);
} break;
case OP_glVertexAttrib4fv: {
  GLuint index = (GLuint)R_u64();
  size_t n_v; const GLfloat * v = (const GLfloat *)R_blob(&n_v); (void)n_v;
  glVertexAttrib4fv(index, v);
} break;
case OP_glViewport: {
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glViewport(x, y, width, height);
} break;
case OP_glReadBuffer: {
  GLenum src = (GLenum)R_u64();
  glReadBuffer(src);
} break;
case OP_glCopyTexSubImage3D: {
  GLenum target = (GLenum)R_u64();
  GLint level = (GLint)R_u64();
  GLint xoffset = (GLint)R_u64();
  GLint yoffset = (GLint)R_u64();
  GLint zoffset = (GLint)R_u64();
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
} break;
case OP_glGenQueries: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * ids = (GLuint *)P_reserve(SZ(n*4));
  glGenQueries(n, ids);
  REPLY();
} break;
case OP_glDeleteQueries: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_ids; const GLuint * ids = (const GLuint *)R_blob(&n_ids); (void)n_ids;
  glDeleteQueries(n, ids);
} break;
case OP_glIsQuery: {
  GLuint id = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsQuery(id);
  REPLY();
} break;
case OP_glBeginQuery: {
  GLenum target = (GLenum)R_u64();
  GLuint id = (GLuint)R_u64();
  glBeginQuery(target, id);
} break;
case OP_glEndQuery: {
  GLenum target = (GLenum)R_u64();
  glEndQuery(target);
} break;
case OP_glGetQueryiv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetQueryiv(target, pname, params);
  REPLY();
} break;
case OP_glGetQueryObjectuiv: {
  GLuint id = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLuint * params = (GLuint *)P_reserve(SZ(4));
  glGetQueryObjectuiv(id, pname, params);
  REPLY();
} break;
case OP_glDrawBuffers: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_bufs; const GLenum * bufs = (const GLenum *)R_blob(&n_bufs); (void)n_bufs;
  glDrawBuffers(n, bufs);
} break;
case OP_glUniformMatrix2x3fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix2x3fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix3x2fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix3x2fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix2x4fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix2x4fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix4x2fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix4x2fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix3x4fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix3x4fv(location, count, transpose, value);
} break;
case OP_glUniformMatrix4x3fv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glUniformMatrix4x3fv(location, count, transpose, value);
} break;
case OP_glBlitFramebuffer: {
  GLint srcX0 = (GLint)R_u64();
  GLint srcY0 = (GLint)R_u64();
  GLint srcX1 = (GLint)R_u64();
  GLint srcY1 = (GLint)R_u64();
  GLint dstX0 = (GLint)R_u64();
  GLint dstY0 = (GLint)R_u64();
  GLint dstX1 = (GLint)R_u64();
  GLint dstY1 = (GLint)R_u64();
  GLbitfield mask = (GLbitfield)R_u64();
  GLenum filter = (GLenum)R_u64();
  glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
} break;
case OP_glRenderbufferStorageMultisample: {
  GLenum target = (GLenum)R_u64();
  GLsizei samples = (GLsizei)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
} break;
case OP_glFramebufferTextureLayer: {
  GLenum target = (GLenum)R_u64();
  GLenum attachment = (GLenum)R_u64();
  GLuint texture = (GLuint)R_u64();
  GLint level = (GLint)R_u64();
  GLint layer = (GLint)R_u64();
  glFramebufferTextureLayer(target, attachment, texture, level, layer);
} break;
case OP_glBindVertexArray: {
  GLuint array = (GLuint)R_u64();
  glBindVertexArray(array);
} break;
case OP_glDeleteVertexArrays: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_arrays; const GLuint * arrays = (const GLuint *)R_blob(&n_arrays); (void)n_arrays;
  glDeleteVertexArrays(n, arrays);
} break;
case OP_glGenVertexArrays: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * arrays = (GLuint *)P_reserve(SZ(n*4));
  glGenVertexArrays(n, arrays);
  REPLY();
} break;
case OP_glIsVertexArray: {
  GLuint array = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsVertexArray(array);
  REPLY();
} break;
case OP_glGetIntegeri_v: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLint * data = (GLint *)P_reserve(SZ(4));
  glGetIntegeri_v(target, index, data);
  REPLY();
} break;
case OP_glBeginTransformFeedback: {
  GLenum primitiveMode = (GLenum)R_u64();
  glBeginTransformFeedback(primitiveMode);
} break;
case OP_glEndTransformFeedback: {
  glEndTransformFeedback();
} break;
case OP_glBindBufferRange: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLuint buffer = (GLuint)R_u64();
  GLintptr offset = (GLintptr)R_u64();
  GLsizeiptr size = (GLsizeiptr)R_u64();
  glBindBufferRange(target, index, buffer, offset, size);
} break;
case OP_glBindBufferBase: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLuint buffer = (GLuint)R_u64();
  glBindBufferBase(target, index, buffer);
} break;
case OP_glGetTransformFeedbackVarying: {
  GLuint program = (GLuint)R_u64();
  GLuint index = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLsizei * size = (GLsizei *)P_reserve(SZ(4));
  GLenum * type = (GLenum *)P_reserve(SZ(4));
  GLchar * name = (GLchar *)P_reserve(SZ(bufSize));
  glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
  REPLY();
} break;
case OP_glGetVertexAttribIiv: {
  GLuint index = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ((pname==0x8626?4:1)*4));
  glGetVertexAttribIiv(index, pname, params);
  REPLY();
} break;
case OP_glGetVertexAttribIuiv: {
  GLuint index = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLuint * params = (GLuint *)P_reserve(SZ((pname==0x8626?4:1)*4));
  glGetVertexAttribIuiv(index, pname, params);
  REPLY();
} break;
case OP_glVertexAttribI4i: {
  GLuint index = (GLuint)R_u64();
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLint z = (GLint)R_u64();
  GLint w = (GLint)R_u64();
  glVertexAttribI4i(index, x, y, z, w);
} break;
case OP_glVertexAttribI4ui: {
  GLuint index = (GLuint)R_u64();
  GLuint x = (GLuint)R_u64();
  GLuint y = (GLuint)R_u64();
  GLuint z = (GLuint)R_u64();
  GLuint w = (GLuint)R_u64();
  glVertexAttribI4ui(index, x, y, z, w);
} break;
case OP_glVertexAttribI4iv: {
  GLuint index = (GLuint)R_u64();
  size_t n_v; const GLint * v = (const GLint *)R_blob(&n_v); (void)n_v;
  glVertexAttribI4iv(index, v);
} break;
case OP_glVertexAttribI4uiv: {
  GLuint index = (GLuint)R_u64();
  size_t n_v; const GLuint * v = (const GLuint *)R_blob(&n_v); (void)n_v;
  glVertexAttribI4uiv(index, v);
} break;
case OP_glGetFragDataLocation: {
  GLuint program = (GLuint)R_u64();
  const GLchar * name = (const GLchar *)R_str();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetFragDataLocation(program, name);
  REPLY();
} break;
case OP_glUniform1ui: {
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  glUniform1ui(location, v0);
} break;
case OP_glUniform2ui: {
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  GLuint v1 = (GLuint)R_u64();
  glUniform2ui(location, v0, v1);
} break;
case OP_glUniform3ui: {
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  GLuint v1 = (GLuint)R_u64();
  GLuint v2 = (GLuint)R_u64();
  glUniform3ui(location, v0, v1, v2);
} break;
case OP_glUniform4ui: {
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  GLuint v1 = (GLuint)R_u64();
  GLuint v2 = (GLuint)R_u64();
  GLuint v3 = (GLuint)R_u64();
  glUniform4ui(location, v0, v1, v2, v3);
} break;
case OP_glUniform1uiv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glUniform1uiv(location, count, value);
} break;
case OP_glUniform2uiv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glUniform2uiv(location, count, value);
} break;
case OP_glUniform3uiv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glUniform3uiv(location, count, value);
} break;
case OP_glUniform4uiv: {
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glUniform4uiv(location, count, value);
} break;
case OP_glClearBufferiv: {
  GLenum buffer = (GLenum)R_u64();
  GLint drawbuffer = (GLint)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glClearBufferiv(buffer, drawbuffer, value);
} break;
case OP_glClearBufferuiv: {
  GLenum buffer = (GLenum)R_u64();
  GLint drawbuffer = (GLint)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glClearBufferuiv(buffer, drawbuffer, value);
} break;
case OP_glClearBufferfv: {
  GLenum buffer = (GLenum)R_u64();
  GLint drawbuffer = (GLint)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glClearBufferfv(buffer, drawbuffer, value);
} break;
case OP_glClearBufferfi: {
  GLenum buffer = (GLenum)R_u64();
  GLint drawbuffer = (GLint)R_u64();
  GLfloat depth = R_f32();
  GLint stencil = (GLint)R_u64();
  glClearBufferfi(buffer, drawbuffer, depth, stencil);
} break;
case OP_glCopyBufferSubData: {
  GLenum readTarget = (GLenum)R_u64();
  GLenum writeTarget = (GLenum)R_u64();
  GLintptr readOffset = (GLintptr)R_u64();
  GLintptr writeOffset = (GLintptr)R_u64();
  GLsizeiptr size = (GLsizeiptr)R_u64();
  glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
} break;
case OP_glGetActiveUniformsiv: {
  GLuint program = (GLuint)R_u64();
  GLsizei uniformCount = (GLsizei)R_u64();
  size_t n_uniformIndices; const GLuint * uniformIndices = (const GLuint *)R_blob(&n_uniformIndices); (void)n_uniformIndices;
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(uniformCount*4));
  glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
  REPLY();
} break;
case OP_glGetUniformBlockIndex: {
  GLuint program = (GLuint)R_u64();
  const GLchar * uniformBlockName = (const GLchar *)R_str();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetUniformBlockIndex(program, uniformBlockName);
  REPLY();
} break;
case OP_glGetActiveUniformBlockName: {
  GLuint program = (GLuint)R_u64();
  GLuint uniformBlockIndex = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLchar * uniformBlockName = (GLchar *)P_reserve(SZ(bufSize));
  glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
  REPLY();
} break;
case OP_glUniformBlockBinding: {
  GLuint program = (GLuint)R_u64();
  GLuint uniformBlockIndex = (GLuint)R_u64();
  GLuint uniformBlockBinding = (GLuint)R_u64();
  glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
} break;
case OP_glDrawArraysInstanced: {
  GLenum mode = (GLenum)R_u64();
  GLint first = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLsizei instancecount = (GLsizei)R_u64();
  glDrawArraysInstanced(mode, first, count, instancecount);
} break;
case OP_glFenceSync: {
  GLenum condition = (GLenum)R_u64();
  GLbitfield flags = (GLbitfield)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)(uintptr_t)glFenceSync(condition, flags);
  REPLY();
} break;
case OP_glIsSync: {
  GLsync sync = (GLsync)(uintptr_t)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsSync(sync);
  REPLY();
} break;
case OP_glDeleteSync: {
  GLsync sync = (GLsync)(uintptr_t)R_u64();
  glDeleteSync(sync);
} break;
case OP_glClientWaitSync: {
  GLsync sync = (GLsync)(uintptr_t)R_u64();
  GLbitfield flags = (GLbitfield)R_u64();
  GLuint64 timeout = (GLuint64)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glClientWaitSync(sync, flags, timeout);
  REPLY();
} break;
case OP_glWaitSync: {
  GLsync sync = (GLsync)(uintptr_t)R_u64();
  GLbitfield flags = (GLbitfield)R_u64();
  GLuint64 timeout = (GLuint64)R_u64();
  glWaitSync(sync, flags, timeout);
} break;
case OP_glGetSynciv: {
  GLsync sync = (GLsync)(uintptr_t)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLint * values = (GLint *)P_reserve(SZ(bufSize*4));
  glGetSynciv(sync, pname, bufSize, length, values);
  REPLY();
} break;
case OP_glGetInteger64i_v: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLint64 * data = (GLint64 *)P_reserve(SZ(8));
  glGetInteger64i_v(target, index, data);
  REPLY();
} break;
case OP_glGetBufferParameteri64v: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint64 * params = (GLint64 *)P_reserve(SZ(8));
  glGetBufferParameteri64v(target, pname, params);
  REPLY();
} break;
case OP_glGenSamplers: {
  GLsizei count = (GLsizei)R_u64();
  GLuint * samplers = (GLuint *)P_reserve(SZ(count*4));
  glGenSamplers(count, samplers);
  REPLY();
} break;
case OP_glDeleteSamplers: {
  GLsizei count = (GLsizei)R_u64();
  size_t n_samplers; const GLuint * samplers = (const GLuint *)R_blob(&n_samplers); (void)n_samplers;
  glDeleteSamplers(count, samplers);
} break;
case OP_glIsSampler: {
  GLuint sampler = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsSampler(sampler);
  REPLY();
} break;
case OP_glBindSampler: {
  GLuint unit = (GLuint)R_u64();
  GLuint sampler = (GLuint)R_u64();
  glBindSampler(unit, sampler);
} break;
case OP_glSamplerParameteri: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint param = (GLint)R_u64();
  glSamplerParameteri(sampler, pname, param);
} break;
case OP_glSamplerParameteriv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_param; const GLint * param = (const GLint *)R_blob(&n_param); (void)n_param;
  glSamplerParameteriv(sampler, pname, param);
} break;
case OP_glSamplerParameterf: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLfloat param = R_f32();
  glSamplerParameterf(sampler, pname, param);
} break;
case OP_glSamplerParameterfv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_param; const GLfloat * param = (const GLfloat *)R_blob(&n_param); (void)n_param;
  glSamplerParameterfv(sampler, pname, param);
} break;
case OP_glGetSamplerParameteriv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetSamplerParameteriv(sampler, pname, params);
  REPLY();
} break;
case OP_glGetSamplerParameterfv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLfloat * params = (GLfloat *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetSamplerParameterfv(sampler, pname, params);
  REPLY();
} break;
case OP_glVertexAttribDivisor: {
  GLuint index = (GLuint)R_u64();
  GLuint divisor = (GLuint)R_u64();
  glVertexAttribDivisor(index, divisor);
} break;
case OP_glBindTransformFeedback: {
  GLenum target = (GLenum)R_u64();
  GLuint id = (GLuint)R_u64();
  glBindTransformFeedback(target, id);
} break;
case OP_glDeleteTransformFeedbacks: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_ids; const GLuint * ids = (const GLuint *)R_blob(&n_ids); (void)n_ids;
  glDeleteTransformFeedbacks(n, ids);
} break;
case OP_glGenTransformFeedbacks: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * ids = (GLuint *)P_reserve(SZ(n*4));
  glGenTransformFeedbacks(n, ids);
  REPLY();
} break;
case OP_glIsTransformFeedback: {
  GLuint id = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsTransformFeedback(id);
  REPLY();
} break;
case OP_glPauseTransformFeedback: {
  glPauseTransformFeedback();
} break;
case OP_glResumeTransformFeedback: {
  glResumeTransformFeedback();
} break;
case OP_glGetProgramBinary: {
  GLuint program = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLenum * binaryFormat = (GLenum *)P_reserve(SZ(4));
  void * binary = (void *)P_reserve(SZ(bufSize));
  glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
  REPLY();
} break;
case OP_glProgramBinary: {
  GLuint program = (GLuint)R_u64();
  GLenum binaryFormat = (GLenum)R_u64();
  size_t n_binary; const void * binary = (const void *)R_blob(&n_binary); (void)n_binary;
  GLsizei length = (GLsizei)R_u64();
  glProgramBinary(program, binaryFormat, binary, length);
} break;
case OP_glProgramParameteri: {
  GLuint program = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint value = (GLint)R_u64();
  glProgramParameteri(program, pname, value);
} break;
case OP_glInvalidateFramebuffer: {
  GLenum target = (GLenum)R_u64();
  GLsizei numAttachments = (GLsizei)R_u64();
  size_t n_attachments; const GLenum * attachments = (const GLenum *)R_blob(&n_attachments); (void)n_attachments;
  glInvalidateFramebuffer(target, numAttachments, attachments);
} break;
case OP_glInvalidateSubFramebuffer: {
  GLenum target = (GLenum)R_u64();
  GLsizei numAttachments = (GLsizei)R_u64();
  size_t n_attachments; const GLenum * attachments = (const GLenum *)R_blob(&n_attachments); (void)n_attachments;
  GLint x = (GLint)R_u64();
  GLint y = (GLint)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
} break;
case OP_glTexStorage2D: {
  GLenum target = (GLenum)R_u64();
  GLsizei levels = (GLsizei)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  glTexStorage2D(target, levels, internalformat, width, height);
} break;
case OP_glTexStorage3D: {
  GLenum target = (GLenum)R_u64();
  GLsizei levels = (GLsizei)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  GLsizei depth = (GLsizei)R_u64();
  glTexStorage3D(target, levels, internalformat, width, height, depth);
} break;
case OP_glGetInternalformativ: {
  GLenum target = (GLenum)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(bufSize*4));
  glGetInternalformativ(target, internalformat, pname, bufSize, params);
  REPLY();
} break;
case OP_glDispatchCompute: {
  GLuint num_groups_x = (GLuint)R_u64();
  GLuint num_groups_y = (GLuint)R_u64();
  GLuint num_groups_z = (GLuint)R_u64();
  glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
} break;
case OP_glDispatchComputeIndirect: {
  GLintptr indirect = (GLintptr)R_u64();
  glDispatchComputeIndirect(indirect);
} break;
case OP_glDrawArraysIndirect: {
  GLenum mode = (GLenum)R_u64();
  const void * indirect = (const void *)(uintptr_t)R_u64();
  glDrawArraysIndirect(mode, indirect);
} break;
case OP_glDrawElementsIndirect: {
  GLenum mode = (GLenum)R_u64();
  GLenum type = (GLenum)R_u64();
  const void * indirect = (const void *)(uintptr_t)R_u64();
  glDrawElementsIndirect(mode, type, indirect);
} break;
case OP_glFramebufferParameteri: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint param = (GLint)R_u64();
  glFramebufferParameteri(target, pname, param);
} break;
case OP_glGetFramebufferParameteriv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetFramebufferParameteriv(target, pname, params);
  REPLY();
} break;
case OP_glGetProgramInterfaceiv: {
  GLuint program = (GLuint)R_u64();
  GLenum programInterface = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetProgramInterfaceiv(program, programInterface, pname, params);
  REPLY();
} break;
case OP_glGetProgramResourceIndex: {
  GLuint program = (GLuint)R_u64();
  GLenum programInterface = (GLenum)R_u64();
  const GLchar * name = (const GLchar *)R_str();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetProgramResourceIndex(program, programInterface, name);
  REPLY();
} break;
case OP_glGetProgramResourceName: {
  GLuint program = (GLuint)R_u64();
  GLenum programInterface = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLchar * name = (GLchar *)P_reserve(SZ(bufSize));
  glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
  REPLY();
} break;
case OP_glGetProgramResourceiv: {
  GLuint program = (GLuint)R_u64();
  GLenum programInterface = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLsizei propCount = (GLsizei)R_u64();
  size_t n_props; const GLenum * props = (const GLenum *)R_blob(&n_props); (void)n_props;
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLint * params = (GLint *)P_reserve(SZ(bufSize*4));
  glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
  REPLY();
} break;
case OP_glGetProgramResourceLocation: {
  GLuint program = (GLuint)R_u64();
  GLenum programInterface = (GLenum)R_u64();
  const GLchar * name = (const GLchar *)R_str();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetProgramResourceLocation(program, programInterface, name);
  REPLY();
} break;
case OP_glUseProgramStages: {
  GLuint pipeline = (GLuint)R_u64();
  GLbitfield stages = (GLbitfield)R_u64();
  GLuint program = (GLuint)R_u64();
  glUseProgramStages(pipeline, stages, program);
} break;
case OP_glActiveShaderProgram: {
  GLuint pipeline = (GLuint)R_u64();
  GLuint program = (GLuint)R_u64();
  glActiveShaderProgram(pipeline, program);
} break;
case OP_glBindProgramPipeline: {
  GLuint pipeline = (GLuint)R_u64();
  glBindProgramPipeline(pipeline);
} break;
case OP_glDeleteProgramPipelines: {
  GLsizei n = (GLsizei)R_u64();
  size_t n_pipelines; const GLuint * pipelines = (const GLuint *)R_blob(&n_pipelines); (void)n_pipelines;
  glDeleteProgramPipelines(n, pipelines);
} break;
case OP_glGenProgramPipelines: {
  GLsizei n = (GLsizei)R_u64();
  GLuint * pipelines = (GLuint *)P_reserve(SZ(n*4));
  glGenProgramPipelines(n, pipelines);
  REPLY();
} break;
case OP_glIsProgramPipeline: {
  GLuint pipeline = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsProgramPipeline(pipeline);
  REPLY();
} break;
case OP_glGetProgramPipelineiv: {
  GLuint pipeline = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetProgramPipelineiv(pipeline, pname, params);
  REPLY();
} break;
case OP_glProgramUniform1i: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  glProgramUniform1i(program, location, v0);
} break;
case OP_glProgramUniform2i: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  GLint v1 = (GLint)R_u64();
  glProgramUniform2i(program, location, v0, v1);
} break;
case OP_glProgramUniform3i: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  GLint v1 = (GLint)R_u64();
  GLint v2 = (GLint)R_u64();
  glProgramUniform3i(program, location, v0, v1, v2);
} break;
case OP_glProgramUniform4i: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLint v0 = (GLint)R_u64();
  GLint v1 = (GLint)R_u64();
  GLint v2 = (GLint)R_u64();
  GLint v3 = (GLint)R_u64();
  glProgramUniform4i(program, location, v0, v1, v2, v3);
} break;
case OP_glProgramUniform1ui: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  glProgramUniform1ui(program, location, v0);
} break;
case OP_glProgramUniform2ui: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  GLuint v1 = (GLuint)R_u64();
  glProgramUniform2ui(program, location, v0, v1);
} break;
case OP_glProgramUniform3ui: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  GLuint v1 = (GLuint)R_u64();
  GLuint v2 = (GLuint)R_u64();
  glProgramUniform3ui(program, location, v0, v1, v2);
} break;
case OP_glProgramUniform4ui: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLuint v0 = (GLuint)R_u64();
  GLuint v1 = (GLuint)R_u64();
  GLuint v2 = (GLuint)R_u64();
  GLuint v3 = (GLuint)R_u64();
  glProgramUniform4ui(program, location, v0, v1, v2, v3);
} break;
case OP_glProgramUniform1f: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  glProgramUniform1f(program, location, v0);
} break;
case OP_glProgramUniform2f: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  GLfloat v1 = R_f32();
  glProgramUniform2f(program, location, v0, v1);
} break;
case OP_glProgramUniform3f: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  GLfloat v1 = R_f32();
  GLfloat v2 = R_f32();
  glProgramUniform3f(program, location, v0, v1, v2);
} break;
case OP_glProgramUniform4f: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLfloat v0 = R_f32();
  GLfloat v1 = R_f32();
  GLfloat v2 = R_f32();
  GLfloat v3 = R_f32();
  glProgramUniform4f(program, location, v0, v1, v2, v3);
} break;
case OP_glProgramUniform1iv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glProgramUniform1iv(program, location, count, value);
} break;
case OP_glProgramUniform2iv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glProgramUniform2iv(program, location, count, value);
} break;
case OP_glProgramUniform3iv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glProgramUniform3iv(program, location, count, value);
} break;
case OP_glProgramUniform4iv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLint * value = (const GLint *)R_blob(&n_value); (void)n_value;
  glProgramUniform4iv(program, location, count, value);
} break;
case OP_glProgramUniform1uiv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glProgramUniform1uiv(program, location, count, value);
} break;
case OP_glProgramUniform2uiv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glProgramUniform2uiv(program, location, count, value);
} break;
case OP_glProgramUniform3uiv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glProgramUniform3uiv(program, location, count, value);
} break;
case OP_glProgramUniform4uiv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLuint * value = (const GLuint *)R_blob(&n_value); (void)n_value;
  glProgramUniform4uiv(program, location, count, value);
} break;
case OP_glProgramUniform1fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniform1fv(program, location, count, value);
} break;
case OP_glProgramUniform2fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniform2fv(program, location, count, value);
} break;
case OP_glProgramUniform3fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniform3fv(program, location, count, value);
} break;
case OP_glProgramUniform4fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniform4fv(program, location, count, value);
} break;
case OP_glProgramUniformMatrix2fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix2fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix3fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix3fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix4fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix4fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix2x3fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix2x3fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix3x2fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix3x2fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix2x4fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix2x4fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix4x2fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix4x2fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix3x4fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix3x4fv(program, location, count, transpose, value);
} break;
case OP_glProgramUniformMatrix4x3fv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei count = (GLsizei)R_u64();
  GLboolean transpose = (GLboolean)R_u64();
  size_t n_value; const GLfloat * value = (const GLfloat *)R_blob(&n_value); (void)n_value;
  glProgramUniformMatrix4x3fv(program, location, count, transpose, value);
} break;
case OP_glValidateProgramPipeline: {
  GLuint pipeline = (GLuint)R_u64();
  glValidateProgramPipeline(pipeline);
} break;
case OP_glGetProgramPipelineInfoLog: {
  GLuint pipeline = (GLuint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLsizei * length = (GLsizei *)P_reserve(SZ(4));
  GLchar * infoLog = (GLchar *)P_reserve(SZ(bufSize));
  glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
  REPLY();
} break;
case OP_glBindImageTexture: {
  GLuint unit = (GLuint)R_u64();
  GLuint texture = (GLuint)R_u64();
  GLint level = (GLint)R_u64();
  GLboolean layered = (GLboolean)R_u64();
  GLint layer = (GLint)R_u64();
  GLenum access = (GLenum)R_u64();
  GLenum format = (GLenum)R_u64();
  glBindImageTexture(unit, texture, level, layered, layer, access, format);
} break;
case OP_glGetBooleani_v: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLboolean * data = (GLboolean *)P_reserve(SZ(1));
  glGetBooleani_v(target, index, data);
  REPLY();
} break;
case OP_glMemoryBarrier: {
  GLbitfield barriers = (GLbitfield)R_u64();
  glMemoryBarrier(barriers);
} break;
case OP_glMemoryBarrierByRegion: {
  GLbitfield barriers = (GLbitfield)R_u64();
  glMemoryBarrierByRegion(barriers);
} break;
case OP_glTexStorage2DMultisample: {
  GLenum target = (GLenum)R_u64();
  GLsizei samples = (GLsizei)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  GLboolean fixedsamplelocations = (GLboolean)R_u64();
  glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
} break;
case OP_glGetMultisamplefv: {
  GLenum pname = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  GLfloat * val = (GLfloat *)P_reserve(SZ(8));
  glGetMultisamplefv(pname, index, val);
  REPLY();
} break;
case OP_glSampleMaski: {
  GLuint maskNumber = (GLuint)R_u64();
  GLbitfield mask = (GLbitfield)R_u64();
  glSampleMaski(maskNumber, mask);
} break;
case OP_glGetTexLevelParameteriv: {
  GLenum target = (GLenum)R_u64();
  GLint level = (GLint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(4));
  glGetTexLevelParameteriv(target, level, pname, params);
  REPLY();
} break;
case OP_glGetTexLevelParameterfv: {
  GLenum target = (GLenum)R_u64();
  GLint level = (GLint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLfloat * params = (GLfloat *)P_reserve(SZ(4));
  glGetTexLevelParameterfv(target, level, pname, params);
  REPLY();
} break;
case OP_glBindVertexBuffer: {
  GLuint bindingindex = (GLuint)R_u64();
  GLuint buffer = (GLuint)R_u64();
  GLintptr offset = (GLintptr)R_u64();
  GLsizei stride = (GLsizei)R_u64();
  glBindVertexBuffer(bindingindex, buffer, offset, stride);
} break;
case OP_glVertexAttribFormat: {
  GLuint attribindex = (GLuint)R_u64();
  GLint size = (GLint)R_u64();
  GLenum type = (GLenum)R_u64();
  GLboolean normalized = (GLboolean)R_u64();
  GLuint relativeoffset = (GLuint)R_u64();
  glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
} break;
case OP_glVertexAttribIFormat: {
  GLuint attribindex = (GLuint)R_u64();
  GLint size = (GLint)R_u64();
  GLenum type = (GLenum)R_u64();
  GLuint relativeoffset = (GLuint)R_u64();
  glVertexAttribIFormat(attribindex, size, type, relativeoffset);
} break;
case OP_glVertexAttribBinding: {
  GLuint attribindex = (GLuint)R_u64();
  GLuint bindingindex = (GLuint)R_u64();
  glVertexAttribBinding(attribindex, bindingindex);
} break;
case OP_glVertexBindingDivisor: {
  GLuint bindingindex = (GLuint)R_u64();
  GLuint divisor = (GLuint)R_u64();
  glVertexBindingDivisor(bindingindex, divisor);
} break;
case OP_glBlendBarrier: {
  glBlendBarrier();
} break;
case OP_glCopyImageSubData: {
  GLuint srcName = (GLuint)R_u64();
  GLenum srcTarget = (GLenum)R_u64();
  GLint srcLevel = (GLint)R_u64();
  GLint srcX = (GLint)R_u64();
  GLint srcY = (GLint)R_u64();
  GLint srcZ = (GLint)R_u64();
  GLuint dstName = (GLuint)R_u64();
  GLenum dstTarget = (GLenum)R_u64();
  GLint dstLevel = (GLint)R_u64();
  GLint dstX = (GLint)R_u64();
  GLint dstY = (GLint)R_u64();
  GLint dstZ = (GLint)R_u64();
  GLsizei srcWidth = (GLsizei)R_u64();
  GLsizei srcHeight = (GLsizei)R_u64();
  GLsizei srcDepth = (GLsizei)R_u64();
  glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
} break;
case OP_glEnablei: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  glEnablei(target, index);
} break;
case OP_glDisablei: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  glDisablei(target, index);
} break;
case OP_glBlendEquationi: {
  GLuint buf = (GLuint)R_u64();
  GLenum mode = (GLenum)R_u64();
  glBlendEquationi(buf, mode);
} break;
case OP_glBlendEquationSeparatei: {
  GLuint buf = (GLuint)R_u64();
  GLenum modeRGB = (GLenum)R_u64();
  GLenum modeAlpha = (GLenum)R_u64();
  glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
} break;
case OP_glBlendFunci: {
  GLuint buf = (GLuint)R_u64();
  GLenum src = (GLenum)R_u64();
  GLenum dst = (GLenum)R_u64();
  glBlendFunci(buf, src, dst);
} break;
case OP_glBlendFuncSeparatei: {
  GLuint buf = (GLuint)R_u64();
  GLenum srcRGB = (GLenum)R_u64();
  GLenum dstRGB = (GLenum)R_u64();
  GLenum srcAlpha = (GLenum)R_u64();
  GLenum dstAlpha = (GLenum)R_u64();
  glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
} break;
case OP_glColorMaski: {
  GLuint index = (GLuint)R_u64();
  GLboolean r = (GLboolean)R_u64();
  GLboolean g = (GLboolean)R_u64();
  GLboolean b = (GLboolean)R_u64();
  GLboolean a = (GLboolean)R_u64();
  glColorMaski(index, r, g, b, a);
} break;
case OP_glIsEnabledi: {
  GLenum target = (GLenum)R_u64();
  GLuint index = (GLuint)R_u64();
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glIsEnabledi(target, index);
  REPLY();
} break;
case OP_glFramebufferTexture: {
  GLenum target = (GLenum)R_u64();
  GLenum attachment = (GLenum)R_u64();
  GLuint texture = (GLuint)R_u64();
  GLint level = (GLint)R_u64();
  glFramebufferTexture(target, attachment, texture, level);
} break;
case OP_glPrimitiveBoundingBox: {
  GLfloat minX = R_f32();
  GLfloat minY = R_f32();
  GLfloat minZ = R_f32();
  GLfloat minW = R_f32();
  GLfloat maxX = R_f32();
  GLfloat maxY = R_f32();
  GLfloat maxZ = R_f32();
  GLfloat maxW = R_f32();
  glPrimitiveBoundingBox(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
} break;
case OP_glGetGraphicsResetStatus: {
  uint64_t *ret_ = (uint64_t *)P_reserve(8);
  *ret_ = (uint64_t)glGetGraphicsResetStatus();
  REPLY();
} break;
case OP_glGetnUniformfv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLfloat * params = (GLfloat *)P_reserve(SZ(bufSize));
  glGetnUniformfv(program, location, bufSize, params);
  REPLY();
} break;
case OP_glGetnUniformiv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLint * params = (GLint *)P_reserve(SZ(bufSize));
  glGetnUniformiv(program, location, bufSize, params);
  REPLY();
} break;
case OP_glGetnUniformuiv: {
  GLuint program = (GLuint)R_u64();
  GLint location = (GLint)R_u64();
  GLsizei bufSize = (GLsizei)R_u64();
  GLuint * params = (GLuint *)P_reserve(SZ(bufSize));
  glGetnUniformuiv(program, location, bufSize, params);
  REPLY();
} break;
case OP_glMinSampleShading: {
  GLfloat value = R_f32();
  glMinSampleShading(value);
} break;
case OP_glPatchParameteri: {
  GLenum pname = (GLenum)R_u64();
  GLint value = (GLint)R_u64();
  glPatchParameteri(pname, value);
} break;
case OP_glTexParameterIiv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_params; const GLint * params = (const GLint *)R_blob(&n_params); (void)n_params;
  glTexParameterIiv(target, pname, params);
} break;
case OP_glTexParameterIuiv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_params; const GLuint * params = (const GLuint *)R_blob(&n_params); (void)n_params;
  glTexParameterIuiv(target, pname, params);
} break;
case OP_glGetTexParameterIiv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetTexParameterIiv(target, pname, params);
  REPLY();
} break;
case OP_glGetTexParameterIuiv: {
  GLenum target = (GLenum)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLuint * params = (GLuint *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetTexParameterIuiv(target, pname, params);
  REPLY();
} break;
case OP_glSamplerParameterIiv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_param; const GLint * param = (const GLint *)R_blob(&n_param); (void)n_param;
  glSamplerParameterIiv(sampler, pname, param);
} break;
case OP_glSamplerParameterIuiv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  size_t n_param; const GLuint * param = (const GLuint *)R_blob(&n_param); (void)n_param;
  glSamplerParameterIuiv(sampler, pname, param);
} break;
case OP_glGetSamplerParameterIiv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLint * params = (GLint *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetSamplerParameterIiv(sampler, pname, params);
  REPLY();
} break;
case OP_glGetSamplerParameterIuiv: {
  GLuint sampler = (GLuint)R_u64();
  GLenum pname = (GLenum)R_u64();
  GLuint * params = (GLuint *)P_reserve(SZ((pname==0x1004?4:1)*4));
  glGetSamplerParameterIuiv(sampler, pname, params);
  REPLY();
} break;
case OP_glTexBuffer: {
  GLenum target = (GLenum)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLuint buffer = (GLuint)R_u64();
  glTexBuffer(target, internalformat, buffer);
} break;
case OP_glTexBufferRange: {
  GLenum target = (GLenum)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLuint buffer = (GLuint)R_u64();
  GLintptr offset = (GLintptr)R_u64();
  GLsizeiptr size = (GLsizeiptr)R_u64();
  glTexBufferRange(target, internalformat, buffer, offset, size);
} break;
case OP_glTexStorage3DMultisample: {
  GLenum target = (GLenum)R_u64();
  GLsizei samples = (GLsizei)R_u64();
  GLenum internalformat = (GLenum)R_u64();
  GLsizei width = (GLsizei)R_u64();
  GLsizei height = (GLsizei)R_u64();
  GLsizei depth = (GLsizei)R_u64();
  GLboolean fixedsamplelocations = (GLboolean)R_u64();
  glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
} break;
