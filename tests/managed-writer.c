#define _GNU_SOURCE
#define ALTSCREEN_TEST
#include "../deploy-usb/payload/altscreen_handler.c"
#include <assert.h>
int main(int n,char **v){assert(n==4);g_frame_directory=v[1];g_handoff_lease_path=v[2];
 assert(frame_gate_at(v[1],1,1)==1);for(unsigned i=0;i<150;i++){if(frame_gate_at(v[1],1,1)==0)break;usleep(100000);}
 assert(frame_gate_at(v[1],1,1)==0);g_rtp_sock=socket(AF_INET6,SOCK_DGRAM,0);assert(g_rtp_sock>=0 && bind_cluster_source(g_rtp_sock)==0);g_rtp_owned=1;
 FILE *f=fopen(v[3],"w");assert(f);fputs("ready",f);fclose(f);
 char stop[512];snprintf(stop,sizeof(stop),"%s.stop",v[3]);
 for(unsigned i=0;i<600 && g_rtp_owned && access(stop,F_OK)!=0;i++){release_cluster_output_if_unleased(v[1]);usleep(100000);}
 close_cluster_socket();end_frame_gate_at(v[1]);return 0;}
