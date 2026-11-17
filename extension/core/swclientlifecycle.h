#pragma once

// The verified L4D2 GameDLL opens a Steam application client in DLLInit.
// Retire that role only after an observed GS shutdown and extension teardown.
// A live hot unload or a map change must never close the host session.
class SteamWorksClientLifecycle
{
public:
 void GameServerStopped() { m_serverStopped = true; }
 void GameServerStarted() { m_serverStopped = false; }
 void FinalizeStoppedHost();
private:
 bool m_serverStopped = false;
};
