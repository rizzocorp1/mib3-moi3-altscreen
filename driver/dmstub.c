/* dmstub.c — provedor DSI fake para o sandbox (estudo, sem carro).
 * Conecta no kipc_hub e ANUNCIA o serviço DSIDisplayManagement (domínio
 * PID_DISPLAY_MGR=0x4000) com interfaceId=9000, que faz a HMI chamar
 * ServiceManager.DSIAdmin.startService(DSIDisplayManagement) e sair do
 * "Required DSI services not registered". Também responde getDisplayExtents/
 * getAvailableDisplays com extents fixos (resolução da central), pra HMI
 * calcular o layout das áreas de tela.
 *
 * Framing do barramento (igual kipc_bus): datagrama = [canal:8][payload].
 * Payload de domínio 0x4000 (visto no event_handler): [interfaceId:4][instanceId:4][apiID:4][data...].
 * Estático (-static): roda sob qualquer glibc, dentro do chroot.
 *
 * args: [sock_path] [width] [height]   (default /run/kipc_hub.sock 1920 720)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#define PID_DISPLAY_MGR 0x4000ULL
#define IFID_REGISTER 9000

static int connect_hub(const char* path, char role){
  int s = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if(s<0) return -1;
  struct sockaddr_un a; memset(&a,0,sizeof a); a.sun_family=AF_UNIX;
  strncpy(a.sun_path, path, sizeof a.sun_path-1);
  if(connect(s,(struct sockaddr*)&a,sizeof a)<0){ close(s); return -1; }
  char hs[5]; hs[0]=role; int pid=getpid();
  hs[1]=pid; hs[2]=pid>>8; hs[3]=pid>>16; hs[4]=pid>>24;
  send(s,hs,5,0);
  return s;
}
/* monta [canal:8][interfaceId:4][instanceId:4][apiID:4][data...] e envia */
static void send_msg(int w, uint64_t chan, uint32_t ifid, uint32_t inst, uint32_t api,
                     const unsigned char* data, int dlen){
  unsigned char buf[8+12+256]; int o=0;
  memcpy(buf+o,&chan,8); o+=8;
  memcpy(buf+o,&ifid,4); o+=4;
  memcpy(buf+o,&inst,4); o+=4;
  memcpy(buf+o,&api,4);  o+=4;
  if(dlen>0 && data){ memcpy(buf+o,data,dlen); o+=dlen; }
  send(w, buf, o, MSG_NOSIGNAL);
}
/* protobuf: int32 varint no campo field (wire type 0) */
static int pb_int32(unsigned char* b, int field, uint32_t v){
  int o=0; b[o++]=(field<<3)|0;          /* tag */
  do { unsigned char x=v&0x7f; v>>=7; if(v) x|=0x80; b[o++]=x; } while(v);
  return o;
}

int main(int argc, char** argv){
  const char* path = argc>1 ? argv[1] : "/run/kipc_hub.sock";
  uint32_t W_ = argc>2 ? (uint32_t)atoi(argv[2]) : 1920;
  uint32_t H_ = argc>3 ? (uint32_t)atoi(argv[3]) : 1080;
  int W = connect_hub(path,'W');
  int R = connect_hub(path,'R');
  if(W<0){ fprintf(stderr,"[dmstub] sem hub em %s\n",path); return 1; }
  fprintf(stderr,"[dmstub] conectado (W=%d R=%d); DSIDisplayManagement dom=0x4000, extents=%ux%u\n",W,R,W_,H_);
  /* getDisplaySizeResult: campo1=displayID(omit 0), campo2=width, campo3=height, campo4=result(omit 0) */
  unsigned char pb[32]; int pbl=0;
  pbl+=pb_int32(pb+pbl,2,W_);
  pbl+=pb_int32(pb+pbl,3,H_);
  int ticks=0;
  for(;;){
    /* 1) registro do serviço (ifid 9000 -> startService(DSIDisplayManagement)) */
    send_msg(W, PID_DISPLAY_MGR, IFID_REGISTER, 0, 0, NULL, 0);
    /* 2) resposta getDisplaySizeResult (apiId 2029): ifid!=9000, inst=0, data=protobuf.
     *    O ASL (GetDisplayExtentsOperation) chama getDisplaySize(1043) e espera isto. */
    send_msg(W, PID_DISPLAY_MGR, 1, 0, 2029, pb, pbl);
    if(ticks<5) fprintf(stderr,"[dmstub] tick#%d: registro + getDisplaySizeResult(%ux%u, %dB pb)\n",ticks,W_,H_,pbl);
    ticks++;
    struct pollfd p={R,POLLIN,0};
    if(R>=0 && poll(&p,1,1000)>0 && (p.revents&POLLIN)){
      unsigned char in[0x6808]; ssize_t n=recv(R,in,sizeof in,0);
      if(n>=20){
        uint64_t ch; memcpy(&ch,in,8);
        /* loga requests vindos no domínio do display (canal 0x4000 seria eco; a HMI manda no PID dela) */
        if(ticks<60 && ch!=PID_DISPLAY_MGR)
          fprintf(stderr,"[dmstub] rx canal=0x%llx len=%zd ifid=%u api=%u\n",
            (unsigned long long)ch,n-8,*(uint32_t*)(in+8),*(uint32_t*)(in+16));
      }
    } else { sleep(1); }
  }
  return 0;
}
