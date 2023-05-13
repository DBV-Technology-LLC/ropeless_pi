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

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers
#include <typeinfo>
#include <wx/graphics.h>

#include "config.h"
#include "ropeless_pi.h"
#include "icons.h"
#include "Select.h"
#include "vector2d.h"
#include "dsPortType.h"
#include "OCP_DataStreamInput_Thread.h"
#include "OCPN_DataStreamEvent.h"
#include "georef.h"
#include "OCPNListCtrl.h"
#include "pugixml.hpp"

#include <arpa/inet.h>
#include <netinet/tcp.h>

#ifdef ocpnUSE_GL
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif

//wxEventType wxEVT_PI_OCPN_DATASTREAM;// = wxNewEventType();

#ifdef __WXMSW__
wxEventType wxEVT_PI_OCPN_DATASTREAM = wxNewEventType();
#else
wxEventType wxEVT_PI_OCPN_DATASTREAM;// = wxNewEventType();
#endif

//      Global Static variable
PlugIn_ViewPort         *g_vp;
PlugIn_ViewPort         g_ovp;

double                   g_Var;         // assummed or calculated variation

NMEA0183             g_NMEA0183;                 // Used to parse NMEA Sentences
double gLat, gLon, gSog, gCog, gHdt, gHdm, gVar;
bool                    ll_valid;
bool                    pos_valid;

#if wxUSE_GRAPHICS_CONTEXT
wxGraphicsContext       *g_gdc;
#endif
wxDC                    *g_pdc;

wxArrayString           g_iconTypeArray;
int                     gHDT_Watchdog;
int                     gGPS_Watchdog;

ropeless_pi             *g_ropelessPI;

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(ArrayOf2DPoints);

#include "default_pi.xpm"

wxString colorTableNames[] = {
  "MAGENTA",
  "CYAN",
  "LIME GREEN",
  "ORANGE",
  "MEDIUM GOLDENROD"
};
#define COLOR_TABLE_COUNT 5

wxString msgFileName = "/home/dsr/RFA01132022.txt"; //"/home/dsr/Projects/ropeless_pi/testset1.txt";

wxDateTime DaysTowDT( double days )
{
  int daysRoundDown = (int)days;
  int daysLinuxEpoch = daysRoundDown - 719528;

  double fraction = days - daysRoundDown;
  double fraction_secs = fraction * 24 * 3600;
  time_t epochTime = (int)(daysLinuxEpoch * 24 * 3600) + (int)fraction_secs;
  return wxDateTime(epochTime);
}


// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi( void *ppimgr )
{
    return (opencpn_plugin *) new ropeless_pi( ppimgr );
}

extern "C" DECL_EXP void destroy_pi( opencpn_plugin* p )
{
    delete p;
}

wxString getWaypointName( wxString &GUID )
{
    PlugIn_Waypoint pwp;
    if(GetSingleWaypoint( GUID, &pwp ))
        return pwp.m_MarkName;
    else
        return _T("");
}


/*  These two function were taken from gpxdocument.cpp */
int GetRandomNumber(int range_min, int range_max)
{
      long u = (long)wxRound(((double)rand() / ((double)(RAND_MAX) + 1) * (range_max - range_min)) + range_min);
      return (int)u;
}

// RFC4122 version 4 compliant random UUIDs generator.
wxString GetUUID(void)
{
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

      uuid.time_low = GetRandomNumber(0, 2147483647);//FIXME: the max should be set to something like MAXINT32, but it doesn't compile un gcc...
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

      str.Printf(_T("%08x-%04x-%04x-%02x%02x-%04x%08x"),
      uuid.time_low,
      uuid.time_mid,
      uuid.time_hi_and_version,
      uuid.clock_seq_hi_and_rsv,
      uuid.clock_seq_low,
      uuid.node_hi,
      uuid.node_low);

      return str;
}

void Clone_VP(PlugIn_ViewPort *dest, PlugIn_ViewPort *src)
{
    dest->clat =                   src->clat;                   // center point
    dest->clon =                   src->clon;
    dest->view_scale_ppm =         src->view_scale_ppm;
    dest->skew =                   src->skew;
    dest->rotation =               src->rotation;
    dest->chart_scale =            src->chart_scale;
    dest->pix_width =              src->pix_width;
    dest->pix_height =             src->pix_height;
    dest->rv_rect =                src->rv_rect;
    dest->b_quilt =                src->b_quilt;
    dest->m_projection_type =      src->m_projection_type;

    dest->lat_min =                src->lat_min;
    dest->lat_max =                src->lat_max;
    dest->lon_min =                src->lon_min;
    dest->lon_max =                src->lon_max;

    dest->bValid =                 src->bValid;                 // This VP is valid
}
//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

//      Event Handler implementation
BEGIN_EVENT_TABLE ( ropeless_pi, wxEvtHandler )
EVT_TIMER ( TIMER_THIS_PI, ropeless_pi::ProcessTimerEvent )
EVT_TIMER ( SIM_TIMER, ropeless_pi::ProcessSimTimerEvent )
END_EVENT_TABLE()

ropeless_pi::ropeless_pi( void *ppimgr ) :
        wxTimer( this ), opencpn_plugin_112( ppimgr )
{
    g_ropelessPI = this;
    m_pplugin_icon = new wxBitmap(default_pi);

}

ropeless_pi::~ropeless_pi( void )
{
}

