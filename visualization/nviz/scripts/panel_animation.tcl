##########################################################################
# 
# Panel to create kframe animation of scenes produced in nviz.
#
##########################################################################
# Original author unknown (U.S. Army Construction Engineering Research 
#	Laboratory?)
#
# Major update of GUI December 2006, Michael Barton, Arizona State University
#
##########################################################################
# COPYRIGHT:	(C) 2006 by Michael Barton and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
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
global animNumFrames animStartX animEndX animFirstKeyX animLastKeyX
global animKeyList animUniqueTag animInterpType animPathState IMG

set animNumFrames 25
Nset_numsteps 25
set animStartX 3
set animEndX 270
set animFirstKeyX $animStartX
set animLastKeyX  $animEndX
set animKeyList [list]
set animUniqueTag 0
set animInterpType linear
set animPathState 0
set animVectState 0
set animSiteState 0
set animVolState 0
set animRunState stop
set animSaveRenderStyle 0
set IMG 2
# Update all the frames before we exit (should be easy since there aren't any)
Nupdate_frames

############################################################################
# procedure to reset animation panel
############################################################################
proc Nviz_animation_reset {} {
    global animKeyList animPathState animVectState animSiteState animVolState
    global animInterpType animRunState
    global animPanelBASE animStartX

    set BASE $animPanelBASE

    # Just erase all the frames and update the display
    foreach i $animKeyList {
	set tag1 [lindex $i 5]
	set tag2 $tag1
	append tag2 "_"
	$BASE.keycontrol.kslide delete $tag1 $tag2
	$BASE.keycontrol.kslide.slider delete $tag1 $tag2
    }

    set animKeyList [list]

    # Manage internal keyframe list
    Nclear_keys
    Nupdate_frames

    # Update the display
    animFixEndpoints $BASE
    set animPathState 0
    set animVectState 0
    set animSiteState 0
	set animVolState 0
    set animInterpType linear
    set animRunState stop
    $BASE.commands.interp.tension set 500
    $BASE.buttons.set_tot_frames delete 0 end
    $BASE.buttons.set_tot_frames insert end "25"
    animChangeNumFrames $BASE
    kfMoveSlider $animStartX $BASE
}

