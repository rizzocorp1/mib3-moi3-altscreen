#define _GNU_SOURCE
#include <sys/socket.h>
#include <time.h>
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
    assert(n==1140 && (n-12)%188==0);
    assert(p[1]==33);
    assert(p[8]==0 && p[9]==1 && p[10]==0x86 && p[11]==0xa0);
    uint32_t ts=((uint32_t)p[4]<<24)|((uint32_t)p[5]<<16)|((uint32_t)p[6]<<8)|p[7];
    struct timespec now;assert(clock_gettime(CLOCK_MONOTONIC,&now)==0);
    uint32_t ms=(uint32_t)((uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000);
    assert((uint32_t)(ms-ts)<2000);
    assert(captured_len+n-12<=sizeof(captured));
    memcpy(captured+captured_len,p+12,n-12); captured_len+=n-12;
    return (ssize_t)n;
}
#define sendto capture_sendto
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
#undef sendto
static void derive_ok(const uint8_t *m,size_t n,uint64_t id,uint8_t *k,uint8_t *iv) {
    (void)m;(void)n;(void)id; memset(k,1,16); memset(iv,2,16);
}
static int32_t init_bad(void *c,const uint8_t *k,const uint8_t *iv) {
    (void)c;(void)k;(void)iv; return -1;
}
static int32_t init_ok(void *c,const uint8_t *k,const uint8_t *iv) {(void)c;(void)k;(void)iv;return 0;}
static int32_t update_bad(void *c,const void *s,size_t n,void *d) {(void)c;(void)s;(void)n;(void)d;return -1;}
static int cbc_result;
static int32_t cbc_stub(void *c,const uint8_t k[16],const uint8_t iv[16],int encrypt) {
    (void)c;(void)k;(void)iv;(void)encrypt;return cbc_result;
}
static void reset_capture(uint32_t ts) {captured_len=0;g_output_failed=0;g_wait_output_idr=1;transport_clock_reset(&g_transport_clock);expected_rtp=ts;g_rtp_ts_in_buf=0;g_rtp_sock=99;}
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
/* Field values independently decoded from MU0841 TransportStream PAT/PMT;
 * capture_sendto also checks the factory RTP clock/identifier/datagram size. */
