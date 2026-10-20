/* kipc_hub.c — broker de mensagens kipc para o sandbox (estudo, sem carro).
 * Emula o barramento que o char device /dev/csr (leitura) + /dev/css (escrita)
 * provê no HW real: um hub que recebe de qualquer "writer" (css) e difunde para
 * todos os "readers" (csr). Assim os daemons (carifservice/persistence-app) e a
 * HMI veem as registrações DSI uns dos outros — o handshake real, não stub de dado.
 *
 * Protocolo com o shim (kipc_bus.c), sobre AF_UNIX SOCK_SEQPACKET em /run/kipc_hub.sock:
 *  - cliente conecta e manda 1 byte de papel: 'R' (reader/csr) ou 'W' (writer/css).
 *  - writer envia 1 datagrama por mensagem: [type:4 LE][payload...]; o hub difunde
 *    o datagrama idêntico para todos os readers.
 *  - reader que se liga depois recebe, nesta ordem: o que veio do microcontrolador de
 *    diagnóstico, as NCACHE primeiras mensagens comuns e o último anúncio de cada canal de que
 *    essas primeiras não trouxeram anúncio (ver IF_ANUNCIO, abaixo).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#define SOCK_PATH "/run/kipc_hub.sock"
#define MAXMSG (0x6800 + 16)
#define MAXCLI 256
/* Marco 6 (11/10/2026): canal do microcontrolador de diagnóstico (PID_DIAG_MCU). No carro ele
 * manda a codificação e a adaptação logo na partida, e a HMI só as aceita ANTES da primeira
 * leitura de persistência. Aqui o que chega nesse canal fica guardado à parte e é entregue a
 * cada leitor novo antes do cache comum; mandar de novo um identificador troca o valor guardado.
 * Carga: [3 bytes][identificador:2][dados]. Ver diagstub.c. */
#define PID_DIAG_MCU 0x0800000000000000ULL
#define NRET 128
#define RETMAX (8 + 5 + 4096)
/* 11/10/2026: anúncio de domínio, a mensagem [canal:8][interface 9000:4]… ("WELCOME") com que um
 * fornecedor DSI diz que está no ar; é ela que faz a HMI ligar os serviços daquele domínio. O
 * cache das NCACHE primeiras mensagens não basta para ela: o carifservice do firmware anuncia o
 * domínio 16 como 61ª mensagem, depois de mais de cinquenta mensagens próprias, e a HMI, que se
 * liga depois, nunca via o anúncio (Marco 8). Por isso o ÚLTIMO anúncio de cada canal fica
 * guardado à parte e vai para o leitor novo depois do cache. Um por canal, e só se o cache não
 * tiver acabado de entregar um anúncio desse canal: a HMI não pode receber dois anúncios do
 * mesmo domínio de uma vez (cada anúncio do domínio 16 agenda nela a criação de dez serviços,
 * sem trava). O que o cache já entregava continua igual. NANU canais: o canal DSI é um bit de
 * uma palavra de 64; passando disso, os canais novos ficam sem reentrega, como era antes. */
#define IF_ANUNCIO 9000
#define NANU 64

