##################################################################################################
# UTILS
# Generic utility functions
#
#
# MODULE:       ACS_Utils.tcl 1.0
#
# AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
#				Updates by Michael Barton, Arizona State University Nov. 2006
#
# PURPOSE:		 Several general purpose utilities:
# 					- Configuration load (rc variables)
# 					- Undo/Redo lists
# 					- scrollframe utility
# 						creates and manages a canvas widget with
#						x and y scrollbars hiding the interiors
# 					- add / remove from lists
#					- dialog utility
# 						creates a dialog toplevel window with info
#						and controls frames and a separator
# 						usually this latter contains ok/cancel buttons
#					- confirm_ask
#						based on dialog utility
#					- modal_edit_list
#						based on dialog utility
#					- double listbox exchange
#						based on dialog utility
# 						widget that moves elements between two listboxes
# 						addremove_list_create  / addremove_list_move
# 							use a simple double list
# 						addremove_list_create2 / addremove_list_move2
#						use a double list for names and another for id's (not shown)
#
# REQUIREMENTS:	file .nvizrc to read rc variables
#
#
# COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
#
#               This program is free software under the
#               GNU General Public License (>=v2).
#               Read the file COPYING that comes with GRASS
#               for details.
#
##################################################################################################

global ___ACS_utils___
set ___ACS_utils___ 1
source $src_boot/etc/nviz2.2/scripts/config.tcl
###########################################################################################
# Configuration load
###########################################################################################
proc rc_load_file {} {
    global tcl_rcValues default_panel_path
    
    set nviz_params_FileName "[file join $default_panel_path nviz_params]"
    array set tcl_rcValues [list]
    if {![file exists $nviz_params_FileName] || ![file readable $nviz_params_FileName]} {
        puts "*** ERROR (TS_loadResource) *** rc file: $nviz_params_FileName dosn't exist of is not readable"
        puts "no rc file: $nviz_params_FileName" ""
		return
    }

    set fd [open $nviz_params_FileName]
    while {![eof $fd]} {
        gets $fd line

        # skip empty lines and comments
        if {[string match "#*" $line]} {continue}
        if {$line == ""} {continue}

        # gets value and fieldname
        set fieldname [string trim [lindex [split $line ":"] 0]]
        set value [string trim [lindex [split $line ":"] 1]]

        set tcl_rcValues($fieldname) $value
    }
}

###########################################################################################
###########################################################################################
###########################################################################################
# EXECUTE NOW!
rc_load_file
###########################################################################################
###########################################################################################
###########################################################################################

proc rc_load_res {_name _var {_default ""}} {
    global tcl_rcValues
	upvar $_var var

	if {[info exists tcl_rcValues($_name)]} {
		set var $tcl_rcValues($_name)
		return 1
	} else {
		set var $_default
		return 0
	}
}


# returns true if argument isn't a number
proc not_a_number { _v } {
	set v $_v
	return [catch { expr $v + 1 }]
}


# Undo/Redo lists
#
# returns id
# PAY ATTENTION: lst passed by name (just for consistency with following undo/redo functions
proc list_new {max_buffers lst} {
	global list_vars
	# unique id
	if { ![info exists list_vars(COUNT)] } {
		set list_vars(COUNT) 0
	} else {
		incr list_vars(COUNT)
	}
	# BUFffers for lists
	set list_vars($list_vars(COUNT).MAX_BUF) $max_buffers
	set list_vars($list_vars(COUNT).CUR_BUF) -1
	# LEVels for undo/redo
	set list_vars($list_vars(COUNT).MAX_LEV) 0
	set list_vars($list_vars(COUNT).CUR_LEV) 0

	upvar $lst l; list_save $list_vars(COUNT) l
	return $list_vars(COUNT)
}

proc list_error {id} {
	global list_vars
	if {[info exists list_vars($id.MAX_BUF)]} {return 0}
	puts "ERROR- LIST Utility - List $id doesn't exists"
	return 1
}

proc list_destroy {id} {
	global list_vars
	if {[list_error $id]} {return 0}
	# destory lists
	for {set i 0} {$i<$list_vars($id.MAX_BUF)} {incr i} {catch {unset list_vars($id.$i)}}
	# destroy variables
	unset list_vars($id.MAX_BUF)
	unset list_vars($id.CUR_BUF)
	unset list_vars($id.MAX_LEV)
	unset list_vars($id.CUR_LEV)
	return 1
}

