##########################################################################
# Default Priority for this panel
#
# priority is from 0 to 10
#  the lower the number, the quicker it will be bumped
#  10 cannot be bumped
#  Panels will be loaded by the greater of 5 or their current priority

##########################################################################

global Light

proc mklightsPanel { BASE } {
    global Nv_

    set Nv_(FollowView) 0
    set Nv_(ShowModel) 1
    set Nv_(LIGHT_XY) $BASE.top.left.1.xy
    set Nv_(LIGHT_HGT) $BASE.top.left.1.height
    set Nv_(LIGHT_BASE) $BASE
    
    catch {destroy $BASE}
    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
        set panel [St_create {window name size priority} $BASE "Lighting" 2 5]
    } else {
	set panel $Nv_($BASE)
    }
    frame $BASE  -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Lighting Panel"
    
    frame $BASE.top
    frame $BASE.bottom
    frame $BASE.top.right
    frame $BASE.top.right.1
    frame $BASE.top.right.2
    frame $BASE.top.left 
    frame $BASE.top.left.1
    
    # Set up a binding so we do ShowModel correctly
    bind $BASE <Map> {if {$Nv_(FollowView) == 0} { do_light_draw } }
    
    Label $BASE.top.left.label -text " Light source position" -fg black
    pack $BASE.top.left.label -side top -fill none -expand 0 -anchor w

    # create lighting height slider
	Nv_mkScale $BASE.top.left.1.height v Height 100 0 80 set_lgt_hgt 2
   # pack $BASE.bottom.height -side right -expand 1 -fill y

	# create interactive panel for lighting point of view
    Nv_mkXYScale $BASE.top.left.1.xy puck LIGHT_POS 125 125 105 105 \
		set_lgt_position unset_follow
	pack $BASE.top.left.1.xy $BASE.top.left.1.height -side left \
		-expand 1 -padx 3
	pack $BASE.top.left.1 -side top -pady 2

	# create checkbuttons
    checkbutton $BASE.top.left.follow -text "Follow surface viewpoint" \
		-variable Nv_(FollowView) -command follow -onvalue 1 -offvalue 0
    checkbutton $BASE.top.left.show -text "Show lighting model" \
		-variable Nv_(ShowModel)
    pack $BASE.top.left.follow $BASE.top.left.show \
		-side top -fill none -expand 0 -anchor w -pady 2
    
    #create lighting color, ambient, and brightness sliders
	Label $BASE.top.right.1.lbl -text "Light color" -fg black
    Nv_mkScale $BASE.top.right.1.red h Red 0 100 100 set_red 2   
    Nv_mkScale $BASE.top.right.1.green h Green 0 100 100 set_green 2
    Nv_mkScale $BASE.top.right.1.blue h Blue 0 100 100 set_blue 2
    Label $BASE.top.right.2.lbl -text "Light intensity" -fg black
    Nv_mkScale $BASE.top.right.2.bright h Brightness 0 100 80 set_brt 2 
    Nv_mkScale $BASE.top.right.2.ambient h Ambient 0 100 20 set_amb 2
    pack $BASE.top.right.1.lbl $BASE.top.right.1.red \
    	$BASE.top.right.1.green $BASE.top.right.1.blue \
		-side top -expand 1 -pady 2
	pack $BASE.top.right.1 -side top -expand 1
    pack $BASE.top.right.2.lbl $BASE.top.right.2.bright \
    	$BASE.top.right.2.ambient -side top -expand 1 -pady 2
	pack $BASE.top.right.2 -side top -expand 1 -pady 4

    button $BASE.bottom.reset -text "Reset" -command "Nviz_lights_reset" -bd 1
    pack $BASE.bottom.reset -side left
    button $BASE.bottom.close -text "Close" -command "Nv_closePanel $BASE" -bd 1
    pack $BASE.bottom.close -side right


    pack $BASE.top.left -side left -expand 1 -pady 4 -anchor n
    pack $BASE.top.right -side right -expand 1 -pady 4 -anchor n
    pack $BASE.top -side top -fill both -expand 1
    pack $BASE.bottom -side bottom -fill x -expand 1

    return $panel
}

# Reset procedure for lights panel
proc Nviz_lights_reset {} {
    global Light Nv_
   
    set light_pos {{0.68 -0.68 0.8 0} {0.0 0.0 1.0 0}} 

    # Reset attributes of both lights
    for {set i 1} {$i < 3} {incr i} {
	if {$i == 1} then {
		set brt 0.8
		set amb 0.2
	} else {
		set brt 0.5
		set amb 0.3
	}

	$Light($i) set_ambient $amb $amb $amb
	$Light($i) set_bright $brt
	$Light($i) set_color 1.0 1.0 1.0

	set pos_data [lindex $light_pos [expr $i - 1]]
	$Light($i) set_position [lindex $pos_data 0] [lindex $pos_data 1] [lindex $pos_data 2] [lindex $pos_data 3]
    }
    
    set Nv_(FollowView) 0
    set Nv_(ShowModel)  1

    update_light_panel
}

