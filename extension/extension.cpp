#include "extension.h"
#include <cstdlib>

namespace {

template <typename T>
void DeleteAndNull(T*& value)
{
	delete value;
	value = nullptr;
}

}  // namespace

SteamWorks g_SteamWorks;

SMEXT_LINK(&g_SteamWorks);
 
bool SteamWorks::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	sharesys->RegisterLibrary(myself, "SteamWorks");

	pSWGameData = new SteamWorksGameData;
	pSWGameServer = new SteamWorksGameServer;
	pSWHTTP = new SteamWorksHTTP;
	
	pSWHTTPNatives = new SteamWorksHTTPNatives;
	pSWForward = new SteamWorksForwards;
	pGSNatives = new SteamWorksGSNatives;
	pGSHooks = new SteamWorksGSHooks;
	pGSDetours = new SteamWorksGSDetours;
	pSSNatives = new SteamWorksSSNatives;
	pGCHooks = new SteamWorksGCHooks;
	pGCNatives = new SteamWorksGCNatives;
	return true;
}

void SteamWorks::SDK_OnUnload()
{
	DeleteAndNull(pGCNatives);
	DeleteAndNull(pGCHooks);
	DeleteAndNull(pSSNatives);
	DeleteAndNull(pGSDetours);
	DeleteAndNull(pGSHooks);
	DeleteAndNull(pGSNatives);
	DeleteAndNull(pSWForward);
	DeleteAndNull(pSWHTTPNatives);
	
	DeleteAndNull(pSWHTTP);
	DeleteAndNull(pSWGameServer);
	DeleteAndNull(pSWGameData);
}

CSteamID SteamWorks::CreateCommonCSteamID(IGamePlayer* pPlayer, const cell_t* params, unsigned char universeplace, unsigned char typeplace)
{
	EUniverse universe = k_EUniversePublic;
	EAccountType type = k_EAccountTypeIndividual;
	
	const char* pAuth = pPlayer->GetAuthString(false);
	if (pAuth == nullptr || pAuth[0] == '\0' || strlen(pAuth) < 7 || pAuth[6] == 'I')
	{
		return CreateCommonCSteamID(pPlayer->GetSteamAccountID(false), params, universeplace, typeplace);
	}

	if (pAuth[0] == '[')
	{
		universe = static_cast<EUniverse>(atoi(&pAuth[3]));
	}
	else
	{
		universe = static_cast<EUniverse>(atoi(&pAuth[6]));
	}

	if (universe == k_EUniverseInvalid)
	{
		universe = k_EUniversePublic;
	}

	return CSteamID(pPlayer->GetSteamAccountID(false), universe, type);
}

CSteamID SteamWorks::CreateCommonCSteamID(uint32_t authid, const cell_t* params, unsigned char universeplace, unsigned char typeplace)
{
	EUniverse universe = k_EUniversePublic;
	EAccountType type = k_EAccountTypeIndividual;
	if (params[0] >= universeplace)
	{
		universe = static_cast<EUniverse>(params[universeplace]);
	}

	if (params[0] >= typeplace)
	{
		type = static_cast<EAccountType>(params[typeplace]);
	}

	return CSteamID(authid, universe, type);
}
