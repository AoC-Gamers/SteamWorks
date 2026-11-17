#include "stats_test_host.h"

static void Check(bool condition, const char* native, const char* scenario)
{
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s / %s\n", native, scenario);
		std::exit(1);
	}
}

static cell_t CellBits(uint32 value)
{
	cell_t cell;
	static_assert(sizeof(value) == sizeof(cell), "AccountID/AppID must fit a cell");
	std::memcpy(&cell, &value, sizeof(cell));
	return cell;
}

int main(int argc, char** argv)
{
	SteamWorksSSNatives registration;
	int scenarios = 0;
	int expectedScenarios = 0;
	for (auto* entry = shareSystem.registered; entry->name != nullptr; ++entry) {
		if (std::strstr(entry->name, "RequestStats") == nullptr) { continue; }
		if (argc > 1 && std::strcmp(entry->name, argv[1]) != 0) { continue; }
		const bool byClient = std::strcmp(entry->name, "SteamWorks_RequestStats") == 0;
		expectedScenarios += byClient ? 24 : 14;
		for (int scenario = 0; scenario < 24; ++scenario) {
			if (!byClient && scenario >= 10 && scenario <= 19) { continue; }
			IPluginContext context;
			ISteamGameServerStats stats;
			ISteamUtils utils;
			GameServer server;
			server.stats = &stats;
			server.utils = &utils;
			g_SteamWorks.pSWGameServer = &server;
			g_SteamWorks.legacyIdentityCalls = 0;
			statsOwner = StatsOwner{};
			g_SteamWorks.pSWStats = &statsOwner;
			playerHelpers.exists = true;
			playerHelpers.player = IGamePlayer{};
			auto& player = playerHelpers.player;
			cell_t params[] = {2, byClient ? 1 : 123, 550};
			CSteamID expectedIdentity(123, k_EUniversePublic, k_EAccountTypeIndividual);
			bool queryExpected = false;
			const char* label = "";
			switch (scenario) {
				case 0: label = "foreign AppID rejected"; params[2] = 440; break;
				case 1: label = "550 does not become universe"; queryExpected = true; break;
				case 2: label = "matched context AppID is not hardcoded"; utils.appid = 440; params[2] = 440; queryExpected = true; break;
				case 3: label = "zero AppID rejected"; params[2] = 0; break;
				case 4: label = "unknown context AppID rejected"; utils.appid = 0; break;
				case 5: label = "Utils unavailable"; server.utils = nullptr; break;
				case 6: label = "Stats unavailable"; server.stats = nullptr; break;
				case 7: label = "invalid async call result"; stats.requestResult = k_uAPICallInvalid; queryExpected = true; break;
				case 8:
					label = "high-bit AccountID preserved";
					expectedIdentity = CSteamID(0x80000001u, k_EUniversePublic, k_EAccountTypeIndividual);
					player.account = 0x80000001u;
					player.steamId = expectedIdentity.ConvertToUint64();
					if (!byClient) { params[1] = CellBits(0x80000001u); }
					queryExpected = true;
					break;
				case 9: label = "zero AccountID rejected"; player.account = 0; if (!byClient) { params[1] = 0; } break;
				case 10: label = "missing client"; playerHelpers.exists = false; break;
				case 11: label = "disconnected client"; player.connected = false; break;
				case 12: label = "bot rejected"; player.fake = true; break;
				case 13: label = "unauthorized client rejected"; player.authorized = false; break;
				case 14: label = "validated AccountID fallback"; player.steamId = 0; queryExpected = true; break;
				case 15:
					label = "validated universe preserved";
					expectedIdentity = CSteamID(123, k_EUniverseBeta, k_EAccountTypeIndividual);
					player.steamId = expectedIdentity.ConvertToUint64();
					queryExpected = true;
					break;
				case 16: label = "conflicting full identity rejected"; player.steamId = CSteamID(124, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64(); break;
				case 17: label = "non-individual identity rejected"; player.steamId = CSteamID(123, k_EUniversePublic, k_EAccountTypeClan).ConvertToUint64(); break;
				case 18: label = "invalid universe rejected without fallback"; player.steamId = CSteamID(123, k_EUniverseInvalid, k_EAccountTypeIndividual).ConvertToUint64(); break;
				case 19: label = "invalid individual instance rejected"; player.steamId = CSteamID(123, 2, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64(); break;
				case 20: label = "AppID bit pattern compared as uint32"; utils.appid = 0x80000001u; params[2] = CellBits(utils.appid); queryExpected = true; break;
				case 21: label = "legacy universe argument is not AppID"; params[2] = 1; break;
				case 22: label = "callback owner unavailable"; g_SteamWorks.pSWStats = nullptr; break;
				case 23: label = "callback owner stopped"; statsOwner.accepting = false; break;
			}
			const cell_t result = entry->function(&context, params);
			Check(result == (queryExpected && scenario != 7 ? 1 : 0), entry->name, label);
			Check(stats.requests == (queryExpected ? 1 : 0), entry->name, label);
			Check(statsOwner.calls == (queryExpected || scenario == 23 ? 1 : 0), entry->name, "guarded async owner acquisition");
			if (queryExpected) { Check(statsOwner.capturedApp == utils.appid, entry->name, "AppID captured independently of identity"); }
			if (queryExpected) {
				Check(stats.requestedIdentity.ConvertToUint64() == expectedIdentity.ConvertToUint64(), entry->name, "exact identity delivered to Steam");
			}
			Check(g_SteamWorks.legacyIdentityCalls == 0, entry->name, "request does not use params-based identity builder");
			Check(player.unvalidatedReads == 0, entry->name, "validated SourceMod identity only");
			Check(context.errors == (scenario == 10 || scenario == 11 ? 1 : 0), entry->name, "error policy");
			Check(context.storage[1] == sentinel, entry->name, "no writes to plugin memory");
			++scenarios;
		}
	}
	Check(scenarios > 0 && scenarios == expectedScenarios, "suite", "all selected request scenarios exercised");
	std::printf("PASS: %d stats request scenarios (real SDK identity types, simulated Steam/SourceMod services)\n", scenarios);
	return 0;
}
