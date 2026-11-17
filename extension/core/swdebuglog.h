#pragma once

#include <cstdio>
#include <cstring>
#include <ctime>

namespace steamworks { namespace debug {

// Owns only the diagnostic file, independently of Steam's borrowed interfaces.
class DebugLog {
public:
 using MakeDirectory = bool (*)(const char*);
 using ReportError = void (*)();
 ~DebugLog() { Close(); }
 DebugLog() = default;
 DebugLog(const DebugLog&) = delete;
 DebugLog& operator=(const DebugLog&) = delete;
 void Initialize(const char* directory, MakeDirectory mkdir, ReportError error, size_t limit = 1024 * 1024) {
  Close(); m_mask = 0; m_failed = false; m_mkdir = mkdir; m_error = error; m_limit = limit;
  std::snprintf(m_directory, sizeof(m_directory), "%s", directory);
  std::snprintf(m_path, sizeof(m_path), "%s/debug.log", directory);
  std::snprintf(m_previous, sizeof(m_previous), "%s/debug.previous.log", directory);
 }
 void SetMask(unsigned mask) { m_mask = mask & 15u; if (!m_mask) Close(); }
 bool Enabled(unsigned category) const { return !m_failed && (m_mask & category) != 0; }
 void Write(unsigned category, const char* name, const char* event, const char* details) {
  if (!Enabled(category)) return;
  if (!m_file) {
   if (!m_mkdir || !m_mkdir(m_directory) || !(m_file = std::fopen(m_path, "ab"))) { Fail(); return; }
   if (std::fseek(m_file, 0, SEEK_END) != 0) { Fail(); return; }
   long size = std::ftell(m_file); if (size < 0) { Fail(); return; } m_bytes = static_cast<size_t>(size);
  }
  char line[1024], stamp[32] = {};
  const std::time_t now = std::time(nullptr); const std::tm* time = std::localtime(&now);
  if (time) std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", time);
  int count = std::snprintf(line, sizeof(line), "L %s: [%s] event=%s %s\n", stamp, name, event, details);
  if (count < 0) { Fail(); return; }
  size_t bytes = static_cast<size_t>(count) < sizeof(line) ? static_cast<size_t>(count) : sizeof(line) - 1;
  if (bytes > m_limit) { Fail(); return; }
  if (m_bytes > m_limit - bytes) {
   Close();
   if (std::rename(m_path, m_previous) != 0 || !(m_file = std::fopen(m_path, "wb"))) { Fail(); return; }
   m_bytes = 0;
  }
  if (std::fwrite(line, 1, bytes, m_file) != bytes || std::fflush(m_file) != 0) { Fail(); return; }
  m_bytes += bytes;
 }
 void Shutdown() { SetMask(0); }
private:
 void Close() { if (m_file) { std::fclose(m_file); m_file = nullptr; } }
 void Fail() { Close(); if (!m_failed) { m_failed = true; if (m_error) m_error(); } }
 FILE* m_file = nullptr;
 unsigned m_mask = 0;
 bool m_failed = false;
 size_t m_bytes = 0, m_limit = 1024 * 1024;
 MakeDirectory m_mkdir = nullptr;
 ReportError m_error = nullptr;
 char m_directory[1024] = {}, m_path[1100] = {}, m_previous[1100] = {};
};

} }
