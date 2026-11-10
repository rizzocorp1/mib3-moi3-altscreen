"""Reproduce missing crash cleanup using the unchanged V27 collector and real systemd."""
from pathlib import Path
import os,sys,socket,subprocess,tempfile,time,uuid
assert os.geteuid()==0 and Path('/proc/1/comm').read_text().strip()=='systemd'
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
 for name,source in (('file-guard',guard),('packet-audit',audit)):shutil.copyfile(source,stage/name);(stage/name).chmod(0o755)
 token=b'AUTHENTICATED_H264_V27\n';request=data/'altscreen-v27-frame-ready';grant=data/'altscreen-v27-encoder-ready';expired=data/'altscreen-v27-expired';request.write_bytes(token)
 (stage/'boot-status.txt').write_text('fixture\n');(r/'native-active').write_text('yes');tools=r/'tools';tools.mkdir()
 script=tools/'systemctl';script.write_text('#!/usr/bin/python3\nimport sys\nfrom pathlib import Path\nr=Path('+repr(str(r))+')\na=sys.argv[1]\nif a=="stop":(r/"native-active").unlink(missing_ok=True)\nelif a=="start":(r/"native-active").write_text("yes")\nelif a=="is-active":sys.exit(0 if (r/"native-active").exists() else 3)\n');script.chmod(0o755)
 unit='moi3-map-death-audit-'+uuid.uuid4().hex;writer=None
 try:
  # Same launcher semantics, shorter runner-only timeout, no post-stop recovery.
  call('systemd-run','--unit='+unit,'--property=RuntimeMaxSec=30','--property=ExecStopPost=/bin/sh '+str(collector)+' '+str(stage)+' '+str(r/'ready')+' 300 '+str(r)+' --recover','--setenv=PATH='+str(tools)+':/usr/bin:/bin','/bin/sh',str(collector),str(stage),str(r/'ready'),'300',str(r))
  wait(lambda:grant.exists() and 'HANDOFF ACTIVE' in (stage/'boot-status.txt').read_text());assert grant.read_bytes()==token and not (r/'native-active').exists()
  writer=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);writer.bind(('::',42989))
  call('systemctl','kill','--kill-whom=main','--signal=SIGKILL',unit)
  wait(lambda:expired.exists());assert not (r/'native-active').exists()
  assert subprocess.run([audit,'--port-free'],capture_output=True).returncode!=0
  writer.close();writer=None;wait(lambda:(r/'native-active').exists());assert expired.read_bytes()==token and not grant.exists()
  print('PASS A4: real monitor SIGKILL independently revokes grant, waits for external writer port release, then restores native sender')
 finally:
  if writer:writer.close()
  subprocess.run(['systemctl','stop',unit],capture_output=True);subprocess.run(['systemctl','reset-failed',unit],capture_output=True)
