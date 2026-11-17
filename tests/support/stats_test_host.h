#pragma once

// Unit-test the production native entrypoints with controlled host/Steam doubles.
// These guards replace host dependencies only in this translation unit. The real
// ssnatives.h and ssnatives.cpp are included below; no production test switch or
// alternate native implementation is compiled into the extension.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "steamclientpublic.h"
#include "../../extension/core/swtypes.h"

using cell_t = int32_t;
constexpr int SP_ERROR_NONE = 0;
constexpr int conversionError = 42;
constexpr cell_t sentinel = 0x12345678;

static cell_t sp_ftoc(float value)
{
	static_assert(sizeof(value) == sizeof(cell_t), "Float/cell size mismatch");
	cell_t result;
	std::memcpy(&result, &value, sizeof(result));
	return result;
}

struct IGamePlayer
{
	bool connected = true;
	bool authorized = true;
	bool fake = false;
	AccountID_t account = 123;
	uint64 steamId = CSteamID(123, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64();
	int unvalidatedReads = 0;
	bool IsConnected() const { return connected; }
	bool IsAuthorized() const { return authorized; }
	bool IsFakeClient() const { return fake; }
	unsigned int GetSteamAccountID(bool validated = true)
	{
		if (!validated) { ++unvalidatedReads; }
		return account;
	}
	uint64_t GetSteamId64(bool validated = true)
	{
		if (!validated) { ++unvalidatedReads; }
		return steamId;
	}
};

struct IPluginContext
{
	cell_t storage[3] = {0x11223344, sentinel, 0x55667788};
	char key[16] = "test.stat";
	bool invalidName = false;
	bool invalidOutput = false;
	int errors = 0;
	int errorCode = 0;
	int nameResolutions = 0;
	int outputResolutions = 0;

	int LocalToString(cell_t, char** output)
	{
		++nameResolutions;
		if (invalidName) {
			return conversionError; // Deliberately leave the pointer untouched.
		}
		*output = key;
		return SP_ERROR_NONE;
	}

	int LocalToPhysAddr(cell_t, cell_t** output)
	{
		++outputResolutions;
		if (invalidOutput) {
			return conversionError;
		}
		*output = &storage[1];
		return SP_ERROR_NONE;
	}

	cell_t ThrowNativeError(const char*, ...)
	{
		++errors;
		return 0;
	}
	cell_t ThrowNativeErrorEx(int code, const char*, ...)
	{
		++errors;
		errorCode = code;
		return 0;
	}
};

struct ISteamGameServerStats
{
	bool succeeds = false;
	bool writesOnFailure = false;
	int calls = 0;
	int integerCalls = 0;
	int floatCalls = 0;
	int32 integerValue = -271;
	float floatValue = -3.25f;
	int requests = 0;
	CSteamID requestedIdentity;
	SteamAPICall_t requestResult = 1;

	SteamAPICall_t RequestUserStats(CSteamID identity)
	{
		++requests;
		requestedIdentity = identity;
		return requestResult;
	}
	bool GetUserStat(CSteamID, const char*, int32* value)
	{
		++calls;
		++integerCalls;
		if (succeeds || writesOnFailure) {
			*value = integerValue;
		}
		return succeeds;
	}
	bool GetUserStat(CSteamID, const char*, float* value)
	{
		++calls;
		++floatCalls;
		if (succeeds || writesOnFailure) {
			*value = floatValue;
		}
		return succeeds;
	}
};

struct ISteamUtils
{
	AppId_t appid = 550;
	AppId_t GetAppID() { return appid; }
};

struct GameServer
{
	ISteamGameServerStats* stats = nullptr;
	ISteamUtils* utils = nullptr;
	ISteamGameServerStats* GetServerStats() { return stats; }
	ISteamUtils* GetUtils() { return utils; }
};
struct StatsOwner {
    bool accepting = true;
    int calls = 0;
    AppId_t capturedApp = 0;
    bool Request(ISteamGameServerStats* stats, CSteamID identity, AppId_t appid) {
        ++calls;
        capturedApp = appid;
        return accepting && stats->RequestUserStats(identity) != k_uAPICallInvalid;
    }
} statsOwner;
struct SteamWorks
{
	GameServer* pSWGameServer = nullptr;
	StatsOwner* pSWStats = &statsOwner;
	int legacyIdentityCalls = 0;
	CSteamID CreateCommonCSteamID(IGamePlayer*, EUniverse, EAccountType)
	{
		++legacyIdentityCalls;
		return CSteamID(123, k_EUniversePublic, k_EAccountTypeIndividual);
	}
	CSteamID CreateCommonCSteamID(steamworks::AccountId account, EUniverse universe, EAccountType type)
	{
		++legacyIdentityCalls;
		return CSteamID(account.value, universe, type);
	}
} g_SteamWorks;

struct GameHelpers
{
	bool entityPresent = true;
	int ReferenceToIndex(cell_t client)
	{
		if (!entityPresent) { return -1; }
		// Controlled reference mapping, independent of player-slot availability.
		return client == static_cast<cell_t>(0x80010001u) ? 1 : client;
	}
} gameHelpers;
auto* gamehelpers = &gameHelpers;

struct PlayerHelpers
{
	IGamePlayer player;
	bool exists = true;
	int lookups = 0;
	int lastIndex = 0;
	int GetMaxClients() { return 32; }
	IGamePlayer* GetGamePlayer(int index)
	{
		++lookups;
		lastIndex = index;
		return exists && index >= 1 && index <= GetMaxClients() ? &player : nullptr;
	}
} playerHelpers;
auto* playerhelpers = &playerHelpers;

using Native = cell_t (*)(IPluginContext*, const cell_t*);
struct sp_nativeinfo_t { const char* name; Native function; };
struct ShareSystem
{
	sp_nativeinfo_t* registered = nullptr;
	void AddNatives(void*, sp_nativeinfo_t* natives) { registered = natives; }
} shareSystem;
auto* sharesys = &shareSystem;
void* myself = nullptr;

#include "../../extension/natives/ssnatives.cpp"

