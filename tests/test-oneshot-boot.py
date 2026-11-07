"""Run the real generated boot script with a real metadata helper and mocked
mount/systemctl commands. No host service or real mount is touched."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import re

guard=Path(sys.argv[1]).resolve(); template=Path(sys.argv[2]).read_text()
stage_name=re.search(r'STAGE="\$BASE/([^"\n]+)"',template).group(1)
ram_name=re.search(r'RAM="\$R/run/([^"\n]+)"',template).group(1)
map_mode='MAP TEST ACTIVE:' in template
stop_mode='systemctl stop' in template
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
scenarios=('active','unarmed','corrupt-patch','corrupt-backup','mount-failure','logger-failure','logger-timeout','corrupt-collector')
if stop_mode:scenarios+=('stop-failure',)
if 'pending authenticated video' in template:scenarios+=('stale-handoff',)
for scenario in scenarios:
    with tempfile.TemporaryDirectory() as directory:
        root=Path(directory);base=root/'lge/app_rw/smartlink/carplay';base.mkdir(parents=True)
        stage=base/stage_name;stage.mkdir()
        startup=base/'carplay.sh';startup.write_bytes(b'#!/bin/sh\nexit 0\n');startup.chmod(0o755)
        os.setxattr(startup,'user.startup-label',b'factory-metadata')
        backup=stage/'factory-carplay.sh'
        subprocess.run([guard,'clone',startup,backup],check=True)
        target=base/'carplay';target.write_bytes(b'factory executable');target.chmod(0o755)
        factory_copy=root/'factory-copy';shutil.copyfile(target,factory_copy)
        patched=stage/'carplay-once';patched.write_bytes(b'experimental executable')
        handler=stage/'altscreen_handler.so';handler.write_bytes(b'handler')
        collector=stage/'managed-log-collector.sh';collector.write_bytes(b'collector fixture')
        local_guard=stage/'file-guard';shutil.copyfile(guard,local_guard);local_guard.chmod(0o700)
        (root/'run').mkdir()
        units=root/'lib/systemd/system';units.mkdir(parents=True);(units/'multi-display.service').write_text('[Service]\n')
        values={'ROOT':str(root),'GUARD_SHA':digest(local_guard),'STARTUP_SHA':digest(backup),'CARPLAY_SHA':digest(target),'PATCH_SHA':digest(patched),'HANDLER_SHA':digest(handler),'COLLECTOR_SHA':digest(collector),'LOG_ITERATIONS':'1'}
        rendered=template
        for key,value in values.items():rendered=rendered.replace('@'+key+'@',value)
        assert '@ROOT@' not in rendered
        startup.write_text(rendered)
        boot=root/'run-boot.sh';boot.write_text(rendered)
        if scenario!='unarmed':(stage/'ARMED').write_text('security-observation-v3\n')
        if scenario=='corrupt-patch':patched.write_bytes(b'bad executable')
        if scenario=='corrupt-backup':backup.write_bytes(b'bad backup')
        if scenario=='corrupt-collector':collector.write_bytes(b'bad collector')
        if scenario=='stale-handoff':
            stale=root/'data/lge/smartlink/carplay/altscreen-v6-frame-ready';stale.parent.mkdir(parents=True);stale.write_text('old request')
        tools=root/'tools';tools.mkdir();events=root/'events'
        stub='''#!/usr/bin/env python3
import os,sys
from pathlib import Path
root=Path(os.environ['TEST_ROOT']);base=root/'lge/app_rw/smartlink/carplay';stage=base/'@@STAGE@@'
command=Path(sys.argv[0]).name
with (root/'events').open('a') as log:log.write(command+' '+' '.join(sys.argv[1:])+'\\n')
if command=='systemd-run':
    assert (base/'carplay.sh').read_bytes()==(stage/'factory-carplay.sh').read_bytes()
    assert (base/'carplay').read_bytes()==(root/'factory-copy').read_bytes()
    if os.environ['SCENARIO']=='logger-failure':sys.exit(1)
    if os.environ['SCENARIO']!='logger-timeout':(root/'run/@@RAM@@/logger-ready').write_text('READY')
elif command=='mount':
    assert (base/'carplay.sh').read_bytes()==(stage/'factory-carplay.sh').read_bytes()
    assert (stage/'CONSUMED').exists() and not (stage/'ARMED').exists()
    if os.environ['SCENARIO']=='mount-failure':sys.exit(1)
    Path(sys.argv[3]).write_bytes(Path(sys.argv[2]).read_bytes())
elif command=='umount':
    Path(sys.argv[1]).write_bytes((root/'factory-copy').read_bytes())
elif command=='systemctl':
    if sys.argv[1]=='is-active':sys.exit(3)
    if sys.argv[1]=='stop' and os.environ['SCENARIO']=='stop-failure':sys.exit(1)
'''.replace('@@STAGE@@',stage_name).replace('@@RAM@@',ram_name)
        for name in ('mount','umount','systemctl','systemd-run'):
            file=tools/name;file.write_text(stub);file.chmod(0o755)
        env=dict(os.environ,TEST_ROOT=str(root),SCENARIO=scenario,PATH=str(tools)+':'+os.environ['PATH'])
        subprocess.run(['sh','-n',boot],check=True)
        result=subprocess.run(['sh',boot],env=env,capture_output=True,timeout=15)
        assert result.returncode==0,(scenario,result.stderr)
        log=(stage/'boot-status.txt').read_text()
        history=events.read_text() if events.exists() else ''
        if scenario=='corrupt-backup':
            assert 'ABORT' in log and not history and target.read_bytes()==factory_copy.read_bytes()
        else:
            assert startup.read_bytes()==backup.read_bytes()
            assert os.getxattr(startup,'user.startup-label')==b'factory-metadata'
            if scenario=='active':
                assert ('MAP TEST ACTIVE:' if map_mode else 'DIAGNOSTIC ACTIVE:') in log and 'mount --bind' in history
                assert ('systemctl stop' in history)==stop_mode
                assert 'systemctl disable' not in history
                assert target.read_bytes()==patched.read_bytes()
                # A reboot removes the bind mount; simulate that, then show a
                # second execution cannot consume another ticket or reactivate.
                target.write_bytes(factory_copy.read_bytes());events.write_text('')
                subprocess.run(['sh',boot],env=env,check=True,timeout=15)
                assert events.read_text()=='' and 'DISARMED' in (stage/'boot-status.txt').read_text()
            elif scenario in ('unarmed','corrupt-patch','corrupt-collector','stale-handoff'):
                assert not history and target.read_bytes()==factory_copy.read_bytes()
            elif scenario in ('logger-failure','logger-timeout'):
                assert 'ABORT' in log and 'mount --bind' not in history and 'systemctl stop' not in history
                assert target.read_bytes()==factory_copy.read_bytes()
            else:
                assert 'ABORT' in log and target.read_bytes()==factory_copy.read_bytes()
                if scenario=='mount-failure':assert 'systemctl stop' not in history
                if scenario=='stop-failure':assert 'umount ' in history
print('PASS: startup restored before activation, ticket consumed once, metadata retained, corruption and mount/service failures leave native executable')
