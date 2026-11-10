"""Real systemd main-process death/expiry with an actual exclusive UDP writer."""
from pathlib import Path
import os,sys,tempfile,subprocess,time,hashlib,uuid
assert os.geteuid()==0 and Path('/proc/1/comm').read_text().strip()=='systemd'
template=Path(sys.argv[1]).read_text()
def cmd(*a):return subprocess.run(a,check=True,capture_output=True,text=True)
def wait(f,seconds=12):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if f():return
  time.sleep(.05)
 raise AssertionError('real service restoration timed out')
with tempfile.TemporaryDirectory() as tmp:
 for mode in ('kill-main','runtime-expiry'):
  r=Path(tmp)/mode;s=r/'lge/app_rw/smartlink/carplay/.moi3-codec-test-v21';s.mkdir(parents=True);base=s.parent;(r/'run').mkdir();tools=r/'tools';tools.mkdir();usb=r/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);marker=usb/'CODEC_TEST_V21.txt';marker.write_text('fixture')
  (base/'carplay').write_text('native');(base/'carplay.sh').write_text('native startup');(r/'native-active').write_text('yes');(s/'boot-status.txt').write_text('restored')
  player=s/'synthetic-player';player.write_text('''#!/usr/bin/python3
import sys,socket,time
from pathlib import Path
r=Path("ROOT")
if sys.argv[1]=='--validate':sys.exit(0)
s=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
try:s.bind(('::',42989))
except OSError:sys.exit(2)
if sys.argv[1]=='--port-free':sys.exit(0)
(r/'playing').write_text('yes');time.sleep(120)
'''.replace('ROOT',str(r)));player.chmod(0o755)
  ctl=tools/'systemctl';ctl.write_text('''#!/usr/bin/python3
import sys,socket
from pathlib import Path
r=Path("ROOT");a=sys.argv[1]
if a=='is-active':sys.exit(0 if (r/'native-active').exists() else 3)
if a=='stop':(r/'native-active').unlink(missing_ok=True)
if a=='start':
 s=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
 try:s.bind(('::',42989))
 except OSError:(r/'unsafe-start').write_text('port occupied');sys.exit(2)
 (r/'native-active').write_text('yes')
'''.replace('ROOT',str(r)));ctl.chmod(0o755)
  sleep=tools/'sleep';sleep.write_text('#!/bin/sh\n/bin/sleep .05\n');sleep.chmod(0o755)
  values={'PLAYER_SHA':hashlib.sha256(player.read_bytes()).hexdigest(),'CARPLAY_SHA':hashlib.sha256((base/'carplay').read_bytes()).hexdigest(),'STARTUP_SHA':hashlib.sha256((base/'carplay.sh').read_bytes()).hexdigest()}
  for i,n in enumerate(('baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic'),1):(s/n).write_text('clip');values['CLIP'+str(i)+'_SHA']=hashlib.sha256((s/n).read_bytes()).hexdigest()
  text=template
  for k,v in values.items():text=text.replace('@'+k+'@',v)
  controller=s/'controller.sh';controller.write_text(text)
  unit='moi3-codec-fixture-'+uuid.uuid4().hex
  try:
   unitfile=Path('/run/systemd/system')/(unit+'.service')
   unittext=Path(sys.argv[2]).read_text().replace('@STAGE@',str(s)).replace('@ROOT@',str(r))
   unittext=unittext.replace('RuntimeMaxSec=180','RuntimeMaxSec=4').replace('TimeoutStopSec=10','TimeoutStopSec=3')
   unittext=unittext.replace('After=carplay.service','After=network.target')
   unittext+='\nEnvironment=PATH='+str(tools)+':/usr/bin:/bin\n'
   assert not unitfile.exists();unitfile.write_text(unittext)
   cmd('systemctl','daemon-reload');cmd('systemctl','start','--no-block',unit)
   wait(lambda:(s/'codec-player.log').exists() and 'READY:' in (s/'codec-player.log').read_text())
   marker.unlink();time.sleep(.25);marker.write_text('fixture');wait(lambda:(r/'playing').exists())
   if mode=='kill-main':cmd('systemctl','kill','--kill-whom=main','--signal=SIGKILL',unit)
   wait(lambda:(r/'native-active').exists() and not (s/'OWNED').exists())
   assert not (r/'unsafe-start').exists();assert 'NATIVE RESTORED' in (s/'codec-player.log').read_text()
  finally:
   subprocess.run(['systemctl','stop',unit],capture_output=True);subprocess.run(['systemctl','reset-failed',unit],capture_output=True)
   unitfile.unlink(missing_ok=True);subprocess.run(['systemctl','daemon-reload'],capture_output=True)
print('PASS: real runtime service file (no transient property parser), KillMode, main SIGKILL and runtime expiry execute post-stop restore only after writer port release')
