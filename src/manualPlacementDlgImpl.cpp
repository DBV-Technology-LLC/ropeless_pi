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
	const wxString& latStr, const wxString& lonStr, const wxString& utcStr, transponder_state* existingState) : manualPlacementDlg(parent, id, title, pos, size, style)
{
	//wxLogMessage("Creating manual placement dlg impl!");
	isOwned = true;
	valid = false;
	positionSource = ePOS_SOURCE_USER; // Default to user
	deviceType = 0; // Default to Ropeless Systems Inc. (0x00)
	selectedTrawlId = 0; // Default to "None"

	wxLogMessage("Creating Manual Placement Dialog! @ %s",utcStr);

	m_staticText7->SetLabel(latStr);
	m_staticText8->SetLabel(lonStr);
	m_staticText111->SetLabel(utcStr);
	
	// Add position source dropdown control
	AddPositionSourceControl();
	
	// Add device type dropdown control
	AddDeviceTypeControl();
	
	// Add trawl selection dropdown control
	AddTrawlSelectionControl();
	
	// Hide Pair controls completely since we now use Trawl ID dropdown
	m_staticText11->Hide();   // "Pair:" label
	m_textCtrl121->Hide();    // Pair text control
	
	// Set initial state of ID control based on default position source
	bool enableId = (positionSource != ePOS_SOURCE_CLOUD);
	m_textCtrl12->Enable(enableId);    // ID control
	
	// Reorganize layout to move Time info to bottom
	ReorganizeLayout();

	// If editing an existing transponder, pre-populate fields
	if (existingState) {
		// Change dialog title
		SetTitle(_("Edit Transponder"));

		// Pre-populate with existing data
		uint32_t serial_num = getSerialNumber(existingState->markID);
		uint8_t mfg_code = getMfgCode(existingState->markID);

		// Set the serial number in the ID field
		m_textCtrl12->SetValue(wxString::Format("%u", serial_num));

		// Set the manufacturer dropdown
		for (int i = 0; i < m_choiceDeviceType->GetCount(); i++) {
			if (GetMfgCodeFromSelection(i) == mfg_code) {
				m_choiceDeviceType->SetSelection(i);
				deviceType = i;
				break;
			}
		}

		// Set position source
		if (m_choicePositionSource) {
			m_choicePositionSource->SetSelection(existingState->position_source);
			positionSource = existingState->position_source;
		}

		// Set trawl selection
		if (existingState->assigned_trawl_id > 0) {
			for (int i = 0; i < m_choiceTrawlId->GetCount(); i++) {
				void* clientData = m_choiceTrawlId->GetClientData(i);
				int trawlId = reinterpret_cast<intptr_t>(clientData);
				if (trawlId == existingState->assigned_trawl_id) {
					m_choiceTrawlId->SetSelection(i);
					selectedTrawlId = trawlId;
					break;
				}
			}
		}

		// Set ownership checkbox
		isOwned = (existingState->ownership == 1);
		m_checkBox1->SetValue(isOwned);

		// Store the original markID for updating
		markID = existingState->markID;

		wxLogMessage("Editing transponder: ID=%u, serial=%u, mfg=0x%02X, owned=%s",
		             existingState->markID, serial_num, mfg_code, isOwned ? "true" : "false");
	}

}

