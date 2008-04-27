##########################################################################
# Default Priority for this panel
#
# priority is from 0 to 10
#  the lower the number, the quicker it will be bumped
#  10 cannot be bumped
#  Panels will be loaded by the greater of 5 or their current priority

##################################
# Globals for animation
##################################
global keyanimStartX keyanimEndX
global keyanimKeyList keyanimUniqueTag keyanimInterpType keyanimPathState
global keyanimChannelList keyanimFrameRate keyanimCurrentKeyTime
global keyanimPlayState keyanimPrecis keyanimSaveRenderStyle
global keyanimVectState keyanimSiteState keyanimVolState keyanimLabState

set keyanimPrecis 1e-5
set keyanimFrameRate 30
set keyanimCurrentKeyTime "00:00:00"
Nset_numsteps 30
set keyanimStartX 3
set keyanimEndX 270
set keyanimKeyList [list]
set keyanimUniqueTag 0
set keyanimInterpType spline
set keyanimPathState off
set keyanimSiteState off
set keyanimVectState off
set keyanimVolState off
set keyanimLabState off
set keyanimChannelList [list]
set keyanimPlayState stop
set keyanimSaveRenderStyle 0
set keyanimSaved 0

# The keyframe list has the following format:
#	{ List of keyframe entries sorted by time}
#
#	Time is a floating point value representing seconds.  A keyframe
#	entry consists of the following:
#		{ Time { List of channel modification } key_tag Selected? }
#	where
#	channel modification = { 	Channel Name
#					channel_entry_list
#				}
#
# The channel list has the following format:
# 	{ List of channels in no particular order }
#
# A channel entry consists of the following:
#	{ name tag_name channel_entry_list get_func set_func }
# where
#	name = name of the channel
# 	tag_name = is an internal name used to manage a keyframe slider
#	channel_entry_list = a list of channel entry pairs used to determine what
#                        values the keyframer tracks and how it should interpolate
#                        among them.
#   get_func = A function to get channel_entry_list values when a keyframe is added
#	set_func = A function to set the state of nviz given a list of channel values
#

# Update all the frames before we exit (should be easy since there aren't any)
Nupdate_frames

############################################################################
# procedure to make animation control area
#
#	Arguments:
#		BASE - A base name for the animation panel
#
###########################################################################
proc mkkanimatorPanel { BASE } {
    global keyanimNumFrames keyanimInterpType keyanimPathState
    global keyanimCurrentKeyTime
    global bit_map_path

    frame $BASE  -relief groove -borderwidth 2
    set panel [St_create {window name size priority} $BASE "Keyframe Animation" 2 5]
    Nv_mkPanelname $BASE "Keyframe Animation Panel"

    # Create the top section containing play control, framerate, and Add and Delete buttons
    frame $BASE.playcontrol -relief groove -borderwidth 2
    set rname $BASE.playcontrol

    button $rname.back  -height 20 -width 20 -bitmap @$bit_map_path/left \
	-command "keyanimPlayBackward $BASE"
    button $rname.slow_back -height 20 -width 20 -bitmap @$bit_map_path/slow_left \
	-command "keyanimOneBackward $BASE"
    button $rname.stop -height 20 -width 20 -bitmap @$bit_map_path/stop \
	-command "keyanimStop $BASE ; move_position"
    button $rname.slow_forward -height 20 -width 20 -bitmap @$bit_map_path/slow_right \
	-command "keyanimOneForward $BASE"
    button $rname.forward -height 20 -width 20 -bitmap @$bit_map_path/right \
	-command "keyanimPlayForward $BASE"
    button $rname.label -text "Framerate :" -command "keyanimChangeFramerate $BASE"
    label  $rname.val   -textvariable keyanimFrameRate
    pack $rname.back $rname.slow_back $rname.stop $rname.slow_forward \
	$rname.forward -side left -padx 4 -pady 2 -fill y -expand no
    pack $rname.val $rname.label -side right -padx 2 -pady 2 -fill y -expand no


    frame $BASE.commands -relief groove -borderwidth 2
    set rname $BASE.commands
    label $rname.label -text "File: "
    button $rname.save          -text "Save"    -command "keyanimSaveAnim $BASE"
    button $rname.load          -text "Load"    -command "keyanimLoadAnim $BASE"

    label $rname.label2 -text "Animation: "
    button $rname.rands -text "Run and Save" -command "keyanimRunAndSave $BASE"
    button $rname.close -text "Close" -command "Nv_closePanel $BASE"

    pack $rname.label2 $rname.rands $rname.label $rname.save $rname.load \
        -side left -padx 2 -pady 2 -padx 2 -fill y -expand no

    pack $rname.close -side right -padx 2 -pady 2 -padx 2 -fill y -expand no

    # Create mid section containing the keyframe manager
    frame $BASE.keycontrol -relief groove -borderwidth 2
    set rname $BASE.keycontrol
    # Commented these two lines to make more space on the display
    # (mark 11/17/94)
    #   label $rname.l -text "Key Frames"
    #   pack $rname.l -padx 3 -pady 3 -expand yes
    mkkeyframeArea $BASE

    # Create bottom section containing current keytime
    frame $BASE.keytime -relief groove -borderwidth 2
    set rname $BASE.keytime
    button $rname.add           -text "Add"     -command "keyanimAddKey $BASE"
    button $rname.delete        -text "Delete"  -command "keyanimDeleteKeys $BASE"
    button $rname.change -text "New Key Time:" -command "keyanimChangeKeytime $BASE"
    label $rname.current -textvariable keyanimCurrentKeyTime
    pack $rname.add $rname.delete -side left -padx 2 -pady 2 -fill y -expand no
    pack $rname.current $rname.change -side right -padx 2 -fill y -expand no

   # Create bottom section containing command buttons for showing
    # paths, sites and vectors, as well as selecting interpolation
    # type and spline tension
    frame $BASE.other_commands -relief groove -borderwidth 2
    set rname $BASE.other_commands
    menubutton $rname.menu1 -menu $rname.menu1.m1 \
        -indicatoron 1 -text "Show Feature" -relief raised

    menu $rname.menu1.m1
    $rname.menu1.m1 add checkbutton -label "Path" -variable keyanimPathState \
        -command {Nshow_path $keyanimPathState} -onvalue on -offvalue off
    $rname.menu1.m1 add checkbutton -label "Vect Lines/Polygons" -variable keyanimVectState \
        -command {Nshow_vect $keyanimVectState} -onvalue on -offvalue off
    $rname.menu1.m1 add checkbutton -label "Vect Points" -variable keyanimSiteState \
        -command {Nshow_site $keyanimSiteState} -onvalue on -offvalue off
    $rname.menu1.m1 add checkbutton -label "Volume" -variable keyanimVolState \
        -command {Nshow_vol $keyanimVolState} -onvalue on -offvalue off
    $rname.menu1.m1 add checkbutton -label "Labels/Legend" \
	-variable keyanimLabState \
	-command {Nshow_lab $keyanimLabState} -onvalue on -offvalue off

    menubutton $rname.menu2 -menu $rname.menu2.m1 \
        -indicatoron 1 -text "Interp." -relief raised

    menu $rname.menu2.m1
    $rname.menu2.m1 add radiobutton -label "linear" \
        -variable keyanimInterpType -value linear \
        -command "Nset_interp_mode linear ; Nupdate_frames ; $rname.tension configure -state disabled -background gray80"
    $rname.menu2.m1 add radiobutton -label "spline" \
        -variable keyanimInterpType -value spline \
        -command "Nset_interp_mode spline ; Nupdate_frames ; $rname.tension configure -state normal -background gray90"
    scale $rname.tension -label "tension" -orient h \
        -showvalue f -from 0 -to 1000 -command keyanimChangeTension \
        -activebackground gray80 -background gray90

    $rname.tension set 500

    pack  $rname.menu1 -side left -padx 6 -pady 2 -expand no
    pack $rname.tension -side right -fill y -padx 2 -pady 2 -expand no
    pack  $rname.menu2 -side right -padx 2 -pady 2 -expand no


    #PACK all frames
    pack $BASE.playcontrol $BASE.keycontrol $BASE.keytime  $BASE.other_commands \
    $BASE.commands \
    -side top -expand yes -fill both

    # Add all animation channels
    keyanimAddChannel $BASE "FromX"
    keyanimAddChannel $BASE "FromY"
    keyanimAddChannel $BASE "FromZ"
    keyanimAddChannel $BASE "DirX"
    keyanimAddChannel $BASE "DirY"
    keyanimAddChannel $BASE "DirZ"
    keyanimAddChannel $BASE "FOV"
    keyanimAddChannel $BASE "TWIST"

    return $panel
}


