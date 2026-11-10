#define _GNU_SOURCE
#include <assert.h>
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
typedef struct Object Object;
struct Object {int kind,refs,count;char text[128];struct {char key[32];Object *value;} fields[4];};
static int live,drop_params,fail_dictionary,fail_retain,native_calls;
static int32_t native_status;
static pthread_mutex_t native_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t native_condition=PTHREAD_COND_INITIALIZER;
static int block_native,native_entered,release_native;
static Object *object(int kind){Object *o=calloc(1,sizeof(*o));assert(o);o->kind=kind;o->refs=1;live++;return o;}
static void *retain(const void *p){if(fail_retain)return NULL;Object *o=(Object*)p;assert(o->refs>0);o->refs++;return o;}
static void release(const void *p){Object *o=(Object*)p;assert(o&&o->refs>0);if(--o->refs)return;for(int i=0;i<o->count;i++)release(o->fields[i].value);live--;free(o);}
static void *string(const void *a,const char *s,uint32_t e){(void)a;(void)e;Object *o=object(1);assert(strlen(s)<sizeof(o->text));strcpy(o->text,s);return o;}
static int cstring(const void *p,char *out,int64_t n,uint32_t e){(void)e;const Object *o=p;if(o->kind!=1 || (int64_t)strlen(o->text)>=n)return 0;strcpy(out,o->text);return 1;}
static void *dictionary(const void *a,int64_t n,const void *k,const void *v){(void)a;(void)n;(void)k;(void)v;return fail_dictionary?NULL:object(2);}
static const void *get(const void *p,const void *key){const Object *o=p,*k=key;assert(o->kind==2&&k->kind==1);for(int i=0;i<o->count;i++)if(!strcmp(o->fields[i].key,k->text))return o->fields[i].value;return NULL;}
static void set(void *p,const void *key,const void *value){Object *o=p;const Object *k=key;assert(o->kind==2&&k->kind==1);if(drop_params&&!strcmp(k->text,"params"))return;assert(o->count<4);strcpy(o->fields[o->count].key,k->text);o->fields[o->count].value=retain(value);o->count++;}
static void *queue(void *session){return session;}
static struct {void *queue,*context;void (*run)(void*);} jobs[8];static int queued;
static void asynchronous(void *q,void *context,void (*run)(void*)){assert(queued<8);jobs[queued].queue=q;jobs[queued].context=context;jobs[queued++].run=run;}
static int32_t send_command(void *session,void *request,void *completion,void *context){
    Object *s=session,*r=request;assert(s->kind==3&&s->refs>=2&&r->count==2&&!completion&&!context);
    int layout=aux_string_equals(request,"type","showUI");
    assert(layout || aux_string_equals(request,"type","forceKeyFrame"));
    Object *key=string(NULL,"params",0);const Object *params=get(r,key);release(key);
    assert(params&&params->count==(layout?2:1)&&aux_string_equals(params,"uuid",ALT_UUID_DEFAULT) && (!layout || aux_string_equals(params,"url",ALT_TOP_URL)));
    native_calls++;
    pthread_mutex_lock(&native_lock);
    if(block_native){native_entered=1;pthread_cond_signal(&native_condition);while(!release_native)pthread_cond_wait(&native_condition,&native_lock);}
    pthread_mutex_unlock(&native_lock);
    return native_status;
}
static void run_one(void){assert(queued>0);void *q=jobs[0].queue,*context=jobs[0].context;void (*run)(void*)=jobs[0].run;assert(q==((AuxRefreshJob*)context)->session);memmove(jobs,jobs+1,(size_t)(--queued)*sizeof(jobs[0]));run(context);}
static void *run_worker(void *unused){(void)unused;run_one();return NULL;}
static void invalidate(void){pthread_mutex_lock(&g_crypto_lock);aux_refresh_invalidate_locked();pthread_mutex_unlock(&g_crypto_lock);}
int main(void){
    s_CFStringCreateWithCString=string;s_CFStringGetCString=cstring;s_CFDictionaryCreateMutable=dictionary;
    s_CFDictionarySetValue=set;s_CFDictionaryGetValue=get;s_CFRetain=retain;s_CFRelease=release;
    s_aux_queue=queue;s_aux_async=asynchronous;s_aux_send=send_command;
    Object *session=object(3),*other=object(3);g_alt_session=session;g_alt_thread_running=1;g_alt_chacha_valid=1;g_received_frames=1;
    assert(aux_refresh_tick_at(100000)==0&&!queued); /* no encoder grant */
    g_handoff_started=1;assert(aux_refresh_tick_at(100000)==0&&!queued); /* active output: no periodic keyframe */
    g_handoff_started=0;g_idr_frames=1;assert(aux_refresh_tick_at(100000)==1&&queued==1&&session->refs==2);
    assert(aux_refresh_tick_at(110000)==0&&queued==1); /* pending command coalesces */
    run_one();assert(native_calls==1&&session->refs==1&&!g_aux_queued_total&&live==2);
    assert(aux_refresh_tick_at(101999)==0);
    native_status=-6709;assert(aux_refresh_tick_at(102000)==1);run_one();assert(g_aux_errors==1&&g_alt_chacha_valid&&g_alt_thread_running&&live==2);
    native_status=0;assert(aux_refresh_tick_at(104000)==1);invalidate();g_alt_session=other;run_one();assert(native_calls==2&&live==2); /* stale session skipped */
    for(int i=0;i<4;i++){assert(aux_refresh_tick_at(200000+i*3000)==1);invalidate();}
    assert(g_aux_queued_total==4&&aux_refresh_tick_at(220000)==0);
    while(queued)run_one();assert(g_aux_queued_total==0&&live==2&&native_calls==2);
    fail_dictionary=1;assert(aux_refresh_tick_at(230000)==-1&&!queued&&live==2);fail_dictionary=0;
    drop_params=1;assert(aux_refresh_tick_at(233000)==-1&&!queued&&live==2);drop_params=0; /* never send an unscoped command */
    fail_retain=1;assert(aux_refresh_tick_at(236000)==-1&&!queued&&live==2);fail_retain=0;
    g_aux_attempts=90;assert(aux_refresh_tick_at(239000)==1&&queued==1);invalidate();run_one();assert(native_calls==2);
    assert(aux_refresh_tick_at(242000)==1);g_alt_chacha_valid=0;run_one();assert(native_calls==2&&live==2);
    g_alt_chacha_valid=1;assert(aux_refresh_tick_at(245000)==1);g_handoff_started=0;invalidate();run_one();assert(native_calls==2&&live==2);
    g_handoff_started=0;invalidate();assert(aux_refresh_tick_at(250000)==1);
    block_native=1;pthread_t worker;assert(pthread_create(&worker,NULL,run_worker,NULL)==0);
    pthread_mutex_lock(&native_lock);while(!native_entered)pthread_cond_wait(&native_condition,&native_lock);pthread_mutex_unlock(&native_lock);
    /* Invalidation/teardown never waits for the blocked native dispatch. */
    invalidate();assert(other->refs==2);
    pthread_mutex_lock(&native_lock);release_native=1;pthread_cond_signal(&native_condition);pthread_mutex_unlock(&native_lock);
    assert(pthread_join(worker,NULL)==0 && g_aux_results==0 && other->refs==1 && live==2);
    block_native=0;invalidate();g_layout_advertised=1;g_handoff_started=1;
    assert(aux_refresh_tick_at(300000)==1 && ((AuxRefreshJob*)jobs[0].context)->layout);
    run_one();assert(g_aux_layout_attempted && g_aux_attempts==0);
    assert(aux_refresh_tick_at(302000)==0&&!queued); /* layout done; active output remains untouched */
    g_handoff_started=0; /* new configuration awaiting an IDR */
    assert(aux_refresh_tick_at(304000)==1 && !((AuxRefreshJob*)jobs[0].context)->layout);
    run_one();assert(g_aux_attempts==1);
    invalidate();assert(aux_refresh_tick_at(304000)==1 && ((AuxRefreshJob*)jobs[0].context)->layout);
    int before=native_calls;invalidate();run_one();assert(native_calls==before);
    g_layout_advertised=0;
    g_alt_session=NULL;g_alt_thread_running=0;release(session);release(other);assert(live==0&&!queued&&!g_aux_queued_total);
    puts("PASS: auxiliary UUID only, no pre-grant command, native submission errors preserve video, asynchronous coalescing/rate limit, retained lifetime, stale/security cancellation, bounded queue, session-bound lifetime beyond90 attempts, missing params/allocation/retain failures reject without leaks");
}
