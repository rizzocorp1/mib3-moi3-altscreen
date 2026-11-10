/* Local enqueue and packetization timestamps share one source epoch.
 * Matches the factory's millisecond quantization; never uses phone absolute time.
 * A backwards/failing clock drops output instead of fabricating a timestamp. */
#ifndef MOI3_TRANSPORT_CLOCK_V27_H
#define MOI3_TRANSPORT_CLOCK_V27_H
#include <stdint.h>
#include <string.h>
#include <time.h>
typedef struct { uint64_t epoch_ms,last_ms,source_us,due_us,last_source_us;int started,picture_started; } transport_clock_v27;
static int transport_clock_at(transport_clock_v27 *c,uint64_t now_ms,uint64_t *ticks) {
    if(c->started && now_ms<c->last_ms)return -1;
    if(!c->started){c->epoch_ms=now_ms;c->started=1;}
    uint64_t elapsed=now_ms-c->epoch_ms;
    if(elapsed>UINT64_MAX/90-1000)return -1;
    c->last_ms=now_ms;*ticks=(1000+elapsed)*90;return 0;
}
static int transport_clock_now(transport_clock_v27 *c,uint64_t *ticks) {
    struct timespec now;
    if(clock_gettime(CLOCK_MONOTONIC,&now)!=0 || now.tv_sec<0 || now.tv_nsec<0 || now.tv_nsec>=1000000000)return -1;
    if((uint64_t)now.tv_sec>UINT64_MAX/1000)return -1;
    uint64_t ms=(uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
    return transport_clock_at(c,ms,ticks);
}
static void transport_clock_reset(transport_clock_v27 *c){memset(c,0,sizeof(*c));}
/* One source-picture epoch starts only when the first valid IDR can be sent.
 * Pace bunched input in source order; PCR/RTP remain local monotonic clocks.
 * Excessive lateness forces reference recovery rather than arbitrary P drops. */
static void transport_picture_reset(transport_clock_v27 *c){c->picture_started=0;}
static int transport_picture_time(transport_clock_v27 *c,uint64_t source,uint64_t *pts,uint64_t *pcr){
    struct timespec t;
    if(clock_gettime(CLOCK_MONOTONIC,&t)!=0 || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000)return -1;
    uint64_t now=(uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;
    if(!c->picture_started){c->source_us=source;c->due_us=now;c->last_source_us=source;c->picture_started=1;}
    if(source<c->last_source_us || source-c->source_us>UINT64_MAX-c->due_us)return -1;
    uint64_t due=c->due_us+(source-c->source_us);
    /* A single future interval is limited so an invalid source cannot stall
     * the receiver, renewal/teardown checks or native handback. */
    if((due>now && due-now>100000) || (now>due && now-due>250000))return -1;
    while(now<due){
        uint64_t wait=due-now;struct timespec pause={.tv_sec=0,.tv_nsec=(long)(wait*1000)};
        if(nanosleep(&pause,NULL)!=0 && errno!=EINTR)return -1;
        if(clock_gettime(CLOCK_MONOTONIC,&t)!=0 || t.tv_sec<0 || t.tv_nsec<0 || t.tv_nsec>=1000000000)return -1;
        uint64_t next=(uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;
        if(next<now)return -1;now=next;
    }
    if(now-due>250000 || transport_clock_at(c,now/1000,pcr)<0)return -1;
    uint64_t due_ms=due/1000;
    if(due_ms<c->epoch_ms || due_ms-c->epoch_ms>UINT64_MAX/90-1100)return -1;
    *pts=(1100+due_ms-c->epoch_ms)*90;c->last_source_us=source;return 0;
}
#endif
