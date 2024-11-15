/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Ropeless Plugin
 * Author:   Colin Vincent
 *
 ******************************************************************************
 * This file is part of the Ropeless plugin
 * (https://github.com/bdbcat/ropeless_pi).
 *   Copyright (C) 2024 by Ropeless Systems, Inc. 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3, or (at your option) any later
 * version of the license.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif

#include "transponderReleaseDlgImpl.h"
#include "ropeless_pi.h"

transponderReleaseDlgImpl::transponderReleaseDlgImpl(wxWindow* parent, ropeless_pi* parent_pi, int id,
	const wxString& title, const wxPoint& pos, const wxSize& size, long style) : transponderReleaseDlg(parent, id, title, pos, size, style)
{

	pParentPi = parent_pi;

	//wxLogMessage("Creating transponderReleaseDlg!");
	wxString test = "---";
	updateStatus(test);

	hideButtons();
}

void transponderReleaseDlgImpl::updateID(int id)
{	
	wxString idStr;
	idStr.Printf("%d",id);
	m_staticTextID->SetLabel(idStr);
}

void transponderReleaseDlgImpl::updateStatus(wxString status)
{	
	m_staticTextStatus->SetLabel(status);
}

void transponderReleaseDlgImpl::showButtons(void)
{
	m_buttonRecovered->Show(true);
	m_buttonRetry->Show(true);
}

void transponderReleaseDlgImpl::hideButtons(void)
{
	m_buttonRecovered->Show(false);
	m_buttonRetry->Show(false);
}

void transponderReleaseDlgImpl::OnClose(wxCloseEvent& event) {
    //Destroy(); // this breaks since it's deleted
    this->Hide();	
}

void transponderReleaseDlgImpl::CloseDialog() {
    wxCloseEvent closeEvent(wxEVT_CLOSE_WINDOW);
    GetEventHandler()->ProcessEvent(closeEvent); // Trigger the close event
}

void transponderReleaseDlgImpl::markRecoveredClick(wxCommandEvent& event)
{
	wxLogMessage("Mark Recovered Clicked in Release Dialog");
	pParentPi->releaseCallbackRecovered();
}

void transponderReleaseDlgImpl::retryClick(wxCommandEvent& event)
{
	wxLogMessage("Retry Clicked in Release Dialog");
	pParentPi->releaseCallbackRetry();
}

void transponderReleaseDlgImpl::okClick(wxCommandEvent& event)
{
	wxLogMessage("Ok Recovered Clicked in Release Dialog");
	CloseDialog();
}