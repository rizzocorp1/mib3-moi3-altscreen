#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
static int receiver;static unsigned packets;static uint64_t first_ns,last_ns;static uint8_t received[262144];static size_t used_bytes;
static void *collect(void *unused){
 (void)unused;struct timeval timeout={1,0};assert(setsockopt(receiver,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0);
 unsigned expected=0;
 for(;;){uint8_t buffer[2048],control[256];struct iovec iov={buffer,sizeof(buffer)};struct msghdr msg={0};msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control;msg.msg_controllen=sizeof(control);
  ssize_t n=recvmsg(receiver,&msg,0);if(n<0){assert(errno==EAGAIN || errno==EWOULDBLOCK);break;}assert(n==1140 && !(msg.msg_flags&MSG_TRUNC));
  unsigned seq=((unsigned)buffer[2]<<8)|buffer[3];assert(seq==expected++);uint64_t stamp=0;
  for(struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c))if(c->cmsg_level==SOL_SOCKET && c->cmsg_type==SO_TIMESTAMPNS){struct timespec t;memcpy(&t,CMSG_DATA(c),sizeof(t));stamp=(uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
  assert(stamp);if(!packets)first_ns=stamp;last_ns=stamp;packets++;
  for(size_t i=12;i+188<=(size_t)n;i+=188){uint8_t *t=buffer+i;unsigned pid=((t[1]&31)<<8)|t[2];if(pid!=17)continue;size_t o=4;if(t[3]&0x20)o+=1+t[4];assert(o<=188 && used_bytes+188-o<=sizeof(received));memcpy(received+used_bytes,t+o,188-o);used_bytes+=188-o;}
 }
 return NULL;
}
int main(void){
 receiver=socket(AF_INET6,SOCK_DGRAM,0);assert(receiver>=0);int one=1,capacity=1048576;assert(setsockopt(receiver,SOL_SOCKET,SO_TIMESTAMPNS,&one,sizeof(one))==0);assert(setsockopt(receiver,SOL_SOCKET,SO_RCVBUF,&capacity,sizeof(capacity))==0);
 struct sockaddr_in6 to={.sin6_family=AF_INET6,.sin6_addr=IN6ADDR_LOOPBACK_INIT};assert(bind(receiver,(struct sockaddr*)&to,sizeof(to))==0);socklen_t size=sizeof(to);assert(getsockname(receiver,(struct sockaddr*)&to,&size)==0);
 g_rtp_sock=socket(AF_INET6,SOCK_DGRAM,0);assert(g_rtp_sock>=0);g_cluster_dest_addr=to;
 pthread_t worker;assert(pthread_create(&worker,NULL,collect,NULL)==0);
 size_t n=112371;uint8_t *frame=malloc(n);assert(frame);memset(frame,0x55,n);frame[0]=0x65;
 assert(packet_pacer_begin()==0);assert(mux_and_stream_h264_frame(frame,n,99000,1,90000)==0);g_packet_pacer.active=0;
 assert(pthread_join(worker,NULL)==0);assert(packets==g_sent_packets && packets>90);
 assert(last_ns-first_ns>=20000000 && last_ns-first_ns<75000000);
 assert(used_bytes==n+14 && !memcmp(received+14,frame,n));
 printf("PASS: real IPv6 UDP and kernel receive timestamps, %u/%llu datagrams, 112371-byte picture exact, spread %.3f ms\n",packets,(unsigned long long)g_sent_packets,(last_ns-first_ns)/1000000.0);
 close(receiver);close(g_rtp_sock);g_rtp_sock=-1;free(frame);return 0;
}
