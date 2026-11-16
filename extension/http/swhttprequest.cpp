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

#include "swhttprequest.h"
#include "../core/swpawn.h"
#include <cerrno>
#include <cstdio>

namespace {

void ReleaseForward(IChangeableForward*& forward)
{
	if (forward == nullptr) {
		return;
	}

	forwards->ReleaseForward(forward);
	forward = nullptr;
}

uint64_t PackContextValue(cell_t high, cell_t low)
{
	return steamworks::PackContext(steamworks::pawn::Bits(high), steamworks::pawn::Bits(low));
}

void PushContextValue(IChangeableForward* forward, uint64_t contextValue)
{
	forward->PushCell(steamworks::pawn::Cell(static_cast<uint32>(contextValue >> 32)));
	forward->PushCell(steamworks::pawn::Cell(static_cast<uint32>(contextValue)));
}

ISteamHTTP *GetHTTPPointer(void)
{
	return g_SteamWorks.pSWHTTP != nullptr && g_SteamWorks.pSWHTTP->IsAccepting()
		&& g_SteamWorks.pSWGameServer != nullptr ? g_SteamWorks.pSWGameServer->GetHTTP() : nullptr;
}

static HandleType_t GetSteamHTTPHandle(void)
{
	return g_SteamWorks.pSWHTTP->GetHTTPHandle();
}

static SteamWorksHTTPRequest *GetRequestPointer(ISteamHTTP *&pHTTP, IPluginContext *pContext, cell_t Handle)
{
	pHTTP = GetHTTPPointer();
	if (pHTTP == nullptr)
	{
		return nullptr;
	}

	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	SteamWorksHTTPRequest *pRequest;
	if ((err = handlesys->ReadHandle(Handle, GetSteamHTTPHandle(), &sec, (void **)&pRequest))
		!= HandleError_None)
	{
		pContext->ThrowNativeError("Invalid Handle %x (error: %d)", Handle, err);
		return nullptr;
	}

	if (!pRequest->IsUsable()) {
		pContext->ThrowNativeError("HTTP request belongs to a stopped runtime or closing owner");
		return nullptr;
	}
	return pRequest;
}

}  // namespace

SteamWorksHTTPRequest::SteamWorksHTTPRequest(ISteamHTTP* owner, IdentityToken_t* pluginOwner)
	: request(INVALID_HTTPREQUEST_HANDLE),
	  handle(BAD_HANDLE),
	  pCompletedForward(nullptr),
	  pHeadersReceivedForward(nullptr),
	  pDataReceivedForward(nullptr)
{
 m_manager = g_SteamWorks.pSWHTTP;
 m_owner = owner != nullptr ? owner : GetHTTPPointer();
 m_pluginOwner = pluginOwner;
 if (m_manager != nullptr) { m_generation = m_manager->Generation(); m_manager->Track(this); }
}

SteamWorksHTTPRequest::~SteamWorksHTTPRequest()
{
 Invalidate();
 ReleaseForwards();
 if (m_manager != nullptr) { m_manager->Untrack(this); }
}

bool SteamWorksHTTPRequest::IsUsable() const
{
 return m_active && m_manager != nullptr && m_manager->IsAccepting()
     && m_generation == m_manager->Generation();
}

bool SteamWorksHTTPRequest::SetContextValue(uint64 context)
{
 if (!IsUsable() || m_owner == nullptr || !m_owner->SetHTTPRequestContextValue(request, context)) {
  return false;
 }
 m_contextValue = context;
 return true;
}

void SteamWorksHTTPRequest::ReleaseForwards()
{
 ReleaseForward(pCompletedForward);
 ReleaseForward(pHeadersReceivedForward);
 ReleaseForward(pDataReceivedForward);
}

