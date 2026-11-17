#include "../extension.h"
#include "swdebug.h"
#include "swdebuglog.h"
#include "tier1/convar.h"
#include "icvar.h"
#include <cstdio>
#include <cstdarg>

// Kept local to this DSO by the build's hidden visibility, as in tier1 clients.
ICvar* g_pCVar = nullptr;

namespace {
steamworks::debug::DebugLog debugLog;
bool ready = false;
void DebugChanged(IConVar*, const char*, float);
void InitializeLog();
void ShutdownLog();
bool LogEnabled(steamworks::debug::Category);
void WriteLog(steamworks::debug::Category, const char*, const char*, va_list);
ConVar debugMask("steamworks_debug", "0", FCVAR_NONE,
 "SteamWorks debug mask: 0 off, 1 lifecycle, 2 interfaces, 4 hooks, 8 auth, 15 all; no tickets or identities.",
 true, 0.0f, true, 15.0f, DebugChanged);
class Accessor : public IConCommandBaseAccessor {
 bool RegisterConCommandBase(ConCommandBase* command) override { return META_REGCVAR(command); }
} accessor;
bool MakeDirectory(const char* path) { return libsys->IsPathDirectory(path) || libsys->CreateFolder(path); }
void ReportError() { g_pSM->LogError(myself, "SteamWorks debug log unavailable; diagnostics disabled until extension reload"); }
void DebugChanged(IConVar*, const char*, float) {
 if (!ready) return;
 debugLog.SetMask(static_cast<unsigned>(debugMask.GetInt()));
 steamworks::debug::Log(steamworks::debug::Lifecycle, "debug_enabled", "mask=%d", debugMask.GetInt());
 if (g_SteamWorks.pGSHooks != nullptr) g_SteamWorks.pGSHooks->TraceState();
}
}

bool SteamWorks::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlength, bool) {
 g_pCVar = static_cast<ICvar*>(ismm->GetEngineFactory()(CVAR_INTERFACE_VERSION, nullptr));
 if (!g_pCVar) { std::snprintf(error, maxlength, "Engine cvar interface unavailable"); return false; }
 if (g_pCVar->FindVar("steamworks_debug") != nullptr) {
  std::snprintf(error, maxlength, "steamworks_debug is already registered by another owner"); return false;
 }
 ConVar_Register(0, &accessor);
 if (!debugMask.IsRegistered()) {
  ConVar_Unregister(); g_pCVar = nullptr;
  std::snprintf(error, maxlength, "Could not register steamworks_debug"); return false;
 }
 steamworks::debug::GetBackend() = {InitializeLog, ShutdownLog, LogEnabled, WriteLog};
 return true;
}

bool SteamWorks::SDK_OnMetamodUnload(char*, size_t) {
 steamworks::debug::Shutdown();
 ConVar_Unregister();
 g_pCVar = nullptr;
 steamworks::debug::GetBackend() = {};
 return true;
}

namespace {
using namespace steamworks::debug;
void InitializeLog() {
 char path[1024]; g_pSM->BuildPath(Path_SM, path, sizeof(path), "logs/steamworks");
 debugLog.Initialize(path, MakeDirectory, ReportError); ready = true;
 debugLog.SetMask(static_cast<unsigned>(debugMask.GetInt()));
}
void ShutdownLog() { debugLog.Shutdown(); ready = false; }
bool LogEnabled(Category category) { return ready && debugLog.Enabled(category); }
void WriteLog(Category category, const char* event, const char* format, va_list args) {
 char details[512];
 std::vsnprintf(details, sizeof(details), format, args);
 const char* name = category == Lifecycle ? "lifecycle" : category == Interfaces ? "interfaces" : category == Hooks ? "hooks" : "auth";
 debugLog.Write(category, name, event, details);
}
}
