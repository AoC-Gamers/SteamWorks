// Laboratory only. The host audits the production getter and resolver slot.
// The real-queue path is read-only. The local-provider path temporarily uses
// the candidate's AddHooks API, then restores the actual GC before returning.
// No forward replacement or GC send is performed.
#include "smsdk_ext.h"
#include "isteamgamecoordinator.h"
#include <algorithm>
#include <array>
#include <cstring>
namespace {
cell_t Retrieve(IPluginContext* ctx, const cell_t* p) {
 auto* gc = reinterpret_cast<ISteamGameCoordinator*>(static_cast<uintptr_t>(static_cast<uint32>(p[1])));
 if (!gc || p[2] < 0 || p[2] > 4096) return ctx->ThrowNativeError("Invalid laboratory GC arguments");
 uint32 queuedSize=0;
 if (gc->IsMessageAvailable(&queuedSize)) {
  smutils->LogMessage(myself,"[SW-BUFFER] GC skipped_real_queue=1 consumed=0");
  return -100; // Refuse to consume application messages.
 }
 std::array<unsigned char,4104> bytes; bytes.fill(0xa5);
 uint32 type=0x13572468, size=0x24681357;
 EGCResults result=gc->RetrieveMessage(&type,bytes.data()+4,static_cast<uint32>(p[2]),&size);
 bool unchanged=std::all_of(bytes.begin(),bytes.end(),[](unsigned char b){return b==0xa5;});
 smutils->LogMessage(myself,"[SW-BUFFER] GC capacity=%d result=%d buffer_unchanged=%d type_sentinel=%d size_sentinel=%d",p[2],static_cast<int>(result),unchanged,type==0x13572468,size==0x24681357);
 return unchanged ? static_cast<cell_t>(result) : -101;
}
// A separate local provider exercises success marshalling through the candidate's
// own hook and the real SourcePawn VM. It is never registered with Steam.
class LocalGC final: public ISteamGameCoordinator {
public:
 int scenario=0, calls=0;
 EGCResults SendMessage(uint32,const void*,uint32) override { return k_EGCResultInvalidMessage; }
 bool IsMessageAvailable(uint32* size) override { *size=0; return false; }
 EGCResults RetrieveMessage(uint32* type,void* dest,uint32 cap,uint32* size) override {
  ++calls;
  if(scenario==3) return k_EGCResultNoMessage; // Deliberately leave outputs alone.
  if(scenario==4) { *size=cap+1; return k_EGCResultBufferTooSmall; }
  *type=0x80000042u; *size=scenario==1 ? cap : (scenario==2 ? 0u : 3u);
  for(uint32 i=0;i<*size;++i) static_cast<unsigned char*>(dest)[i]=scenario==1 ? static_cast<unsigned char>(0x80u+i) : (i==0 ? 0 : (i==1 ? 0x80 : 0xff));
  return k_EGCResultOK;
 }
} localGC;
ISteamGameCoordinator* volatile localTarget=&localGC; // Force the real virtual dispatch.
cell_t Local(IPluginContext* ctx,const cell_t* p) {
 auto* real=reinterpret_cast<ISteamGameCoordinator*>(static_cast<uintptr_t>(static_cast<uint32>(p[1])));
 auto* hooks=reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32>(p[2])));
 using Attach=void(*)(void*,ISteamGameCoordinator*);
 auto attach=reinterpret_cast<Attach>(static_cast<uintptr_t>(static_cast<uint32>(p[3])));
 int scenario=p[4];
 if(!real || !hooks || !attach || scenario<0 || scenario>4) return ctx->ThrowNativeError("Invalid local GC fixture binding");
 const uint32 cap=scenario==0 ? 4096u : (scenario==2 ? 0u : 16u);
 std::array<unsigned char,4104> bytes;bytes.fill(0xa5);
 uint32 type=0x13572468,size=0x24681357;
 localGC.scenario=scenario;localGC.calls=0;
 attach(hooks,&localGC);
 EGCResults result=localTarget->RetrieveMessage(&type,bytes.data()+4,cap,&size);
 // Restore the production target before returning to Pawn/the game frame.
 attach(hooks,real);
 bool ok=localGC.calls==1;
 uint32 valid=scenario<3 ? (scenario==0 ? 3u : (scenario==1 ? cap : 0u)) : 0u;
 for(size_t i=0;i<bytes.size();++i) {
  unsigned char expected=0xa5;
  if(i>=4 && i<4+valid) { size_t j=i-4; expected=scenario==1 ? static_cast<unsigned char>(0x80u+j) : (j==0 ? 0 : (j==1 ? 0x80 : 0xff)); }
  ok=ok && bytes[i]==expected;
 }
 const uint32 expectedType=scenario<3 ? 0x80000042u : 0x13572468u;
 const uint32 expectedSize=scenario==3 ? 0x24681357u : (scenario==4 ? cap+1 : valid);
 ok=ok && type==expectedType && size==expectedSize;
 smutils->LogMessage(myself,"[SW-BUFFER] GC_local scenario=%d capacity=%u result=%d original_calls=%d provider_bytes_and_outputs_preserved=%d real_target_restored=1",scenario,cap,static_cast<int>(result),localGC.calls,ok);
 return ok ? static_cast<cell_t>(result) : -101;
}
const sp_nativeinfo_t natives[]={{"SWBufferFixture_Retrieve",Retrieve},{"SWBufferFixture_Local",Local},{nullptr,nullptr}};
class Fixture final:public SDKExtension {
 bool SDK_OnLoad(char*,size_t,bool) override {sharesys->AddNatives(myself,natives);return true;}
} fixture;
}
SMEXT_LINK(&fixture);
