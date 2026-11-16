#define TEST_REAL_STEAM_CALLBACKS
#include "steam_callbacks_test_host.h"
#include "http_gc_test_host.h"
#include <type_traits>
static_assert(!std::is_copy_constructible<SteamWorksHTTPRequest>::value,"HTTP ownership cannot be copied");
static_assert(!std::is_copy_constructible<SteamWorksHTTP>::value,"HTTP manager cannot be copied");
int checks=0;
void Check(bool ok,const char* why) { ++checks;if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1); } }
IChangeableForward* MakeForward() { return forwards->CreateForwardEx(); }
SteamWorksHTTPRequest* NewRequest(SteamWorksHTTP& manager,HTTPRequestHandle id=17,IdentityToken_t* owner=nullptr) {
 auto* request=new SteamWorksHTTPRequest(&http,owner);request->request=id;
 request->handle=handlesys->CreateHandle(manager.GetHTTPHandle(),request,owner,myself->GetIdentity(),nullptr);
 requestObject=request;return request;
}
int main() {
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;
  Check(plugins.listeners.count(&manager)==1,"plugin unload listener owned by HTTP manager");
  auto* request=NewRequest(manager);auto handle=request->handle;
  request->pCompletedForward=MakeForward();request->pHeadersReceivedForward=MakeForward();request->pDataReceivedForward=MakeForward();
  SteamAPICall_t call=UINT64_C(0xfedcba9876543210);SetCallbacks(call,request);
  Check(steamCallbacks.calls.size()==1 && steamCallbacks.callbacks.size()==2,"completion uses call result, streaming uses regular SDK callbacks");
  for(auto entry:steamCallbacks.callbacks)Check(CCallbackMgr::GameServer(entry.first),"stream callbacks registered for gameserver");
  HTTPRequestHeadersReceived_t headers={};headers.m_hRequest=999;
  DeliverCallback(headers);Check(request->pHeadersReceivedForward->cells.empty(),"other request callback ignored");
  headers.m_hRequest=request->request;headers.m_ulContextValue=UINT64_C(0x80000000ffffffff);
  DeliverCallback(headers);Check(request->pHeadersReceivedForward->cells.size()==4,"headers forward retains four arguments");
  HTTPRequestDataReceived_t data={};data.m_hRequest=request->request;data.m_cOffset=3;data.m_cBytesReceived=8;
  DeliverCallback(data);Check(request->pDataReceivedForward->cells.size()==6 && request->pDataReceivedForward->cells[2]==3,"data forward retains offset/length and six arguments");
  HTTPRequestCompleted_t completed={};completed.m_hRequest=request->request;completed.m_bRequestSuccessful=true;completed.m_eStatusCode=k_EHTTPStatusCode200OK;
  DeliverCall(completed,call);Check(request->pCompletedForward->cells.size()==6 && steamCallbacks.calls.empty(),"completion preserves six arguments and SDK single delivery");
  int releases=http.releases;handlesys->Destroy(handle);
  Check(http.releases==releases+1 && steamCallbacks.callbacks.empty(),"close cancels subscriptions and releases one Steam request");
  Check(forwards->owned.empty(),"close releases all callback forwards");
  handlesys->Destroy(handle);Check(http.releases==releases+1,"repeated handle close does not release twice");
 }
 Check(plugins.listeners.empty(),"manager removes plugin listener");
 for(int kind=0;kind<3;++kind) {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;
  IChangeableForward* forward=MakeForward();
  if(kind==0)request->pCompletedForward=forward;else if(kind==1)request->pHeadersReceivedForward=forward;else request->pDataReceivedForward=forward;
  SteamAPICall_t call=19;SetCallbacks(call,request);int releases=http.releases;int fwdReleases=forwards->releases;
  forward->action=[&] {
   handlesys->Destroy(handle);
   Check(forwards->releases==fwdReleases,"closing inside callback retains executing forward until unwind");
   Check(!request->IsUsable() && http.releases==releases+1,"close immediately blocks work and releases Steam ownership");
  };
  if(kind==0) { HTTPRequestCompleted_t p={};p.m_hRequest=17;DeliverCall(p,call); }
  if(kind==1) { HTTPRequestHeadersReceived_t p={};p.m_hRequest=17;DeliverCallback(p); }
  if(kind==2) { HTTPRequestDataReceived_t p={};p.m_hRequest=17;DeliverCallback(p); }
  Check(forwards->releases==fwdReleases+1 && steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty(),"callback unwinds, deletes receiver and cancels remaining registrations");
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;
  auto* forward=MakeForward();request->pCompletedForward=forward;SteamAPICall_t call=55;SetCallbacks(call,request);
  auto generation=manager.Generation();int releases=http.releases;
  manager.StopRuntime();manager.StopRuntime();
  Check(!manager.IsAccepting() && !request->IsUsable() && manager.Generation()==generation+1,"repeated runtime stop is idempotent");
  Check(http.releases==releases+1 && steamCallbacks.calls.empty() && forwards->owned.empty(),"shutdown cancels and releases before runtime goes away");
  manager.StartRuntime();Check(manager.IsAccepting() && !request->IsUsable(),"restart never revives an old request");
  HTTPRequestCompleted_t stale={};stale.m_hRequest=17;request->OnHTTPRequestCompleted(&stale,false);
  Check(forwards->owned.empty(),"late event with live old receiver is discarded");
  handlesys->Destroy(handle);Check(http.releases==releases+1,"closing invalidated handle never reacquires/releases a replacement interface");
  auto* fresh=NewRequest(manager,18);Check(fresh->IsUsable(),"new generation accepts new requests");handlesys->Destroy(fresh->handle);
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;
  auto* request=NewRequest(manager,17,plugin.GetIdentity());auto handle=request->handle;
  request->pCompletedForward=MakeForward();request->pCompletedForward->targetIdentity=plugin.GetIdentity();
  SteamAPICall_t call=63;SetCallbacks(call,request);int releases=http.releases;
  request->pCompletedForward->action=[&] { manager.OnPluginWillUnload(&plugin);handlesys->Destroy(handle);Check(http.releases==releases+1,"owner unload inside callback releases once"); };
  HTTPRequestCompleted_t p={};p.m_hRequest=17;DeliverCall(p,call);
  Check(steamCallbacks.calls.empty() && forwards->owned.empty(),"owner unload leaves no callback receiver or forward");
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;
  auto* request=NewRequest(manager);auto handle=request->handle;
  request->pCompletedForward=MakeForward();request->pCompletedForward->targetIdentity=plugin.GetIdentity();
  manager.OnPluginWillUnload(&plugin);
  Check(request->IsUsable() && request->pCompletedForward->GetFunctionCount()==0,"unloading callback target removes its functions without closing another owner's request");
  handlesys->Destroy(handle);
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;IPluginContext context;
  handlesys->failCreate=true;cell_t params[3]={2,k_EHTTPMethodGET,1};int releases=http.releases;
  Check(sm_CreateHTTPRequest(&context,params)==BAD_HANDLE && http.releases==releases+1,"SourceMod handle creation failure releases the Steam handle once");handlesys->failCreate=false;
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;
  plugin.context.function.action=[&]{handlesys->Destroy(handle);};cell_t params[5]={4,static_cast<cell_t>(handle),1,0,0};IPluginContext context;int releases=http.releases;
  Check(sm_GetHTTPResponseBodyCallback(&context,params)==1 && http.releases==releases+1,"synchronous body callback can close request safely");plugin.context.function.action={};
 }
 {
  auto* manager=new SteamWorksHTTP;g_SteamWorks.pSWHTTP=manager;auto* request=NewRequest(*manager);request->pCompletedForward=MakeForward();SteamAPICall_t call=72;SetCallbacks(call,request);int releases=http.releases;
  delete manager;g_SteamWorks.pSWHTTP=nullptr;
  Check(handlesys->objects.empty() && steamCallbacks.calls.empty() && plugins.listeners.empty() && http.releases==releases+1,"manager teardown removes type and drains requests synchronously");
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;
  request->pCompletedForward=MakeForward();request->pHeadersReceivedForward=MakeForward();SteamAPICall_t call=81;SetCallbacks(call,request);int releases=forwards->releases;
  request->pHeadersReceivedForward->action=[&]{handlesys->Destroy(handle);};
  request->pCompletedForward->action=[&]{HTTPRequestHeadersReceived_t h={};h.m_hRequest=17;DeliverCallback(h);Check(!request->IsUsable() && forwards->releases==releases,"nested callback close keeps receiver and both forwards until outermost unwind");};
  HTTPRequestCompleted_t p={};p.m_hRequest=17;DeliverCall(p,call);
  Check(forwards->releases==releases+2 && steamCallbacks.callbacks.empty(),"nested callback resources released once after outer return");
 }
 {
  handlesys->failType=true;SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;
  Check(!manager.IsAccepting() && manager.GetHTTPHandle()==0,"failed type creation blocks native requests");handlesys->failType=false;
 }
 for(int cycle=0;cycle<50;++cycle) {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;
  request->pCompletedForward=MakeForward();request->pDataReceivedForward=MakeForward();SteamAPICall_t call=90;SetCallbacks(call,request);
  if(cycle%2==0) { manager.StopRuntime(); }
  handlesys->Destroy(handle);
  Check(steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty() && forwards->owned.empty(),"repeated request/stop/close retains no registrations or forwards");
 }
 for(int failure=1;failure<=3;++failure) {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;
  cell_t params[6]={5,static_cast<cell_t>(handle),1,2,3,0};IPluginContext context;
  forwards->failAt=forwards->creates+failure;int errors=context.errors;
  Check(sm_SetCallbacks(&context,params)==0 && context.errors==errors+1 && forwards->owned.empty(),"partial callback forward allocation rolls back all new resources");
  Check(request->pCompletedForward==nullptr && request->pHeadersReceivedForward==nullptr && request->pDataReceivedForward==nullptr,"failed configuration leaves bindings unchanged");
  forwards->failAt=0;Check(sm_SetCallbacks(&context,params)==1 && forwards->owned.size()==3,"configuration can retry after allocation failure");
  handlesys->Destroy(handle);
 }
 {
  SteamWorksHTTP manager;g_SteamWorks.pSWHTTP=&manager;auto* request=NewRequest(manager);auto handle=request->handle;IPluginContext context;
  cell_t params[6]={5,static_cast<cell_t>(handle),1,2,3,0};plugin.context.invalidFunction=true;int creates=forwards->creates;
  Check(sm_SetCallbacks(&context,params)==0 && forwards->creates==creates,"invalid callback checked before allocation");plugin.context.invalidFunction=false;handlesys->Destroy(handle);
 }
 Check(forwards->releasedDuringExecution==0 && forwards->owned.empty(),"no forward freed while executing and no retained allocation");
 Check(steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty(),"no registration points to destroyed receivers");
 std::printf("PASS: %d HTTP lifecycle checks (production manager/request, real SDK callback classes, simulated host)\n",checks);
}
