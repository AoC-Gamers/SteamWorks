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

#include "swhttp.h"

namespace
{
	void DelayedDeleteSteamWorksHTTPRequest(void *object)
	{
		SteamWorksHTTPRequest *pRequest = reinterpret_cast<SteamWorksHTTPRequest *>(object);
		delete pRequest;
	}
}

SteamWorksHTTP::SteamWorksHTTP()
{
	typeHTTP = handlesys->CreateType("HTTPHandle", this, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);
}

SteamWorksHTTP::~SteamWorksHTTP()
{
	handlesys->RemoveType(typeHTTP, myself->GetIdentity());
}

HandleType_t SteamWorksHTTP::GetHTTPHandle() const
{
	return typeHTTP;
}

void SteamWorksHTTP::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == typeHTTP) /* Heaven forbid we ever offer another handle. */
	{
		smutils->AddFrameAction(DelayedDeleteSteamWorksHTTPRequest, object);
	}
}

bool SteamWorksHTTP::GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
{
	(void)object;

	if (type == typeHTTP && pSize) /* Heaven forbid we ever offer another handle. */
	{
		*pSize = sizeof(SteamWorksHTTPRequest);
		return true;
	}

	return false;
}
