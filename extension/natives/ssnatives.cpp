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

#include "ssnatives.h"
#include "../core/swpawn.h"

namespace {

ISteamGameServerStats *GetServerStatsPointer(void)
{
	return g_SteamWorks.pSWGameServer->GetServerStats();
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

ISteamGameServerStats* GetServerStatsForApp(steamworks::AppId appid)
{
	ISteamGameServerStats* stats = GetServerStatsPointer();
	if (stats == nullptr || appid.value == k_uAppIdInvalid) {
		return nullptr;
	}

	ISteamUtils* utils = g_SteamWorks.pSWGameServer->GetUtils();
	if (utils == nullptr || utils->GetAppID() != appid.value) {
		return nullptr;
	}

	return stats;
}

CSteamID CreateStatsAccountIdentity(steamworks::AccountId account)
{
	return steamworks::IndividualIdentity(account);
}

bool GetStatsClientIdentity(IGamePlayer* player, CSteamID& identity)
{
	if (player->IsFakeClient() || !player->IsAuthorized()) {
		return false;
	}

	AccountID_t account = player->GetSteamAccountID(true);
	if (account == 0) {
		return false;
	}

	uint64 steamId = player->GetSteamId64(true);
	if (steamId != 0) {
		identity = CSteamID(steamId);
		return identity.IsValid() && identity.GetEAccountType() == k_EAccountTypeIndividual
			&& identity.GetAccountID() == account;
	}

	// Only a validated AccountID may supply the public-universe fallback.
	identity = CreateStatsAccountIdentity(steamworks::AccountId(account));
	return identity.IsValid();
}

IGamePlayer* GetConnectedPlayerOrError(IPluginContext* pContext, cell_t client)
{
	// Player slots can be connected before their entity is available.
	if (client < 1 || client > playerhelpers->GetMaxClients()) {
		pContext->ThrowNativeError("Client index %d is invalid", client);
		return nullptr;
	}
	IGamePlayer* pPlayer = playerhelpers->GetGamePlayer(client);
	if (pPlayer == nullptr || !pPlayer->IsConnected()) {
		pContext->ThrowNativeError("Client index %d is invalid", client);
		return nullptr;
	}

	return pPlayer;
}

bool GetStatArguments(IPluginContext* pContext, const cell_t* params, char*& name, cell_t*& value)
{
	return steamworks::pawn::String(pContext, params[2], name)
		&& steamworks::pawn::Output(pContext, params[3], value);
}

static cell_t sm_RequestStatsAuthID(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	ISteamGameServerStats *pStats = GetServerStatsForApp(steamworks::pawn::App(params[2]));
	
	if (pStats == nullptr)
	{
		return 0;
	}

	CSteamID checkid = CreateStatsAccountIdentity(steamworks::pawn::Account(params[1]));
	if (!checkid.IsValid()) {
		return 0;
	}
	return g_SteamWorks.pSWStats != nullptr
		&& g_SteamWorks.pSWStats->Request(pStats, checkid, steamworks::pawn::App(params[2]).value) ? 1 : 0;
}

static cell_t sm_RequestUserStats(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServerStats *pStats = GetServerStatsForApp(steamworks::pawn::App(params[2]));

	if (pStats == nullptr)
	{
		return 0;
	}

	IGamePlayer* pPlayer = GetConnectedPlayerOrError(pContext, params[1]);
	if (pPlayer == nullptr) {
		return 0;
	}

	CSteamID checkid;
	if (!GetStatsClientIdentity(pPlayer, checkid)) {
		return 0;
	}
	return g_SteamWorks.pSWStats != nullptr
		&& g_SteamWorks.pSWStats->Request(pStats, checkid, steamworks::pawn::App(params[2]).value) ? 1 : 0;
}

static cell_t sm_GetStatCell(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServerStats *pStats = GetServerStatsPointer();

	if (pStats == nullptr)
	{
		return 0;
	}

	IGamePlayer* pPlayer = GetConnectedPlayerOrError(pContext, params[1]);
	if (pPlayer == nullptr) {
		return 0;
	}
	
	char* pName;
	cell_t* pValue;
	if (!GetStatArguments(pContext, params, pName, pValue)) {
		return 0;
	}
	CSteamID checkid;
	if (!ReadIdentity(pContext, pPlayer, params, 4, 5, checkid)) { return 0; }
	int32 value = 0;
	if (!pStats->GetUserStat(checkid, pName, &value)) {
		return 0;
	}

	*pValue = value;
	return 1;
}

static cell_t sm_GetStatAuthIDCell(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServerStats *pStats = GetServerStatsPointer();

	if (pStats == nullptr)
	{
		return 0;
	}

	char* pName;
	cell_t* pValue;
	if (!GetStatArguments(pContext, params, pName, pValue)) {
		return 0;
	}
	CSteamID checkid;
	if (!ReadIdentity(pContext, steamworks::pawn::Account(params[1]), params, 4, 5, checkid)) { return 0; }
	int32 value = 0;
	if (!pStats->GetUserStat(checkid, pName, &value)) {
		return 0;
	}

	*pValue = value;
	return 1;
}

static cell_t sm_GetStatFloat(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServerStats *pStats = GetServerStatsPointer();

	if (pStats == nullptr)
	{
		return 0;
	}

	IGamePlayer* pPlayer = GetConnectedPlayerOrError(pContext, params[1]);
	if (pPlayer == nullptr) {
		return 0;
	}
	
	char* pName;
	cell_t* pValue;
	if (!GetStatArguments(pContext, params, pName, pValue)) {
		return 0;
	}
	CSteamID checkid;
	if (!ReadIdentity(pContext, pPlayer, params, 4, 5, checkid)) { return 0; }
	float value = 0.0f;
	if (!pStats->GetUserStat(checkid, pName, &value)) {
		return 0;
	}

	*pValue = sp_ftoc(value);
	return 1;
}

static cell_t sm_GetStatAuthIDFloat(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameServerStats *pStats = GetServerStatsPointer();

	if (pStats == nullptr)
	{
		return 0;
	}

	char* pName;
	cell_t* pValue;
	if (!GetStatArguments(pContext, params, pName, pValue)) {
		return 0;
	}
	CSteamID checkid;
	if (!ReadIdentity(pContext, steamworks::pawn::Account(params[1]), params, 4, 5, checkid)) { return 0; }
	float value = 0.0f;
	if (!pStats->GetUserStat(checkid, pName, &value)) {
		return 0;
	}

	*pValue = sp_ftoc(value);
	return 1;
}

static sp_nativeinfo_t ssnatives[] = {
	{"SteamWorks_RequestStatsAuthID",				sm_RequestStatsAuthID},
	{"SteamWorks_RequestStats",				sm_RequestUserStats},
	{"SteamWorks_GetStatCell",				sm_GetStatCell},
	{"SteamWorks_GetStatAuthIDCell",				sm_GetStatAuthIDCell},
	{"SteamWorks_GetStatFloat",				sm_GetStatFloat},
	{"SteamWorks_GetStatAuthIDFloat",				sm_GetStatAuthIDFloat},
	{nullptr,											nullptr}
};

}  // namespace

SteamWorksSSNatives::SteamWorksSSNatives()
{
	sharesys->AddNatives(myself, ssnatives);
}

SteamWorksSSNatives::~SteamWorksSSNatives()
{
}