############################################################################
# procedure to make animation control area
###########################################################################
proc mkanimationPanel { BASE } {
    global animNumFrames animInterpType animPathState
    global bit_map_path animPanelBASE
    global nviztxtfont

    catch {destroy $BASE}
    set panel [St_create {window name size priority} $BASE "Simple Animation" 2 5]
    set animPanelBASE $BASE

    frame $BASE  -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Simple Keyframe Animation Panel"

	#animation playback control buttons
    set rname $BASE.buttons
    frame $rname -relief flat -borderwidth 0
    Button $rname.slow_back -bitmap @$bit_map_path/slow_left \
		-command "animStepBackward $BASE" -bd 1 \
		-helptext "Step backward frame by frame"
    Button $rname.stop -bitmap @$bit_map_path/stop \
		-command {set animRunState stop; move_position} -bd 1\
		-helptext "Pause playback"
    Button $rname.slow_forward -bitmap @$bit_map_path/slow_right \
		-command "animStepForward $BASE" -bd 1 \
		-helptext "Step forward frame by frame"
    Button $rname.forward -bitmap @$bit_map_path/right \
		-command "animRunAnimation $BASE" -bd 1 \
		-helptext "Run animation from beginning"

    label $rname.title -text "  set max frames:" -font $nviztxtfont -fg black
    entry $rname.set_tot_frames -width 4 -relief sunken -justify r
    $rname.set_tot_frames insert 0 $animNumFrames
    bind $rname.set_tot_frames <Key-Return> "animChangeNumFrames $BASE"

    pack $rname.slow_back $rname.stop $rname.slow_forward $rname.forward \
    	-side left -padx 1 -fill both -expand no
    pack $rname.set_tot_frames $rname.title \
    	-side right -fill x -expand no
    pack $rname -side top -padx 3 -pady 4 -expand 1 -fill both

    # Create mid section containing keyframe control panel
    frame $BASE.keycontrol 
    set rname $BASE.keycontrol
    label $rname.l -text "Key Frames"
    pack  $rname.l -fill x -expand yes -anchor n
    mkkeyframeSlider $BASE
    
    pack $rname.kslide -fill x -expand yes -anchor n
    pack $rname -side top -padx 3 -pady 4
    
    # Section for displaying current frame info.
    set rname [frame $BASE.keycontrol.info]
    label $rname.cf_label -text "Current frame:" -font $nviztxtfont -fg black
    label $rname.cur_frame -text "0" -width 4 -fg black -bg white \
    	-font $nviztxtfont -anchor e
    label $rname.kf_label -text "Selected key frame:" \
    	-font $nviztxtfont -fg black
    label $rname.cur_kframe -text "0" -width 4 -fg black \
    	-bg white -font $nviztxtfont -anchor e
    pack $rname.cf_label $rname.cur_frame -side left -expand 0 -fill x -anchor w
    pack $rname.cur_kframe $rname.kf_label -side right -expand 0 -fil x -anchor e
    pack $rname -side top -expand 1 -fill both -padx 3 -pady 2
    

    # Create bottom section containing command buttons
    set rname [frame $BASE.keycontrol.commands]
    Button $rname.add -text "Add frame" -command "animAddKey $BASE" \
    	-bd 1 -width 10
    Button $rname.clear -text "Clear all" -command "animClearAllKeys $BASE" \
    	-bd 1 -width 10
    pack $rname.add  -fill x -expand no -side left
    pack $rname.clear -fill x -expand no -side right
    pack $rname -side bottom -expand yes -fill both -padx 3 -pady 4

    set rname $BASE.options
    frame $rname -relief groove -bd 2
    frame $rname.row1
    label $rname.row1.lab -text "Show: " -fg black
    checkbutton $rname.row1.spath -text "path" -variable animPathState \
		-command {Nshow_path $animPathState} -onvalue 1 -offvalue 0
    checkbutton $rname.row1.svect -text "lines" -variable animVectState \
		-command {Nshow_vect $animVectState} -onvalue 1 -offvalue 0
    checkbutton $rname.row1.ssite -text "points" -variable animSiteState \
		-command {Nshow_site $animSiteState} -onvalue 1 -offvalue 0
	checkbutton $rname.row1.svol -text "volumes" -variable animVolState \
		-command {Nshow_vol $animVolState} -onvalue 1 -offvalue 0
    pack $rname.row1.lab $rname.row1.spath $rname.row1.svect $rname.row1.ssite $rname.row1.svol\
    	-fill y -side left -padx 2 -pady 2
    pack $rname.row1 -side top -anchor w

    frame $rname.row2
    label $rname.row2.lab -text "Interpolation: " -fg black
    radiobutton $rname.row2.linear -text "linear" -variable animInterpType \
		-value linear -command "Nset_interp_mode linear
			Nupdate_frames
			mkTensionScale linear $rname.row2"
    radiobutton $rname.row2.spline -text "spline" -variable animInterpType \
		-value spline -command "Nset_interp_mode spline
			Nupdate_frames
			mkTensionScale spline $rname.row2"
    pack $rname.row2.lab $rname.row2.linear $rname.row2.spline \
    	-side left -padx 3 -pady 3
    pack $rname.row2 -side top -anchor w
    pack $rname -padx 3

    set rname $BASE.bottom
    frame $rname -relief groove
    Button $rname.rands -text "Run and save" -bd 1 \
    	-command "animRunAndSave $BASE" \
    	-helptext "Run animation and save frames"
    Button $rname.close -text Close -bd 1 \
    	-command "Nv_closePanel $BASE"
    pack $rname.rands -side left -fill y
    pack $rname.close -side right -fill y
    pack $rname -padx 3 -pady 3


    pack $BASE.buttons $BASE.keycontrol $BASE.options \
    $BASE.bottom \
      -side top -pady 3 -fill both -expand yes


    return $panel
}

proc mkTensionScale {type BASE} {
	catch {destroy $BASE.tension}

	if {$type == "spline"} {
		set tscale [scale $BASE.tension -label "spline tension" -orient h -showvalue f \
			-from 0 -to 1000 -command animChangeTension \
			-activebackground gray80 -background gray90 -showvalue 1 -width 14]
	
		$tscale set 500
		pack $tscale -side left
	}

}

