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
#include "transponderReleaseDlgImpl.h"
#include "haversine.h"
//#include "graphics.h"

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

wxString colorTableNames[] = {"LIME GREEN",  // looks darker green
                              "ORANGE",      // looks darker red
                              "MAGENTA", "CYAN", "YELLOW"};

wxString colorTableNamesColorblind[] = {"CYAN",       // colorblind-friendly cyan instead of green (owned)
                                        "ORANGE",     // colorblind-friendly orange instead of red (non-owned)
                                        "MAGENTA", "CYAN", "YELLOW"};
                            
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

  m_Thread_run_flag = -1;

  g_iconTypeArray.Add(_T("Scaled Vector Icon"));
  g_iconTypeArray.Add(_T("Generic Ship Icon"));

  m_NMEA0183.TalkerID = _T ( "RF" );

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
  m_debug_enabled = false;
  m_debug_show_nmea = false;
  m_debug_show_log = false;
  m_pRLDialog = nullptr;
  m_pPrefsDialog = nullptr;
  
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
  Start(1000, wxTIMER_CONTINUOUS);

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
  
  if (m_releaseDlg) {
    try {
      if (m_releaseDlg->IsShown()) {
        m_releaseDlg->Hide();
      }
      m_releaseDlg->Destroy();
    } catch (...) {
      // Ignore any exceptions during cleanup
    }
    m_releaseDlg = nullptr;
  }
  
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
  m_releaseDlg = nullptr;
  
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
        m_parent_window, this, -1, "Ropeless Fishing v3.0.0.0", wxDefaultPosition,
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
      // wxLogMessage("Id: %d, Pair: %d, Owned: %d", mPl.xpdrId, mPl.pairId,
      //              mPl.isOwned);

      if (mPl.valid) {
        placeTransponderManually(mPl.xpdrId, mPl.pairId, mp_lat, mp_lon, utc_d, mPl.positionSource);
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
      // wxLogMessage("Id: %d, Pair: %d, Owned: %d", mPl.xpdrId, mPl.pairId,
      //              mPl.isOwned);

      if (mPl.valid) {
        placeTransponderManually(mPl.xpdrId, mPl.pairId, m_ownship_lat, m_ownship_lon, utc_d, mPl.positionSource);
      }
    }
  }
    // TODO: Manual Placement Option via right-click
    //  Get lat long of mouse when clicked to create new transponder dot
    //  Capture current UTC time / date of placement
    //  Spawn Dialog for manualPlacement
    //  Capture Ok / Cancel
    //  Process fields if OK
    //  Check Lat/Long/Id/Owned

  // startSim();
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
      PushNMEABuffer(snt.Sentence);
      
      // Display sent NMEA message in debug window
      if (m_pRLDialog) {
        m_pRLDialog->AddDebugMessage("--> " + snt.Sentence.Trim());
      }

      handled = true;
      break;
    }

    case ID_TPR_RELEASE: {
      wxString msg("Send Release to Transponder: ");
      wxString msg1;
      msg1.Printf("%d\n", m_foundState->ident);
      msg += msg1;

      long result = -1;
      myNumberEntryDialog dialog;
      myOkDialog okDialog;

      if (m_foundState->ident > 0) {

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

      if (result >= 0) SendCommandMessage(m_foundState, eCMD_RELEASE);

      handled = true;
      break;
    }

    case ID_TPR_DELETE: {
      wxString msg("Delete Transponder: ");
      wxString msg1;
      msg1.Printf("%d\n", g_ropelessPI->m_foundState->ident);
      msg += msg1;

      long result = -1;
      myOkDialog dialog2;

#ifdef __ANDROID__
      wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
      dialog2.SetFont(*pFont);
#endif

      dialog2.Create(GetOCPNCanvasWindow(), msg, "Ropeless Plugin Message", 0,
                     0, 100000, wxDefaultPosition);

      wxLogMessage("Deleting Transponder!");

      if (dialog2.ShowModal() == wxID_OK) {

        DeleteTransponder(g_ropelessPI->m_foundState->ident);

        //m_pRLDialog->RefreshTransponderList();
      }
      handled = true;
      break;
    }
    case ID_TPR_PLACE: {
      handled = true;
      break;
    }
    case ID_TPR_RECOVER: {

      toggleTransponderRecovered(m_foundState->ident);

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
  strncpy(str_ascii, (const char *)msg.mb_str(), 99);
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

bool ropeless_pi::SendCommandMessage(transponder_state *state, long code) {
  bool ret = true;

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

  wxString payload("$RSRLB,");
  wxString pl1;

  pl1.Printf("%d,%ld", state->ident, code);
  payload += pl1;

  unsigned char cs = ComputeChecksum(payload);
  pl1.Printf("*%02X", cs);
  payload += pl1;

  // Send via TCP output client!


  // // Send via UDP
  // payload += "\r\n";  // Add line ending for UDP
  // if (!m_tsock) {
  //   m_tconn_addr.Service(UDP_PORT);
  //   m_tconn_addr.BroadcastAddress();

  //   wxString a = m_tconn_addr.IPAddress();
  //   m_tsock = new wxDatagramSocket(
  //       m_tconn_addr, wxSOCKET_BROADCAST | wxSOCKET_NOBIND | wxSOCKET_NOWAIT |
  //                         wxSOCKET_REUSEADDR);

  //   if (m_tsock == NULL) {
  //     wxLogMessage("Error: Release UDP Socket returned NULL!");
  //     return false;
  //   }

  //   int broadcastEnable = 1;
  //   m_tsock->SetOption(SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
  //                      sizeof(broadcastEnable));
  // }

  // wxDatagramSocket *udp_socket;
  // udp_socket = dynamic_cast<wxDatagramSocket *>(m_tsock);

  // if (udp_socket && udp_socket->IsOk()) {
  //   udp_socket->SendTo(m_tconn_addr, payload.mb_str(), payload.size());

  //   if (udp_socket->Error()) {
  //     wxString emsg;
  //     wxSocketError err = udp_socket->LastError();
  //     emsg.Printf("Error: Sending on UDP Socket: %d", err);
  //     wxLogMessage(emsg);
  //     ret = false;
  //   }
  //   else
  //   {
  //     wxString ws;
  //     ws.Printf("Cmd Sent: %s", payload);
  //     wxLogMessage(ws);
  //   }
  // } 
  // else
  // {
  //   ret = false;
  // }

  if (code == eCMD_RELEASE)
  {
    if (ret != false) {
      state->release_status = -5;
      m_release_tim_state.ptstate = state;
      startReleaseTimer();
    } 
    else{
      state->release_status = -4;
      m_release_tim_state.ptstate = state;
      stopReleaseTimer();

      wxLogMessage("Release request failed!");
      updateReleaseDialog(true);

    }
  }

  return ret;
}

void ropeless_pi::updateReleaseDialog(bool show)
{
  if (NULL == m_releaseDlg) {

    m_releaseDlg = new transponderReleaseDlgImpl(
        m_parent_window, this, -1, "Transponder Release Status", wxDefaultPosition,
        wxDefaultSize, wxCAPTION | wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
    m_releaseDlg->SetFont(*pFont);
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
  m_releaseDlg->updateStatus(rid);

  if (state->release_status != -5 && state->release_status <= 0)
  {
    m_release_tim_state.timer_state = 0;
    m_releaseDlg->showButtons();
  }
  else
  {
    m_releaseDlg->hideButtons();
  }

  if (state->release_status > 0)
  {
    m_releaseDlg->Show(m_releaseDlg->IsShown());
  }
  else
  {
    m_releaseDlg->Show(true);
  }

  m_releaseDlg->updateID(m_release_tim_state.ptstate->ident);
  m_releaseDlg->Layout();
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

  if (state->ident != timer_state->ident) return;

  updateReleaseDialog(true);
}

void ropeless_pi::startDistanceTimer() {
  m_distanceTimer.SetOwner(this, DISTANCE_TIMER);
  m_distanceTimer.Start(1000, wxTIMER_CONTINUOUS);
}

void ropeless_pi::stopDistanceTimer() { m_distanceTimer.Stop(); }

void ropeless_pi::ProcessDistanceTimerEvent(wxTimerEvent &event) {

  if (m_pRLDialog != NULL) {
    for (unsigned int i = 0; i < transponderStatus.size(); i++) {
      transponder_state* t = transponderStatus[i];
      t->distance = haversineDistance(m_ownship_lat,m_ownship_lon,t->predicted_lat,t->predicted_lon);
    }

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
    wxLogMessage("Release Timer Expired for ID: %d",m_release_tim_state.ptstate->ident);
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
  // Age the transponder history records
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];

    std::deque<transponder_state_history *>::iterator it =
        state->historyQ.begin();
    while (it != state->historyQ.end()) {
      transponder_state_history *tsh = *it++;
      if (tsh->tsh_timer_age > 0) tsh->tsh_timer_age--;
    }
  }

  RequestRefresh(GetOCPNCanvasWindow());
}

void ropeless_pi::populateTransponderNode(pugi::xml_node &transponderNode,
                                          transponder_state *state) {
  pugi::xml_node child;

  child = transponderNode.append_child("ID");
  wxString ss;
  ss.Printf("%d", state->ident);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("identPartner");
  ss.Printf("%d", state->ident_partner);
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

  child = transponderNode.append_child("Range");
  ss.Printf("%f", state->range);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Pings");
  ss.Printf("%d", state->pings);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Depth");
  ss.Printf("%f", state->depth);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());

  child = transponderNode.append_child("Temp");
  ss.Printf("%f", state->temp);
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
  ss.Printf("%d", state->mfg_id);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("Mfg");
  ss.Printf("%d", state->mfg);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("Ownership");
  ss.Printf("%d", state->ownership);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("Source");
  ss.Printf("%d", state->source);
  child.append_child(pugi::node_pcdata).set_value(ss.c_str());
  
  child = transponderNode.append_child("DateNum");
  ss.Printf("%f", state->date_num);
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
}

void ropeless_pi::SaveTransponderStatus() {
  pugi::xml_document transponderStatusDoc;
  pugi::xml_node transpondersNode =
      transponderStatusDoc.append_child("transponders");

  pugi::xml_node childT = transpondersNode.append_child("version");
  childT.append_child(pugi::node_pcdata).set_value("3.0.0");
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
        state->ident = atoi(child.first_child().value());
      }
      if (!strcmp(child.name(), "identPartner")) {
        state->ident_partner = atoi(child.first_child().value());
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
      if (!strcmp(child.name(), "Range")) {
        wxString val(child.first_child().value());
        double dval;
        val.ToDouble(&dval);
        state->range = dval;
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
      if (!strcmp(child.name(), "Temp")) {
        wxString val(child.first_child().value());
        double dval;
        val.ToDouble(&dval);
        state->temp = dval;
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
        if (!strcmp(child.name(), "Mfg")) {
          state->mfg = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "Ownership")) {
          state->ownership = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "Source")) {
          state->source = atoi(child.first_child().value());
        }
        if (!strcmp(child.name(), "DateNum")) {
          wxString val(child.first_child().value());
          double dval;
          val.ToDouble(&dval);
          state->date_num = dval;
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
  }

  pugi::xml_node statusRoot = transponderStatusXML.first_child();

  for (pugi::xml_node element = statusRoot.first_child(); element;
       element = element.next_sibling()) {
    if (!strcmp(element.name(), "transponder")) {
      transponder_state *this_state = new transponder_state;
      ;
      if (parseTransponderNode(element, this_state)) {
        if (this_state->ident > 0) {
          this_state->color_index = COLOR_INDEX_GREEN;
        } else {
          this_state->color_index = COLOR_INDEX_RED;
        }

        transponderStatus.push_back(this_state);
      } else {
        delete this_state;
      }
    }
  }
}

void ropeless_pi::RenderTransponder(transponder_state *state) {
  int circle_size = 10;

  wxPoint ab;
  wxString colorName = GetColorName(state->color_index);
  wxColour rcolour = wxTheColourDatabase->Find(colorName);
  int opacity;

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
    // Debug logging
    // wxLogMessage("Rendering cloud position grey dot for transponder ID: %d", state->ident);
    
    // Draw grey circle for cloud positions
    wxColour greyColour(128, 128, 128, opacity); // Grey color
    wxPen greyPen(greyColour);
    wxBrush greyBrush(greyColour);
    m_oDC->SetPen(greyPen);
    m_oDC->SetBrush(greyBrush);
    m_oDC->DrawCircle(ab.x, ab.y, circle_size);
  } else {
    // Debug logging for other position sources
    // wxLogMessage("Rendering circle for transponder ID: %d, position_source: %d (%s)", 
    //             state->ident, state->position_source, 
    //             (state->position_source < 4) ? positionSourceNames[state->position_source] : "UNKNOWN");
    
    // Draw regular circle for other position sources
    wxPen dpen(rcolour);
    wxBrush dbrush(rcolour);
    m_oDC->SetPen(dpen);
    m_oDC->SetBrush(dbrush);
    m_oDC->DrawCircle(ab.x, ab.y, circle_size);
  }

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

#if 0
    // Render the history buffer, if present
    std::deque<transponder_state_history *>::iterator it = state->historyQ.begin();
    while (it != state->historyQ.end()){
        transponder_state_history *tsh = *it++;

        wxPoint abh;
        GetCanvasPixLL(g_vp, &abh, tsh->predicted_lat, tsh->predicted_lon);

        wxColour hcolour(rcolour.Red(), rcolour.Green(), rcolour.Blue(),
                         (tsh->tsh_timer_age / HISTORY_FADE_SECS) * 254);
        wxPen dpen( hcolour );
        wxBrush dbrush( hcolour );
        m_oDC->SetPen( dpen );
        m_oDC->SetBrush( dbrush );
        m_oDC->DrawCircle( abh.x, abh.y, circle_size/2);
    }
#endif

  // Draw an "X" over the primary target
  if (state->ident != state->ident_partner) {
    // Use appropriate size for the "X" based on shape
    int x_size = (state->position_source == ePOS_SOURCE_CLOUD) ? (circle_size + 2) : circle_size;
    
    wxPoint x1(ab.x - x_size * .707, ab.y - x_size * .707);
    wxPoint x2(ab.x + x_size * .707, ab.y + x_size * .707);
    wxPoint x3(ab.x - x_size * .707, ab.y + x_size * .707);
    wxPoint x4(ab.x + x_size * .707, ab.y - x_size * .707);

    wxColour pColour = wxColour(0, 0, 0, opacity);
    wxPen xpen(pColour, 3);
    m_oDC->SetPen(xpen);
    m_oDC->DrawLine(x1.x, x1.y, x2.x, x2.y, true);
    m_oDC->DrawLine(x3.x, x3.y, x4.x, x4.y, true);
  }

  // Display transponder number above the shape (skip for cloud positions and non-owned transponders)
  if (state->position_source != ePOS_SOURCE_CLOUD && state->ident > 0) {
    wxString transponderText = wxString::Format(wxT("%d"), state->ident);
  
  // Create a memory bitmap to render text
  wxFont textFont(16, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
  
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
  } // End of non-cloud position ID display
}

void ropeless_pi::RenderTrawlConnector(transponder_state *state1,
                                       transponder_state *state2) {
  wxPoint P1, P2;
  GetCanvasPixLL(g_vp, &P1, state1->predicted_lat, state1->predicted_lon);
  GetCanvasPixLL(g_vp, &P2, state2->predicted_lat, state2->predicted_lon);

  // TODO: Check if both transponders are recovered. Set trawl connector opacity too

  wxColour rcolour = wxTheColourDatabase->Find(wxString("BLACK"));
  wxPen dpen(rcolour, 3);
  wxBrush dbrush(rcolour);

  m_oDC->DrawLine(P1.x, P1.y, P2.x, P2.y, true);
}

void ropeless_pi::RenderVesselRangeCircle() {
  // Only draw if we have a valid vessel position
  if (m_ownship_lat == 0.0 && m_ownship_lon == 0.0) {
    return;
  }
  
  // Get vessel position in screen coordinates
  wxPoint vesselPos;
  GetCanvasPixLL(g_vp, &vesselPos, m_ownship_lat, m_ownship_lon);
  
  // Calculate 5 nautical miles in screen pixels
  // 1 nautical mile = 1852 meters
  double range_nm = 5.0;
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

transponder_state *ropeless_pi::GetStateByIdent(int identTarget) {
  transponder_state *rv = NULL;

  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];

    if (state->ident == identTarget) return state;
  }

  return rv;
}

bool ropeless_pi::DeleteTransponder(int id)
{
  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->ident == id) {

      SendCommandMessage(GetStateByIdent(id),eCMD_DELETE);

      transponderStatus.erase(transponderStatus.begin() + i);

      return true;
    }
  }

  return false;
}