############################################################################
# procedure to add a channel to the keyframe animator
#
#   Arguments:
#       BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#	       element - channel list entry to add (includes name and all key segments)
#              This is the name of the channel.
#       entry_list - a list of pairs describing the channel entries.  Channel entry
#                    pairs are specified by {NAME TYPE} where name is some name you
#                    wish to assign to the entry and type is either static or dynamic.
#      	get_entries - name of a global function which gets a list of channel entries.
#                     Specifically, this function will be called with no arguments and
#                     will return a list with the same form as an entry list except
#                     that TYPE will be replaced with actual values.
#       set_entries - name of global function which, when given a list of channel entries,
#                     makes the appropriate library calls.  Specifically, this function
#                     will be called with a single argument, a list of channel entries,
#                     and will be responsible for setting the state of nviz to correspond
#                     the values dictated in the channel entry list.
#
############################################################################
proc keyanimAddChannel { BASE element {entry_list null} {get_entries null} {set_entries null} } {
    global keyanimChannelList keyanimFrameRate
    global keyanimStartX keyanimEndX keyanimCurrentKeyTime

    set rname $BASE
    append rname .keycontrol.key_channels.hold_channels.
    set channel_iname [keyanimGenTag]
	append rname $channel_iname

    frame $rname
    label $rname.label -relief raised -text [lindex $element 0] -width 10
    checkbutton $rname.active -text "On" -variable $channel_iname
    global $channel_iname
    set $channel_iname 1

    canvas $rname.keys -relief raised -height 1c -width 20c -bg white
    pack $rname.label $rname.active $rname.keys -side left -fill both
    pack $rname
    set keyanimStartX 7
    set keyanimEndX [expr [lindex [$rname.keys configure -width] 4] - 7 - \
			 [lindex [$rname.keys configure -bd] 4]]

    # Now create a keyframe slider with the iname as the tag.  This slider
    # is used to control the current keyframe.
    $rname.keys create rectangle 3 0 8 1c -fill blue -outline black -tags cur_keyframe
	$rname.keys bind cur_keyframe <B1-Motion> "keyanimMoveSlider %x $BASE"
    bind $rname.keys <B1-Motion> "keyanimMoveSlider %x $BASE"
    bind $rname.keys <1> "keyanimMoveSlider %x $BASE"

    # Also create the key time recorder if it hasn't been created yet
    if {[llength $keyanimChannelList] == 0} then {
	set rname $BASE
	append rname ".keycontrol.key_times.hold_times.times"
	$rname create text 3 3 -anchor nw -text "$keyanimCurrentKeyTime" -tags cur_time
	$rname bind cur_time <B1-Motion> "keyanimMoveSlider %x $BASE"
    }

    # Add the channel to the channel list
    # We use this list to determine what keys are added for each keyframe
    set new_element [list [lindex $element 0] $channel_iname \
			 $entry_list $get_entries $set_entries]
    # puts "Adding channel $new_element"
    lappend keyanimChannelList $new_element

    # Set the scroll region based on the number of key channels
    set num [llength $keyanimChannelList]
    set num [expr $num + $num*10]
    append num m
    $BASE.keycontrol.key_channels configure -scrollregion [list 0 0 28c $num]

    # Also, align all the sliders if there are any
    keyanimMoveSlider $keyanimStartX $BASE

}

############################################################################
# procedure to position slider at a specified time
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkanimationPanel)
#		time - in mm:ss:ff format
#
############################################################################
proc keyanimPosSlider {BASE time} {
    global keyanimStartX keyanimEndX keyanimChannelList
    global keyanimKeyList keyanimFrameRate keyanimSaveRenderStyle
    global keyanimPlayState

    set oBASE $BASE
    append BASE ".keycontrol.key_channels.hold_channels"

    # Set the style if we are doing a run and save
    if {"$keyanimPlayState" == "run_and_save"} then {
	set style $keyanimSaveRenderStyle
    } else {
	set style 0
    }

    # Figure out the x position for the slider
    if {[llength $keyanimKeyList] == 0} then {
	return
    }
    set x [keyanimTimeToPos $time]

    # Update the time slider to indicate the current position of the slider
    set rname $oBASE
    append rname ".keycontrol.key_times.hold_times.times"

    $rname itemconfigure cur_time -text "$time"

    # This is a somewhat complicated move.  We have to move the individual sliders
    # in EACH of the channels.  So we look through the channel list to get the
    # internal names of all the sliders and move them one at a time

    # We constrain slider movement by the variables keyanimStartX and keyanimEndX
    if {($x >= $keyanimStartX) && ($x <= $keyanimEndX)} then {
	foreach i $keyanimChannelList {
	    set rname $BASE
	    append rname . [lindex $i 1]
	    $rname.keys coords cur_keyframe $x 0 [expr $x + 5] 1c
	}
	set rname $oBASE
	append rname ".keycontrol.key_times.hold_times.times"
	$rname coords cur_time $x 3

	# Figure out where to put the scrollbars so that the key
	# slider is always centered
	set page_width [$oBASE.keycontrol.key_channels xview]
	set page_width [expr [lindex $page_width 1] - \
			     [lindex $page_width 0]]
	set page_width [expr ($page_width + 0.0) * 0.5]

	set cur_width [$oBASE.keycontrol.key_channels bbox hc]
	set cur_width [expr [lindex $cur_width 2] - [lindex $cur_width 0]]

	set x2 [expr $x + [$oBASE.keycontrol.key_times.hold_times.spacer1 cget -width] + \
		    [$oBASE.keycontrol.key_times.hold_times.spacer2 cget -width]]

	$oBASE.keycontrol.key_channels xview moveto [expr ($x2 + 0.0)/$cur_width - $page_width]
	$oBASE.keycontrol.key_times xview moveto [expr ($x2 + 0.0)/$cur_width - $page_width]
    } else {
	return
    }
    update

    # Finally, if there are at least two keyframes then move to
    # to the current framestep
    if {[llength $keyanimKeyList] > 1} then {
	# Figure out current frame step
	set start_time [lindex [lindex $keyanimKeyList 0] 0]
	set times [split $time :]
	set tm [expr int([string trimleft [lindex $times 0] 0]0/10.0)]
	set ts [expr int([string trimleft [lindex $times 1] 0]0/10.0)]
	set tf [expr int([string trimleft [lindex $times 2] 0]0/10.0)]
	set tf [expr ($tf + 0.0) / $keyanimFrameRate]
	set curr_time [expr $tm * 60 + $ts + $tf + 0.0]
	set diff [expr $curr_time - $start_time]
	set curr_step [expr $diff * $keyanimFrameRate]

	# Set parameters for other channels
	keyanimSetChannels $BASE $curr_time

	Ndo_framestep [expr int($curr_step + 1)] $style

    }
}

############################################################################
# callback to move keyframe slider
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkanimationPanel)
#
#	This callback responds to B1-Motion events on the cur_keyframe
#	item in the slider.  I.e. we call this routine when the user
#	drags the slider.
#
############################################################################
proc keyanimMoveSlider { x BASE } {
    global keyanimStartX keyanimEndX keyanimChannelList
    global keyanimKeyList keyanimFrameRate

    set oBASE $BASE
    append BASE ".keycontrol.key_channels.hold_channels"

    # This is a somewhat complicated move.  We have to move the individual sliders
    # in EACH of the channels.  So we look through the channel list to get the
    # internal names of all the sliders and move them one at a time

    # We constrain slider movement by the variables keyanimStartX and keyanimEndX
    if {($x >= $keyanimStartX) && ($x <= $keyanimEndX)} then {
	foreach i $keyanimChannelList {
	    set rname $BASE
	    append rname . [lindex $i 1]
	    $rname.keys coords cur_keyframe $x 0 [expr $x + 5] 1c
	}
	set rname $oBASE
	append rname ".keycontrol.key_times.hold_times.times"
	$rname coords cur_time $x 3

    } else {
	return
    }

    # Update the time slider to indicate the current position of the slider
    set text_time [keyanimPosToTime $x]

    set rname $oBASE
    append rname ".keycontrol.key_times.hold_times.times"

    $rname itemconfigure cur_time -text "$text_time"

    # Finally, if there are at least two keyframes then move to
    # to the current framestep
    if {[llength $keyanimKeyList] > 1} then {
	# Figure out current frame step
	set start_time [lindex [lindex $keyanimKeyList 0] 0]
	set times [split [keyanimPosToTime $x] :]
	set tm [expr int([string trimleft [lindex $times 0] 0]0/10.0)]
	set ts [expr int([string trimleft [lindex $times 1] 0]0/10.0)]
	set tf [expr int([string trimleft [lindex $times 2] 0]0/10.0)]
	set tf [expr ($tf + 0.0) / $keyanimFrameRate]
	set curr_time [expr $tm * 60 + $ts + $tf + 0.0]
	set diff [expr $curr_time - $start_time]
	set curr_step [expr $diff * $keyanimFrameRate]
	Ndo_framestep [expr int($curr_step + 1)] 0

	# set parameters for other channels
	keyanimSetChannels $BASE $curr_time
    }

}

