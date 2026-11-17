#pragma once
#include "steam_gameserver.h"
// Independent rejected ABI fixture. Not a supported SDK or product interface.
// The extra ticket argument pins the historical 015 declaration explicitly.
namespace steamworks_test {
struct UnsupportedServer015 {
 virtual bool InitGameServer(uint32, uint16, uint16, uint32, AppId_t, const char *) = 0;
 virtual void SetProduct(const char *) = 0;
 virtual void SetGameDescription(const char *) = 0;
 virtual void SetModDir(const char *) = 0;
 virtual void SetDedicatedServer(bool) = 0;
 virtual void LogOn(const char *) = 0;
 virtual void LogOnAnonymous() = 0;
 virtual void LogOff() = 0;
 virtual bool BLoggedOn() = 0;
 virtual bool BSecure() = 0;
 virtual CSteamID GetSteamID() = 0;
 virtual bool WasRestartRequested() = 0;
 virtual void SetMaxPlayerCount(int) = 0;
 virtual void SetBotPlayerCount(int) = 0;
 virtual void SetServerName(const char *) = 0;
 virtual void SetMapName(const char *) = 0;
 virtual void SetPasswordProtected(bool) = 0;
 virtual void SetSpectatorPort(uint16) = 0;
 virtual void SetSpectatorServerName(const char *) = 0;
 virtual void ClearAllKeyValues() = 0;
 virtual void SetKeyValue(const char *, const char *) = 0;
 virtual void SetGameTags(const char *) = 0;
 virtual void SetGameData(const char *) = 0;
 virtual void SetRegion(const char *) = 0;
 virtual void SetAdvertiseServerActive(bool) = 0;
 virtual HAuthTicket GetAuthSessionTicket(void *, int, uint32 *, const SteamNetworkingIdentity *) = 0;
 virtual EBeginAuthSessionResult BeginAuthSession(const void *, int, CSteamID) = 0;
 virtual void EndAuthSession(CSteamID) = 0;
 virtual void CancelAuthTicket(HAuthTicket) = 0;
 virtual EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID, AppId_t) = 0;
 virtual bool RequestUserGroupStatus(CSteamID, CSteamID) = 0;
 virtual void GetGameplayStats() = 0;
 virtual SteamAPICall_t GetServerReputation() = 0;
 virtual SteamIPAddress_t GetPublicIP() = 0;
 virtual bool HandleIncomingPacket(const void *, int, uint32, uint16) = 0;
 virtual int GetNextOutgoingPacket(void *, int, uint32 *, uint16 *) = 0;
 virtual SteamAPICall_t AssociateWithClan(CSteamID) = 0;
 virtual SteamAPICall_t ComputeNewPlayerCompatibility(CSteamID) = 0;
 virtual bool SendUserConnectAndAuthenticate_DEPRECATED(uint32, const void *, uint32, CSteamID *) = 0;
 virtual CSteamID CreateUnauthenticatedUserConnection() = 0;
 virtual void SendUserDisconnect_DEPRECATED(CSteamID) = 0;
 virtual bool BUpdateUserData(CSteamID, const char *, uint32) = 0;
 virtual void SetMasterServerHeartbeatInterval_DEPRECATED(int) = 0;
 virtual void ForceMasterServerHeartbeat_DEPRECATED() = 0;
};
}
struct NativeServer015Stub : steamworks_test::UnsupportedServer015 {
 int authCalls=0;
 bool InitGameServer(uint32, uint16, uint16, uint32, AppId_t, const char *) override { return {}; }
 void SetProduct(const char *) override {}
 void SetGameDescription(const char *) override {}
 void SetModDir(const char *) override {}
 void SetDedicatedServer(bool) override {}
 void LogOn(const char *) override {}
 void LogOnAnonymous() override {}
 void LogOff() override {}
 bool BLoggedOn() override { return {}; }
 bool BSecure() override { return {}; }
 CSteamID GetSteamID() override { return {}; }
 bool WasRestartRequested() override { return {}; }
 void SetMaxPlayerCount(int) override {}
 void SetBotPlayerCount(int) override {}
 void SetServerName(const char *) override {}
 void SetMapName(const char *) override {}
 void SetPasswordProtected(bool) override {}
 void SetSpectatorPort(uint16) override {}
 void SetSpectatorServerName(const char *) override {}
 void ClearAllKeyValues() override {}
 void SetKeyValue(const char *, const char *) override {}
 void SetGameTags(const char *) override {}
 void SetGameData(const char *) override {}
 void SetRegion(const char *) override {}
 void SetAdvertiseServerActive(bool) override {}
 HAuthTicket GetAuthSessionTicket(void *, int, uint32 *, const SteamNetworkingIdentity *) override { return {}; }
 EBeginAuthSessionResult BeginAuthSession(const void *, int, CSteamID) override { ++authCalls; return k_EBeginAuthSessionResultInvalidTicket; }
 void EndAuthSession(CSteamID) override {}
 void CancelAuthTicket(HAuthTicket) override {}
 EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID, AppId_t) override { return {}; }
 bool RequestUserGroupStatus(CSteamID, CSteamID) override { return {}; }
 void GetGameplayStats() override {}
 SteamAPICall_t GetServerReputation() override { return {}; }
 SteamIPAddress_t GetPublicIP() override { return {}; }
 bool HandleIncomingPacket(const void *, int, uint32, uint16) override { return {}; }
 int GetNextOutgoingPacket(void *, int, uint32 *, uint16 *) override { return {}; }
 SteamAPICall_t AssociateWithClan(CSteamID) override { return {}; }
 SteamAPICall_t ComputeNewPlayerCompatibility(CSteamID) override { return {}; }
 bool SendUserConnectAndAuthenticate_DEPRECATED(uint32, const void *, uint32, CSteamID *) override { return {}; }
 CSteamID CreateUnauthenticatedUserConnection() override { return {}; }
 void SendUserDisconnect_DEPRECATED(CSteamID) override {}
 bool BUpdateUserData(CSteamID, const char *, uint32) override { return {}; }
 void SetMasterServerHeartbeatInterval_DEPRECATED(int) override {}
 void ForceMasterServerHeartbeat_DEPRECATED() override {}
};