void SteamWorksHTTPRequest::Invalidate()
{
 if (!m_active) { return; }
 m_active = false;
 CompletedCallResult.Cancel();
 HeadersCallback.Unregister();
 DataCallback.Unregister();
 if (m_owner != nullptr && request != INVALID_HTTPREQUEST_HANDLE) {
  m_owner->ReleaseHTTPRequest(request);
 }
 request = INVALID_HTTPREQUEST_HANDLE;
 m_owner = nullptr;
 if (m_dispatchDepth == 0) { ReleaseForwards(); }
}

void SteamWorksHTTPRequest::Close()
{
 m_deleteRequested = true;
 Invalidate();
 if (m_dispatchDepth == 0) { delete this; }
}

void SteamWorksHTTPRequest::FinishDispatch()
{
 --m_dispatchDepth;
 if (m_dispatchDepth != 0) { return; }
 if (!m_active) { ReleaseForwards(); }
 if (m_deleteRequested) { delete this; }
}

void SteamWorksHTTPRequest::PluginUnloading(IPlugin* plugin)
{
 if (m_pluginOwner == plugin->GetIdentity()) { Invalidate(); }
 if (pCompletedForward != nullptr) { pCompletedForward->RemoveFunctionsOfPlugin(plugin); }
 if (pHeadersReceivedForward != nullptr) { pHeadersReceivedForward->RemoveFunctionsOfPlugin(plugin); }
 if (pDataReceivedForward != nullptr) { pDataReceivedForward->RemoveFunctionsOfPlugin(plugin); }
}

void SteamWorksHTTPRequest::OnHTTPHeadersCallback(HTTPRequestHeadersReceived_t* response)
{
 OnHTTPHeadersReceived(response, false);
}

void SteamWorksHTTPRequest::OnHTTPDataCallback(HTTPRequestDataReceived_t* response)
{
 OnHTTPDataReceived(response, false);
}

void SteamWorksHTTPRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t *pRequest, bool bFailed)
{
	if (!IsUsable() || pRequest == nullptr || pRequest->m_hRequest != request) { return; }
	DispatchScope dispatch(this);
	if (pCompletedForward == nullptr || pCompletedForward->GetFunctionCount() == 0)
	{
		return;
	}

	pCompletedForward->PushCell(handle);
	pCompletedForward->PushCell(bFailed);
	pCompletedForward->PushCell(pRequest->m_bRequestSuccessful);
	pCompletedForward->PushCell(pRequest->m_eStatusCode);
	PushContextValue(pCompletedForward, pRequest->m_ulContextValue);
	pCompletedForward->Execute(nullptr);
}

void SteamWorksHTTPRequest::OnHTTPHeadersReceived(HTTPRequestHeadersReceived_t *pRequest, bool bFailed)
{
	if (!IsUsable() || pRequest == nullptr || pRequest->m_hRequest != request) { return; }
	DispatchScope dispatch(this);
	if (pHeadersReceivedForward == nullptr || pHeadersReceivedForward->GetFunctionCount() == 0)
	{
		return;
	}

	pHeadersReceivedForward->PushCell(handle);
	pHeadersReceivedForward->PushCell(bFailed);
	PushContextValue(pHeadersReceivedForward, m_contextValue);
	pHeadersReceivedForward->Execute(nullptr);
}

void SteamWorksHTTPRequest::OnHTTPDataReceived(HTTPRequestDataReceived_t *pRequest, bool bFailed)
{
	if (!IsUsable() || pRequest == nullptr || pRequest->m_hRequest != request) { return; }
	DispatchScope dispatch(this);
	if (pDataReceivedForward == nullptr || pDataReceivedForward->GetFunctionCount() == 0)
	{
		return;
	}

	pDataReceivedForward->PushCell(handle);
	pDataReceivedForward->PushCell(bFailed);
	pDataReceivedForward->PushCell(pRequest->m_cOffset);
	pDataReceivedForward->PushCell(pRequest->m_cBytesReceived);
	PushContextValue(pDataReceivedForward, m_contextValue);
	pDataReceivedForward->Execute(nullptr);
}

