#ifndef ALT_LAYOUT_LOCAL
#define _GNU_SOURCE
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
#endif
#include <assert.h>
typedef struct LayoutObject LayoutObject;
struct LayoutObject {
    int kind,refs,count;int64_t number;char text[256];
    struct {char name[48];LayoutObject *value;} fields[24];
    LayoutObject *items[16];
};
static int layout_live,layout_allocations,layout_failure;
static LayoutObject *lo(int kind) {
    if(++layout_allocations==layout_failure)return NULL;
    LayoutObject *o=calloc(1,sizeof(*o));assert(o);o->kind=kind;o->refs=1;layout_live++;return o;
}
static void lr(const void *p) {
    LayoutObject *o=(LayoutObject*)p;assert(o && o->refs>0);if(--o->refs)return;
    for(int i=0;i<o->count;i++)lr(o->kind==2?o->fields[i].value:o->items[i]);
    free(o);layout_live--;
}
static void *ls(const void *a,const char *s,uint32_t e) {
    (void)a;(void)e;LayoutObject *o=lo(1);if(o){assert(strlen(s)<sizeof(o->text));strcpy(o->text,s);}return o;
}
static void *ld(const void *a,int64_t n,const void *k,const void *v){(void)a;(void)n;(void)k;(void)v;return lo(2);}
static void *la(const void *a,int64_t n,const void *c){(void)a;(void)n;(void)c;return lo(3);}
static const void *lg(const void *p,const void *key) {
    const LayoutObject *d=p,*k=key;assert(d->kind==2 && k->kind==1);
    for(int i=0;i<d->count;i++)if(!strcmp(d->fields[i].name,k->text))return d->fields[i].value;
    return NULL;
}
static void lset(void *p,const void *key,const void *value) {
    LayoutObject *d=p,*k=(LayoutObject*)key,*v=(LayoutObject*)value;assert(d->kind==2 && k->kind==1);
    int i;for(i=0;i<d->count;i++)if(!strcmp(d->fields[i].name,k->text))break;
    v->refs++;if(i<d->count)lr(d->fields[i].value);
    else {assert(d->count<24);d->count++;strcpy(d->fields[i].name,k->text);}
    d->fields[i].value=v;
}
static void ln(void *d,const void *key,int64_t value) {
    LayoutObject *v=lo(4);if(v){v->number=value;lset(d,key,v);lr(v);}
}
static int64_t lgetn(const void *d,const void *key,int32_t *error) {
    const LayoutObject *v=lg(d,key);if(error)*error=v&&v->kind==4?0:-1;return v?v->number:0;
}
static void lappend(void *p,const void *v){LayoutObject *a=p;assert(a->kind==3&&a->count<16);a->items[a->count++]=(LayoutObject*)v;((LayoutObject*)v)->refs++;}
static int64_t lcount(const void *p){const LayoutObject *a=p;assert(a->kind==3);return a->count;}
static const void *lat(const void *p,int64_t i){const LayoutObject *a=p;assert(a->kind==3&&i>=0&&i<a->count);return a->items[i];}
static void *lcopy(const void *a,int64_t n,const void *original) {
    (void)a;(void)n;const LayoutObject *old=original;LayoutObject *copy=lo(2);
    if(copy){*copy=*old;copy->refs=1;for(int i=0;i<copy->count;i++)copy->fields[i].value->refs++;}return copy;
}
static const LayoutObject *field(const void *d,const char *name) {
    LayoutObject key={.kind=1};strcpy(key.text,name);return lg(d,&key);
}
int main(void) {
    s_CFStringCreateWithCString=ls;s_CFDictionaryCreateMutable=ld;s_CFDictionaryCreateMutableCopy=lcopy;
    s_CFDictionarySetValue=lset;s_CFDictionaryGetValue=lg;s_CFDictionarySetInt64=ln;s_CFDictionaryGetInt64=lgetn;
    s_CFArrayCreateMutable=la;s_CFArrayAppendValue=lappend;s_CFArrayGetCount=lcount;s_CFArrayGetValueAtIndex=lat;s_CFRelease=lr;
    LayoutObject *stock=la(NULL,0,NULL),*primary=ld(NULL,0,NULL,NULL),*alt=ld(NULL,0,NULL,NULL);
    assert(layout_number(primary,"widthPixels",784)&&layout_number(primary,"heightPixels",468)&&layout_number(primary,"primaryInputDevice",7));
    assert(layout_number(alt,"widthPixels",1280)&&layout_number(alt,"heightPixels",480));lappend(stock,primary);
    int baseline=layout_live;layout_allocations=0;void *prepared=layout_prepare_displays(stock,alt);
    assert(prepared&&lcount(prepared)==2);int allocations=layout_allocations;
    const LayoutObject *p=lat(prepared,0),*c=lat(prepared,1);assert(p!=primary&&c!=alt);
    assert(field(p,"widthPixels")->number==784&&field(p,"heightPixels")->number==468&&field(p,"primaryInputDevice")->number==7);
    const LayoutObject *areas=field(c,"viewAreas"),*view=lat(areas,0),*safe=field(view,"safeArea");
    assert(field(view,"widthPixels")->number==1280&&field(view,"heightPixels")->number==480);
    assert(field(safe,"originXPixels")->number==480&&field(safe,"originYPixels")->number==64&&field(safe,"widthPixels")->number==320&&field(safe,"heightPixels")->number==320);
    assert(field(c,"initialViewArea")->number==0&&lcount(field(c,"adjacentViewAreas"))==0);
    assert(!field(view,"viewAreaTransitionControl")&&!field(view,"drawUIOutsideSafeArea"));
    assert(!field(primary,"viewAreas")&&!field(alt,"viewAreas"));lr(prepared);assert(layout_live==baseline);
    for(int fail=1;fail<=allocations;fail++) {
        layout_allocations=0;layout_failure=fail;prepared=layout_prepare_displays(stock,alt);
        if(prepared)lr(prepared);
        assert(layout_live==baseline&&!field(primary,"viewAreas")&&!field(alt,"viewAreas"));
    }
    layout_failure=0;assert(!layout_rect_valid(1280,480,-1,0,960,320)&&!layout_rect_valid(1280,480,0,400,960,320));
    assert(!layout_rect_valid(1280,480,INT64_MAX,0,1,1));
    assert(strstr(ALT_TOP_URL,"maneuverLayout=topaligned")&&!strstr(ALT_TOP_URL,"applemaps"));
    g_layout_advertised=0;assert(!layout_advertised());g_layout_advertised=1;assert(layout_advertised());
    lr(stock);lr(primary);lr(alt);assert(layout_live==0);
    puts("PASS: complete primary/auxiliary viewAreas, original dimensions and input preserved, top-safe geometry, every allocation failure bounded without stock mutation/leaks, invalid geometry refused");
}
