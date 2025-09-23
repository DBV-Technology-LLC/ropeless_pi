/******************************************************************************
 * $Id:
 *
 * Project:  OpenCPN
 * Purpose:  Ropeless Plugin Preferences Dialog Implementation
 * Author:   dsr
 *
 ***************************************************************************
 *   Copyright (C) 2016 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "RopelessPrefsDialog.h"
#include "ropeless_pi.h"

#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/sizer.h>

enum {
    ID_TCP_ENABLED = 1000,
    ID_DEBUG_ENABLED,
    ID_DEFAULT_SETTINGS
};

wxBEGIN_EVENT_TABLE(RopelessPrefsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, RopelessPrefsDialog::OnOKClick)
    EVT_BUTTON(wxID_CANCEL, RopelessPrefsDialog::OnCancelClick)
    EVT_BUTTON(ID_DEFAULT_SETTINGS, RopelessPrefsDialog::OnDefaultSettingsClick)
    EVT_CHECKBOX(ID_TCP_ENABLED, RopelessPrefsDialog::OnTCPEnabledClick)
    EVT_CHECKBOX(ID_DEBUG_ENABLED, RopelessPrefsDialog::OnDebugEnabledClick)
wxEND_EVENT_TABLE()

RopelessPrefsDialog::RopelessPrefsDialog(ropeless_pi *parent_pi, wxWindow *parent,
                                       wxWindowID id, const wxString &title,
                                       const wxPoint &pos, const wxSize &size,
                                       long style)
    : wxDialog(parent, id, title, pos, size, style), m_parent_pi(parent_pi) {
    
    CreateControls();
    Fit();
    Centre();
}

RopelessPrefsDialog::~RopelessPrefsDialog() {
    // Mark plugin pointer as invalid during destruction
    m_parent_pi = nullptr;
}

void RopelessPrefsDialog::CreateControls() {
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Accessibility section
    wxStaticBox *accessibilityBox = new wxStaticBox(this, wxID_ANY, _("Accessibility"));
    wxStaticBoxSizer *accessibilitySizer = new wxStaticBoxSizer(accessibilityBox, wxVERTICAL);
    
    m_cbColorblind = new wxCheckBox(this, wxID_ANY, _("Colorblind mode"));
    if (m_parent_pi) {
        m_cbColorblind->SetValue(m_parent_pi->m_colorblind_mode);
    }
    accessibilitySizer->Add(m_cbColorblind, 0, wxALL, 5);
    
    mainSizer->Add(accessibilitySizer, 0, wxEXPAND | wxALL, 10);
    
    // TCP NMEA Output section
    wxStaticBox *tcpBox = new wxStaticBox(this, wxID_ANY, _("TCP NMEA Output"));
    wxStaticBoxSizer *tcpSizer = new wxStaticBoxSizer(tcpBox, wxVERTICAL);
    
    m_cbTCPEnabled = new wxCheckBox(this, ID_TCP_ENABLED, _("Enable TCP Output"));
    if (m_parent_pi) {
        m_cbTCPEnabled->SetValue(m_parent_pi->m_tcp_enabled);
    }
    tcpSizer->Add(m_cbTCPEnabled, 0, wxALL, 5);
    
    // TCP Host
    wxBoxSizer *hostSizer = new wxBoxSizer(wxHORIZONTAL);
    hostSizer->Add(new wxStaticText(this, wxID_ANY, _("Host:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_tcTCPHost = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1));
    if (m_parent_pi) {
        m_tcTCPHost->SetValue(m_parent_pi->m_tcp_host);
    }
    hostSizer->Add(m_tcTCPHost, 1, wxEXPAND);
    tcpSizer->Add(hostSizer, 0, wxEXPAND | wxALL, 5);
    
    // TCP Port
    wxBoxSizer *portSizer = new wxBoxSizer(wxHORIZONTAL);
    portSizer->Add(new wxStaticText(this, wxID_ANY, _("Port:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_tcTCPPort = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1));
    if (m_parent_pi) {
        m_tcTCPPort->SetValue(wxString::Format("%d", m_parent_pi->m_tcp_port));
    }
    portSizer->Add(m_tcTCPPort, 0);
    tcpSizer->Add(portSizer, 0, wxEXPAND | wxALL, 5);
    
    m_cbTCPAutoReconnect = new wxCheckBox(this, wxID_ANY, _("Auto-reconnect"));
    if (m_parent_pi) {
        m_cbTCPAutoReconnect->SetValue(m_parent_pi->m_tcp_auto_reconnect);
    }
    tcpSizer->Add(m_cbTCPAutoReconnect, 0, wxALL, 5);
    
    mainSizer->Add(tcpSizer, 0, wxEXPAND | wxALL, 10);
    
    // Advanced Options section
    wxStaticBox *advancedBox = new wxStaticBox(this, wxID_ANY, _("Advanced Options"));
    wxStaticBoxSizer *advancedSizer = new wxStaticBoxSizer(advancedBox, wxVERTICAL);
    
    m_cbDebugEnabled = new wxCheckBox(this, ID_DEBUG_ENABLED, _("Enable Debug"));
    if (m_parent_pi) {
        m_cbDebugEnabled->SetValue(m_parent_pi->m_debug_enabled);
    }
    advancedSizer->Add(m_cbDebugEnabled, 0, wxALL, 5);
    
    m_cbSimulationEnabled = new wxCheckBox(this, wxID_ANY, _("Enable Simulation"));
    advancedSizer->Add(m_cbSimulationEnabled, 0, wxALL, 5);
    
    mainSizer->Add(advancedSizer, 0, wxEXPAND | wxALL, 10);
    
    // Display Options section
    wxStaticBox *displayBox = new wxStaticBox(this, wxID_ANY, _("Display Options"));
    wxStaticBoxSizer *displaySizer = new wxStaticBoxSizer(displayBox, wxVERTICAL);
    
    m_cbShowNonOwned = new wxCheckBox(this, wxID_ANY, _("Show non-owned in list"));
    displaySizer->Add(m_cbShowNonOwned, 0, wxALL, 5);
    
    m_cbShowCloud = new wxCheckBox(this, wxID_ANY, _("Show cloud in list"));
    displaySizer->Add(m_cbShowCloud, 0, wxALL, 5);
    
    m_cbHideRecovered = new wxCheckBox(this, wxID_ANY, _("Hide recovered units"));
    displaySizer->Add(m_cbHideRecovered, 0, wxALL, 5);
    
    m_cbTimeoutCloud = new wxCheckBox(this, wxID_ANY, _("Timeout cloud positions"));
    displaySizer->Add(m_cbTimeoutCloud, 0, wxALL, 5);
    
    // Cloud radius text input
    wxBoxSizer *radiusSizer = new wxBoxSizer(wxHORIZONTAL);
    radiusSizer->Add(new wxStaticText(this, wxID_ANY, _("Cloud radius (nmi):")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_tcCloudRadius = new wxTextCtrl(this, wxID_ANY, _("2"), wxDefaultPosition, wxSize(100, -1));
    radiusSizer->Add(m_tcCloudRadius, 0);
    displaySizer->Add(radiusSizer, 0, wxEXPAND | wxALL, 5);
    
    // // Visual appearance settings
    // wxStaticText *visualLabel = new wxStaticText(this, wxID_ANY, _("Visual Appearance:"));
    // displaySizer->Add(visualLabel, 0, wxALL | wxALIGN_LEFT, 5);
    
    // Circle size setting
    wxBoxSizer *circleSizer = new wxBoxSizer(wxHORIZONTAL);
    circleSizer->Add(new wxStaticText(this, wxID_ANY, _("Circle size:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_tcCircleSize = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
    if (m_parent_pi) {
        m_tcCircleSize->SetValue(wxString::Format("%d", m_parent_pi->m_transponder_circle_size));
    }
    circleSizer->Add(m_tcCircleSize, 0);
    displaySizer->Add(circleSizer, 0, wxEXPAND | wxALL, 5);
    
    // Text size setting  
    wxBoxSizer *textSizer = new wxBoxSizer(wxHORIZONTAL);
    textSizer->Add(new wxStaticText(this, wxID_ANY, _("Text size:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_tcTextSize = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
    if (m_parent_pi) {
        m_tcTextSize->SetValue(wxString::Format("%d", m_parent_pi->m_transponder_text_size));
    }
    textSizer->Add(m_tcTextSize, 0);
    displaySizer->Add(textSizer, 0, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(displaySizer, 0, wxEXPAND | wxALL, 10);
    
    // Default Settings button
    wxButton *defaultButton = new wxButton(this, ID_DEFAULT_SETTINGS, _("Default Settings"));
    mainSizer->Add(defaultButton, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Standard dialog buttons
    wxStdDialogButtonSizer *buttonSizer = new wxStdDialogButtonSizer();
    wxButton *okButton = new wxButton(this, wxID_OK);
    wxButton *cancelButton = new wxButton(this, wxID_CANCEL);
    buttonSizer->AddButton(okButton);
    buttonSizer->AddButton(cancelButton);
    buttonSizer->Realize();
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
    
    // Set initial control states
    UpdateTCPControls();
    UpdateDebugControls();
}

void RopelessPrefsDialog::OnOKClick(wxCommandEvent &event) {
    // Save all settings
    if (m_parent_pi) {
        try {
            // Save colorblind mode
            if (m_cbColorblind) {
                m_parent_pi->m_colorblind_mode = m_cbColorblind->GetValue();
            }
            
            // Save TCP settings
            if (m_cbTCPEnabled && m_tcTCPHost && m_tcTCPPort && m_cbTCPAutoReconnect) {
                bool tcp_enabled = m_cbTCPEnabled->GetValue();
                wxString tcp_host = m_tcTCPHost->GetValue();
                long tcp_port_long;
                m_tcTCPPort->GetValue().ToLong(&tcp_port_long);
                int tcp_port = (int)tcp_port_long;
                bool tcp_auto_reconnect = m_cbTCPAutoReconnect->GetValue();
                
                // Apply the new TCP settings
                m_parent_pi->ConfigureTCPOutput(tcp_host, tcp_port, tcp_enabled, tcp_auto_reconnect);
            }
            
            // Save advanced settings
            if (m_cbDebugEnabled) {
                m_parent_pi->m_debug_enabled = m_cbDebugEnabled->GetValue();
            }
            
            // Save visual settings
            if (m_tcCircleSize) {
                long circle_size;
                if (m_tcCircleSize->GetValue().ToLong(&circle_size) && circle_size >= 5 && circle_size <= 30) {
                    m_parent_pi->m_transponder_circle_size = (int)circle_size;
                }
            }
            if (m_tcTextSize) {
                long text_size;
                if (m_tcTextSize->GetValue().ToLong(&text_size) && text_size >= 6 && text_size <= 24) {
                    m_parent_pi->m_transponder_text_size = (int)text_size;
                }
            }
            
            // Save to config file
            m_parent_pi->SaveConfig();
        } catch (...) {
            // Ignore any exceptions
        }
    }
    EndModal(wxID_OK);
}

void RopelessPrefsDialog::UpdateDebugControls() {
    // No dependent controls to update for debug section anymore
}

void RopelessPrefsDialog::OnDebugEnabledClick(wxCommandEvent &event) {
    UpdateDebugControls();
}

void RopelessPrefsDialog::OnTCPEnabledClick(wxCommandEvent &event) {
    UpdateTCPControls();
}

void RopelessPrefsDialog::UpdateTCPControls() {
    if (!m_cbTCPEnabled || !m_tcTCPHost || !m_tcTCPPort || !m_cbTCPAutoReconnect) return;
    
    bool enabled = m_cbTCPEnabled->GetValue();
    m_tcTCPHost->Enable(enabled);
    m_tcTCPPort->Enable(enabled);
    m_cbTCPAutoReconnect->Enable(enabled);
}

void RopelessPrefsDialog::OnDefaultSettingsClick(wxCommandEvent &event) {
    // Set all controls to their default values
    if (m_cbColorblind) m_cbColorblind->SetValue(false);
    if (m_cbTCPEnabled) m_cbTCPEnabled->SetValue(false);
    if (m_cbTCPAutoReconnect) m_cbTCPAutoReconnect->SetValue(true);
    if (m_tcTCPHost) m_tcTCPHost->SetValue(_("localhost"));
    if (m_tcTCPPort) m_tcTCPPort->SetValue(_("10110"));
    if (m_cbDebugEnabled) m_cbDebugEnabled->SetValue(false);
    if (m_cbSimulationEnabled) m_cbSimulationEnabled->SetValue(false);
    if (m_cbShowNonOwned) m_cbShowNonOwned->SetValue(true);
    if (m_cbShowCloud) m_cbShowCloud->SetValue(true);
    if (m_cbHideRecovered) m_cbHideRecovered->SetValue(false);
    if (m_cbTimeoutCloud) m_cbTimeoutCloud->SetValue(true);
    if (m_tcCloudRadius) m_tcCloudRadius->SetValue(_("2"));
    if (m_tcCircleSize) m_tcCircleSize->SetValue(_("10"));
    if (m_tcTextSize) m_tcTextSize->SetValue(_("12"));
    
    // Update control states
    UpdateTCPControls();
    UpdateDebugControls();
}

void RopelessPrefsDialog::OnCancelClick(wxCommandEvent &event) {
    EndModal(wxID_CANCEL);
}