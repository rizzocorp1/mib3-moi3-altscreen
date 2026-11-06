"""Actual service-manager integration; never uses vehicle units or paths.
Run as root on an isolated Linux runner booted with systemd. No silent skip.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import time
import uuid

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
    collector=Path(__file__).resolve().parents[1]/'recovery/managed-log-collector.sh'
    managed=script('managed.sh',f'exec /bin/sh {collector} {stage} {root}/managed-ready 4 {root}\n')
    pre=script('pre.sh',f'''/bin/sh {legacy} &
systemd-run --unit={logger} {property_arguments} /bin/sh {managed}
i=0
while [ ! -f {root}/legacy-ready ] || [ ! -f {root}/managed-ready ]; do
    i=$((i+1)); [ "$i" -lt 50 ]; sleep .1
done
touch {root}/pre-finished
''')
    main=script('main.sh',f'touch {root}/main-started\nprintf "AFTER MAIN" > {stage}/boot-status.txt\nsleep 10\n')
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
        print('Observed legacy child survival on this manager:', (root/'legacy-survived').exists())
        print(command('systemctl','--version').stdout)
        print('PASS: separately managed production collector exported evidence written after main-service startup')
    finally:
        subprocess.run(['systemctl','stop',unit,logger],capture_output=True)
        subprocess.run(['systemctl','reset-failed',unit,logger],capture_output=True)
        unit_path.unlink(missing_ok=True)
        subprocess.run(['systemctl','daemon-reload'],capture_output=True)
