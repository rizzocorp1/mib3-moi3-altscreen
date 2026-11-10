from pathlib import Path
import zipfile,hashlib,json,sys
r=Path(__file__).resolve().parents[1]
h=lambda b:hashlib.sha256(b).hexdigest()
carrier=r/'firmware-audit/LG-wipe-FAZIT.zip'
assert h(carrier.read_bytes())=='763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d'
source=r/'recovery/codec-v20-collect.sh.in'
files={'fix_hw_partnumber.sh':source.read_text().replace('@ROOT@','').encode(),
 'COLLECT_CODEC_V20.txt':b'Collect retained V20 reports only. Do not launch playback.\n',
 'READ-ME-FIRST.txt':(r/'firmware-audit/CODEC-V20-COLLECT-INSTRUCTIONS.md').read_bytes()}
manifest={'format':'moi3-codec-collect-v20','software':'0841','purpose':'collect-retained-v20-state-only','activation':False,
 'sourceSha256':h(source.read_bytes()),'files':{name:h(b) for name,b in files.items()},'knownSpeechResets':True}
files['build-manifest.json']=json.dumps(manifest,indent=2).encode()
target=r/'firmware-audit/moi3-codec-collect-0841-v20.zip';assert not target.exists()
with zipfile.ZipFile(carrier) as original,zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as out:
 for e in original.infolist():
  if e.filename=='fix_hw_partnumber.sh':continue
  assert e.filename.startswith(('Data/','Meta/')) and '..' not in e.filename.split('/')
  out.writestr(e.filename,original.read(e.filename))
 for name,b in files.items():out.writestr(name,b)
with zipfile.ZipFile(carrier) as original,zipfile.ZipFile(target) as out:
 assert len(out.namelist())==len(set(out.namelist()))
 for name in original.namelist():
  if name!='fix_hw_partnumber.sh':assert original.read(name)==out.read(name)
 for name,b in files.items():assert out.read(name)==b
 assert 'CODEC_TEST_V20.txt' not in out.namelist()
print(json.dumps({'archive':str(target),'sha256':h(target.read_bytes()),'bytes':target.stat().st_size,'vendorUnchanged':True,'activation':False},indent=2))