############################################################################
# procedure to change the current key time
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#
############################################################################
proc keyanimChangeKeytime { BASE } {
    global keyanimCurrentKeyTime keyanimFrameRate

    # Create a popup to change the keytime
    set name .
    append name [keyanimGenTag]

    toplevel $name
    label $name.label -text "New Key Time"
    pack $name.label

    frame $name.lgroup -relief groove
    label $name.lgroup.mlabel -text "Minute:" -relief raised
    label $name.lgroup.slabel -text "Second:" -relief raised
    label $name.lgroup.flabel -text "Frame:" -relief raised
    pack $name.lgroup.mlabel $name.lgroup.slabel \
	$name.lgroup.flabel -fill both -padx 2 -pady 2
    pack $name.lgroup -side left -fill both

    frame $name.egroup -relief groove
    entry $name.egroup.mentry -relief sunken -width 4
    entry $name.egroup.sentry -relief sunken -width 4
    entry $name.egroup.fentry -relief sunken -width 4
    pack $name.egroup.mentry $name.egroup.sentry \
	$name.egroup.fentry -fill both -padx 2 -pady 2
    pack $name.egroup -side right -fill both

    button $name.ok -text "Ok" -command "set button 1"
    pack $name.ok -side bottom -fill both -before $name.label

    # Grab the focus
    set oldFocus [focus]
    grab $name
    focus $name

    # Now wait until the user hits "OK" to single the entry
    tkwait variable button

    # Parse the entries and set the new key time
    set minutes [$name.egroup.mentry get]
    set seconds [$name.egroup.sentry get]
    set frames  [$name.egroup.fentry get]

    # If all fields are left blank then do nothing
    if { ([string length $minutes] == 0) && ([string length $seconds] == 0) &&
	  ([string length $frames] == 0) } then {
	destroy $name
	focus $oldFocus
	return
    }

    # If field is left blank assume "0"
    if { [string length $minutes] == 0 } then { set minutes 0 }
    if { [string length $seconds] == 0 } then { set seconds 0 }
    if { [string length $frames] == 0 } then { set frames 0 }

    if { ($minutes < 0) || ($seconds < 0) || ($frames < 0) } then {
	tk_dialog .kt_error "KeyTime Error" "Error - All values must be at least zero." \
	    {} 0 Dismiss
	destroy $name
	focus $oldFocus
	return
    }

    if { $frames >= $keyanimFrameRate } then {
	tk_dialog .kt_error "KeyTime Error" \
	    "Error - Frame number must be less than frame rate." \
	    {} 0 Dismiss
	destroy $name
	focus $oldFocus
	return
    }

    set minutes [keyanimPadNumber $minutes]
    set seconds [keyanimPadNumber $seconds]
    set frames [keyanimPadNumber $frames]

    set keyanimCurrentKeyTime $minutes
    append keyanimCurrentKeyTime : $seconds : $frames

    # Finally pop down the widget and restore the focus
    destroy $name
    focus $oldFocus
    update

}

############################################################################
# procedure to change the current frame rate
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#
############################################################################
proc keyanimChangeFramerate { BASE } {
    global keyanimCurrentKeyTime keyanimFrameRate keyanimSaved

    # Create a popup to change framerate
    set name .
    append name [keyanimGenTag]

    toplevel $name
    label $name.label -text "New Frame Rate"
    pack $name.label

    frame $name.lgroup -relief groove
    label $name.lgroup.flabel -text "Framerate:" -relief raised
    pack $name.lgroup.flabel -fill both -padx 2 -pady 2
    pack $name.lgroup -side left -fill both

    frame $name.egroup -relief groove
    entry $name.egroup.fentry -relief sunken -width 4
    pack $name.egroup.fentry -fill both -padx 2 -pady 2
    pack $name.egroup -side right -fill both

    button $name.ok -text "Ok" -command "set button 1"
    pack $name.ok -side bottom -fill both -before $name.label

    # Grab the focus
    set oldFocus [focus]
    grab $name
    focus $name

    # Now wait until the user hits "OK" to single the entry
    tkwait variable button

    # Parse the entries and set the new key time
    set framerate [$name.egroup.fentry get]

    if {[catch "expr int($framerate)"] == 1} then {
	destroy $name
	focus $oldFocus
	return
    } else {
	set framerate [expr int($framerate)]
    }

    if {$framerate < 1} then {
	tk_dialog .fr_error "Frame Rate Error" "Error - Frame rate must be at least 1." \
	    {} 0 Dismiss
	destroy $name
	focus $oldFocus
	return
    }

    set keyanimFrameRate $framerate
    keyanimUpdateFramesteps
    keyanimDrawKeys $BASE

    # Finally pop down the widget and restore the focus
    destroy $name
    focus $oldFocus
    update

    # Need to make sure slider indicates new frame rate (may be smaller
    # then current frames set on slider)
    # By calling TimeToString we automatically get a new format
    # with the correct frame setting
    set text_time [keyanimTimeToString [keyanimGetSliderTime $BASE]]
    keyanimPosSlider $BASE $text_time

    set keyanimSaved 0
}

############################################################################
# procedure to draw keyframe entries
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#
############################################################################
proc keyanimDrawKeys { BASE {old_tags {} } } {
    global keyanimChannelList keyanimKeyList keyanimFrameRate
    global keyanimStartX keyanimEndX

    # Erase old keys (if there are any)
    foreach i $keyanimChannelList {
	set rname $BASE
	append rname .keycontrol.key_channels.hold_channels. [lindex $i 1]

	foreach j $keyanimKeyList {
	    # Extract tag name
	    set key_tag [lindex $j 2]

	    # We catch these next two commands in cases where the key
	    # might have disappeared because it never existed in the first
	    # place (like when we load a saved key list file)
	    catch "$rname.keys delete $key_tag"
	    catch "$BASE.keycontrol.key_times.hold_times.times delete $key_tag"
	}

	if {[llength $old_tags] != 0} then {
	    foreach j $old_tags {
		$rname.keys delete $j
		$BASE.keycontrol.key_times.hold_times.times delete $j
	    }
	}
    }

    # First check if there is anything to display
    if {[llength $keyanimKeyList] == 0} then {
	return
    }

    # a priori, the left side of the keyframe canvas is the starting time while the
    # right edge is the ending time, whatever the spacing.
    # Figure out the extremes of the animation
    set min_time [lindex [lindex $keyanimKeyList 0] 0]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]

    # Do for each keyframe
    foreach i $keyanimKeyList {
	# Set time for current key frame
	set ctime [lindex $i 0]
	set key_tag [lindex $i 2]
	set select_flag [lindex $i 3]

	# Figure out the appropriate x position
	if {$max_time == $min_time} then {
	    set ratio 0.0
	} else {
	    set ratio [expr ($ctime - $min_time + 0.0)/($max_time - $min_time + 0.0)]
	}
	set x_pos [expr $ratio * ($keyanimEndX - $keyanimStartX + 0.0) + $keyanimStartX]

	# Do for each channel element in the current keyframe
	foreach j [lindex $i 1] {

	    # Find the channel the current element corresponds to
	    set cname [lindex $j 0]
			set tag_name ""
	    foreach k $keyanimChannelList {
		if {[lindex $k 0] == $cname} then {
		    set tag_name [lindex $k 1]
		    break
		}
	    }
	    if {$tag_name == ""} then {
		tk_dialog .ierror "Internal Error" \
		    "Internal Error - Can't find channel $cname for keyframe $i" \
		    {} 0 Dismiss
		return
	    }

	    # Now place a keyframe marker at the appropriate place in the element
	    set rname $BASE
	    append rname .keycontrol.key_channels.hold_channels. $tag_name

	    # Now create a marker
	    $rname.keys create rectangle [expr $x_pos - 4] 3m [expr $x_pos + 4] 8m \
		-fill skyblue -outline black -tags $key_tag

	}

	# Don't forget to add a time on the key time panel
	set rname $BASE
	append rname .keycontrol.key_times.hold_times.times

	# Convert time back to mm:ss:ff format
	set tm [expr int($ctime) / 60]
	set ts [expr int($ctime) % 60]
	set tf [expr int(($ctime + 0.0 - ($tm * 60) - $ts) * $keyanimFrameRate)]
	set tm [keyanimPadNumber $tm]
	set ts [keyanimPadNumber $ts]
	set tf [keyanimPadNumber $tf]

	# Set appropriate text color
	if {$select_flag} then {
	    set text_color black
	} else {
	    set text_color skyblue
	}

	set text_time $tm
	append text_time : $ts : $tf
	$rname create text $x_pos 1c -anchor nw -text "$text_time" -tags $key_tag \
	    -fill $text_color -width 1 \
	    -font {-misc-fixed-medium-r-normal--10-100-75-75-c-60-iso8859-1}

	# Bind a callback to the time indicator, we use this to move keyframes
	$rname bind $key_tag <B1-Motion> "keyanimMoveKey $BASE %x $key_tag"
	$rname bind $key_tag <ButtonRelease-1> "keyanimMoveKeyRelease $BASE %x $key_tag"
	$rname bind $key_tag <2> "keyanimSelectKey $BASE $key_tag"
    }

}

