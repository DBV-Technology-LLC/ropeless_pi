/******************************************************************************
 * $Id:
 *
 * Project:  OpenCPN
 * Purpose:  Ropeless Dialog Implementation
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

#include "RopelessDialog.h"
#include "ropeless_pi.h"
#include "OCPNListCtrl.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <sstream>

#include "mynumdlg.h"
#include "myokdlg.h"
#include "manualPlacementDlgImpl.h"
#include "transponderReleaseDlgImpl.h"

// External references to global variables from ropeless_pi.cpp
extern std::vector<transponder_state *> transponderStatus;
extern wxString colorTableNames[];
extern char qtRLStyleSheet[];
extern wxString msgFileName;
extern ropeless_pi *g_ropelessPI;
extern const wxString releaseStatusNames[];
extern const wxString recoveredStrList[];
extern int g_RopelessTargetList_sortColumn;
extern bool g_bRopelessTargetList_sortReverse;

// External functions from ropeless_pi.cpp
int wxCALLBACK wxListCompareFunction(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData);

// Helper function for list sorting
static int CompareD(double a, double b) {
  if (g_bRopelessTargetList_sortReverse) {
    if (a > b)
      return 1;
    else if (a < b)
      return -1;
    else
      return 0;
  } else {
    if (a > b)
      return -1;
    else if (a < b)
      return 1;
    else
      return 0;
  }
  return 0;
}

// Helper function to convert chart scale to pixels per meter
double ScaleToPPM(int chart_scale, const PlugIn_ViewPort &vp)
{
    // Calculate pixels per meter from chart scale
    // Chart scale 1:24000 means 1 unit on screen = 24000 units in real world
    // For proper conversion: scale_ppm = pixels_per_inch / (39.37 * chart_scale)
    double ppi = 96.0;  // pixels per inch (standard screen DPI)
    return ppi / (39.37 * chart_scale);  // 39.37 inches per meter
}

BEGIN_EVENT_TABLE(RopelessDialog, wxDialog)
EVT_BUTTON(wxID_OK, RopelessDialog::OnOKClick)
EVT_CLOSE(RopelessDialog::OnClose)
END_EVENT_TABLE()

RopelessDialog::RopelessDialog(wxWindow *parent, ropeless_pi *parent_pi,
                               wxWindowID id, const wxString &title,
                               const wxPoint &pos, const wxSize &size,
                               long style)
    : wxDialog(parent, id, title, pos, size, style) {
  pParentPi = parent_pi;
  wxFont *dFont = OCPNGetFont(_T("Dialog"), 0);
  SetFont(*dFont);

  // Set minimum size hints to ensure dialog can accommodate all content
  this->SetSizeHints(wxSize(900, -1), wxDefaultSize);

  // Create main layout (no tabs)
  wxBoxSizer *overallSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *bSizer2 = new wxBoxSizer(wxHORIZONTAL);

  // Create main content sizer (left side)
  wxBoxSizer *mainContentSizer = new wxBoxSizer(wxVERTICAL);
  
  long flags = wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES |
               wxBORDER_SUNKEN;

  // long flags = wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN;

  m_pListCtrlTranponders = new OCPNListCtrl(
      this, ID_TRANSPONDER_LIST, wxDefaultPosition, wxDefaultSize, flags);
  mainContentSizer->Add(m_pListCtrlTranponders, 1, wxEXPAND | wxALL, 0);

#ifdef __ANDROID__
  wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
  int char_size = pFont->GetPointSize();

  char font_style_sheet[200];
  sprintf(font_style_sheet, "QHeaderView::section {  font-size:%dpt; }",
          char_size);

  char item_font_style_sheet[200];
  sprintf(item_font_style_sheet, "QTreeWidget {  font-size:%dpt; }", char_size);

  std::ostringstream ss;
  ss << qtRLStyleSheet << font_style_sheet << item_font_style_sheet;
  m_pListCtrlTranponders->GetHandle()->setStyleSheet(ss.str().c_str());

#endif

  m_pListCtrlTranponders->Connect(
      wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK,
      wxListEventHandler(RopelessDialog::OnTargetRightClick), NULL, this);

  m_pListCtrlTranponders->Connect(
      wxEVT_COMMAND_LIST_COL_CLICK,
      wxListEventHandler(RopelessDialog::OnTargetListColumnClicked), NULL,
      this);

  m_pListCtrlTranponders->Connect(
      wxEVT_LIST_ITEM_SELECTED,
      wxListEventHandler(RopelessDialog::OnTargetListSelected), NULL, this);

  m_pListCtrlTranponders->Connect(
      wxEVT_LIST_ITEM_DESELECTED,
      wxListEventHandler(RopelessDialog::OnTargetListDeselected), NULL, this);

  int dx = GetCharWidth();

  wxSize txs = GetTextExtent("Color");
  m_pListCtrlTranponders->InsertColumn(tlICON, _("Color"), wxLIST_FORMAT_CENTER,
                                       txs.x + dx * 2);

  txs = GetTextExtent("ID");
  m_pListCtrlTranponders->InsertColumn(tlIDENT, _("ID"), wxLIST_FORMAT_CENTER,
                                       txs.x + dx * 2);

  txs = GetTextExtent("Release Status");
  m_pListCtrlTranponders->InsertColumn(tlRELEASE_STATUS, _("Release Status"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  txs = GetTextExtent("LastReportTime (UTC)");
  m_pListCtrlTranponders->InsertColumn(tlTIMESTAMP, _("LastReportTime (UTC)"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  txs = GetTextExtent("Range, M");
  m_pListCtrlTranponders->InsertColumn(tlRANGE, _("Range, M"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

#ifdef SHOW_DISTANCE
  txs = GetTextExtent("Distance, M");
  m_pListCtrlTranponders->InsertColumn(tlDISTANCE, _("Distance, M"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);
#endif

  // txs = GetTextExtent("Pings");
  // m_pListCtrlTranponders->InsertColumn(tlPINGS, _("Pings"),
  //                                      wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  // txs = GetTextExtent("Depth, M");
  // m_pListCtrlTranponders->InsertColumn(tlDEPTH, _("Depth, M"),
  //                                      wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  // txs = GetTextExtent("Temperature, C");
  // m_pListCtrlTranponders->InsertColumn(tlTEMP, _("Temperature, C"),
  //                                      wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  // txs = GetTextExtent("Battery %");
  // m_pListCtrlTranponders->InsertColumn(tlBATT_STAT, _("Battery %"),
  //                                      wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  txs = GetTextExtent("Recovered Status");
  m_pListCtrlTranponders->InsertColumn(tlRECOVERED, _("Recovered Status"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  // Build the color indicator bitmaps, adding to an image lst
  int imageRefSize = dx * 2;
  wxImageList *imglist = new wxImageList(imageRefSize, imageRefSize, true, 1);

  for (int i = 0; i < COLOR_TABLE_COUNT; i++) {
    wxScreenDC sdc;

    wxBitmap tbm(imageRefSize, imageRefSize, -1);
    wxMemoryDC mdc(tbm);
    mdc.Clear();
    wxString colorName;
    if (g_ropelessPI) {
        colorName = g_ropelessPI->GetColorName(i);
    } else {
        // Fallback if plugin is being destroyed
        colorName = (i == 0) ? "LIME GREEN" : "ORANGE";
    }
    wxColour rcolour = wxTheColourDatabase->Find(colorName);

    if (!rcolour.IsOk()) rcolour = wxColour(255, 000, 255);

    wxPen dpen(rcolour);
    wxBrush dbrush(rcolour);
    mdc.SetPen(dpen);
    mdc.SetBrush(dbrush);

    int xd = 0;
    int yd = 0;
    //    mdc.DrawRoundedRectangle(xd, yd, w+(label_offset * 2), h+2, -.25);
    mdc.DrawRectangle(xd, yd, imageRefSize, imageRefSize);
    mdc.SelectObject(wxNullBitmap);

    imglist->Add(tbm);
  }

  m_pListCtrlTranponders->AssignImageList(imglist, wxIMAGE_LIST_SMALL);

  // Add main content to horizontal sizer and ensure it has minimum height
  mainContentSizer->SetMinSize(-1, 400);  // Ensure table has reasonable minimum height
  bSizer2->Add(mainContentSizer, 1, wxEXPAND | wxALL, 0);
  
  // Create sidebar (right side)
  wxBoxSizer *sidebarSizer = new wxBoxSizer(wxVERTICAL);
  
  // Selected transponder header
  m_selectedTransponderLabel = new wxStaticText(this, wxID_ANY, _("Transponder: None Selected"));
  wxFont headerFont = m_selectedTransponderLabel->GetFont();
  headerFont.SetPointSize(headerFont.GetPointSize() + 2);
  headerFont.SetWeight(wxFONTWEIGHT_BOLD);
  m_selectedTransponderLabel->SetFont(headerFont);
  sidebarSizer->Add(m_selectedTransponderLabel, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
  
  // Tab view for transponder info/status/position (at top of sidebar)
  m_transponderInfoNotebook = new wxNotebook(this, wxID_ANY);
  
  // Info tab
  m_infoPanel = new wxPanel(m_transponderInfoNotebook, wxID_ANY);
  wxBoxSizer *infoSizer = new wxBoxSizer(wxVERTICAL);
  
  // Info tab content
  wxStaticText *infoLabel = new wxStaticText(m_infoPanel, wxID_ANY, _("Transponder Information"));
  wxFont boldFont = infoLabel->GetFont();
  boldFont.SetWeight(wxFONTWEIGHT_BOLD);
  infoLabel->SetFont(boldFont);
  infoSizer->Add(infoLabel, 0, wxALL, 5);
  
  infoSizer->Add(new wxStaticLine(m_infoPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  
  // Create info display controls with text wrapping support
  m_infoIdText = new wxStaticText(m_infoPanel, wxID_ANY, _("ID: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_infoPartnerIdText = new wxStaticText(m_infoPanel, wxID_ANY, _("Partner ID: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_infoManufacturerText = new wxStaticText(m_infoPanel, wxID_ANY, _("Manufacturer: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_infoOwnershipText = new wxStaticText(m_infoPanel, wxID_ANY, _("Ownership: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_infoTrawlIdText = new wxStaticText(m_infoPanel, wxID_ANY, _("Trawl ID: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_infoMarkTypeText = new wxStaticText(m_infoPanel, wxID_ANY, _("Mark Type: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  
  // Add controls and let them size to their content
  infoSizer->Add(m_infoIdText, 0, wxALL, 5);
  infoSizer->Add(m_infoPartnerIdText, 0, wxALL, 5);
  infoSizer->Add(m_infoManufacturerText, 0, wxALL, 5);
  infoSizer->Add(m_infoOwnershipText, 0, wxALL, 5);
  infoSizer->Add(m_infoTrawlIdText, 0, wxALL, 5);
  infoSizer->Add(m_infoMarkTypeText, 0, wxALL, 5);
  
  m_infoPanel->SetSizer(infoSizer);
  infoSizer->Fit(m_infoPanel);  // Size panel to fit its content
  m_transponderInfoNotebook->AddPage(m_infoPanel, _("Info"), true);
  
  // Status tab
  m_statusPanel = new wxPanel(m_transponderInfoNotebook, wxID_ANY);
  wxBoxSizer *statusSizer = new wxBoxSizer(wxVERTICAL);
  
  // Status tab content
  wxStaticText *statusLabel = new wxStaticText(m_statusPanel, wxID_ANY, _("Transponder Status"));
  statusLabel->SetFont(boldFont);
  statusSizer->Add(statusLabel, 0, wxALL, 5);
  
  statusSizer->Add(new wxStaticLine(m_statusPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  
  // Create status display controls with text wrapping support
  m_statusReleaseText = new wxStaticText(m_statusPanel, wxID_ANY, _("Release Status: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_statusRecoveryText = new wxStaticText(m_statusPanel, wxID_ANY, _("Recovery Status: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_statusBatteryText = new wxStaticText(m_statusPanel, wxID_ANY, _("Battery: ---%"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_statusPingsText = new wxStaticText(m_statusPanel, wxID_ANY, _("Pings: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_statusLastReportText = new wxStaticText(m_statusPanel, wxID_ANY, _("Last Report: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_statusPositionSourceText = new wxStaticText(m_statusPanel, wxID_ANY, _("Position Source: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  
  // Add controls and let them size to their content
  statusSizer->Add(m_statusReleaseText, 0, wxALL, 5);
  statusSizer->Add(m_statusRecoveryText, 0, wxALL, 5);
  statusSizer->Add(m_statusBatteryText, 0, wxALL, 5);
  statusSizer->Add(m_statusPingsText, 0, wxALL, 5);
  statusSizer->Add(m_statusLastReportText, 0, wxALL, 5);
  statusSizer->Add(m_statusPositionSourceText, 0, wxALL, 5);
  
  m_statusPanel->SetSizer(statusSizer);
  statusSizer->Fit(m_statusPanel);  // Size panel to fit its content
  m_transponderInfoNotebook->AddPage(m_statusPanel, _("Status"), false);
  
  // Position tab
  m_positionPanel = new wxPanel(m_transponderInfoNotebook, wxID_ANY);
  wxBoxSizer *positionSizer = new wxBoxSizer(wxVERTICAL);
  
  // Position tab content
  wxStaticText *positionLabel = new wxStaticText(m_positionPanel, wxID_ANY, _("Transponder Position"));
  positionLabel->SetFont(boldFont);
  positionSizer->Add(positionLabel, 0, wxALL, 5);
  
  positionSizer->Add(new wxStaticLine(m_positionPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  
  // Create position display controls with text wrapping support
  m_positionLatText = new wxStaticText(m_positionPanel, wxID_ANY, _("Latitude: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_positionLonText = new wxStaticText(m_positionPanel, wxID_ANY, _("Longitude: ---"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_positionRangeText = new wxStaticText(m_positionPanel, wxID_ANY, _("Range: --- m"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_positionBearingText = new wxStaticText(m_positionPanel, wxID_ANY, _("Bearing: ---°"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_positionDepthText = new wxStaticText(m_positionPanel, wxID_ANY, _("Depth: --- m"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_positionTempText = new wxStaticText(m_positionPanel, wxID_ANY, _("Temperature: ---°C"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  
  // Add controls and let them size to their content
  positionSizer->Add(m_positionLatText, 0, wxALL, 5);
  positionSizer->Add(m_positionLonText, 0, wxALL, 5);
  positionSizer->Add(m_positionRangeText, 0, wxALL, 5);
  positionSizer->Add(m_positionBearingText, 0, wxALL, 5);
  positionSizer->Add(m_positionDepthText, 0, wxALL, 5);
  positionSizer->Add(m_positionTempText, 0, wxALL, 5);
  
  m_positionPanel->SetSizer(positionSizer);
  positionSizer->Fit(m_positionPanel);  // Size panel to fit its content
  m_transponderInfoNotebook->AddPage(m_positionPanel, _("Position"), false);
  
  // Let notebook expand to match sidebar width (like Commands section)
  sidebarSizer->Add(m_transponderInfoNotebook, 0, wxEXPAND | wxALL, 5);
  
  // Force notebook to calculate its optimal size based on content
  m_transponderInfoNotebook->Fit();
  
  // Command buttons block
  wxStaticBoxSizer *commandButtonsSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Commands")), wxVERTICAL);
  
  wxBoxSizer *buttonRow1 = new wxBoxSizer(wxHORIZONTAL);
  m_releaseButton = new wxButton(this, wxID_ANY, _("Release"), wxDefaultPosition, wxDefaultSize, 0);
  m_recoverButton = new wxButton(this, wxID_ANY, _("Recover"), wxDefaultPosition, wxDefaultSize, 0);
  m_deleteButton = new wxButton(this, wxID_ANY, _("Delete"), wxDefaultPosition, wxDefaultSize, 0);
  buttonRow1->Add(m_releaseButton, 0, wxALL, 2);
  buttonRow1->Add(m_recoverButton, 0, wxALL, 2);
  buttonRow1->Add(m_deleteButton, 0, wxALL, 2);
  
  wxBoxSizer *buttonRow2 = new wxBoxSizer(wxHORIZONTAL);
  m_muteButton = new wxButton(this, wxID_ANY, _("Mute"), wxDefaultPosition, wxDefaultSize, 0);
  m_sidebarSyncButton = new wxButton(this, wxID_ANY, _("Sync"), wxDefaultPosition, wxDefaultSize, 0);
  buttonRow2->Add(m_muteButton, 0, wxALL, 2);
  buttonRow2->Add(m_sidebarSyncButton, 0, wxALL, 2);
  
  wxBoxSizer *buttonRow3 = new wxBoxSizer(wxHORIZONTAL);
  m_showOnMapButton = new wxButton(this, wxID_ANY, _("Show On Map"), wxDefaultPosition, wxDefaultSize, 0);
  m_ManualReleaseButton = new wxButton(this, wxID_ANY, _("Manual Release"), wxDefaultPosition, wxDefaultSize, 0);
  buttonRow3->Add(m_showOnMapButton, 0, wxALL, 2);
  buttonRow3->Add(m_ManualReleaseButton, 0, wxALL, 2);
  m_sidebarSyncButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnSyncButton, this);
  m_showOnMapButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnShowOnMapButton, this);
  m_ManualReleaseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnManualReleaseButton, this);
  
  commandButtonsSizer->Add(buttonRow1, 0, wxEXPAND, 0);
  commandButtonsSizer->Add(buttonRow2, 0, wxEXPAND, 0);
  commandButtonsSizer->Add(buttonRow3, 0, wxEXPAND, 0);
  sidebarSizer->Add(commandButtonsSizer, 0, wxEXPAND | wxALL, 5);
  
  // Deckbox Status box
  m_deckboxStatusSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Deckbox Status")), wxVERTICAL);
  m_deckboxStatusText = new wxStaticText(this, wxID_ANY, _("Status: Ready"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_deckboxStatusSizer->Add(m_deckboxStatusText, 0, wxALL | wxEXPAND, 5);
  
  // Add TCP Connection status line with text wrapping
  m_tcpConnectionStatusText = new wxStaticText(this, wxID_ANY, _("TCP Connection: Disconnected"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_deckboxStatusSizer->Add(m_tcpConnectionStatusText, 0, wxALL | wxEXPAND, 5);
  
  sidebarSizer->Add(m_deckboxStatusSizer, 0, wxEXPAND | wxALL, 5);
  
  // Release Status block
  m_releaseStatusSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Release Status")), wxVERTICAL);
  m_releaseStatusText = new wxStaticText(this, wxID_ANY, _("Status: Standby"), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
  m_releaseStatusSizer->Add(m_releaseStatusText, 0, wxALL | wxEXPAND, 5);
  sidebarSizer->Add(m_releaseStatusSizer, 0, wxEXPAND | wxALL, 5);
  
  // Add sidebar to main horizontal sizer with proper scaling
  // Set minimum width for sidebar to ensure all content fits, but allow it to expand
  sidebarSizer->SetMinSize(350, -1);  // Ensure minimum width for all sidebar content
  bSizer2->Add(sidebarSizer, 0, wxEXPAND | wxALL, 5);
  
  // Force sidebar to calculate its minimum size based on content
  sidebarSizer->Layout();
  
  // Ensure the main horizontal container is tall enough for all sidebar content
  bSizer2->SetMinSize(-1, 500);  // Set minimum height to accommodate sidebar boxes
  
  // Add table/sidebar combo to main dialog - let it expand as needed
  overallSizer->Add(bSizer2, 1, wxEXPAND, 0);

  // Add debug box with proper sizing
  wxStaticBoxSizer *debugSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Debug Messages")), wxVERTICAL);
  overallSizer->Add(debugSizer, 0, wxALL | wxEXPAND, 5);
  
  m_debugTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, 
                                   wxDefaultPosition, wxSize(-1, 100), 
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
  debugSizer->Add(m_debugTextCtrl, 1, wxEXPAND | wxALL, 5);
  
  // Add Clear button for debug messages
  m_clearDebugButton = new wxButton(this, wxID_ANY, _("Clear Debug Messages"), wxDefaultPosition, wxDefaultSize, 0);
  m_clearDebugButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnClearDebugButton, this);
  debugSizer->Add(m_clearDebugButton, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);

  m_sdbSizer1 = new wxStdDialogButtonSizer();
  m_sdbSizer1OK = new wxButton(this, wxID_OK);
  m_sdbSizer1->AddButton(m_sdbSizer1OK);
  m_sdbSizer1->Realize();

  overallSizer->Add(m_sdbSizer1, 0, wxBOTTOM | wxEXPAND | wxTOP, 5);

  this->SetSizer(overallSizer);
  
  // Size the dialog to fit its content vertically
  this->Fit();
  
  // Final layout to ensure everything is positioned correctly
  this->Layout();
  
  this->Centre(wxBOTH);
  
  // Update initial TCP connection status
  UpdateTCPConnectionStatus();

}

RopelessDialog::~RopelessDialog() {
  // Disconnect all event handlers to prevent crashes during shutdown
  try {
    // Disconnect list control events (using old Connect/Disconnect style)
    if (m_pListCtrlTranponders) {
      m_pListCtrlTranponders->Disconnect(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK,
        wxListEventHandler(RopelessDialog::OnTargetRightClick), NULL, this);
      m_pListCtrlTranponders->Disconnect(wxEVT_COMMAND_LIST_COL_CLICK,
        wxListEventHandler(RopelessDialog::OnTargetListColumnClicked), NULL, this);
      m_pListCtrlTranponders->Disconnect(wxEVT_LIST_ITEM_SELECTED,
        wxListEventHandler(RopelessDialog::OnTargetListSelected), NULL, this);
      m_pListCtrlTranponders->Disconnect(wxEVT_LIST_ITEM_DESELECTED,
        wxListEventHandler(RopelessDialog::OnTargetListDeselected), NULL, this);
    }
    
    // Unbind button events (using newer Bind/Unbind style)
    if (m_sidebarSyncButton) {
      m_sidebarSyncButton->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnSyncButton, this);
    }
    if (m_showOnMapButton) {
      m_showOnMapButton->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnShowOnMapButton, this);
    }
    if (m_ManualReleaseButton) {
      m_ManualReleaseButton->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnManualReleaseButton, this);
    }
    if (m_clearDebugButton) {
      m_clearDebugButton->Unbind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnClearDebugButton, this);
    }
  } catch (...) {
    // Ignore any exceptions during cleanup - dialog may already be destroyed
  }

  // delete m_pSerialArray;
}

// Called on List right click. Attached to wxListEventHandler
void RopelessDialog::OnTargetRightClick(wxListEvent &event) {
  int mouseX;
  int mouseY;
  long index = -1;

  if (m_pListCtrlTranponders->GetItemCount()) {
    wxListItem item;
    item.SetId(0);
    wxRect rect;
    m_pListCtrlTranponders->GetItemRect(item, rect);

    const wxPoint pt = wxGetMousePosition();
    mouseX = pt.x - m_pListCtrlTranponders->GetScreenPosition().x;
    mouseY = pt.y - m_pListCtrlTranponders->GetScreenPosition().y;

#ifndef __WXMSW__
    mouseY -= rect.height;
#endif

    int flags;
    index = m_pListCtrlTranponders->HitTest(wxPoint(mouseX, mouseY), flags);

    if (index >= 0) {
      wxString sID = m_pListCtrlTranponders->GetItemText(index, 1);
      long fid = atoi(sID.ToStdString().c_str());

      // search the transponder list for an ident match
      long foundIndex = -1;
      for (unsigned int i = 0; i < transponderStatus.size(); i++) {
        transponder_state *state = transponderStatus[i];
        if (state->ident == fid) {
          foundIndex = i;
          wxLogMessage("List found index: %d", index);
          break;
        }
      }

      if (foundIndex >= 0) {
        g_ropelessPI->m_foundState = transponderStatus[foundIndex];

        wxLogMessage("Right Clicked via List on ID: %d",
                     g_ropelessPI->m_foundState->ident);

        wxMenu *contextMenu = new wxMenu;

        wxMenuItem *id_item = 0;
        wxString transponderIDString;
        transponderIDString.Printf("ID: %d", g_ropelessPI->m_foundState->ident);
        id_item = new wxMenuItem(contextMenu, ID_TPR_ID, _(transponderIDString));
        
        wxMenuItem *release_item = 0;
        release_item = new wxMenuItem(contextMenu, ID_TPR_RELEASE,
                                      _("Release Transponder"));

        wxMenuItem *recovered_item = 0;
        if (g_ropelessPI->m_foundState->recovered_state == eREC_DEPLOYED)
        {
          recovered_item = new wxMenuItem(contextMenu, ID_TPR_RECOVER, _("Mark Recovered") );
        }
        else if (g_ropelessPI->m_foundState->recovered_state == eREC_RECOVERED)
        {
          recovered_item = new wxMenuItem(contextMenu, ID_TPR_RECOVER, _("Mark Deployed") );
        }

        wxMenuItem *delete_item = 0;
        delete_item = new wxMenuItem(contextMenu, ID_TPR_DELETE, _("Delete"));

#ifdef __ANDROID__
        wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
        release_item->SetFont(*pFont);
#endif

        contextMenu->Append(id_item);
        contextMenu->Append(release_item);
        contextMenu->Append(recovered_item);
        contextMenu->Append(delete_item);

        GetOCPNCanvasWindow()->Connect(
            ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
            pParentPi);

        GetOCPNCanvasWindow()->Connect(
            ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
            pParentPi);

        GetOCPNCanvasWindow()->Connect(
            ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
            pParentPi);

        //   Invoke the drop-down menu
        GetOCPNCanvasWindow()->PopupMenu(contextMenu, wxGetMousePosition().x,
                                         wxGetMousePosition().y);

        if (release_item)
          GetOCPNCanvasWindow()->Disconnect(
              ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
              wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
              pParentPi);

        if (recovered_item)
          GetOCPNCanvasWindow()->Disconnect(
              ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
              wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
              pParentPi);

        if (delete_item)
          GetOCPNCanvasWindow()->Disconnect(
              ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
              wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
              pParentPi);

      }
    }
  }
}

wxArrayInt RopelessDialog::GetSelectedItems() {
  wxArrayInt selectedItems;
  long itemIndex = m_pListCtrlTranponders->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                       wxLIST_STATE_SELECTED);

  while (itemIndex != wxNOT_FOUND) {
    selectedItems.Add(itemIndex);
    itemIndex = m_pListCtrlTranponders->GetNextItem(itemIndex, wxLIST_NEXT_ALL,
                                                    wxLIST_STATE_SELECTED);
  }

  return selectedItems;
}

transponder_state *RopelessDialog::getXpdrFromIndex(int index) {
  long fid;
  transponder_state *state;

  // Get idents of selected items
  if (index >= 0) {
    wxString sID = m_pListCtrlTranponders->GetItemText(index, 1);
    fid = atoi(sID.ToStdString().c_str());

    // search the transponder list for an ident match
    long foundIndex = -1;
    for (unsigned int i = 0; i < transponderStatus.size(); i++) {
      state = transponderStatus[i];
      if (state->ident == fid) {
        foundIndex = i;
        break;
      }
    }
  }

  return state;
}

void RopelessDialog::OnTargetListDeselected(wxListEvent &event) {
  long deselectedIndex = event.GetIndex();
  // wxLogMessage("Item deselected: %ld", deselectedIndex);

  if (deselectedIndex >= 0) {
    transponder_state *state = getXpdrFromIndex(deselectedIndex);

    if (state->ident > 0) {
      state->color_index = COLOR_INDEX_GREEN;

    } else {
      state->color_index = COLOR_INDEX_RED;
    }

    // Clear selected transponder and update info panel
    m_selectedTransponder = NULL;
    UpdateTransponderInfo(NULL);

    RequestRefresh(GetOCPNCanvasWindow());
  }
}

void RopelessDialog::OnTargetListSelected(wxListEvent &event) {
  wxArrayInt selectedItems = GetSelectedItems();
  int numItems = selectedItems.GetCount();

  wxString message = "Selected items: ";
  for (size_t i = 0; i < numItems; i++) {
    message += wxString::Format("%d ", selectedItems[i]);
  }

  if (numItems > 0) {
    // wxLogMessage(message);

    transponder_state *state = getXpdrFromIndex(selectedItems[0]);

    state->color_index = COLOR_INDEX_GOLDEN;
    
    // Update selected transponder and refresh info panel
    m_selectedTransponder = state;
    UpdateTransponderInfo(state);

    RequestRefresh(GetOCPNCanvasWindow());
  }
}

void RopelessDialog::OnTargetListColumnClicked(wxListEvent &event) {
  int key = event.GetColumn();
  wxListItem item;
  // item.SetMask(wxLIST_MASK_IMAGE);

  if (key == g_RopelessTargetList_sortColumn)
    g_bRopelessTargetList_sortReverse = !g_bRopelessTargetList_sortReverse;
  else {
    // item.SetImage(-1);
    // m_pListCtrlAISTargets->SetColumn(g_AisTargetList_sortColumn, item);
    g_bRopelessTargetList_sortReverse = false;
    g_RopelessTargetList_sortColumn = key;
  }
  // item.SetImage(g_bAisTargetList_sortReverse ? 1 : 0);

  // if (!g_bAisTargetList_autosort) g_bsort_once = true;

  //  if (g_RopelessTargetList_sortColumn >= 0) {
  // m_pListCtrlAISTargets->SetColumn(g_AisTargetList_sortColumn, item);
  RefreshTransponderList();
  //  }
}

void RopelessDialog::RefreshTransponderList() {
  
  std::vector<long> selectedIndices;
  long item = -1;
  while ((item = m_pListCtrlTranponders->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
      selectedIndices.push_back(item);
  }

  m_pListCtrlTranponders->Freeze();
  m_pListCtrlTranponders->DeleteAllItems();

  //  Walk the vector of transponder status
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];
    
    // Skip cloud positions from list display by default
    if (state->position_source == ePOS_SOURCE_CLOUD) {
      continue;
    }

    wxListItem item;
    item.SetId(i);
    // long result = m_pListCtrlTranponders->InsertItem(item);
    long result = m_pListCtrlTranponders->InsertItem(i, " ");

    m_pListCtrlTranponders->SetItemData(result, (long)i);

    item.SetColumn(tlICON);
    m_pListCtrlTranponders->SetItemImage(item, state->color_index);

    // item.SetColumn(tlIDENT);
    wxString sid;
    sid.Printf("%d", state->ident);
    // item.SetText(sid);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlIDENT, sid);
    m_pListCtrlTranponders->SetColumnWidth(tlIDENT, wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlRELEASE_STATUS);
    int rlsNum;
    wxString appendStr = "";
    wxString rid;
    if (state->release_status == -4)
    {
      rlsNum = eRELEASE_NETWORK_ERR;
    }
    if (state->release_status == -3)
    {
      rlsNum = eRELEASE_TIMEOUT;
    }
    else if (state->release_status == -2)
    {
      rlsNum = eRELEASE_NOT_INIT;
    }
    else if (state->release_status == -1)
    {
      rlsNum = eRELEASE_NOT_VERIFIED;
    }
    else if (state->release_status == 0)
    {
      rlsNum = eRELEASE_VERIFIED;
    }
    else if (state->release_status > 0)
    {
      rlsNum = eRELEASE_SENDING;
      appendStr.Printf("%d",state->release_status);
    }
    else
    {
      state->release_status = -2;
      rlsNum = eRELEASE_NOT_INIT;
    }

    rid.Printf("%s%s", releaseStatusNames[rlsNum],appendStr);

    // wxListItem testItem;
    // testItem.SetId(i);
    // testItem.SetColumn(1);
    // testItem.SetBackgroundColour(*wxGREEN);
    // m_pListCtrlTranponders->SetItem(testItem);

    m_pListCtrlTranponders->SetItem(result, tlRELEASE_STATUS, rid);
    m_pListCtrlTranponders->SetColumnWidth(tlRELEASE_STATUS,
                                           wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlTIMESTAMP);
    wxString sts;
    // wxDateTime ts = DaysTowDT(state->timeStamp);
    wxDateTime ts((time_t)(state->timeStamp));
    ts.MakeUTC();
    sts = ts.FormatISOCombined(' ');
    m_pListCtrlTranponders->SetItem(result, tlTIMESTAMP, sts);
    m_pListCtrlTranponders->SetColumnWidth(tlTIMESTAMP,
                                           wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlDEPTH);
    // wxString sdp;
    // sdp.Printf("%g", state->depth);
    // item.SetText(sdp);
    // m_pListCtrlTranponders->SetItem(item);
    // m_pListCtrlTranponders->SetItem(result, tlDEPTH, sdp);
    // m_pListCtrlTranponders->SetColumnWidth(tlDEPTH, wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlTEMP);
    // wxString stemp;
    // stemp.Printf("%g", state->temp);
    // item.SetText(stemp);
    // m_pListCtrlTranponders->SetItem(item);
    // m_pListCtrlTranponders->SetItem(result, tlTEMP, stemp);
    // m_pListCtrlTranponders->SetColumnWidth(tlTEMP, wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlPINGS);
    // wxString sping;
    // sping.Printf("%d", state->pings);
    // item.SetText(sping);
    // m_pListCtrlTranponders->SetItem(item);
    // m_pListCtrlTranponders->SetItem(result, tlPINGS, sping);
    // m_pListCtrlTranponders->SetColumnWidth(tlPINGS, wxLIST_AUTOSIZE_USEHEADER);

#ifdef SHOW_DISTANCE
    // item.SetColumn(tlDISTANCE);
    wxString sdist;
    sdist = wxString::Format(wxT("%.*f"), 2, state->distance);
    // wxString sdist;
    // sdist.Printf("%g", state->distance);
    // item.SetText(sdist);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlDISTANCE, sdist);
    m_pListCtrlTranponders->SetColumnWidth(tlDISTANCE,
                                           wxLIST_AUTOSIZE_USEHEADER);
#endif
    
    // item.SetColumn(tlRECOVERED);
    wxString srec;
    srec.Printf("%s", recoveredStrList[state->recovered_state]);
    // item.SetText(sdist);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlRECOVERED, srec);
    m_pListCtrlTranponders->SetColumnWidth(tlRECOVERED,
                                           wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlRANGE);
    wxString srng;
    srng.Printf("%g", state->range);
    // item.SetText(sdist);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlRANGE, srng);
    m_pListCtrlTranponders->SetColumnWidth(tlRANGE,
                                           wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlBATT_STAT);
    // wxString sbatt;
    // sbatt.Printf("%d", state->batt_stat);
    // item.SetText(sdist);
    // m_pListCtrlTranponders->SetItem(item);
    // m_pListCtrlTranponders->SetItem(result, tlBATT_STAT, sbatt);
    // m_pListCtrlTranponders->SetColumnWidth(tlBATT_STAT,
    //                                        wxLIST_AUTOSIZE_USEHEADER);
  }

  if (g_RopelessTargetList_sortColumn > 0)
    m_pListCtrlTranponders->SortItems(
        wxListCompareFunction, reinterpret_cast<wxIntPtr>(&transponderStatus));

  for (auto index : selectedIndices) {
      if (index < m_pListCtrlTranponders->GetItemCount()) {
          m_pListCtrlTranponders->SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      }
  }
  
  m_pListCtrlTranponders->Thaw();

#ifdef __WXMSW__
  m_pListCtrlTranponders->Refresh(false);
#endif

  // Update TCP connection status periodically
  UpdateTCPConnectionStatus();

}

void RopelessDialog::OnChooseFileButton(wxCommandEvent &event) {
  // Simulator functionality removed
}

void RopelessDialog::OnStopSimButton(wxCommandEvent &event) {
  // Simulator functionality removed
}

void RopelessDialog::OnStartSimButton(wxCommandEvent &event) {
  // Simulator functionality removed
}

void RopelessDialog::OnManualReleaseButton(wxCommandEvent &event) {

  wxString msg("Manually Enter Transponder ID to Release: ");

  long result = -1;
  myNumberEntryDialog dialog;

#ifdef __ANDROID__
  wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
  dialog.SetFont(*pFont);
#endif

  dialog.Create(GetOCPNCanvasWindow(), msg, "Enter Transponder ID",
                "Ropeless Plugin Message", 0, 0, 100000, wxDefaultPosition);

  if (dialog.ShowModal() == wxID_OK) {
    result = dialog.GetValue();
  }

  if (result >= 0) {
    wxString s1;
    s1.Printf("Manual Release Req for ID: %d", result);
    wxLogMessage(s1);

    g_ropelessPI->manualReleaseState.ident = result;
    g_ropelessPI->SendCommandMessage(&g_ropelessPI->manualReleaseState, eCMD_RELEASE);

  }
}

void RopelessDialog::OnSyncButton(wxCommandEvent &event)
{
  wxLogMessage("OnSyncButton called - starting sync process");
  
  // Send the original sync message
  g_ropelessPI->SendSyncMessage();
  
  // Also send a GMR NMEA string for sync command
  wxString gmr_sentence = "$ECGMR,1,0,0,0,2,0,0,0*5E";  // Sync command GMR
  
  // Send via TCP connection directly
  bool tcp_sent = g_ropelessPI->SendRawNMEA(gmr_sentence);
  
  // Also send via OpenCPN's NMEA buffer as backup
  PushNMEABuffer(gmr_sentence);
  
  // Display the sent GMR message in debug output
  if (tcp_sent) {
    AddDebugMessage("--> TCP: " + gmr_sentence);
  } else {
    AddDebugMessage("--> TCP FAILED: " + gmr_sentence);
  }
  AddDebugMessage("--> OpenCPN: " + gmr_sentence);
  
  // Also add a simple test message to verify the debug text control is working
  AddDebugMessage("Sync button clicked - test message");
  
  wxLogMessage("Sync button: Sent sync message via TCP (%s) and OpenCPN NMEA buffer", 
               tcp_sent ? "success" : "failed");
}

void RopelessDialog::OnShowOnMapButton(wxCommandEvent &event)
{
  // Check if we have a transponder selected to zoom to
  if (m_selectedTransponder) {

    double scale_ppm = 0.1;  // Gives 1:43000 zoom level
    
    JumpToPosition(m_selectedTransponder->predicted_lat, 
                   m_selectedTransponder->predicted_lon, 
                   scale_ppm);
    
    wxLogMessage("Show On Map: Centering on transponder %d at lat=%.6f, lon=%.6f with scale_ppm=%.6f", 
                 m_selectedTransponder->ident,
                 m_selectedTransponder->predicted_lat,
                 m_selectedTransponder->predicted_lon,
                 scale_ppm);
  }
}

void RopelessDialog::OnClearDebugButton(wxCommandEvent &event)
{
  m_debugTextCtrl->Clear();
  wxLogMessage("Debug messages cleared by user");
}

void RopelessDialog::AddDebugMessage(const wxString &message)
{
  wxLogMessage("AddDebugMessage called with: %s", message);
  
  if (!m_debugTextCtrl) {
    wxLogMessage("ERROR: m_debugTextCtrl is NULL!");
    return;
  }
  
  // Add timestamp to the message
  wxDateTime now = wxDateTime::Now();
  wxString timestampedMessage = wxString::Format("[%s] %s\n", 
                                                  now.FormatISOTime(), 
                                                  message);
  
  // Append to debug text control
  m_debugTextCtrl->AppendText(timestampedMessage);
  
  wxLogMessage("Debug message added to text control");
}

void RopelessDialog::UpdateTransponderInfo(transponder_state *state) {
  if (!state) {
    // Clear all displays when no transponder is selected
    m_selectedTransponderLabel->SetLabel(_("Transponder: None Selected"));
    m_infoIdText->SetLabel(_("ID: ---"));
    m_infoPartnerIdText->SetLabel(_("Partner ID: ---"));
    m_infoManufacturerText->SetLabel(_("Manufacturer: ---"));
    m_infoOwnershipText->SetLabel(_("Ownership: ---"));
    m_infoTrawlIdText->SetLabel(_("Trawl ID: ---"));
    m_infoMarkTypeText->SetLabel(_("Mark Type: ---"));
    
    m_statusReleaseText->SetLabel(_("Release Status: ---"));
    m_statusRecoveryText->SetLabel(_("Recovery Status: ---"));
    m_statusBatteryText->SetLabel(_("Battery: ---%"));
    m_statusPingsText->SetLabel(_("Pings: ---"));
    m_statusLastReportText->SetLabel(_("Last Report: ---"));
    m_statusPositionSourceText->SetLabel(_("Position Source: ---"));
    
    m_positionLatText->SetLabel(_("Latitude: ---"));
    m_positionLonText->SetLabel(_("Longitude: ---"));
    m_positionRangeText->SetLabel(_("Range: --- m"));
    m_positionBearingText->SetLabel(_("Bearing: ---°"));
    m_positionDepthText->SetLabel(_("Depth: --- m"));
    m_positionTempText->SetLabel(_("Temperature: ---°C"));
  } else {
    // Update header with selected transponder ID
    m_selectedTransponderLabel->SetLabel(wxString::Format(_("Transponder: %d"), state->ident));
    
    // Update Info tab
    m_infoIdText->SetLabel(wxString::Format(_("ID: %d"), state->ident));
    m_infoPartnerIdText->SetLabel(wxString::Format(_("Partner ID: %d"), state->ident_partner));
    m_infoManufacturerText->SetLabel(wxString::Format(_("Manufacturer: %d"), state->mfg));
    m_infoOwnershipText->SetLabel(wxString::Format(_("Ownership: %d"), state->ownership));
    m_infoTrawlIdText->SetLabel(wxString::Format(_("Trawl ID: %d"), state->trawl_id));
    m_infoMarkTypeText->SetLabel(wxString::Format(_("Mark Type: %d"), state->mark_type));
    
    // Update Status tab
    wxString releaseStatus = "";
    if (state->release_status == -4) {
      releaseStatus = releaseStatusNames[eRELEASE_NETWORK_ERR];
    } else if (state->release_status == -3) {
      releaseStatus = releaseStatusNames[eRELEASE_TIMEOUT];
    } else if (state->release_status == -2) {
      releaseStatus = releaseStatusNames[eRELEASE_NOT_INIT];
    } else if (state->release_status == -1) {
      releaseStatus = releaseStatusNames[eRELEASE_NOT_VERIFIED];
    } else if (state->release_status == 0) {
      releaseStatus = releaseStatusNames[eRELEASE_VERIFIED];
    } else if (state->release_status > 0) {
      releaseStatus = wxString::Format("%s%d", releaseStatusNames[eRELEASE_SENDING], state->release_status);
    }
    
    m_statusReleaseText->SetLabel(wxString::Format(_("Release Status: %s"), releaseStatus));
    m_statusRecoveryText->SetLabel(wxString::Format(_("Recovery Status: %s"), recoveredStrList[state->recovered_state]));
    m_statusBatteryText->SetLabel(wxString::Format(_("Battery: %d%%"), state->batt_stat));
    m_statusPingsText->SetLabel(wxString::Format(_("Pings: %d"), state->pings));
    
    // Format timestamp
    wxDateTime ts((time_t)(state->timeStamp));
    ts.MakeUTC();
    m_statusLastReportText->SetLabel(wxString::Format(_("Last Report: %s"), ts.FormatISOCombined(' ')));
    m_statusPositionSourceText->SetLabel(wxString::Format(_("Position Source: %s"), positionSourceNames[state->position_source]));
    
    // Update Position tab
    m_positionLatText->SetLabel(wxString::Format(_("Latitude: %.6f"), state->predicted_lat));
    m_positionLonText->SetLabel(wxString::Format(_("Longitude: %.6f"), state->predicted_lon));
    m_positionRangeText->SetLabel(wxString::Format(_("Range: %.1f m"), state->range));
    m_positionBearingText->SetLabel(wxString::Format(_("Bearing: %.1f°"), state->bearing));
    m_positionDepthText->SetLabel(wxString::Format(_("Depth: %.1f m"), state->depth));
    m_positionTempText->SetLabel(wxString::Format(_("Temperature: %.1f°C"), state->temp));
  }
  
  // Refresh the panels to show updated text
  m_infoPanel->Refresh();
  m_statusPanel->Refresh();
  m_positionPanel->Refresh();
}

void RopelessDialog::UpdateTCPConnectionStatus()
{
  wxString statusText = _("TCP Connection: ");
  
  if (pParentPi && pParentPi->IsTCPOutputConnected()) {
    statusText += _("Connected");
  } else {
    statusText += _("Disconnected");
  }
  
  m_tcpConnectionStatusText->SetLabel(statusText);
}


void RopelessDialog::clearHighlighted() {
  wxArrayInt selectedItems = GetSelectedItems();
  int numItems = selectedItems.GetCount();

  // for (size_t i = 0; i < numItems; i++) {
  //     message += wxString::Format("%d ", selectedItems[i]);
  // }

  if (numItems > 0) {
    transponder_state *state = getXpdrFromIndex(selectedItems[0]);

    if (state->ident > 0) {
      state->color_index = COLOR_INDEX_GREEN;

    } else {
      state->color_index = COLOR_INDEX_RED;
    }
  }
}

long RopelessDialog::FindItemByName(wxListCtrl* listCtrl, const wxString& name) {
  long itemIndex = -1;
  while ((itemIndex = listCtrl->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE)) != wxNOT_FOUND) {
      wxLogMessage("Item at index %ld: %s", itemIndex, listCtrl->GetItemText(itemIndex));;
      if (listCtrl->GetItemText(itemIndex) == name) {
          return itemIndex;
      }
  }
  return wxNOT_FOUND; // Return -1 if the item is not found
}

void RopelessDialog::OnClose(wxCloseEvent &event) {
  wxLogMessage("RopelessDialog: OnClose started");
  
  clearHighlighted();
  wxLogMessage("RopelessDialog: clearHighlighted completed");

#ifndef __ANDROID__
  wxPoint p = GetPosition();
  pParentPi->m_dialogPosX = p.x;
  pParentPi->m_dialogPosY = p.y;
  wxSize s = GetSize();
  pParentPi->m_dialogSizeWidth = s.x;
  pParentPi->m_dialogSizeHeight = s.y;
  wxLogMessage("RopelessDialog: Position/size saved");
#endif
  
  wxLogMessage("RopelessDialog: About to call Destroy()");
  Destroy();
  wxLogMessage("RopelessDialog: Destroy() completed");
  
  pParentPi->m_pRLDialog = NULL;
  wxLogMessage("RopelessDialog: OnClose completed");
}

void RopelessDialog::OnOKClick(wxCommandEvent &event) {
  clearHighlighted();

  Close();
}

static int wxCALLBACK wxListCompareFunction(wxIntPtr item1, wxIntPtr item2,
                                            wxIntPtr sortData) {
  std::vector<transponder_state *> *v = &transponderStatus;  // reinterpret_cast<std::vector<transponder_state
                                                             // *>*>(sortData);

  auto tS1 = (*v)[static_cast<size_t>(item1)];
  auto tS2 = (*v)[static_cast<size_t>(item2)];

  switch (g_RopelessTargetList_sortColumn) {

#ifdef SHOW_DISTANCE
    case tlDISTANCE:
      return (CompareD(tS2->distance, tS1->distance));
      break;
#endif

    case tlTIMESTAMP: {
      return (CompareD(tS2->timeStamp, tS1->timeStamp));
      break;
    }

    case tlIDENT:
      return (CompareD((double)tS2->ident, (double)tS1->ident));
      break;

    case tlRANGE:
      return (CompareD(tS2->range, tS1->range));
      break;

    case tlICON:
    case tlRELEASE_STATUS:
    case tlPINGS:
    case tlDEPTH:
    case tlTEMP:
    case tlBATT_STAT:
    default:
      return 0;
  }
}