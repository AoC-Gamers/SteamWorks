#ifndef STEAMWORKS_INIT_CALLSITE_H_
#define STEAMWORKS_INIT_CALLSITE_H_
#include <cstdint>
#include <cstring>
#if defined(__linux__) && defined(__i386__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace steamworks {

// One audited CALL instruction, not a detour/trampoline on the shared SDK entry.
// The target remains the original SDK export, including its current owner.
class InitCallSite
{
public:
 using Writer = bool (*)(void*, const void*, size_t);
 explicit InitCallSite(Writer writer = WriteCode) : m_writer(writer) {}
 ~InitCallSite() { Restore(); }
 InitCallSite(const InitCallSite&) = delete;
 InitCallSite& operator=(const InitCallSite&) = delete;

 bool Prepare(void* site, void* target, void* callback)
 {
  if (sizeof(uintptr_t) != 4 || m_site != nullptr || site == nullptr || target == nullptr || callback == nullptr) return false;
  auto* bytes = static_cast<unsigned char*>(site);
  if (bytes[0] != 0xe8 || DecodeTarget(bytes) != reinterpret_cast<uintptr_t>(target)) return false;
  std::memcpy(m_original, bytes, sizeof(m_original));
  m_patch[0] = 0xe8;
  uint32_t displacement = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(callback))
                        - static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)) - 5u;
  std::memcpy(m_patch + 1, &displacement, sizeof(displacement));
  m_site = bytes;
  return true;
 }
 bool Enable()
 {
  if (m_installed) return true;
  if (m_site == nullptr || std::memcmp(m_site, m_original, 5) != 0 || !m_writer(m_site, m_patch, 5)) return false;
  m_installed = true;
  return true;
 }
 bool Restore()
 {
  if (!m_installed) return true;
  // Never overwrite another owner's unexpected change to the engine call.
  if (std::memcmp(m_site, m_patch, 5) != 0 || !m_writer(m_site, m_original, 5)) return false;
  m_installed = false;
  return true;
 }
 bool IsInstalled() const { return m_installed; }
 void Reset() { if (Restore()) m_site = nullptr; }
 static uintptr_t DecodeTarget(const unsigned char* call)
 {
  uint32_t displacement = 0;
  std::memcpy(&displacement, call + 1, sizeof(displacement));
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(call)) + 5u + displacement;
 }
private:
 static bool WriteCode(void* address, const void* bytes, size_t size)
 {
#if defined(__linux__) && defined(__i386__)
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0 || size != 5) return false;
  uintptr_t begin = reinterpret_cast<uintptr_t>(address);
  uintptr_t page = begin - begin % static_cast<uintptr_t>(pageSize);
  size_t length = begin - page + size;
  // Same executable-page policy as CDetour, with checked protection failure.
  if (mprotect(reinterpret_cast<void*>(page), length, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) return false;
  std::memcpy(address, bytes, size);
  return true;
#else
  (void)address; (void)bytes; (void)size; return false;
#endif
 }
 Writer m_writer;
 unsigned char* m_site = nullptr;
 unsigned char m_original[5] = {}, m_patch[5] = {};
 bool m_installed = false;
};
}
#endif
