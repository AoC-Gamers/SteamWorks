#include <amtl/am-string.h>
#include "public_ip_test_host.h"
#include <limits>
#include <vector>

static void Check(bool condition, const char* scenario, const char* property)
{
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s / %s\n", scenario, property);
		std::exit(1);
	}
}

struct SteamIDCase
{
	const char* name;
	cell_t capacity;
	const char* expectedText;
	cell_t expectedCount;
	bool invalidAddress = false;
	bool clientExists = true;
	bool connected = true;
	uint64 identity = 76561197960265851ull;
	int expectedErrors = 0;
	int expectedErrorCode = 0;
};

int main(int argc, char** argv)
{
	SteamWorksGSNatives registration;
	Native native = nullptr;
	for (auto* entry = shareSystem.registered; entry->name != nullptr; ++entry) {
		if (std::strcmp(entry->name, "SteamWorks_GetClientSteamID") == 0) { native = entry->function; }
	}
	Check(native != nullptr, "registry", "real production native registered");
	const uint64 id = 76561197960265851ull;
	const uint64 max = std::numeric_limits<uint64>::max();
	const std::vector<SteamIDCase> cases = {
		{"large buffer", 30, "76561197960265851", 18},
		{"exact full capacity", 18, "76561197960265851", 18},
		{"one byte short", 17, "7656119796026585", 17},
		{"small buffer", 5, "7656", 5},
		{"two-byte buffer", 2, "7", 2},
		{"terminator-only buffer", 1, "", 1},
		{"zero capacity", 0, nullptr, 0},
		{"negative capacity", -1, nullptr, 0, false, true, true, id, 1, 0},
		{"minimum signed capacity", std::numeric_limits<cell_t>::min(), nullptr, 0, false, true, true, id, 1, 0},
		{"invalid output address", 18, nullptr, 0, true, true, true, id, 1, addressError},
		{"zero capacity before address", 0, nullptr, 0, true},
		{"negative capacity before address", -1, nullptr, 0, true, true, true, id, 1, 0},
		{"missing client", 18, nullptr, 0, false, false, true, id, 1, 0},
		{"disconnected client", 18, nullptr, 0, false, true, false, id, 1, 0},
		{"client error before zero length", 0, nullptr, 0, false, false, true, id, 1, 0},
		{"client error before negative length", -1, nullptr, 0, false, false, true, id, 1, 0},
		// These are formatting boundaries, not claims that UINT64_MAX is a valid user ID.
		{"uint64 formatting boundary", 21, "18446744073709551615", 21, false, true, true, max},
		{"uint64 truncated", 20, "1844674407370955161", 20, false, true, true, max},
		{"uint64 minimum output", 1, "", 1, false, true, true, max},
	};
	int scenarios = 0;
	for (const auto& scenario : cases) {
		if (argc > 1 && std::strcmp(argv[1], scenario.name) != 0) { continue; }
		IPluginContext context;
		std::memset(context.string, 'Z', sizeof(context.string));
		context.stringOffset = 1;
		context.invalidOutput = scenario.invalidAddress;
		playerHelpers.exists = scenario.clientExists;
		playerHelpers.player.connected = scenario.connected;
		g_SteamWorks.identity = CSteamID(scenario.identity);
		g_SteamWorks.identityCalls = 0;
		g_SteamWorks.universeOption = k_EUniverseInvalid;
		g_SteamWorks.typeOption = k_EAccountTypeInvalid;
		sourceModHost.formatter = &ke::SafeVsprintf;
		sourceModHost.formatCalls = 0;
		const cell_t params[] = {3, 1, 64, scenario.capacity};
		const cell_t count = native(&context, params);
		Check(count == scenario.expectedCount, scenario.name, "stored byte count including NUL");
		const bool formatted = scenario.expectedText != nullptr;
		const size_t stored = formatted ? std::strlen(scenario.expectedText) + 1 : 0;
		for (size_t index = 0; index < sizeof(context.string); ++index) {
			if (formatted && index >= 1 && index < stored + 1) {
				Check(context.string[index] == scenario.expectedText[index - 1], scenario.name, "exact text and terminator");
			} else if (!formatted || index == 0 || index >= static_cast<size_t>(scenario.capacity) + 1) {
				Check(context.string[index] == 'Z', scenario.name, "canaries outside capacity and no writes on failure");
			}
		}
		Check(context.errors == scenario.expectedErrors && context.errorCode == scenario.expectedErrorCode, scenario.name, "native/conversion error policy");
		if (scenario.expectedErrors) {
			const char* cause = !scenario.clientExists || !scenario.connected ? "Client index"
				: scenario.capacity < 0 ? "length" : "SteamID output address";
			Check(context.errorMessage && std::strstr(context.errorMessage, cause), scenario.name, "correct error precedence and cause");
		}
		Check(sourceModHost.formatCalls == (formatted ? 1 : 0), scenario.name, "no formatter call on error or zero capacity");
		Check(g_SteamWorks.identityCalls == (formatted ? 1 : 0), scenario.name, "validate arguments before constructing identity");
		if (formatted) {
			Check(g_SteamWorks.universeOption == k_EUniversePublic && g_SteamWorks.typeOption == k_EAccountTypeIndividual, scenario.name, "legacy identity defaults decoded at boundary");
		}
		const bool resolved = scenario.clientExists && scenario.connected && scenario.capacity > 0;
		Check(context.stringResolutions == (resolved ? 1 : 0), scenario.name, "client then length then address precedence");
		++scenarios;
	}
	Check(scenarios == (argc > 1 ? 1 : 19), "suite", "all selected count and buffer scenarios exercised");
	std::printf("PASS: %d SteamID string scenarios (production native, real SDK ID and AMTL formatter, simulated SourceMod memory)\n", scenarios);
	return 0;
}