############################################################################
# procedure to convert from time to x position
#
#	Arguments:
#		time - in mm:ss:ff format
#
############################################################################
proc keyanimTimeToPos { time } {
    global keyanimKeyList keyanimFrameRate
    global keyanimStartX keyanimEndX

    # Check for an animation first
    if {[llength $keyanimKeyList] == 0} then {
	return $keyanimStartX
    }

    # Convert time to just seconds
    set times [split $time :]
    set tm [expr int([string trimleft [lindex $times 0] 0]0/10.0)]
    set ts [expr int([string trimleft [lindex $times 1] 0]0/10.0)]
    set tf [expr int([string trimleft [lindex $times 2] 0]0/10.0)]
    set tf [expr ($tf + 0.0) / $keyanimFrameRate]

    set ctime [expr $tm * 60 + $ts + $tf + 0.0]

    set min_time [lindex [lindex $keyanimKeyList 0] 0]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]

    # Check if ctime less than min_time or greater than max_time
    if {$ctime < $min_time} then {
	set pos 0.0
    } else {
	if {$ctime > $max_time} then {
	    set pos 1.0
	} else {
	    set pos [expr ($ctime - $min_time + 0.0) / ($max_time - $min_time)]
	}
    }

    set x_pos [expr $pos * ($keyanimEndX - $keyanimStartX + 0.0) + $keyanimStartX]

    return [expr int($x_pos)]
}

############################################################################
# procedure to convert from time in seconds to x position
#
#	Arguments:
#		time - float number of seconds
#
############################################################################
proc keyanimSTimeToPos { time } {
    global keyanimKeyList keyanimFrameRate
    global keyanimStartX keyanimEndX

    # Check for an animation first
    if {[llength $keyanimKeyList] == 0} then {
	return $keyanimStartX
    }

    set ctime $time

    set min_time [lindex [lindex $keyanimKeyList 0] 0]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]
    set pos [expr ($ctime - $min_time + 0.0) / ($max_time - $min_time)]

    set x_pos [expr $pos * ($keyanimEndX - $keyanimStartX + 0.0) + $keyanimStartX]

    return $x_pos
}

############################################################################
# procedure to convert from x position to current time
#
#	Arguments:
#		x - pixel position (between startx and endx)
#
############################################################################
proc keyanimPosToTime { x } {
    global keyanimKeyList keyanimFrameRate keyanimStartX keyanimEndX

    if {[llength $keyanimKeyList] == 0} then {
	return "00:00:00"
    }

    # Otherwise find the position of the slider relative to the end points and
    # determine the appropriate key time
    set min_time [lindex [lindex $keyanimKeyList 0] 0]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]
    set pos [expr ($x - $keyanimStartX + 0.0)/($keyanimEndX - $keyanimStartX)]
    set stime [expr $pos * ($max_time - $min_time + 0.0) + $min_time]

    # Convert to mm:ss:ff format
    set tm [expr int($stime) / 60]
    set ts [expr int($stime) % 60]
    set tf [expr int(($stime + 0.0 - ($tm * 60) - $ts) * $keyanimFrameRate)]
    set tm [keyanimPadNumber $tm]
    set ts [keyanimPadNumber $ts]
    set tf [keyanimPadNumber $tf]

    set text_time $tm
    append text_time : $ts : $tf

    return "$text_time"
}

############################################################################
# procedure to delete all selected keys
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#
############################################################################
proc keyanimDeleteKeys { BASE } {
    global keyanimKeyList keyanimSaved

    # Extract selected keys from keyframe list
    set select_keys [list]
    set j 0
    foreach i $keyanimKeyList {
	if {[lindex $i 3]} then {
	    lappend select_keys $j
	}
	incr j
    }

    if {[llength $select_keys] == 0} then {
	return
    }

    # Popup a dialog to verify the delete
    set ans [tk_dialog .delete_keys "Delete Keys" \
		 "Delete the selected keys?" \
		 {} 1 Ok Dismiss]
    if {$ans == 1} then { return }

    # Delete each key in turn
    set num_deletes 0
    set deleted_tags [list]
    foreach i $select_keys {
	set where [expr $i - $num_deletes]
	lappend deleted_tags [lindex [lindex $keyanimKeyList $where] 2]

	# Deleted the specified key internally
	set internal_time [lindex [lindex $keyanimKeyList $where] 0]
	if {[Ndelete_key $internal_time 0 1] == 0} then {
	    tk_dialog .ierror "Internal Error" \
		"Internal Error - Failed to delete keyframe in GK key list" \
		{} 0 Dismiss
	}
	Nupdate_frames

	set keyanimKeyList [lreplace $keyanimKeyList $where $where]
	incr num_deletes
    }

    # Need to remove the tags from the keyframe canvas explicitly first
    # and redraw all the keyframes
    keyanimUpdateFramesteps
    keyanimDrawKeys $BASE $deleted_tags
    set keyanimSaved 0
}


############################################################################
# procedure to select a keyframe
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#		tag - tag for this keyframe (VERY IMPORTANT)
#
############################################################################
proc keyanimSelectKey { BASE tag } {
    global keyanimKeyList

    # Flip the selected flag in the key list and display
    # the keytime in reverse video
    # First find the keyframe corresponding to the tag in the key list
    set found 0
    set index 0
    while {!$found} {
	set frame [lindex $keyanimKeyList $index]
	if {$tag == [lindex $frame 2]} then {
	    set found 1
	} else {
			incr index
	}
    }

    # Flip the selected flag in the current frame
    set frame [lreplace $frame 3 3 [expr ![lindex $frame 3]]]

    # And replace the current element in the key anim list
    set keyanimKeyList [lreplace $keyanimKeyList $index $index $frame]

    # Finally redraw the keys so we see the update
    keyanimDrawKeys $BASE
}

############################################################################
# procedure to move a keyframe
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#		x - x position for event
#		tag - tag for this keyframe (VERY IMPORTANT)
#
############################################################################
proc keyanimMoveKey { BASE x tag } {
    global keyanimStartX keyanimEndX
    global keyanimKeyList keyanimChannelList

    if {[llength $keyanimKeyList] < 2} then {
	return
    }

    if {($x >= $keyanimStartX) && ($x <= $keyanimEndX)} then {

	# From the position (i.e. x) figure out the new time for this
	# keyframe (used to update the textual pointer)
	set cur_time [keyanimPosToTime $x]
	$BASE.keycontrol.key_times.hold_times.times itemconfigure $tag -text "$cur_time"
	$BASE.keycontrol.key_times.hold_times.times coords $tag $x 1c

	# Now we have to go through all the channels and move the keyframe pointers
	# (if they exist)
	foreach i $keyanimChannelList {
	    # Extract the internal channel name
	    set iname [lindex $i 1]

	    # Now move the key if it exists
	    set rname $BASE
	    append rname .keycontrol.key_channels.hold_channels. $iname
	    $rname.keys coords $tag [expr $x - 4] 3m [expr $x + 4] 8m
	}
    }
}

