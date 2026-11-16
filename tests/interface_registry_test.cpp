// Actual resolver + real SDK declarations; service calls are simulated, not ABI acceptance.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include "steam_gameserver.h"
#include "isteamgamecoordinator.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include "stubs/auth_server014.h"

struct ResolverClient
{
 int calls = 0; bool available = true; std::string requested;
 template<class T> T* Resolve(const char* version) {
  ++calls; requested = version;
  return available ? reinterpret_cast<T*>(this) : nullptr; // Never dereferenced.
 }
 ISteamGameServer* GetISteamGameServer(HSteamUser,HSteamPipe,const char* v) { return Resolve<ISteamGameServer>(v); }
 ISteamUtils* GetISteamUtils(HSteamPipe,const char* v) { return Resolve<ISteamUtils>(v); }
 ISteamNetworking* GetISteamNetworking(HSteamUser,HSteamPipe,const char* v) { return Resolve<ISteamNetworking>(v); }
 ISteamGameServerStats* GetISteamGameServerStats(HSteamUser,HSteamPipe,const char* v) { return Resolve<ISteamGameServerStats>(v); }
 ISteamHTTP* GetISteamHTTP(HSteamUser,HSteamPipe,const char* v) { return Resolve<ISteamHTTP>(v); }
 ISteamMatchmaking* GetISteamMatchmaking(HSteamUser,HSteamPipe,const char* v) { return Resolve<ISteamMatchmaking>(v); }
 void* GetISteamGenericInterface(HSteamUser,HSteamPipe,const char* v) { return Resolve<void>(v); }
} client;
#define ISteamClient ResolverClient
#define g_pSteamClientGameServer resolverGlobal
#define SteamGameServer_GetHSteamUser ResolverUser
#define SteamGameServer_GetHSteamPipe ResolverPipe
extern "C" { ResolverClient* resolverGlobal = &client; }
int pipeCalls = 0;
HSteamUser authUser=7;
HSteamUser ResolverUser() { ++pipeCalls; return authUser; }
int authFactoryCalls=0;std::string authVersion;AuthServer014Stub authToken;bool authAvailable=true;
void* ResolverAuth(HSteamUser,const char* version) { ++authFactoryCalls;authVersion=version;return authAvailable?&authToken:nullptr; }
#define SteamInternal_FindOrCreateGameServerInterface ResolverAuth
HSteamPipe ResolverPipe() { ++pipeCalls; return 9; }
struct IGameConfig {
 std::map<std::string,std::string> keys;
 const char* GetKeyValue(const char* key) { auto i=keys.find(key); return i == keys.end() ? nullptr : i->second.c_str(); }
 bool GetMemSig(const char*,void**) { return false; }
} config;
struct GameData { IGameConfig* GetGameData() { return &config; } } gameData;
struct SteamWorks { GameData* pSWGameData = &gameData; bool available=true; bool IsRuntimeAvailable() const { return available; } } g_SteamWorks;
struct Host { int errors = 0; void LogError(void*,const char*,...) { ++errors; } } host;
auto* g_pSM = &host; void* myself = nullptr;
int factoryCalls = 0; std::string factoryVersion;
void* Factory(const char* v) { ++factoryCalls; factoryVersion=v; return &client; }
struct ILibrary { void* GetSymbolAddress(const char*) { return reinterpret_cast<void*>(&Factory); } void CloseLibrary() {} } library;
struct Libraries { ILibrary* OpenLibrary(const char*,char*,size_t) { return &library; } } libraries;
auto* libsys = &libraries;
#define POSIX
#include "../extension/core/swgameserver.cpp"

