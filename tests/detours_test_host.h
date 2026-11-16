#pragma once

// Production wrapper/owner code is included below. Only the SourceMod host,
// detour factory, patch operations and legacy-call trampoline are simulated.
// The macro shapes preserve the callback arguments, but do not validate ABI or
// relocate machine instructions. Those require an identified game runtime.
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_DETOURS_H_
#define STEAM_GAMESERVER_H

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using uint32 = uint32_t;
using uint16 = uint16_t;
enum EServerMode { testServerMode = 3 };
struct ISourcePawnEngine {};
struct ISteamGameServer {};

struct DetourState
{
	bool library = true;
	bool shutdownExport = true;
	bool initExport = true;
	bool shutdownSignature = false;
	bool initSignature = false;
	bool config = true;
	bool failShutdownCreation = false;
	bool failInitCreation = false;
 bool failInitEnable = false;
	bool engine = true;
	bool initResult = true;
	bool validAddress = true;
	int libraryOpens = 0;
	int libraryCloses = 0;
	int exportReads = 0;
	int managerInits = 0;
	int creates = 0;
	int enables = 0;
	int destroys = 0;
	int live = 0;
	int interfaceReads = 0;
	int addHooks = 0;
	int removeHooks = 0;
	int resets = 0;
	int originalInitCalls = 0;
	int originalShutdownCalls = 0;
	std::vector<std::string> events;
	std::vector<std::string> errors;
} detourState;

