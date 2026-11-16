#pragma once
#include "steam_gameserver.h"
struct NativeServer014Stub : ISteamGameServer {
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
 HAuthTicket GetAuthSessionTicket(void *, int, uint32 *) override { return {}; }
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
