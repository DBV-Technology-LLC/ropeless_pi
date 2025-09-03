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
#include <wx/imaglist.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
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

  this->SetSizeHints(wxDefaultSize, wxDefaultSize);

  wxBoxSizer *bSizer2;
  bSizer2 = new wxBoxSizer(wxVERTICAL);

  long flags = wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES |
               wxBORDER_SUNKEN;

  // long flags = wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN;

  m_pListCtrlTranponders = new OCPNListCtrl(
      this, ID_TRANSPONDER_LIST, wxDefaultPosition, wxDefaultSize, flags);
  bSizer2->Add(m_pListCtrlTranponders, 1, wxEXPAND | wxALL, 0);

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

  txs = GetTextExtent("Pings");
  m_pListCtrlTranponders->InsertColumn(tlPINGS, _("Pings"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  txs = GetTextExtent("Depth, M");
  m_pListCtrlTranponders->InsertColumn(tlDEPTH, _("Depth, M"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  txs = GetTextExtent("Temperature, C");
  m_pListCtrlTranponders->InsertColumn(tlTEMP, _("Temperature, C"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

  txs = GetTextExtent("Battery %");
  m_pListCtrlTranponders->InsertColumn(tlBATT_STAT, _("Battery %"),
                                       wxLIST_FORMAT_CENTER, txs.x + dx * 2);

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
    wxString colorName =
        colorTableNames[i];  // colorTableNames[state->color_index];
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

#ifndef __ANDROID__
  wxStaticBoxSizer *sbSizerSim = new wxStaticBoxSizer(
      new wxStaticBox(this, wxID_ANY, _("Simulator")), wxVERTICAL);
  bSizer2->Add(sbSizerSim, 0, wxALL | wxEXPAND, 5);

  m_simTextCtrl =
      new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                     wxDefaultSize, wxTE_READONLY);
  sbSizerSim->Add(m_simTextCtrl, 0, wxEXPAND | wxALL, 5);

  if (wxFileExists(msgFileName)) m_simTextCtrl->SetValue(msgFileName);

  wxBoxSizer *bsizersimButtons = new wxBoxSizer(wxHORIZONTAL);
  sbSizerSim->Add(bsizersimButtons, 0, wxEXPAND, 5);

  m_ChooseFileButton = new wxButton(this, wxID_ANY, _("Choose File..."),
                                    wxDefaultPosition, wxDefaultSize, 0);
  bsizersimButtons->Add(m_ChooseFileButton, 0, wxALL, 5);
  m_ChooseFileButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
                           &RopelessDialog::OnChooseFileButton, this);

  m_StopSimButton = new wxButton(this, wxID_ANY, _("Stop Sim"),
                                 wxDefaultPosition, wxDefaultSize, 0);
  bsizersimButtons->Add(m_StopSimButton, 0, wxALL, 5);
  m_StopSimButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
                        &RopelessDialog::OnStopSimButton, this);

  m_StartSimButton = new wxButton(this, wxID_ANY, _("StartSim"),
                                  wxDefaultPosition, wxDefaultSize, 0);
  bsizersimButtons->Add(m_StartSimButton, 0, wxALL, 5);
  m_StartSimButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
                         &RopelessDialog::OnStartSimButton, this);

  m_ManualReleaseButton = new wxButton(this, wxID_ANY, _("Manual Release"),
                                       wxDefaultPosition, wxDefaultSize, 0);
  bsizersimButtons->Add(m_ManualReleaseButton, 0, wxALL, 5);
  m_ManualReleaseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
                              &RopelessDialog::OnManualReleaseButton, this);

  m_SyncButton = new wxButton(this, wxID_ANY, _("Sync"),
                                       wxDefaultPosition, wxDefaultSize, 0);
  bsizersimButtons->Add(m_SyncButton, 0, wxALL, 5);
  m_SyncButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
                              &RopelessDialog::OnSyncButton, this);

  // Connection Status Display
  m_ConnectionStatusText = new wxStaticText( this, wxID_ANY, _("Mode: UDP"), wxDefaultPosition, wxDefaultSize, 0 );
  m_ConnectionStatusText->Wrap( -1 );
  bsizersimButtons->Add( m_ConnectionStatusText, 0, wxALL, 5 );

  if (pParentPi->m_simulatorTimer.IsRunning()) {
    m_StartSimButton->Hide();
    m_StopSimButton->Show();
  } else {
    m_StopSimButton->Hide();
    m_StartSimButton->Show();
  }