############################################################################
# procedure to handle the move key-release sequence.
#   This procedure actually moves the key in the keyframe list
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#		x - x position of event
#		tag - tag for this keyframe (VERY IMPORTANT)
#
############################################################################
proc keyanimMoveKeyRelease { BASE x tag } {
    global keyanimKeyList keyanimFrameRate keyanimSaved

    # Need to find the keyframe with this tag and move it to it's new position
    # in the keyframe last
    set tag_list [list]
    foreach i $keyanimKeyList {
	lappend tag_list [lindex $i 2]
    }
    set key_num [lsearch -exact $tag_list $tag]
    set moved_key [lindex $keyanimKeyList $key_num]
    set old_time [lindex $moved_key 0]
    set keyanimKeyList [lreplace $keyanimKeyList $key_num $key_num]

    # Figure out the new time for this key
    set text_time [lindex [$BASE.keycontrol.key_times.hold_times.times \
			       itemconfigure $tag -text] 4]
    set text_time [split $text_time :]
    set tm [expr int([string trimleft [lindex $text_time 0] 0]0/10.0)]
    set ts [expr int([string trimleft [lindex $text_time 1] 0]0/10.0)]
    set tf [expr int([string trimleft [lindex $text_time 2] 0]0/10.0)]
    set new_time [expr $tm * 60 + $ts + 0.0 + ($tf + 0.0)/$keyanimFrameRate]
    set moved_key [lreplace $moved_key 0 0 $new_time]

    # Lastly, insert the key into it's appropriate place
    set i 0
    while {($i < [llength $keyanimKeyList]) &&
	   ([lindex [lindex $keyanimKeyList $i] 0] < $new_time)} { incr i }
    set keyanimKeyList [linsert $keyanimKeyList $i $moved_key]

    # Move the key internally
    if {[Nmove_key $old_time 0 $new_time] == 0} then {
	tk_dialog .ierror "Internal Error" \
	    "Internal Error - Failed to move keyframe in GK key list" \
	    {} 0 Dismiss
    }

    # After everything's said and done, redraw the keyframes
    keyanimUpdateFramesteps
    keyanimDrawKeys $BASE

    # One more thing, make sure the slider is somewhere
    # inbetween the first and last keyframes
    set stime [keyanimGetSliderTime $BASE]
    set ftime [lindex [lindex $keyanimKeyList 0] 0]
    set ltime [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]
    if {$stime < $ftime} then {
	keyanimPosSlider $BASE [keyanimTimeToString $ftime]
    }
    if {$stime > $ltime} then {
	keyanimPosSlider $BASE [keyanimTimeToString $ltime]
    }

    set keyanimSaved 0
}

############################################################################
# procedure to add a keyframe
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#
############################################################################
proc keyanimAddKey { BASE } {
    global keyanimChannelList keyanimKeyList keyanimSaved
    global keyanimCurrentKeyTime keyanimFrameRate

    set oBASE $BASE
    set key_mask [list]
    append BASE ".keycontrol.key_channels.hold_channels"

    # Pretty straightforward, record the current key-time, then look at
    # all the channels and record any values for which the channel
    # entry is "ON".
    # First extract the time
    set new_key [list]
    set time [split $keyanimCurrentKeyTime :]
    set tm [expr int([string trimleft [lindex $time 0] 0]0/10.0)]
    set ts [expr int([string trimleft [lindex $time 1] 0]0/10.0)]
    set tf [expr int([string trimleft [lindex $time 2] 0]0/10.0)]
    set new_time [expr $tm*60. + $ts + (1.0*$tf/$keyanimFrameRate) + 0.0]
    lappend new_key $new_time

    # having fetched the time we advance by 3 seconds for the next frame
    set dt 3
    incr ts $dt
    if { $ts >= 60 } then {
	incr ts -60
	incr tm
    }

    set keyanimCurrentKeyTime [keyanimPadNumber $tm]
    append keyanimCurrentKeyTime : [keyanimPadNumber $ts] : [keyanimPadNumber $tf]

    # Now for each channel entry, extract values if the channel is on
    set new_elements [list]
    foreach i $keyanimChannelList {
	set rname $BASE
	append rname . [lindex $i 1]
	set iname [lindex $i 1]
	upvar $iname deref

	if {$deref == 1} then {
	    # In this case add attributes for this channel
	    switch [lindex $i 0] {
		"FromX" { lappend key_mask KF_FROMX_MASK }
		"FromY" { lappend key_mask KF_FROMY_MASK }
		"FromZ" { lappend key_mask KF_FROMZ_MASK }
		"DirX"  { lappend key_mask KF_DIRX_MASK  }
		"DirY"  { lappend key_mask KF_DIRY_MASK  }
		"DirZ"  { lappend key_mask KF_DIRZ_MASK  }
		"FOV"    { lappend key_mask KF_FOV_MASK   }
		"TWIST"  { lappend key_mask KF_TWIST_MASK }
	    }

	    # Now see if we need to call an external function to get channel entry values
	    if {[lindex $i 2] != "null"} then {
		set an_element [list [lindex $i 0] [[lindex $i 3]]]
	    } else {
		set value values
                switch [lindex $i 0] {
                "FromX" { set value [lindex [Nget_from] 0] }
                "FromY" { set value [lindex [Nget_from] 1] }
                "FromZ" { set value [lindex [Nget_from] 2] }
# ACS_MODIFY: Nget_focus replaced by Nget_viewdir to work with flythrough
		"DirX"  { set value [lindex [Nget_viewdir] 0]}
		"DirY"  { set value [lindex [Nget_viewdir] 1]}
		"DirZ"  { set value [lindex [Nget_viewdir] 2]}
                "FOV"   { set value [lindex [Nget_fov] 0] }
                "TWIST" { set value [lindex [Nget_twist] 0] }
                }
		set an_element [list [lindex $i 0] $value]
	    }

	    lappend new_elements $an_element
	}
    }
    lappend new_key $new_elements

    # Now insert the new element into it's appropriate place in the keyframe list
    # Check to see if the key already exists, if so then we don't need to append
    set extract_times [list]
    set extract_where 0
    foreach i $keyanimKeyList {
	lappend extract_times [lindex $i 0]
    }

    set extract_where [lsearch -exact $extract_times $new_time]
    if { $extract_where != -1} then {

	# Found the new key, do a popup for the replace
	set ans [tk_dialog .replace "Replace Key" \
		     "There is already a keyframe at this time, replace it?" \
		     {} 1 Ok Dismiss]
	if {$ans == 1} then { return }

	# Otherwise do the replace
	lappend new_key [lindex [lindex $keyanimKeyList $extract_where] 2] 0
	set keyanimKeyList [lreplace $keyanimKeyList \
				$extract_where $extract_where $new_key]
	set keyanimSaved 0

	# Now add the key internally
	# We do this by going through each of the modified channels
	Nadd_key $new_time $key_mask 1 0.0
#		foreach i $new_elements {
#			switch [lindex $i 0] {
#				camera { Nadd_key $new_time [list KF_ALL_MASK] 1 0.0 }
#				light  { puts "Lights not affected at this time" }
#			}
#		}

    } else {

	# Generate new tag and selected flag for this keyframe
	lappend new_key [keyanimGenTag] 0

	# Insert the new key into the keyframe list
	if {[llength $keyanimKeyList] == 0} then {
	    set keyanimKeyList [list $new_key]
	} else {
	    set i 0
	    while {($i < [llength $keyanimKeyList]) &&
		   ([lindex [lindex $keyanimKeyList $i] 0] < $new_time)} {incr i}
	    set keyanimKeyList [linsert $keyanimKeyList $i $new_key]
	}

	# Now add the key internally
	# We do this by going through each of the modified channels
	Nadd_key $new_time $key_mask 1 0.0
#		foreach i $new_elements {
#			switch [lindex $i 0] {
#				camera { Nadd_key $new_time [list KF_ALL_MASK] 1 0.0 }
#				light  { puts "Lights not affected at this time" }
#			}
#		}

    }

    # Update framesteps
    keyanimUpdateFramesteps

    # Finally, call the redraw so we can see our new frame
    keyanimDrawKeys $oBASE
}

############################################################################
# procedure to update number of framesteps based on start
# and end keytime and framerate.
#
#	Arguments:
#		None
#
############################################################################
proc keyanimUpdateFramesteps {} {
    global keyanimKeyList keyanimFrameRate

    # Only do if there are at least two keyframes
    if {[llength $keyanimKeyList] < 2} then {
	return
    }

    # Otherwise figure out the total number of frames
    set start_time [lindex [lindex $keyanimKeyList 0] 0]
    set end_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]
    set diff [expr $end_time - $start_time + 0.0]
    set num_steps [expr int($diff * $keyanimFrameRate)]

    # Now make the change internally
    Nset_numsteps $num_steps
    Nupdate_frames
}

