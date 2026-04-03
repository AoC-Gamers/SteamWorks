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

#include "gsnatives.h"

static bool IsSteamWorksLoaded(void)
{
	return (g_SteamWorks.pSWGameServer->GetSteamClient() != NULL);
}

static ISteamGameServer *GetGSPointer(void)
{
	return g_SteamWorks.pSWGameServer->GetGameServer();
}

static ISteamFriends *GetFriendsPointer(void)
{
	return g_SteamWorks.pSWGameServer->GetFriends();
}

static CSteamID CreateCommonCSteamID(IGamePlayer *pPlayer, const cell_t *params, unsigned char universeplace = 2, unsigned char typeplace = 3)
{
	return g_SteamWorks.CreateCommonCSteamID(pPlayer, params, universeplace, typeplace);
}

static CSteamID CreateCommonCSteamID(uint32_t authid, const cell_t *params, unsigned char universeplace = 2, unsigned char typeplace = 3)
{
	return g_SteamWorks.CreateCommonCSteamID(authid, params, universeplace, typeplace);
}

static cell_t sm_IsVACEnabled(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();
	
	if (pServer == NULL)
	{
		return 0;
	}
	
	return pServer->BSecure() ? 1 : 0;
}

static cell_t sm_GetPublicIP(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();
	
	if (pServer == NULL)
	{
		return 0;
	}

	SteamIPAddress_t sAddr = pServer->GetPublicIP();
	if (!sAddr.IsSet())
	{
		return 0;
	}

	uint32_t ipaddr = sAddr.m_unIPv4;
	
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);
	for (char iter = 3; iter > -1; --iter)
	{
		addr[(~iter) & 0x03] = (static_cast<unsigned char>(ipaddr >> (iter * 8)) & 0xFF); /* I hate you; SteamTools. */
	}
	
	return 1;
}

static cell_t sm_GetPublicIPCell(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}

	SteamIPAddress_t sAddr = pServer->GetPublicIP();
	if (!sAddr.IsSet())
	{
		return 0;
	}

	return sAddr.m_unIPv4;
}

static cell_t sm_IsLoaded(IPluginContext *pContext, const cell_t *params)
{
	return IsSteamWorksLoaded() ? 1 : 0;
}

static cell_t sm_SetGameData(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}
	
	char *pData;
	pContext->LocalToString(params[1], &pData);
	
	pServer->SetGameData(pData);
	return 1;
}

static cell_t sm_SetGameDescription(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}
	
	char *pDesc;
	pContext->LocalToString(params[1], &pDesc);
	
	pServer->SetGameDescription(pDesc);
	return 1;
}

static cell_t sm_SetMapName(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}
	
	char *pMapName;
	pContext->LocalToString(params[1], &pMapName);
	
	pServer->SetMapName(pMapName);
	return 1;
}

static cell_t sm_IsConnected(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}

	return pServer->BLoggedOn() ? 1 : 0;
}

static cell_t sm_SetRule(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}

	char *pKey, *pValue;
	pContext->LocalToString(params[1], &pKey);
	pContext->LocalToString(params[2], &pValue);

	pServer->SetKeyValue(pKey, pValue);
	return 1;
}

static cell_t sm_ClearRules(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return 0;
	}

	pServer->ClearAllKeyValues();
	return 1;
}

static cell_t sm_UserHasLicenseForApp(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return k_EUserHasLicenseResultNoAuth;
	}
	
	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client); /* Man, including GameHelpers and PlayerHelpers for this native :(. */
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}
	
	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 3, 4);
	return pServer->UserHasLicenseForApp(checkid, params[2]);
}

static cell_t sm_UserHasLicenseForAppId(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return k_EUserHasLicenseResultNoAuth;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 3, 4);
	return pServer->UserHasLicenseForApp(checkid, params[2]);
}

static cell_t sm_GetClientSteamID(IPluginContext *pContext, const cell_t *params)
{
	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);

	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID steamId = CreateCommonCSteamID(pPlayer, params, 4, 5);

	char *steamIdBuffer;
	pContext->LocalToString(params[2], &steamIdBuffer);

	int numBytes = g_pSM->Format(steamIdBuffer, params[3], "%llu", steamId.ConvertToUint64());
	numBytes++; // account for null terminator
	
	return numBytes;
}

