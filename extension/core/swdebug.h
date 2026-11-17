#pragma once

#include <cstdarg>

namespace steamworks { namespace debug {

enum Category : unsigned { Lifecycle = 1, Interfaces = 2, Hooks = 4, Auth = 8, All = 15 };

// The inert default also lets isolated consumers run without an engine logger.
struct Backend {
 void (*initialize)() = nullptr;
 void (*shutdown)() = nullptr;
 bool (*enabled)(Category) = nullptr;
 void (*write)(Category, const char*, const char*, va_list) = nullptr;
};
inline Backend& GetBackend() { static Backend backend; return backend; }
inline void Initialize() { if (GetBackend().initialize) GetBackend().initialize(); }
inline void Shutdown() { if (GetBackend().shutdown) GetBackend().shutdown(); }
inline bool Enabled(Category category) { return GetBackend().enabled && GetBackend().enabled(category); }
inline void Log(Category category, const char* event, const char* format = "", ...) {
 if (!Enabled(category) || !GetBackend().write) return;
 va_list args; va_start(args, format); GetBackend().write(category, event, format, args); va_end(args);
}

} }
