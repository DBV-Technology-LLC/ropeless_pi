/******************************************************************************
 * $Id:
 *
 * Project:  OpenCPN
 * Purpose:  Ropeless Plugin Preferences Dialog Header
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

#ifndef _ROPELESSPREFSDIALOG_H_
#define _ROPELESSPREFSDIALOG_H_

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/sizer.h>

class ropeless_pi;

class RopelessPrefsDialog : public wxDialog {
private:
    ropeless_pi *m_parent_pi;
    
    // TCP NMEA Output controls
    wxStaticBox *m_tcpBox;
    wxCheckBox *m_cbTcpEnabled;
    wxTextCtrl *m_tcTcpHost;
    wxSpinCtrl *m_scTcpPort;
    wxCheckBox *m_cbTcpAutoReconnect;
    
    // Standard dialog buttons
    wxStdDialogButtonSizer *m_sdbSizer;
    wxButton *m_sdbSizerOK;
    wxButton *m_sdbSizerCancel;
    
    void CreateControls();
    void SetSizer();

protected:
public:
    RopelessPrefsDialog(wxWindow *parent, ropeless_pi *parent_pi, 
                       wxWindowID id = wxID_ANY,
                       const wxString &title = _("Ropeless Preferences"),
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    ~RopelessPrefsDialog();

    void OnOKClick(wxCommandEvent &event);
    void OnCancelClick(wxCommandEvent &event);
    void OnClose(wxCloseEvent &event);
    void OnTcpEnabledClick(wxCommandEvent &event);
    
    void LoadSettings();
    void SaveSettings();
    void UpdateTcpControls();

    DECLARE_EVENT_TABLE()
};

#endif // _ROPELESSPREFSDIALOG_H_