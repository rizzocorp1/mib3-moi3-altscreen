from pathlib import Path
import sys,tempfile,subprocess,os,hashlib
template=Path(sys.argv[1]).read_text()
for mode in ('reports','no-stage','no-reports','mixed','unsafe-report'):
 with tempfile.TemporaryDirectory() as tmp:
  r=Path(tmp);usb=r/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);(usb/'COLLECT_CODEC_V20.txt').write_text('collect')
  stage=r/'lge/app_rw/smartlink/carplay/.moi3-codec-test-v20';stage.mkdir(parents=True)
  for name in ('ARMED','factory-carplay.sh','synthetic-player','controller.sh','file-guard','startup-new.sh'): (stage/name).write_bytes(b'preserve '+name.encode())
  for name in ('carplay','carplay.sh'):(stage.parent/name).write_bytes(b'original '+name.encode())
  if mode not in ('no-reports',):
   (stage/'boot-status.txt').write_text('early boot failed: fixture')
   (stage/'codec-player.log').write_text('fixture no trigger')
  if mode=='no-stage':stage.rename(stage.with_name('retained-stage'))
  if mode=='mixed':(usb/'CODEC_TEST_V20.txt').write_text('old activation')
  victim=r/'untouched';victim.write_text('keep')
  if mode=='unsafe-report':(usb/'boot-status-v20.txt').symlink_to(victim)
  tools=r/'tools';tools.mkdir()
  (tools/'id').write_text('#!/bin/sh\necho 0\n');(tools/'id').chmod(0o755)
  (tools/'systemctl').write_text('#!/bin/sh\n[ "$1" = show ] || { echo MUTATION > "'+str(r/'unsafe')+'";exit 1; }\necho ActiveState=inactive\n');(tools/'systemctl').chmod(0o755)
  (tools/'journalctl').write_text('#!/bin/sh\necho fixture-journal\n');(tools/'journalctl').chmod(0o755)
  before={str(p.relative_to(r)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (r/'lge').rglob('*') if p.is_file()}
  script=r/'collector.sh';script.write_text(template.replace('@ROOT@',str(r)))
  subprocess.run(['sh',script],env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH']),check=True,timeout=10)
  after={str(p.relative_to(r)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (r/'lge').rglob('*') if p.is_file()}
  assert before==after and not (r/'unsafe').exists() and victim.read_text()=='keep'
  if mode=='reports':assert (usb/'boot-status-v20.txt').read_text()=='early boot failed: fixture' and (usb/'codec-player-v20.log').read_text()=='fixture no trigger'
  if mode=='no-reports':assert 'BOOT REPORT ABSENT' in (usb/'codec-v20-intake.txt').read_text()
  if mode=='mixed':assert 'ABORT: mixed' in (usb/'codec-v20-intake.txt').read_text() and not (usb/'boot-status-v20.txt').exists()
print('PASS: retained reports/state exported; absent/unsafe/mixed cases handled; no unit-side file or service mutations')
