// Exercise the actual x86 relocation helper on identified SDK reference prefixes.
// Never call a Steam init/shutdown function or continue into its body.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "asm/asm.h"

static void Require(bool ok,const char* reason)
{
 if (!ok) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); }
}
int main(int argc,char** argv)
{
 static_assert(sizeof(void*)==4,"This fixture only validates x86 prefixes");
 Require(argc==2 || argc==3,"SDK/reference path required");
 bool runtime=argc==3;
 void* module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 Require(module!=nullptr,"SDK reference library load");
 const char* names[]={runtime ? "SteamInternal_GameServer_Init" : "SteamGameServer_InitSafe","SteamGameServer_Shutdown"};
 const unsigned int lengths[]={runtime ? 9u : 6u,8};
 const unsigned int pushes[]={runtime ? 4u : 1u,3};
 for(unsigned int i=0;i<2;++i) {
  auto* source=static_cast<unsigned char*>(dlsym(module,names[i]));
  Require(source!=nullptr,"reference export exists");
  Dl_info info={};Require(dladdr(source,&info)!=0 && info.dli_fbase!=nullptr,"export belongs to mapped reference module");
  struct stat requested={},observed={};
  Require(stat(argv[1],&requested)==0 && stat(info.dli_fname,&observed)==0
    && requested.st_dev==observed.st_dev && requested.st_ino==observed.st_ino,"export provider is the exact requested reference file");
  auto* code=static_cast<unsigned char*>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
  Require(code!=MAP_FAILED,"fixture page allocation");
  unsigned char before[16];std::memcpy(before,source,sizeof(before));
  int required=copy_bytes(source,nullptr,6);
  Require(required==static_cast<int>(lengths[i]),"whole-instruction prefix length matches disassembled reference");
  Require(copy_bytes(source,code,required)==required,"production helper relocates complete prefix");
  Require(code[pushes[i]]==0xbb,"PIC call rewritten to mov ebx, original PC");
  uint32_t immediate=0;std::memcpy(&immediate,code+pushes[i]+1,4);
  Require(immediate==reinterpret_cast<uintptr_t>(source+required),"relocated PIC register uses original PC");
  size_t pos=required;code[pos++]=0x89;code[pos++]=0xd8; // mov eax,ebx
  code[pos++]=0x5b; // pop ebx
  if(i==1 || runtime) {code[pos++]=0x5e;code[pos++]=0x5f;}
  if(i==0 && runtime) {code[pos++]=0x5d;} // pop esi,edi
  code[pos++]=0xc3; // return to fixture, never jump into a Steam function body
  Require(mprotect(code,4096,PROT_READ|PROT_EXEC)==0,"fixture executable protection");
  auto invoke=reinterpret_cast<uint32_t(*)()>(code);
  Require(invoke()==immediate,"relocated prefix executes with expected PIC value and balanced stack");
  Require(std::memcmp(before,source,sizeof(before))==0,"reference module never patched");
  Require(munmap(code,4096)==0,"fixture page released");
 }
 Require(dlclose(module)==0,"owned module reference balanced");
 std::printf("PASS: 2 %s x86 PIC-prefix relocation/execution cases (actual copy_bytes, no init/body invocation)\n", runtime ? "selected runtime" : "SDK");
}
