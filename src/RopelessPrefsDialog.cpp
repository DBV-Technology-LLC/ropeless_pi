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
    ID_TEST_CHECKBOX = 1000,
    ID_TCP_ENABLED,
    ID_DEBUG_ENABLED
};

wxBEGIN_EVENT_TABLE(RopelessPrefsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, RopelessPrefsDialog::OnOKClick)
    EVT_BUTTON(wxID_CANCEL, RopelessPrefsDialog::OnCancelClick)
    EVT_CHECKBOX(ID_TEST_CHECKBOX, RopelessPrefsDialog::OnTestCheckbox)
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
    
    // Test checkbox (keeping it for now)
    m_cbTest = new wxCheckBox(this, ID_TEST_CHECKBOX, _("Test checkbox (WITH event)"));
    m_cbTest->SetValue(true);  // Set to true by default
    mainSizer->Add(m_cbTest, 0, wxALL, 20);
    
    // Colorblind mode checkbox - load from plugin setting
    m_cbColorblind = new wxCheckBox(this, wxID_ANY, _("Colorblind mode"));
    if (m_parent_pi) {
        m_cbColorblind->SetValue(m_parent_pi->m_colorblind_mode);
    }
    mainSizer->Add(m_cbColorblind, 0, wxALL, 5);
    
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
    m_scTCPPort = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1));
    m_scTCPPort->SetRange(1, 65535);
    if (m_parent_pi) {
        m_scTCPPort->SetValue(m_parent_pi->m_tcp_port);
    }
    portSizer->Add(m_scTCPPort, 0);
    tcpSizer->Add(portSizer, 0, wxEXPAND | wxALL, 5);
    
    m_cbTCPAutoReconnect = new wxCheckBox(this, wxID_ANY, _("Auto-reconnect"));
    if (m_parent_pi) {
        m_cbTCPAutoReconnect->SetValue(m_parent_pi->m_tcp_auto_reconnect);
    }
    tcpSizer->Add(m_cbTCPAutoReconnect, 0, wxALL, 5);
    
    mainSizer->Add(tcpSizer, 0, wxEXPAND | wxALL, 10);
    
    // Debug section
    wxStaticBox *debugBox = new wxStaticBox(this, wxID_ANY, _("Debug Options"));
    wxStaticBoxSizer *debugSizer = new wxStaticBoxSizer(debugBox, wxVERTICAL);
    
    m_cbDebugEnabled = new wxCheckBox(this, ID_DEBUG_ENABLED, _("Enable Debug"));
    if (m_parent_pi) {
        m_cbDebugEnabled->SetValue(m_parent_pi->m_debug_enabled);
    }
    debugSizer->Add(m_cbDebugEnabled, 0, wxALL, 5);
    
    m_cbShowNMEA = new wxCheckBox(this, wxID_ANY, _("Show NMEA"));
    if (m_parent_pi) {
        m_cbShowNMEA->SetValue(m_parent_pi->m_debug_show_nmea);
    }
    debugSizer->Add(m_cbShowNMEA, 0, wxALL, 5);
    
    m_cbShowLog = new wxCheckBox(this, wxID_ANY, _("Show Log"));
    if (m_parent_pi) {
        m_cbShowLog->SetValue(m_parent_pi->m_debug_show_log);
    }
    debugSizer->Add(m_cbShowLog, 0, wxALL, 5);
    
    mainSizer->Add(debugSizer, 0, wxEXPAND | wxALL, 10);
    
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
            if (m_cbTCPEnabled && m_tcTCPHost && m_scTCPPort && m_cbTCPAutoReconnect) {
                bool tcp_enabled = m_cbTCPEnabled->GetValue();
                wxString tcp_host = m_tcTCPHost->GetValue();
                int tcp_port = m_scTCPPort->GetValue();
                bool tcp_auto_reconnect = m_cbTCPAutoReconnect->GetValue();
                
                // Apply the new TCP settings
                m_parent_pi->ConfigureTCPOutput(tcp_host, tcp_port, tcp_enabled, tcp_auto_reconnect);
            }
            
            // Save debug settings
            if (m_cbDebugEnabled) {
                m_parent_pi->m_debug_enabled = m_cbDebugEnabled->GetValue();
            }
            if (m_cbShowNMEA) {
                m_parent_pi->m_debug_show_nmea = m_cbShowNMEA->GetValue();
            }
            if (m_cbShowLog) {
                m_parent_pi->m_debug_show_log = m_cbShowLog->GetValue();
            }
            
            // Save to config file
            m_parent_pi->SaveConfig();
        } catch (...) {
            // Ignore any exceptions
        }
    }
    EndModal(wxID_OK);
}

void RopelessPrefsDialog::OnTestCheckbox(wxCommandEvent &event) {
    // Simple event handler - do nothing
    // Just testing if having an event handler causes crashes
}

void RopelessPrefsDialog::UpdateDebugControls() {
    if (!m_cbDebugEnabled || !m_cbShowNMEA || !m_cbShowLog) return;
    
    bool enabled = m_cbDebugEnabled->GetValue();
    m_cbShowNMEA->Enable(enabled);
    m_cbShowLog->Enable(enabled);
}

void RopelessPrefsDialog::OnDebugEnabledClick(wxCommandEvent &event) {
    UpdateDebugControls();
}

void RopelessPrefsDialog::OnTCPEnabledClick(wxCommandEvent &event) {
    UpdateTCPControls();
}

void RopelessPrefsDialog::UpdateTCPControls() {
    if (!m_cbTCPEnabled || !m_tcTCPHost || !m_scTCPPort || !m_cbTCPAutoReconnect) return;
    
    bool enabled = m_cbTCPEnabled->GetValue();
    m_tcTCPHost->Enable(enabled);
    m_scTCPPort->Enable(enabled);
    m_cbTCPAutoReconnect->Enable(enabled);
}

void RopelessPrefsDialog::OnCancelClick(wxCommandEvent &event) {
    EndModal(wxID_CANCEL);
}