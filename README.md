# ropeless_pi README
This is a plugin for OpenCPN to support the integration of Ropeless Fishing equipment directly into a chartplotter. The plugin enables fisherman to identify, position, and release RSI MTAs solely through a chartplotter interface with standardized NMEA messages.

Last Updated: 9/25/2025 CTV

## v3.x Features

9/24/2025
- [x] Saving position of Plugin on close
- [x] Merged old / new transponder fields (range,id etc..)
- [x] Updating Transponder color scheme to be handled by render -- not saved with Transponder info
- [x] Adding hide transponder text to preferences
- [x] Adding edit transponder to right click on transponder
- [x] Updating Table Color marking and Transponder color change on select / deselect faster
- [x] Trawl line back to Black line
- [x] Draw black dot in center of transponders in trawls. Back x on end of trawls
- [x] Mark Type enum strings
- [x] Added trawl pos field to manual placement

9/24/2025
- [x] Right click to edit transponder on map. ONLY if USER marking
- [x] Right click on transponder show manuf/sn/trawl_id
- [x] Ownership: "Owned" / "Non-Owned" in info tab
- [x] Fixed trawl loading after startup from xml transponders
- [x] Fix Sorting Issues
- [x] Draw Square for USER positions -- X needs to be drawn bigger due to shape change
- [x] Set Preference defaults properly

9/25/2025
- [x] Removed extra comments from sorting function
- [x] commented out "Mute" button command in GUI / Event
- [x] commented out CTRL+M commadn for "Mute" in accelerator table
- [x] spacer next to command buttons to center
- [x] added is_selected field to trawl_tracker object (for highlighting)
- [x] Advanced button added next to Help
- [x] Added Delete All button w/ stub to Advanced pop-up
- [x] Tweaked trawl list GUI
- [x] Hide / show the Trawl List and Debug sizers
- [x] Showing Transonders in trawl view now
- [x] Highlighting Trawl line when trawl is selected

9/25/2025
- [x] Show "None" if trawl pos is 0 to avoid confusion 
- [x] Draw dashed grey line between non-sequential traps in a trawl
- [x] Only draw solid black line between consecutive traps in a trawl
- [x] Fix manual placement trawl pos
- [x] Allow inserting transponder into Trawl list at specific Position

- [x] Fix Recover command button to execute same as "Mark Recovered"
- [x] Add Delete Trawl button
- [x] Mark trawl_ids as not selected anymore when dialog is closed

## TODO:
- [ ] Fix manual placement edit not changing trawl pos
- [ ] Fix Deleting end point of Trawl not setting next largest point as end of trawl
- [ ] Split trawl_id address space for USER defined. Enforce range on Trawl id creation
- [ ] Split trawl_id address space for CLOUD
- [ ] Make sure Trawl is deleted when CLOUD units are out of scope / deleted
- [ ] Edit Trawl not working to change trawl num
- [ ] Make dotted grey also Yellow on select
- [ ] Hide info on right click for CLOUD positions -- No SN / Manuf
- [ ] Cloud positions don't delete at the same time when moving boat?
- [ ] Cloud radius not updating from preferences
- [ ] Fix crash after laptop wakes from sleep
- [ ] Update trawl list on timer / new trawls only show up when dialog re opened
- [ ] Optionally allow "Recover" pop-up when Release status gets set to "Verified / Not Verified"
- [ ] Cloud positions have red / green ring for ownership?
- [ ] Distance round to M, add comma?
- [ ] Fix manual release to ask for MarkID?

### Bugs:
- [ ] Opacity doesn't work for rectangles -- Remove entirely?
- [ ] When sorting the list with a selected item it doesn't get "deselected" and color stays GOLDEN
- [ ] List scrolls back up every refresh
- [ ] Trawl gets un selected when transponder list is sorted?
- [ ] Right clicking on table popup far to the right

### Maybe Later:
- [ ] Add Initial Timestamp UTC to Transponder when it was first created // parse from GML
- [ ] Add drop down to Filter transponder list by Trawl ID instead of Trawl Table
- [ ] Support trawl_path lat/lon list
- [ ] Add update flag to transponder obj to make rendering faster for non changing transponders ?
- [ ] Create separate log file for NMEA only messages
- [ ] Show trawl pos in status

### Maybe Never:
- [ ] Fix text cutoff in "Transponder Status" Tab? -- commented out for now
- [ ] Trawl creation time tag -- Can just use creation of earliest transponder. plus would have to save to xml?

### Before Release
- Remove position field from manual placement
- ~~Remove unused commands "Mute"~~
- ~~Update defaults in preferences~~
- ~~Hide trawl view by default~~
- Don't allow manual placement
- ~~Hide Debug window by default~~

### Things to Test
- Scrolling in table list 
- colorblind mode

## Preferences / Defaults

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

## Building

- run ./build.sh for full rebuild
- during dev run ./build-cp.sh to copy .so and avoid re-installing

## Installing

Install OpenCPN 5.12.4 or later
Import released plugins via Settings->Plugins->Import Plugin

## Debugging Crashes (linux)

- ./debug_opencpn.sh
- (gdb) run
- .... wait for crash / segfault...
- (gdb) backtrace

Inspecting Variables:
(gdb) frame 1
(gdb) print some_variable

Inspecting Core Dump
gdb ./your_program core
(gdb) bt

Note: make sure to build RelWithDebInfo..

## Logs

- Log file in Windows : `C:\ProgramData\opencpn\opencpn.`
- Log file in Linux : `~/.opencpn/opencpn.log`

Note: .log is most recent. .log.log has older info

## Copyright and licensing

This software is Copyright (c) David Register and Ropeless Systems 2024. It is distributed
under the terms of the Gnu Public License version 3 or, at your option,
any later version. See the file COPYING for details.
