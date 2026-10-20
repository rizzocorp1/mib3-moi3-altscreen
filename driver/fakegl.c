/* fakegl.c — stub de EGL/GLESv3 p/ o sandbox (estudo): faz o loop de render da HMI
 * EXECUTAR sem GPU/compositor (desenha pro nada). Nao gera imagem real.
 * Gerado por gen_fakegl.py a partir dos simbolos que libjopengl_es3 importa do Adreno. */
typedef unsigned long u64;
static int G_DPY,G_CFG,G_CTX,G_SURF,G_SYNC,G_IMG; static unsigned G_ID=0;
static char G_SCRATCH[1<<20];
static long noop_ret0(void){return 0;}

int eglBindAPI(unsigned a){(void)a;return 1;}
int eglBindTexImage(void* d,void* s,int b){(void)d;(void)s;(void)b;return 1;}
int eglChooseConfig(void* d,const int* a,void** c,int n,int* num){(void)d;(void)a;if(c&&n>0)c[0]=&G_CFG;if(num)*num=1;return 1;}
int eglClientWaitSync(void* d,void* s,int f,unsigned long t){(void)d;(void)s;(void)f;(void)t;return 0x30F6;}
int eglCopyBuffers(void* d,void* s,void* t){(void)d;(void)s;(void)t;return 1;}
void* eglCreateContext(void* d,void* c,void* s,const int* a){(void)d;(void)c;(void)s;(void)a;return &G_CTX;}
void* eglCreateImage(void* d,void* c,unsigned t,void* b,const int* a){(void)d;(void)c;(void)t;(void)b;(void)a;return &G_IMG;}
void* eglCreatePbufferFromClientBuffer(void* d,unsigned t,void* b,void* c,const int* a){(void)d;(void)t;(void)b;(void)c;(void)a;return &G_SURF;}
void* eglCreatePbufferSurface(void* d,void* c,const int* a){(void)d;(void)c;(void)a;return &G_SURF;}
void* eglCreatePixmapSurface(void* d,void* c,void* p,const int* a){(void)d;(void)c;(void)p;(void)a;return &G_SURF;}
void* eglCreatePlatformPixmapSurface(void* d,void* c,void* p,const void* a){(void)d;(void)c;(void)p;(void)a;return &G_SURF;}
void* eglCreatePlatformWindowSurface(void* d,void* c,void* w,const void* a){(void)d;(void)c;(void)w;(void)a;return &G_SURF;}
void* eglCreateSync(void* d,unsigned t,const int* a){(void)d;(void)t;(void)a;return &G_SYNC;}
void* eglCreateWindowSurface(void* d,void* c,void* w,const int* a){(void)d;(void)c;(void)w;(void)a;return &G_SURF;}
int eglDestroyContext(void* d,void* c){(void)d;(void)c;return 1;}
int eglDestroyImage(void* d,void* i){(void)d;(void)i;return 1;}
int eglDestroySurface(void* d,void* s){(void)d;(void)s;return 1;}
int eglDestroySync(void* d,void* s){(void)d;(void)s;return 1;}
int eglGetConfigAttrib(void* d,void* c,int a,int* v){(void)d;(void)c;(void)a;if(v)*v=8;return 1;}
int eglGetConfigs(void* d,void** c,int n,int* num){(void)d;if(c&&n>0)c[0]=&G_CFG;if(num)*num=1;return 1;}
void* eglGetCurrentContext(void){return &G_CTX;}
void* eglGetCurrentDisplay(void){return &G_DPY;}
void* eglGetCurrentSurface(int r){(void)r;return &G_SURF;}
void* eglGetDisplay(void* d){(void)d;return &G_DPY;}
int eglGetError(void){return 0x3000;}
void* eglGetPlatformDisplay(unsigned p,void* n,const void* a){(void)p;(void)n;(void)a;return &G_DPY;}
int eglGetSyncAttrib(void* d,void* s,int a,int* v){(void)d;(void)s;(void)a;if(v)*v=0;return 1;}
int eglInitialize(void* d,int* ma,int* mi){(void)d;if(ma)*ma=1;if(mi)*mi=5;return 1;}
int eglMakeCurrent(void* d,void* dr,void* rd,void* c){(void)d;(void)dr;(void)rd;(void)c;return 1;}
unsigned eglQueryAPI(void){return 0x30A0;}
int eglQueryContext(void* d,void* c,int a,int* v){(void)d;(void)c;(void)a;if(v)*v=0;return 1;}
const char* eglQueryString(void* d,int n){(void)d;(void)n;return "stub";}
int eglQuerySurface(void* d,void* s,int a,int* v){(void)d;(void)s;(void)a;if(v)*v=0;return 1;}
int eglReleaseTexImage(void* d,void* s,int b){(void)d;(void)s;(void)b;return 1;}
int eglReleaseThread(void){return 1;}
int eglSurfaceAttrib(void* d,void* s,int a,int v){(void)d;(void)s;(void)a;(void)v;return 1;}
int eglSwapBuffers(void* d,void* s){(void)d;(void)s;return 1;}
int eglSwapInterval(void* d,int i){(void)d;(void)i;return 1;}
int eglTerminate(void* d){(void)d;return 1;}
int eglWaitClient(void){return 1;}
int eglWaitGL(void){return 1;}
int eglWaitNative(int e){(void)e;return 1;}
int eglWaitSync(void* d,void* s,int f){(void)d;(void)s;(void)f;return 1;}
unsigned glCheckFramebufferStatus(unsigned t){(void)t;return 0x8CD5;}
unsigned glClientWaitSync(void* s,unsigned f,unsigned long t){(void)s;(void)f;(void)t;return 0x911A;}
unsigned glCreateProgram(void){return ++G_ID;}
unsigned glCreateShader(unsigned t){(void)t;return ++G_ID;}
void* glFenceSync(unsigned c,unsigned f){(void)c;(void)f;return &G_SYNC;}
void glGenBuffers(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenFramebuffers(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenProgramPipelines(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenQueries(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenRenderbuffers(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenSamplers(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenTextures(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenTransformFeedbacks(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
void glGenVertexArrays(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}
int glGetAttribLocation(unsigned p,const char* n){(void)p;(void)n;return 0;}
void glGetBooleanv(unsigned p,unsigned char* v){(void)p;if(v)*v=0;}
unsigned glGetError(void){return 0;}
void glGetFloatv(unsigned p,float* v){(void)p;if(v)*v=0.0f;}
void glGetInteger64v(unsigned p,long* v){(void)p;if(v)*v=0;}
void glGetIntegerv(unsigned p,int* v){if(!v)return;switch(p){case 0x0D33:*v=8192;break;case 0x8B4D:*v=32;break;case 0x8872:*v=32;break;case 0x821D:*v=0;break;case 0x8CDF:*v=8;break;default:*v=0;}}
void glGetProgramInfoLog(unsigned s,int m,int* l,char* b){(void)s;(void)m;if(l)*l=0;if(b&&m>0)b[0]=0;}
void glGetProgramiv(unsigned s,unsigned p,int* v){(void)s;(void)p;if(v)*v=1;}
void glGetShaderInfoLog(unsigned s,int m,int* l,char* b){(void)s;(void)m;if(l)*l=0;if(b&&m>0)b[0]=0;}
void glGetShaderiv(unsigned s,unsigned p,int* v){(void)s;(void)p;if(v)*v=1;}
const unsigned char* glGetString(unsigned n){switch(n){case 0x1F00:return (const unsigned char*)"stub-vendor";case 0x1F01:return (const unsigned char*)"stub-renderer";case 0x1F02:return (const unsigned char*)"OpenGL ES 3.2 stub";case 0x8B8C:return (const unsigned char*)"OpenGL ES GLSL ES 3.20";case 0x1F03:return (const unsigned char*)"GL_OES_rgb8_rgba8 GL_OES_texture_npot GL_OES_vertex_array_object";default:return (const unsigned char*)"stub";}}
const unsigned char* glGetStringi(unsigned n,unsigned i){(void)n;(void)i;return (const unsigned char*)"";}
unsigned glGetUniformBlockIndex(unsigned p,const char* n){(void)p;(void)n;return 0;}
int glGetUniformLocation(unsigned p,const char* n){(void)p;(void)n;return 0;}
unsigned char glIsEnabled(unsigned c){(void)c;return 0;}
void* glMapBufferRange(unsigned t,long o,long l,unsigned a){(void)t;(void)o;(void)a;return (l>0&&l<(long)sizeof G_SCRATCH)?G_SCRATCH:G_SCRATCH;}
long glActiveShaderProgram(){return 0;}
long glActiveTexture(){return 0;}
long glAttachShader(){return 0;}
long glBeginQuery(){return 0;}
long glBeginTransformFeedback(){return 0;}
long glBindAttribLocation(){return 0;}
long glBindBuffer(){return 0;}
long glBindBufferBase(){return 0;}
long glBindBufferRange(){return 0;}
long glBindFramebuffer(){return 0;}
long glBindImageTexture(){return 0;}
long glBindProgramPipeline(){return 0;}
long glBindRenderbuffer(){return 0;}
long glBindSampler(){return 0;}
long glBindTexture(){return 0;}
long glBindTransformFeedback(){return 0;}
long glBindVertexArray(){return 0;}
long glBindVertexBuffer(){return 0;}
long glBlendBarrier(){return 0;}
long glBlendColor(){return 0;}
long glBlendEquation(){return 0;}
long glBlendEquationSeparate(){return 0;}
long glBlendEquationSeparatei(){return 0;}
long glBlendEquationi(){return 0;}
long glBlendFunc(){return 0;}
long glBlendFuncSeparate(){return 0;}
long glBlendFuncSeparatei(){return 0;}
long glBlendFunci(){return 0;}
long glBlitFramebuffer(){return 0;}
long glBufferData(){return 0;}
long glBufferSubData(){return 0;}
long glClear(){return 0;}
long glClearBufferfi(){return 0;}
long glClearBufferfv(){return 0;}
long glClearBufferiv(){return 0;}
long glClearBufferuiv(){return 0;}
long glClearColor(){return 0;}
long glClearDepthf(){return 0;}
long glClearStencil(){return 0;}
long glColorMask(){return 0;}
long glColorMaski(){return 0;}
long glCompileShader(){return 0;}
long glCompressedTexImage2D(){return 0;}
long glCompressedTexImage3D(){return 0;}
long glCompressedTexSubImage2D(){return 0;}
long glCompressedTexSubImage3D(){return 0;}
long glCopyBufferSubData(){return 0;}
long glCopyImageSubData(){return 0;}
long glCopyTexImage2D(){return 0;}
long glCopyTexSubImage2D(){return 0;}
long glCopyTexSubImage3D(){return 0;}
long glCreateShaderProgramv(){return 0;}
long glCullFace(){return 0;}
long glDebugMessageCallback(){return 0;}
long glDebugMessageControl(){return 0;}
long glDebugMessageInsert(){return 0;}
long glDeleteBuffers(){return 0;}
long glDeleteFramebuffers(){return 0;}
long glDeleteProgram(){return 0;}
long glDeleteProgramPipelines(){return 0;}
long glDeleteQueries(){return 0;}
long glDeleteRenderbuffers(){return 0;}
long glDeleteSamplers(){return 0;}
long glDeleteShader(){return 0;}
long glDeleteSync(){return 0;}
long glDeleteTextures(){return 0;}
long glDeleteTransformFeedbacks(){return 0;}
long glDeleteVertexArrays(){return 0;}
long glDepthFunc(){return 0;}
long glDepthMask(){return 0;}
long glDepthRangef(){return 0;}
long glDetachShader(){return 0;}
long glDisable(){return 0;}
long glDisableVertexAttribArray(){return 0;}
long glDisablei(){return 0;}
long glDispatchCompute(){return 0;}
long glDispatchComputeIndirect(){return 0;}
long glDrawArrays(){return 0;}
long glDrawArraysIndirect(){return 0;}
long glDrawArraysInstanced(){return 0;}
long glDrawBuffers(){return 0;}
long glDrawElements(){return 0;}
long glDrawElementsBaseVertex(){return 0;}
long glDrawElementsIndirect(){return 0;}
long glDrawElementsInstanced(){return 0;}
long glDrawElementsInstancedBaseVertex(){return 0;}
long glDrawRangeElements(){return 0;}
long glDrawRangeElementsBaseVertex(){return 0;}
long glEnable(){return 0;}
long glEnableVertexAttribArray(){return 0;}
long glEnablei(){return 0;}
long glEndQuery(){return 0;}
long glEndTransformFeedback(){return 0;}
long glFinish(){return 0;}
long glFlush(){return 0;}
void* glFlushMappedBufferRange(){return G_SCRATCH;}
long glFramebufferParameteri(){return 0;}
long glFramebufferRenderbuffer(){return 0;}
long glFramebufferTexture(){return 0;}
long glFramebufferTexture2D(){return 0;}
long glFramebufferTextureLayer(){return 0;}
long glFrontFace(){return 0;}
long glGenerateMipmap(){return 0;}
long glGetActiveAttrib(){return 0;}
long glGetActiveUniform(){return 0;}
long glGetActiveUniformBlockName(){return 0;}
long glGetActiveUniformBlockiv(){return 0;}
long glGetActiveUniformsiv(){return 0;}
long glGetAttachedShaders(){return 0;}
long glGetBooleani_v(){return 0;}
long glGetBufferParameteri64v(){return 0;}
long glGetBufferParameteriv(){return 0;}
long glGetBufferPointerv(){return 0;}
long glGetDebugMessageLog(){return 0;}
long glGetFragDataLocation(){return 0;}
long glGetFramebufferAttachmentParameteriv(){return 0;}
long glGetFramebufferParameteriv(){return 0;}
long glGetGraphicsResetStatus(){return 0;}
long glGetInteger64i_v(){return 0;}
long glGetIntegeri_v(){return 0;}
long glGetInternalformativ(){return 0;}
long glGetMultisamplefv(){return 0;}
long glGetObjectLabel(){return 0;}
long glGetObjectPtrLabel(){return 0;}
void* glGetPointerv(){return G_SCRATCH;}
long glGetProgramBinary(){return 0;}
long glGetProgramInterfaceiv(){return 0;}
long glGetProgramPipelineInfoLog(){return 0;}
long glGetProgramPipelineiv(){return 0;}
long glGetProgramResourceIndex(){return 0;}
long glGetProgramResourceLocation(){return 0;}
long glGetProgramResourceName(){return 0;}
long glGetProgramResourceiv(){return 0;}
long glGetQueryObjectuiv(){return 0;}
long glGetQueryiv(){return 0;}
long glGetRenderbufferParameteriv(){return 0;}
long glGetSamplerParameterIiv(){return 0;}
long glGetSamplerParameterIuiv(){return 0;}
long glGetSamplerParameterfv(){return 0;}
long glGetSamplerParameteriv(){return 0;}
long glGetShaderPrecisionFormat(){return 0;}
long glGetShaderSource(){return 0;}
long glGetSynciv(){return 0;}
long glGetTexLevelParameterfv(){return 0;}
long glGetTexLevelParameteriv(){return 0;}
long glGetTexParameterIiv(){return 0;}
long glGetTexParameterIuiv(){return 0;}
long glGetTexParameterfv(){return 0;}
long glGetTexParameteriv(){return 0;}
long glGetTransformFeedbackVarying(){return 0;}
long glGetUniformIndices(){return 0;}
long glGetUniformfv(){return 0;}
long glGetUniformiv(){return 0;}
long glGetUniformuiv(){return 0;}
long glGetVertexAttribIiv(){return 0;}
long glGetVertexAttribIuiv(){return 0;}
long glGetVertexAttribfv(){return 0;}
long glGetVertexAttribiv(){return 0;}
long glGetnUniformfv(){return 0;}
long glGetnUniformiv(){return 0;}
long glGetnUniformuiv(){return 0;}
long glHint(){return 0;}
long glInvalidateFramebuffer(){return 0;}
long glInvalidateSubFramebuffer(){return 0;}
long glIsBuffer(){return 0;}
long glIsEnabledi(){return 0;}
long glIsFramebuffer(){return 0;}
long glIsProgram(){return 0;}
long glIsProgramPipeline(){return 0;}
long glIsQuery(){return 0;}
long glIsRenderbuffer(){return 0;}
long glIsSampler(){return 0;}
long glIsShader(){return 0;}
long glIsSync(){return 0;}
long glIsTexture(){return 0;}
long glIsTransformFeedback(){return 0;}
long glIsVertexArray(){return 0;}
long glLineWidth(){return 0;}
long glLinkProgram(){return 0;}
long glMemoryBarrier(){return 0;}
long glMemoryBarrierByRegion(){return 0;}
long glMinSampleShading(){return 0;}
long glObjectLabel(){return 0;}
long glObjectPtrLabel(){return 0;}
long glPatchParameteri(){return 0;}
long glPauseTransformFeedback(){return 0;}
long glPixelStorei(){return 0;}
long glPolygonOffset(){return 0;}
long glPopDebugGroup(){return 0;}
long glPrimitiveBoundingBox(){return 0;}
long glProgramBinary(){return 0;}
long glProgramParameteri(){return 0;}
long glProgramUniform1f(){return 0;}
long glProgramUniform1fv(){return 0;}
long glProgramUniform1i(){return 0;}
long glProgramUniform1iv(){return 0;}
long glProgramUniform1ui(){return 0;}
long glProgramUniform1uiv(){return 0;}
long glProgramUniform2f(){return 0;}
long glProgramUniform2fv(){return 0;}
long glProgramUniform2i(){return 0;}
long glProgramUniform2iv(){return 0;}
long glProgramUniform2ui(){return 0;}
long glProgramUniform2uiv(){return 0;}
long glProgramUniform3f(){return 0;}
long glProgramUniform3fv(){return 0;}
long glProgramUniform3i(){return 0;}
long glProgramUniform3iv(){return 0;}
long glProgramUniform3ui(){return 0;}
long glProgramUniform3uiv(){return 0;}
long glProgramUniform4f(){return 0;}
long glProgramUniform4fv(){return 0;}
long glProgramUniform4i(){return 0;}
long glProgramUniform4iv(){return 0;}
long glProgramUniform4ui(){return 0;}
long glProgramUniform4uiv(){return 0;}
long glProgramUniformMatrix2fv(){return 0;}
long glProgramUniformMatrix2x3fv(){return 0;}
long glProgramUniformMatrix2x4fv(){return 0;}
long glProgramUniformMatrix3fv(){return 0;}
long glProgramUniformMatrix3x2fv(){return 0;}
long glProgramUniformMatrix3x4fv(){return 0;}
long glProgramUniformMatrix4fv(){return 0;}
long glProgramUniformMatrix4x2fv(){return 0;}
long glProgramUniformMatrix4x3fv(){return 0;}
long glPushDebugGroup(){return 0;}
long glReadBuffer(){return 0;}
long glReadPixels(){return 0;}
long glReadnPixels(){return 0;}
long glReleaseShaderCompiler(){return 0;}
long glRenderbufferStorage(){return 0;}
long glRenderbufferStorageMultisample(){return 0;}
long glResumeTransformFeedback(){return 0;}
long glSampleCoverage(){return 0;}
long glSampleMaski(){return 0;}
long glSamplerParameterIiv(){return 0;}
long glSamplerParameterIuiv(){return 0;}
long glSamplerParameterf(){return 0;}
long glSamplerParameterfv(){return 0;}
long glSamplerParameteri(){return 0;}
long glSamplerParameteriv(){return 0;}
long glScissor(){return 0;}
long glShaderBinary(){return 0;}
long glShaderSource(){return 0;}
long glStencilFunc(){return 0;}
long glStencilFuncSeparate(){return 0;}
long glStencilMask(){return 0;}
long glStencilMaskSeparate(){return 0;}
long glStencilOp(){return 0;}
long glStencilOpSeparate(){return 0;}
long glTexBuffer(){return 0;}
long glTexBufferRange(){return 0;}
long glTexImage2D(){return 0;}
long glTexImage3D(){return 0;}
long glTexParameterIiv(){return 0;}
long glTexParameterIuiv(){return 0;}
long glTexParameterf(){return 0;}
long glTexParameterfv(){return 0;}
long glTexParameteri(){return 0;}
long glTexParameteriv(){return 0;}
long glTexStorage2D(){return 0;}
long glTexStorage2DMultisample(){return 0;}
long glTexStorage3D(){return 0;}
long glTexStorage3DMultisample(){return 0;}
long glTexSubImage2D(){return 0;}
long glTexSubImage3D(){return 0;}
long glTransformFeedbackVaryings(){return 0;}
long glUniform1f(){return 0;}
long glUniform1fv(){return 0;}
long glUniform1i(){return 0;}
long glUniform1iv(){return 0;}
long glUniform1ui(){return 0;}
long glUniform1uiv(){return 0;}
long glUniform2f(){return 0;}
long glUniform2fv(){return 0;}
long glUniform2i(){return 0;}
long glUniform2iv(){return 0;}
long glUniform2ui(){return 0;}
long glUniform2uiv(){return 0;}
long glUniform3f(){return 0;}
long glUniform3fv(){return 0;}
long glUniform3i(){return 0;}
long glUniform3iv(){return 0;}
long glUniform3ui(){return 0;}
long glUniform3uiv(){return 0;}
long glUniform4f(){return 0;}
long glUniform4fv(){return 0;}
long glUniform4i(){return 0;}
long glUniform4iv(){return 0;}
long glUniform4ui(){return 0;}
long glUniform4uiv(){return 0;}
long glUniformBlockBinding(){return 0;}
long glUniformMatrix2fv(){return 0;}
long glUniformMatrix2x3fv(){return 0;}
long glUniformMatrix2x4fv(){return 0;}
long glUniformMatrix3fv(){return 0;}
long glUniformMatrix3x2fv(){return 0;}
long glUniformMatrix3x4fv(){return 0;}
long glUniformMatrix4fv(){return 0;}
long glUniformMatrix4x2fv(){return 0;}
long glUniformMatrix4x3fv(){return 0;}
long glUnmapBuffer(){return 0;}
long glUseProgram(){return 0;}
long glUseProgramStages(){return 0;}
long glValidateProgram(){return 0;}
long glValidateProgramPipeline(){return 0;}
long glVertexAttrib1f(){return 0;}
long glVertexAttrib1fv(){return 0;}
long glVertexAttrib2f(){return 0;}
long glVertexAttrib2fv(){return 0;}
long glVertexAttrib3f(){return 0;}
long glVertexAttrib3fv(){return 0;}
long glVertexAttrib4f(){return 0;}
long glVertexAttrib4fv(){return 0;}
long glVertexAttribBinding(){return 0;}
long glVertexAttribDivisor(){return 0;}
long glVertexAttribFormat(){return 0;}
long glVertexAttribI4i(){return 0;}
long glVertexAttribI4iv(){return 0;}
long glVertexAttribI4ui(){return 0;}
long glVertexAttribI4uiv(){return 0;}
long glVertexAttribIFormat(){return 0;}
long glVertexAttribIPointer(){return 0;}
long glVertexAttribPointer(){return 0;}
long glVertexBindingDivisor(){return 0;}
long glViewport(){return 0;}
long glWaitSync(){return 0;}
static int streq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
struct __pe{const char*n;void*p;};
static struct __pe __petab[]={
  {"eglGetError",(void*)eglGetError},
  {"eglGetDisplay",(void*)eglGetDisplay},
  {"eglGetPlatformDisplay",(void*)eglGetPlatformDisplay},
  {"eglInitialize",(void*)eglInitialize},
  {"eglTerminate",(void*)eglTerminate},
  {"eglBindAPI",(void*)eglBindAPI},
  {"eglQueryAPI",(void*)eglQueryAPI},
  {"eglChooseConfig",(void*)eglChooseConfig},
  {"eglGetConfigs",(void*)eglGetConfigs},
  {"eglGetConfigAttrib",(void*)eglGetConfigAttrib},
  {"eglCreateContext",(void*)eglCreateContext},
  {"eglDestroyContext",(void*)eglDestroyContext},
  {"eglCreateWindowSurface",(void*)eglCreateWindowSurface},
  {"eglCreatePbufferSurface",(void*)eglCreatePbufferSurface},
  {"eglCreatePixmapSurface",(void*)eglCreatePixmapSurface},
  {"eglCreatePlatformWindowSurface",(void*)eglCreatePlatformWindowSurface},
  {"eglCreatePlatformPixmapSurface",(void*)eglCreatePlatformPixmapSurface},
  {"eglDestroySurface",(void*)eglDestroySurface},
  {"eglMakeCurrent",(void*)eglMakeCurrent},
  {"eglSwapBuffers",(void*)eglSwapBuffers},
  {"eglSwapInterval",(void*)eglSwapInterval},
  {"eglQueryString",(void*)eglQueryString},
  {"eglQuerySurface",(void*)eglQuerySurface},
  {"eglQueryContext",(void*)eglQueryContext},
  {"eglGetCurrentDisplay",(void*)eglGetCurrentDisplay},
  {"eglGetCurrentContext",(void*)eglGetCurrentContext},
  {"eglGetCurrentSurface",(void*)eglGetCurrentSurface},
  {"eglReleaseThread",(void*)eglReleaseThread},
  {"eglWaitClient",(void*)eglWaitClient},
  {"eglWaitGL",(void*)eglWaitGL},
  {"eglWaitNative",(void*)eglWaitNative},
  {"eglSurfaceAttrib",(void*)eglSurfaceAttrib},
  {"eglBindTexImage",(void*)eglBindTexImage},
  {"eglReleaseTexImage",(void*)eglReleaseTexImage},
  {"eglCreateSync",(void*)eglCreateSync},
  {"eglDestroySync",(void*)eglDestroySync},
  {"eglClientWaitSync",(void*)eglClientWaitSync},
  {"eglGetSyncAttrib",(void*)eglGetSyncAttrib},
  {"eglWaitSync",(void*)eglWaitSync},
  {"eglCreateImage",(void*)eglCreateImage},
  {"eglDestroyImage",(void*)eglDestroyImage},
  {"eglCreatePbufferFromClientBuffer",(void*)eglCreatePbufferFromClientBuffer},
  {"eglCopyBuffers",(void*)eglCopyBuffers},
  {"glGetError",(void*)glGetError},
  {"glGetString",(void*)glGetString},
  {"glGetStringi",(void*)glGetStringi},
  {"glGetIntegerv",(void*)glGetIntegerv},
  {"glGetInteger64v",(void*)glGetInteger64v},
  {"glGetFloatv",(void*)glGetFloatv},
  {"glGetBooleanv",(void*)glGetBooleanv},
  {"glGetShaderiv",(void*)glGetShaderiv},
  {"glGetProgramiv",(void*)glGetProgramiv},
  {"glGetShaderInfoLog",(void*)glGetShaderInfoLog},
  {"glGetProgramInfoLog",(void*)glGetProgramInfoLog},
  {"glCreateShader",(void*)glCreateShader},
  {"glCreateProgram",(void*)glCreateProgram},
  {"glGetUniformLocation",(void*)glGetUniformLocation},
  {"glGetAttribLocation",(void*)glGetAttribLocation},
  {"glGetUniformBlockIndex",(void*)glGetUniformBlockIndex},
  {"glCheckFramebufferStatus",(void*)glCheckFramebufferStatus},
  {"glIsEnabled",(void*)glIsEnabled},
  {"glFenceSync",(void*)glFenceSync},
  {"glClientWaitSync",(void*)glClientWaitSync},
  {"glMapBufferRange",(void*)glMapBufferRange},
  {"glGenTextures",(void*)glGenTextures},
  {"glGenBuffers",(void*)glGenBuffers},
  {"glGenFramebuffers",(void*)glGenFramebuffers},
  {"glGenRenderbuffers",(void*)glGenRenderbuffers},
  {"glGenVertexArrays",(void*)glGenVertexArrays},
  {"glGenSamplers",(void*)glGenSamplers},
  {"glGenQueries",(void*)glGenQueries},
  {"glGenProgramPipelines",(void*)glGenProgramPipelines},
  {"glGenTransformFeedbacks",(void*)glGenTransformFeedbacks},
  {0,0}};
void* eglGetProcAddress(const char* n){int i;if(n)for(i=0;__petab[i].n;i++)if(streq(n,__petab[i].n))return __petab[i].p;return (void*)noop_ret0;}
