#***************************************************************
#*
#* MODULE:       flythrough.tcl 0.99
#*
#* AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
#*
#* PURPOSE:		 "Immersive" navigation by means of mouse buttons and movement
#* 				 In conjunction with togl_flythrough.c
#*
#* COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
#*
#*               This program is free software under the
#*               GNU General Public License (>=v2).
#*               Read the file COPYING that comes with GRASS
#*               for details.
#*
#**************************************************************

proc fly_set_icons {_dir} {
	global fly
	set fly(ICONS_DIR) $_dir

	# button left icon
	set fly(B_L) "$fly(ICONS_DIR)/b_l.gif"
	# button center icon
	set fly(B_C) "$fly(ICONS_DIR)/b_c.gif"
	# button right icon
	set fly(B_R) "$fly(ICONS_DIR)/b_r.gif"
	# button center+left icon
	set fly(B_CL) "$fly(ICONS_DIR)/b_lc.gif"
	# button center+right icon
	set fly(B_CR) "$fly(ICONS_DIR)/b_cr.gif"
	# button left+right icon
	set fly(B_LR) "$fly(ICONS_DIR)/b_lr.gif"

	# mouse up/down icon
	set fly(M_UD) "$fly(ICONS_DIR)/a_ud.gif"
	# mouse left/right icon
	set fly(M_LR) "$fly(ICONS_DIR)/a_lr.gif"

	# empty icon
	set fly(EMPTY) "$fly(ICONS_DIR)/vuota.gif"
}

################################################################################
# Called now! Not a procedure!!!
# bit_map_path is set by config.tcl
#
fly_set_icons $bit_map_path/flythrough
################################################################################

proc mkFlyButtons {BASE frame draw_lab draw_var1 draw_var2} {
	global Nv_ fly draw_option

	# Flythrough menus
	set fly(EYE_RADIOBUTTON) $draw_var1
	set fly(CENTER_RADIOBUTTON) $draw_var2

	set fly(FLY_RADIOBUTTON) [radiobutton $BASE.$frame.b3 \
                 -variable draw_option -value 3 -text "fly"\
				 -command "set Nv_(FlyThrough) 1; fly_change_mode 0" ]


    set fly(FLY_MENUBUTTON) $BASE.$frame.flymenu
	set m $fly(FLY_MENUBUTTON).m

	menubutton $fly(FLY_MENUBUTTON) -menu $m -text "none" -relief flat -indicatoron 1 -bd -5

	menu $m -tearoff 0
	$m add radiobutton -label "basic" -command "fly_change_mode 0" -variable fly(FLY_MODE) -value "basic"
    $m add radiobutton -label "simple" -command "fly_change_mode 2" -variable fly(FLY_MODE) -value "simple"
    $m add radiobutton -label "orbit" -command "fly_change_mode 1" -variable fly(FLY_MODE) -value "orbit"

	fly_deselect

    pack $draw_lab $draw_var1 $draw_var2 $fly(FLY_RADIOBUTTON) $fly(FLY_MENUBUTTON) -side left -expand 0


	# Flythrough panel
	set fly(BUTTONS) [frame $BASE.f -borderwidth 0 -relief flat]

		frame $fly(BUTTONS).buttons -border 0 -relief flat
			set fly(COARSE_DRAW_B) [checkbutton $fly(BUTTONS).buttons.coarse -text "Coarse Draw" \
						-onvalue 1 -offvalue 0 -variable coarse_draw]
			$fly(COARSE_DRAW_B) select
			set fly(FLY_HELP_PANEL) [button $fly(BUTTONS).buttons.fly_help_panel -text "fly help" -command "fly_create_help_panel"]
		pack $fly(COARSE_DRAW_B) $fly(FLY_HELP_PANEL) -side top -pady 20 -padx 2

		set scales [Nget_fly_scale]

		#Make sure factors are not zero 
		if {[lindex $scales 0] == 0.0} {
			set scales [lreplace $scales 0 0 1.0]
		}
		if {[lindex $scales 1] == 0.0} {
                        set scales [lreplace $scales 1 1 1.0]
                }

		frame $fly(BUTTONS).scales -border 0 -relief flat
			Nv_mkFloatScale $fly(BUTTONS).scales.s0 v "move
exag" 100 1 [lindex $scales 0] fly_update_scale0 2
			Nv_mkFloatScale $fly(BUTTONS).scales.s1 v "turn
exag" 10 1 [lindex $scales 1] fly_update_scale1 2
		pack $fly(BUTTONS).scales.s0 $fly(BUTTONS).scales.s1 -side left

	pack $fly(BUTTONS).buttons $fly(BUTTONS).scales -side left -pady 2 -padx 2
}


proc fly_change_mode {flag} {
    global XY Nv_ fly

	set Nv_(FlyThrough) 1
	inform "Interactively set view position"
	pack forget $XY $Nv_(HEIGHT_SLIDER) $Nv_(TWIST_SLIDER)
	Nset_fly_mode $flag
	pack $fly(BUTTONS) -side left -before $Nv_(EXAG_SLIDER) -expand y
	Nset_focus_state 0
	update
}

proc fly_destroy_help_panel {} {
	set W ".fly_help"
	if {[winfo exists $W]} {destroy $W}
}

