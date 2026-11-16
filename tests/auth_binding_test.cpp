// Production resolver adapter + production hook owner + actual SourceHook.
// All interfaces, tickets and accounts in this process are synthetic.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include "sourcehook_impl.h"
#include "sourcehook.h"
#include "stubs/auth_server014.h"
#include "stubs/native_server015.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
using cell_t=int32_t;
constexpr int ET_Hook=0,ET_Ignore=1,Param_Cell=0,Param_String=1,Param_Array=2;
constexpr int SM_PARAM_STRING_UTF8=1,SM_PARAM_STRING_COPY=2,SM_PARAM_COPYBACK=4;
constexpr int SP_ERROR_NONE=0,Pl_Continue=0,Pl_Handled=3;
SourceHook::Impl::CSourceHookImpl hookSystem;
SourceHook::ISourceHook* g_SHPtr=&hookSystem;
int g_PLID=1;
int checks=0,dispatches=0;
void Check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
struct IForward {
 unsigned receivers=1;std::vector<cell_t> cells,array;std::function<void()> action;
 unsigned GetFunctionCount(){return receivers;}
 int PushCell(cell_t value){cells.push_back(value);return 0;}
 int PushArray(cell_t* data,unsigned count,int flags){Check(flags==0,"ticket remains no-copyback");array.assign(data,data+count);return 0;}
 void PushStringEx(char*,size_t,int,int){}
 void Cancel(){cells.clear();}
 int Execute(cell_t*){++dispatches;if(action)action();return 0;}
};
struct ForwardSystem {
 std::map<std::string,IForward*> owned;
 IForward* CreateForward(const char* name,int,unsigned,void*,...){return owned[name]=new IForward;}
 void ReleaseForward(IForward* f){for(auto it=owned.begin();it!=owned.end();++it)if(it->second==f){owned.erase(it);break;}delete f;}
} forwardSystem;
auto* forwards=&forwardSystem;
NativeServer015Stub native015;
AuthServer014Stub authA,authB;
steamworks::ISteamGameServer014* selectedAuth=&authA;
void* AuthFactory(HSteamUser user,const char* version){Check(user==7 && std::strcmp(version,"SteamGameServer014")==0,"adapter requests exact engine version/user");return selectedAuth;}
struct Resolver {
 ISteamGameServer* GetGameServer(){return &native015;}
 steamworks::ISteamGameServer014* GetGameServerAuth014(){return steamworks::ResolveGameServerAuth014(AuthFactory,7,"SteamGameServer014");}
} resolver;
class SteamWorksGSHooks;
struct SteamWorks {Resolver* pSWGameServer=&resolver;SteamWorksGSHooks* pGSHooks=nullptr;} g_SteamWorks;
struct Host {
 std::set<void(*)(bool)> frames;
 void AddGameFrameHook(void(*f)(bool)){frames.insert(f);}
 void RemoveGameFrameHook(void(*f)(bool)){frames.erase(f);}
 void LogError(void*,const char*,...){Check(false,"no hook installation error");}
} host;
auto* g_pSM=&host;auto* smutils=&host;void* myself=nullptr;
#ifndef AUTH_BINDING_HOOK_SOURCE
#define AUTH_BINDING_HOOK_SOURCE "../extension/hooks/swgshooks.cpp"
#endif
#include AUTH_BINDING_HOOK_SOURCE
// Independent engine-like caller: the analyzed engine uses slot 0x68/4=26.
// Production binding uses the typed declaration; this fixture pins the wire ABI.
__attribute__((noinline)) EBeginAuthSessionResult EngineAuth(steamworks::ISteamGameServer014* p,const void* bytes,int size,CSteamID id){
 void** table=nullptr;std::memcpy(&table,p,sizeof(table));
 using Entry=EBeginAuthSessionResult (*)(void*,const void*,int,CSteamID);
 auto entry=reinterpret_cast<Entry>(table[26]);return entry(p,bytes,size,id);
}
__attribute__((noinline)) EBeginAuthSessionResult NativeAuth(ISteamGameServer* p){return p->BeginAuthSession(nullptr,0,CSteamID());}
int Run(){
 const unsigned char ticket[]={0,0x80,0xff,1,0};
 CSteamID identity(0xf1234567u,1,k_EUniversePublic,k_EAccountTypeIndividual);
 {
  SteamWorksGSHooks hooks;g_SteamWorks.pGSHooks=&hooks;
  IForward* forward=forwards->owned.at("SteamWorks_BeginAuthSession");
  Check(EngineAuth(&authA,ticket,sizeof(ticket),identity)==k_EBeginAuthSessionResultInvalidTicket,"014 engine call preserves original result under real hook");
  Check(dispatches==1 && authA.authCalls==1,"real SourceHook delivers one forward and one original call");
  Check(forward->array.size()==2 && std::memcmp(forward->array.data(),ticket,sizeof(ticket))==0,"actual hook forwards packed binary bytes");
  Check(forward->cells.size()==2 && forward->cells[0]==5 && static_cast<uint32>(forward->cells[1])==0xf1234567u,"actual hook preserves byte count and all AccountID bits");
  Check(NativeAuth(&native015)==k_EBeginAuthSessionResultInvalidTicket && native015.authCalls==1 && dispatches==1,"015 object is not mistaken for the engine auth target");
  Check(EngineAuth(&authB,ticket,sizeof(ticket),identity)==k_EBeginAuthSessionResultInvalidTicket && authB.authCalls==1 && dispatches==1,"same-vtable second 014 instance does not receive first instance hook");
  hooks.Stop();
  Check(EngineAuth(&authA,ticket,sizeof(ticket),identity)==k_EBeginAuthSessionResultInvalidTicket && dispatches==1 && authA.authCalls==2,"stop removes real 014 hook and leaves original callable");
  selectedAuth=&authB;hooks.Start();
  EngineAuth(&authA,ticket,sizeof(ticket),identity);
  Check(dispatches==1,"replacement does not retain old auth target");
  EngineAuth(&authB,ticket,sizeof(ticket),identity);
  Check(dispatches==2,"restart attaches to replacement 014 object");
  forward->action=[&]{hooks.Stop();};
  int original=authB.authCalls;
  Check(EngineAuth(&authB,ticket,sizeof(ticket),identity)==k_EBeginAuthSessionResultInvalidTicket && authB.authCalls==original+1 && dispatches==3,"unhook during forward still calls original exactly once");
  forward->action={};hooks.Start();
 }
 g_SteamWorks.pGSHooks=nullptr;
 Check(forwards->owned.empty() && host.frames.empty(),"destructor releases forwards and polling");
 Check(EngineAuth(&authB,ticket,sizeof(ticket),identity)==k_EBeginAuthSessionResultInvalidTicket && dispatches==3,"destruction removes actual 014 hook before target is released");
 hookSystem.CompleteShutdown();
 std::printf("PASS: %d authentication binding checks (production owner/adapter, real SourceHook, synthetic 014/015 objects)\n",checks);
 return 0;
}
int main(){try{return Run();}catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}}
