"""Exercise production interposition against real native fixture functions."""
from pathlib import Path
import os,subprocess,sys,tempfile
source=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    fixture=root/'fixture.c'
    fixture.write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
static void check(void *s,const uint8_t *k) {
    assert(s==(void *)0x1234);assert(k && !memcmp(k,"KEY_MUST_NEVER_APPEAR",20));
    assert(errno==EDOM);errno=EAGAIN;
}
int32_t AirPlayReceiverSessionSetSecurityInfo(void *s,const uint8_t *k,const uint8_t *v){check(s,k);assert(v==(void *)0x5678);return -23;}
int32_t AirPlayReceiverSessionSetHomeKitSecurityContext(void *s,void *c){assert(s==(void *)0x1234);assert(c==(void *)0x9876 || !c);assert(errno==EDOM);errno=EAGAIN;return c ? 0 : -31;}
int32_t AirPlayReceiverSessionScreen_SetSecurityInfo(void *s,const uint8_t *k,const uint8_t *v){check(s,k);assert(v==(void *)0x5678);return 0;}
int32_t AirPlayReceiverSessionScreen_SetChaChaSecurityInfo(void *s,const uint8_t *k,size_t n){check(s,k);assert(n==32 || n==37);return n==32 ? 0 : -32;}
int32_t AES_CBCFrame_Init(void *s,const uint8_t *k,const uint8_t *v,int e){check(s,k);assert(v==(void *)0x5678);assert(e==0 || e==7);return e ? -7 : 0;}
''')
    driver=root/'driver.c'
    driver.write_text(r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
extern int32_t AirPlayReceiverSessionSetSecurityInfo(void *,const uint8_t *,const uint8_t *);
extern int32_t AirPlayReceiverSessionSetHomeKitSecurityContext(void *,void *);
extern int32_t AirPlayReceiverSessionScreen_SetSecurityInfo(void *,const uint8_t *,const uint8_t *);
extern int32_t AirPlayReceiverSessionScreen_SetChaChaSecurityInfo(void *,const uint8_t *,size_t);
extern int32_t AES_CBCFrame_Init(void *,const uint8_t *,const uint8_t *,int);
#define CHECK(call,result) do {errno=EDOM;assert((call)==(result));assert(errno==EAGAIN);}while(0)
int main(void){
    void *s=(void *)0x1234;const uint8_t *k=(const uint8_t *)"KEY_MUST_NEVER_APPEAR_123456789012",*v=(void *)0x5678;
    CHECK(AirPlayReceiverSessionSetSecurityInfo(s,k,v),-23);
    CHECK(AirPlayReceiverSessionSetHomeKitSecurityContext(s,(void *)0x9876),0);
    CHECK(AirPlayReceiverSessionSetHomeKitSecurityContext(s,0),-31);
    CHECK(AirPlayReceiverSessionScreen_SetSecurityInfo(s,k,v),0);
    CHECK(AirPlayReceiverSessionScreen_SetChaChaSecurityInfo(s,k,32),0);
    CHECK(AirPlayReceiverSessionScreen_SetChaChaSecurityInfo(s,k,37),-32);
    CHECK(AES_CBCFrame_Init(s,k,v,0),0);CHECK(AES_CBCFrame_Init(s,k,v,7),-7);
    assert(k[0]=='K');return 0;
}
''')
    def run(args,**kwargs):return subprocess.run(args,check=True,**kwargs)
    run(['gcc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(fixture),'-o',str(root/'libnative.so')])
    run(['gcc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(source),'-ldl','-pthread','-o',str(root/'probe.so')])
    run(['gcc',str(driver),'-L'+str(root),'-Wl,-rpath,'+str(root),'-lnative','-o',str(root/'driver')])
    result=run([str(root/'driver')],env=dict(os.environ,LD_PRELOAD=str(root/'probe.so')),capture_output=True,text=True)
    for name in ['LOADED_observation_only','session_AES','session_HomeKit','screen_AES','screen_ChaCha','CBC_decrypt','CBC_encrypt']:
        assert name in result.stderr,name
    assert 'KEY_MUST_NEVER_APPEAR' not in result.stderr
    # The probe cannot advertise displays, modify setup, or start networking.
    symbols=subprocess.check_output(['nm','-D','--defined-only',str(root/'probe.so')],text=True)
    exported={line.split()[-1] for line in symbols.splitlines()}
    assert exported=={'AirPlayReceiverSessionSetSecurityInfo','AirPlayReceiverSessionSetHomeKitSecurityContext','AirPlayReceiverSessionScreen_SetSecurityInfo','AirPlayReceiverSessionScreen_SetChaChaSecurityInfo','AES_CBCFrame_Init'},exported
    print('PASS: real observation-shim interposition preserves all arguments, success/failure returns, errno and keys; logs no keys; exports only five security observers')