############################################################################
# procedure to change the tension of spline interpolants
#
#	Arguments:
#		An integer between 0 and 1000 inclusive
#
############################################################################
proc animChangeTension { val } {
    # Cast to floating point arithmetic and scale to 0->1
    set val [expr ($val + 0.0) / 1000.0]

    # Finally, set the tension
    Nset_tension $val
}

############################################################################
# procedure to make keyframe slider for animation panel
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkanimationPanel)
#
############################################################################
proc mkkeyframeSlider { BASE } {
    global animStartX animEndX animFirstKeyX animLastKeyX
    global bit_map_path

    set oBASE $BASE
    append BASE .keycontrol.kslide

    # We use a canvas widget with a few special objects and some
    # special callbacks to simulate the keyframe slider.
    # Specifically, a rectangle object is used to indicate the current key frame
    # slider and independently created polygonal objects are used to indicate the set keyframes

    # Container and keyframe canvases (canvi?)
    canvas $BASE -width 340 -height 60
    canvas $BASE.slider -width 300 -height 25 -relief sunken -bd 2 -bg white
    $BASE create window 10 20 -window $BASE.slider -tags key_slider -anchor nw
    pack $BASE -side top
    
    # Trashcan for deleting single key frames
	image create photo trash -file "$bit_map_path/trash.gif"
    $BASE create image 332 15 -image "trash" -tags dump_key
	DynamicHelp::register "trash" balloon [G_msg "Test"]


    # Make sure we record the dimensions so our slider works correctly
    set temp [$BASE bbox key_slider]
    set temp [expr [lindex $temp 2] - [lindex $temp 0]]
    set animStartX 3
    set animEndX [expr $temp - 8]
    set animFirstKeyX $animStartX
    set animLastKeyX $animEndX

    # Create and bind a sliding rule to indicate the current keyframe
    bind $BASE.slider <1> "kfMoveSlider %x $oBASE"
    $BASE.slider create rectangle 1 0 6 25 -fill blue -outline black -tags cur_keyframe
    $BASE.slider bind cur_keyframe <B1-Motion> "kfMoveSlider %x $oBASE"

    # Create an initial set of tickmarks
    kfCreateTicks $oBASE
    $BASE.slider raise ticks
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
proc kfMoveSlider { x BASE } {
    global animStartX animEndX
    set oBASE $BASE
    append BASE ".keycontrol.kslide.slider"

    # We constrain slider movement by the variables animStartX and animEndX
    if {($x >= $animStartX) && ($x <= $animEndX)} then {
	$BASE coords cur_keyframe $x 0 [expr $x + 5] 1c
    }

    # Update the frame number to indicate the current position of the slider
    set pos [expr ($x - $animStartX + 0.0)/($animEndX - $animStartX)]
    set frame [animPosToFrame $pos]
    $oBASE.keycontrol.info.cur_frame configure -text "$frame"
    Ndo_framestep [expr $frame + 1] 0

}

############################################################################
# procedure to get keyframe slider location
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkanimationPanel)
#
############################################################################
proc kfGetSliderPos { BASE } {
    global animStartX animEndX
    append BASE ".keycontrol.kslide.slider"

    # Pretty simple, just figure out the distance by using the x position
    # and the bounds of the slider as determined by animStartX and animEndX.
    # First get the actual position of the slider
    set coordx [lindex [$BASE coords cur_keyframe] 0]

    # Make value into floating point so we use floating point math
    set pos [expr $coordx + 0.0]

    # Now figure out the position relative to start and end
    set pos [expr ($pos - $animStartX)/($animEndX - $animStartX)]

    # Return, we're done
    return $pos
}

############################################################################
# procedure to set position of keyframe slider
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkanimationPanel)
#
# 	Position is assumed to be between 0 and 1 inclusive
#
############################################################################
proc kfSetSliderPos { BASE pos } {
    global animStartX animEndX

    set oBASE $BASE
    append BASE ".keycontrol.kslide"

    # Determine x coordinate relative to pos on slider
    set x [expr $pos * ($animEndX - $animStartX) + $animStartX]
    set x [expr int($x)]
    append BASE ".slider"

    # We constrain slider movement by the variables animStartX and animEndX
    if {($x >= $animStartX) && ($x <= $animEndX)} then {
	$BASE coords cur_keyframe $x 0 [expr $x + 5] 1c
    }

    # Update the frame number to indicate the current position of the slider
    set pos [expr ($x - $animStartX + 0.0)/($animEndX - $animStartX)]
    set frame [animPosToFrame $pos]
    $oBASE.keycontrol.info.cur_frame configure -text "$frame"

}

