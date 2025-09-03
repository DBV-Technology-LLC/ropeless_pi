/***************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  NMEA0183 TCP Output Connection
 * Author:   David S. Register
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
 **************************************************************************/

#ifndef __NMEA_TCP_OUTPUTCONNECTION_H__
#define __NMEA_TCP_OUTPUTCONNECTION_H__

#include <wx/socket.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/thread.h>
#include <wx/string.h>
#include <queue>

// Forward declarations
class RESPONSE;
class DBS;
class GML;
class GMS;
class GMR;
class SENTENCE;

/**
 * TCP Output Connection for NMEA0183 messages
 * 
 * This class provides a thread-safe TCP output connection that can send
 * NMEA0183 messages using the built-in NMEA classes. It handles connection
 * management, queuing, and automatic reconnection.
 */
class NMEA_TCP_OutputConnection : public wxEvtHandler
{
public:
    /**
     * Constructor
     * @param host Hostname or IP address to connect to
     * @param port TCP port number
     */
    NMEA_TCP_OutputConnection(const wxString& host, int port);
    
    /**
     * Destructor - ensures clean shutdown
     */
    virtual ~NMEA_TCP_OutputConnection();

    /**
     * Connection Management
     */
    bool Connect();
    void Disconnect();
    bool IsConnected() const;
    bool IsConnecting() const;
    
    /**
     * Configuration
     */
    void SetHost(const wxString& host) { m_host = host; }
    void SetPort(int port) { m_port = port; }
    void SetReconnectInterval(int seconds) { m_reconnectInterval = seconds; }
    void SetAutoReconnect(bool enable) { m_autoReconnect = enable; }
    
    wxString GetHost() const { return m_host; }
    int GetPort() const { return m_port; }
    bool GetAutoReconnect() const { return m_autoReconnect; }
    
    /**
     * Send NMEA messages using built-in NMEA classes
     */
    bool SendNMEAMessage(RESPONSE* nmea_message);
    bool SendDBS(const DBS& dbs_msg);
    bool SendGML(const GML& gml_msg);
    bool SendGMS(const GMS& gms_msg);
    bool SendGMR(const GMR& gmr_msg);
    
    /**
     * Send raw NMEA sentence
     */
    bool SendRawNMEA(const wxString& nmea_sentence);
    
    /**
     * Queue status
     */
    size_t GetQueueSize() const;
    void ClearQueue();
    
    /**
     * Statistics
     */
    unsigned long GetMessagesSent() const { return m_messagesSent; }
    unsigned long GetMessagesQueued() const { return m_messagesQueued; }
    unsigned long GetConnectionErrors() const { return m_connectionErrors; }
    void ResetStatistics();

private:
    // Event handlers
    void OnSocketEvent(wxSocketEvent& event);
    void OnReconnectTimer(wxTimerEvent& event);
    void OnProcessQueueTimer(wxTimerEvent& event);
    
    // Internal methods
    void ProcessOutgoingQueue();
    bool InternalSendMessage(const wxString& message);
    void HandleConnectionError();
    void StartReconnectTimer();
    void StopReconnectTimer();
    
    // Connection state
    enum ConnectionState {
        STATE_DISCONNECTED,
        STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_ERROR
    };
    
    ConnectionState m_connectionState;
    wxSocketClient* m_socket;
    wxString m_host;
    int m_port;
    
    // Reconnection handling
    wxTimer* m_reconnectTimer;
    wxTimer* m_queueProcessTimer;
    bool m_autoReconnect;
    int m_reconnectInterval; // seconds
    
    // Message queue (thread-safe)
    mutable wxMutex m_queueMutex;
    std::queue<wxString> m_messageQueue;
    static const size_t MAX_QUEUE_SIZE = 1000;
    
    // Statistics
    unsigned long m_messagesSent;
    unsigned long m_messagesQueued;
    unsigned long m_connectionErrors;
    
    // Socket event IDs
    enum {
        SOCKET_ID = 1000,
        RECONNECT_TIMER_ID = 1001,
        QUEUE_TIMER_ID = 1002
    };
    
    DECLARE_EVENT_TABLE()
};

#endif // __NMEA_TCP_OUTPUTCONNECTION_H__