int ropeless_pi::Init( void )
{
    AddLocaleCatalog( _T("opencpn-ropeless_pi") );
    m_config_version = -1;

    m_oDC = NULL;

    //  Configure the NMEA processor
    mHDx_Watchdog = 2;
    mHDT_Watchdog = 2;
    mGPS_Watchdog = 2;
    mVar_Watchdog = 2;
    mPriPosition = 9;
    mPriDateTime = 9;
    mPriVar = 9;
    mPriHeadingM = 9;
    mPriHeadingT = 9;

    gHDT_Watchdog = 10;
    gGPS_Watchdog = 10;

    mVar = 0;
    m_hdt = 0;
    m_ownship_cog = 0;
    m_nfix = 0;
    m_bshow_fix_hat = false;

    m_FixHatColor = wxColour(0, 128, 128);
    m_Thread_run_flag = -1;

    g_iconTypeArray.Add(_T("Scaled Vector Icon"));
    g_iconTypeArray.Add(_T("Generic Ship Icon"));

    m_tenderGPS_y = 37.1;               // From bow
    m_tenderGPS_x  = 2.6;               // stbd from midships
    m_tenderLength = 41.1;
    m_tenderWidth = 10.7;

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

    //    And load the configuration items
    LoadConfig();

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
    Start( 1000, wxTIMER_CONTINUOUS );

    m_event_handler = new PI_EventHandler(this);
    m_tsock = NULL;

#if 0
#ifndef __WXMSW__
    wxEVT_PI_OCPN_DATASTREAM = wxNewEventType();
#endif

    m_event_handler = new PI_EventHandler(this);
    m_serialThread = NULL;

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

    LoadTransponderStatus();    //Load persistant XML file

    wxMenuItem *sim_item = new wxMenuItem(NULL, ID_PLAY_SIM, _("Start Ropeless Simulation") );
    m_start_sim_id = AddCanvasContextMenuItem(sim_item, this );
    SetCanvasContextMenuItemViz(m_start_sim_id, true);

    wxMenuItem *sim_stop_item = new wxMenuItem(NULL, ID_STOP_SIM, _("Stop Simulation") );
    m_stop_sim_id = AddCanvasContextMenuItem(sim_stop_item, this );
    SetCanvasContextMenuItemViz(m_stop_sim_id, false);

    //    This PlugIn needs a toolbar icon, so request its insertion
    m_leftclick_tool_id =
    InsertPlugInTool(_T(""), _img_plus, _img_plus, wxITEM_NORMAL, _("Ropeless"),
                         _T(""), NULL, -1, 0, this);


    return (WANTS_OVERLAY_CALLBACK |
            WANTS_OPENGL_OVERLAY_CALLBACK |
            WANTS_CURSOR_LATLON       |
            WANTS_TOOLBAR_CALLBACK    |
            INSTALLS_TOOLBAR_TOOL     |
            WANTS_CONFIG              |
            WANTS_PREFERENCES         |
            WANTS_PLUGIN_MESSAGING    |
            WANTS_NMEA_SENTENCES      |
            WANTS_NMEA_EVENTS         |
            WANTS_PREFERENCES         |
            WANTS_MOUSE_EVENTS        |
            INSTALLS_CONTEXTMENU_ITEMS
            );

}

bool ropeless_pi::DeInit( void )
{
    if( IsRunning() ) // Timer started?
        Stop(); // Stop timer

//    delete m_event_handler;             // also diconnects serial events

//     int tsec = 2;
//     while(tsec--)
//         wxSleep(1);

    RemovePlugInTool(m_leftclick_tool_id);

    // Persist control dialog size/position
    if (m_pRLDialog){
        wxPoint p = m_pRLDialog->GetPosition();
        m_dialogPosX = p.x;
        m_dialogPosY = p.y;
        wxSize s = m_pRLDialog->GetSize();
        m_dialogSizeWidth = s.x;
        m_dialogSizeHeight =s.y;
    }

    SaveConfig();

    SaveTransponderStatus();      // Create XML file

    return true;
}


int ropeless_pi::GetAPIVersionMajor()
{
    return MY_API_VERSION_MAJOR;
}

int ropeless_pi::GetAPIVersionMinor()
{
    return MY_API_VERSION_MINOR;
}

int ropeless_pi::GetPlugInVersionMajor()
{
    return PLUGIN_VERSION_MAJOR;
}

int ropeless_pi::GetPlugInVersionMinor()
{
    return PLUGIN_VERSION_MINOR;
}

wxBitmap *ropeless_pi::GetPlugInBitmap()
{
    return m_pplugin_icon;
}

wxString ropeless_pi::GetCommonName()
{
    return _("Ropeless");
}

wxString ropeless_pi::GetShortDescription()
{
    return _("Ropeless PlugIn for OpenCPN");
}

wxString ropeless_pi::GetLongDescription()
{
    return _("Ropeless PlugIn for OpenCPN");

}

void ropeless_pi::OnToolbarToolCallback(int id) {
  //if (!m_buseable) return;
  if (NULL == m_pRLDialog) {
    m_pRLDialog = new RopelessDialog(m_parent_window, this);
    wxFont *pFont = OCPNGetFont(_T("Dialog"), 0);
    m_pRLDialog->SetFont(*pFont);

  }

  //RearrangeWindow();
  /*m_pRLDialog->SetMaxSize(m_pRLDialog->GetSize());
  m_pRLDialog->SetMinSize(m_pRLDialog->GetSize());*/
  m_pRLDialog->Show(!m_pRLDialog->IsShown());
  m_pRLDialog->Layout();  // Some platforms need a re-Layout at this point
                           // (gtk, at least)

  if ((m_dialogSizeWidth > 0) && (m_dialogSizeHeight > 0))
      m_pRLDialog->SetSize(wxSize(m_dialogSizeWidth, m_dialogSizeHeight));

  if ((m_dialogPosX > 0) && (m_dialogPosY > 0))
      m_pRLDialog->Move(wxPoint(m_dialogPosX, m_dialogPosY));


//  wxPoint p = m_pRLDialog->GetPosition();
//  m_pRLDialog->Move(0, 0);  // workaround for gtk autocentre dialog behavior
//  m_pRLDialog->Move(p);

#ifdef __OCPN__ANDROID__
  m_pRLDialog->CentreOnScreen();
  m_pRLDialog->Move(-1, 0);
#endif

  m_pRLDialog->RefreshTransponderList();    //Pick up initial XML load
}


void ropeless_pi::OnContextMenuItemCallback(int id)
{

  if (id == m_start_sim_id){

    wxString file;
    int response = PlatformFileSelectorDialog(
        NULL, &file, _("Select an NMEA text file"), *GetpPrivateApplicationDataLocation(), _T(""), _T("*.*"));

    if (response == wxID_OK) {

      msgFileName = file;
      if(::wxFileExists(msgFileName)){

        SetCanvasContextMenuItemViz(m_start_sim_id, false);
        SetCanvasContextMenuItemViz(m_stop_sim_id, true);

        startSim();
      }
    }
  }
  else if (id == m_stop_sim_id){
    SetCanvasContextMenuItemViz(m_start_sim_id, true);
    SetCanvasContextMenuItemViz(m_stop_sim_id, false);

    stopSim();
  }


    //startSim();
}

