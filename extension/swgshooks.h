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

#include "isteamgameserver.h"
#include "smsdk_ext.h"
#include "sourcehook.h"
#include "steam_gameserver.h"

class SteamWorksGSHooks
{
public:
	SteamWorksGSHooks();
	~SteamWorksGSHooks();

	void AddHooks(ISteamGameServer *pGameServer);
	void RemoveHooks(ISteamGameServer *pGameServer, bool destroyed = false);

	bool WasRestartRequested(void);
	void LogOnAnonymous(void);
	EBeginAuthSessionResult BeginAuthSession(const void* pAuthTicket, int cbAuthTicket, CSteamID steamID);

private:
	IForward *pRestartRequestedForward = nullptr;
	IForward *pTokenRequestedForward = nullptr;
	IForward *pBeginAuthSessionForward = nullptr;
	unsigned char uHooked = 0;
};

void OurGameFrameHook(bool simulating);

#include "extension.h"
