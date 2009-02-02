##########################################################################
#
# Main panel and display controls for NVIZ
# Probably originally written written ca. 1994 by
# U.S. Army Construction Engineering Research Laboratory
#
# Updates 2005 by Massimo Cuomo, ACS - m.cuomo at acsys.it
# Major update of GUI Nov 2006, Michael Barton, Arizona State University
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

#*** ACS_MODIFY 1.0 BEGIN ******************************************************
# flags to enable(1)/disable(0) added FlyThrough Functions
	set Nv_(FlyThrough) 1
	if {$Nv_(FlyThrough)} {source $src_boot/etc/nviz2.2/scripts/flythrough.tcl}

###########################################################################
# procedure to make main control area
###########################################################################

global arw_clr arw_text_clr
global src_boot
source $src_boot/etc/nviz2.2/scripts/config.tcl

proc mkmainPanel { BASE } {
	global Nv_
	global XY
	
	#Globals for draw features
	global surface vector sites volume
	global legend labels n_arrow scalebar
	global fringe fringe_elev fringe_color
	global n_arrow_x n_arrow_y n_arrow_z
	global arw_clr arw_text_clr
	global scalebar_x scalebar_y scalebar_z
	global bar_clr bar_text_clr
	global fringe_nw fringe_ne fringe_sw fringe_se
	global Nauto_draw

	#Set defaults
	set surface 1
	set vector 1
	set sites 1
	set volume 1

	set legend 0
	set labels 0
	set n_arrow 0
	set scalebar 0
	set fringe 0
	set fringe_color #AAAAAA

	#Set North Arrow defaults
	set n_arrow_x 999
	set n_arrow_y 999
	set n_arrow_z 999
	set arw_clr #000000
	set arw_text_clr #DDDDDD

	#Set Scalebar defaults
	set scalebar_x 999
	set scalebar_y 999
	set scalebar_z 999
	set bar_clr #000000
	set bar_text_clr #DDDDDD

	set Nv_(cursor) [$Nv_(TOP) cget -cursor]	

	catch {destroy $BASE}

	#  Initialize panel info
	if [catch {set Nv_($BASE)}] {
		set panel [St_create {window name size priority} $BASE "Main" 1 10]
	} else {
		set panel $Nv_($BASE)
	}

	frame $BASE -relief flat -borderwidth 0
		set Nv_(main_BASE) $BASE

	# make redraw button area
	pack [frame $BASE.redrawf -bd 1 -relief flat ] -padx 3 -pady 5 -side top -fill x -expand 1
	# frame for draw, clear, cancel buttons
	pack [frame $BASE.redrawf.f2 -relief flat -bd 0] -side top -fill x -expand 1
	# frame for auto checkbuttons
	pack [frame $BASE.redrawf.f1 -relief flat -bd 0] -side top -fill x -expand 1
	# frame for features to display menu buttons
	pack [frame $BASE.redrawf.f11 -relief flat -bd 0] -side top -fill x -expand 1

	#Execute buttons
	set drawbtn [button $BASE.redrawf.f2.exec -text DRAW -bd 1 -fg "darkgreen"]
	bind $drawbtn <1> "Nset_cancel 0"
	bind $drawbtn <B1-ButtonRelease> {Ndraw_all}
	help $drawbtn balloon "Draw selected features"

	set clearbtn [button $BASE.redrawf.f2.clear -text Clear	 -bd 1 -command {do_clear}]
	help $clearbtn balloon "Clear NVIZ display"

	set cancelbtn [button $BASE.redrawf.f2.cancel -text Cancel -bd 1 -command {Nset_cancel 1}]
	help $cancelbtn balloon "Cancel current draw"

	pack $drawbtn  $clearbtn $cancelbtn \
		-side left -expand 1 -fill x

	# Auto check boxes
	set labl1 [label $BASE.redrawf.f1.label1 -text "Automatically render display:" -anchor w]
	set auto_d [checkbutton $BASE.redrawf.f1.autodraw \
		-onvalue 1 -offvalue 0 -variable Nauto_draw \
		-justify left  -anchor center -padx 5]
	help $BASE.redrawf.f1.autodraw balloon "Automatically render display after changing parameters"
	$auto_d select
	pack $labl1 $auto_d -side left -expand 0 -fill x -pady 3
	
	#checkbuttons for features to draw
	set labl2 [label $BASE.redrawf.f11.label1 -text "Show features:" -anchor w]

	menubutton $BASE.redrawf.f11.m1 -menu $BASE.redrawf.f11.m1.m \
		-text "Main features..." -underline 0 -justify left \
		-indicator on -anchor center -relief raised -bd 1
	#help $BASE.redrawf.f11.m1 balloon "Select main draw features"

	menubutton $BASE.redrawf.f11.m2 -menu $BASE.redrawf.f11.m2.m \
		-text "Decorations..." -underline 0	 -justify left \
		-indicator on -anchor center -relief raised -bd 1
	#help $BASE.redrawf.f11.m2 balloon "Select misc. draw features"

	menu $BASE.redrawf.f11.m1.m

	$BASE.redrawf.f11.m1.m add checkbutton -label "Surface" \
		 -onvalue 1 -offvalue 0 -variable surface

	$BASE.redrawf.f11.m1.m add checkbutton -label "Vectors" \
		 -onvalue 1 -offvalue 0 -variable vector

	$BASE.redrawf.f11.m1.m add checkbutton -label "Sites" \
		 -onvalue 1 -offvalue 0 -variable sites

	$BASE.redrawf.f11.m1.m add checkbutton -label "Volumes" \
		 -onvalue 1 -offvalue 0 -variable volume

	menu $BASE.redrawf.f11.m2.m

	$BASE.redrawf.f11.m2.m add checkbutton -label "Legend" \
		 -onvalue 1 -offvalue 0 -variable legend
	$BASE.redrawf.f11.m2.m add checkbutton -label "Labels" \
		 -onvalue 1 -offvalue 0 -variable labels
	$BASE.redrawf.f11.m2.m add checkbutton -label "North Arrow" \
		 -onvalue 1 -offvalue 0 -variable n_arrow
	$BASE.redrawf.f11.m2.m add checkbutton -label "Scale Bar" \
		 -onvalue 1 -offvalue 0 -variable scalebar
	$BASE.redrawf.f11.m2.m add checkbutton -label "Fringe" \
		 -onvalue 1 -offvalue 0 -variable fringe

	pack $labl2	 $BASE.redrawf.f11.m1 $BASE.redrawf.f11.m2 -side left \
		-expand 1 -fill x

	#pack frames
	pack [frame $BASE.midt -relief flat -bd 0] -side top -expand 1 -fill x -padx 5 -pady 5
	pack [frame $BASE.midf -relief flat -bd 0] -side left -expand 1 -padx 5

	# set view method radiobuttons
	set draw_lab [label $BASE.midt.lablev1 -text "View method:" -anchor w]

	set draw_var1 [radiobutton $BASE.midt.b1 -text "eye" \
		-variable draw_option -value 0 -width 8 \
		-command "change_display 1" ]

	set draw_var2 [radiobutton $BASE.midt.b2 -text "center" \
		-variable draw_option -value 1 -width 8 \
		-command "change_display 0" ]
	$draw_var1 select

	help $BASE.midt.b1 balloon "Change view by moving eye position"
	help $BASE.midt.b2 balloon "Change view by moving scene center position"


	if {$Nv_(FlyThrough)} {
		mkFlyButtons $BASE "midt" $draw_lab $draw_var1 $draw_var2
	} else {
		# original code
		pack $draw_lab $draw_var1 $draw_var2 -side left -expand 0
	}
	help $BASE.midt.b3 balloon "Change view using mouse to control fly-through"

	# make	position "widget"
	set XY [Nv_mkXYScale $BASE.midf.pos puck XY_POS 125 125 105 105 update_eye_position]

	# make vertical exageration and eye height sliders
	set H [mk_hgt_slider $BASE.midf]
	set E [mk_exag_slider $BASE.midf]
	
	help $E.scale balloon "Set vertical exaggeration"
	help $E.entry balloon "Set vertical exaggeration"
	help $H.scale balloon "Set eye height"
	help $H.entry balloon "Set eye height"
	
	# make lookat buttons
	frame $BASE.midf.lookat -relief flat -borderwidth 0

	Label $BASE.midf.lookat.l -text "Look"
	Button $BASE.midf.lookat.here -text "here" -bd 1 \
		-helptext "Center view at point marked with mouse click" \
		-command {bind $Nv_(TOP).canvas <Button> {look_here %W %x %y
		if {$Nauto_draw == 1} {Ndraw_all}
		}}
		
	Button $BASE.midf.lookat.center -text "center" -bd 1 \
		-helptext "Center view at center of displayed surface" \
		-command { look_center
			if {$Nauto_draw == 1} {Ndraw_all} 
			}
	Button $BASE.midf.lookat.top -text "top" -bd 1 \
		-helptext "View directly from above" \
		-command {
			# Nv_itemDrag $Nv_(main_BASE).midf.pos $Nv_(XY_POS) 62.5 62.5
			# note: below value is somewhat strange, but with 0.5 0.5 the map rotates:
			#	update_eye_position 0.496802 0.50100
			set val2 [$Nv_(HEIGHT_SLIDER).f.entry get]
			Nset_focus_top $val2
			change_display 1
			update

			if {$Nauto_draw == 1} {Ndraw_all}
			}
			
	# CMB Nov.2006: As far as I can tell, this button does nothing. This command doesn't exist
	#button $BASE.midf.lookat.cancel -text "cancel"  -bd 1 -command no_focus
	
	# make perspective and twist sliders
	frame $BASE.bframe -relief flat -bd 0
	frame $BASE.bframe.cframe -relief flat -borderwidth 0

	set P [Nv_mkScale $BASE.bframe.cframe.pers h perspective 120 3 40 Nchange_persp 0]
	set T [Nv_mkScale $BASE.bframe.cframe.tw h twist -180 180 0 Nchange_twist 0]

	help $BASE.bframe.cframe.pers balloon "Set field of view size (degrees)"
	help $BASE.bframe.cframe.tw balloon "Set twist angle (degrees)"

	# reset button goes here so it can reference P
	Button $BASE.midf.lookat.reset -text "reset" \
		-bd 1 -command "do_reset $XY $H $E $P $T" \
		-helptext "Reset view to default"

	pack $BASE.midf.lookat.l $BASE.midf.lookat.here \
		$BASE.midf.lookat.center $BASE.midf.lookat.top \
		$BASE.midf.lookat.reset -side top -fill x -expand 1 -anchor n

	pack $BASE.midf.lookat $XY -side left -expand 1 -padx 5 -anchor w
	pack $H $E -side left -expand y -padx 2
	pack $BASE.midf -side top -fill both -expand 1

	pack $BASE.bframe.cframe.pers $BASE.bframe.cframe.tw -side left -fill x -expand 1 -padx 3
	pack $BASE.bframe -side top -fill x -expand 1 -padx 3
	pack $BASE.bframe.cframe -side top -pady 5

#*** ACS_MODIFY 1.0 BEGIN ******************************************************
	if {$Nv_(FlyThrough)} {
		set Nv_(TWIST_SLIDER) $T
		set Nv_(EXAG_SLIDER) $E
	}
#*** ACS_MODIFY 1.0 END ******************************************************


# According to the documentation, the Main panel can never be closed
#	button $BASE.close -text Close -command "Nv_closePanel $BASE" -anchor s
#	pack $BASE.close -side right

	return $panel
}

