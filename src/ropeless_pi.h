/******************************************************************************
 * $Id:
 *
 * Project:  OpenCPN
 * Purpose:
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

#ifndef _ROPELESSPI_H_
#define _ROPELESSPI_H_

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif  // precompiled headers

#define     PLUGIN_VERSION_MAJOR    3
#define     PLUGIN_VERSION_MINOR    0

#define MY_API_VERSION_MAJOR 1
#define MY_API_VERSION_MINOR 12

#include <vector>

#include "graphics.h"

#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/spinctrl.h>
#include <wx/aui/aui.h>
#include <wx/fontpicker.h>
#include <wx/socket.h>

#include "ocpn_plugin.h"
#include "ODdc.h"
#include "pugixml.hpp"

#include "nmea0183/nmea0183.h"
#include "PI_RolloverWin.h"
#include "TexFont.h"
#include "vector2d.h"
#include "OCPN_DataStreamEvent.h"
#include "NMEA_TCP_OutputConnection.h"


#define EPL_TOOL_POSITION -1  // Request default positioning of toolbar tool

#define gps_watchdog_timeout_ticks 10

#define UDP_PORT 59647
#define RELEASE_TIME_MS 5000

//    Constants
#ifndef PI
#define PI 3.1415926535897931160E0 /* pi */
#endif

#define SEL_POINT_A 0
#define SEL_POINT_B 1
#define SEL_SEG 2

//      Menu items
#define ID_EPL_DELETE 8867
#define ID_EPL_XMIT 8868
#define ID_TPR_RELEASE 8869
#define ID_TPR_DELETE 8870
#define ID_TPR_ID 8871
#define ID_TPR_MANUAL_RELEASE 8872
#define ID_TPR_PLACE 8873
#define ID_TPR_RECOVER 8874
#define ID_TPR_EDIT 8875

//      Message IDs
#define SIM_TIMER 5003
#define RELEASE_TIMER 5004
#define DISTANCE_TIMER 5005
#define ID_PLAY_SIM 5058
#define ID_STOP_SIM 5059
#define ID_TRANSPONDER_LIST 5060

//      Options
#define COLOR_TABLE_COUNT 5
#define COLOR_INDEX_GOLDEN 4
#define COLOR_INDEX_GREEN 1
#define COLOR_INDEX_RED 0

#define SET_RECOVERED_OPACITY
#define SHOW_DISTANCE

enum {
  eMARK_TYPE_ACOUSTIC = 0,
  eMARK_TYPE_TIMER,
  eMARK_TYPE_GALVANIC,
  eMARK_TYPE_OTHER,
  eMARK_TYPE_SURFACE_FIXED,
  eMARK_TYPE_SURFACE_DRIFT
};

enum {
  tlICON = 0,
  tlIDENT,
  tlRELEASE_STATUS,
  tlTIMESTAMP,
  tlDISTANCE,
  tlRECOVERED
  // Removed columns: tlPINGS, tlDEPTH, tlTEMP, tlBATT_STAT, tlRANGE
};  // Transponder list Columns;

enum {
  eRELEASE_TIMEOUT = 0,
  eRELEASE_SENDING = 1,
  eRELEASE_VERIFIED = 2,
  eRELEASE_NOT_VERIFIED = 3,
  eRELEASE_FAILED = 4,
  eRELEASE_NOT_INIT = 5,
  eRELEASE_NETWORK_ERR = 6,
  eRELEASE_CONNECTING = 7,
};

enum {
  eREC_DEPLOYED = 0,
  eREC_RECOVERED = 1,
  eREC_LOST = 2,
};

enum {
  ePOS_SOURCE_USER = 0,
  ePOS_SOURCE_CLOUD = 1,
  ePOS_SOURCE_ACOUSTIC = 2,
  ePOS_SOURCE_GPS = 3,
};

enum {
  eCOMM_UDP = 0,
};