void ropeless_pi::SendSyncMessage(void)
{
  transponder_state empty = {};
  SendCommandMessage(&empty,eCMD_SYNC);
}

wxString ropeless_pi::GetConnectionStatusText() {
  return _("Mode: UDP");
}

wxString ropeless_pi::GetColorName(int color_index) {
  if (m_colorblind_mode) {
    if (color_index >= 0 && color_index < COLOR_TABLE_COUNT) {
      return colorTableNamesColorblind[color_index];
    }
  } else {
    if (color_index >= 0 && color_index < COLOR_TABLE_COUNT) {
      return colorTableNames[color_index];
    }
  }
  return "MAGENTA";  // fallback color
}

void ropeless_pi::RenderTrawls() {
  //  Walk the vector of transponder status  
  static int render_log_count = 0;
  render_log_count++;
  
  // Log every 100 renders to avoid spam, but always log if we have transponders
  if (render_log_count % 100 == 0 || transponderStatus.size() > 0) {
    wxLogMessage("RenderTrawls: Found %d transponders to render", (int)transponderStatus.size());
  }
  
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    transponder_state *state = transponderStatus[i];
    
    // Always log cloud positions for debugging
    // if (state->position_source == ePOS_SOURCE_CLOUD || render_log_count % 100 == 0) {
    //   wxLogMessage("Rendering transponder %d: ID=%d, PosSource=%d, Lat=%.6f, Lon=%.6f", 
    //                i, state->ident, state->position_source, 
    //                state->predicted_lat, state->predicted_lon);
    // }

    RenderTransponder(state);

    if (state->ident_partner != state->ident) {
      transponder_state *statePartner = GetStateByIdent(state->ident_partner);
      if (statePartner) {
        RenderTrawlConnector(state, statePartner);
      }
    }
  }
}

