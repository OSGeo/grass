############################################################################
#
# LIBRARY:      runandoutput.tcl
# AUTHOR(S):    Cedric Shock (cedricgrass AT shockfamily.net)
# PURPOSE:      Interactive console for gis.m and other run commands
# COPYRIGHT:    (C) 2006 GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
############################################################################

#############################################
#Overloaded gui.tcl behaviour:

# Overload run_cmd (proc called when run button is pushed)
proc run_cmd {dlg} {
	global gronsole
	global opt

	set cmd [dialog_get_command $dlg]

	catch {$opt($dlg,run_button) configure -state disabled}

	$gronsole run $cmd {} "catch {$opt($dlg,run_button) configure -state active}"
	
	# Bring that output window up:
	raise [winfo toplevel $gronsole]
}

# no output or progress or buttons:
proc make_output {dlg path root} {}
proc make_progress {dlg path root} {}
proc make_buttons {dlg path root} {}

###########################################

proc make_fun_buttons {dlg path} {
	global opt env
	set pgm_name $opt($dlg,pgm_name)

	set buttonframe [frame $path.buttonframe]
	button $buttonframe.run   -text [G_msg "Run"]   -command "run_cmd $dlg"
	# In the future we'll have a button to make a layer from here:
	# button $buttonframe.layer -text [G_msg "Layer"] -command "layer_cmd $dlg"
	button $buttonframe.help  -text [G_msg "Help"]  -command "help_cmd $dlg"
	button $buttonframe.close -text [G_msg "Close"] -command "close_cmd $dlg"

	set opt($dlg,run_button) $buttonframe.run 

	# Turn off help button if the help file doesn't exist
	if {! [file exists $env(GISBASE)/docs/html/$pgm_name.html]} {
		$buttonframe.help configure -state disabled
	}

	pack $buttonframe.run $buttonframe.help $buttonframe.close \
		-side left -expand yes -padx 5 -pady 5
	pack $buttonframe -expand no -fill x -side bottom -before [lindex [pack slaves $path] 0]

	# Set the starting window size if this is a toplevel window
	if {[winfo toplevel $path] == $path} {
		wm geometry $path "560x400"
	}
}

proc run_ui {cmd} {
    global dlg 
    global path
    global devnull

    set program [lindex $cmd 0]
    set code ""

	if {[catch {set code [exec -- $program --tcltk 2> $devnull]} error]} {
		GmLib::errmsg $error
	}

    set path .dialog$dlg
    toplevel $path
    
    if {$code == ""} { return }
    eval $code

    # Push the command to the dialog
    set thisdialog $dlg
    dialog_set_command $dlg $cmd

    # Add our ui
    make_fun_buttons $dlg $path
}

#################################################

proc run_disabled {gronsole button cmd} {
	if {[catch {$button configure -state disabled} error]} {
		GmLib::errmsg $error
	}

	$gronsole run $cmd {running} "catch {$button configure -state active}"
}

proc gronsole_history {cmdtext ci cmd} {
	$cmdtext delete 1.0 end
	$cmdtext insert end $cmd
}

proc command_window {where} {
	global keycontrol
	global bgcolor
	set cmdpane [frame $where.command -bg $bgcolor]
	set cmdwin [ScrolledWindow $where.win -relief flat -borderwidth 1]
	set gronsole [Gronsole $where.gronsole -clickcmd "gronsole_history $cmdwin.text"]
	set cmdtext [text $cmdwin.text -height 2 -width 80] 
	$cmdwin setwidget $cmdtext
	set runbutton [button $cmdpane.run -text [G_msg "Run"] -width 14 -wraplength 90 -default active -bd 1 \
		-command "run_disabled $gronsole $cmdpane.run \[string trim \[$cmdtext get 1.0 end\]\]"]
	set run2button [button $cmdpane.run2 -text [G_msg "Run (background)"] -width 14 -wraplength 90 -bd 1 \
		-command "$gronsole run \[string trim \[$cmdtext get 1.0 end\]\] {} {}"]
	set runuibutton [button $cmdpane.runui -text [G_msg "Run (GUI)"] -width 14 -wraplength 90 -bd 1 \
		-command "run_ui \[string trim \[$cmdtext get 1.0 end\]\]"]
	set runxterm [button $cmdpane.xterm -text [G_msg "Run (in Xterm)"] -width 14 -wraplength 90 -bd 1 \
		-command "$gronsole run_xterm \[string trim \[$cmdtext get 1.0 end\]\] {}"]
	set outpane [frame $where.output -bg $bgcolor]
	set savebutton [button $outpane.save -text [G_msg "Save"] -command "$gronsole save" \
		-bd 1 -padx 10]
	set clearbutton [button $outpane.clear -text [G_msg "Clear"] -command "$gronsole clear" \
		-bd 1 -padx 10]

	pack $runbutton $run2button $runuibutton $runxterm \
		-side left -expand yes -padx 5 -pady 5
	pack $savebutton $clearbutton \
		-side left -expand yes -padx 5 -pady 5

	pack $cmdpane -fill x -side bottom
	pack $cmdwin -fill x -side bottom
	pack $outpane -fill x -side bottom
	pack $gronsole -side top -fill both -expand yes

	bind $cmdtext <$keycontrol-c> "tk_textCopy %W"
	bind $cmdtext <$keycontrol-v> "tk_textPaste %W"
	bind $cmdtext <$keycontrol-x> "tk_textCut %W"

	return $gronsole
}

