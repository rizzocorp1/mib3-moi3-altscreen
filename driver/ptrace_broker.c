/* ptrace_broker.c — broker kipc por ptrace (intercepta TODO ioctl, inclusive
 * os por syscall direto que o LD_PRELOAD não alcança). Para req==0 (kipc):
 * pula o syscall real, devolve 0, e zera descritor+0x00 (len=0) e +0x10 (tipo=0)
 * = "sem mensagem". Testa se o double-free era do caminho de erro do EBADF.
 * Estudo/sandbox; roda o binário dentro do chroot /mnt/low. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <elf.h>

#ifndef NT_ARM_SYSTEM_CALL
#define NT_ARM_SYSTEM_CALL 0x404
#endif

static int getregs(pid_t p, struct user_regs_struct* r){ struct iovec io={r,sizeof(*r)}; return ptrace(PTRACE_GETREGSET,p,(void*)NT_PRSTATUS,&io); }
static int setregs(pid_t p, struct user_regs_struct* r){ struct iovec io={r,sizeof(*r)}; return ptrace(PTRACE_SETREGSET,p,(void*)NT_PRSTATUS,&io); }
static int set_scno(pid_t p, int nr){ struct iovec io={&nr,sizeof(nr)}; return ptrace(PTRACE_SETREGSET,p,(void*)NT_ARM_SYSTEM_CALL,&io); }

int main(void){
  pid_t c=fork();
  if(c==0){
    ptrace(PTRACE_TRACEME,0,0,0);
    if(chroot("/mnt/low")!=0){ perror("chroot"); _exit(127); }
    if(chdir("/")!=0){ _exit(126); }
    char* env[]={ "PATH=/usr/bin:/bin:/sbin", NULL };
    execle("/lge/app_rw/smartlink/dcmanager/sdcm","sdcm",(char*)NULL,env);
    perror("exec"); _exit(125);
  }
  int st; waitpid(c,&st,0);  /* para no exec */
  fprintf(stderr,"[ptrace] tracer iniciou, child=%d stopped=%d sig=%d\n",c,WIFSTOPPED(st),WIFSTOPPED(st)?WSTOPSIG(st):-1);
  ptrace(PTRACE_SETOPTIONS,c,0,(void*)(PTRACE_O_TRACESYSGOOD|PTRACE_O_EXITKILL));
  long faked=0, total=0; int entry=1;
  for(;;){
    if(ptrace(PTRACE_SYSCALL,c,0,0)<0) break;
    if(waitpid(c,&st,0)<0) break;
    if(WIFEXITED(st)){ fprintf(stderr,"[ptrace] child exit code %d\n",WEXITSTATUS(st)); break; }
    if(WIFSIGNALED(st)){ fprintf(stderr,"[ptrace] child killed by sig %d (faked=%ld total=%ld)\n",WTERMSIG(st),faked,total); break; }
    if(!(WIFSTOPPED(st) && (WSTOPSIG(st)==(SIGTRAP|0x80)))) continue; /* so paradas de syscall */
    struct user_regs_struct r; if(getregs(c,&r)<0) break;
    if(entry){
      total++;
      if((total % 2000)==0) fprintf(stderr,"[ptrace] VIVO: syscalls=%ld, ioctls_kipc_faked=%ld\n",total,faked);
      long nr=r.regs[8];
      if(nr==29 /*ioctl*/ && r.regs[1]==0UL /*req==0 = kipc*/){
        unsigned long desc=r.regs[2];
        set_scno(c,-1);                 /* pula o syscall real */
        r.regs[8]=(unsigned long)-1; setregs(c,&r);
        /* zera descritor+0x00 (len) e +0x10 (tipo) = sem mensagem */
        if(desc){
          unsigned long z=0; struct iovec lv={&z,8}, rv={(void*)desc,8}; process_vm_writev(c,&lv,1,&rv,1,0);
          unsigned int z2=0; struct iovec lv2={&z2,4}, rv2={(void*)(desc+0x10),4}; process_vm_writev(c,&lv2,1,&rv2,1,0);
        }
        r.regs[0]=0;                    /* marca p/ forçar 0 na saida */
        faked++;
      }
      entry=0;
    } else {
      /* saida de syscall: se foi um ioctl kipc pulado, força x0=0 */
      /* (nr ja foi -1; o retorno veio -ENOSYS; sobrescreve) */
      long ret=r.regs[0];
      if((long)ret==-ENOSYS){ r.regs[0]=0; setregs(c,&r); }
      entry=1;
    }
  }
  fprintf(stderr,"[ptrace] fim: faked=%ld ioctls kipc de %ld syscalls\n",faked,total);
  return 0;
}
