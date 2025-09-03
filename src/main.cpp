#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/statbox.h>
#include <wx/gauge.h>
#include <wx/timer.h>
#include <vector>
#include <map>

// Transponder data structure
struct Transponder {
    wxString id;
    wxString name;
    wxString type;
    wxString frequency;
    wxString status;
    int signal;  // dBm, -999 for no signal
    int battery; // percentage
    wxString location;
    wxString lastSeen;
    wxString firmware;
    double temperature; // Celsius, -999 for N/A
    int humidity; // percentage, -1 for N/A
    
    Transponder(const wxString& _id, const wxString& _name, const wxString& _type,
                const wxString& _freq, const wxString& _status, int _signal, int _battery,
                const wxString& _loc, const wxString& _lastSeen, const wxString& _fw,
                double _temp = -999, int _humid = -1)
        : id(_id), name(_name), type(_type), frequency(_freq), status(_status),
          signal(_signal), battery(_battery), location(_loc), lastSeen(_lastSeen),
          firmware(_fw), temperature(_temp), humidity(_humid) {}
};

class TransponderPanel : public wxPanel {
public:
    TransponderPanel(wxWindow* parent);
    
private:
    // UI Components
    wxListCtrl* m_transponderList;
    wxPanel* m_detailPanel;
    wxSplitterWindow* m_splitter;
    
    // Detail panel components
    wxStaticText* m_deviceName;
    wxStaticText* m_deviceId;
    wxStaticText* m_statusText;
    wxStaticText* m_signalText;
    wxStaticText* m_batteryText;
    wxStaticText* m_lastSeenText;
    wxStaticText* m_typeText;
    wxStaticText* m_frequencyText;
    wxStaticText* m_firmwareText;
    wxStaticText* m_locationText;
    wxStaticText* m_temperatureText;
    wxStaticText* m_humidityText;
    wxGauge* m_batteryGauge;
    
    // Command buttons
    wxButton* m_pingBtn;
    wxButton* m_rebootBtn;
    wxButton* m_calibrateBtn;
    wxButton* m_shutdownBtn;
    
    // Data
    std::vector<Transponder> m_transponders;
    int m_selectedIndex;
    
    // Timer for updates
    wxTimer* m_updateTimer;
    
    // Event handlers
    void OnTransponderSelected(wxListEvent& event);
    void OnPingCommand(wxCommandEvent& event);
    void OnRebootCommand(wxCommandEvent& event);
    void OnCalibrateCommand(wxCommandEvent& event);
    void OnShutdownCommand(wxCommandEvent& event);
    void OnUpdateTimer(wxTimerEvent& event);
    
    // Helper functions
    void InitializeData();
    void PopulateList();
    void UpdateDetailPanel();
    wxColour GetStatusColor(const wxString& status);
    wxString GetSignalStrength(int signal);
    void CreateDetailPanel();
    void ExecuteCommand(const wxString& command);
    
    DECLARE_EVENT_TABLE()
};

// Event table
wxBEGIN_EVENT_TABLE(TransponderPanel, wxPanel)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, TransponderPanel::OnTransponderSelected)
    EVT_BUTTON(1001, TransponderPanel::OnPingCommand)
    EVT_BUTTON(1002, TransponderPanel::OnRebootCommand)
    EVT_BUTTON(1003, TransponderPanel::OnCalibrateCommand)
    EVT_BUTTON(1004, TransponderPanel::OnShutdownCommand)
    EVT_TIMER(2001, TransponderPanel::OnUpdateTimer)
wxEND_EVENT_TABLE()