enum {
  eCMD_RESPONSE = 0,
  eCMD_DELETE = 1,
  eCMD_RECOVER = 2,
  eCMD_LOST = 3,
  eCMD_MUTE = 4,
  eCMD_SYNC = 5,
  eCMD_RANGE = 6,
  eCMD_INTERROGATE = 7,
  eCMD_RELEASE = 8
};

const wxString releaseStatusNames[] = {"TIMEOUT", "SENDING...", "RELEASED", "NOT VERIFIED", "FAILED", "---", "NETWORK ERROR", "CONNECTING..."};
const wxString recoveredStrList[] = {"DEPLOYED","RECOVERED"};
const wxString positionSourceNames[] = {"USER", "CLOUD", "ACOUSTIC", "GPS"};
const wxString markTypeNames[] = {"Acoustic", "Timer", "Galvanic", "Other", "Surface Fixed", "Surface Drift"};

// Manufacturer lookup table structure
struct ManufacturerInfo {
    uint8_t code;
    const char* name;
    const char* displayName;  // For dropdowns with hex code
};

// Centralized manufacturer lookup table
const ManufacturerInfo manufacturerTable[] = {
    {0x00, "Ropeless Systems Inc.", "0x00 - Ropeless Systems Inc."},
    {0x01, "Desert Star Systems", "0x01 - Desert Star Systems"},
    {0x02, "EdgeTech", "0x02 - EdgeTech"},
    {0x03, "Benthos", "0x03 - Benthos"},
    {0x04, "SubSea Sonics", "0x04 - SubSea Sonics"},
    {0x05, "Teledyne Marine", "0x05 - Teledyne Marine"},
    {0x10, "Ashored Innovations", "0x10 - Ashored Innovations"},
    {0xFF, "Ephemeral", "0xFF - Ephemeral"}
};

const int manufacturerTableSize = sizeof(manufacturerTable) / sizeof(ManufacturerInfo);

//----------------------------------------------------------------------------------------------------------
//    Manufacturer ID Utility Functions
//----------------------------------------------------------------------------------------------------------

// Create 32-bit transponder ID from manufacturer code and serial number
inline uint32_t createTransponderID(uint8_t mfg_code, uint32_t serial_number) {
    return (static_cast<uint32_t>(mfg_code) << 24) | (serial_number & 0xFFFFFF);
}

// Extract manufacturer code from 32-bit transponder ID
inline uint8_t getMfgCode(uint32_t transponder_id) {
    return static_cast<uint8_t>(transponder_id >> 24);
}

// Extract serial number from 32-bit transponder ID
inline uint32_t getSerialNumber(uint32_t transponder_id) {
    return transponder_id & 0xFFFFFF;
}

// Get manufacturer string name from manufacturer code
wxString getMfgString(uint8_t mfg_code);

// static int wxCALLBACK wxListCompareFunction(wxIntPtr item1, wxIntPtr item2,
//                                             wxIntPtr sortData);

//----------------------------------------------------------------------------------------------------------
//    Forward declarations
//----------------------------------------------------------------------------------------------------------
class Select;
class SelectItem;
class PI_EventHandler;
class PI_OCP_DataStreamInput_Thread;
class RopelessDialog;
class RopelessPrefsDialog;
class OCPNListCtrl;

WX_DECLARE_OBJARRAY(vector2D *, ArrayOf2DPoints);

// void AlphaBlending( wxDC *pdc, int x, int y, int size_x, int size_y, float
// radius, wxColour color,
//                     unsigned char transparency );
// void RenderLine(int x1, int y1, int x2, int y2, wxColour color, int width);
// void GLDrawLine( wxCoord x1, wxCoord y1, wxCoord x2, wxCoord y2 );
// void RenderGLText( wxString &msg, wxFont *font, int xp, int yp, double
// angle);


