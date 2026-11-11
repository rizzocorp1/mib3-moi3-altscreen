/* Read-only, bounded IPv6 UDP/RTP/TS metadata observer. No frame bytes, network
 * addresses, promiscuous mode, firewall changes or interface changes. */
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "packet-detail-v30.h"
typedef struct {PacketDetail detail;unsigned port,kind,grant;uint32_t ssrc,first_ts,last_ts;uint16_t seq;
    uint64_t source_tag,packets,bytes,gaps;int pat,pmt,sps,aud;} Flow;
static Flow flows[16];static unsigned used,g_grant;
static uint64_t raw_packets,ipv6_packets,target_udp_packets,rtp_packets;
static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static uint64_t tag(const uint8_t *p){uint64_t v=14695981039346656037ULL;for(int i=0;i<16;i++){v^=p[i];v*=1099511628211ULL;}return v;}
static uint32_t crc(const uint8_t *p,size_t n){uint32_t v=0xffffffff;for(size_t i=0;i<n;i++){v^=(uint32_t)p[i]<<24;for(int b=0;b<8;b++)v=(v<<1)^((v&0x80000000)?0x04c11db7:0);}return v;}
static void observe(FILE *out,const uint8_t *p,size_t n,unsigned kind) {
    if(n<48 || p[0]>>4!=6)return;
    ipv6_packets++;
    size_t end=40+be16(p+4),offset=40;if(end>n)return;
    unsigned next=p[6];
    for(int i=0;next!=17 && i<8;i++) {
        if(offset+8>end)return;
        unsigned following=p[offset];size_t length;
        if(next==0 || next==43 || next==60)length=((size_t)p[offset+1]+1)*8;
        else if(next==44){if(be16(p+offset+2)&0xfff8)return;length=8;}
        else return;
        if(length>end-offset)return;
        offset+=length;next=following;
    }
    if(next!=17 || offset+8>end)return;
    const uint8_t *udp=p+offset;size_t udp_n=be16(udp+4);
    if(be16(udp+2)!=42800 || udp_n<20 || udp_n>end-offset)return;
    target_udp_packets++;
    const uint8_t *r=udp+8;size_t rn=udp_n-8;
    if(r[0]>>6!=2)return;
    size_t header=12+4*(r[0]&15);if(header>rn)return;
    if(r[0]&0x10){if(header+4>rn)return;header+=4+4*(size_t)be16(r+header+2);if(header>rn)return;}
    size_t payload_n=rn-header;
    if(r[0]&0x20){if(!payload_n || !r[rn-1] || r[rn-1]>payload_n)return;payload_n-=r[rn-1];}
    rtp_packets++;
    uint32_t ssrc=be32(r+8);unsigned port=be16(udp);uint64_t source=tag(p+8);unsigned index;
    for(index=0;index<used;index++)if(flows[index].port==port && flows[index].ssrc==ssrc && flows[index].kind==kind && flows[index].source_tag==source && flows[index].grant==g_grant)break;
    if(index==used){
        if(used==16)return;
        used++;
        flows[index]=(Flow){.port=port,.kind=kind,.ssrc=ssrc,.source_tag=source,.grant=g_grant,.first_ts=be32(r+4)};
        static const uint8_t group[16]={0xff,0x14,0,0,0,0,0,0,0,0,0,0,0,2,0,4};
        fprintf(out,"FLOW %u encoderGrant=%u direction=%s sourcePort=%u sourceTag=%016llx ssrc=%u pt=%u marker=%u rtpBytes=%zu tsSlots=%zu clusterGroup=%d firstSeq=%u firstTimestamp=%u hopLimit=%u trafficClass=%u\n",index,g_grant,kind==PACKET_OUTGOING?"outgoing":"incoming",port,(unsigned long long)source,ssrc,r[1]&127,r[1]>>7,rn,payload_n/188,!memcmp(p+24,group,16),be16(r+2),be32(r+4),p[7],((p[0]&15)<<4)|(p[1]>>4));
        fflush(out);
    }
    Flow *f=&flows[index];uint16_t seq=be16(r+2);
    if(f->packets && (uint16_t)(f->seq+1)!=seq)f->gaps++;
    uint64_t observed_us=detail_now_us();detail_packet(&f->detail,observed_us,be32(r+4));
    f->seq=seq;f->last_ts=be32(r+4);f->packets++;f->bytes+=rn;
    for(size_t i=0;i+188<=payload_n;i+=188){
        const uint8_t *t=r+header+i;if(t[0]!=0x47)continue;
        unsigned video_pid=be16(t+1)&0x1fff;
        if(video_pid==17 && (t[3]&0x20) && t[4]>=7 && t[4]<=183 && (t[5]&0x10)){
            f->detail.last_pcr=((uint64_t)t[6]<<25)|((uint64_t)t[7]<<17)|((uint64_t)t[8]<<9)|((uint64_t)t[9]<<1)|(t[10]>>7);f->detail.pcr_valid=1;
        }
        unsigned mode=(t[3]>>4)&3;if(mode!=1 && mode!=3)continue;
        size_t o=4;if(mode==3){o+=1+(size_t)t[4];if(o>=188)continue;}
        unsigned pid=be16(t+1)&0x1fff;
        if(pid==17)detail_video(out,index,&f->detail,t,o,observed_us);
        if(pid==17 && (!f->sps || !f->aud)){
            for(size_t j=o;j+7<=188;j++){
                if(t[j]==0 && t[j+1]==0 && t[j+2]==1 && (t[j+3]&31)==9 && !f->aud){f->aud=1;fprintf(out,"AUD flow=%u found=1\n",index);}
                if(t[j]==0 && t[j+1]==0 && t[j+2]==1 && (t[j+3]&31)==7 && !f->sps){
                    f->sps=1;fprintf(out,"SPS flow=%u profile=%u compatibility=%u level=%u\n",index,t[j+4],t[j+5],t[j+6]);
                }
            }
        }
        if(pid!=0 && pid!=1000)continue;
        if(!(t[1]&0x40))continue;
        o+=1+(size_t)t[o];if(o+3>188)continue;
        const uint8_t *section=t+o;size_t length=3+(be16(section+1)&0x0fff);
        if(length>188-o)continue;
        if(section[0]==0 && !f->pat && length>=16){
            f->pat=1;fprintf(out,"PAT flow=%u transportId=%u program=%u pmtPid=%u crcValid=%d\n",index,be16(section+3),be16(section+8),be16(section+10)&0x1fff,crc(section,length)==0);
        }else if(section[0]==2 && !f->pmt && length>=21){
            size_t es=12+(be16(section+10)&0x0fff);if(es+5>length-4)continue;
            f->pmt=1;fprintf(out,"PMT flow=%u program=%u pcrPid=%u firstStreamType=%u firstStreamPid=%u crcValid=%d\n",index,be16(section+3),be16(section+8)&0x1fff,section[es],be16(section+es+1)&0x1fff,crc(section,length)==0);
        }
    }
}
static void summary(FILE *out){fprintf(out,"SEEN raw=%llu ipv6=%llu targetUdp=%llu rtp=%llu\n",(unsigned long long)raw_packets,(unsigned long long)ipv6_packets,(unsigned long long)target_udp_packets,(unsigned long long)rtp_packets);for(unsigned i=0;i<used;i++){Flow *f=&flows[i];fprintf(out,"COUNT flow=%u packets=%llu bytes=%llu rtpDelta=%u sequenceDiscontinuities=%llu lastSeq=%u lastTimestamp=%u audSeen=%d\n",i,(unsigned long long)f->packets,(unsigned long long)f->bytes,f->last_ts-f->first_ts,(unsigned long long)f->gaps,f->seq,f->last_ts,f->aud);detail_summary(out,i,&f->detail);}fflush(out);}
static uint64_t milliseconds(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
#ifndef PACKET_AUDIT_TEST
int main(int argc,char **argv){
    if(argc==2 && !strcmp(argv[1],"--port-free")) {
        int probe=socket(AF_INET6,SOCK_DGRAM|SOCK_CLOEXEC,0);if(probe<0)return 2;
        struct sockaddr_in6 local={0};local.sin6_family=AF_INET6;local.sin6_port=htons(42989);
        int result=bind(probe,(struct sockaddr*)&local,sizeof(local));close(probe);return result==0?0:1;
    }
    if(argc!=5 && argc!=6)return 2;
    char *tail=NULL;long seconds=strtol(argv[4],&tail,10);if(!tail || *tail || seconds<1 || seconds>600)return 2;
    unsigned index=if_nametoindex(argv[1]);if(!index){perror("audit interface");return 1;}
    /* Outgoing Linux taps are delivered to ETH_P_ALL subscribers. The
     * parser below still admits only bounded IPv6/UDP42800 metadata. */
    int fd=socket(AF_PACKET,SOCK_DGRAM|SOCK_CLOEXEC,htons(ETH_P_ALL));if(fd<0){perror("audit socket");return 1;}
    int requested_buffer=1048576;setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&requested_buffer,sizeof(requested_buffer));
    struct sockaddr_ll address={.sll_family=AF_PACKET,.sll_protocol=htons(ETH_P_ALL),.sll_ifindex=(int)index};
    if(bind(fd,(struct sockaddr*)&address,sizeof(address))){perror("audit bind");close(fd);return 1;}
    /* argv[2] is a new log; argv[3] is a separate readiness path. */
    int output=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(output<0){perror("audit output");close(fd);return 1;}
    FILE *out=fdopen(output,"w");if(!out){close(output);close(fd);return 1;}
    fprintf(out,"AUDIT ACTIVE interface=%s seconds=%ld metadataOnly=1 promiscuous=0 outgoingTap=ETH_P_ALL\n",argv[1],seconds);fflush(out);
    int actual_buffer=0;socklen_t buffer_length=sizeof(actual_buffer);getsockopt(fd,SOL_SOCKET,SO_RCVBUF,&actual_buffer,&buffer_length);fprintf(out,"CAPTURE_BUFFER requestedBytes=1048576 actualBytes=%d\n",actual_buffer);fflush(out);
    int ready=open(argv[3],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
    if(ready<0 || write(ready,"READY\n",6)!=6 || fsync(ready)){perror("audit ready");if(ready>=0)close(ready);fclose(out);close(fd);return 1;}close(ready);
    uint64_t capture_packets=0,capture_drops=0;
    uint64_t start=milliseconds(),last=start;
    while(milliseconds()-start<(uint64_t)seconds*1000){
        struct pollfd event={.fd=fd,.events=POLLIN};int result=poll(&event,1,100);
        if(result<0){if(errno==EINTR)continue;break;}
        if(result>0){g_grant=argc==6 && access(argv[5],F_OK)==0;uint8_t packet[65536];struct sockaddr_ll from;socklen_t length=sizeof(from);ssize_t n=recvfrom(fd,packet,sizeof(packet),0,(struct sockaddr*)&from,&length);if(n>0){raw_packets++;observe(out,packet,(size_t)n,from.sll_pkttype);}}
        if(milliseconds()-last>=5000){struct tpacket_stats stats;socklen_t size=sizeof(stats);if(getsockopt(fd,SOL_PACKET,PACKET_STATISTICS,&stats,&size)==0){capture_packets+=stats.tp_packets;capture_drops+=stats.tp_drops;fprintf(out,"CAPTURE_STATS packets=%llu drops=%llu available=1\n",(unsigned long long)capture_packets,(unsigned long long)capture_drops);}else fprintf(out,"CAPTURE_STATS available=0 errno=%d\n",errno);summary(out);last=milliseconds();}
    }
    {struct tpacket_stats stats;socklen_t size=sizeof(stats);if(getsockopt(fd,SOL_PACKET,PACKET_STATISTICS,&stats,&size)==0){capture_packets+=stats.tp_packets;capture_drops+=stats.tp_drops;fprintf(out,"CAPTURE_STATS packets=%llu drops=%llu available=1\n",(unsigned long long)capture_packets,(unsigned long long)capture_drops);}}
    summary(out);fprintf(out,"AUDIT COMPLETE flows=%u\n",used);fclose(out);close(fd);return 0;
}
#endif
