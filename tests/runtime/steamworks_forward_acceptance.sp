#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>
native bool SWForwardFixture_Run(int receiverOrder);
public Plugin myinfo = {name = "SteamWorks Forward Acceptance", author = "Lechuga", version = "1.0.0"};
public void OnPluginStart()
{
    RegAdminCmd("sm_sw_forward_check", CheckForwards, ADMFLAG_ROOT);
    PrintToServer("[SW-FWD] controller_ready");
}
public Action CheckForwards(int client, int args)
{
    if (client != 0 || args != 1) return Plugin_Handled;
    char order[8];
    GetCmdArg(1, order, sizeof(order));
    bool passed = SWForwardFixture_Run(StringToInt(order));
    PrintToServer("[SW-FWD] run_complete pass=%d", passed);
    return Plugin_Handled;
}
