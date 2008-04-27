# This file provides support for the scripting operations 
# implemented in nviz

# A quick routine for playing a scriptfile
proc PlayScriptFile {new_file} {
    global ScriptPlaying

    # Now read in a new file name
    # set new_file [create_file_browser .script_file_browser 1]
    set ScriptPlaying [open $new_file r]

    while {[eof $ScriptPlaying] != 1} {
	PlayNextLine
	after 100
    }
    close $ScriptPlaying
    set ScriptPlaying 0
}

proc PlayNextLine {} {
    global ScriptPlaying ProcessName

    if {$ScriptPlaying == 0} return 
    if {[eof $ScriptPlaying] == 1} {
	set ScriptPlaying 0
	return
    }

    set line ""
    set next_line ""
    puts "looking for next line"
    gets $ScriptPlaying next_line
    while {("$next_line" != "!sep!") && ([eof $ScriptPlaying] != 1)} {
	append line $next_line "\n"
	gets $ScriptPlaying next_line
    }
    puts "found it...executing"
    # uplevel \#0 "$line"
    catch "send $ProcessName \"$line\""
}
