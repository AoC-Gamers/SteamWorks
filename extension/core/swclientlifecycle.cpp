#include "swclientlifecycle.h"
#include "../extension.h"
#include "swdebug.h"

void SteamWorksClientLifecycle::FinalizeStoppedHost()
{
 // SDK_OnUnload runs after plugin_end and before this extension is unmapped.
 // Never hook VSP::Unload: its original unloads SourceHook and this DSO while
 // the generated dispatcher is still on the stack.
 if (!m_serverStopped || SteamGameServer_GetHSteamPipe() != 0) { return; }
 m_serverStopped = false;
 const bool active = SteamAPI_GetHSteamPipe() != 0;
 if (active) { SteamAPI_Shutdown(); }
 steamworks::debug::Log(steamworks::debug::Lifecycle, "client_host_shutdown",
  "active_before=%d active_after=%d server_active=%d", active,
  SteamAPI_GetHSteamPipe() != 0, SteamGameServer_GetHSteamPipe() != 0);
}
