"""Real file-guard and real templates; no vehicle inputs or mocked metadata."""
from pathlib import Path
import os,sys,hashlib,subprocess,tempfile,shutil
guard=Path(sys.argv[1]).resolve();templates=Path(sys.argv[2]).resolve()
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
for scenario in ('normal','wrong-baseline','bad-clip','low-space','copy-failure','disarmed','bad-controller','unit-conflict','reload-failure'):
 with tempfile.TemporaryDirectory() as tmp:
  r=Path(tmp);base=r/'lge/app_rw/smartlink/carplay';base.mkdir(parents=True);stage=base/'.moi3-codec-test-v21';(r/'run/systemd/system').mkdir(parents=True);(r/'lib/systemd/system').mkdir(parents=True);(r/'lib/systemd/system/carplay.service').write_text('[Service]\nExecStart=/native\n');(r/'data').mkdir();(r/'etc').mkdir();(r/'etc/os-release').write_bytes(b'fixture release')
  startup=base/'carplay.sh';startup.write_bytes(b'#!/bin/sh\nexit 0\n');startup.chmod(0o755);os.setxattr(startup,'user.baseline',b'keep')
  native=base/'carplay';native.write_bytes(b'original executable');native.chmod(0o755)
  usb=r/'tmp/USB/PORT1/PART1';payload=usb/'payload';payload.mkdir(parents=True);(usb/'CODEC_TEST_V21.txt').write_text('fixture')
  shutil.copyfile(guard,payload/'file-guard')
  if scenario=='low-space':(payload/'file-guard').write_text('#!/bin/sh\nif [ "$1" = available ];then echo 0;exit 0;fi\nexec '+str(guard)+' "$@"\n')
  player=payload/'synthetic-player';player.write_text('#!/bin/sh\n[ "$1" = --validate ]\n');player.chmod(0o755)
  controller=payload/'controller.sh';controller.write_text('#!/bin/sh\nexit 0\n')
  v={'ROOT':str(r),'OS_SHA':digest(r/'etc/os-release'),'CARPLAY_SHA':digest(native),'STARTUP_SHA':digest(startup),'GUARD_SHA':digest(payload/'file-guard'),'PLAYER_SHA':digest(player),'CONTROLLER_SHA':digest(controller),'STAGE':str(stage)}
  for i,name in enumerate(('baseline-ref1-p.synthetic','high-ref1-p.synthetic','high-ref4-p.synthetic','high-intra.synthetic'),1):(payload/name).write_bytes(b'clip fixture');v['CLIP'+str(i)+'_SHA']=digest(payload/name)
  def render(name):
   text=(templates/name).read_text()
   for k,value in v.items():text=text.replace('@'+k+'@',value)
   return text
  (payload/'codec-test.service').write_text(render('codec-v21.service.in'));v['UNIT_SHA']=digest(payload/'codec-test.service')
  (payload/'oneshot-boot.sh').write_text(render('codec-v21-boot.sh.in'));v['BOOT_SHA']=digest(payload/'oneshot-boot.sh')
  entry=usb/'fix_hw_partnumber.sh';entry.write_text(render('codec-v21-usb.sh.in'))
  tools=r/'tools';tools.mkdir();idtool=tools/'id';idtool.write_text('#!/bin/sh\necho 0\n');idtool.chmod(0o755)
  launch=tools/'systemctl';launch.write_text('#!/bin/sh\nif [ "$1" = daemon-reload ];then [ "$CODEC_TEST_SCENARIO" != reload-failure ];exit $?;fi\nprintf "%s\\n" "$@" > "'+str(r/'launched')+'"\n');launch.chmod(0o755)
  env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'],CODEC_TEST_SCENARIO=scenario)
  if scenario=='wrong-baseline':native.write_bytes(b'unknown')
  if scenario=='bad-clip':(payload/'high-ref1-p.synthetic').write_bytes(b'changed')
  if scenario=='copy-failure':env['FILE_GUARD_FAIL_BEFORE_COMMIT']='1'
  subprocess.run(['sh',entry],env=env,check=True,timeout=10);report=(usb/'altscreen-setup.txt').read_text()
  assert not list((r/'tmp').glob('moi3-codec-v21-preflight-*'))
  if scenario in ('wrong-baseline','bad-clip','low-space','copy-failure'):
   assert 'SETUP FAILED' in report and digest(startup)==v['STARTUP_SHA'] and not (stage/'ARMED').exists(),report
   continue
  assert 'SETUP COMPLETE' in report and digest(startup)==v['BOOT_SHA'] and (stage/'ARMED').exists(),report
  assert digest(native)==v['CARPLAY_SHA'] and os.getxattr(stage/'factory-carplay.sh','user.baseline')==b'keep'
  if scenario=='disarmed':(stage/'ARMED').unlink()
  if scenario=='bad-controller':(stage/'controller.sh').write_text('changed')
  if scenario=='unit-conflict':(r/'run/systemd/system/moi3-codec-v21.service').write_text('preserve unknown unit')
  subprocess.run(['sh',stage/'startup-new.sh'],env=env,check=True,timeout=10)
  assert digest(startup)==v['STARTUP_SHA'] and os.getxattr(startup,'user.baseline')==b'keep' and digest(native)==v['CARPLAY_SHA']
  assert (r/'launched').exists()==(scenario=='normal')
  assert 'RECOVERY: exact factory startup restored' in (usb/'boot-status.txt').read_text()
  if scenario=='unit-conflict':assert (r/'run/systemd/system/moi3-codec-v21.service').read_text()=='preserve unknown unit'
  assert 'systemd-run --' not in render('codec-v21-boot.sh.in')
  if scenario=='normal':
   args=(r/'launched').read_text();assert 'start' in args and '--no-block' in args and 'moi3-codec-v21.service' in args
   unit=(r/'run/systemd/system/moi3-codec-v21.service').read_text();assert 'RuntimeMaxSec=180' in unit and 'ExecStopPost=' in unit and 'TimeoutStopSec=10' in unit and 'KillMode=control-group' in unit
   (r/'launched').unlink();subprocess.run(['sh',stage/'startup-new.sh'],env=env,check=True,timeout=10);assert not (r/'launched').exists()
print('PASS: exact baseline and Linux metadata, refusal before arming, startup restored before monitor, one-shot consumption and no executable changes')
