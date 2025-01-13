tell application "Terminal"
	set this_scpt to (POSIX path of (path to me as string))
	set grass_path to characters 1 thru -((offset of "/" in (reverse of items of this_scpt as string)) + 1) of this_scpt as string
	set grass_startup to (quoted form of (grass_path & "/grass.sh"))
	set grassRun to grass_startup & "; exit"
	do script grassRun
end tell
