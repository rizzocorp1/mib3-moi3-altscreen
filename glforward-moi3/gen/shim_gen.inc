/* GERADO por gen_glfwd.py. Não editar. Lado da HMI. */
void glActiveTexture(GLenum texture) {
  W_begin(OP_glActiveTexture);
  W_u64((uint64_t)texture);
  W_end();
}
void glAttachShader(GLuint program, GLuint shader) {
  W_begin(OP_glAttachShader);
  W_u64((uint64_t)program);
  W_u64((uint64_t)shader);
  W_end();
}
void glBindAttribLocation(GLuint program, GLuint index, const GLchar * name) {
  W_begin(OP_glBindAttribLocation);
  W_u64((uint64_t)program);
  W_u64((uint64_t)index);
  W_str(name);
  W_end();
}
void glBindBuffer(GLenum target, GLuint buffer) {
  H_bindbuffer(target, buffer);
  W_begin(OP_glBindBuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)buffer);
  W_end();
}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
  W_begin(OP_glBindFramebuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)framebuffer);
  W_end();
}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
  W_begin(OP_glBindRenderbuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)renderbuffer);
  W_end();
}
void glBindTexture(GLenum target, GLuint texture) {
  W_begin(OP_glBindTexture);
  W_u64((uint64_t)target);
  W_u64((uint64_t)texture);
  W_end();
}
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  W_begin(OP_glBlendColor);
  W_f32(red);
  W_f32(green);
  W_f32(blue);
  W_f32(alpha);
  W_end();
}
void glBlendEquation(GLenum mode) {
  W_begin(OP_glBlendEquation);
  W_u64((uint64_t)mode);
  W_end();
}
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
  W_begin(OP_glBlendEquationSeparate);
  W_u64((uint64_t)modeRGB);
  W_u64((uint64_t)modeAlpha);
  W_end();
}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {
  W_begin(OP_glBlendFunc);
  W_u64((uint64_t)sfactor);
  W_u64((uint64_t)dfactor);
  W_end();
}
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
  W_begin(OP_glBlendFuncSeparate);
  W_u64((uint64_t)sfactorRGB);
  W_u64((uint64_t)dfactorRGB);
  W_u64((uint64_t)sfactorAlpha);
  W_u64((uint64_t)dfactorAlpha);
  W_end();
}
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage) {
  W_begin(OP_glBufferData);
  W_u64((uint64_t)target);
  W_u64((uint64_t)size);
  W_blob(data, SZ(size));
  W_u64((uint64_t)usage);
  W_end();
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) {
  W_begin(OP_glBufferSubData);
  W_u64((uint64_t)target);
  W_u64((uint64_t)offset);
  W_u64((uint64_t)size);
  W_blob(data, SZ(size));
  W_end();
}
GLenum glCheckFramebufferStatus(GLenum target) {
  W_begin(OP_glCheckFramebufferStatus);
  W_u64((uint64_t)target);
  W_call();
  GLenum ret_ = (GLenum)R_u64();
  W_done();
  return ret_;
}
void glClear(GLbitfield mask) {
  W_begin(OP_glClear);
  W_u64((uint64_t)mask);
  W_end();
}
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  W_begin(OP_glClearColor);
  W_f32(red);
  W_f32(green);
  W_f32(blue);
  W_f32(alpha);
  W_end();
}
void glClearDepthf(GLfloat d) {
  W_begin(OP_glClearDepthf);
  W_f32(d);
  W_end();
}
void glClearStencil(GLint s) {
  W_begin(OP_glClearStencil);
  W_u64((uint64_t)s);
  W_end();
}
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
  W_begin(OP_glColorMask);
  W_u64((uint64_t)red);
  W_u64((uint64_t)green);
  W_u64((uint64_t)blue);
  W_u64((uint64_t)alpha);
  W_end();
}
void glCompileShader(GLuint shader) {
  W_begin(OP_glCompileShader);
  W_u64((uint64_t)shader);
  W_end();
}
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
  W_begin(OP_glCopyTexImage2D);
  W_u64((uint64_t)target);
  W_u64((uint64_t)level);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_u64((uint64_t)border);
  W_end();
}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
  W_begin(OP_glCopyTexSubImage2D);
  W_u64((uint64_t)target);
  W_u64((uint64_t)level);
  W_u64((uint64_t)xoffset);
  W_u64((uint64_t)yoffset);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
