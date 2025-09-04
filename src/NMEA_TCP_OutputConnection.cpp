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
    // Create timers
    m_reconnectTimer = new wxTimer(this, RECONNECT_TIMER_ID);
    m_queueProcessTimer = new wxTimer(this, QUEUE_TIMER_ID);
    
    // Start queue processing timer (process queue every 100ms)
    m_queueProcessTimer->Start(100);
}

NMEA_TCP_OutputConnection::~NMEA_TCP_OutputConnection()
{
    Disconnect();
    
    if (m_reconnectTimer) {
        m_reconnectTimer->Stop();
        delete m_reconnectTimer;
        m_reconnectTimer = nullptr;
    }
    
    if (m_queueProcessTimer) {
        m_queueProcessTimer->Stop();
        delete m_queueProcessTimer;
        m_queueProcessTimer = nullptr;
    }
    
    ClearQueue();
}

bool NMEA_TCP_OutputConnection::Connect()
{
    if (m_connectionState == STATE_CONNECTED || m_connectionState == STATE_CONNECTING) {
        return m_connectionState == STATE_CONNECTED;
    }
    
    // Clean up existing socket
    if (m_socket) {
        m_socket->Destroy();
        m_socket = nullptr;
    }
    
    // Create new socket
    m_socket = new wxSocketClient(wxSOCKET_NOWAIT);
    m_socket->SetEventHandler(*this, SOCKET_ID);
    m_socket->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG | wxSOCKET_OUTPUT_FLAG);
    m_socket->Notify(true);
    
    // Create address
    wxIPV4address addr;
    if (!addr.Hostname(m_host) || !addr.Service(m_port)) {
        wxLogError("NMEA TCP Output: Invalid host/port: %s:%d", m_host, m_port);
        m_connectionState = STATE_ERROR;
        m_connectionErrors++;
        return false;
    }
    
    // Attempt connection
    m_connectionState = STATE_CONNECTING;
    bool result = m_socket->Connect(addr, false); // Non-blocking connect
    
    if (!result && m_socket->LastError() != wxSOCKET_WOULDBLOCK) {
        wxLogError("NMEA TCP Output: Connection failed to %s:%d - Error: %d", 
                   m_host, m_port, m_socket->LastError());
        HandleConnectionError();
        return false;
    }
    
    // wxLogMessage("NMEA TCP Output: Connecting to %s:%d", m_host, m_port);
    return true;
}

void NMEA_TCP_OutputConnection::Disconnect()
{
    StopReconnectTimer();
    
    if (m_socket) {
        if (m_connectionState == STATE_CONNECTED) {
            m_socket->Close();
        }
        m_socket->Destroy();
        m_socket = nullptr;
    }
    
    m_connectionState = STATE_DISCONNECTED;
    // wxLogMessage("NMEA TCP Output: Disconnected from %s:%d", m_host, m_port);
}

bool NMEA_TCP_OutputConnection::IsConnected() const
{
    return m_connectionState == STATE_CONNECTED && m_socket && m_socket->IsConnected();
}

bool NMEA_TCP_OutputConnection::IsConnecting() const
{
    return m_connectionState == STATE_CONNECTING;
}

bool NMEA_TCP_OutputConnection::SendNMEAMessage(RESPONSE* nmea_message)
{
    if (!nmea_message) {
        return false;
    }
    
    // Create sentence and write message to it
    SENTENCE sentence;
    if (!nmea_message->Write(sentence)) {
        wxLogError("NMEA TCP Output: Failed to write message to sentence");
        return false;
    }
    
    // Send the sentence
    return SendRawNMEA(sentence.Sentence);
}

bool NMEA_TCP_OutputConnection::SendDBS(const DBS& dbs_msg)
{
    DBS dbs_copy = dbs_msg;  // Make a copy since Write() is not const
    return SendNMEAMessage(&dbs_copy);
}

bool NMEA_TCP_OutputConnection::SendGML(const GML& gml_msg)
{
    GML gml_copy = gml_msg;  // Make a copy since Write() is not const
    return SendNMEAMessage(&gml_copy);
}

bool NMEA_TCP_OutputConnection::SendGMS(const GMS& gms_msg)
{
    GMS gms_copy = gms_msg;  // Make a copy since Write() is not const
    return SendNMEAMessage(&gms_copy);
}

bool NMEA_TCP_OutputConnection::SendGMR(const GMR& gmr_msg)
{
    GMR gmr_copy = gmr_msg;  // Make a copy since Write() is not const
    return SendNMEAMessage(&gmr_copy);
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
    
    // Add to queue
    {
        wxMutexLocker lock(m_queueMutex);
        
        // Check queue size limit
        if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
            wxLogWarning("NMEA TCP Output: Message queue full, dropping oldest message");
            m_messageQueue.pop();
        }
        
        m_messageQueue.push(message);
        m_messagesQueued++;
    }
    
    // Try to establish connection if not connected
    if (!IsConnected() && !IsConnecting() && m_autoReconnect) {
        Connect();
    }
    
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