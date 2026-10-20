/*
 * gl_shim_moi3.c — ESQUELETO de shim GLES2/EGL para o sandbox MOI3 (Jetta MIB3 LG).
 *
 * Adaptado de tools/mhi2/glforward/guest/gl_shim.c + bridge_transport.h do
 * iscle/qemu-mhi2 (estudo em ../estudo-qemu-mhi2.md). Duas diferenças vs o fork:
 *   1. TRANSPORTE = socket Unix (não a janela MMIO 0x5f000000 do QEMU). O nosso
 *      sandbox roda a HMI direto num container, então dispensa o device model.
 *   2. Alvo = Adreno/Qualcomm. Builda como libGLESv2_adreno.so / libEGL_adreno.so
 *      (os sonames que o libjopengl do MOI3 pede como NEEDED — ver NOTAS-RE-DINAMICA).
 *
 * A numeração de opcodes é IDÊNTICA ao host/glhost.c do fork, então o glhost é
 * reusável quase sem mudança (remover só a tradução de shader binário NVIDIA,
 * op 110 — a HMI Kanzi do MOI3 manda GLSL-fonte via op 8).
 *
 * ISTO É UM ESQUELETO: traz o transporte completo + EGL + um subconjunto
 * representativo de GLES2 (pipeline de frame). Os demais entry points são
 * mecânicos — copiar o corpo do gl_shim.c do fork, trocando só a camada de
 * transporte (já feita aqui). Marcados com TODO.
 *
 * Build (cross aarch64, dentro do rootfs/container do MOI3):
 *   aarch64-linux-gnu-gcc -shared -fPIC -O2 -pthread \
 *     -Wl,-soname,libGLESv2_adreno.so -o libGLESv2_adreno.so gl_shim_moi3.c
 *   ln -sf libGLESv2_adreno.so libEGL_adreno.so   # mesmos símbolos; ou buildar 2x
 *   (um .so só exportando GLES2+EGL serve aos dois NEEDED; symlink resolve o soname)
 *
 * Runtime: exportar MHI2_GL_SOCKET=/tmp/moi3-gl.sock e subir o glhost escutando
 * nesse socket (adaptar o serve() do glhost pra accept() em vez de ler stdin).
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef uint32_t u32;
typedef int32_t  i32;

/* ---------------------------------------------------------------------------
 * Transporte: socket Unix. Um record é escrito atomicamente sob g_lock; se o
 * op for um getter, a resposta é lida sob o mesmo lock (protocolo request/reply
 * serializado). A HMI renderiza de poucas threads; o lock global basta.
 * ------------------------------------------------------------------------- */
static int g_fd = -1;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static u32 g_id;           /* alocador de nomes de objeto GL do lado guest */

static int gl_connect(void)
{
    if (g_fd >= 0) return g_fd;
    const char *path = getenv("MHI2_GL_SOCKET");
    if (!path || !*path) return -1;
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_un a; memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, path, sizeof a.sun_path - 1);
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) { close(fd); return -1; }
    g_fd = fd;
    return fd;
}
static int write_all(const void *p, size_t n)
{
    const uint8_t *c = p;
    while (n) { ssize_t r = write(g_fd, c, n); if (r <= 0) return -1; c += r; n -= r; }
    return 0;
}
static int read_all(void *p, size_t n)
{
    uint8_t *c = p;
    while (n) { ssize_t r = read(g_fd, c, n); if (r <= 0) return -1; c += r; n -= r; }
    return 0;
}

/* Record montado em thread-local, igual ao fork (clientes renderam de várias
 * threads). Header: [magic 119][versão 4][pid][op][total], depois o payload. */
