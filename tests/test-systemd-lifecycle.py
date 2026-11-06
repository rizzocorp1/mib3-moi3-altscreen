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
unit=identifier+'.service';logger=identifier+'-logger.service'
unit_path=Path('/run/systemd/system')/unit
with tempfile.TemporaryDirectory(prefix='moi3-lifecycle-') as directory:
    root=Path(directory);root.chmod(0o755)
    def script(name,content):
        file=root/name;file.write_text('#!/bin/sh\nset -eu\n'+content);file.chmod(0o755);return file
    legacy=script('legacy.sh',f'touch {root}/legacy-ready\nwhile [ ! -f {root}/main-started ]; do sleep .1; done\nsleep .5\ntouch {root}/legacy-survived\n')
    managed=script('managed.sh',f'touch {root}/managed-ready\nsleep 2\ntouch {root}/managed-survived\n')
    pre=script('pre.sh',f'''/bin/sh {legacy} &
systemd-run --unit={logger} /bin/sh {managed}
i=0
while [ ! -f {root}/legacy-ready ] || [ ! -f {root}/managed-ready ]; do
    i=$((i+1)); [ "$i" -lt 50 ]; sleep .1
done
touch {root}/pre-finished
''')
    main=script('main.sh',f'touch {root}/main-started\nsleep 5\n')
    unit_path.write_text(f'[Unit]\nDescription=Isolated MOI3 lifecycle regression fixture\n[Service]\nType=simple\nExecStartPre=/bin/sh {pre}\nExecStart=/bin/sh {main}\nTimeoutStartSec=15\nTimeoutStopSec=3\n')
    try:
        command('systemctl','daemon-reload');command('systemctl','start',unit)
        wait(root/'main-started');wait(root/'managed-survived')
        assert (root/'pre-finished').exists()
        assert not (root/'legacy-survived').exists(), 'Legacy ExecStartPre child unexpectedly survived'
        print('PASS: actual systemd kills the old collector, while a separately managed collector survives into main service execution')
    finally:
        subprocess.run(['systemctl','stop',unit,logger],capture_output=True)
        subprocess.run(['systemctl','reset-failed',unit,logger],capture_output=True)
        unit_path.unlink(missing_ok=True)
        subprocess.run(['systemctl','daemon-reload'],capture_output=True)
