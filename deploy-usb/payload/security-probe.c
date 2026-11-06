/* MU0841 observation shim. No display advertisements, receivers, key capture,
 * decryption, network sockets, or cluster output. Factory signatures verified
 * against the matching library. Never logs key bytes or context addresses. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <dlfcn.h>
#include <pthread.h>
#include <errno.h>

static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static void record(const char *event,int present,int32_t result) {
    int saved=errno;
    pthread_mutex_lock(&lock);
    FILE *file=fopen("/data/lge/smartlink/carplay/carplay-security-probe-v3.log","a");
    if(!file)file=fopen("/tmp/carplay-security-probe-v3.log","a");
    fprintf(stderr,"[SECURITY-PROBE-V3] %s present=%d status=%d\n",event,present,result);
    if(file){fprintf(file,"[SECURITY-PROBE-V3] %s present=%d status=%d\n",event,present,result);fclose(file);}
    pthread_mutex_unlock(&lock);
    errno=saved;
}
typedef int32_t (*security_fn)(void *,const uint8_t *,const uint8_t *);
typedef int32_t (*homekit_fn)(void *,void *);
typedef int32_t (*chacha_fn)(void *,const uint8_t *,size_t);
typedef int32_t (*cbc_fn)(void *,const uint8_t *,const uint8_t *,int);
static void *lookup(const char *name) {
    int saved=errno;void *symbol=dlsym(RTLD_NEXT,name);errno=saved;return symbol;
}

int32_t AirPlayReceiverSessionSetSecurityInfo(void *session,const uint8_t *key,const uint8_t *iv) {
    security_fn native=(security_fn)lookup("AirPlayReceiverSessionSetSecurityInfo");
    if(!native){record("MISSING_session_AES",0,-1);return -1;}
    int32_t result=native(session,key,iv);
    record("session_AES",key!=NULL && iv!=NULL,result);return result;
}
int32_t AirPlayReceiverSessionSetHomeKitSecurityContext(void *session,void *context) {
    homekit_fn native=(homekit_fn)lookup("AirPlayReceiverSessionSetHomeKitSecurityContext");
    if(!native){record("MISSING_session_HomeKit",0,-1);return -1;}
    int32_t result=native(session,context);
    record("session_HomeKit",context!=NULL,result);return result;
}
int32_t AirPlayReceiverSessionScreen_SetSecurityInfo(void *screen,const uint8_t *key,const uint8_t *iv) {
    security_fn native=(security_fn)lookup("AirPlayReceiverSessionScreen_SetSecurityInfo");
    if(!native){record("MISSING_screen_AES",0,-1);return -1;}
    int32_t result=native(screen,key,iv);
    record("screen_AES",key!=NULL && iv!=NULL,result);return result;
}
int32_t AirPlayReceiverSessionScreen_SetChaChaSecurityInfo(void *screen,const uint8_t *key,size_t length) {
    chacha_fn native=(chacha_fn)lookup("AirPlayReceiverSessionScreen_SetChaChaSecurityInfo");
    if(!native){record("MISSING_screen_ChaCha",0,-1);return -1;}
    int32_t result=native(screen,key,length);
    record("screen_ChaCha",key!=NULL && length==32,result);return result;
}
int32_t AES_CBCFrame_Init(void *context,const uint8_t *key,const uint8_t *iv,int encrypt) {
    cbc_fn native=(cbc_fn)lookup("AES_CBCFrame_Init");
    if(!native){record("MISSING_CBC",0,-1);return -1;}
    int32_t result=native(context,key,iv,encrypt);
    record(encrypt ? "CBC_encrypt" : "CBC_decrypt",key!=NULL && iv!=NULL,result);return result;
}
__attribute__((constructor)) static void start(void) {
    record("LOADED_observation_only",1,0);
    const char *names[]={"AirPlayReceiverSessionSetSecurityInfo","AirPlayReceiverSessionSetHomeKitSecurityContext",
        "AirPlayReceiverSessionScreen_SetSecurityInfo","AirPlayReceiverSessionScreen_SetChaChaSecurityInfo","AES_CBCFrame_Init"};
    for(size_t i=0;i<sizeof(names)/sizeof(names[0]);i++)record(names[i],dlsym(RTLD_NEXT,names[i])!=NULL,0);
}
