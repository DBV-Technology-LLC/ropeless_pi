/***************************************************************************
 *
 * Project:  OpenCPN
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register                               *
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
#include <typeinfo>
#include <map>
#include <wx/graphics.h>
#include <wx/popupwin.h>
#include <wx/window.h>
//#include <wx/sound.h>
#include <wx/timer.h>

#include "config.h"
#include "ropeless_pi.h"
#include "RopelessDialog.h"
#include "RopelessPrefsDialog.h"
#include "icons.h"
#include "Select.h"
#include "vector2d.h"
#include "dsPortType.h"
#include "OCP_DataStreamInput_Thread.h"
#include "OCPN_DataStreamEvent.h"
#include "georef.h"
#include "OCPNListCtrl.h"
#include "pugixml.hpp"
#include "mynumdlg.h"
#include "myokdlg.h"
#include "manualPlacementDlgImpl.h"
#include "haversine.h"

#ifdef __WXMSW__
#include <windows.h>
#endif

#ifdef ocpnUSE_GL
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif

#include <sstream>
#include <iostream>
#include <string.h>
#include <functional>

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double *)&lNaN)
#endif

#ifdef __ANDROID__

char qtRLStyleSheet[] =
    "QScrollBar:horizontal {border: 0px solid grey; background-color: rgb(240, 240, 240); height: 30px; margin: 0px 1px 0 1px;}\
QScrollBar::handle:horizontal {background-color: rgb(200, 200, 200); min-width: 20px; border-radius: 10px; }\
QScrollBar::add-line:horizontal {border: 0px solid grey; background: #32CC99; width: 0px; subcontrol-position: right; subcontrol-origin: margin; }\
QScrollBar::sub-line:horizontal {border: 0px solid grey; background: #32CC99; width: 0px; subcontrol-position: left; subcontrol-origin: margin; }\
QScrollBar:vertical {border: 0px solid grey; background-color: rgb(240, 240, 240); width: 30px; margin: 1px 0px 1px 0px; }\
QScrollBar::handle:vertical {background-color: rgb(200, 200, 200); min-height: 50px; border-radius: 10px; }\
QScrollBar::add-line:vertical {border: 0px solid grey; background: #32CC99; height: 0px; subcontrol-position: top; subcontrol-origin: margin; }\
QScrollBar::sub-line:vertical {border: 0px solid grey; background: #32CC99; height: 0px; subcontrol-position: bottom; subcontrol-origin: margin; }\
";
#endif

#ifdef __WXMSW__
wxEventType wxEVT_PI_OCPN_DATASTREAM = wxNewEventType();
#else
wxEventType wxEVT_PI_OCPN_DATASTREAM;  // = wxNewEventType();
#endif

//      Global Static variable
PlugIn_ViewPort *g_vp;
PlugIn_ViewPort g_ovp;

double g_Var;  // assummed or calculated variation

NMEA0183 g_NMEA0183;  // Used to parse NMEA Sentences
deckbox_status g_deckboxStatus;  // Global deckbox status
double gLat, gLon, gSog, gCog, gHdt, gHdm, gVar;
bool ll_valid;
bool pos_valid;

#if wxUSE_GRAPHICS_CONTEXT
wxGraphicsContext *g_gdc;
#endif
wxDC *g_pdc;

wxArrayString g_iconTypeArray;
int gHDT_Watchdog;
int gGPS_Watchdog;

ropeless_pi *g_ropelessPI;

bool g_bRopelessTargetList_sortReverse;
int g_RopelessTargetList_sortColumn;
std::vector<transponder_state *> transponderStatus;
std::vector<trawl_tracker *> trawlList;

wxPopupWindow *popup;
wxStaticText *popupText;
bool popupVis = false;

// Simulation Variables
int n_tick;
double countRun;
double countTarget;
double accelFactor;
wxString pendingMsg;
unsigned int inext;
unsigned int msgCount;
wxTextFile msgFile;
double tstamp_current;

#include <wx/arrimpl.cpp>  // this is a magic incantation which must be done!

WX_DEFINE_OBJARRAY(ArrayOf2DPoints);

#include "default_pi.xpm"
#include "ocpn_plugin.h"

// // Color table for Transponder rendering
// // -- supports colorblind mode via 3rd column
// wxString colorTable[3][3] = {
//             { "Owned", "LIME GREEN", "CYAN" },
//             { "Non-Owned", "ORANGE", "ORANGE" },
//             { "Cloud", "GREY", "GREY"},
//         };

// const wxString colorNames[] = {
//     "Black", "Blue", "Cyan", "Green", "Yellow", "Red", "White",
//     "Light Grey", "Medium Grey", "Grey", "Dark Grey"
// };

wxString colorTableNames[COLOR_TABLE_COUNT] = {"ORANGE",  // looks darker green
                                          "LIME GREEN",      // looks darker red
                                          "MAGENTA", "CYAN", "YELLOW"};

wxString colorTableNamesColorblind[COLOR_TABLE_COUNT] = {"ORANGE",       // colorblind-friendly cyan instead of green (owned)
                                        "CYAN",     // colorblind-friendly orange instead of red (non-owned)
                                        "MAGENTA", "CYAN", "YELLOW"};
  
// TODO: Put an example sim file packaged with app for demo purposes                          
wxString msgFileName = "/home/dsr/Projects/ropeless_pi/NMEArevC_06072023.txt";

wxDateTime DaysTowDT(double days) {
  int daysRoundDown = (int)days;
  int daysLinuxEpoch = daysRoundDown - 719528;

  double fraction = days - daysRoundDown;
  double fraction_secs = fraction * 24 * 3600;
  time_t epochTime = (int)(daysLinuxEpoch * 24 * 3600) + (int)fraction_secs;
  return wxDateTime(epochTime);
}

// the class factories, used to create and destroy instances of the PlugIn
extern "C" DECL_EXP opencpn_plugin *create_pi(void *ppimgr) {
  return (opencpn_plugin *)new ropeless_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin *p) { delete p; }

wxString getWaypointName(wxString &GUID) {
  PlugIn_Waypoint pwp;
  if (GetSingleWaypoint(GUID, &pwp))
    return pwp.m_MarkName;
  else
    return _T("");
}

/*  These two function were taken from gpxdocument.cpp */
int GetRandomNumber(int range_min, int range_max) {
  long u = (long)wxRound(
      ((double)rand() / ((double)(RAND_MAX) + 1) * (range_max - range_min)) +
      range_min);
  return (int)u;
}

// RFC4122 version 4 compliant random UUIDs generator.
wxString GetUUID(void) {
  wxString str;
  struct {
    int time_low;
    int time_mid;
    int time_hi_and_version;
    int clock_seq_hi_and_rsv;
    int clock_seq_low;
    int node_hi;
    int node_low;
  } uuid;

  uuid.time_low = GetRandomNumber(
      0, 2147483647);  // FIXME: the max should be set to something like
                       // MAXINT32, but it doesn't compile un gcc...
  uuid.time_mid = GetRandomNumber(0, 65535);
  uuid.time_hi_and_version = GetRandomNumber(0, 65535);
  uuid.clock_seq_hi_and_rsv = GetRandomNumber(0, 255);
  uuid.clock_seq_low = GetRandomNumber(0, 255);
  uuid.node_hi = GetRandomNumber(0, 65535);
  uuid.node_low = GetRandomNumber(0, 2147483647);

  /* Set the two most significant bits (bits 6 and 7) of the
   * clock_seq_hi_and_rsv to zero and one, respectively. */
  uuid.clock_seq_hi_and_rsv = (uuid.clock_seq_hi_and_rsv & 0x3F) | 0x80;

  /* Set the four most significant bits (bits 12 through 15) of the
   * time_hi_and_version field to 4 */
  uuid.time_hi_and_version = (uuid.time_hi_and_version & 0x0fff) | 0x4000;

  str.Printf(_T("%08x-%04x-%04x-%02x%02x-%04x%08x"), uuid.time_low,
             uuid.time_mid, uuid.time_hi_and_version, uuid.clock_seq_hi_and_rsv,
             uuid.clock_seq_low, uuid.node_hi, uuid.node_low);

  return str;
}

void Clone_VP(PlugIn_ViewPort *dest, PlugIn_ViewPort *src) {
  dest->clat = src->clat;  // center point
  dest->clon = src->clon;
  dest->view_scale_ppm = src->view_scale_ppm;
  dest->skew = src->skew;
  dest->rotation = src->rotation;
  dest->chart_scale = src->chart_scale;
  dest->pix_width = src->pix_width;
  dest->pix_height = src->pix_height;
  dest->rv_rect = src->rv_rect;
  dest->b_quilt = src->b_quilt;
  dest->m_projection_type = src->m_projection_type;

  dest->lat_min = src->lat_min;
  dest->lat_max = src->lat_max;
  dest->lon_min = src->lon_min;
  dest->lon_max = src->lon_max;

  dest->bValid = src->bValid;  // This VP is valid
}

// static int CompareD(double a, double b) {
//   if (g_bRopelessTargetList_sortReverse) {
//     if (a > b)
//       return 1;
//     else if (a < b)
//       return -1;
//     else
//       return 0;
//   } else {
//     if (a > b)
//       return -1;
//     else if (a < b)
//       return 1;
//     else
//       return 0;
//   }
//   return 0;
// }



//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

//      Event Handler implementation
BEGIN_EVENT_TABLE(ropeless_pi, wxEvtHandler)
EVT_TIMER(TIMER_THIS_PI, ropeless_pi::ProcessTimerEvent)
EVT_TIMER(SIM_TIMER, ropeless_pi::ProcessSimTimerEvent)
EVT_TIMER(RELEASE_TIMER, ropeless_pi::ProcessReleaseTimerEvent)
EVT_TIMER(DISTANCE_TIMER, ropeless_pi::ProcessDistanceTimerEvent)
END_EVENT_TABLE()

ropeless_pi::ropeless_pi(void *ppimgr)
    : wxTimer(this), opencpn_plugin_112(ppimgr) {
  g_ropelessPI = this;
  m_pplugin_icon = new wxBitmap(default_pi);
}

ropeless_pi::~ropeless_pi(void) {}

int ropeless_pi::Init(void) {
  AddLocaleCatalog(_T("opencpn-ropeless_pi"));
  m_config_version = -1;

  m_oDC = NULL;

  //  Configure the NMEA processor
  mHDx_Watchdog = 2;
  mHDT_Watchdog = 2;
  mGPS_Watchdog = 2;
  mVar_Watchdog = 2;

  gHDT_Watchdog = 10;
  gGPS_Watchdog = 10;

  mVar = 0;
  m_hdt = 0;
  m_ownship_cog = 0;
  m_nfix = 0;
  m_bshow_fix_hat = false;
  
  // Initialize cloud ID counter (starts at 32768 = 65535/2)
  m_nextCloudId = 32768;

  m_Thread_run_flag = -1;

  g_iconTypeArray.Add(_T("Scaled Vector Icon"));
  g_iconTypeArray.Add(_T("Generic Ship Icon"));

  m_NMEA0183.TalkerID = _T ( "RF" );
  m_NMEA0183_tx.TalkerID = _T ( "CP" );

  //     Length = 41.1
  //     Beam = 10.7
  //     GPS offset from bow = 37.1
  //     GPS offset from midship = 2.6

  //    Get a pointer to the opencpn display canvas, to use as a parent for the
  //    POI Manager dialog
  m_parent_window = GetOCPNCanvasWindow();

  gHdt = NAN;
  gHdm = NAN;
  gVar = NAN;
  gSog = NAN;
  gCog = NAN;

  //    Get a pointer to the opencpn configuration object
  m_pconfig = GetOCPNConfigObject();

  m_event_handler = new PI_EventHandler(this);
  m_tsock = NULL;
  m_nmea_tcp_output = NULL;
  
  // Initialize configuration variables to default values before loading config
  m_tcp_enabled = true;
  m_tcp_auto_reconnect = true;
  m_tcp_host = "127.0.0.1";
  m_tcp_port = 4001;
  m_colorblind_mode = false;
  m_hide_transponder_text = false;
  m_transponder_circle_size = 10;
  m_transponder_text_size = 12;
  m_debug_enabled = false;
  m_debug_show_nmea = false;
  m_debug_show_log = false;
  m_pRLDialog = nullptr;
  m_pPrefsDialog = nullptr;
  m_loopbackNMEATx = false;     // TODO: Implement this

  //    And load the configuration items
  LoadConfig();
  
  // Initialize TCP NMEA Output to 127.0.0.1:4001
  InitializeTCPOutput();

  //     m_pTrackRolloverWin = new RolloverWin( GetOCPNCanvasWindow() );
  //     m_pTrackRolloverWin->SetPosition( wxPoint( 5, 150 ) );
  //     m_pTrackRolloverWin->IsActive( false );

  //     m_pTrackRolloverWin->SetString( _T("Brg:   0\nDist:   0") );
  //     m_pTrackRolloverWin->SetBestSize();
  //     m_pTrackRolloverWin->SetBitmap( 0 );
  //     m_pTrackRolloverWin->SetPosition( wxPoint( 5, 50 ) );
  //
  //     m_pTrackRolloverWin->IsActive( true );

  SetOwner(this, TIMER_THIS_PI);
  Start(100, wxTIMER_CONTINUOUS);

#ifdef SHOW_DISTANCE
  startDistanceTimer();
#endif

#if 0
#ifndef __WXMSW__
    wxEVT_PI_OCPN_DATASTREAM = wxNewEventType();
#endif

    m_event_handler = new PI_EventHandler(this);
    // DISABLED: m_serialThread = NULL;

    //startSerial(m_serialPort);

    m_RolloverPopupTimer.SetOwner( m_event_handler, ROLLOVER_TIMER );
    m_rollover_popup_timer_msec = 20;

    m_select = new Select();
    m_tenderSelect = NULL;

    m_head_dog_timer.SetOwner( m_event_handler, HEAD_DOG_TIMER );
    m_head_active = false;

    setTrackedWPSelect(m_trackedWPGUID);
#endif
  m_select = new Select();

  initialize_images();
  m_pRLDialog = NULL;

  m_colorIndexNext = 0;

  LoadTransponderStatus();  // Load persistant XML file

  wxMenuItem *rptrp =
      new wxMenuItem(NULL, ID_TPR_PLACE, _("Ropeless: Place Trap Manually"));
  m_place_trap_manually = AddCanvasContextMenuItem(rptrp, this);
  SetCanvasContextMenuItemViz(m_place_trap_manually, true);

  wxMenuItem *rptrp2 =
      new wxMenuItem(NULL, ID_TPR_PLACE, _("Ropeless: Place Trap at Vessel Position"));
  m_place_trap_now = AddCanvasContextMenuItem(rptrp2, this);
  SetCanvasContextMenuItemViz(m_place_trap_now, true);

  popup = new wxPopupWindow(m_parent_window);
  popupText =
      new wxStaticText(popup, wxID_ANY, "TEST", wxPoint(5, 5), wxSize(100, 20));
  popup->SetSize(110, 30);
  bool popupVis = false;

  // This PlugIn needs a toolbar icon, so request its insertion
  wxFileName fn;
  wxString tmp_path;
  tmp_path = GetPluginDataDir("ropeless_pi");
  fn.SetPath(tmp_path);
  fn.AppendDir(_T("data"));

  fn.SetFullName(_T("rsi-icon-color-solid.svg"));
  wxString tb_icon = fn.GetFullPath();

  InsertPlugInToolSVG("Ropeless", tb_icon, tb_icon, tb_icon, wxITEM_CHECK,
                      "Ropeless", "", NULL, -1, 0, this);

  return (WANTS_OVERLAY_CALLBACK | WANTS_OPENGL_OVERLAY_CALLBACK |
          WANTS_CURSOR_LATLON | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL |
          WANTS_CONFIG | WANTS_PREFERENCES | WANTS_PLUGIN_MESSAGING |
          WANTS_NMEA_SENTENCES | WANTS_NMEA_EVENTS | WANTS_PREFERENCES |
          WANTS_MOUSE_EVENTS | INSTALLS_CONTEXTMENU_ITEMS);
}

