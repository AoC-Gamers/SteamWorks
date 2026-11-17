#include "stats_test_host.h"

static void Check(bool condition, const char* native, const char* scenario)
{
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s / %s\n", native, scenario);
		std::exit(1);
	}
}

int main(int argc, char** argv)
{
	SteamWorksSSNatives registration;
	Check(shareSystem.registered == ssnatives, "registry", "production native table");
	int scenarios = 0;
	int expectedScenarios = 0;
	for (auto* entry = shareSystem.registered; entry->name != nullptr; ++entry) {
		if (std::strstr(entry->name, "GetStat") == nullptr) {
			continue;
		}
		if (argc > 1 && std::strcmp(entry->name, argv[1]) != 0) {
			continue;
		}
		const bool isFloat = std::strstr(entry->name, "Float") != nullptr;
		const bool byClient = std::strstr(entry->name, "AuthID") == nullptr;
		expectedScenarios += byClient ? 9 : 7;
		// Exercise the three public arguments. Identity is simulated; the
		// legacy identity options and RequestStats AppID contract are not tested.
		const cell_t params[] = {3, byClient ? 1 : 123, 64, 128};
		for (int scenario = 0; scenario < 9; ++scenario) {
			IPluginContext context;
			ISteamGameServerStats stats;
			GameServer server;
			server.stats = &stats;
			g_SteamWorks.pSWGameServer = &server;
			playerHelpers.exists = true;
			playerHelpers.player.connected = true;
			const char* label = "";
			switch (scenario) {
				case 0: label = "failure after writing temporary"; stats.writesOnFailure = true; break;
				case 1: label = "failure without writing"; break;
				case 2: label = "success with nonzero value"; stats.succeeds = true; break;
				case 3: label = "success with zero"; stats.succeeds = true; stats.integerValue = 0; stats.floatValue = 0.0f; break;
				case 4: label = "invalid output address"; context.invalidOutput = true; stats.succeeds = true; break;
				case 5: label = "invalid name address"; context.invalidName = true; stats.succeeds = true; break;
				case 6: label = "missing stats interface"; server.stats = nullptr; context.invalidOutput = true; break;
				case 7: label = "missing client"; playerHelpers.exists = false; stats.succeeds = true; break;
				case 8: label = "disconnected client"; playerHelpers.player.connected = false; stats.succeeds = true; break;
			}
			if (!byClient && scenario >= 7) {
				continue;
			}
			const cell_t result = entry->function(&context, params);
			const bool success = scenario == 2 || scenario == 3;
			Check(result == (success ? 1 : 0), entry->name, label);
			const cell_t expected = success ? (isFloat ? sp_ftoc(stats.floatValue) : stats.integerValue) : sentinel;
			Check(context.storage[1] == expected, entry->name, label);
			Check(context.storage[0] == 0x11223344 && context.storage[2] == 0x55667788, entry->name, "output canaries");
			const bool queryExpected = scenario <= 3;
			Check(stats.calls == (queryExpected ? 1 : 0), entry->name, "no Steam query on invalid input/unavailable interface");
			Check(stats.floatCalls == (queryExpected && isFloat ? 1 : 0), entry->name, "float overload");
			Check(stats.integerCalls == (queryExpected && !isFloat ? 1 : 0), entry->name, "integer overload");
			Check(context.errors == ((scenario == 4 || scenario == 5 || scenario >= 7) ? 1 : 0), entry->name, "native error count");
			if (scenario == 4 || scenario == 5) {
				Check(context.errorCode == conversionError, entry->name, "conversion error propagated");
			}
			if (scenario == 5 || scenario >= 6) {
				Check(context.outputResolutions == 0, entry->name, "preserve error precedence");
			}
			++scenarios;
		}
	}
	Check(scenarios > 0 && scenarios == expectedScenarios, "suite", "all selected native variants exercised");
	std::printf("PASS: %d stats native scenarios (production entrypoints, simulated Steam/SourceMod)\n", scenarios);
	return 0;
}
