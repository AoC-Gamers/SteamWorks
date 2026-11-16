#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include "steam_gameserver.h"
#include "isteamgamecoordinator.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <functional>
#include <ctime>
#include <cstdarg>
#include <new>
#include <limits>
#if defined(__clang__)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define TEST_GC_ASAN_READ_GUARD
#endif
#elif defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#define TEST_GC_ASAN_READ_GUARD
#endif
#include "../support/stubs/auth_server014.h"
using cell_t=int32_t;
constexpr int ET_Hook=0,ET_Ignore=1,ET_Event=2,Param_Cell=0,Param_String=1,Param_Array=2;
constexpr int SM_PARAM_STRING_UTF8=1,SM_PARAM_STRING_COPY=2,SM_PARAM_COPYBACK=4,SM_PARAM_STRING_BINARY=8;
constexpr int Pl_Continue=0,Pl_Handled=3,MRES_IGNORED=0,MRES_SUPERCEDE=1;
constexpr int SP_ERROR_NONE=0;
int meta=0;bool originalAvailable=true;
#define RETURN_META(result) do { meta=result;return; } while(false)
#define RETURN_META_VALUE(result,value) do { meta=result;return value; } while(false)
#define META_RESULT_ORIG_RET(type) originalAvailable
#define SH_DECL_HOOK0(...) static_assert(true,"simulated hook declaration")
#define SH_DECL_HOOK0_void(...) static_assert(true,"simulated hook declaration")
#define SH_DECL_HOOK1(...) static_assert(true,"simulated hook declaration")
#define SH_DECL_HOOK3(...) static_assert(true,"simulated hook declaration")
#define SH_DECL_HOOK4(...) static_assert(true,"simulated hook declaration")
#define SH_MEMBER(...) 0
struct HookRecord { void* target;std::string method;bool post; };
std::map<int,HookRecord> hooks;std::vector<HookRecord> removed;
int creates=0,nextId=1,failAt=0,errors=0;
int TestAdd(const char* method,void* target,bool post) {
 ++creates;if(creates==failAt)return 0;int id=nextId++;hooks[id]={target,method,post};return id;
}
void TestRemove(int id) { auto i=hooks.find(id);if(i==hooks.end()){std::fprintf(stderr,"double hook removal\n");std::exit(1);}removed.push_back(i->second);hooks.erase(i); }
#define SH_ADD_HOOK(type,method,target,member,post) TestAdd(#method,target,post)
#define SH_REMOVE_HOOK_ID(id) TestRemove(id)
template<class T,class M>auto Bind(T* target,M method) { return [target,method](auto... args){return (target->*method)(args...);}; }
#define SH_CALL(target,method) Bind(target,method)
struct HookServer {
 int logons=0,restarts=0;bool restart=true;std::function<void()> action;
 void LogOn(const char*) { ++logons; }
 bool WasRestartRequested() { ++restarts;if(action)action();return restart; }
};
struct HookGC {
 int receives=0;std::function<void()> action;
 EGCResults result=k_EGCResultNoMessage; bool writeOutputs=false;
 uint32 reportedSize=0,reportedType=0x80000042;std::vector<unsigned char> payload;
 EGCResults RetrieveMessage(uint32* type,void* dest,uint32 capacity,uint32* size) {
  ++receives;
  if(writeOutputs) {
   if(type){*type=reportedType;}
   if(size){*size=reportedSize;}
   if(result==k_EGCResultOK && dest && !payload.empty() && payload.size()<=capacity)std::memcpy(dest,payload.data(),payload.size());
  }
  if(action){action();}
  return result;
 }
 bool IsMessageAvailable(uint32* size) { *size=5;return true; }
};
#define ISteamGameServer HookServer
#define ISteamGameCoordinator HookGC
struct IForward {
 int consumers=1,executions=0;cell_t result=0;std::vector<cell_t> cells;std::function<void()> action;
 std::vector<int> types;std::vector<cell_t> array;int arrayFlags=-1,pushes=0,failPushAt=0,cancels=0;
 int GetFunctionCount() { return consumers; }
 int PushCell(cell_t value) { if(++pushes==failPushAt)return 4;cells.push_back(value);return 0; }
 int PushArray(cell_t* data,unsigned int count,int flags) { if(++pushes==failPushAt)return 4;array.assign(data,data+count);arrayFlags=flags;return 0; }
 void Cancel() {++cancels;cells.clear();}
 std::vector<unsigned char> bytes;int stringFlags=0,stringCopyback=-1;
 void PushStringEx(char* data,size_t size,int flags,int copyback) {
  bytes.assign(reinterpret_cast<unsigned char*>(data),reinterpret_cast<unsigned char*>(data)+size);
  stringFlags=flags;stringCopyback=copyback;
 }
 int Execute(cell_t* output) { ++executions;auto run=action;if(run)run();if(output)*output=result;return 0; }
};
struct ForwardSystem {
 std::map<std::string,IForward*> owned;
 IForward* CreateForward(const char* name,int,unsigned count,void* types,...) {
  (void)types;auto* f=new IForward;va_list ap;va_start(ap,types);
  for(unsigned i=0;i<count;++i)f->types.push_back(va_arg(ap,int));
  va_end(ap);owned[name]=f;return f;
 }
 void ReleaseForward(IForward* f) { for(auto i=owned.begin();i!=owned.end();++i)if(i->second==f){owned.erase(i);break;}delete f; }
 void ReleaseGCForward(IForward* f) { ReleaseForward(f); }
} forwardSystem;
auto* forwards=&forwardSystem;
using Frame=void(*)(bool);
struct Host {
 std::set<Frame> frames;int frameAdds=0,frameRemoves=0;
 void AddGameFrameHook(Frame frame) { if(!frames.insert(frame).second){std::fprintf(stderr,"duplicate frame\n");std::exit(1);}++frameAdds; }
 void RemoveGameFrameHook(Frame frame) { if(!frames.erase(frame)){std::fprintf(stderr,"duplicate frame removal\n");std::exit(1);}++frameRemoves; }
 void LogError(void*,const char*,...) { ++errors; }
 void Tick() { auto snapshot=frames;for(auto frame:snapshot)if(frames.count(frame))frame(true); }
} host;
auto* smutils=&host;auto* g_pSM=&host;void* myself=nullptr;
HookServer server1,server2;HookGC gc1,gc2;AuthServer014Stub auth1,auth2;
struct Resolver {
 HookServer* server=nullptr;steamworks::ISteamGameServer014* auth=nullptr;HookGC* gc=nullptr;int reads=0;
 HookServer* GetGameServer() { ++reads;return server; }
 steamworks::ISteamGameServer014* GetGameServerAuth014() { ++reads;return auth; }
 HookGC* GetGameCoordinator() { ++reads;return gc; }
} resolver;
class SteamWorksGSHooks;class SteamWorksGCHooks;
struct SteamWorks { Resolver* pSWGameServer=&resolver;SteamWorksGSHooks* pGSHooks=nullptr;SteamWorksGCHooks* pGCHooks=nullptr; } g_SteamWorks;
#include "../../extension/hooks/swgshooks.cpp"
// Both original files use local ReleaseForward with identical signature.
#define ReleaseForward ReleaseGCForward
#ifndef GC_HOOK_SOURCE
#define GC_HOOK_SOURCE "../../extension/hooks/swgchooks.cpp"
#endif
#include GC_HOOK_SOURCE
#undef ReleaseForward
int checks=0;
void Check(bool ok,const char* why) { ++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);} }
bool failNextSnapshotAllocation=false;
void* operator new[](std::size_t size,const std::nothrow_t&) noexcept {
 if(failNextSnapshotAllocation){failNextSnapshotAllocation=false;return nullptr;}
 return ::operator new[](size);
}
void CheckReceiveBuffers(SteamWorksGCHooks& hook,HookGC& provider,IForward* forward) {
 auto reset=[&]{provider.result=k_EGCResultOK;provider.writeOutputs=true;provider.reportedSize=3;provider.payload={0,0x80,0xff};forward->result=0;forward->cells.clear();forward->bytes.clear();};
 auto checkShape=[&](uint32 cap,uint32 size){Check(forward->cells.size()==3 && static_cast<uint32>(forward->cells[1])==cap && static_cast<uint32>(forward->cells[2])==size,"GC forward capacity and valid length describe delivered storage");};
 std::vector<unsigned char> buffer(4096,0xa5);uint32 type=0x11111111,size=0x22222222;
 reset();int calls=provider.receives;
 Check(hook.RetrieveMessage(&type,buffer.data(),buffer.size(),&size)==k_EGCResultOK && provider.receives==calls+1 && meta==MRES_SUPERCEDE,"GC short valid retrieval preserves original call/result");
 Check(forward->bytes.size()==buffer.size() && forward->bytes[0]==0 && forward->bytes[1]==0x80 && forward->bytes[2]==0xff,"GC binary prefix copied with legacy capacity");
 bool clean=true;for(size_t i=3;i<forward->bytes.size();++i)clean=clean && forward->bytes[i]==0;
 Check(clean,"GC payload tail contains only initialized zeros");checkShape(buffer.size(),3);
 Check(buffer[3]==0xa5 && forward->stringCopyback==0,"GC forwarding never clears or copies back engine buffer");
 Check(static_cast<uint32>(forward->cells[0])==provider.reportedType,"GC validated message type keeps all protobuf/sign bits");
 reset();provider.payload.assign(16,0x7e);provider.reportedSize=16;hook.RetrieveMessage(&type,buffer.data(),16,&size);
 Check(forward->bytes==provider.payload,"GC full-capacity valid payload is not truncated");checkShape(16,16);
 for(auto result:{k_EGCResultNoMessage,k_EGCResultBufferTooSmall,k_EGCResultNotLoggedOn,k_EGCResultInvalidMessage}){
  reset();provider.result=result;provider.writeOutputs=result==k_EGCResultBufferTooSmall;provider.reportedSize=4097;type=0x11111111;size=0x22222222;buffer.assign(4096,0xa5);
  Check(hook.RetrieveMessage(&type,buffer.data(),32,&size)==result,"GC unsuccessful provider result preserved");
  Check(forward->bytes==std::vector<unsigned char>(32,0) && forward->cells[0]==0,"GC errors expose zero storage/type without relying on provider outputs");checkShape(32,0);Check(buffer[0]==0xa5,"GC error engine buffer remains untouched");
  Check(type==(provider.writeOutputs?provider.reportedType:0x11111111u) && size==(provider.writeOutputs?provider.reportedSize:0x22222222u),"GC forwarding preserves original SDK output cells even when sanitizing callback metadata");
 }
#ifdef TEST_GC_ASAN_READ_GUARD
 reset();provider.result=k_EGCResultNoMessage;provider.writeOutputs=false;
 __asan_poison_memory_region(&type,sizeof(type));__asan_poison_memory_region(&size,sizeof(size));
 auto noMessage=hook.RetrieveMessage(&type,buffer.data(),32,&size);
 __asan_unpoison_memory_region(&type,sizeof(type));__asan_unpoison_memory_region(&size,sizeof(size));
 Check(noMessage==k_EGCResultNoMessage && forward->cells[0]==0 && forward->cells[2]==0,"GC no-message never reads poisoned unwritten provider outputs");
#endif
 reset();provider.result=k_EGCResultNoMessage;provider.writeOutputs=false;hook.RetrieveMessage(nullptr,nullptr,12,nullptr);
 Check(forward->bytes==std::vector<unsigned char>(12,0),"GC null destination still supplies declared capacity safely");checkShape(12,0);
 reset();provider.payload.clear();provider.reportedSize=0;hook.RetrieveMessage(nullptr,nullptr,0,&size);
 Check(forward->bytes==std::vector<unsigned char>(1,0),"GC zero capacity supplies only an empty backing byte");checkShape(0,0);
 reset();hook.RetrieveMessage(nullptr,buffer.data(),16,&size);Check(forward->cells[0]==0 && forward->bytes[2]==0xff,"GC missing type retains zero type and known valid payload");
 reset();hook.RetrieveMessage(&type,buffer.data(),16,nullptr);Check(forward->bytes==std::vector<unsigned char>(16,0) && forward->cells[0]==0,"GC missing size never invents payload validity");checkShape(16,0);
 reset();provider.payload.clear();provider.reportedSize=17;int dispatched=forward->executions;hook.RetrieveMessage(&type,buffer.data(),16,&size);
 Check(forward->executions==dispatched && meta==MRES_SUPERCEDE,"GC incoherent success size skips forward before copying");
 reset();dispatched=forward->executions;auto nullResult=hook.RetrieveMessage(&type,nullptr,16,&size);
 Check(nullResult==k_EGCResultOK && forward->executions==dispatched,"GC nonempty success without destination skips invalid payload copy");
 reset();provider.result=k_EGCResultNoMessage;provider.writeOutputs=false;dispatched=forward->executions;hook.RetrieveMessage(&type,buffer.data(),UINT32_MAX,&size);
 Check(forward->executions==dispatched,"GC unrepresentable capacity skips snapshot allocation");
 reset();provider.result=k_EGCResultNoMessage;provider.writeOutputs=false;
 failNextSnapshotAllocation=true;dispatched=forward->executions;
 hook.RetrieveMessage(&type,buffer.data(),static_cast<uint32>(INT32_MAX)&~3u,&size);
 Check(forward->executions==dispatched && !failNextSnapshotAllocation,"GC representable maximum reaches controlled allocation without arbitrary policy cap");
 reset();failNextSnapshotAllocation=true;dispatched=forward->executions;auto result=hook.RetrieveMessage(&type,buffer.data(),16,&size);
 Check(result==k_EGCResultOK && forward->executions==dispatched && !failNextSnapshotAllocation,"GC allocation failure preserves SDK result and skips dispatch");
 for(cell_t choice:{0,-1,2,4}){
  reset();provider.result=k_EGCResultNoMessage;provider.writeOutputs=false;forward->result=choice;
  Check(hook.RetrieveMessage(&type,buffer.data(),16,&size)==(choice==0?k_EGCResultNoMessage:choice==-1?k_EGCResultOK:static_cast<EGCResults>(choice)) && meta==MRES_SUPERCEDE,"GC existing receiver result/force-OK convention preserved even on errors");
 }
 reset();provider.result=k_EGCResultNoMessage;provider.writeOutputs=false;
}
unsigned debugMask=0;
std::vector<std::string> debugEvents;
bool DebugEnabled(steamworks::debug::Category category) { return (debugMask & category)!=0; }
void DebugWrite(steamworks::debug::Category,const char* event,const char* format,va_list args) {
 char details[512];std::vsnprintf(details,sizeof(details),format,args);
 debugEvents.push_back(std::string(event)+" "+details);
}
int main() {
 size_t count=99;
 Check(!steamworks::AuthTicketCellCount(-1,count) && count==99,"negative ticket length rejected without changing count");
 Check(!steamworks::AuthTicketCellCount(INT32_MIN,count) && count==99,"minimum signed ticket length rejected without conversion");
 for(int bytes=0;bytes<=9;++bytes) {
  Check(steamworks::AuthTicketCellCount(bytes,count) && count==static_cast<size_t>(bytes==0 ? 1 : (bytes+3)/4),"ticket layout rounds whole and partial cells, including empty input");
 }
 if(sizeof(void*)==4) {
  for(int bytes=INT32_MAX-7;bytes<=INT32_MAX-3;++bytes) {
   Check(steamworks::AuthTicketCellCount(bytes,count) && count==(static_cast<size_t>(bytes)+3)/4 && count*4<=static_cast<size_t>(PTRDIFF_MAX),"largest representable padded layouts accepted without allocating");
  }
  for(int delta=0;delta<3;++delta) {
   count=99;
   Check(!steamworks::AuthTicketCellCount(INT32_MAX-delta,count) && count==99,"x86 top three byte lengths reject padded object overflow without allocating");
  }
 }
 steamworks::debug::GetBackend()={nullptr,nullptr,DebugEnabled,DebugWrite};
 {
  SteamWorksGSHooks gs;SteamWorksGCHooks gc;g_SteamWorks.pGSHooks=&gs;g_SteamWorks.pGCHooks=&gc;
  Check(host.frames.size()==2 && hooks.empty(),"late load waits once per hook group");
  int added=host.frameAdds;gs.Start();gc.Start();host.Tick();Check(host.frameAdds==added,"repeat start/wait does not duplicate frame registrations");
  resolver.server=&server1;resolver.gc=&gc1;host.Tick();
  Check(hooks.size()==3 && host.frames.size()==1,"GS group waits for missing 014 instead of binding auth to available 015");
  resolver.auth=&auth1;host.Tick();
  Check(hooks.size()==6 && host.frames.empty(),"each group installs once when runtime appears and stops polling");
  for(auto entry:hooks) if(entry.second.method=="BeginAuthSession") Check(entry.second.target==&auth1,"auth hook targets 014 object rather than 015");
  int installed=creates;gs.Start();gc.Start();host.Tick();Check(creates==installed,"repeat successful startup does not duplicate hooks");
  auto* auth=forwards->owned["SteamWorks_BeginAuthSession"];
  Check(auth->types==std::vector<int>({Param_Array,Param_Cell,Param_Cell}),"auth forward registration and arity preserved");
  const unsigned char ticket[]={0x00,0x80,0xff,0x01,0x00};
  debugMask=steamworks::debug::Auth;
  gs.BeginAuthSession(ticket,sizeof(ticket),CSteamID(0xf1234567u,1,k_EUniversePublic,k_EAccountTypeIndividual));
  Check(debugEvents==std::vector<std::string>{"begin_auth_enter enabled=1 target=1 receivers=1 bytes=5","begin_auth_dispatch result=0 cells=2"},"auth debug records scalar flow without ticket or identity and preserves dispatch");
  debugMask=0;auto debugBefore=debugEvents;gs.TraceState();
  Check(debugEvents==debugBefore,"off suppresses both state categories");
  debugMask=steamworks::debug::Hooks;gs.TraceState();
  Check(debugEvents.size()==debugBefore.size()+1 && debugEvents.back().find("gameserver_state enabled=1 target=1") == 0,"enabling hook diagnostics exposes already installed state without reacquisition");
  debugMask=0;
  Check(auth->executions==1 && auth->array.size()==2 && auth->arrayFlags==0,"auth is packed aligned cells with no copyback");
  Check(std::memcmp(auth->array.data(),ticket,sizeof(ticket))==0 && reinterpret_cast<unsigned char*>(auth->array.data())[7]==0,"auth byte length copied exactly and padding zeroed");
  Check(auth->cells.size()==2 && auth->cells[0]==5 && static_cast<uint32>(auth->cells[1])==0xf1234567u,"auth byte count and AccountID bits unchanged");
  for(int fail=1;fail<=3;++fail){
   int executions=auth->executions,cancels=auth->cancels;auth->failPushAt=auth->pushes+fail;
   gs.BeginAuthSession(ticket,sizeof(ticket),CSteamID());
   Check(auth->executions==executions && auth->cancels==cancels+1 && meta==MRES_IGNORED,"auth partial parameter failure cancels dispatch and preserves original call");
  }
  auth->failPushAt=0;int executions=auth->executions;
  gs.BeginAuthSession(nullptr,1,CSteamID());gs.BeginAuthSession(ticket,-1,CSteamID());
  Check(auth->executions==executions,"invalid auth input does not dispatch");
  if(sizeof(void*)==4) {
   debugMask=steamworks::debug::Auth;
   for(int delta=0;delta<3;++delta) {
    const int pushes=auth->pushes,cancels=auth->cancels;
    const auto result=gs.BeginAuthSession(ticket,INT32_MAX-delta,CSteamID());
    Check(result==k_EBeginAuthSessionResultOK && meta==MRES_IGNORED,"oversized forward snapshot leaves engine auth decision unchanged");
    Check(auth->executions==executions && auth->pushes==pushes && auth->cancels==cancels,"oversized snapshot rejected before reading tiny source or pushing parameters");
    Check(debugEvents.back()=="begin_auth_skip reason=ticket_too_large","oversized snapshot has scalar-only diagnostic");
   }
   debugMask=0;
  }
  resolver.server=&server2;resolver.auth=&auth2;resolver.gc=&gc2;int reads=resolver.reads;
  gs.Stop();gc.Stop();gs.Stop();gc.Stop();
  Check(resolver.reads==reads && hooks.empty(),"stop is idempotent and never consults resolver");
  for(auto entry:removed)Check(entry.target==&server1 || entry.target==&gc1 || entry.target==&auth1,"removal refers to installed instance rather than replacement");
  gs.Start();gc.Start();Check(hooks.size()==6,"restart installs on replacement instances");
  for(auto entry:hooks)Check(entry.second.target==&server2 || entry.second.target==&gc2 || entry.second.target==&auth2,"replacement hook targets recorded");
  auto* token=forwards->owned["SteamWorks_TokenRequested"];
  token->action=[&]{gs.Stop();gs.Start();};int logons=server2.logons;gs.LogOnAnonymous();
  Check(server2.logons==logons,"stop/restart during token forward cannot call even same-address replacement generation");token->action={};
  token->consumers=0;gs.LogOnAnonymous();Check(meta==MRES_IGNORED,"zero token consumers preserve original LogOnAnonymous");token->consumers=1;
  auto* restart=forwards->owned["SteamWorks_RestartRequested"];restart->result=Pl_Handled;
  Check(gs.WasRestartRequested() && meta==MRES_SUPERCEDE,"restart forward result convention preserved");
  auto* send=forwards->owned["SteamWorks_GCSendMessage"];
  for(cell_t result:{0,-1,2}) {send->result=result;Check(gc.SendMessage(5,"x",1)==(result==-1 ? k_EGCResultOK : static_cast<EGCResults>(result)) && meta==(result==0?MRES_IGNORED:MRES_SUPERCEDE),"GC forward result and supercede convention preserved");}
  auto* receive=forwards->owned["SteamWorks_GCRetrieveMessage"];receive->result=0;uint32 type=1,size=0;char bytes[4]={};int receives=gc2.receives;
  Check(gc.RetrieveMessage(&type,bytes,sizeof(bytes),&size)==k_EGCResultNoMessage && gc2.receives==receives+1 && meta==MRES_SUPERCEDE,"GC retrieval calls hooked original once");
  CheckReceiveBuffers(gc,gc2,receive);
  int restartCalls=restart->executions;server2.action=[&]{gs.Stop();gs.Start();};gs.WasRestartRequested();server2.action={};
  Check(restart->executions==restartCalls,"reentrant runtime transition suppresses stale restart forward");
  size_t receiveCells=receive->cells.size();gc2.action=[&]{gc.Stop();gc.Start();};gc.RetrieveMessage(&type,bytes,sizeof(bytes),&size);gc2.action={};
  Check(receive->cells.size()==receiveCells,"reentrant runtime transition suppresses stale retrieval forward without extracting twice");
  gs.Stop();gc.Stop();resolver.server=nullptr;resolver.auth=nullptr;resolver.gc=nullptr;gs.Start();gc.Start();
  Check(host.frames.size()==2,"waiting resumes only after explicit new startup");
 }
 g_SteamWorks.pGSHooks=nullptr;g_SteamWorks.pGCHooks=nullptr;
 Check(host.frames.empty() && hooks.empty() && forwards->owned.empty(),"destruction while waiting removes subscriptions and forwards");
 {
  resolver.server=&server1;resolver.auth=&auth1;
  SteamWorksGSHooks owner;resolver.auth=&auth2;owner.Start();
  for(auto entry:hooks) Check(entry.second.method=="BeginAuthSession" ? entry.second.target==&auth2 : entry.second.target==&server1,"auth-only replacement rebinds even when native 015 pointer is unchanged");
 }
 Check(hooks.empty() && host.frames.empty(),"replacement group fully removed on destruction");
 for(int family=0;family<2;++family)for(int fail=1;fail<=3;++fail) {
  resolver.server=&server1;resolver.auth=&auth1;resolver.gc=&gc1;failAt=creates+fail;int before=errors;
  if(family==0){SteamWorksGSHooks owner;Check(hooks.empty() && host.frames.empty(),"partial GS installation rolled back without retry subscription");}
  else{SteamWorksGCHooks owner;Check(hooks.empty() && host.frames.empty(),"partial GC installation rolled back without retry subscription");}
  Check(errors==before+1,"one diagnostic per failed installation");failAt=0;
 }
 Check(hooks.empty() && host.frames.empty() && forwards->owned.empty(),"all partial paths clean");
 std::printf("PASS: %d hook lifecycle checks (production owners, simulated SourceHook/frames/SDK receiver)\n",checks);
 return 0;
}
