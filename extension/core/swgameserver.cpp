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
#include "swdebug.h"

template <typename Interface>
const char* SteamWorksGameServer::CompatibleVersion()
{
	const auto record = steamworks::InterfaceContract<Interface>::Record();
	const size_t index = static_cast<size_t>(record.id);
	if (m_rejectedVersions[index]) {
		return nullptr;
	}
	const char* requested = record.version;
	if (record.configKey != nullptr && g_SteamWorks.pSWGameData != nullptr) {
		IGameConfig* config = g_SteamWorks.pSWGameData->GetGameData();
		if (config != nullptr) {
			const char* overrideVersion = config->GetKeyValue(record.configKey);
			if (overrideVersion != nullptr) { requested = overrideVersion; }
		}
	}
	if (!steamworks::AcceptsVersion<Interface>(requested)) {
		steamworks::debug::Log(steamworks::debug::Interfaces, "version_rejected", "family=%s expected=%s", record.family, record.version);
		m_rejectedVersions[index] = true;
		g_pSM->LogError(myself, "Unsupported %s interface version '%s'; expected '%s'; capability disabled until reset",
			record.family, requested, record.version);
		return nullptr;
	}
	return record.version;
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
	m_pAuthServer014 = nullptr;
	m_pUtils = nullptr;
	m_pNetworking = nullptr;
	m_pStats = nullptr;
	m_pHTTP = nullptr;
	m_pMatchmaking = nullptr;
	m_pGC = nullptr;
	for (bool& rejected : m_rejectedVersions) { rejected = false; }
}

ISteamClient *SteamWorksGameServer::GetSteamClient(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (g_pSteamClientGameServer != nullptr)
	{
		return g_pSteamClientGameServer;
	}

	if (m_pClient == nullptr) {
		ILibrary* library = libsys->OpenLibrary(GetLibraryPath(), nullptr, 0);
		if (library != nullptr) {
			using Factory = void* (S_CALLTYPE *)(const char*);
			Factory create = reinterpret_cast<Factory>(library->GetSymbolAddress("SteamInternal_CreateInterface"));
			if (create != nullptr) { m_pClient = static_cast<ISteamClient*>(create(steamworks::InterfaceContract<ISteamClient>::Record().version)); }
			library->CloseLibrary();
		}
	}

	return m_pClient;
}

ISteamGameServer *SteamWorksGameServer::GetGameServer(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pGameServer != nullptr) { return m_pGameServer; }
	const char* pVersion = CompatibleVersion<ISteamGameServer>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		m_pGameServer = client->GetISteamGameServer(hSteamUser, hSteamPipe, pVersion);
		if (m_pGameServer != nullptr) {
			steamworks::debug::Log(steamworks::debug::Interfaces, "gameserver_resolved", "version=%s available=1", pVersion);
		}
	}
	
	return m_pGameServer;
}

steamworks::ISteamGameServer014* SteamWorksGameServer::GetGameServerAuth014()
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pAuthServer014 != nullptr) { return m_pAuthServer014; }
	const char* version = CompatibleVersion<steamworks::ISteamGameServer014>();
	if (version == nullptr) { return nullptr; }
	// Same factory/user as the verified engine, without casting the 015 getter.
	m_pAuthServer014 = steamworks::ResolveGameServerAuth014(
		&SteamInternal_FindOrCreateGameServerInterface, SteamGameServer_GetHSteamUser(), version);
	if (m_pAuthServer014 != nullptr) {
		steamworks::debug::Log(steamworks::debug::Interfaces, "auth_server_resolved", "version=%s available=1", version);
	}
	return m_pAuthServer014;
}

ISteamUtils *SteamWorksGameServer::GetUtils(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pUtils != nullptr) { return m_pUtils; }
	const char* pVersion = CompatibleVersion<ISteamUtils>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamPipe hSteamPipe = SteamGameServer_GetHSteamPipe();
		
		m_pUtils = client->GetISteamUtils(hSteamPipe, pVersion);
	}
	
	return m_pUtils;
}

ISteamNetworking *SteamWorksGameServer::GetNetworking(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pNetworking != nullptr) { return m_pNetworking; }
	const char* pVersion = CompatibleVersion<ISteamNetworking>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		m_pNetworking = client->GetISteamNetworking(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pNetworking;
}

ISteamGameServerStats *SteamWorksGameServer::GetServerStats(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pStats != nullptr) { return m_pStats; }
	const char* pVersion = CompatibleVersion<ISteamGameServerStats>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		m_pStats = client->GetISteamGameServerStats(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pStats;
}

ISteamHTTP *SteamWorksGameServer::GetHTTP(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pHTTP != nullptr) { return m_pHTTP; }
	const char* pVersion = CompatibleVersion<ISteamHTTP>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		m_pHTTP = client->GetISteamHTTP(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pHTTP;
}

ISteamMatchmaking *SteamWorksGameServer::GetMatchmaking(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pMatchmaking != nullptr) { return m_pMatchmaking; }
	const char* pVersion = CompatibleVersion<ISteamMatchmaking>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);
		
		m_pMatchmaking = client->GetISteamMatchmaking(hSteamUser, hSteamPipe, pVersion);
	}
	
	return m_pMatchmaking;
}

ISteamGameCoordinator *SteamWorksGameServer::GetGameCoordinator(void)
{
	if (!g_SteamWorks.IsRuntimeAvailable()) { return nullptr; }
	if (m_pGC != nullptr) { return m_pGC; }
	const char* pVersion = CompatibleVersion<ISteamGameCoordinator>();
	if (pVersion == nullptr) { return nullptr; }
	ISteamClient* client = GetSteamClient();
	if (client != nullptr)
	{
		HSteamUser hSteamUser;
		HSteamPipe hSteamPipe;
		GetUserAndPipe(hSteamUser, hSteamPipe);

		m_pGC = static_cast<ISteamGameCoordinator *>(client->GetISteamGenericInterface(hSteamUser, hSteamPipe, pVersion));
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
