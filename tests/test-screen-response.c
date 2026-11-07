#define _GNU_SOURCE
#include <assert.h>
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
typedef struct Object {int kind,count;const char *keys[8];int64_t numbers[8];struct Object *items[8],*streams;} Object;
static Object pool[32];static int used,fail_array,fail_dict,fail_number;
static void *str(const void *a,const char *s,uint32_t e){(void)a;(void)e;return (void *)s;}
static void release(const void *a){(void)a;}
static void *new_array(const void *a,int64_t n,const void *c){(void)a;(void)n;(void)c;if(fail_array)return NULL;Object *o=&pool[used++];memset(o,0,sizeof(*o));o->kind=1;return o;}
static void *new_dict(const void *a,int64_t n,const void *k,const void *v){(void)a;(void)n;(void)k;(void)v;if(fail_dict)return NULL;Object *o=&pool[used++];memset(o,0,sizeof(*o));o->kind=2;return o;}
static int64_t count(const void *a){return ((const Object *)a)->count;}
static const void *at(const void *a,int64_t n){return ((const Object *)a)->items[n];}
static void append(void *a,const void *v){Object *o=a;assert(o->count<8);o->items[o->count++]=(Object *)v;}
static const void *get(const void *d,const void *k){assert(!strcmp(k,"streams"));return ((const Object *)d)->streams;}
static void set(void *d,const void *k,const void *v){assert(!strcmp(k,"streams"));((Object *)d)->streams=(Object *)v;}
static void set_number(void *d,const void *k,int64_t n){Object *o=d;if(fail_number)return;assert(o->count<8);o->keys[o->count]=k;o->numbers[o->count++]=n;}
static int64_t get_number(const void *d,const void *k,int32_t *error){const Object *o=d;for(int i=0;i<o->count;i++)if(!strcmp(o->keys[i],k)){if(error)*error=0;return o->numbers[i];}if(error)*error=-1;return 0;}
int main(void){
    s_CFStringCreateWithCString=str;s_CFRelease=release;s_CFDictionaryGetValue=get;s_CFDictionarySetValue=set;
    s_CFDictionaryCreateMutable=new_dict;s_CFArrayCreateMutable=new_array;s_CFArrayGetCount=count;
    s_CFArrayGetValueAtIndex=at;s_CFArrayAppendValue=append;s_CFDictionarySetInt64=set_number;s_CFDictionaryGetInt64=get_number;
    Object request={.kind=2,.count=3,.keys={"type","streamConnectionID","streamID"},.numbers={111,1234,9999}};
    Object primary={.kind=2,.count=2,.keys={"type","dataPort"},.numbers={110,54321}};
    Object old_alt={.kind=2,.count=2,.keys={"type","dataPort"},.numbers={111,12345}};
    Object old_array={.kind=1,.count=3,.items={&primary,&old_alt,&old_alt}};
    Object response={.kind=2,.streams=&old_array};
    assert(append_alt_setup_response(&response,&request,45678)==1);
    assert(response.streams!=&old_array && response.streams->count==2 && response.streams->items[0]==&primary);
    Object *aux=response.streams->items[1];assert(aux->count==2 && stream_type(aux)==111);
    int32_t error;assert(get_number(aux,"dataPort",&error)==45678 && !error);
    get_number(aux,"streamID",&error);assert(error);get_number(aux,"streamConnectionID",&error);assert(error);
    assert(old_array.count==3 && request.count==3 && primary.numbers[1]==54321);
    for(int failure=0;failure<3;failure++){
        response.streams=&old_array;fail_array=failure==0;fail_dict=failure==1;fail_number=failure==2;
        assert(append_alt_setup_response(&response,&request,45678)==0 && response.streams==&old_array);
    }
    fail_array=fail_dict=fail_number=0;
    response.streams=NULL;assert(append_alt_setup_response(&response,&request,45678)==1 && response.streams->count==1);
    assert(!append_alt_setup_response(&response,&request,0) && !append_alt_setup_response(&response,&request,65536));
    puts("PASS: minimal type/dataPort reply, no request echo or invented streamID, duplicate auxiliary replacement, primary preservation and allocation failure");
}
