"""Pass-through /info ABI, bounded binary snapshots and write failure tests."""
from pathlib import Path
import os,plistlib,subprocess,sys,tempfile
source=Path(sys.argv[1]).resolve()
fixture_value={'enabledFeatures':['factoryFeature','viewAreas'],'displays':[{'type':110,'widthPixels':784}]}
encoded=plistlib.dumps(fixture_value,fmt=plistlib.FMT_BINARY)
fixture=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
static int object=12345,calls,writes;
void *FixtureObject(void){return &object;}
int FixtureCalls(void){return calls;}
int FixtureWrites(void){return writes;}
void *AirPlayCopyServerInfo(void *session,void *properties,int32_t *error){
    assert(session==(void *)0x1234);assert(properties==(void *)0x5678 || properties==(void *)0x1111 || !properties);
    calls++;errno=EAGAIN;
    if(properties==(void *)0x1111){assert(error);*error=-42;return NULL;}
    if(error)*error=0;
    return properties ? &object : NULL;
}
#ifndef NO_WRITER
int32_t CFPropertyListWriteToFilePath(const void *plist,const char *format,const char *path){
    writes++;assert(plist==&object && object==12345);assert(format && !strcmp(format,"binary1"));
    assert(path && strstr(path,"/server-info-v5-stock-") && strstr(path,".plist.pending"));
    errno=EINVAL;
    if(!strncmp(path,"/data/",6))return -30;
#ifdef FAIL_WRITER
    return -30;
#else
    FILE *file=fopen(path,"wb");assert(file);
    static const unsigned char bytes[]={FIXTURE_BYTES};
    assert(fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes));assert(fclose(file)==0);return 0;
#endif
}
#endif
int32_t AirPlayReceiverSessionSetSecurityInfo(void *a,const uint8_t *b,const uint8_t *c){(void)a;(void)b;(void)c;return 0;}
int32_t AirPlayReceiverSessionSetHomeKitSecurityContext(void *a,void *b){(void)a;(void)b;return 0;}
int32_t AirPlayReceiverSessionScreen_SetSecurityInfo(void *a,const uint8_t *b,const uint8_t *c){(void)a;(void)b;(void)c;return 0;}
int32_t AirPlayReceiverSessionScreen_SetChaChaSecurityInfo(void *a,const uint8_t *b,size_t c){(void)a;(void)b;(void)c;return 0;}
int32_t AES_CBCFrame_Init(void *a,const uint8_t *b,const uint8_t *c,int d){(void)a;(void)b;(void)c;(void)d;return 0;}
'''.replace('FIXTURE_BYTES',','.join(str(b) for b in encoded))
driver=r'''
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
extern void *AirPlayCopyServerInfo(void *,void *,int32_t *);
extern void *FixtureObject(void);
extern int FixtureCalls(void),FixtureWrites(void);
int main(void){
    int32_t error=99;errno=EDOM;
    assert(!AirPlayCopyServerInfo((void *)0x1234,(void *)0x1111,&error) && error==-42 && errno==EAGAIN);
    errno=EDOM;assert(!AirPlayCopyServerInfo((void *)0x1234,NULL,NULL) && errno==EAGAIN);
    for(int i=0;i<5;i++){
        error=99;errno=EDOM;int32_t *out=i==0 ? NULL : &error;
        assert(AirPlayCopyServerInfo((void *)0x1234,(void *)0x5678,out)==FixtureObject());
        assert(errno==EAGAIN && (!out || error==0));assert(*(int *)FixtureObject()==12345);
    }
    assert(FixtureCalls()==7);assert(FixtureWrites()==EXPECTED_WRITES);return 0;
}
'''
paths=[Path('/tmp')/f'server-info-v5-stock-{n}.plist' for n in (1,2,3)]
def run(args,**kwargs):return subprocess.run(args,check=True,**kwargs)
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    probe=root/'probe.so'
    run(['gcc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(source),'-ldl','-pthread','-o',str(probe)])
    symbols=subprocess.check_output(['nm','-D','--defined-only',str(probe)],text=True)
    assert {line.split()[-1] for line in symbols.splitlines()}=={
        'AirPlayCopyServerInfo','AirPlayReceiverSessionSetSecurityInfo','AirPlayReceiverSessionSetHomeKitSecurityContext',
        'AirPlayReceiverSessionScreen_SetSecurityInfo','AirPlayReceiverSessionScreen_SetChaChaSecurityInfo','AES_CBCFrame_Init'}
    for mode in ('success','write-failure','missing-writer'):
        assert not any(p.exists() or Path(str(p)+'.pending').exists() for p in paths),'Unexpected pre-existing snapshot'
        case=root/mode;case.mkdir()
        native=case/'native.c';native.write_text(fixture)
        defines=['-DNO_WRITER'] if mode=='missing-writer' else ['-DFAIL_WRITER'] if mode=='write-failure' else []
        run(['gcc','-shared','-fPIC','-Wall','-Wextra','-Werror',*defines,str(native),'-o',str(case/'libnative.so')])
        client=case/'driver.c';client.write_text(driver.replace('EXPECTED_WRITES','0' if mode=='missing-writer' else '6'))
        run(['gcc',str(client),'-L'+str(case),'-Wl,-rpath,'+str(case),'-lnative','-o',str(case/'driver')])
        try:
            result=run([str(case/'driver')],env=dict(os.environ,LD_PRELOAD=str(probe)),capture_output=True,text=True)
            assert result.stderr.count('STOCK_INFO_SNAPSHOT')==3
            assert result.stderr.count('STOCK_INFO_PASSTHROUGH')==7
            for path in paths:
                if mode=='success':assert plistlib.loads(path.read_bytes())==fixture_value
                else:assert not path.exists()
        finally:
            for path in paths:
                if path.exists():path.unlink()
                pending=Path(str(path)+'.pending')
                if pending.exists():pending.unlink()
print('PASS: original /info object, properties, errors and errno preserved; correct format/path ABI; three atomic binary snapshots; writer absence/failure cannot alter native results')
