"""Reuse verified original player/clips; render the corrected runtime-unit launcher."""
from pathlib import Path
import zipfile,hashlib,json
r=Path(__file__).resolve().parents[1];h=lambda b:hashlib.sha256(b).hexdigest()
base=r/'firmware-audit/moi3-codec-comparison-0841-v22.zip'
assert h(base.read_bytes())=='d326c6527713d90a01fda9521c7d8484854a937f993336dd4def066fabf65613'
out=r/'firmware-audit/moi3-codec-comparison-0841-v23.zip';assert not out.exists()
with zipfile.ZipFile(base) as z:
 old=json.loads(z.read('build-manifest.json'));files={}
 for name in ('synthetic-player','file-guard','baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic'):
  p='payload/'+name;b=z.read(p);assert h(b)==old['files'][p];files[p]=b
 v={k:str(value) for k,value in old['factoryHashes'].items() if k not in ('CONTROLLER_SHA','BOOT_SHA')}
 files['payload/packet-timing']=(r/'firmware-audit/codec-v23-build/packet-timing').read_bytes()
 v['AUDIT_SHA']=h(files['payload/packet-timing'])
 v['PLAYER_SHA']=h(files['payload/synthetic-player'])
 v.update(ROOT='',STAGE='/lge/app_rw/smartlink/carplay/.moi3-codec-test-v23')
 def render(name):
  text=(r/'recovery'/name).read_text()
  for key,value in v.items():text=text.replace('@'+key+'@',value)
  import re
  assert not re.search('@[A-Z0-9_]+@',text),name
  return text.encode()
 files['payload/codec-test.service']=render('codec-v23.service.in');v['UNIT_SHA']=h(files['payload/codec-test.service'])
 files['payload/controller.sh']=render('codec-v23-controller.sh.in');v['CONTROLLER_SHA']=h(files['payload/controller.sh'])
 files['payload/oneshot-boot.sh']=render('codec-v23-boot.sh.in');v['BOOT_SHA']=h(files['payload/oneshot-boot.sh'])
 files['fix_hw_partnumber.sh']=render('codec-v23-usb.sh.in')
 files['CODEC_TEST_V23.txt']=b'Parked-only factory20seconds then Baseline32seconds timing comparison.\n'
 files['READ-ME-FIRST.txt']=(r/'firmware-audit/CODEC-V23-TEST-INSTRUCTIONS.md').read_bytes()
 sources={}
 for key,path in {'player':'implementation/synthetic-player-v22.c','transport':'implementation/synthetic-transport-v20.h','guard':'recovery/file-guard.c','resource':'recovery/resource-audit-v17.h','export':'recovery/video-export-v16.h'}.items():
  sources[key]=h((r/path).read_bytes());assert key=='player' or sources[key]==old['sourceHashes'][key]
 sources['audit']=h((r/'recovery/packet-timing-v23.c').read_bytes())
 for key,path in {'controller':'codec-v23-controller.sh.in','boot':'codec-v23-boot.sh.in','entry':'codec-v23-usb.sh.in','service':'codec-v23.service.in'}.items():sources[key]=h((r/'recovery'/path).read_bytes())
 m={'format':'moi3-codec-test-v23','unit':'3G5035842B','software':'0841','purpose':'parked-factory-custom-timing','factoryExecutableChanged':False,'startupRestoresBeforeMonitor':True,'newRawCapture':False,'maximumPlaybackSeconds':32,'monitorRuntimeMaxSeconds':240,'factoryReferenceSeconds':20,'observerMaximumSeconds':70,'launcher':'volatile-service-file','factoryHashes':v,'sourceHashes':sources,'files':{name:h(b) for name,b in files.items()},'hardwareValidated':False,'knownSpeechResets':True,'completeVehicleRollback':False}
 files['build-manifest.json']=json.dumps(m,indent=2).encode()
 with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as target:
  for name in z.namelist():
   if name.startswith(('Data/','Meta/')):target.writestr(name,z.read(name))
  for name,b in files.items():target.writestr(name,b)
print(json.dumps({'archive':str(out),'sha256':h(out.read_bytes()),'bytes':out.stat().st_size,'payloadFiles':len(m['files']),'unchangedClips':True},indent=2))