static void test_factory_transport_profile(void) {
    reset_capture(0);emit_pat_packet();emit_pmt_packet();rtp_flush_buffer(0);
    assert(captured_len==6*188);
    const uint8_t *pat=captured,*pmt=captured+188;
    assert(pat[0]==0x47 && pat[1]==0x40 && pat[2]==0);
    assert(pat[8]==0 && pat[9]==100 && pat[10]==0xdb);
    assert(pat[13]==0 && pat[14]==1 && pat[15]==0xe3 && pat[16]==0xe8);
    assert(ts_crc32(pat+5,16)==0);
    assert(pmt[1]==0x43 && pmt[2]==0xe8 && pmt[8]==0 && pmt[9]==1 && pmt[10]==0xc3);
    assert(pmt[13]==0xe0 && pmt[14]==17 && pmt[17]==0x1b && pmt[18]==0xe0 && pmt[19]==17);
    assert(ts_crc32(pmt+5,21)==0);
    for(int i=2;i<6;i++){const uint8_t *p=captured+i*188;assert(p[0]==0x47 && p[1]==0x1f && p[2]==0xff && p[3]==0x10);}
}
static void test_factory_source_port(void) {
    int first=socket(AF_INET6,SOCK_DGRAM,0),second=socket(AF_INET6,SOCK_DGRAM,0);assert(first>=0&&second>=0);
    assert(bind_cluster_source(first)==0);
    struct sockaddr_in6 address;socklen_t size=sizeof(address);
    assert(getsockname(first,(struct sockaddr*)&address,&size)==0 && ntohs(address.sin6_port)==42989);
    assert(bind_cluster_source(second)==-1 && errno==EADDRINUSE);
    close(first);assert(bind_cluster_source(second)==0);close(second);
}
static void test_revoked_port_release(void) {
    char directory[]="/tmp/lease-port-XXXXXX";assert(mkdtemp(directory));
    g_rtp_sock=socket(AF_INET6,SOCK_DGRAM,0);assert(g_rtp_sock>=0&&bind_cluster_source(g_rtp_sock)==0);g_rtp_owned=1;
    release_cluster_output_if_unleased(directory);assert(g_rtp_sock==-1&&!g_rtp_owned);
    int probe=socket(AF_INET6,SOCK_DGRAM,0);assert(probe>=0&&bind_cluster_source(probe)==0);close(probe);rmdir(directory);
}
static void test_mux(void) {
    const size_t sizes[]={1,161,162,163,345,346,347,2000};
    uint8_t frame[2000],pes[4096]; memset(frame,0x65,sizeof(frame));
    for(size_t i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
        reset_capture(90000); mux_and_stream_h264_frame(frame,sizes[i],90000,0,90000);
        assert(pcr(captured)==90000);
        assert(video_payload(pes)==sizes[i]+14);
        assert(!memcmp(pes+14,frame,sizes[i]));
    }
    reset_capture(0xffffffff); mux_and_stream_h264_frame(frame,2000,(1ULL<<33)-1,0,(1ULL<<33)-1);
    assert(pcr(captured)==(1ULL<<33)-1);
}
static void test_source_signals(void) {
    uint8_t frame[2000]={0},pes[4096];g_video_discontinuity=1;
    reset_capture(90000);mux_and_stream_h264_frame(frame,sizeof(frame),90000,1,90000);
    assert(captured[5]==0xd0 && !g_video_discontinuity && pcr(captured)==90000);
    assert(video_payload(pes)==sizeof(frame)+14&&!memcmp(pes+14,frame,sizeof(frame)));
    reset_capture(93000);mux_and_stream_h264_frame(frame,sizeof(frame),93000,0,93000);assert(captured[5]==0x10);
    reset_capture(96000);mux_and_stream_h264_frame(frame,sizeof(frame),96000,1,96000);assert(captured[5]==0x50);
    close_cluster_socket();assert(g_video_discontinuity);
    reset_capture(99000);mux_and_stream_h264_frame(frame,sizeof(frame),99000,1,99000);assert(captured[5]==0xd0);
}
static void test_local_transport_clock(void) {
    transport_clock_v28 clock={0};uint64_t pts,pcr_ticks,value;
    assert(transport_clock_at(&clock,8000000,&pts)==0&&pts==90000);
    assert(transport_clock_at(&clock,8000007,&pcr_ticks)==0&&pcr_ticks==90630);
    assert(transport_clock_at(&clock,8000006,&value)==-1);
    assert(transport_clock_at(&clock,8000033,&value)==0&&value==92970);
    assert(transport_clock_at(&clock,8000033,&value)==0&&value==92970);
    assert(transport_clock_at(&clock,UINT64_MAX,&value)==-1);
    uint8_t frame[2000]={0},pes[4096];reset_capture(0);
    mux_and_stream_h264_frame(frame,sizeof(frame),pts,0,pcr_ticks);
    assert(pcr(captured)==pcr_ticks&&video_payload(pes)==sizeof(frame)+14);
    uint64_t encoded_pts=((uint64_t)((pes[9]>>1)&7)<<30)|((uint64_t)pes[10]<<22)|
        ((uint64_t)(pes[11]>>1)<<15)|((uint64_t)pes[12]<<7)|(pes[13]>>1);
    assert(pes[4]==0 && pes[5]==0);
    assert(encoded_pts==pts&&memcmp(pes+14,frame,sizeof(frame))==0);
    reset_capture(0);mux_and_stream_h264_frame(frame,sizeof(frame),(1ULL<<33)-90,0,(1ULL<<33)+90);
    assert(pcr(captured)==90);
    transport_clock_reset(&clock);assert(!clock.started);
    assert(transport_clock_at(&clock,12,&value)==0&&value==90000);
    g_transport_clock=clock;close_cluster_socket();assert(!g_transport_clock.started&&g_video_discontinuity);
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
static void test_permitted_ports(void) {
    int held[ALT_TCP_PORT_LAST-ALT_TCP_PORT_FIRST+1],release_index=-1;
    /* CI may already have sockets/TIME_WAIT in the permitted ephemeral range.
     * Reserve every available port; occupied ports must also be skipped. */
    for(int i=0;i<(int)(sizeof(held)/sizeof(held[0]));i++) {
        held[i]=bind_listener_stream111(ALT_TCP_PORT_FIRST+i);
        if(held[i]>=0)release_index=i;
    }
    assert(release_index>=0);
    assert(bind_permitted_listener_stream111()==-1);
    close(held[release_index]);held[release_index]=-1;
    int selected=bind_permitted_listener_stream111();assert(selected>=0);
    struct sockaddr_in6 a;socklen_t n=sizeof(a);
    assert(getsockname(selected,(struct sockaddr*)&a,&n)==0);
    assert(ntohs(a.sin6_port)==ALT_TCP_PORT_FIRST+release_index);
    close(selected);
    for(int i=0;i<(int)(sizeof(held)/sizeof(held[0]));i++)if(held[i]>=0)close(held[i]);
}
static void test_receiver(void) {
    uint8_t key[16];memset(key,0x47,sizeof(key));
    real_AES_CBCFrame_Init=cbc_stub;g_master_valid=0;cbc_result=-42;
    assert(AES_CBCFrame_Init(NULL,key,key,0)==-42 && !g_master_valid);
    cbc_result=0;assert(AES_CBCFrame_Init(NULL,key,key,0)==0 && !g_master_valid);
    /* Explicit legacy-key fixture; the generic CBC observer must not associate
     * another caller's key with an existing CarPlay session. */
    memcpy(g_master_key,key,sizeof(key));g_master_session=NULL;
    s_AirPlay_DeriveAESKeySHA512ForScreen=derive_ok;
    s_AES_CTR_Init=init_ok; s_AES_CTR_Update=update_bad;
    g_master_valid=0; assert(start_alt_receiver(NULL,1)==-1);
    assert(pthread_mutex_trylock(&g_crypto_lock)==0); pthread_mutex_unlock(&g_crypto_lock);
    g_master_valid=1; s_AES_CTR_Init=init_bad;
    assert(start_alt_receiver(NULL,1)==-1); assert(g_alt_listen_fd==-1);
    s_AES_CTR_Init=init_ok;
    for(int i=0;i<25;i++) {
        int port=start_alt_receiver(NULL,1);assert(port>=ALT_TCP_PORT_FIRST && port<=ALT_TCP_PORT_LAST);
        if(i%3) {
            int family=i%3==1 ? AF_INET : AF_INET6;
            int fd=socket(family,SOCK_STREAM,0); assert(fd>=0);
            if(family==AF_INET) {
                struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons((uint16_t)port);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
                assert(connect(fd,(struct sockaddr*)&a,sizeof(a))==0);
            } else {
                struct sockaddr_in6 a={0};a.sin6_family=AF_INET6;a.sin6_port=htons((uint16_t)port);a.sin6_addr=in6addr_loopback;
                assert(connect(fd,(struct sockaddr*)&a,sizeof(a))==0);
            }
            AirPlayScreenHeaderCompat h={0};h.bodySize=4;h.opcode=K_SCREEN_VIDEO_FRAME;
            uint8_t body[4]={0}; reset_capture(0);
            assert(write(fd,&h,sizeof(h))==(ssize_t)sizeof(h)); assert(write(fd,body,4)==4);
            usleep(10000); assert(captured_len==0); stop_alt_receiver();close(fd);
        } else stop_alt_receiver(); /* also wake blocked accept */
        assert(!g_alt_thread_created && g_alt_listen_fd==-1 && g_alt_client_fd==-1 && !g_alt_aes_valid);
    }
}
typedef struct { const char *items[16]; int64_t count; } FeatureArray;
static FeatureArray feature_copy;
static void *feature_stock, *feature_committed;
static void *requested_stream_array;
static int feature_fail_copy, feature_fail_append;
static void *feature_string(const void *a,const char *s,uint32_t e) {(void)a;(void)e;return (void *)s;}
static const void *feature_get(const void *d,const void *k) {(void)d;return !strcmp(k,"streams") ? requested_stream_array : feature_stock;}
static void feature_set(void *d,const void *k,const void *v) {(void)d;(void)k;feature_committed=(void *)v;}
static void feature_release(const void *p) {(void)p;}
static void *feature_clone(const void *a,int64_t n,const void *p) {
    (void)a;(void)n;if(feature_fail_copy)return NULL;feature_copy=*(const FeatureArray *)p;return &feature_copy;
}
static int64_t feature_count(const void *p) {return ((const FeatureArray *)p)->count;}
static const void *feature_at(const void *p,int64_t n) {return ((const FeatureArray *)p)->items[n];}
static int feature_cstring(const void *p,char *b,int64_t n,uint32_t e) {(void)e;return snprintf(b,(size_t)n,"%s",(const char *)p)<n;}
static int32_t feature_append(void **p,const char *s,size_t n) {
    (void)n;if(feature_fail_append)return -1;
    if(!*p){memset(&feature_copy,0,sizeof(feature_copy));*p=&feature_copy;}
    FeatureArray *a=*p;assert(a->count<16);a->items[a->count++]=s;return 0;
}
static void test_factory_features(void) {
    s_CFStringCreateWithCString=feature_string;s_CFDictionaryGetValue=feature_get;
    s_CFDictionarySetValue=feature_set;s_CFRelease=feature_release;
    s_CFArrayCreateMutableCopy=feature_clone;s_CFArrayGetCount=feature_count;
    s_CFArrayGetValueAtIndex=feature_at;s_CFStringGetCString=feature_cstring;
    s_CFArrayEnsureCreatedAndAppendCString=feature_append;
    FeatureArray original={{"factoryNavigation","factoryFeature"},2};feature_stock=&original;
    set_reference_enabled_features(&original,0);
    assert(feature_committed==&feature_copy && feature_copy.count==3 && original.count==2);
    assert(!strcmp(feature_copy.items[0],"factoryNavigation") && !strcmp(feature_copy.items[2],"altScreen"));
    FeatureArray augmented=feature_copy;feature_stock=&augmented;
    set_reference_enabled_features(&original,1);assert(feature_copy.count==4 && augmented.count==3);
    augmented=feature_copy;feature_stock=&augmented;
    set_reference_enabled_features(&original,1);assert(feature_copy.count==4);
    feature_committed=NULL;feature_fail_copy=1;
    set_reference_enabled_features(&original,1);assert(!feature_committed);feature_fail_copy=0;
    feature_stock=&original;feature_fail_append=1;
    set_reference_enabled_features(&original,1);assert(!feature_committed && original.count==2);
    feature_fail_append=0;feature_stock=NULL;
    set_reference_enabled_features(&original,0);assert(feature_copy.count==1);
    puts("PASS: factory features preserved, deduplicated and unchanged on allocation failure");
}
static int32_t native_security_status;
static void *expected_security_session, *expected_security_context;
static const uint8_t *expected_security_key, *expected_security_iv;
static int32_t native_security_stub(void *s,const uint8_t *k,const uint8_t *v) {
    assert(s==expected_security_session && k==expected_security_key && v==expected_security_iv);
    return native_security_status;
}
static int32_t native_homekit_stub(void *s,void *c) {
    assert(s==expected_security_session && c==expected_security_context);return native_security_status;
}
static void test_security_observers(void) {
    uint8_t key[16]={1},iv[16]={2};int session,context;
    expected_security_session=&session;expected_security_context=&context;
    expected_security_key=key;expected_security_iv=iv;
    real_session_security=native_security_stub;real_homekit_security=native_homekit_stub;
    for(int i=0;i<2;i++) {
        native_security_status=i ? -37 : 0;
        assert(AirPlayReceiverSessionSetSecurityInfo(&session,key,iv)==native_security_status);
        assert(g_master_valid==(i==0));
        if(i==0)assert(g_master_session==&session && !memcmp(g_master_key,key,sizeof(key)));
        assert(AirPlayReceiverSessionSetHomeKitSecurityContext(&session,&context)==native_security_status);
        assert(key[0]==1 && iv[0]==2);
    }
    expected_security_context=NULL;
    assert(AirPlayReceiverSessionSetHomeKitSecurityContext(&session,NULL)==native_security_status);
    puts("PASS: security observers preserve native arguments, keys and success/failure returns");
}
static void *setup_session,*setup_request,*setup_response;
static int32_t setup_result;
static int32_t setup_stub(void *session,void *request,void **response) {
    assert(session==setup_session && request==setup_request);
    if(response)*response=setup_response;return setup_result;
}
static void test_initial_acknowledgement(void) {
    int session,request,response;void *out=NULL;
    setup_session=&session;setup_request=&request;setup_response=&response;setup_result=0;
    real_AirPlayReceiverSessionSetup=setup_stub;feature_stock=NULL;feature_committed=NULL;
    requested_stream_array=NULL;
    assert(AirPlayReceiverSessionSetup(&session,&request,&out)==0 && out==&response);
    assert(feature_committed==&feature_copy && feature_copy.count==1 && !strcmp(feature_copy.items[0],"altScreen"));
    g_layout_advertised=1;feature_committed=NULL;
    assert(AirPlayReceiverSessionSetup(&session,&request,&out)==0);
    assert(feature_copy.count==2 && !strcmp(feature_copy.items[1],"viewAreas"));
    g_layout_advertised=0;
    FeatureArray primary={{"native-primary-stream"},1};requested_stream_array=&primary;feature_committed=NULL;
    assert(AirPlayReceiverSessionSetup(&session,&request,&out)==0 && !feature_committed);
    requested_stream_array=NULL;setup_result=-77;feature_committed=NULL;
    assert(AirPlayReceiverSessionSetup(&session,&request,&out)==-77 && !feature_committed);
    setup_result=0;setup_request=NULL;
    assert(AirPlayReceiverSessionSetup(&session,NULL,&out)==0 && !feature_committed);
    puts("PASS: initial SETUP acknowledges AltScreen; primary media, failed SETUP and NULL request remain unchanged");
}
int main(int argc,char **argv) {
    assert(PRIVATE_CAPTURE_ENABLED==0);
    test_factory_features();
    test_security_observers();
    test_initial_acknowledgement();
    test_revoked_port_release();test_factory_source_port();test_factory_transport_profile();test_mux();test_source_signals();test_local_transport_clock();test_parser();test_permitted_ports();test_receiver();
    AirPlayScreenHeaderCompat timed={0};
    memset(&g_video_clock,0,sizeof(g_video_clock));
    uint64_t ntp=100ULL<<32;memcpy(timed.params,&ntp,8);
    uint64_t first=screen_timestamp_us(&timed);
    ntp+=(1ULL<<30);memcpy(timed.params,&ntp,8);
    assert(screen_timestamp_us(&timed)-first==250000);
    ntp+=(3ULL<<30);memcpy(timed.params,&ntp,8);
    assert(screen_timestamp_us(&timed)-first==1000000);
    ntp=99ULL<<32;memcpy(timed.params,&ntp,8);
    assert(screen_timestamp_us(&timed)>=first+1000000);
    assert(g_video_clock.rebases==1);
    (void)screen_timestamp_us(&timed);assert(g_video_clock.raw_repeats==1);
    memset(&g_video_clock,0,sizeof(g_video_clock));memset(timed.params,0,8);
    first=screen_timestamp_us(&timed);usleep(2000);
    assert(screen_timestamp_us(&timed)>first);
    /* Metadata observation must remain bounded even for unsupported/truncated
     * syntax. Sanitizers exercise every prefix of deterministic noisy data. */
    uint8_t noise[256];for(unsigned i=0;i<256;i++)noise[i]=(uint8_t)(i*73+19);
    for(size_t n=0;n<=sizeof(noise);n++) {
        video_observe_parameter(noise,n);video_observe_config(noise,n);
        video_observe_frame(noise,n,4,100);
    }
    video_observe_reset();
    if(argc==4 && !strcmp(argv[1],"--avcc-fixture")) {
        FILE *in=fopen(argv[2],"rb");assert(in);
        uint32_t n;assert(fread(&n,4,1,in)==1 && n<=512);
        uint8_t config[512];assert(fread(config,1,n,in)==n);
        reset_capture(90000);video_observe_reset();parse_and_store_avcc(config,n);assert(g_sps_pps_len);
        unsigned frames=0;
        while(fread(&n,4,1,in)==1) {
            assert(n && n<1024*1024);uint8_t *frame=malloc(n);assert(frame);
            assert(fread(frame,1,n,in)==n);
            video_observe_frame(frame,n,4,frames+1);
            assert(emit_h264_frame(frame,n,1000000+(uint64_t)frames*40000)==0);
            free(frame);frames++;
        }
        assert(feof(in)&&!ferror(in));fclose(in);assert(frames==60);
        assert(g_video_observe.slices==60 && g_video_observe.parse_fail==0);
        assert(g_video_observe.reference_gaps==0);
        FILE *out=fopen(argv[3],"wb");assert(out);
        assert(fwrite(captured,1,captured_len,out)==captured_len);fclose(out);
        puts("PASS: actual AVC parameter parsing, length-prefix conversion, IDR parameter insertion and RTP/TS output");
    }
    if(argc==3) {
        FILE *in=fopen(argv[1],"rb"); assert(in); size_t length=fread(captured,1,sizeof(captured),in);
        assert(!ferror(in) && feof(in));fclose(in);assert(length>0);
        uint8_t *frame=malloc((size_t)length);assert(frame);memcpy(frame,captured,(size_t)length);
        reset_capture(90000);
        size_t start=0; unsigned frames=0;
        /* Fixture encoder emits AUD NALs: preserve one access unit per PES and
         * advance both PCR and PTS by 3000 ticks for each 30-fps frame. */
        for(size_t i=0;i+4<length;i++) {
            size_t prefix=0;
            if(frame[i]==0 && frame[i+1]==0 && frame[i+2]==1) prefix=3;
            else if(frame[i]==0 && frame[i+1]==0 && frame[i+2]==0 && frame[i+3]==1) prefix=4;
            if(!prefix) continue;
            if((frame[i+prefix]&31)==9 && i>start) {
                expected_rtp=90000+frames*3000;
                if(frames%10==0){emit_pat_packet();emit_pmt_packet();}
                mux_and_stream_h264_frame(frame+start,i-start,expected_rtp,frames%10==0,expected_rtp);
                frames++;start=i;
            }
            i+=prefix-1;
        }
        expected_rtp=90000+frames*3000;
        if(frames%10==0){emit_pat_packet();emit_pmt_packet();}
        mux_and_stream_h264_frame(frame+start,length-start,expected_rtp,frames%10==0,expected_rtp);free(frame);frames++;
        printf("Fixture access units muxed: %u\n",frames);
        FILE *out=fopen(argv[2],"wb");assert(out);assert(fwrite(captured,1,captured_len,out)==captured_len);fclose(out);
    }
    puts("PASS: PCR/PES, malformed AVC, config length, IDR parameter sets, crypto failures and receiver restart/stop");
}
