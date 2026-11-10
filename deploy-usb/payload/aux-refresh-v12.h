/* Request fresh reference pictures only for the advertised auxiliary UUID.
 * Queue asynchronously on the public session queue; never dispatch_sync from
 * the receiver thread that teardown must join. Native SendCommand uses its own
 * event queue. Retained session/request objects survive cancellation races. */
typedef void *(*aux_queue_fn)(void *);
typedef void (*aux_async_fn)(void *,void *,void (*)(void *));
typedef int32_t (*aux_send_fn)(void *,void *,void *,void *);
static aux_queue_fn s_aux_queue;
static aux_async_fn s_aux_async;
static aux_send_fn s_aux_send;
static uint64_t g_aux_generation=1,g_aux_pending_generation,g_aux_last_ms;
static unsigned g_aux_queued_total,g_aux_attempts,g_aux_results,g_aux_errors;
static int g_aux_symbols_warned;
typedef struct {void *session,*request;uint64_t generation;} AuxRefreshJob;
static void aux_refresh_invalidate_locked(void) {
    if(++g_aux_generation==0)g_aux_generation=1;
    g_aux_pending_generation=0;g_aux_last_ms=0;
    g_aux_attempts=g_aux_results=g_aux_errors=0;g_aux_symbols_warned=0;
}
static int aux_string_equals(const void *dictionary,const char *name,const char *expected) {
    void *key=s_cf(name);if(!key)return 0;
    const void *value=s_CFDictionaryGetValue(dictionary,key);s_CFRelease(key);
    char bytes[128];return value && s_CFStringGetCString(value,bytes,sizeof(bytes),0x08000100) && !strcmp(bytes,expected);
}
static void *aux_refresh_request(void) {
    if(!s_CFDictionaryCreateMutable || !s_CFDictionarySetValue || !s_CFDictionaryGetValue ||
       !s_CFStringGetCString || !s_CFRelease || !s_CFRetain)return NULL;
    void *request=s_CFDictionaryCreateMutable(NULL,0,s_kCFLDictKeyCallBacks,s_kCFLDictValCallBacks);
    void *params=s_CFDictionaryCreateMutable(NULL,0,s_kCFLDictKeyCallBacks,s_kCFLDictValCallBacks);
    if(!request || !params){if(request)s_CFRelease(request);if(params)s_CFRelease(params);return NULL;}
    set_str(request,"type","forceKeyFrame");set_str(params,"uuid",ALT_UUID_DEFAULT);
    const void *stored=NULL;
    void *key=s_cf("params");if(key){s_CFDictionarySetValue(request,key,params);stored=s_CFDictionaryGetValue(request,key);s_CFRelease(key);}
    int valid=stored==params && aux_string_equals(request,"type","forceKeyFrame") && aux_string_equals(stored,"uuid",ALT_UUID_DEFAULT);
    s_CFRelease(params);
    if(!valid){s_CFRelease(request);return NULL;}
    return request;
}
static void aux_refresh_finish(AuxRefreshJob *job) {
    pthread_mutex_lock(&g_crypto_lock);
    if(g_aux_pending_generation==job->generation)g_aux_pending_generation=0;
    if(g_aux_queued_total)g_aux_queued_total--;
    pthread_mutex_unlock(&g_crypto_lock);
    if(job->request)s_CFRelease(job->request);
    s_CFRelease(job->session);free(job);
}
static void aux_refresh_run(void *context) {
    AuxRefreshJob *job=context;
    pthread_mutex_lock(&g_crypto_lock);
    int current=job->generation==g_aux_generation && job->session==g_alt_session && g_alt_thread_running && g_alt_chacha_valid && g_handoff_started;
    pthread_mutex_unlock(&g_crypto_lock);
    if(current) {
        int32_t status=s_aux_send(job->session,job->request,NULL,NULL);
        pthread_mutex_lock(&g_crypto_lock);
        if(job->generation==g_aux_generation){g_aux_results++;if(status)g_aux_errors++;}
        pthread_mutex_unlock(&g_crypto_lock);
        log_msg("AUX_REFRESH","Auxiliary-only forceKeyFrame submission status=%d (not a phone acknowledgement)",status);
    }
    aux_refresh_finish(job);
}
static int aux_refresh_tick_at(uint64_t now_ms) {
    if(!s_aux_queue)s_aux_queue=(aux_queue_fn)dlsym(RTLD_DEFAULT,"AirPlayReceiverSessionGetDispatchQueue");
    if(!s_aux_async)s_aux_async=(aux_async_fn)dlsym(RTLD_DEFAULT,"dispatch_async_f");
    if(!s_aux_send)s_aux_send=(aux_send_fn)dlsym(RTLD_DEFAULT,"AirPlayReceiverSessionSendCommand");
    pthread_mutex_lock(&g_crypto_lock);
    if(!g_alt_thread_running || !g_alt_session || !g_alt_chacha_valid || !g_handoff_started ||
       !g_received_frames || g_aux_attempts>=90 || g_aux_pending_generation || g_aux_queued_total>=4 ||
       (g_aux_last_ms && now_ms-g_aux_last_ms<2000)) {
        pthread_mutex_unlock(&g_crypto_lock);return 0;
    }
    if(!s_aux_queue || !s_aux_async || !s_aux_send || !s_CFRetain || !s_CFRelease) {
        if(!g_aux_symbols_warned){g_aux_symbols_warned=1;log_msg("AUX_REFRESH","Native auxiliary refresh API unavailable; video path retained");}
        pthread_mutex_unlock(&g_crypto_lock);return 0;
    }
    AuxRefreshJob *job=calloc(1,sizeof(*job));
    if(!job){pthread_mutex_unlock(&g_crypto_lock);return -1;}
    job->session=s_CFRetain(g_alt_session);job->generation=g_aux_generation;
    if(!job->session){free(job);pthread_mutex_unlock(&g_crypto_lock);return -1;}
    g_aux_pending_generation=job->generation;g_aux_queued_total++;g_aux_last_ms=now_ms;g_aux_attempts++;
    pthread_mutex_unlock(&g_crypto_lock);
    job->request=aux_refresh_request();
    void *queue=job->request?s_aux_queue(job->session):NULL;
    if(!queue){aux_refresh_finish(job);return -1;}
    s_aux_async(queue,job,aux_refresh_run);
    return 1;
}
static void aux_refresh_tick(void) {
    struct timespec now;
    if(clock_gettime(CLOCK_MONOTONIC,&now)==0)
        aux_refresh_tick_at((uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000);
}
