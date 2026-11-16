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

#include "swgchooks.h"


namespace
{
	void ReleaseForward(IForward*& forward)
	{
		if (!forward)
		{
			return;
		}

		forwards->ReleaseForward(forward);
		forward = nullptr;
	}
}

SH_DECL_HOOK3(ISteamGameCoordinator, SendMessage, SH_NOATTRIB, 0, EGCResults, uint32, const void *, uint32);
SH_DECL_HOOK1(ISteamGameCoordinator, IsMessageAvailable, SH_NOATTRIB, 0, bool, uint32 *);
SH_DECL_HOOK4(ISteamGameCoordinator, RetrieveMessage, SH_NOATTRIB, 0, EGCResults, uint32 *, void *, uint32, uint32 *);

static ISteamGameCoordinator *GetSteamGCPointer()
{
	return g_SteamWorks.pSWGameServer != nullptr ? g_SteamWorks.pSWGameServer->GetGameCoordinator() : nullptr;
}

SteamWorksGCHooks::SteamWorksGCHooks()
{
	pGCSendMessageForward = forwards->CreateForward("SteamWorks_GCSendMessage", ET_Event, 3, nullptr, Param_Cell, Param_String, Param_Cell);
	pGCMessageAvailableForward = forwards->CreateForward("SteamWorks_GCMsgAvailable", ET_Ignore, 1, nullptr, Param_Cell);
	pGCRetrieveMessageForward = forwards->CreateForward("SteamWorks_GCRetrieveMessage", ET_Event, 4, nullptr, Param_Cell, Param_String, Param_Cell, Param_Cell);
	
	Start();
}

SteamWorksGCHooks::~SteamWorksGCHooks()
{
	Stop();
	ReleaseForward(pGCSendMessageForward);
	ReleaseForward(pGCMessageAvailableForward);
	ReleaseForward(pGCRetrieveMessageForward);
}

EGCResults SteamWorksGCHooks::SendMessage(uint32 unMsgType, const void *pubData, uint32 cubData)
{
	if (!m_enabled || m_target == nullptr || pGCSendMessageForward == nullptr || pGCSendMessageForward->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, k_EGCResultOK);
	}

	cell_t Result = k_EGCResultOK;
	pGCSendMessageForward->PushCell(unMsgType);
	pGCSendMessageForward->PushStringEx(reinterpret_cast<char *>(const_cast<void *>(pubData)), cubData, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
	pGCSendMessageForward->PushCell(cubData);
	pGCSendMessageForward->Execute(&Result);

	if (Result != k_EGCResultOK)
	{
		if (Result == -1)
		{
			Result = k_EGCResultOK;
		}

		RETURN_META_VALUE(MRES_SUPERCEDE, static_cast<EGCResults>(Result));
	}

	RETURN_META_VALUE(MRES_IGNORED, k_EGCResultOK);
}

