#include "../extension.h"
#include <cstdlib>
#include <cstring>

CSteamID SteamWorks::CreateCommonCSteamID(IGamePlayer* player, EUniverse fallbackUniverse, EAccountType fallbackType)
{
	const char* auth = player->GetAuthString(false);
	if (auth == nullptr || auth[0] == '\0' || std::strlen(auth) < 7 || auth[6] == 'I') {
		return CreateCommonCSteamID(steamworks::AccountId(player->GetSteamAccountID(false)), fallbackUniverse, fallbackType);
	}
	int value = std::atoi(auth[0] == '[' ? &auth[3] : &auth[6]);
	EUniverse universe = k_EUniversePublic;
	if (value > k_EUniverseInvalid && value < k_EUniverseMax) { universe = static_cast<EUniverse>(value); }
	return CSteamID(player->GetSteamAccountID(false), universe, k_EAccountTypeIndividual);
}

CSteamID SteamWorks::CreateCommonCSteamID(steamworks::AccountId account, EUniverse universe, EAccountType type)
{
	return CSteamID(account.value, universe, type);
}
