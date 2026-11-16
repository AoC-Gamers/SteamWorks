#pragma semicolon 1
#pragma newdecls required

// Synthetic fixture only. Never receives, prints or stores a real Steam ticket.
native void InspectAuthBytes(const char[] ticket, int length, int authid);

public void SteamWorks_BeginAuthSession(char[] ticket, int length, int authid)
{
	InspectAuthBytes(ticket, length, authid);
	if (length > 0)
	{
		ticket[0] = 0x55;
	}
}