# Procedure to reset the main panel
proc Nviz_main_reset {} {
	global Nv_

	# Simple, just invoke the reset button
	$Nv_(main_BASE).midf.lookat.reset invoke
}

# Procedure to save camera parameters
proc Nviz_main_save { file_hook } {
	global Nv_

	set BASE $Nv_(main_BASE)

	#Get canvas size
	set width [lindex [$Nv_(TOP).canvas configure -width] 4]
	set height [lindex [$Nv_(TOP).canvas configure -height] 4]


	# Need to make this accurate
	# Also need to save "look here" information
	# TODO prob. need focus indication AND realto (if focused)
	puts $file_hook ">>>start main"
	puts $file_hook "$width $height"
	puts $file_hook "[$BASE.bframe.cframe.pers.f.entry get]"
	puts $file_hook "[$BASE.midf.zexag.f.entry get]"
	puts $file_hook "[$Nv_(HEIGHT_SLIDER).f.entry get]"
	puts $file_hook "[Nv_getXYPos  XY_POS]"
	puts $file_hook "[Nhas_focus]"
	puts $file_hook "[Nget_focus]"
	# if not focused, should use view_to
}

# Procedure to load camera parameters
proc Nviz_main_load { file_hook } {
	global Nv_

	# window size
	gets $file_hook data
	set win_width [lindex $data 0]
	set win_height [lindex $data 1]
	$Nv_(TOP).canvas configure -width $win_width -height $win_height
	pack $Nv_(TOP).canvas -side top -expand 1 -fill both

	# perspective
	gets $file_hook data
	Nv_setEntry $Nv_(main_BASE).bframe.cframe.pers.f.entry [expr int($data)]
	Nv_scaleCallback $Nv_(main_BASE).bframe.cframe.pers e 0 null [expr int($data)]
	update

	# zexag
	gets $file_hook data
	Nv_setEntry $Nv_(main_BASE).midf.zexag.f.entry $data
	Nv_floatscaleCallback $Nv_(main_BASE).midf.zexag e 2 null $data
	update

	# height
	gets $file_hook data
	Nv_setEntry $Nv_(HEIGHT_SLIDER).f.entry $data
	Nv_floatscaleCallback $Nv_(HEIGHT_SLIDER) e 2 null $data
	update

	# XY position
	gets $file_hook data
	set data [split "$data"]
	Nv_itemDrag $Nv_(main_BASE).midf.pos $Nv_(XY_POS) \
	[expr int([lindex $data 0] * 125)]	[expr int([lindex $data 1] * 125)]
	update_eye_position [lindex $data 0] [lindex $data 1]
	update


	# focus
	gets $file_hook data
	set data [split "$data"]
	if {"[lindex $data 0]" == "1"} then {
	    gets $file_hook data
	    set data [split "$data"]
	    Nset_focus [lindex $data 0] [lindex $data 1] [lindex $data 2]
	} else {
	    # insert code to set view_to here
	    Nset_focus_state 0
	}
	update

}

