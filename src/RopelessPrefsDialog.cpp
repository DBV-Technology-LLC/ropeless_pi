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

enum {
    ID_TCP_ENABLED = 1000
};

wxBEGIN_EVENT_TABLE(RopelessPrefsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, RopelessPrefsDialog::OnOKClick)
    EVT_BUTTON(wxID_CANCEL, RopelessPrefsDialog::OnCancelClick)
    EVT_CLOSE(RopelessPrefsDialog::OnClose)
    EVT_CHECKBOX(ID_TCP_ENABLED, RopelessPrefsDialog::OnTcpEnabledClick)
wxEND_EVENT_TABLE()

RopelessPrefsDialog::RopelessPrefsDialog(wxWindow *parent, ropeless_pi *parent_pi,
                                       wxWindowID id, const wxString &title,
                                       const wxPoint &pos, const wxSize &size,
                                       long style)
    : wxDialog(parent, id, title, pos, size, style), m_parent_pi(parent_pi) {
    
    CreateControls();
    SetSizer();
    LoadSettings();
    UpdateTcpControls();
    
    // Set dialog colors to match OpenCPN theme
    wxColour cl;
    GetGlobalColor(_T("DILG1"), &cl);
    SetBackgroundColour(cl);
    
    Fit();
    Centre();
}

RopelessPrefsDialog::~RopelessPrefsDialog() {
}

void RopelessPrefsDialog::CreateControls() {
    // TCP NMEA Output section
    m_tcpBox = new wxStaticBox(this, wxID_ANY, _("TCP NMEA Output"));
    
    m_cbTcpEnabled = new wxCheckBox(this, ID_TCP_ENABLED, _("Enable TCP NMEA Output"));
    
    wxStaticText *hostLabel = new wxStaticText(this, wxID_ANY, _("Host:"));
    m_tcTcpHost = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(150, -1));
    
    wxStaticText *portLabel = new wxStaticText(this, wxID_ANY, _("Port:"));
    m_scTcpPort = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1),
                                wxSP_ARROW_KEYS, 1, 65535, 4001);
    
    m_cbTcpAutoReconnect = new wxCheckBox(this, wxID_ANY, _("Auto-reconnect on connection loss"));
    
    // Standard dialog buttons
    m_sdbSizer = new wxStdDialogButtonSizer();
    m_sdbSizerOK = new wxButton(this, wxID_OK);
    m_sdbSizerCancel = new wxButton(this, wxID_CANCEL);
    m_sdbSizer->AddButton(m_sdbSizerOK);
    m_sdbSizer->AddButton(m_sdbSizerCancel);
    m_sdbSizer->Realize();
}

void RopelessPrefsDialog::SetSizer() {
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // TCP section
    wxStaticBoxSizer *tcpSizer = new wxStaticBoxSizer(m_tcpBox, wxVERTICAL);
    
    tcpSizer->Add(m_cbTcpEnabled, 0, wxALL, 5);
    
    // Host and port on same line
    wxBoxSizer *hostPortSizer = new wxBoxSizer(wxHORIZONTAL);
    hostPortSizer->Add(new wxStaticText(this, wxID_ANY, _("Host:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    hostPortSizer->Add(m_tcTcpHost, 1, wxALL, 5);
    hostPortSizer->Add(new wxStaticText(this, wxID_ANY, _("Port:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    hostPortSizer->Add(m_scTcpPort, 0, wxALL, 5);
    
    tcpSizer->Add(hostPortSizer, 0, wxEXPAND | wxALL, 5);
    tcpSizer->Add(m_cbTcpAutoReconnect, 0, wxALL, 5);
    
    mainSizer->Add(tcpSizer, 0, wxEXPAND | wxALL, 10);
    
    // Add some spacing before buttons
    mainSizer->AddSpacer(10);
    
    // Dialog buttons
    mainSizer->Add(m_sdbSizer, 0, wxEXPAND | wxALL, 10);
    
    wxDialog::SetSizer(mainSizer);
}

void RopelessPrefsDialog::LoadSettings() {
    if (!m_parent_pi) return;
    
    // Load TCP settings
    m_cbTcpEnabled->SetValue(m_parent_pi->m_tcp_enabled);
    m_tcTcpHost->SetValue(m_parent_pi->m_tcp_host);
    m_scTcpPort->SetValue(m_parent_pi->m_tcp_port);
    m_cbTcpAutoReconnect->SetValue(m_parent_pi->m_tcp_auto_reconnect);
}

void RopelessPrefsDialog::SaveSettings() {
    if (!m_parent_pi) return;
    
    // Save TCP settings
    m_parent_pi->m_tcp_enabled = m_cbTcpEnabled->GetValue();
    m_parent_pi->m_tcp_host = m_tcTcpHost->GetValue();
    m_parent_pi->m_tcp_port = m_scTcpPort->GetValue();
    m_parent_pi->m_tcp_auto_reconnect = m_cbTcpAutoReconnect->GetValue();
    
    // Configure TCP output with new settings
    m_parent_pi->ConfigureTCPOutput(m_parent_pi->m_tcp_host, m_parent_pi->m_tcp_port, 
                                   m_parent_pi->m_tcp_enabled, m_parent_pi->m_tcp_auto_reconnect);
    
    // Save to config file
    m_parent_pi->SaveConfig();
}

void RopelessPrefsDialog::UpdateTcpControls() {
    bool enabled = m_cbTcpEnabled->GetValue();
    
    m_tcTcpHost->Enable(enabled);
    m_scTcpPort->Enable(enabled);
    m_cbTcpAutoReconnect->Enable(enabled);
}

void RopelessPrefsDialog::OnOKClick(wxCommandEvent &event) {
    SaveSettings();
    EndModal(wxID_OK);
}

void RopelessPrefsDialog::OnCancelClick(wxCommandEvent &event) {
    EndModal(wxID_CANCEL);
}

void RopelessPrefsDialog::OnClose(wxCloseEvent &event) {
    EndModal(wxID_CANCEL);
}

void RopelessPrefsDialog::OnTcpEnabledClick(wxCommandEvent &event) {
    UpdateTcpControls();
}