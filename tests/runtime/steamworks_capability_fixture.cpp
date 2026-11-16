// Laboratory-only typed probes; pointers come from audited candidate getters.
#include "smsdk_ext.h"
#include "steam_gameserver.h"
#include "isteamgamecoordinator.h"
#include "isteamhttp.h"
#include "isteammatchmaking.h"
#include "isteamnetworking.h"
#include "isteamgameserverstats.h"
#include "swgameauth014.h"
#include <cstring>

namespace {
cell_t Probe(IPluginContext* ctx, const cell_t* p) {
 void* pointer = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(p[2])));
 if (!pointer) return 0;
 bool ok = false;
 switch (p[1]) {
  case 0: { auto* v = static_cast<ISteamClient*>(pointer); (void)v->GetIPCCallCount(); ok = true; break; }
  case 1: ok = static_cast<ISteamGameServer*>(pointer)->BLoggedOn(); break;
  case 2: { auto* v = static_cast<ISteamUtils*>(pointer); ok = v->GetAppID() == 550 && v->GetConnectedUniverse() == k_EUniversePublic; break; }
  case 3: { uint32 size=0; (void)static_cast<ISteamNetworking*>(pointer)->IsP2PPacketAvailable(&size, 0); ok=true; break; }
  case 4: { int32 value=0x13572468; ok = !static_cast<ISteamGameServerStats*>(pointer)->GetUserStat(CSteamID(), "__steamworks_capability_missing__", &value); break; }
  case 5: { auto* v=static_cast<ISteamHTTP*>(pointer); HTTPRequestHandle h=v->CreateHTTPRequest(k_EHTTPMethodGET,"http://127.0.0.1:27880/text"); ok=h!=INVALID_HTTPREQUEST_HANDLE && v->ReleaseHTTPRequest(h); break; }
  case 6: ok = static_cast<ISteamMatchmaking*>(pointer)->GetFavoriteGameCount() >= 0; break;
  case 7: { auto* v=static_cast<ISteamGameCoordinator*>(pointer); uint32 size=0; bool queued=v->IsMessageAvailable(&size); EGCResults result=k_EGCResultOK;
   // Never consume real queued data. Retrieval is exercised only on an empty queue.
   if (!queued) { uint32 type=0; unsigned char bytes[16]={}; result=v->RetrieveMessage(&type,bytes,sizeof(bytes),&size); }
   smutils->LogMessage(myself,"[SW-CAP] gc queued=%d retrieval_attempted=%d retrieval_result=%d",queued,!queued,static_cast<int>(result));
   ok=queued || result==k_EGCResultNoMessage; break;
  }
  case 8: ok = static_cast<steamworks::ISteamGameServer014*>(pointer)->BLoggedOn(); break;
  default: return ctx->ThrowNativeError("Unknown capability fixture kind");
 }
 smutils->LogMessage(myself,"[SW-CAP] typed_probe kind=%d pass=%d",p[1],ok);
 return ok;
}
const sp_nativeinfo_t natives[]={{"SWCapabilityFixture_Probe",Probe},{nullptr,nullptr}};
class Fixture final:public SDKExtension {
 bool SDK_OnLoad(char*,size_t,bool) override {sharesys->AddNatives(myself,natives);return true;}
} fixture;
}
SMEXT_LINK(&fixture);