bool ropeless_pi::DeInit(void) {
  // Clear global pointer FIRST to prevent other code from accessing plugin
  g_ropelessPI = nullptr;
  
  // Stop all timers to prevent callbacks during shutdown
  m_simulatorTimer.Stop();
  m_releaseTimer.Stop();
  m_distanceTimer.Stop();
  m_RolloverPopupTimer.Stop();
  m_head_dog_timer.Stop();
  
  // Disconnect ALL event handlers that reference 'this' plugin
  try {
    wxWindow* canvas = GetOCPNCanvasWindow();
    if (canvas) {
      // Disconnect popup menu event handlers to prevent callbacks to destroyed plugin
      canvas->Disconnect(ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
                        wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
      canvas->Disconnect(ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
                        wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
      canvas->Disconnect(ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
                        wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
      canvas->Disconnect(ID_TPR_EDIT, wxEVT_COMMAND_MENU_SELECTED,
                        wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
    }
  } catch (...) {
    // Ignore any exceptions - canvas may already be destroyed
  }
  
  // Remove plugin UI elements AFTER disconnecting events
  try {
    RemovePlugInTool(m_leftclick_tool_id);
    RemoveCanvasContextMenuItem(m_place_trap_manually);
    RemoveCanvasContextMenuItem(m_place_trap_now);
  } catch (...) {
    // Ignore any exceptions during UI cleanup
  }
  
  // Safely close dialogs without using potentially invalid parent windows
  if (m_pRLDialog) {
    try {
      if (m_pRLDialog->IsShown()) {
        m_pRLDialog->Hide();  // Hide first, safer than Close()
      }
      m_pRLDialog->Destroy();  // Then destroy
    } catch (...) {
      // Ignore any exceptions during cleanup
    }
    m_pRLDialog = nullptr;
  }
  
  // m_releaseDlg cleanup removed - functionality moved to RopelessDialog
  // if (m_releaseDlg) {
  //   try {
  //     if (m_releaseDlg->IsShown()) {
  //       m_releaseDlg->Hide();
  //     }
  //     m_releaseDlg->Destroy();
  //   } catch (...) {
  //     // Ignore any exceptions during cleanup
  //   }
  //   m_releaseDlg = nullptr;
  // }
  
  // Clean up preferences dialog if it exists
  if (m_pPrefsDialog) {
    try {
      if (m_pPrefsDialog->IsShown()) {
        m_pPrefsDialog->Hide();  // Hide first, safer than Close()
      }
      m_pPrefsDialog->Destroy();  // Then destroy
    } catch (...) {
      // Ignore any exceptions during cleanup
    }
    m_pPrefsDialog = nullptr;
  }
  
  // Clean up popup window safely
  if (popup) {
    try {
      if (popup->IsShown()) {
        popup->Hide();
      }
      popup->Destroy();
    } catch (...) {
      // Ignore any exceptions during cleanup
    }
    popup = nullptr;
  }
  
  // Save transponder status to XML file before shutdown
  try {
    SaveTransponderStatus();
    wxLogMessage("Ropeless Plugin: Transponder status saved to XML during shutdown");
  } catch (...) {
    wxLogMessage("Ropeless Plugin: ERROR - Failed to save transponder status to XML during shutdown");
  }
  
  // Clean up TCP output connection
  try {
    ShutdownTCPOutput();
  } catch (...) {
    // Ignore any exceptions during TCP cleanup
  }
  
  // Clear event handler
  if (m_event_handler) {
    try {
      delete m_event_handler;
    } catch (...) {
      // Ignore any exceptions during cleanup
    }
    m_event_handler = nullptr;
  }
  
  // Clear window pointer to prevent use of invalid parent
  m_parent_window = nullptr;
  
  // Final safety - clear all dialog pointers
  m_pRLDialog = nullptr;
  // m_releaseDlg = nullptr;  // Functionality moved to RopelessDialog
  
  return true;
}

int ropeless_pi::GetAPIVersionMajor() { return MY_API_VERSION_MAJOR; }

int ropeless_pi::GetAPIVersionMinor() { return MY_API_VERSION_MINOR; }

int ropeless_pi::GetPlugInVersionMajor() { return PLUGIN_VERSION_MAJOR; }

int ropeless_pi::GetPlugInVersionMinor() { return PLUGIN_VERSION_MINOR; }

wxBitmap *ropeless_pi::GetPlugInBitmap() { return m_pplugin_icon; }

wxString ropeless_pi::GetCommonName() { return _("Ropeless"); }

wxString ropeless_pi::GetShortDescription() {
  return _("Ropeless PlugIn for OpenCPN");
}

wxString ropeless_pi::GetLongDescription() {
  return _("Ropeless PlugIn for OpenCPN");
}

void ropeless_pi::OnToolbarToolCallback(int id) {
  // if (!m_buseable) return;
  if (NULL == m_pRLDialog) {
    m_pRLDialog = new RopelessDialog(
        m_parent_window, this, -1, "Ropeless Fishing v3.0.0.1", wxDefaultPosition,
        wxDefaultSize, wxCAPTION | wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
    m_pRLDialog->SetFont(*pFont);
  }

  // RearrangeWindow();
  /*m_pRLDialog->SetMaxSize(m_pRLDialog->GetSize());
  m_pRLDialog->SetMinSize(m_pRLDialog->GetSize());*/
  m_pRLDialog->Show(!m_pRLDialog->IsShown());
  m_pRLDialog->Layout();  // Some platforms need a re-Layout at this point
                          // (gtk, at least)

#ifndef __ANDROID__
  // Comment out saved size restoration to let dialog size to fit content
  // if ((m_dialogSizeWidth > 0) && (m_dialogSizeHeight > 0))
  //   m_pRLDialog->SetSize(wxSize(m_dialogSizeWidth, m_dialogSizeHeight));

  // if ((m_dialogPosX > 0) && (m_dialogPosY > 0))
  //   m_pRLDialog->Move(wxPoint(m_dialogPosX, m_dialogPosY));
#else

  wxSize parent_size = GetOCPNCanvasWindow()->GetSize();
  m_pRLDialog->SetSize(wxSize(parent_size.x * 7 / 10, parent_size.y * 5 / 10));
  m_pRLDialog->CentreOnScreen();
  m_pRLDialog->Move(-1, 0);

#endif

  //  wxPoint p = m_pRLDialog->GetPosition();
  //  m_pRLDialog->Move(0, 0);  // workaround for gtk autocentre dialog behavior
  //  m_pRLDialog->Move(p);

  m_pRLDialog->RefreshTransponderList();  // Pick up initial XML load
}

void ropeless_pi::OnContextMenuItemCallback(int id) {
  if (id == m_start_sim_id) {
    wxLogMessage("Ropeless: m_start_sim_id");

    wxString file;
    int response = PlatformFileSelectorDialog(
        NULL, &file, _("Select an NMEA text file"),
        *GetpPrivateApplicationDataLocation(), _T(""), _T("*.*"));

    if (response == wxID_OK) {
      msgFileName = file;
      if (::wxFileExists(msgFileName)) {
        SetCanvasContextMenuItemViz(m_start_sim_id, false);
        SetCanvasContextMenuItemViz(m_stop_sim_id, true);

        startSim();
      }
    }
  } else if (id == m_stop_sim_id) {
    wxLogMessage("Ropeless: m_stop_sim_id!");

    SetCanvasContextMenuItemViz(m_start_sim_id, true);
    SetCanvasContextMenuItemViz(m_stop_sim_id, false);

    stopSim();
  } else if (id == m_place_trap_manually) {
    double mp_lat = m_cursor_lat;
    double mp_lon = m_cursor_lon;

    wxString latStr = wxString::Format("%.7f", mp_lat);
    wxString lonStr = wxString::Format("%.7f", mp_lon);

    wxLogMessage("Placing Trap at Manual Location: %s, %s",latStr,lonStr);

    wxDateTime lognow = wxDateTime::Now();
    lognow.MakeGMT();
    wxString day = lognow.FormatISODate();
    wxString utc = lognow.FormatISOTime();

    time_t sec = lognow.GetTicks();
    double ms = lognow.GetMillisecond();
    double utc_d = static_cast <double>(sec) + (ms / 1000.0);

    //wxLogMessage("Manual Placement Date: %f",utc_d);

    manualPlacementDlgImpl mPl(GetOCPNCanvasWindow(), wxID_ANY,
                               _("Place Transponder"), wxDefaultPosition,
                               wxSize(-1, -1), wxDEFAULT_DIALOG_STYLE, latStr,
                               lonStr, day + " " + utc);

    int res = mPl.ShowModal();

    if (res == wxID_OK) {
      // wxLogMessage("Manual Placement res OK!");
      // wxLogMessage("Id: %d, Pair: %d, Owned: %d", mPl.markID, mPl.pairId,
      //              mPl.isOwned);

      if (mPl.valid) {
        placeTransponderManually(mPl.markID, mPl.pairId, mp_lat, mp_lon, utc_d, mPl.positionSource, mPl.isOwned ? 1 : 0);
        
        // Handle trawl assignment if a trawl was selected
        if (mPl.selectedTrawlId > 0) {
          transponder_state* newTransponder = GetStateByMarkID(mPl.markID);
          if (newTransponder) {
            newTransponder->trawl_id = mPl.selectedTrawlId;
            if (mPl.selectedTrawlId == 0) {
              newTransponder->is_trawl_start_end = false;  // Clear start/end flag when removing from trawl
              newTransponder->trawl_num = -1;  // Reset position when removing from trawl
            } else {
              newTransponder->is_trawl_start_end = false;  // Reset start/end flag when assigning to trawl
            }
            wxLogMessage("Assigned transponder %d to trawl %d", mPl.markID, mPl.selectedTrawlId);
          }
        }
      }
    }
  } else if (id == m_place_trap_now) {
    wxString ownlatStr = wxString::Format("%.7f", m_ownship_lat);
    wxString ownlonStr = wxString::Format("%.7f", m_ownship_lon);
    
    wxLogMessage("Placing Trap at Vessel Location: %s, %s",ownlatStr,ownlonStr);

    wxDateTime lognow = wxDateTime::Now();
    lognow.MakeGMT();
    wxString day = lognow.FormatISODate();
    wxString utc = lognow.FormatISOTime();

    time_t sec = lognow.GetTicks();
    double ms = lognow.GetMillisecond();
    double utc_d = static_cast <double>(sec) + (ms / 1000.0);

    manualPlacementDlgImpl mPl(GetOCPNCanvasWindow(), wxID_ANY,
                               _("Place Transponder"), wxDefaultPosition,
                               wxSize(-1, -1), wxDEFAULT_DIALOG_STYLE, ownlatStr,
                               ownlonStr, day + " " + utc);
    int res = mPl.ShowModal();

    if (res == wxID_OK) {
      // wxLogMessage("Manual Placement res OK!");
      // wxLogMessage("Id: %d, Pair: %d, Owned: %d", mPl.markID, mPl.pairId,
      //              mPl.isOwned);

      if (mPl.valid) {
        placeTransponderManually(mPl.markID, mPl.pairId, m_ownship_lat, m_ownship_lon, utc_d, mPl.positionSource, mPl.isOwned ? 1 : 0);
        
        // Handle trawl assignment if a trawl was selected
        if (mPl.selectedTrawlId > 0) {
          transponder_state* newTransponder = GetStateByMarkID(mPl.markID);
          if (newTransponder) {
            newTransponder->trawl_id = mPl.selectedTrawlId;
            if (mPl.selectedTrawlId == 0) {
              newTransponder->is_trawl_start_end = false;  // Clear start/end flag when removing from trawl
              newTransponder->trawl_num = -1;  // Reset position when removing from trawl
            } else {
              newTransponder->is_trawl_start_end = false;  // Reset start/end flag when assigning to trawl
            }
            wxLogMessage("Assigned transponder %d to trawl %d", mPl.markID, mPl.selectedTrawlId);
          }
        }
      }
    }
  }
}

// Called from wxMenu AND OnTargetRightClick
void ropeless_pi::PopupMenuHandler(wxCommandEvent &event) {
  bool handled = false;
  switch (event.GetId()) {

    case ID_PLAY_SIM: {
      startSim();
      wxLogMessage("Starting Sim!!!");
      handled = true;
      break;
    }
    case ID_EPL_XMIT: {
      m_NMEA0183.TalkerID = _T("EC");

      SENTENCE snt;

      if (m_fix_lat < 0.)
        m_NMEA0183.Gll.Position.Latitude.Set(-m_fix_lat, _T("S"));
      else
        m_NMEA0183.Gll.Position.Latitude.Set(m_fix_lat, _T("N"));

      if (m_fix_lon < 0.)
        m_NMEA0183.Gll.Position.Longitude.Set(-m_fix_lon, _T("W"));
      else
        m_NMEA0183.Gll.Position.Longitude.Set(m_fix_lon, _T("E"));

      wxDateTime now = wxDateTime::Now();
      wxDateTime utc = now.ToUTC();
      wxString time = utc.Format(_T("%H%M%S"));
      m_NMEA0183.Gll.UTCTime = time;

      m_NMEA0183.Gll.Mode = _T("M");  // Use GLL 2.3 specification
                                      // and send "M" for "Manual fix"
      m_NMEA0183.Gll.IsDataValid =
          NFalse;  // Spec requires "Invalid" for manual fix

      m_NMEA0183.Gll.Write(snt);

      wxLogMessage(snt.Sentence);

      // TODO: Make this an option to 
      //PushNMEABuffer(snt.Sentence);
      
      // Display sent NMEA message in debug window
      if (m_pRLDialog) {
        m_pRLDialog->AddDebugMessage("--> " + snt.Sentence.Trim());
      }

      handled = true;
      break;
    }

    case ID_TPR_RELEASE: {
      
      ConfirmAndReleaseTransponder(g_ropelessPI->m_foundState);

      handled = true;
      break;
    }

    case ID_TPR_DELETE: {
      ConfirmAndDeleteTransponder(g_ropelessPI->m_foundState->markID);
      handled = true;
      break;
    }
    case ID_TPR_EDIT: {
      EditTransponder(g_ropelessPI->m_foundState);
      handled = true;
      break;
    }
    case ID_TPR_PLACE: {
      handled = true;
      break;
    }
    case ID_TPR_RECOVER: {

      toggleTransponderRecovered(m_foundState->markID);

      handled = true;
      break;
    }
    default:
      break;
  }

  if (!handled) event.Skip();
}

#if 0
void ropeless_pi::setTrackedWPSelect(wxString GUID)
{
    if(GUID.Length()){
        // if it is already set
        m_select->DeleteSelectablePoint( this, SELTYPE_POINT_GENERIC, SELTYPE_ROUTEPOINT );

        // Get the WP location

        bool bfound = false;
        wxArrayString guidArray = GetWaypointGUIDArray();
        for(unsigned int i=0 ; i < guidArray.GetCount() ; i++){
            if(GUID.IsSameAs( guidArray[i] )){
                if(GetSingleWaypoint( GUID, &m_TrackedWP )){
                    bfound = true;
                    break;
                }
            }
        }

        if(bfound){
            m_select->AddSelectablePoint( m_TrackedWP.m_lat, m_TrackedWP.m_lon, this, SELTYPE_ROUTEPOINT, 0 );
        }
    }
}
#endif

unsigned char ropeless_pi::ComputeChecksum(wxString msg) {
  unsigned char checksum_value = 0;

  char str_ascii[101];
  const char* mb_str = (const char *)msg.mb_str();
  if (!mb_str) {
    wxLogMessage("ERROR: ComputeChecksum received NULL mb_str");
    return 0;
  }
  strncpy(str_ascii, mb_str, 99);
  str_ascii[100] = '\0';

  int string_length = strlen(str_ascii);
  int index = 1;  // Skip over the $ at the begining of the sentence

  while (index < string_length && str_ascii[index] != '*' &&
         str_ascii[index] != CARRIAGE_RETURN && str_ascii[index] != LINE_FEED) {
    checksum_value ^= str_ascii[index];
    index++;
  }

  return (checksum_value);
}

void ropeless_pi::GlobalDebugMessage(const wxString& message, bool alsoLog) {
  // Send to dialog if available
  if (m_pRLDialog) {
    m_pRLDialog->DebugMessage(message, false); // Don't log twice
  }
  
  // Always log if requested
  if (alsoLog) {
    wxLogMessage(message);
  }
}

// Global function accessible from anywhere in the plugin
void GlobalRopelessDebugMessage(const wxString& message, bool alsoLog) {
  if (g_ropelessPI) {
    g_ropelessPI->GlobalDebugMessage(message, alsoLog);
  } else {
    // Fallback to just logging if plugin instance is not available
    if (alsoLog) {
      wxLogMessage(message);
    }
  }
}

bool ropeless_pi::SendCommandMessage(transponder_state *state, long code) {
  bool ret = true;

  wxLogMessage("Sending Command Message! %d,code");

  // Don't send a release if we're actively tracking a current request
  if (m_release_tim_state.timer_state == 1 && code == eCMD_RELEASE) {
    wxLogMessage("Release in progress! Can't send release yet...");
    return false;
  }

  if (state == NULL) 
  {
    wxLogMessage("Error: SendCommandMessage state = NULL");
    return false;
  }

  // Create GMR message instead of RSRLB
  GMR gmr_msg;
  gmr_msg.CmdUID = 1;           // Command UID (can be incremented for tracking)
  gmr_msg.SourceID = 0;         // Source ID (0 = this system)
  gmr_msg.TargetID = 0;         // Target ID (0 = broadcast)
  gmr_msg.MarkID = state->markID;// Mark ID (transponder identifier)
  gmr_msg.CmdType = code;       // Command type from enum
  gmr_msg.ResCode = 0;          // Response code (0 for requests)
  gmr_msg.Param1 = 0;           // Additional parameter 1
  gmr_msg.Param2 = 0;           // Additional parameter 2

  gmr_msg.SetContainer(&m_NMEA0183_tx);

  // wxLogMessage("GMR Message created: CmdUID=%d, MarkID=%d, CmdType=%ld", 
  //              gmr_msg.CmdUID, gmr_msg.MarkID, gmr_msg.CmdType);
  
  // Send NMEA message via TCP!
  SendNMEAMessageTCP(&gmr_msg);

  // TODO: Track status of release requestsNo 
  // if (code == eCMD_RELEASE)
  // {
  //   wxLogMessage("SendCommandMessage: Processing RELEASE command for ID %d, ret=%s", state->markID, ret ? "true" : "false");
  //   if (ret != false) {
  //     state->release_status = -5;
  //     m_release_tim_state.ptstate = state;
  //     wxLogMessage("SendCommandMessage: About to call startReleaseTimer()");
  //     startReleaseTimer();
  //     wxLogMessage("SendCommandMessage: startReleaseTimer() completed");
  //   } 
  //   else{
  //     state->release_status = -4;
  //     m_release_tim_state.ptstate = state;
  //     stopReleaseTimer();

  //     wxLogMessage("Release request failed!");
  //     wxLogMessage("SendCommandMessage: About to call updateReleaseDialog(true)");
  //     updateReleaseDialog(true);
  //     wxLogMessage("SendCommandMessage: updateReleaseDialog(true) completed");
  //   }
  // }

  return ret;
}

void ropeless_pi::updateReleaseDialog(bool show)
{
  // Functionality moved to RopelessDialog Release Status section
  // if (NULL == m_releaseDlg) {
  //   m_releaseDlg = new transponderReleaseDlgImpl(...);
  //   wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
  //   m_releaseDlg->SetFont(*pFont);
  // }
  
  if (!m_pRLDialog) {
    return;  // Main dialog not available
  }

  int rlsNum;
  wxString appendStr = "";
  wxString rid;

  transponder_state* state = m_release_tim_state.ptstate;

  if (state == NULL) 
  {
    wxLogMessage("Release Dialog has NULL ptr");
    return;
  }

  if (state->release_status == -5)
  {
    rlsNum = eRELEASE_CONNECTING;
  }
  else if (state->release_status == -4)
  {
    rlsNum = eRELEASE_NETWORK_ERR;
  }
  else if (state->release_status == -3)
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
    m_releaseTimer.Start(RELEASE_TIME_MS);
  }
  else
  {
    state->release_status = -2;
    rlsNum = eRELEASE_NOT_INIT;
  }

  rid.Printf("%s%s", releaseStatusNames[rlsNum],appendStr);
  
  // Update the Release Status section in the main dialog instead of separate dialog
  wxLogMessage("updateReleaseDialog: About to call UpdateReleaseStatusInfo for ID %d with status '%s'", state->markID, rid);
  m_pRLDialog->UpdateReleaseStatusInfo(state->markID, rid);
  wxLogMessage("updateReleaseDialog: UpdateReleaseStatusInfo completed");

  if (state->release_status != -5 && state->release_status <= 0)
  {
    m_release_tim_state.timer_state = 0;
  }
  
  // No need to show/hide separate dialog - functionality is now in main dialog
}

void ropeless_pi::startReleaseTimer(void){

  m_release_tim_state.timer_state = 1;

  updateReleaseDialog(true);

  m_releaseTimer.SetOwner(this, RELEASE_TIMER);
  m_releaseTimer.Stop();
  m_releaseTimer.Start(RELEASE_TIME_MS, wxTIMER_CONTINUOUS);
}

void ropeless_pi::stopReleaseTimer(void) { 
  m_release_tim_state.timer_state = 0;
  m_releaseTimer.Stop(); 
}

void ropeless_pi::updateReleaseTimer(transponder_state * state)
{
  // We got an RLA message from the NMEA input
  // - are we currently displaying release state?
  // - does this match the currently tracked transponder?
  m_releaseTimer.Stop();

  if (m_release_tim_state.timer_state == 0 || m_release_tim_state.ptstate == NULL) return;

  transponder_state* timer_state = m_release_tim_state.ptstate;

  if (state->markID != timer_state->markID) return;

  updateReleaseDialog(true);
}

void ropeless_pi::startDistanceTimer() {
  m_distanceTimer.SetOwner(this, DISTANCE_TIMER);
  m_distanceTimer.Start(1000, wxTIMER_CONTINUOUS);
}

void ropeless_pi::stopDistanceTimer() { m_distanceTimer.Stop(); }

void ropeless_pi::ProcessDistanceTimerEvent(wxTimerEvent &event) {

  //TODO: Only calculate this when dialog is opened or on button press to save processing time?
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state* t = transponderStatus[i];

    if (t->predicted_lat > 90.0 || t->predicted_lon > 180.0) {
      t->distance = -1.0;
    }
    else{
      t->distance = haversineDistance(m_ownship_lat,m_ownship_lon,t->predicted_lat,t->predicted_lon);
    }

    double distnmi = t->distance / 1852.0;

    if (t->position_source == ePOS_SOURCE_CLOUD)
    {
      if (distnmi > 2.0)
      {
        wxLogMessage("Deleting Cloud transponder outside 2nmi! Ident: " + t->markID);
        DeleteTransponder(t->markID);
      }
      else if (distnmi > 1.0 && t->hide_pos == false)
      {
        wxLogMessage("Hiding cloud transponder!");
        t->hide_pos = true;
      }
      else if (distnmi < 1.0 && t->hide_pos == true)
      {
        wxLogMessage("Showing Transponder!");
        t->hide_pos = false;
      }
    }
  }

  if (m_pRLDialog != NULL) {
    // Refresh the list to show the updated distances
    m_pRLDialog->RefreshTransponderList();
  }
}

void ropeless_pi::ProcessReleaseTimerEvent(wxTimerEvent &event) {

  stopReleaseTimer();

  transponder_state* this_transponder_state = m_release_tim_state.ptstate;
  if (this_transponder_state != NULL)
  {
    m_release_tim_state.ptstate->release_status = -3;
    wxLogMessage("Release Timer Expired for ID: %d",m_release_tim_state.ptstate->markID);
    updateReleaseDialog(true);
  }

  //RequestRefresh(GetOCPNCanvasWindow());
}

void ropeless_pi::startSim() {

  // Open the data file
  msgFile.Open(msgFileName);
  msgCount = msgFile.GetLineCount();
  inext = 0;
  n_tick = 0;

  countRun = 0;
  countTarget = 5;
  accelFactor = 40;

  m_simulatorTimer.SetOwner(this, SIM_TIMER);
  m_simulatorTimer.Start(100, wxTIMER_CONTINUOUS);
}

void ropeless_pi::stopSim() { m_simulatorTimer.Stop(); }

void ropeless_pi::ProcessSimTimerEvent(wxTimerEvent &event) {
  n_tick++;
  countRun += .100 * accelFactor;  // 100 msec basic timer

  if (countRun < countTarget) {
    if ((n_tick % 10) == 0) {  // once per second
      printf("next msg in: %g\n", countTarget - countRun);
    }
  } else {
    //  Send the pending msg
    SetNMEASentence(pendingMsg);
    RequestRefresh(GetOCPNCanvasWindow());

    // Fetch the next message
    if (inext < msgCount) {
      pendingMsg = msgFile.GetLine(inext);
      pendingMsg.Append("\r\n");
      inext++;

      double tstamp_last = tstamp_current;

      // parse the pending msg to get the timestamp
      m_NMEA0183 << pendingMsg;

      if (m_NMEA0183.PreParse()) {
        if (m_NMEA0183.Parse()) {
          tstamp_current = m_NMEA0183.Rfa.TimeStamp;

          if (inext > 1) {
            // countTarget = (tstamp_current - tstamp_last) * 3600 * 24;   First
            // data set, time stamp in julian days
            countTarget =
                (tstamp_current -
                 tstamp_last);  // Second data set, time stamp is in seconds
          }
          countRun = 0;
        }
      }
    } else {
      SetCanvasContextMenuItemViz(m_start_sim_id, true);
      SetCanvasContextMenuItemViz(m_stop_sim_id, false);

      stopSim();
    }
  }
}

void ropeless_pi::ProcessTimerEvent(wxTimerEvent &event) {
  RequestRefresh(GetOCPNCanvasWindow());
}

void ropeless_pi::populateTransponderNode(pugi::xml_node &transponderNode,
                                          transponder_state *state) {
  pugi::xml_node child;

  child = transponderNode.append_child("ID");
  wxString ss;
  ss.Printf("%d", state->markID);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("timeStamp");
  ss.Printf("%f", state->timeStamp);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Lat");
  ss.Printf("%f", state->predicted_lat);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Lon");
  ss.Printf("%f", state->predicted_lon);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Pings");
  ss.Printf("%d", state->pings);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Depth");
  ss.Printf("%f", state->depth);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("RecoveredStatus");
  ss.Printf("%d", state->recovered_state);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("PositionSource");
  ss.Printf("%d", state->position_source);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  // GML (Gear Mark Location) parameters
  child = transponderNode.append_child("MarkType");
  ss.Printf("%d", state->mark_type);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("PosStatus");
  ss.Printf("%d", state->pos_status);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("TrawlID");
  ss.Printf("%d", state->trawl_id);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("TrawlNum");
  ss.Printf("%d", state->trawl_num);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("MfgID");
  ss.Printf("%u", state->mfg_id);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("MfgCode");
  ss.Printf("%u", (unsigned int)state->mfg_code);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("SerialNum");
  ss.Printf("%u", state->serial_num);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("MfgStr");
  child.append_child(pugi::node_pcdata).set_value(state->mfg_str.c_str());
  
  child = transponderNode.append_child("Ownership");
  ss.Printf("%d", state->ownership);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  // GMS (Gear Mark Status) parameters
  child = transponderNode.append_child("SurfaceRange");
  ss.Printf("%d", state->surface_range);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("SlantRange");
  ss.Printf("%d", state->slant_range);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("Tilt");
  ss.Printf("%d", state->tilt);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("SeafloorTemp");
  ss.Printf("%d", state->seafloor_temp);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("AirPressure");
  ss.Printf("%d", state->air_pressure);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("GMSDateNum");
  ss.Printf("%f", state->gms_date_num);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  // Position source
  child = transponderNode.append_child("PositionSource");
  ss.Printf("%d", state->position_source);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  // Lifecycle timestamps
  child = transponderNode.append_child("InitialDeploymentUTC");
  ss.Printf("%f", state->initial_deployment_utc);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("RecoveredUTC");
  ss.Printf("%f", state->recovered_utc);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("ReleasedUTC");
  ss.Printf("%f", state->released_utc);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
}

void ropeless_pi::SaveTransponderStatus() {
  pugi::xml_document transponderStatusDoc;
  pugi::xml_node transpondersNode =
      transponderStatusDoc.append_child("transponders");

  pugi::xml_node childT = transpondersNode.append_child("version");
  childT.append_child(pugi::node_pcdata).set_value("3.0.0.1");
  childT = transpondersNode.append_child("date");
  wxDateTime now = wxDateTime::GetTimeNow();
  wxString timeFormat = now.FormatISOCombined(' ');
  childT.append_child(pugi::node_pcdata).set_value(timeFormat.mb_str());

  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];

    pugi::xml_node transponderNode =
        transpondersNode.append_child("transponder");
    pugi::xml_attribute version = transponderNode.append_attribute("version");
    version.set_value("3");

    populateTransponderNode(transponderNode, state);
  }

  wxString fileName = *GetpPrivateApplicationDataLocation() +
                      wxFileName::GetPathSeparator() +
                      _T("ropeless-transponders.xml");

  transponderStatusDoc.save_file(fileName.mb_str(), "  ");
}

bool ropeless_pi::parseTransponderNode(pugi::xml_node &transponderNode,
                                       transponder_state *state) {
  if (!strcmp(transponderNode.name(), "transponder")) {
    // Check transponder version for backward compatibility
    pugi::xml_attribute version_attr = transponderNode.attribute("version");
    int transponder_version = 2; // default to version 2 if no version specified
    if (version_attr) {
      transponder_version = atoi(version_attr.value());
    }
    for (pugi::xml_node child = transponderNode.first_child(); child;
         child = child.next_sibling()) {
      if (!strcmp(child.name(), "ID")) {
        state->markID = atoi(child.first_child().value());
      }
      if (!strcmp(child.name(), "timeStamp")) {
        wxString val(child.first_child().value());
        double dval;
        val.ToDouble(&dval);
        state->timeStamp = dval;
      }
      if (!strcmp(child.name(), "Lat")) {
        wxString val(child.first_child().value());
        double dval;
        val.ToDouble(&dval);
        state->predicted_lat = dval;
      }
      if (!strcmp(child.name(), "Lon")) {
        wxString val(child.first_child().value());
        double dval;
        val.ToDouble(&dval);
        state->predicted_lon = dval;
      }
      if (!strcmp(child.name(), "Pings")) {
        state->pings = atoi(child.first_child().value());
      }
      if (!strcmp(child.name(), "Depth")) {
        wxString val(child.first_child().value());
        double dval;
        val.ToDouble(&dval);
        state->depth = dval;
      }
      if (!strcmp(child.name(), "RecoveredStatus")) {
        state->recovered_state = atoi(child.first_child().value());
        //wxLogMessage("Parsed Recovered State: %d",state->recovered_state);
      }
      if (!strcmp(child.name(), "PositionSource")) {
        state->position_source = atoi(child.first_child().value());
      }
      
      // GML/GMS parameters (version 3+ only)
      if (transponder_version >= 3) {
        // GML (Gear Mark Location) parameters
        if (!strcmp(child.name(), "MarkType")) {
          state->mark_type = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "PosStatus")) {
          state->pos_status = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "TrawlID")) {
          state->trawl_id = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "TrawlNum")) {
          state->trawl_num = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "MfgID")) {
          state->mfg_id = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "MfgCode")) {
          state->mfg_code = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "SerialNum")) {
          state->serial_num = strtoul(child.first_child().value(), NULL, 10);
        }
        if (!strcmp(child.name(), "MfgStr")) {
          state->mfg_str = wxString(child.first_child().value());
        }
        if (!strcmp(child.name(), "Ownership")) {
          state->ownership = atoi(child.first_child().value());
        }
        // GMS (Gear Mark Status) parameters
        if (!strcmp(child.name(), "SurfaceRange")) {
          state->surface_range = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "SlantRange")) {
          state->slant_range = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "Tilt")) {
          state->tilt = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "SeafloorTemp")) {
          state->seafloor_temp = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "AirPressure")) {
          state->air_pressure = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "GMSDateNum")) {
          wxString val(child.first_child().value());
          double dval;
          val.ToDouble(&dval);
          state->gms_date_num = dval;
        }

        // Lifecycle timestamps
        if (!strcmp(child.name(), "InitialDeploymentUTC")) {
          wxString val(child.first_child().value());
          double dval;
          val.ToDouble(&dval);
          state->initial_deployment_utc = dval;
        }
        if (!strcmp(child.name(), "RecoveredUTC")) {
          wxString val(child.first_child().value());
          double dval;
          val.ToDouble(&dval);
          state->recovered_utc = dval;
        }
        if (!strcmp(child.name(), "ReleasedUTC")) {
          wxString val(child.first_child().value());
          double dval;
          val.ToDouble(&dval);
          state->released_utc = dval;
        }
      }
    }
  }

  return true;
}

