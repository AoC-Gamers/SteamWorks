#pragma once
#include "../../extension/core/swgameauth014.h"
struct AuthServer014Stub : steamworks::ISteamGameServer014 {
 int authCalls = 0;
 EBeginAuthSessionResult originalResult = k_EBeginAuthSessionResultInvalidTicket;
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
 EBeginAuthSessionResult BeginAuthSession(const void *, int, CSteamID) override { ++authCalls; return originalResult; }
};
