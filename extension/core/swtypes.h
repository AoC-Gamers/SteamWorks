#pragma once

#include "steamclientpublic.h"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace steamworks {

struct AccountId { explicit AccountId(AccountID_t bits) : value(bits) {} AccountID_t value; };
struct AppId { explicit AppId(AppId_t bits) : value(bits) {} AppId_t value; };
struct ByteCount { explicit ByteCount(size_t bytes) : value(bytes) {} size_t value; };

static_assert(sizeof(AccountID_t) == 4 && sizeof(AppId_t) == 4, "Steam IDs must retain 32 bits");
static_assert(sizeof(SteamAPICall_t) == 8, "Async Steam calls must retain 64 bits");

inline bool ToSteamSize(ByteCount size, uint32& output)
{
	if (size.value > (std::numeric_limits<uint32>::max)()) { return false; }
	output = static_cast<uint32>(size.value);
	return true;
}

inline uint64 PackContext(uint32 high, uint32 low)
{
	return (static_cast<uint64>(high) << 32) | low;
}

inline CSteamID IndividualIdentity(AccountId account)
{
	return CSteamID(account.value, k_EUniversePublic, k_EAccountTypeIndividual);
}

} // namespace steamworks