class transponder_state {
public:
  transponder_state() {
    markID = 0;
    timeStamp = 0;
    
    release_status = -2;
    bearing = 0;
    depth = 0;
    timeStamp = 0;
    batt_stat = 0;
    recovered_state = eREC_DEPLOYED;
    distance = 0;
    pings = 0;
    position_source = ePOS_SOURCE_USER;

    // Initialize GML parameters
    mark_type = 0;
    pos_status = 0;
    trawl_id = 0;
    trawl_num = 0;
    mfg_id = 0;
    mfg_code = 0;
    serial_num = 0;
    mfg_str = "";
    ownership = 0;
    
    // Initialize GMS parameters  
    surface_range = 0;
    slant_range = 0;
    tilt = 0;
    seafloor_temp = 0;
    air_pressure = 0;
    gms_date_num = 0.0;
    
    // Initialize lifecycle timestamps
    initial_deployment_utc = 0.0;
    recovered_utc = 0.0;
    released_utc = 0.0;
    
    // Initialize trawl relationship fields
    assigned_trawl_id = 0;
    is_trawl_start_end = false;
    trawl_position = -1;

    predicted_lat = 999.0;
    predicted_lon = 999.0;
    hide_pos = false;
    selected = false;

  }

  ~transponder_state() {};
  
  // Unique Identifier
  uint32_t markID;      // Full 32-bit transponder ID (MarkID from GML)

  double timeStamp;

  // Parsed
  uint32_t mfg_id;      // Full 32-bit manufacturer ID (derived from MarkID)
  uint8_t mfg_code;     // 8-bit manufacturer code (extracted from mfg_id)
  uint32_t serial_num;  // 24-bit serial number (extracted from mfg_id)
  wxString mfg_str;     // Manufacturer string name (derived from mfg_code)

  // Calculated
  double distance;      // UI distance vessel to Transponder lat/lon
  int pings;            // Number of GML messages received from transponder

  // Plugin Status
  bool hide_pos;
  int recovered_state;
  int opacity;
  bool selected;

  // GML (Gear Mark Location) status parameters
  int mark_type;        // MarkType from GML
  int pos_status;       // PosStatus from GML
  uint16_t trawl_id;    // TrawlID from GML (16-bit: 0-65535)
  int trawl_num;        // TrawlNum from GML
  double predicted_lat; // current pos lat
  double predicted_lon; // current pos lon
  double depth;         // Depth m GML
  int ownership;        // Ownership from GML
  int position_source;  // Source from GML
  double gml_timestamp; // timeStamp from GML
  
  // GMS (Gear Mark Status) parameters
  int release_status;   // Release status from GMS
  int batt_stat;        // Battery from GMS
  int surface_range;    // SurfaceRange from GMS
  int slant_range;      // SlantRange from GMS  
  double bearing;       // Bearing from GMS
  int tilt;             // Tilt from GMS
  int seafloor_temp;    // SeafloorTemp from GMS
  int air_pressure;     // AirPressure from GMS
  double gms_date_num;  // DateNum from GMS
  
  // Lifecycle UTC timestamps
  double initial_deployment_utc;  // UTC time of initial deployment
  double recovered_utc;           // UTC time when marked as recovered
  double released_utc;            // UTC time when released
  
  // Trawl relationship fields
  uint16_t assigned_trawl_id;     // ID of trawl this transponder belongs to (0 = not in trawl, range: 0-65535)
  bool is_trawl_start_end;        // Is this a start/end marker transponder
  int trawl_position;             // Order position in trawl (0=start, 1=second, etc.)
  
};

// Coordinate structure for trawl path
struct trawl_coordinate {
  double lat;
  double lon;
  
  trawl_coordinate() : lat(0.0), lon(0.0) {}
  trawl_coordinate(double latitude, double longitude) : lat(latitude), lon(longitude) {}
};

// Forward declaration for access to global transponder list
extern class ropeless_pi *g_ropelessPI;

// Trawl tracking class using ID-based approach
class trawl_tracker {
public:
  trawl_tracker() {
    trawl_id = 0;
    devices_in_set = 0;
  }
  
  ~trawl_tracker() {}
  
