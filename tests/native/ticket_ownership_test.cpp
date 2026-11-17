// The legacy reader is not used by the auth hook. Test its latent ownership
// contract directly with synthetic bytes, never real auth tickets.
#include "steamclientpublic.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>
#include <vector>
#ifndef TICKET_SOURCE
#define TICKET_SOURCE "../../extension/steamtools/ticket.h"
#endif
#include TICKET_SOURCE

static_assert(sizeof(void*) == 4, "Ticket regressions require x86");
static_assert(!std::is_copy_constructible<DLCInfo>::value && !std::is_copy_assignable<DLCInfo>::value,
              "DLCInfo must reject copying");
static_assert(!std::is_copy_constructible<OwnershipSection>::value && !std::is_copy_assignable<OwnershipSection>::value,
              "OwnershipSection must reject copying");
static_assert(!std::is_copy_constructible<AuthBlob>::value && !std::is_copy_assignable<AuthBlob>::value,
              "AuthBlob must reject copying");
int checks = 0;
void Check(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); }
}
template<class T> void Append(std::vector<unsigned char>& bytes, T value) {
    auto* start = reinterpret_cast<const unsigned char*>(&value);
    bytes.insert(bytes.end(), start, start + sizeof(value));
}
std::vector<unsigned char> Ticket() {
    std::vector<unsigned char> bytes;
    Append<uint32>(bytes, GCTokenSection::expectedlen);
    Append<uint64>(bytes, 11); Append<uint64>(bytes, 22); Append<time_t>(bytes, 33);
    Append<uint32>(bytes, SessionSection::expectedlen);
    for (uint32 n = 0; n < 6; ++n) Append(bytes, n);
    Append<uint32>(bytes, 1); Append<uint32>(bytes, 200); Append<uint32>(bytes, 4);
    Append<uint64>(bytes, 22);
    for (uint32 n : {550u, 0u, 0u, 0u}) Append(bytes, n);
    Append<time_t>(bytes, 33); Append<time_t>(bytes, 44);
    Append<uint16>(bytes, 2); Append<uint32>(bytes, 77); Append<uint32>(bytes, 88);
    Append<uint16>(bytes, 1); Append<uint32>(bytes, 551);
    Append<uint16>(bytes, 2); Append<uint32>(bytes, 99); Append<uint32>(bytes, 100);
    Append<uint16>(bytes, 0); bytes.insert(bytes.end(), 128, 0xab);
    return bytes;
}
int main() {
    alignas(DLCInfo) unsigned char storage[sizeof(DLCInfo)];
    std::memset(storage, 0xa5, sizeof(storage));
    auto* dlc = new (storage) DLCInfo;
    const uint32 zeroApp = 0; const uint16 zeroCount = 0;
    // Compare representations: the old constructor must fail without reading
    // an indeterminate scalar value or relying on a sanitizer crash.
    Check(std::memcmp(&dlc->appid, &zeroApp, sizeof(zeroApp)) == 0, "default DLC appid is zero");
    Check(std::memcmp(&dlc->subcount, &zeroCount, sizeof(zeroCount)) == 0, "default DLC subcount is zero");
    Check(dlc->pSub == nullptr, "default DLC has no subscription owner");
    dlc->~DLCInfo();
    auto bytes = Ticket();
    {
        AuthBlob auth(bytes.data(), static_cast<int>(bytes.size()));
        Check(auth.bExpectedTicket, "synthetic complete ticket remains readable");
        auto& owner = *auth.pOwnershipSection;
        Check(owner.appid == 550 && owner.licenseCount == 2 && owner.pLicense[1] == 88, "licenses have one live owner");
        Check(owner.dlcCount == 1 && owner.pDLC[0].appid == 551 && owner.pDLC[0].subcount == 2
            && owner.pDLC[0].pSub[1] == 100, "nested DLC subscriptions survive until owner teardown");
    }
    // Every strict prefix exercises constructor early returns and destructors.
    for (size_t size = 0; size < bytes.size(); ++size) {
        AuthBlob auth(bytes.data(), static_cast<int>(size));
        Check(!auth.bExpectedTicket, "truncated ticket remains invalid and destructible");
    }
    {
        AuthBlob empty(nullptr, 0);
        Check(!empty.bExpectedTicket && empty.pOwnershipSection->pLicense == nullptr
            && empty.pOwnershipSection->pDLC == nullptr, "empty ticket has no uninitialized resource owner");
    }
    std::printf("PASS: %d legacy ticket initialization/ownership checks (ASan/UBSan x86)\n", checks);
}