static cell_t sm_GetUserGroupStatus(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return false;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client); /* Man, including GameHelpers and PlayerHelpers for this native :(. */
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 3, 4);
	return pServer->RequestUserGroupStatus(checkid, CSteamID(params[2], k_EUniversePublic, k_EAccountTypeClan));
}

static cell_t sm_GetUserGroupStatusAuthID(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == NULL)
	{
		return false;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 3, 4);
	return pServer->RequestUserGroupStatus(checkid, CSteamID(params[2], k_EUniversePublic, k_EAccountTypeClan));
}

static cell_t sm_RequestUserInformation(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 3, 4);
	bool started = pFriends->RequestUserInformation(checkid, params[2] ? true : false) ? true : false;
	g_SteamWorks.pSWForward->NotifyPawnUserInformationRequested(checkid.GetAccountID(), started);
	return started ? 1 : 0;
}

static cell_t sm_RequestUserInformationAuthID(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 4, 5);
	bool started = pFriends->RequestUserInformation(checkid, params[2] ? true : false) ? true : false;
	g_SteamWorks.pSWForward->NotifyPawnUserInformationRequested(checkid.GetAccountID(), started);
	return started ? 1 : 0;
}

static cell_t sm_GetFriendPersonaName(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 4, 5);
	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	const char *persona = pFriends->GetFriendPersonaName(checkid);
	if (persona == NULL)
	{
		buffer[0] = '\0';
		return 0;
	}

	return g_pSM->Format(buffer, params[3], "%s", persona) + 1;
}

static cell_t sm_GetFriendPersonaNameAuthID(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 4, 5);
	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	const char *persona = pFriends->GetFriendPersonaName(checkid);
	if (persona == NULL)
	{
		buffer[0] = '\0';
		return 0;
	}

	return g_pSM->Format(buffer, params[3], "%s", persona) + 1;
}

static cell_t sm_GetFriendPersonaState(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return -1;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 2, 3);
	return static_cast<cell_t>(pFriends->GetFriendPersonaState(checkid));
}

static cell_t sm_GetFriendPersonaStateAuthID(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return -1;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 2, 3);
	return static_cast<cell_t>(pFriends->GetFriendPersonaState(checkid));
}

static cell_t sm_GetFriendRelationship(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return -1;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 2, 3);
	return static_cast<cell_t>(pFriends->GetFriendRelationship(checkid));
}

static cell_t sm_GetFriendRelationshipAuthID(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return -1;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 2, 3);
	return static_cast<cell_t>(pFriends->GetFriendRelationship(checkid));
}

static cell_t sm_GetPlayerNickname(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 4, 5);
	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	const char *nickname = pFriends->GetPlayerNickname(checkid);
	if (nickname == NULL)
	{
		buffer[0] = '\0';
		return 0;
	}

	return g_pSM->Format(buffer, params[3], "%s", nickname) + 1;
}

static cell_t sm_GetPlayerNicknameAuthID(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	CSteamID checkid = CreateCommonCSteamID(params[1], params, 4, 5);
	char *buffer;
	pContext->LocalToString(params[2], &buffer);

	const char *nickname = pFriends->GetPlayerNickname(checkid);
	if (nickname == NULL)
	{
		buffer[0] = '\0';
		return 0;
	}

	return g_pSM->Format(buffer, params[3], "%s", nickname) + 1;
}

static cell_t sm_GetFriendGamePlayed(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	int client = gamehelpers->ReferenceToIndex(params[1]);
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == NULL || pPlayer->IsConnected() == false)
	{
		return pContext->ThrowNativeError("Client index %d is invalid", params[1]);
	}

	CSteamID checkid = CreateCommonCSteamID(pPlayer, params, 7, 8);
	FriendGameInfo_t info;
	if (!pFriends->GetFriendGamePlayed(checkid, &info))
	{
		return 0;
	}

	cell_t *appid, *ip, *gamePort, *queryPort, *lobbyAuthid;
	pContext->LocalToPhysAddr(params[2], &appid);
	pContext->LocalToPhysAddr(params[3], &ip);
	pContext->LocalToPhysAddr(params[4], &gamePort);
	pContext->LocalToPhysAddr(params[5], &queryPort);
	pContext->LocalToPhysAddr(params[6], &lobbyAuthid);

	*appid = static_cast<cell_t>(info.m_gameID.AppID());
	*ip = static_cast<cell_t>(info.m_unGameIP);
	*gamePort = static_cast<cell_t>(info.m_usGamePort);
	*queryPort = static_cast<cell_t>(info.m_usQueryPort);
	*lobbyAuthid = info.m_steamIDLobby.IsValid() ? static_cast<cell_t>(info.m_steamIDLobby.GetAccountID()) : 0;
	return 1;
}