void ropeless_pi::LoadTransponderStatus() {
  pugi::xml_document transponderStatusXML;

  wxString fileName = *GetpPrivateApplicationDataLocation() +
                      wxFileName::GetPathSeparator() +
                      _T("ropeless-transponders.xml");

  if (!wxFileExists(fileName)) {
    wxLogMessage("Ropeless: No existing transponder status file found");
    return;
  }

  bool ret = transponderStatusXML.load_file(fileName.mb_str());

  if (ret) {
    // Check root version for compatibility
    pugi::xml_node root = transponderStatusXML.first_child();
    pugi::xml_node version_node = root.child("version");
    wxString xml_version = "unknown";
    if (version_node) {
      xml_version = wxString(version_node.first_child().value());
    }
    
    wxLogMessage("Ropeless: Loading transponder status from XML version %s", xml_version);
    
    transponder_state state;
    pugi::xml_node transponderRoot = transponderStatusXML.first_child();

    if (!parseTransponderNode(transponderRoot, &state)) {
      OCPNMessageBox_PlugIn(
          GetOCPNCanvasWindow(),
          _("Error processing Ropeless Transponder status (XML) file."),
          _("OpenCPN Ropeless Plugin Error"));
      return;
    }

    // Re-generate the trawl list from the transponder info
    RegenerateTrawlListFromTransponders();

  }

  pugi::xml_node statusRoot = transponderStatusXML.first_child();

  for (pugi::xml_node element = statusRoot.first_child(); element;
       element = element.next_sibling()) {
    if (!strcmp(element.name(), "transponder")) {
      transponder_state *this_state = new transponder_state;

      if (parseTransponderNode(element, this_state)) {
        transponderStatus.push_back(this_state);
      } else {
        delete this_state;
      }
    }
  }

  // Validate and fix manufacturer fields for existing transponders
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];
    if (state->serial_num == 0 && state->markID != 0) {
      // Extract manufacturer information from transponder ID if missing
      state->mfg_id = state->markID;
      state->mfg_code = getMfgCode(state->markID);
      state->serial_num = getSerialNumber(state->markID);
      state->mfg_str = getMfgString(state->mfg_code);
      wxLogMessage("Fixed manufacturer fields for transponder %u: mfg_code=%u, serial_num=%u",
                   state->markID, state->mfg_code, state->serial_num);
    }
  }
}

