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

#include "swforwards.h"
#include "swdebug.h"

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

SteamWorksForwards::SteamWorksForwards() :
		m_CallbackGSClientApprove(this, &SteamWorksForwards::OnGSClientApprove),
		m_CallbackValidateTicket(this, &SteamWorksForwards::OnValidateTicket),
		m_CallbackSteamConnected(this, &SteamWorksForwards::OnSteamServersConnected),
		m_CallbackSteamConnectFailure(this, &SteamWorksForwards::OnSteamServersConnectFailure),
		m_CallbackSteamDisconnected(this, &SteamWorksForwards::OnSteamServersDisconnected),
		m_CallbackGroupStatus(this, &SteamWorksForwards::OnGroupStatusResult)
{
	pLegacyValidateClientForward = forwards->CreateForward("SW_OnValidateClient", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);
	pValidateClientForward = forwards->CreateForward("SteamWorks_OnValidateClient", ET_Ignore, 2, nullptr, Param_Cell, Param_Cell);
	pSteamServersConnectedForward = forwards->CreateForward("SteamWorks_SteamServersConnected", ET_Ignore, 0, nullptr);
	pSteamServersConnectFailureForward = forwards->CreateForward("SteamWorks_SteamServersConnectFailure", ET_Ignore, 1, nullptr, Param_Cell);
	pSteamServersDisconnectedForward = forwards->CreateForward("SteamWorks_SteamServersDisconnected", ET_Ignore, 1, nullptr, Param_Cell);
	pClientGroupStatusForward = forwards->CreateForward("SteamWorks_OnClientGroupStatus", ET_Ignore, 4, nullptr, Param_Cell, Param_Cell, Param_Cell, Param_Cell);
}

SteamWorksForwards::~SteamWorksForwards()
{
	Stop();
	ReleaseForward(pLegacyValidateClientForward);
	ReleaseForward(pValidateClientForward);
	ReleaseForward(pSteamServersConnectedForward);
	ReleaseForward(pSteamServersConnectFailureForward);
	ReleaseForward(pSteamServersDisconnectedForward);
	ReleaseForward(pClientGroupStatusForward);
}

void SteamWorksForwards::ExecuteValidateClientForward(IForward* forward, Account_t parent, Account_t child)
{
	if (!m_running || forward == nullptr || forward->GetFunctionCount() == 0) {
		return;
	}

	forward->PushCell(parent);
	forward->PushCell(child);
	steamworks::debug::Log(steamworks::debug::Auth, "validation_dispatch", "forward=%s receivers=%u", forward == pLegacyValidateClientForward ? "legacy" : "current", forward->GetFunctionCount());
	forward->Execute(nullptr);
}

void SteamWorksForwards::ExecuteSteamServerResultForward(IForward* forward, cell_t result)
{
	if (!m_running || forward == nullptr || forward->GetFunctionCount() == 0) {
		return;
	}

	forward->PushCell(result);
	forward->Execute(nullptr);
}

void SteamWorksForwards::NotifyPawnValidateClient(Account_t parent, Account_t child)
{
	const uint64 generation = m_generation;
	ExecuteValidateClientForward(pLegacyValidateClientForward, parent, child);
	if (m_generation == generation) { ExecuteValidateClientForward(pValidateClientForward, parent, child); }
}

void SteamWorksForwards::OnGSClientApprove(GSClientApprove_t *pApprove)
{
	steamworks::debug::Log(steamworks::debug::Auth, "validation_callback", "source=GSClientApprove");
	this->NotifyPawnValidateClient(pApprove->m_OwnerSteamID.GetAccountID(), pApprove->m_SteamID.GetAccountID());
}

void SteamWorksForwards::OnValidateTicket(ValidateAuthTicketResponse_t *pTicket)
{
	steamworks::debug::Log(steamworks::debug::Auth, "validation_callback", "source=ValidateAuthTicketResponse result=%d", static_cast<int>(pTicket->m_eAuthSessionResponse));
	this->NotifyPawnValidateClient(pTicket->m_OwnerSteamID.GetAccountID(), pTicket->m_SteamID.GetAccountID());
}

void SteamWorksForwards::OnSteamServersConnected(SteamServersConnected_t *pResponse)
{
	(void)pResponse;
	if (!m_running || pSteamServersConnectedForward == nullptr || pSteamServersConnectedForward->GetFunctionCount() == 0) {
		return;
	}

	pSteamServersConnectedForward->Execute(nullptr);
}

void SteamWorksForwards::OnSteamServersConnectFailure(SteamServerConnectFailure_t *pResponse)
{
	ExecuteSteamServerResultForward(pSteamServersConnectFailureForward, pResponse->m_eResult);
}

void SteamWorksForwards::OnSteamServersDisconnected(SteamServersDisconnected_t *pResponse)
{
	ExecuteSteamServerResultForward(pSteamServersDisconnectedForward, pResponse->m_eResult);
}

void SteamWorksForwards::OnGroupStatusResult(GSClientGroupStatus_t *pResponse)
{
	if (!m_running || pClientGroupStatusForward == nullptr || pClientGroupStatusForward->GetFunctionCount() == 0) {
		return;
	}

	pClientGroupStatusForward->PushCell(pResponse->m_SteamIDUser.GetAccountID());
	pClientGroupStatusForward->PushCell(pResponse->m_SteamIDGroup.GetAccountID());
	pClientGroupStatusForward->PushCell(pResponse->m_bMember);
	pClientGroupStatusForward->PushCell(pResponse->m_bOfficer);
	pClientGroupStatusForward->Execute(nullptr);
}

void SteamWorksForwards::Stop()
{
 if (!m_running) { return; }
 m_running = false;
 ++m_generation;
 m_CallbackGSClientApprove.Unregister();
 m_CallbackValidateTicket.Unregister();
 m_CallbackSteamConnected.Unregister();
 m_CallbackSteamConnectFailure.Unregister();
 m_CallbackSteamDisconnected.Unregister();
 m_CallbackGroupStatus.Unregister();
}

void SteamWorksForwards::Start()
{
 if (m_running) { return; }
 m_running = true;
 m_CallbackGSClientApprove.Register(this, &SteamWorksForwards::OnGSClientApprove);
 m_CallbackValidateTicket.Register(this, &SteamWorksForwards::OnValidateTicket);
 m_CallbackSteamConnected.Register(this, &SteamWorksForwards::OnSteamServersConnected);
 m_CallbackSteamConnectFailure.Register(this, &SteamWorksForwards::OnSteamServersConnectFailure);
 m_CallbackSteamDisconnected.Register(this, &SteamWorksForwards::OnSteamServersDisconnected);
 m_CallbackGroupStatus.Register(this, &SteamWorksForwards::OnGroupStatusResult);
}
