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
#include <wx/spinctrl.h>

class ropeless_pi;

class RopelessPrefsDialog : public wxDialog {
public:
    RopelessPrefsDialog(ropeless_pi *parent_pi, wxWindow *parent,
                       wxWindowID id = wxID_ANY,
                       const wxString &title = _("Ropeless Preferences"),
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    ~RopelessPrefsDialog();

private:
    ropeless_pi *m_parent_pi;
    
    // Colorblind mode setting
    wxCheckBox *m_cbColorblind;
    // TCP NMEA Output settings
    wxCheckBox *m_cbTCPEnabled;
    wxCheckBox *m_cbTCPAutoReconnect;
    wxTextCtrl *m_tcTCPHost;
    wxTextCtrl *m_tcTCPPort;
    // Advanced options
    wxCheckBox *m_cbDebugEnabled;
    wxCheckBox *m_cbSimulationEnabled;
    // Display options
    wxCheckBox *m_cbShowNonOwned;
    wxCheckBox *m_cbShowCloud;
    wxCheckBox *m_cbHideRecovered;
    wxCheckBox *m_cbTimeoutCloud;
    wxCheckBox *m_cbHideTransponderText;
    wxTextCtrl *m_tcCloudRadius;
    // Visual settings
    wxTextCtrl *m_tcCircleSize;
    wxTextCtrl *m_tcTextSize;
    
    void CreateControls();
    void OnOKClick(wxCommandEvent &event);
    void OnCancelClick(wxCommandEvent &event);
    void OnDefaultSettingsClick(wxCommandEvent &event);
    void OnTCPEnabledClick(wxCommandEvent &event);
    void OnDebugEnabledClick(wxCommandEvent &event);
    void UpdateDebugControls();
    void UpdateTCPControls();

    DECLARE_EVENT_TABLE()
};

#endif // _ROPELESSPREFSDIALOG_H_