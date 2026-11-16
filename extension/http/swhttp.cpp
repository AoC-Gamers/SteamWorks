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

SteamWorksHTTP::SteamWorksHTTP()
{
	plsys->AddPluginsListener(this);
	typeHTTP = handlesys->CreateType("HTTPHandle", this, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);
}

SteamWorksHTTP::~SteamWorksHTTP()
{
    m_destroying = true;
    StopRuntime();
    plsys->RemovePluginsListener(this);
    if (typeHTTP != 0) { handlesys->RemoveType(typeHTTP, myself->GetIdentity()); typeHTTP = 0; }
    // RemoveType normally closes every handle. Also detach any construction/dispatch survivor.
    while (!m_requests.empty()) {
        auto* request = *m_requests.begin();
        m_requests.erase(request);
        request->DetachManager();
        request->Close();
    }
}

HandleType_t SteamWorksHTTP::GetHTTPHandle() const
{
	return typeHTTP;
}

void SteamWorksHTTP::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == typeHTTP) /* Heaven forbid we ever offer another handle. */
	{
		static_cast<SteamWorksHTTPRequest*>(object)->Close();
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

void SteamWorksHTTP::Track(SteamWorksHTTPRequest* request) { m_requests.insert(request); }
void SteamWorksHTTP::Untrack(SteamWorksHTTPRequest* request) { m_requests.erase(request); }

void SteamWorksHTTP::StopRuntime()
{
 if (!m_accepting) { return; }
 m_accepting = false;
 ++m_generation;
 auto requests = m_requests;
 for (auto* request : requests) {
  if (m_requests.count(request) != 0) { request->Invalidate(); }
 }
}

void SteamWorksHTTP::StartRuntime()
{
 if (!m_destroying) { m_accepting = true; }
}

void SteamWorksHTTP::OnPluginWillUnload(IPlugin* plugin)
{
 auto requests = m_requests;
 for (auto* request : requests) {
  if (m_requests.count(request) != 0) { request->PluginUnloading(plugin); }
 }
}