############################################################################
# procedure to make tickmarks on slider
#
#	Arguments:
#		BASE - Base name of animation panel (returned by mkanimationPanel)
#
#	In actuality, the ticks should be placed between the most extreme keyframes
#	We'll implement this when we get the animation library (and hence keyframe
#	management) into nvwish
#
###########################################################################
proc kfCreateTicks { BASE } {
    global animNumFrames animStartX animEndX
    global animFirstKeyX animLastKeyX

    append BASE ".keycontrol.kslide"

    set spc [expr ($animLastKeyX - $animFirstKeyX + 0.0) / ($animNumFrames - 1.0)]

    $BASE.slider delete ticks

    set j 0
    for {set i $animFirstKeyX} { $j < $animNumFrames } {set i [expr $i + $spc]} {
	$BASE.slider create line $i 5m $i 1c -fill black -tags ticks
	incr j
    }
}

###########################################################################
# Procedure to refresh the animation endpoints after keyframe modification
#
#	Arguments:
#		BASE - Base frame of animation control panel (returned by mkanimationPanel)
#
###########################################################################
proc animFixEndpoints { BASE } {
    global animNumFrames animKeyList
    global animStartX animEndX animFirstKeyX animLastKeyX

    # If the animation is empty then set default end points
    if {[llength $animKeyList] == 0} then {
	set animFirstKeyX	$animStartX
	set animLastKeyX	$animEndX
    } else {
	# Otherwise find the extreme endpoints and set those
	# Anim list is already sorted so this is easy
	set min [lindex [lindex $animKeyList 0] 0]
	set max [lindex [lindex $animKeyList [expr [llength $animKeyList] - 1]] 0]
	set minx [expr $min * ($animEndX - $animStartX + 0.0) + $animStartX]
	if {$min == $max} then {
	    set maxx $animEndX
	} else {
	    set maxx [expr $max * ($animEndX - $animStartX + 0.0) + $animStartX]
	}
	set animFirstKeyX 	$minx
	set animLastKeyX	$maxx
    }

    # Finally redraw the tickmarks
    kfCreateTicks $BASE
}

###########################################################################
# Procedure to change the number of frames in the animation
#
#	Arguments:
#		BASE - Base frame of animation control panel (returned by mkanimationPanel)
#
###########################################################################
proc animChangeNumFrames { BASE } {
    global animNumFrames animStartX animEndX

    # Make sure num frames is at least 1
    set newNumFrames [$BASE.buttons.set_tot_frames get]
    if {$newNumFrames < 1} then {
	$BASE.buttons.set_tot_frames delete 0 end
	$BASE.buttons.set_tot_frames insert 0 "$animNumFrames"
	return
    }

    # Otherwise set the new number of frames and adjust the frame slider
    set animNumFrames $newNumFrames
    kfCreateTicks $BASE

    # Update internal keyframe management
    Nset_numsteps $animNumFrames
    Nupdate_frames
}

