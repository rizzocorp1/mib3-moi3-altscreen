"""Actual read-only observer: bounded parser fixtures and root loopback capture.
No image or packet bytes are written by the observer; generated fixtures only."""
from pathlib import Path
import os,socket,struct,subprocess,sys,tempfile,time
binary=Path(sys.argv[1]).resolve();source=Path(sys.argv[2]).resolve()
def crc(data):
    value=0xffffffff
    for byte in data:
        value^=byte<<24
        for _ in range(8):value=((value<<1)^(0x04c11db7 if value&0x80000000 else 0))&0xffffffff
    return struct.pack('!I',value)
def ts(pid,section):
    data=bytes([0x47,0x40|(pid>>8),pid&255,0x10,0])+section+crc(section)
    return data+b'\xff'*(188-len(data))
pat=bytes.fromhex('00b00d0064db00000001e3e8')
pmt=bytes.fromhex('02b0120001c30000e011f0001be011f000')
sps_packet=bytes.fromhex('474011100000016764001f')+b'\xff'*(188-11)
payload=struct.pack('!BBHII',0x80,33,5,1000,100000)+ts(0,pat)+ts(1000,pmt)+sps_packet
udp=struct.pack('!HHHH',42989,42800,8+len(payload),0)+payload
ip=struct.pack('!IHBB',6<<28,len(udp),17,5)+socket.inet_pton(socket.AF_INET6,'::1')+socket.inet_pton(socket.AF_INET6,'ff14::2:4')+udp
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory);fixture=root/'fixture';fixture.write_bytes(ip)
    driver=root/'driver.c'
    driver.write_text('''#define PACKET_AUDIT_TEST
#include "SOURCE"
#include <assert.h>
int main(int argc,char **argv){assert(argc==2);FILE *in=fopen(argv[1],"rb"),*out=tmpfile();assert(in&&out);uint8_t bytes[65536];size_t n=fread(bytes,1,sizeof(bytes),in);fclose(in);
for(size_t i=0;i<n;i++){observe(out,bytes,i,PACKET_OUTGOING);assert(used==0);}
observe(out,bytes,n,PACKET_OUTGOING);assert(used==1&&flows[0].port==42989&&flows[0].ssrc==100000&&flows[0].pat&&flows[0].pmt&&flows[0].sps&&flows[0].packets==1);
g_grant=1;observe(out,bytes,n,PACKET_OUTGOING);assert(used==2&&flows[1].grant==1&&flows[1].pat&&flows[1].pmt&&flows[1].sps);summary(out);rewind(out);char text[4096]={0};assert(fread(text,1,sizeof(text)-1,out)>0);assert(strstr(text,"transportId=100 program=1 pmtPid=1000 crcValid=1"));assert(strstr(text,"pcrPid=17 firstStreamType=27 firstStreamPid=17 crcValid=1"));assert(!strstr(text,"::1")&&!strstr(text,"ff14::2:4"));
uint32_t state=12345;for(unsigned j=0;j<2000;j++){size_t length=j%600;for(size_t i=0;i<length;i++){state=state*1664525u+1013904223u;bytes[i]=(uint8_t)(state>>24);}observe(out,bytes,length,PACKET_OUTGOING);}
fclose(out);puts("PASS: bounded RTP/TS metadata, CRC/profile fields, truncated/random packet rejection, no addresses or pictures logged");}
'''.replace('SOURCE',str(source)))
    parser=root/'parser'
    subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(driver),'-o',str(parser)],check=True)
    subprocess.run([parser,fixture],check=True)
    report=root/'metadata.txt';ready=root/'ready';grant=root/'grant'
    process=subprocess.Popen([binary,'lo',report,ready,'3',grant],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    try:
        for _ in range(100):
            if ready.exists():break
            if process.poll() is not None:break
            time.sleep(.02)
        assert ready.exists(),process.communicate()
        receiver=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);receiver.bind(('::1',42800))
        sender=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
        for _ in range(5):sender.sendto(payload,('::1',42800));time.sleep(.03)
        for _ in range(100):
            if 'encoderGrant=0 direction=outgoing' in report.read_text():break
            time.sleep(.01)
        assert 'encoderGrant=0 direction=outgoing' in report.read_text()
        grant.write_text('ready')
        for _ in range(5):sender.sendto(payload,('::1',42800));time.sleep(.03)
        sender.close();receiver.close();stdout,stderr=process.communicate(timeout=6);assert process.returncode==0,(stdout,stderr)
        metadata=report.read_text();assert 'AUDIT ACTIVE' in metadata and 'ssrc=100000' in metadata and 'PAT flow=' in metadata and 'PMT flow=' in metadata and 'SPS flow=' in metadata and 'AUDIT COMPLETE' in metadata
        assert 'encoderGrant=0 direction=outgoing' in metadata and 'encoderGrant=1 direction=outgoing' in metadata
        assert '::1' not in metadata and 'ff14::2:4' not in metadata
    finally:
        if process.poll() is None:process.kill();process.wait()
print('PASS: real IPv6 loopback packet capture, readiness before send, finite lifetime, metadata-only output')
