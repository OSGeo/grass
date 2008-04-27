# This file provides support for the scripting operations 
# implemented in nviz
# A quick routine for specifying the scriptfile to use
proc SetScriptFile {} {
    global ScriptFile

    set new_file [create_file_browser .script_file_browser 1]

    if {$new_file == -1} then {
	bgerror "Can't create script file"
	return
    }
    # append ".nvscr" extension if it isn't already there
    if { [string compare [file extension $new_file ] ".nvscr"] != 0 } then {
	append new_file ".nvscr"
    }
    Nv_set_script_file $new_file
}

# Allow the user to add a single line to the current script file
proc AddScriptLine {} {
    global ScriptPlaying src_boot env

    if $ScriptPlaying return

    # Create a simple entry popup
    set line [exec nviz -f $env(GISBASE)/etc/nviz2.2/scripts/script_get_line -q]
    if {"$line" == "-1"} return
    Nv_script_add_string "$line"
}

# Allow the user to add a single cmd to the current script file
# The difference between this routine and the previous is that the
# command entered here is sent to the nviz process and executed there
proc AddScriptCmd {} {
    global ScriptPlaying src_boot env

    if $ScriptPlaying return

    # Create a simple entry popup
    set line [exec nviz -f $env(GISBASE)/etc/nviz2.2/scripts/script_get_line -q]
    if {"$line" == "-1"} return
    Nv_script_add_string "catch \{send \$ProcessName \{$line\}\}"
}

# Useful from within a script for sending things without the
# nasty catch-send bit
proc SendScriptLine {line} {
    global ProcessName

    catch {send $ProcessName "$line"} rval

    return $rval
}

# Same as above except waits for command to terminate
proc SendScriptLineWait {line this_proc} {
    global ProcessName WaitPoint

    set WaitPoint -1
    puts "Sending..."
    catch {send $ProcessName "block $this_proc \{$line\}"} rval

    while {$WaitPoint == -1} { 
	update
    }

    puts "Exiting SendScriptLineWait"

    return $WaitPoint

}

# Other end of send script which blocks
proc block { proc_name cmd } {
    puts "In block with args $proc_name , $cmd"

    set rval [eval "$cmd"]
    if {"$rval" == ""} then {
	set rval 0
    }
    puts "returning $rval"
    send $proc_name "global WaitPoint ; set WaitPoint $rval"
    puts "exiting block cmd"

}

# This is a special block which sends back a
# default value in the case that no value is to be
# returned
proc block2 { proc_name cmd default} {
    puts "In block2 with args $proc_name, $cmd, $default"

    set rval [eval $cmd]
    if {"$rval" == ""} then {
	set rval "$default"
    }
    puts "returning $rval"
    send $proc_name "global WaitPoint ; set WaitPoint $rval"
    puts "exiting block cmd"
}
    
# Quicky tool to extract map object id
proc ExtractMapID {map_name} {
    return [string range $map_name 5 end]
}

# Newer, better version
proc ReturnMapHandle {logical_name} {
    return [SendScriptLine "Nliteral_from_logical $logical_name"]
}




