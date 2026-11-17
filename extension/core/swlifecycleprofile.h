#ifndef STEAMWORKS_LIFECYCLE_PROFILE_H_
#define STEAMWORKS_LIFECYCLE_PROFILE_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "swinitcallsite.h"
#if defined(__linux__) && defined(__i386__)
#include <link.h>
#endif

namespace steamworks {

constexpr const char* LifecycleProfileId = "l4d2-23990100-linux-x86";
constexpr const char* LifecycleInitSymbol = "SteamInternal_GameServer_Init";
constexpr const char* LifecycleShutdownSymbol = "SteamGameServer_Shutdown";
constexpr const char* LifecycleEngineBuildId = "b152439170af081a0d31fc357364a4cd4caa56ac";
constexpr const char* LifecycleSteamBuildId = "0d18f257d51538a7b34c594a31e042b04001fb1c";
enum class LifecycleEntry { Init, Shutdown };

inline bool MatchesLifecycleIdentity(bool linuxX86, const char* engine, const char* steam,
                                    size_t engineCount, size_t steamCount)
{
 return linuxX86 && engineCount == 1 && steamCount == 1 && engine != nullptr && steam != nullptr
     && std::strcmp(engine, LifecycleEngineBuildId) == 0
     && std::strcmp(steam, LifecycleSteamBuildId) == 0;
}

struct LifecycleModules {
 const char* steamPath = nullptr;
 uintptr_t steamBase = 0;
 uintptr_t engineBase = 0;
 size_t engineCount = 0, steamCount = 0;
 char engineId[65] = {}, steamId[65] = {};
};

#if defined(__linux__) && defined(__i386__)
inline bool ProfileMappedRange(const dl_phdr_info* module, uintptr_t address, size_t size, bool executable)
{
 for (ElfW(Half) i = 0; i < module->dlpi_phnum; ++i) {
  const auto& ph = module->dlpi_phdr[i];
  if (ph.p_type != PT_LOAD || (executable && !(ph.p_flags & PF_X))) { continue; }
  uintptr_t begin = module->dlpi_addr + ph.p_vaddr;
  if (address >= begin && size <= ph.p_memsz && address - begin <= ph.p_memsz - size) { return true; }
 }
 return false;
}

inline void ProfileReadBuildId(const dl_phdr_info* module, char (&output)[65])
{
 for (ElfW(Half) i = 0; i < module->dlpi_phnum; ++i) {
  const auto& ph = module->dlpi_phdr[i];
  if (ph.p_type != PT_NOTE) { continue; }
  uintptr_t address = module->dlpi_addr + ph.p_vaddr;
  if (!ProfileMappedRange(module, address, ph.p_filesz, false)) { continue; }
  const auto* bytes = reinterpret_cast<const unsigned char*>(address);
  size_t offset = 0;
  while (offset <= ph.p_filesz && ph.p_filesz - offset >= sizeof(ElfW(Nhdr))) {
   ElfW(Nhdr) note;
   std::memcpy(&note, bytes + offset, sizeof(note)); offset += sizeof(note);
   size_t remaining = ph.p_filesz - offset;
   if (note.n_namesz > remaining) { return; }
   size_t nameSize = (static_cast<size_t>(note.n_namesz) + 3) & ~size_t(3);
   if (nameSize > remaining) { return; }
   const auto* name = bytes + offset; offset += nameSize;
   remaining = ph.p_filesz - offset;
   if (note.n_descsz > remaining) { return; }
   size_t descSize = (static_cast<size_t>(note.n_descsz) + 3) & ~size_t(3);
   if (descSize > remaining) { return; }
   if (note.n_type == NT_GNU_BUILD_ID && note.n_namesz == 4 && std::memcmp(name, "GNU\0", 4) == 0
       && note.n_descsz > 0 && note.n_descsz <= 32) {
    static const char hex[] = "0123456789abcdef";
    for (size_t n = 0; n < note.n_descsz; ++n) {
     output[n * 2] = hex[bytes[offset + n] >> 4];
     output[n * 2 + 1] = hex[bytes[offset + n] & 15];
    }
    output[note.n_descsz * 2] = '\0'; return;
   }
   offset += descSize;
  }
 }
}

inline int ProfileVisitModule(dl_phdr_info* module, size_t, void* data)
{
 auto& result = *static_cast<LifecycleModules*>(data);
 const char* path = module->dlpi_name;
 if (path == nullptr) { return 0; }
 const char* name = std::strrchr(path, '/'); name = name != nullptr ? name + 1 : path;
 if (std::strcmp(name, "engine_srv.so") == 0) {
  ++result.engineCount; ProfileReadBuildId(module, result.engineId);
  result.engineBase = module->dlpi_addr;
 } else if (std::strcmp(name, "libsteam_api.so") == 0) {
  ++result.steamCount; ProfileReadBuildId(module, result.steamId);
  result.steamPath = path; result.steamBase = module->dlpi_addr;
 }
 return 0;
}
#endif

inline LifecycleModules FindLifecycleModules()
{
 LifecycleModules result;
#if defined(__linux__) && defined(__i386__)
 dl_iterate_phdr(ProfileVisitModule, &result);
#endif
 return result;
}

inline bool HasVerifiedLifecycleProfile()
{
 const auto modules = FindLifecycleModules();
#if defined(__linux__) && defined(__i386__)
 return MatchesLifecycleIdentity(true, modules.engineId, modules.steamId, modules.engineCount, modules.steamCount);
#else
 (void)modules; return false;
#endif
}

inline bool IsVerifiedLifecycleAddress(void* candidate, LifecycleEntry entry)
{
#if defined(__linux__) && defined(__i386__)
 const auto modules = FindLifecycleModules();
 if (!MatchesLifecycleIdentity(true, modules.engineId, modules.steamId, modules.engineCount, modules.steamCount)) { return false; }
 const uintptr_t rva = entry == LifecycleEntry::Init ? 0x1b430 : 0x1b690;
 static const unsigned char init[] = {0x55,0x57,0x56,0x53,0xe8,0x42,0xaa,0xff,0xff};
 static const unsigned char shutdown[] = {0x57,0x56,0x53,0xe8,0xe3,0xa7,0xff,0xff};
 const unsigned char* expected = entry == LifecycleEntry::Init ? init : shutdown;
 size_t length = entry == LifecycleEntry::Init ? sizeof(init) : sizeof(shutdown);
 if (reinterpret_cast<uintptr_t>(candidate) != modules.steamBase + rva) { return false; }
 struct AddressCheck { uintptr_t base, address; size_t length; bool valid; } check{modules.steamBase, reinterpret_cast<uintptr_t>(candidate), length, false};
 dl_iterate_phdr([](dl_phdr_info* module, size_t, void* data) -> int {
  auto& c = *static_cast<AddressCheck*>(data);
  if (module->dlpi_addr == c.base) { c.valid = ProfileMappedRange(module, c.address, c.length, true); return 1; }
  return 0;
 }, &check);
 return check.valid && std::memcmp(candidate, expected, length) == 0;
#else
 (void)candidate; (void)entry; return false;
#endif
}

// The engine CALL stays independent of any hook on the SDK's shared export.
// Fingerprint the caller and its relocated destination, not a foreign detour.
inline void* VerifiedLifecycleInitCall(void* sdkInit)
{
#if defined(__linux__) && defined(__i386__)
 const auto modules = FindLifecycleModules();
 if (!MatchesLifecycleIdentity(true, modules.engineId, modules.steamId, modules.engineCount, modules.steamCount)
     || reinterpret_cast<uintptr_t>(sdkInit) != modules.steamBase + 0x1b430) return nullptr;
 struct Check { uintptr_t engine, steam; bool caller=false, target=false; } check{modules.engineBase, modules.steamBase};
 dl_iterate_phdr([](dl_phdr_info* module, size_t, void* data)->int {
  auto& c=*static_cast<Check*>(data);
  if (module->dlpi_addr==c.engine) c.caller=ProfileMappedRange(module,c.engine+0x2018e2,18,true);
  if (module->dlpi_addr==c.steam) c.target=ProfileMappedRange(module,c.steam+0x1b430,9,true);
  return 0;
 }, &check);
 if (!check.caller || !check.target) return nullptr;
 auto* call=reinterpret_cast<unsigned char*>(modules.engineBase+0x2018eb);
 static const unsigned char before[]={0x8b,0x83,0xb4,0,0,0,0x89,0x04,0x24};
 static const unsigned char after[]={0x84,0xc0,0x0f,0x84};
 if (std::memcmp(call-sizeof(before),before,sizeof(before))!=0 || call[0]!=0xe8
     || std::memcmp(call+5,after,sizeof(after))!=0
     || InitCallSite::DecodeTarget(call)!=reinterpret_cast<uintptr_t>(sdkInit)) return nullptr;
 return call;
#else
 (void)sdkInit; return nullptr;
#endif
}

} // namespace steamworks
#endif