static void Require(bool condition, const char* message)
{
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

alignas(4) unsigned char shutdownExportSymbol[16] = {};
alignas(4) unsigned char initExportSymbol[16] = {};
void* initProviderOverride = nullptr;
void* initCallOverride = nullptr;
alignas(4) unsigned char shutdownSignatureSymbol[16] = {};
alignas(4) unsigned char initSignatureSymbol[16] = {};

struct IGameConfig
{
	bool GetMemSig(const char* name, void** address)
	{
		const bool shutdown = std::strcmp(name, "SteamGameServer_Shutdown") == 0;
		Require(shutdown || std::strcmp(name, "SteamInternal_GameServer_Init") == 0, "expected gamedata symbol key");
		const bool available = shutdown ? detourState.shutdownSignature : detourState.initSignature;
		if (available) { *address = shutdown ? &shutdownSignatureSymbol : &initSignatureSymbol; }
		return available;
	}
} testConfig;
struct GameData
{
	IGameConfig* GetGameData() { return detourState.config ? &testConfig : nullptr; }
} gameData;

struct ILibrary
{
	void* GetSymbolAddress(const char* name)
	{
		++detourState.exportReads;
		if (std::strcmp(name, "SteamGameServer_Shutdown") == 0) {
			return detourState.shutdownExport ? &shutdownExportSymbol : nullptr;
		}
		Require(std::strcmp(name, "SteamInternal_GameServer_Init") == 0, "expected legacy init symbol");
		return detourState.initExport ? (initProviderOverride ? initProviderOverride : &initExportSymbol) : nullptr;
	}
	void CloseLibrary() { ++detourState.libraryCloses; }
} library;
struct LibrarySystem
{
	ILibrary* OpenLibrary(const char*, char*, size_t)
	{
		++detourState.libraryOpens;
		return detourState.library ? &library : nullptr;
	}
} librarySystem;
auto* libsys = &librarySystem;

struct SourceModHost
{
	ISourcePawnEngine engine;
	ISourcePawnEngine* GetScriptingEngine() { return detourState.engine ? &engine : nullptr; }
	void LogMessage(void*, const char*, ...) {}
	void LogError(void*, const char* format, ...)
	{
		char message[512];
		va_list arguments;
		va_start(arguments, format);
		std::vsnprintf(message, sizeof(message), format, arguments);
		va_end(arguments);
		detourState.errors.emplace_back(message);
	}
} sourceModHost;
auto* g_pSM = &sourceModHost;
void* myself = nullptr;

struct GameServer
{
	ISteamGameServer instance;
	ISteamGameServer* server = &instance;
	const char* GetLibraryPath() { return "test/steam-api"; }
	ISteamGameServer* GetGameServer()
	{
		++detourState.interfaceReads;
		detourState.events.emplace_back("get-interface");
		return server;
	}
	void Reset()
	{
		++detourState.resets;
		detourState.events.emplace_back("reset");
		server = nullptr;
	}
} gameServer;
struct GameServerHooks
{
	void AddHooks(ISteamGameServer* server)
	{
		Require(server != nullptr, "no AddHooks with null interface");
		++detourState.addHooks;
		detourState.events.emplace_back("add-hooks");
	}
	void RemoveHooks(ISteamGameServer* server, bool destroyed)
	{
		Require(server == gameServer.server && !destroyed, "shutdown hook arguments preserved");
		++detourState.removeHooks;
		detourState.events.emplace_back("remove-hooks");
	}
} gameServerHooks;
struct ClientLifecycle {void GameServerStopped(){detourState.events.emplace_back("host-stopped");}} clientLifecycle;
struct SteamWorks
{
 ClientLifecycle* pClientLifecycle=&clientLifecycle;
	GameServer* pSWGameServer = &gameServer;
	GameData* pSWGameData = &gameData;
	GameServerHooks* pGSHooks = &gameServerHooks;
    void RuntimeStarted() {
        if (pSWGameServer != nullptr && pGSHooks != nullptr) {
            auto* server=pSWGameServer->GetGameServer(); if (server != nullptr) pGSHooks->AddHooks(server);
        }
    }
    void RuntimeStopped() {
        if (pGSHooks != nullptr) { ++detourState.removeHooks; detourState.events.emplace_back("remove-hooks"); }
        if (pSWGameServer != nullptr) { pSWGameServer->Reset(); }
    }
} g_SteamWorks;

static bool OriginalInit(uint32 ip, uint16 steamPort, uint16 gamePort, uint16 queryPort,
	EServerMode mode, const char* version)
{
	Require(ip == 0x12345678u && steamPort == 123 && gamePort == 456 && queryPort == 789
		&& mode == testServerMode && std::strcmp(version, "fixture-version") == 0,
		"all six original arguments preserved");
	++detourState.originalInitCalls;
	detourState.events.emplace_back("original-init");
	return detourState.initResult;
}
static void OriginalShutdown()
{
	++detourState.originalShutdownCalls;
	detourState.events.emplace_back("original-shutdown");
}

struct CDetour
{
	std::string name;
	bool patched = false;
	void EnableDetour()
	{
		#ifndef TEST_REAL_INIT_CALLSITE
  Require(detourState.creates >= 2 && detourState.live == 2, "complete pair created before any enable");
#endif
		Require(!patched, "no duplicate enable");
		patched = true;
		++detourState.enables;
		detourState.events.push_back("enable-" + name);
	}
	void Destroy()
	{
		++detourState.destroys;
		--detourState.live;
		detourState.events.push_back("destroy-" + name);
		delete this; // Like CDetour, does not clear the caller's trampoline slot.
	}
};
struct CDetourManager
{
	static void Init(ISourcePawnEngine* engine, IGameConfig*)
	{
		Require(engine != nullptr, "valid SourcePawn engine required");
		++detourState.managerInits;
	}
	static CDetour* CreateDetour(void*, void** trampoline, void* address)
	{
		++detourState.creates;
		const bool shutdown = address == &shutdownExportSymbol || address == &shutdownSignatureSymbol;
		Require(shutdown || address == &initExportSymbol || address == &initSignatureSymbol, "expected resolved address");
		const std::string name = shutdown ? "shutdown" : "init";
		detourState.events.push_back("create-" + name);
		if ((shutdown && detourState.failShutdownCreation) || (!shutdown && detourState.failInitCreation)) { return nullptr; }
		*trampoline = shutdown ? reinterpret_cast<void*>(&OriginalShutdown) : reinterpret_cast<void*>(&OriginalInit);
		++detourState.live;
		return new CDetour{name, false};
	}
};

#define DETOUR_DECL_STATIC0(name, ret) ret (*name##_Actual)() = nullptr; ret name()
#define DETOUR_DECL_STATIC6(name, ret, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6) \
	ret (*name##_Actual)(t1, t2, t3, t4, t5, t6) = nullptr; ret name(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6)
#define GET_STATIC_CALLBACK(name) reinterpret_cast<void*>(&name)
#define DETOUR_STATIC_CALL(name) name##_Actual
#define DETOUR_CREATE_STATIC_FIXED(name, address) \
	CDetourManager::CreateDetour(reinterpret_cast<void*>(&name), reinterpret_cast<void**>(&name##_Actual), address)

#ifndef TEST_REAL_INIT_CALLSITE
#define STEAMWORKS_INIT_CALLSITE_H_
namespace steamworks {
class InitCallSite {
 CDetour* patch=nullptr;
public:
 static uintptr_t DecodeTarget(const unsigned char*){Require(false,"unidentified host must reject before decoding any engine call");return 0;}
 bool Prepare(void*,void*,void* callback){void* unused=nullptr;patch=CDetourManager::CreateDetour(callback,&unused,&initExportSymbol);return patch!=nullptr;}
 bool Enable(){if(detourState.failInitEnable)return false;patch->EnableDetour();return true;}
 bool Restore(){return true;}
 void Reset(){if(patch){patch->Destroy();patch=nullptr;}}
 bool IsInstalled()const{return patch && patch->patched;}
};
}
#endif
#ifndef TEST_REAL_LIFECYCLE_PROFILE_POLICY
#define STEAMWORKS_LIFECYCLE_PROFILE_H_
namespace steamworks {
inline bool HasVerifiedLifecycleProfile() { return true; }
constexpr const char* LifecycleProfileId="synthetic-test-profile";
constexpr const char* LifecycleInitSymbol="SteamInternal_GameServer_Init";
constexpr const char* LifecycleShutdownSymbol="SteamGameServer_Shutdown";
enum class LifecycleEntry { Init, Shutdown };
inline bool IsVerifiedLifecycleAddress(void*,LifecycleEntry) { return detourState.validAddress; }
inline void* VerifiedLifecycleInitCall(void*) { return detourState.validAddress ? (initCallOverride ? initCallOverride : &initExportSymbol) : nullptr; }
}
#endif
#include "../extension/hooks/swgsdetours.cpp"

static void ResetFixture()
{
	Require(detourState.live == 0, "previous detours released");
	detourState = DetourState{};
	gameServer.server = &gameServer.instance;
	g_SteamWorks.pSWGameServer = &gameServer;
	g_SteamWorks.pSWGameData = &gameData;
	g_SteamWorks.pGSHooks = &gameServerHooks;
 g_SteamWorks.pClientLifecycle=&clientLifecycle;
	SteamAPIShutdown_Actual = nullptr;
	SteamGameServer_InitLegacyDetour_Actual = nullptr;
}

static bool InvokeInit()
{
	return SteamGameServer_InitLegacyDetour(0x12345678u, 123, 456, 789, testServerMode, "fixture-version");
}