############################################################################
# procedure to make keyframe slider for keyanimation panel
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkkeyanimationPanel)
#
############################################################################
proc mkkeyframeArea { BASE } {
    global keyanimStartX keyanimEndX
    global keyanimChannelList

    set oBASE $BASE
    append BASE .keycontrol

    # Create canvas to hold channels for keyframes
    label $BASE.title -text "Keyframe Attributes"
    canvas $BASE.key_channels -width 10c -height 2c -yscrollcommand "$BASE.scrolly set" \
	-xscrollcommand "$BASE.scrollx set" -confine true -xscrollincrement 1 \
	-yscrollincrement 1
    $BASE.key_channels configure -scrollregion [list 0 0 28c 0]
    canvas $BASE.key_times -width 10c -height 3.7c \
	-xscrollcommand "$BASE.scrollx set" -confine true \
	-xscrollincrement 1 -yscrollincrement 1
    scrollbar $BASE.scrolly -orient vertical 	-command "$BASE.key_channels yview"
    scrollbar $BASE.scrollx -orient horizontal	-command "keyanimSpecialHScroll $BASE"
    $BASE.key_times configure -scrollregion [list 0 0 28c 2c]
    $BASE.scrolly set 0.0 0.1
    $BASE.scrollx set 0.0 0.36
    pack $BASE.title -side top -fill x
    pack $BASE.key_channels $BASE.key_times -fill both
    pack $BASE.scrolly -side left 	-fill y -padx 2 -pady 2 -before $BASE.key_channels
    pack $BASE.scrollx -side bottom	-fill x -padx 2 -pady 2

    # Create frame which holds channel entries
    frame $BASE.key_channels.hold_channels -width 10c -height 10c -bg skyblue
    $BASE.key_channels create window 0 0 -anchor nw \
	-window $BASE.key_channels.hold_channels -tag hc


    # Create frame which holds keytimes for keyframes
    frame $BASE.key_times.hold_times -width 10c -height 10c
    label $BASE.key_times.hold_times.spacer1 -width 10 -text ""
    checkbutton $BASE.key_times.hold_times.spacer2 \
	-text "On" -state disabled -relief flat -bd 0 -padx 3
    $BASE.key_times.hold_times.spacer2 configure \
	-disabledforeground [lindex [$BASE.key_times.hold_times.spacer2 \
					 configure -background] 4]
    canvas $BASE.key_times.hold_times.times -width 24c \
	-height 3.7c -relief raised -bg white
    pack $BASE.key_times.hold_times.spacer1 $BASE.key_times.hold_times.spacer2 \
	$BASE.key_times.hold_times.times -side left -fill both
    $BASE.key_times create window 0 0 -anchor nw -window $BASE.key_times.hold_times -tag ht

}

############################################################################
# Special horizontal scroll procedure since our horizontal scroll bar
# actually controls two widgets
#
############################################################################
proc keyanimSpecialHScroll { BASE args } {
    eval $BASE.key_channels xview $args
    eval $BASE.key_times xview $args
}

############################################################################
# Simple routine to create a unique tag identifier
#
# 	Arguments:
#		None
#
############################################################################
proc keyanimGenTag {} {
    global keyanimUniqueTag keyanimKeyList

    set name "keyanimtag"
    set new 0
    set old_num 0
#Added check for identical keytags
    if {[llength $keyanimKeyList] > 0} {
    	foreach j $keyanimKeyList {
		set tmp [lindex [split [lindex $j 2] g] 1]
		if {$tmp > $old_num} {
		set max_num $tmp
		} else {
		set max_num $old_num
		}
    		if { $tmp == $keyanimUniqueTag} {
    			puts "Correcting identical tag ID"
			set new 1
    		}
		set old_num $tmp
    	}
    }
    if {$new > 0} {
	set keyanimUniqueTag [expr $max_num + 1]
    }
    append name $keyanimUniqueTag
    incr keyanimUniqueTag

    return $name
}

############################################################################
# procedure to play animation backwards
#
############################################################################
proc keyanimPlayBackward { BASE } {
    global keyanimPlayState keyanimKeyList

    # Pretty easy, just set the play state and start callbacks running
    if {[llength $keyanimKeyList] < 2} then {
	return
    }

    set keyanimPlayState backward
    keyanimOneBackward $BASE
}

############################################################################
# procedure to play animation one step backwards
#
############################################################################
proc keyanimOneBackward { BASE } {
    global keyanimPlayState keyanimKeyList keyanimFrameRate keyanimPrecis

    # Get the current time from the time counter
    if {[llength $keyanimKeyList] < 2} then {
	return
    }
    set rname $BASE
    append rname ".keycontrol.key_times.hold_times.times"
    set text_time [lindex [$rname itemconfigure cur_time -text] 4]
    set times [split $text_time :]

    # Convert time to all seconds
    set tm [expr int([string trimleft [lindex $times 0] 0]0/10.0)]
    set ts [expr int([string trimleft [lindex $times 1] 0]0/10.0)]
    set tf [expr int([string trimleft [lindex $times 2] 0]0/10.0)]
    set aux_f $tf
    set tf [expr ($tf + 0.0) / $keyanimFrameRate]
    set ctime [expr $tm * 60 + $ts + $tf + 0.0]

    # Get time boundaries of animation and figure out if we can play
    # a frame back.
    set min_time [lindex [lindex $keyanimKeyList 0] 0]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]
    set ctime [expr $ctime - (1.0 / $keyanimFrameRate)]

    if {[expr $ctime - $min_time] >= [expr -1 * $keyanimPrecis] } then {
	incr aux_f -1
		if {$aux_f < 0} then {
		    set aux_f [expr $keyanimFrameRate - 1]
		    incr ts -1
		    if {$ts < 0} then {
			set ts 59
			incr tm -1
		    }
		}
	set aux_f [keyanimPadNumber $aux_f]
	set ts [keyanimPadNumber $ts]
	set tm [keyanimPadNumber $tm]
	set aux_time $tm
	append aux_time : $ts : $aux_f

	keyanimPosSlider $BASE $aux_time
    } else {
	keyanimStop $BASE
    }

    # Now add a callback if we are in the process of playing
    if {"$keyanimPlayState" == "backward"} then {
	after 1 keyanimOneBackward $BASE
    }
}

############################################################################
# procedure to stop animation
#
############################################################################
proc keyanimStop { BASE } {
    global keyanimPlayState ScriptPlaying keyanimOff IMG

	if {$keyanimPlayState == "run_and_save"} {
		if {$keyanimOff == 1} {
			Noff_screen 0
		}
		if {$IMG == 4} {
			Nclose_mpeg
		}
		
	}
    set keyanimPlayState stop
    if $ScriptPlaying then {
	send script_play {set pause_play 1}
    }
}

############################################################################
# procedure to play animation one step forward
#
############################################################################
proc keyanimOneForward { BASE } {
    global keyanimPlayState keyanimKeyList keyanimFrameRate keyanimPrecis
    global keyanimFrameNum keyanimSaveRenderStyle

    # Get the current time from the time counter
    if {[llength $keyanimKeyList] < 2} then {
	return
    }
    set rname $BASE
    append rname ".keycontrol.key_times.hold_times.times"
    set text_time [lindex [$rname itemconfigure cur_time -text] 4]
    set times [split $text_time :]

    # Convert time to all seconds
    set tm [expr int([string trimleft [lindex $times 0] 0]0/10.0)]
    set ts [expr int([string trimleft [lindex $times 1] 0]0/10.0)]
    set tf [expr int([string trimleft [lindex $times 2] 0]0/10.0)]
    set aux_f $tf
    set tf [expr ($tf + 0.0) / $keyanimFrameRate]
    set ctime [expr $tm * 60 + $ts + $tf + 0.0]

    # Get time boundaries of animation and figure out if we can play
    # a frame back.
    set min_time [lindex [lindex $keyanimKeyList 0] 0]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]
    set ctime [expr $ctime + (1.0 / $keyanimFrameRate)]

    if {[expr $ctime - $max_time] <= $keyanimPrecis } then {
	incr aux_f
	if {$aux_f >= $keyanimFrameRate} then {
	    set aux_f 0
	    incr ts
	    if {$ts == 60} then {
		set ts 0
		incr tm
	    }
	}
	set aux_f [keyanimPadNumber $aux_f]
	set ts [keyanimPadNumber $ts]
	set tm [keyanimPadNumber $tm]
	set aux_time $tm
	append aux_time : $ts : $aux_f

	keyanimPosSlider $BASE $aux_time
    } else {
	keyanimStop $BASE
    }

    # Now add a callback if we are in the process of playing
    if {"$keyanimPlayState" == "forward"} then {
	after 1 keyanimOneForward $BASE
    }

    if {"$keyanimPlayState" == "run_and_save"} then {
	keyanimSaveFrame $keyanimFrameNum
	incr keyanimFrameNum
	after 1 keyanimOneForward $BASE
    }
}

############################################################################
# procedure to play animation forwards
#
############################################################################
proc keyanimPlayForward { BASE } {
    global keyanimPlayState keyanimKeyList ScriptState

    if {[llength $keyanimKeyList] < 2} then {
	return
    }

    # Pretty easy, just set play state and start the callbacks
    if $ScriptState then {
	Nv_script_add_string "set pause_play 0"
	Nv_script_add_string "tkwait variable pause_play"
    }
    set keyanimPlayState forward
    keyanimOneForward $BASE
}

############################################################################
# procedure to pad numerical argument with leading zeroes
############################################################################
proc keyanimPadNumber { num } {
    if {[string length $num] < 2} then {
	set temp 0
	append temp $num
	return $temp
    } else {
	return $num
    }
}

