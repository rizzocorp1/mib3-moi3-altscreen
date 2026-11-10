/* Bounded authenticated GOP buffering while root transfers encoder ownership.
 * Keep the opening IDR and all dependent access units in order. Never output
 * plaintext before a verified grant, and never retain it across stream teardown.
 * Only the receiver thread owns this queue. */
#define DEFERRED_MAX_BYTES (16u*1024u*1024u)
#define DEFERRED_MAX_FRAMES 256u
typedef int (*deferred_emit_fn)(const uint8_t *,size_t,uint64_t);
typedef struct DeferredFrame {
    struct DeferredFrame *next;
    size_t length;
    uint64_t timestamp,queued_us;
    uint8_t bytes[];
} DeferredFrame;
static DeferredFrame *g_deferred_first,*g_deferred_last;
static size_t g_deferred_bytes,g_deferred_count;
static const char *g_frame_directory="/data/lge/smartlink/carplay";
static void deferred_video_clear(void) {
    while(g_deferred_first) {
        DeferredFrame *f=g_deferred_first;g_deferred_first=f->next;
        volatile uint8_t *p=f->bytes;for(size_t i=0;i<f->length;i++)p[i]=0;
        free(f);
    }
    g_deferred_last=NULL;g_deferred_bytes=0;g_deferred_count=0;
}
static int deferred_video_flush(const char *directory,deferred_emit_fn emit) {
    if(!g_deferred_first)return 0;
    int status=frame_gate_at(directory,1,1);
    if(status<0){deferred_video_clear();return -1;}
    if(status!=0)return 0;
    struct timespec clock;
    if(clock_gettime(CLOCK_MONOTONIC,&clock)!=0){deferred_video_clear();deferred_video_need_idr();return -1;}
    uint64_t now=(uint64_t)clock.tv_sec*1000000+(uint64_t)clock.tv_nsec/1000;
    if(now<g_deferred_first->queued_us || now-g_deferred_first->queued_us>250000){
        deferred_video_clear();deferred_video_need_idr();
        log_msg("HANDOFF","Opening GOP stale after ownership wait; awaiting a fresh IDR instead of replaying backlog");return 0;
    }
    size_t count=g_deferred_count;
    for(DeferredFrame *f=g_deferred_first;f;f=f->next) {
        if(emit(f->bytes,f->length,f->timestamp)!=0){deferred_video_clear();return -1;}
    }
    deferred_video_clear();
    log_msg("HANDOFF","Forwarded buffered authenticated access units: %zu",count);
    return 1;
}
static int deferred_video_submit(const char *directory,const uint8_t *data,size_t length,
                                 uint64_t timestamp,int idr,int authenticated,deferred_emit_fn emit) {
    if(!authenticated){deferred_video_clear();return -1;}
    if(deferred_video_flush(directory,emit)<0)return -1;
    int status=frame_gate_at(directory,idr,authenticated);
    if(status==0)return emit(data,length,timestamp);
    if(status<0){deferred_video_clear();return -1;}
    if(idr)deferred_video_clear(); /* A new IDR supersedes older queued GOPs. */
    if(!idr && !g_deferred_first)return 0;
    if(length>DEFERRED_MAX_BYTES-g_deferred_bytes || g_deferred_count>=DEFERRED_MAX_FRAMES) {
        log_msg("HANDOFF","Authenticated video buffer limit reached; awaiting a new IDR");
        deferred_video_clear();deferred_video_need_idr();return 0;
    }
    DeferredFrame *frame=malloc(sizeof(*frame)+length);
    if(!frame){deferred_video_clear();return -1;}
    struct timespec now;
    if(clock_gettime(CLOCK_MONOTONIC,&now)!=0){free(frame);deferred_video_clear();deferred_video_need_idr();return -1;}
    frame->queued_us=(uint64_t)now.tv_sec*1000000+(uint64_t)now.tv_nsec/1000;
    frame->next=NULL;frame->length=length;frame->timestamp=timestamp;
    memcpy(frame->bytes,data,length);
    if(g_deferred_last)g_deferred_last->next=frame;else g_deferred_first=frame;
    g_deferred_last=frame;g_deferred_bytes+=length;g_deferred_count++;
    return 0;
}
