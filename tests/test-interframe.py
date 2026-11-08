"""Synthetic video only: verify every decoded picture through production AVC path.

No vehicle data, credentials, firmware or captured picture payloads are inputs.
The fixture strips AUDs, uses four-byte AVC lengths and one IDR per 60 pictures.
"""
import re,struct,subprocess,sys,tempfile
from pathlib import Path

def run(*args):
    return subprocess.run(args,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE).stdout

def nals(data):
    starts=list(re.finditer(b'\x00\x00(?:\x00)?\x01',data))
    return [data[m.end():starts[i+1].start() if i+1<len(starts) else len(data)] for i,m in enumerate(starts)]

def hashes(path):
    data=run('ffmpeg','-v','error','-i',str(path),'-map','0:v:0','-fps_mode','passthrough','-f','framemd5','-')
    return [line.rsplit(b',',1)[-1].strip() for line in data.splitlines() if line and not line.startswith(b'#')]

with tempfile.TemporaryDirectory() as temp:
    root=Path(temp)
    for profile,options in [('baseline','cabac=0:8x8dct=0'),('high','cabac=1:8x8dct=1')]:
        source=root/(profile+'.h264');fixture=root/(profile+'.avcc');output=root/(profile+'.ts')
        run('ffmpeg','-v','error','-f','lavfi','-i','testsrc2=size=1280x480:rate=25',
            '-frames:v','60','-c:v','libx264','-profile:v',profile,'-level:v','3.1',
            '-preset','veryfast','-tune','zerolatency','-x264-params',
            'aud=1:keyint=60:min-keyint=60:scenecut=0:bframes=0:ref=1:slices=1:'+options,
            '-f','h264',str(source))
        units=[];current=[];sps=pps=None
        for nal in nals(source.read_bytes()):
            kind=nal[0]&31
            if kind==9:
                if current:units.append(current);current=[]
            elif kind==7:sps=nal
            elif kind==8:pps=nal
            elif kind in (1,5):current.append(nal)
        if current:units.append(current)
        assert sps and pps and len(units)==60
        assert sps[1]==(66 if profile=='baseline' else 100)
        assert sum(any(n[0]&31==5 for n in unit) for unit in units)==1
        config=bytes([1,sps[1],sps[2],sps[3],255,225])+struct.pack('>H',len(sps))+sps+bytes([1])+struct.pack('>H',len(pps))+pps
        with fixture.open('wb') as out:
            out.write(struct.pack('<I',len(config))+config)
            for unit in units:
                body=b''.join(struct.pack('>I',len(n))+n for n in unit)
                out.write(struct.pack('<I',len(body))+body)
        run(sys.argv[1],'--avcc-fixture',str(fixture),str(output))
        before,after=hashes(source),hashes(output)
        assert len(before)==60 and after==before,(profile,len(before),len(after))
        print('PASS:',profile,'60/60 pictures pixel-identical through actual AVC conversion; one IDR, no AUDs, 59 dependent pictures')
