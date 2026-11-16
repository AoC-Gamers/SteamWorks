// Actual production owner, typed wrapper and executable x86 CALL patch.
// SDK/SourceTV behavior and the independent shutdown patch transport are fixtures.
#define TEST_REAL_INIT_CALLSITE
#include "detours_test_host.h"
#include <sys/mman.h>
#include <type_traits>

using Init = bool (*)(uint32,uint16,uint16,uint16,EServerMode,const char*);
static int checks=0,providerCalls=0,foreignCalls=0;
static bool expectForeign=false;
static Init foreignOriginal=nullptr;
static void Check(bool ok,const char* why){++checks;Require(ok,why);}

static bool Provider(uint32 ip,uint16 steam,uint16 game,uint16 query,EServerMode mode,const char* version)
{
 ++providerCalls;
 Check(ip==0x12345678u && steam==123 && game==456 && query==(expectForeign?122:789)
  && mode==testServerMode && std::strcmp(version,"fixture-version")==0,"all six arguments preserved including consumer query-port correction");
 if(detourState.initResult)gameServer.server=&gameServer.instance;
 return detourState.initResult;
}
static bool ForeignConsumer(uint32 ip,uint16 steam,uint16 game,uint16,EServerMode mode,const char* version)
{
 ++foreignCalls;
 return foreignOriginal(ip,steam,game,static_cast<uint16>(steam-1),mode,version);
}
static void Branch(unsigned char* at,unsigned char opcode,void* target)
{
 at[0]=opcode;
 uint32_t offset=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target))
                -static_cast<uint32_t>(reinterpret_cast<uintptr_t>(at))-5u;
 std::memcpy(at+1,&offset,4);
}
struct ForeignPatch
{
 unsigned char* entry;
 unsigned char saved[6];
 ForeignPatch(unsigned char* sdk,unsigned char* trampoline):entry(sdk)
 {
  // Reproduce capture during consumer load and activation at AllLoaded.
  std::memcpy(saved,sdk,6);std::memcpy(trampoline,saved,6);
  Branch(trampoline+6,0xe9,sdk+6);foreignOriginal=reinterpret_cast<Init>(trampoline);
 }
 void Enable(){Branch(entry,0xe9,reinterpret_cast<void*>(&ForeignConsumer));entry[5]=0x90;expectForeign=true;}
 void Disable(){std::memcpy(entry,saved,6);expectForeign=false;}
};
static bool RefuseWrite(void*,const void*,size_t){return false;}
static bool Copy(void* to,const void* from,size_t n){std::memcpy(to,from,n);return true;}

