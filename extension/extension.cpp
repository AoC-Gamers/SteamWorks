#include "extension.h"
#include "core/swdebug.h"
#include <cstdlib>
#include <cstdio>

namespace {

template <typename T>
void DeleteAndNull(T*& value)
{
	delete value;
	value = nullptr;
}

}  // namespace

SteamWorks g_SteamWorks;

SMEXT_LINK(&g_SteamWorks);
 
bool SteamWorks::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	steamworks::debug::Initialize();
	steamworks::debug::Log(steamworks::debug::Lifecycle, "extension_load", "late=%d", late ? 1 : 0);
	m_unloading = false;
	m_runtimeAvailable = true;
	sharesys->RegisterLibrary(myself, "SteamWorks");

	pSWGameData = new SteamWorksGameData;
	pSWGameServer = new SteamWorksGameServer;
	pSWHTTP = new SteamWorksHTTP;
	if (pSWHTTP->GetHTTPHandle() == 0) {
		std::snprintf(error, maxlength, "Could not create SteamWorks HTTP handle type");
		SDK_OnUnload();
		return false;
	}
	
	pGSDetours = new SteamWorksGSDetours;
	if (!pGSDetours->IsInstalled()) {
		std::snprintf(error, maxlength, "Steam runtime lifecycle detours unavailable; see error log");
		SDK_OnUnload();
		return false;
	}
	pClientLifecycle = new SteamWorksClientLifecycle;

	pSWHTTPNatives = new SteamWorksHTTPNatives;
	pSWForward = new SteamWorksForwards;
	pGSNatives = new SteamWorksGSNatives;
	pGSHooks = new SteamWorksGSHooks;
	pSWStats = new SteamWorksStats;
	if (!pSWStats->IsReady()) {
		std::snprintf(error, maxlength, "Could not create SteamWorks stats forwards");
		SDK_OnUnload();
		return false;
	}
	pSSNatives = new SteamWorksSSNatives;
	pGCHooks = new SteamWorksGCHooks;
	pGCNatives = new SteamWorksGCNatives;
	return true;
}

void SteamWorks::SDK_OnUnload()
{
	m_unloading = true;
	steamworks::debug::Log(steamworks::debug::Lifecycle, "extension_unload");
	RuntimeStopped();
	if (pClientLifecycle != nullptr) { pClientLifecycle->FinalizeStoppedHost(); }
	DeleteAndNull(pClientLifecycle);
	DeleteAndNull(pGCNatives);
	DeleteAndNull(pGCHooks);
	DeleteAndNull(pSSNatives);
	DeleteAndNull(pSWStats);
	DeleteAndNull(pGSDetours);
	DeleteAndNull(pGSHooks);
	DeleteAndNull(pGSNatives);
	DeleteAndNull(pSWForward);
	DeleteAndNull(pSWHTTPNatives);
	
	DeleteAndNull(pSWHTTP);
	DeleteAndNull(pSWGameServer);
	DeleteAndNull(pSWGameData);
	steamworks::debug::Shutdown();
}

void SteamWorks::RuntimeStopped()
{
 if (!m_runtimeAvailable) { return; }
 steamworks::debug::Log(steamworks::debug::Lifecycle, "runtime_stop");
 m_runtimeAvailable = false; // Block new acquisitions before touching borrowed objects.
 if (pGSHooks != nullptr) { pGSHooks->Stop(); }
 if (pGCHooks != nullptr) { pGCHooks->Stop(); }
 if (pSWForward != nullptr) { pSWForward->Stop(); }
 if (pSWStats != nullptr) { pSWStats->Stop(); }
 if (pSWHTTP != nullptr) { pSWHTTP->StopRuntime(); }
 if (pSWGameServer != nullptr) { pSWGameServer->Reset(); }
}

void SteamWorks::RuntimeStarted()
{
 if (m_unloading) { return; }
 if (pClientLifecycle != nullptr) { pClientLifecycle->GameServerStarted(); }
 steamworks::debug::Log(steamworks::debug::Lifecycle, "runtime_start");
 m_runtimeAvailable = true;
 if (pSWHTTP != nullptr) { pSWHTTP->StartRuntime(); }
 if (pSWStats != nullptr) { pSWStats->Start(); }
 if (pSWForward != nullptr) { pSWForward->Start(); }
 if (pGSHooks != nullptr) { pGSHooks->Start(); }
 if (pGCHooks != nullptr) { pGCHooks->Start(); }
}
