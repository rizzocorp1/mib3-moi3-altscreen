/* kipc_shim.c — LD_PRELOAD de estudo.
 * Faz ioctl(req=0) nos nós kipc (/dev/csr, /dev/css) retornar 0 (sucesso falso)
 * e loga os primeiros bytes do MsgPkt, pra revelar o wire do kipc.
 * Sem libc: syscalls inline aarch64 (compatível com qualquer glibc do firmware).
 * Escopo: sandbox de estudo, sem carro. Não toca secure boot.
 */
typedef unsigned long u64; typedef long i64;
static i64 sc6(i64 n,i64 a,i64 b,i64 c,i64 d,i64 e,i64 f){
  register i64 x8 asm("x8")=n,x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c,
               x3 asm("x3")=d,x4 asm("x4")=e,x5 asm("x5")=f;
  asm volatile("svc #0":"+r"(x0):"r"(x8),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory");
  return x0;
}
#define SYS_openat 56
#define SYS_ioctl  29
#define SYS_write  64
#define SYS_close  57
#define ATFDCWD   -100

static int kfds[256]; static int nk=0;      /* qualquer no kipc */
static int rfds[256]; static int nr=0;      /* so o lado de leitura (/csr) */
static int isk(int fd){ for(int i=0;i<nk;i++) if(kfds[i]==fd) return 1; return 0; }
static int isr(int fd){ for(int i=0;i<nr;i++) if(rfds[i]==fd) return 1; return 0; }
static void addk(int fd){ if(nk<256) kfds[nk++]=fd; }
static void addr(int fd){ if(nr<256) rfds[nr++]=fd; }
static int g_logs=0;
static int contains(const char*p,const char*n){
  if(!p) return 0;
  for(; *p; p++){ const char*a=p,*b=n; while(*a&&*b&&*a==*b){a++;b++;} if(!*b) return 1; }
  return 0;
}
static void wr(const char*s,int n){ sc6(SYS_write,2,(i64)s,n,0,0,0); }
static void dump(unsigned char*p){
  if(!p) return; static const char h[]="0123456789abcdef"; char b[200]; int o=0;
  const char*t="[kipc msgpkt] "; while(*t) b[o++]=*t++;
  for(int i=0;i<48;i++){ b[o++]=h[p[i]>>4]; b[o++]=h[p[i]&15]; if((i&3)==3) b[o++]=' '; }
  b[o++]='\n'; wr(b,o);
}
static void tag(const char*p,int fd){
  if(fd<0||!p) return;
  if(contains(p,"/dev/csr")){ addk(fd); addr(fd); }       /* leitura */
  else if(contains(p,"/dev/css")){ addk(fd); }            /* escrita */
}
int openat(int d,const char*p,int fl,unsigned m){ i64 r=sc6(SYS_openat,d,(i64)p,fl,m,0,0); tag(p,(int)r); return (int)r; }
int open(const char*p,int fl,unsigned m){ i64 r=sc6(SYS_openat,ATFDCWD,(i64)p,fl,m,0,0); tag(p,(int)r); return (int)r; }
int ioctl(int fd,u64 req,void*arg){
  if(isk(fd) || req==0){
    if(g_logs<6){ g_logs++; wr("[kipc ioctl->0]\n",16); dump((unsigned char*)arg); }
    /* RE de kipc::CommunicationService::read: após o ioctl o binário lê
       descritor+0x00 = tamanho recebido, +0x10 = tipo, +0x18 = payload.
       Para "sem mensagem" (poll limpo, sem corromper heap) basta zerar +0x00 e +0x10
       — SÓ esses campos (zerar o +0x18/payload ptr às cegas = crash). */
    if(isr(fd) && arg){
      unsigned char* d=(unsigned char*)arg;
      *(unsigned long*)(d+0x00)=0UL;   /* len recebido = 0 */
      *(unsigned int*)(d+0x10)=0U;     /* tipo = 0 */
    }
    return 0;
  }
  return (int)sc6(SYS_ioctl,fd,req,(i64)arg,0,0,0);
}
int close(int fd){
  for(int i=0;i<nk;i++) if(kfds[i]==fd) kfds[i]=-1;
  return (int)sc6(SYS_close,fd,0,0,0,0,0);
}
