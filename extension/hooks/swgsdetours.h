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

#include "smsdk_ext.h"
#include "../CDetour/detours.h"
#include "steam_gameserver.h"
#include "../core/swinitcallsite.h"

class SteamWorksGSDetours
{
public:
	SteamWorksGSDetours();
	~SteamWorksGSDetours();
	void Shutdown();
	bool IsInstalled() const { return m_initCall.IsInstalled() && m_pShutdownDetour != nullptr; }
	SteamWorksGSDetours(const SteamWorksGSDetours&) = delete;
	SteamWorksGSDetours& operator=(const SteamWorksGSDetours&) = delete;

private:
	ILibrary* m_library = nullptr;
	steamworks::InitCallSite m_initCall;
	CDetour *m_pShutdownDetour = nullptr;
};

#include "../extension.h"
