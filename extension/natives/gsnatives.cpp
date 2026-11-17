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
#include "../core/swpawn.h"
#include <cstring>

namespace {

bool IsSteamWorksLoaded(void)
{
	return g_SteamWorks.pSWGameServer->GetSteamClient() != nullptr;
}

ISteamGameServer *GetGSPointer(void)
{
	return g_SteamWorks.pSWGameServer->GetGameServer();
}

template <typename Identity>
bool ReadIdentity(IPluginContext* context, Identity identity, const cell_t* params,
	unsigned char universePosition, unsigned char typePosition, CSteamID& output)
{
	EUniverse universe;
	EAccountType type;
	if (!steamworks::pawn::IdentityOptions(context, params, universePosition, typePosition, universe, type)) { return false; }
	output = g_SteamWorks.CreateCommonCSteamID(identity, universe, type);
	return true;
}

IGamePlayer* GetConnectedPlayerOrError(IPluginContext* pContext, cell_t clientRef)
{
	int client = gamehelpers->ReferenceToIndex(clientRef);
	IGamePlayer* pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == nullptr || !pPlayer->IsConnected()) {
		pContext->ThrowNativeError("Client index %d is invalid", clientRef);
		return nullptr;
	}

	return pPlayer;
}

cell_t sm_IsVACEnabled(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	(void)params;
	ISteamGameServer *pServer = GetGSPointer();
	
	if (pServer == nullptr)
	{
		return 0;
	}
	
	return pServer->BSecure() ? 1 : 0;
}

static cell_t sm_GetPublicIP(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();
	
	if (pServer == nullptr)
	{
		return 0;
	}

	SteamIPAddress_t sAddr = pServer->GetPublicIP();
	if (sAddr.m_eType != k_ESteamIPTypeIPv4 || !sAddr.IsSet())
	{
		return 0;
	}

	uint32_t ipaddr = sAddr.m_unIPv4;
	
	cell_t* addr;
	int error = pContext->LocalToPhysAddr(params[1], &addr);
	if (error != SP_ERROR_NONE) {
		return pContext->ThrowNativeErrorEx(error, "Invalid IPv4 output address");
	}
	for (int octet = 0; octet < 4; ++octet)
	{
		addr[octet] = static_cast<cell_t>((ipaddr >> (24 - octet * 8)) & 0xFFu);
	}
	
	return 1;
}

static cell_t sm_GetPublicIPCell(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	(void)params;
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}

	SteamIPAddress_t sAddr = pServer->GetPublicIP();
	if (sAddr.m_eType != k_ESteamIPTypeIPv4 || !sAddr.IsSet())
	{
		return 0;
	}

	cell_t ip;
	static_assert(sizeof(ip) == sizeof(sAddr.m_unIPv4), "IPv4 must fit a SourcePawn cell");
	std::memcpy(&ip, &sAddr.m_unIPv4, sizeof(ip));
	return ip;
}

static cell_t sm_IsLoaded(IPluginContext *pContext, const cell_t *params)
{
	return IsSteamWorksLoaded() ? 1 : 0;
}

static cell_t sm_SetGameData(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}
	
	char *pData;
	if (!steamworks::pawn::String(pContext, params[1], pData)) { return 0; }
	
	pServer->SetGameData(pData);
	return 1;
}

static cell_t sm_SetGameDescription(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}
	
	char *pDesc;
	if (!steamworks::pawn::String(pContext, params[1], pDesc)) { return 0; }
	
	pServer->SetGameDescription(pDesc);
	return 1;
}

static cell_t sm_SetMapName(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}
	
	char *pMapName;
	if (!steamworks::pawn::String(pContext, params[1], pMapName)) { return 0; }
	
	pServer->SetMapName(pMapName);
	return 1;
}

static cell_t sm_IsConnected(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	(void)params;
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}

	return pServer->BLoggedOn() ? 1 : 0;
}

static cell_t sm_SetRule(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}

	char *pKey, *pValue;
	if (!steamworks::pawn::String(pContext, params[1], pKey)) { return 0; }
	if (!steamworks::pawn::String(pContext, params[2], pValue)) { return 0; }

	pServer->SetKeyValue(pKey, pValue);
	return 1;
}

static cell_t sm_ClearRules(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	(void)params;
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return 0;
	}

	pServer->ClearAllKeyValues();
	return 1;
}

static cell_t sm_UserHasLicenseForApp(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return k_EUserHasLicenseResultNoAuth;
	}
	
	IGamePlayer* pPlayer = GetConnectedPlayerOrError(pContext, params[1]);
	if (pPlayer == nullptr) {
		return 0;
	}
	
	CSteamID checkid;
	if (!ReadIdentity(pContext, pPlayer, params, 3, 4, checkid)) { return 0; }
	return pServer->UserHasLicenseForApp(checkid, steamworks::pawn::App(params[2]).value);
}

static cell_t sm_UserHasLicenseForAppId(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return k_EUserHasLicenseResultNoAuth;
	}

	CSteamID checkid;
	if (!ReadIdentity(pContext, steamworks::pawn::Account(params[1]), params, 3, 4, checkid)) { return 0; }
	return pServer->UserHasLicenseForApp(checkid, steamworks::pawn::App(params[2]).value);
}

static cell_t sm_GetClientSteamID(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer* pPlayer = GetConnectedPlayerOrError(pContext, params[1]);
	if (pPlayer == nullptr) {
		return 0;
	}

	if (params[3] < 0) {
		return pContext->ThrowNativeError("SteamID buffer length cannot be negative");
	}
	if (params[3] == 0) {
		return 0;
	}

	char* steamIdBuffer;
	int error = pContext->LocalToString(params[2], &steamIdBuffer);
	if (error != SP_ERROR_NONE) {
		return pContext->ThrowNativeErrorEx(error, "Invalid SteamID output address");
	}

	CSteamID steamId;
	if (!ReadIdentity(pContext, pPlayer, params, 4, 5, steamId)) { return 0; }
	size_t written = g_pSM->Format(steamIdBuffer, static_cast<size_t>(params[3]), "%llu", steamId.ConvertToUint64());
	// Preserve the legacy count including NUL; Format counts only text bytes.
	return static_cast<cell_t>(written + 1);
}

static cell_t sm_GetUserGroupStatus(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return false;
	}

	IGamePlayer* pPlayer = GetConnectedPlayerOrError(pContext, params[1]);
	if (pPlayer == nullptr) {
		return 0;
	}

	CSteamID checkid;
	if (!ReadIdentity(pContext, pPlayer, params, 3, 4, checkid)) { return 0; }
	return pServer->RequestUserGroupStatus(checkid, CSteamID(steamworks::pawn::Account(params[2]).value, k_EUniversePublic, k_EAccountTypeClan));
}

static cell_t sm_GetUserGroupStatusAuthID(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	ISteamGameServer *pServer = GetGSPointer();

	if (pServer == nullptr)
	{
		return false;
	}

	CSteamID checkid;
	if (!ReadIdentity(pContext, steamworks::pawn::Account(params[1]), params, 3, 4, checkid)) { return 0; }
	return pServer->RequestUserGroupStatus(checkid, CSteamID(steamworks::pawn::Account(params[2]).value, k_EUniversePublic, k_EAccountTypeClan));
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
	{nullptr,											nullptr}
};

}  // namespace

SteamWorksGSNatives::SteamWorksGSNatives()
{
	sharesys->AddNatives(myself, gsnatives);
}

SteamWorksGSNatives::~SteamWorksGSNatives()
{
}
