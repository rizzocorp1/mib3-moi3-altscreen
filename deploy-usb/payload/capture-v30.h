/* Private, bounded authenticated video capture for offline decoder diagnosis.
 * Format: MOI3VC1\n, followed by LE uint32 length, LE uint64 PTS, Annex-B AU.
 * No headers from pairing/control, key material, audio or absolute phone time.
 * Receiver thread only; disabled after one capture attempt per process. */
#define VIDEO_CAPTURE_LIMIT (2u*1024u*1024u)
static struct {int fd,attempted,frames;size_t bytes;uint64_t first_us;} g_video_capture={.fd=-1};
static const char *g_capture_directory="/data/lge/smartlink/carplay";
static int capture_write_all(int fd,const uint8_t *bytes,size_t size) {
    while(size){ssize_t n=write(fd,bytes,size);if(n<0 && errno==EINTR)continue;if(n<=0)return -1;bytes+=n;size-=(size_t)n;}return 0;
}
static void capture_finish(void) {
    if(g_video_capture.fd<0)return;
    int good=g_video_capture.frames>0 && fsync(g_video_capture.fd)==0;
    close(g_video_capture.fd);g_video_capture.fd=-1;
    if(good) {
        char ready[512];int n=snprintf(ready,sizeof(ready),"%s/map-v30-video.ready",g_capture_directory);
        if(n>0 && n<(int)sizeof(ready)) {
            int fd=open(ready,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
            if(fd>=0){good=capture_write_all(fd,(const uint8_t*)"MOI3VC1\n",8)==0 && fsync(fd)==0;close(fd);}else good=0;
        } else good=0;
    }
    log_msg("PRIVATE_CAPTURE","finished=%d frames=%d bytes=%zu; private map imagery, no keys/audio; never publish",good,g_video_capture.frames,g_video_capture.bytes);
}
static void capture_video(const uint8_t *annexb,size_t size,uint64_t pts,uint64_t timestamp_us,int idr) {
    if(!annexb || !size)return;
    if(g_video_capture.fd<0) {
        if(g_video_capture.attempted || !idr || size<1024)return;
        g_video_capture.attempted=1;
        char path[512];int n=snprintf(path,sizeof(path),"%s/map-v30-video.bin",g_capture_directory);
        if(n<=0 || n>=(int)sizeof(path))return;
        g_video_capture.fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(g_video_capture.fd<0){log_msg("PRIVATE_CAPTURE","not started errno=%d; video output retained",errno);return;}
        if(capture_write_all(g_video_capture.fd,(const uint8_t*)"MOI3VC1\n",8)<0){close(g_video_capture.fd);g_video_capture.fd=-1;return;}
        g_video_capture.bytes=8;g_video_capture.first_us=timestamp_us;
    }
    if(size>VIDEO_CAPTURE_LIMIT-12 || g_video_capture.bytes>VIDEO_CAPTURE_LIMIT-size-12 ||
       g_video_capture.frames>=90 || (g_video_capture.frames && timestamp_us>=g_video_capture.first_us && timestamp_us-g_video_capture.first_us>=3000000)) {
        capture_finish();return;
    }
    uint8_t header[12];for(unsigned i=0;i<4;i++)header[i]=(uint8_t)(size>>(i*8));
    for(unsigned i=0;i<8;i++)header[4+i]=(uint8_t)(pts>>(i*8));
    if(capture_write_all(g_video_capture.fd,header,sizeof(header))<0 || capture_write_all(g_video_capture.fd,annexb,size)<0) {
        close(g_video_capture.fd);g_video_capture.fd=-1;log_msg("PRIVATE_CAPTURE","write failed; capture disabled; video output retained");return;
    }
    g_video_capture.frames++;g_video_capture.bytes+=12+size;
}
