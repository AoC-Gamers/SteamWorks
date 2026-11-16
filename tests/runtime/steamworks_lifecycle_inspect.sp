#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>

public Plugin myinfo =
{
 name = "SteamWorks Lifecycle Inspection",
 author = "Lechuga",
 description = "Read-only entrypoint inspection in the isolated Linux x86 lab",
 version = "1.0.0"
};

public void OnPluginStart()
{
 RegAdminCmd("sm_sw_lifecycle_inspect", InspectLifecycle, ADMFLAG_ROOT);
}

public Action InspectLifecycle(int client, int args)
{
 // Console-only laboratory input: the host verifies mapped file identity and
 // executable ranges before passing the two exact profiled SDK addresses.
 if (client != 0 || args < 2 || args > 3) return Plugin_Handled;
 char argument[16];
 GetCmdArg(1, argument, sizeof(argument));
 InspectEntry("SteamInit", view_as<Address>(StringToInt(argument, 16)));
 GetCmdArg(2, argument, sizeof(argument));
 InspectEntry("SteamShutdown", view_as<Address>(StringToInt(argument, 16)));
 if (args == 3)
 {
  GetCmdArg(3, argument, sizeof(argument));
  InspectEntry("EngineInitCall", view_as<Address>(StringToInt(argument, 16)));
 }
 return Plugin_Handled;
}

void InspectEntry(const char[] name, Address entry)
{
 if (entry == Address_Null) { PrintToServer("[SW-LIFETIME] unresolved=%s", name); return; }
 int opcode = LoadFromAddress(entry, NumberType_Int8) & 255;
 bool relative = opcode == 0xE9 || opcode == 0xE8;
 int displacement = relative ? LoadFromAddress(entry + view_as<Address>(1), NumberType_Int32) : 0;
 int target = relative ? view_as<int>(entry) + 5 + displacement : 0;
 if (opcode == 0xFF && (LoadFromAddress(entry + view_as<Address>(1), NumberType_Int8) & 255) == 0x25)
 {
  Address slot = view_as<Address>(LoadFromAddress(entry + view_as<Address>(2), NumberType_Int32));
  target = LoadFromAddress(slot, NumberType_Int32);
 }
 PrintToServer("[SW-LIFETIME] entry=%s address=%08x opcode=%02x target=%08x", name, entry, opcode, target);
 if (target != 0)
 {
  char bytes[193];
  for (int i = 0; i < 64; i++) Format(bytes, sizeof(bytes), "%s%02x", bytes, LoadFromAddress(view_as<Address>(target + i), NumberType_Int8) & 255);
  PrintToServer("[SW-LIFETIME] code=%s target=%08x bytes=%s", name, target, bytes);
 }
}
