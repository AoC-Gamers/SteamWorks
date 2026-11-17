#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
std::vector<std::string> events;
bool failHTTPType=false,failDetours=false;
struct SteamWorksClientLifecycle;
std::function<bool()> available;
int checks=0;
void Check(bool ok,const char* why) { ++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);} }
struct Component {
 std::string name;
 explicit Component(const char* type):name(type) { events.push_back("create-"+name); }
 ~Component() { events.push_back("delete-"+name); }
 void Start() { Check(available(),"start only after runtime published");events.push_back("start-"+name); }
 void Stop() { Check(!available(),"stop only after acquisitions blocked");events.push_back("stop-"+name); }
};
#define COMPONENT(type) struct type:Component { type():Component(#type){} }
COMPONENT(SteamWorksGameData);
COMPONENT(SteamWorksGSNatives);
COMPONENT(SteamWorksHTTPNatives);
COMPONENT(SteamWorksSSNatives);
COMPONENT(SteamWorksGCNatives);
struct SteamWorksGSDetours:Component { SteamWorksGSDetours():Component("SteamWorksGSDetours"){} bool IsInstalled() const { return !failDetours; } };
struct SteamWorksClientLifecycle:Component { SteamWorksClientLifecycle():Component("SteamWorksClientLifecycle"){} void FinalizeStoppedHost(){events.push_back("finalize-stopped-host");} void GameServerStarted(){} };
COMPONENT(SteamWorksGSHooks);
COMPONENT(SteamWorksGCHooks);
COMPONENT(SteamWorksForwards);
struct SteamWorksGameServer:Component {
 SteamWorksGameServer():Component("SteamWorksGameServer"){}
 void Reset() { Check(!available(),"cache reset only after acquisitions blocked");events.push_back("reset"); }
};
struct SteamWorksHTTP:Component {
 SteamWorksHTTP():Component("SteamWorksHTTP"){}
 int GetHTTPHandle() { return failHTTPType?0:1; }
 void StopRuntime() { Stop(); }
 void StartRuntime() { Start(); }
};
struct ShareSystem { void RegisterLibrary(void*,const char*) {} } shareSystem;
auto* sharesys=&shareSystem;void* myself=nullptr;
#define SMEXT_LINK(...) static_assert(true,"simulated extension registration")
class SteamWorks {
public:
 bool SDK_OnLoad(char*,size_t,bool);
 void SDK_OnUnload();void RuntimeStarted();void RuntimeStopped();
 bool IsRuntimeAvailable() const { return m_runtimeAvailable && !m_unloading; }
 SteamWorksGameData* pSWGameData=nullptr;SteamWorksGameServer* pSWGameServer=nullptr;
 SteamWorksGSNatives* pGSNatives=nullptr;SteamWorksGSHooks* pGSHooks=nullptr;
 SteamWorksSSNatives* pSSNatives=nullptr;SteamWorksGSDetours* pGSDetours=nullptr;
 SteamWorksHTTP* pSWHTTP=nullptr;SteamWorksHTTPNatives* pSWHTTPNatives=nullptr;
 SteamWorksGCHooks* pGCHooks=nullptr;SteamWorksGCNatives* pGCNatives=nullptr;
 SteamWorksForwards* pSWForward=nullptr;SteamWorksClientLifecycle* pClientLifecycle=nullptr;
private:
 bool m_runtimeAvailable=true,m_unloading=false;
};
#include "../../extension/extension.cpp"
int main() {
 available=[] {return g_SteamWorks.IsRuntimeAvailable();};char error[128]={};
 Check(g_SteamWorks.SDK_OnLoad(error,sizeof(error),true),"late extension load succeeds");events.clear();
 g_SteamWorks.RuntimeStopped();
 Check(events==std::vector<std::string>{"stop-SteamWorksGSHooks","stop-SteamWorksGCHooks","stop-SteamWorksForwards","stop-SteamWorksHTTP","reset"},"both hook groups, callbacks and HTTP stopped before cache reset");
 auto stopped=events;g_SteamWorks.RuntimeStopped();Check(events==stopped,"duplicate runtime stop is inert");
 events.clear();g_SteamWorks.RuntimeStarted();
 Check(events==std::vector<std::string>{"start-SteamWorksHTTP","start-SteamWorksForwards","start-SteamWorksGSHooks","start-SteamWorksGCHooks"},"successful restart enables consumers in dependency order");
 events.clear();g_SteamWorks.SDK_OnUnload();
 Check(events.size()>=5 && std::vector<std::string>(events.begin(),events.begin()+5)==stopped,"unload stops resources before deleting components");
 Check(std::find(events.begin(),events.end(),"finalize-stopped-host")<std::find(events.begin(),events.end(),"delete-SteamWorksGSDetours"),"host finalization runs before lifecycle library owner release");
 Check(events.back()=="delete-SteamWorksGameData" && events[events.size()-2]=="delete-SteamWorksGameServer","interfaces/gamedata outlive resources and detours");
 Check(!g_SteamWorks.IsRuntimeAvailable() && g_SteamWorks.pSWHTTP==nullptr && g_SteamWorks.pGSHooks==nullptr && g_SteamWorks.pGCHooks==nullptr,"unload leaves no live owners or capability");
 auto unloaded=events;g_SteamWorks.SDK_OnUnload();g_SteamWorks.RuntimeStarted();Check(events==unloaded,"double unload and late restart cannot resurrect owners");
 failHTTPType=true;events.clear();Check(!g_SteamWorks.SDK_OnLoad(error,sizeof(error),false),"handle type failure fails load safely");
 Check(std::string(error).find("HTTP handle type")!=std::string::npos && g_SteamWorks.pSWHTTP==nullptr && g_SteamWorks.pSWGameData==nullptr,"partial load rolls back with diagnostic");
 auto failed=events;g_SteamWorks.SDK_OnUnload();Check(events==failed,"failed load can be unloaded again");
 failHTTPType=false;failDetours=true;events.clear();
 Check(!g_SteamWorks.SDK_OnLoad(error,sizeof(error),false) && std::string(error).find("lifecycle detours")!=std::string::npos,"missing lifecycle observer prevents unsafe partial load");
 Check(g_SteamWorks.pGSHooks==nullptr && g_SteamWorks.pGCHooks==nullptr && g_SteamWorks.pSWForward==nullptr && g_SteamWorks.pGSDetours==nullptr,"observer failure leaves no hooks/subscriptions/resources");
 for(const auto& event:events)Check(event!="create-SteamWorksGSHooks" && event!="create-SteamWorksGCHooks" && event!="create-SteamWorksForwards","dependent consumers never created before observer succeeds");
 failDetours=false;
 Check(g_SteamWorks.SDK_OnLoad(error,sizeof(error),false),"fresh load after failed attempt works");g_SteamWorks.SDK_OnUnload();
 std::printf("PASS: %d runtime coordination checks (production load/stop/start/unload, simulated components)\n",checks);
}
