"""Actual service-manager integration; never uses vehicle units or paths.
Run as root on an isolated Linux runner booted with systemd. No silent skip.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import time
import uuid
import sys

assert os.geteuid()==0, 'Root required for isolated transient-service test'
assert Path('/proc/1/comm').read_text().strip()=='systemd', 'BLOCKED: real systemd PID 1 required; mocks are insufficient'
def command(*args):return subprocess.run(args,check=True,capture_output=True,text=True)
def wait(path, seconds=10):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        if path.exists():return
        time.sleep(.1)
    raise AssertionError('Timed out waiting for '+str(path))
identifier='moi3-lifecycle-'+uuid.uuid4().hex
# Exercise the actual boot template's option spelling. v232 accepts
# RuntimeMaxSec but its transient assignment parser rejects TimeoutStopSec.
boot_template=(Path(__file__).resolve().parents[1]/'recovery/oneshot-boot.sh.in').read_text()
launch=next(line for line in boot_template.splitlines() if line.startswith('if ! systemd-run '))
properties=[word for word in launch.split() if word.startswith('--property=')]
assert properties==['--property=RuntimeMaxSec=330'], 'Unreviewed factory-manager properties'
map_template=Path(__file__).resolve().parents[1]/'recovery/map-v4-boot.sh.in'
if map_template.exists():
    map_launch=next(line for line in map_template.read_text().splitlines() if line.startswith('if ! systemd-run '))
    assert [w for w in map_launch.split() if w.startswith('--property=')]==properties
info_template=Path(__file__).resolve().parents[1]/'recovery/info-v5-boot.sh.in'
if info_template.exists():
    info_launch=next(line for line in info_template.read_text().splitlines() if line.startswith('if ! systemd-run '))
    assert [w for w in info_launch.split() if w.startswith('--property=')]==properties
v6_template=Path(__file__).resolve().parents[1]/'recovery/map-v6-boot.sh.in'
if v6_template.exists():
    v6_launch=next(line for line in v6_template.read_text().splitlines() if line.startswith('if ! systemd-run '))
    assert [w for w in v6_launch.split() if w.startswith('--property=')]==properties
v7_template=Path(__file__).resolve().parents[1]/'recovery/map-v7-boot.sh.in'
if v7_template.exists():
    v7_launch=next(line for line in v7_template.read_text().splitlines() if line.startswith('if ! systemd-run '))
    assert [w for w in v7_launch.split() if w.startswith('--property=')]==properties
lease_template=Path(__file__).resolve().parents[1]/'recovery/map-v15-boot.sh.in'
if lease_template.exists():
    lease_launch=next(line for line in lease_template.read_text().splitlines() if line.startswith('if ! systemd-run '))
    lease_properties=[w for w in lease_launch.split() if w.startswith('--property=')]
    assert lease_properties==['--property=RuntimeMaxSec=720']
    command('systemd-run','--wait','--unit='+identifier+'-lease-property',*lease_properties,'/bin/true')
property_arguments=' '.join(properties)
unit=identifier+'.service';logger=identifier+'-logger.service'
unit_path=Path('/run/systemd/system')/unit
with tempfile.TemporaryDirectory(prefix='moi3-lifecycle-') as directory:
    root=Path(directory);root.chmod(0o755)
    def script(name,content):
        file=root/name;file.write_text('#!/bin/sh\nset -eu\n'+content);file.chmod(0o755);return file
    legacy=script('legacy.sh',f'touch {root}/legacy-ready\nwhile [ ! -f {root}/main-started ]; do sleep .1; done\nsleep .5\ntouch {root}/legacy-survived\n')
    stage=root/'stage';stage.mkdir()
    usb=root/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True)
    (usb/'ALTSCREEN_TEST_ONCE.txt').write_text('fixture')
    (stage/'boot-status.txt').write_text('BEFORE MAIN')
    collector=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'recovery/managed-log-collector.sh'
    info_mode=collector.name=='info-v5-log-collector.sh'
    managed=script('managed.sh',f'exec /bin/sh {collector} {stage} {root}/managed-ready 4 {root}\n')
    pre=script('pre.sh',f'''/bin/sh {legacy} &
systemd-run --unit={logger} {property_arguments} /bin/sh {managed}
i=0
while [ ! -f {root}/legacy-ready ] || [ ! -f {root}/managed-ready ]; do
    i=$((i+1)); [ "$i" -lt 50 ]; sleep .1
done
touch {root}/pre-finished
''')
    main=script('main.sh',f'touch {root}/main-started\nprintf "AFTER MAIN" > {stage}/boot-status.txt\nprintf "STOCK SNAPSHOT" > {root}/tmp/server-info-v5-stock-1.plist\nsleep 10\n')
    unit_path.write_text(f'[Unit]\nDescription=Isolated MOI3 lifecycle regression fixture\n[Service]\nType=simple\nExecStartPre=/bin/sh {pre}\nExecStart=/bin/sh {main}\nTimeoutStartSec=15\nTimeoutStopSec=3\n')
    try:
        command('systemctl','daemon-reload');command('systemctl','start',unit)
        wait(root/'main-started')
        end=time.monotonic()+8
        while time.monotonic()<end:
            if (usb/'boot-status.txt').exists() and (usb/'boot-status.txt').read_text()=='AFTER MAIN':break
            time.sleep(.1)
        else:raise AssertionError('Managed collector did not export main-service evidence')
        assert (root/'pre-finished').exists()
        if info_mode:
            wait(usb/'stock-info-1.plist')
            assert (usb/'stock-info-1.plist').read_text()=='STOCK SNAPSHOT'
        print('Observed legacy child survival on this manager:', (root/'legacy-survived').exists())
        print(command('systemctl','--version').stdout)
        print('PASS: separately managed production collector exported evidence written after main-service startup')
    finally:
        subprocess.run(['systemctl','stop',unit,logger],capture_output=True)
        subprocess.run(['systemctl','reset-failed',unit,logger],capture_output=True)
        unit_path.unlink(missing_ok=True)
        subprocess.run(['systemctl','daemon-reload'],capture_output=True)
