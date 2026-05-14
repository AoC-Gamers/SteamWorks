#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include "isteamgameserver.h"
#include "steam_gameserver.h"

#include "core/swgameserver.h"
#include "core/swgamedata.h"

#include "core/swforwards.h"
#include "natives/gsnatives.h"
#include "hooks/swgshooks.h"
#include "hooks/swgchooks.h"
#include "natives/ssnatives.h"
#include "natives/gcnatives.h"
#include "hooks/swgsdetours.h"
#include "http/swhttp.h"

/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class SteamWorks :
	public SDKExtension
{
public:
	bool SDK_OnLoad(char* error, size_t maxlength, bool late) override;
	void SDK_OnUnload() override;

	CSteamID CreateCommonCSteamID(IGamePlayer *pPlayer, const cell_t *params, unsigned char universeplace, unsigned char typeplace);
	CSteamID CreateCommonCSteamID(uint32_t authid, const cell_t *params, unsigned char universeplace, unsigned char typeplace);

public:
	SteamWorksForwards* pSWForward = nullptr;
	SteamWorksGameData* pSWGameData = nullptr;
	SteamWorksGameServer* pSWGameServer = nullptr;
	SteamWorksGSNatives* pGSNatives = nullptr;
	SteamWorksGSHooks* pGSHooks = nullptr;
	SteamWorksSSNatives* pSSNatives = nullptr;
	SteamWorksGSDetours* pGSDetours = nullptr;
	SteamWorksHTTP* pSWHTTP = nullptr;
	SteamWorksHTTPNatives* pSWHTTPNatives = nullptr;
	SteamWorksGCHooks* pGCHooks = nullptr;
	SteamWorksGCNatives* pGCNatives = nullptr;
};

extern SteamWorks g_SteamWorks;
#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