void ropeless_pi::PopupMenuHandler( wxCommandEvent& event )
{
    bool handled = false;
    switch( event.GetId() ) {

        case ID_PLAY_SIM:
            startSim();

            handled = true;
            break;

        case ID_EPL_XMIT:
        {

            m_NMEA0183.TalkerID = _T("EC");

            SENTENCE snt;

            if( m_fix_lat < 0. )
                m_NMEA0183.Gll.Position.Latitude.Set( -m_fix_lat, _T("S") );
            else
                m_NMEA0183.Gll.Position.Latitude.Set( m_fix_lat, _T("N") );

            if( m_fix_lon < 0. )
                m_NMEA0183.Gll.Position.Longitude.Set( -m_fix_lon, _T("W") );
            else
                m_NMEA0183.Gll.Position.Longitude.Set( m_fix_lon, _T("E") );

            wxDateTime now = wxDateTime::Now();
            wxDateTime utc = now.ToUTC();
            wxString time = utc.Format( _T("%H%M%S") );
            m_NMEA0183.Gll.UTCTime = time;

            m_NMEA0183.Gll.Mode = _T("M");              // Use GLL 2.3 specification
                                                        // and send "M" for "Manual fix"
            m_NMEA0183.Gll.IsDataValid = NFalse;        // Spec requires "Invalid" for manual fix

            m_NMEA0183.Gll.Write( snt );

            wxLogMessage(snt.Sentence);
            PushNMEABuffer( snt.Sentence );

            handled = true;
            break;
        }

        case ID_TPR_RELEASE:
        {
          wxString msg("Send Release command to transponder IDENT: \n\n");
          wxString msg1;
          msg1.Printf("%d\n\n", m_foundState->ident);
          msg += msg1;
          int ret = OCPNMessageBox_PlugIn(NULL, msg, _("ropeless_pi Message"), wxYES_NO);

          if(ret == wxID_YES){
            SendReleaseMessage(m_foundState);
          }

          handled = true;
          break;
        }


        default:
            break;
    }

    if(!handled)
        event.Skip();
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

unsigned char ropeless_pi::ComputeChecksum( wxString msg )
{
   unsigned char checksum_value = 0;

   char str_ascii[101];
   strncpy(str_ascii, (const char *)msg.mb_str(), 99);
   str_ascii[100] = '\0';

   int string_length = strlen(str_ascii);
   int index = 1; // Skip over the $ at the begining of the sentence

   while( index < string_length    &&
          str_ascii[ index ] != '*' &&
          str_ascii[ index ] != CARRIAGE_RETURN &&
          str_ascii[ index ] != LINE_FEED )
   {
         checksum_value ^= str_ascii[ index ];
         index++;
   }

   return( checksum_value );
}


bool ropeless_pi::SendReleaseMessage(transponder_state *state)
{
  bool ret = true;

  // Create payload
  wxString payload("$RSRLB,");
  wxString pl1;
  pl1.Printf("%d,0*", state->ident);
  payload += pl1;
  unsigned char cs = ComputeChecksum(payload);
  pl1.Printf("%02X", cs);
  payload += pl1;

  // Create a UDP transmit socket
  if( !m_tsock ){
    m_tconn_addr.Service(59647);
    // m_tconn_addr.Hostname("192.168.37.255");  //OK
    m_tconn_addr.Hostname("255.255.255.255");  //OK
    //m_tconn_addr.AnyAddress();      // NOPE, produces "0.0.0.0" for address, i.e. for listening
    wxString a = m_tconn_addr.IPAddress();
    m_tsock =  new wxDatagramSocket(m_tconn_addr, wxSOCKET_NOWAIT | wxSOCKET_REUSEADDR);

     int broadcastEnable = 1;
       m_tsock->SetOption(
           SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

  }

  wxDatagramSocket* udp_socket;
  udp_socket = dynamic_cast<wxDatagramSocket*>(m_tsock);
  if (udp_socket && udp_socket->IsOk()) {
     udp_socket->SendTo(m_tconn_addr, payload.mb_str(), payload.size());
     if (udp_socket->Error()){
       //wxSocketError err = udp_socket->LastError();
       ret = false;
     }
  }
  else
    ret = false;

  return ret;
}


int n_tick;
double countRun;
double countTarget;
double accelFactor;
wxString pendingMsg;
unsigned int inext;
unsigned int msgCount;
wxTextFile msgFile;
double tstamp_current;

void ropeless_pi::startSim()
{
    // Open the data file
  msgFile.Open(msgFileName);
  msgCount = msgFile.GetLineCount();
  inext = 0;
  n_tick = 0;

  countRun = 0;
  countTarget = 5;
  accelFactor = 40;

  m_simulatorTimer.SetOwner( this, SIM_TIMER );
  m_simulatorTimer.Start(100, wxTIMER_CONTINUOUS);

}

void ropeless_pi::stopSim()
{
  m_simulatorTimer.Stop();

}


void ropeless_pi::ProcessSimTimerEvent( wxTimerEvent& event )
{
  n_tick++;
  countRun += .100 * accelFactor;     // 100 msec basic timer

  if(countRun < countTarget){
    if( (n_tick %10) == 0){            // once per second
      printf("next msg in: %g\n", countTarget-countRun);
    }
  }
  else{
    //  Send the pending msg
    SetNMEASentence( pendingMsg );
    RequestRefresh(GetOCPNCanvasWindow());

    // Fetch the next message
    if(inext < msgCount){
      pendingMsg = msgFile.GetLine(inext);
      pendingMsg.Append("\r\n");
      inext++;

      double tstamp_last = tstamp_current;

      // parse the pending msg to get the timestamp
      m_NMEA0183 << pendingMsg;

      if( m_NMEA0183.PreParse() ) {
        if( m_NMEA0183.Parse() ){
          tstamp_current = m_NMEA0183.Rfa.TimeStamp;

          if(inext > 1){
            //countTarget = (tstamp_current - tstamp_last) * 3600 * 24;   First data set, time stamp in julian days
            countTarget = (tstamp_current - tstamp_last);   //Second data set, time stamp is in seconds
          }
          countRun = 0;
        }
      }
    }
    else{
      SetCanvasContextMenuItemViz(m_start_sim_id, true);
      SetCanvasContextMenuItemViz(m_stop_sim_id, false);

      stopSim();
    }
  }
}



void ropeless_pi::ProcessTimerEvent( wxTimerEvent& event )
{

    // Age the transponder history records
    for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
      transponder_state *state = transponderStatus[i];

      std::deque<transponder_state_history *>::iterator it = state->historyQ.begin();
      while (it != state->historyQ.end()){
        transponder_state_history *tsh = *it++;
        if (tsh->tsh_timer_age > 0)
          tsh->tsh_timer_age--;
      }
    }

    RequestRefresh(GetOCPNCanvasWindow());

}

void ropeless_pi::populateTransponderNode(pugi::xml_node &transponderNode,
                                   transponder_state *state)
{
  pugi::xml_node child;

  child = transponderNode.append_child("ident");
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

}

void ropeless_pi::SaveTransponderStatus(){

    pugi::xml_document transponderStatusDoc;
    pugi::xml_node transpondersNode = transponderStatusDoc.append_child("transponders");

    pugi::xml_node childT = transpondersNode.append_child("version");
    childT.append_child(pugi::node_pcdata).set_value("0.0.0");
    childT = transpondersNode.append_child("date");
    wxDateTime now = wxDateTime::GetTimeNow();
    wxString timeFormat = now.FormatISOCombined(' ');
    childT.append_child(pugi::node_pcdata).set_value(timeFormat.mb_str());

    for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
      transponder_state *state = transponderStatus[i];

      pugi::xml_node transponderNode = transpondersNode.append_child("transponder");
      pugi::xml_attribute version = transponderNode.append_attribute("version");
      version.set_value("1");

      populateTransponderNode(transponderNode, state);
    }

    wxString fileName = *GetpPrivateApplicationDataLocation() +
                           wxFileName::GetPathSeparator() +
                           _T("ropeless-transponders.xml");
    transponderStatusDoc.save_file(fileName.mb_str(), "  ");
}


