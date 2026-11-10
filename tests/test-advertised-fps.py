"""Exercise actual AirPlay info hook: primary cap untouched, auxiliary cap30."""
from pathlib import Path
import sys,tempfile,subprocess
handler=Path(sys.argv[1]).resolve();fixture=Path(sys.argv[2]).read_text().split('int main(void)',1)[0]
fixture=fixture.replace('#include "../deploy-usb/payload/altscreen_handler.c"','#include "'+str(handler)+'"')
main=r'''
static LayoutObject *native_root;
static void *native_info(void *s,void *p,int32_t *err){(void)s;(void)p;*err=0;native_root->refs++;return native_root;}
int main(void){
 s_CFStringCreateWithCString=ls;s_CFDictionaryCreateMutable=ld;s_CFDictionaryCreateMutableCopy=lcopy;
 s_CFDictionarySetValue=lset;s_CFDictionaryGetValue=lg;s_CFDictionarySetInt64=ln;s_CFDictionaryGetInt64=lgetn;
 s_CFArrayCreateMutable=la;s_CFArrayAppendValue=lappend;s_CFArrayGetCount=lcount;s_CFArrayGetValueAtIndex=lat;s_CFRelease=lr;
 LayoutObject *stock=la(NULL,0,NULL),*primary=ld(NULL,0,NULL,NULL);native_root=ld(NULL,0,NULL,NULL);
 assert(layout_number(primary,"widthPixels",784)&&layout_number(primary,"heightPixels",468)&&layout_number(primary,"maxFPS",60));
 lappend(stock,primary);assert(layout_object(native_root,"displays",stock));real_AirPlayCopyServerInfo=native_info;
 int32_t err=-1;void *info=AirPlayCopyServerInfo(NULL,NULL,&err);assert(info && err==0 && info!=native_root);
 const LayoutObject *displays=field(info,"displays");assert(displays && displays->count==2);
 const LayoutObject *main_display=lat(displays,0),*aux=lat(displays,1);
 assert(field(main_display,"maxFPS")->number==60 && field(primary,"maxFPS")->number==60);
 assert(field(main_display,"widthPixels")->number==784 && field(main_display,"heightPixels")->number==468);
 assert(field(aux,"type")->number==111 && field(aux,"maxFPS")->number==30);
 assert(field(aux,"widthPixels")->number==1280 && field(aux,"heightPixels")->number==480);
 lr(info);lr(native_root);lr(stock);lr(primary);assert(layout_live==0);
 puts("PASS: actual info hook advertises auxiliary30fps, preserves native60fps/geometry and original dictionary, no object leaks");return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);source=p/'fixture.c';source.write_text(fixture+main);exe=p/'fixture'
 subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(source),'-ldl','-pthread','-o',str(exe)],check=True)
 subprocess.run([exe],check=True,timeout=20)