############################################################################
# Callback to change tension of spline interpolants
#
#	Arguments:
#		An integer betwen 0 and 1000 inclusive
#
############################################################################
proc keyanimChangeTension { val } {
    # Cast to floating point arithmetic and scale to [0,1]
    set val [expr ($val + 0.0) / 1000.0]

    # Finally, set the tension
    Nset_tension $val
}

############################################################################
# procedure to set the state of nviz given the current
# real time.  i.e. look through the channel list and call
# set_entry functions for each channel with a list of changes.
#
#	Arguments:
#		BASE - base name of kanimator panel
#		time - current time in seconds
#
############################################################################
proc keyanimSetChannels { BASE time } {
    global keyanimKeyList keyanimChannelList

    # First, figure out if we are on a keyframe or between keyframes
    set ignore_list [list FromX FromY FromZ DirX DirY DirZ FOV TWIST]
    set extract_times [list]
    set extract_where 0
    foreach i $keyanimKeyList {
	lappend extract_times [lindex $i 0]
    }
    set extract_where [lsearch -exact $extract_times $time]

    # set the two bounding keys based on whether or not we
    # found an exact match (i.e. we're on a keyframe)
    if { $extract_where != -1 } then {
	set first_key_index $extract_where
	set second_key_index $extract_where
    } else {
	set i 0
	while {($i < [llength $keyanimKeyList]) &&
	       ([lindex [lindex $keyanimKeyList $i] 0] < $time)} {incr i}
	set first_key_index [expr $i - 1]
	set second_key_index $i
    }

    # For each channel we do the following:
    # 1) Using the indices found above as a guide, look
    #    backward and forward for keyframes with an entry
    # 	 for the current channel name.  If we can't find
    #    a pair of keys then quit trivially.
    # 2) If we have a static change and the two key indices
    #    are the same then make the change (i.e. static changes
    #    only take effect at keyframe boundaries).
    # 3) If we have a dynamic change then interpolate based
    #    on the times of the two keyframes with entries for
    #    the given channel.

    foreach i $keyanimChannelList {
	set channel_name [lindex $i 0]
	set change_list [list]

	# First check if we should ignore this channel (do this for
	# all channels which handle camera motion since underlying
	# library handles the interpolation).
	if {[lsearch -exact $ignore_list $channel_name] != -1} then {
	    continue
	}

	# Otherwise, see if we can find keys with entries for the
	# given channel
	set first_key [keyanimFindKeyBack $channel_name $first_key_index]
	set second_key [keyanimFindKeyForward $channel_name $second_key_index]

	if {($first_key != -1) && ($second_key != -1)} then {

	    # Get the actual keys
	    set fkey [lindex $keyanimKeyList $first_key]
	    set skey [lindex $keyanimKeyList $second_key]

	    # Get the channel description from the channel list
	    set channel_desc [lindex [assoc $keyanimChannelList $channel_name] 2]

	    # For dynamic entries, interpolate between last and previous
	    # For static, no change unless first_key_index==second_key_index
	    set f_entry [lindex [assoc [lindex $fkey 1] $channel_name] 1]
	    set s_entry [lindex [assoc [lindex $skey 1] $channel_name] 1]

	    foreach desc $channel_desc {

		# Check static case
		if {[lindex $desc 1] == "static"} then {
		    if { $first_key == $second_key } then {
			lappend change_list [list [lindex $desc 0] \
						 [lindex [assoc $f_entry \
							      [lindex $desc 0]] 1]]
		    } else {
			continue
		    }
		} else {
		    # Otherwise dynamic so linearly interpolate between values
		    set f_val [lindex [assoc $f_entry [lindex $desc 0]] 1]
		    set s_val [lindex [assoc $s_entry [lindex $desc 0]] 1]
		    set t1 [lindex $fkey 0]
		    set t2 [lindex $skey 0]

		    if {$t1 == $t2} then {
			set x 0
		    } else {
			set x [expr ($time - $t1)/($t2 - $t1)]
		    }

		    set new_val [expr (1 - $x) * $f_val + $x * $s_val]

		    lappend change_list [list [lindex $desc 0] $new_val]
		}
	    }
	}

	[lindex $i 4] $change_list
    }
}

############################################################################
# procedure to look back from the current keyframe index for a keyframe
# containing an entry for the specified channel.  If none is found then
# -1 is returned.
#
#	Arguments:
#		cname - name of channel
#		cindex - current index
############################################################################
proc keyanimFindKeyBack { cname cindex } {
    global keyanimKeyList

    if {$cindex == -1} then {
	return -1
    } else {
	set ckey [lindex $keyanimKeyList $cindex]

	if {[assoc [lindex $ckey 1] $cname] != "NIL"} then {
	    return $cindex
	} else {
	    return [keyanimFindKeyBack $cname [expr $cindex - 1]]
	}
    }
}

############################################################################
# procedure to look forward from the current keyframe index for a keyframe
# containing an entry for the specified channel.  If none is found then
# -1 is returned.
#
#	Arguments:
#		cname - name of channel
#		cindex - current index
############################################################################
proc keyanimFindKeyForward { cname cindex } {
    global keyanimKeyList

    if {$cindex == [llength $keyanimKeyList]} then {
	return -1
    } else {
	set ckey [lindex $keyanimKeyList $cindex]

	if {[assoc [lindex $ckey 1] $cname] != "NIL"} then {
	    return $cindex
	} else {
	    return [keyanimFindKeyBack $cname [expr $cindex + 1]]
	}
    }
}

############################################################################
# Callback to run and save frames in an animation
#
############################################################################
proc keyanimRunAndSave { BASE } {
    global keyanimKeyList keyanimPlayState keyanimFrameRate
    global keyanimWaitPress keyanimBaseName keyanimSaveRenderStyle
    global IMG keyanimFrameNum keyanimStartFrame keyanimOff

    if {[llength $keyanimKeyList] < 2} then { return }

    # First create a popup to get the filename prefix to use
    # for images
    set keyanimWaitPress false
    set IMG 2
    toplevel .ras_fname
    frame .ras_fname.frame1
    frame .ras_fname.frame2
    frame .ras_fname.frame3
    frame .ras_fname.frame4
    frame .ras_fname.frame5
    label .ras_fname.title -text "Enter a base name:"
    entry .ras_fname.enter -relief sunken

    label .ras_fname.label -text "Render:"
    radiobutton .ras_fname.norm -text "Wireframe" \
	-variable keyanimSaveRenderStyle -value 0
    radiobutton .ras_fname.fancy -text "Full Rendering" \
	-variable keyanimSaveRenderStyle -value 1

    button .ras_fname.ok -text "Ok" -command "set keyanimWaitPress true"

    label .ras_fname.label1 -text "Image:"
    # Start at value 2. 1 used to be SGI .rgb support
    radiobutton .ras_fname.img2 -text "PPM" -variable IMG -value 2
    radiobutton .ras_fname.img3 -text "TIFF" -variable IMG -value 3
    radiobutton .ras_fname.img4 -text "MPEG-1" -variable IMG -value 4

    label .ras_fname.label2 -text "Start Frame:"
    entry .ras_fname.enter2 -relief sunken -width 6
    checkbutton .ras_fname.check1 -text "Off-Screen" \
    -variable "keyanimOff"

#Pack Menu
    pack .ras_fname.frame1 -side top -fill both -expand 1
    pack .ras_fname.frame2 -side top -fill both -expand 1
    pack .ras_fname.frame3 -side top -fill both -expand 1
    pack .ras_fname.frame4 -side top -fill both -expand 1
    pack .ras_fname.frame5 -side bottom -fill both -expand 1

    pack .ras_fname.title .ras_fname.enter -side top\
    -in .ras_fname.frame1 -fill both
    pack .ras_fname.label2 .ras_fname.enter2 -side left \
    -in .ras_fname.frame2 -fill both
    pack .ras_fname.check1 -side right \
    -in .ras_fname.frame2 -fill both
    pack .ras_fname.label1 .ras_fname.img2 \
	.ras_fname.img3 .ras_fname.img4 \
    -in .ras_fname.frame3 -side left -fill both
    pack .ras_fname.label .ras_fname.norm .ras_fname.fancy -side left \
    -in .ras_fname.frame4 -fill both
    pack .ras_fname.ok -side bottom -fill both \
    -in .ras_fname.frame5 -expand 1
    .ras_fname.enter2 insert end "0"

#Disable option for Off Screen render until checked
#    .ras_fname.check1 configure -state disabled

    tkwait variable keyanimWaitPress

    if {$keyanimOff == 1} {
	Noff_screen 1
    }

    set keyanimBaseName [.ras_fname.enter get]
    set keyanimStartFrame [.ras_fname.enter2 get]
    destroy .ras_fname

    if {$IMG == 4} {
	set fnameExt [file extension $keyanimBaseName]
#comment out auto-extension so that FFMPEG guess_format() can write to asf, avi, flv, swf, etc.
#	if { [string compare $fnameExt ".mpg"] != 0  &&
#		[string compare $fnameExt ".mpeg"] != 0 } then {
#	    append keyanimBaseName ".mpg"
#	}
	Ninit_mpeg $keyanimBaseName
    }

    # Automatically start from the beginning
    set first_time [lindex [lindex $keyanimKeyList 0] 0]
    set ft_tm [expr int($first_time) / 60]
    set ft_ts [expr int($first_time) % 60]
    set ft_tf [expr int(($first_time + 0.0 - ($ft_tm * 60) - $ft_ts) * $keyanimFrameRate)]
    set ft_tm [keyanimPadNumber $ft_tm]
    set ft_ts [keyanimPadNumber $ft_ts]
    set ft_tf [keyanimPadNumber $ft_tf]

    set keyanimPlayState run_and_save
    keyanimPosSlider $BASE "$ft_tm:$ft_ts:$ft_tf"

    # Save the first frame since we miss it initially
    keyanimSaveFrame 0
    set keyanimFrameNum 1

    keyanimOneForward $BASE

}