toplevel .gronsole
wm title .gronsole [G_msg "Output - GIS.m"]

# If we had our own window manager we could withdraw windows instead of iconifying them:
wm protocol .gronsole WM_DELETE_WINDOW "wm iconify .gronsole"

set gronsole [command_window {.gronsole}]

###############################################################################
# Run procs for gis.m:

################################################################################

proc execute {cmd} {
    # Use run and output
    run_ui $cmd
}

###############################################################################
proc spawn {cmd args} {

	if {[catch {eval [list exec -- $cmd] $args &} error]} {
		GmLib::errmsg $error
	}

}

###############################################################################
proc spawn_panel {cmd args} {
	global gronsole

	$gronsole run $cmd gism {}
}

###############################################################################

proc run_panel {cmd} {
	global gronsole

	$gronsole run_wait $cmd gism
}

###############################################################################
proc run {cmd args} {
	global devnull

	# This and runcmd are being used to run command in the background
	# These used to go to stdout and stderr
	# but we don't want to pollute that console.
	# eval exec -- $cmd $args >@ stdout 2>@ stderr
	
	if {[catch {eval [list exec -- $cmd] $args >& $devnull} error]} {
		GmLib::errmsg $error
	}

	
}

###############################################################################

proc runcmd {cmd args} {
	global gronsole

	set ci [$gronsole annotate $cmd [list gism running]]

	eval run $cmd $args

	$gronsole remove_tag $ci running	
}

###############################################################################
proc term_panel {cmd} {
	global gronsole

	$gronsole run_xterm $cmd gism
}

###############################################################################
proc term {cmd args} {
	global env
	global mingw
	
	if { $mingw == "1" } {
		if {[catch {eval [list exec -- cmd.exe /c start $env(GISBASE)/etc/grass-run.bat $cmd] $args &} error]} {
			GmLib::errmsg $error
		}
	   
	} else {
		if {[catch {eval [list exec -- $env(GISBASE)/etc/grass-xterm-wrapper -name xterm-grass -e $env(GISBASE)/etc/grass-run.sh $cmd] $args &} error]} {
			GmLib::errmsg $error
		}
	   
	}
}

###############################################################################
# Make sure there's an xmon before running some commands.
# Used in menus.

proc guarantee_xmon {} {
	if {![catch {open "|d.mon -L" r} input]} {
		while {[gets $input line] >= 0 } {
			if {[regexp -nocase {(x[0-9]+).*not running} $line dummy monitor]} {
				# $monnum is the monitor number
				#create list of non-running monitors
				lappend xmonlist $monitor
			}
		}

	}

	if {[catch {close $input} error]} {
		tk_messageBox -type ok -icon error -title [G_msg "Error"] \
			-message [G_msg "d.mon error: problem launching xmon, $error"]
		return
	}

	if { ![info exists xmonlist] } {
		tk_messageBox -type ok -icon error -title [G_msg "Error"] \
			-message [G_msg "This module requires X11 support, but no xmons were found"]
		return
	}

	set xmon  [lindex $xmonlist 0]
	spawn d.mon start=$xmon
}

###############################################################################
# Annotation procs for gis.m:

proc monitor_annotation_start {mon title tags} {
	global gronsole
	set handle [$gronsole annotate $title $tags]
	$gronsole set_click_command $handle {}
	return $handle
}

proc monitor_annotate {handle text} {
	global gronsole
	$gronsole annotate_text $handle $text
}
