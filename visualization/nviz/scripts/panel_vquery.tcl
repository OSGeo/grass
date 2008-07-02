#***************************************************************
#*
#* MODULE:       panel_vquery.tcl 1.0 (developed from old
#*				 panel_pick.tcl and panel_highlight.tcl)
#*
#* AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
#*				 Major update Nov 2006 by Michael Barton, 
#*				 	Arizona State University
#*
#* PURPOSE:		 Query vector objects, visually highlight
#*					queried object and display associated
#*					attribute data. Vector map must be enabled with
#*					vquery_add_map (and removed with (vquery_remove_map)
#*
#* REQUIREMENTS: ACS_utils.tcl
#*
#* COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
#*				 (C) 2006 Michael Barton, Arizona State University
#*				 	and GRASS Development Team
#*				
#*               This program is free software under the
#*               GNU General Public License (>=v2).
#*               Read the file COPYING that comes with GRASS
#*               for details.
#*
#**************************************************************
namespace eval NV_panel_vquery {
    variable maxdist # snapping distance for selecting objects to query
    variable array highlight
}

set maxdist 1000

global ___ACS_utils___
if {![info exists ___ACS_utils___]} {
	source $default_panel_path/ACS_utils.tcl
}

proc mkpickPanel { BASE } {
	global Nv_
	global highlight
	global nviztxtfont
	variable maxdist
	

    catch {destroy $BASE}

    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
	set panel [St_create {window name size priority} \
		       $BASE "Vector Query" 1 5]
    } else {
	set panel $Nv_($BASE)
    }

    frame $BASE  -relief flat -bd 0
    Nv_mkPanelname $BASE "Vector Query Panel"

	# set highlight defaults
	set highlight(COLOR) 1
	set highlight(SIZE) 0
	set highlight(MARKER) 0
	set highlight(COLOR_VALUE) #ffff00
	set highlight(SIZE_VALUE) 1.5
	set highlight(MARKER_VALUE) 4

	Nsite_highlight_set_default size $highlight(SIZE)
	Nsite_highlight_set_default color $highlight(COLOR)
	Nsite_highlight_set_default marker $highlight(MARKER)
	Nsite_highlight_set_default_value color $highlight(COLOR_VALUE)
	Nsite_highlight_set_default_value size $highlight(SIZE_VALUE)
	Nsite_highlight_set_default_value marker $highlight(MARKER_VALUE)

	# make main frames for panel
    set query [frame $BASE.query]
    set ptsel [frame $BASE.highlight]
    set bottom [frame $BASE.bottom]
    
    # make frame and widgets to control query
    frame $query.row1
	set Nv_(PICK) 0
	checkbutton $query.row1.on -variable Nv_(PICK) \
		-text "query on/off" \
		-command "pick_set Nv_(PICK) $Nv_(TOP).canvas"

	Button $query.row1.addremovemap -text "Choose map(s)" \
		-command "pick_GUI_addremovemap" -bd 1 \
		-helptext "Select/unselect vector map(s) to query"
	pack $query.row1.on $query.row1.addremovemap -side left -padx 3
	pack $query.row1 -side top -expand 1 -fill both -pady 3 -padx 3

	frame $query.row2	
	set Nv_(PICK_MAXDIST) [SpinBox $query.row2.maxdist\
		-range {1 100000 1} \
		-editable 1 \
		-textvariable maxdist \
		-width 5 -entrybg white \
		-helptext "Set threshold distance for selecting objects"]

	label $query.row2.l -text "threshold dist" -fg black -font $nviztxtfont

	rc_load_res "hyperlink.display.maxNumber" maxnumber 20	
	set Nv_(PICK_SHOW_HL) 0
	checkbutton $query.row2.hl -text "show hyperlink" \
		-variable Nv_(PICK_SHOW_HL) \
		-command "pick_highlight_hyperlinks Nv_(PICK_SHOW_HL) $Nv_(TOP).canvas [expr $maxnumber - 1]"

	pack $query.row2.maxdist $query.row2.l $query.row2.hl -side left -padx 3
	pack $query.row2 -side top -expand 1 -fill both -pady 3 -padx 3

	# make frame and widgets for highlighting vector points
	frame $ptsel.row1
	label $ptsel.row1.l -text "Highlight queried vector points with..."
	pack $ptsel.row1.l -side left -expand 0
	pack $ptsel.row1 -side top -fill both -pady 3 -padx 3
	
	frame $ptsel.row2
	checkbutton $ptsel.row2.color -text "color  " -variable highlight(COLOR) \
		-command {Nsite_highlight_set_default color $highlight(COLOR)}
	Button $ptsel.row2.color_value -command "highlight_set_color $ptsel.row2.color_value" \
		-bg $highlight(COLOR_VALUE) -width 0 -height 0 \
		-bd 1 -helptext "Choose color to indicate point queried"
				
	checkbutton $ptsel.row2.size -text "icon size  X" -variable highlight(SIZE) \
		-command {Nsite_highlight_set_default size $highlight(SIZE)}
	set ptsize [SpinBox $ptsel.row2.size_value \
		-range {.5 5 .1} \
		-textvariable highlight(SIZE_VALUE) \
		-modifycmd {Nsite_highlight_set_default_value size $highlight(SIZE_VALUE)} \
		-width 4 -helptext "Choose size in multiples of default to indicate point queried"]

	pack $ptsel.row2.color $ptsel.row2.color_value -side left -expand 0
	pack $ptsel.row2.size_value $ptsel.row2.size -side right -expand 0
	pack $ptsel.row2 -side top -expand 1 -fill both -pady 3 -padx 3

	frame $ptsel.row3
	checkbutton $ptsel.row3.marker -text "specific icon  " -variable highlight(MARKER) \
		-command {Nsite_highlight_set_default marker $highlight(MARKER)}
	highlight_set_marker_button $ptsel.row3.marker_value
	pack $ptsel.row3.marker $ptsel.row3.marker_value -side left -expand 0
	pack $ptsel.row3 -side top -expand 1 -fill both -pady 3 -padx 3
	
    # make frame and widgets for panel bottom
	button $bottom.close -text Close \
		-command "Nv_closePanel $BASE" -bd 1
    pack $bottom.close -side right

	# pack them all
    pack $query $ptsel $bottom \
    	-expand 1 -fill both -side top

	pick_init

	return $panel
}