int checks = 0;
void Check(bool ok,const char* message) { ++checks; if (!ok) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); } }
template<class T> void Family(T* (SteamWorksGameServer::*getter)()) {
 auto record=steamworks::InterfaceContract<T>::Record();
 SteamWorksGameServer server;
 config.keys.clear(); client.calls=0; host.errors=0; pipeCalls=0;
 Check((server.*getter)()!=nullptr,"default version resolves");
 Check(client.calls==1 && client.requested==record.version,"exact registered version requested");
 g_SteamWorks.available=false;int suspendedCalls=client.calls;
 Check((server.*getter)()==nullptr && client.calls==suspendedCalls,"stopped runtime hides even cached interfaces");
 g_SteamWorks.available=true;
 config.keys[record.configKey]="unapproved-after-cache";
 Check((server.*getter)()!=nullptr && client.calls==1,"successful acquisition cached until reset");
 for (const char* invalid : {"", "unknown", "SteamClient020", "SteamHTTP002", "SteamUtils999"}) {
  server.Reset(); config.keys[record.configKey]=invalid; int calls=client.calls; int pipes=pipeCalls; int factories=factoryCalls; int errors=host.errors;
  resolverGlobal=nullptr;
  Check((server.*getter)()==nullptr,"unknown override refused");
  Check(client.calls==calls && pipeCalls==pipes && factoryCalls==factories,"refused before client factory/getter/user/pipe");
  Check(host.errors==errors+1,"one rejection diagnostic");
  config.keys[record.configKey]=record.version;
  Check((server.*getter)()==nullptr && host.errors==errors+1,"rejection sticky until reset, no log flood");
  resolverGlobal=&client;
 }
 server.Reset(); config.keys[record.configKey]=record.version; client.available=false;
 int calls=client.calls;
 Check((server.*getter)()==nullptr && client.calls==calls+1,"runtime temporarily unavailable");
 client.available=true;
 Check((server.*getter)()!=nullptr && client.calls==calls+2,"retry transient absence using approved version");
 Check(!steamworks::AcceptsVersion<T>(nullptr),"null version rejected");
}
int main() {
 Family(&SteamWorksGameServer::GetGameServer); Family(&SteamWorksGameServer::GetUtils);
 Family(&SteamWorksGameServer::GetNetworking); Family(&SteamWorksGameServer::GetServerStats);
 Family(&SteamWorksGameServer::GetHTTP); Family(&SteamWorksGameServer::GetMatchmaking);
 Family(&SteamWorksGameServer::GetGameCoordinator);
 {
  int rawCalls=authFactoryCalls;
  for(const char* invalid:{static_cast<const char*>(nullptr),"","SteamGameServer015","SteamGameServer016"})
   Check(steamworks::ResolveGameServerAuth014(ResolverAuth,7,invalid)==nullptr && authFactoryCalls==rawCalls,"adapter independently refuses unknown version before factory");
  Check(steamworks::ResolveGameServerAuth014(nullptr,7,"SteamGameServer014")==nullptr,"adapter handles absent factory");
  Check(steamworks::ResolveGameServerAuth014(ResolverAuth,0,"SteamGameServer014")==nullptr && authFactoryCalls==rawCalls,"adapter refuses missing engine user");
  SteamWorksGameServer adapter;config.keys.clear();g_SteamWorks.available=true;
  int nativeCalls=client.calls;
  Check(adapter.GetGameServerAuth014()==&authToken && authVersion=="SteamGameServer014","auth uses exact 014 factory and typed object");
  Check(client.calls==nativeCalls,"auth does not go through the SDK 015 getter");
  int authCalls=authFactoryCalls;g_SteamWorks.available=false;
  Check(adapter.GetGameServerAuth014()==nullptr && authFactoryCalls==authCalls,"stopped runtime hides cached auth object");
  g_SteamWorks.available=true;Check(adapter.GetGameServerAuth014()==&authToken && authFactoryCalls==authCalls,"014 result cached independently");
  for(const char* invalid:{"", "unknown", "SteamGameServer013", "SteamGameServer015", "SteamGameServer016"}) {
   adapter.Reset();config.keys["SteamGameServerAuthInterfaceVersion"]=invalid;
   int before=authFactoryCalls,users=pipeCalls,errors=host.errors;
   Check(adapter.GetGameServerAuth014()==nullptr && authFactoryCalls==before && pipeCalls==users,"unknown auth version rejected before user/factory");
   config.keys["SteamGameServerAuthInterfaceVersion"]="SteamGameServer014";
   Check(adapter.GetGameServerAuth014()==nullptr && host.errors==errors+1,"auth version rejection sticky with one diagnostic");
  }
  adapter.Reset();config.keys.clear();authUser=0;int before=authFactoryCalls;
  Check(adapter.GetGameServerAuth014()==nullptr && authFactoryCalls==before,"no engine user means no auth factory call");
  authUser=7;authAvailable=false;
  Check(adapter.GetGameServerAuth014()==nullptr && authFactoryCalls==before+1,"missing 014 object has no 015 fallback");
  authAvailable=true;Check(adapter.GetGameServerAuth014()==&authToken,"transient 014 absence can recover");
  adapter.Reset();config.keys["SteamGameServerInterfaceVersion"]="SteamGameServer014";
  Check(adapter.GetGameServer()==nullptr && adapter.GetGameServerAuth014()==&authToken,"native 015 registry remains distinct from engine auth 014");
  config.keys.clear();
 }
 SteamWorksGameServer server; resolverGlobal=nullptr;
 Check(server.GetSteamClient()==&client && factoryVersion=="SteamClient020","root factory uses pinned client declaration");
 int calls=factoryCalls; Check(server.GetSteamClient()==&client && factoryCalls==calls,"root factory result cached");
 g_SteamWorks.available=false;resolverGlobal=&client;
 Check(server.GetSteamClient()==nullptr && factoryCalls==calls,"stopped runtime hides global client without calling factory");
 std::printf("PASS: %d registry/resolver checks (production resolver, real SDK declarations, simulated services)\n",checks);
}
