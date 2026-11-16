/*
    This file is part of SourcePawn SteamWorks.

    SourcePawn SteamWorks is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, as per version 3 of the License.

    SourcePawn SteamWorks is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourcePawn SteamWorks.  If not, see <http://www.gnu.org/licenses/>.
	
	Author: Kyle Sanderson (KyleS).
*/

#pragma once
#include "steam_gameserver.h"
#include "isteamgamecoordinator.h"

#include "smsdk_ext.h"
#include "sourcehook.h"

#ifdef _WIN32
#undef SendMessage
#endif

class SteamWorksGCHooks
{
	public:
		SteamWorksGCHooks();
		~SteamWorksGCHooks();

	public:
		void AddHooks(ISteamGameCoordinator *pGC);
		void Stop();
	void Start();
	void TryAttach();
	SteamWorksGCHooks(const SteamWorksGCHooks&) = delete;
	SteamWorksGCHooks& operator=(const SteamWorksGCHooks&) = delete;

	public:
		EGCResults SendMessage(uint32 unMsgType, const void *pubData, uint32 cubData);
		bool IsMessageAvailable(uint32_t *pcubMsgSize);
		EGCResults RetrieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize);

	private:
		IForward *pGCSendMessageForward = nullptr;
		IForward *pGCMessageAvailableForward = nullptr;
		IForward *pGCRetrieveMessageForward = nullptr;
		ISteamGameCoordinator* m_target = nullptr;
	int m_hookIds[3] = {};
	bool m_frameHook = false;
	bool m_enabled = false;
	uint64 m_generation = 0;
	void Detach();
	void SetFrameHook(bool enabled);
};

void OurGCGameFrameHook(bool simulating);

#include "../extension.h"
