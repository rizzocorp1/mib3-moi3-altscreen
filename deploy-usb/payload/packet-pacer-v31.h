/* Smooth per-picture datagram bursts, not a claim about decoder bandwidth.
 * Forty Mbit/s is an experimental output ceiling above observed sustained load.
 * No catch-up bursts, sleeping across teardown for seconds, or silent AU loss. */
#ifndef MOI3_PACKET_PACER_V31_H
#define MOI3_PACKET_PACER_V31_H
#define PACKET_PACING_BITS_PER_SECOND 40000000ULL
static struct {uint64_t next_us,au_start_us,packets,wait_us,max_late_us,aborts;int active;unsigned batch_left;} g_packet_pacer;
static int packet_pacer_now(uint64_t *value){
 struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)!=0 || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000)return -1;
 if((uint64_t)t.tv_sec>UINT64_MAX/1000000)return -1;
 *value=(uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;return 0;
}
static void packet_pacer_reset(void){memset(&g_packet_pacer,0,sizeof(g_packet_pacer));}
static int packet_pacer_begin(void){
 if(packet_pacer_now(&g_packet_pacer.au_start_us)<0)return -1;
 g_packet_pacer.active=1;return 0;
}
static int packet_pacer_wait(size_t bytes){
 uint64_t now;if(packet_pacer_now(&now)<0)return -1;
 uint64_t begin=now;
 if(g_packet_pacer.active && (now<g_packet_pacer.au_start_us || now-g_packet_pacer.au_start_us>75000)){
  g_packet_pacer.aborts++;return -1;
 }
 while(!g_packet_pacer.batch_left && now<g_packet_pacer.next_us){
  uint64_t wait=g_packet_pacer.next_us-now;
  if(wait>2000)return -1;
  struct timespec pause={0,(long)(wait*1000)};
  if(nanosleep(&pause,NULL)!=0 && errno!=EINTR)return -1;
  uint64_t next;if(packet_pacer_now(&next)<0 || next<now)return -1;now=next;
 }
 if(g_packet_pacer.active && now-g_packet_pacer.au_start_us>75000){g_packet_pacer.aborts++;return -1;}
 if(g_packet_pacer.next_us && now>g_packet_pacer.next_us && now-g_packet_pacer.next_us<10000 && now-g_packet_pacer.next_us>g_packet_pacer.max_late_us)g_packet_pacer.max_late_us=now-g_packet_pacer.next_us;
 uint64_t interval=(bytes*8000000ULL+PACKET_PACING_BITS_PER_SECOND-1)/PACKET_PACING_BITS_PER_SECOND;
 if(now>=g_packet_pacer.next_us){g_packet_pacer.next_us=now+4*interval;g_packet_pacer.batch_left=4;}
 if(!g_packet_pacer.batch_left)return -1;
 g_packet_pacer.batch_left--;g_packet_pacer.wait_us+=now-begin;g_packet_pacer.packets++;return 0;
}
#endif
