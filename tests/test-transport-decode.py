from pathlib import Path
import sys,tempfile,subprocess
from codec_fixture_utils import make_clip,decode_hashes
with tempfile.TemporaryDirectory() as temp:
 p=Path(temp);source=p/'frames.h264';output=p/'transport.ts'
 make_clip(source,'baseline',30,30,1,0,10)
 subprocess.run([sys.argv[1],source,output],check=True,timeout=30)
 before,after=decode_hashes(source),decode_hashes(output)
 assert len(before)==30 and before==after
print('PASS: all 30 synthetic pictures decode pixel-identically through production Annex-B transport')
