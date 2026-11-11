/* Original implementation of display geometry and the publically observable
 * maneuverLayout URI convention. No vendor renderer or reference-project code.
 * One display-wide profile; apps may render its request differently. */
#ifndef ALT_SAFE_X
#define ALT_SAFE_X 464
#define ALT_SAFE_Y 48
#define ALT_SAFE_W 352
#define ALT_SAFE_H 368
#endif
#define ALT_TOP_URL "maps:/car/instrumentcluster?showSpeedLimit=user&showCompass=user&showETA=yes&maneuverLayout=topaligned"
static int g_layout_advertised;
static int layout_advertised(void) {
    pthread_mutex_lock(&g_info_snapshot_lock);int ready=g_layout_advertised;
    pthread_mutex_unlock(&g_info_snapshot_lock);return ready;
}
static int layout_number(void *dict,const char *name,int64_t number) {
    void *key=s_cf(name);if(!key)return 0;
    s_CFDictionarySetInt64(dict,key,number);int32_t error=-1;
    int good=s_CFDictionaryGetInt64(dict,key,&error)==number && !error;
    s_CFRelease(key);return good;
}
static int layout_object(void *dict,const char *name,const void *value) {
    void *key=s_cf(name);if(!key)return 0;
    s_CFDictionarySetValue(dict,key,value);
    int good=s_CFDictionaryGetValue(dict,key)==value;s_CFRelease(key);return good;
}
static int64_t layout_dimension(const void *dict,const char *name) {
    void *key=s_cf(name);if(!key)return -1;
    int32_t error=-1;int64_t value=s_CFDictionaryGetInt64(dict,key,&error);
    s_CFRelease(key);return error?-1:value;
}
static int layout_rect_valid(int64_t w,int64_t h,int64_t x,int64_t y,int64_t sw,int64_t sh) {
    return w>0 && h>0 && w<=8192 && h<=8192 && x>=0 && y>=0 && sw>0 && sh>0 && x<=w && y<=h && sw<=w-x && sh<=h-y;
}
static void *layout_rectangle(int64_t w,int64_t h,int64_t x,int64_t y) {
    void *rect=s_CFDictionaryCreateMutable(NULL,0,s_kCFLDictKeyCallBacks,s_kCFLDictValCallBacks);
    if(rect && (!layout_number(rect,"widthPixels",w) || !layout_number(rect,"heightPixels",h) ||
                !layout_number(rect,"originXPixels",x) || !layout_number(rect,"originYPixels",y))) {
        s_CFRelease(rect);rect=NULL;
    }
    return rect;
}
static int layout_apply(void *display,int64_t w,int64_t h,int64_t x,int64_t y,int64_t sw,int64_t sh) {
    if(!layout_rect_valid(w,h,x,y,sw,sh))return 0;
    void *view=layout_rectangle(w,h,0,0),*safe=layout_rectangle(sw,sh,x,y);
    void *areas=s_CFArrayCreateMutable(NULL,0,s_kCFLArrayCallBacks);
    void *adjacent=s_CFArrayCreateMutable(NULL,0,s_kCFLArrayCallBacks);
    int good=0;
    if(view && safe && areas && adjacent && layout_object(view,"safeArea",safe)) {
        s_CFArrayAppendValue(areas,view);
        good=s_CFArrayGetCount(areas)==1 && s_CFArrayGetValueAtIndex(areas,0)==view &&
             layout_object(display,"viewAreas",areas) && layout_number(display,"initialViewArea",0) &&
             layout_object(display,"adjacentViewAreas",adjacent);
    }
    if(view)s_CFRelease(view);
    if(safe)s_CFRelease(safe);
    if(areas)s_CFRelease(areas);
    if(adjacent)s_CFRelease(adjacent);
    return good;
}
static void *layout_prepare_displays(const void *stock,const void *alt) {
    if(!stock || !alt || !s_CFDictionaryCreateMutable || !s_CFDictionaryCreateMutableCopy ||
       !s_CFArrayCreateMutable || !s_CFArrayAppendValue || !s_CFArrayGetCount || !s_CFArrayGetValueAtIndex ||
       !s_CFDictionaryGetInt64 || !s_CFDictionarySetInt64 || !s_CFDictionarySetValue ||
       !s_CFDictionaryGetValue || !s_CFRelease || !s_CFStringCreateWithCString)return NULL;
    int64_t count=s_CFArrayGetCount(stock);if(count<1 || count>16)return NULL;
    void *out=s_CFArrayCreateMutable(NULL,0,s_kCFLArrayCallBacks);if(!out)return NULL;
    for(int64_t i=0;i<=count;i++) {
        const void *original=i<count?s_CFArrayGetValueAtIndex(stock,i):alt;
        void *display=original?s_CFDictionaryCreateMutableCopy(NULL,0,original):NULL;
        if(!display)goto fail;
        int64_t w=layout_dimension(display,"widthPixels"),h=layout_dimension(display,"heightPixels");
        int valid=i<count?layout_apply(display,w,h,0,0,w,h):
                          (w==1280 && h==480 && layout_apply(display,w,h,ALT_SAFE_X,ALT_SAFE_Y,ALT_SAFE_W,ALT_SAFE_H));
        if(valid) {
            s_CFArrayAppendValue(out,display);
            valid=s_CFArrayGetCount(out)==i+1 && s_CFArrayGetValueAtIndex(out,i)==display;
        }
        s_CFRelease(display);if(!valid)goto fail;
    }
    return out;
fail:
    s_CFRelease(out);return NULL;
}