bool ropeless_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp) {
#if 0
    g_vp = vp;
    Clone_VP(&g_ovp, vp);                // deep copy

    if (!m_oDC) m_oDC = new ODDC(dc);

    m_oDC->SetVP(vp);



    double selec_radius = (10 / vp->view_scale_ppm) / (1852. * 60.);
    m_select->SetSelectLLRadius(selec_radius);


// #if wxUSE_GRAPHICS_CONTEXT
//     wxMemoryDC *pmdc;
//     pmdc = wxDynamicCast(&dc, wxMemoryDC);
//     wxGraphicsContext *pgc = wxGraphicsContext::Create( *pmdc );
//     g_gdc = pgc;
//     g_pdc = &dc;
// #else
//     g_pdc = &dc;
// #endif

    //Render
    //RenderTrawls();
    //RenderIconDC( dc );


//#if wxUSE_GRAPHICS_CONTEXT
//    delete g_gdc;
//#endif

#endif
  return true;
}

bool ropeless_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp) {
  if (!m_oDC) m_oDC = new ODDC();

  m_oDC->SetVP(vp);

  //     g_gdc = NULL;
  //     g_pdc = NULL;

  g_vp = vp;
  Clone_VP(&g_ovp, vp);  // deep copy

  m_selectRadius = (10 / vp->view_scale_ppm) / (1852. * 60.);

  // m_select->SetSelectLLRadius(selec_radius);

  // Render vessel range circle first (behind transponders)
  RenderVesselRangeCircle();
  
  // Render transponders
  RenderTrawls();

  return true;
}

void ropeless_pi::ProcessRFACapture(void) {
  if (m_NMEA0183.LastSentenceIDReceived != _T("RFA")) {
    return;
  }

  int transponderIdent = m_NMEA0183.Rfa.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->ident == transponderIdent) {
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // If not present, create a new record, and add to vector
  if (this_transponder_state == NULL) {
    this_transponder_state = new transponder_state;

    if (transponderIdent > 0) {
      this_transponder_state->color_index = COLOR_INDEX_GREEN;
    } else {
      this_transponder_state->color_index = COLOR_INDEX_RED;
    }

    transponderStatus.push_back(this_transponder_state);
  } else {

    this_transponder_state->pings++;

    // Maintain history buffer
    transponder_state_history *this_transponder_state_history =
        new transponder_state_history;
    this_transponder_state_history->ident = this_transponder_state->ident;
    this_transponder_state_history->ident_partner =
        this_transponder_state->ident_partner;
    this_transponder_state_history->timeStamp =
        this_transponder_state->timeStamp;
    this_transponder_state_history->predicted_lat =
        this_transponder_state->predicted_lat;
    this_transponder_state_history->predicted_lon =
        this_transponder_state->predicted_lon;
    this_transponder_state_history->position_source =
        this_transponder_state->position_source;
    this_transponder_state_history->color_index =
        this_transponder_state->color_index;
    this_transponder_state_history->tsh_timer_age = HISTORY_FADE_SECS;

    if (this_transponder_state->historyQ.size() > 10) {
      this_transponder_state->historyQ.pop_back();
    }

    this_transponder_state->historyQ.push_front(this_transponder_state_history);
  }

  //  Update the instant record
  this_transponder_state->ident = transponderIdent;
  this_transponder_state->ident_partner = m_NMEA0183.Rfa.TransponderPartner;

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

  this_transponder_state->range = m_NMEA0183.Rfa.TransponderRange;
  this_transponder_state->bearing = m_NMEA0183.Rfa.TransponderBearing;
  this_transponder_state->depth = m_NMEA0183.Rfa.TransponderDepth;
  this_transponder_state->temp = m_NMEA0183.Rfa.TransponderTemp;
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

  PushNMEABuffer(rmc_sentence.Sentence);
  
  // Display sent NMEA message in debug window
  if (m_pRLDialog) {
    m_pRLDialog->AddDebugMessage("--> " + rmc_sentence.Sentence.Trim());
  }
}

