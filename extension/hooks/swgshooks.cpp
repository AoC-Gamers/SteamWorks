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
#include "../core/swdebug.h"
#include "../core/swauthticket.h"
#include <cstring>
#include <memory>
#include <new>

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


SH_DECL_HOOK0(ISteamGameServer, WasRestartRequested, SH_NOATTRIB, 0, bool); /* From SteamTools. */
using steamworks::ISteamGameServer014;
SH_DECL_HOOK3(ISteamGameServer014, BeginAuthSession, SH_NOATTRIB, 0, EBeginAuthSessionResult, const void *, int, CSteamID);
SH_DECL_HOOK0_void(ISteamGameServer, LogOnAnonymous, SH_NOATTRIB, 0); /* From VoiDeD's MM:S plugin. */

static ISteamGameServer *GetGameServerPointer()
{
	return g_SteamWorks.pSWGameServer != nullptr ? g_SteamWorks.pSWGameServer->GetGameServer() : nullptr;
}

SteamWorksGSHooks::SteamWorksGSHooks()
{
	pRestartRequestedForward = forwards->CreateForward("SteamWorks_RestartRequested", ET_Hook, 0, nullptr);
	pTokenRequestedForward = forwards->CreateForward("SteamWorks_TokenRequested", ET_Ignore, 2, nullptr, Param_String, Param_Cell);
	pBeginAuthSessionForward = forwards->CreateForward("SteamWorks_BeginAuthSession", ET_Ignore, 3, nullptr, Param_Array, Param_Cell, Param_Cell);
	
	Start();
}

SteamWorksGSHooks::~SteamWorksGSHooks()
{
	Stop();
	ReleaseForward(pRestartRequestedForward);
	ReleaseForward(pTokenRequestedForward);
	ReleaseForward(pBeginAuthSessionForward);
}

void SteamWorksGSHooks::LogOnAnonymous(void)
{
	ISteamGameServer *pGameServer = m_target;
	const uint64 generation = m_generation;
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

	if (m_enabled && m_target == pGameServer && m_generation == generation) { pGameServer->LogOn(pToken); }
	RETURN_META(MRES_SUPERCEDE);
}

EBeginAuthSessionResult SteamWorksGSHooks::BeginAuthSession(const void *pAuthTicket, int cbAuthTicket, CSteamID steamID)
{
	steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_enter", "enabled=%d target=%d receivers=%u bytes=%d", m_enabled ? 1 : 0, m_authTarget != nullptr ? 1 : 0, pBeginAuthSessionForward != nullptr ? pBeginAuthSessionForward->GetFunctionCount() : 0, cbAuthTicket);
	if (m_enabled && m_authTarget != nullptr && pBeginAuthSessionForward != nullptr && pBeginAuthSessionForward->GetFunctionCount() != 0)
	{
		// Param_Array is the existing registration. PushStringEx is rejected by
		// SourceMod for that type. Copy packed bytes into aligned, padded cells;
		// never interpret the ticket as text or read beyond its byte length.
		if (cbAuthTicket < 0 || (cbAuthTicket > 0 && pAuthTicket == nullptr))
		{
			steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_skip", "reason=invalid_input");
			RETURN_META_VALUE(MRES_IGNORED, k_EBeginAuthSessionResultOK);
		}
		static_assert(sizeof(cell_t) == 4, "Authentication forward requires 32-bit Pawn cells");
		size_t cells;
		if (!steamworks::AuthTicketCellCount(cbAuthTicket, cells))
		{
			steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_skip", "reason=ticket_too_large");
			RETURN_META_VALUE(MRES_IGNORED, k_EBeginAuthSessionResultOK);
		}
		const size_t bytes = static_cast<size_t>(cbAuthTicket);
		std::unique_ptr<cell_t[]> ticket(new (std::nothrow) cell_t[cells]());
		if (!ticket)
		{
			steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_skip", "reason=allocation_failed");
			RETURN_META_VALUE(MRES_IGNORED, k_EBeginAuthSessionResultOK);
		}
		if (bytes != 0) { std::memcpy(ticket.get(), pAuthTicket, bytes); }
		const uint32 account = steamID.GetAccountID();
		cell_t authid;
		std::memcpy(&authid, &account, sizeof(authid));
		// No copyback: plugin writes cannot change the engine's auth ticket,
		// and each receiver gets its own copy from this immutable snapshot.
		if (pBeginAuthSessionForward->PushArray(ticket.get(), static_cast<unsigned int>(cells), 0) != SP_ERROR_NONE ||
			pBeginAuthSessionForward->PushCell(cbAuthTicket) != SP_ERROR_NONE ||
			pBeginAuthSessionForward->PushCell(authid) != SP_ERROR_NONE)
		{
			pBeginAuthSessionForward->Cancel();
			steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_skip", "reason=push_failed");
			RETURN_META_VALUE(MRES_IGNORED, k_EBeginAuthSessionResultOK);
		}
		const int result = pBeginAuthSessionForward->Execute(nullptr);
		steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_dispatch", "result=%d cells=%u", result, static_cast<unsigned>(cells));
	}
	else
	{
		steamworks::debug::Log(steamworks::debug::Auth, "begin_auth_skip", "reason=inactive_or_no_receivers");
	}

	RETURN_META_VALUE(MRES_IGNORED, k_EBeginAuthSessionResultOK);
}