###########################################################################################
# Procedures for vector querying

proc pick_panel_get_Nv_height {} {global Nv_; return $Nv_(height)}

proc pick_panel_get_pick_maxdist {} {
	global Nv_
	set maxdist [$Nv_(PICK_MAXDIST) get]
	if {[not_a_number $maxdist]} {
		set maxdist 10000
		$Nv_(PICK_MAXDIST) delete 0 end
		$Nv_(PICK_MAXDIST) insert 0 10000
	}
	return $maxdist
}


proc pick_GUI_addremovemap {} {
	global pick

	set l [list]
# isolate Nviz Commands?
	set ll [Nget_site_list]
	foreach ele $ll {lappend l Nsite$ele}

	set ll [Nget_vect_list]
	foreach ele $ll {lappend l Nvect$ele}

	foreach ele $l {puts "[$ele get_att map]"}

	set r $pick(MAP_LIST)

	# list l contains all the active maps at the moment
	# list r contains all the pickable maps
	# we should remove from l all the maps in r
	# if a map is in r, but not in l, it must be removed from r
	# at the end: l U r = original l
	foreach ele $r {
		if {![remove_from_list $ele l]} {
			# $ele is in r but not in l: remove it from pick(MAP_LIST)!
			vquery_remove_map $ele
		}
	}
	# update r
	set r $pick(MAP_LIST)

	set lname [list]
	foreach ele $l {lappend lname [$ele get_att map]}
	set rname [list]
	foreach ele $r {lappend rname [$ele get_att map]}

	if {[addremove_list_create2 "Select vectors to query" \
								"vectors displayed" "vectors to query" lname rname l r]} {
		# now r is modified and we have to set the differences from pick(MAP_LIST)

		# add new pickable elements
		foreach ele $r {
			set index [lsearch -exact $pick(MAP_LIST) $ele]
			# if not present $ele is new
			if {$index < 0} {vquery_add_map $ele}
		}

		# remove new non-pickable elements
		foreach ele $pick(MAP_LIST) {
			set index [lsearch -exact $r $ele]
			# if not present $ele has been deleted from list
			if {$index < 0} {vquery_remove_map $ele}
		}
	}
}



proc pick_init {} {
	global pick

	rc_load_res "hyperlink.field" pick(MM_NAME) "MULTIMEDIA"
	set pick(MAP_LIST) [list]
}

proc pick_set {_pick _win} {
	global pick
	upvar $_pick pick

	if {$pick} {
		bind $_win <Button-1> {pick %x %y }
	} else {
		bind $_win <Button-1> {}
    }
}

proc pick_map_is_pickable {_map} {
	global pick
	set index [lsearch -exact $_map $pick(MAP_LIST)]
	if {$index < 0} {
		return 0
	} else {
		return 1
	}
}

