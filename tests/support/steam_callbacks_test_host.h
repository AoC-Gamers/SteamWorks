#pragma once
// Real SDK callback classes; only the Steam registration/dispatch service is simulated.
#include "steam_gameserver.h"
#include <map>
#include <vector>
#include <utility>
struct SteamCallbackRegistry {
 std::map<CCallbackBase*,int> callbacks;
 std::map<CCallbackBase*,SteamAPICall_t> calls;
 int registrations=0,unregistrations=0;
} steamCallbacks;
class CCallbackMgr {
public:
 static void Register(CCallbackBase* callback,int id) { callback->m_nCallbackFlags|=1; callback->m_iCallback=id; }
 static void Unregister(CCallbackBase* callback) { callback->m_nCallbackFlags&=~1; }
 static bool GameServer(CCallbackBase* callback) { return (callback->m_nCallbackFlags&2)!=0; }
};
S_API void S_CALLTYPE SteamAPI_RegisterCallback(CCallbackBase* callback,int id) {
 CCallbackMgr::Register(callback,id);steamCallbacks.callbacks[callback]=id;++steamCallbacks.registrations;
}
S_API void S_CALLTYPE SteamAPI_UnregisterCallback(CCallbackBase* callback) {
 CCallbackMgr::Unregister(callback);if(steamCallbacks.callbacks.erase(callback))++steamCallbacks.unregistrations;
}
S_API void S_CALLTYPE SteamAPI_RegisterCallResult(CCallbackBase* callback,SteamAPICall_t call) {
 steamCallbacks.calls[callback]=call;++steamCallbacks.registrations;
}
S_API void S_CALLTYPE SteamAPI_UnregisterCallResult(CCallbackBase* callback,SteamAPICall_t) {
 if(steamCallbacks.calls.erase(callback))++steamCallbacks.unregistrations;
}
enum class SteamCallbackContext { Client, GameServer };
template<class P> void DeliverCallback(P& payload, SteamCallbackContext context = SteamCallbackContext::GameServer) {
 auto snapshot=steamCallbacks.callbacks;
 for(auto entry:snapshot) {
  if(entry.second==P::k_iCallback && steamCallbacks.callbacks.count(entry.first)
     && CCallbackMgr::GameServer(entry.first) == (context == SteamCallbackContext::GameServer)) {
   entry.first->Run(&payload);
  }
 }
}
template<class P> void DeliverCall(P& payload,SteamAPICall_t call,bool failed=false) {
 auto snapshot=steamCallbacks.calls;
 for(auto entry:snapshot) if(entry.first->GetICallback()==P::k_iCallback && entry.second==call && steamCallbacks.calls.count(entry.first)) {
  steamCallbacks.calls.erase(entry.first); // SDK caller unregisters before invoking Run.
  entry.first->Run(&payload,failed,call);
 }
}
