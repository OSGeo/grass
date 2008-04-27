--  Created by William Kyngesburye on 2006-12-12.
--  GRASS Applescript startup
--  COPYRIGHT: (C) 2006-2008 by the GRASS Development Team
-- 	This program is free software under the GPL (>=v2)
--	Read the file COPYING that comes with GRASS for details.

-- note: handler order on launch:
--   1-will finish launching (good place to check prefs; no nibs loaded yet)
--   2-launched (good place to show initial windows or dialogs)
--   2.5-open?
--   3-idle (waits for user action)
-- 
-- idle is supposed to be last, but open seems to cause it to think
-- it's not idle, so we can process drag-n-drop before idling.  No docs
-- say explicitly that this is the case, so speed of Mac and process load
-- could affect this.

property grassMap : ""
property grassGui : ""
property grassLaunched : false

on will finish launching theObject
	set grassLaunched to false
	-- eventually, catch modifier key here? to show gui choice
end will finish launching

--on launch theObject
--end launch

on open maps
	--display dialog (count of maps)
	if count of maps is 1 then
		if (folder of (info for (item 1 of maps))) then
			set grassMap to " " & (quoted form of (POSIX path of (item 1 of maps)))
		end if
	end if
	launchgrass()
end open

on idle theObject
	if not grassLaunched then
		launchgrass()
	end if
end launched

on launchgrass()
	set grassLaunched to true
	set grass_path to (posix path of (path to me as string)) & "Contents/MacOS/"
	set grass_startup to (quoted form of (grass_path & "grass.sh"))
	set grassRun to grass_startup & grassGui & grassMap & "; exit"
	
	set TerminalRunning to false
	try
		if ((do shell script "ps -axc | grep '\\bTerminal\\b'") is not null) then
			set TerminalRunning to true
		end if
	end try

	tell application "Terminal"
		activate
		if TerminalRunning then
			do script (grassRun)
		else
			do script (grassRun) in window 1
		end if
	end tell
	
	tell me to quit
end launchgrass
