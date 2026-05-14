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
#include "swgamedata.h"

SteamWorksGameData::SteamWorksGameData()
{
	char buffer[PLATFORM_MAX_PATH];
	
	smutils->BuildPath(Path_SM, buffer, sizeof(buffer), "gamedata/steamworks.txt");
	if (libsys->PathExists(buffer) && libsys->IsPathFile(buffer))
	{
		gameconfs->LoadGameConfigFile("steamworks", &pGameConf, buffer, sizeof(buffer));
	}
}

SteamWorksGameData::~SteamWorksGameData()
{
	if (pGameConf != nullptr)
	{
		gameconfs->CloseGameConfigFile(pGameConf);
		pGameConf = nullptr;
	}
}

bool SteamWorksGameData::HasGameData(void) const
{
	return pGameConf != nullptr;
}

IGameConfig *SteamWorksGameData::GetGameData(void) const
{
	return pGameConf;
}