proc do_clear {} {

# TEST	  Nset_draw both
	Nset_draw front

	Nready_draw
	Nclear
	Ndone_draw
	Nset_draw back
}

# TODO - if started with view file, use these params for reset

proc do_reset {XY H E P T} {
	global Nv_
	global Nauto_draw

	appBusy

	Nset_focus_map
	Nv_itemDrag $XY $Nv_(XY_POS) 105 105
	Nv_xyCallback Nchange_position 125 125 105 105

	set exag [Nget_first_exag]
	set val $exag
	Nv_floatscaleCallback $E b 2 Nchange_exag $val

	set list [Nget_height]
	set val [lindex $list 0]
	Nv_floatscaleCallback $H b 2 update_height $val

	Nv_scaleCallback $P b 0 Nchange_persp 40
	Nv_scaleCallback $T b 0 Nchange_twist 0
	if {$Nauto_draw == 1} {Ndraw_all}

	appNotBusy
}

proc mk_exag_slider {W} {

	# init z-exag slider values
	set exag [Nget_first_exag]
	set val $exag
	set exag [expr $val * 10]
	set min 0

	Nv_mkFloatScale $W.zexag v z-exag $exag $min $val update_exag 1

	return $W.zexag
}

proc mk_hgt_slider {W} {
	global Nv_

	# init height slider values
	set list [Nget_height]
	set val [lindex $list 0]
	set min [lindex $list 1]
	set max [lindex $list 2]

	# make sliders
	set Nv_(HEIGHT_SLIDER) $W.height
	Nv_mkFloatScale $Nv_(HEIGHT_SLIDER) v height $max $min $val update_height 1

	return $Nv_(HEIGHT_SLIDER)
}

