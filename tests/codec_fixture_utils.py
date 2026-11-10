"""Original synthetic fixtures and independent FFmpeg/PyAV pixel comparison."""
import sys,hashlib
from pathlib import Path
from fractions import Fraction
try:import av
except ImportError:
 sys.path[:0]=['/tmp/codec-pyav',str(Path(__file__).resolve().parents[1]/'tools-local/pyav')]
 import av
def make_clip(path,profile,fps,count,refs,cabac,keyint):
 c=av.CodecContext.create('libx264','w');c.width=1280;c.height=480;c.pix_fmt='yuv420p';c.time_base=Fraction(1,fps);c.framerate=Fraction(fps)
 c.options={'profile':profile,'level':'3.1','preset':'veryfast','tune':'zerolatency','crf':'24',
  'x264-params':f'aud=1:keyint={keyint}:min-keyint={keyint}:scenecut=0:bframes=0:ref={refs}:cabac={cabac}:8x8dct={cabac}:repeat-headers=1:slices=1'}
 packets=[]
 for i in range(count):
  f=av.VideoFrame(1280,480,'yuv420p');f.pts=i;f.time_base=Fraction(1,fps)
  y=bytearray([32+i%120])*f.planes[0].buffer_size;x=400+(i*7)%400
  for row in range(200,240):y[row*f.planes[0].line_size+x:row*f.planes[0].line_size+x+40]=bytes([225])*40
  f.planes[0].update(y)
  for p in f.planes[1:]:p.update(bytes([128])*p.buffer_size)
  packets.extend(c.encode(f))
 packets.extend(c.encode(None));assert len(packets)==count
 Path(path).write_bytes(b''.join(bytes(p) for p in packets))
def decode_hashes(path):
 hashes=[]
 with av.open(str(path)) as container:
  for f in container.decode(video=0):
   assert not f.is_corrupt
   f=f.reformat(format='yuv420p');h=hashlib.sha256()
   for p in f.planes:
    b=bytes(p)
    for row in range(p.height):h.update(b[row*p.line_size:row*p.line_size+p.width])
   hashes.append(h.hexdigest())
 return hashes