void ropeless_pi::placeTransponderManually(int xpdrId, int pairId, double lat,
                                           double lon, double utc, int pos_source) {
  
  wxLogMessage("Placing Transponder Manually - ID: %d, Position Source: %s (%d)", 
               xpdrId, positionSourceNames[pos_source], pos_source);

  transponder_state *this_transponder_state;
  this_transponder_state = addTransponderPos(xpdrId);

  if (!this_transponder_state) {
    wxLogMessage("ERROR: Failed to create transponder state for ID: %d", xpdrId);
    return;
  }

  this_transponder_state->ident = xpdrId;
  this_transponder_state->ident_partner = pairId;

  this_transponder_state->timeStamp = utc;

  this_transponder_state->predicted_lat = lat;
  this_transponder_state->predicted_lon = lon;
  this_transponder_state->position_source = pos_source;
  
  wxLogMessage("Transponder created: ID=%d, Lat=%.6f, Lon=%.6f, PosSource=%d (%s)", 
               this_transponder_state->ident, 
               this_transponder_state->predicted_lat, 
               this_transponder_state->predicted_lon,
               this_transponder_state->position_source,
               positionSourceNames[this_transponder_state->position_source]);

  this_transponder_state->range = 0;
  this_transponder_state->bearing = 0;
  this_transponder_state->depth = 0;
  this_transponder_state->temp = 0;
  this_transponder_state->pings = 0;
  this_transponder_state->batt_stat = 0;

  // Generate and send RSGML message for manual placement
  SendGMLMessageForManualPlacement(this_transponder_state, lat, lon, utc);

}

// Check for transponder state in list. if does not exist add new one
transponder_state *ropeless_pi::addTransponderPos(int transponderIdent) {
  transponder_state *this_transponder_state = NULL;

  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->ident == transponderIdent) {
      this_transponder_state = transponderStatus[i];
      return this_transponder_state;
    }
  }

  // If not present, create a new record, and add to vector
  if (this_transponder_state == NULL) {
    this_transponder_state = new transponder_state;

    if (transponderIdent > 0) {
      this_transponder_state->color_index = 0;
    } else {
      this_transponder_state->color_index = 1;
    }

    // Select a new color for this new transponder
    // this_transponder_state->color_index = m_colorIndexNext;
    // m_colorIndexNext++;

    // if(m_colorIndexNext == COLOR_TABLE_COUNT)
    //   m_colorIndexNext = 0;

    transponderStatus.push_back(this_transponder_state);
  } else {
    // Maintain history buffer
    transponder_state_history *this_transponder_state_history =
        new transponder_state_history;
    this_transponder_state_history->ident = this_transponder_state->ident;
    this_transponder_state_history->ident_partner =
        this_transponder_state->ident_partner;
    this_transponder_state_history->timeStamp =
        this_transponder_state->timeStamp;
    this_transponder_state_history->predicted_lat =
        this_transponder_state->predicted_lat;
    this_transponder_state_history->predicted_lon =
        this_transponder_state->predicted_lon;
    this_transponder_state_history->position_source =
        this_transponder_state->position_source;
    this_transponder_state_history->color_index =
        this_transponder_state->color_index;
    this_transponder_state_history->tsh_timer_age = HISTORY_FADE_SECS;

    if (this_transponder_state->historyQ.size() > 10) {
      this_transponder_state->historyQ.pop_back();
    }

    this_transponder_state->historyQ.push_front(this_transponder_state_history);
  }

  return this_transponder_state;
}

