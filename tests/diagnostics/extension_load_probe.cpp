// x86 loader-scope probe. Does not call extension interfaces or initialize Steam.
#include <dlfcn.h>
#include <cstdio>
static_assert(sizeof(void*) == 4, "The L4D2 diagnostic must be compiled for x86");
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 void* module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 if(!module){std::fprintf(stderr,"LOCAL_LOAD_FAILED: %s\n",dlerror());return 1;}
 std::puts("LOCAL_LOAD_OK ELF32");
 // Keep dependencies mapped until process exit; no runtime objects are acquired.
 return 0;
}
