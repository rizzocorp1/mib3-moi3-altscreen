/* host.c — Pista II: prova dinâmica de que o membro 0x19a (switchMapSupported)
 * controla o VideoNavigation (FsgSetup Df44 Bit0), rodando o código REAL da
 * libMIBNavSDClientLib. Estudo/sandbox, sem carro. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*ctor_t)(void*, void*);
typedef void (*voidm_t)(void*);
typedef int  (*getb_t)(void*);

static long noop_fn(void){ return 0; }

int main(void){
  void* h = dlopen("libMIBNavSDClientLib_CarIF.so.00", RTLD_NOW|RTLD_GLOBAL);
  if(!h){ fprintf(stderr,"dlopen FAIL: %s\n", dlerror()); return 2; }
  ctor_t  ctor   = (ctor_t) dlsym(h,"_ZN4Viwi5NavSd11FctFsgSetupC1EPNS0_15INavSdInterfaceE");
  voidm_t setbap = (voidm_t)dlsym(h,"_ZN4Viwi5NavSd11FctFsgSetup12setBapEntityEv");
  getb_t  getVid = (getb_t) dlsym(h,"_ZNK4Viwi3Bap12NavigationSd8FsgSetup6Status76isFunctionSupportActivationDeactivationOfTheVideoNavigationSupportedDf44Bit0Ev");
  fprintf(stderr,"[*] ctor=%p setbap=%p getVid=%p\n",(void*)ctor,(void*)setbap,(void*)getVid);
  if(!ctor||!setbap||!getVid){ fprintf(stderr,"dlsym FAIL\n"); return 3; }

  /* vtable de no-ops p/ interface fake (se o ctor chamar virtuais) */
  static void* vt[128]; for(int i=0;i<128;i++) vt[i]=(void*)noop_fn;
  void* iface = calloc(1,0x200); *(void**)iface = vt;

  void* obj = calloc(1,0x1000);
  fprintf(stderr,"[*] chamando ctor FctFsgSetup(iface)...\n");
  ctor(obj, iface);
  void* status = *(void**)((char*)obj+0x160);
  fprintf(stderr,"[*] ctor OK. Status(obj+0x160)=%p\n", status);
  if(!status){ fprintf(stderr,"Status nulo -> nao da pra ler o getter\n"); return 4; }

  /* setBapEntity faz, no fim, um push BAP virtual via this+0x100 (vtable+0x50) que
   * crasha sem uma IBapEntity real. Como SÓ nos interessa a lógica dos bits (que roda
   * ANTES), apontamos this+0x100 pra nossa vtable de no-ops -> o push vira no-op. */
  *(void**)((char*)obj+0x100) = vt;

  fprintf(stderr,"[*] setBapEntity com switchMapSupported(0x19a)=1...\n");
  *((unsigned char*)obj+0x19a) = 1;
  setbap(obj);
  int v1 = getVid(status);
  fprintf(stderr,"[*] setBapEntity com 0x19a=0...\n");
  *((unsigned char*)obj+0x19a) = 0;
  setbap(obj);
  int v0 = getVid(status);

  printf("\n==== RESULTADO ====\n");
  printf("switchMapSupported=1 -> VideoNavigation(Df44Bit0)=%d\n", v1);
  printf("switchMapSupported=0 -> VideoNavigation(Df44Bit0)=%d\n", v0);
  printf("%s\n", (v1==1 && v0==0)
    ? ">>> PROVA DINAMICA OK: o bit 0x19a controla o VideoNavigation (byte7/bit1 da DASHBOARD_DISPLAY_CONFIGURATION)"
    : ">>> inconclusivo (v1 ou v0 fora do esperado)");
  return 0;
}
