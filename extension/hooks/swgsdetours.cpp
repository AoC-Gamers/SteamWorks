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
	if (g_SteamWorks.pSWGameServer != nullptr)
	{
		if (g_SteamWorks.pGSHooks != nullptr)
		{
			g_SteamWorks.pGSHooks->RemoveHooks(g_SteamWorks.pSWGameServer->GetGameServer(), false);
		}
		
		g_SteamWorks.pSWGameServer->Reset();
	}

	DETOUR_STATIC_CALL(SteamAPIShutdown)(); /* We're not a monster. */
}

DETOUR_DECL_STATIC6(SteamGameServer_InitSafeDetour, bool, uint32, unIP, uint16, usSteamPort, uint16, usGamePort, uint16, usQueryPort, EServerMode, eServerMode, const char *, pchVersionString)
{
	bool bRet = DETOUR_STATIC_CALL(SteamGameServer_InitSafeDetour)(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString);
	
	if (g_SteamWorks.pSWGameServer != nullptr && g_SteamWorks.pGSHooks != nullptr)
	{
		g_SteamWorks.pGSHooks->AddHooks(g_SteamWorks.pSWGameServer->GetGameServer());
	}
	
	return bRet;
}

SteamWorksGSDetours::SteamWorksGSDetours()
{
	const char *pLibSteamPath = g_SteamWorks.pSWGameServer->GetLibraryPath();

	void *pSteamSafeInitAddress = nullptr;
	void *pSteamShutdownAddress = nullptr;

	const char *pInitSafeFuncName = "SteamGameServer_InitSafe";
	const char *pShutdownFuncName = "SteamGameServer_Shutdown";

	IGameConfig *pConfig = nullptr;
	if (g_SteamWorks.pSWGameData)
	{
		pConfig = g_SteamWorks.pSWGameData->GetGameData();
		if (pConfig != nullptr)
		{
			pConfig->GetMemSig(pShutdownFuncName, &pSteamShutdownAddress);
			pConfig->GetMemSig(pInitSafeFuncName, &pSteamSafeInitAddress);
		}
	}

	ILibrary *pLibrary = libsys->OpenLibrary(pLibSteamPath, nullptr, 0);
	if (pLibrary != nullptr)
	{
		if (pSteamShutdownAddress == nullptr)
		{
			pSteamShutdownAddress = pLibrary->GetSymbolAddress(pShutdownFuncName);
		}
		
		if (pSteamSafeInitAddress == nullptr)
		{
			pSteamSafeInitAddress = pLibrary->GetSymbolAddress(pInitSafeFuncName);
		}
		
		pLibrary->CloseLibrary();
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), pConfig);
	if (pSteamShutdownAddress != nullptr)
	{
		m_pShutdownDetour = DETOUR_CREATE_STATIC_FIXED(SteamAPIShutdown, pSteamShutdownAddress);
		m_pShutdownDetour->EnableDetour();
	}

	if (pSteamSafeInitAddress != nullptr)
	{
		m_pSafeInitDetour = DETOUR_CREATE_STATIC_FIXED(SteamGameServer_InitSafeDetour, pSteamSafeInitAddress);
		m_pSafeInitDetour->EnableDetour();
	}
}

SteamWorksGSDetours::~SteamWorksGSDetours()
{
	DestroyDetour(m_pShutdownDetour);
	DestroyDetour(m_pSafeInitDetour);
}
