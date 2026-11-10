/* Audit reproductions against unchanged V25. A passing run confirms the
 * stated defects exist; it is not a correctness test or a proposed fix. */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>
static uint64_t fake_us=10000000,pts_seen[16];static unsigned seen;
static int fail_allocation=-1,fail_send;
static void *audit_malloc(size_t n){if(fail_allocation==0)return NULL;if(fail_allocation>0)fail_allocation--;return malloc(n);}
static int audit_clock(clockid_t id,struct timespec *t){(void)id;t->tv_sec=fake_us/1000000;t->tv_nsec=(fake_us%1000000)*1000;return 0;}
static ssize_t audit_send(int fd,const void *bytes,size_t n,int flags,const struct sockaddr *a,socklen_t length){
 (void)fd;(void)flags;(void)a;(void)length;
 if(fail_send){errno=ENOBUFS;return -1;}
 const uint8_t *r=bytes;assert(n==1140);
 for(size_t i=12;i+188<=n;i+=188){const uint8_t *t=r+i;if((((t[1]&31)<<8)|t[2])!=17 || !(t[1]&64))continue;
  size_t o=4;if((t[3]&0x30)==0x30)o+=1+t[4];assert(o+14<=188);const uint8_t *p=t+o;assert(!p[0]&&!p[1]&&p[2]==1&&p[3]==0xe0);
  uint64_t pts=((uint64_t)((p[9]>>1)&7)<<30)|((uint64_t)p[10]<<22)|((uint64_t)(p[11]>>1)<<15)|((uint64_t)p[12]<<7)|(p[13]>>1);
  assert(seen<16);pts_seen[seen++]=pts;
 }return (ssize_t)n;
}
#define ALTSCREEN_TEST
#define malloc audit_malloc
#define clock_gettime audit_clock
#define sendto audit_send
#include "../deploy-usb/payload/altscreen_handler.c"
#undef malloc
#undef clock_gettime
#undef sendto
static const uint8_t idr[]={0,0,0,2,0x65,0xaa},dependent[]={0,0,0,2,0x41,0xbb};
static void reset(void){seen=0;fail_allocation=-1;fail_send=0;g_rtp_sock=99;g_rtp_owned=0;g_rtp_ts_in_buf=0;g_total_frames_sent=0;g_send_errors=0;g_video_discontinuity=1;transport_clock_reset(&g_transport_clock);g_nal_length_bytes=4;g_sps_pps_len=12;memcpy(g_sps_pps_buf,"\0\0\0\1\x67\x42\0\x1f\0\0\0\1",12);}
int main(void){
 reset();char dir[]="/tmp/v25-audit-XXXXXX";assert(mkdtemp(dir));g_handoff_started=0;
 assert(deferred_video_submit(dir,idr,sizeof(idr),1000000,1,1,emit_h264_frame)==0);fake_us+=33333;
 assert(deferred_video_submit(dir,dependent,sizeof(dependent),1033333,0,1,emit_h264_frame)==0);fake_us+=33333;
 assert(deferred_video_submit(dir,dependent,sizeof(dependent),1066666,0,1,emit_h264_frame)==0);assert(g_deferred_count==3&&!seen);
 char grant[512],request[512];snprintf(grant,sizeof(grant),"%s/" HANDOFF_GRANT_FILE,dir);snprintf(request,sizeof(request),"%s/" HANDOFF_REQUEST_FILE,dir);
 FILE *f=fopen(grant,"w");assert(f);assert(fwrite(HANDOFF_TOKEN,1,sizeof(HANDOFF_TOKEN)-1,f)==sizeof(HANDOFF_TOKEN)-1);fclose(f);fake_us+=1000000;
 assert(deferred_video_flush(dir,emit_h264_frame)==1);assert(seen==3&&g_total_frames_sent==3);assert(pts_seen[0]==pts_seen[1]&&pts_seen[1]==pts_seen[2]);
 printf("REPRODUCED A1: three buffered units with 33333us source spacing get PTS=%llu,%llu,%llu when drained in one millisecond\n",(unsigned long long)pts_seen[0],(unsigned long long)pts_seen[1],(unsigned long long)pts_seen[2]);unlink(grant);unlink(request);rmdir(dir);
 reset();fail_allocation=0;assert(emit_h264_frame(idr,sizeof(idr),1000000)==0&&g_total_frames_sent==0&&!seen);puts("REPRODUCED A2a: AnnexB allocation failure returns success while dropping the access unit");
 reset();fail_allocation=1;assert(emit_h264_frame(idr,sizeof(idr),1000000)==0&&g_total_frames_sent==0&&!seen);puts("REPRODUCED A2b: PES allocation failure is silently propagated as successful emission");
 reset();fail_send=1;assert(emit_h264_frame(idr,sizeof(idr),1000000)==0&&g_send_errors>0);puts("REPRODUCED A3: UDP failure increments a counter but emission still reports success to the GOP queue");
 g_rtp_sock=-1;return 0;
}