# PAY ATTENTION: lst passed by name (just for consistency with following undo/redo functions
proc list_save {id lst} {
	if {[list_error $id]} {return 0}
	global list_vars
	upvar $lst l
	set list_vars($id.CUR_BUF) [expr ($list_vars($id.CUR_BUF)+1) % $list_vars($id.MAX_BUF)]
	if {$list_vars($id.CUR_LEV) < $list_vars($id.MAX_BUF)} {incr list_vars($id.CUR_LEV)}
	set list_vars($id.MAX_LEV) $list_vars($id.CUR_LEV)
	set list_vars($id.$list_vars($id.CUR_BUF)) $l
	return 1
}

# fills $lst with the buffered list, if any
# PAY ATTENTION: lst passed by name
proc list_undo {id lst} {
	if {[list_error $id]} {return 0}
	global list_vars
	upvar $lst l
	if {$list_vars($id.CUR_LEV) > 1} {
		incr list_vars($id.CUR_LEV) -1
		set list_vars($id.CUR_BUF) [expr ($list_vars($id.CUR_BUF) + $list_vars($id.MAX_LEV) -1) % $list_vars($id.MAX_LEV)]
		set l $list_vars($id.$list_vars($id.CUR_BUF))
		return 1
	} else {
		puts "WARNING- LIST Utility - List $id no more undo levels"
		return 0
	}
}

# fills $lst with the buffered list, if any
# PAY ATTENTION: lst passed by name
proc list_redo {id lst} {
	if {[list_error $id]} {return 0}
	global list_vars
	upvar $lst l
	if {$list_vars($id.CUR_LEV) < $list_vars($id.MAX_LEV)} {
		incr list_vars($id.CUR_LEV)
		set list_vars($id.CUR_BUF) [expr ($list_vars($id.CUR_BUF)+1) % $list_vars($id.MAX_LEV)]
		set l $list_vars($id.$list_vars($id.CUR_BUF))
		return 1
	} else {
		puts "WARNING- LIST Utility - List $id no more redo levels"
		return 0
	}
}


# scrollframe utility
# creates and manages a canvas widget with x and y scrollbars hiding the interiors

proc scrollframe_create {win} {
	frame $win -class Scrollframe

	scrollbar $win.ybar -command "$win.vport yview"
	pack $win.ybar -side right -fill y

	scrollbar $win.xbar -orient horizontal -command "$win.vport xview"
	pack $win.xbar -side bottom -fill x

	canvas $win.vport -yscrollcommand "$win.ybar set" -xscrollcommand "$win.xbar set"
	pack $win.vport -side left -fill both -expand true

	frame $win.vport.frame
	$win.vport create window 0 0 -anchor nw -window $win.vport.frame

	bind $win.vport.frame <Configure> "scrollframe_resize $win %w %h"
	return $win
}

proc scrollframe_resize {win _w _h} {
	set bbox [$win.vport bbox all]

	$win.vport configure -width $_w -height $_h -scrollregion $bbox \
		-yscrollincrement 0.1i -xscrollincrement 0.1i
}


proc scrollframe_yscroll { win _how_many _of_what } {
	$win.vport yview scroll $_how_many $_of_what
}

proc scrollframe_ymoveto { win _where } {
	$win.vport yview moveto $_where
}

proc scrollframe_interior {win} {
	return "$win.vport.frame"
}

proc scrollframe_clear {win} {
	catch {destroy $win.vport.frame}

	frame $win.vport.frame
	$win.vport create window 0 0 -anchor nw -window $win.vport.frame

	bind $win.vport.frame <Configure> "scrollframe_resize $win %w %h"
	return "$win.vport.frame"
}

# dialog utility
# creates a dialog toplevel window with info and controls frames and a separator
# usually this latter contains ok/cancel buttons
#
proc dialog_create {class {win "auto"}} {
	if {$win == "auto"} {
		set count 0
		set win ".dialog[incr count]"
		while {[winfo exists $win]} {
			set win ".dialog[incr count]"
		}
	}
	toplevel $win -class $class
	frame $win.info
	pack $win.info -expand yes -fill both -padx 2 -pady 2
	frame $win.sep -height 2 -borderwidth 1 -relief sunken
	pack $win.sep -fill x -pady 4
	frame $win.controls
	pack $win.controls -fill x -padx 4 -pady 4
	wm title $win $class
	wm group $win .
	after idle [format {
		update idletasks
		wm minsize %s [winfo reqwidth %s] [winfo reqheight %s]
	} $win $win $win]
	return $win
}

