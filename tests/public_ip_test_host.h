#pragma once

// Exercise the production GameServer native table with real SDK address/ID
// types. SourceMod memory and the Steam service are controlled test doubles.
// Unrelated GameServer operations below are stubs, not tested implementations.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include "steamclientpublic.h"
#include "../extension/core/swtypes.h"

using cell_t = int32_t;
constexpr int SP_ERROR_NONE = 0;
constexpr int addressError = 42;

struct IPluginContext
{
	cell_t memory[6] = {0x12345678, 11, 22, 33, 44, 0x23456789};
	bool invalidOutput = false;
	int resolutions = 0;
	int errors = 0;
	int errorCode = 0;
	const char* errorMessage = nullptr;
	int stringResolutions = 0;
	int stringOffset = 0;
	char string[32] = {};
	int LocalToPhysAddr(cell_t, cell_t** output)
	{
		++resolutions;
		if (invalidOutput) { return addressError; }
		*output = &memory[1];
		return SP_ERROR_NONE;
	}
	int LocalToString(cell_t, char** output)
	{
		++stringResolutions;
		if (invalidOutput) { return addressError; }
		*output = string + stringOffset;
		return SP_ERROR_NONE;
	}
	cell_t ThrowNativeError(const char* message, ...) { ++errors; errorMessage = message; return 0; }
	cell_t ThrowNativeErrorEx(int code, const char* message, ...) { ++errors; errorCode = code; errorMessage = message; return 0; }
};

struct ISteamGameServer
{
	SteamIPAddress_t address = {};
	int queries = 0;
	SteamIPAddress_t GetPublicIP() { ++queries; return address; }
	bool BSecure() { return true; }
	bool BLoggedOn() { return true; }
	void SetGameData(const char*) {}
	void SetGameDescription(const char*) {}
	void SetMapName(const char*) {}
	void SetKeyValue(const char*, const char*) {}
	void ClearAllKeyValues() {}
	EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID, int) { return k_EUserHasLicenseResultNoAuth; }
	bool RequestUserGroupStatus(CSteamID, CSteamID) { return true; }
};
struct IGamePlayer { bool connected = true; bool IsConnected() { return connected; } };
struct PlayerHelpers
{
	IGamePlayer player;
	bool exists = true;
	IGamePlayer* GetGamePlayer(int) { return exists ? &player : nullptr; }
} playerHelpers;
auto* playerhelpers = &playerHelpers;
struct GameHelpers { int ReferenceToIndex(cell_t index) { return index; } } gameHelpers;
auto* gamehelpers = &gameHelpers;

struct GameServer
{
	ISteamGameServer* server = nullptr;
	ISteamGameServer* GetGameServer() { return server; }
	void* GetSteamClient() { return this; }
};
struct SteamWorks
{
	GameServer* pSWGameServer = nullptr;
	CSteamID identity = CSteamID(123, k_EUniversePublic, k_EAccountTypeIndividual);
	int identityCalls = 0;
	EUniverse universeOption = k_EUniverseInvalid;
	EAccountType typeOption = k_EAccountTypeInvalid;
	CSteamID CreateCommonCSteamID(IGamePlayer*, EUniverse universe, EAccountType type)
	{
		++identityCalls;
		universeOption = universe;
		typeOption = type;
		return identity;
	}
	CSteamID CreateCommonCSteamID(steamworks::AccountId account, EUniverse universe, EAccountType type)
	{
		return CSteamID(account.value, universe, type);
	}
} g_SteamWorks;

using Native = cell_t (*)(IPluginContext*, const cell_t*);
struct sp_nativeinfo_t { const char* name; Native function; };
struct ShareSystem
{
	sp_nativeinfo_t* registered = nullptr;
	void AddNatives(void*, sp_nativeinfo_t* natives) { registered = natives; }
} shareSystem;
auto* sharesys = &shareSystem;
void* myself = nullptr;
struct SourceModHost
{
	size_t (*formatter)(char*, size_t, const char*, va_list) = nullptr;
	int formatCalls = 0;
	size_t Format(char* buffer, size_t capacity, const char* format, ...)
	{
		++formatCalls;
		va_list arguments;
		va_start(arguments, format);
		size_t count = formatter ? formatter(buffer, capacity, format, arguments) : 0;
		va_end(arguments);
		return count;
	}
} sourceModHost;
auto* g_pSM = &sourceModHost;

#include "../extension/natives/gsnatives.cpp"
