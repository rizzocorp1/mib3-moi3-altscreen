"""Reproduce missing crash cleanup using the unchanged V28 collector and real systemd."""
from pathlib import Path
import os,sys,socket,subprocess,tempfile,time,uuid
assert os.geteuid()==0 and Path('/proc/1/comm').read_text().strip()=='systemd'
boot=Path(sys.argv[4]).read_text()
mode=sys.argv[5] if len(sys.argv)>5 else 'kill'
collector=Path(sys.argv[1]).resolve();guard=Path(sys.argv[2]).resolve();audit=Path(sys.argv[3]).resolve()
def call(*a):return subprocess.run(a,check=True,capture_output=True,text=True)
def wait(f):
 end=time.monotonic()+10
 while time.monotonic()<end:
  if f():return
  time.sleep(.05)
 raise AssertionError('fixture deadline')
with tempfile.TemporaryDirectory() as tmp:
 r=Path(tmp);stage=r/'stage';stage.mkdir();data=r/'data/lge/smartlink/carplay';data.mkdir(parents=True);usb=r/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);(usb/'ALTSCREEN_TEST_ONCE.txt').write_text('fixture')
 import shutil
 shutil.copyfile(collector,stage/"managed-log-collector.sh")
 for name,source in (('file-guard',guard),('packet-audit',audit)):shutil.copyfile(source,stage/name);(stage/name).chmod(0o755)
 token=b'AUTHENTICATED_H264_V28\n';request=data/'altscreen-v28-frame-ready';grant=data/'altscreen-v28-encoder-ready';expired=data/'altscreen-v28-expired';request.write_bytes(token)
 (stage/'boot-status.txt').write_text('fixture\n');(r/'native-active').write_text('yes');tools=r/'tools';tools.mkdir()
 script=tools/'systemctl';script.write_text('#!/usr/bin/python3\nimport sys\nfrom pathlib import Path\nr=Path('+repr(str(r))+')\na=sys.argv[1]\nif a=="stop":(r/"native-active").unlink(missing_ok=True)\nelif a=="start":(r/"native-active").write_text("yes")\nelif a=="is-active":sys.exit(0 if (r/"native-active").exists() else 3)\n');script.chmod(0o755)
 unit='moi3-map-death-audit-'+uuid.uuid4().hex;writer=None
 try:
  # Same launcher semantics, shorter runner-only timeout, no post-stop recovery.
  unitfile=Path('/run/systemd/system')/(unit+'.service')
  text=boot.split('<<EOF_MAP_UNIT\n',1)[1].split('\nEOF_MAP_UNIT',1)[0]
  for key,value in (('$STAGE',str(stage)),('$RAM',str(r)),('$R',str(r)),('@LOG_ITERATIONS@','300')):text=text.replace(key,value)
  text=text.replace('RuntimeMaxSec=660','RuntimeMaxSec='+('3' if mode=='timeout' else '30')).replace('TimeoutStopSec=20','TimeoutStopSec=1')
  text+='\nEnvironment=PATH='+str(tools)+':/usr/bin:/bin\n'
  assert not unitfile.exists();unitfile.write_text(text)
  call('systemctl','daemon-reload');call('systemctl','start','--no-block',unit)
  wait(lambda:grant.exists() and 'HANDOFF ACTIVE' in (stage/'boot-status.txt').read_text());assert grant.read_bytes()==token and not (r/'native-active').exists()
  writer=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);writer.bind(('::',42989))
  call('systemctl','kill','--kill-whom=main','--signal='+('SIGSTOP' if mode=='timeout' else 'SIGKILL'),unit)
  wait(lambda:expired.exists());assert not (r/'native-active').exists()
  assert subprocess.run([audit,'--port-free'],capture_output=True).returncode!=0
  writer.close();writer=None;wait(lambda:(r/'native-active').exists());assert expired.read_bytes()==token and not grant.exists()
  print('PASS A4 '+mode+': real monitor death independently revokes grant, waits for external writer port release, then restores native sender')
 finally:
  print((stage/"boot-status.txt").read_text())
  print(subprocess.run(["journalctl","-u",unit,"--no-pager","-n","30"],capture_output=True,text=True).stdout)
  if writer:writer.close()
  subprocess.run(['systemctl','stop',unit],capture_output=True);subprocess.run(['systemctl','reset-failed',unit],capture_output=True)
  unitfile.unlink(missing_ok=True);subprocess.run(['systemctl','daemon-reload'],capture_output=True)