void manualPlacementDlgImpl::cancelPlaceTransponder(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void manualPlacementDlgImpl::okPlaceTransponder(wxCommandEvent& event)
{
	// Check if Cloud position source is selected
	if (positionSource == ePOS_SOURCE_CLOUD) {
		// Auto-generate cloud IDs
		extern ropeless_pi *g_ropelessPI;
		if (g_ropelessPI) {
			markID = g_ropelessPI->getNextCloudId();
			pairId = markID; // Use same ID for pair
			valid = true;
			wxLogMessage("Auto-generated cloud IDs: ID=%d, Pair=%d", markID, pairId);
		} else {
			wxLogMessage("ERROR: Cannot access plugin instance for cloud ID generation");
			valid = false;
		}
	} else {
		// Use manual entry for non-cloud positions
		long xid;
		int ownf = isOwned ? 1 : -1;

		wxString xStr = m_textCtrl12->GetValue();

		if (xStr.IsEmpty())
		{
			wxLogMessage("ID string is empty!");
			valid = false;
		}
		else
		{
			xStr.ToLong(&xid);

			// Get manufacturer code from selected device type
			uint8_t mfg_code = GetMfgCodeFromSelection(deviceType);

			// Use the entered ID as the serial number (24-bit max)
			uint32_t serial_number = static_cast<uint32_t>(abs(xid)) & 0xFFFFFF;

			// Create proper 32-bit transponder ID: [8-bit mfg][24-bit serial]
			markID = createTransponderID(mfg_code, serial_number);
			pairId = markID;  // Use same ID for pair for backward compatibility

			wxLogMessage("Manual placement: Created transponder ID %u (mfg=%u, serial=%u)",
			            markID, mfg_code, serial_number);

			valid = true;
		}
	}
	
	// Handle trawl selection
	if (valid && selectedTrawlId == -1) {
		// "New" trawl selected - create a new trawl
		extern std::vector<trawl_tracker *> trawlList;
		
		// Find next available trawl ID
		int nextTrawlId = 1;
		for (auto* trawl : trawlList) {
			if (trawl && trawl->trawl_id >= nextTrawlId) {
				nextTrawlId = trawl->trawl_id + 1;
			}
		}
		
		// Create new trawl
		trawl_tracker* newTrawl = new trawl_tracker();
		newTrawl->trawl_id = nextTrawlId;
		trawlList.push_back(newTrawl);
		
		selectedTrawlId = nextTrawlId;
		wxLogMessage("Created new trawl with ID: %d", selectedTrawlId);
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
	
	// Get selected device type
	if (m_choiceDeviceType) {
		deviceType = m_choiceDeviceType->GetSelection();
		uint8_t selected_mfg_code = GetMfgCodeFromSelection(deviceType);
		wxString deviceTypeName = getMfgString(selected_mfg_code);
		wxLogMessage("Manual placement dialog: Selected device type = %d (%s, code: 0x%02X)",
		             deviceType, deviceTypeName, selected_mfg_code);
	} else {
		wxLogMessage("Manual placement dialog: WARNING - m_choiceDeviceType is NULL");
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
    
    // Grey out ID box when CLOUD is selected
    bool enableId = (positionSource != ePOS_SOURCE_CLOUD);
    m_textCtrl12->Enable(enableId);    // ID control
}

void manualPlacementDlgImpl::AddDeviceTypeControl() {
    // Get the main sizer
    wxSizer* mainSizer = this->GetSizer();
    if (!mainSizer) return;
    
    // Create a horizontal sizer for the device type controls
    wxBoxSizer* deviceTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Add label
    wxStaticText* deviceTypeLabel = new wxStaticText(this, wxID_ANY, _("Device Type:"), 
                                                    wxDefaultPosition, wxSize(100, -1), 0);
    deviceTypeSizer->Add(deviceTypeLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    // Create the dropdown with manufacturer options
    wxArrayString choices;
    choices.Add(_("0x00 - Ropeless Systems Inc."));
    choices.Add(_("0x01 - Desert Star Systems"));
    choices.Add(_("0x02 - EdgeTech"));
    choices.Add(_("0x03 - Benthos"));
    choices.Add(_("0x04 - SubSea Sonics"));
    choices.Add(_("0x05 - Teledyne Marine"));
    choices.Add(_("0x10 - Ashored Innovations"));
    choices.Add(_("0xFF - Ephemeral"));

    m_choiceDeviceType = new wxChoice(this, wxID_ANY, wxDefaultPosition,
                                     wxSize(200, -1), choices);
    m_choiceDeviceType->SetSelection(0); // Default to Ropeless Systems Inc. (0x00)
    
    deviceTypeSizer->Add(m_choiceDeviceType, 0, wxALL, 5);
    
    // Add spacer to right-align
    deviceTypeSizer->AddStretchSpacer(1);
    
    // Insert the device type sizer before the button sizer
    // Find the button sizer (it should be the last item)
    size_t sizerCount = mainSizer->GetItemCount();
    if (sizerCount > 0) {
        // Insert before the last item (which should be the buttons)
        mainSizer->Insert(sizerCount - 1, deviceTypeSizer, 0, wxEXPAND | wxALL, 5);
    } else {
        // Fallback: just add it
        mainSizer->Add(deviceTypeSizer, 0, wxEXPAND | wxALL, 5);
    }
    
    // Connect the event handler
    m_choiceDeviceType->Connect(wxEVT_COMMAND_CHOICE_SELECTED, 
                               wxCommandEventHandler(manualPlacementDlgImpl::OnDeviceTypeChanged), 
                               NULL, this);
    
    // Refresh the layout
    this->Layout();
    this->Fit();
}

void manualPlacementDlgImpl::OnDeviceTypeChanged(wxCommandEvent& event) {
    deviceType = m_choiceDeviceType->GetSelection();
    uint8_t mfg_code = GetMfgCodeFromSelection(deviceType);
    wxString deviceTypeName = getMfgString(mfg_code);
    wxLogMessage("Device type changed to: %s (code: 0x%02X)", deviceTypeName, mfg_code);
}

void manualPlacementDlgImpl::AddTrawlSelectionControl() {
    // Get the main sizer
    wxSizer* mainSizer = this->GetSizer();
    if (!mainSizer) return;
    
    // Create a horizontal sizer for the trawl selection controls
    wxBoxSizer* trawlSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Add label
    wxStaticText* trawlLabel = new wxStaticText(this, wxID_ANY, _("Trawl ID:"), 
                                               wxDefaultPosition, wxSize(100, -1), 0);
    trawlSizer->Add(trawlLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    // Create the dropdown with trawl options
    m_choiceTrawlId = new wxChoice(this, wxID_ANY, wxDefaultPosition, 
                                  wxSize(120, -1));
    
    // Populate the dropdown
    PopulateTrawlDropdown();
    
    trawlSizer->Add(m_choiceTrawlId, 0, wxALL, 5);
    
    // Add spacer to right-align
    trawlSizer->AddStretchSpacer(1);
    
    // Insert the trawl sizer before the button sizer
    // Find the button sizer (it should be the last item)
    size_t sizerCount = mainSizer->GetItemCount();
    if (sizerCount > 0) {
        // Insert before the last item (which should be the buttons)
        mainSizer->Insert(sizerCount - 1, trawlSizer, 0, wxEXPAND | wxALL, 5);
    } else {
        // Fallback: just add it
        mainSizer->Add(trawlSizer, 0, wxEXPAND | wxALL, 5);
    }
    
    // Connect the event handler
    m_choiceTrawlId->Connect(wxEVT_COMMAND_CHOICE_SELECTED, 
                            wxCommandEventHandler(manualPlacementDlgImpl::OnTrawlSelectionChanged), 
                            NULL, this);
    
    // Refresh the layout
    this->Layout();
    this->Fit();
}

void manualPlacementDlgImpl::PopulateTrawlDropdown() {
    if (!m_choiceTrawlId) return;
    
    // Clear existing items
    m_choiceTrawlId->Clear();
    
    // Add "None" and "New" options at the top
    m_choiceTrawlId->Append(_("None"), reinterpret_cast<void*>(0));
    m_choiceTrawlId->Append(_("New"), reinterpret_cast<void*>(-1));
    
    // Add existing trawls from the global trawl list
    extern std::vector<trawl_tracker *> trawlList;
    for (auto* trawl : trawlList) {
        if (trawl) {
            wxString trawlText = wxString::Format(_("Trawl %d"), trawl->trawl_id);
            m_choiceTrawlId->Append(trawlText, reinterpret_cast<void*>(trawl->trawl_id));
        }
    }
    
    // Default selection to "None"
    m_choiceTrawlId->SetSelection(0);
    selectedTrawlId = 0;
}

void manualPlacementDlgImpl::OnTrawlSelectionChanged(wxCommandEvent& event) {
    int selection = m_choiceTrawlId->GetSelection();
    if (selection == wxNOT_FOUND) return;
    
    void* clientData = m_choiceTrawlId->GetClientData(selection);
    selectedTrawlId = reinterpret_cast<intptr_t>(clientData);
    
    wxString selectionText;
    if (selectedTrawlId == 0) {
        selectionText = "None";
    } else if (selectedTrawlId == -1) {
        selectionText = "New";
    } else {
        selectionText = wxString::Format("Trawl %d", selectedTrawlId);
    }
    
    wxLogMessage("Trawl selection changed to: %s (ID: %d)", selectionText, selectedTrawlId);
}

uint8_t manualPlacementDlgImpl::GetMfgCodeFromSelection(int selection) {
	switch (selection) {
		case 0: return 0x00; // Ropeless Systems Inc.
		case 1: return 0x01; // Desert Star Systems
		case 2: return 0x02; // EdgeTech
		case 3: return 0x03; // Benthos
		case 4: return 0x04; // SubSea Sonics
		case 5: return 0x05; // Teledyne Marine
		case 6: return 0x10; // Ashored Innovations
		case 7: return 0xFF; // Ephemeral
		default: return 0x01; // Default to Desert Star Systems
	}
}

void manualPlacementDlgImpl::ReorganizeLayout() {
    // Get the main sizer
    wxSizer* mainSizer = this->GetSizer();
    if (!mainSizer) return;
    
    // Find the Time sizer (contains m_staticText10 and m_staticText111)
    wxSizer* timeSizer = nullptr;
    size_t timeIndex = 0;
    
    // Look for the sizer containing the Time controls
    for (size_t i = 0; i < mainSizer->GetItemCount(); ++i) {
        wxSizerItem* item = mainSizer->GetItem(i);
        if (item && item->IsSizer()) {
            wxSizer* sizer = item->GetSizer();
            if (sizer) {
                // Check if this sizer contains the Time label
                for (size_t j = 0; j < sizer->GetItemCount(); ++j) {
                    wxSizerItem* subItem = sizer->GetItem(j);
                    if (subItem && subItem->IsWindow()) {
                        wxWindow* window = subItem->GetWindow();
                        if (window == m_staticText10) {  // "Time:" label
                            timeSizer = sizer;
                            timeIndex = i;
                            break;
                        }
                    }
                }
                if (timeSizer) break;
            }
        }
    }
    
    if (timeSizer) {
        // Detach the time sizer from its current position
        mainSizer->Detach(timeSizer);
        
        // Find the button sizer (should be the last item now)
        size_t buttonIndex = mainSizer->GetItemCount() - 1;
        
        // Insert the time sizer before the button sizer
        mainSizer->Insert(buttonIndex, timeSizer, 0, wxEXPAND | wxALL, 5);
        
        // Refresh the layout
        this->Layout();
        this->Fit();
    }
}
