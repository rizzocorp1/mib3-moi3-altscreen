import sys
syms = [l.strip() for l in open(sys.argv[1]) if l.strip()]
# funcoes EGL/GL criticas, escritas a mao (retornos sanos p/ a HMI aceitar o contexto)
HAND = {
 # --- EGL ---
 "eglGetError":           "int eglGetError(void){return 0x3000;}",            # EGL_SUCCESS
 "eglGetDisplay":         "void* eglGetDisplay(void* d){(void)d;return &G_DPY;}",
 "eglGetPlatformDisplay": "void* eglGetPlatformDisplay(unsigned p,void* n,const void* a){(void)p;(void)n;(void)a;return &G_DPY;}",
 "eglInitialize":         "int eglInitialize(void* d,int* ma,int* mi){(void)d;if(ma)*ma=1;if(mi)*mi=5;return 1;}",
 "eglTerminate":          "int eglTerminate(void* d){(void)d;return 1;}",
 "eglBindAPI":            "int eglBindAPI(unsigned a){(void)a;return 1;}",
 "eglQueryAPI":           "unsigned eglQueryAPI(void){return 0x30A0;}",       # EGL_OPENGL_ES_API
 "eglChooseConfig":       "int eglChooseConfig(void* d,const int* a,void** c,int n,int* num){(void)d;(void)a;if(c&&n>0)c[0]=&G_CFG;if(num)*num=1;return 1;}",
 "eglGetConfigs":         "int eglGetConfigs(void* d,void** c,int n,int* num){(void)d;if(c&&n>0)c[0]=&G_CFG;if(num)*num=1;return 1;}",
 "eglGetConfigAttrib":    "int eglGetConfigAttrib(void* d,void* c,int a,int* v){(void)d;(void)c;(void)a;if(v)*v=8;return 1;}",
 "eglCreateContext":      "void* eglCreateContext(void* d,void* c,void* s,const int* a){(void)d;(void)c;(void)s;(void)a;return &G_CTX;}",
 "eglDestroyContext":     "int eglDestroyContext(void* d,void* c){(void)d;(void)c;return 1;}",
 "eglCreateWindowSurface":"void* eglCreateWindowSurface(void* d,void* c,void* w,const int* a){(void)d;(void)c;(void)w;(void)a;return &G_SURF;}",
 "eglCreatePbufferSurface":"void* eglCreatePbufferSurface(void* d,void* c,const int* a){(void)d;(void)c;(void)a;return &G_SURF;}",
 "eglCreatePixmapSurface":"void* eglCreatePixmapSurface(void* d,void* c,void* p,const int* a){(void)d;(void)c;(void)p;(void)a;return &G_SURF;}",
 "eglCreatePlatformWindowSurface":"void* eglCreatePlatformWindowSurface(void* d,void* c,void* w,const void* a){(void)d;(void)c;(void)w;(void)a;return &G_SURF;}",
 "eglCreatePlatformPixmapSurface":"void* eglCreatePlatformPixmapSurface(void* d,void* c,void* p,const void* a){(void)d;(void)c;(void)p;(void)a;return &G_SURF;}",
 "eglDestroySurface":     "int eglDestroySurface(void* d,void* s){(void)d;(void)s;return 1;}",
 "eglMakeCurrent":        "int eglMakeCurrent(void* d,void* dr,void* rd,void* c){(void)d;(void)dr;(void)rd;(void)c;return 1;}",
 "eglSwapBuffers":        "int eglSwapBuffers(void* d,void* s){(void)d;(void)s;return 1;}",
 "eglSwapInterval":       "int eglSwapInterval(void* d,int i){(void)d;(void)i;return 1;}",
 "eglQueryString":        "const char* eglQueryString(void* d,int n){(void)d;(void)n;return \"stub\";}",
 "eglQuerySurface":       "int eglQuerySurface(void* d,void* s,int a,int* v){(void)d;(void)s;(void)a;if(v)*v=0;return 1;}",
 "eglQueryContext":       "int eglQueryContext(void* d,void* c,int a,int* v){(void)d;(void)c;(void)a;if(v)*v=0;return 1;}",
 "eglGetCurrentDisplay":  "void* eglGetCurrentDisplay(void){return &G_DPY;}",
 "eglGetCurrentContext":  "void* eglGetCurrentContext(void){return &G_CTX;}",
 "eglGetCurrentSurface":  "void* eglGetCurrentSurface(int r){(void)r;return &G_SURF;}",
 "eglReleaseThread":      "int eglReleaseThread(void){return 1;}",
 "eglWaitClient":         "int eglWaitClient(void){return 1;}",
 "eglWaitGL":             "int eglWaitGL(void){return 1;}",
 "eglWaitNative":         "int eglWaitNative(int e){(void)e;return 1;}",
 "eglSurfaceAttrib":      "int eglSurfaceAttrib(void* d,void* s,int a,int v){(void)d;(void)s;(void)a;(void)v;return 1;}",
 "eglBindTexImage":       "int eglBindTexImage(void* d,void* s,int b){(void)d;(void)s;(void)b;return 1;}",
 "eglReleaseTexImage":    "int eglReleaseTexImage(void* d,void* s,int b){(void)d;(void)s;(void)b;return 1;}",
 "eglCreateSync":         "void* eglCreateSync(void* d,unsigned t,const int* a){(void)d;(void)t;(void)a;return &G_SYNC;}",
 "eglDestroySync":        "int eglDestroySync(void* d,void* s){(void)d;(void)s;return 1;}",
 "eglClientWaitSync":     "int eglClientWaitSync(void* d,void* s,int f,unsigned long t){(void)d;(void)s;(void)f;(void)t;return 0x30F6;}",
 "eglGetSyncAttrib":      "int eglGetSyncAttrib(void* d,void* s,int a,int* v){(void)d;(void)s;(void)a;if(v)*v=0;return 1;}",
 "eglWaitSync":           "int eglWaitSync(void* d,void* s,int f){(void)d;(void)s;(void)f;return 1;}",
 "eglCreateImage":        "void* eglCreateImage(void* d,void* c,unsigned t,void* b,const int* a){(void)d;(void)c;(void)t;(void)b;(void)a;return &G_IMG;}",
 "eglDestroyImage":       "int eglDestroyImage(void* d,void* i){(void)d;(void)i;return 1;}",
 "eglCreatePbufferFromClientBuffer":"void* eglCreatePbufferFromClientBuffer(void* d,unsigned t,void* b,void* c,const int* a){(void)d;(void)t;(void)b;(void)c;(void)a;return &G_SURF;}",
 "eglCopyBuffers":        "int eglCopyBuffers(void* d,void* s,void* t){(void)d;(void)s;(void)t;return 1;}",
 # --- GL: queries que a HMI checa ---
 "glGetError":            "unsigned glGetError(void){return 0;}",
 "glGetString":           "const unsigned char* glGetString(unsigned n){switch(n){case 0x1F00:return (const unsigned char*)\"stub-vendor\";case 0x1F01:return (const unsigned char*)\"stub-renderer\";case 0x1F02:return (const unsigned char*)\"OpenGL ES 3.2 stub\";case 0x8B8C:return (const unsigned char*)\"OpenGL ES GLSL ES 3.20\";case 0x1F03:return (const unsigned char*)\"GL_OES_rgb8_rgba8 GL_OES_texture_npot GL_OES_vertex_array_object\";default:return (const unsigned char*)\"stub\";}}",
 "glGetStringi":          "const unsigned char* glGetStringi(unsigned n,unsigned i){(void)n;(void)i;return (const unsigned char*)\"\";}",
 "glGetIntegerv":         "void glGetIntegerv(unsigned p,int* v){if(!v)return;switch(p){case 0x0D33:*v=8192;break;case 0x8B4D:*v=32;break;case 0x8872:*v=32;break;case 0x821D:*v=0;break;case 0x8CDF:*v=8;break;default:*v=0;}}",
 "glGetInteger64v":       "void glGetInteger64v(unsigned p,long* v){(void)p;if(v)*v=0;}",
 "glGetFloatv":           "void glGetFloatv(unsigned p,float* v){(void)p;if(v)*v=0.0f;}",
 "glGetBooleanv":         "void glGetBooleanv(unsigned p,unsigned char* v){(void)p;if(v)*v=0;}",
 "glGetShaderiv":         "void glGetShaderiv(unsigned s,unsigned p,int* v){(void)s;(void)p;if(v)*v=1;}",
 "glGetProgramiv":        "void glGetProgramiv(unsigned s,unsigned p,int* v){(void)s;(void)p;if(v)*v=1;}",
 "glGetShaderInfoLog":    "void glGetShaderInfoLog(unsigned s,int m,int* l,char* b){(void)s;(void)m;if(l)*l=0;if(b&&m>0)b[0]=0;}",
 "glGetProgramInfoLog":   "void glGetProgramInfoLog(unsigned s,int m,int* l,char* b){(void)s;(void)m;if(l)*l=0;if(b&&m>0)b[0]=0;}",
 "glCreateShader":        "unsigned glCreateShader(unsigned t){(void)t;return ++G_ID;}",
 "glCreateProgram":       "unsigned glCreateProgram(void){return ++G_ID;}",
 "glGetUniformLocation":  "int glGetUniformLocation(unsigned p,const char* n){(void)p;(void)n;return 0;}",
 "glGetAttribLocation":   "int glGetAttribLocation(unsigned p,const char* n){(void)p;(void)n;return 0;}",
 "glGetUniformBlockIndex":"unsigned glGetUniformBlockIndex(unsigned p,const char* n){(void)p;(void)n;return 0;}",
 "glCheckFramebufferStatus":"unsigned glCheckFramebufferStatus(unsigned t){(void)t;return 0x8CD5;}",
 "glIsEnabled":           "unsigned char glIsEnabled(unsigned c){(void)c;return 0;}",
 "glFenceSync":           "void* glFenceSync(unsigned c,unsigned f){(void)c;(void)f;return &G_SYNC;}",
 "glClientWaitSync":      "unsigned glClientWaitSync(void* s,unsigned f,unsigned long t){(void)s;(void)f;(void)t;return 0x911A;}",
 "glMapBufferRange":      "void* glMapBufferRange(unsigned t,long o,long l,unsigned a){(void)t;(void)o;(void)a;return (l>0&&l<(long)sizeof G_SCRATCH)?G_SCRATCH:G_SCRATCH;}",
}
# gl*Gen*/glGen...(n,ids) e Create: geram ids nao-nulos
GENOBJ = ["glGenTextures","glGenBuffers","glGenFramebuffers","glGenRenderbuffers","glGenVertexArrays",
          "glGenSamplers","glGenQueries","glGenProgramPipelines","glGenTransformFeedbacks"]