static cell_t sm_CreateHTTPRequest(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP = GetHTTPPointer();
	if (pHTTP == NULL)
	{
		return BAD_HANDLE;
	}

	if (params[1] <= k_EHTTPMethodInvalid || params[1] > k_EHTTPMethodPATCH) {
		return pContext->ThrowNativeError("Unsupported HTTP method");
	}

	char *pURL;
	if (!steamworks::pawn::String(pContext, params[2], pURL)) { return 0; }

	HTTPRequestHandle request = pHTTP->CreateHTTPRequest(static_cast<EHTTPMethod>(params[1]), pURL);
	if (request == INVALID_HTTPREQUEST_HANDLE)
	{
		return BAD_HANDLE;
	}

	SteamWorksHTTPRequest *pRequest = new SteamWorksHTTPRequest(pHTTP, pContext->GetIdentity());
	pRequest->request = request;
	Handle_t handle = handlesys->CreateHandle(GetSteamHTTPHandle(), pRequest, pContext->GetIdentity(), myself->GetIdentity(), NULL);
	if (handle == BAD_HANDLE)
	{
		delete pRequest;
		return BAD_HANDLE;
	}

	pRequest->request = request;
	pRequest->handle = handle;
	
	return handle;
}

static cell_t sm_SetHTTPRequestContextValue(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	return pRequest->SetContextValue(PackContextValue(params[2], params[3])) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestNetworkActivityTimeout(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	uint32 size;
	if (!steamworks::pawn::SteamSize(pContext, params[2], size)) { return 0; }

	return pHTTP->SetHTTPRequestNetworkActivityTimeout(pRequest->request, size) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestHeaderValue(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pName, *pValue;
	if (!steamworks::pawn::String(pContext, params[2], pName)) { return 0; }
	if (!steamworks::pawn::String(pContext, params[3], pValue)) { return 0; }
	return pHTTP->SetHTTPRequestHeaderValue(pRequest->request, pName, pValue) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestGetOrPostParameter(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pName, *pValue;
	if (!steamworks::pawn::String(pContext, params[2], pName)) { return 0; }
	if (!steamworks::pawn::String(pContext, params[3], pValue)) { return 0; }
	return pHTTP->SetHTTPRequestGetOrPostParameter(pRequest->request, pName, pValue) ? 1 : 0;
}

static cell_t sm_SetCallbacks(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	IPlugin *pPlugin;
	if (params[5] == BAD_HANDLE)
	{
		pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	} else {
		HandleError err;
		pPlugin = plsys->PluginFromHandle(params[5], &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", params[5], err);
		}
	}

 if (pPlugin == nullptr) { return pContext->ThrowNativeError("HTTP callback plugin unavailable"); }
 IPluginFunction* functions[3] = {};
 IChangeableForward* created[3] = {};
 IChangeableForward** slots[3] = {&pRequest->pCompletedForward, &pRequest->pHeadersReceivedForward, &pRequest->pDataReceivedForward};
 // Validate the complete request before allocating or changing existing bindings.
 for (size_t i = 0; i < 3; ++i) {
  if (params[i + 2] > 0) {
   functions[i] = pPlugin->GetBaseContext()->GetFunctionById(params[i + 2]);
   if (functions[i] == nullptr) { return pContext->ThrowNativeError("Invalid function id (%X)", params[i + 2]); }
  }
 }
 for (size_t i = 0; i < 3; ++i) {
  if (functions[i] == nullptr || *slots[i] != nullptr) { continue; }
  if (i == 1) {
   created[i] = forwards->CreateForwardEx(nullptr, ET_Ignore, 4, nullptr, Param_Cell, Param_Cell, Param_Cell, Param_Cell);
  } else {
   created[i] = forwards->CreateForwardEx(nullptr, ET_Ignore, 6, nullptr, Param_Cell, Param_Cell, Param_Cell, Param_Cell, Param_Cell, Param_Cell);
  }
  if (created[i] == nullptr) {
   for (auto*& forward : created) { ReleaseForward(forward); }
   return pContext->ThrowNativeError("Could not allocate HTTP callback forward");
  }
 }
 for (size_t i = 0; i < 3; ++i) {
  if (created[i] != nullptr) { *slots[i] = created[i]; }
  if (functions[i] != nullptr) { (*slots[i])->AddFunction(functions[i]); }
 }

	return 1;
}

static void SetCallbacks(SteamAPICall_t &hCall, SteamWorksHTTPRequest *pRequest)
{
	if (pRequest->pCompletedForward != NULL)
	{
		pRequest->CompletedCallResult.SetGameserverFlag();
		pRequest->CompletedCallResult.Set(hCall, pRequest, &SteamWorksHTTPRequest::OnHTTPRequestCompleted);
	}

	if (pRequest->pHeadersReceivedForward != NULL)
	{
		pRequest->HeadersCallback.Register(pRequest, &SteamWorksHTTPRequest::OnHTTPHeadersCallback);
	}

	if (pRequest->pDataReceivedForward != NULL)
	{
		pRequest->DataCallback.Register(pRequest, &SteamWorksHTTPRequest::OnHTTPDataCallback);
	}
}

static cell_t sm_SendHTTPRequestAndStreamResponse(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	SteamAPICall_t hCall = k_uAPICallInvalid;
	cell_t result = pHTTP->SendHTTPRequestAndStreamResponse(pRequest->request, &hCall) ? 1 : 0;

	if (result)
	{
		SetCallbacks(hCall, pRequest);
	}

	return result;
}

static cell_t sm_SendHTTPRequest(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	SteamAPICall_t hCall = k_uAPICallInvalid;
	cell_t result = pHTTP->SendHTTPRequest(pRequest->request, &hCall) ? 1 : 0;

	if (result)
	{
		SetCallbacks(hCall, pRequest);
	}

	return result;
}

static cell_t sm_DeferHTTPRequest(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	return pHTTP->DeferHTTPRequest(pRequest->request) ? 1 : 0;
}

static cell_t sm_PrioritizeHTTPRequest(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	return pHTTP->PrioritizeHTTPRequest(pRequest->request) ? 1 : 0;
}

static cell_t sm_GetHTTPResponseHeaderSize(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pName;
	if (!steamworks::pawn::String(pContext, params[2], pName)) { return 0; }
	
	cell_t *pSize;
	if (!steamworks::pawn::Output(pContext, params[3], pSize)) { return 0; }
	uint32 size = 0;
	if (!pHTTP->GetHTTPResponseHeaderSize(pRequest->request, pName, &size)) { return 0; }
	return steamworks::pawn::StoreSize(pContext, pSize, size) ? 1 : 0;
}

static cell_t sm_GetHTTPResponseHeaderValue(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pName;
	if (!steamworks::pawn::String(pContext, params[2], pName)) { return 0; }
	
	char *pBuffer;
	if (!steamworks::pawn::String(pContext, params[3], pBuffer)) { return 0; }
	uint32 size;
	if (!steamworks::pawn::SteamSize(pContext, params[4], size)) { return 0; }

	return pHTTP->GetHTTPResponseHeaderValue(pRequest->request, pName, reinterpret_cast<uint8_t *>(pBuffer), size) ? 1 : 0; /* uint8? wtf? */
}

static cell_t sm_GetHTTPResponseBodySize(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	cell_t *pSize;
	if (!steamworks::pawn::Output(pContext, params[2], pSize)) { return 0; }
	uint32 size = 0;
	if (!pHTTP->GetHTTPResponseBodySize(pRequest->request, &size)) { return 0; }
	return steamworks::pawn::StoreSize(pContext, pSize, size) ? 1 : 0;
}

static cell_t sm_GetHTTPResponseBodyData(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pBuffer;
	if (!steamworks::pawn::String(pContext, params[2], pBuffer)) { return 0; }
	uint32 size;
	if (!steamworks::pawn::SteamSize(pContext, params[3], size)) { return 0; }

	return pHTTP->GetHTTPResponseBodyData(pRequest->request, reinterpret_cast<uint8_t *>(pBuffer), size) ? 1 : 0;
}

static cell_t sm_GetHTTPResponseBodyString(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	if (params[0] < 5) {
		return pContext->ThrowNativeError("GetHTTPResponseBodyString requires written and truncated output arguments");
	}

	if (params[3] <= 0)
	{
		return pContext->ThrowNativeError("Buffer length must be greater than 0");
	}

	cell_t *pWritten;
	if (!steamworks::pawn::Output(pContext, params[4], pWritten)) { return 0; }

	cell_t *pTruncated;
	if (!steamworks::pawn::Output(pContext, params[5], pTruncated)) { return 0; }

	char *pBuffer;
	if (!steamworks::pawn::String(pContext, params[2], pBuffer)) { return 0; }

	uint32_t size;
	if (!pHTTP->GetHTTPResponseBodySize(pRequest->request, &size))
	{
		return 0;
	}

	bool truncated = false;
	uint32_t readLength = size;
	if (readLength >= static_cast<uint32_t>(params[3]))
	{
		readLength = static_cast<uint32_t>(params[3] - 1);
		truncated = true;
	}

	if (readLength > 0 && !pHTTP->GetHTTPResponseBodyData(pRequest->request, reinterpret_cast<uint8_t *>(pBuffer), readLength))
	{
		return 0;
	}

	pBuffer[readLength] = '\0';
	*pWritten = static_cast<cell_t>(readLength);
	*pTruncated = truncated ? 1 : 0;
	return 1;
}

static cell_t sm_GetHTTPDownloadProgressPct(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP* pHTTP;
	SteamWorksHTTPRequest* request = GetRequestPointer(pHTTP, pContext, params[1]);
	if (request == nullptr) { return 0; }
	cell_t* output;
	if (!steamworks::pawn::Output(pContext, params[2], output)) { return 0; }
	float percent = 0.0f;
	if (!pHTTP->GetHTTPDownloadProgressPct(request->request, &percent)) { return 0; }
	steamworks::pawn::StoreFloat(output, percent);
	return 1;
}

static cell_t sm_SetHTTPRequestRawPostBody(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pName, *pBuffer;
	if (!steamworks::pawn::String(pContext, params[2], pName)) { return 0; }
	if (!steamworks::pawn::String(pContext, params[3], pBuffer)) { return 0; }

	uint32 size;
	if (!steamworks::pawn::SteamSize(pContext, params[4], size)) { return 0; }

	return pHTTP->SetHTTPRequestRawPostBody(pRequest->request, pName, reinterpret_cast<uint8_t *>(pBuffer), size) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestRawPostBodyFromFile(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pContentType, *pFilePath;
	if (!steamworks::pawn::String(pContext, params[2], pContentType)) { return 0; }
	if (!steamworks::pawn::String(pContext, params[3], pFilePath)) { return 0; }

	char szFinalPath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, szFinalPath, sizeof(szFinalPath), "%s", pFilePath);

	FILE *pInputFile = fopen(szFinalPath, "rb");
	if (!pInputFile)
	{
		return pContext->ThrowNativeError("Unable to open %s for reading. errno: %d", szFinalPath, errno);
	}

	long fileSize = -1;
	if (fseek(pInputFile, 0, SEEK_END) == 0) { fileSize = ftell(pInputFile); }
	uint32 size = 0;
	if (fileSize <= 0 || fseek(pInputFile, 0, SEEK_SET) != 0
		|| !steamworks::ToSteamSize(steamworks::ByteCount(static_cast<size_t>(fileSize)), size))
	{
		fclose(pInputFile);
		return 0;
	}

	char *pBuffer = new char[size];
	size_t itemsRead = fread(pBuffer, sizeof(char), size, pInputFile);
	fclose(pInputFile);

	if (itemsRead != size)
	{
		delete [] pBuffer;
		return 0;
	}

	cell_t result = pHTTP->SetHTTPRequestRawPostBody(pRequest->request, pContentType, reinterpret_cast<uint8_t *>(pBuffer), size) ? 1 : 0;

	delete [] pBuffer;
	return result;
}

static cell_t sm_GetHTTPResponseBodyCallback(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	SteamWorksHTTPRequest::DispatchScope dispatch(pRequest);

	IPlugin *pPlugin;
	if (params[4] == BAD_HANDLE)
	{
		pPlugin = plsys->FindPluginByContext(pContext->GetContext());
	} else {
		HandleError err;
		pPlugin = plsys->PluginFromHandle(params[4], &err);

		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", params[4], err);
		}
	}

	IPluginFunction *pFunction = pPlugin->GetBaseContext()->GetFunctionById(params[2]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[2]);
	}

	uint32_t size;
	if (pHTTP->GetHTTPResponseBodySize(pRequest->request, &size) == false)
	{
		return 0;
	}

	// The callback exposes length as one Pawn cell, including a terminator.
	if (size >= static_cast<uint32>((std::numeric_limits<cell_t>::max)())) {
		return pContext->ThrowNativeError("HTTP callback body exceeds SourcePawn length range");
	}
	const size_t capacity = static_cast<size_t>(size) + 1;
	char *pBuffer = new char[capacity];

	if (pHTTP->GetHTTPResponseBodyData(pRequest->request, reinterpret_cast<uint8_t *>(pBuffer), size) == false)
	{
		delete [] pBuffer;
		return 0;
	}

	pBuffer[size] = '\0'; /* Incase users do something bad; we want to protect userspace; kind of. */
	pFunction->PushStringEx(pBuffer, capacity, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
	pFunction->PushCell(params[3]);
	pFunction->PushCell(size);
	pFunction->Execute(NULL);

	delete [] pBuffer;
	return 1;
}

static cell_t sm_WriteHTTPResponseBodyToFile(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pFilePath;
	if (!steamworks::pawn::String(pContext, params[2], pFilePath)) { return 0; }

	uint32_t size;
	if (pHTTP->GetHTTPResponseBodySize(pRequest->request, &size) == false)
	{
		return 0;
	}

	char *pBuffer = new char[size];

	if (pHTTP->GetHTTPResponseBodyData(pRequest->request, reinterpret_cast<uint8_t *>(pBuffer), size) == false)
	{
		delete [] pBuffer;
		return 0;
	}

	char szFinalPath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, szFinalPath, sizeof(szFinalPath), "%s", pFilePath);

	FILE *pOutputFile = fopen(szFinalPath, "wb");

	if (!pOutputFile)
	{
		delete [] pBuffer;
		return pContext->ThrowNativeError("Unable to open %s for writing. errno: %d", szFinalPath, errno);
	}

	fwrite(pBuffer, sizeof(char), size, pOutputFile);

	delete [] pBuffer;
	fclose(pOutputFile);

	return 1;
}

static cell_t sm_GetHTTPStreamingResponseBodyData(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	uint32 offset, size;
	if (!steamworks::pawn::SteamSize(pContext, params[2], offset)
		|| !steamworks::pawn::SteamSize(pContext, params[4], size)) { return 0; }

	char *pBuffer;
	if (!steamworks::pawn::String(pContext, params[3], pBuffer)) { return 0; }
	return pHTTP->GetHTTPStreamingResponseBodyData(pRequest->request, offset, reinterpret_cast<uint8_t *>(pBuffer), size) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestUserAgentInfo(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	char *pName;
	if (!steamworks::pawn::String(pContext, params[2], pName)) { return 0; }

	return pHTTP->SetHTTPRequestUserAgentInfo(pRequest->request, pName) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestRequiresVerifiedCertificate(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	return pHTTP->SetHTTPRequestRequiresVerifiedCertificate(pRequest->request, !!params[2]) ? 1 : 0;
}

static cell_t sm_SetHTTPRequestAbsoluteTimeoutMS(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	uint32 size;
	if (!steamworks::pawn::SteamSize(pContext, params[2], size)) { return 0; }

	return pHTTP->SetHTTPRequestAbsoluteTimeoutMS(pRequest->request, size) ? 1 : 0;
}

static cell_t sm_GetHTTPRequestWasTimedOut(IPluginContext *pContext, const cell_t *params)
{
	ISteamHTTP *pHTTP;
	SteamWorksHTTPRequest *pRequest = GetRequestPointer(pHTTP, pContext, params[1]);
	if (pRequest == NULL)
	{
		return 0;
	}

	cell_t *pData;
	if (!steamworks::pawn::Output(pContext, params[2], pData)) { return 0; }

	bool timedOut = false;
	if (!pHTTP->GetHTTPRequestWasTimedOut(pRequest->request, &timedOut)) { return 0; }
	*pData = timedOut ? 1 : 0;
	return 1;
}

static sp_nativeinfo_t httpnatives[] = {
	{"SteamWorks_CreateHTTPRequest",				sm_CreateHTTPRequest},
	{"SteamWorks_SetHTTPRequestContextValue",				sm_SetHTTPRequestContextValue},
	{"SteamWorks_SetHTTPRequestNetworkActivityTimeout",				sm_SetHTTPRequestNetworkActivityTimeout},
	{"SteamWorks_SetHTTPRequestHeaderValue",				sm_SetHTTPRequestHeaderValue},
	{"SteamWorks_SetHTTPRequestGetOrPostParameter",				sm_SetHTTPRequestGetOrPostParameter},
	{"SteamWorks_SetHTTPRequestUserAgentInfo",					sm_SetHTTPRequestUserAgentInfo},
	{"SteamWorks_SetHTTPRequestRequiresVerifiedCertificate",				sm_SetHTTPRequestRequiresVerifiedCertificate},
	{"SteamWorks_SetHTTPRequestAbsoluteTimeoutMS",						sm_SetHTTPRequestAbsoluteTimeoutMS},
	{"SteamWorks_SetHTTPCallbacks",				sm_SetCallbacks},
	{"SteamWorks_SendHTTPRequest",				sm_SendHTTPRequest},
	{"SteamWorks_DeferHTTPRequest",				sm_DeferHTTPRequest},
	{"SteamWorks_PrioritizeHTTPRequest",				sm_PrioritizeHTTPRequest},
	{"SteamWorks_GetHTTPResponseHeaderSize",				sm_GetHTTPResponseHeaderSize},
	{"SteamWorks_GetHTTPResponseHeaderValue",				sm_GetHTTPResponseHeaderValue},
	{"SteamWorks_GetHTTPResponseBodySize",				sm_GetHTTPResponseBodySize},
	{"SteamWorks_GetHTTPResponseBodyData",				sm_GetHTTPResponseBodyData},
	{"SteamWorks_GetHTTPResponseBodyString",				sm_GetHTTPResponseBodyString},
	{"SteamWorks_GetHTTPDownloadProgressPct",				sm_GetHTTPDownloadProgressPct},
	{"SteamWorks_GetHTTPRequestWasTimedOut",				sm_GetHTTPRequestWasTimedOut},
	{"SteamWorks_SetHTTPRequestRawPostBody",				sm_SetHTTPRequestRawPostBody},
	{"SteamWorks_SetHTTPRequestRawPostBodyFromFile",		sm_SetHTTPRequestRawPostBodyFromFile},
	{"SteamWorks_GetHTTPResponseBodyCallback",				sm_GetHTTPResponseBodyCallback},
	{"SteamWorks_WriteHTTPResponseBodyToFile",				sm_WriteHTTPResponseBodyToFile},
	{"SteamWorks_SendHTTPRequestAndStreamResponse",			sm_SendHTTPRequestAndStreamResponse},
	{"SteamWorks_GetHTTPStreamingResponseBodyData",			sm_GetHTTPStreamingResponseBodyData},
	{NULL,											NULL}
};

SteamWorksHTTPNatives::SteamWorksHTTPNatives()
{
	sharesys->AddNatives(myself, httpnatives);
}

SteamWorksHTTPNatives::~SteamWorksHTTPNatives()
{
}
