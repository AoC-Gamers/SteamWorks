#pragma once
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include "steam_gameserver.h"
#include "isteamgamecoordinator.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <functional>
using cell_t=int32_t;
using IdentityToken_t=void;
struct IPlugin;
struct IPluginsListener { virtual ~IPluginsListener()=default; virtual void OnPluginWillUnload(IPlugin*) {} };
struct IHandleTypeDispatch {
 virtual ~IHandleTypeDispatch()=default;
 virtual void OnHandleDestroy(uint32_t,void*)=0;
 virtual bool GetHandleApproxSize(uint32_t,void*,unsigned int*)=0;
};
using Handle_t=uint32_t; using HandleType_t=uint32_t; using HandleError=int;
constexpr int SP_ERROR_NONE=0, HandleError_None=0, Path_Game=0, ET_Ignore=0, Param_Cell=0;
constexpr int PLATFORM_MAX_PATH=1024, SM_PARAM_STRING_BINARY=1, SM_PARAM_STRING_COPY=2;
constexpr Handle_t BAD_HANDLE=0;
struct IPluginFunction {
 std::vector<cell_t> cells;
 std::function<void()> action; int executionDepth=0;
 void PushCell(cell_t value) { cells.push_back(value); }
 void PushStringEx(const char*,size_t,int,int) {}
 void Execute(cell_t*) { ++executionDepth; auto invoke=action; if(invoke) invoke(); --executionDepth; }
};
struct IChangeableForward : IPluginFunction {
 int consumers=1; IdentityToken_t* targetIdentity=nullptr;
 int GetFunctionCount() { return consumers; } void AddFunction(IPluginFunction*) {}
 void RemoveFunctionsOfPlugin(IPlugin* plugin);
};
struct IPluginContext {
 bool invalidFunction=false; bool invalidString=false,invalidOutput=false; int errors=0; int resolutions=0;
 cell_t storage[3]={0x11223344,0x12345678,0x55667788}; char data[64]="test";
 IPluginFunction function;
 std::map<cell_t,char*> strings;std::map<cell_t,cell_t*> outputs;
 int LocalToString(cell_t address,char** out) { ++resolutions; if(invalidString)return 42; auto i=strings.find(address);*out=i==strings.end()?data:i->second; return 0; }
 int LocalToPhysAddr(cell_t address,cell_t** out) { ++resolutions; if(invalidOutput)return 42; auto i=outputs.find(address);*out=i==outputs.end()?&storage[1]:i->second; return 0; }
 cell_t ThrowNativeError(const char*,...) { ++errors; return 0; }
 cell_t ThrowNativeErrorEx(int,const char*,...) { ++errors; return 0; }
 void* GetIdentity() { return this; } void* GetContext() { return this; }
 IPluginFunction* GetFunctionById(cell_t) { return invalidFunction ? nullptr : &function; }
};
struct IPlugin { IPluginContext context; IPluginContext* GetBaseContext() { return &context; } IdentityToken_t* GetIdentity() { return context.GetIdentity(); } } plugin;
inline void IChangeableForward::RemoveFunctionsOfPlugin(IPlugin* target) {
 if (targetIdentity==target->GetIdentity()) { consumers=0; action={}; }
}
struct Plugins {
 std::set<IPluginsListener*> listeners;
 void AddPluginsListener(IPluginsListener* listener) { listeners.insert(listener); }
 void RemovePluginsListener(IPluginsListener* listener) { listeners.erase(listener); }
 IPlugin* FindPluginByContext(void*) { return &plugin; } IPlugin* PluginFromHandle(Handle_t,HandleError*) { return &plugin; } } plugins;