############################################################################
# Simple procedure to write out the current GL screen to a file
#
############################################################################
proc keyanimSaveFrame { fnum } {
    global IMG keyanimBaseName keyanimSaveRenderStyle keyanimStartFrame keyanimKeyList keyanimFrameRate

    # First create a file name
    set fname $keyanimBaseName
    set num [format "%04d" [expr $fnum + $keyanimStartFrame]]
    set max_time [lindex [lindex $keyanimKeyList [expr [llength $keyanimKeyList] - 1]] 0]

#    while {[string length $num] < 5} {
#	set num 0$num
#    }

    inform "Saving Frame $num of [expr int($max_time*$keyanimFrameRate)]"

    # Start at 2. 1 used to be SGI .rgb support
    if {$IMG == 2} {
        append fname $num ".ppm"
        Nwrite_ppm $fname
    }
    if {$IMG == 3} {
        append fname $num ".tif"
        Nwrite_tif $fname
    }
    if {$IMG == 4} {
	Nwrite_mpeg_frame
    }
}

############################################################################
# Procedure to return the current time from the slider in seconds
#
############################################################################
proc keyanimGetSliderTime { BASE } {
    global keyanimFrameRate

    set rname $BASE
    append rname ".keycontrol.key_times.hold_times.times"
    set text_time [lindex [$rname itemconfigure cur_time -text] 4]
    set times [split $text_time :]

    # Convert time to all seconds
    set tm [expr int([string trimleft [lindex $times 0] 0]0/10.0)]
    set ts [expr int([string trimleft [lindex $times 1] 0]0/10.0)]
    set tf [expr int([string trimleft [lindex $times 2] 0]0/10.0)]
    set tf [expr ($tf + 0.0) / $keyanimFrameRate]
    set ctime [expr $tm * 60 + $ts + $tf + 0.0]

    return $ctime
}

############################################################################
# Procedure to convert keytime in seconds to mm:ss:ff format
#
############################################################################
proc keyanimTimeToString { stime} {
    global keyanimFrameRate

    # Convert to mm:ss:ff format
    set tm [expr int($stime) / 60]
    set ts [expr int($stime) % 60]
    set tf [expr int(($stime + 0.0 - ($tm * 60) - $ts) * $keyanimFrameRate)]
    set tm [keyanimPadNumber $tm]
    set ts [keyanimPadNumber $ts]
    set tf [keyanimPadNumber $tf]

    set text_time $tm
    append text_time : $ts : $tf

    return "$text_time"
}

############################################################################
# Procedure to save the current animation to a file
#
############################################################################
proc keyanimSaveAnim { base } {
    global keyanimKeyList keyanimFrameRate keyanimSaved

    # Make sure there is an animation to save
    if {[llength $keyanimKeyList] == 0} then {
	tk_dialog .na_error "No Anim Error" \
	    "There are no keys in the current animation to save" \
	    {} 0 Dismiss
	return
    }

    # Otherwise get a filename and save the animation (currently
    # we only save the nviz maintained portion of the animation,
    # a separate file will be required for the gsf library maintained
    # portion)
    set file_name [create_file_browser .save_file_browser 1]
    if {$file_name == -1} return

    # give it a ".kanim" extension if it doesn't already have one
    if { [string compare [file extension $file_name] ".kanim"] != 0 } then {
	append file_name ".kanim"
    }

    # Simply write out the file as a string, tcl preserves list ordering,
    # etc.
    # File format:
    #      Framerate
    #      List of keys
    set file_handle [open $file_name w]
    puts $file_handle $keyanimFrameRate
    puts $file_handle $keyanimKeyList
    close $file_handle
    set keyanimSaved 1
}

############################################################################
# Procedure to load an animation from a file
#
############################################################################
proc keyanimLoadAnim { base } {
    global keyanimKeyList keyanimFrameRate keyanimSaved keyanimUniqueTag

    if {($keyanimSaved == 0) && ([llength $keyanimKeyList] != 0)} then {
	set status [tk_dialog .ka_check "Verify" \
			"Warning - Current animation has not been saved, continue?" \
			{} 1 Ok Cancel]
	if {$status == 1} return
    }

    # This is going to be more difficult, quite literally we have to
    # add the keys one at a time so that the appropriate display elements are
    # created
    # Get the file name
    set file_name [create_file_browser .load_file_browser 1]
    if {$file_name == -1} return

    set file_handle ""
    if {[catch "open $file_name r" file_handle] != 0} then {
	tk_dialog .ka_error "File Error" \
	    "Error - Could not open file $file_name for reading" \
	    {} 0 Dismiss
	return
    }

    # Read in the new framerate and keys into temporary storage
    gets $file_handle tempFrameRate
    gets $file_handle tempKeyList
    close $file_handle

    # Delete all the old keys
    # This is done by collecting all tags from the current keyframe list, then
    # calling keyanimDrawKeys with a list of the old tags
    set old_tag_list [list]
    foreach i $keyanimKeyList {
	lappend old_tag_list [lindex $i 2]
    }

      #Really make sure old frames are gone
      Nclear_keys
      Nupdate_frames

	#Attempt to set Key Frame positions from Mask
	#TODO get and set other mask features

	#Run through new frames and extract max ID number
	set max_id 0
	foreach i $tempKeyList {
		set id_num [lindex [split [lindex $i 2] g] 1]
		if {$id_num > $max_id} {
			set max_id $id_num
		}
	}

	#Use maxID number to set new UniqueTag
	set keyanimUniqueTag [expr $max_id + 1]
	set cnt 0

        foreach i $tempKeyList {
		set time [lindex $i 0]
		set id_num [lindex [split [lindex $i 2] g] 1]
                set k [lindex $i 1]
                        set name [lindex $k 0]
                        set value1 [lindex $name 1]
                        set name [lindex $k 1]
                        set value2 [lindex $name 1]
                        set name [lindex $k 2]
                        set value3 [lindex $name 1]
                #Set Center of View -- FROM
                Nmove_to $value1 $value2 $value3
                        set name [lindex $k 3]
                        set value1 [lindex $name 1]
                        set name [lindex $k 4]
                        set value2 [lindex $name 1]
                        set name [lindex $k 5]
                        set value3 [lindex $name 1]
		#Set Camera position -- TO
		Nset_viewdir $value1 $value2 $value3
                #Set FOV
                        set name [lindex $k 6]
                        set value1 [lindex $name 1]
                if {$value1 != "fov"} {Nset_fov $value1}
                #Set TWIST
                        set name [lindex $k 7]
                        set value1 [lindex $name 1]
                if {$value1 != "twist"} {Nset_twist $value1}

		#Compare tag id to all other Tag ID's and replace with
		#UnigieTag if duplicate
		set tag "keyanimtag"
                foreach m $tempKeyList {
                	if {$m != $i} {
                		set id_num_tmp [lindex [split [lindex $m 2] g] 1]
                		if {$id_num == $id_num_tmp} {
				puts "Replacing duplicate tag ID"
				append tag $keyanimUniqueTag
				set i [lreplace $i 2 2 $tag]
				set tempKeyList [lreplace $tempKeyList $cnt $cnt $i]
				incr keyanimUniqueTag
				break
                		}
                	}
                }


	Nadd_key $time KF_ALL_MASK 1 0.0
	incr cnt

		}
    set keyanimKeyList $tempKeyList
    keyanimDrawKeys $base $old_tag_list

    # Set the new framerate
    set keyanimFrameRate $tempFrameRate

    keyanimUpdateFramesteps

    set keyanimSaved 1
}