bool SteamWorksGSHooks::WasRestartRequested(void) /* Mimic SteamTools. */
{
	if (!m_enabled || m_target == nullptr) { RETURN_META_VALUE(MRES_IGNORED, false); }
	const uint64 generation = m_generation;
	bool bWasRestartRequested = SH_CALL(m_target, &ISteamGameServer::WasRestartRequested)();
	if (m_enabled && m_generation == generation && bWasRestartRequested && pRestartRequestedForward != nullptr && pRestartRequestedForward->GetFunctionCount() != 0)
	{
		cell_t Result = Pl_Continue;
		pRestartRequestedForward->Execute(&Result);
		bWasRestartRequested = (Result >= Pl_Handled);
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, bWasRestartRequested); 
}

void SteamWorksGSHooks::SetFrameHook(bool enabled)
{
 if (m_frameHook == enabled) { return; }
 m_frameHook = enabled;
 if (enabled) { smutils->AddGameFrameHook(OurGameFrameHook); }
 else { smutils->RemoveGameFrameHook(OurGameFrameHook); }
}

void SteamWorksGSHooks::Detach()
{
 if (m_target != nullptr) { steamworks::debug::Log(steamworks::debug::Hooks, "gameserver_detach", "generation=%llu", static_cast<unsigned long long>(m_generation)); }
 ++m_generation;
 for (int& id : m_hookIds) {
  if (id != 0) { SH_REMOVE_HOOK_ID(id); id = 0; }
 }
 m_target = nullptr;
 m_authTarget = nullptr;
}

void SteamWorksGSHooks::Stop()
{
 m_enabled = false;
 SetFrameHook(false);
 Detach();
}

void SteamWorksGSHooks::Start()
{
 m_enabled = true;
 TryAttach();
}

void SteamWorksGSHooks::TraceState() const
{
 steamworks::debug::Log(steamworks::debug::Hooks, "gameserver_state", "enabled=%d target=%d waiting=%d restart=%d logon=%d auth=%d nativeVersion=SteamGameServer015 authVersion=SteamGameServer014", m_enabled ? 1 : 0, m_target != nullptr ? 1 : 0, m_frameHook ? 1 : 0, m_hookIds[0] != 0 ? 1 : 0, m_hookIds[1] != 0 ? 1 : 0, m_hookIds[2] != 0 ? 1 : 0);
 steamworks::debug::Log(steamworks::debug::Auth, "auth_state", "enabled=%d attached=%d receivers=%u", m_enabled ? 1 : 0, m_hookIds[2] != 0 ? 1 : 0, pBeginAuthSessionForward != nullptr ? pBeginAuthSessionForward->GetFunctionCount() : 0);
}

void SteamWorksGSHooks::TryAttach()
{
 if (!m_enabled) { return; }
 auto* target = GetGameServerPointer();
 auto* authTarget = g_SteamWorks.pSWGameServer != nullptr ? g_SteamWorks.pSWGameServer->GetGameServerAuth014() : nullptr;
 if (target != nullptr && authTarget != nullptr) { AddHooks(target, authTarget); }
 else {
  if (!m_frameHook) { steamworks::debug::Log(steamworks::debug::Hooks, "gameserver_wait"); }
  SetFrameHook(true);
 }
}

void SteamWorksGSHooks::AddHooks(ISteamGameServer* target, ISteamGameServer014* authTarget)
{
 if (!m_enabled || target == nullptr || authTarget == nullptr || (m_target == target && m_authTarget == authTarget)) { return; }
 Detach();
 m_target = target;
 m_authTarget = authTarget;
 m_hookIds[0] = SH_ADD_HOOK(ISteamGameServer, WasRestartRequested, target, SH_MEMBER(this, &SteamWorksGSHooks::WasRestartRequested), false);
 steamworks::debug::Log(steamworks::debug::Hooks, "hook_install", "method=WasRestartRequested success=%d", m_hookIds[0] != 0 ? 1 : 0);
 if (m_hookIds[0] == 0) { Stop(); g_pSM->LogError(myself, "Could not install ISteamGameServer WasRestartRequested hook; group disabled until restart"); return; }
 m_hookIds[1] = SH_ADD_HOOK(ISteamGameServer, LogOnAnonymous, target, SH_MEMBER(this, &SteamWorksGSHooks::LogOnAnonymous), false);
 steamworks::debug::Log(steamworks::debug::Hooks, "hook_install", "method=LogOnAnonymous success=%d", m_hookIds[1] != 0 ? 1 : 0);
 if (m_hookIds[1] == 0) { Stop(); g_pSM->LogError(myself, "Could not install ISteamGameServer LogOnAnonymous hook; group disabled until restart"); return; }
 m_hookIds[2] = SH_ADD_HOOK(ISteamGameServer014, BeginAuthSession, authTarget, SH_MEMBER(this, &SteamWorksGSHooks::BeginAuthSession), false);
 steamworks::debug::Log(steamworks::debug::Hooks, "hook_install", "method=BeginAuthSession success=%d version=SteamGameServer014", m_hookIds[2] != 0 ? 1 : 0);
 if (m_hookIds[2] == 0) { Stop(); g_pSM->LogError(myself, "Could not install ISteamGameServer014 BeginAuthSession hook; group disabled until restart"); return; }
 SetFrameHook(false);
}

void OurGameFrameHook(bool simulating)
{
 (void)simulating;
 if (g_SteamWorks.pGSHooks != nullptr) { g_SteamWorks.pGSHooks->TryAttach(); }
}
