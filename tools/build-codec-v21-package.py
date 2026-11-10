"""Reuse verified original player/clips; render the corrected runtime-unit launcher."""
from pathlib import Path
import zipfile,hashlib,json
r=Path(__file__).resolve().parents[1];h=lambda b:hashlib.sha256(b).hexdigest()
base=r/'firmware-audit/moi3-codec-comparison-0841-v20.zip'
assert h(base.read_bytes())=='1b779d599aca40778c054280fa25647a953d6a691a60753740c152127285eb68'
out=r/'firmware-audit/moi3-codec-comparison-0841-v21.zip';assert not out.exists()
with zipfile.ZipFile(base) as z:
 old=json.loads(z.read('build-manifest.json'));files={}
 for name in ('synthetic-player','file-guard','baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic'):
  p='payload/'+name;b=z.read(p);assert h(b)==old['files'][p];files[p]=b
 v={k:str(value) for k,value in old['factoryHashes'].items() if k not in ('CONTROLLER_SHA','BOOT_SHA')}
 v.update(ROOT='',STAGE='/lge/app_rw/smartlink/carplay/.moi3-codec-test-v21')
 def render(name):
  text=(r/'recovery'/name).read_text()
  for key,value in v.items():text=text.replace('@'+key+'@',value)
  import re
  assert not re.search('@[A-Z0-9_]+@',text),name
  return text.encode()
 files['payload/codec-test.service']=render('codec-v21.service.in');v['UNIT_SHA']=h(files['payload/codec-test.service'])
 files['payload/controller.sh']=render('codec-v21-controller.sh.in');v['CONTROLLER_SHA']=h(files['payload/controller.sh'])
 files['payload/oneshot-boot.sh']=render('codec-v21-boot.sh.in');v['BOOT_SHA']=h(files['payload/oneshot-boot.sh'])
 files['fix_hw_partnumber.sh']=render('codec-v21-usb.sh.in')
 files['CODEC_TEST_V21.txt']=b'Parked-only four-phase codec test with corrected runtime-unit launcher.\n'
 files['READ-ME-FIRST.txt']=(r/'firmware-audit/CODEC-V21-TEST-INSTRUCTIONS.md').read_bytes()
 sources={}
 for key,path in {'player':'implementation/synthetic-player-v20.c','transport':'implementation/synthetic-transport-v20.h','guard':'recovery/file-guard.c','resource':'recovery/resource-audit-v17.h','export':'recovery/video-export-v16.h'}.items():
  sources[key]=h((r/path).read_bytes());assert sources[key]==old['sourceHashes'][key]
 for key,path in {'controller':'codec-v21-controller.sh.in','boot':'codec-v21-boot.sh.in','entry':'codec-v21-usb.sh.in','service':'codec-v21.service.in'}.items():sources[key]=h((r/'recovery'/path).read_bytes())
 m={'format':'moi3-codec-test-v21','unit':'3G5035842B','software':'0841','purpose':'parked-synthetic-codec-comparison','factoryExecutableChanged':False,'startupRestoresBeforeMonitor':True,'newRawCapture':False,'maximumPlaybackSeconds':32,'monitorRuntimeMaxSeconds':180,'launcher':'volatile-service-file','factoryHashes':v,'sourceHashes':sources,'files':{name:h(b) for name,b in files.items()},'hardwareValidated':False,'knownSpeechResets':True,'completeVehicleRollback':False}
 files['build-manifest.json']=json.dumps(m,indent=2).encode()
 with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as target:
  for name in z.namelist():
   if name.startswith(('Data/','Meta/')):target.writestr(name,z.read(name))
  for name,b in files.items():target.writestr(name,b)
print(json.dumps({'archive':str(out),'sha256':h(out.read_bytes()),'bytes':out.stat().st_size,'payloadFiles':len(m['files']),'unchangedPlayerAndClips':True},indent=2))
