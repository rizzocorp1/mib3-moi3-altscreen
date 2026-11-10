"""Generate non-personal codec controls; offline only, no vehicle deployment."""
from pathlib import Path
from fractions import Fraction
import sys,json,struct,re,hashlib
r=Path(__file__).resolve().parents[1];sys.path.insert(0,str(r/'tools-local/pyav'));import av
out=r/'firmware-audit/codec-refresh-audit/synthetic-controls-validated';out.mkdir(exist_ok=False)
W,H,FPS,COUNT=1280,480,10,80
segments={'0':'abcedf','1':'bc','2':'abged','3':'abgcd','4':'fgbc','5':'afgcd','6':'afgecd','7':'abc','8':'abcdefg','9':'abfgcd'}
rects={'a':(5,0,35,5),'b':(35,5,40,35),'c':(35,40,40,70),'d':(5,70,35,75),'e':(0,40,5,70),'f':(0,5,5,35),'g':(5,35,35,40)}
def frame(index,phase):
 f=av.VideoFrame(W,H,'yuv420p');stride=f.planes[0].line_size;y=bytearray([32])*f.planes[0].buffer_size
 def box(x0,y0,x1,y1,value):
  for yy in range(y0,y1):y[yy*stride+x0:yy*stride+x1]=bytes([value])*(x1-x0)
 def number(n,x,ypos):
  for digit in str(n):
   for s in segments[digit]:
    a,b,c,d=rects[s];box(x+a,ypos+b,x+c,ypos+d,225)
   x+=52
 number(phase,620,90);number(index,590 if index<10 else 564,220)
 box(440,370,840,374,100);pos=440+(index*7)%360;box(pos,385,pos+40,415,225)
 f.planes[0].update(y)
 for p in f.planes[1:]:p.update(bytes([128])*p.buffer_size)
 f.pts=index;f.time_base=Fraction(1,FPS);return f
cases=[('baseline-ref1-p', 'baseline',1,0,20),('high-ref1-p','high',1,1,20),
       ('high-ref4-p','high',4,1,20),('high-intra','high',1,1,1)]
reports=[]
for phase,(name,profile,refs,cabac,keyint) in enumerate(cases,1):
 c=av.CodecContext.create('libx264','w');c.width=W;c.height=H;c.pix_fmt='yuv420p';c.time_base=Fraction(1,FPS);c.framerate=Fraction(FPS)
 c.options={'preset':'veryfast','tune':'zerolatency','profile':profile,'level':'3.1','crf':'24',
  'x264-params':f'bframes=0:ref={refs}:cabac={cabac}:8x8dct={cabac}:keyint={keyint}:min-keyint={keyint}:scenecut=0:repeat-headers=1:aud=1'}
 packets=[]
 for i in range(COUNT):packets.extend(c.encode(frame(i,phase)))
 packets.extend(c.encode(None));assert len(packets)==COUNT
 blob=b''.join(bytes(p) for p in packets);(out/(name+'.h264')).write_bytes(blob)
 records=b'MOI3VC1\n'+b''.join(struct.pack('<IQ',p.size,90000+i*9000)+bytes(p) for i,p in enumerate(packets))
 # Synthetic extended playback file is NOT a private-capture file: its duration
 # exceeds the capture export bounds. Separate magic prevents accidental use.
 (out/(name+'.synthetic')).write_bytes(b'MOI3SYN1'+records[8:])
 dec=av.CodecContext.create('h264','r');dec.options={'err_detect':'explode'};frames=[]
 for p in packets:frames.extend(dec.decode(p))
 frames.extend(dec.decode(None));assert len(frames)==COUNT and not any(f.is_corrupt for f in frames)
 nals=[n for n in re.split(b'\x00\x00\x00?\x01',blob) if n];sps=next(n for n in nals if n[0]&31==7)
 idrs=sum(1 for p in packets if p.is_keyframe)
 assert idrs==(COUNT if keyint==1 else 4),(name,idrs)
 reports.append({'phase':phase,'name':name,'pictures':len(frames),'fps':FPS,'seconds':COUNT/FPS,'idrPictures':idrs,
  'decodedProfile':str(dec.profile),'spsProfile':sps[1],'spsConstraints':sps[2],'spsLevel':sps[3],
  'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest(),'corruptPictures':0})
(out/'MANIFEST.json').write_text(json.dumps({'synthetic':True,'deployed':False,'canvas':[W,H],
 'purpose':'Compare dependent-picture playback across Baseline, High/ref1, High/ref4 and all-intra controls using the same motion/counter generator.',
 'limits':'Requires an isolated bounded vehicle playback runner that does not yet exist. Offline decoding proves file integrity only. All-intra has different bitrate and SPS constraints; it is a control, not a perfectly isolated profile comparison.',
 'cases':reports},indent=2))
print(json.dumps(reports,indent=2))