void ropeless_pi::RegenerateTrawlListFromTransponders() {
  // Clear existing trawl list
  for (auto* trawl : trawlList) {
    delete trawl;
  }
  trawlList.clear();

  // Create a map to collect unique trawl IDs from transponders
  std::map<uint16_t, std::vector<transponder_state*>> trawlMap;

  // Group transponders by their trawl_id
  for (auto* transponder : transponderStatus) {
    if (transponder && transponder->trawl_id > 0) {
      trawlMap[transponder->trawl_id].push_back(transponder);
    }
  }

  // Create trawl_tracker objects for each unique trawl ID
  for (const auto& pair : trawlMap) {
    uint16_t trawlId = pair.first;
    const std::vector<transponder_state*>& transponders = pair.second;

    trawl_tracker* newTrawl = new trawl_tracker();
    newTrawl->trawl_id = trawlId;
    newTrawl->devices_in_set = transponders.size();

    // Add transponders to the trawl
    for (auto* transponder : transponders) {
      newTrawl->addTransponder(transponder->markID, transponder->trawl_num);

      // Set start/end marker if flagged
      if (transponder->is_trawl_start_end) {
        newTrawl->setStartEnd(transponder->markID);
      }
    }

    // Reorder transponders based on their positions
    newTrawl->reorderTransponders();

    trawlList.push_back(newTrawl);
  }

  wxLogMessage("Regenerated trawl list: %zu trawls from transponder data", trawlList.size());
}

wxString ropeless_pi::GetColorName(int color_index)
{
  if (color_index >= 0 && color_index < COLOR_TABLE_COUNT)
  {
    // colorblind friendly color map
      if (m_colorblind_mode) {
        return colorTableNamesColorblind[color_index];
      }
      else
      {
        return colorTableNames[color_index];
      }
  }

  return "MAGENTA";
}

void ropeless_pi::RenderTransponder(transponder_state *state) {

  // don't render if the lat lon are invalid pos 999.00000
  if (state->predicted_lat > 90.0 || state->predicted_lon > 180.0)
  {
    //wxLogMessage("Invalid lat/lon for transpoder state: " + state->markID);
    return;
  }

  // Don't render if the pos is hidden
  if (state->hide_pos == true) return;

  // Circle size now set by preferences
  int circle_size = m_transponder_circle_size;

  // Determine color here by ownership
  wxPoint ab;
  wxString colorName;

  if (state->selected){
    colorName = GetColorName(COLOR_INDEX_GOLDEN);
  }
  else{
    if (m_colorblind_mode)
    {
      colorName = colorTableNamesColorblind[state->ownership];
    }
    else {
      colorName = colorTableNames[state->ownership];
    }
  }

  wxColour rcolour = wxTheColourDatabase->Find(colorName);
  int opacity;

  // Set opacity for Recovered transponders
  if (state->recovered_state == eREC_DEPLOYED)
  {
    opacity = 255;
  }
  else if (state->recovered_state == eREC_RECOVERED)
  {
    opacity = 64;
  }

#ifdef SET_RECOVERED_OPACITY
  rcolour.Set(rcolour.Red(), rcolour.Green(), rcolour.Blue(), opacity);
#endif

  if (!rcolour.IsOk()) rcolour = wxColour(255, 000, 255);

  // Render the primary instant transponder using custom bitmap
  GetCanvasPixLL(g_vp, &ab, state->predicted_lat, state->predicted_lon);
  
  // TODO: Draw custom bitmaps for Transponders frome file instead of shapes
  // // Load custom transponder bitmap
  // static wxBitmap customBitmap;
  // if (!customBitmap.IsOk()) {
  //   // Get the plugin data directory path
  //   wxString pluginDir = GetPluginDataDir("ropeless_pi");
  //   wxString bitmapPath = pluginDir + wxFileName::GetPathSeparator() + _T("test_xpdr.png");
    
  //   // Try to load the bitmap
  //   if (wxFileExists(bitmapPath)) {
  //     customBitmap.LoadFile(bitmapPath, wxBITMAP_TYPE_PNG);
  //   }
    
  //   // Fallback if bitmap doesn't load - create a simple colored square
  //   if (!customBitmap.IsOk()) {
  //     customBitmap = wxBitmap(circle_size * 2, circle_size * 2);
  //     wxMemoryDC memDC(customBitmap);
  //     memDC.SetBackground(wxBrush(rcolour));
  //     memDC.Clear();
  //     memDC.SetPen(wxPen(wxColour(0, 0, 0), 2));
  //     memDC.DrawRectangle(2, 2, circle_size * 2 - 4, circle_size * 2 - 4);
  //     memDC.SelectObject(wxNullBitmap);
  //   }
  // }
  
  // Draw the custom bitmap centered at the transponder position
  // if (customBitmap.IsOk()) {
  //   int bmp_x = ab.x - customBitmap.GetWidth() / 2;
  //   int bmp_y = ab.y - customBitmap.GetHeight() / 2;
  //   m_oDC->DrawBitmap(customBitmap, bmp_x, bmp_y, true); // true = use transparency
  // } else {
  //   // Fallback to original circle if bitmap fails
  //   wxPen dpen(rcolour);
  //   wxBrush dbrush(rcolour);
  //   m_oDC->SetPen(dpen);
  //   m_oDC->SetBrush(dbrush);
  //   m_oDC->DrawCircle(ab.x, ab.y, circle_size);
  // }
  
  // Check position source and draw appropriate shape
  if (state->position_source == ePOS_SOURCE_CLOUD) {

    // Draw grey circle for cloud positions
    wxColour greyColour(128, 128, 128, opacity); // Grey color
    wxPen greyPen(greyColour);
    wxBrush greyBrush(greyColour);
    m_oDC->SetPen(greyPen);
    m_oDC->SetBrush(greyBrush);
    m_oDC->DrawCircle(ab.x, ab.y, circle_size);
  } else {

    // Draw regular circle for other position sources
    wxPen dpen(rcolour);
    wxBrush dbrush(rcolour);
    m_oDC->SetPen(dpen);
    m_oDC->SetBrush(dbrush);
    m_oDC->DrawCircle(ab.x, ab.y, circle_size);
  }

  // Draw marking for Trawl End / Midpoint
  if (state->trawl_id > 0)
  {
    // Are we an endpoint?
    if (state->is_trawl_start_end)
    {
      // Draw X marker for start/end transponders
      wxPoint x1(ab.x - circle_size * .707, ab.y - circle_size * .707);
      wxPoint x2(ab.x + circle_size * .707, ab.y + circle_size * .707);
      wxPoint x3(ab.x - circle_size * .707, ab.y + circle_size * .707);
      wxPoint x4(ab.x + circle_size * .707, ab.y - circle_size * .707);

      wxColour pColour = wxColour(0, 0, 0, opacity);
      wxPen xpen(pColour, 3);
      m_oDC->SetPen(xpen);
      m_oDC->DrawLine(x1.x, x1.y, x2.x, x2.y, true);
      m_oDC->DrawLine(x3.x, x3.y, x4.x, x4.y, true);
    }
    else
    {
      // Draw a small black dot in the center to mark it's part of trawl but not the end
      int dot_size = circle_size / 3;  // Small dot, 1/3 the size of the main circle
      wxColour dotColour = wxColour(0, 0, 0, opacity);
      wxPen dotPen(dotColour);
      wxBrush dotBrush(dotColour);
      m_oDC->SetPen(dotPen);
      m_oDC->SetBrush(dotBrush);
      m_oDC->DrawCircle(ab.x, ab.y, dot_size);
    }
  }


  // TODO: Remove me? 
  // // Draw 6 evenly spaced segments around the circle
  // wxPen solidBlackPen(wxColour(0, 0, 0), 3);
  // m_oDC->SetPen(solidBlackPen);
  
  // int radius = circle_size + 4;
  // int dashLength = 30;  // Each dash covers 30 degrees (360/6 = 60, so 30 dash + 30 gap)
  
  // // Draw 6 segments spaced evenly around the circle
  // for (int i = 0; i < 10; i++) {
  //   int startAngle = i * 60;  // Start every 60 degrees (0, 60, 120, 180, 240, 300)
  //   int endAngle = startAngle + dashLength;  // Each dash is 30 degrees long
    
  //   // Calculate start and end coordinates
  //   double startRad = startAngle * M_PI / 180.0;
  //   double endRad = endAngle * M_PI / 180.0;
    
  //   int x1 = ab.x + radius * cos(startRad);
  //   int y1 = ab.y + radius * sin(startRad);
  //   int x2 = ab.x + radius * cos(endRad);
  //   int y2 = ab.y + radius * sin(endRad);
    
  //   // Draw the segment
  //   m_oDC->DrawLine(x1, y1, x2, y2, true);
  // }

}

