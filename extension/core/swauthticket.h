#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace steamworks {

// SourcePawn Param_Array carries packed bytes in 32-bit cells. Its padded
// allocation must fit an object, not merely size_t (PTRDIFF_MAX on x86).
inline bool AuthTicketCellCount(int byteCount, size_t& output)
{
	if (byteCount < 0) { return false; }
	constexpr size_t cellBytes = sizeof(std::int32_t);
	const size_t bytes = static_cast<size_t>(byteCount);
	const size_t cells = bytes == 0 ? 1 : bytes / cellBytes + (bytes % cellBytes != 0);
	if (cells > static_cast<size_t>((std::numeric_limits<std::ptrdiff_t>::max)()) / cellBytes) {
		return false;
	}
	output = cells;
	return true;
}

} // namespace steamworks