proc fly_create_help_panel {} {
	global fly

	set W ".fly_help"
	if {[winfo exists $W]} {raise $W; return 0}
	toplevel $W

    wm resizable $W false false
    wm title $W "flythrough help"

	frame $W.lab
		set row 0

		set lab $W.lab.fly
		label $lab -text "fly" -relief flat -borderwidth 0 -background grey90
		grid $lab -row $row -column 1 -columnspan 1 -sticky nse

		set lab $W.lab.sep3
		label $lab -text "" -background grey90 -borderwidth 0 -relief flat
		grid $lab -row $row -column 2 -sticky nsew

		set lab $W.lab.basic
		label $lab -text "basic" -relief flat -borderwidth 1 -background grey80
		grid $lab -row $row -column 3 -columnspan 2 -sticky nsew

		set lab $W.lab.sep1
		label $lab -text "" -background grey90 -borderwidth 0 -relief flat
		grid $lab -row $row -column 5 -sticky nsew

		set lab $W.lab.simple
		label $lab -text "simple" -relief flat -borderwidth 1 -background grey80
		grid $lab -row $row -column 6 -columnspan 2 -sticky nsew

		set lab $W.lab.sep2
		label $lab -text "" -background grey90 -borderwidth 0 -relief flat
		grid $lab -row $row -column 8 -sticky nsew

		set lab $W.lab.orbit
		label $lab -text "orbit" -relief flat -borderwidth 1 -background grey80
		grid $lab -row $row -column 9 -columnspan 2 -sticky nsew

		incr row

		fly_label_sep $W.lab $row
		incr row

		set lab $W.lab.move
		label $lab -text "move" -relief flat -borderwidth 1 -background grey80
		grid $lab -row $row -column 0 -sticky nsew

		fly_label_row $W.lab $row "fwd/bkw" [list $fly(B_CL) $fly(B_L) $fly(B_CL)] [list $fly(B_CR) $fly(M_UD) $fly(B_CR)]
		incr row

		fly_label_sep $W.lab $row
		incr row

		fly_label_row $W.lab $row "left/right" [list $fly(B_LR) $fly(B_R) $fly(EMPTY)] [list $fly(M_LR) $fly(M_LR) $fly(EMPTY)]
		incr row

		fly_label_sep $W.lab $row
		incr row

		fly_label_row $W.lab $row "up/down" [list $fly(B_LR) $fly(B_R) $fly(EMPTY)] [list $fly(M_UD) $fly(M_UD) $fly(EMPTY)]
		incr row

		fly_label_sep $W.lab $row
		incr row

		set lab $W.lab.turn
		label $lab -text "turn" -relief flat -borderwidth 1 -background grey80
		grid $lab -row $row -column 0 -sticky nsew

		fly_label_row $W.lab $row "heading" [list $fly(B_C) $fly(B_L) $fly(B_C)] [list $fly(M_LR) $fly(M_LR) $fly(M_LR)]
		incr row

		fly_label_sep $W.lab $row
		incr row

		fly_label_row $W.lab $row "pitch" [list $fly(B_C) $fly(B_LR) $fly(B_C)] [list $fly(M_UD) $fly(M_UD) $fly(M_UD)]
		incr row

		fly_label_sep $W.lab $row
		incr row

		fly_label_row $W.lab $row "roll(twist)" [list $fly(EMPTY) $fly(B_LR) $fly(EMPTY)] [list $fly(EMPTY) $fly(M_LR) $fly(EMPTY)]

	pack $W.lab -side left -pady 5 -padx 5
}


proc fly_label_sep {_name _row} {
		set lab $_name.vsep$_row
		frame $lab -background grey90 -borderwidth 0 -relief flat
		grid $lab -row $_row -column 0 -columnspan 10 -sticky nsew -pady 1
}

proc fly_label_row {_name _row _title _lst1 _lst2} {
	set i 1

	label $_name.t$_row -text $_title -relief flat -borderwidth 1 -background grey80
	grid $_name.t$_row -row $_row -column $i -rowspan 1 -sticky nsew
	incr i

	foreach img1 $_lst1 img2 $_lst2 {
		set lab $_name.r$_row\c$i
		label $lab -text "" -background grey90 -borderwidth 0 -relief flat
		grid $lab -row $_row -column $i -sticky nsew
		incr i

		if {$img1 != ""} {
			set lab $_name.r$_row\c$i
			label $lab -image [image create photo -file $img1] -background white
			grid $lab -row $_row -column $i -sticky nsew
		}
		incr i

		if {$img2 != ""} {
			set lab $_name.r$_row\c$i
			label $lab -image [image create photo -file $img2] -background white
			grid $lab -row $_row -column $i -sticky nsew
		}
		incr i
	}
}


proc fly_update_scale0 {_val} {fly_update_scale 0 $_val}
proc fly_update_scale1 {_val} {fly_update_scale 1 $_val}

proc fly_update_scale {_sc _val} {
	set scales [Nget_fly_scale]
	set old_val [lindex $scales $_sc]

	if {$old_val != $_val} {
		set scales [lreplace $scales $_sc $_sc $_val]
		Nset_fly_scale [lindex $scales 0] [lindex $scales 1]
	}
}

# These two function are called by Nset_fly_mode
proc fly_deselect {} {
	global fly;
	$fly(FLY_RADIOBUTTON) deselect
	$fly(FLY_MENUBUTTON) configure -text "none"
	set fly(FLY_MODE) "basic"
}
proc fly_select {} {
	global fly;
	$fly(EYE_RADIOBUTTON) deselect
	$fly(CENTER_RADIOBUTTON) deselect
	$fly(FLY_RADIOBUTTON) select

	$fly(FLY_MENUBUTTON) configure -text $fly(FLY_MODE)
}

proc pack_XY {} {
    global XY Nv_ fly

	pack forget $fly(BUTTONS)
	fly_destroy_help_panel
        pack $XY $Nv_(HEIGHT_SLIDER) -side left -before $Nv_(EXAG_SLIDER)
	pack $Nv_(TWIST_SLIDER) -side bottom
	#Nset_focus_map
}