struct Record { uint8_t *data; unsigned size, used, cap; int reply; };
static pthread_key_t rec_key;
static pthread_once_t rec_once = PTHREAD_ONCE_INIT;
static void rec_free(void *p){ struct Record *r=p; if(r){ free(r->data); free(r);} }
static void rec_init(void){ pthread_key_create(&rec_key, rec_free); }
static struct Record *cur(void)
{
    pthread_once(&rec_once, rec_init);
    struct Record *r = pthread_getspecific(rec_key);
    if (!r) { r = calloc(1, sizeof *r); pthread_setspecific(rec_key, r); }
    return r;
}
/* ops que retornam valor (precisam de reply pelo mesmo canal) — igual ao fork */
static int is_reply(u32 op)
{
    return (op>=100 && op<=109) || op==120 || op==121 || op==123 || op==124 ||
           op==126 || op==127 || op==131 || op==132 || op==138 || (op>=134 && op<=137);
}
static void commit(struct Record *r)
{
    if (gl_connect() < 0) return;
    pthread_mutex_lock(&g_lock);
    write_all(r->data, r->size);
    if (!r->reply) pthread_mutex_unlock(&g_lock);   /* reply-less: solta já */
    /* se for reply, o caller faz recv_*() e depois solta o lock */
}
static void rec(u32 op, u32 total)
{
    struct Record *r = cur();
    unsigned size = 20 + total;
    if (size > r->cap) { r->data = realloc(r->data, size); r->cap = size; }
    u32 *h = (u32 *)r->data;
    h[0]=119; h[1]=4; h[2]=(u32)getpid(); h[3]=op; h[4]=total;
    r->size=size; r->used=20; r->reply=is_reply(op);
    if (!total) commit(r);
}
static void part(const void *p, unsigned n)
{
    if (!n) return;
    struct Record *r = cur();
    memcpy(r->data + r->used, p, n); r->used += n;
    if (r->used == r->size) commit(r);
}
#define PART(p,n) part((p),(n))
static void emit_iv(u32 op, const i32 *v, u32 n){ rec(op, n*4); PART(v, n*4); }
/* recv de reply: lê n bytes e solta o lock que o commit() segurou */
static int recv_all(void *p, unsigned n){ int rc = read_all(p, n); pthread_mutex_unlock(&g_lock); return rc; }

/* ---------------------------------------------------------------------------
 * EGL — na maioria faticado localmente; só eglInitialize (op 1, passa w/h) e
 * eglSwapBuffers (op 5) são encaminhados. O host (glhost) é o dono do EGL real.
 * Handles opacos não-nulos bastam pra HMI achar que deu certo.
 * ------------------------------------------------------------------------- */
#define FAKE ((void *)0x1)
void *eglGetDisplay(void *id){ (void)id; return FAKE; }
void *eglGetCurrentDisplay(void){ return FAKE; }
unsigned eglInitialize(void *d, i32 *major, i32 *minor)
{ (void)d; if(major)*major=1; if(minor)*minor=4; return 1; }
unsigned eglChooseConfig(void *d, const i32 *attr, void **cfg, i32 n, i32 *num)
{ (void)d;(void)attr;(void)n; if(cfg)*cfg=FAKE; if(num)*num=1; return 1; }
unsigned eglGetConfigAttrib(void *d,void *c,i32 a,i32 *v){ (void)d;(void)c;(void)a; if(v)*v=0; return 1; }
void *eglCreateContext(void *d,void *c,void *share,const i32 *a){ (void)d;(void)c;(void)share;(void)a; return FAKE; }
void *eglCreateWindowSurface(void *d,void *c,void *win,const i32 *a){ (void)d;(void)c;(void)win;(void)a; return FAKE; }
void *eglCreatePbufferSurface(void *d,void *c,const i32 *a){ (void)d;(void)c;(void)a; return FAKE; }
unsigned eglMakeCurrent(void *d,void *draw,void *read,void *ctx){ (void)d;(void)draw;(void)read;(void)ctx; return 1; }
unsigned eglSwapBuffers(void *d, void *surf)
{ (void)d;(void)surf; rec(5, 0); return 1; }            /* op 5 = swap -> host faz readpixels->PPM */
unsigned eglSwapInterval(void *d, i32 i){ (void)d;(void)i; return 1; }
unsigned eglDestroyContext(void *d,void *c){ (void)d;(void)c; return 1; }
unsigned eglDestroySurface(void *d,void *s){ (void)d;(void)s; return 1; }
unsigned eglTerminate(void *d){ (void)d; return 1; }
unsigned eglBindAPI(u32 api){ (void)api; return 1; }
i32 eglGetError(void){ return 0x3000; }                 /* EGL_SUCCESS */
const char *eglQueryString(void *d, i32 name){ (void)d;(void)name; return ""; }
/* a HMI resolve ponteiros por eglGetProcAddress — devolver os nossos símbolos.
 * TODO: tabela nome->ponteiro das extensões que a HMI pedir (ex. glMapBufferRange,
 * eglCreateImageKHR/glEGLImageTargetTexture2DOES pro caminho de cluster). */