TransponderPanel::TransponderPanel(wxWindow* parent)
    : wxPanel(parent), m_selectedIndex(-1) {
    
    // Initialize data
    InitializeData();
    
    // Create splitter window
    m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    
    // Create transponder list
    wxPanel* listPanel = new wxPanel(m_splitter);
    m_transponderList = new wxListCtrl(listPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_SINGLE_SEL);
    
    // Setup list columns
    m_transponderList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
    m_transponderList->AppendColumn("ID", wxLIST_FORMAT_LEFT, 80);
    m_transponderList->AppendColumn("Status", wxLIST_FORMAT_CENTER, 80);
    m_transponderList->AppendColumn("Signal", wxLIST_FORMAT_RIGHT, 70);
    m_transponderList->AppendColumn("Battery", wxLIST_FORMAT_RIGHT, 70);
    m_transponderList->AppendColumn("Type", wxLIST_FORMAT_LEFT, 120);
    m_transponderList->AppendColumn("Location", wxLIST_FORMAT_LEFT, 100);
    
    // Layout for list panel
    wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* title = new wxStaticText(listPanel, wxID_ANY, "Transponder Network");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    
    listSizer->Add(title, 0, wxALL, 10);
    listSizer->Add(m_transponderList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    listPanel->SetSizer(listSizer);
    
    // Create detail panel
    CreateDetailPanel();
    
    // Setup splitter
    m_splitter->SplitVertically(listPanel, m_detailPanel, -350);
    m_splitter->SetMinimumPaneSize(200);
    
    // Main layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_splitter, 1, wxEXPAND);
    SetSizer(mainSizer);
    
    // Populate the list
    PopulateList();
    
    // Setup update timer (every 5 seconds)
    m_updateTimer = new wxTimer(this, 2001);
    m_updateTimer->Start(5000);
}

void TransponderPanel::InitializeData() {
    m_transponders = {
        Transponder("TXP-001", "Acoustic Sensor Alpha", "Ultrasonic", "40kHz", 
                   "Active", -45, 87, "Sector A1", "2 min ago", "v2.3.1", 23.5, 45),
        Transponder("TXP-002", "Hydrophone Beta", "Hydrophone", "20kHz", 
                   "Warning", -52, 34, "Sector B2", "5 min ago", "v2.2.8", 21.8, 52),
        Transponder("TXP-003", "Sonar Gamma", "Sonar", "200kHz", 
                   "Offline", -999, 0, "Sector C1", "2 hours ago", "v2.1.5"),
        Transponder("TXP-004", "Acoustic Array Delta", "Phased Array", "1-100kHz", 
                   "Active", -38, 92, "Sector D3", "30 sec ago", "v2.4.0", 24.2, 41),
        Transponder("TXP-005", "Piezo Transducer Echo", "Piezoelectric", "5MHz", 
                   "Maintenance", -48, 78, "Sector A3", "1 min ago", "v2.3.2", 22.9, 38)
    };
}

void TransponderPanel::CreateDetailPanel() {
    m_detailPanel = new wxPanel(m_splitter);
    
    // Create static box for grouping
    wxStaticBox* statusBox = new wxStaticBox(m_detailPanel, wxID_ANY, "Device Information");
    wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(statusBox, wxVERTICAL);
    
    // Device header
    m_deviceName = new wxStaticText(m_detailPanel, wxID_ANY, "No Device Selected");
    wxFont nameFont = m_deviceName->GetFont();
    nameFont.SetPointSize(nameFont.GetPointSize() + 1);
    nameFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_deviceName->SetFont(nameFont);
    
    m_deviceId = new wxStaticText(m_detailPanel, wxID_ANY, "");
    
    statusSizer->Add(m_deviceName, 0, wxALL, 5);
    statusSizer->Add(m_deviceId, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Status information
    wxFlexGridSizer* infoSizer = new wxFlexGridSizer(2, 5, 5);
    infoSizer->AddGrowableCol(1);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Status:"), 0, wxALIGN_LEFT);
    m_statusText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_statusText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Signal:"), 0, wxALIGN_LEFT);
    m_signalText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_signalText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Battery:"), 0, wxALIGN_LEFT);
    m_batteryText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_batteryText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Last Seen:"), 0, wxALIGN_LEFT);
    m_lastSeenText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_lastSeenText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Type:"), 0, wxALIGN_LEFT);
    m_typeText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_typeText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Frequency:"), 0, wxALIGN_LEFT);
    m_frequencyText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_frequencyText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Firmware:"), 0, wxALIGN_LEFT);
    m_firmwareText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_firmwareText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Location:"), 0, wxALIGN_LEFT);
    m_locationText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_locationText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Temperature:"), 0, wxALIGN_LEFT);
    m_temperatureText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_temperatureText, 0, wxEXPAND);
    
    infoSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Humidity:"), 0, wxALIGN_LEFT);
    m_humidityText = new wxStaticText(m_detailPanel, wxID_ANY, "");
    infoSizer->Add(m_humidityText, 0, wxEXPAND);
    
    statusSizer->Add(infoSizer, 0, wxEXPAND | wxALL, 5);
    
    // Battery gauge
    statusSizer->Add(new wxStaticText(m_detailPanel, wxID_ANY, "Battery Level:"), 0, wxLEFT | wxTOP, 5);
    m_batteryGauge = new wxGauge(m_detailPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 20));
    statusSizer->Add(m_batteryGauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Command buttons
    wxStaticBox* cmdBox = new wxStaticBox(m_detailPanel, wxID_ANY, "Commands");
    wxStaticBoxSizer* cmdSizer = new wxStaticBoxSizer(cmdBox, wxVERTICAL);
    
    wxGridSizer* buttonSizer = new wxGridSizer(2, 2, 5, 5);
    
    m_pingBtn = new wxButton(m_detailPanel, 1001, "Ping");
    m_rebootBtn = new wxButton(m_detailPanel, 1002, "Reboot");
    m_calibrateBtn = new wxButton(m_detailPanel, 1003, "Calibrate");
    m_shutdownBtn = new wxButton(m_detailPanel, 1004, "Shutdown");
    
    buttonSizer->Add(m_pingBtn, 0, wxEXPAND);
    buttonSizer->Add(m_rebootBtn, 0, wxEXPAND);
    buttonSizer->Add(m_calibrateBtn, 0, wxEXPAND);
    buttonSizer->Add(m_shutdownBtn, 0, wxEXPAND);
    
    cmdSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    // Main detail panel layout
    wxBoxSizer* detailSizer = new wxBoxSizer(wxVERTICAL);
    detailSizer->Add(statusSizer, 0, wxEXPAND | wxALL, 10);
    detailSizer->Add(cmdSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    m_detailPanel->SetSizer(detailSizer);
    
    // Initially disable command buttons
    m_pingBtn->Enable(false);
    m_rebootBtn->Enable(false);
    m_calibrateBtn->Enable(false);
    m_shutdownBtn->Enable(false);
}

void TransponderPanel::PopulateList() {
    m_transponderList->DeleteAllItems();
    
    for (size_t i = 0; i < m_transponders.size(); ++i) {
        const auto& trans = m_transponders[i];
        
        long index = m_transponderList->InsertItem(i, trans.name);
        m_transponderList->SetItem(index, 1, trans.id);
        m_transponderList->SetItem(index, 2, trans.status);
        
        wxString signalStr = (trans.signal == -999) ? "No Signal" : 
                           wxString::Format("%d dBm", trans.signal);
        m_transponderList->SetItem(index, 3, signalStr);
        
        m_transponderList->SetItem(index, 4, wxString::Format("%d%%", trans.battery));
        m_transponderList->SetItem(index, 5, trans.type);
        m_transponderList->SetItem(index, 6, trans.location);
        
        // Color code by status
        wxColour color = GetStatusColor(trans.status);
        m_transponderList->SetItemTextColour(index, color);
    }
}

void TransponderPanel::UpdateDetailPanel() {
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_transponders.size()) {
        m_deviceName->SetLabel("No Device Selected");
        m_deviceId->SetLabel("");
        // Clear all other fields...
        m_pingBtn->Enable(false);
        m_rebootBtn->Enable(false);
        m_calibrateBtn->Enable(false);
        m_shutdownBtn->Enable(false);
        return;
    }
    
    const auto& trans = m_transponders[m_selectedIndex];
    
    m_deviceName->SetLabel(trans.name);
    m_deviceId->SetLabel(trans.id);
    m_statusText->SetLabel(trans.status);
    m_statusText->SetForegroundColour(GetStatusColor(trans.status));
    
    wxString signalStr = (trans.signal == -999) ? "No Signal" : 
                        wxString::Format("%d dBm (%s)", trans.signal, GetSignalStrength(trans.signal));
    m_signalText->SetLabel(signalStr);
    
    m_batteryText->SetLabel(wxString::Format("%d%%", trans.battery));
    m_lastSeenText->SetLabel(trans.lastSeen);
    m_typeText->SetLabel(trans.type);
    m_frequencyText->SetLabel(trans.frequency);
    m_firmwareText->SetLabel(trans.firmware);
    m_locationText->SetLabel(trans.location);
    
    wxString tempStr = (trans.temperature == -999) ? "N/A" : 
                      wxString::Format("%.1f°C", trans.temperature);
    m_temperatureText->SetLabel(tempStr);
    
    wxString humidStr = (trans.humidity == -1) ? "N/A" : 
                       wxString::Format("%d%%", trans.humidity);
    m_humidityText->SetLabel(humidStr);
    
    m_batteryGauge->SetValue(trans.battery);
    
    // Enable/disable buttons based on status
    bool isOnline = (trans.status != "Offline");
    m_pingBtn->Enable(true);  // Ping always available
    m_rebootBtn->Enable(isOnline);
    m_calibrateBtn->Enable(isOnline);
    m_shutdownBtn->Enable(isOnline);
    
    m_detailPanel->Layout();
}

wxColour TransponderPanel::GetStatusColor(const wxString& status) {
    if (status == "Active") return wxColour(0, 128, 0);      // Green
    if (status == "Warning") return wxColour(255, 140, 0);   // Orange
    if (status == "Offline") return wxColour(255, 0, 0);     // Red
    if (status == "Maintenance") return wxColour(0, 0, 255); // Blue
    return wxColour(0, 0, 0); // Black
}

wxString TransponderPanel::GetSignalStrength(int signal) {
    if (signal == -999) return "No Signal";
    if (signal > -40) return "Excellent";
    if (signal > -50) return "Good";
    if (signal > -60) return "Fair";
    return "Poor";
}

void TransponderPanel::OnTransponderSelected(wxListEvent& event) {
    m_selectedIndex = event.GetIndex();
    UpdateDetailPanel();
}

void TransponderPanel::OnPingCommand(wxCommandEvent& event) {
    ExecuteCommand("Ping");
}

void TransponderPanel::OnRebootCommand(wxCommandEvent& event) {
    ExecuteCommand("Reboot");
}

void TransponderPanel::OnCalibrateCommand(wxCommandEvent& event) {
    ExecuteCommand("Calibrate");
}

void TransponderPanel::OnShutdownCommand(wxCommandEvent& event) {
    ExecuteCommand("Shutdown");
}

void TransponderPanel::ExecuteCommand(const wxString& command) {
    if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_transponders.size()) {
        return;
    }
    
    const auto& trans = m_transponders[m_selectedIndex];
    wxString msg = wxString::Format("Executing %s command on %s (%s)", 
                                   command, trans.name, trans.id);
    
    wxMessageBox(msg, "Command Execution", wxOK | wxICON_INFORMATION);
    
    // Simulate command execution (you would replace this with actual command logic)
    if (command == "Reboot") {
        m_transponders[m_selectedIndex].status = "Maintenance";
        m_transponders[m_selectedIndex].lastSeen = "Rebooting...";
        PopulateList();
        UpdateDetailPanel();
    }
}

