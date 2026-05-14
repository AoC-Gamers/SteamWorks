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

#include "gcnatives.h"

namespace {

ISteamGameCoordinator *GetSteamGCPointer(void)
{
	return g_SteamWorks.pSWGameServer->GetGameCoordinator();
}

cell_t sm_GCSendMessage(IPluginContext *pContext, const cell_t *params)
{
	ISteamGameCoordinator *pGC = GetSteamGCPointer();
	if (pGC == nullptr)
	{
		return static_cast<cell_t>(k_EGCResultNotLoggedOn);
	}

	char *pData;
	pContext->LocalToString(params[2], &pData);

	return static_cast<cell_t>(pGC->SendMessage(params[1], pData, params[3]));
}

static sp_nativeinfo_t gcnatives[] = {
	{"SteamWorks_SendMessageToGC",				sm_GCSendMessage},
	{nullptr,											nullptr}
};

}  // namespace

SteamWorksGCNatives::SteamWorksGCNatives()
{
	sharesys->AddNatives(myself, gcnatives);
}

SteamWorksGCNatives::~SteamWorksGCNatives()
{
}