void *eglGetProcAddress(const char *name){ (void)name; return 0; }

/* ---------------------------------------------------------------------------
 * GLES2 — subconjunto representativo (pipeline de frame). Opcodes IDÊNTICOS ao
 * glhost.c do fork. O restante é mecânico: copiar de gl_shim.c trocando só a
 * camada de transporte (já pronta acima). Ver tabela no fim.
 * ------------------------------------------------------------------------- */
void glViewport(i32 x,i32 y,i32 w,i32 h){ i32 p[4]={x,y,w,h}; emit_iv(2,p,4); }
void glClearColor(float r,float g,float b,float a){ float p[4]={r,g,b,a}; rec(3,16); PART(p,16); }
void glClear(u32 mask){ emit_iv(4,(i32*)&mask,1); }
void glFinish(void){ rec(6,0); }

u32  glCreateShader(u32 type){ u32 id=__sync_add_and_fetch(&g_id,1); u32 p[2]={id,type}; emit_iv(7,(i32*)p,2); return id; }
void glShaderSource(u32 id,i32 count,const char *const *str,const i32 *len)
{ /* junta as count strings numa só */
    u32 total=0; for(i32 i=0;i<count;i++) total += len&&len[i]>=0 ? (u32)len[i] : (u32)strlen(str[i]);
    rec(8, 8+total); u32 h[2]={id,total}; PART(h,8);
    for(i32 i=0;i<count;i++){ u32 n=len&&len[i]>=0?(u32)len[i]:(u32)strlen(str[i]); PART(str[i],n);} }
void glCompileShader(u32 s){ emit_iv(9,(i32*)&s,1); }
u32  glCreateProgram(void){ u32 id=__sync_add_and_fetch(&g_id,1); emit_iv(10,(i32*)&id,1); return id; }
void glAttachShader(u32 p,u32 s){ u32 v[2]={p,s}; emit_iv(11,(i32*)v,2); }
void glLinkProgram(u32 p){ emit_iv(12,(i32*)&p,1); }
void glUseProgram(u32 p){ emit_iv(13,(i32*)&p,1); }

void glGenBuffers(i32 n,u32 *b){ if(!b)return; rec(15,4+(u32)n*4); u32 c=(u32)n; PART(&c,4);
    for(i32 i=0;i<n;i++){ b[i]=__sync_add_and_fetch(&g_id,1); PART(&b[i],4);} }
void glBindBuffer(u32 t,u32 id){ u32 p[2]={t,id}; emit_iv(16,(i32*)p,2); }
void glBufferData(u32 t,i32 size,const void *data,u32 usage){ u32 sz=size<0?0:(u32)size, dn=data?sz:0;
    u32 h[3]={t,sz,usage}; rec(17,12+dn); PART(h,12); if(dn)PART(data,dn); }

void glVertexAttribPointer(u32 i,i32 size,u32 type,u32 norm,i32 stride,const void *ptr){
    /* NOTA: aqui o fork trata ponteiros client-side (sem VBO) copiando no draw (op 128/138).
     * TODO: portar essa lógica de ClientAttrib do gl_shim.c. */
    u32 p[6]={i,(u32)size,type,norm?1u:0u,(u32)stride,(u32)(uintptr_t)ptr}; emit_iv(19,(i32*)p,6); }
void glEnableVertexAttribArray(u32 i){ emit_iv(20,(i32*)&i,1); }
void glDisableVertexAttribArray(u32 i){ emit_iv(21,(i32*)&i,1); }

void glUniform1i(i32 loc,i32 v){ i32 p[2]={loc,v}; emit_iv(30,p,2); }
void glUniformMatrix4fv(i32 loc,i32 count,u32 t,const float *v){ u32 h[3]={(u32)loc,(u32)count,t};
    u32 fn=(u32)count*16*4; rec(28,12+fn); PART(h,12); if(fn)PART(v,fn); }
/* TODO: glUniform{1..4}{f,i}{,v}, glUniformMatrix{2,3}fv — ops 22-30, 80-87 */

