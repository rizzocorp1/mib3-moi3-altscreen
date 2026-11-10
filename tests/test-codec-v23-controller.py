"""Controller ordering and recovery with real exclusive UDP writer, fake native service."""
from pathlib import Path
import os,sys,subprocess,tempfile,time,signal,hashlib,shutil
template=Path(sys.argv[1]).read_text()
def wait(check,seconds=5):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if check():return
  time.sleep(.02)
 raise AssertionError('fixture timeout')
with tempfile.TemporaryDirectory() as tmp:
 for scenario in ('complete','no-trigger','bad-clip','stop-failure','still-active','player-failure','signal','unknown-owned','port-busy','observer-failure','no-factory-timing'):
  r=Path(tmp)/scenario;r.mkdir();base=r/'lge/app_rw/smartlink/carplay';s=base/'.moi3-codec-test-v23';s.mkdir(parents=True);(r/'run').mkdir();tools=r/'tools';tools.mkdir()
  usb=r/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);marker=usb/'CODEC_TEST_V23.txt';marker.write_text('fixture')
  (base/'carplay').write_bytes(b'native executable');(base/'carplay.sh').write_bytes(b'native startup');(r/'native-active').write_text('yes');(s/'boot-status.txt').write_text('restored before monitor')
  player=s/'synthetic-player';player.write_text('''#!/usr/bin/env python3
import socket,sys,os,time
from pathlib import Path
r=Path(os.environ['TEST_ROOT']);scenario=os.environ['SCENARIO']
if sys.argv[1]=='--validate':sys.exit(0)
sock=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
try:sock.bind(('::',42989))
except OSError:sys.exit(2)
if sys.argv[1]=='--port-free':sys.exit(0)
(r/'playing').write_text('yes')
if scenario=='player-failure':sys.exit(2)
time.sleep(30 if scenario=='signal' else .15)
''');player.chmod(0o755)
  ctl=tools/'systemctl';ctl.write_text('''#!/usr/bin/env python3
import os,sys,socket
from pathlib import Path
r=Path(os.environ['TEST_ROOT']);scenario=os.environ['SCENARIO'];a=sys.argv[1]
with (r/'events').open('a') as f:f.write(a+'\\n')
if a=='is-active':sys.exit(0 if (r/'native-active').exists() else 3)
if a=='stop':
 if scenario=='stop-failure':sys.exit(1)
 if scenario!='still-active':(r/'native-active').unlink(missing_ok=True)
if a=='start':
 sock=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
 try:sock.bind(('::',42989))
 except OSError:(r/'unsafe-start').write_text('busy');sys.exit(2)
 (r/'native-active').write_text('yes')
''');ctl.chmod(0o755)
  sleep=tools/'sleep';sleep.write_text('#!/bin/sh\n/bin/sleep .03\n');sleep.chmod(0o755)
  audit=s/'packet-timing';audit.write_text("#!/usr/bin/python3\nimport sys,time,signal,os\nfrom pathlib import Path\nscenario=os.environ.get('SCENARIO','')\nif scenario=='observer-failure':sys.exit(2)\nPath(sys.argv[2]).write_text('fixture observer\\n'+('' if scenario=='no-factory-timing' else 'FRAME_TIMING flow=0 direction=outgoing sourcePort=42989 encoderGrant=0 pts=100\\n'))\nPath(sys.argv[3]).write_text('READY')\nsignal.signal(signal.SIGTERM,lambda *_:sys.exit(0))\ntime.sleep(120)\n");audit.chmod(0o755)
  values={'AUDIT_SHA':hashlib.sha256(audit.read_bytes()).hexdigest(),'PLAYER_SHA':hashlib.sha256(player.read_bytes()).hexdigest(),'CARPLAY_SHA':hashlib.sha256((base/'carplay').read_bytes()).hexdigest(),'STARTUP_SHA':hashlib.sha256((base/'carplay.sh').read_bytes()).hexdigest()}
  for i,name in enumerate(('baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic'),1):
   (s/name).write_bytes(b'synthetic fixture');values['CLIP'+str(i)+'_SHA']=hashlib.sha256((s/name).read_bytes()).hexdigest()
  text=template.replace('[ "$i" -lt 120 ]','[ "$i" -lt 12 ]')
  for k,v in values.items():text=text.replace('@'+k+'@',v)
  controller=s/'controller.sh';controller.write_text(text)
  if scenario=='bad-clip':(s/'high-ref1-p.synthetic').write_bytes(b'changed')
  if scenario=='unknown-owned':(s/'OWNED').write_text('unknown state')
  busy=None
  if scenario=='port-busy':
   import socket
   busy=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);busy.bind(('::',42989))
  env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'],TEST_ROOT=str(r),SCENARIO=scenario)
  p=subprocess.Popen(['sh',controller,s,'--monitor',r],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
  if scenario not in ('bad-clip','unknown-owned'):
   wait(lambda:(s/'codec-player.log').exists() and 'READY:' in (s/'codec-player.log').read_text())
   if scenario!='no-trigger':marker.unlink();time.sleep(.13);marker.write_text('fixture')
   if scenario=='signal':wait(lambda:(r/'playing').exists());p.terminate()
  stdout,stderr=p.communicate(timeout=8)
  if busy:busy.close()
  log=(s/'codec-player.log').read_text();events=(r/'events').read_text() if (r/'events').exists() else ''
  assert not (r/'unsafe-start').exists(),(scenario,log)
  assert (base/'carplay').read_bytes()==b'native executable' and (base/'carplay.sh').read_bytes()==b'native startup'
  if scenario in ('complete','player-failure','signal'):assert (r/'native-active').exists() and not (s/'OWNED').exists() and 'NATIVE RESTORED' in log,(scenario,log,stderr)
  if scenario in ('bad-clip','unknown-owned','no-trigger','observer-failure','no-factory-timing'):assert 'stop' not in events and not (r/'playing').exists(),(scenario,events)
  if scenario in ('stop-failure','still-active','port-busy','observer-failure','no-factory-timing'):assert not (r/'playing').exists()
  assert 'stop' not in events or scenario not in ('bad-clip','unknown-owned','no-trigger','observer-failure','no-factory-timing')
 print('PASS: USB edge trigger, preflight refusal, confirmed stop before output, completion/failure/signal restoration and occupied-port refusal')
