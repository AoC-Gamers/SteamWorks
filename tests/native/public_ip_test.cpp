#include "public_ip_test_host.h"
#include <array>
#include <limits>
#include <vector>

static void Check(bool condition, const char* native, const char* scenario)
{
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s / %s\n", native, scenario);
		std::exit(1);
	}
}

static SteamIPAddress_t IPv4(uint32 bits)
{
	SteamIPAddress_t address = {};
	address.m_eType = k_ESteamIPTypeIPv4;
	address.m_unIPv4 = bits;
	return address;
}

struct IPCase
{
	const char* name;
	SteamIPAddress_t address;
	bool available;
	bool invalidOutput;
	bool ipv4Ready;
	uint32 expectedBits;
	std::array<cell_t, 4> expectedOctets;
};

int main(int argc, char** argv)
{
	SteamWorksGSNatives registration;
	SteamIPAddress_t ipv6 = SteamIPAddress_t::IPv6Any();
	ipv6.m_rgubIPv6[0] = 0x20;
	ipv6.m_rgubIPv6[1] = 0x01;
	ipv6.m_rgubIPv6[2] = 0x0d;
	ipv6.m_rgubIPv6[3] = 0xb8;
	ipv6.m_rgubIPv6[15] = 1;
	const std::vector<IPCase> cases = {
		{"octet order", IPv4(0x01020304u), true, false, true, 0x01020304u, {{1, 2, 3, 4}}},
		{"loopback", SteamIPAddress_t::IPv4Loopback(), true, false, true, 0x7f000001u, {{127, 0, 0, 1}}},
		{"IPv4 unset", SteamIPAddress_t::IPv4Any(), true, false, false, 0, {{}}},
		{"IPv6 loopback rejected", SteamIPAddress_t::IPv6Loopback(), true, false, false, 0, {{}}},
		{"IPv6 nonzero prefix rejected", ipv6, true, false, false, 0, {{}}},
		{"IPv6 unset", SteamIPAddress_t::IPv6Any(), true, false, false, 0, {{}}},
		{"high-bit IPv4", IPv4(0xcb007109u), true, false, true, 0xcb007109u, {{203, 0, 113, 9}}},
		{"all bits set", IPv4(0xffffffffu), true, false, true, 0xffffffffu, {{255, 255, 255, 255}}},
		{"low bit", IPv4(1), true, false, true, 1, {{0, 0, 0, 1}}},
		{"interface absent", IPv4(0x01020304u), false, false, false, 0, {{}}},
		{"invalid output address", IPv4(0x01020304u), true, true, true, 0x01020304u, {{1, 2, 3, 4}}},
		{"sign boundary", IPv4(0x80000000u), true, false, true, 0x80000000u, {{128, 0, 0, 0}}},
		{"interior zero octets", IPv4(0x00ff0080u), true, false, true, 0x00ff0080u, {{0, 255, 0, 128}}},
		{"family rejected before output resolution", ipv6, true, true, false, 0, {{}}},
		{"interface absent before output resolution", IPv4(0x01020304u), false, true, false, 0, {{}}},
	};
	int scenarios = 0;
	for (auto* entry = shareSystem.registered; entry->name != nullptr; ++entry) {
		if (std::strcmp(entry->name, "SteamWorks_GetPublicIP") != 0 && std::strcmp(entry->name, "SteamWorks_GetPublicIPCell") != 0) { continue; }
		if (argc > 2 && std::strcmp(argv[2], entry->name) != 0) { continue; }
		const bool isCell = std::strcmp(entry->name, "SteamWorks_GetPublicIPCell") == 0;
		for (const auto& scenario : cases) {
			if (argc > 1 && std::strcmp(argv[1], scenario.name) != 0) { continue; }
			IPluginContext context;
			context.invalidOutput = scenario.invalidOutput;
			ISteamGameServer service;
			service.address = scenario.address;
			GameServer server;
			server.server = scenario.available ? &service : nullptr;
			g_SteamWorks.pSWGameServer = &server;
			const cell_t params[] = {isCell ? 0 : 1, 32};
			const cell_t result = entry->function(&context, params);
			const bool writes = !isCell && scenario.ipv4Ready && !scenario.invalidOutput;
			if (isCell) {
				uint32 bits;
				static_assert(sizeof(bits) == sizeof(result), "Cell must preserve all IPv4 bits");
				std::memcpy(&bits, &result, sizeof(bits));
				Check(bits == scenario.expectedBits, entry->name, scenario.name);
				if (scenario.expectedBits & 0x80000000u) { Check(result < 0, entry->name, "high-bit cell remains signed without losing bits"); }
			} else {
				Check(result == (writes ? 1 : 0), entry->name, scenario.name);
			}
			for (int octet = 0; octet < 4; ++octet) {
				Check(context.memory[octet + 1] == (writes ? scenario.expectedOctets[octet] : (octet + 1) * 11), entry->name, scenario.name);
			}
			Check(context.memory[0] == 0x12345678 && context.memory[5] == 0x23456789, entry->name, "four-cell output canaries");
			Check(service.queries == (scenario.available ? 1 : 0), entry->name, "one address query only when interface available");
			Check(context.resolutions == (!isCell && scenario.ipv4Ready ? 1 : 0), entry->name, "reject unavailable family/interface before output conversion");
			const bool errorExpected = !isCell && scenario.ipv4Ready && scenario.invalidOutput;
			Check(context.errors == (errorExpected ? 1 : 0), entry->name, "conversion error policy");
			if (errorExpected) { Check(context.errorCode == addressError, entry->name, "original conversion error propagated"); }
			++scenarios;
		}
	}
	Check(scenarios == (argc > 2 ? 1 : argc > 1 ? 2 : 30), "suite", "selected production IP entrypoints exercised");
	std::printf("PASS: %d public IP scenarios (real SDK address types, %s char, simulated host/Steam)\n",
		scenarios, std::numeric_limits<char>::is_signed ? "signed" : "unsigned");
	return 0;
}
