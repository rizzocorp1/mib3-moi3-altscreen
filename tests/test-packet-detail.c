#define PACKET_AUDIT_TEST
#include "../recovery/packet-audit-v30.c"
#include <assert.h>
int main(void){
 FILE *out=tmpfile();assert(out);PacketDetail d={0};
 const uint8_t a[]={0,0},b[]={0,1,0x67,0x64,0,0x1f,0xaa,0,0},c[]={1,0x68,0xee,0x3c,0xb0,0,0,1,0x65};
 detail_annexb(out,0,&d,a,sizeof(a));detail_annexb(out,0,&d,b,sizeof(b));detail_annexb(out,0,&d,c,sizeof(c));
 detail_annexb(out,0,&d,(const uint8_t*)"PRIVATE_PICTURE_PAYLOAD",23);
 assert(d.sps_size==5 && d.pps_size==4 && d.previous_sps[0]==0x67 && d.previous_pps[0]==0x68);
 uint8_t large[700];memset(large,0xab,sizeof(large));uint8_t start[]={0,0,1,0x67},end[]={0,0,1,0x65};
 detail_annexb(out,0,&d,start,sizeof(start));detail_annexb(out,0,&d,large,sizeof(large));detail_annexb(out,0,&d,end,sizeof(end));assert(d.sps_size==5);
 detail_packet(&d,1000000,1000);detail_packet(&d,1000010,1000);detail_packet(&d,1000010,1000);
 assert(d.max_bucket==3 && d.min_gap_us==0 && d.max_same_clock==3);
 uint8_t t[188];memset(t,0xff,sizeof(t));t[0]=0x47;t[1]=0x40;t[2]=17;t[3]=0x30;t[4]=7;t[5]=0x10;
 size_t o=12;t[o]=0;t[o+1]=0;t[o+2]=1;t[o+3]=0xe0;t[o+4]=0;t[o+5]=0;t[o+6]=0x80;t[o+7]=0x80;t[o+8]=5;
 uint64_t pts=99000;t[o+9]=0x21|((pts>>29)&14);t[o+10]=pts>>22;t[o+11]=((pts>>14)&254)|1;t[o+12]=pts>>7;t[o+13]=((pts<<1)&254)|1;
 d.last_pcr=90000;d.pcr_valid=1;detail_video(out,0,&d,t,o,1000050);assert(d.lead_samples==1&&d.lead_min==9000&&d.lead_max==9000&&d.pes_count==1);
 detail_video(out,0,&d,t,o,1000060);assert(d.cc_duplicates==1);t[3]=0x33;detail_video(out,0,&d,t,o,1000070);assert(d.cc_gaps==1);
 detail_summary(out,0,&d);rewind(out);char text[8192]={0};assert(fread(text,1,sizeof(text)-1,out)>0);
 assert(strstr(text,"hex=6764001faa") && strstr(text,"hex=68ee3cb0") && strstr(text,"maxPacketsIn1ms=3"));
 assert(!strstr(text,"PRIVATE_PICTURE") && !strstr(text,"505249564154455f50494354555245"));
 fclose(out);puts("PASS: split codec NALs reassembled, oversized codec bounded, slice pixels never logged, burst/PES/PCR/TS continuity metadata exact");return 0;
}
