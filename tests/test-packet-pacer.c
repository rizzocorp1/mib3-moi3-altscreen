#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <errno.h>
static uint64_t fake_us=10000000,sent_at[10000],sleep_extra;static unsigned sends;
static uint8_t wire[2*1024*1024];static size_t wire_n;
static int clock_probe(clockid_t id,struct timespec *t){(void)id;t->tv_sec=fake_us/1000000;t->tv_nsec=(fake_us%1000000)*1000;return 0;}
static int sleep_probe(const struct timespec *t,struct timespec *r){(void)r;fake_us+=(uint64_t)t->tv_sec*1000000+t->tv_nsec/1000+sleep_extra;sleep_extra=0;return 0;}
static ssize_t send_probe(int f,const void *b,size_t n,int flags,const struct sockaddr *a,socklen_t l){
 (void)f;(void)flags;(void)a;(void)l;assert(n==1140 && sends<10000 && wire_n+n-12<=sizeof(wire));sent_at[sends++]=fake_us;memcpy(wire+wire_n,(const uint8_t*)b+12,n-12);wire_n+=n-12;return n;
}
#define ALTSCREEN_TEST
#define clock_gettime clock_probe
#define nanosleep sleep_probe
#define sendto send_probe
#include "../deploy-usb/payload/altscreen_handler.c"
static uint8_t idr[]={0,0,0,2,0x65,0xaa};
int main(void){
 g_rtp_sock=99;g_sps_pps_len=12;memcpy(g_sps_pps_buf,"\0\0\0\1\x67\x42\0\x1f\0\0\0\1",12);g_wait_output_idr=1;
 assert(emit_h264_frame(idr,sizeof(idr),1000000)==0);sends=0;wire_n=0;
 size_t n=112371;uint8_t *frame=malloc(n);assert(frame);memset(frame,0x55,n);uint32_t len=n-4;
 frame[0]=len>>24;frame[1]=len>>16;frame[2]=len>>8;frame[3]=len;frame[4]=0x41;
 assert(emit_h264_frame(frame,n,1033333)==0 && g_total_frames_sent==2 && !g_wait_output_idr);
 assert(sends>90 && sent_at[sends-1]-sent_at[0]>=20000 && sent_at[sends-1]-sent_at[0]<75000);
 unsigned burst=1,max=1;for(unsigned i=1;i<sends;i++){if(sent_at[i]==sent_at[i-1])burst++;else {assert(sent_at[i]-sent_at[i-1]>=912);burst=1;}if(burst>max)max=burst;}assert(max<=4);
 uint8_t pes[120000];size_t size=0;
 for(size_t i=0;i<wire_n;i+=188){uint8_t *t=wire+i;if((((t[1]&31)<<8)|t[2])!=17)continue;unsigned mode=(t[3]>>4)&3;size_t o=4;if(mode==3)o+=1+t[4];assert(o<=188);memcpy(pes+size,t+o,188-o);size+=188-o;}
 assert(size==n+14 && !memcmp(pes+14,"\0\0\0\1",4) && !memcmp(pes+18,frame+4,n-4));
 printf("PASS: 112371-byte dependent picture preserved byte-for-byte; %u datagrams spread over %llu us, max batch=%u\n",sends,(unsigned long long)(sent_at[sends-1]-sent_at[0]),max);
 unsigned before=sends;uint64_t muxed=g_total_frames_sent;sleep_extra=100000;
 assert(emit_h264_frame(frame,n,1066666)==-1 && g_wait_output_idr && g_total_frames_sent==muxed && sends-before<=4);
 assert(emit_h264_frame(frame,n,1099999)==0 && g_total_frames_sent==muxed);
 assert(emit_h264_frame(idr,sizeof(idr),1133333)==0 && g_total_frames_sent==muxed+1 && !g_wait_output_idr);
 puts("PASS: forced scheduler stall bounds partial output, reports failure, withholds dependent pictures and recovers on IDR");
 fake_us+=1000000;packet_pacer_reset();assert(packet_pacer_begin()==0);uint64_t start=fake_us;for(unsigned i=0;i<8;i++)assert(packet_pacer_wait(1140)==0);assert(fake_us-start==912);
 fake_us-=2000;assert(packet_pacer_wait(1140)==-1);free(frame);g_rtp_sock=-1;
 puts("PASS: idle does not accumulate catch-up burst credit; backwards clock fails closed");return 0;
}