proc vquery_add_map {_map} {
	global pick
	add_to_list $_map pick(MAP_LIST)
}

proc vquery_remove_map {_map} {
	global pick
	remove_from_list $_map pick(MAP_LIST)

	catch {unset pick(HL_CAT_LIST.$_map)}
	catch {pick_erase_records $pick(SF.$_map)}
	catch {pick_delete_records $pick(SF.$_map)}
	catch {destroy $pick(WIN.$_map)}
}


proc pick {_x _y} {
	global pick
	variable maxdist
	
	set _y [expr [pick_panel_get_Nv_height] - $_y]

	#set maxdist [pick_panel_get_pick_maxdist]

	set redraw 0
	foreach map $pick(MAP_LIST) {
		set cat [lindex [Npick_vect $_x $_y [$map get_att map] $maxdist] 0]

		if {$cat != ""} {
			pick_draw_win $map $cat
			set redraw 1
		}
	}

	if {$redraw > 0} {Ndraw_all}
}


proc pick_highlight_hyperlinks {_on _win _max_found} {
	global pick

	upvar $_on on
	appBusy

	if {$on} {
# shapes:
# ST_X 1/ST_BOX 2/ST_SPHERE 3/ST_CUBE 4/ST_DIAMOND 5/ST_DEC_TREE 6/ST_CON_TREE 7/ST_ASTER 8/ST_GYRO 9
		rc_load_res "hyperlink.display.scale" scl 5
		rc_load_res "hyperlink.display.shape" shp 3
		rc_load_res "hyperlink.display.color" clr "#0000ff"

		foreach map $pick(MAP_LIST) {
			set name [$map get_att map]

			set i 0
			foreach {n t} [Nsite_attr_get_fields_name_and_type $name] {
				#SOSTITUIRE $pick(MM_NAME) / "STAZIONE_P"
				if {$n == $pick(MM_NAME)} {
					set pick(HL_CAT_LIST.$map) [lrange [Nsite_attr_get_field_not_emtpy_cats $name $i] 0 $_max_found]
					# catch used in case highlight hasn't been defined
					catch {Nsite_highlight_list color [string range $map 5 end] $pick(HL_CAT_LIST.$map) $clr}
					catch {Nsite_highlight_list size [string range $map 5 end] $pick(HL_CAT_LIST.$map) $scl}
					catch {Nsite_highlight_list marker [string range $map 5 end] $pick(HL_CAT_LIST.$map) $shp}
					Ndraw_all
				}
				incr i
			}
		}
	} else {
		# off
		foreach map $pick(MAP_LIST) {
			if {[info exists pick(HL_CAT_LIST.$map)]} {
				# catch used in case highlight hasn't been defined
				catch {Nsite_unhighlight_list all [string range $map 5 end] $pick(HL_CAT_LIST.$map)}
				unset pick(HL_CAT_LIST.$map)
			}
		}
		Ndraw_all
	}

	appNotBusy
}


proc pick_draw_maxdist {_surf_id _x _y _maxdist} {
	set x1 [expr $_x - $_maxdist]
	set x2 [expr $_x + $_maxdist]
	set y1 [expr $_y - $_maxdist]
	set y2 [expr $_y + $_maxdist]
	Ndraw_line_on_surf $_surf_id $x1 $y1 $x2 $y1
	Ndraw_line_on_surf $_surf_id $x2 $y1 $x2 $y2
	Ndraw_line_on_surf $_surf_id $x2 $y2 $x1 $y2
	Ndraw_line_on_surf $_surf_id $x1 $y2 $x1 $y1
}


proc pick_select_record { _sf _irec} {
	global pick

	set w [scrollframe_interior $_sf]

	set first_row [expr $pick($_sf.ROW) - $pick($_sf.NREC)]
	set irow [expr $_irec + $first_row]
	set nfields [llength $pick($_sf.RECORD.$_irec)]

	for {set i $first_row} {$i < $pick($_sf.ROW)} {incr i} {
		for {set j 0} {$j < $nfields} {incr j} {
			if {$j != $pick($_sf.MM_INDEX)} {
				catch {$w.r$i\c$j configure -background "white"}
			}
		}
	}

	for {set j 0} {$j < $nfields} {incr j} {
		if {$j != $pick($_sf.MM_INDEX)} {
			catch {$w.r$irow\c$j configure -background "yellow"}
		}
	}

	# position scrollframe on selected row
	update
	scrollframe_ymoveto $_sf [expr ($irow.0 - 1.0) / ($pick($_sf.ROW).0)]

	# catch used in case highlight hasn't been defined
	catch {Nsite_unhighlight all [string range $pick($_sf.MAP) 5 end] $pick($_sf.HIGHLIGTHED)}
	set pick($_sf.HIGHLIGTHED) $pick($_sf.UNIQUE.$_irec)
	catch {Nsite_highlight default [string range $pick($_sf.MAP) 5 end] $pick($_sf.UNIQUE.$_irec)}
	Ndraw_all
}

