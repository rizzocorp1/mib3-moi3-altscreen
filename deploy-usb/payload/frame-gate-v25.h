/* Root collector grants cluster-output ownership only after this process has
 * authenticated and validated an IDR. No UDP is emitted before the grant. */
#define HANDOFF_TOKEN "AUTHENTICATED_H264_V25\n"
#define HANDOFF_REQUEST_FILE "altscreen-v25-frame-ready"
#define HANDOFF_GRANT_FILE "altscreen-v25-encoder-ready"
#define HANDOFF_REVOKE_FILE "altscreen-v25-expired"
#include <stddef.h>
/* This exact AArch64 glibc 2.23 exports __fxstat(version=0), not the newer
 * fstat symbol. Its native wrapper directly issues syscall 80. */
#if defined(__aarch64__)
extern int __fxstat(int,int,struct stat *);
_Static_assert(sizeof(struct stat)==128 && offsetof(struct stat,st_mode)==16 && offsetof(struct stat,st_size)==48,"MU0841 stat ABI");
static int handoff_stat(int fd,struct stat *st){return __fxstat(0,fd,st);}
#else
static int handoff_stat(int fd,struct stat *st){return fstat(fd,st);}
#endif
static int g_handoff_started;
static int g_handoff_last_error;
static int handoff_file_valid(const char *path) {
    int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
    if(fd<0) {
        if(errno!=ENOENT && errno!=g_handoff_last_error)log_msg("HANDOFF","Grant/request open denied or failed: errno=%d",errno);
        g_handoff_last_error=errno;return 0;
    }
    struct stat st;char bytes[sizeof(HANDOFF_TOKEN)]={0};
    int good=handoff_stat(fd,&st)==0 && S_ISREG(st.st_mode) && st.st_size==(off_t)(sizeof(HANDOFF_TOKEN)-1);
    ssize_t n=good ? read(fd,bytes,sizeof(bytes)) : -1;
    if(!good && g_handoff_last_error!=-1)log_msg("HANDOFF","Grant/request metadata invalid or stat failed");
    if(!good)g_handoff_last_error=-1;
    close(fd);
    return good && n==(ssize_t)(sizeof(HANDOFF_TOKEN)-1) && !memcmp(bytes,HANDOFF_TOKEN,sizeof(HANDOFF_TOKEN)-1);
}
static int frame_gate_revoked_at(const char *directory) {
    char path[512];
    return snprintf(path,sizeof(path),"%s/" HANDOFF_REVOKE_FILE,directory)>=(int)sizeof(path) || handoff_file_valid(path);
}
static int frame_gate_at(const char *directory,int idr,int authenticated) {
    if(!authenticated)return 1;
    if(frame_gate_revoked_at(directory))return -1;
    char request[512],grant[512];
    if(snprintf(request,sizeof(request),"%s/" HANDOFF_REQUEST_FILE,directory)>=(int)sizeof(request) ||
       snprintf(grant,sizeof(grant),"%s/" HANDOFF_GRANT_FILE,directory)>=(int)sizeof(grant))return -1;
    /* A request must exist for each live receiver, even if a prior session's
     * grant is still held by the same monitor during a quick reconnect. */
    if(!handoff_file_valid(request)) {
        g_handoff_started=0;
        if(!idr)return 1;
        int fd=open(request,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(fd<0)return -1;
        ssize_t n=write(fd,HANDOFF_TOKEN,sizeof(HANDOFF_TOKEN)-1);int synced=fsync(fd);close(fd);
        if(n!=(ssize_t)(sizeof(HANDOFF_TOKEN)-1) || synced!=0)return -1;
        log_msg("HANDOFF","Authenticated IDR requested encoder ownership");
    }
    if(handoff_file_valid(grant)) {
        if(!g_handoff_started && !idr)return 1;
        if(!g_handoff_started)log_msg("HANDOFF","Encoder grant validated; authenticated output enabled");
        g_handoff_last_error=0;g_handoff_started=1;return 0;
    }
    g_handoff_started=0;return 1;
}
static void end_frame_gate_at(const char *directory) {
    char request[512];snprintf(request,sizeof(request),"%s/" HANDOFF_REQUEST_FILE,directory);
    if(handoff_file_valid(request))unlink(request);
    g_handoff_started=0;
}
