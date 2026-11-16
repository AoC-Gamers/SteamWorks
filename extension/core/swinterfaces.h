#pragma once

#include "steam_gameserver.h"
#include "isteamgamecoordinator.h"
#include "swgameauth014.h"
#include <cstddef>
#include <cstring>

namespace steamworks {

enum class InterfaceId : size_t
{
	Client, GameServer, Utils, Networking, Stats, HTTP, Matchmaking, Coordinator, GameServerAuth, Count
};

struct InterfaceRecord
{
	InterfaceId id;
	const char* family;
	const char* configKey;
	const char* version;
	const char* declaration;
};

constexpr bool SameVersion(const char* left, const char* right)
{
	return *left == *right && (*left == '\0' || SameVersion(left + 1, right + 1));
}

// Explicit allowlist tied to the exact declarations audited in SDK 1.58a.
// A changed SDK macro must trigger an audit, not silently approve a new ABI.
template <typename Interface> struct InterfaceContract;
#define SW_INTERFACE_CONTRACT(type, id, key, literal, macro, header) \
	static_assert(SameVersion(macro, literal), "Steam interface declaration changed: " #type); \
	template <> struct InterfaceContract<type> { \
		static constexpr InterfaceRecord Record() { return {InterfaceId::id, #type, key, literal, header}; } \
	};

SW_INTERFACE_CONTRACT(ISteamClient, Client, nullptr, "SteamClient020", STEAMCLIENT_INTERFACE_VERSION, "isteamclient.h")
SW_INTERFACE_CONTRACT(ISteamGameServer, GameServer, "SteamGameServerInterfaceVersion", "SteamGameServer015", STEAMGAMESERVER_INTERFACE_VERSION, "isteamgameserver.h")
SW_INTERFACE_CONTRACT(ISteamUtils, Utils, "SteamUtilsInterfaceVersion", "SteamUtils010", STEAMUTILS_INTERFACE_VERSION, "isteamutils.h")
SW_INTERFACE_CONTRACT(ISteamNetworking, Networking, "SteamNetworkingInterfaceVersion", "SteamNetworking006", STEAMNETWORKING_INTERFACE_VERSION, "isteamnetworking.h")
SW_INTERFACE_CONTRACT(ISteamGameServerStats, Stats, "SteamGameServerStatsInterfaceVersion", "SteamGameServerStats001", STEAMGAMESERVERSTATS_INTERFACE_VERSION, "isteamgameserverstats.h")
SW_INTERFACE_CONTRACT(ISteamHTTP, HTTP, "SteamHTTPInterfaceVersion", "STEAMHTTP_INTERFACE_VERSION003", STEAMHTTP_INTERFACE_VERSION, "isteamhttp.h")
SW_INTERFACE_CONTRACT(ISteamMatchmaking, Matchmaking, "SteamMatchmakingVersion", "SteamMatchMaking009", STEAMMATCHMAKING_INTERFACE_VERSION, "isteammatchmaking.h")
SW_INTERFACE_CONTRACT(ISteamGameCoordinator, Coordinator, "SteamGameCoordinatorVersion", "SteamGameCoordinator001", STEAMGAMECOORDINATOR_INTERFACE_VERSION, "isteamgamecoordinator.h")
#undef SW_INTERFACE_CONTRACT

template <> struct InterfaceContract<ISteamGameServer014> {
 static constexpr InterfaceRecord Record() {
  return {InterfaceId::GameServerAuth, "ISteamGameServer014", "SteamGameServerAuthInterfaceVersion",
          GameServerAuth014Version, "core/swgameauth014.h (SDK 1.55, slots 0..26)"};
 }
};

template <typename Interface>
inline bool AcceptsVersion(const char* version)
{
	return version != nullptr && std::strcmp(version, InterfaceContract<Interface>::Record().version) == 0;
}

} // namespace steamworks
