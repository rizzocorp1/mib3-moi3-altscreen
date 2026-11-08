/* Read-only resource inventory for our named test stages. No pairing data. */
#include <sys/statvfs.h>
#include <dirent.h>
static int resource_space(const char *path) {
    int fd=safe_open(path,1);struct statvfs s;if(fstatvfs(fd,&s))fail("filesystem space");close(fd);
    unsigned long long unit=s.f_frsize?s.f_frsize:s.f_bsize;
    printf("FS totalBytes=%llu availableBytes=%llu freeBytes=%llu fragmentBytes=%llu\n",
           (unsigned long long)s.f_blocks*unit,(unsigned long long)s.f_bavail*unit,(unsigned long long)s.f_bfree*unit,unit);
    return 0;
}
static int resource_available(const char *path) {
    int fd=safe_open(path,1);struct statvfs s;if(fstatvfs(fd,&s))fail("filesystem available space");close(fd);
    unsigned long long unit=s.f_frsize?s.f_frsize:s.f_bsize;
    printf("%llu\n",(unsigned long long)s.f_bavail*unit);return 0;
}
static int resource_size(const char *path) {
    int fd=safe_open(path,0);struct stat s;if(fstat(fd,&s))fail("resource size");close(fd);
    printf("%llu\n",(unsigned long long)s.st_size);return 0;
}
static int resource_inventory(const char *path) {
    int base=safe_open(path,1);DIR *entries=fdopendir(base);if(!entries)fail("stage inventory directory");
    struct dirent *entry;unsigned seen=0;
    while((entry=readdir(entries))) {
        if(++seen>4096){errno=EOVERFLOW;fail("stage inventory entry bound");}
        if(strncmp(entry->d_name,".moi3-",6))continue;
        int fd=openat(base,entry->d_name,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if(fd<0){printf("STAGE name=%s unreadableOrNotDirectory=1 preserved=1\n",entry->d_name);continue;}
        DIR *files=fdopendir(fd);if(!files){close(fd);continue;}
        unsigned count=0,other=0,scanned=0;unsigned long long bytes=0;struct dirent *file;
        while((file=readdir(files))) {
            if(!strcmp(file->d_name,".")||!strcmp(file->d_name,".."))continue;
            if(++scanned>128){other++;break;}
            struct stat s;
            if(fstatat(fd,file->d_name,&s,AT_SYMLINK_NOFOLLOW)==0 && S_ISREG(s.st_mode)){count++;bytes+=(unsigned long long)s.st_size;}
            else other++;
        }
        closedir(files);printf("STAGE name=%s immediateRegularFiles=%u logicalBytes=%llu otherEntries=%u\n",entry->d_name,count,bytes,other);
    }
    closedir(entries);return 0;
}
