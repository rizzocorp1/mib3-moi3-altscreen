from pathlib import Path
import zipfile,hashlib,json,sys
r=Path(__file__).resolve().parents[1]
h=lambda b:hashlib.sha256(b).hexdigest()
carrier=r/'firmware-audit/LG-wipe-FAZIT.zip'
assert h(carrier.read_bytes())=='763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d'
source=r/'recovery/map-v24-collect.sh.in'
files={'fix_hw_partnumber.sh':source.read_text().replace('@ROOT@','').encode(),
 'COLLECT_MAP_V24.txt':b'Recover retained V24 reports only. Do not launch a map test.\n',
 'READ-ME-FIRST.txt':(r/'firmware-audit/MAP-V24-COLLECT-INSTRUCTIONS.md').read_bytes()}
manifest={'format':'moi3-map-collect-v24','software':'0841','purpose':'recover-retained-v24-reports-only','activation':False,
 'sourceSha256':h(source.read_bytes()),'files':{name:h(b) for name,b in files.items()},'knownSpeechResets':True}
files['build-manifest.json']=json.dumps(manifest,indent=2).encode()
target=r/'firmware-audit/moi3-map-collect-0841-v24.zip';assert not target.exists()
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
 assert 'ALTSCREEN_TEST_ONCE.txt' not in out.namelist()
print(json.dumps({'archive':str(target),'sha256':h(target.read_bytes()),'bytes':target.stat().st_size,'vendorUnchanged':True,'activation':False},indent=2))
