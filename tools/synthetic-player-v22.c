#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
static volatile sig_atomic_t stopped;
static void stop(int signal_number){(void)signal_number;stopped=1;}
static void log_msg(const char *tag,const char *fmt,...){
    va_list ap;va_start(ap,fmt);printf("[%s] ",tag);vprintf(fmt,ap);putchar('\n');va_end(ap);
}
#include "synthetic-transport-v20.h"
static const char *names[]={"baseline-ref1-p.synthetic","high-ref1-p.synthetic","high-ref4-p.synthetic","high-intra.synthetic"};
typedef struct {uint8_t *bytes;size_t length;size_t offset[80];uint32_t size[80];} Clip;
static uint64_t read_le(const uint8_t *p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
static int contains_idr(const uint8_t *p,size_t n){
    for(size_t i=0;i+4<n;i++)if(!p[i]&&!p[i+1]&&p[i+2]==1&&(p[i+3]&31)==5)return 1;
    return 0;
}
static int load_clips(const char *directory,Clip clips[4]){
    int dir=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(dir<0)return -1;
    for(unsigned c=0;c<4;c++){
        int fd=openat(dir,names[c],O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);struct stat st;
        if(fd<0){close(dir);return -1;}
        if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_size<100||st.st_size>1048576){close(fd);close(dir);return -1;}
        Clip *clip=&clips[c];clip->length=(size_t)st.st_size;clip->bytes=malloc(clip->length);if(!clip->bytes){close(fd);close(dir);return -1;}
        size_t used=0;while(used<clip->length){ssize_t n=read(fd,clip->bytes+used,clip->length-used);if(n<=0){close(fd);close(dir);return -1;}used+=(size_t)n;}close(fd);
        if(memcmp(clip->bytes,"MOI3SYN1",8)){close(dir);return -1;}
        size_t pos=8;unsigned idrs=0;
        for(unsigned i=0;i<80;i++){
            if(clip->length-pos<12){close(dir);return -1;}
            uint32_t n=(uint32_t)read_le(clip->bytes+pos,4);uint64_t pts=read_le(clip->bytes+pos+4,8);pos+=12;
            if(!n||n>262144||n>clip->length-pos||pts!=90000+(uint64_t)i*9000){close(dir);return -1;}
            if(n<5||clip->bytes[pos]||clip->bytes[pos+1]||(!(clip->bytes[pos+2]==1)&&!(clip->bytes[pos+2]==0&&clip->bytes[pos+3]==1))){close(dir);return -1;}
            int idr=contains_idr(clip->bytes+pos,n);if(i==0&&!idr){close(dir);return -1;}idrs+=(unsigned)idr;
            clip->offset[i]=pos;clip->size[i]=n;pos+=n;
        }
        if(pos!=clip->length||idrs!=(c==3?80u:4u)){close(dir);return -1;}
    }
    close(dir);return 0;
}
static int now_us(uint64_t *out){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return -1;*out=(uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;return 0;}
static int pace(uint64_t deadline){
    struct timespec t={(time_t)(deadline/1000000),(long)((deadline%1000000)*1000)};
    int result;do{result=clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&t,NULL);}while(result==EINTR&&!stopped);
    uint64_t now;if(result||stopped||now_us(&now)||now>deadline+250000)return -1;return 0;
}
int main(int argc,char **argv){
    setvbuf(stdout,NULL,_IONBF,0);signal(SIGTERM,stop);signal(SIGINT,stop);signal(SIGALRM,stop);alarm(45);
    if(argc==2&&!strcmp(argv[1],"--port-free")){
        int fd=socket(AF_INET6,SOCK_DGRAM,0);if(fd<0)return 2;int good=bind_cluster_source(fd)==0;close(fd);return good?0:2;
    }
    if(argc!=3||(strcmp(argv[1],"--validate")&&strcmp(argv[1],"--play")))return 2;
    Clip clips[4]={0};int result=2;
    if(load_clips(argv[2],clips)){puts("ABORT: clip validation failed; no video sent");goto done;}
    puts("VALIDATED: V22 validated controls; playback uses Baseline/ref1 only, 10 fps, 32 seconds; counter repeats every eight seconds");
    if(!strcmp(argv[1],"--validate")){result=0;goto done;}
    init_cluster_network();if(g_rtp_sock<0)goto done;
    uint64_t start,previous=0;if(now_us(&start))goto done;
    for(unsigned c=0;c<4;c++){
        if(c==0)g_video_discontinuity=1;
        printf("WINDOW %u baseline-ref1-p.synthetic; same codec, counter 0-79\n",c+1);
        uint64_t max_late=0,late5=0,late20=0,max_mux=0,min_gap=UINT64_MAX,max_gap=0;
        int64_t min_lead=INT64_MAX,max_lead=INT64_MIN;
        for(unsigned i=0;i<80;i++){
            uint64_t deadline=start+((uint64_t)c*80+i)*100000;
            if(pace(deadline)){puts("ABORT: interrupted or pacing deadline missed; no catch-up burst");goto done;}
            if(i%20==0){emit_pat_packet();emit_pmt_packet();}
            uint64_t pcr_now;if(now_us(&pcr_now))goto done;
            uint64_t late=pcr_now>deadline?pcr_now-deadline:0;
            if(late>max_late)max_late=late;
            late5+=(late>5000);late20+=(late>20000);
            if(previous){uint64_t gap=pcr_now-previous;if(gap<min_gap)min_gap=gap;if(gap>max_gap)max_gap=gap;}previous=pcr_now;
            uint64_t pts=90000+((uint64_t)c*80+i)*9000,pcr=90000+(pcr_now-start)*90/1000;
            int64_t lead=(int64_t)pts-(int64_t)pcr;
            if(lead<min_lead)min_lead=lead;
            if(lead>max_lead)max_lead=lead;
            const uint8_t *data=clips[0].bytes+clips[0].offset[i];
            mux_and_stream_h264_frame(data,clips[0].size[i],pts,contains_idr(data,clips[0].size[i]),pcr);
            uint64_t sent_at;if(now_us(&sent_at))goto done;
            if(sent_at-pcr_now>max_mux)max_mux=sent_at-pcr_now;
            if(g_total_frames_sent!=(uint64_t)c*80+i+1){puts("ABORT: access unit not fully muxed");goto done;}
            if(g_send_errors){puts("ABORT: UDP send error");goto done;}
        }
        printf("TIMING window=%u frames=80 maxLateUs=%llu over5ms=%llu over20ms=%llu minStartGapUs=%llu maxStartGapUs=%llu maxMuxSendUs=%llu minPtsMinusPcr90k=%lld maxPtsMinusPcr90k=%lld\n",c+1,(unsigned long long)max_late,(unsigned long long)late5,(unsigned long long)late20,(unsigned long long)min_gap,(unsigned long long)max_gap,(unsigned long long)max_mux,(long long)min_lead,(long long)max_lead);
    }
    if(pace(start+32000000))goto done;
    printf("COMPLETE frames=%llu packets=%llu errors=%llu\n",(unsigned long long)g_total_frames_sent,(unsigned long long)g_sent_packets,(unsigned long long)g_send_errors);result=0;
done:
    if(g_rtp_sock>=0)close(g_rtp_sock);
    for(unsigned c=0;c<4;c++)free(clips[c].bytes);
    return result;
}
