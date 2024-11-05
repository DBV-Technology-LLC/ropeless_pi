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

#include "manualPlacementDlgImpl.h"

manualPlacementDlgImpl::manualPlacementDlgImpl(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style,
	const wxString& latStr, const wxString& lonStr, const wxString& utcStr) : manualPlacementDlg(parent, id, title, pos, size, style)
{
	//wxLogMessage("Creating manual placement dlg impl!");
	isOwned = true;
	valid = false;

	wxLogMessage("Creating Manual Placement Dialog! @ %s",utcStr);

	m_staticText7->SetLabel(latStr);
	m_staticText8->SetLabel(lonStr);
	m_staticText111->SetLabel(utcStr);

}

void manualPlacementDlgImpl::cancelPlaceTransponder(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void manualPlacementDlgImpl::okPlaceTransponder(wxCommandEvent& event)
{
	// Set xpdr id and pair id
	long xid;
	long pid;
	int ownf = isOwned ? 1 : -1;

	wxString xStr = m_textCtrl12->GetValue();
	wxString pStr = m_textCtrl121->GetValue();

	if (xStr.IsEmpty() || pStr.IsEmpty())
	{
		wxLogMessage("One or more strings are empty!");
		valid = false;
	}
	else
	{
		xStr.ToLong(&xid);
		pStr.ToLong(&pid);

		xpdrId = int(xid)*ownf;
		pairId = int(pid)*ownf;

		valid = true;
	}

    EndModal(wxID_OK);
}

void manualPlacementDlgImpl::idOnChar( wxKeyEvent& event )
{
	OnChar(event);
}

void manualPlacementDlgImpl::pairOnChar( wxKeyEvent& event )
{
	OnChar(event);
}

void manualPlacementDlgImpl::ownedChecked( wxCommandEvent& event )
{
	isOwned = event.IsChecked();
	//wxLogMessage("Set ownership to  %s", isOwned ? "true" : "false");
}

void manualPlacementDlgImpl::OnChar(wxKeyEvent& event) {

	//wxLogMessage("Processing char event!");

    int keyCode = event.GetKeyCode();

    // Allow only digits, backspace, delete, and navigation keys
    if ((keyCode >= '0' && keyCode <= '9') || keyCode == WXK_BACK || keyCode == WXK_DELETE ||
        keyCode == WXK_LEFT || keyCode == WXK_RIGHT || keyCode == WXK_TAB) {
        event.Skip(); // Skip the event to allow these keys
    }
    // Optionally, allow Enter to submit if desired
    else if (keyCode == WXK_RETURN || keyCode == WXK_NUMPAD_ENTER) {
        event.Skip();
    }
}
