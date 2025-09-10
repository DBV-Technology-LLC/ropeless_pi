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

#include "NMEA_TCP_OutputConnection.h"
#include "nmea0183/nmea0183.h"

#include <wx/log.h>

// Event table
wxBEGIN_EVENT_TABLE(NMEA_TCP_OutputConnection, wxEvtHandler)
    EVT_SOCKET(SOCKET_ID, NMEA_TCP_OutputConnection::OnSocketEvent)
    EVT_TIMER(RECONNECT_TIMER_ID, NMEA_TCP_OutputConnection::OnReconnectTimer)
    EVT_TIMER(QUEUE_TIMER_ID, NMEA_TCP_OutputConnection::OnProcessQueueTimer)
wxEND_EVENT_TABLE()

NMEA_TCP_OutputConnection::NMEA_TCP_OutputConnection(const wxString& host, int port)
    : m_connectionState(STATE_DISCONNECTED)
    , m_socket(nullptr)
    , m_host(host)
    , m_port(port)
    , m_autoReconnect(true)
    , m_reconnectInterval(5)
    , m_messagesSent(0)
    , m_messagesQueued(0)
    , m_connectionErrors(0)
{
    // Create timers with proper event handlers
    m_reconnectTimer = new wxTimer(this, RECONNECT_TIMER_ID);
    m_queueProcessTimer = new wxTimer(this, QUEUE_TIMER_ID);
    
    // DON'T start timer in constructor - let caller start it when ready
    // m_queueProcessTimer->Start(100);
}

NMEA_TCP_OutputConnection::~NMEA_TCP_OutputConnection()
{
    // CRITICAL: Stop all timers and clear event handlers first
    try {
        if (m_reconnectTimer) {
            if (m_reconnectTimer->IsRunning()) {
                m_reconnectTimer->Stop();
            }
            // Clear the event handler to prevent callbacks to destroyed object
            m_reconnectTimer->SetOwner(nullptr);
            delete m_reconnectTimer;
            m_reconnectTimer = nullptr;
        }
        
        if (m_queueProcessTimer) {
            if (m_queueProcessTimer->IsRunning()) {
                m_queueProcessTimer->Stop();
            }
            // Clear the event handler to prevent callbacks to destroyed object  
            m_queueProcessTimer->SetOwner(nullptr);
            delete m_queueProcessTimer;
            m_queueProcessTimer = nullptr;
        }
        
        // Now safely disconnect socket
        Disconnect();
        
        // Final queue cleanup
        ClearQueue();
        
        // Force event processing to clear any pending events
        wxSafeYield();
        
    } catch (...) {
        // Force cleanup even if exceptions occur
        if (m_reconnectTimer) {
            try { delete m_reconnectTimer; } catch (...) {}
            m_reconnectTimer = nullptr;
        }
        if (m_queueProcessTimer) {
            try { delete m_queueProcessTimer; } catch (...) {}
            m_queueProcessTimer = nullptr;
        }
        m_socket = nullptr;
    }
}

bool NMEA_TCP_OutputConnection::Connect()
{
    if (m_connectionState == STATE_CONNECTED) {
        return true;
    }

    // Always ensure a socket exists
    if (!m_socket || m_connectionState == STATE_ERROR) {
        if (m_socket) {
            m_socket->Destroy();
            m_socket = nullptr;
        }

        m_socket = new wxSocketClient(wxSOCKET_NOWAIT);
        m_socket->SetEventHandler(*this, SOCKET_ID);
        m_socket->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG | wxSOCKET_OUTPUT_FLAG);
        m_socket->Notify(true);
    }

    // Build address
    wxIPV4address addr;
    if (!addr.Hostname(m_host) || !addr.Service(m_port)) {
        wxLogError("NMEA TCP Output: Invalid host/port: %s:%d", m_host.mb_str(), m_port);
        m_connectionState = STATE_ERROR;
        m_connectionErrors++;
        if (m_socket) { m_socket->Destroy(); m_socket = nullptr; }
        return false;
    }

    // Attempt connection
    m_connectionState = STATE_CONNECTING;
    bool result = m_socket->Connect(addr, false); // Non-blocking

    if (!result && m_socket->LastError() != wxSOCKET_WOULDBLOCK) {
        wxLogError("NMEA TCP Output: Connection failed to %s:%d - Error: %d",
                   m_host.mb_str(), m_port, m_socket->LastError());
        HandleConnectionError();
        return false;
    }

    return true;
}

void NMEA_TCP_OutputConnection::Disconnect()
{
    StopReconnectTimer();
    
    if (m_queueProcessTimer && m_queueProcessTimer->IsRunning()) {
        m_queueProcessTimer->Stop();
    }
    
    if (m_socket) {
        try {
            // Disable socket notifications first to prevent events during cleanup
            m_socket->Notify(false);
            m_socket->SetEventHandler(*this, wxID_ANY);  // Clear event handler
            
            if (m_connectionState == STATE_CONNECTED) {
                m_socket->Close();
            }
            m_socket->Destroy();
        } catch (...) {
            // Force cleanup if socket operations fail
            wxLogMessage("NMEA TCP Output: Exception during socket cleanup, forcing disconnect");
        }
        m_socket = nullptr;
    }
    
    // Clear any remaining queued messages
    ClearQueue();
    
    m_connectionState = STATE_DISCONNECTED;
    wxLogMessage("NMEA TCP Output: Disconnected from %s:%d", m_host, m_port);
}

