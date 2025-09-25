# ropeless_pi README
This is a plugin for OpenCPN to support the integration of Ropeless Fishing equipment directly into a chartplotter. The plugin enables fisherman to identify, position, and release RSI MTAs solely through a chartplotter interface with standardized NMEA messages.

## Debugging Crashes (linux)

- ./debug_opencpn.sh
- (gdb) run
- .... wait for crash / segfault...
- (gdb) backtrace

Note: make sure to build RelWithDebInfo..

## v3.x Features

9/24/2025
[x] - Saving position of Plugin on close
[x] - Merged old / new transponder fields (range,id etc..)
[x] - Updating Transponder color scheme to be handled by render -- not saved with Transponder info
[x] - Adding hide transponder text to preferences
[x] - Adding edit transponder to right click on transponder
[x] - Updating Table Color marking and Transponder color change on select / deselect faster
[x] - Trawl line back to Black line
[x] - Draw black dot in center of transponders in trawls. Back x on end of trawls
[x] - Mark Type enum strings
[x] - Added trawl pos field to manual placement

9/24/2025
[x] - Right click to edit transponder on map. ONLY if USER marking
[x] - Right click on transponder show manuf/sn/trawl_id
[x] - Ownership: "Owned" / "Non-Owned" in info tab
[x] - Fixed trawl loading after startup from xml transponders
[x] - Fix Sorting Issues
[x] - Draw Square for USER positions -- X needs to be drawn bigger due to shape change
[x] - Set Preference defaults properly

9/25/2025
[x] - Removed extra comments from sorting function
[x] - commented out "Mute" button command in GUI / Event
[x] - commented out CTRL+M commadn for "Mute" in accelerator table
[x] - spacer next to command buttons to center
[x] - added is_selected field to trawl_tracker object (for highlighting)
[x] - Advanced button added next to Help
[x] - Added Delete All button w/ stub to Advanced pop-up
[x] - Tweaked trawl list GUI
[x] - Hide / show the Trawl List and Debug sizers

[x] - Showing Transonders in trawl view now
[x] - Highlighting Trawl line when trawl is selected



## TODO:


TEST
1. Add transponder to trawl at position 3
1. Add transponder to trawl at position 2
1. Add transponder to trawl at position 1
1. Add transponder to trawl at position 5

[] - Allow inserting transponder into Trawl list at specific Position
[] - Do not draw line unless transponders are consecutive in trawl

[] - Split trawl_id address space for USER defined so it doesn't clash with Deckbox assigned IDs
[] - Show trawl pos in status
[] - Trawl View show all transponders in trawl
[] - Trawl View Delete Trawl

Bugs:
[] - When sorting the list with a selected item it doesn't get "deselected" and color stays GOLDEN
[] - List scrolls back up every refresh
[] - Trawl gets un selected when transponder list is sorted?

Maybe Later:
[] - Support trawl_path lat/lon list
[] - Add update flag to transponder obj to make rendering faster for non changing transponders ?
[] - Create separate log file for NMEA only messages

Maybe Never:
[] - Fix text cutoff in "Transponder Status" Tab? -- commented out for now
[] - Trawl creation time tag -- Can just use creation of earliest transponder. plus would have to save to xml?

Before Release
- Remove position field from manual placement
- Remove unused commands "Mute"
- Update defaults in preferences
- Hide trawl view
- Don't allow manual placement
- Hide Debug window by default

Things to Test
- Scrolling in table list 
- colorblind mode

Preferences / Defaults
- Accessibility
	- Colorblind Mode : false
- Data Connections
	- Enable TCP Output
	- Host
	- Port
	- Auto-reconnect
- Advanced Options
	- Enable Debug
	- Enable Simulation
- Display Options
	- Hide Transponder Text 
	- Show non-owned
	- Show cloud
	- Hide recovered units
	- Timeout cloud pos
	- Cloud radius
	- Circle size
	- Text size

## Users

Install OpenCPN 5.8.4 or later

Import released plugins via Settings->Plugins->Import Plugin

### General

- Log file in Windows : C:\ProgramData\opencpn\opencpn.log

## Copyright and licensing

This software is Copyright (c) David Register and Ropeless Systems 2024. It is distributed
under the terms of the Gnu Public License version 3 or, at your option,
any later version. See the file COPYING for details.
