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
#include "../core/swgameauth014.h"

class SteamWorksGSHooks
{
public:
	SteamWorksGSHooks();
	~SteamWorksGSHooks();

	void AddHooks(ISteamGameServer* pGameServer, steamworks::ISteamGameServer014* authServer);
	void Stop();
	void Start();
	void TryAttach();
	void TraceState() const;
	SteamWorksGSHooks(const SteamWorksGSHooks&) = delete;
	SteamWorksGSHooks& operator=(const SteamWorksGSHooks&) = delete;

	bool WasRestartRequested(void);
	void LogOnAnonymous(void);
	EBeginAuthSessionResult BeginAuthSession(const void* pAuthTicket, int cbAuthTicket, CSteamID steamID);

private:
	IForward *pRestartRequestedForward = nullptr;
	IForward *pTokenRequestedForward = nullptr;
	IForward *pBeginAuthSessionForward = nullptr;
	ISteamGameServer* m_target = nullptr;
	steamworks::ISteamGameServer014* m_authTarget = nullptr;
	int m_hookIds[3] = {};
	bool m_frameHook = false;
	bool m_enabled = false;
	uint64 m_generation = 0;
	void Detach();
	void SetFrameHook(bool enabled);
};

void OurGameFrameHook(bool simulating);

#include "../extension.h"
