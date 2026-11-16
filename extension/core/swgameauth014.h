// Version-specific, non-owning virtual prefix audited against Valve SDK 1.55.
// Reference: AoC-Gamers/Steamworks-SDK a7aaa2754d9cda244d0cd9d7523c1144a1c472d2,
// sdk_155/public/steam/isteamgameserver.h (SteamGameServer014).
// Declare slots 0..26 exactly; no virtual destructor or inheritance from 015.
#pragma once
#include "steam_gameserver.h"
#include <cstring>
#include <type_traits>

namespace steamworks {
constexpr const char* GameServerAuth014Version = "SteamGameServer014";
class ISteamGameServer014 {
public:
 virtual bool InitGameServer( uint32 unIP, uint16 usGamePort, uint16 usQueryPort, uint32 unFlags, AppId_t nGameAppId, const char *pchVersionString ) = 0;
 virtual void SetProduct( const char *pszProduct ) = 0;
 virtual void SetGameDescription( const char *pszGameDescription ) = 0;
 virtual void SetModDir( const char *pszModDir ) = 0;
 virtual void SetDedicatedServer( bool bDedicated ) = 0;
 virtual void LogOn( const char *pszToken ) = 0;
 virtual void LogOnAnonymous() = 0;
 virtual void LogOff() = 0;
 virtual bool BLoggedOn() = 0;
 virtual bool BSecure() = 0;
 virtual CSteamID GetSteamID() = 0;
 virtual bool WasRestartRequested() = 0;
 virtual void SetMaxPlayerCount( int cPlayersMax ) = 0;
 virtual void SetBotPlayerCount( int cBotplayers ) = 0;
 virtual void SetServerName( const char *pszServerName ) = 0;
 virtual void SetMapName( const char *pszMapName ) = 0;
 virtual void SetPasswordProtected( bool bPasswordProtected ) = 0;
 virtual void SetSpectatorPort( uint16 unSpectatorPort ) = 0;
 virtual void SetSpectatorServerName( const char *pszSpectatorServerName ) = 0;
 virtual void ClearAllKeyValues() = 0;
 virtual void SetKeyValue( const char *pKey, const char *pValue ) = 0;
 virtual void SetGameTags( const char *pchGameTags ) = 0;
 virtual void SetGameData( const char *pchGameData ) = 0;
 virtual void SetRegion( const char *pszRegion ) = 0;
 virtual void SetAdvertiseServerActive( bool bActive ) = 0;
 virtual HAuthTicket GetAuthSessionTicket( void *pTicket, int cbMaxTicket, uint32 *pcbTicket ) = 0;
 virtual EBeginAuthSessionResult BeginAuthSession( const void *pAuthTicket, int cbAuthTicket, CSteamID steamID ) = 0;
protected:
 ~ISteamGameServer014() = default;
};
static_assert(sizeof(CSteamID) == 8, "Auth ABI requires a 64-bit SteamID");
static_assert(sizeof(HSteamUser) == 4 && sizeof(int) == 4 && sizeof(EBeginAuthSessionResult) == 4,
              "Auth ABI requires 32-bit user, length and result");
static_assert(!std::has_virtual_destructor<ISteamGameServer014>::value,
              "A virtual destructor would shift the audited slots");
static_assert(!std::is_convertible<ISteamGameServer014*, ISteamGameServer*>::value,
              "014 must never be used as the SDK 015 declaration");
static_assert(!std::is_convertible<ISteamGameServer*, ISteamGameServer014*>::value,
              "015 must never be used as the engine 014 declaration");
using GameServerAuthFactory = void* (S_CALLTYPE *)(HSteamUser, const char*);
inline ISteamGameServer014* ResolveGameServerAuth014(GameServerAuthFactory factory, HSteamUser user, const char* version) {
 if (!factory || user == 0 || !version || std::strcmp(version, GameServerAuth014Version) != 0) return nullptr;
 // Only a factory result for the exact audited version crosses this boundary.
 return static_cast<ISteamGameServer014*>(factory(user, GameServerAuth014Version));
}
}
