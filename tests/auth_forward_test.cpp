// Includes the unmodified CForward class/methods extracted from the selected
// SourceMod checkout. Runs real compiled SourcePawn receivers in a standalone VM.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include <IForwardSys.h>
#include <ReentrantList.h>
#include <amtl/am-string.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <ctime>
#include "steam_gameserver.h"
#include "stubs/auth_server014.h"
using namespace SourceMod;
using namespace SourcePawn;
using FuncIter = ReentrantList<IPluginFunction*>::iterator;
#define FORWARDS_NAME_MAX 64
struct Reporter {
 int errors=0;
 template<class... Args> void GenerateError(Args...) { ++errors; }
} g_DbgReporter;
using namespace ke;
#include "sourcemod-forward.inc"

int checks=0;
void Check(bool ok,const char* message) {
 ++checks;
 if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
struct ForwardSystem {
 std::map<std::string,CForward*> owned;
 IForward* CreateForward(const char* name,ExecType type,unsigned count,const ParamType* types,...) {
  va_list ap;va_start(ap,types);auto* f=CForward::CreateForward(name,type,count,types,ap);va_end(ap);
  Check(f!=nullptr,"real SourceMod forward creation");owned[name]=f;return f;
 }
 void ReleaseForward(IForward* f){owned.erase(f->GetForwardName());delete f;}
} forwardSystem;
auto* forwards=&forwardSystem;
constexpr int MRES_IGNORED=0,MRES_SUPERCEDE=1;
int meta=-1,hookCount=0;
#define RETURN_META(result) do {meta=result;return;} while(false)
#define RETURN_META_VALUE(result,value) do {meta=result;return value;} while(false)
#define SH_DECL_HOOK0(...) static_assert(true,"hook transport simulated")
#define SH_DECL_HOOK0_void(...) static_assert(true,"hook transport simulated")
#define SH_DECL_HOOK3(...) static_assert(true,"hook transport simulated")
#define SH_MEMBER(...) 0
#define SH_ADD_HOOK(...) (++hookCount)
#define SH_REMOVE_HOOK_ID(...) ((void)0)
#define SH_CALL(target,method) [pointer=target](){return pointer->WasRestartRequested();}
struct HookServer { void LogOn(const char*){} bool WasRestartRequested(){return false;} } server;
#define ISteamGameServer HookServer
AuthServer014Stub authServer;
struct Resolver {HookServer* GetGameServer(){return &server;} steamworks::ISteamGameServer014* GetGameServerAuth014(){return &authServer;}} resolver;
class SteamWorksGSHooks;
struct SteamWorks {Resolver* pSWGameServer=&resolver;SteamWorksGSHooks* pGSHooks=nullptr;} g_SteamWorks;
struct Host {
 void AddGameFrameHook(void(*)(bool)){} void RemoveGameFrameHook(void(*)(bool)){}
 void LogError(void*,const char*,...) {Check(false,"unexpected hook error");}
} host;
auto* smutils=&host;auto* g_pSM=&host;void* myself=nullptr;
#include "../extension/hooks/swgshooks.cpp"

std::vector<unsigned char> expected;
uint32 expectedAccount=0;
int calls=0;
cell_t InspectAuthBytes(IPluginContext* context,const cell_t* params) {
 Check(params[0]==3,"receiver arity unchanged");
 Check(params[2]==static_cast<cell_t>(expected.size()),"length remains byte count");
 uint32 account;std::memcpy(&account,&params[3],sizeof(account));
 Check(account==expectedAccount,"AccountID bits preserved");
 cell_t* buffer=nullptr;
 Check(context->LocalToPhysAddr(params[1],&buffer)==SP_ERROR_NONE,"actual VM buffer address");
 Check(expected.empty() || std::memcmp(buffer,expected.data(),expected.size())==0,"exact bytes including zeros and high bits");
 size_t padded=expected.empty()?sizeof(cell_t):((expected.size()+3)/4)*4;
 for(size_t i=expected.size();i<padded;++i)
  Check(reinterpret_cast<unsigned char*>(buffer)[i]==0,"padding initialized, no source overread");
 ++calls;return 0;
}

int main(int argc,char** argv) {
 Check(argc==3,"VM library and compiled fixture required");
 void* library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 if(!library){std::fprintf(stderr,"VM load failed: %s\n",dlerror());return 1;}
 auto factory=reinterpret_cast<GetSourcePawnFactoryFn>(dlsym(library,"GetSourcePawnFactory"));
 Check(factory!=nullptr,"SourcePawn factory export");
 auto* api=factory(SOURCEPAWN_API_VERSION);Check(api!=nullptr,"SourcePawn factory API");
 auto* environment=api->NewEnvironment();Check(environment!=nullptr,"standalone VM environment");
 std::unique_ptr<IPluginRuntime> receivers[2];
 for(auto& receiver:receivers){
  char error[256]={};receiver.reset(environment->APIv2()->LoadBinaryFromFile(argv[2],error,sizeof(error)));
  if(!receiver){std::fprintf(stderr,"Fixture load failed: %s\n",error);return 1;}
  uint32_t index=0;
  Check(receiver->FindNativeByName("InspectAuthBytes",&index)==SP_ERROR_NONE,"find fixture inspector");
  Check(receiver->UpdateNativeBinding(index,InspectAuthBytes,0,nullptr)==SP_ERROR_NONE,"bind fixture inspector");
 }
 {
  SteamWorksGSHooks gs;
  auto* auth=forwards->owned.at("SteamWorks_BeginAuthSession");
  auto* first=receivers[0]->GetFunctionByName("SteamWorks_BeginAuthSession");
  auto* second=receivers[1]->GetFunctionByName("SteamWorks_BeginAuthSession");
  Check(first && second,"compiled packed-char receivers available");
  Check(auth->AddFunction(first),"register first receiver");
  // Negative control against the actual SourceMod implementation, not a mock:
  char byte=0;
  auto* broken=forwards->CreateForward("synthetic_negative_control",ET_Ignore,3,nullptr,Param_Array,Param_Cell,Param_Cell);
  static_cast<CForward*>(broken)->AddFunction(first);
  Check(broken->PushStringEx(&byte,1,SM_PARAM_STRING_BINARY|SM_PARAM_STRING_COPY,0)==SP_ERROR_PARAM,"old Param_Array/string mismatch rejected by SourceMod");
  Check(broken->Execute(nullptr)==SP_ERROR_PARAM && calls==0,"old send invokes no receiver");
  forwards->ReleaseForward(broken);
  for(int count:{1,2}) {
   if(count==2)Check(auth->AddFunction(second),"register second independent receiver");
   for(size_t length:{size_t(0),size_t(1),size_t(3),size_t(4),size_t(5),size_t(7),size_t(8),size_t(1024)}) {
    // Deliberately unaligned and exact-size source allocation for sanitizer checks.
    std::unique_ptr<unsigned char[]> source(new unsigned char[length+1]);
    source[0]=0xa5;expected.resize(length);
    for(size_t i=0;i<length;++i)source[i+1]=expected[i]=static_cast<unsigned char>((i%3==0)?0:0x80+i);
    expectedAccount=0xf1234567u;int before=calls;
    auto result=gs.BeginAuthSession(length?source.get()+1:nullptr,static_cast<int>(length),CSteamID(expectedAccount,1,k_EUniversePublic,k_EAccountTypeIndividual));
    Check(result==k_EBeginAuthSessionResultOK && meta==MRES_IGNORED,"observer preserves original auth dispatch");
    Check(calls==before+count,"each consumer called once");
    Check(source[0]==0xa5 && (length==0 || std::memcmp(source.get()+1,expected.data(),length)==0),"receiver mutation never copied to engine ticket");
   }
  }
  int before=calls;gs.BeginAuthSession(nullptr,4,CSteamID());gs.BeginAuthSession(nullptr,-1,CSteamID());
  Check(calls==before && meta==MRES_IGNORED,"invalid pointer/negative byte length skipped without changing original hook");
  gs.Stop();gs.BeginAuthSession(nullptr,0,CSteamID());Check(calls==before,"stopped owner does not notify");
  gs.Start();Check(auth->RemoveFunction(first) && auth->RemoveFunction(second),"receivers removable");
  gs.BeginAuthSession(reinterpret_cast<const void*>(1),1024,CSteamID());Check(calls==before,"zero consumers never read ticket");
 }
 Check(forwardSystem.owned.empty(),"all real forwards released");
 Check(g_DbgReporter.errors==0,"no SourceMod forwarding error");
 for(auto& receiver:receivers)receiver.reset();
 delete environment;dlclose(library);
 std::printf("PASS: %d authentication forward checks (production hook, actual SourceMod CForward and standalone SourcePawn VM; synthetic bytes only)\n",checks);
}
