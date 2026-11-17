#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
#include <sdktools>

public Plugin myinfo = {name = "SteamWorks Extension Observer", author = "Lechuga", version = "1.0.0"};

enum struct PluginRuntimeState
{
 bool lateLoad;
 bool hasSteamWorks;
 int added;
 int removed;
 Handle clientPipeCall;
 Handle serverPipeCall;
 Address engineCall;
 Address shutdownEntry;
 Address initEntry;
 void Reset()
 {
  this.lateLoad = false;
  this.hasSteamWorks = false;
  this.added = 0;
  this.removed = 0;
  this.clientPipeCall = null;
  this.serverPipeCall = null;
  this.engineCall = Address_Null;
  this.shutdownEntry = Address_Null;
  this.initEntry = Address_Null;
 }
}
PluginRuntimeState g_Runtime;

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int maxlen)
{
 g_Runtime.Reset();
 g_Runtime.lateLoad = late;
 return APLRes_Success;
}
public void OnPluginStart()
{
 RegAdminCmd("sm_sw_extension_observe", ObserveExtension, ADMFLAG_ROOT);
 PrintToServer("[SW-EXT] observer_ready");
}
public void OnAllPluginsLoaded() { g_Runtime.hasSteamWorks = LibraryExists("SteamWorks"); }
public void OnLibraryAdded(const char[] name)
{
 if (StrEqual(name, "SteamWorks")) { g_Runtime.hasSteamWorks = true; g_Runtime.added++; }
}
public void OnLibraryRemoved(const char[] name)
{
 if (StrEqual(name, "SteamWorks")) { g_Runtime.hasSteamWorks = false; g_Runtime.removed++; }
}
public void OnPluginEnd()
{
 delete g_Runtime.clientPipeCall;
 delete g_Runtime.serverPipeCall;
}
Handle PipeCall(Address address)
{
 StartPrepSDKCall(SDKCall_Static);
 if (!PrepSDKCall_SetAddress(address)) SetFailState("SDK pipe export unavailable");
 PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
 Handle call = EndPrepSDKCall();
 if (call == null) SetFailState("SDK pipe call unavailable");
 return call;
}
Address ArgumentAddress(int argument)
{
 char text[16]; GetCmdArg(argument, text, sizeof(text));
 return view_as<Address>(StringToInt(text, 16));
}
void Entry(const char[] name, Address address, int count)
{
 char bytes[65];
 for (int i = 0; i < count; i++) Format(bytes, sizeof(bytes), "%s%02x", bytes, LoadFromAddress(address + view_as<Address>(i), NumberType_Int8) & 255);
 PrintToServer("[SW-EXT] code=%s bytes=%s", name, bytes);
}
public Action ObserveExtension(int client, int args)
{
 if (client != 0) return Plugin_Handled;
 if (args == 5)
 {
  // Host validates all five addresses against the exact mapped SDK/engine
  // executable regions before supplying them. No signatures or guessing here.
  delete g_Runtime.clientPipeCall;
  delete g_Runtime.serverPipeCall;
  g_Runtime.clientPipeCall = PipeCall(ArgumentAddress(1));
  g_Runtime.serverPipeCall = PipeCall(ArgumentAddress(2));
  g_Runtime.engineCall = ArgumentAddress(3);
  g_Runtime.shutdownEntry = ArgumentAddress(4);
  g_Runtime.initEntry = ArgumentAddress(5);
 }
 PrintToServer("[SW-EXT] library present=%d added=%d removed=%d late=%d convar=%d", g_Runtime.hasSteamWorks, g_Runtime.added, g_Runtime.removed, g_Runtime.lateLoad, FindConVar("steamworks_debug") != null);
 if (g_Runtime.clientPipeCall == null || g_Runtime.serverPipeCall == null) return Plugin_Handled;
 PrintToServer("[SW-EXT] pipes client=%d server=%d", SDKCall(g_Runtime.clientPipeCall), SDKCall(g_Runtime.serverPipeCall));
 Entry("engine", g_Runtime.engineCall, 5);
 Entry("shutdown", g_Runtime.shutdownEntry, 20);
 Entry("init", g_Runtime.initEntry, 20);
 return Plugin_Handled;
}