void ropeless_pi::RenderTrawlConnector(transponder_state *state1,
                                       transponder_state *state2) {

  // Don't render the trawl connector if either of the transponders are hidden (cloud)
  if (state1->hide_pos == true || state2->hide_pos == true) return;
  
  wxPoint P1, P2;
  GetCanvasPixLL(g_vp, &P1, state1->predicted_lat, state1->predicted_lon);
  GetCanvasPixLL(g_vp, &P2, state2->predicted_lat, state2->predicted_lon);

  // TODO: Check if both transponders are recovered. Set trawl connector opacity too

  wxColour rcolour = wxTheColourDatabase->Find(wxString("BLACK"));
  wxPen dpen(rcolour, 3);
  wxBrush dbrush(rcolour);

  m_oDC->DrawLine(P1.x, P1.y, P2.x, P2.y, true);
}

void ropeless_pi::RenderTransponderTexts() {
  int circle_size = m_transponder_circle_size; // Same as in RenderTransponder
  
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];
    
    // Skip rendering text for hidden, cloud positions, or invalid transponders
    if (state->hide_pos == true || state->position_source == ePOS_SOURCE_CLOUD || state->markID <= 0 || m_hide_transponder_text) {
      continue;
    }
    
    // Skip if coordinates are invalid
    if (state->predicted_lat > 90.0 || state->predicted_lon > 180.0) {
      continue;
    }
    
    wxPoint ab;
    GetCanvasPixLL(g_vp, &ab, state->predicted_lat, state->predicted_lon);
    
    wxString transponderText = wxString::Format(wxT("%d"), state->markID);
    
    // Create a memory bitmap to render text
    wxFont textFont(m_transponder_text_size, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    
    // Create a memory DC to measure text size
    wxMemoryDC memDC;
    wxBitmap tempBmp(100, 50); // Temporary bitmap for measurement
    memDC.SelectObject(tempBmp);
    memDC.SetFont(textFont);
    
    // Measure the text
    wxCoord textW, textH;
    memDC.GetTextExtent(transponderText, &textW, &textH);
    
    // Create the actual bitmap with proper size and padding
    int bmpW = textW + 8; // Add padding
    int bmpH = textH + 8;
    wxBitmap textBmp(bmpW, bmpH);
    
    // Render text to the bitmap
    memDC.SelectObject(textBmp);
    memDC.SetBackground(wxBrush(wxColour(255, 255, 255))); // White background
    memDC.Clear();
    memDC.SetFont(textFont);
    memDC.SetTextForeground(wxColour(0, 0, 0)); // Black text
    
    // Draw text centered in the bitmap
    memDC.DrawText(transponderText, 4, 4); // 4px padding
    
    // Draw black border around the bitmap
    wxPen borderPen(wxColour(0, 0, 0), 2);
    memDC.SetPen(borderPen);
    memDC.SetBrush(*wxTRANSPARENT_BRUSH);
    memDC.DrawRectangle(0, 0, bmpW, bmpH);
    
    memDC.SelectObject(wxNullBitmap); // Deselect bitmap
    
    // Position the bitmap above the circle
    int bmp_x = ab.x - bmpW/2;
    int bmp_y = ab.y - circle_size - bmpH - 8;
    
    // Draw the bitmap using ODDC's DrawBitmap function
    m_oDC->DrawBitmap(textBmp, bmp_x, bmp_y, false);
  }
}

void ropeless_pi::RenderVesselRangeCircle() {

  // Only draw if we have a valid vessel position
  if (m_ownship_lat == 0.0 && m_ownship_lon == 0.0) {
    return;
  }
  
  // Get vessel position in screen coordinates
  wxPoint vesselPos;
  GetCanvasPixLL(g_vp, &vesselPos, m_ownship_lat, m_ownship_lon);
  
  // Calculate 1 nautical miles in screen pixels
  // 1 nautical mile = 1852 meters
  double range_nm = 1.0;
  double range_meters = range_nm * 1852.0;
  
  // Calculate a point 10nm to the east of vessel
  double east_lat = m_ownship_lat;
  double east_lon = m_ownship_lon + (range_meters / (111320.0 * cos(m_ownship_lat * M_PI / 180.0)));
  
  wxPoint eastPos;
  GetCanvasPixLL(g_vp, &eastPos, east_lat, east_lon);
  
  // Calculate radius in pixels
  int radius_pixels = abs(eastPos.x - vesselPos.x);
  
  // Don't draw if circle would be too small or too large
  if (radius_pixels < 5 || radius_pixels > 2000) {
    return;
  }
  
  // Set up grey circle with no fill, just outline
  wxColour greyColour(128, 128, 128, 180); // Semi-transparent grey
  wxPen greyPen(greyColour, 2); // 2 pixel wide line
  wxBrush transparentBrush(wxColour(0, 0, 0), wxBRUSHSTYLE_TRANSPARENT);
  
  m_oDC->SetPen(greyPen);
  m_oDC->SetBrush(transparentBrush);
  
  // Draw the circle
  m_oDC->DrawCircle(vesselPos.x, vesselPos.y, radius_pixels);
  
  // Optional: Add text label
  wxFont labelFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
  m_oDC->SetFont(labelFont);
  m_oDC->SetTextForeground(greyColour);
  
  // Position text at top of circle
  wxString rangeText = wxString::Format(_("10nm"));
  wxCoord textW, textH;
  m_oDC->GetTextExtent(rangeText, &textW, &textH);
  
  int text_x = vesselPos.x - textW/2;
  int text_y = vesselPos.y - radius_pixels - textH - 5;
  
  m_oDC->DrawText(rangeText, text_x, text_y);
}

transponder_state *ropeless_pi::GetStateByMarkID(uint32_t markID) {
  transponder_state *rv = NULL;

  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];

    if (state->markID == markID) return state;
  }

  return rv;
}

bool ropeless_pi::DeleteTransponder(uint32_t markID)
{
  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->markID == markID) {

      SendCommandMessage(GetStateByMarkID(markID),eCMD_DELETE);

      transponderStatus.erase(transponderStatus.begin() + i);

      return true;
    }
  }

  return false;
}

bool ropeless_pi::ConfirmAndDeleteTransponder(uint32_t markID)
{
  wxString msg("Delete Transponder: ");
  wxString msg1;
  msg1.Printf("%d\n", markID);
  msg += msg1;

  myOkDialog dialog2;

#ifdef __ANDROID__
  wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
  dialog2.SetFont(*pFont);
#endif

  dialog2.Create(GetOCPNCanvasWindow(), msg, "Ropeless Plugin Message", 0,
                 0, 100000, wxDefaultPosition);

  wxLogMessage("Deleting Transponder!");

  if (dialog2.ShowModal() == wxID_OK) {
    if (DeleteTransponder(markID)) {
      SendCommandMessage(GetStateByMarkID(markID), eCMD_RELEASE);
      return true;
    }
  }
  
  return false;
}

bool ropeless_pi::ConfirmAndReleaseTransponder(transponder_state* state)
{
  if (!state) return false;
  
  wxString msg("Send Release to Transponder: ");
  wxString msg1;
  msg1.Printf("%d\n", state->markID);
  msg += msg1;

  long result = -1;
  myNumberEntryDialog dialog;
  myOkDialog okDialog;

  if (state->markID > 0) {

#ifdef __ANDROID__
    wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
    okDialog.SetFont(*pFont);
#endif

    okDialog.Create(GetOCPNCanvasWindow(), msg, "Ropeless Plugin Message",
                    0, 0, 100000, wxDefaultPosition);

    if (okDialog.ShowModal() == wxID_OK) {result = 1;}
   
  } 
  else {

#ifdef __ANDROID__
    wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
    dialog.SetFont(*pFont);
#endif

    dialog.Create(GetOCPNCanvasWindow(), msg, "Enter Release Code",
                  "Ropeless Plugin Message", 0, 0, 100000,
                  wxDefaultPosition);

    if (dialog.ShowModal() == wxID_OK) result = dialog.GetValue();

    result = -1;
  }

  if (result >= 0) {
    SendCommandMessage(state, eCMD_RELEASE);
    return true;
  }
  
  return false;
}

wxString getMfgString(uint8_t mfg_code) {
  // Use centralized manufacturer lookup table
  for (int i = 0; i < manufacturerTableSize; i++) {
    if (manufacturerTable[i].code == mfg_code) {
      return wxString(manufacturerTable[i].name);
    }
  }
  return wxString::Format("Unknown Mfg 0x%02X", mfg_code);
}

void ropeless_pi::RenderTransponders() {
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];
    RenderTransponder(state);
  }
}

void ropeless_pi::RenderTrawlConnectors() {

  // Iterate through each trawl
  for (auto* trawl : trawlList) {

    if (trawl && trawl->traps_in_set() > 1) {

      // Get list of transponders in trawl order
      auto transponders = trawl->getOrderedTransponders();

      // Draw connections between consecutive transponders in trawl
      for (size_t i = 0; i < transponders.size() - 1; i++) {

        // make sure they exist -- then render
        if (transponders[i] && transponders[i+1]) {
          RenderTrawlConnector(transponders[i], transponders[i+1]);
        }
      }
    }
  }
}

bool ropeless_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp) {
  ODDC oddc(dc);
  if (!m_oDC) m_oDC = new ODDC();

  // Use the local DC-based ODDC for rendering
  ODDC *old_oDC = m_oDC;
  m_oDC = &oddc;
  m_oDC->SetVP(vp);

  g_vp = vp;
  Clone_VP(&g_ovp, vp);  // deep copy

  // TODO: Don't render transponders outside a certain render distance?
  m_selectRadius = (10 / vp->view_scale_ppm) / (1852. * 60.);

  // Render vessel range circle first (behind transponders)
  RenderVesselRangeCircle();

  // Render trawl connectors (behind transponders)
  RenderTrawlConnectors();

  // Render individual transponders (on top of connectors)
  RenderTransponders();

  // Render transponder texts on top of everything else
  RenderTransponderTexts();

  // Restore original m_oDC
  m_oDC = old_oDC;

  return true;
}

bool ropeless_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp) {
  if (!m_oDC) m_oDC = new ODDC();

  m_oDC->SetVP(vp);

  //     g_gdc = NULL;
  //     g_pdc = NULL;

  g_vp = vp;
  Clone_VP(&g_ovp, vp);  // deep copy

  // TODO: Don't render transponders outside a certain render distance?
  m_selectRadius = (10 / vp->view_scale_ppm) / (1852. * 60.);

  // m_select->SetSelectLLRadius(selec_radius);

  // Render vessel range circle first (behind transponders)
  RenderVesselRangeCircle();

  // Render trawl connectors (behind transponders)
  RenderTrawlConnectors();

  // Render individual transponders (on top of connectors)
  RenderTransponders();

  // Render transponder texts on top of everything else
  RenderTransponderTexts();

  return true;
}

void ropeless_pi::ProcessRFACapture(void) {
  if (m_NMEA0183.LastSentenceIDReceived != _T("RFA")) {
    return;
  }

  int markID = m_NMEA0183.Rfa.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->markID == markID) {
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // If not present, create a new record, and add to vector
  if (this_transponder_state == NULL) {
    this_transponder_state = new transponder_state;

    // Extract manufacturer information from transponder ID
    this_transponder_state->mfg_id = markID;
    this_transponder_state->mfg_code = getMfgCode(markID);
    this_transponder_state->serial_num = getSerialNumber(markID);
    this_transponder_state->mfg_str = getMfgString(this_transponder_state->mfg_code);

    // Set initial ownership to owned (1) for new transponders
    this_transponder_state->ownership = 1;

    transponderStatus.push_back(this_transponder_state);
  } else {

    this_transponder_state->pings++;

  }

  //  Update the instant record
  this_transponder_state->markID = markID;

  this_transponder_state->timeStamp = m_NMEA0183.Rfa.TimeStamp;

  this_transponder_state->predicted_lat =
      m_NMEA0183.Rfa.TransponderPosition.Latitude.Latitude;
  if (m_NMEA0183.Rfa.TransponderPosition.Latitude.Northing == South)
    this_transponder_state->predicted_lat =
        -this_transponder_state->predicted_lat;

  this_transponder_state->predicted_lon =
      m_NMEA0183.Rfa.TransponderPosition.Longitude.Longitude;
  if (m_NMEA0183.Rfa.TransponderPosition.Longitude.Easting == West)
    this_transponder_state->predicted_lon =
        -this_transponder_state->predicted_lon;
  
  this_transponder_state->position_source = ePOS_SOURCE_ACOUSTIC;

  this_transponder_state->slant_range = m_NMEA0183.Rfa.TransponderRange;
  this_transponder_state->bearing = m_NMEA0183.Rfa.TransponderBearing;
  this_transponder_state->depth = m_NMEA0183.Rfa.TransponderDepth;
  this_transponder_state->seafloor_temp = m_NMEA0183.Rfa.TransponderTemp;
  this_transponder_state->batt_stat = m_NMEA0183.Rfa.TransponderBattStat;

  //  Capture ownship position/COG
  double ownship_lat = m_NMEA0183.Rfa.OwnshipPosition.Latitude.Latitude;
  double ownship_lon = m_NMEA0183.Rfa.OwnshipPosition.Longitude.Longitude;
  double ownship_cog = m_NMEA0183.Rfa.OwnshipHeading;

  if (m_pRLDialog) {
    m_pRLDialog->RefreshTransponderList();
  }

  // Synthesize a RMC message, and send it upstream
  m_NMEA0183.Rmc.IsDataValid = NTrue;
  m_NMEA0183.Rmc.SpeedOverGroundKnots = 1.0;
  m_NMEA0183.Rmc.Position.Latitude.Set(ownship_lat);
  m_NMEA0183.Rmc.Position.Longitude.Set(-ownship_lon);
  m_NMEA0183.Rmc.TrackMadeGoodDegreesTrue = ownship_cog;
  m_NMEA0183.Rmc.Date.Empty();
  m_NMEA0183.Rmc.MagneticVariation = 0.0;
  m_NMEA0183.Rmc.MagneticVariationDirection = EW_Unknown;

  SENTENCE rmc_sentence;
  m_NMEA0183.Rmc.Write(rmc_sentence);

  // TODO: Remove this in favor of separate GPS pos source?
  //PushNMEABuffer(rmc_sentence.Sentence);
  
  // Display sent NMEA message in debug window
  if (m_pRLDialog) {
    m_pRLDialog->AddDebugMessage("--> " + rmc_sentence.Sentence.Trim());
  }
}

