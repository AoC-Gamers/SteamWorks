#pragma once

#include "smsdk_ext.h"
#include "swtypes.h"
#include <cstring>

namespace steamworks { namespace pawn {

static_assert(sizeof(cell_t) == sizeof(uint32), "SourcePawn cells must retain 32 bits");

inline uint32 Bits(cell_t cell)
{
	uint32 bits;
	std::memcpy(&bits, &cell, sizeof(bits));
	return bits;
}

inline cell_t Cell(uint32 bits)
{
	cell_t cell;
	std::memcpy(&cell, &bits, sizeof(cell));
	return cell;
}

inline AccountId Account(cell_t cell) { return AccountId(Bits(cell)); }
inline AppId App(cell_t cell) { return AppId(Bits(cell)); }

inline bool IdentityOptions(IPluginContext* context, const cell_t* params,
	unsigned char universePosition, unsigned char typePosition, EUniverse& universe, EAccountType& type)
{
	universe = k_EUniversePublic;
	type = k_EAccountTypeIndividual;
	if (params[0] >= universePosition) {
		cell_t value = params[universePosition];
		if (value < k_EUniverseInvalid || value >= k_EUniverseMax) {
			context->ThrowNativeError("Unsupported Steam universe");
			return false;
		}
		universe = static_cast<EUniverse>(value);
	}
	if (params[0] >= typePosition) {
		cell_t value = params[typePosition];
		if (value < k_EAccountTypeInvalid || value >= k_EAccountTypeMax) {
			context->ThrowNativeError("Unsupported Steam account type");
			return false;
		}
		type = static_cast<EAccountType>(value);
	}
	return true;
}

inline bool String(IPluginContext* context, cell_t address, char*& output)
{
	int error = context->LocalToString(address, &output);
	if (error == SP_ERROR_NONE) { return true; }
	context->ThrowNativeErrorEx(error, "Invalid SourcePawn string/buffer address");
	return false;
}

inline bool Output(IPluginContext* context, cell_t address, cell_t*& output)
{
	int error = context->LocalToPhysAddr(address, &output);
	if (error == SP_ERROR_NONE) { return true; }
	context->ThrowNativeErrorEx(error, "Invalid SourcePawn output address");
	return false;
}

inline bool Size(IPluginContext* context, cell_t input, ByteCount& output, bool allowZero = true)
{
	if (input < 0 || (!allowZero && input == 0)) {
		context->ThrowNativeError("Invalid buffer length or timeout");
		return false;
	}
	output = ByteCount(static_cast<size_t>(input));
	return true;
}

inline bool SteamSize(IPluginContext* context, cell_t input, uint32& output, bool allowZero = true)
{
	ByteCount size(0);
	return Size(context, input, size, allowZero) && ToSteamSize(size, output);
}

inline void StoreFloat(cell_t* output, float value)
{
	static_assert(sizeof(value) == sizeof(*output), "Float must fit a cell");
	std::memcpy(output, &value, sizeof(value));
}

inline bool StoreSize(IPluginContext* context, cell_t* output, uint32 size)
{
	if (size > static_cast<uint32>((std::numeric_limits<cell_t>::max)())) {
		context->ThrowNativeError("Steam size exceeds SourcePawn length range");
		return false;
	}
	*output = static_cast<cell_t>(size);
	return true;
}

}} // namespace steamworks::pawn
