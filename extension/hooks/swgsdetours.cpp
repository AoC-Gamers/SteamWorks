/*
    This file is part of SourcePawn SteamWorks.

    SourcePawn SteamWorks is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, as per version 3 of the License.

    SourcePawn SteamWorks is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourcePawn SteamWorks.  If not, see <http://www.gnu.org/licenses/>.
	
	Author: Kyle Sanderson (KyleS).
*/

#include "swgsdetours.h"
#include "../core/swlifecycleprofile.h"

static_assert(sizeof(uint32) == 4 && sizeof(uint16) == 2 && sizeof(EServerMode) == 4, "Lifecycle argument widths must match the validated i386 ABI");

namespace {

void DestroyDetour(CDetour*& detour)
{
	if (detour == nullptr) {
		return;
	}

	detour->Destroy();
	detour = nullptr;
}

}  // namespace

DETOUR_DECL_STATIC0(SteamAPIShutdown, void)
{
	g_SteamWorks.RuntimeStopped();
	g_pSM->LogMessage(myself, "Steam lifecycle shutdown observed: consumers stopped");

	DETOUR_STATIC_CALL(SteamAPIShutdown)(); /* We're not a monster. */
	if (g_SteamWorks.pClientLifecycle != nullptr) { g_SteamWorks.pClientLifecycle->GameServerStopped(); }
}

DETOUR_DECL_STATIC6(SteamGameServer_InitLegacyDetour, bool, uint32, unIP, uint16, usSteamPort, uint16, usGamePort, uint16, usQueryPort, EServerMode, eServerMode, const char *, pchVersionString)
{
	bool bRet = DETOUR_STATIC_CALL(SteamGameServer_InitLegacyDetour)(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString);
	
	if (bRet) { g_SteamWorks.RuntimeStarted(); }
	g_pSM->LogMessage(myself, "Steam lifecycle init observed: result=%d", bRet ? 1 : 0);

	return bRet;
}

SteamWorksGSDetours::SteamWorksGSDetours()
{
	const char* profileId = steamworks::VerifiedLifecycleProfileId();
	if (profileId == nullptr) {
		g_pSM->LogError(myself, "No verified Steam lifecycle binary profile; internal init/shutdown detours unavailable");
		return;
	}
	if (g_SteamWorks.pSWGameServer == nullptr) {
		g_pSM->LogError(myself, "Steam gameserver resolver unavailable; init/shutdown detours disabled");
		return;
	}

	const char* pLibSteamPath = g_SteamWorks.pSWGameServer->GetLibraryPath();
	m_library = libsys->OpenLibrary(pLibSteamPath, nullptr, 0);
	void* pSteamInitAddress = m_library != nullptr ? m_library->GetSymbolAddress(steamworks::LifecycleInitSymbol) : nullptr;
	void* pSteamShutdownAddress = m_library != nullptr ? m_library->GetSymbolAddress(steamworks::LifecycleShutdownSymbol) : nullptr;
	if (pSteamInitAddress == nullptr || pSteamShutdownAddress == nullptr) {
		Shutdown();
		g_pSM->LogError(myself, "Unresolved lifecycle exports SteamInternal_GameServer_Init/SteamGameServer_Shutdown; detours disabled");
		return;
	}
	void* initCall = steamworks::VerifiedLifecycleInitCall(pSteamInitAddress);
	if (initCall == nullptr || !steamworks::IsVerifiedLifecycleAddress(pSteamShutdownAddress, steamworks::LifecycleEntry::Shutdown)) {
		Shutdown();
		g_pSM->LogError(myself, "Lifecycle provider/address/prologue differs from profile; detours disabled");
		return;
	}

	ISourcePawnEngine* engine = g_pSM->GetScriptingEngine();
	if (engine == nullptr) {
		Shutdown();
		g_pSM->LogError(myself, "SourcePawn engine unavailable; init/shutdown detours disabled");
		return;
	}

	CDetourManager::Init(engine, nullptr);
	m_pShutdownDetour = DETOUR_CREATE_STATIC_FIXED(SteamAPIShutdown, pSteamShutdownAddress);
	if (m_pShutdownDetour == nullptr) {
		Shutdown();
		g_pSM->LogError(myself, "Could not create Steam shutdown detour; init/shutdown detours disabled");
		return;
	}

	if (!m_initCall.Prepare(initCall, pSteamInitAddress, GET_STATIC_CALLBACK(SteamGameServer_InitLegacyDetour))) {
		Shutdown();
		g_pSM->LogError(myself, "Could not prepare Steam init call observer; lifecycle observers disabled");
		return;
	}
	SteamGameServer_InitLegacyDetour_Actual = reinterpret_cast<decltype(SteamGameServer_InitLegacyDetour_Actual)>(pSteamInitAddress);
	if (!m_initCall.Enable()) {
		Shutdown();
		g_pSM->LogError(myself, "Could not patch Steam init call; lifecycle observers disabled");
		return;
	}
	// Shared SDK init entry belongs to its existing consumers, e.g. SourceTVSupport.
	m_pShutdownDetour->EnableDetour();
	g_pSM->LogMessage(myself, "Steam lifecycle profile installed: %s", profileId);
}

SteamWorksGSDetours::~SteamWorksGSDetours()
{
	Shutdown();
}

void SteamWorksGSDetours::Shutdown()
{
	if (!m_initCall.Restore()) { g_pSM->LogError(myself, "Steam init call ownership changed; refusing to overwrite foreign patch"); }
	m_initCall.Reset();
	SteamGameServer_InitLegacyDetour_Actual = nullptr;
	DestroyDetour(m_pShutdownDetour);
	SteamAPIShutdown_Actual = nullptr;
	if (m_library != nullptr) { m_library->CloseLibrary(); m_library = nullptr; }
}
