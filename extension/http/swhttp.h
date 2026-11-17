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
#include <unordered_set>
#include "isteamgameserver.h"
#include "steam_gameserver.h"
#include "smsdk_ext.h"

class SteamWorksHTTP :
	public IHandleTypeDispatch, public IPluginsListener
{
	public:
		SteamWorksHTTP();
		~SteamWorksHTTP();

	public:
		void OnHandleDestroy(HandleType_t type, void *object) override;
		bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize) override;
		
	public:
		HandleType_t GetHTTPHandle() const;
        void Track(class SteamWorksHTTPRequest* request);
        void Untrack(class SteamWorksHTTPRequest* request);
        void StopRuntime();
        void StartRuntime();
        bool IsAccepting() const { return m_accepting && !m_destroying && typeHTTP != 0; }
        uint64 Generation() const { return m_generation; }
        void OnPluginWillUnload(IPlugin* plugin) override;
        SteamWorksHTTP(const SteamWorksHTTP&) = delete;
        SteamWorksHTTP& operator=(const SteamWorksHTTP&) = delete;

	private:
		HandleType_t typeHTTP = 0;
        bool m_accepting = true;
        bool m_destroying = false;
        uint64 m_generation = 1;
        std::unordered_set<class SteamWorksHTTPRequest*> m_requests;
};

#include "swhttprequest.h"
#include "../extension.h"