bool ropeless_pi::parseTransponderNode(pugi::xml_node &transponderNode, transponder_state *state)
{

     if (!strcmp(transponderNode.name(), "transponder")) {
        for (pugi::xml_node child = transponderNode.first_child(); child;
            child = child.next_sibling()) {

          if (!strcmp(child.name(), "ident")) {
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
     }
  }

  return true;
}

void ropeless_pi::LoadTransponderStatus()
{
    pugi::xml_document transponderStatusXML;

    wxString fileName = *GetpPrivateApplicationDataLocation() +
                           wxFileName::GetPathSeparator() +
                           _T("ropeless-transponders.xml");

    if (!wxFileExists(fileName))
      return;

    bool ret = transponderStatusXML.load_file(fileName.mb_str());
    if (ret) {
      transponder_state state;
      pugi::xml_node transponderRoot = transponderStatusXML.first_child();

      if (!parseTransponderNode(transponderRoot, &state)) {
        OCPNMessageBox_PlugIn(GetOCPNCanvasWindow(), _("Error processing Ropless Transponder status (XML) file."),
                       _("OpenCPN Ropeless Plugin Error"));
        return;
      }
    }

    pugi::xml_node statusRoot = transponderStatusXML.first_child();
    for (pugi::xml_node element = statusRoot.first_child(); element;
         element = element.next_sibling()) {
      if (!strcmp(element.name(), "transponder")) {
        transponder_state *this_state = new transponder_state;;
        if(parseTransponderNode(element, this_state)){
          // Select a new color for this new transponder
          this_state->color_index = m_colorIndexNext;
          m_colorIndexNext++;
          if(m_colorIndexNext == COLOR_TABLE_COUNT)
            m_colorIndexNext = 0;

          transponderStatus.push_back( this_state );
        }
        else
          delete this_state;
      }
    }
}




void ropeless_pi::RenderTransponder( transponder_state *state)
{
    int circle_size = 10;

    wxPoint ab;
    wxString colorName = colorTableNames[state->color_index];
    wxColour rcolour = wxTheColourDatabase->Find(colorName);
    if (!rcolour.IsOk())
      rcolour = wxColour(255,000,255);

    // Render the primary instant transponder
    GetCanvasPixLL(g_vp, &ab, state->predicted_lat, state->predicted_lon);
    wxPen dpen( rcolour );
    wxBrush dbrush( rcolour );
    m_oDC->SetPen( dpen );
    m_oDC->SetBrush( dbrush );
    m_oDC->DrawCircle( ab.x, ab.y, circle_size);


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
    // Draw an "X" over the prinary target
    wxPoint x1(ab.x - circle_size * .707, ab.y - circle_size * .707);
    wxPoint x2(ab.x + circle_size * .707, ab.y + circle_size * .707);
    wxPoint x3(ab.x - circle_size * .707, ab.y + circle_size * .707);
    wxPoint x4(ab.x + circle_size * .707, ab.y - circle_size * .707);
    wxPen xpen( *wxBLACK, 3 );
    m_oDC->SetPen( xpen );
    m_oDC->DrawLine(x1.x, x1.y, x2.x, x2.y, true);
    m_oDC->DrawLine(x3.x, x3.y, x4.x, x4.y, true);

}

void ropeless_pi::RenderTrawlConnector( transponder_state *state1, transponder_state *state2 )
{
    wxPoint P1, P2;
    GetCanvasPixLL(g_vp, &P1, state1->predicted_lat, state1->predicted_lon);
    GetCanvasPixLL(g_vp, &P2, state2->predicted_lat, state2->predicted_lon);

    wxColour rcolour = wxTheColourDatabase->Find(wxString("BLACK"));
    wxPen dpen( rcolour, 3 );
    wxBrush dbrush( rcolour );

    m_oDC->DrawLine(P1.x, P1.y, P2.x, P2.y, true);
}



transponder_state *ropeless_pi::GetStateByIdent( int identTarget )
{
  transponder_state *rv = NULL;
  for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
    transponder_state *state = transponderStatus[i];
    if ( state->ident == identTarget)
      return state;
  }

  return rv;
}

void ropeless_pi::RenderTrawls()
{
    //  Walk the vector of transponder status
  for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
    transponder_state *state = transponderStatus[i];

    RenderTransponder(state);

    if (state->ident_partner != state->ident){
      transponder_state *statePartner = GetStateByIdent( state->ident_partner );
      if (statePartner){
        RenderTrawlConnector( state, statePartner );
      }
    }
  }
}


bool ropeless_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
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

bool ropeless_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    if (!m_oDC) m_oDC = new ODDC();

    m_oDC->SetVP(vp);


//     g_gdc = NULL;
//     g_pdc = NULL;

    g_vp = vp;
    Clone_VP(&g_ovp, vp);                // deep copy

    m_selectRadius = (10 / vp->view_scale_ppm) / (1852. * 60.);

    //m_select->SetSelectLLRadius(selec_radius);

    //Render
    RenderTrawls();

    return true;
}


