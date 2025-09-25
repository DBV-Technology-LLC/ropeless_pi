# ropeless_pi README
This is a plugin for OpenCPN to support the integration of Ropeless Fishing equipment directly into a chartplotter. The plugin enables fisherman to identify, position, and release RSI MTAs solely through a chartplotter interface with standardized NMEA messages.

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

9/25/2025
[x] - Right click to edit transponder on map. ONLY if USER marking
[x] - Right click on transponder show manuf/sn/trawl_id
[x] - Ownership: "Owned" / "Non-Owned" in info tab
[x] - Fixed trawl loading after startup from xml transponders

## TODO:

4. Keep track of ends of trawls
8. User trawls assigned trawl_ids descending from 65k
9. Verify Cloud pos
10. Cloud trawl_id 
16. Show trawl pos in status
18. Move manual release to bottom near OK. Remove mute?
19. Set TCP connection default / enable
20. Fix trawl rendering order by enforcing trawl positions 
21. Trawl view? Show all trawls as sub table?
23. Limit trawl length to 32?
25. Delete trawl by id?

Bugs:
[] - Fix sorting in LIst
[] - List scrolls back up every refresh

Maybe Later:
[] - Support trawl_path lat/lon list
[] - Add update flag to transponder obj to make rendering faster for non changing transponders ?

Maybe Never:
[] - Fix text cutoff in "Transponder Status" Tab? -- commented out for now
[] - Trawl creation time tag -- Can just use creation of earliest transponder. plus would have to save to xml?

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

Nice to Have
- support trawl path. add preference to enable / disable
- save NMEA messages to separate file

Before Release
- Remove position field from manual placement
- Remove unused commands "Mute"
- Update defaults in preferences

Things to Test
- Scrolling in table list 
- colorblind mode

## Users

Install OpenCPN 5.8.4 or later

Import released plugins via Settings->Plugins->Import Plugin

### General

- Log file in Windows : C:\ProgramData\opencpn\opencpn.log

## Copyright and licensing

This software is Copyright (c) David Register and Ropeless Systems 2024. It is distributed
under the terms of the Gnu Public License version 3 or, at your option,
any later version. See the file COPYING for details.