void ropeless_pi::placeTransponderManually(uint32_t markID, uint32_t pairId, double lat,
                                           double lon, double utc, int pos_source, int ownership) {
  
  wxLogMessage("Placing Transponder Manually - ID: %d, Position Source: %s (%d)", 
               markID, positionSourceNames[pos_source], pos_source);

  transponder_state *this_transponder_state;
  this_transponder_state = addTransponderPos(markID);

  if (!this_transponder_state) {
    wxLogMessage("ERROR: Failed to create transponder state for ID: %d", markID);
    return;
  }

  this_transponder_state->markID = markID;

  this_transponder_state->timeStamp = utc;

  this_transponder_state->predicted_lat = lat;
  this_transponder_state->predicted_lon = lon;
  this_transponder_state->position_source = pos_source;
  this_transponder_state->ownership = ownership;
  
  wxLogMessage("Transponder created: ID=%d, Lat=%.6f, Lon=%.6f, PosSource=%d (%s)", 
               this_transponder_state->markID, 
               this_transponder_state->predicted_lat, 
               this_transponder_state->predicted_lon,
               this_transponder_state->position_source,
               positionSourceNames[this_transponder_state->position_source]);

  // Generate and send RSGML message for manual placement
  SendGMLMessageForManualPlacement(this_transponder_state, lat, lon, utc);

}

void ropeless_pi::EditTransponder(transponder_state* state) {
  if (!state) {
    wxLogMessage("ERROR: EditTransponder called with null state");
    return;
  }

  // Get current position for dialog
  wxString latStr = wxString::Format("%.6f", state->predicted_lat);
  wxString lonStr = wxString::Format("%.6f", state->predicted_lon);

  // Format timestamp for display
  wxDateTime timestamp = wxDateTime((time_t)state->timeStamp);
  timestamp.MakeFromUTC();
  wxString utcStr = timestamp.FormatISOTime();

  wxLogMessage("Opening edit dialog for transponder ID: %u", state->markID);

  // Create edit dialog with existing transponder data
  manualPlacementDlgImpl editDlg(GetOCPNCanvasWindow(), wxID_ANY,
                                _("Edit Transponder"), wxDefaultPosition,
                                wxSize(-1, -1), wxDEFAULT_DIALOG_STYLE,
                                latStr, lonStr, utcStr, state);

  int res = editDlg.ShowModal();

  if (res == wxID_OK && editDlg.valid) {
    // Update the existing transponder with new values
    uint32_t newMarkID = editDlg.markID;

    wxLogMessage("Updating transponder: old ID=%u, new ID=%u", state->markID, newMarkID);

    // Update the transponder fields
    state->markID = newMarkID;

    // Extract manufacturer and serial number for logging
    uint8_t mfg_code = getMfgCode(newMarkID);
    uint32_t serial_num = getSerialNumber(newMarkID);
    state->mfg_code = mfg_code;
    state->serial_num = serial_num;
    state->mfg_str = getMfgString(mfg_code);

    // Update position source
    state->position_source = editDlg.positionSource;

    // Update ownership
    state->ownership = editDlg.isOwned ? 1 : 0;

    // Update trawl assignment
    state->trawl_id = editDlg.selectedTrawlId;
    if (editDlg.selectedTrawlId == 0) {
      state->is_trawl_start_end = false;  // Clear start/end flag when removing from trawl
      state->trawl_num = -1;  // Reset position when removing from trawl
    } else {
      state->is_trawl_start_end = false;  // Reset start/end flag when assigning to new trawl
    }

    wxLogMessage("Transponder updated: ID=%u (mfg=0x%02X, serial=%u), source=%s, trawl=%d",
                 newMarkID, mfg_code, serial_num,
                 positionSourceNames[state->position_source], state->trawl_id);

    // Save changes and refresh UI
    SaveTransponderStatus();
    if (m_pRLDialog) {
      m_pRLDialog->RefreshTransponderList();
    }
  }
}

int ropeless_pi::getNextCloudId() {
  return m_nextCloudId++;
}

// Check for transponder state in list. if does not exist add new one
transponder_state *ropeless_pi::addTransponderPos(uint32_t markID) {

  wxLogMessage("Creating new transponder id: " + markID);

  transponder_state *this_transponder_state = NULL;

  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->markID == markID) {
      this_transponder_state = transponderStatus[i];

      wxLogMessage("Already exists!");
      return this_transponder_state;
    }
  }

  // If not present, create a new record, and add to vector
  if (this_transponder_state == NULL) {

    this_transponder_state = new transponder_state;

    this_transponder_state->markID = markID;

    // Extract manufacturer information from transponder ID
    this_transponder_state->mfg_id = markID;
    this_transponder_state->mfg_code = getMfgCode(markID);
    this_transponder_state->serial_num = getSerialNumber(markID);
    this_transponder_state->mfg_str = getMfgString(this_transponder_state->mfg_code);

    this_transponder_state->ownership = 1;

    // Push to vector
    transponderStatus.push_back(this_transponder_state);

  } 

  return this_transponder_state;
}

void ropeless_pi::ProcessRLACapture(void) {
  if (m_NMEA0183.LastSentenceIDReceived != _T("RLA")) {
    return;
  }

  int markID = m_NMEA0183.Rla.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->markID == markID) {
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // Check if ident matches released transponder
  if (manualReleaseState.markID == markID)
  {
    this_transponder_state = &manualReleaseState;
  }
  
  // If specified transponder is not present, ignore the message
  if (this_transponder_state == NULL) {
    return;
  }

  // Update the record
  this_transponder_state->release_status = m_NMEA0183.Rla.TransponderStatus;

  if (m_pRLDialog) {
    m_pRLDialog->RefreshTransponderList();
  }

  updateReleaseTimer(this_transponder_state);

}

void ropeless_pi::SetNMEASentence(wxString &sentence) {
  if (sentence.IsEmpty()) {
    return;
  }

  // Debug: Log the source of this NMEA sentence
  printf("RECEIVED NMEA: %s\n", sentence.ToStdString().c_str());
  wxLogMessage("RECEIVED NMEA from OpenCPN: %s", sentence);
  
  m_NMEA0183 << sentence;

  wxLogMessage(sentence);
  
  // Display NMEA message in debug window if dialog is open
  if (m_pRLDialog) {
    m_pRLDialog->AddDebugMessage("<-- " + sentence.Trim());
  }

  if (m_NMEA0183.PreParse()) {

    if (m_NMEA0183.LastSentenceIDReceived == _T("RFA")) {
      if (m_NMEA0183.Parse()) {
        ProcessRFACapture();
      }
    }

    if (m_NMEA0183.LastSentenceIDReceived == _T("RLA")) {
      if (m_NMEA0183.Parse()) ProcessRLACapture();
    }

    if (m_NMEA0183.LastSentenceIDReceived == _T("DBS")) {
      if (m_NMEA0183.Parse()) {
        wxLogMessage("DBS Message Received: DeckboxID=%s, Manuf=%s, AcousticStatus=%s, CloudStatus=%s, NumDevices=%d",
                     m_NMEA0183.Dbs.DeckboxID,
                     m_NMEA0183.Dbs.DeckboxManuf,
                     m_NMEA0183.Dbs.AcousticStatus,
                     m_NMEA0183.Dbs.CloudStatus,
                     m_NMEA0183.Dbs.NumDevices);
        
        // Update global deckbox status
        g_deckboxStatus.UpdateFromDBS(m_NMEA0183.Dbs);
        
        // Update UI if dialog is open
        if (m_pRLDialog) {
          m_pRLDialog->UpdateDeviceIdStatus(m_NMEA0183.Dbs.DeckboxID);
          m_pRLDialog->UpdateAcousticStatus(m_NMEA0183.Dbs.AcousticStatus);
          m_pRLDialog->UpdateCloudStatus(m_NMEA0183.Dbs.CloudStatus);
        }
        
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          // Create a completely fresh DBS message to avoid any shared state
          DBS fresh_dbs;
          fresh_dbs.DeckboxID = m_NMEA0183.Dbs.DeckboxID;
          fresh_dbs.DeckboxManuf = m_NMEA0183.Dbs.DeckboxManuf;
          fresh_dbs.AcousticStatus = m_NMEA0183.Dbs.AcousticStatus;
          fresh_dbs.CloudStatus = m_NMEA0183.Dbs.CloudStatus;
          fresh_dbs.NumDevices = m_NMEA0183.Dbs.NumDevices;
          
          wxLogMessage("SENDING DBS via TCP: DeckboxID=%s", fresh_dbs.DeckboxID);
          //SendNMEAMessage(&fresh_dbs);
        }
      }
    }

    if (m_NMEA0183.LastSentenceIDReceived == _T("GML")) {
      if (m_NMEA0183.Parse()) {
        wxLogMessage("GML Message Received: MarkID=%d, MarkType=%d, PosStatus=%d, TrawlID=%d, TrawlNum=%d, Lat=%.6f, Lon=%.6f, Depth=%d, Ownership=%d, Source=%d, DateNum=%.6f",
                     m_NMEA0183.Gml.MarkID,
                     m_NMEA0183.Gml.MarkType,
                     m_NMEA0183.Gml.PosStatus,
                     m_NMEA0183.Gml.TrawlID,
                     m_NMEA0183.Gml.TrawlNum,
                     m_NMEA0183.Gml.Latitude,
                     m_NMEA0183.Gml.Longitude,
                     m_NMEA0183.Gml.Depth,
                     m_NMEA0183.Gml.Ownership,
                     m_NMEA0183.Gml.Source,
                     m_NMEA0183.Gml.DateNum);
        
        // Update transponder state with GML data
        transponder_state *tstate = GetStateByMarkID(m_NMEA0183.Gml.MarkID);

        if (!tstate) {
            wxLogMessage("Couldn't find Transponder Ident. Creating new...");
            tstate = addTransponderPos(m_NMEA0183.Gml.MarkID);
        }
        
        if (tstate) {
            // Update GML status parameters
            tstate->mark_type = m_NMEA0183.Gml.MarkType;
            tstate->pos_status = m_NMEA0183.Gml.PosStatus;
            tstate->trawl_id = m_NMEA0183.Gml.TrawlID;
            tstate->trawl_num = m_NMEA0183.Gml.TrawlNum;

            // Split MarkID into manufacturer components
            tstate->mfg_id = static_cast<uint32_t>(m_NMEA0183.Gml.MarkID);  // Use MarkID directly
            tstate->mfg_code = getMfgCode(tstate->mfg_id);
            tstate->serial_num = getSerialNumber(tstate->mfg_id);
            tstate->mfg_str = getMfgString(tstate->mfg_code);

            tstate->ownership = m_NMEA0183.Gml.Ownership;
            tstate->position_source = m_NMEA0183.Gml.Source;
            tstate->timeStamp = m_NMEA0183.Gml.DateNum;
            
            // Update position if available
            if (m_NMEA0183.Gml.Latitude != 0.0 && m_NMEA0183.Gml.Longitude != 0.0) {
                tstate->predicted_lat = m_NMEA0183.Gml.Latitude;
                tstate->predicted_lon = m_NMEA0183.Gml.Longitude;
                tstate->position_source = (m_NMEA0183.Gml.Source == 3) ? ePOS_SOURCE_GPS : ePOS_SOURCE_ACOUSTIC;
            }
            
            // Update depth if available  
            if (m_NMEA0183.Gml.Depth > 0) {
                tstate->depth = m_NMEA0183.Gml.Depth;
            }
            
            tstate->timeStamp = wxDateTime::Now().GetTicks();

            tstate->pings++;

        }
        else
        {
          wxLogMessage("Failed to update transponder from GML message!");
        }
                     
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          //SendNMEAMessage(&m_NMEA0183.Gml);
        }
      }
    }
    if (m_NMEA0183.LastSentenceIDReceived == _T("GMS")) {
      if (m_NMEA0183.Parse()) {
        wxLogMessage("GMS Message Received: MarkID=%d, ReleaseStatus=%d, Battery=%d, SurfaceRange=%d, SlantRange=%d, Bearing=%d, Tilt=%d, SeafloorTemp=%d, AirPressure=%d, DateNum=%.6f",
                     m_NMEA0183.Gms.MarkID,
                     m_NMEA0183.Gms.ReleaseStatus,
                     m_NMEA0183.Gms.Battery,
                     m_NMEA0183.Gms.SurfaceRange,
                     m_NMEA0183.Gms.SlantRange,
                     m_NMEA0183.Gms.Bearing,
                     m_NMEA0183.Gms.Tilt,
                     m_NMEA0183.Gms.SeafloorTemp,
                     m_NMEA0183.Gms.AirPressure,
                     m_NMEA0183.Gms.DateNum);
        
        // Update transponder state with GMS data  
        transponder_state *tstate = GetStateByMarkID(m_NMEA0183.Gms.MarkID);
        if (!tstate) {
            wxLogMessage("Couldn't find Transponder Ident. Creating new...");
            tstate = addTransponderPos(m_NMEA0183.Gms.MarkID);
        }
        
        if (tstate) {
            // Update existing fields that map to GMS data
            tstate->release_status = m_NMEA0183.Gms.ReleaseStatus;
            tstate->batt_stat = m_NMEA0183.Gms.Battery;
            tstate->bearing = m_NMEA0183.Gms.Bearing;
            
            // Update new GMS status parameters
            tstate->surface_range = m_NMEA0183.Gms.SurfaceRange;
            tstate->slant_range = m_NMEA0183.Gms.SlantRange;
            tstate->tilt = m_NMEA0183.Gms.Tilt;
            tstate->seafloor_temp = m_NMEA0183.Gms.SeafloorTemp;
            tstate->air_pressure = m_NMEA0183.Gms.AirPressure;
            tstate->gms_date_num = m_NMEA0183.Gms.DateNum;
            
            tstate->timeStamp = wxDateTime::Now().GetTicks();
        }
                     
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          //SendNMEAMessage(&m_NMEA0183.Gms);
        }
      }
    }
    if (m_NMEA0183.LastSentenceIDReceived == _T("GMR")) {
      if (m_NMEA0183.Parse()) {
        wxLogMessage("GMR Message Received: CmdUID=%d, SourceID=%d, TargetID=%d, MarkID=%d, CmdType=%d, ResCode=%d, Param1=%d, Param2=%d",
                     m_NMEA0183.Gmr.CmdUID,
                     m_NMEA0183.Gmr.SourceID,
                     m_NMEA0183.Gmr.TargetID,
                     m_NMEA0183.Gmr.MarkID,
                     m_NMEA0183.Gmr.CmdType,
                     m_NMEA0183.Gmr.ResCode,
                     m_NMEA0183.Gmr.Param1,
                     m_NMEA0183.Gmr.Param2);
                     
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          //SendNMEAMessage(&m_NMEA0183.Gmr);
        }
      }
    }
  }
}

