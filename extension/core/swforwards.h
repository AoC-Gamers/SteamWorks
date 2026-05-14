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
#include "steam_gameserver.h"

typedef uint32_t Account_t;

class SteamWorksForwards
{
public:
	SteamWorksForwards();
	~SteamWorksForwards();

	void NotifyPawnValidateClient(Account_t parent, Account_t child);

private:
	void ExecuteValidateClientForward(IForward* forward, Account_t parent, Account_t child);
	void ExecuteSteamServerResultForward(IForward* forward, cell_t result);

	STEAM_GAMESERVER_CALLBACK(SteamWorksForwards, OnGSClientApprove, GSClientApprove_t, m_CallbackGSClientApprove);
	STEAM_GAMESERVER_CALLBACK(SteamWorksForwards, OnValidateTicket, ValidateAuthTicketResponse_t, m_CallbackValidateTicket);
	STEAM_GAMESERVER_CALLBACK(SteamWorksForwards, OnSteamServersConnected, SteamServersConnected_t, m_CallbackSteamConnected);
	STEAM_GAMESERVER_CALLBACK(SteamWorksForwards, OnSteamServersConnectFailure, SteamServerConnectFailure_t, m_CallbackSteamConnectFailure);
	STEAM_GAMESERVER_CALLBACK(SteamWorksForwards, OnSteamServersDisconnected, SteamServersDisconnected_t, m_CallbackSteamDisconnected);
	STEAM_GAMESERVER_CALLBACK(SteamWorksForwards, OnGroupStatusResult, GSClientGroupStatus_t, m_CallbackGroupStatus);

	IForward* pValidateClientForward = nullptr;
	IForward* pLegacyValidateClientForward = nullptr;
	IForward* pSteamServersConnectedForward = nullptr;
	IForward* pSteamServersConnectFailureForward = nullptr;
	IForward* pSteamServersDisconnectedForward = nullptr;
	IForward* pClientGroupStatusForward = nullptr;
};
