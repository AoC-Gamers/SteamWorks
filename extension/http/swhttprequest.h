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

#include "isteamgameserver.h"
#include "smsdk_ext.h"
#include "steam_gameserver.h"

class SteamWorksHTTPRequest
{
public:
	SteamWorksHTTPRequest(ISteamHTTP* owner = nullptr, IdentityToken_t* pluginOwner = nullptr);
	~SteamWorksHTTPRequest();

	void OnHTTPRequestCompleted(HTTPRequestCompleted_t *pRequest, bool bFailed);
	void OnHTTPHeadersReceived(HTTPRequestHeadersReceived_t *pRequest, bool bFailed);
	void OnHTTPDataReceived(HTTPRequestDataReceived_t *pRequest, bool bFailed);

    bool IsUsable() const;
    bool SetContextValue(uint64 context);
    void Invalidate();
    void Close();
    void PluginUnloading(IPlugin* plugin);
    void DetachManager() { m_manager = nullptr; }
    SteamWorksHTTPRequest(const SteamWorksHTTPRequest&) = delete;
    SteamWorksHTTPRequest& operator=(const SteamWorksHTTPRequest&) = delete;
    class DispatchScope {
    public:
        explicit DispatchScope(SteamWorksHTTPRequest* request) : m_request(request) { ++m_request->m_dispatchDepth; }
        ~DispatchScope() { m_request->FinishDispatch(); }
        DispatchScope(const DispatchScope&) = delete;
        DispatchScope& operator=(const DispatchScope&) = delete;
    private:
        SteamWorksHTTPRequest* m_request;
    };

	void OnHTTPHeadersCallback(HTTPRequestHeadersReceived_t* response);
	void OnHTTPDataCallback(HTTPRequestDataReceived_t* response);

	HTTPRequestHandle request;
	Handle_t handle;
	CCallResult<SteamWorksHTTPRequest, HTTPRequestCompleted_t> CompletedCallResult;
	CCallbackManual<SteamWorksHTTPRequest, HTTPRequestHeadersReceived_t, true> HeadersCallback;
	CCallbackManual<SteamWorksHTTPRequest, HTTPRequestDataReceived_t, true> DataCallback;
	IChangeableForward *pCompletedForward;
	IChangeableForward *pHeadersReceivedForward;
	IChangeableForward *pDataReceivedForward;
private:
    void FinishDispatch();
    void ReleaseForwards();
    class SteamWorksHTTP* m_manager = nullptr;
    ISteamHTTP* m_owner = nullptr; // Borrowed instance that created request; never reacquire on release.
    // The x86 Steam HTTP provider truncates the high word in streaming events.
    // Commit the complete context only after the creating interface accepts it.
    uint64 m_contextValue = 0;
    IdentityToken_t* m_pluginOwner = nullptr;
    uint64 m_generation = 0;
    unsigned int m_dispatchDepth = 0;
    bool m_active = true;
    bool m_deleteRequested = false;
};

class SteamWorksHTTPNatives
{
public:
	SteamWorksHTTPNatives();
	~SteamWorksHTTPNatives();
};

#include "../extension.h"
