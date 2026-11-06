/* SPDX-License-Identifier: MIT
 * Narrow file copy/atomic replacement with complete Linux xattr preservation.
 * Not a partition backup. Callers must restrict paths and verify content hashes.
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char *name; void *value; size_t size; } Attr;
typedef struct { struct stat st; Attr *attrs; size_t count; } Meta;
static void fail(const char *what) { perror(what); exit(1); }
static int safe_open(const char *path, int directory) {
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|(directory?O_DIRECTORY:0));
    if(fd<0)fail(path);
    struct stat st;if(fstat(fd,&st))fail("fstat");
    if(!S_ISREG(st.st_mode)&&!(directory&&S_ISDIR(st.st_mode))) {
        fprintf(stderr,"Not a regular file/directory: %s\n",path);exit(1);
    }
    if(S_ISREG(st.st_mode)&&st.st_nlink!=1) {fprintf(stderr,"Hard-linked input refused\n");exit(1);}
    return fd;
}
static int attr_compare(const void *a,const void *b) {return strcmp(((const Attr*)a)->name,((const Attr*)b)->name);}
static Meta capture(int fd) {
    Meta m={0};if(fstat(fd,&m.st))fail("fstat metadata");
    ssize_t length=flistxattr(fd,NULL,0);if(length<0||length>65536)fail("list xattrs");
    char *names=malloc((size_t)length+1);if(!names)fail("allocate xattrs");
    if(flistxattr(fd,names,(size_t)length)!=length)fail("changing xattr list");
    for(size_t p=0;p<(size_t)length;) {
        size_t n=strnlen(names+p,(size_t)length-p);if(n==(size_t)length-p){errno=EINVAL;fail("xattr name");}
        Attr *next=realloc(m.attrs,(m.count+1)*sizeof(Attr));if(!next)fail("allocate attrs");m.attrs=next;
        Attr *a=&m.attrs[m.count++];a->name=strdup(names+p);if(!a->name)fail("allocate name");
        ssize_t size=fgetxattr(fd,a->name,NULL,0);if(size<0||size>65536)fail("get xattr size");
        a->size=(size_t)size;a->value=malloc(a->size+1);if(!a->value)fail("allocate value");
        if(fgetxattr(fd,a->name,a->value,a->size)!=size)fail("changing xattr value");p+=n+1;
    }
    free(names);if(m.count)qsort(m.attrs,m.count,sizeof(Attr),attr_compare);return m;
}
static void release(Meta *m) {for(size_t i=0;i<m->count;i++){free(m->attrs[i].name);free(m->attrs[i].value);}free(m->attrs);}
static int equal(const Meta *a,const Meta *b) {
    if((a->st.st_mode&07777)!=(b->st.st_mode&07777)||a->st.st_uid!=b->st.st_uid||a->st.st_gid!=b->st.st_gid||a->count!=b->count)return 0;
    for(size_t i=0;i<a->count;i++)if(strcmp(a->attrs[i].name,b->attrs[i].name)||a->attrs[i].size!=b->attrs[i].size||memcmp(a->attrs[i].value,b->attrs[i].value,a->attrs[i].size))return 0;
    return 1;
}
static int apply(int fd,const Meta *m) {
    if(fchown(fd,m->st.st_uid,m->st.st_gid)||fchmod(fd,m->st.st_mode&07777))return -1;
    Meta current=capture(fd);
    for(size_t i=0;i<current.count;i++) {
        int keep=0;for(size_t j=0;j<m->count;j++)if(!strcmp(current.attrs[i].name,m->attrs[j].name))keep=1;
        if(!keep&&fremovexattr(fd,current.attrs[i].name)){release(&current);return -1;}
    }
    release(&current);
    for(size_t i=0;i<m->count;i++)if(fsetxattr(fd,m->attrs[i].name,m->attrs[i].value,m->attrs[i].size,0))return -1;
    struct timespec times[2]={m->st.st_atim,m->st.st_mtim};
    if(futimens(fd,times))return -1;
    Meta actual=capture(fd);int ok=equal(m,&actual);release(&actual);
    if(!ok){errno=EIO;return -1;}return 0;
}
static int parent_fd(const char *path) {
    char p[PATH_MAX];if(strlen(path)>=sizeof(p)){errno=ENAMETOOLONG;fail("parent path");}strcpy(p,path);
    char *slash=strrchr(p,'/');if(!slash)strcpy(p,".");else if(slash==p)slash[1]=0;else *slash=0;
    int fd=open(p,O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOFOLLOW);if(fd<0)fail("open parent");return fd;
}
int main(int argc,char **argv) {
    if(argc<4){fprintf(stderr,"usage: file-guard clone|replace|same-meta|meta SOURCE DEST [REFERENCE]\n");return 2;}
    const char *action=argv[1],*source=argv[2],*dest=argv[3];
    int is_meta=!strcmp(action,"meta"),is_same=!strcmp(action,"same-meta"),is_replace=!strcmp(action,"replace"),is_create=!strcmp(action,"create");
    if(!is_meta&&!is_same&&!is_replace&&!is_create&&strcmp(action,"clone"))return 2;
    if((is_create&&argc!=5)||(!is_create&&argc!=4))return 2;
    int src=safe_open(source,is_meta||is_same),ref=-1;
    if(is_meta||is_same) {
        int dst=safe_open(dest,1);Meta a=capture(src);int result;
        if(is_same){Meta b=capture(dst);result=equal(&a,&b)?0:1;release(&b);}
        else {result=apply(dst,&a);if(!result&&fsync(dst))result=-1;}
        release(&a);close(src);close(dst);if(result<0)fail("apply metadata");return result;
    }
    if(is_replace)ref=safe_open(dest,0);else if(is_create)ref=safe_open(argv[4],0);else ref=dup(src);
    if(ref<0)fail("reference");Meta metadata=capture(ref);struct stat source_before;if(fstat(src,&source_before))fail("source stat");
    char temporary[PATH_MAX];const char *output=dest;
    if(is_replace){int n=snprintf(temporary,sizeof(temporary),"%s.moi3-%ld",dest,(long)getpid());if(n<0||n>=(int)sizeof(temporary))return 1;output=temporary;}
    int parent=parent_fd(dest),out=open(output,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if(out<0)fail("exclusive output");int ok=1;char buffer[65536];ssize_t n;
    while((n=read(src,buffer,sizeof(buffer)))>0){size_t written=0;while(written<(size_t)n){ssize_t w=write(out,buffer+written,(size_t)n-written);if(w<=0){ok=0;break;}written+=(size_t)w;}if(!ok)break;}
    if(n<0)ok=0;
    struct stat source_after;if(fstat(src,&source_after)||source_before.st_size!=source_after.st_size||source_before.st_mtim.tv_sec!=source_after.st_mtim.tv_sec||source_before.st_mtim.tv_nsec!=source_after.st_mtim.tv_nsec||source_before.st_ctim.tv_sec!=source_after.st_ctim.tv_sec||source_before.st_ctim.tv_nsec!=source_after.st_ctim.tv_nsec){errno=EBUSY;ok=0;}
    if(ok&&apply(out,&metadata))ok=0;if(ok&&fsync(out))ok=0;
    if(close(out))ok=0;
#ifdef FILE_GUARD_TEST
    if(getenv("FILE_GUARD_FAIL_BEFORE_COMMIT")){errno=EIO;ok=0;}
#endif
    if(ok&&is_replace) {
        struct stat now;if(lstat(dest,&now)||now.st_dev!=metadata.st.st_dev||now.st_ino!=metadata.st.st_ino||now.st_ctim.tv_sec!=metadata.st.st_ctim.tv_sec||now.st_ctim.tv_nsec!=metadata.st.st_ctim.tv_nsec){errno=EBUSY;ok=0;}
        if(ok&&rename(output,dest))ok=0;
    }
    int saved=errno;if(!ok)unlink(output);
    /* Failure here can mean the rename committed but directory sync failed.
     * Callers must inspect hashes and must not arm activation on any failure. */
    if(ok&&fsync(parent)){saved=errno;ok=0;}
    release(&metadata);close(ref);close(src);close(parent);errno=saved;
    if(!ok)fail("copy/replace");return 0;
}
