from pathlib import Path
import os,sys,subprocess,tempfile,time,uuid
from managed_fixture import fixture,digest
assert os.geteuid()==0 and Path('/proc/1/comm').read_text().strip()=='systemd'
guard,writer,templates=map(lambda x:Path(x).resolve(),sys.argv[1:4])
def call(*a):return subprocess.run(a,check=True,capture_output=True,text=True)
def wait(check,seconds=12):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if check():return
  time.sleep(.05)
 raise AssertionError('condition timed out')
for mode in ('disconnect-reconnect','kill','hang'):
 with tempfile.TemporaryDirectory() as tmp:
  r=Path(tmp);base,stage,ram,data,values=fixture(r,guard,templates)
  call(guard,'replace',stage/'managed-boot.sh',base/'carplay.sh')
  tools=r/'tools';tools.mkdir();ctl=tools/'systemctl'
  ctl.write_text('''#!/usr/bin/python3
from pathlib import Path
import sys,socket
r=Path(ROOT);a=sys.argv[1]
if a=='stop':(r/'native-active').unlink(missing_ok=True)
elif a=='is-active':sys.exit(0 if (r/'native-active').exists() else 3)
elif a=='start':
 s=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
 try:s.bind(('::',42989))
 except OSError:(r/'unsafe-native-start').write_text('busy');sys.exit(2)
 s.close();(r/'native-active').write_text('yes')
'''.replace('ROOT',repr(str(r))));ctl.chmod(0o755)
  boot=(stage/'managed-boot.sh').read_text();text=boot.split('<<EOF_MANAGED_UNIT\n',1)[1].split('\nEOF_MANAGED_UNIT',1)[0]
  for key,value in (('$STAGE',str(stage)),('$RAM',str(ram)),('$R',str(r))):text=text.replace(key,value)
  text+='\nEnvironment=PATH='+str(tools)+':/usr/bin:/bin\n'
  unit='moi3-managed-fixture-'+uuid.uuid4().hex;unitfile=Path('/run/systemd/system')/(unit+'.service');unitfile.write_text(text)
  process=None
  try:
   call('systemctl','daemon-reload');call('systemctl','start','--no-block',unit);wait(lambda:(ram/'ready').exists())
   ready=r/'writer-ready';process=subprocess.Popen([writer,data,ram/'lease',ready],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
   wait(lambda:ready.exists());assert not (r/'native-active').exists()
   if mode=='disconnect-reconnect':
    for cycle in range(2):
     Path(str(ready)+'.stop').write_text('stop');process.wait(timeout=4);process=None
     wait(lambda:(r/'native-active').exists());assert (stage/'ENABLED').exists()
     if cycle==0:
      ready=r/'writer-ready-2';process=subprocess.Popen([writer,data,ram/'lease',ready],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);wait(lambda:ready.exists());assert not (r/'native-active').exists()
    call('systemctl','stop',unit);assert (stage/'ENABLED').exists() and digest(base/'carplay.sh')==values['BOOT_SHA']
   else:
    call('systemctl','kill','--kill-whom=main','--signal='+('SIGSTOP' if mode=='hang' else 'SIGKILL'),unit)
    if mode=='hang':
     wait(lambda:process.poll() is not None,17);assert not (r/'native-active').exists()
     print('PASS: actual C sender independently released port after RAM lease expired while controller was stopped')
    wait(lambda:(r/'native-active').exists(),65);wait(lambda:not (stage/'ENABLED').exists(),5)
    assert digest(base/'carplay.sh')==values['STARTUP_SHA'];process.wait(timeout=4);process=None
   assert not (r/'unsafe-native-start').exists()
   print('PASS: real systemd '+mode+', renewed lease, no competing writers, correct persistent/stock fallback state')
  finally:
   if process and process.poll() is None:process.kill();process.wait()
   subprocess.run(['systemctl','stop',unit],capture_output=True);subprocess.run(['systemctl','reset-failed',unit],capture_output=True)
   print((stage/'boot-status.txt').read_text())
   if mode!='disconnect-reconnect':print(subprocess.run(['journalctl','-u',unit,'--no-pager','-n','18'],capture_output=True,text=True).stdout)
   unitfile.unlink(missing_ok=True);subprocess.run(['systemctl','daemon-reload'],capture_output=True)