auto* plsys=&plugins;
struct ForwardSystem {
 int creates=0,failAt=0; int releases=0; int releasedDuringExecution=0;
 std::set<IChangeableForward*> owned;
 void ReleaseForward(IChangeableForward* forward) {
  ++releases; if(forward->executionDepth) ++releasedDuringExecution;
  if(owned.erase(forward)) delete forward;
 }
 template<class... Args> IChangeableForward* CreateForwardEx(Args...) { ++creates;if(creates==failAt)return nullptr; auto* f=new IChangeableForward; owned.insert(f); return f; }
} forwardSystem;
auto* forwards=&forwardSystem;
#ifndef TEST_REAL_STEAM_CALLBACKS
template<class O,class P> struct TestCallResult {
 SteamAPICall_t call=0;
 void SetGameserverFlag() {} void Set(SteamAPICall_t value,O*,void(O::*)(P*,bool)) { call=value; }
 void Cancel() { call=0; }
};
#define CCallResult TestCallResult
template<class O,class P,bool> struct TestManualCallback {
 void Register(O*,void(O::*)(P*)) {} void Unregister() {}
};
#define CCallbackManual TestManualCallback
#endif
struct HandleSecurity { HandleSecurity(void*,void*) {} };
void* requestObject=nullptr;
struct Handles {
 bool invalid=false; bool failCreate=false; bool failType=false; int reads=0;
 IHandleTypeDispatch* dispatch=nullptr; uint32_t nextHandle=100;
 std::map<Handle_t,void*> objects;
 HandleType_t CreateType(const char*,IHandleTypeDispatch* d,HandleType_t,void*,void*,IdentityToken_t*,void*) { dispatch=d; return failType ? 0 : 1; }
 void Destroy(Handle_t handle) { auto i=objects.find(handle); if(i!=objects.end()) { void* object=i->second; objects.erase(i); dispatch->OnHandleDestroy(1,object); } }
 void RemoveType(HandleType_t,IdentityToken_t*) { while(!objects.empty()) Destroy(objects.begin()->first); dispatch=nullptr; }
 HandleError ReadHandle(Handle_t,HandleType_t,HandleSecurity*,void** out) { ++reads; if(invalid)return 9; auto i=objects.find(1); (void)i; *out=requestObject;return 0; }
 Handle_t CreateHandle(HandleType_t,void* object,IdentityToken_t*,IdentityToken_t*,void*) { if(failCreate)return BAD_HANDLE; auto handle=nextHandle++; objects[handle]=object; return handle; }
} handleSystem;
auto* handlesys=&handleSystem;
struct Extension { void* GetIdentity() { return this; } } extension;
auto* myself=&extension;
struct Host { void BuildPath(int,char* out,size_t length,const char*,const char* path) { std::snprintf(out,length,"%s",path); } } host;
auto* g_pSM=&host; auto* smutils=&host;
using Native=cell_t(*)(IPluginContext*,const cell_t*);
struct sp_nativeinfo_t { const char* name; Native function; };
struct ShareSystem { void AddNatives(void*,sp_nativeinfo_t*) {} } shareSystem;
auto* sharesys=&shareSystem;

