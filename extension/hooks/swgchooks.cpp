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

enum
{
	eUnhooked = 0,
	eHooking,
	eHooked
};

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
	return g_SteamWorks.pSWGameServer->GetGameCoordinator();
}

SteamWorksGCHooks::SteamWorksGCHooks()
{
	uHooked = eHooking;
	pGCSendMessageForward = forwards->CreateForward("SteamWorks_GCSendMessage", ET_Event, 3, nullptr, Param_Cell, Param_String, Param_Cell);
	pGCMessageAvailableForward = forwards->CreateForward("SteamWorks_GCMsgAvailable", ET_Ignore, 1, nullptr, Param_Cell);
	pGCRetrieveMessageForward = forwards->CreateForward("SteamWorks_GCRetrieveMessage", ET_Event, 4, nullptr, Param_Cell, Param_String, Param_Cell, Param_Cell);
	
	ISteamGameCoordinator *pGC = GetSteamGCPointer();
	if (pGC)
	{
		AddHooks(pGC);
	}
	else
	{
		smutils->AddGameFrameHook(OurGCGameFrameHook);
	}
}

SteamWorksGCHooks::~SteamWorksGCHooks()
{
	RemoveHooks(GetSteamGCPointer(), true);
	smutils->RemoveGameFrameHook(OurGCGameFrameHook);
	ReleaseForward(pGCSendMessageForward);
	ReleaseForward(pGCMessageAvailableForward);
	ReleaseForward(pGCRetrieveMessageForward);
}

EGCResults SteamWorksGCHooks::SendMessage(uint32 unMsgType, const void *pubData, uint32 cubData)
{
	if (pGCSendMessageForward->GetFunctionCount() == 0)
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
	if (pGCMessageAvailableForward->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	bool res = META_RESULT_ORIG_RET(bool);
	if (!res)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	uint32_t shill;
	if (!pcubMsgSize)
	{
		shill = 0;
		SH_CALL(GetSteamGCPointer(), &ISteamGameCoordinator::IsMessageAvailable)(&shill);
		pcubMsgSize = &shill;
	}

	pGCMessageAvailableForward->PushCell(*pcubMsgSize);
	pGCMessageAvailableForward->Execute(nullptr);
	RETURN_META_VALUE(MRES_IGNORED, true);
}

EGCResults SteamWorksGCHooks::RetrieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize)
{
	if (pGCRetrieveMessageForward->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, k_EGCResultOK);
	}

	/* Don't trust a bitch, except for 2GD. https://www.youtube.com/watch?v=MgePh_YJgrc */
	cell_t Result = k_EGCResultOK;
	EGCResults res = SH_CALL(GetSteamGCPointer(), &ISteamGameCoordinator::RetrieveMessage)(punMsgType, pubDest, cubDest, pcubMsgSize);
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

void SteamWorksGCHooks::AddHooks(ISteamGameCoordinator *pGC)
{
	if (uHooked == eHooked || pGC == nullptr)
	{
		return;
	}

	uHooked = eHooked;
	SH_ADD_HOOK(ISteamGameCoordinator, SendMessage, pGC, SH_MEMBER(this, &SteamWorksGCHooks::SendMessage), false);
	SH_ADD_HOOK(ISteamGameCoordinator, IsMessageAvailable, pGC, SH_MEMBER(this, &SteamWorksGCHooks::IsMessageAvailable), true);
	SH_ADD_HOOK(ISteamGameCoordinator, RetrieveMessage, pGC, SH_MEMBER(this, &SteamWorksGCHooks::RetrieveMessage), false);
}

void SteamWorksGCHooks::RemoveHooks(ISteamGameCoordinator *pGC, bool destroyed)
{
	if (uHooked != eHooked || pGC == nullptr)
	{
		return;
	}

	SH_REMOVE_HOOK(ISteamGameCoordinator, SendMessage, pGC, SH_MEMBER(this, &SteamWorksGCHooks::SendMessage), false);
	SH_REMOVE_HOOK(ISteamGameCoordinator, IsMessageAvailable, pGC, SH_MEMBER(this, &SteamWorksGCHooks::IsMessageAvailable), true);
	SH_REMOVE_HOOK(ISteamGameCoordinator, RetrieveMessage, pGC, SH_MEMBER(this, &SteamWorksGCHooks::RetrieveMessage), false);

	if (destroyed)
	{
		uHooked = eUnhooked;
		return;
	}

	uHooked = eHooking;
	smutils->AddGameFrameHook(OurGCGameFrameHook);
}

void OurGCGameFrameHook(bool simulating) /* What we do for SDK independence. */
{
	(void)simulating;

	ISteamGameCoordinator *pGC = GetSteamGCPointer();
	if (pGC == nullptr)
	{
		return;
	}

	g_SteamWorks.pGCHooks->AddHooks(pGC);
	smutils->RemoveGameFrameHook(OurGCGameFrameHook);
}