proc dialog_info 		{win} 			{return "$win.info"}
proc dialog_controls	{win} 			{return "$win.controls"}
proc dialog_setvar 		{varName val} 	{upvar $varName v; set v $val}


# "modal" behavior
#
proc dialog_wait {win varName} {
	dialog_safeguard $win
	set x [expr [winfo rootx .] + 50]
	set y [expr [winfo rooty .] + 50]
	wm geometry $win "+$x+$y"
	wm deiconify $win
	tkwait visibility $win
	catch {grab set $win}
	vwait $varName
	catch {grab release $win}
#	wm withdraw $win
}

# NOT PROCEDURE ##########################################################################
bind modalDialog <ButtonPress> {
	wm deiconify %W
	raise %W
}
##########################################################################################
proc dialog_safeguard {win} {
	if {[lsearch [bindtags $win] modalDialog] < 0} {
		bindtags $win [linsert [bindtags $win] 0 modalDialog]
	}
}

# Fill controls (OK/CANCEL) section
proc dialog_fill_controls {win varName focus {ok "OK"} {cancel "Cancel"}} {
	set cntls [dialog_controls $win]

	button $cntls.ok -text $ok -command "dialog_setvar $varName 1"
	pack $cntls.ok -side left -expand yes
	button $cntls.cancel -text $cancel -command "dialog_setvar $varName 0"
	pack $cntls.cancel -side left -expand yes

	wm protocol $win WM_DELETE_WINDOW "$cntls.cancel invoke"
	focus $cntls.$focus
}


# NOT PROCEDURE ##########################################################################
option add *Confirm*icon.bitmap questhead widgetDefault
option add *Confirm*icon.msg.wrapLength 4i widgetDefault
option add *Warning*icon.bitmap "error" widgetDefault
option add *Warning*icon.msg.wrapLength 4i widgetDefault
##########################################################################################
proc warning_ask {msg {ok "dismiss"}} {
	global confirmStatus

	set top [dialog_create Warning]

	set cntls [dialog_controls $top]

	button $cntls.ok -text $ok -command "dialog_setvar confirmStatus 1"
	pack $cntls.ok -side left -expand yes

	focus $cntls.ok

	set info [dialog_info $top]
	label $info.icon
	pack $info.icon -side left -padx 8 -pady 8
	label $info.msg -text $msg
	pack $info.msg -side right -expand yes -fill both -padx 8 -pady 8

	dialog_wait $top confirmStatus
	destroy $top
	return $confirmStatus
}

proc confirm_ask {msg {ok "OK"} {cancel "Cancel"}} {
	global confirmStatus

	set top [dialog_create Confirm]

	dialog_fill_controls $top confirmStatus cancel $ok $cancel

	set info [dialog_info $top]
	label $info.icon
	pack $info.icon -side left -padx 8 -pady 8
	label $info.msg -text $msg
	pack $info.msg -side right -expand yes -fill both -padx 8 -pady 8

	dialog_wait $top confirmStatus
	destroy $top
	return $confirmStatus
}

