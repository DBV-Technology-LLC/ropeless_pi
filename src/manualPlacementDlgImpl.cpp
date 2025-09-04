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
#include "ropeless_pi.h"

manualPlacementDlgImpl::manualPlacementDlgImpl(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style,
	const wxString& latStr, const wxString& lonStr, const wxString& utcStr) : manualPlacementDlg(parent, id, title, pos, size, style)
{
	//wxLogMessage("Creating manual placement dlg impl!");
	isOwned = true;
	valid = false;
	positionSource = ePOS_SOURCE_USER; // Default to user

	wxLogMessage("Creating Manual Placement Dialog! @ %s",utcStr);

	m_staticText7->SetLabel(latStr);
	m_staticText8->SetLabel(lonStr);
	m_staticText111->SetLabel(utcStr);
	
	// Add position source dropdown control
	AddPositionSourceControl();

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
	
	// Get selected position source
	if (m_choicePositionSource) {
		positionSource = m_choicePositionSource->GetSelection();
		wxLogMessage("Manual placement dialog: Selected position source = %d (%s)", 
		             positionSource, 
		             (positionSource < 4) ? positionSourceNames[positionSource] : "UNKNOWN");
	} else {
		wxLogMessage("Manual placement dialog: WARNING - m_choicePositionSource is NULL");
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

void manualPlacementDlgImpl::AddPositionSourceControl() {
    // Get the main sizer
    wxSizer* mainSizer = this->GetSizer();
    if (!mainSizer) return;
    
    // Create a horizontal sizer for the position source controls
    wxBoxSizer* posSourceSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Add label
    wxStaticText* posSourceLabel = new wxStaticText(this, wxID_ANY, _("Position Source:"), 
                                                   wxDefaultPosition, wxSize(100, -1), 0);
    posSourceSizer->Add(posSourceLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    // Create the dropdown with position source options
    wxArrayString choices;
    choices.Add(_("USER"));
    choices.Add(_("CLOUD"));
    choices.Add(_("ACOUSTIC"));
    choices.Add(_("GPS"));
    
    m_choicePositionSource = new wxChoice(this, wxID_ANY, wxDefaultPosition, 
                                         wxSize(120, -1), choices);
    m_choicePositionSource->SetSelection(ePOS_SOURCE_USER); // Default to USER
    
    posSourceSizer->Add(m_choicePositionSource, 0, wxALL, 5);
    
    // Add spacer to right-align
    posSourceSizer->AddStretchSpacer(1);
    
    // Insert the position source sizer before the button sizer
    // Find the button sizer (it should be the last item)
    size_t sizerCount = mainSizer->GetItemCount();
    if (sizerCount > 0) {
        // Insert before the last item (which should be the buttons)
        mainSizer->Insert(sizerCount - 1, posSourceSizer, 0, wxEXPAND | wxALL, 5);
    } else {
        // Fallback: just add it
        mainSizer->Add(posSourceSizer, 0, wxEXPAND | wxALL, 5);
    }
    
    // Connect the event handler
    m_choicePositionSource->Connect(wxEVT_COMMAND_CHOICE_SELECTED, 
                                   wxCommandEventHandler(manualPlacementDlgImpl::OnPositionSourceChanged), 
                                   NULL, this);
    
    // Refresh the layout
    this->Layout();
    this->Fit();
}

void manualPlacementDlgImpl::OnPositionSourceChanged(wxCommandEvent& event) {
    positionSource = m_choicePositionSource->GetSelection();
    wxLogMessage("Position source changed to: %s", 
                positionSourceNames[positionSource]);
}
