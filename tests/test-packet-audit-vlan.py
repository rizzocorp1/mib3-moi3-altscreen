"""Require genuine outgoing multicast capture on an isolated VLAN fixture.
Run only under unshare --net; never reconfigure the host network."""
import os,socket,struct,subprocess,sys,tempfile,time
from pathlib import Path
assert len(sys.argv)==3 and sys.argv[2]=='--isolated'
assert os.stat('/proc/self/ns/net').st_ino!=os.stat('/proc/1/ns/net').st_ino,'network namespace isolation required'
binary=Path(sys.argv[1]).resolve()
def ip(*arguments):subprocess.run(['ip',*arguments],check=True)
ip('link','add','moi3audit0','type','veth','peer','name','moi3audit1')
for parent in ('moi3audit0','moi3audit1'):
    ip('link','set',parent,'up')
    ip('link','add','link',parent,'name',parent+'.5','type','vlan','id','5')
    ip('link','set',parent+'.5','up')
ip('-6','addr','add','2001:db8::1/64','dev','moi3audit0.5','nodad')
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory);report=root/'metadata';ready=root/'ready'
    process=subprocess.Popen([binary,'moi3audit0.5',report,ready,'3'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    try:
        for _ in range(100):
            if ready.exists() or process.poll() is not None:break
            time.sleep(.02)
        assert ready.exists(),process.communicate()
        sender=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
        index=socket.if_nametoindex('moi3audit0.5')
        sender.setsockopt(socket.IPPROTO_IPV6,socket.IPV6_MULTICAST_IF,index)
        sender.bind(('2001:db8::1',42989))
        null_ts=bytes.fromhex('471fff10')+b'\xff'*184
        for sequence in range(5):
            payload=struct.pack('!BBHII',0x80,33,sequence,1000+sequence,100000)+null_ts
            sender.sendto(payload,('ff14::2:4',42800,0,index));time.sleep(.03)
        sender.close();stdout,stderr=process.communicate(timeout=6)
        assert process.returncode==0,(stdout,stderr)
        text=report.read_text()
        assert 'direction=outgoing sourcePort=42989' in text and 'clusterGroup=1' in text,text
        assert '2001:db8' not in text and 'ff14::2:4' not in text
    finally:
        if process.poll() is None:process.kill();process.wait()
print('PASS: actual outgoing IPv6 multicast on VLAN5, not merely loopback receive copies; no host network changes')
