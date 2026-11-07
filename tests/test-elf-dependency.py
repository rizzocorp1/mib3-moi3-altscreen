import hashlib
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile

spec=importlib.util.spec_from_file_location('patcher',sys.argv[1]);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    (root/'stock.c').write_text('int fixture_value(void){return 7;}\n')
    (root/'hook.c').write_text('int fixture_value(void){return 42;}\n')
    (root/'main.c').write_text('#include <stdio.h>\nextern int fixture_value(void);int main(void){printf("%d\\n",fixture_value());return 0;}\n')
    subprocess.run(['gcc','-fPIC','-shared',str(root/'stock.c'),'-Wl,-soname,libstock.so','-o',str(root/'libstock.so')],check=True)
    subprocess.run(['gcc','-fPIC','-shared',str(root/'hook.c'),'-o',str(root/'hook.so')],check=True)
    original=root/'original';copy=root/'copy'
    subprocess.run(['gcc',str(root/'main.c'),'-L'+str(root),'-lstock','-Wl,-rpath,$ORIGIN','-o',str(original)],check=True)
    before=original.read_bytes();env={k:v for k,v in os.environ.items() if k not in ('LD_PRELOAD','LD_LIBRARY_PATH')}
    assert subprocess.check_output([original],env=env)==b'7\n'
    report=module.patch(original,copy,str(root/'hook.so'));copy.chmod(0o755)
    assert subprocess.check_output([copy],env=env)==b'42\n'
    assert original.read_bytes()==before and subprocess.check_output([original],env=env)==b'7\n'
    assert report['instructionPatches']==0 and report['newSegmentFlags']=='R'
    try:module.patch(original,copy,str(root/'hook.so'))
    except ValueError:pass
    else:raise AssertionError('Existing output overwritten')
    try:module.patch(copy,root/'double',str(root/'hook.so'))
    except ValueError:pass
    else:raise AssertionError('Duplicate hook accepted')
    print('PASS: executable dependency interposition without LD_PRELOAD; original unchanged; overwrite and duplicate refused')