void TransponderPanel::OnUpdateTimer(wxTimerEvent& event) {
    // Simulate data updates
    // In a real application, this would query your hardware/database
    PopulateList();
    UpdateDetailPanel();
}

// Main frame class
class TransponderFrame : public wxFrame {
public:
    TransponderFrame() : wxFrame(nullptr, wxID_ANY, "Transponder Management System") {
        SetSize(1200, 800);
        
        TransponderPanel* panel = new TransponderPanel(this);
        
        // Create status bar
        CreateStatusBar();
        SetStatusText("Ready - Monitoring 5 transponders");
        
        // Create menu bar
        wxMenuBar* menuBar = new wxMenuBar;
        wxMenu* fileMenu = new wxMenu;
        fileMenu->Append(wxID_EXIT, "E&xit\tAlt-X", "Quit this program");
        
        wxMenu* helpMenu = new wxMenu;
        helpMenu->Append(wxID_ABOUT, "&About\tF1", "Show about dialog");
        
        menuBar->Append(fileMenu, "&File");
        menuBar->Append(helpMenu, "&Help");
        SetMenuBar(menuBar);
        
        Bind(wxEVT_MENU, &TransponderFrame::OnExit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &TransponderFrame::OnAbout, this, wxID_ABOUT);
    }
    
private:
    void OnExit(wxCommandEvent& event) {
        Close(true);
    }
    
    void OnAbout(wxCommandEvent& event) {
        wxMessageBox("Transponder Management System\nBuilt with wxWidgets",
                    "About", wxOK | wxICON_INFORMATION);
    }
};

// Application class
class TransponderApp : public wxApp {
public:
    bool OnInit() override {
        TransponderFrame* frame = new TransponderFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TransponderApp);