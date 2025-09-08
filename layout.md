# RopelessDialog UI Layout

This document shows the visual layout structure of the RopelessDialog as defined in RopelessDialog.cpp.

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                RopelessDialog                                       │
│                              (Overall Vertical)                                     │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────────┐ │
│ │                          Main Content (Horizontal)                              │ │
│ │                                                                                 │ │
│ │ ┌─────────────────────────────────┐ ┌─────────────────────────────────────────┐ │ │
│ │ │        Main Content             │ │              Sidebar                    │ │ │
│ │ │        (Vertical)               │ │             (Vertical)                  │ │ │
│ │ │                                 │ │                                         │ │ │
│ │ │ ┌─────────────────────────────┐ │ │ ┌─────────────────────────────────────┐ │ │ │
│ │ │ │                             │ │ │ │    Selected Transponder Label       │ │ │ │
│ │ │ │      Transponder List       │ │ │ │         (Header Text)               │ │ │ │
│ │ │ │     (OCPNListCtrl)          │ │ │ └─────────────────────────────────────┘ │ │ │
│ │ │ │                             │ │ │                                         │ │ │
│ │ │ │   - Color                   │ │ │ ┌─────────────────────────────────────┐ │ │ │
│ │ │ │   - ID                      │ │ │ │         Notebook Tabs               │ │ │ │
│ │ │ │   - Release Status          │ │ │ │                                     │ │ │ │
│ │ │ │   - LastReportTime (UTC)    │ │ │ │ ┌─────┬─────────┬──────────┐        │ │ │ │
│ │ │ │   - Range, M                │ │ │ │ │Info │ Status  │ Position │        │ │ │ │
│ │ │ │   - Recovered Status        │ │ │ │ └─────┴─────────┴──────────┘        │ │ │ │
│ │ │ │                             │ │ │ │                                     │ │ │ │
│ │ │ └─────────────────────────────┘ │ │ │ Info Tab:                           │ │ │ │
│ │ └─────────────────────────────────┘ │ │ │ - ID: ---                         │ │ │ │
│ └─────────────────────────────────────┘ │ │ - Partner ID: ---                 │ │ │ │
│                                         │ │ - Manufacturer: ---               │ │ │ │
│                                         │ │ - Ownership: ---                  │ │ │ │
│                                         │ │ - Trawl ID: ---                   │ │ │ │
│                                         │ │ - Mark Type: ---                  │ │ │ │
│                                         │ │                                   │ │ │ │
│                                         │ │ Status Tab:                       │ │ │ │
│                                         │ │ - Release Status: ---             │ │ │ │
│                                         │ │ - Recovery Status: ---            │ │ │ │
│                                         │ │ - Battery: ---%                   │ │ │ │
│                                         │ │ - Pings: ---                      │ │ │ │
│                                         │ │ - Last Report: ---                │ │ │ │
│                                         │ │ - Position Source: ---            │ │ │ │
│                                         │ │                                   │ │ │ │
│                                         │ │ Position Tab:                     │ │ │ │
│                                         │ │ - Latitude: ---                   │ │ │ │
│                                         │ │ - Longitude: ---                  │ │ │ │
│                                         │ │ - Range: --- m                    │ │ │ │
│                                         │ │ - Bearing: ---°                   │ │ │ │
│                                         │ │ - Depth: --- m                    │ │ │ │
│                                         │ │ - Temperature: ---°C              │ │ │ │
│                                         │ └─────────────────────────────────────┘ │ │ 
│                                         │                                         │ │ 
│                                         │ ┌─────────────────────────────────────┐ │ │ 
│                                         │ │            Commands                 │ │ │ 
│                                         │ │                                     │ │ │ 
│                                         │ │  [Release] [Recover] [Delete]       │ │ │ 
│                                         │ │    [Mute]    [Sync]                 │ │ │ 
│                                         │ │ [Show On Map] [Manual Release]      │ │ │ 
│                                         │ └─────────────────────────────────────┘ │ │ 
│                                         │                                         │ │ 
│                                         │ ┌─────────────────────────────────────┐ │ │ 
│                                         │ │          Deckbox Status             │ │ │ 
│                                         │ │                                     │ │ │ 
│                                         │ │  Status: Ready                      │ │ │ 
│                                         │ │  TCP Connection: Disconnected       │ │ │ 
│                                         │ └─────────────────────────────────────┘ │ │ 
│                                         │                                         │ │ 
│                                         │ ┌─────────────────────────────────────┐ │ │ 
│                                         │ │         Release Status              │ │ │ 
│                                         │ │                                     │ │ │ 
│                                         │ │  Status: Standby                    │ │ │ 
│                                         │ └─────────────────────────────────────┘ │ │ 
│                                         └─────────────────────────────────────────┘ │ 
│                                                                                   │ │
│ └─────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────────┐ │
│ │                           Debug Messages                                        │ │
│ │                                                                                 │ │
│ │ ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│ │ │                        Text Control Area                                    │ │ │
│ │ │                      (Multiline, Read-only)                                 │ │ │
│ │ │                                                                             │ │ │
│ │ │  [Debug messages appear here...]                                            │ │ │
│ │ └─────────────────────────────────────────────────────────────────────────────┘ │ │
│ │                                                                                 │ │
│ │ ┌─────────────────────────────────────────────────────────────────────────────┐ │ │
│ │ │                        Debug Controls (Horizontal)                          │ │ │
│ │ │                                                                             │ │ │
│ │ │  [Clear]  ☑ Show NMEA  ☑ Show Debug                                      │ │ │
│ │ └─────────────────────────────────────────────────────────────────────────────┘ │ │
│ └─────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                     │
│ ┌─────────────────────────────────────────────────────────────────────────────────┐ │
│ │                        Standard Dialog Buttons                                  │ │
│ │                                                                                 │ │
│ │                                  [OK]                                           │ │
│ └─────────────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## Layout Hierarchy

### Main Structure (Vertical)
1. **Main Content Area (Horizontal)**
   - **Left Side: Transponder List** (OCPNListCtrl with columns)
   - **Right Side: Sidebar** (Fixed width, contains multiple sections)

2. **Debug Messages Section** (Full width)
   - Text control for debug output
   - Controls row with Clear button and checkboxes

3. **Dialog Buttons** (Standard OK button)

### Sidebar Components (Top to Bottom)
1. **Selected Transponder Header** - Shows currently selected transponder
2. **Notebook with 3 tabs:**
   - **Info Tab** - Transponder identification info
   - **Status Tab** - Operational status information  
   - **Position Tab** - Location and sensor data
3. **Commands Section** - Action buttons in 3 rows
4. **Deckbox Status** - Connection and system status
5. **Release Status** - Current release operation status

### Key Layout Features
- **Responsive Width**: Dialog fits content horizontally (min 900px)
- **Responsive Height**: Dialog uses `Fit()` to size to content vertically
- **Fixed Elements**: Sidebar has fixed 350px minimum width
- **Expandable**: Main transponder list expands to fill available space
- **Text Wrapping**: Long text fields use ellipsize for overflow handling

### Recent Changes
- Debug section now has horizontal layout with Clear button and two checkboxes
- Button text changed from "Clear Debug Messages" to just "Clear"
- Checkboxes default to checked state
- Controls are left-aligned within the debug section