bool SteamWorksGCHooks::IsMessageAvailable(uint32_t *pcubMsgSize)
{
	if (!m_enabled || m_target == nullptr || pGCMessageAvailableForward == nullptr || pGCMessageAvailableForward->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	const uint64 generation = m_generation;
	bool res = META_RESULT_ORIG_RET(bool);
	if (!res)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	uint32_t shill;
	if (!pcubMsgSize)
	{
		shill = 0;
		SH_CALL(m_target, &ISteamGameCoordinator::IsMessageAvailable)(&shill);
		pcubMsgSize = &shill;
	}

	if (!m_enabled || m_generation != generation) { RETURN_META_VALUE(MRES_IGNORED, res); }
	pGCMessageAvailableForward->PushCell(*pcubMsgSize);
	pGCMessageAvailableForward->Execute(nullptr);
	RETURN_META_VALUE(MRES_IGNORED, true);
}

EGCResults SteamWorksGCHooks::RetrieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize)
{
	if (!m_enabled || m_target == nullptr || pGCRetrieveMessageForward == nullptr || pGCRetrieveMessageForward->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, k_EGCResultOK);
	}

	/* Don't trust a bitch, except for 2GD. https://www.youtube.com/watch?v=MgePh_YJgrc */
	cell_t Result = k_EGCResultOK;
	const uint64 generation = m_generation;
	EGCResults res = SH_CALL(m_target, &ISteamGameCoordinator::RetrieveMessage)(punMsgType, pubDest, cubDest, pcubMsgSize);
	if (!m_enabled || m_generation != generation) { RETURN_META_VALUE(MRES_SUPERCEDE, res); }
	if (punMsgType)
		pGCRetrieveMessageForward->PushCell(*punMsgType);
	else
		pGCRetrieveMessageForward->PushCell(0);

	if (pubDest)
		pGCRetrieveMessageForward->PushStringEx(reinterpret_cast<char *>(pubDest), cubDest, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
	else
		pGCRetrieveMessageForward->PushStringEx(const_cast<char *>(""), 1, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);

	pGCRetrieveMessageForward->PushCell(cubDest);

	if (pcubMsgSize)
		pGCRetrieveMessageForward->PushCell(*pcubMsgSize);
	else
		pGCRetrieveMessageForward->PushCell(0);

	pGCRetrieveMessageForward->Execute(&Result);

	if (Result != k_EGCResultOK)
	{
		if (Result == -1)
		{
			Result = k_EGCResultOK;
		}

		RETURN_META_VALUE(MRES_SUPERCEDE, static_cast<EGCResults>(Result));
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, res);
}

void SteamWorksGCHooks::SetFrameHook(bool enabled)
{
 if (m_frameHook == enabled) { return; }
 m_frameHook = enabled;
 if (enabled) { smutils->AddGameFrameHook(OurGCGameFrameHook); }
 else { smutils->RemoveGameFrameHook(OurGCGameFrameHook); }
}

void SteamWorksGCHooks::Detach()
{
 ++m_generation;
 for (int& id : m_hookIds) {
  if (id != 0) { SH_REMOVE_HOOK_ID(id); id = 0; }
 }
 m_target = nullptr;
}

void SteamWorksGCHooks::Stop()
{
 m_enabled = false;
 SetFrameHook(false);
 Detach();
}

void SteamWorksGCHooks::Start()
{
 m_enabled = true;
 TryAttach();
}

void SteamWorksGCHooks::TryAttach()
{
 if (!m_enabled) { return; }
 auto* target = GetSteamGCPointer();
 if (target != nullptr) { AddHooks(target); }
 else { SetFrameHook(true); }
}

void SteamWorksGCHooks::AddHooks(ISteamGameCoordinator* target)
{
 if (!m_enabled || target == nullptr || m_target == target) { return; }
 Detach();
 m_target = target;
 m_hookIds[0] = SH_ADD_HOOK(ISteamGameCoordinator, SendMessage, target, SH_MEMBER(this, &SteamWorksGCHooks::SendMessage), false);
 if (m_hookIds[0] == 0) { Stop(); g_pSM->LogError(myself, "Could not install ISteamGameCoordinator SendMessage hook; group disabled until restart"); return; }
 m_hookIds[1] = SH_ADD_HOOK(ISteamGameCoordinator, IsMessageAvailable, target, SH_MEMBER(this, &SteamWorksGCHooks::IsMessageAvailable), true);
 if (m_hookIds[1] == 0) { Stop(); g_pSM->LogError(myself, "Could not install ISteamGameCoordinator IsMessageAvailable hook; group disabled until restart"); return; }
 m_hookIds[2] = SH_ADD_HOOK(ISteamGameCoordinator, RetrieveMessage, target, SH_MEMBER(this, &SteamWorksGCHooks::RetrieveMessage), false);
 if (m_hookIds[2] == 0) { Stop(); g_pSM->LogError(myself, "Could not install ISteamGameCoordinator RetrieveMessage hook; group disabled until restart"); return; }
 SetFrameHook(false);
}

void OurGCGameFrameHook(bool simulating)
{
 (void)simulating;
 if (g_SteamWorks.pGCHooks != nullptr) { g_SteamWorks.pGCHooks->TryAttach(); }
}