void ropeless_pi::ProcessRFACapture( void )
{
  if( m_NMEA0183.LastSentenceIDReceived != _T("RFA") )
    return;

  int transponderIdent = m_NMEA0183.Rfa.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
    if (transponderStatus[i]->ident == transponderIdent){
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // If not present, create a new record, and add to vector
  if (this_transponder_state == NULL){
    this_transponder_state = new transponder_state;

    // Select a new color for this new transponder
    this_transponder_state->color_index = m_colorIndexNext;
    m_colorIndexNext++;
    if(m_colorIndexNext == COLOR_TABLE_COUNT)
      m_colorIndexNext = 0;

    transponderStatus.push_back( this_transponder_state );
  }
  else {                         // Maintain history buffer
    transponder_state_history *this_transponder_state_history = new transponder_state_history;
    this_transponder_state_history->ident = this_transponder_state->ident;
    this_transponder_state_history->ident_partner = this_transponder_state->ident_partner;
    this_transponder_state_history->timeStamp = this_transponder_state->timeStamp;
    this_transponder_state_history->predicted_lat = this_transponder_state->predicted_lat;
    this_transponder_state_history->predicted_lon = this_transponder_state->predicted_lon;
    this_transponder_state_history->color_index = this_transponder_state->color_index;
    this_transponder_state_history->tsh_timer_age = HISTORY_FADE_SECS;

    if (this_transponder_state->historyQ.size() > 10){
        this_transponder_state->historyQ.pop_back();
    }
    this_transponder_state->historyQ.push_front(this_transponder_state_history);
  }

  //  Update the instant record
  this_transponder_state->ident = transponderIdent;
  this_transponder_state->ident_partner = m_NMEA0183.Rfa.TransponderPartner;

  this_transponder_state->timeStamp = m_NMEA0183.Rfa.TimeStamp;

  this_transponder_state->predicted_lat = m_NMEA0183.Rfa.TransponderPosition.Latitude.Latitude;
      if (m_NMEA0183.Rfa.TransponderPosition.Latitude.Northing == South)
        this_transponder_state->predicted_lat = -this_transponder_state->predicted_lat;

  this_transponder_state->predicted_lon = m_NMEA0183.Rfa.TransponderPosition.Longitude.Longitude;
  if (m_NMEA0183.Rfa.TransponderPosition.Longitude.Easting == West)
    this_transponder_state->predicted_lon = -this_transponder_state->predicted_lon;


  if(m_pRLDialog){
    m_pRLDialog->RefreshTransponderList();
  }
}


void ropeless_pi::ProcessRLACapture( void )
{
  if( m_NMEA0183.LastSentenceIDReceived != _T("RLA") )
    return;

  int transponderIdent = m_NMEA0183.Rla.TransponderCode;

  // Check if status for this transponder is in the status vector
  transponder_state *this_transponder_state = NULL;
  for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
    if (transponderStatus[i]->ident == transponderIdent){
      this_transponder_state = transponderStatus[i];
      break;
    }
  }

  // If specified transponder is not present, ignore the message
  if (this_transponder_state == NULL)
    return;

  // Update the record
  this_transponder_state->release_status = m_NMEA0183.Rla.TransponderStatus;

   if(m_pRLDialog){
    m_pRLDialog->RefreshTransponderList();
  }
}


void ropeless_pi::SetNMEASentence( wxString &sentence )
{
    if (sentence.IsEmpty())
      return;

    m_NMEA0183 << sentence;

    if( m_NMEA0183.PreParse() ) {
        if( m_NMEA0183.LastSentenceIDReceived == _T("RFA") ) {
            if( m_NMEA0183.Parse() )
                ProcessRFACapture();
        }
        if( m_NMEA0183.LastSentenceIDReceived == _T("RLA") ) {
            if( m_NMEA0183.Parse() )
                ProcessRLACapture();
        }


    }
}

void ropeless_pi::SetPositionFix( PlugIn_Position_Fix &pfix )
{
    if(1){
        m_ownship_lat = pfix.Lat;
        m_ownship_lon = pfix.Lon;
        if(!wxIsNaN( pfix.Cog ))
            m_ownship_cog = pfix.Cog;
        if(!wxIsNaN( pfix.Var ))
            mVar = pfix.Var;
    }
}

void ropeless_pi::SetCursorLatLon( double lat, double lon )
{
}

void ropeless_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
}

int ropeless_pi::GetToolbarToolCount( void )
{
    return 1;
}


void ropeless_pi::SetColorScheme( PI_ColorScheme cs )
{
}


bool ropeless_pi::LoadConfig( void )
{

    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if( pConf ) {

        pConf->SetPath ( _T( "/Settings/Ropeless_pi" ) );
        pConf->Read("dialogSizeWidth", &m_dialogSizeWidth, -1);
        pConf->Read("dialogSizeHeight", &m_dialogSizeHeight, -1);
        pConf->Read("dialogPosX", &m_dialogPosX, -1);
        pConf->Read("dialogPosY", &m_dialogPosY, -1);



        pConf->Read ( _T( "SerialPort" ),  &m_serialPort );
        pConf->Read ( _T( "TrackedPoint" ),  &m_trackedWPGUID );

        m_trackedWP = getWaypointName(m_trackedWPGUID);

        pConf->Read ( _T( "TenderLength" ),  &m_tenderLength );
        pConf->Read ( _T( "TenderWidth" ),  &m_tenderWidth );
        pConf->Read ( _T( "TenderGPSOffsetX" ),  &m_tenderGPS_x );
        pConf->Read ( _T( "TenderGPSOffsetY" ),  &m_tenderGPS_y );

        pConf->Read ( _T( "TenderIconType" ),  &m_tenderIconType );


        // Qualify the input
        m_tenderLength = wxMax(1, m_tenderLength);
        m_tenderWidth = wxMax(1, m_tenderWidth);

        bool bfound = false;
        for(unsigned int i=0 ; i < g_iconTypeArray.Count() ; i++){
            if(m_tenderIconType.IsSameAs(g_iconTypeArray.Item(i))){
                bfound = true;
                break;
            }
        }
        if(!bfound)
            m_tenderIconType = g_iconTypeArray.Item(0);


        //setIcon( shippity );

        return true;
    } else
        return false;
}

