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

#include "swgshooks.h"
#include "../steamtools/ticket.h"

namespace {

void ReleaseForward(IForward*& forward)
{
	if (forward == nullptr) {
		return;
	}

	forwards->ReleaseForward(forward);
	forward = nullptr;
}

}  // namespace

enum
{
	eUnhooked = 0,
	eHooking,
	eHooked
};

SH_DECL_HOOK0(ISteamGameServer, WasRestartRequested, SH_NOATTRIB, 0, bool); /* From SteamTools. */
SH_DECL_HOOK3(ISteamGameServer, BeginAuthSession, SH_NOATTRIB, 0, EBeginAuthSessionResult, const void *, int, CSteamID); /* From SteamTools. */
SH_DECL_HOOK0_void(ISteamGameServer, LogOnAnonymous, SH_NOATTRIB, 0); /* From VoiDeD's MM:S plugin. */

static ISteamGameServer *GetGameServerPointer()
{
	return g_SteamWorks.pSWGameServer->GetGameServer();
}

SteamWorksGSHooks::SteamWorksGSHooks()
{
	uHooked = eHooking;
	pRestartRequestedForward = forwards->CreateForward("SteamWorks_RestartRequested", ET_Hook, 0, nullptr);
	pTokenRequestedForward = forwards->CreateForward("SteamWorks_TokenRequested", ET_Ignore, 2, nullptr, Param_String, Param_Cell);
	pBeginAuthSessionForward = forwards->CreateForward("SteamWorks_BeginAuthSession", ET_Ignore, 3, nullptr, Param_Array, Param_Cell, Param_Cell);
	
	ISteamGameServer *pGameServer = GetGameServerPointer();
	if (pGameServer)
	{
		AddHooks(pGameServer);
	}
	else
	{
		smutils->AddGameFrameHook(OurGameFrameHook);
	}
}

SteamWorksGSHooks::~SteamWorksGSHooks()
{
	RemoveHooks(GetGameServerPointer(), true);
	smutils->RemoveGameFrameHook(OurGameFrameHook);
	ReleaseForward(pRestartRequestedForward);
	ReleaseForward(pTokenRequestedForward);
	ReleaseForward(pBeginAuthSessionForward);
}

void SteamWorksGSHooks::LogOnAnonymous(void)
{
	ISteamGameServer *pGameServer = GetGameServerPointer();
	if (pGameServer == nullptr)
	{
		RETURN_META(MRES_SUPERCEDE);
	}

	if (pTokenRequestedForward == nullptr || pTokenRequestedForward->GetFunctionCount() == 0)
	{
		RETURN_META(MRES_IGNORED);
	}

	char pToken[256];
	pToken[0] = '\0';
	pTokenRequestedForward->PushStringEx(pToken, sizeof(pToken), SM_PARAM_STRING_UTF8 | SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
	pTokenRequestedForward->PushCell(sizeof(pToken));
	pTokenRequestedForward->Execute(nullptr);

	pGameServer->LogOn(pToken);
	RETURN_META(MRES_SUPERCEDE);
}

EBeginAuthSessionResult SteamWorksGSHooks::BeginAuthSession(const void *pAuthTicket, int cbAuthTicket, CSteamID steamID)
{
	if (pBeginAuthSessionForward != nullptr && pBeginAuthSessionForward->GetFunctionCount() != 0)
	{
		char *pszAuthTicket = reinterpret_cast<char *>(const_cast<void *>(pAuthTicket));

		pBeginAuthSessionForward->PushStringEx(pszAuthTicket, cbAuthTicket, SM_PARAM_STRING_BINARY | SM_PARAM_STRING_COPY, 0);
		pBeginAuthSessionForward->PushCell(cbAuthTicket);
		pBeginAuthSessionForward->PushCell(steamID.GetAccountID());
		pBeginAuthSessionForward->Execute(nullptr);
	}

	RETURN_META_VALUE(MRES_IGNORED, k_EBeginAuthSessionResultOK);
}

bool SteamWorksGSHooks::WasRestartRequested(void) /* Mimic SteamTools. */
{
	bool bWasRestartRequested = SH_CALL(GetGameServerPointer(), &ISteamGameServer::WasRestartRequested)();
	if (bWasRestartRequested && pRestartRequestedForward != nullptr && pRestartRequestedForward->GetFunctionCount() != 0)
	{
		cell_t Result = Pl_Continue;
		pRestartRequestedForward->Execute(&Result);
		bWasRestartRequested = (Result >= Pl_Handled);
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, bWasRestartRequested); 
}

void SteamWorksGSHooks::AddHooks(ISteamGameServer *pGameServer)
{
	if (uHooked == eHooked || pGameServer == nullptr)
	{
		return;
	}

	uHooked = eHooked;
	SH_ADD_HOOK(ISteamGameServer, WasRestartRequested, pGameServer, SH_MEMBER(this, &SteamWorksGSHooks::WasRestartRequested), false);
	SH_ADD_HOOK(ISteamGameServer, LogOnAnonymous, pGameServer, SH_MEMBER(this, &SteamWorksGSHooks::LogOnAnonymous), false);
	SH_ADD_HOOK(ISteamGameServer, BeginAuthSession, pGameServer, SH_MEMBER(this, &SteamWorksGSHooks::BeginAuthSession), false);
}

void SteamWorksGSHooks::RemoveHooks(ISteamGameServer *pGameServer, bool destroyed)
{
	if (uHooked != eHooked || pGameServer == nullptr)
	{
		return;
	}

	SH_REMOVE_HOOK(ISteamGameServer, WasRestartRequested, pGameServer, SH_MEMBER(this, &SteamWorksGSHooks::WasRestartRequested), false);
	SH_REMOVE_HOOK(ISteamGameServer, LogOnAnonymous, pGameServer, SH_MEMBER(this, &SteamWorksGSHooks::LogOnAnonymous), false);
	SH_REMOVE_HOOK(ISteamGameServer, BeginAuthSession, pGameServer, SH_MEMBER(this, &SteamWorksGSHooks::BeginAuthSession), false);
	if (destroyed)
	{
		uHooked = eUnhooked;
		return;
	}

	uHooked = eHooking;
	smutils->AddGameFrameHook(OurGameFrameHook);
}

void OurGameFrameHook(bool simulating) /* What we do for SDK independence. */
{
	(void)simulating;

	ISteamGameServer *pGameServer = GetGameServerPointer();
	if (pGameServer == nullptr)
	{
		return;
	}

	g_SteamWorks.pGSHooks->AddHooks(pGameServer);
	smutils->RemoveGameFrameHook(OurGameFrameHook);
}
