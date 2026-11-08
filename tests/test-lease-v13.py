"""Real C gate, root monitor, metadata helper and UDP port probe; mocked services.
Checks disconnect/reconnect/expiry/signal restoration without competing writers."""
from pathlib import Path
import os,shutil,socket,subprocess,sys,tempfile,time
header,collector,guard,audit=map(lambda p:Path(p).resolve(),sys.argv[1:])
def wait_for(check,timeout=8):
    until=time.monotonic()+timeout
    while time.monotonic()<until:
        if check():return
        time.sleep(.03)
    raise AssertionError('fixture condition timed out')
driver='''#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
static void log_msg(const char *a,const char *b,...){(void)a;(void)b;}
#include "HEADER"
int main(int n,char **v){assert(n==3);
if(!strcmp(v[2],"request")){int r=frame_gate_at(v[1],1,1);assert(r==1||r==0);}
else if(!strcmp(v[2],"end"))end_frame_gate_at(v[1]);
else if(!strcmp(v[2],"grant")){assert(frame_gate_at(v[1],0,1)==1);assert(frame_gate_at(v[1],1,1)==0);assert(frame_gate_at(v[1],0,1)==0);}
else if(!strcmp(v[2],"expired"))assert(frame_gate_at(v[1],1,1)==-1);
else abort();return 0;}
'''.replace('HEADER',str(header))
with tempfile.TemporaryDirectory() as directory:
    workspace=Path(directory);(workspace/'gate.c').write_text(driver);binary=workspace/'gate'
    subprocess.run(['gcc','-Wall','-Wextra','-Werror',workspace/'gate.c','-o',binary],check=True)
    for scenario in ('no-video','ended-before-grant','disconnect','reconnect','expiry-writer','port-stuck','stop-failure','still-active','unknown-grant','start-failure','signal'):
        root=workspace/scenario;root.mkdir();stage=root/'stage';stage.mkdir()
        data=root/'data/lge/smartlink/carplay';data.mkdir(parents=True)
        request=data/'altscreen-v13-frame-ready';grant=data/'altscreen-v13-encoder-ready';expired=data/'altscreen-v13-expired'
        usb=root/'tmp/USB/PORT1/PART1';usb.mkdir(parents=True);(usb/'ALTSCREEN_TEST_ONCE.txt').write_text('fixture')
        for name,source in [('file-guard',guard),('packet-audit',audit)]:shutil.copyfile(source,stage/name);(stage/name).chmod(0o700)
        (stage/'boot-status.txt').write_text('fixture\n');(root/'native-active').write_text('active')
        def gate(mode):subprocess.run([binary,data,mode],check=True)
        def events():return (root/'events').read_text() if (root/'events').exists() else ''
        def status():return (stage/'boot-status.txt').read_text()
        if scenario!='no-video':gate('request');os.setxattr(request,'user.handoff',b'preserve')
        if scenario=='ended-before-grant':gate('end');assert not request.exists()
        if scenario=='unknown-grant':grant.write_text('unknown state must be preserved')
        tools=root/'tools';tools.mkdir();stub=tools/'systemctl'
        stub.write_text('''#!/usr/bin/env python3
import os,sys,subprocess
from pathlib import Path
r=Path(os.environ['TEST_ROOT']);s=os.environ['SCENARIO'];action=sys.argv[1]
with (r/'events').open('a') as out:out.write(' '.join(sys.argv[1:])+'\\n')
if action=='stop':
 if s=='stop-failure':sys.exit(1)
 if s!='still-active':(r/'native-active').unlink(missing_ok=True)
elif action=='start':
 if subprocess.run([r/'stage/packet-audit','--port-free']).returncode:
  (r/'unsafe-start').write_text('port busy');sys.exit(99)
 if s=='start-failure':sys.exit(1)
 (r/'native-active').write_text('active')
elif action=='is-active':sys.exit(0 if (r/'native-active').exists() else 3)
''');stub.chmod(0o755)
        env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'],TEST_ROOT=str(root),SCENARIO=scenario)
        iterations=5 if scenario=='reconnect' else 3 if scenario in ('disconnect','start-failure') else 1
        process=subprocess.Popen(['sh',collector,stage,root/'logger-ready',str(iterations),root],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        writer=None
        try:
            success=scenario in ('disconnect','reconnect','expiry-writer','port-stuck','start-failure','signal')
            if success:
                wait_for(lambda:'HANDOFF ACTIVE' in status())
                assert grant.read_bytes()==request.read_bytes() and grant.stat().st_mode==request.stat().st_mode
                assert grant.stat().st_uid==request.stat().st_uid and os.getxattr(grant,'user.handoff')==b'preserve'
                gate('grant')
            if scenario in ('disconnect','reconnect','start-failure'):
                gate('end');assert not request.exists()
                wait_for(lambda:'RESTORE FAILED' in status() if scenario=='start-failure' else 'NATIVE RESTORED' in status())
                if scenario=='reconnect':
                    gate('request');wait_for(lambda:events().count('stop multi-display.service')==2 and grant.exists())
                    gate('grant');gate('end');wait_for(lambda:status().count('NATIVE RESTORED')==2)
            elif scenario in ('expiry-writer','port-stuck'):
                writer=socket.socket(socket.AF_INET6,socket.SOCK_DGRAM);writer.bind(('::',42989))
                wait_for(lambda:expired.exists() and not grant.exists())
                assert 'start multi-display.service' not in events()
                if scenario=='expiry-writer':writer.close();writer=None;gate('end')
            elif scenario=='signal':process.terminate()
            stdout,stderr=process.communicate(timeout=15)
            assert process.returncode==0,(scenario,stdout,stderr)
            assert not (root/'unsafe-start').exists()
            assert expired.read_text()=='AUTHENTICATED_H264_V13\n' and expired.stat().st_mode&0o777==0o644
            gate('expired')
            if scenario in ('no-video','ended-before-grant','unknown-grant'):assert 'stop ' not in events() and 'start ' not in events()
            if scenario=='unknown-grant':assert grant.read_text()=='unknown state must be preserved'
            elif success or scenario in ('stop-failure','still-active'):assert not grant.exists()
            if scenario=='port-stuck':assert 'start ' not in events() and 'RESTORE WAIT' in status()
            elif scenario=='start-failure':assert events().count('start multi-display.service')==1 and 'RESTORE FAILED' in status()
            else:assert (root/'native-active').exists()
            if scenario=='reconnect':assert events().count('stop multi-display.service')==2 and events().count('start multi-display.service')==2
            assert (usb/'boot-status.txt').read_text()==status()
        finally:
            if writer:writer.close()
            if process.poll() is None:process.kill();process.wait()
print('PASS: metadata-preserving lease, disconnect/reconnect, expiry/SIGTERM cleanup, final report export, port-busy refusal, failed stop/start and unknown-grant preservation; no competing native writer')
