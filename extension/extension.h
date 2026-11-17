#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include "isteamgameserver.h"
#include "steam_gameserver.h"
#include "core/swtypes.h"

#include "core/swgameserver.h"
#include "core/swgamedata.h"
#include "core/swclientlifecycle.h"

#include "core/swforwards.h"
#include "core/swstats.h"
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
	bool SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlength, bool late) override;
	bool SDK_OnMetamodUnload(char* error, size_t maxlength) override;

	void RuntimeStarted();
	void RuntimeStopped();
	bool IsRuntimeAvailable() const { return m_runtimeAvailable && !m_unloading; }

	CSteamID CreateCommonCSteamID(IGamePlayer* player, EUniverse fallbackUniverse, EAccountType fallbackType);
	CSteamID CreateCommonCSteamID(steamworks::AccountId account, EUniverse universe, EAccountType type);

public:
	SteamWorksForwards* pSWForward = nullptr;
	SteamWorksClientLifecycle* pClientLifecycle = nullptr;
	SteamWorksGameData* pSWGameData = nullptr;
	SteamWorksGameServer* pSWGameServer = nullptr;
	SteamWorksGSNatives* pGSNatives = nullptr;
	SteamWorksGSHooks* pGSHooks = nullptr;
	SteamWorksSSNatives* pSSNatives = nullptr;
	SteamWorksStats* pSWStats = nullptr;
	SteamWorksGSDetours* pGSDetours = nullptr;
	SteamWorksHTTP* pSWHTTP = nullptr;
	SteamWorksHTTPNatives* pSWHTTPNatives = nullptr;
	SteamWorksGCHooks* pGCHooks = nullptr;
	SteamWorksGCNatives* pGCNatives = nullptr;

private:
	bool m_runtimeAvailable = true;
	bool m_unloading = false;
};

extern SteamWorks g_SteamWorks;
#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
