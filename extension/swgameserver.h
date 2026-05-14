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

#pragma once

#include "smsdk_ext.h"
#include "isteamgamecoordinator.h"
#include "steam_gameserver.h"

#if defined(STEAM_API_INTERNAL_H) || !defined(STEAM_API_EXPORTS)
	S_API ISteamClient *g_pSteamClientGameServer;
#endif

class SteamWorksGameServer
{
public:
	SteamWorksGameServer();
	~SteamWorksGameServer();

	ISteamClient *GetSteamClient(void);
	ISteamGameServer *GetGameServer(void);
	ISteamUtils *GetUtils(void);
	ISteamNetworking *GetNetworking(void);
	ISteamGameServerStats *GetServerStats(void);
	ISteamHTTP *GetHTTP(void);
	ISteamMatchmaking *GetMatchmaking(void);
	ISteamGameCoordinator *GetGameCoordinator(void);

	void Reset(void);
	const char *GetLibraryPath(void);

private:
	void GetUserAndPipe(HSteamUser &hSteamUser, HSteamPipe &hSteamPipe);

	ISteamClient *m_pClient = nullptr;
	ISteamGameServer *m_pGameServer = nullptr;
	ISteamUtils *m_pUtils = nullptr;
	ISteamNetworking *m_pNetworking = nullptr;
	ISteamGameServerStats *m_pStats = nullptr;
	ISteamHTTP *m_pHTTP = nullptr;
	ISteamMatchmaking *m_pMatchmaking = nullptr;
	ISteamGameCoordinator *m_pGC = nullptr;
};

#include "extension.h"
