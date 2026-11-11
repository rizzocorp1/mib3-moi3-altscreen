#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
static uint64_t fake_ms=1000000;
static int fake_clock(clockid_t id,struct timespec *t){(void)id;t->tv_sec=fake_ms/1000;t->tv_nsec=(fake_ms%1000)*1000000;return 0;}
static void log_msg(const char *a,const char *b,...){(void)a;(void)b;}
#define clock_gettime fake_clock
#include "../deploy-usb/payload/frame-gate-v31.h"
static void pulse(const char *path,uint64_t deadline){FILE *f=fopen(path,"w");assert(f);fprintf(f,HANDOFF_TOKEN "%llu\n",(unsigned long long)deadline);fclose(f);}
int main(void){
 char dir[]="/tmp/managed-lease-XXXXXX";assert(mkdtemp(dir));char lease[512],request[512],grant[512];snprintf(lease,sizeof(lease),"%s/lease",dir);g_handoff_lease_path=lease;
 snprintf(request,sizeof(request),"%s/" HANDOFF_REQUEST_FILE,dir);snprintf(grant,sizeof(grant),"%s/" HANDOFF_GRANT_FILE,dir);
 assert(frame_gate_at(dir,1,1)==1);FILE *f=fopen(grant,"w");assert(f);fputs(HANDOFF_TOKEN,f);fclose(f);assert(frame_gate_at(dir,1,1)==-1);
 pulse(lease,fake_ms+12000);assert(frame_gate_at(dir,1,1)==0);
 for(unsigned i=0;i<14400;i++){fake_ms+=2000;pulse(lease,fake_ms+12000);assert(frame_gate_at(dir,0,1)==0 && !handoff_lifetime_expired());}
 assert(fake_ms>8*60*60*1000ULL);puts("PASS: eight hours of virtual renewable ownership, no ten-minute cutoff");
 fake_ms+=12001;assert(handoff_lifetime_expired() && frame_gate_at(dir,1,1)==-1);pulse(lease,fake_ms+16000);assert(!handoff_lease_valid());
 pulse(lease,fake_ms+12000);assert(handoff_lease_valid());unlink(lease);assert(symlink(request,lease)==0);assert(!handoff_lease_valid());unlink(lease);
 f=fopen(lease,"w");assert(f);fputs(HANDOFF_TOKEN "1\ntrailing",f);fclose(f);assert(!handoff_lease_valid());
 unlink(lease);unlink(request);unlink(grant);rmdir(dir);puts("PASS: stale/missing/symlink/far-future/malformed leases fail closed");return 0;
}