struct TestHTTP : ISteamHTTP {
 int calls=0; bool succeeds=true; bool sizeSucceeds=true,bodySucceeds=true; int bodyReads=0;std::vector<uint8> body; int releases=0; std::vector<HTTPRequestHandle> releasedHandles; uint32 size=4; uint32 lastLength=0; uint32 lastOffset=0; uint64 context=0; bool timeout=false;
 HTTPRequestHandle CreateHTTPRequest(EHTTPMethod, const char *) override { ++calls;  return 1; }
 bool SetHTTPRequestContextValue(HTTPRequestHandle, uint64 ulContextValue) override { ++calls; context=ulContextValue; return succeeds; }
 bool SetHTTPRequestNetworkActivityTimeout(HTTPRequestHandle, uint32 unTimeoutSeconds) override { ++calls; lastLength=unTimeoutSeconds; return succeeds; }
 bool SetHTTPRequestHeaderValue(HTTPRequestHandle, const char *, const char *) override { ++calls;  return succeeds; }
 bool SetHTTPRequestGetOrPostParameter(HTTPRequestHandle, const char *, const char *) override { ++calls;  return succeeds; }
 bool SendHTTPRequest(HTTPRequestHandle, SteamAPICall_t *call) override { ++calls; *call=UINT64_C(0xfedcba9876543210); return succeeds; }
 bool SendHTTPRequestAndStreamResponse(HTTPRequestHandle, SteamAPICall_t *) override { ++calls;  return succeeds; }
 bool DeferHTTPRequest(HTTPRequestHandle) override { ++calls;  return succeeds; }
 bool PrioritizeHTTPRequest(HTTPRequestHandle) override { ++calls;  return succeeds; }
 bool GetHTTPResponseHeaderSize(HTTPRequestHandle, const char *, uint32 *unResponseHeaderSize) override { ++calls; *unResponseHeaderSize=size; return succeeds; }
 bool GetHTTPResponseHeaderValue(HTTPRequestHandle, const char *, uint8 *, uint32 unBufferSize) override { ++calls; lastLength=unBufferSize; return succeeds; }
 bool GetHTTPResponseBodySize(HTTPRequestHandle, uint32 *unBodySize) override { ++calls; *unBodySize=size; return succeeds && sizeSucceeds; }
 bool GetHTTPResponseBodyData(HTTPRequestHandle, uint8* dest, uint32 unBufferSize) override {
  ++calls;++bodyReads;lastLength=unBufferSize;
  if(unBufferSize!=size || dest==nullptr || (!body.empty() && body.size()!=size))return false;
  if(!succeeds || !bodySucceeds){if(size!=0)dest[0]=0xee;return false;}
  for(uint32 i=0;i<size;++i)dest[i]=body.empty()?static_cast<uint8>('B'):body[i];
  return true;
 }
 bool GetHTTPStreamingResponseBodyData(HTTPRequestHandle, uint32 cOffset, uint8 *, uint32 unBufferSize) override { ++calls; lastLength=unBufferSize; lastOffset=cOffset; return succeeds; }
 bool ReleaseHTTPRequest(HTTPRequestHandle handle) override { ++calls; ++releases; releasedHandles.push_back(handle); return succeeds; }
 bool GetHTTPDownloadProgressPct(HTTPRequestHandle, float *pflPercentOut) override { ++calls; *pflPercentOut=42.5f; return succeeds; }
 bool SetHTTPRequestRawPostBody(HTTPRequestHandle, const char *, uint8 *, uint32 unBodyLen) override { ++calls; lastLength=unBodyLen; return succeeds; }
 HTTPCookieContainerHandle CreateCookieContainer(bool) override { ++calls;  return 1; }
 bool ReleaseCookieContainer(HTTPCookieContainerHandle) override { ++calls;  return succeeds; }
 bool SetCookie(HTTPCookieContainerHandle, const char *, const char *, const char *) override { ++calls;  return succeeds; }
 bool SetHTTPRequestCookieContainer(HTTPRequestHandle, HTTPCookieContainerHandle) override { ++calls;  return succeeds; }
 bool SetHTTPRequestUserAgentInfo(HTTPRequestHandle, const char *) override { ++calls;  return succeeds; }
 bool SetHTTPRequestRequiresVerifiedCertificate(HTTPRequestHandle, bool) override { ++calls;  return succeeds; }
 bool SetHTTPRequestAbsoluteTimeoutMS(HTTPRequestHandle, uint32 unMilliseconds) override { ++calls; lastLength=unMilliseconds; return succeeds; }
 bool GetHTTPRequestWasTimedOut(HTTPRequestHandle, bool *pbWasTimedOut) override { ++calls; *pbWasTimedOut=timeout; return succeeds; }
} http;
struct TestGC : ISteamGameCoordinator {
 int calls=0; uint32 message=0,size=0;
 EGCResults SendMessage(uint32 type,const void*,uint32 length) override { ++calls;message=type;size=length;return k_EGCResultOK; }
 bool IsMessageAvailable(uint32*) override { return false; }
 EGCResults RetrieveMessage(uint32*,void*,uint32,uint32*) override { return k_EGCResultNoMessage; }
} gc;
struct GameServer {
 bool available=true;
 ISteamHTTP* GetHTTP() { return available ? &http : nullptr; }
 ISteamGameCoordinator* GetGameCoordinator() { return available ? &gc : nullptr; }
} gameServer;
#include "../../extension/http/swhttp.h"
struct SteamWorks { GameServer* pSWGameServer=&gameServer; SteamWorksHTTP* pSWHTTP=nullptr; } g_SteamWorks;
#ifndef HTTP_REQUEST_SOURCE
#define HTTP_REQUEST_SOURCE "../../extension/http/swhttprequest.cpp"
#endif
#include HTTP_REQUEST_SOURCE
#include "../../extension/http/swhttp.cpp"
#include "../../extension/natives/gcnatives.cpp"