int main()
{
 (void)&InvokeInit;
 static_assert(sizeof(void*)==4,"Executable coexistence regression requires x86");
 static_assert(!std::is_copy_constructible<steamworks::InitCallSite>::value,"one patch owner");
 auto* page=static_cast<unsigned char*>(mmap(nullptr,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
 Check(page!=MAP_FAILED,"allocate independent engine/SDK fixture");
 auto* sdk=page+0x100;auto* trampoline=page+0x200;auto* engine=page+0x300;
 std::memset(sdk,0x90,6);Branch(sdk+6,0xe9,reinterpret_cast<void*>(&Provider));
 // cdecl engine caller forwards all six stack arguments; balanced/aligned stack.
 unsigned char prologue[]={0x55,0x89,0xe5};std::memcpy(engine,prologue,3);size_t pos=3;
 for(int offset=28;offset>=8;offset-=4){engine[pos++]=0xff;engine[pos++]=0x75;engine[pos++]=static_cast<unsigned char>(offset);}
 auto* call=engine+pos;Branch(call,0xe8,sdk);pos+=5;
 unsigned char tail[]={0x83,0xc4,0x18,0x5d,0xc3};std::memcpy(engine+pos,tail,sizeof(tail));
 unsigned char originalCall[5],originalSDK[16];std::memcpy(originalCall,call,5);std::memcpy(originalSDK,sdk,16);
 Init invoke=reinterpret_cast<Init>(engine);
 auto run=[&](bool result,bool observed){
  int before=providerCalls,consumer=foreignCalls,hooks=detourState.addHooks;
  detourState.initResult=result;
  Check(invoke(0x12345678u,123,456,789,testServerMode,"fixture-version")==result,"engine receives original bool result");
  Check(providerCalls==before+1 && foreignCalls==consumer+(expectForeign?1:0),"SDK and consumer each execute exactly once");
  Check(detourState.addHooks==hooks+((result&&observed)?1:0),"only original success reactivates production runtime");
 };
 // Consumer captured before SteamWorks, but enables after it: observed failing order.
 // Also exercise consumer enabled before SteamWorks and loaded after it.
 for(int order=0;order<3;++order){
  ResetFixture();initProviderOverride=sdk;initCallOverride=call;
  ForeignPatch* foreign=nullptr;
  if(order!=2)foreign=new ForeignPatch(sdk,trampoline);
  if(order==1)foreign->Enable();
  {
   SteamWorksGSDetours owner;Check(owner.IsInstalled(),"production pair installed with shared SDK entry already patched or clean");
   if(order==2)foreign=new ForeignPatch(sdk,trampoline);
   if(order!=1)foreign->Enable();
   unsigned char consumerPatch[16];std::memcpy(consumerPatch,sdk,16);
   run(true,true);run(false,true);run(true,true);
   SteamAPIShutdown();run(true,true);
   Check(std::memcmp(sdk,consumerPatch,16)==0,"SteamWorks never rewrites consumer SDK bytes");
   foreign->Disable();run(true,true); // Consumer unload while observer stays live.
   foreign->Enable();run(true,true);
   owner.Shutdown();Check(!owner.IsInstalled(),"observer teardown completes");
   Check(std::memcmp(call,originalCall,5)==0 && std::memcmp(sdk,consumerPatch,16)==0,"restore only owned engine CALL, preserving consumer patch");
   run(true,false);
   {SteamWorksGSDetours reload;Check(reload.IsInstalled(),"reload accepts same SDK export with active consumer");run(true,true);}
   Check(std::memcmp(call,originalCall,5)==0,"reload restores audited engine call");
  }
  foreign->Disable();delete foreign;Check(std::memcmp(sdk,originalSDK,16)==0,"consumer restores its own entry");
  run(true,false);
 }
 // Local rejection and transactional write/restore failures never claim success.
 {
  steamworks::InitCallSite patch(Copy);
  Check(!patch.Prepare(nullptr,sdk,engine),"null call refused");
  Check(!patch.Prepare(call,sdk+1,engine),"wrong decoded target refused");
  unsigned char opcode=call[0];call[0]=0xe9;Check(!patch.Prepare(call,sdk,engine),"JMP or changed call opcode refused");call[0]=opcode;
  Check(patch.Prepare(call,sdk,reinterpret_cast<void*>(&SteamGameServer_InitLegacyDetour)),"prepare verified call");
  Check(!patch.Prepare(call,sdk,engine),"duplicate preparation refused");
  call[1]^=1;Check(!patch.Enable()&&!patch.IsInstalled(),"foreign change between prepare and enable refused");call[1]^=1;
  Check(patch.Enable()&&patch.Enable(),"repeat enable is idempotent");
  unsigned char owned[5];std::memcpy(owned,call,5);call[1]^=1;
  unsigned char foreign[5];std::memcpy(foreign,call,5);
  Check(!patch.Restore() && std::memcmp(call,foreign,5)==0,"foreign replacement is never overwritten at restore");
  std::memcpy(call,owned,5);Check(patch.Restore()&&patch.Restore(),"restoration and duplicate cleanup are safe");
 }
 {steamworks::InitCallSite patch(RefuseWrite);Check(patch.Prepare(call,sdk,engine)&&!patch.Enable()&&!patch.IsInstalled(),"failed code protection/write leaves uninstalled patch");}
 Check(std::memcmp(call,originalCall,5)==0,"all negative cases preserve original call");
 Check(munmap(page,4096)==0,"fixture pages released");
 std::printf("PASS: %d executable x86 init coexistence checks (production owner/caller patch/wrapper; synthetic SDK and staged consumer)\n",checks);
}