proc pick_draw_win {_map _cat} {
	global pick

	set win ".pick2$_map"
	set sf $win.f

	set hopt "-relief raised -background gray"

	if {[winfo exists $win]} {
		raise $win
		set w [scrollframe_interior $sf]
		pick_add_record $sf $_map $_cat
		return $sf
	}

	set pick($sf.HEADER) [Nsite_attr_get_fields_name [$_map get_att map]]
	if {$pick($sf.HEADER) == ""} {return $win.f}

	set pick($sf.MM_INDEX) [pick_get_multimedia_field $pick($sf.HEADER) $pick(MM_NAME)]

	toplevel $win
	wm resizable $win true true
	wm title $win "Map: [$_map get_att map]"

	wm geometry $win "820x200+10+250"
	wm minsize $win 150 100
	bind $win <Destroy> "pick_unselect_all %W $win $_map"

	scrollframe_create $sf
	pack $sf -expand yes -fill both -side bottom
	set w [scrollframe_interior $sf]

	set pick(SF.$_map) $sf
	set pick(WIN.$_map) $win
	set pick($sf.MAP) $_map

	set pick($sf.ROW) 0

	button $w.b -text "clear" -command "pick_clear_window $win $sf $_map" -bd 1
	grid $w.b	-row $pick($sf.ROW) -column 0 -columnspan 2 -sticky nsw
	incr pick($sf.ROW)

	pick_draw_row $sf $w $pick($sf.ROW) $pick($sf.HEADER) $hopt -1 ""
	incr pick($sf.ROW)

	if {[info exists pick($sf.NREC)]} {
		for {set i 0} {$i < $pick($sf.NREC)} {incr i} {
					pick_draw_row $sf $w $pick($sf.ROW) $pick($sf.RECORD.$i) \
					"-relief groove -background white" $pick($sf.MM_INDEX) "pick_multimedia"

			incr pick($sf.ROW)
		}
	} else {
		set pick($sf.NREC) 0
	}

	pick_add_record $sf $_map $_cat

	return $sf
}

proc pick_add_record {sf _map _cat} {
	global pick

	set w [scrollframe_interior $sf]

	set index $pick($sf.NREC)
	set pick($sf.RECORD.$index) [Nsite_attr_get_record_values [$_map get_att map] $_cat]
	set pick($sf.UNIQUE.$index) $_cat

	if {$pick($sf.RECORD.$index) != ""} {

		set irec [pick_seek $sf $pick($sf.UNIQUE.$index)]

		if {0 > $irec} {
			# not present
			pick_draw_row $sf $w $pick($sf.ROW) "$pick($sf.RECORD.$index)" \
					"-relief groove -background white" $pick($sf.MM_INDEX) "pick_multimedia"

			pick_select_record $sf $index

			incr pick($sf.NREC)
			incr pick($sf.ROW)
		} else {
			# already present
			pick_select_record $sf $irec
		}
	}
}

proc pick_unselect_all {_w _win _map} {
	if {$_w != $_win} {return}

	# catch used in case highlight hasn't been defined
	catch {Nsite_unhighlight all [string range $pick($_sf.MAP) 5 end] $pick($_sf.HIGHLIGTHED)}
	Ndraw_all
}

proc pick_seek {sf which} {
	global pick

	for {set i 0} {$i < $pick($sf.NREC)} {incr i} {
		if {$which == $pick($sf.UNIQUE.$i)} {return $i}
	}
	return -1
}

proc pick_clear_window {w sf _map} {
	if {![confirm_ask "Clear Window?" "Yes" "No"]} {return}
#	catch {destroy $w}
	pick_erase_records $sf
	pick_delete_records $sf

	# catch used in case highlight hasn't been defined
	catch {Nsite_unhighlight all [string range $pick($_sf.MAP) 5 end] $pick($_sf.HIGHLIGTHED)}
	Ndraw_all
}

proc pick_delete_records {sf} {
	global pick

	for {set i 0} {$i < $pick($sf.NREC)} {incr i} {
		unset pick($sf.RECORD.$i)
	}
	set pick($sf.NREC) 0
}

