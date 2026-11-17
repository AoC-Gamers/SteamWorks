#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#ifndef STEAM_CALLBACKS_TEST_HOST
#define STEAM_CALLBACKS_TEST_HOST "steam_callbacks_test_host.h"
#endif
#include STEAM_CALLBACKS_TEST_HOST
#include "isteamuserstats.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
using cell_t = int32_t;
constexpr int ET_Ignore = 0, Param_Cell = 0;
struct IForward {
    int arity = 0, consumers = 1;
    std::vector<cell_t> cells;
    std::vector<std::vector<cell_t>> events;
    std::function<void()> action;
    int GetFunctionCount() { return consumers; }
    void PushCell(cell_t value) { cells.push_back(value); }
    void Execute(cell_t*) { events.push_back(cells); cells.clear(); auto run = action; if (run) run(); }
};
struct ForwardSystem {
    std::map<std::string, IForward*> owned;
    std::string failName;
    template<class... T> IForward* CreateForward(const char* name, int, int arity, void*, T...) {
        if (failName == name) return nullptr;
        auto* f = new IForward; f->arity = arity; owned[name] = f; return f;
    }
    void ReleaseForward(IForward* f) {
        for (auto it = owned.begin(); it != owned.end(); ++it) if (it->second == f) { owned.erase(it); break; }
        delete f;
    }
} forwardSystem;
auto* forwards = &forwardSystem;
#include "../../extension/core/swstats.cpp"

struct StatsProvider final : ISteamGameServerStats {
    SteamAPICall_t next = 1;
    int requests = 0;
    SteamAPICall_t RequestUserStats(CSteamID) override { ++requests; return next; }
    bool GetUserStat(CSteamID,const char*,int32*) override { return false; }
    bool GetUserStat(CSteamID,const char*,float*) override { return false; }
    bool GetUserAchievement(CSteamID,const char*,bool*) override { return false; }
    bool SetUserStat(CSteamID,const char*,int32) override { return false; }
    bool SetUserStat(CSteamID,const char*,float) override { return false; }
    bool UpdateUserAvgRateStat(CSteamID,const char*,float,double) override { return false; }
    bool SetUserAchievement(CSteamID,const char*) override { return false; }
    bool ClearUserAchievement(CSteamID,const char*) override { return false; }
    SteamAPICall_t StoreUserStats(CSteamID) override { return k_uAPICallInvalid; }
};
// The ID is shared; the registration context must select the recipient.
static_assert(GSStatsUnloaded_t::k_iCallback == 1108, "SDK 1.55 server unload callback ID");
static_assert(UserStatsUnloaded_t::k_iCallback == 1108, "SDK 1.55 client unload callback ID");
static_assert(sizeof(GSStatsUnloaded_t) == 8 && sizeof(UserStatsUnloaded_t) == 8,
    "Unload payload contains only the 64-bit Steam identity");
