#include "../../extension/core/swlifecycleprofile.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static int checks=0;
static void Check(bool ok,const char* reason) { ++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);} }
int main()
{
 using namespace steamworks;
 Check(MatchesLifecycleIdentity(true,LifecycleEngineBuildId,LifecycleSteamBuildId,1,1),"identified Linux x86 pair allowed");
 Check(!MatchesLifecycleIdentity(false,LifecycleEngineBuildId,LifecycleSteamBuildId,1,1),"Windows or other architecture rejected");
 for(size_t n : {size_t(0),size_t(2),size_t(3)}) {
  Check(!MatchesLifecycleIdentity(true,LifecycleEngineBuildId,LifecycleSteamBuildId,n,1),"missing/ambiguous engine rejected");
  Check(!MatchesLifecycleIdentity(true,LifecycleEngineBuildId,LifecycleSteamBuildId,1,n),"missing/ambiguous provider rejected");
 }
 for(const char* changed : {"","unknown","b152439170af081a0d31fc357364a4cd4caa56ab"}) {
  Check(!MatchesLifecycleIdentity(true,changed,LifecycleSteamBuildId,1,1),"unknown engine fingerprint rejected");
  Check(!MatchesLifecycleIdentity(true,LifecycleEngineBuildId,changed,1,1),"unknown provider fingerprint rejected");
 }
 Check(!MatchesLifecycleIdentity(true,nullptr,LifecycleSteamBuildId,1,1),"null engine identity rejected");
 Check(!MatchesLifecycleIdentity(true,LifecycleEngineBuildId,nullptr,1,1),"null provider identity rejected");
 Check(!IsVerifiedLifecycleAddress(nullptr,LifecycleEntry::Init),"no address without matching live modules");
 Check(VerifiedLifecycleInitCall(nullptr)==nullptr,"no engine call admission without matching live modules");
 Check(!HasVerifiedLifecycleProfile(),"test process cannot impersonate identified game");
#if defined(__linux__) && defined(__i386__)
 alignas(4) unsigned char memory[64]={};
 ElfW(Phdr) ph[2]={};ph[0].p_type=PT_LOAD;ph[0].p_memsz=sizeof(memory);ph[0].p_flags=PF_R;
 ph[1].p_type=PT_NOTE;ph[1].p_filesz=36;
 dl_phdr_info module={};module.dlpi_addr=reinterpret_cast<uintptr_t>(memory);module.dlpi_phdr=ph;module.dlpi_phnum=2;
 ElfW(Nhdr) note={4,20,NT_GNU_BUILD_ID};std::memcpy(memory,&note,sizeof(note));std::memcpy(memory+12,"GNU\0",4);
 for(unsigned int n=0;n<20;++n)memory[16+n]=static_cast<unsigned char>(n);
 char id[65]={};ProfileReadBuildId(&module,id);
 Check(std::strcmp(id,"000102030405060708090a0b0c0d0e0f10111213")==0,"GNU note parsed with explicit byte length");
 Check(ProfileMappedRange(&module,reinterpret_cast<uintptr_t>(memory)+63,1,false),"last valid mapped byte");
 Check(!ProfileMappedRange(&module,reinterpret_cast<uintptr_t>(memory)+63,2,false),"range cannot cross segment boundary");
 Check(!ProfileMappedRange(&module,reinterpret_cast<uintptr_t>(memory),1,true),"non-executable segment rejected for entrypoint");
 for(int mode=0;mode<3;++mode) {
  ElfW(Nhdr) bad=note;if(mode==0)bad.n_namesz=UINT32_MAX;if(mode==1)bad.n_descsz=UINT32_MAX;
  ph[1].p_filesz=mode==2 ? 11 : 36;std::memcpy(memory,&bad,sizeof(bad));id[0]='\0';
  ProfileReadBuildId(&module,id);Check(id[0]=='\0',"truncated/oversized GNU note rejected without out-of-range read");
 }
#endif
 std::printf("PASS: %d lifecycle identity checks (known pair, architecture, unknown/ambiguous fingerprints and live-module refusal)\n",checks);
}