for g in GENOBJ:
    HAND[g] = "void %s(int n,unsigned* ids){int i;if(ids)for(i=0;i<n;i++)ids[i]=++G_ID;}"%g

out=[]
out.append("""/* fakegl.c — stub de EGL/GLESv3 p/ o sandbox (estudo): faz o loop de render da HMI
 * EXECUTAR sem GPU/compositor (desenha pro nada). Nao gera imagem real.
 * Gerado por gen_fakegl.py a partir dos simbolos que libjopengl_es3 importa do Adreno. */
typedef unsigned long u64;
static int G_DPY,G_CFG,G_CTX,G_SURF,G_SYNC,G_IMG; static unsigned G_ID=0;
static char G_SCRATCH[1<<20];
static long noop_ret0(void){return 0;}
""")
done=set()
for s in syms:
    if s in HAND:
        out.append(HAND[s]); done.add(s)
done.add("eglGetProcAddress")  # emitido no fim com tabela
# resto: ponteiro-ish -> retorna scratch; demais -> no-op 0
PTRISH=("Map","GetPointer")
for s in syms:
    if s in done: continue
    if any(k in s for k in PTRISH):
        out.append("void* %s(){return G_SCRATCH;}"%s)
    else:
        out.append("long %s(){return 0;}"%s)

# eglGetProcAddress com tabela: mapeia nome->stub real (p/ quem resolve por procaddress)
tbl=[k for k in HAND.keys()]
rows="".join('  {"%s",(void*)%s},\n'%(k,k) for k in tbl)
out.append("static int streq(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}")
out.append("struct __pe{const char*n;void*p;};")
out.append("static struct __pe __petab[]={\n"+rows+"  {0,0}};")
out.append("void* eglGetProcAddress(const char* n){int i;if(n)for(i=0;__petab[i].n;i++)if(streq(n,__petab[i].n))return __petab[i].p;return (void*)noop_ret0;}")

open(sys.argv[2],"w").write("\n".join(out)+"\n")
print("gerado %s com %d simbolos (%d a mao)"%(sys.argv[2],len(syms),len(done)))
