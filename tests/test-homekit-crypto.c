/* Independent OpenSSL AEAD reference behind the verified factory ABI. This
 * tests framing/state/authentication, not execution of factory AArch64 code. */
#define _GNU_SOURCE
#include <openssl/evp.h>
#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <sys/socket.h>
static _Atomic unsigned packets;
static ssize_t capture_packet(int fd,const void *p,size_t n,int flags,const struct sockaddr *a,socklen_t z) {
    (void)fd;(void)p;(void)flags;(void)a;(void)z;atomic_fetch_add(&packets,1);return (ssize_t)n;
}
#define sendto capture_packet
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
#undef sendto

typedef struct {int refs;} Pairing;
static Pairing pairing_a,pairing_b;
static void *expected_pairing;
static uint64_t expected_cid;
static int derive_failure,tail_mode,retain_failure;
static uint8_t reference_key[32];
static const void *retain_pairing(const void *p){if(retain_failure)return NULL;((Pairing *)p)->refs++;return p;}
static void release_pairing(const void *p){assert(((Pairing *)p)->refs>0);((Pairing *)p)->refs--;}
static int32_t derive_pairing(void *context,const void *salt,size_t salt_len,const void *info,size_t info_len,size_t output_len,void *out) {
    char expected[64];snprintf(expected,sizeof(expected),"DataStream-Salt%llu",(unsigned long long)expected_cid);
    assert(context==expected_pairing);
    assert(strlen(expected)==salt_len && !memcmp(salt,expected,salt_len));
    assert(info_len==32 && !memcmp(info,"DataStream-Output-Encryption-Key",32));
    assert(output_len==32);
    memcpy(out,reference_key,32);return derive_failure ? -51 : 0;
}
typedef struct {EVP_CIPHER_CTX *cipher;uint8_t tail[16];size_t tail_len;} Bridge;
_Static_assert(sizeof(Bridge)<=sizeof(aes_ctr_storage_t),"Bridge fits native storage");
static void native_init(void *storage,const uint8_t *key,const uint8_t *nonce8) {
    Bridge *b=storage;uint8_t nonce12[12]={0};memcpy(nonce12+4,nonce8,8);
    b->cipher=EVP_CIPHER_CTX_new();assert(b->cipher);b->tail_len=0;
    assert(EVP_DecryptInit_ex(b->cipher,EVP_chacha20_poly1305(),NULL,key,nonce12)==1);
}
static void native_aad(void *storage,const void *aad,size_t n) {
    Bridge *b=storage;int written;assert(n==128);
    assert(EVP_DecryptUpdate(b->cipher,NULL,&written,aad,(int)n)==1);
}
static size_t native_decrypt(void *storage,const void *input,size_t n,void *output) {
    Bridge *b=storage;uint8_t *plain=malloc(n+16);assert(plain);int written;
    assert(EVP_DecryptUpdate(b->cipher,plain,&written,input,(int)n)==1);assert((size_t)written==n);
    b->tail_len=tail_mode && n>3 ? 3 : 0;
    memcpy(output,plain,n-b->tail_len);memcpy(b->tail,plain+n-b->tail_len,b->tail_len);
    secret_zero(plain,n+16);free(plain);return n-b->tail_len;
}
static size_t native_verify(void *storage,void *output,const uint8_t *tag,int32_t *status) {
    Bridge *b=storage;int written;uint8_t end[16];
    assert(EVP_CIPHER_CTX_ctrl(b->cipher,EVP_CTRL_AEAD_SET_TAG,16,(void *)tag)==1);
    int ok=EVP_DecryptFinal_ex(b->cipher,end,&written);assert(written==0);
    memcpy(output,b->tail,b->tail_len);*status=ok==1 ? 0 : -6754;
    size_t tail=b->tail_len;EVP_CIPHER_CTX_free(b->cipher);b->cipher=NULL;return tail;
}
static size_t encrypt_reference(const uint8_t header[128],const uint8_t *plain,size_t n,uint64_t counter,uint8_t *output) {
    uint8_t nonce[12]={0};for(int i=0;i<8;i++)nonce[4+i]=(uint8_t)(counter>>(8*i));
    EVP_CIPHER_CTX *c=EVP_CIPHER_CTX_new();assert(c);int written,last;
    assert(EVP_EncryptInit_ex(c,EVP_chacha20_poly1305(),NULL,reference_key,nonce)==1);
    assert(EVP_EncryptUpdate(c,NULL,&written,header,128)==1);
    assert(EVP_EncryptUpdate(c,output,&written,plain,(int)n)==1);assert((size_t)written==n);
    assert(EVP_EncryptFinal_ex(c,output+written,&last)==1);assert(last==0);
    assert(EVP_CIPHER_CTX_ctrl(c,EVP_CTRL_AEAD_GET_TAG,16,output+n)==1);
    EVP_CIPHER_CTX_free(c);return n+16;
}
static void prepare(void *session,uint64_t cid) {
    expected_cid=cid;expected_pairing=pairing_for_session(session);
    assert(expected_pairing);assert(prepare_homekit_stream(expected_pairing,cid)==0);
}
static void test_tcp_receiver(void *session) {
    expected_pairing=pairing_for_session(session);expected_cid=777;
    int port=start_alt_receiver(session,777);assert(port>0 && g_alt_chacha_valid && !g_alt_aes_valid);
    int client=socket(AF_INET,SOCK_STREAM,0);assert(client>=0);
    struct timeval timeout={2,0};assert(setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0);
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_port=htons((uint16_t)port);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(connect(client,(struct sockaddr *)&address,sizeof(address))==0);
    uint8_t config[]={1,100,0,31,0xff,0xe1,0,2,0x67,0x42,1,0,2,0x68,0xce};
    AirPlayScreenHeaderCompat header={0};header.opcode=K_SCREEN_VIDEO_CONFIG;header.bodySize=sizeof(config);
    assert(write(client,&header,sizeof(header))==(ssize_t)sizeof(header));assert(write(client,config,sizeof(config))==(ssize_t)sizeof(config));
    uint8_t plain[]={0,0,0,3,0x65,0xaa,0xbb},body[64];
    header.opcode=K_SCREEN_VIDEO_FRAME;header.bodySize=sizeof(plain)+16;
    size_t n=encrypt_reference((const uint8_t *)&header,plain,sizeof(plain),0,body);
    g_rtp_sock=99;atomic_store(&packets,0);
    assert(write(client,&header,sizeof(header))==(ssize_t)sizeof(header));assert(write(client,body,n)==(ssize_t)n);
    for(int i=0;i<200 && !atomic_load(&packets);i++)usleep(10000);
    unsigned sent=atomic_load(&packets);assert(sent>0);
    n=encrypt_reference((const uint8_t *)&header,plain,sizeof(plain),1,body);body[n-1]^=1;
    assert(write(client,&header,sizeof(header))==(ssize_t)sizeof(header));assert(write(client,body,n)==(ssize_t)n);
    uint8_t byte;assert(recv(client,&byte,1,0)==0);
    assert(atomic_load(&packets)==sent);close(client);stop_alt_receiver();g_rtp_sock=-1;
    assert(!g_alt_chacha_valid && !g_alt_thread_created);
}
int main(void) {
    for(int i=0;i<32;i++)reference_key[i]=(uint8_t)(i+1);
    s_pairing_retain=retain_pairing;s_pairing_release=release_pairing;s_pairing_derive=derive_pairing;
    s_chacha_init=native_init;s_chacha_aad=native_aad;s_chacha_decrypt=native_decrypt;s_chacha_verify=native_verify;
    int session_a,session_b,unrelated;
    assert(remember_pairing(&session_a,&pairing_a)==0 && pairing_a.refs==1);
    assert(remember_pairing(&session_b,&pairing_b)==0 && pairing_b.refs==1);
    retain_failure=1;
    assert(remember_pairing(&session_a,&pairing_b)==-1 && !pairing_for_session(&session_a));
    assert(pairing_a.refs==0 && pairing_b.refs==1);retain_failure=0;
    assert(remember_pairing(&session_a,&pairing_a)==0 && pairing_a.refs==1);
    int extra[3];
    assert(remember_pairing(&extra[0],&pairing_a)==0 && remember_pairing(&extra[1],&pairing_a)==0);
    assert(remember_pairing(&extra[2],&pairing_a)==-1 && !pairing_for_session(&extra[2]));
    assert(remember_pairing(&extra[0],NULL)==0 && remember_pairing(&extra[1],NULL)==0);
    assert(remember_pairing(&session_b,&pairing_b)==0 && pairing_b.refs==1);
    assert(pairing_for_session(&unrelated)==NULL);
    assert(remember_pairing(&session_a,&pairing_a)==0 && pairing_a.refs==1);
    for(int tail=0;tail<2;tail++) {
        tail_mode=tail;prepare(&session_a,UINT64_MAX);
        for(uint64_t counter=0;counter<260;counter++) {
            uint8_t header[128],plain[257],body[300];memset(header,(int)counter,sizeof(header));
            for(size_t i=0;i<sizeof(plain);i++)plain[i]=(uint8_t)(i+counter);
            size_t n=encrypt_reference(header,plain,sizeof(plain),counter,body);
            assert(decrypt_homekit_frame(header,sizeof(header),body,&n)==0);
            assert(n==sizeof(plain) && !memcmp(body,plain,n));
        }
    }
    /* Authenticate the header, ciphertext and tag independently; a wrong key
     * or a replay also must not release plaintext or retain usable state. */
    for(int fault=0;fault<5;fault++) {
        uint8_t header[128]={0},plain[37]={1},body[64];prepare(&session_b,123);
        size_t n=encrypt_reference(header,plain,sizeof(plain),0,body);
        if(fault==0)header[64]^=1;
        if(fault==1)body[0]^=1;
        if(fault==2)body[n-1]^=1;
        if(fault==3)g_alt_chacha_key[0]^=1;
        if(fault==4){size_t valid_n=n;uint8_t first[64];memcpy(first,body,n);assert(decrypt_homekit_frame(header,128,first,&valid_n)==0);}
        assert(decrypt_homekit_frame(header,128,body,&n)==-1 && !g_alt_chacha_valid);
        for(size_t i=0;i<n;i++)assert(body[i]==0);
        for(size_t i=0;i<32;i++)assert(g_alt_chacha_key[i]==0);
    }
    prepare(&session_a,1);uint8_t header[128]={0},short_body[16]={0};size_t short_n=16;
    assert(decrypt_homekit_frame(header,128,short_body,&short_n)==-1);
    assert(!g_alt_chacha_valid);
    prepare(&session_a,1);memset(g_alt_chacha_nonce,255,8);
    uint8_t last_plain[3]={1,2,3},last_body[32];
    size_t last_n=encrypt_reference(header,last_plain,sizeof(last_plain),UINT64_MAX,last_body);
    assert(decrypt_homekit_frame(header,128,last_body,&last_n)==-1 && !g_alt_chacha_valid);
    derive_failure=1;expected_cid=1;expected_pairing=&pairing_a;
    assert(prepare_homekit_stream(&pairing_a,1)==-1 && !g_alt_chacha_valid);derive_failure=0;
    test_tcp_receiver(&session_a);
    assert(remember_pairing(&session_a,NULL)==0 && pairing_a.refs==0);
    assert(remember_pairing(&session_b,NULL)==0 && pairing_b.refs==0);
    assert(!pairing_for_session(&session_a));
    puts("PASS: HomeKit session ownership, CID derivation, 520 authenticated frames, buffered tails, nonce carry, corrupt header/body/tag, wrong key, replay and derive failure");
}
