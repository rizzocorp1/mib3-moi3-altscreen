import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import re

guard=Path(sys.argv[1]).resolve()
entry_template=Path(sys.argv[2]).read_text();boot_template=Path(sys.argv[3]).read_text()
stage_name=re.search(r'STAGE="\$BASE/([^"\n]+)"',entry_template).group(1)
map_mode='MAP TEST ARMED' in entry_template
info_mode='STOCK INFO OBSERVATION' in entry_template
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
scenarios=('arm-and-restore','mixed-stick','copy-failure','wrong-baseline')
marker='v9' if 'altscreen-v9-frame-ready' in entry_template else 'v6'
if ('altscreen-v6-frame-ready' in entry_template or 'altscreen-v9-frame-ready' in entry_template):scenarios+=('stale-handoff',)
for scenario in scenarios:
    with tempfile.TemporaryDirectory() as directory:
        root=Path(directory);base=root/'lge/app_rw/smartlink/carplay';base.mkdir(parents=True)
        startup=base/'carplay.sh';startup.write_bytes(b'#!/bin/sh\nexit 0\n');startup.chmod(0o755)
        os.setxattr(startup,'user.original-label',b'preserve')
        target=base/'carplay';target.write_bytes(b'original executable');target.chmod(0o755)
        library=base.parents[1]/'lib64/libAirPlay.so';library.parent.mkdir();library.write_bytes(b'original library');library.chmod(0o755)
        os.setxattr(library,'user.library-label',b'library')
        release=root/'etc/os-release';release.parent.mkdir();release.write_bytes(b'ID=test-fixture\n')
        if info_mode:
            prior=root/'data/lge/smartlink/carplay/server_info_v62.plist';prior.parent.mkdir(parents=True)
            prior.write_bytes(b'prior modified snapshot fixture')
        usb=root/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);payload=usb/'payload';payload.mkdir()
        shutil.copyfile(guard,payload/'file-guard')
        (payload/'carplay-once').write_bytes(b'test executable')
        (payload/'altscreen_handler.so').write_bytes(b'test handler')
        (payload/'managed-log-collector.sh').write_bytes(b'collector fixture')
        (usb/'ALTSCREEN_TEST_ONCE.txt').write_text('one session')
        values={'ROOT':str(root),'GUARD_SHA':digest(payload/'file-guard'),'STARTUP_SHA':digest(startup),'CARPLAY_SHA':digest(target),'OS_SHA':digest(release),'PATCH_SHA':digest(payload/'carplay-once'),'HANDLER_SHA':digest(payload/'altscreen_handler.so'),'LOG_ITERATIONS':'0'}
        values['COLLECTOR_SHA']=digest(payload/'managed-log-collector.sh')
        old_stage=base/'.moi3-altscreen-once-v1';old_stage.mkdir()
        (old_stage/'boot-status.txt').write_text('retained previous test evidence')
        (base/'carplay-handler.log').write_text('previous v1 handler evidence')
        def render(template):
            for key,value in values.items():template=template.replace('@'+key+'@',value)
            return template
        (payload/'oneshot-boot.sh').write_text(render(boot_template));values['BOOT_SHA']=digest(payload/'oneshot-boot.sh')
        entry=usb/'fix_hw_partnumber.sh';entry.write_text(render(entry_template))
        factory=startup.read_bytes();original_exec=target.read_bytes()
        tools=root/'tools';tools.mkdir();mock_id=tools/'id';mock_id.write_text('#!/bin/sh\nprintf "0\\n"\n');mock_id.chmod(0o755)
        env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'])
        if scenario=='mixed-stick':(usb/'PROBE_ONLY.txt').write_text('old probe')
        if scenario=='copy-failure':env['FILE_GUARD_FAIL_BEFORE_COMMIT']='1'
        if scenario=='wrong-baseline':target.write_bytes(b'unknown executable')
        if scenario=='stale-handoff':
            stale=root/('data/lge/smartlink/carplay/altscreen-'+marker+'-frame-ready');stale.parent.mkdir(parents=True);stale.write_text('old request')
        subprocess.run(['sh','-n',entry],check=True)
        result=subprocess.run(['sh',entry],env=env,capture_output=True,timeout=20)
        assert result.returncode==0,(scenario,result.stderr)
        report=(usb/'altscreen-setup.txt').read_text();stage=base/stage_name
        if scenario=='arm-and-restore':
            assert ('RESULT: MAP TEST ARMED' if map_mode else 'RESULT: DIAGNOSTIC ARMED') in report and 'SETUP COMPLETE' in report,report
            assert (stage/'ARMED').is_file() and digest(startup)==values['BOOT_SHA']
            assert (stage/'factory-carplay.sh').read_bytes()==factory
            assert (usb/'previous-v1-boot-status.txt').read_text()=='retained previous test evidence'
            assert (usb/'previous-v1-handler-apprw.log').read_text()=='previous v1 handler evidence'
            if info_mode:assert (usb/'previous-modified-info.plist').read_bytes()==prior.read_bytes()
            assert (stage/'managed-log-collector.sh').read_bytes()==(payload/'managed-log-collector.sh').read_bytes()
            assert (usb/'factory-carplay.sh.backup').read_bytes()==factory
            assert target.read_bytes()==original_exec
            assert os.getxattr(startup,'user.original-label')==b'preserve'
            assert os.getxattr(stage/'altscreen_handler.so','user.library-label')==b'library'
            # Explicit recovery runs from the same USB helper and preserves the
            # exact original startup metadata without deleting backup assets.
            (usb/'RESTORE_FACTORY_STARTUP.txt').write_text('restore')
            subprocess.run(['sh',entry],env=env,check=True,timeout=20)
            report=(usb/'altscreen-setup.txt').read_text()
            assert 'RESULT: FACTORY STARTUP RESTORED' in report,report
            assert startup.read_bytes()==factory and target.read_bytes()==original_exec
            assert os.getxattr(startup,'user.original-label')==b'preserve'
            assert not (stage/'ARMED').exists() and (stage/'factory-carplay.sh').exists()
        else:
            assert 'SETUP FAILED' in report,report
            assert startup.read_bytes()==factory and not (stage/'ARMED').exists()
print('PASS: real setup staging/arming, original executable untouched, metadata preserved, explicit recovery and failed setup refusal')
