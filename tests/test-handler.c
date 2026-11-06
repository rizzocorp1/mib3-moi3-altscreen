#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static unsigned char captured[2*1024*1024];
static size_t captured_len;
static uint32_t expected_rtp;
static ssize_t capture_sendto(int fd, const void *buf, size_t n, int flags,
                             const struct sockaddr *addr, socklen_t alen) {
    (void)fd; (void)flags; (void)addr; (void)alen;
    const uint8_t *p=buf;
    assert(n>=12 && (n-12)%188==0);
    uint32_t ts=((uint32_t)p[4]<<24)|((uint32_t)p[5]<<16)|((uint32_t)p[6]<<8)|p[7];
    assert(ts==expected_rtp);
    assert(captured_len+n-12<=sizeof(captured));
    memcpy(captured+captured_len,p+12,n-12); captured_len+=n-12;
    return (ssize_t)n;
}
#define sendto capture_sendto
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
#undef sendto
static int32_t derive_ok(const uint8_t *m,size_t n,uint64_t id,uint8_t *k,uint8_t *iv) {
    (void)m;(void)n;(void)id; memset(k,1,16); memset(iv,2,16); return 0;
}
static int32_t derive_bad(const uint8_t *m,size_t n,uint64_t id,uint8_t *k,uint8_t *iv) {
    (void)m;(void)n;(void)id;(void)k;(void)iv; return -1;
}
static int32_t init_ok(void *c,const uint8_t *k,const uint8_t *iv) {(void)c;(void)k;(void)iv;return 0;}
static int32_t update_bad(void *c,const void *s,size_t n,void *d) {(void)c;(void)s;(void)n;(void)d;return -1;}
static int cbc_result;
static int32_t cbc_stub(void *c,const uint8_t k[16],const uint8_t iv[16],int encrypt) {
    (void)c;(void)k;(void)iv;(void)encrypt;return cbc_result;
}
static void reset_capture(uint32_t ts) {captured_len=0;expected_rtp=ts;g_rtp_ts_in_buf=0;g_rtp_sock=99;}
static uint64_t pcr(const uint8_t *p) {
    return ((uint64_t)p[6]<<25)|((uint64_t)p[7]<<17)|((uint64_t)p[8]<<9)|((uint64_t)p[9]<<1)|(p[10]>>7);
}
static size_t video_payload(uint8_t *out) {
    size_t pos=0;
    for(size_t i=0;i<captured_len;i+=188) {
        uint8_t *p=captured+i; assert(p[0]==0x47);
        unsigned pid=((p[1]&31)<<8)|p[2];
        if(pid!=17) continue;
        size_t offset=4; if(p[3]&0x20) offset+=1+p[4];
        assert(offset<=188); memcpy(out+pos,p+offset,188-offset); pos+=188-offset;
    }
    return pos;
}
static void test_mux(void) {
    const size_t sizes[]={1,161,162,163,345,346,347,2000};
    uint8_t frame[2000],pes[4096]; memset(frame,0x65,sizeof(frame));
    for(size_t i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
        reset_capture(90000); mux_and_stream_h264_frame(frame,sizes[i],90000);
        assert(pcr(captured)==90000);
        assert(video_payload(pes)==sizes[i]+14);
        assert(!memcmp(pes+14,frame,sizes[i]));
    }
    reset_capture(0xffffffff); mux_and_stream_h264_frame(frame,2000,(1ULL<<33)-1);
    assert(pcr(captured)==(1ULL<<33)-1);
}
static void test_parser(void) {
    uint8_t config[]={1,66,0,30,0xff,0xe1,0,2,0x67,0x42,1,0,2,0x68,0xce};
    parse_and_store_avcc(config,sizeof(config)); assert(g_sps_pps_len==12);
    uint8_t zero[]={0,0,0,0}, truncated[]={0,0,0,5,0x65}, trailing[]={0,0,0,1,0x65,0};
    reset_capture(0);
    assert(process_h264_frame(zero,sizeof(zero),0)==-1);
    assert(process_h264_frame(truncated,sizeof(truncated),0)==-1);
    assert(process_h264_frame(trailing,sizeof(trailing),0)==-1);
    assert(captured_len==0);
    uint8_t mixed[]={0,0,0,1,0x09,0,0,0,2,0x65,0xaa};
    reset_capture(90000); assert(process_h264_frame(mixed,sizeof(mixed),1000000)==0);
    uint8_t pes[100]; size_t n=video_payload(pes);
    assert(n==14+12+5+6); assert(!memcmp(pes+14,g_sps_pps_buf,12));
    assert(pes[14+12+4]==0x09); /* AUD before IDR still receives SPS/PPS */
    config[4]=0xfc; parse_and_store_avcc(config,sizeof(config));
    uint8_t one_byte[]={2,0x65,0xaa}; reset_capture(90000);
    assert(process_h264_frame(one_byte,sizeof(one_byte),1000000)==0);
    parse_and_store_avcc(config,8); assert(!g_sps_pps_len);
    reset_capture(0); assert(process_h264_frame(one_byte,sizeof(one_byte),0)==-1); assert(!captured_len);
}
static void test_receiver(void) {
    uint8_t key[16];memset(key,0x47,sizeof(key));
    real_AES_CBCFrame_Init=cbc_stub;g_master_valid=0;cbc_result=-42;
    assert(AES_CBCFrame_Init(NULL,key,key,0)==-42 && !g_master_valid);
    cbc_result=0;assert(AES_CBCFrame_Init(NULL,key,key,0)==0 && g_master_valid);
    assert(!memcmp(g_master_key,key,sizeof(key)));
    s_AirPlay_DeriveAESKeySHA512ForScreen=derive_ok;
    s_AES_CTR_Init=init_ok; s_AES_CTR_Update=update_bad;
    g_master_valid=0; assert(start_alt_receiver(1)==-1);
    assert(pthread_mutex_trylock(&g_crypto_lock)==0); pthread_mutex_unlock(&g_crypto_lock);
    g_master_valid=1; s_AirPlay_DeriveAESKeySHA512ForScreen=derive_bad;
    assert(start_alt_receiver(1)==-1); assert(g_alt_listen_fd==-1);
    s_AirPlay_DeriveAESKeySHA512ForScreen=derive_ok;
    for(int i=0;i<25;i++) {
        assert(start_alt_receiver(1)==6031);
        if(i%2) {
            int fd=socket(AF_INET,SOCK_STREAM,0); assert(fd>=0);
            struct sockaddr_in a={0}; a.sin_family=AF_INET;a.sin_port=htons(6031);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
            assert(connect(fd,(struct sockaddr*)&a,sizeof(a))==0);
            AirPlayScreenHeaderCompat h={0};h.bodySize=4;h.opcode=K_SCREEN_VIDEO_FRAME;
            uint8_t body[4]={0}; reset_capture(0);
            assert(write(fd,&h,sizeof(h))==(ssize_t)sizeof(h)); assert(write(fd,body,4)==4);
            usleep(10000); assert(captured_len==0); stop_alt_receiver();close(fd);
        } else stop_alt_receiver(); /* also wake blocked accept */
        assert(!g_alt_thread_created && g_alt_listen_fd==-1 && g_alt_client_fd==-1 && !g_alt_aes_valid);
    }
}
int main(int argc,char **argv) {
    test_mux();test_parser();test_receiver();
    if(argc==3) {
        FILE *in=fopen(argv[1],"rb"); assert(in); size_t length=fread(captured,1,sizeof(captured),in);
        assert(!ferror(in) && feof(in));fclose(in);assert(length>0);
        uint8_t *frame=malloc((size_t)length);assert(frame);memcpy(frame,captured,(size_t)length);
        reset_capture(90000);emit_pat_packet();emit_pmt_packet();mux_and_stream_h264_frame(frame,(size_t)length,90000);free(frame);
        FILE *out=fopen(argv[2],"wb");assert(out);assert(fwrite(captured,1,captured_len,out)==captured_len);fclose(out);
    }
    puts("PASS: PCR/PES, malformed AVC, config length, IDR parameter sets, crypto failures and receiver restart/stop");
}