############################################################################
# Procedure to create a keyframe object and place it on the keyframe slider
#
#	Arguments:
#		BASE	- Base name of animation panel widget
#		pos 	- Position between 0 and 1 (inclusive) of new keyframe
#		tag	- tag to associate with this keyframe (so it may be identified later)
#
############################################################################
proc kfMakeKeyPointer { BASE pos tag } {
    global animStartX animEndX

    set oBASE $BASE
    append BASE ".keycontrol.kslide"

    # Get configuration of slider so we can figure out where to put the new keyframe
    set bd_width [lindex [$BASE.slider configure -bd] 4]
    set x [expr $pos * ($animEndX - $animStartX + 0.0) + $animStartX]
    set lx [expr 0.0 + [lindex [$BASE coords key_slider] 0] + $x]
    set tag1 $tag
    set tag2 $tag
    append tag2 "_"

    # Create the top portion consisting of a polygon with an outline
    $BASE create polygon $lx 10 [expr $lx - 4] 20 [expr $lx + 4] 20 $lx 10 \
	-fill green -tags $tag1
    $BASE create line    $lx 10 [expr $lx - 4] 20 [expr $lx + 4] 20 $lx 10 \
	-fill black -tags $tag2

    # Create the bottom portion consisting of a polygon with an outline
    $BASE.slider create polygon $x 10 [expr $x - 4] 0 [expr $x + 4] 0 $x 10 \
	-fill green -tags $tag1
    $BASE.slider create line    $x 10 [expr $x - 4] 0 [expr $x + 4] 0 $x 10 \
	-fill black -tags $tag2

    # Create bindings for the top portion which controls keyframe movement
    $BASE bind $tag1 <B1-Motion>       "kfKeyPointerMove $oBASE %x $tag1"
    $BASE bind $tag1 <ButtonRelease-1> "kfKeyPointerMoveBot $oBASE $tag1"
    $BASE bind $tag2 <B1-Motion>	   "kfKeyPointerMove $oBASE %x $tag1"
    $BASE bind $tag2 <ButtonRelease-1> "kfKeyPointerMoveBot $oBASE $tag1"
}

############################################################################
# Callback to move keyframe pointers
#
#	The callbacks are constructed in two parts since the top half
#	of a keyframe is dragged while the bottom moves AFTER the button
#	is released.  This routine handles the first half.
#
#	Arguments:
#		BASE 	- Base name of animation panel (returned by mkanimationPanel)
#		x 		- x position of mouse when button pressed
#		tag		- tag for keyframe pointer to move
#
############################################################################
proc kfKeyPointerMove { BASE x tag } {
    global animStartX animEndX

    append BASE ".keycontrol.kslide"

    # Figure out the configuration of the slider
    set lx [lindex [$BASE coords key_slider] 0]
    set x [expr $x - $lx]
    set nx [expr $lx + $x]
    set tag1 $tag
    set tag2 $tag
    append tag2 "_"

    # If x position is within bounds of slider then we'll move everything
    if {($x < $animStartX) || ($x > [expr $animEndX + 20])} then {
	return
    }

    # Change coordinates of object
    $BASE coords $tag1 $nx 10 [expr $nx - 4] 20 [expr $nx + 4] 20 $nx 10
    $BASE coords $tag2 $nx 10 [expr $nx - 4] 20 [expr $nx + 4] 20 $nx 10
}

