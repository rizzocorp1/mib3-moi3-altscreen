/* Regression: inspect actual mux output and faults after delayed ownership. */
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
static uint64_t fake_us=10000000,pts_seen[16],send_us[16],pcr_seen[16];static unsigned seen;
static int fail_allocation=-1,fail_send;
static int audit_sleep(const struct timespec *t,struct timespec *unused){(void)unused;fake_us+=(uint64_t)t->tv_sec*1000000+t->tv_nsec/1000;return 0;}
static void *audit_malloc(size_t n){if(fail_allocation==0)return NULL;if(fail_allocation>0)fail_allocation--;return malloc(n);}
static int audit_clock(clockid_t id,struct timespec *t){(void)id;t->tv_sec=fake_us/1000000;t->tv_nsec=(fake_us%1000000)*1000;return 0;}
static ssize_t audit_send(int fd,const void *bytes,size_t n,int flags,const struct sockaddr *a,socklen_t length){
 (void)fd;(void)flags;(void)a;(void)length;
 if(fail_send){errno=ENOBUFS;return -1;}
 const uint8_t *r=bytes;assert(n==1140);
 for(size_t i=12;i+188<=n;i+=188){const uint8_t *t=r+i;if((((t[1]&31)<<8)|t[2])!=17 || !(t[1]&64))continue;
  size_t o=4;if((t[3]&0x30)==0x30)o+=1+t[4];assert(o+14<=188);const uint8_t *p=t+o;assert(!p[0]&&!p[1]&&p[2]==1&&p[3]==0xe0);
  uint64_t pts=((uint64_t)((p[9]>>1)&7)<<30)|((uint64_t)p[10]<<22)|((uint64_t)(p[11]>>1)<<15)|((uint64_t)p[12]<<7)|(p[13]>>1);
  assert(seen<16);pts_seen[seen]=pts;send_us[seen]=fake_us;
  assert((t[3]&0x30)==0x30 && (t[5]&0x10));
  pcr_seen[seen]=((uint64_t)t[6]<<25)|((uint64_t)t[7]<<17)|((uint64_t)t[8]<<9)|((uint64_t)t[9]<<1)|(t[10]>>7);seen++;
 }return (ssize_t)n;
}
#define ALTSCREEN_TEST
#define malloc audit_malloc
#define clock_gettime audit_clock
#define sendto audit_send
#define nanosleep audit_sleep
#include "../deploy-usb/payload/altscreen_handler.c"
#undef malloc
#undef clock_gettime
#undef sendto
static const uint8_t idr[]={0,0,0,2,0x65,0xaa},dependent[]={0,0,0,2,0x41,0xbb};
static void reset(void){packet_pacer_reset();seen=0;fail_allocation=-1;fail_send=0;g_rtp_sock=99;g_rtp_owned=0;g_rtp_ts_in_buf=0;g_total_frames_sent=0;g_send_errors=0;g_video_discontinuity=1;g_wait_output_idr=1;g_output_failed=0;transport_clock_reset(&g_transport_clock);g_nal_length_bytes=4;g_sps_pps_len=12;memcpy(g_sps_pps_buf,"\0\0\0\1\x67\x42\0\x1f\0\0\0\1",12);}
static char lease_path[512];
static void fixture_lease(const char *dir){snprintf(lease_path,sizeof(lease_path),"%s/lease",dir);g_handoff_lease_path=lease_path;FILE *f=fopen(lease_path,"w");assert(f);fprintf(f,HANDOFF_TOKEN "%llu\n",(unsigned long long)(fake_us/1000+12000));fclose(f);}
int main(void){
 reset();char dir[]="/tmp/v25-audit-XXXXXX";assert(mkdtemp(dir));g_handoff_started=0;
 assert(deferred_video_submit(dir,idr,sizeof(idr),1000000,1,1,emit_h264_frame)==0);fake_us+=33333;
 assert(deferred_video_submit(dir,dependent,sizeof(dependent),1033333,0,1,emit_h264_frame)==0);fake_us+=33333;
 assert(deferred_video_submit(dir,dependent,sizeof(dependent),1066666,0,1,emit_h264_frame)==0);assert(g_deferred_count==3&&!seen);
 char grant[512],request[512];snprintf(grant,sizeof(grant),"%s/" HANDOFF_GRANT_FILE,dir);snprintf(request,sizeof(request),"%s/" HANDOFF_REQUEST_FILE,dir);
 FILE *f=fopen(grant,"w");assert(f);assert(fwrite(HANDOFF_TOKEN,1,sizeof(HANDOFF_TOKEN)-1,f)==sizeof(HANDOFF_TOKEN)-1);fclose(f);fixture_lease(dir);fake_us+=100000;
 assert(deferred_video_flush(dir,emit_h264_frame)==1);assert(seen==3&&g_total_frames_sent==3);assert(pts_seen[1]-pts_seen[0]>=2880 && pts_seen[1]-pts_seen[0]<=3060);assert(pts_seen[2]-pts_seen[1]>=2880 && pts_seen[2]-pts_seen[1]<=3060);assert(fake_us>=10233332);
 assert(send_us[1]-send_us[0]>=33333 && send_us[2]-send_us[1]>=33333);
 for(unsigned i=0;i<3;i++)assert(pts_seen[i]-pcr_seen[i]==9000);
 printf("PASS A1: buffered frames paced with source-spaced PTS=%llu,%llu,%llu with source-paced delivery and 100ms presentation lead\n",(unsigned long long)pts_seen[0],(unsigned long long)pts_seen[1],(unsigned long long)pts_seen[2]);unlink(grant);unlink(request);unlink(lease_path);rmdir(dir);
 reset();char stale[]="/tmp/v31-stale-XXXXXX";assert(mkdtemp(stale));g_handoff_started=0;
 assert(deferred_video_submit(stale,idr,sizeof(idr),1000000,1,1,emit_h264_frame)==0);
 snprintf(grant,sizeof(grant),"%s/" HANDOFF_GRANT_FILE,stale);snprintf(request,sizeof(request),"%s/" HANDOFF_REQUEST_FILE,stale);
 f=fopen(grant,"w");assert(f);fputs(HANDOFF_TOKEN,f);fclose(f);fixture_lease(stale);fake_us+=2000000;
 assert(deferred_video_flush(stale,emit_h264_frame)==0 && !seen && g_wait_output_idr && !g_deferred_count);
 assert(deferred_video_submit(stale,dependent,sizeof(dependent),3000000,0,1,emit_h264_frame)==0 && !seen);
 assert(deferred_video_submit(stale,idr,sizeof(idr),3033333,1,1,emit_h264_frame)==0 && seen==1);
 puts("PASS: stale startup GOP discarded as a whole; dependent pictures withheld until new IDR");
 fake_us+=600000000;assert(handoff_lifetime_expired());assert(frame_gate_at(stale,1,1)==-1);
 puts("PASS: receiver lifetime expires independently of a valid grant/hung monitor");
 unlink(grant);unlink(request);unlink(lease_path);rmdir(stale);
 reset();fail_allocation=0;assert(emit_h264_frame(idr,sizeof(idr),1000000)==-1&&g_total_frames_sent==0&&!seen);puts("PASS A2a: AnnexB allocation failure propagates");
 reset();fail_allocation=1;assert(emit_h264_frame(idr,sizeof(idr),1000000)==-1&&g_total_frames_sent==0&&!seen);puts("PASS A2b: PES allocation failure propagates");
 reset();fail_send=1;assert(emit_h264_frame(idr,sizeof(idr),1000000)==-1&&g_send_errors>0&&g_total_frames_sent==0&&g_wait_output_idr);puts("PASS A3: UDP failure propagates without false frame success");

 assert(emit_h264_frame(dependent,sizeof(dependent),1033333)==0 && g_total_frames_sent==0);
 fail_send=0;assert(emit_h264_frame(idr,sizeof(idr),1066666)==0 && g_total_frames_sent==1 && !g_wait_output_idr);
 puts("PASS: dependent frames withheld after output hole; fresh IDR recovers");
 reset();uint64_t a,b;assert(transport_picture_time(&g_transport_clock,1000000,&a,&b)==0);
 fake_us+=500000;assert(transport_picture_time(&g_transport_clock,1033333,&a,&b)==-1);
 puts("PASS: excessive lateness requests reference recovery instead of catch-up burst");
 reset();uint8_t config[]={1,66,0,30,0xff,0xe1,0,2,0x67,0x42,1,0,2,0x68,0xce};
 parse_and_store_avcc(config,sizeof(config));g_wait_output_idr=0;g_handoff_started=1;
 parse_and_store_avcc(config,sizeof(config));assert(!g_wait_output_idr && g_handoff_started);
 config[14]^=1;parse_and_store_avcc(config,sizeof(config));assert(g_wait_output_idr);
 config[0]=0;parse_and_store_avcc(config,sizeof(config));assert(!g_sps_pps_len);
 puts("PASS: identical decoder configuration preserves playback; changed/invalid config cannot continue dependent output");
 g_rtp_sock=-1;return 0;
}