  // Core fields
  uint16_t trawl_id;                                   // Unique trawl identifier (16-bit: 0-65535)
  int devices_in_set;                                  // Number of devices in this trawl set (for reference)
  std::vector<trawl_coordinate> trawl_path;           // Array of lat/lon coordinates defining trawl path
  
  // ID-based transponder tracking methods
  std::vector<transponder_state*> getTransponders() const;
  std::vector<uint32_t> getTransponderIds() const;
  transponder_state* getStartEndTransponder() const;
  std::vector<transponder_state*> getOrderedTransponders() const;
  
  // Transponder management
  bool addTransponder(uint32_t transponder_id, int position = -1);
  bool removeTransponder(uint32_t transponder_id);
  void setStartEnd(uint32_t transponder_id);
  void reorderTransponders();
  
  // Query methods
  int traps_in_set() const;
  bool hasTransponder(uint32_t transponder_id) const;
  bool isStartEnd(uint32_t transponder_id) const;
  int getTransponderPosition(uint32_t transponder_id) const;
  
  // Path management
  void addPathPoint(double lat, double lon);
  void clearPath();
};

struct deckbox_status {
public:
  deckbox_status() {
    deckboxID.Clear();
    deckboxManuf.Clear();  
    acousticStatus.Clear();
    cloudStatus.Clear();
    numDevices = 0;
    connectionStatus = false;
    lastUpdateTime = wxDateTime::Now();
  }
  
  wxString deckboxID;
  wxString deckboxManuf;
  wxString acousticStatus;
  wxString cloudStatus;
  int numDevices;
  bool connectionStatus;
  wxDateTime lastUpdateTime;
  
  void UpdateFromDBS(const DBS& dbs) {
    deckboxID = dbs.DeckboxID;
    deckboxManuf = dbs.DeckboxManuf;
    acousticStatus = dbs.AcousticStatus;
    cloudStatus = dbs.CloudStatus;
    numDevices = dbs.NumDevices;
    lastUpdateTime = wxDateTime::Now();
  }
};

class release_timer_state{
public:

  release_timer_state() {
    timer_state = 0;
    ptstate = NULL;
  }

  ~release_timer_state() {};

