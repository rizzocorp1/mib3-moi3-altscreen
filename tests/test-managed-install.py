from pathlib import Path
import os,sys,subprocess,tempfile,shutil,socket
from managed_fixture import digest,render
assert os.geteuid()==0 and os.stat('/proc/self/ns/mnt').st_ino!=os.stat('/proc/1/ns/mnt').st_ino
assert os.stat('/proc/self/ns/net').st_ino!=os.stat('/proc/1/ns/net').st_ino
subprocess.run(['ip','link','set','lo','up'],check=True)
guard,templates=map(lambda x:Path(x).resolve(),sys.argv[1:3])
def call(*a,**k):return subprocess.run(a,check=True,capture_output=True,text=True,**k)
def metadata(p):return (p.stat().st_uid,p.stat().st_gid,p.stat().st_mode&0o7777,{n:os.getxattr(p,n) for n in os.listxattr(p)})
scenarios=['roundtrip','unarmed','corrupt-handler','corrupt-helper','corrupt-backup','corrupt-backup-meta','mount-failure','controller-failure','controller-timeout','reload-failure','unknown-runtime-marker','bad-usb-helper','backup-conflict','backup-copy-failure','unknown-startup','unknown-executable','stop-failure','port-busy','modified-installed-asset']
for scenario in scenarios:
 with tempfile.TemporaryDirectory() as tmp:
  r=Path(tmp);base=r/'lge/app_rw/smartlink/carplay';base.mkdir(parents=True);data=r/'data/lge/smartlink/carplay';data.mkdir(parents=True);(r/'run').mkdir();(r/'etc').mkdir();(r/'etc/os-release').write_text('fixture os')
  (base/'carplay.sh').write_text('#!/bin/sh\nexit 0\n');(base/'carplay').write_text('factory executable');(base/'carplay.sh').chmod(0o755);(base/'carplay').chmod(0o755)
  os.setxattr(base/'carplay.sh','user.factory',b'startup');os.setxattr(base/'carplay','user.factory',b'executable');stock_script=(base/'carplay.sh').read_bytes();stock_binary=(base/'carplay').read_bytes();stock_meta=metadata(base/'carplay.sh');binary_meta=metadata(base/'carplay')
  lib=r/'lge/app_rw/lib64';lib.mkdir(parents=True);(lib/'libAirPlay.so').write_text('factory library reference')
  usb=r/'tmp/USB/PORT1/PART1';payload=usb/'payload';payload.mkdir(parents=True);(usb/'ALTSCREEN_MANAGED_INSTALL.txt').write_text('INSTALL');shutil.copyfile(guard,payload/'file-guard');(payload/'file-guard').chmod(0o755)
  (payload/'carplay-managed').write_text('managed copy');(payload/'altscreen_handler.so').write_text('handler');shutil.copyfile(base/'carplay.sh',payload/'factory-carplay.sh');shutil.copyfile(base/'carplay',payload/'factory-carplay')
  values={'ROOT':str(r),'MODE':'install','OS_SHA':digest(r/'etc/os-release'),'STARTUP_SHA':digest(base/'carplay.sh'),'CARPLAY_SHA':digest(base/'carplay'),'GUARD_SHA':digest(guard),'PATCH_SHA':digest(payload/'carplay-managed'),'HANDLER_SHA':digest(payload/'altscreen_handler.so')}
  (payload/'managed-controller.sh').write_text(render((templates/'managed-v31-controller.sh.in').read_text(),values));values['CONTROLLER_SHA']=digest(payload/'managed-controller.sh')
  (payload/'managed-boot.sh').write_text(render((templates/'managed-v31-boot.sh.in').read_text(),values));values['BOOT_SHA']=digest(payload/'managed-boot.sh')
  stage=base/'.moi3-altscreen-managed-v31';ram=r/'run/moi3-altscreen-map-v31'
  (r/'native-active').write_text('yes');tools=r/'tools';tools.mkdir()
  script='''#!/usr/bin/python3
from pathlib import Path
import os,sys,socket
r=Path(ROOT);a=sys.argv[1:];scenario=os.environ.get('SCENARIO','roundtrip');unit='moi3-altscreen-managed-v31.service'
if a[0]=='daemon-reload' and scenario=='reload-failure':sys.exit(1)
if a[0]=='is-active':sys.exit(0 if (r/('controller-active' if a[-1]==unit else 'native-active')).exists() else 3)
if a[0]=='start' and a[-1]==unit:
 if scenario=='controller-failure':sys.exit(1)
 text=(r/'run/systemd/system'/unit).read_text();assert 'WatchdogSec=20' in text and 'NotifyAccess=all' in text and 'After=carplay.service' not in text
 (r/'controller-active').write_text('yes')
 if scenario!='controller-timeout':(r/'run/moi3-altscreen-map-v31/ready').write_text('READY')
if a[0]=='stop' and a[-1]==unit:
 if scenario=='stop-failure':sys.exit(1)
 (r/'controller-active').unlink(missing_ok=True)
 for name in ('altscreen-v31-encoder-ready',):(r/'data/lge/smartlink/carplay'/name).unlink(missing_ok=True)
 (r/'run/moi3-altscreen-map-v31/lease').unlink(missing_ok=True)
if a[0]=='start' and a[-1]=='multi-display.service':
 s=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
 try:s.bind(('::',42989))
 except OSError:(r/'unsafe-start').write_text('busy');sys.exit(1)
 s.close();(r/'native-active').write_text('yes')
'''.replace('ROOT',repr(str(r)))
  (tools/'systemctl').write_text(script);(tools/'systemctl').chmod(0o755)
  (tools/'mount').write_text('#!/bin/sh\n[ "$SCENARIO" != mount-failure ] || exit 1\nexec /usr/bin/mount "$@"\n');(tools/'mount').chmod(0o755)
  (tools/'cp').write_text('#!/usr/bin/python3\nimport os,sys,shutil\nfrom pathlib import Path\nif os.environ.get("SCENARIO")=="backup-copy-failure":Path(sys.argv[2]).write_bytes(b"\\0"*16)\nelse:shutil.copyfile(sys.argv[1],sys.argv[2])\n');(tools/'cp').chmod(0o755)
  env=dict(os.environ,SCENARIO=scenario,PATH=str(tools)+':'+os.environ['PATH'])
  entry=r/'entry.sh';entry.write_text(render((templates/'managed-v31-entry.sh.in').read_text(),values));call('sh','-n',entry)
  sock=None
  try:
   if scenario=='bad-usb-helper':(payload/'file-guard').write_text('bad')
   if scenario=='backup-conflict':b=usb/'managed-v31-stock-backup';b.mkdir();(b/'SCOPE.txt').write_text('unknown')
   call('sh',entry,env=env)
   report=(usb/'managed-v31-setup.txt').read_text()
   if scenario in ('bad-usb-helper','backup-conflict','backup-copy-failure'):
    assert 'FAILED' in report and (base/'carplay.sh').read_bytes()==stock_script and (base/'carplay').read_bytes()==stock_binary
    print('PASS: '+scenario+' refuses activation');continue
   assert 'INSTALLED:' in report and (stage/'ENABLED').exists();assert digest(base/'carplay.sh')==values['BOOT_SHA'] and metadata(base/'carplay.sh')==stock_meta
   if scenario=='unarmed':(stage/'ENABLED').unlink()
   if scenario=='corrupt-handler':(stage/'altscreen_handler.so').write_text('bad')
   if scenario=='corrupt-helper':(stage/'file-guard').write_text('bad')
   if scenario=='corrupt-backup':(stage/'factory-carplay.sh').write_text('bad')
   if scenario=='corrupt-backup-meta':os.setxattr(stage/'factory-carplay.sh','user.factory',b'bad')
   if scenario=='unknown-runtime-marker':(data/'altscreen-v31-expired').write_text('unknown')
   call('sh',payload/'managed-boot.sh',env=env)
   aborts=('unarmed','corrupt-handler','corrupt-helper','corrupt-backup','corrupt-backup-meta','mount-failure','controller-failure','controller-timeout','reload-failure','unknown-runtime-marker')
   if scenario in aborts:
    assert (base/'carplay').read_bytes()==stock_binary
    if scenario not in ('corrupt-helper','corrupt-backup','corrupt-backup-meta'):assert (base/'carplay.sh').read_bytes()==stock_script
    print('PASS: '+scenario+' leaves native executable and avoids activation');continue
   assert digest(base/'carplay')==values['PATCH_SHA'] and digest(base/'carplay.sh')==values['BOOT_SHA'] and (stage/'ENABLED').exists()
   # Warm service restart and two complete reboot cycles, using real private mounts.
   call('sh',payload/'managed-boot.sh',env=env);assert digest(base/'carplay')==values['PATCH_SHA']
   if scenario=='roundtrip':
    for cycle in range(2):
     call('/usr/bin/umount',base/'carplay');shutil.rmtree(r/'run');(r/'run').mkdir();(r/'controller-active').unlink();call('sh',payload/'managed-boot.sh',env=env);assert digest(base/'carplay')==values['PATCH_SHA'] and (stage/'ENABLED').exists()
   (usb/'ALTSCREEN_MANAGED_INSTALL.txt').unlink();(usb/'ALTSCREEN_MANAGED_RESTORE.txt').write_text('RESTORE');restore=r/'restore.sh';restore.write_text(render((templates/'managed-v31-entry.sh.in').read_text(),dict(values,MODE='restore')))
   if scenario=='unknown-startup':(base/'carplay.sh').write_text('unknown startup')
   if scenario=='unknown-executable':(base/'carplay').write_text('unknown executable')
   if scenario=='modified-installed-asset':(stage/'altscreen_handler.so').write_text('unrecognized modified library')
   if scenario=='port-busy':
    (r/'native-active').unlink();sock=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);sock.bind(('::',42989))
   call('sh',restore,env=env);report=(usb/'managed-v31-setup.txt').read_text()
   if scenario in ('unknown-startup','unknown-executable','stop-failure','port-busy'):
    assert 'FAILED/PENDING' in report
    if scenario=='unknown-startup':assert (base/'carplay.sh').read_text()=='unknown startup'
    if scenario=='unknown-executable':assert (base/'carplay').read_text()=='unknown executable'
    assert not (r/'unsafe-start').exists();print('PASS: '+scenario+' preserved/refused safely');continue
   assert 'STOCK RESTORED:' in report and (base/'carplay.sh').read_bytes()==stock_script and (base/'carplay').read_bytes()==stock_binary
   assert metadata(base/'carplay.sh')==stock_meta and metadata(base/'carplay')==binary_meta
   assert not stage.exists() and list(base.glob('.moi3-altscreen-managed-v31-backup-*'))
   if scenario=='modified-installed-asset':assert any(x.read_text()=='unrecognized modified library' for x in base.glob('.moi3-altscreen-managed-v31-backup-*/altscreen_handler.so'))
   call('sh',restore,env=env);assert 'ALREADY STOCK:' in (usb/'managed-v31-setup.txt').read_text()
   assert not (r/'unsafe-start').exists();print('PASS: '+scenario+' verified persistent boots and exact startup/executable/metadata stock return')
  finally:
   if sock:sock.close()
   subprocess.run(['/usr/bin/umount',str(base/'carplay')],capture_output=True)