proc pick_erase_records {_sf} {
	global pick

	set nfields [llength $pick($_sf.HEADER)]

	set w [scrollframe_interior $_sf]
	set first_row [expr $pick($_sf.ROW) - $pick($_sf.NREC)]

	for {set i $first_row} {$i < $pick($_sf.ROW)} {incr i} {
		for {set j 0} {$j < $nfields} {incr j} {catch {destroy $w.r$i\c$j}}
	}
	set pick($_sf.ROW) $first_row
}

proc pick_get_multimedia_field {_header _field} {
	if {$_field == ""} {return -1}
	set i 0
	foreach elt $_header {
		if {$elt == $_field} {return $i}
		incr i
	}
	return -1
}

proc pick_draw_row {sf w row lst opt field cmd} {
	set i -1
	foreach elt $lst {
		incr i
		if {$elt == ""} {set elt "***"}
		if {$i == $field && $cmd != ""} {
			button $w.r$row\c$i -text "[list $elt]" -bd 1  -anchor e -command "$cmd [list $elt]"
			bind $w.r$row\c$i <ButtonPress-1> "pick_mouse_XY %X %Y"
		} else {
			eval label $w.r$row\c$i -text "[list $elt]" -bd 1  -anchor e "$opt"

			bind $w.r$row\c$i <Button-1> "pick_select_record_from_record $sf $row"
		}
		grid $w.r$row\c$i -row $row -column $i -sticky nsew
	}
}

proc pick_select_record_from_record {_sf _row} {
	global pick

	set first_row [expr $pick($_sf.ROW) - $pick($_sf.NREC)]
	set irec [expr $_row - $first_row]

	if {$irec < 0} {return}

	pick_select_record $_sf $irec
}

proc pick_mouse_XY {_x _y} {
	global pick
	set pick(MOUSE_X) $_x
	set pick(MOUSE_Y) $_y
}

proc pick_multimedia {_field} {
	global pick

	set ext [file extension $_field]
	rc_load_res "hyperlink.path" dir

	if {![rc_load_res "hyperlink.extension$ext" app]} {
		if {[file isdirectory $dir/$_field]} {
			set filelist [glob $dir/$_field/*]
			set m ".pick_multimedia_hyperlink"
			catch {destroy $m}
			menu $m -tearoff 0
			foreach f $filelist {
				#$m add command -label "$f" -command "pick_multimedia  $_field/$f"
				$m add command -label "[file tail $f]" -command "pick_multimedia  \"$f\""
			}
			tk_popup $m $pick(MOUSE_X) $pick(MOUSE_Y)
		} else {
			puts "*** WARNING (PICK) *** extension \"$ext\" has no associated application"
		}
	} else {
		if {[string range $_field 0 2] == "www"} {
			set filename $_field
		} else {
			set filename [file join $dir $_field]
		}

		puts "pick_multimedia: exec $app $filename"
		if {[catch {exec $app "$filename" &}]} {
			puts "*** WARNING (PICK) *** command \"$app $filename\" returns an error"
		}
	}
}

###########################################################################################
# Setting icon for selected vector objects

proc highlight_set_marker_button {_w} {
	global highlight

	set highlight(MARKERS_NAME) {"x" "sphere" "diamond" "cube" "box" "gyro" "aster"}
	set highlight(MARKERS_INDEX) {"1"  "3"        "5"     "4"    "2"  "9"    "8"}

	set m $_w
	menubutton $m -menu $m.m -relief raised -indicatoron 1 \
		-bd 1 -text "set icon" -width 10
	#pack $m -side left -fill y -padx 3 -pady 3

	menu $m.m -tearoff 0
	foreach elt $highlight(MARKERS_NAME) i $highlight(MARKERS_INDEX) {
		set highlight(MARKERS_NAME.$i) $elt
		$m.m add radiobutton -label "$elt" -command "highlight_set_marker $_w" -value $i -variable highlight(MARKER_VALUE)
	}
	
	$m configure -text $highlight(MARKERS_NAME.$highlight(MARKER_VALUE))
}

###########################################################################################
# Procedures for highlighting selected vector objects

proc highlight_set_color {_w} {
	global highlight

	set highlight(COLOR_VALUE) [mkColorPopup .colorPop color $highlight(COLOR_VALUE) 1]
	$_w configure -bg $highlight(COLOR_VALUE)
	Nsite_highlight_set_default_value color $highlight(COLOR_VALUE)
}


proc highlight_set_marker {_w} {
	global highlight

	Nsite_highlight_set_default_value marker $highlight(MARKER_VALUE)
	$_w configure -text $highlight(MARKERS_NAME.$highlight(MARKER_VALUE))
}
