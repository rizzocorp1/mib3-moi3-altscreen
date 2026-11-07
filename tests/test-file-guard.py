"""Linux integration checks for actual copy/atomic-restore behavior."""
import ctypes
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

binary = os.path.abspath(sys.argv[1])
def run(*args, success=True, env=None):
    result = subprocess.run([binary, *map(str,args)],capture_output=True,env=env)
    assert (result.returncode == 0) == success, (args,result.stdout,result.stderr)
    return result
def attrs(file):
    return {name:os.getxattr(file,name) for name in os.listxattr(file)}
def metadata(file):
    s=os.stat(file)
    return (stat.S_IMODE(s.st_mode),s.st_uid,s.st_gid,attrs(file))
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp); original=root/'original'; backup=root/'backup'; new=root/'new'
    original.write_bytes(b'factory startup\n'*5000); original.chmod(0o640)
    os.setxattr(original,'user.guard-test',b'original metadata\x00\xff')
    acl=ctypes.CDLL('libacl.so.1')
    acl.acl_from_text.argtypes=[ctypes.c_char_p];acl.acl_from_text.restype=ctypes.c_void_p
    acl.acl_set_file.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.c_void_p]
    acl.acl_free.argtypes=[ctypes.c_void_p]
    value=acl.acl_from_text(b'u::rw-,u:65534:r--,g::---,m::r--,o::---')
    assert value and acl.acl_set_file(os.fsencode(original),0x8000,value)==0
    acl.acl_free(value)
    before=metadata(original); contents=original.read_bytes()
    assert 'system.posix_acl_access' in before[3]
    run('clone',original,backup)
    assert backup.read_bytes()==contents and metadata(backup)==before
    run('clone',original,backup,success=False)
    assert backup.read_bytes()==contents
    new.write_bytes(b'temporary one-shot startup\n')
    new.chmod(0o777);os.setxattr(new,'user.wrong-metadata',b'never copy this')
    run('replace',new,original)
    assert original.read_bytes()==new.read_bytes() and metadata(original)==before
    run('same-meta',backup,original)
    run('same-meta',new,original,success=False)
    run('replace',backup,original)
    assert original.read_bytes()==contents and metadata(original)==before
    created=root/'launcher'
    run('create',new,created,original)
    assert created.read_bytes()==new.read_bytes() and metadata(created)==before
    link=root/'link';link.symlink_to(original)
    run('replace',new,link,success=False)
    run('clone',link,root/'bad-copy',success=False)
    assert original.read_bytes()==contents and link.is_symlink()
    hard=root/'hard';os.link(new,hard)
    run('clone',new,root/'bad-hard-copy',success=False)
    hard.unlink()
    directory=root/'directory';directory.mkdir();directory.chmod(0o750)
    os.setxattr(directory,'user.directory-label',b'label-fixture')
    other=root/'other-directory';other.mkdir();os.setxattr(other,'user.extra',b'remove')
    run('meta',directory,other)
    assert metadata(directory)==metadata(other)
    failure_env=dict(os.environ,FILE_GUARD_FAIL_BEFORE_COMMIT='1')
    run('replace',new,original,success=False,env=failure_env)
    assert original.read_bytes()==contents and metadata(original)==before
    assert not list(root.glob('original.moi3-*'))
    if os.geteuid()!=0:
        protected=root/'protected';protected.mkdir();target=protected/'startup';target.write_bytes(b'untouched')
        protected.chmod(0o500)
        try:
            run('replace',new,target,success=False)
            assert target.read_bytes()==b'untouched'
        finally:protected.chmod(0o700)
print('PASS: exact copy, ACL/xattrs/ownership/modes, atomic restore, immutable backup, symlink/hardlink refusal and pre-commit failure')