# Save procedure for saving state of Nviz lights
proc Nviz_lights_save {file_hook} {
    global Light Nv_

    # Fairly straightforward, save the attributes of the two lights
    # plus the status of the panel
    puts $file_hook ">>>start lights"
    for {set i 1} {$i < 3} {incr i} {
        puts $file_hook "[$Light($i) get_ambient]"
        puts $file_hook "[$Light($i) get_bright]"
        puts $file_hook "[$Light($i) get_color]"
        puts $file_hook "[$Light($i) get_position]"
    }
    
    puts $file_hook "$Nv_(FollowView)"
    puts $file_hook "$Nv_(ShowModel)"
}

# Load procedure for loading state of Nviz lights
proc Nviz_lights_load {file_hook} {
    global Light Nv_

    # Fairly straightforward, load the attributes of the two lights
    # plus the status of the panel
    for {set i 1} {$i < 3} {incr i} {
	gets $file_hook amb_data
	set amb_data [split $amb_data]
	$Light($i) set_ambient [lindex $amb_data 0] [lindex $amb_data 1] [lindex $amb_data 2]

	gets $file_hook brt_data
	$Light($i) set_bright $brt_data

	gets $file_hook col_data
	set col_data [split $col_data]
	$Light($i) set_color [lindex $col_data 0] [lindex $col_data 1] [lindex $col_data 2]

	gets $file_hook pos_data
	set pos_data [split $pos_data]
	$Light($i) set_position [lindex $pos_data 0] [lindex $pos_data 1] [lindex $pos_data 2] [lindex $pos_data 3]
    }
    
    gets $file_hook Nv_(FollowView)    
    gets $file_hook Nv_(ShowModel)

    update_light_panel
}

# Quicky procedure to update the light panel based
# on the current settings of light attributes
proc update_light_panel {} {
    global Nv_ Light
    global Nauto_draw

    set BASE $Nv_(LIGHT_BASE)

    # brightness
    set val [$Light(1) get_bright]
    Nv_scaleCallback $BASE.top.right.2.bright b 2 null [expr int($val * 100)]

    # ambient
    set val [$Light(1) get_ambient]
    set val [lindex $val 0]
    Nv_scaleCallback $BASE.top.right.2.ambient b 2 null [expr int($val * 100)]

    # color
    set val [$Light(1) get_color]
    Nv_scaleCallback $BASE.top.right.1.red b 2 null [expr int([lindex $val 0] * 100)]
    Nv_scaleCallback $BASE.top.right.1.green b 2 null [expr int([lindex $val 1] * 100)]
    Nv_scaleCallback $BASE.top.right.1.blue b 2 null [expr int([lindex $val 2] * 100)]

    # height + XY position (puck)
    set val [$Light(1) get_position]
    set x [expr [lindex $val 0] / 2.0 + 0.5]
    set y [expr [lindex $val 1] / -2.0 + 0.5]
    set z [lindex $val 2]
    Nv_scaleCallback $BASE.top.left.1.height b 2 null [expr int($z * 100)]
    Nv_itemDrag $BASE.top.left.1.xy $Nv_(LIGHT_POS) [expr int(125 * $x)] [expr int(125 * $y)]

	if {$Nauto_draw == 1} {
		Ndraw_all
	} 
}

proc do_light_draw {} {
    global Nv_
    
    if {$Nv_(ShowModel)} {
            Nset_draw front
            Nready_draw
            Ndraw_model
            Ndone_draw
            Nset_draw back
    }
}

proc set_red {val} {
	set_lgt_color r $val
}
proc set_green {val} {set_lgt_color g $val}
proc set_blue {val} {set_lgt_color b $val}

proc set_lgt_color {c val} {
    global Light
    
    set c_list [$Light(1) get_color]
    if {[llength $c_list] == 3} {
        if {$c == "r"} {set r $val} else {set r [lindex $c_list 0]}
        if {$c == "g"} {set g $val} else {set g [lindex $c_list 1]}
        if {$c == "b"} {set b $val} else {set b [lindex $c_list 2]}
        $Light(1) set_color $r $g $b
        do_light_draw
    } else { 
    	return -code error "light colors not set $c_list"
    }
}

proc set_amb {a} {
    global Light

    $Light(1) set_ambient $a $a $a
#    $Light(2) set_ambient $a $a $a

    do_light_draw
}
proc set_brt {b} {
    global Light

    $Light(1) set_bright $b

    do_light_draw
}
proc set_lgt_position {x y} {
    global Light
    global Nv_

    set list [$Light(1) get_position]
    if {[llength $list] != 4} {return -code error "unable to get old position"}
    # origin at top right
    set x [expr -1.0 + 2.0*$x]
    set y [expr -1.0 + 2.0*(1.0 - $y)]
    set z [lindex $list 2]
    set w [lindex $list 3]
    $Light(1)  set_position $x $y  $z $w
    if {$Nv_(FollowView) == 0} { do_light_draw}
}
proc set_lgt_hgt  {z} {  
    global Light
    global Nv_


    set list [$Light(1)  get_position]
    if {[llength $list] != 4} {return -code error "unable to get old position"}
    set x [lindex $list 0]
    set y [lindex $list 1]
    set w [lindex $list 3]
    $Light(1)  set_position $x $y  $z $w
    if {$Nv_(FollowView) == 0} { do_light_draw}
}

proc follow {} {
    global Nv_

    if {$Nv_(FollowView)} { Nset_light_to_view}
}
proc unset_follow {args} {
    global Nv_
    $Nv_(LIGHT_BASE).top.left.follow deselect
}