#
# lists "label" and "entries" are passed by name and the procedure access them via upvar
#
proc modal_edit_list {msg labels entries images commands {ok "OK"} {cancel "Cancel"}} {
	global confirmStatus

	# Pay attention!
	# list entries *IS* modified by this procedure
	upvar $labels lab $entries ent

	set top [dialog_create Edit]

	dialog_fill_controls $top confirmStatus cancel $ok $cancel

	# INFO SECTION
	set info [dialog_info $top]
	label $info.msg -text $msg
	grid $info.msg	-row 0 -column 0 -columnspan 2 -sticky nsew

	set i 0
	foreach l $lab e $ent {
		set i0 $i
		incr i

		label $info.l$i -text $l
		grid $info.l$i	-row $i -column 0 -sticky nsew

		entry $info.e$i -relief sunken -width [string length $e] -justify right
		$info.e$i  insert 0 $e
		grid $info.e$i	-row $i -column 1 -sticky nsew

		set img [lindex $images $i0]
		set cmd [lindex $commands $i0]

		if {$img != "NO_MAP"} {
			if {$cmd != "NO_COMMAND"} {
#				button $info.get$i -image $img -command "$cmd $info.e$i"
				button $info.get$i -image $img
				grid $info.get$i	-row $i -column 2 -sticky nsew
				bind $info.get$i <ButtonPress-1> "$cmd $info.e$i %X %Y"
			}
		}
	}

#	if {$type != "NO_MAP"} {
#		button $info.get -image [TS_get_map_image $type] -command "TS_nviz_choose_map $info.e$i $type"
#		grid $info.get	-row $i -column 2 -sticky nsew
#	}

	dialog_wait $top confirmStatus

	# update list
	set ent [list]; for {set j 1} {$j<=$i} {incr j} {lappend ent [$info.e$j get]}

	destroy $top
	return $confirmStatus
}

proc modal_edit_list_old {msg labels entries {type "NO_MAP"} {ok "OK"} {cancel "Cancel"}} {
	global confirmStatus

	# Pay attention!
	# list entries *IS* modified by this procedure
	upvar $labels lab $entries ent

	set top [dialog_create Edit]

	dialog_fill_controls $top confirmStatus cancel $ok $cancel

	# INFO SECTION
	set info [dialog_info $top]
	label $info.msg -text $msg
	grid $info.msg	-row 0 -column 0 -columnspan 2 -sticky nsew

	set i 0
	foreach l $lab e $ent {
		incr i

		label $info.l$i -text $l
		grid $info.l$i	-row $i -column 0 -sticky nsew

		entry $info.e$i -relief sunken -width [string length $e] -justify right
		$info.e$i  insert 0 $e
		grid $info.e$i	-row $i -column 1 -sticky nsew
	}

	if {$type != "NO_MAP"} {
		button $info.get -image [TS_get_map_image $type] -command "TS_nviz_choose_map $info.e$i $type"
		grid $info.get	-row $i -column 2 -sticky nsew
	}

	dialog_wait $top confirmStatus

	# update list
	set ent [list]; for {set j 1} {$j<=$i} {incr j} {lappend ent [$info.e$j get]}

	destroy $top
	return $confirmStatus
}

#
# lists "label" and "entries" are passed by name and the procedure access them via upvar
#
proc modal_edit_list_plain {msg labels entries {ok "OK"} {cancel "Cancel"}} {
	global confirmStatus

	# Pay attention!
	# list entries *IS* modified by this procedure
	upvar $labels lab $entries ent

	set top [dialog_create Edit]

	dialog_fill_controls $top confirmStatus cancel $ok $cancel

	# INFO SECTION
	set info [dialog_info $top]
	label $info.msg -text $msg
	grid $info.msg	-row 0 -column 0 -columnspan 2 -sticky nsew

	set i 0
	foreach l $lab e $ent {
		incr i

		label $info.l$i -text $l
		grid $info.l$i	-row $i -column 0 -sticky nsew

		entry $info.e$i -relief sunken -width [string length $e] -justify right
		$info.e$i  insert 0 $e
		grid $info.e$i	-row $i -column 1 -sticky nsew
	}

	dialog_wait $top confirmStatus

	# update list
	set ent [list]; for {set j 1} {$j<=$i} {incr j} {lappend ent [$info.e$j get]}

	destroy $top
	return $confirmStatus
}

##################################################################################

proc menu_button_create {win opt labels command} {

   	eval menubutton $win.menu -menu $win.menu.m -relief raised -indicatoron 0 -bd 1 $opt
    pack $win.menu
    menu $win.menu.m -tearoff 0
    foreach l $labels {$win.menu.m add command -label "$l" -command "$command $l"}
	return $win.menu
}

##################################################################################

# removes $elt from $lst
proc remove_from_list {elt lst} {
	# Pay attention!
	# list lst is passed by *NAME* and *IS* modified by this procedure
	upvar $lst llst

	if {![info exists llst]} {return 0}

	set index [lsearch -exact $llst $elt]
	if {$index >= 0} {
		set llst [lreplace $llst $index $index]
		return 1
	} else {
		return 0
	}
}

