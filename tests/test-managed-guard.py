from pathlib import Path
import os,sys,subprocess,tempfile,time,socket
helper=Path(sys.argv[1]).resolve()
def run(*a):return subprocess.run([helper,*map(str,a)],check=True,capture_output=True,text=True)
with tempfile.TemporaryDirectory() as tmp:
 r=Path(tmp);ram=r/'run/moi3-altscreen-map-v31';ram.mkdir(parents=True);ref=r/'request';ref.write_bytes(b'AUTHENTICATED_H264_V31\n');ref.chmod(0o640);os.setxattr(ref,'user.scope',b'lease-test')
 lease=ram/'lease'
 for i in range(30):
  run('pulse',lease,ref);data=lease.read_text().splitlines();assert data[0]=='AUTHENTICATED_H264_V31';deadline=int(data[1]);now=int(time.monotonic()*1000);assert 11000<deadline-now<=12000
  assert os.getxattr(lease,'user.scope')==b'lease-test' and (lease.stat().st_mode&0o777)==0o640
 assert len(list(ram.iterdir()))==1
 before=lease.read_bytes();lease.unlink();lease.symlink_to(ref);bad=subprocess.run([helper,'pulse',lease,ref],capture_output=True);assert bad.returncode and ref.read_bytes()==b'AUTHENTICATED_H264_V31\n';lease.unlink()
 lease.write_text('unknown');assert subprocess.run([helper,'pulse',lease,ref],capture_output=True).returncode and lease.read_text()=='unknown'
 result=run('metadata',ref).stdout;assert 'mode=640' in result and 'user.scope' in result
 run('same-inode',ref,ref);copy=r/'copy';copy.write_bytes(ref.read_bytes());assert subprocess.run([helper,'same-inode',ref,copy],capture_output=True).returncode
 probe=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);probe.bind(('::',42989));assert subprocess.run([helper,'port-free'],capture_output=True).returncode;probe.close();run('port-free')
print('PASS: atomic RAM pulses preserve mode/xattrs, refuse unknown/symlink files, leave no temp accumulation; inode/port/metadata commands exact')