bool ropeless_pi::SaveConfig( void )
{
    wxFileConfig *pConf = (wxFileConfig *) m_pconfig;

    if( pConf ) {

        pConf->SetPath ( _T( "/Settings/Ropeless_pi" ) );

        pConf->Write ( "dialogSizeWidth",  m_dialogSizeWidth );
        pConf->Write ( "dialogSizeHeight",  m_dialogSizeHeight );
        pConf->Write ( "dialogPosX",  m_dialogPosX );
        pConf->Write ( "dialogPosY",  m_dialogPosY );


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

void ropeless_pi::ApplyConfig( void )
{

}


bool ropeless_pi::MouseEventHook( wxMouseEvent &event )
{
    bool bret = false;

    m_mouse_x = event.m_x;
    m_mouse_y = event.m_y;

//     //  Retrigger the rollover timer
//     if( m_pBrgRolloverWin && m_pBrgRolloverWin->IsActive() )
//         m_RolloverPopupTimer.Start( 10, wxTIMER_ONE_SHOT );               // faster response while the rollover is turned on
//     else
//         m_RolloverPopupTimer.Start( m_rollover_popup_timer_msec, wxTIMER_ONE_SHOT );

    wxPoint mp(event.m_x, event.m_y);
    GetCanvasLLPix( &g_ovp, mp, &m_cursor_lat, &m_cursor_lon);

    m_foundState = NULL;
    //  On button push, find any transponders
    if( event.RightDown() || event.LeftDown()) {


      for (unsigned int i = 0 ; i < transponderStatus.size() ; i++){
        transponder_state *state = transponderStatus[i];

        double a = fabs( m_cursor_lat - state->predicted_lat );
        double b = fabs( m_cursor_lon - state->predicted_lon );

        if( ( a < m_selectRadius ) && ( b < m_selectRadius ) ){
          m_foundState = state;
          break;
        }
      }



//        m_pFind = m_select->FindSelection( m_cursor_lat, m_cursor_lon, SELTYPE_POINT_GENERIC );

//        if(m_pFind){
#if 0
            for(unsigned int i=0 ; i < m_brg_array.GetCount() ; i++){
                brg_line *pb = m_brg_array.Item(i);

                if(m_pFind->m_pData1 == pb){
                    m_sel_brg = pb;
                    m_sel_part = m_pFind->GetUserData();
                    if(SEL_POINT_A == m_sel_part){
                        m_sel_pt_lat = pb->GetLatA();
                        m_sel_pt_lon = pb->GetLonA();
                    }
                    else {
                        m_sel_pt_lat = pb->GetLatB();
                        m_sel_pt_lon = pb->GetLonB();
                    }

                    break;
                }
            }
#endif
//        }
    }



    if( event.RightDown() ) {

        if( 1/*m_sel_brg || m_bshow_fix_hat*/){

            wxMenu* contextMenu = new wxMenu;

            wxMenuItem *release_item = 0;

            if(m_foundState){
                release_item = new wxMenuItem(contextMenu, ID_TPR_RELEASE, _("Release Transponder") );
                contextMenu->Append(release_item);
                GetOCPNCanvasWindow()->Connect( ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
                                                wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
            }

//             if(!m_bshow_fix_hat && m_sel_brg){
//                 brg_item = new wxMenuItem(contextMenu, ID_EPL_DELETE, _("Delete Bearing") );
//                 contextMenu->Append(brg_item);
//                 GetOCPNCanvasWindow()->Connect( ID_EPL_DELETE, wxEVT_COMMAND_MENU_SELECTED,
//                                                 wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
//             }

//             if(m_bshow_fix_hat){
//                 wxMenuItem *fix_item = new wxMenuItem(contextMenu, ID_EPL_XMIT, _("Send sighted fix to device") );
//                 contextMenu->Append(fix_item);
//                 GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
//                                                 wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
//             }

            //   Invoke the drop-down menu
            GetOCPNCanvasWindow()->PopupMenu( contextMenu, m_mouse_x, m_mouse_y );

            if(release_item){
                GetOCPNCanvasWindow()->Disconnect( ID_TPR_RELEASE, wxEVT_COMMAND_MENU_SELECTED,
                                                   wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
            }

//             if(fix_item){
//                 GetOCPNCanvasWindow()->Connect( ID_EPL_XMIT, wxEVT_COMMAND_MENU_SELECTED,
//                                                 wxCommandEventHandler( ropeless_pi::PopupMenuHandler ), NULL, this );
//             }

            bret = true;                // I have eaten this event
        }

    }

    else if( event.LeftDown() ) {
    }

//     if( event.LeftUp() ) {
//         if(m_sel_brg)
//             bret = true;
//
//         m_pFind = NULL;
//         m_sel_brg = NULL;
//
//         m_nfix = CalculateFix();
//
//     }

    return bret;
}





void ropeless_pi::ShowPreferencesDialog( wxWindow* parent )
{
#if 0
    TenderPrefsDialog *dialog = new TenderPrefsDialog( parent, wxID_ANY, _("Ropeless_pi Preferences"), wxPoint( 20, 20), wxDefaultSize, wxDEFAULT_DIALOG_STYLE );
    dialog->Fit();
    wxColour cl;
    GetGlobalColor(_T("DILG1"), &cl);
    dialog->SetBackgroundColour(cl);

    dialog->m_comboPort->SetValue(m_serialPort);
    dialog->m_wpComboPort->SetValue(m_trackedWP);

    dialog->m_comboIcon->SetValue(m_tenderIconType);

    wxString val;

    val.Printf(_T("%4d"), m_tenderGPS_x);
    dialog->m_pTenderGPSOffsetX->SetValue(val);
    val.Printf(_T("%4d"), m_tenderGPS_y);
    dialog->m_pTenderGPSOffsetY->SetValue(val);
    val.Printf(_T("%4d"), m_tenderLength);
    dialog->m_pTenderLength->SetValue(val);
    val.Printf(_T("%4d"), m_tenderWidth);
    dialog->m_pTenderWidth->SetValue(val);



    if(dialog->ShowModal() == wxID_OK)
    {
        m_serialPort = dialog->m_comboPort->GetValue();

        m_trackedWP = dialog->m_trackedPointName;
        m_trackedWPGUID = dialog->m_trackedPointGUID;

        m_tenderIconType = dialog->m_comboIcon->GetValue();

        long val;
        wxString str;
        str = dialog->m_pTenderGPSOffsetX->GetValue();
        if(str.ToLong(&val)) { m_tenderGPS_x = val; }

        str = dialog->m_pTenderGPSOffsetY->GetValue();
        if(str.ToLong(&val)) { m_tenderGPS_y = val; }

        str = dialog->m_pTenderLength->GetValue();
        if(str.ToLong(&val)) { m_tenderLength = val; }

        str = dialog->m_pTenderWidth->GetValue();
        if(str.ToLong(&val)) { m_tenderWidth = val; }

         SaveConfig();

         setTrackedWPSelect(m_trackedWPGUID);

    }
    delete dialog;
#endif
}



//      Event Handler implementation
BEGIN_EVENT_TABLE ( PI_EventHandler, wxEvtHandler )
EVT_TIMER ( ROLLOVER_TIMER, PI_EventHandler::OnTimerEvent )
EVT_TIMER ( HEAD_DOG_TIMER, PI_EventHandler::OnTimerEvent )

END_EVENT_TABLE()



PI_EventHandler::PI_EventHandler(ropeless_pi * parent)
{
    m_parent = parent;
    Connect( wxEVT_PI_OCPN_DATASTREAM, (wxObjectEventFunction) (wxEventFunction) &PI_EventHandler::OnEvtOCPN_NMEA );

}

PI_EventHandler::~PI_EventHandler()
{
    Disconnect( wxEVT_PI_OCPN_DATASTREAM, (wxObjectEventFunction) (wxEventFunction) &PI_EventHandler::OnEvtOCPN_NMEA );

}


void PI_EventHandler::OnTimerEvent(wxTimerEvent& event)
{
    m_parent->ProcessTimerEvent( event );
}

void PI_EventHandler::PopupMenuHandler(wxCommandEvent& event )
{
    m_parent->PopupMenuHandler( event );

}

void PI_EventHandler::OnEvtOCPN_NMEA( PI_OCPN_DataStreamEvent& event )
{
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




BEGIN_EVENT_TABLE( RopelessDialog, wxDialog )
EVT_BUTTON( wxID_OK, RopelessDialog::OnOkClick )
EVT_CLOSE(RopelessDialog::OnClose)
END_EVENT_TABLE()

RopelessDialog::RopelessDialog( wxWindow* parent, ropeless_pi *parent_pi,
                                wxWindowID id, const wxString& title,
                                const wxPoint& pos, const wxSize& size, long style )
            : wxDialog( parent, id, title, pos, size, style )
{
        pParentPi = parent_pi;

        this->SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* bSizer2;
        bSizer2 = new wxBoxSizer( wxVERTICAL );

        // The transponder list
        long flags = wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES |
               wxBORDER_SUNKEN;

        m_pListCtrlTranponders = new OCPNListCtrl(this, ID_TRANSPONDER_LIST, wxDefaultPosition, wxDefaultSize, flags);
        bSizer2->Add(m_pListCtrlTranponders, 1, wxEXPAND | wxALL, 0);

        int dx = GetCharWidth();

        int colWidth = dx * 6;
        m_pListCtrlTranponders->InsertColumn(tlICON, _("Color"), wxLIST_FORMAT_LEFT,  colWidth);

        colWidth = dx * 6;
        m_pListCtrlTranponders->InsertColumn(tlIDENT, _("Ident"), wxLIST_FORMAT_LEFT,  colWidth);

        colWidth = dx * 13;
        m_pListCtrlTranponders->InsertColumn(tlRELEASE_STATUS, _("Release Status"), wxLIST_FORMAT_LEFT, colWidth);

        colWidth = dx * 18;
        m_pListCtrlTranponders->InsertColumn(tlTIMESTAMP, _("LastReportTime (UTC)"), wxLIST_FORMAT_LEFT, colWidth);

        // Build the color indicator bitmaps, adding to an image lst
        int imageRefSize = dx * 2;
        wxImageList *imglist = new wxImageList(imageRefSize, imageRefSize, true, 1);

        for (int i=0 ; i < COLOR_TABLE_COUNT ; i++){
          wxScreenDC sdc;

          wxBitmap tbm(imageRefSize, imageRefSize, -1);
          wxMemoryDC mdc(tbm);
          mdc.Clear();
          wxString colorName = colorTableNames[i]; //colorTableNames[state->color_index];
          wxColour rcolour = wxTheColourDatabase->Find(colorName);
          if (!rcolour.IsOk())
            rcolour = wxColour(255,000,255);
          wxPen dpen( rcolour );
          wxBrush dbrush( rcolour );
          mdc.SetPen( dpen );
          mdc.SetBrush( dbrush );

          int xd = 0;
          int yd = 0;
    //    mdc.DrawRoundedRectangle(xd, yd, w+(label_offset * 2), h+2, -.25);
          mdc.DrawRectangle(xd, yd, imageRefSize, imageRefSize);
          mdc.SelectObject(wxNullBitmap);

          imglist->Add(tbm);
        }

        m_pListCtrlTranponders->AssignImageList(imglist, wxIMAGE_LIST_SMALL);






        wxStaticBoxSizer* sbSizerSim = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Simulator") ), wxVERTICAL );
        bSizer2->Add( sbSizerSim, 0, wxALL|wxEXPAND, 5 );

        m_simTextCtrl = new wxTextCtrl (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
        sbSizerSim->Add( m_simTextCtrl, 0, wxEXPAND | wxALL, 5);

        if (wxFileExists(msgFileName))
          m_simTextCtrl->SetValue(msgFileName);

        wxBoxSizer *bsizersimButtons = new wxBoxSizer( wxHORIZONTAL );
        sbSizerSim->Add( bsizersimButtons, 0, wxEXPAND, 5);

        m_ChooseFileButton = new wxButton(this, wxID_ANY, _("Choose File..."),  wxDefaultPosition, wxDefaultSize, 0);
        bsizersimButtons->Add( m_ChooseFileButton, 0, wxALIGN_LEFT | wxALL, 5);
        m_ChooseFileButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnChooseFileButton, this);

        m_StopSimButton = new wxButton(this, wxID_ANY, _("Stop Sim"),  wxDefaultPosition, wxDefaultSize, 0);
        bsizersimButtons->Add( m_StopSimButton, 0, wxALIGN_RIGHT | wxALL, 5);
        m_StopSimButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnStopSimButton, this);

        m_StartSimButton = new wxButton(this, wxID_ANY, _("StartSim"),  wxDefaultPosition, wxDefaultSize, 0);
        bsizersimButtons->Add( m_StartSimButton, 0, wxALIGN_RIGHT | wxALL, 5);
        m_StartSimButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RopelessDialog::OnStartSimButton, this);

        if (pParentPi->m_simulatorTimer.IsRunning()){
          m_StartSimButton->Hide();
          m_StopSimButton->Show();
        }
        else{
          m_StopSimButton->Hide();
          m_StartSimButton->Show();
        }

#if 0
        wxStaticBoxSizer* sbSizerP = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tender GPS Serial Port") ), wxVERTICAL );

        m_comboPort = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
        sbSizerP->Add(m_comboPort, 0, wxEXPAND | wxTOP, 5);

        bSizer2->Add( sbSizerP, 0, wxALL|wxEXPAND, 5 );

        m_pSerialArray = NULL;
        m_pSerialArray = EnumerateSerialPorts();

        if (m_pSerialArray) {
            m_comboPort->Clear();
            for (size_t i = 0; i < m_pSerialArray->Count(); i++) {
                m_comboPort->Append(m_pSerialArray->Item(i));
            }
        }

        // Get the global waypoint list

        wxStaticBoxSizer* sbSizerwP = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracked Waypoint (by Name)") ), wxVERTICAL );

        m_wpComboPort = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
        sbSizerwP->Add(m_wpComboPort, 0, wxEXPAND | wxTOP, 5);

        bSizer2->Add( sbSizerwP, 0, wxALL|wxEXPAND, 5 );

        wxArrayString guidArray = GetWaypointGUIDArray();

        m_wpComboPort->Clear();
        for(unsigned int i=0 ; i < guidArray.GetCount() ; i++){
            wxString name = getWaypointName( guidArray[i] );
            if(name.Length())
                m_wpComboPort->Append(name);
        }


        // Icon type
        wxStaticBoxSizer* sbSizerIcon = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tender Icon") ), wxVERTICAL );

        m_comboIcon = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
        sbSizerIcon->Add(m_comboIcon, 0, wxEXPAND | wxTOP, 5);

        bSizer2->Add( sbSizerIcon, 0, wxALL|wxEXPAND, 5 );


        m_comboIcon->Clear();
        for (size_t i = 0; i < g_iconTypeArray.Count(); i++) {
            m_comboIcon->Append(g_iconTypeArray.Item(i));
        }
#endif

#if 0
        // Icon parameters
        wxStaticBoxSizer* sbSizerDimensions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tender Dimensions") ), wxVERTICAL );
        bSizer2->Add( sbSizerDimensions, 1, wxALL|wxEXPAND, 5 );

        wxFlexGridSizer *realSizes = new wxFlexGridSizer(5, 2, 4, 4);
        realSizes->AddGrowableCol(1);

        sbSizerDimensions->Add(realSizes, 0, wxEXPAND | wxLEFT, 30);

        realSizes->Add( new wxStaticText(this, wxID_ANY, _("Length Over All (m)")), 1, wxALIGN_LEFT);
        m_pTenderLength = new wxTextCtrl(this, 1);
        realSizes->Add(m_pTenderLength, 1, wxALIGN_RIGHT | wxALL, 4);

        realSizes->Add( new wxStaticText(this, wxID_ANY, _("Width Over All (m)")), 1, wxALIGN_LEFT);
        m_pTenderWidth = new wxTextCtrl(this, wxID_ANY);
        realSizes->Add(m_pTenderWidth, 1, wxALIGN_RIGHT | wxALL, 4);

        realSizes->Add( new wxStaticText(this, wxID_ANY, _("GPS Offset from Bow (m)")),1, wxALIGN_LEFT);
        m_pTenderGPSOffsetY = new wxTextCtrl(this, wxID_ANY);
        realSizes->Add(m_pTenderGPSOffsetY, 1, wxALIGN_RIGHT | wxALL, 4);

        realSizes->Add(new wxStaticText(this, wxID_ANY, _("GPS Offset from Midship (m)")), 1, wxALIGN_LEFT);
        m_pTenderGPSOffsetX = new wxTextCtrl(this, wxID_ANY);
        realSizes->Add(m_pTenderGPSOffsetX, 1, wxALIGN_RIGHT | wxALL, 4);
#endif

 /*
        wxString m_rbViewTypeChoices[] = { _("Extended"), _("Variation only") };
        int m_rbViewTypeNChoices = sizeof( m_rbViewTypeChoices ) / sizeof( wxString );
        m_rbViewType = new wxRadioBox( this, wxID_ANY, _("View"), wxDefaultPosition, wxDefaultSize, m_rbViewTypeNChoices, m_rbViewTypeChoices, 2, wxRA_SPECIFY_COLS );
        m_rbViewType->SetSelection( 1 );
        bSizer2->Add( m_rbViewType, 0, wxALL|wxEXPAND, 5 );

        m_cbShowPlotOptions = new wxCheckBox( this, wxID_ANY, _("Show Plot Options"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbShowPlotOptions, 0, wxALL, 5 );

        m_cbShowAtCursor = new wxCheckBox( this, wxID_ANY, _("Show also data at cursor position"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbShowAtCursor, 0, wxALL, 5 );

        m_cbShowIcon = new wxCheckBox( this, wxID_ANY, _("Show toolbar icon"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbShowIcon, 0, wxALL, 5 );

        m_cbLiveIcon = new wxCheckBox( this, wxID_ANY, _("Show data in toolbar icon"), wxDefaultPosition, wxDefaultSize, 0 );
        bSizer2->Add( m_cbLiveIcon, 0, wxALL, 5 );

        wxStaticBoxSizer* sbSizer4;
        sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Window transparency") ), wxVERTICAL );

        m_sOpacity = new wxSlider( this, wxID_ANY, 255, 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_INVERSE );
        sbSizer4->Add( m_sOpacity, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


        bSizer2->Add( sbSizer4, 1, wxALL|wxEXPAND, 5 );
*/

        m_sdbSizer1 = new wxStdDialogButtonSizer();
        m_sdbSizer1OK = new wxButton( this, wxID_OK );
        m_sdbSizer1->AddButton( m_sdbSizer1OK );
        m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
        m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
        m_sdbSizer1->Realize();

        bSizer2->Add( m_sdbSizer1, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


        this->SetSizer( bSizer2 );
        this->Layout();
        bSizer2->Fit( this );

        this->Centre( wxBOTH );
}

RopelessDialog::~RopelessDialog()
{
        //delete m_pSerialArray;
}

void RopelessDialog::RefreshTransponderList()
{
  m_pListCtrlTranponders->DeleteAllItems();

      //  Walk the vector of transponder status
  for (unsigned int i = 0 ; i < pParentPi->transponderStatus.size() ; i++){
    transponder_state *state = pParentPi->transponderStatus[i];

    wxListItem item;
    item.SetId(i);
    m_pListCtrlTranponders->InsertItem(item);

    item.SetColumn(tlICON);
    m_pListCtrlTranponders->SetItemImage(item, state->color_index);


    item.SetColumn(tlIDENT);
    wxString sid;
    sid.Printf("%d", state->ident);
    item.SetText(sid);
    m_pListCtrlTranponders->SetItem(item);

    item.SetColumn(tlRELEASE_STATUS);
    wxString srs;
    if (state->release_status == -2)
      sid = "---";
    else
      sid.Printf("%d", state->release_status);
    item.SetText(sid);
    m_pListCtrlTranponders->SetItem(item);

    item.SetColumn(tlTIMESTAMP);
    wxString sts;
    //wxDateTime ts = DaysTowDT(state->timeStamp);
    wxDateTime ts((time_t)(state->timeStamp));
    ts.MakeUTC();
    sts = ts.FormatISOCombined(' ');
    //sts.Printf("%g", state->timeStamp);
    item.SetText(sts);
    m_pListCtrlTranponders->SetItem(item);

  }
#ifdef __WXMSW__
    m_pListCtrlTranponders->Refresh(false);
#endif

}

void RopelessDialog::OnChooseFileButton(wxCommandEvent &event)
{
    wxString file;
    int response = PlatformFileSelectorDialog(
        NULL, &file, _("Select an NMEA text file"), *GetpPrivateApplicationDataLocation(), _T(""), _T("*.*"));

    if (response == wxID_OK) {

      if(::wxFileExists( file)){
        msgFileName = file;
        m_simTextCtrl->SetValue(msgFileName);
      }
    }
}

void RopelessDialog::OnStopSimButton(wxCommandEvent &event)
{
    SetCanvasContextMenuItemViz(pParentPi->m_start_sim_id, true);
    SetCanvasContextMenuItemViz(pParentPi->m_stop_sim_id, false);

    m_StopSimButton->Hide();
    m_StartSimButton->Show();

    pParentPi->stopSim();
    Layout();

}

void RopelessDialog::OnStartSimButton(wxCommandEvent &event)
{

   if(::wxFileExists(msgFileName)){

        SetCanvasContextMenuItemViz(pParentPi->m_start_sim_id, false);
        SetCanvasContextMenuItemViz(pParentPi->m_stop_sim_id, true);
        m_StartSimButton->Hide();
        m_StopSimButton->Show();
        pParentPi->startSim();
        Layout();

   }
}

void RopelessDialog::OnClose(wxCloseEvent& event)
{
  wxPoint p = GetPosition();
  pParentPi->m_dialogPosX = p.x;
  pParentPi->m_dialogPosY = p.y;
  wxSize s = GetSize();
  pParentPi->m_dialogSizeWidth = s.x;
  pParentPi->m_dialogSizeHeight = s.y;

	event.Skip();
}

void RopelessDialog::OnOkClick(wxCommandEvent& event)
{
    m_StopSimButton->Hide();
    m_StartSimButton->Show();
    g_ropelessPI->stopSim();

    //EndModal( wxID_OK );
    Close();
}


