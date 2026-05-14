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

#include "swgameserver.h"

static void GetGameSpecificConfigInterface(const char *pName, const char *&pVersion)
{
	if (g_SteamWorks.pSWGameData == nullptr)
	{
		return;
	}

	IGameConfig *pConfig = g_SteamWorks.pSWGameData->GetGameData();
	if (pConfig == nullptr)
	{
		return;
	}
	
	const char *pNewVersion = pConfig->GetKeyValue(pName);
	if (pNewVersion != nullptr)
	{
		pVersion = pNewVersion;
	}
}

SteamWorksGameServer::SteamWorksGameServer()
{
	this->Reset();
}

SteamWorksGameServer::~SteamWorksGameServer()
{
}

void SteamWorksGameServer::Reset(void)
{
	m_pClient = nullptr;
	m_pGameServer = nullptr;
	m_pUtils = nullptr;
	m_pNetworking = nullptr;
	m_pStats = nullptr;
	m_pHTTP = nullptr;
	m_pMatchmaking = nullptr;
	m_pGC = nullptr;
}

ISteamClient *SteamWorksGameServer::GetSteamClient(void)
{
	if (g_pSteamClientGameServer != nullptr)
	{
		return g_pSteamClientGameServer;
	}

	/*
		The following is assumed from an unreleased version of the SteamWorks SDK, first seen (and reversed) in CS:GO.
		Thanks CS:GO team! @:|
	*/

	if (m_pClient == nullptr)
	{
		const char *pLibSteamPath = GetLibraryPath();

		void *(*pGSInternalCreateAddress)(const char *) = nullptr;
		void *(*pInternalCreateAddress)(const char *) = nullptr;
		const char *pGSInternalFuncName = "SteamGameServerInternal_CreateInterface";
		const char *pInternalFuncName = "SteamInternal_CreateInterface";

		if (g_SteamWorks.pSWGameData)
		{
			IGameConfig *pConfig = g_SteamWorks.pSWGameData->GetGameData();
			if (pConfig != nullptr)
			{
				pConfig->GetMemSig(pGSInternalFuncName, reinterpret_cast<void **>(&pGSInternalCreateAddress));
				pConfig->GetMemSig(pInternalFuncName, reinterpret_cast<void **>(&pInternalCreateAddress));
			}
		}

		ILibrary *pLibrary = libsys->OpenLibrary(pLibSteamPath, nullptr, 0);
		if (pLibrary != nullptr)
		{
			if (pGSInternalCreateAddress == nullptr)
			{
				pGSInternalCreateAddress = reinterpret_cast<void *(*)(const char *)>(pLibrary->GetSymbolAddress(pGSInternalFuncName));
			}

			if (pInternalCreateAddress == nullptr)
			{
				pInternalCreateAddress = reinterpret_cast<void *(*)(const char *)>(pLibrary->GetSymbolAddress(pInternalFuncName));
			}

			pLibrary->CloseLibrary();
		}

		if (pGSInternalCreateAddress != nullptr)
		{
			m_pClient = static_cast<ISteamClient *>((*pGSInternalCreateAddress)(STEAMCLIENT_INTERFACE_VERSION));
		}
		
		if (m_pClient == nullptr && pInternalCreateAddress != nullptr)
		{
			m_pClient = static_cast<ISteamClient *>((*pInternalCreateAddress)(STEAMCLIENT_INTERFACE_VERSION));
		}
	}

	return m_pClient;
}

ISteamGameServer *SteamWorksGameServer::GetGameServer(void)
{
	if (m_pGameServer == nullptr && GetSteamClient() != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		const char *pVersion = STEAMGAMESERVER_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamGameServerInterfaceVersion", pVersion);
		m_pGameServer = GetSteamClient()->GetISteamGameServer(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pGameServer;
}

ISteamUtils *SteamWorksGameServer::GetUtils(void)
{
	if (m_pUtils == nullptr && GetSteamClient() != nullptr)
	{
		HSteamPipe hSteamPipe = SteamGameServer_GetHSteamPipe();
		
		const char *pVersion = STEAMUTILS_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamUtilsInterfaceVersion", pVersion);
		m_pUtils = GetSteamClient()->GetISteamUtils(hSteamPipe, pVersion);
	}
	
	return m_pUtils;
}

ISteamNetworking *SteamWorksGameServer::GetNetworking(void)
{
	if (m_pNetworking == nullptr && GetSteamClient() != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		const char *pVersion = STEAMNETWORKING_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamNetworkingInterfaceVersion", pVersion);
		m_pNetworking = GetSteamClient()->GetISteamNetworking(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pNetworking;
}

ISteamGameServerStats *SteamWorksGameServer::GetServerStats(void)
{
	if (m_pStats == nullptr && GetSteamClient() != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		const char *pVersion = STEAMGAMESERVERSTATS_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamGameServerStatsInterfaceVersion", pVersion);
		m_pStats = GetSteamClient()->GetISteamGameServerStats(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pStats;
}

ISteamHTTP *SteamWorksGameServer::GetHTTP(void)
{
	if (m_pHTTP == nullptr && GetSteamClient() != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		const char *pVersion = STEAMHTTP_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamHTTPInterfaceVersion", pVersion);
		m_pHTTP = GetSteamClient()->GetISteamHTTP(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pHTTP;
}

ISteamMatchmaking *SteamWorksGameServer::GetMatchmaking(void)
{
	if (m_pMatchmaking == nullptr && GetSteamClient() != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		const char *pVersion = STEAMMATCHMAKING_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamMatchmakingVersion", pVersion);
		m_pMatchmaking = GetSteamClient()->GetISteamMatchmaking(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pMatchmaking;
}

ISteamGameCoordinator *SteamWorksGameServer::GetGameCoordinator(void)
{
	if (m_pGC == nullptr && GetSteamClient() != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);

		const char *pVersion = STEAMGAMECOORDINATOR_INTERFACE_VERSION;
		GetGameSpecificConfigInterface("SteamGameCoordinatorVersion", pVersion);
		m_pGC = static_cast<ISteamGameCoordinator *>(GetSteamClient()->GetISteamGenericInterface(hSteamUser, hSteamPipe, pVersion));
	}

	return m_pGC;
}

void SteamWorksGameServer::GetUserAndPipe(HSteamUser &hSteamUser, HSteamPipe &hSteamPipe)
{
	hSteamUser = SteamGameServer_GetHSteamUser();
	hSteamPipe = SteamGameServer_GetHSteamPipe();
}

const char *SteamWorksGameServer::GetLibraryPath(void)
{
	static const char *pLibSteamPath = nullptr;

	if (pLibSteamPath == nullptr)
	{
#if defined POSIX
		pLibSteamPath = "./bin/libsteam_api.so";
#elif defined WIN32_LEAN_AND_MEAN
		pLibSteamPath = "./bin/steam_api.dll"; /* Naming from SteamTools. */
#endif

		if (g_SteamWorks.pSWGameData)
		{
			IGameConfig *pConfig = g_SteamWorks.pSWGameData->GetGameData();
			if (pConfig != nullptr)
			{
				const char *kvLibSteamAPI = pConfig->GetKeyValue("LibSteamAPI");
				if (kvLibSteamAPI != nullptr)
				{
					pLibSteamPath = kvLibSteamAPI;
				}
			}
		}
	}

	return pLibSteamPath;
}