  int timer_state;
  transponder_state* ptstate;

};

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class ropeless_pi : public wxTimer, opencpn_plugin_112 {
public:
  ropeless_pi(void *ppimgr);
  virtual ~ropeless_pi(void);

  //    The required PlugIn Methods
  int Init(void);
  bool DeInit(void);

  int GetAPIVersionMajor();
  int GetAPIVersionMinor();
  int GetPlugInVersionMajor();
  int GetPlugInVersionMinor();
  wxBitmap *GetPlugInBitmap();
  wxString GetCommonName();
  wxString GetShortDescription();
  wxString GetLongDescription();

  //    The optional method overrides
  void SetNMEASentence(wxString &sentence);
  void SetPositionFix(PlugIn_Position_Fix &pfix);
  void SetCursorLatLon(double lat, double lon);
  int GetToolbarToolCount(void);
  void OnToolbarToolCallback(int id);
  //      void ShowPreferencesDialog( wxWindow* parent );
  void SetColorScheme(PI_ColorScheme cs);
  void OnContextMenuItemCallback(int id);

  bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp);
  bool RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp);
  bool MouseEventHook(wxMouseEvent &event);
  bool KeyboardEventHook(wxKeyEvent &event);

  //      void OnRolloverPopupTimerEvent( wxTimerEvent& event );
  void PopupMenuHandler(wxCommandEvent &event);

  bool SaveConfig(void);
  void PopulateContextMenu(wxMenu *menu);
  int GetToolbarItemId() { return m_toolbar_item_id; }
  void SetPluginMessage(wxString &message_id, wxString &message_body);

  void ProcessTimerEvent(wxTimerEvent &event);
  void ProcessSimTimerEvent(wxTimerEvent &event);
  void ProcessReleaseTimerEvent(wxTimerEvent &event);
  void ProcessDistanceTimerEvent(wxTimerEvent &event);

  //      void RenderFixHat( void );
  void ShowPreferencesDialog(wxWindow *parent);

  //    Secondary thread life toggle
  //    Used to inform launching object (this) to determine if the thread can
  //    be safely called or polled, e.g. wxThread->Destroy();
  void SetSecThreadActive(void) { m_bsec_thread_active = true; }
  void SetSecThreadInActive(void) { m_bsec_thread_active = false; }
  bool IsSecThreadActive() { return m_bsec_thread_active; }
  bool m_bsec_thread_active;
  int m_Thread_run_flag;

  void startSim();
  void stopSim();

  void startReleaseTimer();
  void stopReleaseTimer();
  void updateReleaseTimer(transponder_state * state);
  void toggleTransponderRecovered(uint32_t markID);
  void updateReleaseDialog(bool show);

  void startDistanceTimer();
  void stopDistanceTimer();

  int m_dialogSizeWidth;
  int m_dialogSizeHeight;
  int m_dialogPosX;
  int m_dialogPosY;
  
  // TCP NMEA Output Configuration
  wxString m_tcp_host;
  int m_tcp_port;
  bool m_tcp_enabled;
  bool m_tcp_auto_reconnect;
  
  // Display settings
  bool m_colorblind_mode;
  int m_transponder_circle_size;
  int m_transponder_text_size;
  
  // Debug settings
  bool m_debug_enabled;
  bool m_debug_show_nmea;
  bool m_debug_show_log;
  bool m_loopbackNMEATx;

  RopelessDialog *m_pRLDialog;
  RopelessPrefsDialog *m_pPrefsDialog;

  wxTimer m_simulatorTimer;
  int m_start_sim_id, m_stop_sim_id;

  transponder_state *m_foundState;
  bool SendCommandMessage(transponder_state *state, long code);
  void SendSyncMessage(void);
  wxString GetConnectionStatusText();
  wxString GetColorName(int color_index);
  
  // TCP NMEA Output Methods
  void InitializeTCPOutput();
  void ShutdownTCPOutput();
  bool IsTCPOutputConnected() const;
  bool SendNMEAMessageTCP(RESPONSE* message);
  bool SendRawNMEATCP(const wxString& nmea_sentence);
  void ConfigureTCPOutput(const wxString& host, int port, bool enabled, bool auto_reconnect);
  wxString GetTCPOutputStatus() const;
  
  // RSGML Message Generation
  void SendGMLMessageForManualPlacement(transponder_state* state, double lat, double lon, double utc);
  
  // Global Debug Message Function
  void GlobalDebugMessage(const wxString& message, bool alsoLog = true);

  int m_place_trap_manually;
  int m_place_trap_now;

  wxTimer m_releaseTimer;
  wxTimer m_distanceTimer;

  release_timer_state m_release_tim_state;

  transponder_state manualReleaseState;
  
  void releaseCallbackRecovered(void);
  void releaseCallbackRetry(void);
  void releaseCallbackExit(void);
  
  bool ConfirmAndDeleteTransponder(uint32_t markID);
  bool ConfirmAndReleaseTransponder(transponder_state* state);
  int getNextCloudId();
  transponder_state *GetStateByMarkID(uint32_t markID);