#endif

  m_sdbSizer1 = new wxStdDialogButtonSizer();
  m_sdbSizer1OK = new wxButton(this, wxID_OK);
  m_sdbSizer1->AddButton(m_sdbSizer1OK);
  m_sdbSizer1->Realize();

  bSizer2->Add(m_sdbSizer1, 0, wxBOTTOM | wxEXPAND | wxTOP, 5);

  this->SetSizer(bSizer2);
  this->Layout();
  // bSizer2->Fit( this );

  this->Centre(wxBOTH);

}

RopelessDialog::~RopelessDialog() {

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
    wxString sdp;
    sdp.Printf("%g", state->depth);
    // item.SetText(sdp);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlDEPTH, sdp);
    m_pListCtrlTranponders->SetColumnWidth(tlDEPTH, wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlTEMP);
    wxString stemp;
    stemp.Printf("%g", state->temp);
    // item.SetText(stemp);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlTEMP, stemp);
    m_pListCtrlTranponders->SetColumnWidth(tlTEMP, wxLIST_AUTOSIZE_USEHEADER);

    // item.SetColumn(tlPINGS);
    wxString sping;
    sping.Printf("%d", state->pings);
    // item.SetText(sping);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlPINGS, sping);
    m_pListCtrlTranponders->SetColumnWidth(tlPINGS, wxLIST_AUTOSIZE_USEHEADER);

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
    wxString sbatt;
    sbatt.Printf("%d", state->batt_stat);
    // item.SetText(sdist);
    // m_pListCtrlTranponders->SetItem(item);
    m_pListCtrlTranponders->SetItem(result, tlBATT_STAT, sbatt);
    m_pListCtrlTranponders->SetColumnWidth(tlBATT_STAT,
                                           wxLIST_AUTOSIZE_USEHEADER);
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

}

void RopelessDialog::OnChooseFileButton(wxCommandEvent &event) {
  wxString file;
  int response = PlatformFileSelectorDialog(
      NULL, &file, _("Select an NMEA text file"),
      *GetpPrivateApplicationDataLocation(), _T(""), _T("*.*"));

  if (response == wxID_OK) {
    if (::wxFileExists(file)) {
      msgFileName = file;
      m_simTextCtrl->SetValue(msgFileName);
    }
  }
}

void RopelessDialog::OnStopSimButton(wxCommandEvent &event) {
  SetCanvasContextMenuItemViz(pParentPi->m_start_sim_id, true);
  SetCanvasContextMenuItemViz(pParentPi->m_stop_sim_id, false);

  m_StopSimButton->Hide();
  m_StartSimButton->Show();

  pParentPi->stopSim();
  Layout();
}

void RopelessDialog::OnStartSimButton(wxCommandEvent &event) {
  wxLogMessage("OnStartSimButton!");

  if (::wxFileExists(msgFileName)) {
    SetCanvasContextMenuItemViz(pParentPi->m_start_sim_id, false);
    SetCanvasContextMenuItemViz(pParentPi->m_stop_sim_id, true);
    m_StartSimButton->Hide();
    m_StopSimButton->Show();
    pParentPi->startSim();
    Layout();
  }
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
    g_ropelessPI->SendReleaseMessage(&g_ropelessPI->manualReleaseState, eCMD_RELEASE);

  }
}

void RopelessDialog::OnSyncButton(wxCommandEvent &event)
{
  g_ropelessPI->SendSyncMessage();
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
  clearHighlighted();

#ifndef __ANDROID__
  wxPoint p = GetPosition();
  pParentPi->m_dialogPosX = p.x;
  pParentPi->m_dialogPosY = p.y;
  wxSize s = GetSize();
  pParentPi->m_dialogSizeWidth = s.x;
  pParentPi->m_dialogSizeHeight = s.y;
#endif
  // wxLogMessage("Closing Ropeless window [x]...");
  // event.Skip();
  Destroy();
  pParentPi->m_pRLDialog = NULL;
}

void RopelessDialog::OnOKClick(wxCommandEvent &event) {
  clearHighlighted();

#ifndef __ANDROID__
  m_StopSimButton->Hide();
  m_StartSimButton->Show();
  g_ropelessPI->stopSim();
#endif
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