void ropeless_pi::ProcessRLACapture(void) {
  if (m_NMEA0183.LastSentenceIDReceived != _T("RLA")) {
    return;
  }

  int transponderIdent = m_NMEA0183.Rla.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0; i < transponderStatus.size(); i++) {
    if (transponderStatus[i]->ident == transponderIdent) {
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // Check if ident matches released transponder
  if (manualReleaseState.ident == transponderIdent)
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

  printf("%s\n", sentence.ToStdString().c_str());
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
        
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          SendNMEAMessage(&m_NMEA0183.Dbs);
        }
      }
    }
    if (m_NMEA0183.LastSentenceIDReceived == _T("GML")) {
      if (m_NMEA0183.Parse()) {
        wxLogMessage("GML Message Received: MarkID=%d, MarkType=%d, PosStatus=%d, TrawlID=%d, TrawlNum=%d, Lat=%.6f, Lon=%.6f, Depth=%d, MfgID=%d, Mfg=%d, Ownership=%d, Source=%d, DateNum=%.6f",
                     m_NMEA0183.Gml.MarkID,
                     m_NMEA0183.Gml.MarkType,
                     m_NMEA0183.Gml.PosStatus,
                     m_NMEA0183.Gml.TrawlID,
                     m_NMEA0183.Gml.TrawlNum,
                     m_NMEA0183.Gml.Latitude,
                     m_NMEA0183.Gml.Longitude,
                     m_NMEA0183.Gml.Depth,
                     m_NMEA0183.Gml.MfgID,
                     m_NMEA0183.Gml.Mfg,
                     m_NMEA0183.Gml.Ownership,
                     m_NMEA0183.Gml.Source,
                     m_NMEA0183.Gml.DateNum);
        
        // Update transponder state with GML data
        transponder_state *tstate = GetStateByIdent(m_NMEA0183.Gml.MarkID);
        if (!tstate) {
            tstate = addTransponderPos(m_NMEA0183.Gml.MarkID);
        }
        
        if (tstate) {
            // Update GML status parameters
            tstate->mark_type = m_NMEA0183.Gml.MarkType;
            tstate->pos_status = m_NMEA0183.Gml.PosStatus;
            tstate->trawl_id = m_NMEA0183.Gml.TrawlID;
            tstate->trawl_num = m_NMEA0183.Gml.TrawlNum;
            tstate->mfg_id = m_NMEA0183.Gml.MfgID;
            tstate->mfg = m_NMEA0183.Gml.Mfg;
            tstate->ownership = m_NMEA0183.Gml.Ownership;
            tstate->source = m_NMEA0183.Gml.Source;
            tstate->date_num = m_NMEA0183.Gml.DateNum;
            
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
        }
                     
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          SendNMEAMessage(&m_NMEA0183.Gml);
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
        transponder_state *tstate = GetStateByIdent(m_NMEA0183.Gms.MarkID);
        if (!tstate) {
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
            
            // Update range - use surface range as primary range
            if (m_NMEA0183.Gms.SurfaceRange > 0) {
                tstate->range = m_NMEA0183.Gms.SurfaceRange;
            }
            
            tstate->timeStamp = wxDateTime::Now().GetTicks();
        }
                     
        // Forward message via TCP if connected
        if (IsTCPOutputConnected()) {
          SendNMEAMessage(&m_NMEA0183.Gms);
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
          SendNMEAMessage(&m_NMEA0183.Gmr);
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
      transponderFoundID = transponderStatus[i]->ident;
      //wxLogMessage("Found Transponder! %d", m_foundState->ident);
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
  //       transponderFoundID = transponderStatus[i]->ident;
  //       //wxLogMessage("Found Transponder! %d",m_foundState->ident);
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

      // wxMenuItem *release_item = 0;
      // release_item = new wxMenuItem(contextMenu, ID_TPR_RELEASE, _("Info") );

#ifdef __ANDROID__
      wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
      release_item->SetFont(*pFont);
      id_item->SetFont(*pFont);
#endif

      contextMenu->Append(id_item);
      contextMenu->Append(release_item);

      if (m_foundState->ident > 0)
      {
        contextMenu->Append(recovered_item);
      }

      contextMenu->Append(delete_item);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL, this);

      GetOCPNCanvasWindow()->Connect(
          ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
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

void ropeless_pi::toggleTransponderRecovered(int id)
{

  transponder_state* tstate = GetStateByIdent(id);

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
  transponder_state* tstate = GetStateByIdent(m_release_tim_state.ptstate->ident);

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

bool ropeless_pi::SendNMEAMessage(RESPONSE* message) {
    if (!m_nmea_tcp_output || !message) {
        return false;
    }
    
    // Create sentence for logging before sending
    SENTENCE sentence;
    if (message->Write(sentence)) {
        // Display sent NMEA message in debug window
        if (m_pRLDialog) {
            m_pRLDialog->AddDebugMessage("--> " + sentence.Sentence.Trim());
        }
    }
    
    return m_nmea_tcp_output->SendNMEAMessage(message);
}

bool ropeless_pi::SendRawNMEA(const wxString& nmea_sentence) {
    if (!m_nmea_tcp_output) {
        return false;
    }
    
    if (!m_nmea_tcp_output->IsConnected()) {
        wxLogMessage("TCP Output: Not connected, attempting to send: %s", nmea_sentence);
        return false;
    }
    
    wxLogMessage("TCP Output: Sending raw NMEA: %s", nmea_sentence);
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
    
    // Set talker ID to "CP" for OpenCPN
    gml_msg.Talker = _T("CP");
    
    // Fill in the RSGML fields based on manual placement
    gml_msg.MarkID = state->ident;              // Use transponder ID as MarkID
    gml_msg.MarkType = 1;                       // 1 = Manual placement mark type
    gml_msg.PosStatus = 1;                      // 1 = Valid/confirmed position
    gml_msg.TrawlID = state->ident_partner;     // Use partner ID as TrawlID
    gml_msg.TrawlNum = 1;                       // Default trawl number
    gml_msg.Latitude = lat;                     // Placement latitude
    gml_msg.Longitude = lon;                    // Placement longitude
    gml_msg.Depth = (int)state->depth;          // Depth from state (may be 0 for manual)
    gml_msg.MfgID = 1001;                       // Default manufacturer ID
    gml_msg.Mfg = 10;                          // Default manufacturer code
    gml_msg.Ownership = 1;                      // 1 = Own vessel's equipment
    gml_msg.Source = 2;                         // 2 = Manual entry source
    gml_msg.DateNum = utc;                      // UTC timestamp as matlab datenum
    
    // Send via TCP if connected
    if (IsTCPOutputConnected()) {
        if (SendNMEAMessage(&gml_msg)) {
            wxLogMessage("RSGML: Sent manual mark placement - MarkID=%d, Lat=%.6f, Lon=%.6f, Time=%.6f", 
                         gml_msg.MarkID, gml_msg.Latitude, gml_msg.Longitude, gml_msg.DateNum);
        } else {
            wxLogMessage("RSGML: Failed to send manual mark placement message");
        }
    } else {
        wxLogMessage("RSGML: TCP not connected - manual mark placement not sent (MarkID=%d)", gml_msg.MarkID);
    }
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

// BEGIN_EVENT_TABLE(RopelessDialog, wxDialog)
// EVT_BUTTON(wxID_OK, RopelessDialog::OnOKClick)
// EVT_CLOSE(RopelessDialog::OnClose)
// END_EVENT_TABLE()

// RopelessDialog::RopelessDialog(wxWindow *parent, ropeless_pi *parent_pi,
//                                wxWindowID id, const wxString &title,
//                                const wxPoint &pos, const wxSize &size,
//                                long style)
//     : wxDialog(parent, id, title, pos, size, style) {
//   pParentPi = parent_pi;
//   wxFont *dFont = OCPNGetFont(_T("Dialog"), 0);
//   SetFont(*dFont);

//   this->SetSizeHints(wxDefaultSize, wxDefaultSize);

//   wxBoxSizer *bSizer2;
//   bSizer2 = new wxBoxSizer(wxVERTICAL);

//   long flags = wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES |
//                wxBORDER_SUNKEN;

//   // long flags = wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN;

//   m_pListCtrlTranponders = new OCPNListCtrl(
//       this, ID_TRANSPONDER_LIST, wxDefaultPosition, wxDefaultSize, flags);
//   bSizer2->Add(m_pListCtrlTranponders, 1, wxEXPAND | wxALL, 0);

// #ifdef __ANDROID__
//   wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
//   int char_size = pFont->GetPointSize();

//   char font_style_sheet[200];
//   sprintf(font_style_sheet, "QHeaderView::section {  font-size:%dpt; }",
//           char_size);

//   char item_font_style_sheet[200];
//   sprintf(item_font_style_sheet, "QTreeWidget {  font-size:%dpt; }", char_size);

//   std::ostringstream ss;
//   ss << qtRLStyleSheet << font_style_sheet << item_font_style_sheet;
//   m_pListCtrlTranponders->GetHandle()->setStyleSheet(ss.str().c_str());

// #endif

//   m_pListCtrlTranponders->Connect(
//       wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK,
//       wxListEventHandler(RopelessDialog::OnTargetRightClick), NULL, this);

//   m_pListCtrlTranponders->Connect(
//       wxEVT_COMMAND_LIST_COL_CLICK,
//       wxListEventHandler(RopelessDialog::OnTargetListColumnClicked), NULL,
//       this);

//   m_pListCtrlTranponders->Connect(
//       wxEVT_LIST_ITEM_SELECTED,
//       wxListEventHandler(RopelessDialog::OnTargetListSelected), NULL, this);

//   m_pListCtrlTranponders->Connect(
//       wxEVT_LIST_ITEM_DESELECTED,
//       wxListEventHandler(RopelessDialog::OnTargetListDeselected), NULL, this);

//   int dx = GetCharWidth();

//   wxSize txs = GetTextExtent("Color");
//   m_pListCtrlTranponders->InsertColumn(tlICON, _("Color"), wxLIST_FORMAT_CENTER,
//                                        txs.x + dx * 2);

//   txs = GetTextExtent("ID");
//   m_pListCtrlTranponders->InsertColumn(tlIDENT, _("ID"), wxLIST_FORMAT_CENTER,
//                                        txs.x + dx * 2);

//   txs = GetTextExtent("Release Status");
//   m_pListCtrlTranponders->InsertColumn(tlRELEASE_STATUS, _("Release Status"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   txs = GetTextExtent("LastReportTime (UTC)");
//   m_pListCtrlTranponders->InsertColumn(tlTIMESTAMP, _("LastReportTime (UTC)"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   txs = GetTextExtent("Range, M");
//   m_pListCtrlTranponders->InsertColumn(tlRANGE, _("Range, M"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

// #ifdef SHOW_DISTANCE
//   txs = GetTextExtent("Distance, M");
//   m_pListCtrlTranponders->InsertColumn(tlDISTANCE, _("Distance, M"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);
// #endif

//   txs = GetTextExtent("Pings");
//   m_pListCtrlTranponders->InsertColumn(tlPINGS, _("Pings"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   txs = GetTextExtent("Depth, M");
//   m_pListCtrlTranponders->InsertColumn(tlDEPTH, _("Depth, M"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   txs = GetTextExtent("Temperature, C");
//   m_pListCtrlTranponders->InsertColumn(tlTEMP, _("Temperature, C"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   txs = GetTextExtent("Battery %");
//   m_pListCtrlTranponders->InsertColumn(tlBATT_STAT, _("Battery %"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   txs = GetTextExtent("Recovered Status");
//   m_pListCtrlTranponders->InsertColumn(tlRECOVERED, _("Recovered Status"),
//                                        wxLIST_FORMAT_CENTER, txs.x + dx * 2);

//   // Build the color indicator bitmaps, adding to an image lst
//   int imageRefSize = dx * 2;
//   wxImageList *imglist = new wxImageList(imageRefSize, imageRefSize, true, 1);

//   for (int i = 0; i < COLOR_TABLE_COUNT; i++) {
//     wxScreenDC sdc;

//     wxBitmap tbm(imageRefSize, imageRefSize, -1);
//     wxMemoryDC mdc(tbm);
//     mdc.Clear();
//     wxString colorName =
//         colorTableNames[i];  // colorTableNames[state->color_index];
//     wxColour rcolour = wxTheColourDatabase->Find(colorName);

//     if (!rcolour.IsOk()) rcolour = wxColour(255, 000, 255);

//     wxPen dpen(rcolour);
//     wxBrush dbrush(rcolour);
//     mdc.SetPen(dpen);
//     mdc.SetBrush(dbrush);

//     int xd = 0;
//     int yd = 0;
//     //    mdc.DrawRoundedRectangle(xd, yd, w+(label_offset * 2), h+2, -.25);
//     mdc.DrawRectangle(xd, yd, imageRefSize, imageRefSize);
//     mdc.SelectObject(wxNullBitmap);

//     imglist->Add(tbm);
//   }

//   m_pListCtrlTranponders->AssignImageList(imglist, wxIMAGE_LIST_SMALL);

// #ifndef __ANDROID__
//   wxStaticBoxSizer *sbSizerSim = new wxStaticBoxSizer(
//       new wxStaticBox(this, wxID_ANY, _("Simulator")), wxVERTICAL);
//   bSizer2->Add(sbSizerSim, 0, wxALL | wxEXPAND, 5);

//   m_simTextCtrl =
//       new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
//                      wxDefaultSize, wxTE_READONLY);
//   sbSizerSim->Add(m_simTextCtrl, 0, wxEXPAND | wxALL, 5);

//   if (wxFileExists(msgFileName)) m_simTextCtrl->SetValue(msgFileName);

//   wxBoxSizer *bsizersimButtons = new wxBoxSizer(wxHORIZONTAL);
//   sbSizerSim->Add(bsizersimButtons, 0, wxEXPAND, 5);

//   m_ChooseFileButton = new wxButton(this, wxID_ANY, _("Choose File..."),
//                                     wxDefaultPosition, wxDefaultSize, 0);
//   bsizersimButtons->Add(m_ChooseFileButton, 0, wxALL, 5);
//   m_ChooseFileButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
//                            &RopelessDialog::OnChooseFileButton, this);

//   m_StopSimButton = new wxButton(this, wxID_ANY, _("Stop Sim"),
//                                  wxDefaultPosition, wxDefaultSize, 0);
//   bsizersimButtons->Add(m_StopSimButton, 0, wxALL, 5);
//   m_StopSimButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
//                         &RopelessDialog::OnStopSimButton, this);

//   m_StartSimButton = new wxButton(this, wxID_ANY, _("StartSim"),
//                                   wxDefaultPosition, wxDefaultSize, 0);
//   bsizersimButtons->Add(m_StartSimButton, 0, wxALL, 5);
//   m_StartSimButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
//                          &RopelessDialog::OnStartSimButton, this);

//   m_ManualReleaseButton = new wxButton(this, wxID_ANY, _("Manual Release"),
//                                        wxDefaultPosition, wxDefaultSize, 0);
//   bsizersimButtons->Add(m_ManualReleaseButton, 0, wxALL, 5);
//   m_ManualReleaseButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
//                               &RopelessDialog::OnManualReleaseButton, this);

//   m_SyncButton = new wxButton(this, wxID_ANY, _("Sync"),
//                                        wxDefaultPosition, wxDefaultSize, 0);
//   bsizersimButtons->Add(m_SyncButton, 0, wxALL, 5);
//   m_SyncButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED,
//                               &RopelessDialog::OnSyncButton, this);

//   // Connection Status Display
//   m_ConnectionStatusText = new wxStaticText( this, wxID_ANY, _("Mode: UDP"), wxDefaultPosition, wxDefaultSize, 0 );
//   m_ConnectionStatusText->Wrap( -1 );
//   bsizersimButtons->Add( m_ConnectionStatusText, 0, wxALL, 5 );

//   if (pParentPi->m_simulatorTimer.IsRunning()) {
//     m_StartSimButton->Hide();
//     m_StopSimButton->Show();
//   } else {
//     m_StopSimButton->Hide();
//     m_StartSimButton->Show();
//   }
// #endif

//   m_sdbSizer1 = new wxStdDialogButtonSizer();
//   m_sdbSizer1OK = new wxButton(this, wxID_OK);
//   m_sdbSizer1->AddButton(m_sdbSizer1OK);
//   m_sdbSizer1->Realize();

//   bSizer2->Add(m_sdbSizer1, 0, wxBOTTOM | wxEXPAND | wxTOP, 5);

//   this->SetSizer(bSizer2);
//   this->Layout();
//   // bSizer2->Fit( this );

//   this->Centre(wxBOTH);

// }

// RopelessDialog::~RopelessDialog() {

//   // delete m_pSerialArray;
// }

// // Called on List right click. Attached to wxListEventHandler
// void RopelessDialog::OnTargetRightClick(wxListEvent &event) {
//   int mouseX;
//   int mouseY;
//   long index = -1;

//   if (m_pListCtrlTranponders->GetItemCount()) {
//     wxListItem item;
//     item.SetId(0);
//     wxRect rect;
//     m_pListCtrlTranponders->GetItemRect(item, rect);

//     const wxPoint pt = wxGetMousePosition();
//     mouseX = pt.x - m_pListCtrlTranponders->GetScreenPosition().x;
//     mouseY = pt.y - m_pListCtrlTranponders->GetScreenPosition().y;

// #ifndef __WXMSW__
//     mouseY -= rect.height;
// #endif

//     int flags;
//     index = m_pListCtrlTranponders->HitTest(wxPoint(mouseX, mouseY), flags);

//     if (index >= 0) {
//       wxString sID = m_pListCtrlTranponders->GetItemText(index, 1);
//       long fid = atoi(sID.ToStdString().c_str());

//       // search the transponder list for an ident match
//       long foundIndex = -1;
//       for (unsigned int i = 0; i < transponderStatus.size(); i++) {
//         transponder_state *state = transponderStatus[i];
//         if (state->ident == fid) {
//           foundIndex = i;
//           wxLogMessage("List found index: %d", index);
//           break;
//         }
//       }

//       if (foundIndex >= 0) {
//         g_ropelessPI->m_foundState = transponderStatus[foundIndex];

//         wxLogMessage("Right Clicked via List on ID: %d",
//                      g_ropelessPI->m_foundState->ident);

//         wxMenu *contextMenu = new wxMenu;

//         wxMenuItem *id_item = 0;
//         wxString transponderIDString;
//         transponderIDString.Printf("ID: %d", g_ropelessPI->m_foundState->ident);
//         id_item = new wxMenuItem(contextMenu, ID_TPR_ID, _(transponderIDString));
        
//         wxMenuItem *release_item = 0;
//         release_item = new wxMenuItem(contextMenu, ID_TPR_RELEASE,
//                                       _("Release Transponder"));

//         wxMenuItem *recovered_item = 0;
//         if (g_ropelessPI->m_foundState->recovered_state == eREC_DEPLOYED)
//         {
//           recovered_item = new wxMenuItem(contextMenu, ID_TPR_RECOVER, _("Mark Recovered") );
//         }
//         else if (g_ropelessPI->m_foundState->recovered_state == eREC_RECOVERED)
//         {
//           recovered_item = new wxMenuItem(contextMenu, ID_TPR_RECOVER, _("Mark Deployed") );
//         }

//         wxMenuItem *delete_item = 0;
//         delete_item = new wxMenuItem(contextMenu, ID_TPR_DELETE, _("Delete"));

// #ifdef __ANDROID__
//         wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
//         release_item->SetFont(*pFont);
// #endif

//         contextMenu->Append(id_item);
//         contextMenu->Append(release_item);
//         contextMenu->Append(recovered_item);
//         contextMenu->Append(delete_item);

//         GetOCPNCanvasWindow()->Connect(
//             ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
//             wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
//             pParentPi);

//         GetOCPNCanvasWindow()->Connect(
//             ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
//             wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
//             pParentPi);

//         GetOCPNCanvasWindow()->Connect(
//             ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
//             wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
//             pParentPi);

//         //   Invoke the drop-down menu
//         GetOCPNCanvasWindow()->PopupMenu(contextMenu, wxGetMousePosition().x,
//                                          wxGetMousePosition().y);

//         if (release_item)
//           GetOCPNCanvasWindow()->Disconnect(
//               ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
//               wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
//               pParentPi);

//         if (recovered_item)
//           GetOCPNCanvasWindow()->Disconnect(
//               ID_TPR_RECOVER, wxEVT_COMMAND_MENU_SELECTED,
//               wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
//               pParentPi);

//         if (delete_item)
//           GetOCPNCanvasWindow()->Disconnect(
//               ID_TPR_DELETE, wxEVT_COMMAND_MENU_SELECTED,
//               wxCommandEventHandler(ropeless_pi::PopupMenuHandler), NULL,
//               pParentPi);

//       }
//     }
//   }
// }

// wxArrayInt RopelessDialog::GetSelectedItems() {
//   wxArrayInt selectedItems;
//   long itemIndex = m_pListCtrlTranponders->GetNextItem(-1, wxLIST_NEXT_ALL,
//                                                        wxLIST_STATE_SELECTED);

//   while (itemIndex != wxNOT_FOUND) {
//     selectedItems.Add(itemIndex);
//     itemIndex = m_pListCtrlTranponders->GetNextItem(itemIndex, wxLIST_NEXT_ALL,
//                                                     wxLIST_STATE_SELECTED);
//   }

//   return selectedItems;
// }

// transponder_state *RopelessDialog::getXpdrFromIndex(int index) {
//   long fid;
//   transponder_state *state;

//   // Get idents of selected items
//   if (index >= 0) {
//     wxString sID = m_pListCtrlTranponders->GetItemText(index, 1);
//     fid = atoi(sID.ToStdString().c_str());

//     // search the transponder list for an ident match
//     long foundIndex = -1;
//     for (unsigned int i = 0; i < transponderStatus.size(); i++) {
//       state = transponderStatus[i];
//       if (state->ident == fid) {
//         foundIndex = i;
//         break;
//       }
//     }
//   }

//   return state;
// }

// void RopelessDialog::OnTargetListDeselected(wxListEvent &event) {
//   long deselectedIndex = event.GetIndex();
//   // wxLogMessage("Item deselected: %ld", deselectedIndex);

//   if (deselectedIndex >= 0) {
//     transponder_state *state = getXpdrFromIndex(deselectedIndex);

//     if (state->ident > 0) {
//       state->color_index = COLOR_INDEX_GREEN;

//     } else {
//       state->color_index = COLOR_INDEX_RED;
//     }

//     RequestRefresh(GetOCPNCanvasWindow());
//   }
// }

// void RopelessDialog::OnTargetListSelected(wxListEvent &event) {
//   wxArrayInt selectedItems = GetSelectedItems();
//   int numItems = selectedItems.GetCount();

//   wxString message = "Selected items: ";
//   for (size_t i = 0; i < numItems; i++) {
//     message += wxString::Format("%d ", selectedItems[i]);
//   }

//   if (numItems > 0) {
//     // wxLogMessage(message);

//     transponder_state *state = getXpdrFromIndex(selectedItems[0]);

//     state->color_index = COLOR_INDEX_GOLDEN;

//     RequestRefresh(GetOCPNCanvasWindow());
//   }
// }

// void RopelessDialog::OnTargetListColumnClicked(wxListEvent &event) {
//   int key = event.GetColumn();
//   wxListItem item;
//   // item.SetMask(wxLIST_MASK_IMAGE);

//   if (key == g_RopelessTargetList_sortColumn)
//     g_bRopelessTargetList_sortReverse = !g_bRopelessTargetList_sortReverse;
//   else {
//     // item.SetImage(-1);
//     // m_pListCtrlAISTargets->SetColumn(g_AisTargetList_sortColumn, item);
//     g_bRopelessTargetList_sortReverse = false;
//     g_RopelessTargetList_sortColumn = key;
//   }
//   // item.SetImage(g_bAisTargetList_sortReverse ? 1 : 0);

//   // if (!g_bAisTargetList_autosort) g_bsort_once = true;

//   //  if (g_RopelessTargetList_sortColumn >= 0) {
//   // m_pListCtrlAISTargets->SetColumn(g_AisTargetList_sortColumn, item);
//   RefreshTransponderList();
//   //  }
// }

// void RopelessDialog::RefreshTransponderList() {
  
//   std::vector<long> selectedIndices;
//   long item = -1;
//   while ((item = m_pListCtrlTranponders->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
//       selectedIndices.push_back(item);
//   }

//   m_pListCtrlTranponders->Freeze();
//   m_pListCtrlTranponders->DeleteAllItems();

//   //  Walk the vector of transponder status
//   for (unsigned int i = 0; i < transponderStatus.size(); i++) {
//     transponder_state *state = transponderStatus[i];

//     wxListItem item;
//     item.SetId(i);
//     // long result = m_pListCtrlTranponders->InsertItem(item);
//     long result = m_pListCtrlTranponders->InsertItem(i, " ");

//     m_pListCtrlTranponders->SetItemData(result, (long)i);

//     item.SetColumn(tlICON);
//     m_pListCtrlTranponders->SetItemImage(item, state->color_index);

//     // item.SetColumn(tlIDENT);
//     wxString sid;
//     sid.Printf("%d", state->ident);
//     // item.SetText(sid);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlIDENT, sid);
//     m_pListCtrlTranponders->SetColumnWidth(tlIDENT, wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlRELEASE_STATUS);
//     int rlsNum;
//     wxString appendStr = "";
//     wxString rid;
//     if (state->release_status == -4)
//     {
//       rlsNum = eRELEASE_NETWORK_ERR;
//     }
//     if (state->release_status == -3)
//     {
//       rlsNum = eRELEASE_TIMEOUT;
//     }
//     else if (state->release_status == -2)
//     {
//       rlsNum = eRELEASE_NOT_INIT;
//     }
//     else if (state->release_status == -1)
//     {
//       rlsNum = eRELEASE_NOT_VERIFIED;
//     }
//     else if (state->release_status == 0)
//     {
//       rlsNum = eRELEASE_VERIFIED;
//     }
//     else if (state->release_status > 0)
//     {
//       rlsNum = eRELEASE_SENDING;
//       appendStr.Printf("%d",state->release_status);
//     }
//     else
//     {
//       state->release_status = -2;
//       rlsNum = eRELEASE_NOT_INIT;
//     }

//     rid.Printf("%s%s", releaseStatusNames[rlsNum],appendStr);

//     // wxListItem testItem;
//     // testItem.SetId(i);
//     // testItem.SetColumn(1);
//     // testItem.SetBackgroundColour(*wxGREEN);
//     // m_pListCtrlTranponders->SetItem(testItem);

//     m_pListCtrlTranponders->SetItem(result, tlRELEASE_STATUS, rid);
//     m_pListCtrlTranponders->SetColumnWidth(tlRELEASE_STATUS,
//                                            wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlTIMESTAMP);
//     wxString sts;
//     // wxDateTime ts = DaysTowDT(state->timeStamp);
//     wxDateTime ts((time_t)(state->timeStamp));
//     ts.MakeUTC();
//     sts = ts.FormatISOCombined(' ');
//     m_pListCtrlTranponders->SetItem(result, tlTIMESTAMP, sts);
//     m_pListCtrlTranponders->SetColumnWidth(tlTIMESTAMP,
//                                            wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlDEPTH);
//     wxString sdp;
//     sdp.Printf("%g", state->depth);
//     // item.SetText(sdp);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlDEPTH, sdp);
//     m_pListCtrlTranponders->SetColumnWidth(tlDEPTH, wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlTEMP);
//     wxString stemp;
//     stemp.Printf("%g", state->temp);
//     // item.SetText(stemp);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlTEMP, stemp);
//     m_pListCtrlTranponders->SetColumnWidth(tlTEMP, wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlPINGS);
//     wxString sping;
//     sping.Printf("%d", state->pings);
//     // item.SetText(sping);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlPINGS, sping);
//     m_pListCtrlTranponders->SetColumnWidth(tlPINGS, wxLIST_AUTOSIZE_USEHEADER);

// #ifdef SHOW_DISTANCE
//     // item.SetColumn(tlDISTANCE);
//     wxString sdist;
//     sdist = wxString::Format(wxT("%.*f"), 2, state->distance);
//     // wxString sdist;
//     // sdist.Printf("%g", state->distance);
//     // item.SetText(sdist);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlDISTANCE, sdist);
//     m_pListCtrlTranponders->SetColumnWidth(tlDISTANCE,
//                                            wxLIST_AUTOSIZE_USEHEADER);
// #endif
    
//     // item.SetColumn(tlRECOVERED);
//     wxString srec;
//     srec.Printf("%s", recoveredStrList[state->recovered_state]);
//     // item.SetText(sdist);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlRECOVERED, srec);
//     m_pListCtrlTranponders->SetColumnWidth(tlRECOVERED,
//                                            wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlRANGE);
//     wxString srng;
//     srng.Printf("%g", state->range);
//     // item.SetText(sdist);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlRANGE, srng);
//     m_pListCtrlTranponders->SetColumnWidth(tlRANGE,
//                                            wxLIST_AUTOSIZE_USEHEADER);

//     // item.SetColumn(tlBATT_STAT);
//     wxString sbatt;
//     sbatt.Printf("%d", state->batt_stat);
//     // item.SetText(sdist);
//     // m_pListCtrlTranponders->SetItem(item);
//     m_pListCtrlTranponders->SetItem(result, tlBATT_STAT, sbatt);
//     m_pListCtrlTranponders->SetColumnWidth(tlBATT_STAT,
//                                            wxLIST_AUTOSIZE_USEHEADER);
//   }

//   if (g_RopelessTargetList_sortColumn > 0)
//     m_pListCtrlTranponders->SortItems(
//         wxListCompareFunction, reinterpret_cast<wxIntPtr>(&transponderStatus));

//   for (auto index : selectedIndices) {
//       if (index < m_pListCtrlTranponders->GetItemCount()) {
//           m_pListCtrlTranponders->SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
//       }
//   }
  
//   m_pListCtrlTranponders->Thaw();

// #ifdef __WXMSW__
//   m_pListCtrlTranponders->Refresh(false);
// #endif

// }

// void RopelessDialog::OnChooseFileButton(wxCommandEvent &event) {
//   wxString file;
//   int response = PlatformFileSelectorDialog(
//       NULL, &file, _("Select an NMEA text file"),
//       *GetpPrivateApplicationDataLocation(), _T(""), _T("*.*"));

//   if (response == wxID_OK) {
//     if (::wxFileExists(file)) {
//       msgFileName = file;
//       m_simTextCtrl->SetValue(msgFileName);
//     }
//   }
// }

// void RopelessDialog::OnStopSimButton(wxCommandEvent &event) {
//   SetCanvasContextMenuItemViz(pParentPi->m_start_sim_id, true);
//   SetCanvasContextMenuItemViz(pParentPi->m_stop_sim_id, false);

//   m_StopSimButton->Hide();
//   m_StartSimButton->Show();

//   pParentPi->stopSim();
//   Layout();
// }

// void RopelessDialog::OnStartSimButton(wxCommandEvent &event) {
//   wxLogMessage("OnStartSimButton!");

//   if (::wxFileExists(msgFileName)) {
//     SetCanvasContextMenuItemViz(pParentPi->m_start_sim_id, false);
//     SetCanvasContextMenuItemViz(pParentPi->m_stop_sim_id, true);
//     m_StartSimButton->Hide();
//     m_StopSimButton->Show();
//     pParentPi->startSim();
//     Layout();
//   }
// }

// void RopelessDialog::OnManualReleaseButton(wxCommandEvent &event) {

//   wxString msg("Manually Enter Transponder ID to Release: ");

//   long result = -1;
//   myNumberEntryDialog dialog;

// #ifdef __ANDROID__
//   wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
//   dialog.SetFont(*pFont);
// #endif

//   dialog.Create(GetOCPNCanvasWindow(), msg, "Enter Transponder ID",
//                 "Ropeless Plugin Message", 0, 0, 100000, wxDefaultPosition);

//   if (dialog.ShowModal() == wxID_OK) {
//     result = dialog.GetValue();
//   }

//   if (result >= 0) {
//     wxString s1;
//     s1.Printf("Manual Release Req for ID: %d", result);
//     wxLogMessage(s1);

//     g_ropelessPI->manualReleaseState.ident = result;
//     g_ropelessPI->SendCommandMessage(&g_ropelessPI->manualReleaseState, eCMD_RELEASE);

//   }
// }

// void RopelessDialog::OnSyncButton(wxCommandEvent &event)
// {
//   g_ropelessPI->SendSyncMessage();
// }


// void RopelessDialog::clearHighlighted() {
//   wxArrayInt selectedItems = GetSelectedItems();
//   int numItems = selectedItems.GetCount();

//   // for (size_t i = 0; i < numItems; i++) {
//   //     message += wxString::Format("%d ", selectedItems[i]);
//   // }

//   if (numItems > 0) {
//     transponder_state *state = getXpdrFromIndex(selectedItems[0]);

//     if (state->ident > 0) {
//       state->color_index = COLOR_INDEX_GREEN;

//     } else {
//       state->color_index = COLOR_INDEX_RED;
//     }
//   }
// }

// long RopelessDialog::FindItemByName(wxListCtrl* listCtrl, const wxString& name) {
//   long itemIndex = -1;
//   while ((itemIndex = listCtrl->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE)) != wxNOT_FOUND) {
//       wxLogMessage("Item at index %ld: %s", itemIndex, listCtrl->GetItemText(itemIndex));;
//       if (listCtrl->GetItemText(itemIndex) == name) {
//           return itemIndex;
//       }
//   }
//   return wxNOT_FOUND; // Return -1 if the item is not found
// }

// void RopelessDialog::OnClose(wxCloseEvent &event) {
//   clearHighlighted();

// #ifndef __ANDROID__
//   wxPoint p = GetPosition();
//   pParentPi->m_dialogPosX = p.x;
//   pParentPi->m_dialogPosY = p.y;
//   wxSize s = GetSize();
//   pParentPi->m_dialogSizeWidth = s.x;
//   pParentPi->m_dialogSizeHeight = s.y;
// #endif
//   // wxLogMessage("Closing Ropeless window [x]...");
//   // event.Skip();
//   Destroy();
//   pParentPi->m_pRLDialog = NULL;
// }

// void RopelessDialog::OnOKClick(wxCommandEvent &event) {
//   clearHighlighted();

// #ifndef __ANDROID__
//   m_StopSimButton->Hide();
//   m_StartSimButton->Show();
//   g_ropelessPI->stopSim();
// #endif
//   Close();
// }