proc update_exag {exag} {
	global Nv_ 
	global draw_option

	if {$exag == 0.} {
		set exag [lindex [$Nv_(main_BASE).midf.zexag.scale configure -resolution] 4]
		Nv_setEntry $Nv_(main_BASE).midf.zexag.f.entry $exag
		Nv_floatscaleCallback $Nv_(main_BASE).midf.zexag e 2 null $exag
	}
	Nchange_exag $exag

#*** ACS_MODIFY 1.0 BEGIN ******************************************************
	if {$draw_option == 3} {
		set ht1 [lindex [Nget_real_position 1] 2]
		set ht2 [lindex [Nget_height] 0]
		
		## Update height to avoid scene jump
		## Changing the exag changes the height
		if {$ht1 == $ht2} {
			Nv_floatscaleCallback $Nv_(HEIGHT_SLIDER) b 2 update_height $ht2
		} else {
			Nv_floatscaleCallback $Nv_(HEIGHT_SLIDER) b 2 update_height \
				[lindex [Nget_real_position 1] 2]
		}

	} else {
		# original 2 lines
		Nv_floatscaleCallback $Nv_(HEIGHT_SLIDER) b 2 update_height \
		[$Nv_(HEIGHT_SLIDER).f.entry get]
	}

#*** ACS_MODIFY 1.0 END ********************************************************
#	 Nv_floatscaleCallback $Nv_(HEIGHT_SLIDER) b 2 update_height [lindex [Nget_height] 0]
#	 Nquick_draw
}

