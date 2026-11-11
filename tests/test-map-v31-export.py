from pathlib import Path
import os,sys,subprocess,tempfile,json,base64,zlib,hashlib
assert os.geteuid()==0
text=Path(sys.argv[1]).read_text();body=text.split("<<'PYV31EXPORT'\n",1)[1].split('\nPYV31EXPORT',1)[0]
with tempfile.TemporaryDirectory() as tmp:
 r=Path(tmp);usb=r/'usb';usb.mkdir();source=r/'data/lge/smartlink/carplay/carplay-handler-v31.log';source.parent.mkdir(parents=True);source.write_bytes(b'private fixture report')
 script=r/'export.py';script.write_text(body)
 for value in (b'private fixture report',b'new\0binary report'):
  source.write_bytes(value);subprocess.run([sys.executable,script,r,usb],check=True,timeout=5)
  bundle=json.loads((usb/'map-v31-reports.json').read_text());rec=next(x for x in bundle['records'] if x['state']=='read')
  data=zlib.decompress(base64.b64decode(rec['zlibBase64']));assert data==value and hashlib.sha256(data).hexdigest()==rec['sha256'];assert source.read_bytes()==value
 out=usb/'map-v31-reports.json';out.write_text('unknown report');result=subprocess.run([sys.executable,script,r,usb],capture_output=True,timeout=5);assert result.returncode!=0 and out.read_text()=='unknown report'
print('PASS: exact embedded checked export, latest snapshot replacement, source preservation and unknown-output refusal')