void ropeless_pi::SetPositionFix(PlugIn_Position_Fix &pfix) {
  if (1) {
    m_ownship_lat = pfix.Lat;
    m_ownship_lon = pfix.Lon;
    if (!wxIsNaN(pfix.Cog)) m_ownship_cog = pfix.Cog;
    if (!wxIsNaN(pfix.Var)) mVar = pfix.Var;
  }
}

void ropeless_pi::SetCursorLatLon(double lat, double lon) {}

void ropeless_pi::SetPluginMessage(wxString &message_id,
                                   wxString &message_body) {}

int ropeless_pi::GetToolbarToolCount(void) { return 1; }

void ropeless_pi::SetColorScheme(PI_ColorScheme cs) {}

bool ropeless_pi::LoadConfig(void) {

  wxLogMessage("Loading config...\n");
  
  wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

  if (pConf) {
    wxLogMessage("Reading config file from OpenCPN configuration");

    pConf->SetPath(_T( "/Settings/Ropeless_pi" ));
    pConf->Read("dialogSizeWidth", &m_dialogSizeWidth, -1);
    pConf->Read("dialogSizeHeight", &m_dialogSizeHeight, -1);
    pConf->Read("dialogPosX", &m_dialogPosX, -1);
    pConf->Read("dialogPosY", &m_dialogPosY, -1);

    pConf->Read(_T( "SerialPort" ), &m_serialPort);
    pConf->Read(_T( "TrackedPoint" ), &m_trackedWPGUID);

    // TCP NMEA Output Configuration
    pConf->Read(_T( "TCP_Host" ), &m_tcp_host, _T("127.0.0.1"));
    pConf->Read(_T( "TCP_Port" ), &m_tcp_port, 4001);
    
    // Read boolean values more explicitly
    bool tcp_enabled_default = true;
    bool tcp_auto_reconnect_default = true;
    pConf->Read(_T( "TCP_Enabled" ), &m_tcp_enabled, tcp_enabled_default);
    pConf->Read(_T( "TCP_AutoReconnect" ), &m_tcp_auto_reconnect, tcp_auto_reconnect_default);
    
    // Display Configuration
    pConf->Read(_T( "Colorblind_Mode" ), &m_colorblind_mode, false);
    pConf->Read(_T( "Hide_Transponder_Text" ), &m_hide_transponder_text, false);
    pConf->Read(_T( "Transponder_Circle_Size" ), &m_transponder_circle_size, 10);
    pConf->Read(_T( "Transponder_Text_Size" ), &m_transponder_text_size, 12);
    
    // Debug Configuration
    pConf->Read(_T( "Debug_Enabled" ), &m_debug_enabled, false);
    pConf->Read(_T( "Debug_ShowNMEA" ), &m_debug_show_nmea, false);
    pConf->Read(_T( "Debug_ShowLog" ), &m_debug_show_log, false);
    
    // Debug logging to see what was loaded
    wxLogMessage("TCP Config loaded - Host: %s, Port: %d, Enabled: %s, AutoReconnect: %s", 
                 m_tcp_host, m_tcp_port, 
                 m_tcp_enabled ? "true" : "false", 
                 m_tcp_auto_reconnect ? "true" : "false");
    
    // Force TCP to be enabled for initial testing (remove this later)
    if (!m_tcp_enabled) {
        wxLogMessage("TCP was disabled in config - forcing it enabled for testing");
        m_tcp_enabled = true;
    }

    // Communication mode (UDP only)
  
    m_trackedWP = getWaypointName(m_trackedWPGUID);
    
    ApplyConfig();

    return true;
  } else
  {
    wxLogMessage("Failed to load config");
    return false;
  }
}

bool ropeless_pi::SaveConfig(void) {
  wxFileConfig *pConf = (wxFileConfig *)m_pconfig;

  if (pConf) {
    pConf->SetPath(_T( "/Settings/Ropeless_pi" ));

    pConf->Write("dialogSizeWidth", m_dialogSizeWidth);
    pConf->Write("dialogSizeHeight", m_dialogSizeHeight);
    pConf->Write("dialogPosX", m_dialogPosX);
    pConf->Write("dialogPosY", m_dialogPosY);

    // TCP NMEA Output Configuration
    pConf->Write(_T( "TCP_Host" ), m_tcp_host);
    pConf->Write(_T( "TCP_Port" ), m_tcp_port);
    pConf->Write(_T( "TCP_Enabled" ), m_tcp_enabled);
    pConf->Write(_T( "TCP_AutoReconnect" ), m_tcp_auto_reconnect);
    
    // Display Configuration
    pConf->Write(_T( "Colorblind_Mode" ), m_colorblind_mode);
    pConf->Write(_T( "Hide_Transponder_Text" ), m_hide_transponder_text);
    pConf->Write(_T( "Transponder_Circle_Size" ), m_transponder_circle_size);
    pConf->Write(_T( "Transponder_Text_Size" ), m_transponder_text_size);
    
    // Debug Configuration
    pConf->Write(_T( "Debug_Enabled" ), m_debug_enabled);
    pConf->Write(_T( "Debug_ShowNMEA" ), m_debug_show_nmea);
    pConf->Write(_T( "Debug_ShowLog" ), m_debug_show_log);

    // Communication mode (UDP only) - no need to save

    //         pConf->Write ( _T( "SerialPort" ),  m_serialPort );
    //         pConf->Write ( _T( "TrackedPoint" ),  m_trackedWPGUID );
    //
    //         pConf->Write ( _T( "TenderLength" ),  m_tenderLength );
    //         pConf->Write ( _T( "TenderWidth" ),  m_tenderWidth );
    //         pConf->Write ( _T( "TenderGPSOffsetX" ),  m_tenderGPS_x );
    //         pConf->Write ( _T( "TenderGPSOffsetY" ),  m_tenderGPS_y );
    //         pConf->Write ( _T( "TenderIconType" ),  m_tenderIconType );

    return true;
  } else
    return false;
}

void ropeless_pi::ApplyConfig(void) {
  // UDP mode requires no special initialization - handled in SendCommandMessage
}

bool ropeless_pi::MouseEventHook(wxMouseEvent &event) {
  bool bret = false;

  m_mouse_x = event.m_x;
  m_mouse_y = event.m_y;

  //     //  Retrigger the rollover timer
  //     if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() )
  //         m_RolloverPopupTimer.Start( 10, wxTIMER_ONE_SHOT ); // faster
  //         response while the rollover is turned on
  //     else
  //         m_RolloverPopupTimer.Start( m_rollover_popup_timer_msec,
  //         wxTIMER_ONE_SHOT );

  wxPoint mp(event.m_x, event.m_y);
  GetCanvasLLPix(&g_ovp, mp, &m_cursor_lat, &m_cursor_lon);

  //wxLogMessage("Mouse Event!");

  int transponderFoundID = 0;
  m_foundState = NULL;

  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];

    double a = fabs(m_cursor_lat - state->predicted_lat);
    double b = fabs(m_cursor_lon - state->predicted_lon);

    if ((a < m_selectRadius) && (b < m_selectRadius)) {
      m_foundState = state;
      transponderFoundID = transponderStatus[i]->markID;
      //wxLogMessage("Found Transponder! %d", m_foundState->markID);
      break;
    }
  }

  // //  On right button push, find any transponders
  // if( event.RightDown()) {

  //   m_foundState = NULL;

  //   wxLogMessage("Looking for Transponder on right click...");

  //   for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
  //     transponder_state *state = transponderStatus[i];

  //     double a = fabs( m_cursor_lat - state->predicted_lat );
  //     double b = fabs( m_cursor_lon - state->predicted_lon );

  //     if( ( a < m_selectRadius ) && ( b < m_selectRadius ) ){
  //       m_foundState = state;
  //       transponderFoundID = transponderStatus[i]->markID;
  //       //wxLogMessage("Found Transponder! %d",m_foundState->markID);
  //       break;
  //     }
  //   }
  // }

#if 0
    if (m_foundState && !popupVis)
    {
        wxLogMessage("Hovering over Transponder. Showing Message!");

        wxPoint clientPoint(m_mouse_x, m_mouse_y); // your client coordinates
        wxPoint screenPoint = m_parent_window->ClientToScreen(clientPoint); // Converts to screen coordinates

        popup->Position(screenPoint, wxSize(0, 0));
        popup->Show();

        popupVis = true;
    }
    else if (!m_foundState && popupVis)
    {
        wxLogMessage("Hiding!");
        popup->Hide();
        popupVis = false;
    }
#endif

  if (event.RightDown()) {
    if (m_foundState) {
      wxMenu *contextMenu = new wxMenu;

      //wxLogMessage("Right Clicked on Transponder!");

      wxMenuItem *id_item = 0;
      wxString transponderIDString;
      transponderIDString.Printf("ID: %d", transponderFoundID);
      id_item = new wxMenuItem(contextMenu, ID_TPR_ID, _(transponderIDString));

      wxMenuItem *release_item = 0;
      release_item = new wxMenuItem(contextMenu, ID_TPR_RELEASE, _("Release Transponder"));

      // wxMenuItem *release_item = 0;
      // release_item = new wxMenuItem(contextMenu, ID_TPR_QUERY, _("Query
      // Transponder") );

      // wxMenuItem *release_item = 0;
      // release_item = new wxMenuItem(contextMenu, ID_TPR_MUTE, _("Mute
      // Transponder") );

      wxMenuItem *recovered_item = 0;
      if (m_foundState->recovered_state == eREC_DEPLOYED)
      {
        recovered_item = new wxMenuItem(contextMenu, ID_TPR_RECOVER, _("Mark Recovered") );
      }
      else if (m_foundState->recovered_state == eREC_RECOVERED)
      {
        recovered_item = new wxMenuItem(contextMenu, ID_TPR_RECOVER, _("Mark Deployed") );
      }
      
      // wxMenuItem *release_item = 0;
      // release_item = new wxMenuItem(contextMenu, ID_TPR_LOST, _("Report Lost") );

      wxMenuItem *delete_item = 0;
      delete_item = new wxMenuItem(contextMenu, ID_TPR_DELETE, _("Delete Transponder") );

      wxMenuItem *edit_item = 0;
      edit_item = new wxMenuItem(contextMenu, ID_TPR_EDIT, _("Edit Transponder") );

      // wxMenuItem *release_item = 0;
      // release_item = new wxMenuItem(contextMenu, ID_TPR_RELEASE, _("Info") );

#ifdef __ANDROID__
      wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
      release_item->SetFont(*pFont);
      id_item->SetFont(*pFont);
      edit_item->SetFont(*pFont);
#endif

      contextMenu->Append(id_item);
      contextMenu->Append(release_item);

      if (m_foundState->markID > 0)
      {
        contextMenu->Append(recovered_item);
      }

      contextMenu->Append(delete_item);
      contextMenu->Append(edit_item);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_EDIT, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);

      wxLogMessage("Creating popup!");

      //   Invoke the drop-down menu
      GetOCPNCanvasWindow()->PopupMenu(contextMenu, m_mouse_x, m_mouse_y);

      if (release_item) {
        GetOCPNCanvasWindow()->Disconnect(
            ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
      }
      else if (recovered_item){
        // Tell Transceiver we've recovered! Set Transponder
        wxLogMessage("Setting %d as Recovered!",transponderFoundID);

        GetOCPNCanvasWindow()->Disconnect(
          ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
      }
      else if (delete_item)
      {
        GetOCPNCanvasWindow()->Disconnect(
            ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);
      }

      bret = true;  // I have eaten this event
    }
  }

  return bret;
}

bool ropeless_pi::KeyboardEventHook(wxKeyEvent &event) {
  // Commented out for now - accelerator table approach is working
  // // Check if we have the Ropeless dialog open
  // if (m_pRLDialog) {
  //   // Check for Ctrl+M hotkey
  //   if (event.ControlDown() && event.GetKeyCode() == 'M') {
  //     wxLogMessage("Plugin KeyboardEventHook: Ctrl+M pressed!");
  //     wxCommandEvent cmdEvent(wxEVT_COMMAND_BUTTON_CLICKED);
  //     m_pRLDialog->OnShowOnMapButton(cmdEvent);
  //     return true;  // Event consumed
  //   }
  // }
  
  return false;  // Event not consumed, pass it on
}

void ropeless_pi::ShowPreferencesDialog(wxWindow *parent) {
    // Only allow one preferences dialog at a time
    if (m_pPrefsDialog) {
        // If dialog already exists, just bring it to front
        m_pPrefsDialog->Raise();
        return;
    }
    
    // Create the preferences dialog and track it
    m_pPrefsDialog = new RopelessPrefsDialog(this, parent);
    
    // Show modal dialog
    int result = m_pPrefsDialog->ShowModal();
    
    // Clean up after dialog closes
    if (m_pPrefsDialog) {
        m_pPrefsDialog->Destroy();
        m_pPrefsDialog = nullptr;
    }
}

void ropeless_pi::toggleTransponderRecovered(uint32_t markID)
{

  transponder_state* tstate = GetStateByMarkID(markID);

  if (tstate != NULL) 
  {     
    if (tstate->recovered_state == eREC_RECOVERED)
    {
      tstate->recovered_state = eREC_DEPLOYED;
    }
    else if (tstate->recovered_state == eREC_DEPLOYED)
    {
      tstate->recovered_state = eREC_RECOVERED;
      SendCommandMessage(tstate, eCMD_RECOVER);
    }
  }
  else
  {
    return;
  }
}

void ropeless_pi::releaseCallbackRecovered(void)
{
  transponder_state* tstate = GetStateByMarkID(m_release_tim_state.ptstate->markID);

  if (tstate != NULL) 
  {   
    tstate->recovered_state = eREC_RECOVERED;
    SendCommandMessage(tstate, eCMD_RECOVER);
  }
}

void ropeless_pi::releaseCallbackRetry(void)
{
  SendCommandMessage(m_release_tim_state.ptstate, eCMD_RELEASE);
}

// TCP NMEA Output Methods
void ropeless_pi::InitializeTCPOutput() {
    if (m_nmea_tcp_output) {
        return; // Already initialized
    }
    
    if (m_tcp_enabled && !m_tcp_host.IsEmpty() && m_tcp_port > 0) {
        wxLogMessage("NMEA TCP Output: Initializing TCP client connection to %s:%d", m_tcp_host, m_tcp_port);
        
        m_nmea_tcp_output = new NMEA_TCP_OutputConnection(m_tcp_host, m_tcp_port);
        m_nmea_tcp_output->SetAutoReconnect(m_tcp_auto_reconnect);
        
        // Start queue processing after object is fully constructed
        m_nmea_tcp_output->StartQueueProcessing();
        
        // Attempt initial connection
        m_nmea_tcp_output->Connect();
        wxLogMessage("NMEA TCP Output: TCP client initialized and connecting...");
    } else if (m_tcp_enabled) {
        wxLogMessage("NMEA TCP Output: TCP enabled but invalid host/port configuration - Host: '%s', Port: %d", 
                     m_tcp_host, m_tcp_port);
    } else {
        wxLogMessage("NMEA TCP Output: TCP client disabled in configuration");
    }
}

