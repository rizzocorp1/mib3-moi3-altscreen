/* Bounded private capture recovery. No external head/stat/cp/hash commands.
 * Byte-for-byte readback verifies USB before removing the unit-side source.
 * Refuse unknown/different existing output; preserve evidence on failure. */
#include <stdint.h>
#define PRIVATE_VIDEO_MAX (2u*1024u*1024u)
static void video_read_exact(int fd,unsigned char *out,size_t size) {
    while(size) {
        ssize_t n=read(fd,out,size);
        if(n<0 && errno==EINTR)continue;
        if(n<=0){errno=EIO;fail("private video read");}
        out+=n;size-=(size_t)n;
    }
}
static uint64_t video_le(const unsigned char *bytes,unsigned count) {
    uint64_t n=0;for(unsigned i=0;i<count;i++)n|=(uint64_t)bytes[i]<<(8*i);return n;
}
static unsigned video_validate(const unsigned char *bytes,size_t size) {
    if(size<25 || size>PRIVATE_VIDEO_MAX || memcmp(bytes,"MOI3VC1\n",8)){errno=EINVAL;fail("private video format");}
    size_t pos=8;unsigned frames=0;uint64_t first=0,last=0;
    while(pos<size) {
        if(size-pos<12){errno=EINVAL;fail("private video record header");}
        size_t length=(size_t)video_le(bytes+pos,4);uint64_t pts=video_le(bytes+pos+4,8);pos+=12;
        if(length<5 || length>size-pos || ++frames>90 || (frames>1 && (pts<last || pts-first>270000))) {
            errno=EINVAL;fail("private video record bounds");
        }
        if(memcmp(bytes+pos,"\0\0\0\1",4) && memcmp(bytes+pos,"\0\0\1",3)){errno=EINVAL;fail("private video Annex-B prefix");}
        if(frames==1)first=pts;
        last=pts;pos+=length;
    }
    return frames;
}
static int video_same_path(const char *path,const struct stat *expected) {
    struct stat now;
    return lstat(path,&now)==0 && S_ISREG(now.st_mode) && now.st_nlink==1 && now.st_ino==expected->st_ino &&
           now.st_dev==expected->st_dev && now.st_size==expected->st_size && now.st_mtim.tv_sec==expected->st_mtim.tv_sec && now.st_mtim.tv_nsec==expected->st_mtim.tv_nsec;
}
static int video_export(const char *source,const char *ready,const char *destination) {
    int marker=safe_open(ready,0),input=safe_open(source,0);struct stat mi,si;
    if(fstat(marker,&mi)||fstat(input,&si))fail("private video metadata");
    if(mi.st_size!=8 || si.st_size<25 || si.st_size>PRIVATE_VIDEO_MAX){errno=EINVAL;fail("private video size");}
    unsigned char magic[8];video_read_exact(marker,magic,8);
    if(memcmp(magic,"MOI3VC1\n",8)){errno=EINVAL;fail("private video completion token");}
    size_t size=(size_t)si.st_size;unsigned char *bytes=malloc(size);
    if(!bytes)fail("private video buffer");
    video_read_exact(input,bytes,size);unsigned frames=video_validate(bytes,size);
    int output=open(destination,O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
    if(output<0) {
        if(errno!=EEXIST)fail("private video destination create");
        output=safe_open(destination,0);
    } else {
        size_t pos=0;
        while(pos<size) {
            ssize_t n=write(output,bytes+pos,size-pos);
            if(n<0 && errno==EINTR)continue;
            if(n<=0)fail("private video destination write");
            pos+=(size_t)n;
        }
        if(fsync(output)||lseek(output,0,SEEK_SET)<0)fail("private video destination sync");
    }
    struct stat dest;
    if(fstat(output,&dest)||dest.st_size!=(off_t)size || !S_ISREG(dest.st_mode)||dest.st_nlink!=1){errno=EINVAL;fail("private video destination metadata");}
    if(dest.st_ino==si.st_ino && dest.st_dev==si.st_dev){errno=EINVAL;fail("private video destination aliases source");}
    unsigned char block[16384];size_t pos=0;
    while(pos<size) {
        size_t n=size-pos;if(n>sizeof(block))n=sizeof(block);
        video_read_exact(output,block,n);
        if(memcmp(block,bytes+pos,n)){errno=EIO;fail("private video readback differs; source preserved");}
        pos+=n;
    }
    if(!video_same_path(source,&si)||!video_same_path(ready,&mi)||!video_same_path(destination,&dest)){errno=EBUSY;fail("private video path changed; source preserved");}
    if(unlink(source)||unlink(ready))fail("private video source removal after verified export");
    close(output);close(input);close(marker);free(bytes);
    printf("PRIVATE CAPTURE EXPORTED: frames=%u bytes=%zu; USB byte-readback verified; unit source removed; never publish\n",frames,size);
    return 0;
}