# adds $elt to $lst if not present
proc add_to_list {elt lst} {
	# Pay attention!
	# list lst is passed by *NAME* and *IS* modified by this procedure
	upvar $lst llst

	if {![info exists llst]} {set llst [list]}

	set index [lsearch -exact $llst $elt]
	if {$index >= 0} {
		return 0
	} else {
		set llst [lappend llst $elt]
		return 1
	}
}

##################################################################################
# double listbox exchange
# widget that moves elements between two listboxes

# addremove_list_create  / addremove_list_move
# use a simple double list

# addremove_list_create2 / addremove_list_move2
# use a double list for names and another for id's (not shown)
##################################################################################
proc addremove_list_create2 {title left_msg right_msg left_entries right_entries left_ids right_ids {ok "OK"} {cancel "Cancel"}} {
	global confirmStatus

	# Pay attention!
	# lists left/right_entries *ARE* modified by this procedure
	upvar $left_entries lent
	upvar $right_entries rent
	upvar $left_ids lid
	upvar $right_ids rid

	set top [dialog_create $title]

	dialog_fill_controls $top confirmStatus cancel $ok $cancel

	# INFO SECTION
	set info [dialog_info $top]

	set llabel [label $info.llabel -text "$left_msg"]
	set rlabel [label $info.rlabel -text "$right_msg"]

	grid $llabel -row 0 -column 0
	grid $rlabel -row 0 -column 2

	set llbox [listbox $info.llistbox -width 0]
	set rlbox [listbox $info.rlistbox -width 0]
	set llbox_id [listbox $info.llistbox_id -width 0]
	set rlbox_id [listbox $info.rlistbox_id -width 0]

	foreach e $lent {$llbox insert end $e}
	foreach e $rent {$rlbox insert end $e}
	# these two are not shown: they will keep the map id's
	foreach e $lid {$llbox_id insert end $e}
	foreach e $rid {$rlbox_id insert end $e}

	frame $info.buttons
		set add [button $info.buttons.add -text ">>" -command "addremove_list_move2 $llbox $rlbox $llbox_id $rlbox_id"]
		set rem [button $info.buttons.remove -text "<<" -command "addremove_list_move2 $rlbox $llbox $rlbox_id $llbox_id"]
		pack $add $rem -side top
	grid $info.buttons -row 1 -column 1

	grid $llbox -row 1 -column 0
	grid $rlbox -row 1 -column 2

# decomment next two for debugging
#	grid $llbox_id -row 2 -column 0
#	grid $rlbox_id -row 2 -column 2

	dialog_wait $top confirmStatus

	if {$confirmStatus} {
		# update list
		set lent [$llbox get 0 end]
		set rent [$rlbox get 0 end]
		set lid [$llbox_id get 0 end]
		set rid [$rlbox_id get 0 end]
	}

	destroy $top
	return $confirmStatus
}

proc addremove_list_move2 {_src _dst _src_id _dst_id} {
	set sele [$_src curselection]
	foreach ele $sele {
		$_dst insert end [$_src get $ele]
		$_dst_id insert end [$_src_id get $ele]

		$_src delete $ele
		$_src_id delete $ele
	}
}

proc addremove_list_create {title left_msg right_msg left_entries right_entries {ok "OK"} {cancel "Cancel"}} {
	global confirmStatus

	# Pay attention!
	# lists left/right_entries *ARE* modified by this procedure
	upvar $left_entries lent
	upvar $right_entries rent

	set top [dialog_create $title]

	dialog_fill_controls $top confirmStatus cancel $ok $cancel

	# INFO SECTION
	set info [dialog_info $top]

	set llabel [label $info.llabel -text "$left_msg"]
	set rlabel [label $info.rlabel -text "$right_msg"]

	grid $llabel -row 0 -column 0
	grid $rlabel -row 0 -column 2

	set llbox [listbox $info.llistbox -width 0]
	set rlbox [listbox $info.rlistbox -width 0]

	foreach e $lent {$llbox insert end $e}
	foreach e $rent {$rlbox insert end $e}

	frame $info.buttons
		set add [button $info.buttons.add -text ">>" -command "addremove_list_move $llbox $rlbox"]
		set rem [button $info.buttons.remove -text "<<" -command "addremove_list_move $rlbox $llbox"]
		pack $add $rem -side top
	grid $info.buttons -row 1 -column 1

	grid $llbox -row 1 -column 0
	grid $rlbox -row 1 -column 2

	dialog_wait $top confirmStatus

	if {$confirmStatus} {
		# update list
		set lent [$llbox get 0 end]
		set rent [$rlbox get 0 end]
	}

	destroy $top
	return $confirmStatus
}