void ropeless_pi::ShutdownTCPOutput() {
    if (m_nmea_tcp_output) {
        try {
            wxLogMessage("NMEA TCP Output: Shutting down TCP connection...");
            
            // Ensure we don't try to use TCP connection during shutdown
            NMEA_TCP_OutputConnection* temp_tcp = m_nmea_tcp_output;
            m_nmea_tcp_output = nullptr;  // Null pointer FIRST to prevent further use
            
            // Now safely destroy the object
            temp_tcp->Disconnect();
            delete temp_tcp;
            
            wxLogMessage("NMEA TCP Output: Connection shutdown completed");
        } catch (...) {
            wxLogMessage("NMEA TCP Output: Exception during TCP shutdown, forcing cleanup");
            m_nmea_tcp_output = nullptr;
        }
    }
}

bool ropeless_pi::IsTCPOutputConnected() const {
    return m_nmea_tcp_output && m_nmea_tcp_output->IsConnected();
}

bool ropeless_pi::SendNMEAMessageTCP(RESPONSE* message) {

  // Send generic message via TCP output if possible

  // Check if tcp output exists // check if message is valid
  if (!m_nmea_tcp_output || !message) {
      wxLogMessage("Failed to send NMEA message");
      return false;
  }

  
  // Create sentence and write message to it
  SENTENCE sentence;
  if (!message->Write(sentence)) {
      wxLogError("NMEA TCP Output: Failed to write message to sentence");
      return false;
  }

  if (m_nmea_tcp_output->SendRawNMEA(wxString(sentence)))
  {
    // Log this to debug window / log file. trim /r/n off end
    GlobalRopelessDebugMessage("--> " +  wxString(sentence).Trim());
  }
  else
  {
    GlobalRopelessDebugMessage("xxx Failed to send message");
  }
}

bool ropeless_pi::SendRawNMEATCP(const wxString& nmea_sentence) {

  // Send raw string via TCP NMEA output
  if (!m_nmea_tcp_output) {
      return false;
  }
  
  if (!m_nmea_tcp_output->IsConnected()) {
      GlobalRopelessDebugMessage(wxString::Format("TCP Output: Not connected, attempting to send: %s", nmea_sentence));
      return false;
  }
  
  return m_nmea_tcp_output->SendRawNMEA(nmea_sentence);
}

void ropeless_pi::ConfigureTCPOutput(const wxString& host, int port, bool enabled, bool auto_reconnect) {
    m_tcp_host = host;
    m_tcp_port = port;
    m_tcp_enabled = enabled;
    m_tcp_auto_reconnect = auto_reconnect;
    
    // Restart connection with new settings
    ShutdownTCPOutput();
    if (enabled) {
        InitializeTCPOutput();
    }
}

wxString ropeless_pi::GetTCPOutputStatus() const {
    if (!m_tcp_enabled) {
        return _T("TCP Output: Disabled");
    }
    
    if (!m_nmea_tcp_output) {
        return wxString::Format(_T("TCP Output: Not initialized (Host: %s, Port: %d)"), m_tcp_host, m_tcp_port);
    }
    
    wxString status;
    if (m_nmea_tcp_output->IsConnected()) {
        status = wxString::Format(_T("TCP Output: Connected to %s:%d"), m_tcp_host, m_tcp_port);
    } else if (m_nmea_tcp_output->IsConnecting()) {
        status = wxString::Format(_T("TCP Output: Connecting to %s:%d..."), m_tcp_host, m_tcp_port);
    } else {
        status = wxString::Format(_T("TCP Output: Disconnected from %s:%d"), m_tcp_host, m_tcp_port);
    }
    
    if (m_nmea_tcp_output) {
        size_t queued = m_nmea_tcp_output->GetQueueSize();
        unsigned long sent = m_nmea_tcp_output->GetMessagesSent();
        if (queued > 0 || sent > 0) {
            status += wxString::Format(_T(" (Sent: %lu, Queued: %zu)"), sent, queued);
        }
    }
    
    return status;
}

// RSGML Message Generation
void ropeless_pi::SendGMLMessageForManualPlacement(transponder_state* state, double lat, double lon, double utc) {
  if (!state) return;

  wxLogMessage("Sending GML for manual placement!");

  // Create GML message
  GML gml_msg;

  // Fill in the RSGML fields based on manual placement
  gml_msg.MarkID = state->markID;              // Use transponder ID as MarkID
  gml_msg.MarkType = 1;                       // 1 = Manual placement mark type
  gml_msg.PosStatus = 1;                      // 1 = Valid/confirmed position
  gml_msg.TrawlID = state->trawl_id; // Use assigned trawl ID
  gml_msg.TrawlNum = state->trawl_num;    // Default trawl number
  gml_msg.Latitude = lat;                     // Placement latitude
  gml_msg.Longitude = lon;                    // Placement longitude
  gml_msg.Depth = (int)state->depth;          // Depth from state (may be 0 for manual)
  gml_msg.Ownership = 1;                      // 1 = Own vessel's equipment
  gml_msg.Source = 2;                         // 2 = Manual entry source
  gml_msg.DateNum = utc;                      // UTC timestamp as matlab datenum

  // Set ouptput container
  gml_msg.SetContainer(&m_NMEA0183_tx);

  // Send via TCP if connected
  SendNMEAMessageTCP(&gml_msg);

}

// Event Handler implementation
BEGIN_EVENT_TABLE(PI_EventHandler, wxEvtHandler)
EVT_TIMER(ROLLOVER_TIMER, PI_EventHandler::OnTimerEvent)
EVT_TIMER(HEAD_DOG_TIMER, PI_EventHandler::OnTimerEvent)

END_EVENT_TABLE()

PI_EventHandler::PI_EventHandler(ropeless_pi *parent) {
  m_parent = parent;
  Connect(
      wxEVT_PI_OCPN_DATASTREAM,
      (wxObjectEventFunction)(wxEventFunction)&PI_EventHandler::OnEvtOCPN_NMEA);
}

PI_EventHandler::~PI_EventHandler() {
  Disconnect(
      wxEVT_PI_OCPN_DATASTREAM,
      (wxObjectEventFunction)(wxEventFunction)&PI_EventHandler::OnEvtOCPN_NMEA);
}

void PI_EventHandler::OnTimerEvent(wxTimerEvent &event) {
  m_parent->ProcessTimerEvent(event);
}

void PI_EventHandler::PopupMenuHandler(wxCommandEvent &event) {
  m_parent->PopupMenuHandler(event);
}

void PI_EventHandler::OnEvtOCPN_NMEA(PI_OCPN_DataStreamEvent &event) {
#if 0
    wxString str_buf = event.ProcessNMEA4Tags();

    g_NMEA0183 << str_buf;
    if( g_NMEA0183.PreParse() )
    {
        if( g_NMEA0183.LastSentenceIDReceived == _T("RMC") )
        {
            if( g_NMEA0183.Parse() )
            {
                if( g_NMEA0183.Rmc.IsDataValid == NTrue )
                {
                    ll_valid = true;    // tentatively

                    if( !wxIsNaN(g_NMEA0183.Rmc.Position.Latitude.Latitude) )
                    {
                        double llt = g_NMEA0183.Rmc.Position.Latitude.Latitude;
                        int lat_deg_int = (int) ( llt / 100 );
                        double lat_deg = lat_deg_int;
                        double lat_min = llt - ( lat_deg * 100 );
                        gLat = lat_deg + ( lat_min / 60. );
                        if( g_NMEA0183.Rmc.Position.Latitude.Northing == South ) gLat = -gLat;
                    }
                    else
                        ll_valid = false;

                    if( !wxIsNaN(g_NMEA0183.Rmc.Position.Longitude.Longitude) )
                    {
                        double lln = g_NMEA0183.Rmc.Position.Longitude.Longitude;
                        int lon_deg_int = (int) ( lln / 100 );
                        double lon_deg = lon_deg_int;
                        double lon_min = lln - ( lon_deg * 100 );
                        gLon = lon_deg + ( lon_min / 60. );
                        if( g_NMEA0183.Rmc.Position.Longitude.Easting == West )
                            gLon = -gLon;
                    }
                    else
                        ll_valid = false;

                    gSog = g_NMEA0183.Rmc.SpeedOverGroundKnots;
                    gCog = g_NMEA0183.Rmc.TrackMadeGoodDegreesTrue;

                     if( !wxIsNaN(g_NMEA0183.Rmc.MagneticVariation) )
                     {
                         if( g_NMEA0183.Rmc.MagneticVariationDirection == East )
                             gVar = g_NMEA0183.Rmc.MagneticVariation;
                         else
                             if( g_NMEA0183.Rmc.MagneticVariationDirection == West )
                                 gVar = -g_NMEA0183.Rmc.MagneticVariation;

//                             g_bVAR_Rx = true;
//                         gVAR_Watchdog = gps_watchdog_timeout_ticks;
                     }

 //                   sfixtime = g_NMEA0183.Rmc.UTCTime;

//                     if(ll_valid )
//                     {
//                         gGPS_Watchdog = gps_watchdog_timeout_ticks;
//                         wxDateTime now = wxDateTime::Now();
//                         m_fixtime = now.GetTicks();
//                     }
                    pos_valid = ll_valid;
                    gGPS_Watchdog = 0;          // feed the dog
                    m_parent->ProcessTenderFix();

                }
                else
                    pos_valid = false;
            }
        }
        else
            if( g_NMEA0183.LastSentenceIDReceived == _T("HDT") )
            {
                if( g_NMEA0183.Parse() )
                {
                    gHdt = g_NMEA0183.Hdt.DegreesTrue;
                    if( !wxIsNaN(g_NMEA0183.Hdt.DegreesTrue) )
                    {
                      //  g_bHDT_Rx = true;
                        gHDT_Watchdog = 0;
                    }
                }
            }

        else
              if( g_NMEA0183.LastSentenceIDReceived == _T("HDM") )
              {
                  if( g_NMEA0183.Parse() )
                  {
                      gHdm = g_NMEA0183.Hdm.DegreesMagnetic;
                      //if( !wxIsNaN(g_NMEA0183.Hdm.DegreesMagnetic) )
                        //  gHDx_Watchdog = gps_watchdog_timeout_ticks;
                  }
              }

    }
#endif
}

//=============================================================================
// Trawl Tracker Method Implementations
//=============================================================================

std::vector<transponder_state*> trawl_tracker::getTransponders() const {
  std::vector<transponder_state*> result;
  
  for (auto* t : transponderStatus) {
    if (t && t->trawl_id == trawl_id) {
      result.push_back(t);
    }
  }
  return result;
}

std::vector<uint32_t> trawl_tracker::getTransponderIds() const {
  std::vector<uint32_t> result;
  
  for (auto* t : transponderStatus) {
    if (t && t->trawl_id == trawl_id) {
      result.push_back(t->markID);
    }
  }
  return result;
}

transponder_state* trawl_tracker::getStartEndTransponder() const {
  for (auto* t : transponderStatus) {
    if (t && t->trawl_id == trawl_id && t->is_trawl_start_end) {
      return t;
    }
  }
  return nullptr;
}

std::vector<transponder_state*> trawl_tracker::getOrderedTransponders() const {
  auto transponders = getTransponders();
  
  // Sort by trawl_num
  std::sort(transponders.begin(), transponders.end(), 
           [](const transponder_state* a, const transponder_state* b) {
             return a->trawl_num < b->trawl_num;
           });
  
  return transponders;
}

bool trawl_tracker::addTransponder(uint32_t transponder_id, int position) {
  transponder_state* t = nullptr;
  for (auto* state : transponderStatus) {
    if (state && state->markID == transponder_id) {
      t = state;
      break;
    }
  }
  if (!t) return false;
  
  // Remove from any existing trawl first
  if (t->trawl_id != 0) {
    // Could notify old trawl here if needed
  }
  
  // Assign to this trawl
  t->trawl_id = trawl_id;
  t->trawl_num = (position >= 0) ? position : traps_in_set();
  t->is_trawl_start_end = false;  // Reset start/end flag when adding to trawl
  
  return true;
}

bool trawl_tracker::removeTransponder(uint32_t transponder_id) {
  transponder_state* t = nullptr;
  for (auto* state : transponderStatus) {
    if (state && state->markID == transponder_id) {
      t = state;
      break;
    }
  }
  if (!t || t->trawl_id != trawl_id) return false;
  
  // Remove from trawl
  t->trawl_id = 0;
  t->is_trawl_start_end = false;
  t->trawl_num = -1;
  
  // Reorder remaining transponders
  reorderTransponders();
  
  return true;
}

void trawl_tracker::setStartEnd(uint32_t transponder_id) {
  // Clear any existing start/end markers in this trawl
  for (auto* t : getTransponders()) {
    if (t) t->is_trawl_start_end = false;
  }
  
  // Set new start/end
  transponder_state* t = nullptr;
  for (auto* state : transponderStatus) {
    if (state && state->markID == transponder_id) {
      t = state;
      break;
    }
  }
  if (t && t->trawl_id == trawl_id) {
    t->is_trawl_start_end = true;
  }
}

void trawl_tracker::reorderTransponders() {
  auto transponders = getTransponders();
  
  // Sort by current position, then reassign consecutive positions
  std::sort(transponders.begin(), transponders.end(), 
           [](const transponder_state* a, const transponder_state* b) {
             return a->trawl_num < b->trawl_num;
           });
  
  for (size_t i = 0; i < transponders.size(); ++i) {
    transponders[i]->trawl_num = static_cast<int>(i);
  }
}

int trawl_tracker::traps_in_set() const {
  return static_cast<int>(getTransponders().size());
}

bool trawl_tracker::hasTransponder(uint32_t transponder_id) const {
  for (auto* state : transponderStatus) {
    if (state && state->markID == transponder_id) {
      return state->trawl_id == trawl_id;
    }
  }
  return false;
}

bool trawl_tracker::isStartEnd(uint32_t transponder_id) const {
  for (auto* state : transponderStatus) {
    if (state && state->markID == transponder_id) {
      return state->trawl_id == trawl_id && state->is_trawl_start_end;
    }
  }
  return false;
}

int trawl_tracker::getTransponderPosition(uint32_t transponder_id) const {
  for (auto* state : transponderStatus) {
    if (state && state->markID == transponder_id) {
      return (state->trawl_id == trawl_id) ? state->trawl_num : -1;
    }
  }
  return -1;
}


void trawl_tracker::addPathPoint(double lat, double lon) {
  trawl_path.push_back(trawl_coordinate(lat, lon));
}

void trawl_tracker::clearPath() {
  trawl_path.clear();
}
