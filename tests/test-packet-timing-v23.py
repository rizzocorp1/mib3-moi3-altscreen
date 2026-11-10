"""Independent exact PTS/PCR, wrap and malformed-header fixtures; sanitized."""
from pathlib import Path
import subprocess,sys,tempfile
source=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as tmp:
 r=Path(tmp);driver=r/'timing.c'
 driver.write_text(r'''#define PACKET_AUDIT_TEST
#define PACKET_TIMING_TEST
#include "SOURCE"
#include <assert.h>
uint64_t fixture_now_us;
static void make(uint8_t t[188],uint64_t pcr,uint64_t pts){
 memset(t,0xff,188);t[0]=0x47;t[1]=0x40;t[2]=17;t[3]=0x30;t[4]=7;t[5]=0x10;
 t[6]=pcr>>25;t[7]=pcr>>17;t[8]=pcr>>9;t[9]=pcr>>1;t[10]=((pcr&1)<<7)|0x7e;t[11]=0;
 uint8_t *p=t+12;p[0]=0;p[1]=0;p[2]=1;p[3]=0xe0;p[4]=0;p[5]=8;p[6]=0x80;p[7]=0x80;p[8]=5;
 p[9]=0x21|((pts>>29)&14);p[10]=pts>>22;p[11]=((pts>>14)&0xfe)|1;p[12]=pts>>7;p[13]=((pts<<1)&0xfe)|1;
}
int main(void){
 Flow f={0};uint8_t t[188];fixture_now_us=1000000;make(t,90000,117000);video_timing(&f,t,3,12);
 fixture_now_us+=100000;make(t,99000,126000);video_timing(&f,t,3,12);
 assert(f.pts_count==2 && f.pairs==2 && f.min_lead==27000 && f.max_lead==27000);
 assert(f.min_gap_us==100000 && f.max_gap_us==100000 && f.min_pts_step==9000 && f.max_pts_step==9000);
 t[21]&=0xfe;video_timing(&f,t,3,12);assert(f.pts_count==2);
 make(t,99000,126000);video_timing(&f,t,3,187);assert(f.pts_count==2);
 t[5]=0;fixture_now_us+=100000;video_timing(&f,t,3,12);assert(f.pts_count==3 && f.pairs==2);
 make(t,180000,0);video_timing(&f,t,2,188);assert(f.pcr_count==5 && f.pts_count==3);
 Flow wrap={0};make(t,(1ULL<<33)-4500,4500);video_timing(&wrap,t,3,12);assert(wrap.min_lead==9000);
 make(t,9000,18000);fixture_now_us+=100000;video_timing(&wrap,t,3,12);assert(wrap.min_pts_step==13500);
 Flow negative={0};make(t,180000,179910);video_timing(&negative,t,3,12);assert(negative.min_lead==-90);
 assert(delta33(10,(1ULL<<33)-10)==20 && delta33((1ULL<<33)-10,10)==-20);
 puts("PASS: exact300ms lead, negative lead,33bit wrap,100ms cadence, malformed/truncated PTS, stale-PCR refusal, adaptation-only PCR");
}
'''.replace('SOURCE',str(source)))
 binary=r/'fixture'
 subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(driver),'-o',str(binary)],check=True)
 subprocess.run([binary],check=True)
