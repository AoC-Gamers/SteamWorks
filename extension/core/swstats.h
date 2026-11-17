#pragma once

#include "smsdk_ext.h"
#include "steam_gameserver.h"
#include "isteamgameserverstats.h"
#include <map>
#include <memory>

// Owns async stats calls independently of plugin handles and client-slot reuse.
class SteamWorksStats
{
public:
    SteamWorksStats();
    ~SteamWorksStats();
    bool IsReady() const;
    void Start();
    void Stop();
    bool Request(ISteamGameServerStats* provider, CSteamID identity, AppId_t appid);
    SteamWorksStats(const SteamWorksStats&) = delete;
    SteamWorksStats& operator=(const SteamWorksStats&) = delete;

private:
    struct Pending;
    void Complete(Pending& request, GSStatsReceived_t* payload, bool failed);
    void OnUnloaded(GSStatsUnloaded_t* payload);
    std::map<SteamAPICall_t, std::unique_ptr<Pending>> m_pending;
    CCallbackManual<SteamWorksStats, GSStatsUnloaded_t, true> m_unloadedCallback;
    IForward* m_received = nullptr;
    IForward* m_unloaded = nullptr;
    bool m_running = false;
    uint64 m_generation = 0;
};
