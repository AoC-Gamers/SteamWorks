// Both production native groups must consume player slots independently of entities.
#ifdef STEAMWORKS_TEST_STATS_CLIENTS
#include "stats_test_host.h"
#else
#include <amtl/am-string.h>
#include "public_ip_test_host.h"
#endif
#include <limits>

static void Check(bool condition, const char* native, const char* scenario, const char* property)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s / %s / %s\n", native, scenario, property);
        std::exit(1);
    }
}

struct ClientCase
{
    const char* name;
    cell_t index;
    bool entityPresent;
    bool exists;
    bool connected;
    bool valid;
};

int main()
{
#ifdef STEAMWORKS_TEST_STATS_CLIENTS
    SteamWorksSSNatives registration;
    const char* names[] = {"SteamWorks_RequestStats", "SteamWorks_GetStatCell", "SteamWorks_GetStatFloat"};
#else
    SteamWorksGSNatives registration;
    const char* names[] = {"SteamWorks_HasLicenseForApp", "SteamWorks_GetUserGroupStatus", "SteamWorks_GetClientSteamID"};
#endif
    const ClientCase cases[] = {
        {"first slot", 1, true, true, true, true},
        {"last slot", 32, true, true, true, true},
        {"connected player without entity", 1, false, true, true, true},
        {"server index", 0, true, true, true, false},
        {"negative index", -1, true, true, true, false},
        {"above maximum", 33, true, true, true, false},
        {"maximum cell", std::numeric_limits<cell_t>::max(), true, true, true, false},
        {"minimum cell", std::numeric_limits<cell_t>::min(), true, true, true, false},
        {"entity reference is not a client index", static_cast<cell_t>(0x80010001u), true, true, true, false},
        {"missing player", 1, true, false, true, false},
        {"disconnected player", 1, true, true, false, false},
    };
    int scenarios = 0;
    for (const char* name : names) {
        Native native = nullptr;
        for (auto* entry = shareSystem.registered; entry->name; ++entry) {
            if (std::strcmp(entry->name, name) == 0) { native = entry->function; }
        }
        Check(native != nullptr, name, "registry", "production native registered");
        for (const auto& scenario : cases) {
            IPluginContext context;
            GameServer server;
            playerHelpers = PlayerHelpers{};
            gameHelpers = GameHelpers{};
            playerHelpers.exists = scenario.exists;
            playerHelpers.player.connected = scenario.connected;
            gameHelpers.entityPresent = scenario.entityPresent;
            g_SteamWorks.pSWGameServer = &server;
#ifdef STEAMWORKS_TEST_STATS_CLIENTS
            ISteamGameServerStats steam;
            ISteamUtils utils;
            steam.succeeds = true;
            server.stats = &steam;
            server.utils = &utils;
            const bool request = std::strcmp(name, "SteamWorks_RequestStats") == 0;
            const cell_t params[] = {request ? 2 : 3, scenario.index, request ? 550 : 64, 128};
            const cell_t result = native(&context, params);
            const cell_t expected = scenario.valid ? 1 : 0;
            const int backendCalls = steam.requests + steam.calls;
            if (!scenario.valid) {
                Check(context.storage[1] == sentinel, name, scenario.name, "stat output preserved");
            }
#else
            ISteamGameServer steam;
            server.server = &steam;
            g_SteamWorks.identityCalls = 0;
            sourceModHost.formatter = &ke::SafeVsprintf;
            sourceModHost.formatCalls = 0;
            std::memset(context.string, 'Z', sizeof(context.string));
            const bool identity = std::strcmp(name, "SteamWorks_GetClientSteamID") == 0;
            const bool license = std::strcmp(name, "SteamWorks_HasLicenseForApp") == 0;
            const cell_t params[] = {identity ? 3 : 2, scenario.index, identity ? 64 : 550, 18};
            const cell_t result = native(&context, params);
            const cell_t expected = !scenario.valid ? 0 : identity ? 18
                : license ? static_cast<cell_t>(k_EUserHasLicenseResultNoAuth) : 1;
            const int backendCalls = steam.licenseQueries + steam.groupQueries + sourceModHost.formatCalls;
            Check(g_SteamWorks.identityCalls == (scenario.valid ? 1 : 0), name, scenario.name, "identity only for valid connected player");
            if (!scenario.valid) {
                for (char byte : context.string) {
                    Check(byte == 'Z', name, scenario.name, "text output preserved");
                }
            }
#endif
            Check(result == expected, name, scenario.name, "result independent of entity availability");
            Check(context.errors == (scenario.valid ? 0 : 1), name, scenario.name, "native error policy");
            Check(backendCalls == (scenario.valid ? 1 : 0), name, scenario.name, "no downstream work on invalid clients");
            const bool inRange = scenario.index >= 1 && scenario.index <= playerHelpers.GetMaxClients();
            Check(playerHelpers.lookups == (inRange ? 1 : 0), name, scenario.name, "bounds checked before player lookup");
            if (inRange) {
                Check(playerHelpers.lastIndex == scenario.index, name, scenario.name, "exact client slot selected");
            }
            ++scenarios;
        }
    }
    Check(scenarios == 33, "suite", "matrix", "all three natives and eleven client states exercised");
    std::printf("PASS: %d client-index scenarios (production natives, independent player/entity doubles)\n", scenarios);
}
