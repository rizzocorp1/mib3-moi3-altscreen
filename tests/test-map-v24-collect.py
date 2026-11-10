"""Recover exact retained bytes without source/service mutation; adversarial files."""
from pathlib import Path
import sys,os,tempfile,subprocess,json,hashlib,base64,zlib
assert os.geteuid()==0
template=Path(sys.argv[1]).read_text()
for mode in ('normal','zeros','missing','symlink','oversize','existing-output','mixed'):
 with tempfile.TemporaryDirectory() as tmp:
  root=Path(tmp);usb=root/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);(usb/'COLLECT_MAP_V24.txt').write_text('collect')
  stage=root/'lge/app_rw/smartlink/carplay/.moi3-altscreen-map-v24';stage.mkdir(parents=True)
  source=root/'data/lge/smartlink/carplay/carplay-handler-v24.log';source.parent.mkdir(parents=True)
  expected=b'log with binary'+bytes([0,1,255])+b' exact retained data'
  source.write_bytes(b'\0'*4000 if mode=='zeros' else expected);(stage/'packet-audit.txt').write_text('retained packets')
  if mode=='missing':source.unlink()
  victim=root/'victim';victim.write_bytes(b'preserve')
  if mode=='symlink':source.unlink();source.symlink_to(victim)
  if mode=='oversize':source.write_bytes(b'x'*2097153)
  out=usb/'v24-recovered-reports.json'
  if mode=='existing-output':out.symlink_to(victim)
  if mode=='mixed':(usb/'ALTSCREEN_TEST_ONCE.txt').write_text('active')
  before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for d in ('lge','data') for p in (root/d).rglob('*') if p.is_file()}
  script=root/'collector.sh';script.write_text(template.replace('@ROOT@',str(root)))
  subprocess.run(['sh',script],check=True,timeout=10)
  after={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for d in ('lge','data') for p in (root/d).rglob('*') if p.is_file()}
  assert before==after and victim.read_bytes()==b'preserve'
  if mode in ('existing-output','mixed'):
   if mode=='existing-output':assert out.is_symlink()
   else:assert not out.exists()
   continue
  bundle=json.loads(out.read_text());assert bundle['unitSideMutations'] is False
  record=next(x for x in bundle['records'] if x['name'].endswith('/data/lge/smartlink/carplay/carplay-handler-v24.log'))
  if mode in ('missing','symlink','oversize'):assert record['state']!='read'
  else:
   data=zlib.decompress(base64.b64decode(record['zlibBase64']));assert hashlib.sha256(data).hexdigest()==record['sha256']
   assert data==(b'\0'*4000 if mode=='zeros' else expected)
   assert record['zeroBytes']==data.count(b'\0')
  assert 'COLLECTION COMPLETE' in (usb/'v24-recovery-intake.txt').read_text()
print('PASS: exact binary and zero-filled source classification, compressed ASCII export, readback, source preservation, unsafe/oversize/mixed/output refusal')
