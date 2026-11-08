#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
static void log_msg(const char *tag,const char *fmt,...){(void)tag;(void)fmt;}
#include "../deploy-usb/payload/frame-gate-v9.h"
#include "../deploy-usb/payload/deferred-video-v9.h"
static uint8_t seen[1024];static size_t count;
static int emit(const uint8_t *data,size_t n,uint64_t timestamp) {
    assert(n>0 && timestamp==data[0]);seen[count++]=data[0];return 0;
}
static void grant(const char *directory) {
    char path[512];snprintf(path,sizeof(path),"%s/altscreen-v9-encoder-ready",directory);
    int fd=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);assert(fd>=0);
    assert(write(fd,HANDOFF_TOKEN,sizeof(HANDOFF_TOKEN)-1)==sizeof(HANDOFF_TOKEN)-1);close(fd);
}
static void revoke(const char *directory) {
    char path[512];snprintf(path,sizeof(path),"%s/altscreen-v9-encoder-ready",directory);unlink(path);
    snprintf(path,sizeof(path),"%s/altscreen-v9-frame-ready",directory);unlink(path);
    g_handoff_started=0;deferred_video_clear();
}
int main(void) {
    char directory[]="/tmp/deferred-video-XXXXXX";assert(mkdtemp(directory));
    uint8_t idr[]={1,0x65},p[]={2,0x41};
    assert(deferred_video_submit(directory,idr,2,1,1,1,emit)==0);
    assert(deferred_video_submit(directory,p,2,2,0,1,emit)==0);
    assert(!count && g_deferred_count==2);grant(directory);
    /* Grant alone can release the opening GOP: no second keyframe/input needed. */
    assert(deferred_video_flush(directory,emit)==1 && count==2 && seen[0]==1 && seen[1]==2);
    p[0]=3;assert(deferred_video_submit(directory,p,2,3,0,1,emit)==0 && count==3);
    revoke(directory);count=0;
    assert(deferred_video_submit(directory,idr,2,1,1,0,emit)==-1 && !g_deferred_count);
    assert(deferred_video_submit(directory,p,2,3,0,1,emit)==0 && !g_deferred_count);
    assert(deferred_video_submit(directory,idr,2,1,1,1,emit)==0);
    idr[0]=4;assert(deferred_video_submit(directory,idr,2,4,1,1,emit)==0 && g_deferred_count==1);
    grant(directory);assert(deferred_video_flush(directory,emit)==1 && count==1 && seen[0]==4);
    revoke(directory);count=0;
    assert(deferred_video_submit(directory,idr,2,4,1,1,emit)==0);
    for(unsigned i=1;i<=DEFERRED_MAX_FRAMES;i++)assert(deferred_video_submit(directory,p,2,3,0,1,emit)==0);
    assert(!g_deferred_count);grant(directory);
    assert(deferred_video_submit(directory,p,2,3,0,1,emit)==0 && !count);
    assert(deferred_video_submit(directory,idr,2,4,1,1,emit)==0 && count==1);
    revoke(directory);count=0;
    assert(deferred_video_submit(directory,idr,2,4,1,1,emit)==0);
    uint8_t *large=malloc(DEFERRED_MAX_BYTES);assert(large);memset(large,3,DEFERRED_MAX_BYTES);
    assert(deferred_video_submit(directory,large,DEFERRED_MAX_BYTES,3,0,1,emit)==0 && !g_deferred_count);
    free(large);revoke(directory);
    assert(deferred_video_submit(directory,idr,2,4,1,1,emit)==0);
    deferred_video_clear();grant(directory);assert(!deferred_video_flush(directory,emit) && !count);
    revoke(directory);rmdir(directory);
    puts("PASS: opening authenticated GOP retained until grant, ordered release without second IDR, superseding IDR, unauthenticated/P-only rejection, bounded overflow and teardown discard");
}
