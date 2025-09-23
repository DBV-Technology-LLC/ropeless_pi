/******************************************************************************
 * $Id:
 *
 * Project:  OpenCPN
 * Purpose:  Ropeless Dialog Header
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

#ifndef _ROPELESSDIALOG_H_
#define _ROPELESSDIALOG_H_

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/notebook.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>

class ropeless_pi;
class OCPNListCtrl;
struct transponder_state;


class RopelessDialog : public wxDialog {
private:
protected:
  wxStdDialogButtonSizer *m_sdbSizer1;
  wxButton *m_sdbSizer1OK;
  wxButton *m_sdbSizer1Cancel;
  wxButton *m_sdbSizer1Help;

public:
  wxComboBox *m_comboPort;
  wxArrayString *m_pSerialArray;

  wxComboBox *m_wpComboPort;

  wxString m_trackedPointName;
  wxString m_trackedPointGUID;

  wxComboBox *m_comboIcon;
  wxTextCtrl *m_pTenderGPSOffsetX;
  wxTextCtrl *m_pTenderGPSOffsetY;
  wxTextCtrl *m_pTenderLength;
  wxTextCtrl *m_pTenderWidth;

  wxButton *m_ManualReleaseButton, *m_SyncButton;

  
  // Sidebar components
  wxStaticText *m_selectedTransponderLabel;
  wxNotebook *m_transponderInfoNotebook;
  wxPanel *m_infoPanel;
  wxPanel *m_statusPanel;
  wxPanel *m_positionPanel;
  
  // Debug box
  wxTextCtrl *m_debugTextCtrl;
  wxButton *m_clearDebugButton;
  wxCheckBox *m_showNmeaCheckbox;
  wxCheckBox *m_showDebugCheckbox;
  
  // Command buttons
  wxButton *m_releaseButton;
  wxButton *m_recoverButton;
  wxButton *m_deleteButton;
  wxButton *m_muteButton;
  wxButton *m_sidebarSyncButton;
  wxButton *m_showOnMapButton;
  
  // Status boxes
  wxStaticBoxSizer *m_deckboxStatusSizer;
  wxStaticText *m_connectionStatusText;
  wxStaticText *m_deviceIdStatusText;
  wxStaticText *m_acousticStatusText;
  wxStaticText *m_cloudStatusText;
  wxStaticBoxSizer *m_releaseStatusSizer;
  wxStaticText *m_releaseStatusIdText;
  wxStaticText *m_releaseStatusText;
  
  // Release Status buttons (moved from transponderReleaseDlg)
  wxButton *m_markRecoveredButton;
  wxButton *m_retryReleaseButton;

  ropeless_pi *pParentPi;
  OCPNListCtrl *m_pListCtrlTranponders;
  
  // Selected transponder tracking
  transponder_state *m_selectedTransponder;
  
  // Info tab controls
  wxStaticText *m_infoIdText;
  wxStaticText *m_infoPartnerIdText;
  wxStaticText *m_infoManufacturerText;
  wxStaticText *m_infoSerialNumberText;
  wxStaticText *m_infoOwnershipText;
  wxStaticText *m_infoTrawlIdText;
  wxStaticText *m_infoMarkTypeText;
  
  // Status tab controls
  wxStaticText *m_statusReleaseText;
  wxStaticText *m_statusRecoveryText;
  wxStaticText *m_statusBatteryText;
  wxStaticText *m_statusPingsText;
  wxStaticText *m_statusLastReportText;
  wxStaticText *m_statusPositionSourceText;
  
  // Position tab controls
  wxStaticText *m_positionLatText;
  wxStaticText *m_positionLonText;
  wxStaticText *m_positionRangeText;
  wxStaticText *m_positionBearingText;
  wxStaticText *m_positionDepthText;
  wxStaticText *m_positionTempText;

  RopelessDialog(wxWindow *parent, ropeless_pi *parent_pi,
                 wxWindowID id = wxID_ANY,
                 const wxString &title = _("Ropeless"),
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = wxCAPTION | wxDEFAULT_DIALOG_STYLE);
  virtual ~RopelessDialog();

  void OnOKClick(wxCommandEvent &event);
  void OnHelpClick(wxCommandEvent &event);
  void OnClose(wxCloseEvent &event);
  void OnChooseFileButton(wxCommandEvent &event);
  void OnStopSimButton(wxCommandEvent &event);
  void OnStartSimButton(wxCommandEvent &event);
  void OnManualReleaseButton(wxCommandEvent &event);
  void RefreshTransponderList();
  void OnTargetListSelected(wxListEvent &event);
  void OnTargetListDeselected(wxListEvent &event);
  void OnTargetListColumnClicked(wxListEvent &event);
  void OnTargetRightClick(wxListEvent &event);
  void OnSyncButton(wxCommandEvent &event);
  void OnShowOnMapButton(wxCommandEvent &event);
  void OnClearDebugButton(wxCommandEvent &event);
  void OnMarkRecoveredButton(wxCommandEvent &event);
  void OnRetryReleaseButton(wxCommandEvent &event);
  void OnReleaseButton(wxCommandEvent &event);
  void OnRecoverButton(wxCommandEvent &event);
  void OnDeleteButton(wxCommandEvent &event);
  void OnMuteButton(wxCommandEvent &event);
  void OnKeyDown(wxKeyEvent &event);
  void OnShowOnMapAccelerator(wxCommandEvent &event);
  void OnReleaseAccelerator(wxCommandEvent &event);
  void OnDeleteAccelerator(wxCommandEvent &event);
  void OnRecoverAccelerator(wxCommandEvent &event);
  void OnManualReleaseAccelerator(wxCommandEvent &event);
  void OnSyncAccelerator(wxCommandEvent &event);
  void OnMuteAccelerator(wxCommandEvent &event);
  void UpdateTransponderInfo(transponder_state *state);
  void AddDebugMessage(const wxString &message);
  void DebugMessage(const wxString &message, bool alsoLog = true);
  void UpdateTCPConnectionStatus();
  void UpdateDeviceIdStatus(const wxString &deviceId);
  void UpdateAcousticStatus(const wxString &acousticStatus);
  void UpdateCloudStatus(const wxString &cloudStatus);
  void ShowReleaseStatusButtons(bool show);
  void UpdateReleaseStatusInfo(int transponder_id, const wxString &status);
  

  wxArrayInt GetSelectedItems();
  transponder_state *getXpdrFromIndex(int index);
  void clearHighlighted();
  long FindItemByName(wxListCtrl* listCtrl, const wxString& name);

  DECLARE_EVENT_TABLE()
};

#endif // _ROPELESSDIALOG_H_