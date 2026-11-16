#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <initializer_list>
#include <unistd.h>
#define PLATFORM_POSIX
#include "../extension/CDetour/detourhelpers.h"

static int checks = 0;
static void Check(bool condition, const char* message)
{
	++checks;
	if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main()
{
	unsigned char input[20];
	for (size_t i = 0; i < sizeof(input); ++i) { input[i] = static_cast<unsigned char>(i + 1); }
	patch_t restore;
	for (size_t bytes = 1; bytes <= sizeof(input); ++bytes) {
		std::memset(restore.patch, 0xa5, sizeof(restore.patch));
		restore.bytes = 99;
		Check(SaveRestorePatch(input, bytes, restore), "every representable restore length accepted");
		Check(restore.bytes == bytes && std::memcmp(input, restore.patch, bytes) == 0, "exact restore bytes and length saved");
		for (size_t i = bytes; i < sizeof(restore.patch); ++i) {
			Check(restore.patch[i] == 0xa5, "unused backup bytes preserved");
		}
	}
	for (size_t bytes : {size_t(0), size_t(21), size_t(22), std::numeric_limits<size_t>::max()}) {
		std::memset(restore.patch, 0xa5, sizeof(restore.patch));
		restore.bytes = 99;
		// A null source proves rejection precedes any source read or backup write.
		Check(!SaveRestorePatch(nullptr, bytes, restore), "zero or oversized restore rejected before reading source");
		Check(restore.bytes == 99, "rejection preserves backup length");
		for (unsigned char byte : restore.patch) { Check(byte == 0xa5, "rejection preserves backup bytes"); }
	}
	std::printf("PASS: %d CDetour backup bounds checks (production helper, no live patching)\n", checks);
}