int main(void){
  unlink(SOCK_PATH);
  int ls = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if(ls<0){ perror("socket"); return 1; }
  struct sockaddr_un a; memset(&a,0,sizeof a); a.sun_family=AF_UNIX;
  strncpy(a.sun_path, SOCK_PATH, sizeof a.sun_path-1);
  if(bind(ls,(struct sockaddr*)&a,sizeof a)<0){ perror("bind"); return 1; }
  if(listen(ls,64)<0){ perror("listen"); return 1; }
  fprintf(stderr,"[kipc_hub] ouvindo em %s\n", SOCK_PATH);

  struct pollfd pfd[MAXCLI]; char role[MAXCLI]; int cpid[MAXCLI];
  int nc=0; pfd[nc].fd=ls; pfd[nc].events=POLLIN; role[nc]='L'; cpid[nc]=0; nc++;
  static char buf[MAXMSG];
  long nmsg=0;
  long maxfwd=0; { const char*e=getenv("HUB_MAXFWD"); if(e) maxfwd=atol(e); }
  if(maxfwd) fprintf(stderr,"[kipc_hub] HUB_MAXFWD=%ld (nao encaminha msgs maiores)\n",maxfwd);
  /* cache dos primeiros anuncios p/ reentregar a readers que conectam depois
   * (a HMI sobe ~15s depois dos daemons; sem replay ela perde o WELCOME). */
#define NCACHE 48
  static char cache[NCACHE][MAXMSG]; static int clen[NCACHE]; static int cpido[NCACHE]; int ncache=0;
  static char ret[NRET][RETMAX]; static int rlen[NRET]; int nret=0;   /* guardados do PID_DIAG_MCU */
  static char anu[NANU][MAXMSG]; static int alen[NANU]; static int apid[NANU];   /* o último anúncio de cada canal */
  static unsigned long long acan[NANU]; int nanu=0;
  for(;;){
    if(poll(pfd,nc,-1)<0){ if(errno==EINTR) continue; break; }
    for(int i=0;i<nc;i++){
      if(!(pfd[i].revents & (POLLIN|POLLHUP|POLLERR))) continue;
      if(role[i]=='L'){
        int cs=accept(ls,0,0); if(cs<0) continue;
        char hs[5]={0}; recv(cs,hs,5,0);
        int pid = (unsigned char)hs[1] | ((unsigned char)hs[2]<<8) | ((unsigned char)hs[3]<<16) | ((unsigned char)hs[4]<<24);
        /* revents=0: a vaga pode ter sido de um cliente que saiu, e o laço abaixo ainda passa por
         * ela nesta volta; com o revents antigo o hub fechava o cliente novo ou travava num recv */
        if(nc<MAXCLI){ pfd[nc].fd=cs; pfd[nc].events=POLLIN; pfd[nc].revents=0; role[nc]=(hs[0]=='W'?'W':'R'); cpid[nc]=pid; nc++;
          fprintf(stderr,"[kipc_hub] +cliente fd=%d papel=%c pid=%d (total=%d)\n",cs,role[nc-1],pid,nc-1);
          if(role[nc-1]=='R'){ /* reentrega o cache de anuncios (menos os do proprio pid) */
            int rep=0;
            /* primeiro o que o microcontrolador de diagnostico mandou; quem mandou ja pode ter
             * saido e o pid dele ter sido reaproveitado, entao aqui nao se filtra por pid */
            for(int k=0;k<nret;k++) send(cs,ret[k],rlen[k],MSG_DONTWAIT|MSG_NOSIGNAL);
            if(nret) fprintf(stderr,"[kipc_hub]   (entregou %d conjuntos de diagnostico ao novo reader)\n",nret);
            for(int k=0;k<ncache;k++){ if(cpido[k]!=pid){ send(cs,cache[k],clen[k],MSG_DONTWAIT|MSG_NOSIGNAL); rep++; } }
            if(rep) fprintf(stderr,"[kipc_hub]   (reentregou %d anuncios ao novo reader)\n",rep);
            /* por fim os anúncios guardados, na ordem em que cada canal anunciou pela primeira vez;
             * fica de fora o canal de que o cache acabou de entregar um anúncio a este leitor */
            int ranu=0;
            for(int a=0;a<nanu;a++){
              int veio=0;
              if(apid[a]==pid) continue;
              for(int k=0;k<ncache && !veio;k++){
                unsigned long long c; unsigned f;
                if(cpido[k]==pid) continue;
                memcpy(&c,cache[k],8); memcpy(&f,cache[k]+8,4);
                if(c==acan[a] && f==IF_ANUNCIO) veio=1;
              }
              if(!veio){ send(cs,anu[a],alen[a],MSG_DONTWAIT|MSG_NOSIGNAL); ranu++; }
            }
            if(ranu) fprintf(stderr,"[kipc_hub]   (entregou %d anuncios de dominio guardados ao novo reader)\n",ranu);
          }
        }
        else close(cs);
        continue;
      }
      /* quem manda e fecha em seguida (o diagstub) deixa mensagens na fila junto com o HUP:
       * enquanto houver o que ler, le; o recv devolve 0 quando a fila acaba e ai sim fecha */
      if((pfd[i].revents & (POLLHUP|POLLERR)) && !(pfd[i].revents & POLLIN)){
        close(pfd[i].fd); for(int j=i;j<nc-1;j++){pfd[j]=pfd[j+1];role[j]=role[j+1];cpid[j]=cpid[j+1];} nc--; i--; continue;
      }
      /* dados de um cliente */
      ssize_t n=recv(pfd[i].fd,buf,sizeof buf,0);
      if(n<=0){ close(pfd[i].fd); for(int j=i;j<nc-1;j++){pfd[j]=pfd[j+1];role[j]=role[j+1];cpid[j]=cpid[j+1];} nc--; i--; continue; }
      if(role[i]=='W'){
        nmsg++;
        /* HUB_DUMP=n (11/10/2026): despeja em hexa os primeiros 40 bytes das n primeiras mensagens */
        { static long dump=-1; if(dump<0){ const char*e=getenv("HUB_DUMP"); dump=e?atol(e):0; }
          if(nmsg<=dump){ char hx[3*40+1]; int k,m=n<40?(int)n:40; for(k=0;k<m;k++) sprintf(hx+3*k,"%02x ",(unsigned char)buf[k]);
            fprintf(stderr,"[dump] #%ld pid=%d len=%zd: %s\n",nmsg,cpid[i],n,hx); } }
        if(nmsg<=30 || (nmsg%500)==0){
          unsigned t = n>=4 ? *(unsigned*)buf : 0;
          fprintf(stderr,"[kipc_hub] msg#%ld de fd=%d len=%zd type=0x%x -> difundindo\n",nmsg,pfd[i].fd,n,t);
        }
        /* Guard anti-crash: o event_handler da HMI tira header de ate 0xc bytes do payload e
         * passa o resto como msgSize p/ NewByteArray(); payload < 0xc vira msgSize negativo =
         * SIGSEGV na JVM. Framing no barramento = [canal:8][payload], entao exijo n >= 8+0xc.
         * Tambem nao cacheia o curto, p/ o replay nao reintroduzir o crash. */
        int payload = (int)n - 8;
        unsigned long long chan = 0; if(n>=8) memcpy(&chan,buf,8);
        if(chan==PID_DIAG_MCU){
          /* A libDSIService entrega a carga deste canal inteira (interface 1901), sem tirar
           * cabecalho, entao o limite de 0xc nao vale aqui. O minimo e 5 (3 bytes + identificador):
           * com menos o DiagHandler do persistence-app lanca excecao. */
          if(payload >= 5 && n <= RETMAX){
            int slot=-1;
            for(int k=0;k<nret;k++) if(ret[k][11]==buf[11] && ret[k][12]==buf[12]){ slot=k; break; }
            if(slot<0 && nret<NRET) slot=nret++;
            if(slot>=0){ memcpy(ret[slot],buf,n); rlen[slot]=(int)n; }
            for(int j=0;j<nc;j++) if(role[j]=='R' && cpid[j]!=cpid[i]) send(pfd[j].fd,buf,n,MSG_DONTWAIT|MSG_NOSIGNAL);
          }
        }
        else if(payload < 0xc){ /* descarta mensagem curta demais p/ ser evento DSI valido */ }
        else if(maxfwd && (long)n>maxfwd){ /* HUB_MAXFWD: pula msgs grandes (knob de teste) */ }
        else {
          if(ncache<NCACHE){ memcpy(cache[ncache],buf,n); clen[ncache]=n; cpido[ncache]=cpid[i]; ncache++; }
          /* anúncio de domínio: o último de cada canal fica guardado, caiba ou não no cache */
          unsigned ifc; memcpy(&ifc,buf+8,4);
          if(ifc==IF_ANUNCIO){
            int slot=-1;
            for(int k=0;k<nanu;k++) if(acan[k]==chan){ slot=k; break; }
            if(slot<0 && nanu<NANU) slot=nanu++;
            if(slot>=0){ memcpy(anu[slot],buf,n); alen[slot]=(int)n; apid[slot]=cpid[i]; acan[slot]=chan; }
          }
          for(int j=0;j<nc;j++) if(role[j]=='R' && cpid[j]!=cpid[i]) send(pfd[j].fd,buf,n,MSG_DONTWAIT|MSG_NOSIGNAL);
        }
      }
    }
  }
  return 0;
}
