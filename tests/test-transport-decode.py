from pathlib import Path
import sys,tempfile,subprocess
from codec_fixture_utils import make_clip,decode_hashes
with tempfile.TemporaryDirectory() as temp:
 p=Path(temp);source=p/'frames.h264';output=p/'transport.ts'
 make_clip(source,'baseline',30,30,1,0,10)
 subprocess.run([sys.argv[1],source,output],check=True,timeout=30)
 packets=output.read_bytes();starts=0
 for i in range(0,len(packets),188):
  t=packets[i:i+188]
  if len(t)!=188 or t[0]!=0x47 or ((t[1]&31)<<8|t[2])!=17 or not(t[1]&64):continue
  mode=(t[3]>>4)&3;o=4+(1+t[4] if mode==3 else 0)
  if t[o:o+4]==b'\0\0\1\xe0':assert t[o+4:o+6]==b'\0\0';starts+=1
 assert starts==30,starts
 before,after=decode_hashes(source),decode_hashes(output)
 assert len(before)==30 and before==after
print('PASS: all 30 synthetic pictures decode pixel-identically through production Annex-B transport')
