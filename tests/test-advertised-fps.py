"""Exercise actual AirPlay info hook: primary cap untouched, auxiliary cap30."""
from pathlib import Path
import sys,tempfile,subprocess
handler=Path(sys.argv[1]).resolve();fixture=Path(sys.argv[2]).read_text().split('int main(void)',1)[0]
fixture=fixture.replace('#include "../deploy-usb/payload/altscreen_handler.c"','#include "'+str(handler)+'"')
main=r'''
static void *lacopy(const void *a,int64_t n,const void *original){
 (void)a;(void)n;const LayoutObject *o=original;LayoutObject *c=la(NULL,0,NULL);if(c)for(int i=0;i<o->count;i++)lappend(c,o->items[i]);return c;
}
static int32_t lacstring(void **arr,const char *text,size_t len){
 (void)len;if(!*arr)*arr=la(NULL,0,NULL);if(!*arr)return -1;LayoutObject *s=ls(NULL,text,0);if(!s)return -1;lappend(*arr,s);lr(s);return 0;
}
static int lcstring(const void *s,char *out,int64_t cap,uint32_t enc){(void)enc;const LayoutObject *o=s;if(o->kind!=1 || strlen(o->text)>=(size_t)cap)return 0;strcpy(out,o->text);return 1;}
static LayoutObject *native_root;
static void *native_info(void *s,void *p,int32_t *err){(void)s;(void)p;*err=0;native_root->refs++;return native_root;}
int main(void){
 s_CFArrayCreateMutableCopy=lacopy;s_CFArrayEnsureCreatedAndAppendCString=lacstring;s_CFStringGetCString=lcstring;
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
 assert(!strcmp(field(aux,"initialURL")->text,ALT_TOP_URL));
 assert(strstr(field(aux,"initialURL")->text,"showETA=yes"));
 const LayoutObject *areas=field(aux,"viewAreas"),*view=lat(areas,0),*safe=field(view,"safeArea");
 assert(field(safe,"originXPixels")->number==464 && field(safe,"widthPixels")->number==352);
 assert(field(safe,"originYPixels")->number==56 && field(safe,"heightPixels")->number==368);
 lr(info);lr(native_root);lr(stock);lr(primary);assert(layout_live==0);
 puts("PASS: actual info hook advertises auxiliary30fps, preserves native60fps/geometry and original dictionary, no object leaks");return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);source=p/'fixture.c';source.write_text(fixture+main);exe=p/'fixture'
 subprocess.run(['gcc','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(source),'-ldl','-pthread','-o',str(exe)],check=True)
 subprocess.run([exe],check=True,timeout=20)
