"""Real production C gate + root collector + metadata helper; mocked systemctl.
No host services or mounts are changed."""
from pathlib import Path
import os,shutil,subprocess,sys,tempfile,re
header=Path(sys.argv[1]).resolve();collector=Path(sys.argv[2]).resolve();guard=Path(sys.argv[3]).resolve()
protocol=re.search(r'AUTHENTICATED_H264_(V[0-9]+)',header.read_text()).group(1)
marker=protocol.lower()
driver=r'''
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
static void log_msg(const char *a,const char *b,...){(void)a;(void)b;}
#include "HEADER"
int main(int n,char **v){
    assert(n==3);
    if(!strcmp(v[2],"unauthenticated"))assert(frame_gate_at(v[1],1,0)==1);
    else if(!strcmp(v[2],"non-IDR"))assert(frame_gate_at(v[1],0,1)==1);
    else if(!strcmp(v[2],"request"))assert(frame_gate_at(v[1],1,1)==1);
    else if(!strcmp(v[2],"end"))end_frame_gate_at(v[1]);
    else if(!strcmp(v[2],"grant")){
        assert(frame_gate_at(v[1],0,1)==1);
        assert(frame_gate_at(v[1],1,1)==0);
        assert(frame_gate_at(v[1],0,1)==0);
    }else abort();return 0;
}
'''.replace('HEADER',str(header))
with tempfile.TemporaryDirectory() as directory:
    workspace=Path(directory);source=workspace/'gate.c';source.write_text(driver);binary=workspace/'gate'
    subprocess.run(['gcc','-Wall','-Wextra','-Werror',str(source),'-o',str(binary)],check=True)
    for scenario in ('no-video','ended-before-grant','success','stop-failure','still-active','clone-failure'):
        root=workspace/scenario;root.mkdir();stage=root/'stage';stage.mkdir()
        data=root/'data/lge/smartlink/carplay';data.mkdir(parents=True)
        request=data/('altscreen-'+marker+'-frame-ready');grant=data/('altscreen-'+marker+'-encoder-ready')
        shutil.copyfile(guard,stage/'file-guard');(stage/'file-guard').chmod(0o700)
        (stage/'boot-status.txt').write_text('fixture\n')
        subprocess.run([binary,str(data),'unauthenticated'],check=True);assert not request.exists()
        subprocess.run([binary,str(data),'non-IDR'],check=True);assert not request.exists()
        if scenario!='no-video':
            subprocess.run([binary,str(data),'request'],check=True)
            assert request.read_text()=='AUTHENTICATED_H264_'+protocol+'\n'
            os.setxattr(request,'user.handoff',b'preserve')
        if scenario=='clone-failure':grant.write_text('existing grant must not be overwritten')
        if scenario=='ended-before-grant':subprocess.run([binary,str(data),'end'],check=True);assert not request.exists()
        tools=root/'tools';tools.mkdir();stub=tools/'systemctl'
        stub.write_text('''#!/usr/bin/env python3
import os,sys
from pathlib import Path
root=Path(os.environ['TEST_ROOT']);scenario=os.environ['SCENARIO']
with (root/'events').open('a') as f:f.write(' '.join(sys.argv[1:])+'\\n')
if sys.argv[1]=='stop' and scenario=='stop-failure':sys.exit(1)
if sys.argv[1]=='is-active':sys.exit(0 if scenario=='still-active' else 3)
''');stub.chmod(0o755)
        environment=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH'],TEST_ROOT=str(root),SCENARIO=scenario)
        subprocess.run(['sh',collector,str(stage),str(root/'logger-ready'),'2',str(root)],env=environment,check=True,timeout=15)
        events=(root/'events').read_text() if (root/'events').exists() else ''
        if scenario in ('no-video','ended-before-grant'):assert not events and not grant.exists()
        elif scenario=='success':
            assert events.count('stop multi-display.service')==1 and 'start ' not in events
            assert grant.read_bytes()==request.read_bytes()
            assert grant.stat().st_mode==request.stat().st_mode
            assert grant.stat().st_uid==request.stat().st_uid
            assert os.getxattr(grant,'user.handoff')==b'preserve'
            subprocess.run([binary,str(data),'grant'],check=True)
        else:
            assert events.count('stop multi-display.service')==1 and 'start multi-display.service' in events
            assert not grant.exists() and 'HANDOFF FAILED' in (stage/'boot-status.txt').read_text()
print('PASS: no encoder changes before authenticated IDR; one metadata-preserving grant; output waits for grant and IDR; failed handoff restores native encoder')
