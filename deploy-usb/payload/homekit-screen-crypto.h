/* MU0841 native HomeKit screen security. Pairing derivation labels, argument
 * order and framing verified from the matching factory library. No secrets
 * are logged or written to disk. Caller holds g_crypto_lock. */
typedef int32_t (*pairing_derive_fn)(void *,const void *,size_t,const void *,size_t,size_t,void *);
typedef void (*chacha_init_fn)(void *,const uint8_t *,const uint8_t *);
typedef void (*chacha_aad_fn)(void *,const void *,size_t);
typedef size_t (*chacha_decrypt_fn)(void *,const void *,size_t,void *);
typedef size_t (*chacha_verify_fn)(void *,void *,const uint8_t *,int32_t *);
typedef const void *(*pairing_retain_fn)(const void *);
typedef void (*pairing_release_fn)(const void *);
static pairing_derive_fn s_pairing_derive;
static chacha_init_fn s_chacha_init;
static chacha_aad_fn s_chacha_aad;
static chacha_decrypt_fn s_chacha_decrypt;
static chacha_verify_fn s_chacha_verify;
static pairing_retain_fn s_pairing_retain;
static pairing_release_fn s_pairing_release;
static struct {void *session;void *context;} g_pairings[4];
static uint8_t g_alt_chacha_key[32],g_alt_chacha_nonce[8];
static int g_alt_chacha_valid;
static void *g_alt_session;
static void secret_zero(void *memory,size_t n) {
    volatile uint8_t *p=memory;while(n--)*p++=0;
}
static void init_homekit_symbols(void) {
    s_pairing_derive=(pairing_derive_fn)dlsym(RTLD_DEFAULT,"PairingSessionDeriveKey");
    s_chacha_init=(chacha_init_fn)dlsym(RTLD_DEFAULT,"chacha20_poly1305_init_64x64");
    s_chacha_aad=(chacha_aad_fn)dlsym(RTLD_DEFAULT,"chacha20_poly1305_add_aad");
    s_chacha_decrypt=(chacha_decrypt_fn)dlsym(RTLD_DEFAULT,"chacha20_poly1305_decrypt");
    s_chacha_verify=(chacha_verify_fn)dlsym(RTLD_DEFAULT,"chacha20_poly1305_verify");
    s_pairing_retain=(pairing_retain_fn)dlsym(RTLD_DEFAULT,"CFRetain");
    s_pairing_release=(pairing_release_fn)dlsym(RTLD_DEFAULT,"CFRelease");
}
/* Own a reference instead of retaining a borrowed pointer past its lifetime.
 * A full cache refuses observation; factory security setup is unaffected. */
static int remember_pairing(void *session,void *context) {
    if(!session || !s_pairing_retain || !s_pairing_release)return -1;
    void *old=NULL;int result=-1;
    pthread_mutex_lock(&g_crypto_lock);
    int slot=-1;
    for(int i=0;i<4;i++)if(g_pairings[i].session==session){slot=i;break;}
    if(slot<0 && context)for(int i=0;i<4;i++)if(!g_pairings[i].session){slot=i;break;}
    if(slot>=0) {
        const void *owned=context ? s_pairing_retain(context) : NULL;
        if(!context || owned) {
            old=g_pairings[slot].context;
            g_pairings[slot].context=(void *)owned;
            g_pairings[slot].session=context ? session : NULL;result=0;
        } else {
            old=g_pairings[slot].context;
            g_pairings[slot].context=NULL;g_pairings[slot].session=NULL;
        }
    } else if(!context)result=0;
    pthread_mutex_unlock(&g_crypto_lock);
    if(old)s_pairing_release(old);
    return result;
}
static void *pairing_for_session(void *session) {
    for(int i=0;i<4;i++)if(g_pairings[i].session==session && session)return g_pairings[i].context;
    return NULL;
}
static void reset_homekit_stream(void) {
    secret_zero(g_alt_chacha_key,sizeof(g_alt_chacha_key));
    secret_zero(g_alt_chacha_nonce,sizeof(g_alt_chacha_nonce));
    g_alt_chacha_valid=0;
}
static int prepare_homekit_stream(void *context,uint64_t connection_id) {
    reset_homekit_stream();
    if(!context || !connection_id || !s_pairing_derive || !s_chacha_init ||
       !s_chacha_aad || !s_chacha_decrypt || !s_chacha_verify)return -1;
    char salt[64];
    int n=snprintf(salt,sizeof(salt),"DataStream-Salt%llu",(unsigned long long)connection_id);
    static const char info[]="DataStream-Output-Encryption-Key";
    if(n<=0 || (size_t)n>=sizeof(salt))return -1;
    int32_t result=s_pairing_derive(context,salt,(size_t)n,info,sizeof(info)-1,
                                  sizeof(g_alt_chacha_key),g_alt_chacha_key);
    secret_zero(salt,sizeof(salt));
    if(result!=0){reset_homekit_stream();return -1;}
    g_alt_chacha_valid=1;return 0;
}
/* Native context occupies offsets through 0xf8 (factory screen reserves 0x100).
 * Use an aligned 512-byte buffer. Native decrypt may buffer trailing bytes;
 * verify emits that tail and returns its length separately from auth status. */
static int decrypt_homekit_frame(const void *header,size_t header_len,uint8_t *body,size_t *length) {
    if(!g_alt_chacha_valid || !header || header_len!=128 || !body || !length)return -1;
    if(*length<=16){secret_zero(body,*length);reset_homekit_stream();return -1;}
    size_t total=*length,encrypted=total-16;
    uint8_t tag[16];memcpy(tag,body+encrypted,sizeof(tag));
    aes_ctr_storage_t context;memset(&context,0,sizeof(context));
    s_chacha_init(&context,g_alt_chacha_key,g_alt_chacha_nonce);
    s_chacha_aad(&context,header,header_len);
    size_t first=s_chacha_decrypt(&context,body,encrypted,body);
    int32_t status=-1;size_t tail=0;
    if(first<=encrypted)tail=s_chacha_verify(&context,body+first,tag,&status);
    secret_zero(&context,sizeof(context));secret_zero(tag,sizeof(tag));
    if(status!=0 || first>encrypted || tail>encrypted-first || first+tail!=encrypted) {
        secret_zero(body,total);reset_homekit_stream();return -1;
    }
    /* Match the factory's per-video-frame 64-bit little-endian counter. */
    int i=0;while(i<8 && ++g_alt_chacha_nonce[i]==0)i++;
    if(i==8){secret_zero(body,total);reset_homekit_stream();return -1;}
    *length=encrypted;return 0;
}