static cell_t sm_RequestClanOfficerList(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	SteamAPICall_t hCall = pFriends->RequestClanOfficerList(CSteamID(params[1], k_EUniversePublic, k_EAccountTypeClan));
	return (hCall != k_uAPICallInvalid) ? 1 : 0;
}

static cell_t sm_GetClanOfficerCount(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return -1;
	}

	return static_cast<cell_t>(pFriends->GetClanOfficerCount(CSteamID(params[1], k_EUniversePublic, k_EAccountTypeClan)));
}

static cell_t sm_GetClanOfficerByIndex(IPluginContext *pContext, const cell_t *params)
{
	ISteamFriends *pFriends = GetFriendsPointer();

	if (pFriends == NULL)
	{
		return 0;
	}

	CSteamID officer = pFriends->GetClanOfficerByIndex(CSteamID(params[1], k_EUniversePublic, k_EAccountTypeClan), params[2]);
	if (!officer.IsValid())
	{
		return 0;
	}

	return static_cast<cell_t>(officer.GetAccountID());
}

static sp_nativeinfo_t gsnatives[] = {
	{"SteamWorks_IsVACEnabled",				sm_IsVACEnabled},
	{"SteamWorks_GetPublicIP",				sm_GetPublicIP},
	{"SteamWorks_GetPublicIPCell",				sm_GetPublicIPCell},
	{"SteamWorks_IsLoaded",				sm_IsLoaded},
	{"SteamWorks_SetGameData",				sm_SetGameData},
	{"SteamWorks_SetGameDescription",	sm_SetGameDescription},
	{"SteamWorks_SetMapName",	sm_SetMapName},
	{"SteamWorks_IsConnected",				sm_IsConnected},
	{"SteamWorks_SetRule",						sm_SetRule},
	{"SteamWorks_ClearRules",						sm_ClearRules},
	{"SteamWorks_HasLicenseForApp",			sm_UserHasLicenseForApp},
	{"SteamWorks_HasLicenseForAppId",			sm_UserHasLicenseForAppId},
	{"SteamWorks_GetClientSteamID",			sm_GetClientSteamID},
	{"SteamWorks_GetUserGroupStatus",			sm_GetUserGroupStatus},
	{"SteamWorks_GetUserGroupStatusAuthID",			sm_GetUserGroupStatusAuthID},
	{"SteamWorks_RequestUserInformation",			sm_RequestUserInformation},
	{"SteamWorks_RequestUserInformationAuthID",		sm_RequestUserInformationAuthID},
	{"SteamWorks_GetFriendPersonaName",			sm_GetFriendPersonaName},
	{"SteamWorks_GetFriendPersonaNameAuthID",		sm_GetFriendPersonaNameAuthID},
	{"SteamWorks_GetFriendPersonaState",			sm_GetFriendPersonaState},
	{"SteamWorks_GetFriendPersonaStateAuthID",		sm_GetFriendPersonaStateAuthID},
	{"SteamWorks_GetFriendRelationship",			sm_GetFriendRelationship},
	{"SteamWorks_GetFriendRelationshipAuthID",		sm_GetFriendRelationshipAuthID},
	{"SteamWorks_GetPlayerNickname",				sm_GetPlayerNickname},
	{"SteamWorks_GetPlayerNicknameAuthID",			sm_GetPlayerNicknameAuthID},
	{"SteamWorks_GetFriendGamePlayed",				sm_GetFriendGamePlayed},
	{"SteamWorks_RequestClanOfficerList",			sm_RequestClanOfficerList},
	{"SteamWorks_GetClanOfficerCount",				sm_GetClanOfficerCount},
	{"SteamWorks_GetClanOfficerByIndex",			sm_GetClanOfficerByIndex},
	{NULL,											NULL}
};

SteamWorksGSNatives::SteamWorksGSNatives()
{
	sharesys->AddNatives(myself, gsnatives);
}

SteamWorksGSNatives::~SteamWorksGSNatives()
{
	/* We tragically can't remove ourselves... hopefully no one uses this class, you know, like a class. */
}
