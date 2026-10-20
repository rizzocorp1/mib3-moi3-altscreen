/* kipc_bus.c — LD_PRELOAD que liga os nós kipc (/dev/csr leitura, /dev/css escrita)
 * ao broker kipc_hub via AF_UNIX. SEM libc: syscalls inline aarch64, pra carregar sob
 * qualquer glibc do firmware (o dlsym viraria dlsym@GLIBC_2.34, que a 2.17 não tem).
 * Estudo/sandbox, sem carro, sem secure boot.
 *
 * open("/dev/csr")->socket SEQPACKET pro hub, papel 'R'; open("/dev/css")-> papel 'W'.
 * ioctl(fd,0,desc):
 *   writer: len=*(u64*)(desc+0), type=*(u32*)(desc+0x10), payload=*(void**)(desc+0x18)
 *           -> envia [type:4][payload:len] pro hub. retorna 0.
 *   reader: recebe -> copia payload p/ *(desc+0x18), *(desc+0)=canal, *(desc+0x10)=len e
 *           retorna 0. Sem mensagem, espera por ela até ESPERA_NS e só então devolve
 *           -1/EAGAIN, sem tocar no descritor (ver o comentário no ioctl).
 */
typedef unsigned long u64; typedef long i64; typedef unsigned int u32;
static i64 sc(i64 n,i64 a,i64 b,i64 c,i64 d,i64 e,i64 f){
  register i64 x8 asm("x8")=n,x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c,
               x3 asm("x3")=d,x4 asm("x4")=e,x5 asm("x5")=f;
  asm volatile("svc #0":"+r"(x0):"r"(x8),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory");
  return x0;
}
#define SYS_openat 56
#define SYS_close  57
#define SYS_ioctl  29
#define SYS_socket 198
#define SYS_connect 203
#define SYS_sendto 206
#define SYS_recvfrom 207
#define SYS_ppoll  73
#define SYS_write  64
#define SYS_getpid 172
#define ATFDCWD   -100
#define AF_UNIX    1
#define SOCK_SEQPACKET 5
#define MSG_DONTWAIT 0x40
#define MSG_NOSIGNAL 0x4000
#define MAXMSG (0x6800 + 16)
/* 12/10/2026: quanto a leitura espera por uma mensagem antes de voltar vazia (100 ms). O laço de
 * leitura do firmware (frame::Frame::event_checker_kipc) não tem pausa: se a leitura volta sem
 * mensagem, ele chama de novo na hora. Com a leitura voltando no mesmo instante, cada processo
 * que carrega esta biblioteca girava sem parar e gastava um núcleo inteiro (medido: cinco núcleos
 * por central parada; 1,8 milhão de leituras vazias por segundo no cliente de teste, que imita o
 * laço). No carro a leitura espera: o csr_ioctl do kernel do firmware faz down_timeout(…, 500),
 * que com CONFIG_HZ=100 são 5 s, e devolve -1 se o prazo passa (lido por desmontagem; o
 * carifservice chama essa volta de "timeout"). Aqui o prazo é mais curto de propósito: é só
 * entre uma leitura e outra que o laço confere a batida de vida dele (passados mais de 15 s da
 * anterior) e a ordem de parar, e com 100 ms isso continua acontecendo na hora em que acontecia
 * quando a leitura não esperava. Medidas e leitura do firmware: modelagem-soc.md e traces/cpu_*. */
#define ESPERA_NS 100000000

static const char SOCK_PATH[] = "/run/kipc_hub.sock";
static char fdrole[4096];   /* 0=nao-kipc, 'R' reader, 'W' writer */

/* __errno_location: simbolo base (@GLIBC_2.17 no aarch64), sem versao nova.
 * Pass-throughs devem emular a libc: em erro, syscall devolve [-4095,-1];
 * a libc poe errno=-ret e retorna -1. Sem isso a JVM trata -2 como fd valido. */
extern int *__errno_location(void);
static i64 ret_errno(i64 r){
  if(r<0 && r>=-4095){ *__errno_location() = (int)(-r); return -1; }
  return r;
}

static void bcopy_(void*d,const void*s,u64 n){ char*a=d; const char*b=s; for(u64 i=0;i<n;i++) a[i]=b[i]; }
static int has_(const char*p,const char*n){
  if(!p) return 0;
  for(; *p; p++){ const char*a=p,*b=n; while(*a&&*b&&*a==*b){a++;b++;} if(!*b) return 1; }
  return 0;
}
static int slen_(const char*s){ int n=0; while(s[n]) n++; return n; }
static void wlog(const char*s){ sc(SYS_write,2,(i64)s,slen_(s),0,0,0); }

static int connect_hub(char role){
  int s = (int)sc(SYS_socket, AF_UNIX, SOCK_SEQPACKET, 0,0,0,0);
  if(s<0) return -1;
  /* sockaddr_un: u16 family + path[108] */
  char addr[112]; for(int i=0;i<112;i++) addr[i]=0;
  *(unsigned short*)addr = AF_UNIX;
  int i=0; for(; SOCK_PATH[i]; i++) addr[2+i]=SOCK_PATH[i];
  i64 r = sc(SYS_connect, s, (i64)addr, 2+i+1, 0,0,0);
  if(r<0){ sc(SYS_close,s,0,0,0,0,0); return -1; }
  /* handshake: [role:1][pid:4] — o hub nao reentrega msg de um writer aos readers do mesmo pid */
  char hs[5]; hs[0]=role; int pid=(int)sc(SYS_getpid,0,0,0,0,0,0);
  hs[1]=pid&0xff; hs[2]=(pid>>8)&0xff; hs[3]=(pid>>16)&0xff; hs[4]=(pid>>24)&0xff;
  sc(SYS_sendto, s, (i64)hs, 5, 0, 0, 0);
  return s;
}
static char kind_of(const char*p){
  if(!p) return 0;
  if(has_(p,"/dev/csr")) return 'R';
  if(has_(p,"/dev/css")) return 'W';
  return 0;
}
static int tagfd(int fd,char role){ if(fd>=0&&fd<4096) fdrole[fd]=role; return fd; }

int openat(int d,const char*p,int fl,unsigned m){
  char k=kind_of(p);
  if(k){ int s=connect_hub(k); if(s>=0){ wlog(k=='R'?"[kipc_bus] csr->hub\n":"[kipc_bus] css->hub\n"); return tagfd(s,k);} }
  return (int)ret_errno(sc(SYS_openat,d,(i64)p,fl,m,0,0));
}
int open(const char*p,int fl,unsigned m){ return openat(ATFDCWD,p,fl,m); }

int ioctl(int fd,u64 req,void*arg){
  char role = (fd>=0&&fd<4096)?fdrole[fd]:0;
  if(role && req==0 && arg){
    /* MsgPkt (ABI confirmado por RE do kipc::MsgPkt::MsgPkt / CommunicationService::read):
     *   word0 (0x00)=canal, word1 (0x08)=id, word2 (0x10)=len (uint), word3 (0x18)=ponteiro do payload.
     * Framing no barramento: [canal:8][payload:len] (len implicito = tam. do datagrama - 8). */
    unsigned char* dsc=(unsigned char*)arg;
    if(role=='W'){
      u64 chan=*(u64*)(dsc+0x00); u32 len=*(u32*)(dsc+0x10); void* pl=*(void**)(dsc+0x18);
      if(len>0x6800) len=0x6800;
      static char out[MAXMSG];
      *(u64*)out=chan;
      if(len&&pl) bcopy_(out+8,pl,len);
      sc(SYS_sendto, fd, (i64)out, 8+len, MSG_DONTWAIT|MSG_NOSIGNAL, 0, 0);
      return 0;
    } else {
      static char in[MAXMSG];
      i64 n=sc(SYS_recvfrom, fd,(i64)in,sizeof in, MSG_DONTWAIT, 0,0);
      if(n==-11){
        /* fila vazia: dorme até chegar mensagem, no máximo ESPERA_NS, e tenta de novo. O que
         * quem chamou recebe não muda: a mensagem, ou -1/EAGAIN se o prazo passou sem nenhuma. */
        struct { int fd; short events, revents; } pf = { fd, 1 /* POLLIN */, 0 };
        i64 prazo[2] = { 0, ESPERA_NS };
        sc(SYS_ppoll,(i64)&pf,1,(i64)prazo,0,0,0);
        n=sc(SYS_recvfrom, fd,(i64)in,sizeof in, MSG_DONTWAIT, 0,0);
      }
      if(n>=8){
        u64 chan=*(u64*)in; u64 len=(u64)n-8; void* pl=*(void**)(dsc+0x18);
        if(len>0x6800) len=0x6800;
        if(pl&&len) bcopy_(pl,in+8,len);
        *(u64*)(dsc+0x00)=chan; *(u32*)(dsc+0x10)=(u32)len;
        return 0;
      }
      /* sem mensagem: a volta e -1/EAGAIN, nao sucesso-vazio
       * (sucesso-vazio joga o libDSIService da HMI num terminate recursivo). */
      *__errno_location()=11; /* EAGAIN */
      return -1;
    }
  }
  return (int)ret_errno(sc(SYS_ioctl,fd,req,(i64)arg,0,0,0));
}
int close(int fd){ if(fd>=0&&fd<4096) fdrole[fd]=0; return (int)ret_errno(sc(SYS_close,fd,0,0,0,0,0)); }