struct ClientCacheReceiver {
    int calls = 0;
    CSteamID last;
    CCallbackManual<ClientCacheReceiver, UserStatsUnloaded_t, false> callback;
    ClientCacheReceiver() { callback.Register(this, &ClientCacheReceiver::OnUnloaded); }
    void OnUnloaded(UserStatsUnloaded_t* payload) { ++calls; last = payload->m_steamIDUser; }
};
static int checks = 0;
static void Check(bool value, const char* label) {
    ++checks; if (!value) { std::fprintf(stderr,"FAIL: %s\n",label); std::exit(1); }
}
static CSteamID User(uint32 id = 123) { return CSteamID(id,k_EUniversePublic,k_EAccountTypeIndividual); }
static GSStatsReceived_t Result(CSteamID id, EResult result = k_EResultOK) {
    GSStatsReceived_t out = {}; out.m_steamIDUser = id; out.m_eResult = result; return out;
}
int main() {
    {
        SteamWorksStats owner; StatsProvider provider;
        auto* received = forwards->owned.at("SteamWorks_OnUserStatsReceived");
        auto* unloaded = forwards->owned.at("SteamWorks_OnUserStatsUnloaded");
        Check(owner.IsReady() && received->arity == 3 && unloaded->arity == 1,"public forward arities match declarations");
        Check(steamCallbacks.callbacks.size() == 1 && steamCallbacks.calls.empty(),"only unloaded is a broadcast subscription");
        Check(CCallbackMgr::GameServer(steamCallbacks.callbacks.begin()->first),"unloaded uses GameServer context");
        const int registrations = steamCallbacks.registrations;
        owner.Start(); Check(steamCallbacks.registrations == registrations,"start is idempotent");
        Check(!owner.Request(nullptr,User(),550) && !owner.Request(&provider,User(),0)
            && !owner.Request(&provider,CSteamID(),550) && provider.requests == 0,"invalid request rejected before provider");
        provider.next = k_uAPICallInvalid;
        Check(!owner.Request(&provider,User(),550) && steamCallbacks.calls.empty(),"SDK rejection creates no subscription");
        provider.next = 1;
        Check(owner.Request(&provider,User(),550) && steamCallbacks.calls.size() == 1,"accepted request owns a CallResult");
        Check(CCallbackMgr::GameServer(steamCallbacks.calls.begin()->first),"CallResult uses GameServer context");
        int count = provider.requests;
        Check(owner.Request(&provider,User(),550) && provider.requests == count && steamCallbacks.calls.size() == 1,"pending duplicate coalesced");
        auto result = Result(User());
        DeliverCallback(result);
        Check(received->events.empty() && steamCallbacks.calls.size() == 1,"broadcast cannot complete a CallResult");
        DeliverCall(result,1);
        Check(received->events == std::vector<std::vector<cell_t>>{{123,k_EResultOK,550}} && steamCallbacks.calls.empty(),"success includes captured AccountID result AppID");
        DeliverCall(result,1); Check(received->events.size() == 1,"duplicate result ignored");
        provider.next = 2; owner.Request(&provider,User(124),440);
        provider.next = 3; owner.Request(&provider,User(125),550);
        result = Result(User(125),k_EResultFail); DeliverCall(result,3);
        result = Result(User(124)); DeliverCall(result,2);
        Check(received->events[1] == std::vector<cell_t>{125,k_EResultFail,550}
            && received->events[2] == std::vector<cell_t>{124,k_EResultOK,440},"out-of-order requests retain identity and AppID");
        provider.next = 4; owner.Request(&provider,User(),550);
        result = Result(User(999)); DeliverCall(result,4);
        Check(received->events.back() == std::vector<cell_t>{123,k_EResultFail,550},"mismatched payload never attributes another account");
        provider.next = 5; owner.Request(&provider,User(),550);
        result = Result(User()); DeliverCall(result,5,true);
        Check(received->events.back()[1] == k_EResultIOFailure,"I/O failure mapped explicitly");
        provider.next = 6; owner.Request(&provider,User(),550);
        auto* call = steamCallbacks.calls.begin()->first; steamCallbacks.calls.erase(call);
        call->Run(nullptr,false,6);
        Check(received->events.back()[1] == k_EResultFail,"null result is failure not success");
        provider.next = 7; owner.Request(&provider,User(0x80000001u),0x80000002u);
        result = Result(User(0x80000001u)); DeliverCall(result,7);
        Check(static_cast<uint32>(received->events.back()[0]) == 0x80000001u
            && static_cast<uint32>(received->events.back()[2]) == 0x80000002u,"AccountID/AppID retain all 32 bits");
        {
            auto* serverDispatch = steamCallbacks.callbacks.begin()->first;
            ClientCacheReceiver client;
            bool clientRegistered = false;
            for (const auto& entry : steamCallbacks.callbacks) {
                if (entry.second == 1108 && !CCallbackMgr::GameServer(entry.first)) clientRegistered = true;
            }
            Check(clientRegistered && steamCallbacks.callbacks.size() == 2,
                "client and GameServer receivers coexist for the same callback ID");
            UserStatsUnloaded_t clientGone = {}; clientGone.m_steamIDUser = User(999);
            DeliverCallback(clientGone, SteamCallbackContext::Client);
            Check(client.calls == 1 && client.last == User(999) && unloaded->events.empty(),
                "client unload reaches only client subscribers");
            GSStatsUnloaded_t gone = {}; gone.m_steamIDUser = User();
            DeliverCallback(gone, SteamCallbackContext::GameServer);
            Check(unloaded->events == std::vector<std::vector<cell_t>>{{123}} && client.calls == 1,
                "GameServer unload forwards AccountID without notifying client subscribers");
            gone.m_steamIDUser = CSteamID(); DeliverCallback(gone);
            Check(unloaded->events.size() == 1,"invalid unload identity ignored");
            gone.m_steamIDUser = CSteamID(123, 0, k_EUniversePublic, k_EAccountTypeGameServer);
            DeliverCallback(gone);
            Check(unloaded->events.size() == 1,"non-individual unload identity ignored");
            serverDispatch->Run(nullptr);
            Check(unloaded->events.size() == 1,"null unload payload ignored");
            gone.m_steamIDUser = User(0x80000001u); DeliverCallback(gone);
            Check(unloaded->events.size() == 2 && static_cast<uint32>(unloaded->events.back()[0]) == 0x80000001u,
                "unload AccountID retains the high bit");
            unloaded->consumers = 0; DeliverCallback(gone);
            Check(unloaded->events.size() == 2 && client.calls == 1,"zero unload consumers emits nothing in either context");
            unloaded->consumers = 1;
            owner.Stop(); DeliverCallback(gone); DeliverCallback(clientGone, SteamCallbackContext::Client);
            Check(unloaded->events.size() == 2 && client.calls == 2 && steamCallbacks.callbacks.size() == 1,
                "stopping server owner cancels its callback without cancelling client subscription");
            owner.Start(); owner.Start(); DeliverCallback(clientGone, SteamCallbackContext::Client);
            Check(unloaded->events.size() == 2 && client.calls == 3 && steamCallbacks.callbacks.size() == 2,
                "restart registers exactly one GameServer receiver and preserves context isolation");
            DeliverCallback(gone);
            Check(unloaded->events.size() == 3 && client.calls == 3,"server delivery resumes once after restart");
            unloaded->action = [&] { owner.Stop(); owner.Start(); };
            DeliverCallback(gone);
            Check(unloaded->events.size() == 4 && steamCallbacks.callbacks.size() == 2,
                "unload forward permits reentrant stop and restart without duplicate subscription");
            unloaded->action = {};
        }
        Check(steamCallbacks.callbacks.size() == 1,"client receiver destruction leaves server receiver intact");
        received->consumers = 0; provider.next = 8; owner.Request(&provider,User(),550);
        result = Result(User()); const auto events = received->events.size(); DeliverCall(result,8);
        Check(received->events.size() == events && steamCallbacks.calls.empty(),"zero receivers still release request");
        received->consumers = 1;
        provider.next = 9; owner.Request(&provider,User(),550);
        provider.next = 10; owner.Request(&provider,User(124),550);
        owner.Stop(); Check(steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty(),"stop cancels pending calls and broadcast");
        owner.Stop(); Check(!owner.Request(&provider,User(),550),"stopped owner rejects requests");
        result = Result(User()); DeliverCall(result,9); Check(received->events.size() == events,"late result suppressed after stop");
        owner.Start(); Check(steamCallbacks.callbacks.size() == 1,"restart restores one cache subscription");
        provider.next = 11; owner.Request(&provider,User(),550);
        provider.next = 12; owner.Request(&provider,User(124),550);
        received->action = [&] { owner.Stop(); owner.Start(); provider.next = 13; Check(owner.Request(&provider,User(),550),"request allowed from new generation"); };
        result = Result(User()); DeliverCall(result,11);
        Check(steamCallbacks.calls.size() == 1 && steamCallbacks.calls.begin()->second == 13,"reentrant stop cancels old generation not new request");
        received->action = {}; DeliverCall(result,12);
        auto after = received->events.size(); DeliverCall(result,13);
        Check(received->events.size() == after + 1,"new generation completes exactly once");
        provider.next = 14; owner.Request(&provider,User(),550);
        provider.next = 14;
        Check(!owner.Request(&provider,User(124),550) && steamCallbacks.calls.size() == 1,"reused backend handle cannot replace another account");
    }
    Check(steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty() && forwards->owned.empty(),"destruction releases every subscription and forward");
    {
        auto* owner = new SteamWorksStats; StatsProvider provider;
        auto* forward = forwards->owned.at("SteamWorks_OnUserStatsReceived");
        forward->action = [&] { delete owner; owner = nullptr; };
        owner->Request(&provider,User(),550); auto result = Result(User()); DeliverCall(result,1);
        Check(owner == nullptr && forwards->owned.empty() && steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty(),"owner can be torn down during result dispatch");
    }
    {
        auto* owner = new SteamWorksStats; StatsProvider provider;
        auto* forward = forwards->owned.at("SteamWorks_OnUserStatsUnloaded");
        forward->action = [&] { delete owner; owner = nullptr; };
        owner->Request(&provider,User(),550);
        GSStatsUnloaded_t gone = {}; gone.m_steamIDUser = User(); DeliverCallback(gone);
        Check(owner == nullptr && forwards->owned.empty() && steamCallbacks.calls.empty() && steamCallbacks.callbacks.empty(),
            "owner teardown during unload dispatch cancels pending results and every subscription");
    }
    for (const char* name : {"SteamWorks_OnUserStatsReceived","SteamWorks_OnUserStatsUnloaded"}) {
        forwards->failName = name;
        { SteamWorksStats owner; StatsProvider provider;
          Check(!owner.IsReady() && !owner.Request(&provider,User(),550) && provider.requests == 0,"partial forward failure refuses async work"); }
        Check(forwards->owned.empty() && steamCallbacks.callbacks.empty(),"partial creation cleans up safely");
    }
    forwards->failName.clear();
    std::printf("PASS: %d stats callback checks (production owner, real SDK CallResult/callback classes)\n", checks);
}
