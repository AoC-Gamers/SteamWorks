#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#include "steam_callbacks_test_host.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
using cell_t=int32_t;
constexpr int ET_Ignore=0,Param_Cell=0;
struct IForward {
 int consumers=1,executions=0;std::vector<cell_t> cells;std::function<void()> action;
 int GetFunctionCount() { return consumers; }
 void PushCell(cell_t cell) { cells.push_back(cell); }
 void Execute(cell_t*) { ++executions;auto invoke=action;if(invoke)invoke(); }
};
struct ForwardSystem {
 std::map<std::string,IForward*> owned;
 template<class... Args>IForward* CreateForward(const char* name,Args...) { auto* f=new IForward;owned[name]=f;return f; }
 void ReleaseForward(IForward* f) { for(auto i=owned.begin();i!=owned.end();++i)if(i->second==f) {owned.erase(i);break;}delete f; }
} forwardSystem;
auto* forwards=&forwardSystem;
#include "../extension/core/swforwards.cpp"
int checks=0;
void Check(bool ok,const char* why) { ++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);} }
int main() {
 {
  SteamWorksForwards owner;
  Check(steamCallbacks.callbacks.size()==6 && forwards->owned.size()==6,"six subscriptions and forwards owned");
  for(auto entry:steamCallbacks.callbacks)Check(CCallbackMgr::GameServer(entry.first),"server callback route retained");
  int registrations=steamCallbacks.registrations;owner.Start();Check(steamCallbacks.registrations==registrations,"start idempotent");
  auto* legacy=forwards->owned["SW_OnValidateClient"];auto* modern=forwards->owned["SteamWorks_OnValidateClient"];
  GSClientApprove_t approve={};approve.m_OwnerSteamID=CSteamID(1,k_EUniversePublic,k_EAccountTypeIndividual);approve.m_SteamID=CSteamID(2,k_EUniversePublic,k_EAccountTypeIndividual);
  DeliverCallback(approve);Check(legacy->cells==std::vector<cell_t>{1,2} && modern->cells==legacy->cells,"legacy/current identity forwards retain argument order");
  legacy->action=[&]{owner.Stop();};DeliverCallback(approve);
  Check(legacy->executions==2 && modern->executions==1 && steamCallbacks.callbacks.empty(),"stop during first forward suppresses later forward and unregisters callbacks");
  int unregistrations=steamCallbacks.unregistrations;owner.Stop();Check(steamCallbacks.unregistrations==unregistrations,"repeated stop idempotent");
  owner.NotifyPawnValidateClient(3,4);Check(legacy->executions==2 && modern->executions==1,"direct late dispatch suppressed while stopped");
  legacy->action={};owner.Start();Check(steamCallbacks.callbacks.size()==6,"restart restores exactly one subscription each");
  auto* connected=forwards->owned["SteamWorks_SteamServersConnected"];SteamServersConnected_t success={};DeliverCallback(success);
  Check(connected->executions==1 && connected->cells.empty(),"connected callback retains zero arguments");
  auto* failure=forwards->owned["SteamWorks_SteamServersConnectFailure"];SteamServerConnectFailure_t fail={};fail.m_eResult=k_EResultFail;DeliverCallback(fail);
  Check(failure->cells==std::vector<cell_t>{k_EResultFail},"failure forward retains result");
  connected->consumers=0;DeliverCallback(success);Check(connected->executions==1,"zero consumers skipped");
  auto* disconnected=forwards->owned["SteamWorks_SteamServersDisconnected"];SteamServersDisconnected_t lost={};lost.m_eResult=k_EResultNoConnection;DeliverCallback(lost);
  Check(disconnected->cells==std::vector<cell_t>{k_EResultNoConnection} && steamCallbacks.callbacks.size()==6,"network disconnect does not destroy integration");
  GSClientGroupStatus_t group={};group.m_SteamIDUser=approve.m_OwnerSteamID;group.m_SteamIDGroup=approve.m_SteamID;group.m_bMember=true;group.m_bOfficer=false;DeliverCallback(group);
  Check(forwards->owned["SteamWorks_OnClientGroupStatus"]->cells==std::vector<cell_t>{1,2,1,0},"group forward retains four arguments");
  int modernCalls=modern->executions;legacy->action=[&]{owner.Stop();owner.Start();};DeliverCallback(approve);
  Check(modern->executions==modernCalls && steamCallbacks.callbacks.size()==6,"stop/restart inside legacy forward cannot deliver old event to new generation");legacy->action={};
 }
 Check(steamCallbacks.callbacks.empty() && forwards->owned.empty(),"destruction unregisters before destroying owned forwards");
 std::printf("PASS: %d global callback lifecycle checks (production forwards, real SDK subscriptions, simulated services)\n",checks);
}