proc update_eye_position {x y} {
	global Nv_

	Nset_focus_state 1
	Nchange_position $x $y

	if {$Nv_(FollowView)} {
		set_lgt_position $x $y
		set x [expr int($x*125)]
		set y [expr int($y*125)]
		Nv_itemDrag $Nv_(LIGHT_XY) $Nv_(LIGHT_POS) $x $y
	}
}

proc update_center_position {x y} {
	global Nv_

	 Nset_focus_state 1
	 Nset_focus_gui $x $y

	if {$Nv_(FollowView)} {
		set_lgt_position $x $y
		set x [expr int($x*125)]
		set y [expr int($y*125)]
		Nv_itemDrag $Nv_(LIGHT_XY) $Nv_(LIGHT_POS) $x $y
	}
}

proc change_display {flag} {
	global XY Nv_
	global Nauto_draw

	set NAME $XY
	set NAME2 [winfo parent $NAME]
	catch "destroy $XY"
	
	if {$Nv_(FlyThrough)} {Nset_fly_mode -1}
	
	set h [lindex [Nget_real_position 1] 2]
	set min [lindex [Nget_height] 1]
	set max [lindex [Nget_height] 2]
	
	if {$flag == 1} {
		#draw eye position
		inform "Set eye position"
		set XY [Nv_mkXYScale $NAME puck XY_POS 125 125 105 105 update_eye_position]
		
	} elseif {$flag == 0} {
		#draw center position
		inform "Set center of view position"
		set XY [Nv_mkXYScale $NAME cross XY_POS 125 125 109 109 update_center_position]
	}
		
	if {$Nv_(FlyThrough)} {
		pack_XY
	} else {
		pack $XY -side left -before $Nv_(HEIGHT_SLIDER)
	}

	set Nv_(FlyThrough) 0
	update_height $h		
	reset_res
	move_position
	if {$Nauto_draw == 1} {Ndraw_all}
}

proc reset_res { } {
	global Nv_

	set h [lindex [Nget_real_position 1] 2]
	set from [lindex [Nget_height] 1]
	set to [lindex [Nget_height] 2]
	Nv_floatscaleCallback $Nv_(HEIGHT_SLIDER) b 2 update_height $h
	$Nv_(HEIGHT_SLIDER).scale configure -resolution [expr -1.0 * (($to - $from)/140.0)]

}

proc update_height {h} {
	global Nv_

	Nset_focus_state 1
	Nchange_height $h
		
	if {$Nv_(FollowView)} {
		set val [lindex [Nget_real_position 1] 2]
		set min [lindex [Nget_height] 1]
		set max [lindex [Nget_height] 2]
		set h [expr int((100.0*($h -$min))/($max - $min))]
		Nv_floatscaleCallback $Nv_(LIGHT_HGT) b 2 set_lgt_hgt $h
	}

}


proc move_position {} {
	global Nv_ draw_option

	#Make sure in correct view mode
	Nset_focus_state 1

	if {$draw_option == 0} {
		#Move position puck
		set E [lindex [Nget_position] 0]
		if {$E < 0.} {set $E 0.}
		if {$E > 1.} {set $E 1.}

		set N [lindex [Nget_position] 1]
		set N [expr 1. - $N]
		if {$N < 0.} {set $N 0.}
		if {$N > 1.} {set $N 1.}

		set E [expr $E * 125.]
		set N [expr $N * 125.]

		Nv_itemDrag $Nv_(main_BASE).midf.pos $Nv_(XY_POS) $E $N
		update
	}

	if {$draw_option == 1} {
		#Move center of view cross hair

		set E [lindex [Nget_focus_gui] 0]
		if {$E > 1.} { set E 1.}
		if {$E < 0.} {set E 0.}

		set N [lindex [Nget_focus_gui] 1]
		if {$N > 1.} {set N 1.}
		if {$N < 0.} {set N 0.}

		set E [expr ($E * 125.)]
		#reverse northing for canvas
		set N [expr 125 - ($N * 125.)]

		Nv_itemDrag $Nv_(main_BASE).midf.pos $Nv_(XY_POS) $E $N
		update
	}

}

###########################################
# cursor setting and resetting functions

proc setcursor { ctype } {
	global Nv_

	$Nv_(TOP).canvas configure -cursor $ctype
	return
}

proc restorecursor {} {
	global Nv_

	$can($mon) configure -cursor $Nv_(cursor)
	return
}