void glDrawArrays(u32 mode,i32 first,i32 count){ i32 p[3]={(i32)mode,first,count}; emit_iv(31,p,3); }
void glDrawElements(u32 mode,i32 count,u32 type,const void *idx){
    /* caminho com EBO ligado (sem client arrays) — o completo trata client arrays via 129/138 */
    u32 p[4]={mode,(u32)count,type,(u32)(uintptr_t)idx}; emit_iv(32,(i32*)p,4); }

void glEnable(u32 c){ emit_iv(33,(i32*)&c,1); }
void glDisable(u32 c){ emit_iv(34,(i32*)&c,1); }
void glBlendFunc(u32 s,u32 d){ u32 p[2]={s,d}; emit_iv(35,(i32*)p,2); }

void glGenTextures(i32 n,u32 *t){ if(!t)return; rec(50,4+(u32)n*4); u32 c=(u32)n; PART(&c,4);
    for(i32 i=0;i<n;i++){ t[i]=__sync_add_and_fetch(&g_id,1); PART(&t[i],4);} }
void glBindTexture(u32 t,u32 id){ u32 p[2]={t,id}; emit_iv(51,(i32*)p,2); }
void glTexParameteri(u32 t,u32 pn,i32 v){ i32 p[3]={(i32)t,(i32)pn,v}; emit_iv(53,p,3); }
void glActiveTexture(u32 tex){ emit_iv(54,(i32*)&tex,1); }
/* TODO: glTexImage2D/glTexSubImage2D (ops 52/56, com payload+pixels) */

/* getters (bloqueiam por reply) */
i32 glGetUniformLocation(u32 prog,const char *name){ u32 l=(u32)strlen(name),h[2]={prog,l};
    rec(101,8+l); PART(h,8); PART(name,l); i32 loc=-1; recv_all(&loc,4); return loc; }
i32 glGetAttribLocation(u32 prog,const char *name){ u32 l=(u32)strlen(name),h[2]={prog,l};
    rec(100,8+l); PART(h,8); PART(name,l); i32 loc=-1; recv_all(&loc,4); return loc; }
i32 glGetError(void){ rec(105,0); i32 e=0; recv_all(&e,4); return e; }
void glGetShaderiv(u32 s,u32 pn,i32 *p){ u32 v[2]={s,pn}; emit_iv(102,(i32*)v,2); i32 o=0; recv_all(&o,4); if(p)*p=o; }
void glGetProgramiv(u32 pr,u32 pn,i32 *p){ u32 v[2]={pr,pn}; emit_iv(103,(i32*)v,2); i32 o=0; recv_all(&o,4); if(p)*p=o; }
const char *glGetString(u32 name){ switch(name){ case 0x1F02: return "OpenGL ES 2.0 (moi3-fwd)";
    case 0x8B8C: return "OpenGL ES GLSL ES 1.00"; default: return "moi3-fwd"; } }

/* ---------------------------------------------------------------------------
 * TODO — restante da ABI (mecânico, copiar de gl_shim.c do fork):
 *   ops 14 glBindAttribLocation | 18 glBufferSubData | 22-29 uniform{f} |
 *   26/27 UniformMatrix{2,3}fv | 36-43 estado | 52/56 Tex{Sub}Image2D |
 *   57/58 mipmap/paramf | 60-79 FBO/renderbuffer/stencil | 80-87 uniform{i}/attrib |
 *   104/106/107/108/109 getters | 120 glReadPixels | 123/124 GetActive* |
 *   130 glDelete* | 110 glShaderBinary (pode virar no-op no MOI3).
 * E os entry points de extensão Adreno/QCOM (glExtGet*QCOM, glStartTilingQCOM, ...):
 *   no-op stub que avisa 1x — a HMI só precisa que o símbolo exista pra linkar.
 * Caminho de CLUSTER no MOI3: NÃO é o encoder do fork. É o stream H.264 multicast
 *   ff14::2:4 — capturar na rede do container e decodificar à parte (ver
 *   ../estudo-qemu-mhi2.md item 4). Os ops de vídeo 122/126/127/134-137 não se
 *   aplicam; o cluster sai fora da pilha GL.
 * ------------------------------------------------------------------------- */