############################################################################
# Callback to move bottom half of keyframe pointer
# This case is more complicated because we have to move the keyframe in the list
# as well.
#
#	Arguments:
#		BASE 	- Base name of animation panel (returned by mkanimationPanel)
#		tag		- tag name of keyframe pointer
#
############################################################################
proc kfKeyPointerMoveBot { BASE tag } {
    global animStartX animEndX animKeyList
    set oBASE $BASE
    append BASE ".keycontrol.kslide"

    # Get coordinates of moved top half
    set x [lindex [$BASE coords $tag] 0]

    # Find center and use to redraw bottom half of keyframe pointer
    set x [expr $x - [lindex [$BASE coords key_slider] 0]]
    set tag1 $tag
    set tag2 $tag
    append tag2 "_"

    # Check if the keyframe pointer is over the trash can
    if {$x > $animEndX} then {
		set ans [tk_dialog .verify "Verify" "Do you really want to delete this keyframe?" \
				 {} 1 Ok Dismiss]
		if {$ans == 1} then {
	
			# Cancel the delete so move top pointer back to original position
			set x [expr [lindex [$BASE.slider coords $tag1] 0] + \
				   [lindex [$BASE coords key_slider] 0]]
			$BASE coords $tag1 $x 10 [expr $x - 4] 20 [expr $x + 4] 20 $x 10
			$BASE coords $tag2 $x 10 [expr $x - 4] 20 [expr $x + 4] 20 $x 10
	
		} else {
	
			# Do the delete:  Remove the key pointers and remove the key from the list
			$BASE delete $tag1 $tag2
			$BASE.slider delete $tag1 $tag2
	
			# Find the keyframe in the list
			set tag_list [list]
			foreach i $animKeyList { lappend tag_list [lindex $i 5] }
			set i [lsearch -exact $tag_list $tag1]
	
			# Remove the given keytime from the GK keyframe list
			set key_time [lindex [lindex $animKeyList $i] 0]
			puts "Deleting key at $key_time"
			if {[Ndelete_key $key_time 0 1] == 0} then {
			tk_dialog .ierror "Internal Error" \
				"Internal Error - Failed to delete keyframe in GK key list" \
				{} 0 Dismiss
			}
			Nupdate_frames
	
			# Remove the key from the tcl keyframe list
			set animKeyList [lreplace $animKeyList $i $i]
	
			# Update the display
			animFixEndpoints $oBASE
		}
	
		return
    }

    $BASE.slider coords $tag1 $x 10 [expr $x - 4] 0 [expr $x + 4] 0 $x 10
    $BASE.slider coords $tag2 $x 10 [expr $x - 4] 0 [expr $x + 4] 0 $x 10

    # Now find the entry in the key frame list
    set tags [list]
    foreach i $animKeyList {
		lappend tags [lindex $i 5]
    }
    set key_num [lsearch -exact $tags $tag]
    set moved_key [lindex $animKeyList $key_num]
    set animKeyList [lreplace $animKeyList $key_num $key_num]

    # Now figure out the new time for this key
    set old_time [lindex $moved_key 0]
    set new_time [expr ($x - $animStartX + 0.0) / ($animEndX - $animStartX)]
    set moved_key [lreplace $moved_key 0 0 $new_time]

    # Finally insert the key into it's appropriate place
    set i 0
    while {($i < [llength $animKeyList]) &&
	   ([lindex [lindex $animKeyList $i] 0] < $new_time)} { incr i }
    set animKeyList [linsert $animKeyList $i $moved_key]

    # Also, move the key in the GK keyframe list
    if {[Nmove_key $old_time 0 $new_time] == 0} then {
	tk_dialog .ierror "Internal Error" \
	    "Internal Error - Failed to move keyframe in GK key list" \
	    {} 0 Dismiss
    }
    
    set new_time_int [expr round($new_time * 100)]
    $oBASE.keycontrol.info.cur_kframe configure -text "$new_time_int"    
    Nupdate_frames

    # Update the display
    animFixEndpoints $oBASE
}

set animUniqueTag 0
############################################################################
# Simple routine to create a unique tag identifier
#
# 	Arguments:
#		None
#
############################################################################
proc animGenTag {} {
    global animUniqueTag

    set name "animtag"
    append name $animUniqueTag
    incr animUniqueTag

    return $name
}

############################################################################
# Two auxiliary routines to convert from a slider position to a frame
# number and vice versa.
############################################################################
proc animFrameToPos { frame } {
    global animNumFrames animStartX animEndX
    global animFirstKeyX animLastKeyX

    # Convert frame to value between 0 and 1.0 on keyframe scale
    set pos [expr ($frame + 0.0)/($animNumFrames - 1.0)]

    # Convert this value to an x position on the scale
    set xpos [expr $pos * ($animLastKeyX - $animFirstKeyX) + $animFirstKeyX]

    # Finally convert this x value to a postition between 0 and 1.0 on
    # the absolute scale
    set pos [expr ($xpos - $animStartX)/($animEndX - $animStartX)]

    if {$pos < 0} then {
	return 0
    }
    if {$pos > 1} then {
	return 1
    }

    return $pos
}

proc animPosToFrame { pos } {
    global animNumFrames animStartX animEndX
    global animFirstKeyX animLastKeyX

    # Convert pos to x value on total scale
    set xpos [expr $pos * ($animEndX - $animStartX) + $animStartX]

    # Convert new x position to position between 0 and 1 on keyframe scale
    set pos [expr ($xpos - $animFirstKeyX)/($animLastKeyX - $animFirstKeyX)]

    # Finally convert this value to an actual frame
    set frame [expr int($pos * ($animNumFrames - 1))]

    if {$frame < 0} then {
	return 0
    }

    if {$frame > [expr $animNumFrames - 1]} then {
	return [expr $animNumFrames - 1]
    }

    return $frame
}