private:
  bool LoadConfig(void);
  void ApplyConfig(void);

  bool DeleteTransponder(uint32_t markID);

  void RenderTransponder(transponder_state *state);
  void RenderTransponders();
  void RenderTrawlConnectors();
  void RenderTransponderTexts();
  void RenderTrawlConnector(transponder_state *state1,
                            transponder_state *state2);
  void RenderVesselRangeCircle();

  void ProcessRFACapture(void);
  void ProcessRLACapture(void);
  transponder_state *addTransponderPos(uint32_t markID);
  void placeTransponderManually(uint32_t markID, uint32_t pairId, double lat, double lon,
                                double utc, int pos_source = ePOS_SOURCE_USER, int ownership = 1);
  void EditTransponder(transponder_state* state);

  void SaveTransponderStatus();
  void populateTransponderNode(pugi::xml_node &transponderNode,
                               transponder_state *state);
  void LoadTransponderStatus();
  void RegenerateTrawlListFromTransponders();
  bool parseTransponderNode(pugi::xml_node &transponderNode,
                            transponder_state *state);

  unsigned char ComputeChecksum(wxString msg);

  wxBitmap *m_pplugin_icon;
  wxFileConfig *m_pconfig;
  int m_toolbar_item_id;

  int m_show_id;
  int m_hide_id;

  NMEA0183 m_NMEA0183;     // Used to parse incoming NMEA Sentences
  NMEA0183 m_NMEA0183_tx;  // outgoing NMEA sentences

  // FFU
  int m_config_version;
  wxString m_VDO_accumulator;

  Select *m_select;

  //      ArrayOfBrgLines      m_brg_array;
  double m_fix_lat;
  double m_fix_lon;
  ArrayOf2DPoints m_hat_array;
  int m_nfix;
  bool m_bshow_fix_hat;

  //        Selection variables
  //     brg_line             *m_sel_brg;
  int m_sel_part;
  SelectItem *m_pFind;
  double m_sel_pt_lat;
  double m_sel_pt_lon;
  double m_segdrag_ref_x;
  double m_segdrag_ref_y;
  double m_cursor_lat;
  double m_cursor_lon;
  int m_mouse_x;
  int m_mouse_y;

  //        State variables captured from NMEA stream
  int mHDx_Watchdog;
  int mHDT_Watchdog;
  int mGPS_Watchdog;
  int mVar_Watchdog;
  double mVar;
  //      double               mHdm;
  double m_ownship_cog;
  bool m_head_active;
  wxTimer m_head_dog_timer;

  //   int                  mPriPosition;
  //   int                  mPriDateTime;
  //   int                  mPriVar;
  //   int                  mPriHeadingM;
  //   int                  mPriHeadingT;

  double m_ownship_lat;
  double m_ownship_lon;
  double m_hdt;
  wxDateTime mUTCDateTime;
  
  // Cloud ID assignment counter (starts at 32768)
  int m_nextCloudId;

  //        Rollover Window support
  RolloverWin *m_pBrgRolloverWin;
  wxTimer m_RolloverPopupTimer;
  int m_rollover_popup_timer_msec;

  //     wxColour              m_FixHatColor;

  PI_EventHandler *m_event_handler;
  PI_OCP_DataStreamInput_Thread *m_serialThread;
  wxString m_serialPort;

  unsigned int m_iconTexture;
  int m_texwidth, m_texheight;

  SelectItem *m_tenderSelect;
  wxString m_trackedWP;
  wxString m_trackedWPGUID;

  unsigned int m_colorIndexNext;

  ODDC *m_oDC;

  int m_leftclick_tool_id;

  wxWindow *m_parent_window;
  double m_selectRadius;

  wxSocketBase *m_tsock;
  wxIPV4address m_tconn_addr;
  
  // TCP NMEA Output Connection
  NMEA_TCP_OutputConnection *m_nmea_tcp_output;

  DECLARE_EVENT_TABLE();
};

// Global vectors for tracking objects (declared after class definitions)
extern std::vector<transponder_state *> transponderStatus;
extern std::vector<trawl_tracker *> trawlList;

// Global debug message function accessible from anywhere
void GlobalRopelessDebugMessage(const wxString& message, bool alsoLog = true);

//      An event handler to manage timer ticks, and the like
class PI_EventHandler : public wxEvtHandler {
public:
  PI_EventHandler(ropeless_pi *parent);
  ~PI_EventHandler();

  void OnTimerEvent(wxTimerEvent &event);
  void PopupMenuHandler(wxCommandEvent &event);
  void OnEvtOCPN_NMEA(PI_OCPN_DataStreamEvent &event);

private:
  ropeless_pi *m_parent;

  DECLARE_EVENT_TABLE();
};

typedef enum BearingTypeEnum { MAG_BRG = 0, TRUE_BRG } _BearingTypeEnum;

#endif
