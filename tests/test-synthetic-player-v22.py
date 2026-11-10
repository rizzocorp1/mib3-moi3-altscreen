"""Independent generated fixtures, parser refusal, real isolated UDP and decode."""
from pathlib import Path
import os,sys,subprocess,tempfile,struct,re,shutil,socket,time,json,signal
from codec_fixture_utils import make_clip,decode_hashes
player=Path(sys.argv[1]).resolve()
names=['baseline-ref1-p','high-ref1-p','high-ref4-p','high-intra']
def fixtures(root):
 for n,name in enumerate(names):
  profile='baseline' if n==0 else 'high';keyint=1 if n==3 else 20;refs=4 if n==2 else 1;cabac=0 if n==0 else 1
  raw=root/(name+'.h264')
  make_clip(raw,profile,10,80,refs,cabac,keyint)
  b=raw.read_bytes();starts=[m.start() for m in re.finditer(b'\x00\x00(?:\x00)?\x01',b) if b[m.end()]&31==9];assert len(starts)==80 and starts[0]==0
  units=[b[start:(starts[i+1] if i+1<len(starts) else len(b))] for i,start in enumerate(starts)]
  (root/(name+'.synthetic')).write_bytes(b'MOI3SYN1'+b''.join(struct.pack('<IQ',len(au),90000+i*9000)+au for i,au in enumerate(units)))
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);fixtures(root)
 subprocess.run([player,'--validate',root],check=True,timeout=5)
 p=root/(names[0]+'.synthetic');original=p.read_bytes()
 for mode in ('magic','truncated','oversize','timestamp','missing-idr','trailing','symlink','hardlink','fifo'):
  b=bytearray(original)
  if mode=='magic':b[:8]=b'NOT_VIDEO'
  elif mode=='truncated':b=b[:-3]
  elif mode=='oversize':struct.pack_into('<I',b,8,262145)
  elif mode=='timestamp':struct.pack_into('<Q',b,12,89999)
  elif mode=='missing-idr':
   first_size=struct.unpack_from('<I',b,8)[0]
   for m in re.finditer(b'\x00\x00(?:\x00)?\x01',b[20:20+first_size]):
    if b[20+m.end()]&31==5:b[20+m.end()]=0x61
  elif mode=='trailing':b+=b'extra'
  if mode in ('symlink','hardlink','fifo'):
   p.unlink();other=root/'untouched';other.write_bytes(original)
   if mode=='symlink':p.symlink_to(other)
   elif mode=='hardlink':os.link(other,p)
   else:os.mkfifo(p)
  else:p.write_bytes(b)
  result=subprocess.run([player,'--validate',root],capture_output=True,timeout=5);assert result.returncode!=0,mode
  p.unlink();p.write_bytes(original)
  if (root/'untouched').exists():(root/'untouched').unlink()
 print('PASS: all four valid fixtures accepted; malformed/aliased/nonregular data refused without network activation')
 if '--network' not in sys.argv:sys.exit(0)
 assert os.geteuid()==0
 subprocess.run(['ip','link','set','lo','up'],check=True)
 subprocess.run(['ip','link','add','eth0.5','type','dummy'],check=True)
 subprocess.run(['ip','link','set','eth0.5','multicast','on'],check=True)
 subprocess.run(['ip','link','set','eth0.5','up'],check=True)
 subprocess.run(['ip','-6','addr','add','fd00:20::1/64','dev','eth0.5','nodad'],check=True)
 subprocess.run(['ip','-6','route','add','ff00::/8','dev','eth0.5'],check=True)
 rx=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);rx.bind(('::',42800));rx.settimeout(.3)
 rx.setsockopt(socket.IPPROTO_IPV6,socket.IPV6_JOIN_GROUP,socket.inet_pton(socket.AF_INET6,'ff14::2:4')+struct.pack('@I',socket.if_nametoindex('eth0.5')))
 busy=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);busy.bind(('::',42989))
 assert subprocess.run([player,'--play',root],capture_output=True,timeout=5).returncode!=0;busy.close()
 start=time.monotonic();proc=subprocess.Popen([player,'--play',root],stdout=subprocess.PIPE,stderr=subprocess.STDOUT);packets=[];arrivals=[]
 while proc.poll() is None:
  try:
   b,source=rx.recvfrom(2000);assert len(b)==1140 and source[1]==42989 and b[1]==33
   packets.append(b);arrivals.append(time.monotonic()-start)
  except socket.timeout:pass
 output=proc.communicate(timeout=5)[0].decode();assert proc.returncode==0,output
 elapsed=time.monotonic()-start;assert 31.5<=elapsed<=36,elapsed
 assert 'COMPLETE frames=320' in output and packets
 assert len(re.findall(r'TIMING window=',output))==4 and 'PHASE ' not in output
 assert output.count('New source clock marked discontinuous')==1
 summaries=re.findall(r'TIMING window=(\d+) frames=80 maxLateUs=(\d+) over5ms=(\d+) over20ms=(\d+) minStartGapUs=(\d+) maxStartGapUs=(\d+) maxMuxSendUs=(\d+) minPtsMinusPcr90k=(-?\d+) maxPtsMinusPcr90k=(-?\d+)',output)
 assert len(summaries)==4
 for row in summaries:
  w,late,n5,n20,ming,maxg,mux,low,high=map(int,row)
  assert 1<=w<=4 and 0<=n20<=n5<=80 and late<=250000 and 0<ming<=maxg and low<=high<=0
  assert -late*90//1000-1<=low
 print(output)
 for i,pkt in enumerate(packets):assert struct.unpack_from('!H',pkt,2)[0]==i%65536
 # First packets span all eight-second phases instead of an immediate burst.
 assert arrivals[-1]-arrivals[0]>31
 ts=root/'controls.ts';ts.write_bytes(b''.join(p[12:] for p in packets))
 decoded=decode_hashes(ts);expected=[]
 expected=decode_hashes(root/(names[0]+'.h264'))*4
 assert len(decoded)==320 and decoded==expected,(len(decoded),len(expected))
 assert subprocess.run([player,'--port-free']).returncode==0
 # Interrupt active playback: bounded exit releases the exclusive source port.
 proc=subprocess.Popen([player,'--play',root],stdout=subprocess.PIPE);time.sleep(.4);proc.terminate();proc.communicate(timeout=3)
 assert proc.returncode!=0 and subprocess.run([player,'--port-free']).returncode==0
 print('PASS: real isolated multicast, exclusive source port, 32-second paced playback, 320 decoded pictures, continuity, interruption and release')
