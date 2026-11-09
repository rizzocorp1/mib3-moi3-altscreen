"""Exercise actual narrow cleanup script/native inventory against isolated files."""
from pathlib import Path
import hashlib,os,shutil,subprocess,sys,tempfile
guard=Path(sys.argv[1]).resolve();template=Path(sys.argv[2]).read_text()
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
for scenario in ('normal','active','changed-program','changed-baseline','changed-metadata','copy-failure','unknown-archive','conflicting-backup','source-symlink','wrong-current','budget'):
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp);base=root/'lge/app_rw/smartlink/carplay';base.mkdir(parents=True)
        (root/'data').mkdir();stage=base/'.moi3-altscreen-map-v19';stage.mkdir();old=base/'.moi3-altscreen-map-v16';old.mkdir()
        usb=root/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);tools=root/'tools';tools.mkdir()
        startup=base/'carplay.sh';startup.write_bytes(b'factory startup');startup.chmod(0o755);os.setxattr(startup,'user.factory',b'preserve')
        binary=base/'carplay';binary.write_bytes(b'factory executable')
        (base/'pairing-data').write_bytes(b'never touch secret pairing data')
        shutil.copy2(startup,stage/'factory-carplay.sh');shutil.copy2(startup,old/'factory-carplay.sh')
        shutil.copyfile(guard,stage/'file-guard');(stage/'file-guard').chmod(0o755)
        program=old/'carplay-once';program.write_bytes(b'obsolete known copied program'*200)
        if scenario=='budget':program.write_bytes(b'x'*(33554432+1))
        expected=digest(program);size=program.stat().st_size;(old/'user-custom-file').write_bytes(b'preserve custom content')
        (old/'boot-status.txt').write_bytes(b'preserve diagnostic evidence')
        values={'GUARD_SHA':digest(stage/'file-guard'),'STARTUP_SHA':digest(startup),'CARPLAY_SHA':digest(binary)}
        script=template
        for key,value in values.items():script=script.replace('@'+key+'@',value)
        begin=script.index("    done <<'VERIFIED_PROGRAM_ROWS'\n")+len("    done <<'VERIFIED_PROGRAM_ROWS'\n")
        end=script.index('\nVERIFIED_PROGRAM_ROWS',begin)
        script=script[:begin]+'v16|carplay-once|'+expected+script[end:]
        path=root/'cleanup.sh';path.write_text(script)
        systemctl=tools/'systemctl';systemctl.write_text('#!/bin/sh\nexit '+('0' if scenario=='active' else '3')+'\n');systemctl.chmod(0o755)
        if scenario=='changed-program':program.write_bytes(b'unknown program modification')
        if scenario=='changed-baseline':(old/'factory-carplay.sh').write_bytes(b'unknown baseline')
        if scenario=='changed-metadata':(old/'factory-carplay.sh').chmod(0o700)
        if scenario=='wrong-current':binary.write_bytes(b'unknown or active mapped executable')
        if scenario=='source-symlink':
            victim=root/'unrelated';program.rename(victim);program.symlink_to(victim)
        if scenario=='copy-failure':
            cp=tools/'cp';cp.write_text('#!/bin/sh\nexit 1\n');cp.chmod(0o755)
        archive=usb/'moi3-obsolete-programs-v19'
        if scenario in ('unknown-archive','conflicting-backup'):
            archive.mkdir()
            if scenario=='unknown-archive':(archive/'user-file').write_bytes(b'preserve user backup')
            else:
                (archive/'BACKUP_SCOPE.txt').write_text('MOI3_OBSOLETE_TEST_PROGRAMS_V19\n');(archive/'v16').mkdir()
                (archive/'v16'/('carplay-once.'+expected+'.backup')).write_bytes(b'preserve conflicting backup')
        env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'])
        result=subprocess.run(['sh',path,stage,usb,root],env=env,capture_output=True,timeout=20)
        report=(usb/'housekeeping-v19.txt').read_text()
        assert old.is_dir() and (old/'factory-carplay.sh').exists() and (old/'boot-status.txt').read_bytes()==b'preserve diagnostic evidence'
        assert (old/'user-custom-file').read_bytes()==b'preserve custom content' and (base/'pairing-data').read_bytes()==b'never touch secret pairing data'
        if scenario=='normal':
            assert result.returncode==0 and not program.exists()
            backup=archive/'v16'/('carplay-once.'+expected+'.backup');assert digest(backup)==expected
            assert 'removedCopies=1' in report and 'logicalBytesRemoved='+str(size) in report and 'availableBytes=' in report
        else:assert program.exists(),(scenario,result.stdout,result.stderr,report)
        if scenario=='budget':assert 'CLEANUP BUDGET REACHED' in report
        assert 'never touch secret pairing data' not in report
print('PASS: native disk inventory; only hash-identified obsolete program removed after verified USB backup; active/unknown/changed/symlink/copy-failure cases preserved; factory metadata, reports and pairing data untouched')
