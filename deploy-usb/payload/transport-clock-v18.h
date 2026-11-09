/* Local enqueue and packetization timestamps share one source epoch.
 * Matches the factory's millisecond quantization; never uses phone absolute time.
 * A backwards/failing clock drops output instead of fabricating a timestamp. */
#ifndef MOI3_TRANSPORT_CLOCK_V18_H
#define MOI3_TRANSPORT_CLOCK_V18_H
#include <stdint.h>
#include <string.h>
#include <time.h>
typedef struct { uint64_t epoch_ms,last_ms;int started; } transport_clock_v18;
static int transport_clock_at(transport_clock_v18 *c,uint64_t now_ms,uint64_t *ticks) {
    if(c->started && now_ms<c->last_ms)return -1;
    if(!c->started){c->epoch_ms=now_ms;c->started=1;}
    uint64_t elapsed=now_ms-c->epoch_ms;
    if(elapsed>UINT64_MAX/90-1000)return -1;
    c->last_ms=now_ms;*ticks=(1000+elapsed)*90;return 0;
}
static int transport_clock_now(transport_clock_v18 *c,uint64_t *ticks) {
    struct timespec now;
    if(clock_gettime(CLOCK_MONOTONIC,&now)!=0 || now.tv_sec<0 || now.tv_nsec<0 || now.tv_nsec>=1000000000)return -1;
    if((uint64_t)now.tv_sec>UINT64_MAX/1000)return -1;
    uint64_t ms=(uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
    return transport_clock_at(c,ms,ticks);
}
static void transport_clock_reset(transport_clock_v18 *c){memset(c,0,sizeof(*c));}
#endif