GLuint glCreateProgram(void) {
  W_begin(OP_glCreateProgram);
  W_call();
  GLuint ret_ = (GLuint)R_u64();
  W_done();
  return ret_;
}
GLuint glCreateShader(GLenum type) {
  W_begin(OP_glCreateShader);
  W_u64((uint64_t)type);
  W_call();
  GLuint ret_ = (GLuint)R_u64();
  W_done();
  return ret_;
}
void glCullFace(GLenum mode) {
  W_begin(OP_glCullFace);
  W_u64((uint64_t)mode);
  W_end();
}
void glDeleteBuffers(GLsizei n, const GLuint * buffers) {
  H_delbuffers(n, buffers);
  W_begin(OP_glDeleteBuffers);
  W_u64((uint64_t)n);
  W_blob(buffers, SZ(n*4));
  W_end();
}
void glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers) {
  W_begin(OP_glDeleteFramebuffers);
  W_u64((uint64_t)n);
  W_blob(framebuffers, SZ(n*4));
  W_end();
}
void glDeleteProgram(GLuint program) {
  W_begin(OP_glDeleteProgram);
  W_u64((uint64_t)program);
  W_end();
}
void glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers) {
  W_begin(OP_glDeleteRenderbuffers);
  W_u64((uint64_t)n);
  W_blob(renderbuffers, SZ(n*4));
  W_end();
}
void glDeleteShader(GLuint shader) {
  W_begin(OP_glDeleteShader);
  W_u64((uint64_t)shader);
  W_end();
}
void glDeleteTextures(GLsizei n, const GLuint * textures) {
  W_begin(OP_glDeleteTextures);
  W_u64((uint64_t)n);
  W_blob(textures, SZ(n*4));
  W_end();
}
void glDepthFunc(GLenum func) {
  W_begin(OP_glDepthFunc);
  W_u64((uint64_t)func);
  W_end();
}
void glDepthMask(GLboolean flag) {
  W_begin(OP_glDepthMask);
  W_u64((uint64_t)flag);
  W_end();
}
void glDepthRangef(GLfloat n, GLfloat f) {
  W_begin(OP_glDepthRangef);
  W_f32(n);
  W_f32(f);
  W_end();
}
void glDetachShader(GLuint program, GLuint shader) {
  W_begin(OP_glDetachShader);
  W_u64((uint64_t)program);
  W_u64((uint64_t)shader);
  W_end();
}
void glDisable(GLenum cap) {
  W_begin(OP_glDisable);
  W_u64((uint64_t)cap);
  W_end();
}
void glDisableVertexAttribArray(GLuint index) {
  W_begin(OP_glDisableVertexAttribArray);
  W_u64((uint64_t)index);
  W_end();
}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
  W_begin(OP_glDrawArrays);
  W_u64((uint64_t)mode);
  W_u64((uint64_t)first);
  W_u64((uint64_t)count);
  W_end();
}
void glEnable(GLenum cap) {
  W_begin(OP_glEnable);
  W_u64((uint64_t)cap);
  W_end();
}
void glEnableVertexAttribArray(GLuint index) {
  W_begin(OP_glEnableVertexAttribArray);
  W_u64((uint64_t)index);
  W_end();
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
  W_begin(OP_glFramebufferRenderbuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)attachment);
  W_u64((uint64_t)renderbuffertarget);
  W_u64((uint64_t)renderbuffer);
  W_end();
}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
  W_begin(OP_glFramebufferTexture2D);
  W_u64((uint64_t)target);
  W_u64((uint64_t)attachment);
  W_u64((uint64_t)textarget);
  W_u64((uint64_t)texture);
  W_u64((uint64_t)level);
  W_end();
}
void glFrontFace(GLenum mode) {
  W_begin(OP_glFrontFace);
  W_u64((uint64_t)mode);
  W_end();
}
void glGenBuffers(GLsizei n, GLuint * buffers) {
  W_begin(OP_glGenBuffers);
  W_u64((uint64_t)n);
  W_call();
  R_copy(buffers, SZ(n*4));
  W_done();
}
void glGenerateMipmap(GLenum target) {
  W_begin(OP_glGenerateMipmap);
  W_u64((uint64_t)target);
  W_end();
}
void glGenFramebuffers(GLsizei n, GLuint * framebuffers) {
  W_begin(OP_glGenFramebuffers);
  W_u64((uint64_t)n);
  W_call();
  R_copy(framebuffers, SZ(n*4));
  W_done();
}
void glGenRenderbuffers(GLsizei n, GLuint * renderbuffers) {
  W_begin(OP_glGenRenderbuffers);
  W_u64((uint64_t)n);
  W_call();
  R_copy(renderbuffers, SZ(n*4));
  W_done();
}
void glGenTextures(GLsizei n, GLuint * textures) {
  W_begin(OP_glGenTextures);
  W_u64((uint64_t)n);
  W_call();
  R_copy(textures, SZ(n*4));
  W_done();
}
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name) {
  W_begin(OP_glGetActiveAttrib);
  W_u64((uint64_t)program);
  W_u64((uint64_t)index);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(size, SZ(4));
  R_copy(type, SZ(4));
  R_copy(name, SZ(bufSize));
  W_done();
}
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name) {
  W_begin(OP_glGetActiveUniform);
  W_u64((uint64_t)program);
  W_u64((uint64_t)index);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(size, SZ(4));
  R_copy(type, SZ(4));
  R_copy(name, SZ(bufSize));
  W_done();
}
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * shaders) {
  W_begin(OP_glGetAttachedShaders);
  W_u64((uint64_t)program);
  W_u64((uint64_t)maxCount);
  W_call();
  R_copy(count, SZ(4));
  R_copy(shaders, SZ(maxCount*4));
  W_done();
}
GLint glGetAttribLocation(GLuint program, const GLchar * name) {
  W_begin(OP_glGetAttribLocation);
  W_u64((uint64_t)program);
  W_str(name);
  W_call();
  GLint ret_ = (GLint)R_u64();
  W_done();
  return ret_;
}
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint * params) {
  W_begin(OP_glGetFramebufferAttachmentParameteriv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)attachment);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetProgramiv(GLuint program, GLenum pname, GLint * params) {
  W_begin(OP_glGetProgramiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
  W_begin(OP_glGetProgramInfoLog);
  W_u64((uint64_t)program);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(infoLog, SZ(bufSize));
  W_done();
}
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint * params) {
  W_begin(OP_glGetRenderbufferParameteriv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetShaderiv(GLuint shader, GLenum pname, GLint * params) {
  W_begin(OP_glGetShaderiv);
  W_u64((uint64_t)shader);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
  W_begin(OP_glGetShaderInfoLog);
  W_u64((uint64_t)shader);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(infoLog, SZ(bufSize));
  W_done();
}
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint * range, GLint * precision) {
  W_begin(OP_glGetShaderPrecisionFormat);
  W_u64((uint64_t)shadertype);
  W_u64((uint64_t)precisiontype);
  W_call();
  R_copy(range, SZ(8));
  R_copy(precision, SZ(4));
  W_done();
}
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source) {
  W_begin(OP_glGetShaderSource);
  W_u64((uint64_t)shader);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(source, SZ(bufSize));
  W_done();
}
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params) {
  W_begin(OP_glGetTexParameterfv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glGetTexParameteriv(GLenum target, GLenum pname, GLint * params) {
  W_begin(OP_glGetTexParameteriv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glGetUniformfv(GLuint program, GLint location, GLfloat * params) { static int once; unimpl("glGetUniformfv", &once); }
void glGetUniformiv(GLuint program, GLint location, GLint * params) { static int once; unimpl("glGetUniformiv", &once); }
GLint glGetUniformLocation(GLuint program, const GLchar * name) {
  W_begin(OP_glGetUniformLocation);
  W_u64((uint64_t)program);
  W_str(name);
  W_call();
  GLint ret_ = (GLint)R_u64();
  W_done();
  return ret_;
}
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat * params) {
  W_begin(OP_glGetVertexAttribfv);
  W_u64((uint64_t)index);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x8626?4:1)*4));
  W_done();
}
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint * params) {
  W_begin(OP_glGetVertexAttribiv);
  W_u64((uint64_t)index);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x8626?4:1)*4));
  W_done();
}
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void ** pointer) { static int once; unimpl("glGetVertexAttribPointerv", &once); }
void glHint(GLenum target, GLenum mode) {
  W_begin(OP_glHint);
  W_u64((uint64_t)target);
  W_u64((uint64_t)mode);
  W_end();
}
GLboolean glIsBuffer(GLuint buffer) {
  W_begin(OP_glIsBuffer);
  W_u64((uint64_t)buffer);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsEnabled(GLenum cap) {
  W_begin(OP_glIsEnabled);
  W_u64((uint64_t)cap);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsFramebuffer(GLuint framebuffer) {
  W_begin(OP_glIsFramebuffer);
  W_u64((uint64_t)framebuffer);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsProgram(GLuint program) {
  W_begin(OP_glIsProgram);
  W_u64((uint64_t)program);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsRenderbuffer(GLuint renderbuffer) {
  W_begin(OP_glIsRenderbuffer);
  W_u64((uint64_t)renderbuffer);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsShader(GLuint shader) {
  W_begin(OP_glIsShader);
  W_u64((uint64_t)shader);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsTexture(GLuint texture) {
  W_begin(OP_glIsTexture);
  W_u64((uint64_t)texture);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glLineWidth(GLfloat width) {
  W_begin(OP_glLineWidth);
  W_f32(width);
  W_end();
}
void glLinkProgram(GLuint program) {
  W_begin(OP_glLinkProgram);
  W_u64((uint64_t)program);
  W_end();
}
void glPixelStorei(GLenum pname, GLint param) {
  H_pixelstore(pname, param);
  W_begin(OP_glPixelStorei);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)param);
  W_end();
}
void glPolygonOffset(GLfloat factor, GLfloat units) {
  W_begin(OP_glPolygonOffset);
  W_f32(factor);
  W_f32(units);
  W_end();
}
void glReleaseShaderCompiler(void) {
  W_begin(OP_glReleaseShaderCompiler);
  W_end();
}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
  W_begin(OP_glRenderbufferStorage);
  W_u64((uint64_t)target);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glSampleCoverage(GLfloat value, GLboolean invert) {
  W_begin(OP_glSampleCoverage);
  W_f32(value);
  W_u64((uint64_t)invert);
  W_end();
}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
  W_begin(OP_glScissor);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glShaderBinary(GLsizei count, const GLuint * shaders, GLenum binaryformat, const void * binary, GLsizei length) { static int once; unimpl("glShaderBinary", &once); }
void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
  W_begin(OP_glStencilFunc);
  W_u64((uint64_t)func);
  W_u64((uint64_t)ref);
  W_u64((uint64_t)mask);
  W_end();
}
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
  W_begin(OP_glStencilFuncSeparate);
  W_u64((uint64_t)face);
  W_u64((uint64_t)func);
  W_u64((uint64_t)ref);
  W_u64((uint64_t)mask);
  W_end();
}
void glStencilMask(GLuint mask) {
  W_begin(OP_glStencilMask);
  W_u64((uint64_t)mask);
  W_end();
}
void glStencilMaskSeparate(GLenum face, GLuint mask) {
  W_begin(OP_glStencilMaskSeparate);
  W_u64((uint64_t)face);
  W_u64((uint64_t)mask);
  W_end();
}
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
  W_begin(OP_glStencilOp);
  W_u64((uint64_t)fail);
  W_u64((uint64_t)zfail);
  W_u64((uint64_t)zpass);
  W_end();
}
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
  W_begin(OP_glStencilOpSeparate);
  W_u64((uint64_t)face);
  W_u64((uint64_t)sfail);
  W_u64((uint64_t)dpfail);
  W_u64((uint64_t)dppass);
  W_end();
}
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
  W_begin(OP_glTexParameterf);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_f32(param);
  W_end();
}
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params) {
  W_begin(OP_glTexParameterfv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_blob(params, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
  W_begin(OP_glTexParameteri);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)param);
  W_end();
}
void glTexParameteriv(GLenum target, GLenum pname, const GLint * params) {
  W_begin(OP_glTexParameteriv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_blob(params, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glUniform1f(GLint location, GLfloat v0) {
  W_begin(OP_glUniform1f);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_end();
}
void glUniform1fv(GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glUniform1fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*1*4));
  W_end();
}
void glUniform1i(GLint location, GLint v0) {
  W_begin(OP_glUniform1i);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_end();
}
void glUniform1iv(GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glUniform1iv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*1*4));
  W_end();
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
  W_begin(OP_glUniform2f);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_f32(v1);
  W_end();
}
void glUniform2fv(GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glUniform2fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*2*4));
  W_end();
}
void glUniform2i(GLint location, GLint v0, GLint v1) {
  W_begin(OP_glUniform2i);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_end();
}
void glUniform2iv(GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glUniform2iv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*2*4));
  W_end();
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
  W_begin(OP_glUniform3f);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_f32(v1);
  W_f32(v2);
  W_end();
}
void glUniform3fv(GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glUniform3fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*3*4));
  W_end();
}
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
  W_begin(OP_glUniform3i);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_end();
}
void glUniform3iv(GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glUniform3iv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*3*4));
  W_end();
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
  W_begin(OP_glUniform4f);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_f32(v1);
  W_f32(v2);
  W_f32(v3);
  W_end();
}
void glUniform4fv(GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glUniform4fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
  W_begin(OP_glUniform4i);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_u64((uint64_t)v3);
  W_end();
}
void glUniform4iv(GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glUniform4iv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix2fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix3fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*9*4));
  W_end();
}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix4fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*16*4));
  W_end();
}
void glUseProgram(GLuint program) {
  W_begin(OP_glUseProgram);
  W_u64((uint64_t)program);
  W_end();
}
void glValidateProgram(GLuint program) {
  W_begin(OP_glValidateProgram);
  W_u64((uint64_t)program);
  W_end();
}
void glVertexAttrib1f(GLuint index, GLfloat x) {
  W_begin(OP_glVertexAttrib1f);
  W_u64((uint64_t)index);
  W_f32(x);
  W_end();
}
void glVertexAttrib1fv(GLuint index, const GLfloat * v) {
  W_begin(OP_glVertexAttrib1fv);
  W_u64((uint64_t)index);
  W_blob(v, SZ(1*4));
  W_end();
}
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
  W_begin(OP_glVertexAttrib2f);
  W_u64((uint64_t)index);
  W_f32(x);
  W_f32(y);
  W_end();
}
void glVertexAttrib2fv(GLuint index, const GLfloat * v) {
  W_begin(OP_glVertexAttrib2fv);
  W_u64((uint64_t)index);
  W_blob(v, SZ(2*4));
  W_end();
}
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
  W_begin(OP_glVertexAttrib3f);
  W_u64((uint64_t)index);
  W_f32(x);
  W_f32(y);
  W_f32(z);
  W_end();
}
void glVertexAttrib3fv(GLuint index, const GLfloat * v) {
  W_begin(OP_glVertexAttrib3fv);
  W_u64((uint64_t)index);
  W_blob(v, SZ(3*4));
  W_end();
}
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  W_begin(OP_glVertexAttrib4f);
  W_u64((uint64_t)index);
  W_f32(x);
  W_f32(y);
  W_f32(z);
  W_f32(w);
  W_end();
}
void glVertexAttrib4fv(GLuint index, const GLfloat * v) {
  W_begin(OP_glVertexAttrib4fv);
  W_u64((uint64_t)index);
  W_blob(v, SZ(4*4));
  W_end();
}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  W_begin(OP_glViewport);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glReadBuffer(GLenum src) {
  W_begin(OP_glReadBuffer);
  W_u64((uint64_t)src);
  W_end();
}
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
  W_begin(OP_glCopyTexSubImage3D);
  W_u64((uint64_t)target);
  W_u64((uint64_t)level);
  W_u64((uint64_t)xoffset);
  W_u64((uint64_t)yoffset);
  W_u64((uint64_t)zoffset);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glGenQueries(GLsizei n, GLuint * ids) {
  W_begin(OP_glGenQueries);
  W_u64((uint64_t)n);
  W_call();
  R_copy(ids, SZ(n*4));
  W_done();
}
void glDeleteQueries(GLsizei n, const GLuint * ids) {
  W_begin(OP_glDeleteQueries);
  W_u64((uint64_t)n);
  W_blob(ids, SZ(n*4));
  W_end();
}
GLboolean glIsQuery(GLuint id) {
  W_begin(OP_glIsQuery);
  W_u64((uint64_t)id);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glBeginQuery(GLenum target, GLuint id) {
  W_begin(OP_glBeginQuery);
  W_u64((uint64_t)target);
  W_u64((uint64_t)id);
  W_end();
}
void glEndQuery(GLenum target) {
  W_begin(OP_glEndQuery);
  W_u64((uint64_t)target);
  W_end();
}
void glGetQueryiv(GLenum target, GLenum pname, GLint * params) {
  W_begin(OP_glGetQueryiv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint * params) {
  W_begin(OP_glGetQueryObjectuiv);
  W_u64((uint64_t)id);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glDrawBuffers(GLsizei n, const GLenum * bufs) {
  W_begin(OP_glDrawBuffers);
  W_u64((uint64_t)n);
  W_blob(bufs, SZ(n*4));
  W_end();
}
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix2x3fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*6*4));
  W_end();
}
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix3x2fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*6*4));
  W_end();
}
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix2x4fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*8*4));
  W_end();
}
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix4x2fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*8*4));
  W_end();
}
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix3x4fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*12*4));
  W_end();
}
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glUniformMatrix4x3fv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*12*4));
  W_end();
}
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
  W_begin(OP_glBlitFramebuffer);
  W_u64((uint64_t)srcX0);
  W_u64((uint64_t)srcY0);
  W_u64((uint64_t)srcX1);
  W_u64((uint64_t)srcY1);
  W_u64((uint64_t)dstX0);
  W_u64((uint64_t)dstY0);
  W_u64((uint64_t)dstX1);
  W_u64((uint64_t)dstY1);
  W_u64((uint64_t)mask);
  W_u64((uint64_t)filter);
  W_end();
}
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
  W_begin(OP_glRenderbufferStorageMultisample);
  W_u64((uint64_t)target);
  W_u64((uint64_t)samples);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
  W_begin(OP_glFramebufferTextureLayer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)attachment);
  W_u64((uint64_t)texture);
  W_u64((uint64_t)level);
  W_u64((uint64_t)layer);
  W_end();
}
void glBindVertexArray(GLuint array) {
  H_bindvao(array);
  W_begin(OP_glBindVertexArray);
  W_u64((uint64_t)array);
  W_end();
}
void glDeleteVertexArrays(GLsizei n, const GLuint * arrays) {
  H_delvaos(n, arrays);
  W_begin(OP_glDeleteVertexArrays);
  W_u64((uint64_t)n);
  W_blob(arrays, SZ(n*4));
  W_end();
}
void glGenVertexArrays(GLsizei n, GLuint * arrays) {
  W_begin(OP_glGenVertexArrays);
  W_u64((uint64_t)n);
  W_call();
  R_copy(arrays, SZ(n*4));
  W_done();
}
GLboolean glIsVertexArray(GLuint array) {
  W_begin(OP_glIsVertexArray);
  W_u64((uint64_t)array);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glGetIntegeri_v(GLenum target, GLuint index, GLint * data) {
  W_begin(OP_glGetIntegeri_v);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_call();
  R_copy(data, SZ(4));
  W_done();
}
void glBeginTransformFeedback(GLenum primitiveMode) {
  W_begin(OP_glBeginTransformFeedback);
  W_u64((uint64_t)primitiveMode);
  W_end();
}
void glEndTransformFeedback(void) {
  W_begin(OP_glEndTransformFeedback);
  W_end();
}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
  H_bindbuffer(target, buffer);
  W_begin(OP_glBindBufferRange);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)offset);
  W_u64((uint64_t)size);
  W_end();
}
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
  H_bindbuffer(target, buffer);
  W_begin(OP_glBindBufferBase);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_u64((uint64_t)buffer);
  W_end();
}
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name) {
  W_begin(OP_glGetTransformFeedbackVarying);
  W_u64((uint64_t)program);
  W_u64((uint64_t)index);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(size, SZ(4));
  R_copy(type, SZ(4));
  R_copy(name, SZ(bufSize));
  W_done();
}
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint * params) {
  W_begin(OP_glGetVertexAttribIiv);
  W_u64((uint64_t)index);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x8626?4:1)*4));
  W_done();
}
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint * params) {
  W_begin(OP_glGetVertexAttribIuiv);
  W_u64((uint64_t)index);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x8626?4:1)*4));
  W_done();
}
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
  W_begin(OP_glVertexAttribI4i);
  W_u64((uint64_t)index);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)z);
  W_u64((uint64_t)w);
  W_end();
}
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
  W_begin(OP_glVertexAttribI4ui);
  W_u64((uint64_t)index);
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)z);
  W_u64((uint64_t)w);
  W_end();
}
void glVertexAttribI4iv(GLuint index, const GLint * v) {
  W_begin(OP_glVertexAttribI4iv);
  W_u64((uint64_t)index);
  W_blob(v, SZ(16));
  W_end();
}
void glVertexAttribI4uiv(GLuint index, const GLuint * v) {
  W_begin(OP_glVertexAttribI4uiv);
  W_u64((uint64_t)index);
  W_blob(v, SZ(16));
  W_end();
}
void glGetUniformuiv(GLuint program, GLint location, GLuint * params) { static int once; unimpl("glGetUniformuiv", &once); }
GLint glGetFragDataLocation(GLuint program, const GLchar * name) {
  W_begin(OP_glGetFragDataLocation);
  W_u64((uint64_t)program);
  W_str(name);
  W_call();
  GLint ret_ = (GLint)R_u64();
  W_done();
  return ret_;
}
void glUniform1ui(GLint location, GLuint v0) {
  W_begin(OP_glUniform1ui);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_end();
}
void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
  W_begin(OP_glUniform2ui);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_end();
}
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
  W_begin(OP_glUniform3ui);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_end();
}
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
  W_begin(OP_glUniform4ui);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_u64((uint64_t)v3);
  W_end();
}
void glUniform1uiv(GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glUniform1uiv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*1*4));
  W_end();
}
void glUniform2uiv(GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glUniform2uiv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*2*4));
  W_end();
}
void glUniform3uiv(GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glUniform3uiv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*3*4));
  W_end();
}
void glUniform4uiv(GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glUniform4uiv);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value) {
  W_begin(OP_glClearBufferiv);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)drawbuffer);
  W_blob(value, SZ((buffer==0x1800?4:1)*4));
  W_end();
}
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value) {
  W_begin(OP_glClearBufferuiv);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)drawbuffer);
  W_blob(value, SZ((buffer==0x1800?4:1)*4));
  W_end();
}
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value) {
  W_begin(OP_glClearBufferfv);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)drawbuffer);
  W_blob(value, SZ((buffer==0x1800?4:1)*4));
  W_end();
}
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
  W_begin(OP_glClearBufferfi);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)drawbuffer);
  W_f32(depth);
  W_u64((uint64_t)stencil);
  W_end();
}
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
  W_begin(OP_glCopyBufferSubData);
  W_u64((uint64_t)readTarget);
  W_u64((uint64_t)writeTarget);
  W_u64((uint64_t)readOffset);
  W_u64((uint64_t)writeOffset);
  W_u64((uint64_t)size);
  W_end();
}
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params) {
  W_begin(OP_glGetActiveUniformsiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)uniformCount);
  W_blob(uniformIndices, SZ(uniformCount*4));
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(uniformCount*4));
  W_done();
}
GLuint glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName) {
  W_begin(OP_glGetUniformBlockIndex);
  W_u64((uint64_t)program);
  W_str(uniformBlockName);
  W_call();
  GLuint ret_ = (GLuint)R_u64();
  W_done();
  return ret_;
}
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName) {
  W_begin(OP_glGetActiveUniformBlockName);
  W_u64((uint64_t)program);
  W_u64((uint64_t)uniformBlockIndex);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(uniformBlockName, SZ(bufSize));
  W_done();
}
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
  W_begin(OP_glUniformBlockBinding);
  W_u64((uint64_t)program);
  W_u64((uint64_t)uniformBlockIndex);
  W_u64((uint64_t)uniformBlockBinding);
  W_end();
}
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
  W_begin(OP_glDrawArraysInstanced);
  W_u64((uint64_t)mode);
  W_u64((uint64_t)first);
  W_u64((uint64_t)count);
  W_u64((uint64_t)instancecount);
  W_end();
}
GLsync glFenceSync(GLenum condition, GLbitfield flags) {
  W_begin(OP_glFenceSync);
  W_u64((uint64_t)condition);
  W_u64((uint64_t)flags);
  W_call();
  GLsync ret_ = (GLsync)(uintptr_t)R_u64();
  W_done();
  return ret_;
}
GLboolean glIsSync(GLsync sync) {
  W_begin(OP_glIsSync);
  W_u64((uint64_t)(uintptr_t)sync);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glDeleteSync(GLsync sync) {
  W_begin(OP_glDeleteSync);
  W_u64((uint64_t)(uintptr_t)sync);
  W_end();
}
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
  W_begin(OP_glClientWaitSync);
  W_u64((uint64_t)(uintptr_t)sync);
  W_u64((uint64_t)flags);
  W_u64((uint64_t)timeout);
  W_call();
  GLenum ret_ = (GLenum)R_u64();
  W_done();
  return ret_;
}
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
  W_begin(OP_glWaitSync);
  W_u64((uint64_t)(uintptr_t)sync);
  W_u64((uint64_t)flags);
  W_u64((uint64_t)timeout);
  W_end();
}
void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei * length, GLint * values) {
  W_begin(OP_glGetSynciv);
  W_u64((uint64_t)(uintptr_t)sync);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(values, SZ(bufSize*4));
  W_done();
}
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 * data) {
  W_begin(OP_glGetInteger64i_v);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_call();
  R_copy(data, SZ(8));
  W_done();
}
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 * params) {
  W_begin(OP_glGetBufferParameteri64v);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(8));
  W_done();
}
void glGenSamplers(GLsizei count, GLuint * samplers) {
  W_begin(OP_glGenSamplers);
  W_u64((uint64_t)count);
  W_call();
  R_copy(samplers, SZ(count*4));
  W_done();
}
void glDeleteSamplers(GLsizei count, const GLuint * samplers) {
  W_begin(OP_glDeleteSamplers);
  W_u64((uint64_t)count);
  W_blob(samplers, SZ(count*4));
  W_end();
}
GLboolean glIsSampler(GLuint sampler) {
  W_begin(OP_glIsSampler);
  W_u64((uint64_t)sampler);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glBindSampler(GLuint unit, GLuint sampler) {
  W_begin(OP_glBindSampler);
  W_u64((uint64_t)unit);
  W_u64((uint64_t)sampler);
  W_end();
}
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
  W_begin(OP_glSamplerParameteri);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)param);
  W_end();
}
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param) {
  W_begin(OP_glSamplerParameteriv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_blob(param, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
  W_begin(OP_glSamplerParameterf);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_f32(param);
  W_end();
}
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param) {
  W_begin(OP_glSamplerParameterfv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_blob(param, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint * params) {
  W_begin(OP_glGetSamplerParameteriv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat * params) {
  W_begin(OP_glGetSamplerParameterfv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glVertexAttribDivisor(GLuint index, GLuint divisor) {
  W_begin(OP_glVertexAttribDivisor);
  W_u64((uint64_t)index);
  W_u64((uint64_t)divisor);
  W_end();
}
void glBindTransformFeedback(GLenum target, GLuint id) {
  W_begin(OP_glBindTransformFeedback);
  W_u64((uint64_t)target);
  W_u64((uint64_t)id);
  W_end();
}
void glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids) {
  W_begin(OP_glDeleteTransformFeedbacks);
  W_u64((uint64_t)n);
  W_blob(ids, SZ(n*4));
  W_end();
}
void glGenTransformFeedbacks(GLsizei n, GLuint * ids) {
  W_begin(OP_glGenTransformFeedbacks);
  W_u64((uint64_t)n);
  W_call();
  R_copy(ids, SZ(n*4));
  W_done();
}
GLboolean glIsTransformFeedback(GLuint id) {
  W_begin(OP_glIsTransformFeedback);
  W_u64((uint64_t)id);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glPauseTransformFeedback(void) {
  W_begin(OP_glPauseTransformFeedback);
  W_end();
}
void glResumeTransformFeedback(void) {
  W_begin(OP_glResumeTransformFeedback);
  W_end();
}
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei * length, GLenum * binaryFormat, void * binary) {
  W_begin(OP_glGetProgramBinary);
  W_u64((uint64_t)program);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(binaryFormat, SZ(4));
  R_copy(binary, SZ(bufSize));
  W_done();
}
void glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length) {
  W_begin(OP_glProgramBinary);
  W_u64((uint64_t)program);
  W_u64((uint64_t)binaryFormat);
  W_blob(binary, SZ(length));
  W_u64((uint64_t)length);
  W_end();
}
void glProgramParameteri(GLuint program, GLenum pname, GLint value) {
  W_begin(OP_glProgramParameteri);
  W_u64((uint64_t)program);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)value);
  W_end();
}
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments) {
  W_begin(OP_glInvalidateFramebuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)numAttachments);
  W_blob(attachments, SZ(numAttachments*4));
  W_end();
}
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
  W_begin(OP_glInvalidateSubFramebuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)numAttachments);
  W_blob(attachments, SZ(numAttachments*4));
  W_u64((uint64_t)x);
  W_u64((uint64_t)y);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
  W_begin(OP_glTexStorage2D);
  W_u64((uint64_t)target);
  W_u64((uint64_t)levels);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_end();
}
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
  W_begin(OP_glTexStorage3D);
  W_u64((uint64_t)target);
  W_u64((uint64_t)levels);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_u64((uint64_t)depth);
  W_end();
}
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint * params) {
  W_begin(OP_glGetInternalformativ);
  W_u64((uint64_t)target);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(params, SZ(bufSize*4));
  W_done();
}
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
  W_begin(OP_glDispatchCompute);
  W_u64((uint64_t)num_groups_x);
  W_u64((uint64_t)num_groups_y);
  W_u64((uint64_t)num_groups_z);
  W_end();
}
void glDispatchComputeIndirect(GLintptr indirect) {
  W_begin(OP_glDispatchComputeIndirect);
  W_u64((uint64_t)indirect);
  W_end();
}
void glDrawArraysIndirect(GLenum mode, const void * indirect) {
  W_begin(OP_glDrawArraysIndirect);
  W_u64((uint64_t)mode);
  W_u64((uint64_t)(uintptr_t)indirect);
  W_end();
}
void glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect) {
  W_begin(OP_glDrawElementsIndirect);
  W_u64((uint64_t)mode);
  W_u64((uint64_t)type);
  W_u64((uint64_t)(uintptr_t)indirect);
  W_end();
}
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {
  W_begin(OP_glFramebufferParameteri);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)param);
  W_end();
}
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint * params) {
  W_begin(OP_glGetFramebufferParameteriv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint * params) {
  W_begin(OP_glGetProgramInterfaceiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)programInterface);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name) {
  W_begin(OP_glGetProgramResourceIndex);
  W_u64((uint64_t)program);
  W_u64((uint64_t)programInterface);
  W_str(name);
  W_call();
  GLuint ret_ = (GLuint)R_u64();
  W_done();
  return ret_;
}
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, GLchar * name) {
  W_begin(OP_glGetProgramResourceName);
  W_u64((uint64_t)program);
  W_u64((uint64_t)programInterface);
  W_u64((uint64_t)index);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(name, SZ(bufSize));
  W_done();
}
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei * length, GLint * params) {
  W_begin(OP_glGetProgramResourceiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)programInterface);
  W_u64((uint64_t)index);
  W_u64((uint64_t)propCount);
  W_blob(props, SZ(propCount*4));
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(params, SZ(bufSize*4));
  W_done();
}
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name) {
  W_begin(OP_glGetProgramResourceLocation);
  W_u64((uint64_t)program);
  W_u64((uint64_t)programInterface);
  W_str(name);
  W_call();
  GLint ret_ = (GLint)R_u64();
  W_done();
  return ret_;
}
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {
  W_begin(OP_glUseProgramStages);
  W_u64((uint64_t)pipeline);
  W_u64((uint64_t)stages);
  W_u64((uint64_t)program);
  W_end();
}
void glActiveShaderProgram(GLuint pipeline, GLuint program) {
  W_begin(OP_glActiveShaderProgram);
  W_u64((uint64_t)pipeline);
  W_u64((uint64_t)program);
  W_end();
}
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings) { static int once; unimpl("glCreateShaderProgramv", &once); return 0; }
void glBindProgramPipeline(GLuint pipeline) {
  W_begin(OP_glBindProgramPipeline);
  W_u64((uint64_t)pipeline);
  W_end();
}
void glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines) {
  W_begin(OP_glDeleteProgramPipelines);
  W_u64((uint64_t)n);
  W_blob(pipelines, SZ(n*4));
  W_end();
}
void glGenProgramPipelines(GLsizei n, GLuint * pipelines) {
  W_begin(OP_glGenProgramPipelines);
  W_u64((uint64_t)n);
  W_call();
  R_copy(pipelines, SZ(n*4));
  W_done();
}
GLboolean glIsProgramPipeline(GLuint pipeline) {
  W_begin(OP_glIsProgramPipeline);
  W_u64((uint64_t)pipeline);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint * params) {
  W_begin(OP_glGetProgramPipelineiv);
  W_u64((uint64_t)pipeline);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
  W_begin(OP_glProgramUniform1i);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_end();
}
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
  W_begin(OP_glProgramUniform2i);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_end();
}
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
  W_begin(OP_glProgramUniform3i);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_end();
}
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
  W_begin(OP_glProgramUniform4i);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_u64((uint64_t)v3);
  W_end();
}
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {
  W_begin(OP_glProgramUniform1ui);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_end();
}
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {
  W_begin(OP_glProgramUniform2ui);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_end();
}
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
  W_begin(OP_glProgramUniform3ui);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_end();
}
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
  W_begin(OP_glProgramUniform4ui);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)v0);
  W_u64((uint64_t)v1);
  W_u64((uint64_t)v2);
  W_u64((uint64_t)v3);
  W_end();
}
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
  W_begin(OP_glProgramUniform1f);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_end();
}
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
  W_begin(OP_glProgramUniform2f);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_f32(v1);
  W_end();
}
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
  W_begin(OP_glProgramUniform3f);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_f32(v1);
  W_f32(v2);
  W_end();
}
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
  W_begin(OP_glProgramUniform4f);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_f32(v0);
  W_f32(v1);
  W_f32(v2);
  W_f32(v3);
  W_end();
}
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glProgramUniform1iv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*1*4));
  W_end();
}
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glProgramUniform2iv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*2*4));
  W_end();
}
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glProgramUniform3iv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*3*4));
  W_end();
}
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value) {
  W_begin(OP_glProgramUniform4iv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glProgramUniform1uiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*1*4));
  W_end();
}
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glProgramUniform2uiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*2*4));
  W_end();
}
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glProgramUniform3uiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*3*4));
  W_end();
}
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) {
  W_begin(OP_glProgramUniform4uiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glProgramUniform1fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*1*4));
  W_end();
}
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glProgramUniform2fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*2*4));
  W_end();
}
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glProgramUniform3fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*3*4));
  W_end();
}
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) {
  W_begin(OP_glProgramUniform4fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix2fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*4*4));
  W_end();
}
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix3fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*9*4));
  W_end();
}
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix4fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*16*4));
  W_end();
}
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix2x3fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*6*4));
  W_end();
}
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix3x2fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*6*4));
  W_end();
}
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix2x4fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*8*4));
  W_end();
}
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix4x2fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*8*4));
  W_end();
}
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix3x4fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*12*4));
  W_end();
}
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
  W_begin(OP_glProgramUniformMatrix4x3fv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)count);
  W_u64((uint64_t)transpose);
  W_blob(value, SZ(count*12*4));
  W_end();
}
void glValidateProgramPipeline(GLuint pipeline) {
  W_begin(OP_glValidateProgramPipeline);
  W_u64((uint64_t)pipeline);
  W_end();
}
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei * length, GLchar * infoLog) {
  W_begin(OP_glGetProgramPipelineInfoLog);
  W_u64((uint64_t)pipeline);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(length, SZ(4));
  R_copy(infoLog, SZ(bufSize));
  W_done();
}
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
  W_begin(OP_glBindImageTexture);
  W_u64((uint64_t)unit);
  W_u64((uint64_t)texture);
  W_u64((uint64_t)level);
  W_u64((uint64_t)layered);
  W_u64((uint64_t)layer);
  W_u64((uint64_t)access);
  W_u64((uint64_t)format);
  W_end();
}
void glGetBooleani_v(GLenum target, GLuint index, GLboolean * data) {
  W_begin(OP_glGetBooleani_v);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_call();
  R_copy(data, SZ(1));
  W_done();
}
void glMemoryBarrier(GLbitfield barriers) {
  W_begin(OP_glMemoryBarrier);
  W_u64((uint64_t)barriers);
  W_end();
}
void glMemoryBarrierByRegion(GLbitfield barriers) {
  W_begin(OP_glMemoryBarrierByRegion);
  W_u64((uint64_t)barriers);
  W_end();
}
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
  W_begin(OP_glTexStorage2DMultisample);
  W_u64((uint64_t)target);
  W_u64((uint64_t)samples);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_u64((uint64_t)fixedsamplelocations);
  W_end();
}
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat * val) {
  W_begin(OP_glGetMultisamplefv);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)index);
  W_call();
  R_copy(val, SZ(8));
  W_done();
}
void glSampleMaski(GLuint maskNumber, GLbitfield mask) {
  W_begin(OP_glSampleMaski);
  W_u64((uint64_t)maskNumber);
  W_u64((uint64_t)mask);
  W_end();
}
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params) {
  W_begin(OP_glGetTexLevelParameteriv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)level);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params) {
  W_begin(OP_glGetTexLevelParameterfv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)level);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ(4));
  W_done();
}
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
  W_begin(OP_glBindVertexBuffer);
  W_u64((uint64_t)bindingindex);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)offset);
  W_u64((uint64_t)stride);
  W_end();
}
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
  W_begin(OP_glVertexAttribFormat);
  W_u64((uint64_t)attribindex);
  W_u64((uint64_t)size);
  W_u64((uint64_t)type);
  W_u64((uint64_t)normalized);
  W_u64((uint64_t)relativeoffset);
  W_end();
}
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
  W_begin(OP_glVertexAttribIFormat);
  W_u64((uint64_t)attribindex);
  W_u64((uint64_t)size);
  W_u64((uint64_t)type);
  W_u64((uint64_t)relativeoffset);
  W_end();
}
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
  W_begin(OP_glVertexAttribBinding);
  W_u64((uint64_t)attribindex);
  W_u64((uint64_t)bindingindex);
  W_end();
}
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
  W_begin(OP_glVertexBindingDivisor);
  W_u64((uint64_t)bindingindex);
  W_u64((uint64_t)divisor);
  W_end();
}
void glBlendBarrier(void) {
  W_begin(OP_glBlendBarrier);
  W_end();
}
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
  W_begin(OP_glCopyImageSubData);
  W_u64((uint64_t)srcName);
  W_u64((uint64_t)srcTarget);
  W_u64((uint64_t)srcLevel);
  W_u64((uint64_t)srcX);
  W_u64((uint64_t)srcY);
  W_u64((uint64_t)srcZ);
  W_u64((uint64_t)dstName);
  W_u64((uint64_t)dstTarget);
  W_u64((uint64_t)dstLevel);
  W_u64((uint64_t)dstX);
  W_u64((uint64_t)dstY);
  W_u64((uint64_t)dstZ);
  W_u64((uint64_t)srcWidth);
  W_u64((uint64_t)srcHeight);
  W_u64((uint64_t)srcDepth);
  W_end();
}
void glEnablei(GLenum target, GLuint index) {
  W_begin(OP_glEnablei);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_end();
}
void glDisablei(GLenum target, GLuint index) {
  W_begin(OP_glDisablei);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_end();
}
void glBlendEquationi(GLuint buf, GLenum mode) {
  W_begin(OP_glBlendEquationi);
  W_u64((uint64_t)buf);
  W_u64((uint64_t)mode);
  W_end();
}
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
  W_begin(OP_glBlendEquationSeparatei);
  W_u64((uint64_t)buf);
  W_u64((uint64_t)modeRGB);
  W_u64((uint64_t)modeAlpha);
  W_end();
}
void glBlendFunci(GLuint buf, GLenum src, GLenum dst) {
  W_begin(OP_glBlendFunci);
  W_u64((uint64_t)buf);
  W_u64((uint64_t)src);
  W_u64((uint64_t)dst);
  W_end();
}
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
  W_begin(OP_glBlendFuncSeparatei);
  W_u64((uint64_t)buf);
  W_u64((uint64_t)srcRGB);
  W_u64((uint64_t)dstRGB);
  W_u64((uint64_t)srcAlpha);
  W_u64((uint64_t)dstAlpha);
  W_end();
}
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
  W_begin(OP_glColorMaski);
  W_u64((uint64_t)index);
  W_u64((uint64_t)r);
  W_u64((uint64_t)g);
  W_u64((uint64_t)b);
  W_u64((uint64_t)a);
  W_end();
}
GLboolean glIsEnabledi(GLenum target, GLuint index) {
  W_begin(OP_glIsEnabledi);
  W_u64((uint64_t)target);
  W_u64((uint64_t)index);
  W_call();
  GLboolean ret_ = (GLboolean)R_u64();
  W_done();
  return ret_;
}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {
  W_begin(OP_glFramebufferTexture);
  W_u64((uint64_t)target);
  W_u64((uint64_t)attachment);
  W_u64((uint64_t)texture);
  W_u64((uint64_t)level);
  W_end();
}
void glPrimitiveBoundingBox(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW) {
  W_begin(OP_glPrimitiveBoundingBox);
  W_f32(minX);
  W_f32(minY);
  W_f32(minZ);
  W_f32(minW);
  W_f32(maxX);
  W_f32(maxY);
  W_f32(maxZ);
  W_f32(maxW);
  W_end();
}
GLenum glGetGraphicsResetStatus(void) {
  W_begin(OP_glGetGraphicsResetStatus);
  W_call();
  GLenum ret_ = (GLenum)R_u64();
  W_done();
  return ret_;
}
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat * params) {
  W_begin(OP_glGetnUniformfv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(params, SZ(bufSize));
  W_done();
}
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint * params) {
  W_begin(OP_glGetnUniformiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(params, SZ(bufSize));
  W_done();
}
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint * params) {
  W_begin(OP_glGetnUniformuiv);
  W_u64((uint64_t)program);
  W_u64((uint64_t)location);
  W_u64((uint64_t)bufSize);
  W_call();
  R_copy(params, SZ(bufSize));
  W_done();
}
void glMinSampleShading(GLfloat value) {
  W_begin(OP_glMinSampleShading);
  W_f32(value);
  W_end();
}
void glPatchParameteri(GLenum pname, GLint value) {
  W_begin(OP_glPatchParameteri);
  W_u64((uint64_t)pname);
  W_u64((uint64_t)value);
  W_end();
}
void glTexParameterIiv(GLenum target, GLenum pname, const GLint * params) {
  W_begin(OP_glTexParameterIiv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_blob(params, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint * params) {
  W_begin(OP_glTexParameterIuiv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_blob(params, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint * params) {
  W_begin(OP_glGetTexParameterIiv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint * params) {
  W_begin(OP_glGetTexParameterIuiv);
  W_u64((uint64_t)target);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint * param) {
  W_begin(OP_glSamplerParameterIiv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_blob(param, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint * param) {
  W_begin(OP_glSamplerParameterIuiv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_blob(param, SZ((pname==0x1004?4:1)*4));
  W_end();
}
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint * params) {
  W_begin(OP_glGetSamplerParameterIiv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint * params) {
  W_begin(OP_glGetSamplerParameterIuiv);
  W_u64((uint64_t)sampler);
  W_u64((uint64_t)pname);
  W_call();
  R_copy(params, SZ((pname==0x1004?4:1)*4));
  W_done();
}
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {
  W_begin(OP_glTexBuffer);
  W_u64((uint64_t)target);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)buffer);
  W_end();
}
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
  W_begin(OP_glTexBufferRange);
  W_u64((uint64_t)target);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)buffer);
  W_u64((uint64_t)offset);
  W_u64((uint64_t)size);
  W_end();
}
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
  W_begin(OP_glTexStorage3DMultisample);
  W_u64((uint64_t)target);
  W_u64((uint64_t)samples);
  W_u64((uint64_t)internalformat);
  W_u64((uint64_t)width);
  W_u64((uint64_t)height);
  W_u64((uint64_t)depth);
  W_u64((uint64_t)fixedsamplelocations);
  W_end();
}
