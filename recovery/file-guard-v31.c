/* Original managed-runtime extensions. Existing metadata operations unchanged. */
#define main stock_file_guard_main
#include "file-guard.c"
#undef main
#include <stdint.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <netinet/in.h>
#define LEASE_TOKEN "AUTHENTICATED_H264_V31\n"
static int runtime_pulse(const char *dest,const char *reference){
 if(!strstr(dest,"/run/moi3-altscreen-map-v31/lease") || strlen(dest)<6 || strcmp(dest+strlen(dest)-6,"/lease")){errno=EINVAL;return 1;}
 int ref=safe_open(reference,0);Meta metadata=capture(ref);close(ref);
 struct timespec now;if(clock_gettime(CLOCK_MONOTONIC,&now)!=0 || now.tv_sec<0){release(&metadata);return 1;}
 uint64_t ms=(uint64_t)now.tv_sec*1000+(uint64_t)now.tv_nsec/1000000;
 char body[96];int n=snprintf(body,sizeof(body),LEASE_TOKEN "%llu\n",(unsigned long long)(ms+12000));
 if(n<1 || n>=(int)sizeof(body)){release(&metadata);return 1;}
 struct stat prior;if(lstat(dest,&prior)==0){int existing=safe_open(dest,0);char prefix[sizeof(LEASE_TOKEN)];ssize_t count=read(existing,prefix,sizeof(prefix)-1);close(existing);if(count!=(ssize_t)(sizeof(prefix)-1) || memcmp(prefix,LEASE_TOKEN,sizeof(prefix)-1)){release(&metadata);errno=EINVAL;return 1;}}
 else if(errno!=ENOENT){release(&metadata);return 1;}
 char temp[PATH_MAX];if(snprintf(temp,sizeof(temp),"%s.pulse-%ld",dest,(long)getpid())>=(int)sizeof(temp)){release(&metadata);return 1;}
 int parent=parent_fd(dest);int fd=open(temp,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(fd<0){release(&metadata);close(parent);return 1;}
 int ok=write(fd,body,(size_t)n)==n && apply(fd,&metadata)==0 && fsync(fd)==0;
 int saved=errno;close(fd);
 if(ok && rename(temp,dest)) {saved=errno;ok=0;}
 if(ok && fsync(parent)){saved=errno;ok=0;}
 if(!ok){unlink(temp);}close(parent);release(&metadata);errno=saved;return ok?0:1;
}
static int runtime_notify(void){
 const char *path=getenv("NOTIFY_SOCKET");if(!path || !path[0] || strlen(path)>=sizeof(((struct sockaddr_un*)0)->sun_path)){errno=EINVAL;return 1;}
 struct sockaddr_un address={.sun_family=AF_UNIX};size_t size=strlen(path);memcpy(address.sun_path,path,size+1);
 if(path[0]=='@')address.sun_path[0]=0;else if(path[0]!='/'){errno=EINVAL;return 1;}else size++;
 int fd=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC,0);if(fd<0)return 1;
 char body[]="WATCHDOG=1";struct iovec iov={body,sizeof(body)-1};char control[CMSG_SPACE(sizeof(struct ucred))];memset(control,0,sizeof(control));
 struct msghdr msg={0};msg.msg_name=&address;msg.msg_namelen=(socklen_t)(offsetof(struct sockaddr_un,sun_path)+size);msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control;msg.msg_controllen=sizeof(control);
 struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_CREDENTIALS;c->cmsg_len=CMSG_LEN(sizeof(struct ucred));
 /* Attribute to the still-live controller parent, not a short-lived child.
  * This operation requires the root service's existing CAP_SYS_ADMIN. */
 struct ucred cred={.pid=getppid(),.uid=getuid(),.gid=getgid()};memcpy(CMSG_DATA(c),&cred,sizeof(cred));
 int ok=sendmsg(fd,&msg,MSG_NOSIGNAL)==(ssize_t)(sizeof(body)-1);int saved=errno;close(fd);errno=saved;return ok?0:1;
}
static int runtime_metadata(const char *path){
 int fd=safe_open(path,0);Meta m=capture(fd);close(fd);
 printf("uid=%lu gid=%lu mode=%o\n",(unsigned long)m.st.st_uid,(unsigned long)m.st.st_gid,(unsigned)(m.st.st_mode&07777));
 for(size_t i=0;i<m.count;i++){printf("xattr=%s bytes=%zu hex=",m.attrs[i].name,m.attrs[i].size);for(size_t j=0;j<m.attrs[i].size;j++)printf("%02x",((unsigned char*)m.attrs[i].value)[j]);putchar('\n');}
 release(&m);return 0;
}
int main(int argc,char **argv){
 if(argc==2 && !strcmp(argv[1],"port-free")){int fd=socket(AF_INET6,SOCK_DGRAM|SOCK_CLOEXEC,0);if(fd<0)return 2;struct sockaddr_in6 a={.sin6_family=AF_INET6,.sin6_port=htons(42989)};int ok=bind(fd,(struct sockaddr*)&a,sizeof(a))==0;close(fd);return ok?0:1;}
 if(argc==2 && !strcmp(argv[1],"notify-watchdog"))return runtime_notify();
 if(argc==4 && !strcmp(argv[1],"pulse"))return runtime_pulse(argv[2],argv[3]);
 if(argc==3 && !strcmp(argv[1],"metadata"))return runtime_metadata(argv[2]);
 if(argc==4 && !strcmp(argv[1],"same-inode")){int a=safe_open(argv[2],0),b=safe_open(argv[3],0);struct stat x,y;int ok=fstat(a,&x)==0&&fstat(b,&y)==0&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino;close(a);close(b);return ok?0:1;}
 return stock_file_guard_main(argc,argv);
}