proc addremove_list_move {_src _dst} {
	set sele [$_src curselection]
	foreach ele $sele {
		$_dst insert end [$_src get $ele]
		$_src delete $ele
	}
}

################################################################################
# lut_create_canvas
# creates a canvas that contains a lut image and a legend
#
# _canvas is the name of the widget
#
# the lut image is obtained by the list of colors given by _lut_list
# the legend is given by _val_list
# the _lut_list is appropriately supersampled or subampled to match _lut_width
#
# _canvas_width/height are the size of the whole returned canvas widget
# _lut_width/height are the size of the lut image inside the returned canvas widget
# for both width/height = X/Y if _orientation is h, Y/X otherwise)
# _canvas_height - _lut_height is the room left for legend
#
# _steps is the number-1 of legend numbers
# _orientation is... ok you can figure what it is :-)
################################################################################
proc lut_create_canvas {_canvas _val_list _lut_list \
						_canvas_width _canvas_height _lut_width _lut_height \
						{_steps 3} {_orientation h}} {

	# canvas size (rotated if vertical)
	set size(X) $_canvas_width; set size(Y) $_canvas_height
	# lut image size (rotated if vertical)
	set lut_size(X) $_lut_width; set lut_size(Y) $_lut_height
	# lut image origin in canvas (rotated if vertical)
	set lut_origin(X) [expr ($size(X) - $lut_size(X)) / 2]; set lut_origin(Y) [expr ($size(Y) - $lut_size(Y)) / 8]
	# lut image zoom (rotated if vertical)
	set lut_zoom(X) 1; set lut_zoom(Y) $lut_size(Y)
	# line length in lut legend (rotated if vertical)
	set lp(X) 0; set lp(Y) 15
	# text position in lut legend (rotated if vertical)
	set tp(X) 0; set tp(Y) 20
	# text anchor position if horizontal (X) and if vertical (Y)
	set tanchor(X) "n"; set tanchor(Y) "w"

	if {$_steps > 0} {set legend_delta [expr $lut_size(X) / $_steps]}

	# more pixel to draw than entries in the lut: must supersample
	# less pixel to draw than entries in the lut: must subsample
	set w 0; set n [llength $_lut_list]
	foreach color $_lut_list value $_val_list {
		incr w $lut_size(X)
		while {$w >= $n} {
			set w [expr $w - $n]
			lappend llut $color
			lappend lleg $value
		}
	}

	# create lut image
	set pic [image create photo]
	if {$_orientation == "h"} {
		set x "X"; set y "Y"
		# this is a list of one element that contains the "row" $_lut_list as a list
		$pic put [list $llut]
	} else {
		set x "Y"; set y "X"
		# this is a list of one "column" $_lut_list
		$pic put $llut
	}
	set pic_lut [image create photo -width $lut_size($x) -height $lut_size($y)]
	$pic_lut copy $pic -zoom $lut_zoom($x) $lut_zoom($y)
	image delete $pic

	canvas $_canvas -width $size($x) -height $size($y)

	# draw lut
	$_canvas create image $lut_origin($x) $lut_origin($y) -image $pic_lut -anchor nw

	# draw legend
	if {$_steps > 0} {
		set last_p [expr $lut_origin(X) + $lut_size(X)]
		set p(Y) [expr $lut_origin(Y) + $lut_size(Y) + 3]

		for {set p(X) $lut_origin(X)} {$p(X) <= $last_p} {incr p(X) $legend_delta} {
			$_canvas create line $p($x) $p($y) [expr $p($x) + $lp($x)] [expr $p($y) + $lp($y)]
			$_canvas create text [expr $p($x) + $tp($x)] [expr $p($y) + $tp($y)]  -anchor $tanchor($x)\
						-text "[lindex $lleg [expr $p(X) - $lut_origin(X)]]"
		}
	}

	return $_canvas
}
