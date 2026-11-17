#include "detours_test_host.h"
#include <type_traits>

static_assert(!std::is_copy_constructible<SteamWorksGSDetours>::value, "Detour ownership cannot be copied");
static_assert(!std::is_copy_assignable<SteamWorksGSDetours>::value, "Detour ownership cannot be duplicated");

int main()
{
	int scenarios = 0;
 for (int scenario=0; scenario<15; ++scenario) {
  ResetFixture();
  if(scenario==1)detourState.failShutdownCreation=true;
  if(scenario==2)detourState.failInitCreation=true;
  if(scenario==3)detourState.shutdownExport=false;
  if(scenario==4)detourState.initExport=false;
  if(scenario==5)detourState.library=false;
  if(scenario==6)detourState.engine=false;
  if(scenario==7)g_SteamWorks.pSWGameServer=nullptr;
  if(scenario==8)detourState.validAddress=false;
  if(scenario==9){detourState.shutdownSignature=true;detourState.initSignature=true;}
  if(scenario==10)g_SteamWorks.pSWGameData=nullptr;
  if(scenario==11)detourState.config=false;
  if(scenario==12){detourState.shutdownSignature=true;detourState.initSignature=true;detourState.library=false;}
  if(scenario==13){detourState.shutdownExport=false;detourState.initExport=false;}
  if(scenario==14)detourState.failInitEnable=true;
  bool success=scenario==0 || (scenario>=9 && scenario<=11);
  {
   SteamWorksGSDetours owner;
   Require(owner.IsInstalled()==success,"readiness requires complete verified pair");
   Require(detourState.enables==(success?2:0),"no partially enabled pair");
   Require(detourState.errors.size()==(success?0u:1u),"one diagnostic on failure");
   Require(detourState.live==(success?2:0),"partial failure rolls back patches");
   if(scenario==7)Require(detourState.libraryOpens==0,"missing resolver stops before opening module");
   if(scenario==8)Require(detourState.creates==0,"unverified address rejected before creating patch");
   if(scenario==9)Require(detourState.exportReads==2,"gamedata signatures cannot bypass approved exports");
   if(success)Require(detourState.libraryCloses==0,"owned module retained while trampoline is live");
   owner.Shutdown();
   Require(!owner.IsInstalled() && detourState.live==0,"shutdown clears pair and readiness");
   Require(SteamAPIShutdown_Actual==nullptr && SteamGameServer_InitLegacyDetour_Actual==nullptr,"no stale trampoline pointers");
   Require(detourState.libraryCloses==((scenario!=7 && detourState.library)?1:0),"owned module reference balanced");
   int closes=detourState.libraryCloses,destroys=detourState.destroys;
   owner.Shutdown();Require(detourState.libraryCloses==closes && detourState.destroys==destroys,"cleanup repeat does not free twice");
  }
  ++scenarios;
 }

	ResetFixture();
	detourState.failInitCreation = true;
	{ SteamWorksGSDetours failed; }
	Require(detourState.live == 0 && SteamAPIShutdown_Actual == nullptr, "failed owner left no resources");
	detourState.failInitCreation = false;
	{ SteamWorksGSDetours retry; Require(detourState.live == 2, "fresh owner can create pair after failed attempt"); }
	Require(detourState.live == 0 && detourState.destroys == 3, "retry and prior partial pair each released once");
	++scenarios;

	for (int scenario = 0; scenario < 8; ++scenario) {
		ResetFixture();
		SteamGameServer_InitLegacyDetour_Actual = &OriginalInit;
		if (scenario == 0 || scenario == 5 || scenario == 6) { detourState.initResult = false; }
		if (scenario == 2) { gameServer.server = nullptr; }
		if (scenario == 3 || scenario == 5) { g_SteamWorks.pSWGameServer = nullptr; }
		if (scenario == 4 || scenario == 6) { g_SteamWorks.pGSHooks = nullptr; }
		Require(InvokeInit() == detourState.initResult, "original init result preserved");
		if (scenario == 7) { Require(InvokeInit(), "repeated original call result preserved"); }
		const int repeats = scenario == 7 ? 2 : 1;
		Require(detourState.originalInitCalls == repeats, "original init called exactly once per wrapper invocation");
		Require(detourState.interfaceReads == ((scenario == 1 || scenario == 2 || scenario == 7) ? repeats : 0), "failed init never acquires interface");
		Require(detourState.addHooks == ((scenario == 1 || scenario == 7) ? repeats : 0), "only successful init with valid interface requests hooks");
		Require(detourState.errors.empty(), "wrapper does not emit recurring diagnostics");
		++scenarios;
	}

	for (int scenario = 0; scenario < 3; ++scenario) {
		ResetFixture();
		SteamAPIShutdown_Actual = &OriginalShutdown;
		if (scenario == 1) { g_SteamWorks.pGSHooks = nullptr; }
		if (scenario == 2) { g_SteamWorks.pSWGameServer = nullptr; }
		SteamAPIShutdown();
		Require(detourState.originalShutdownCalls == 1, "original shutdown called once");
		const std::vector<std::string> expected = scenario == 0
			? std::vector<std::string>{"remove-hooks", "reset", "original-shutdown", "host-stopped"}
			: scenario == 1 ? std::vector<std::string>{"reset", "original-shutdown", "host-stopped"}
			: std::vector<std::string>{"remove-hooks", "original-shutdown", "host-stopped"};
		Require(detourState.events == expected, "shutdown stops owned hooks without reacquisition before original");
		++scenarios;
	}
	ResetFixture();
	std::printf("PASS: %d detour owner/wrapper scenarios (simulated factory, patches and host; no ABI claim)\n", scenarios);
	return 0;
}
