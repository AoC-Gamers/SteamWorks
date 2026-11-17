// Exercise the actual admission code with synthetic loaded ELF metadata.
#include <link.h>
static int IterateFixture(int (*)(dl_phdr_info*,size_t,void*),void*);
#define dl_iterate_phdr IterateFixture
#include "../../extension/core/swlifecycleprofile.h"
#undef dl_iterate_phdr
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static dl_phdr_info modules[2];
static Elf32_Phdr segments[2][2];
static bool duplicate=false;
static int checks=0;
static void Check(bool ok,const char* reason){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
static int IterateFixture(int (*visit)(dl_phdr_info*,size_t,void*),void* context)
{
 for(auto& m:modules)if(visit(&m,sizeof(m),context))return 0;
 if(duplicate)visit(&modules[0],sizeof(modules[0]),context);
 return 0;
}
static void Setup(int index,const char* name,const char* id,size_t size)
{
 auto* mem=static_cast<unsigned char*>(mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
 Check(mem!=MAP_FAILED,"allocate bounded ELF fixture");
 auto& m=modules[index];m.dlpi_addr=reinterpret_cast<uintptr_t>(mem);m.dlpi_name=name;m.dlpi_phdr=segments[index];m.dlpi_phnum=2;
 auto& load=segments[index][0];load.p_type=PT_LOAD;load.p_memsz=size;load.p_flags=PF_R|PF_X;
 auto& note=segments[index][1];note.p_type=PT_NOTE;note.p_vaddr=0x100;note.p_filesz=36;
 Elf32_Nhdr header={4,20,NT_GNU_BUILD_ID};std::memcpy(mem+0x100,&header,sizeof(header));std::memcpy(mem+0x10c,"GNU\0",4);
 for(size_t i=0;i<20;++i){char byte[3]={id[2*i],id[2*i+1],0};mem[0x110+i]=static_cast<unsigned char>(std::strtoul(byte,nullptr,16));}
}
int main()
{
 static_assert(sizeof(void*)==4,"x86 admission fixture");
 using namespace steamworks;
 Setup(0,"/fixture/engine_srv.so",LifecycleEngineBuildId,0x210000);
 Setup(1,"/fixture/libsteam_api.so",LifecycleSteamBuildId,0x20000);
 auto* call=reinterpret_cast<unsigned char*>(modules[0].dlpi_addr+0x2018eb);
 auto* target=reinterpret_cast<unsigned char*>(modules[1].dlpi_addr+0x1b430);
 const unsigned char before[]={0x8b,0x83,0xb4,0,0,0,0x89,0x04,0x24};
 const unsigned char after[]={0x84,0xc0,0x0f,0x84};
 std::memcpy(call-9,before,9);std::memcpy(call+5,after,4);call[0]=0xe8;
 uint32_t relative=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(call)-5u);std::memcpy(call+1,&relative,4);
 Check(VerifiedLifecycleInitCall(target)==call,"exact mapped pair/call/destination admitted");
 target[0]=0xe9;Check(VerifiedLifecycleInitCall(target)==call,"SDK consumer patch is untouched and does not invalidate independent caller");
 target[0]=0xff;target[1]=0x25;Check(VerifiedLifecycleInitCall(target)==call,"no interpretation or chaining of SDK patch opcode");
 Check(VerifiedLifecycleInitCall(target+1)==nullptr,"different export RVA rejected");
 for(int offset:{-9,-1,0,1,4,5,8}){
  call[offset]^=1;Check(VerifiedLifecycleInitCall(target)==nullptr,"mutated caller context/opcode/destination rejected");call[offset]^=1;
 }
 segments[0][0].p_flags=PF_R;Check(VerifiedLifecycleInitCall(target)==nullptr,"non-executable caller rejected");segments[0][0].p_flags|=PF_X;
 segments[1][0].p_flags=PF_R;Check(VerifiedLifecycleInitCall(target)==nullptr,"non-executable SDK destination rejected");segments[1][0].p_flags|=PF_X;
 auto size=segments[0][0].p_memsz;segments[0][0].p_memsz=0x2018f3;Check(VerifiedLifecycleInitCall(target)==nullptr,"truncated caller mapping rejected before reads");segments[0][0].p_memsz=size;
 auto* identity=reinterpret_cast<unsigned char*>(modules[1].dlpi_addr+0x110);*identity^=1;
 Check(VerifiedLifecycleInitCall(target)==nullptr,"unknown SDK build rejected even with patched entry");*identity^=1;
 duplicate=true;Check(VerifiedLifecycleInitCall(target)==nullptr,"ambiguous engine module rejected");duplicate=false;
 Check(VerifiedLifecycleInitCall(target)==call,"valid state recovers without activation bypass");
 for(auto& m:modules)Check(munmap(reinterpret_cast<void*>(m.dlpi_addr),m.dlpi_phdr[0].p_memsz)==0,"ELF fixture released");
 std::printf("PASS: %d x86 init caller admission checks (production profile, synthetic ELF mappings)\n",checks);
}
