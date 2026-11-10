#define _GNU_SOURCE
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
#include <assert.h>
static void reset_private_fixture(const char *directory) {
    char path[512];snprintf(path,sizeof(path),"%s/map-v25-video.bin",directory);unlink(path);
    snprintf(path,sizeof(path),"%s/map-v25-video.ready",directory);unlink(path);
    g_video_capture.fd=-1;g_video_capture.attempted=0;g_video_capture.frames=0;g_video_capture.bytes=0;
}
int main(void) {
    char directory[]="/tmp/private-video-XXXXXX";assert(mkdtemp(directory));g_capture_directory=directory;
    uint8_t bytes[1024];for(size_t i=0;i<sizeof(bytes);i++)bytes[i]=(uint8_t)i;
    capture_video(bytes,sizeof(bytes),90000,1000000,0);assert(!g_video_capture.attempted);
    capture_video(bytes,153,90000,1000000,1);assert(!g_video_capture.attempted);
    capture_video(bytes,sizeof(bytes),90000,1000000,1);assert(g_video_capture.frames==1 && g_video_capture.fd>=0);
    capture_video(bytes,sizeof(bytes),93000,1033333,0);assert(g_video_capture.frames==2);
    capture_video(bytes,sizeof(bytes),360000,4000000,0);assert(g_video_capture.fd==-1);
    char path[512],ready[512];snprintf(path,sizeof(path),"%s/map-v25-video.bin",directory);snprintf(ready,sizeof(ready),"%s/map-v25-video.ready",directory);
    FILE *f=fopen(path,"rb");assert(f);uint8_t all[4096];size_t n=fread(all,1,sizeof(all),f);assert(feof(f));fclose(f);
    assert(n==8+2*(12+sizeof(bytes))&&!memcmp(all,"MOI3VC1\n",8));
    assert(all[8]==0 && all[9]==4 && all[10]==0 && all[11]==0);
    uint64_t pts=0;for(unsigned i=0;i<8;i++)pts|=(uint64_t)all[12+i]<<(8*i);
    assert(pts==90000&&!memcmp(all+20,bytes,sizeof(bytes))&&access(ready,F_OK)==0);
    capture_video(bytes,sizeof(bytes),390000,4333333,1);assert(g_video_capture.frames==2);
    struct stat st;assert(stat(path,&st)==0&&(st.st_mode&0777)==0600);
    reset_private_fixture(directory);
    for(int i=0;i<91;i++)capture_video(bytes,sizeof(bytes),90000,1000000,i==0);
    assert(g_video_capture.frames==90&&g_video_capture.fd==-1);
    reset_private_fixture(directory);
    uint8_t *large=calloc(1,262144);assert(large);
    for(int i=0;i<10;i++)capture_video(large,262144,90000,1000000,i==0);
    assert(g_video_capture.fd==-1&&g_video_capture.frames==7&&stat(path,&st)==0&&st.st_size<=VIDEO_CAPTURE_LIMIT);free(large);
    reset_private_fixture(directory);
    f=fopen(path,"wb");assert(f);fputs("preserve",f);fclose(f);
    capture_video(bytes,sizeof(bytes),90000,1000000,1);assert(g_video_capture.fd==-1&&stat(path,&st)==0&&st.st_size==8&&access(ready,F_OK)!=0);
    reset_private_fixture(directory);assert(symlink("unrelated",path)==0);
    capture_video(bytes,sizeof(bytes),90000,1000000,1);assert(g_video_capture.fd==-1&&access(ready,F_OK)!=0);
    reset_private_fixture(directory);assert(rmdir(directory)==0);
    puts("PASS: exact bounded private capture format, 0600, IDR start, 3-second/90-frame/2MiB bounds, one attempt, existing/symlink refusal; no key/audio inputs");
}