############################################################################
# Callback to add a keyframe
#
#	Arguments:
#		BASE 	- Base name of animation panel (returned by mkanimationPanel)
#
############################################################################
proc animAddKey { BASE } {
    global animNumFrames animKeyList

    # Get the position of the slider, this gives the time for the new key
    set new_pos [kfGetSliderPos $BASE]

    # Set the attributes for this new key frame
    set new_key [list $new_pos [Nget_from] [Nget_to] [Nget_fov] [Nget_twist]]

    # Check to see if the key already exists, if so then we don't
    # need a new tag
    set extract_times [list]
    set extract_where 0
    foreach i $animKeyList {
	lappend extract_times [lindex $i 0]
    }

    set extract_where [lsearch -exact $extract_times $new_pos]
    if { $extract_where != -1} then {

	# Found the new key, do a popup for the replace
	set ans [tk_dialog .replace "Replace Key" \
		     "There is already a keyframe at this time, do you wish to replace it?" \
		     {} 1 Ok Dismiss]
	if {$ans == 1} then { return }

	# Otherwise do the replace
	lappend new_key [lindex [lindex $animKeyList $extract_where] 5]
	set animKeyList [lreplace $animKeyList $extract_where $extract_where $new_key]

	# Now add the key internally
	Nadd_key $new_pos [list KF_ALL_MASK] 1 0.0

    } else {

	# Create a new tag and append it
	set ntag [animGenTag]
	lappend new_key $ntag

	# Create the animation pointer on the display
	kfMakeKeyPointer $BASE $new_pos $ntag

	# And insert the new key into the keyframe list
	if {[llength $animKeyList] == 0} then {
	    set animKeyList [list $new_key]
	} else {
	    set i 0
	    while {($i < [llength $animKeyList]) &&
		   ([lindex [lindex $animKeyList $i] 0] < $new_pos)} {incr i}
	    set animKeyList [linsert $animKeyList $i $new_key]
	}

	# Now add the key internally
	Nadd_key $new_pos [list KF_ALL_MASK] 0 0.0
    }

    # Finally redraw the display
    animFixEndpoints $BASE
}

############################################################################
# Callback to erase the keyframe list
#
############################################################################
proc animClearAllKeys { BASE } {
    global animKeyList

    # First make sure they really want to do this
    if {[tk_dialog .verify "Verify" "Do you really want to delete all keyframes?" \
	     {} 1 Ok Dismiss] == 1} then {
	return
    } else {
	foreach i $animKeyList {
	    set tag1 [lindex $i 5]
	    set tag2 $tag1
	    append tag2 "_"
	    $BASE.keycontrol.kslide delete $tag1 $tag2
	    $BASE.keycontrol.kslide.slider delete $tag1 $tag2
	}

	set animKeyList [list]

	# Manage internal keyframe list
	Nclear_keys
	Nupdate_frames
    }

    # Update the display
    animFixEndpoints $BASE
}

############################################################################
# Two simple routines to step forward or backward one frame at a time
#
############################################################################
proc animStepForward { BASE } {
    global animNumFrames animKeyList animRunState animSaveRenderStyle IMG

    if {"$animRunState" == "run_and_save"} then {
	set style $animSaveRenderStyle
    } else {
	set style 0
    }

    if {[llength $animKeyList] < 2} then {
	set animRunState stop
	return
    }

    # Get current frame
    set cur_frame [lindex [$BASE.keycontrol.info.cur_frame configure -text] 4]

    # Increment if possible and update the display
    if {$cur_frame < [expr $animNumFrames - 1]} then {
		set cur_frame [expr $cur_frame + 1]
		kfSetSliderPos $BASE [animFrameToPos $cur_frame]
		$BASE.keycontrol.info.cur_frame configure -text "$cur_frame"
		Ndo_framestep [expr $cur_frame + 1] $style
    } else {
		$BASE.keycontrol.info.cur_frame configure -text "[expr $animNumFrames - 1]"
		Ndo_framestep $animNumFrames $style
		if {$IMG == "4" && $animRunState == "run_and_save"} {
			Nclose_mpeg
		}
		set animRunState stop
    }

    update

    if {"$animRunState" == "run"} then {
		after 1 animStepForward $BASE
    }

    if {"$animRunState" == "run_and_save"} then {
		animSaveFrame [lindex [$BASE.keycontrol.info.cur_frame configure -text] 4]
		after 1 animStepForward $BASE
    }
}

