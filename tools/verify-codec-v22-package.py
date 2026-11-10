from pathlib import Path
import zipfile,hashlib,json,re
r=Path(__file__).resolve().parents[1];h=lambda b:hashlib.sha256(b).hexdigest()
carrier=r/'firmware-audit/LG-wipe-FAZIT.zip';assert h(carrier.read_bytes())=='763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d'
target=r/'firmware-audit/moi3-codec-comparison-0841-v22.zip'
required={'payload/synthetic-player','payload/file-guard','payload/codec-test.service','payload/controller.sh','payload/oneshot-boot.sh','payload/baseline-ref1-p.synthetic','payload/high-ref1-p.synthetic','payload/high-ref4-p.synthetic','payload/high-intra.synthetic','fix_hw_partnumber.sh','CODEC_TEST_V22.txt','READ-ME-FIRST.txt'}
paths={'player':'implementation/synthetic-player-v22.c','transport':'implementation/synthetic-transport-v20.h','guard':'recovery/file-guard.c','resource':'recovery/resource-audit-v17.h','export':'recovery/video-export-v16.h','controller':'recovery/codec-v22-controller.sh.in','boot':'recovery/codec-v22-boot.sh.in','entry':'recovery/codec-v22-usb.sh.in','service':'recovery/codec-v22.service.in'}
with zipfile.ZipFile(carrier) as src,zipfile.ZipFile(target) as z:
 assert len(z.namelist())==len(set(z.namelist()))
 for name in src.namelist():
  if name!='fix_hw_partnumber.sh':assert src.read(name)==z.read(name)
 assert set(z.namelist())==set(src.namelist())|required|{'build-manifest.json'}
 m=json.loads(z.read('build-manifest.json'));assert set(m['files'])==required
 assert m['format']=='moi3-codec-test-v22' and m['launcher']=='volatile-service-file' and m['factoryExecutableChanged'] is False and m['newRawCapture'] is False
 for name,digest in m['files'].items():assert h(z.read(name))==digest,name
 for key,name in paths.items():assert h((r/name).read_bytes())==m['sourceHashes'][key],name
 for source,name in [('codec-v22.service.in','payload/codec-test.service'),('codec-v22-controller.sh.in','payload/controller.sh'),('codec-v22-boot.sh.in','payload/oneshot-boot.sh'),('codec-v22-usb.sh.in','fix_hw_partnumber.sh')]:
  text=(r/'recovery'/source).read_text()
  for key,value in m['factoryHashes'].items():text=text.replace('@'+key+'@',value)
  assert not re.search('@[A-Z0-9_]+@',text) and text.encode()==z.read(name),name
 assert z.read('payload/synthetic-player')==(r/'firmware-audit/codec-v22-build/synthetic-player').read_bytes()
 assert b'systemd-run --' not in z.read('payload/oneshot-boot.sh')
 assert z.read('READ-ME-FIRST.txt')==(r/'firmware-audit/CODEC-V22-TEST-INSTRUCTIONS.md').read_bytes()
 # These are actual unit-file parser table entries in the matching firmware,
 # not assumptions based only on a newer Linux test machine.
 binary=(r/'firmware-audit/systemd-compat-audit/lib/systemd/systemd').read_bytes()
 for key in ('ExecStart','ExecStopPost','RuntimeMaxSec','TimeoutStopSec','KillMode','Type'):assert ('Service.'+key).encode()+b'\0' in binary,key
 with zipfile.ZipFile(r/'firmware-audit/moi3-codec-comparison-0841-v20.zip') as old:
  for name in required:
   if name.endswith('.synthetic') or name in ('payload/file-guard',):assert old.read(name)==z.read(name)
print(json.dumps({'vendorUnchanged':True,'payloadFilesVerified':len(required),'sourceTemplatesMatch':True,'unitDirectivesPresentInMatchingFirmwareParser':True,'clipsUnchanged':True,'onCarValidated':False,'sha256':h(target.read_bytes())},indent=2))
