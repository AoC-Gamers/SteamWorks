#include "swstats.h"
#include <cstring>
#include <new>

namespace {
cell_t CellBits(uint32 bits)
{
    cell_t result;
    static_assert(sizeof(result) == sizeof(bits), "AccountID/AppID must fit a cell");
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
void ReleaseStatsForward(IForward*& forward)
{
    if (forward != nullptr) { forwards->ReleaseForward(forward); forward = nullptr; }
}
}

struct SteamWorksStats::Pending
{
    SteamWorksStats& owner;
    CSteamID identity;
    AppId_t appid;
    uint64 generation;
    SteamAPICall_t call = k_uAPICallInvalid;
    CCallResult<Pending, GSStatsReceived_t> result;
    Pending(SteamWorksStats& value, CSteamID user, AppId_t app, uint64 epoch)
      : owner(value), identity(user), appid(app), generation(epoch)
    {
        result.SetGameserverFlag();
    }
    void OnResult(GSStatsReceived_t* payload, bool failed)
    {
        // Complete takes ownership; do not access this after it returns. SDK 1.55
        // invalidates the call before invoking us and does not access it afterward.
        owner.Complete(*this, payload, failed);
    }
};

SteamWorksStats::SteamWorksStats()
{
    m_received = forwards->CreateForward("SteamWorks_OnUserStatsReceived", ET_Ignore, 3, nullptr, Param_Cell, Param_Cell, Param_Cell);
    m_unloaded = forwards->CreateForward("SteamWorks_OnUserStatsUnloaded", ET_Ignore, 1, nullptr, Param_Cell);
    Start();
}
SteamWorksStats::~SteamWorksStats()
{
    Stop();
    ReleaseStatsForward(m_received);
    ReleaseStatsForward(m_unloaded);
}
bool SteamWorksStats::IsReady() const { return m_received != nullptr && m_unloaded != nullptr; }
void SteamWorksStats::Start()
{
    if (m_running || !IsReady()) { return; }
    m_running = true;
    m_unloadedCallback.Register(this, &SteamWorksStats::OnUnloaded);
}
void SteamWorksStats::Stop()
{
    m_running = false;
    ++m_generation;
    m_unloadedCallback.Unregister();
    m_pending.clear(); // CCallResult destruction cancels every pending subscription.
}
bool SteamWorksStats::Request(ISteamGameServerStats* provider, CSteamID identity, AppId_t appid)
{
    if (!m_running || provider == nullptr || appid == k_uAppIdInvalid || !identity.IsValid()
        || identity.GetEAccountType() != k_EAccountTypeIndividual || identity.GetAccountID() == 0) { return false; }
    for (const auto& item : m_pending) {
        if (item.second->identity == identity && item.second->appid == appid) { return true; }
    }
    std::unique_ptr<Pending> request(new (std::nothrow) Pending(*this, identity, appid, m_generation));
    if (!request) { return false; }
    SteamAPICall_t call = provider->RequestUserStats(identity);
    if (call == k_uAPICallInvalid || m_pending.count(call) != 0) { return false; }
    request->call = call;
    auto* pending = request.get();
    m_pending.emplace(call, std::move(request));
    pending->result.Set(call, pending, &Pending::OnResult);
    return true;
}
void SteamWorksStats::Complete(Pending& pending, GSStatsReceived_t* payload, bool failed)
{
    auto entry = m_pending.find(pending.call);
    if (entry == m_pending.end() || entry->second.get() != &pending) { return; }
    // Retain this request across reentrant Stop/Start, new requests or owner teardown.
    auto request = std::move(entry->second);
    m_pending.erase(entry);
    if (!m_running || request->generation != m_generation || m_received == nullptr
        || m_received->GetFunctionCount() == 0) { return; }
    EResult result = failed ? k_EResultIOFailure : k_EResultFail;
    if (!failed && payload != nullptr && payload->m_steamIDUser == request->identity) { result = payload->m_eResult; }
    m_received->PushCell(CellBits(request->identity.GetAccountID()));
    m_received->PushCell(static_cast<cell_t>(result));
    m_received->PushCell(CellBits(request->appid));
    m_received->Execute(nullptr);
    // No owner access here: a forward may have stopped or destroyed the owner.
}
void SteamWorksStats::OnUnloaded(GSStatsUnloaded_t* payload)
{
    if (!m_running || payload == nullptr || !payload->m_steamIDUser.IsValid()
        || payload->m_steamIDUser.GetEAccountType() != k_EAccountTypeIndividual
        || m_unloaded == nullptr || m_unloaded->GetFunctionCount() == 0) { return; }
    m_unloaded->PushCell(CellBits(payload->m_steamIDUser.GetAccountID()));
    m_unloaded->Execute(nullptr);
}
