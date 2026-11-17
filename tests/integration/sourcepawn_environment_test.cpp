// Minimal ownership regression: no SourceMod forwards, SourceHook or Steam.
#include <sp_vm_api.h>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
using namespace SourcePawn;
void Check(bool ok,const char* message) {
 if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
int main(int argc,char** argv) {
 Check(argc==2,"VM library argument");
 void* library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 if(!library){std::fprintf(stderr,"dlopen: %s\n",dlerror());return 1;}
 auto entry=reinterpret_cast<GetSourcePawnFactoryFn>(dlsym(library,"GetSourcePawnFactory"));
 Check(entry!=nullptr,"factory export");
 auto* factory=entry(SOURCEPAWN_API_VERSION);Check(factory!=nullptr,"factory API");
 for(int cycle=0;cycle<16;++cycle){
  auto* environment=factory->NewEnvironment();Check(environment!=nullptr,"new environment");
  Check(environment->APIv1() && environment->APIv2(),"both owned APIs available");
  Check(factory->CurrentEnvironment()==environment,"environment registered");
  if(cycle%2)environment->Shutdown();
  delete environment;
  Check(factory->CurrentEnvironment()==nullptr,"destructor releases environment exactly once");
 }
 Check(dlclose(library)==0,"VM library closed");
 std::puts("PASS: 16 VM ownership cycles (delete only / explicit shutdown then delete)");
}