bool NMEA_TCP_OutputConnection::IsConnected() const
{
    return m_connectionState == STATE_CONNECTED && m_socket && m_socket->IsConnected();
}

bool NMEA_TCP_OutputConnection::IsConnecting() const
{
    return m_connectionState == STATE_CONNECTING;
}

bool NMEA_TCP_OutputConnection::SendRawNMEA(const wxString& nmea_sentence)
{
    if (nmea_sentence.IsEmpty()) {
        return false;
    }
    
    // Ensure the sentence has proper line endings
    wxString message = nmea_sentence;
    if (!message.EndsWith("\r\n")) {
        if (message.EndsWith("\r") || message.EndsWith("\n")) {
            // Replace single line ending with CRLF
            message = message.Left(message.Length() - 1) + "\r\n";
        } else {
            // Add CRLF
            message += "\r\n";
        }
    }
    
    wxLogMessage("Connection state = " + m_connectionState);

    // Try to establish connection if not connected
    if (!IsConnected() && m_autoReconnect) {

        // Force a re-connect here. but the state won't change due to non block connect
        // will have to retry once connection is up
        Connect();

        return false;
    }

    wxLogMessage("Connected sending to queue!");


    // Add to queue
    wxMutexLocker lock(m_queueMutex);
    
    // Check queue size limit
    if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
        wxLogWarning("NMEA TCP Output: Message queue full, dropping oldest message");
        m_messageQueue.pop();
    }
    
    m_messageQueue.push(message);
    m_messagesQueued++;
    
    return true;
}

size_t NMEA_TCP_OutputConnection::GetQueueSize() const
{
    wxMutexLocker lock(m_queueMutex);
    return m_messageQueue.size();
}

void NMEA_TCP_OutputConnection::ClearQueue()
{
    wxMutexLocker lock(m_queueMutex);
    while (!m_messageQueue.empty()) {
        m_messageQueue.pop();
    }
}

void NMEA_TCP_OutputConnection::ResetStatistics()
{
    m_messagesSent = 0;
    m_messagesQueued = 0;
    m_connectionErrors = 0;
}

void NMEA_TCP_OutputConnection::OnSocketEvent(wxSocketEvent& event)
{
    switch (event.GetSocketEvent()) {
        case wxSOCKET_CONNECTION:
            // wxLogMessage("NMEA TCP Output: Connected to %s:%d", m_host, m_port);
            m_connectionState = STATE_CONNECTED;
            StopReconnectTimer();
            ProcessOutgoingQueue(); // Send any queued messages
            break;
            
        case wxSOCKET_LOST:
            // wxLogMessage("NMEA TCP Output: Connection lost to %s:%d", m_host, m_port);
            HandleConnectionError();
            break;
            
        case wxSOCKET_OUTPUT:
            // Socket is ready for more data
            ProcessOutgoingQueue();
            break;
            
        default:
            break;
    }
}

void NMEA_TCP_OutputConnection::OnReconnectTimer(wxTimerEvent& event)
{
    if (m_connectionState != STATE_CONNECTED && m_autoReconnect) {
        // wxLogMessage("NMEA TCP Output: Attempting to reconnect to %s:%d", m_host, m_port);
        Connect();
    }
}

void NMEA_TCP_OutputConnection::OnProcessQueueTimer(wxTimerEvent& event)
{
    if (IsConnected()) {
        ProcessOutgoingQueue();
    }
}

void NMEA_TCP_OutputConnection::ProcessOutgoingQueue()
{
    if (!IsConnected()) {
        return;
    }
    
    wxMutexLocker lock(m_queueMutex);
    
    // Process messages from queue
    while (!m_messageQueue.empty() && m_socket->IsConnected() && !m_socket->Error()) {
        const wxString& message = m_messageQueue.front();
        
        if (InternalSendMessage(message)) {
            m_messageQueue.pop();
            m_messagesSent++;
        } else {
            // Failed to send, stop processing and handle error
            break;
        }
    }
}

bool NMEA_TCP_OutputConnection::InternalSendMessage(const wxString& message)
{
    if (!m_socket || !m_socket->IsConnected()) {
        return false;
    }
    
    const char* data = message.mb_str();
    size_t len = strlen(data);
    
    m_socket->Write(data, len);
    
    if (m_socket->Error()) {
        wxLogError("NMEA TCP Output: Send error: %d", m_socket->LastError());
        HandleConnectionError();
        return false;
    }
    
    size_t sent = m_socket->LastCount();
    if (sent != len) {
        wxLogWarning("NMEA TCP Output: Partial send: %zu of %zu bytes", sent, len);
        // For simplicity, we'll consider partial sends as failures
        // In production code, you might want to handle partial sends
        return false;
    }
    
    return true;
}

void NMEA_TCP_OutputConnection::HandleConnectionError()
{
    m_connectionState = STATE_ERROR;
    m_connectionErrors++;
    
    if (m_socket) {
        m_socket->Close();
    }
    
    if (m_autoReconnect) {
        StartReconnectTimer();
    }
}

void NMEA_TCP_OutputConnection::StartReconnectTimer()
{
    if (m_reconnectTimer && m_reconnectInterval > 0) {
        m_reconnectTimer->Start(m_reconnectInterval * 1000, wxTIMER_ONE_SHOT);
    }
}

void NMEA_TCP_OutputConnection::StopReconnectTimer()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->Stop();
    }
}

void NMEA_TCP_OutputConnection::StartQueueProcessing()
{
    if (m_queueProcessTimer && !m_queueProcessTimer->IsRunning()) {
        m_queueProcessTimer->Start(100);
    }
}