proc animStepBackward { BASE } {
    global animNumFrames animKeyList

    if {[llength $animKeyList] < 2} then { return }

    # Get current frame
    set cur_frame [lindex [$BASE.keycontrol.info.cur_frame configure -text] 4]

    # Increment if possible and update the display
    if {$cur_frame > 0} then {
		set cur_frame [expr $cur_frame - 1]
		kfSetSliderPos $BASE [animFrameToPos $cur_frame]
		$BASE.keycontrol.info.cur_frame configure -text "$cur_frame"
		Ndo_framestep [expr $cur_frame + 1] 0
    }

    update
}

############################################################################
# Callback to run an animation
#
############################################################################
proc animRunAnimation { BASE } {
    global animNumFrames animKeyList animRunState

    if {[llength $animKeyList] < 2} then { return }

    # If we are already at the end then restart from the beginning
    set cur_frame [lindex [$BASE.keycontrol.info.cur_frame configure -text] 4]
    if {$cur_frame >= [expr $animNumFrames - 1]} then {
		$BASE.keycontrol.info.cur_frame configure -text 0
		update
		Ndo_framestep 1 0
    }

    set animRunState run
    animStepForward $BASE

}

############################################################################
# Callback to run and save frames in an animation
#
############################################################################
proc animRunAndSave { BASE } {
    global animNumFrames animKeyList animRunState
    global IMG animWaitPress animBaseName animSaveRenderStyle
    global nviztxtfont

    if {[llength $animKeyList] < 2} then { return }

    # First create a popup to get the filename prefix to use
    # for images
    set animWaitPress false
    set IMG 2
    toplevel .ras_fname
    wm title .ras_fname "Save Animation Frames"
    set row1 [frame .ras_fname.frame1]
    set row2 [frame .ras_fname.frame2]
    set row3 [frame .ras_fname.frame3]
    set row4 [frame .ras_fname.frame4]
    
    Label $row1.label -text "Prefix for images: " \
    	-helptext "Enter a prefix name for images to be created from animation frames"
    entry $row1.enter -relief sunken
    pack $row1.label $row1.enter -side left -fill x -expand 0 -anchor w
    pack $row1 -side top -padx 3 -pady 4 -expand 1 -fill both
    
    Label $row2.formatlabel -text "Output format: "
    radiobutton $row2.img2 -text "PPM" -variable IMG -value 2
    radiobutton $row2.img3 -text "TIFF" -variable IMG -value 3
    radiobutton $row2.img4 -text "MPEG-1" -variable IMG -value 4
    pack $row2.formatlabel $row2.img2 $row2.img3 $row2.img4 -side left \
    	-anchor w -fill x -expand 0
    pack $row2 -side top -padx 3 -expand 1 -fill both
    
    Label $row3.rendlabel -text "Rendering mode: "
    radiobutton $row3.norm -text "coarse" -variable animSaveRenderStyle -value 0
    radiobutton $row3.fancy -text "fine" -variable animSaveRenderStyle -value 1
    pack $row3.rendlabel $row3.norm $row3.fancy -side left -anchor w \
    	 -fill x -expand 0
    pack $row3 -side top -padx 3 -pady 4 -expand 1 -fill both

    button $row4.ok -text "OK" -width 4 -bd 1 -command "set animWaitPress true"
    pack $row4.ok
    pack $row4 -side top -pady 3 -expand 1 -fill both

    tkwait variable animWaitPress
    set animBaseName [$row1.enter get]
    destroy .ras_fname

    if {$IMG == 4} {
		Ninit_mpeg $animBaseName
    }

    # If we are already at the end then restart from the beginning
    set cur_frame [lindex [$BASE.keycontrol.info.cur_frame configure -text] 4]
    if {$cur_frame >= [expr $animNumFrames - 1]} then {
		$BASE.keycontrol.info.cur_frame configure -text 0
		update
		Ndo_framestep 1 $animSaveRenderStyle
		animSaveFrame 0
    }

    set animRunState run_and_save
    animStepForward $BASE

}

############################################################################
# Simple procedure to write out the current GL screen to a file
#
############################################################################
proc animSaveFrame { fnum } {
    global IMG animBaseName animSaveRenderStyle

    # First create a file name
    set fname $animBaseName
    set num $fnum
    while {[string length $num] < 5} {
	set num 0$num
    }

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
