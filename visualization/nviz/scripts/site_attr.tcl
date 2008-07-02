#***************************************************************
#*
#* MODULE:       site_attr.tcl 1.0
#*
#* AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
#*				 Major update Nov 2006 by Michael Barton, 
#*				 	Arizona State University
#*
#* PURPOSE:		 Variable site attribute (each point: different
#*					geometry depending from DB fields
#* 				 In conjunction with site_attr_commands.c
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
global ___ACS_utils___
if {![info exists ___ACS_utils___]} {
	# FUNCTIONS USED:
	# modal_edit_list_plain
	# scrollframe_...
	# lut_create_canvas
	source $default_panel_path/ACS_utils.tcl
}

option add *label.foreground "black"

### site markers in gsurf.h
#define ST_X    	1
#define ST_BOX    	2
#define ST_SPHERE	3
#define ST_CUBE   	4
#define ST_DIAMOND 	5
#define ST_DEC_TREE 6
#define ST_CON_TREE 7
#define ST_ASTER	8
#define ST_GYRO	    9
#define ST_HISTOGRAM    10

################################################################################
proc site_attr_init {} {
	global site_attr
	global env
	
	set site_attr(INIT) 1

	set site_attr(MENU_DEFAULT_BG) white
	set site_attr(DEFAULT_COLOR) "#ffffff"
	#set site_attr(MENU_DEFAULT_BG) "#e6e6e6"
	set site_attr(ATTR_LIST) {color size marker}
	set site_attr(NONE) "default"
	set site_attr(FIXED) "fixed"
	set site_attr(AUTO_LUT) {"#ffff00" "#00ff00" "#00ffff" "#0000ff" "#ff00ff" "#ff0044" "#ff0000"}

	set site_attr(LUT_NUMBER) 0
	set site_attr(LUT_LIST) [list]
	set site_attr(POPUP) 0
	set site_attr(LUT_SEP) "@@@"
	if {[info exists env(HOME)]} {
		#set site_attr(LUT_DIR) "~/"
		set site_attr(LUT_DIR) $env(HOME)
	} else {
		set site_attr(LUT_DIR) "/"
	}
	set site_attr(LUT_EXT) "lut"
	set site_attr(LUT_FILETYPES) {
		{{NVIZ thematic prefs files} {.lut}}
		{{All files}   *   }
	}

	set site_attr(MARKERS_NAME) {"histogram" "x"  "sphere" "cube" "box" "diamond" "gyro" "aster"}
	set site_attr(MARKERS_INDEX) {"10"       "1"     "3"      "4"   "2"    "5"      "9"    "8"}

	set site_attr(FIRST_ROW) 1


}

################################################################################
proc site_attr_gui {themefr bottomfr curr} {
	global site_attr

	if {![info exists site_attr(INIT)]} {site_attr_init}


# Set variable for LUT_MENUBUTTON ##############################################
	set site_attr(LUT_MENUBUTTON_WIN) $bottomfr
	set site_attr(LUT_MENUBUTTON) $bottomfr.lut_menu
	#set site_attr(FIELD_ATTR_PANEL) $theme_pt

# Set fields/attribute panel and LUT menubutton# ###############################

	if {$site_attr(FIELD_ATTR_PANEL) == 1} {
		set container [frame $themefr.win -container 1]
		pack $container -padx 10 -side bottom -expand 0 -fill x
		set win $themefr.extra
		toplevel $win -use [winfo id $container]
		wm maxsize $win 500 150
		set site_attr(WIN) $win
	}
	
	site_attr_lut_menubutton_win
	site_attr_fields_attributes_win $curr		
	pack forget $themefr $bottomfr
	if {$site_attr(FIELD_ATTR_PANEL) == 1} {
		pack $themefr -side top -expand yes -fill none -pady 2
	}
	pack $bottomfr -fill x -expand 1 -padx 3 -pady 3

}

################################################################################
# Add button for LUT load ######################################################
# assumes that $bottom is defined and $bottom.draw_current is packed
proc site_attr_lut_menubutton_win {} {
	global site_attr

	if {$site_attr(FIELD_ATTR_PANEL) == 0} {
		catch {destroy $site_attr(LUT_MENUBUTTON)}
		pack $site_attr(LUT_MENUBUTTON_WIN).draw_current \
			-side left 
		return
	}

	catch {pack forget $site_attr(LUT_MENUBUTTON_WIN).draw_current}

	site_attr_lut_menu $site_attr(LUT_MENUBUTTON_WIN)

	pack $site_attr(LUT_MENUBUTTON_WIN).draw_current \
		$site_attr(LUT_MENUBUTTON) \
		-padx 3 -side left 
}

proc site_attr_fields_attributes_win {_curr_site} {
	global site_attr

	if {0 == $_curr_site}  {return}

	if {$site_attr(FIELD_ATTR_PANEL) == 0} {
		catch {destroy $site_attr(WIN).sf}
		catch {destroy $site_attr(WIN)}
		return
	}

	set sf [scrollframe_create $site_attr(WIN).sf]
	pack $sf -side bottom -expand yes -fill x

	set tmp [scrollframe_interior $sf]
	
	frame $tmp.attributes
		set site_attr(FIELD_NAMES) {}
		set site_attr(FIELD_TYPES) {}
		foreach {n t} [Nsite_attr_get_fields_name_and_type [Nsite$_curr_site get_att map]] {
			lappend site_attr(FIELD_NAMES) $n
			lappend site_attr(FIELD_TYPES) $t
		}

		set _col 0
		set l $tmp.attributes.row\_label
		label $l -text "" -padx 3
		grid $l -column $_col -row 0 -sticky ew
		incr _col
		foreach _attr $site_attr(ATTR_LIST) {
			set l $tmp.attributes.$_attr\_label
			if {$_attr=="marker"} {set _attr "icon"}
			label $l -text "$_attr" -padx 3 -fg black
			grid $l -column $_col -row 0 -sticky ew
			incr _col
		}

		set max [Nsite_attr_get_value "GPT_MAX_ATTR"]
		for {set i $site_attr(FIRST_ROW)} {$i <= $max} {incr i} {
			site_attr_fields_attributes_row $tmp.attributes Nsite$_curr_site $i
		}

	pack $tmp.attributes -side bottom -expand 1 -fill x

}

################################################################################
# Here idx is created

proc site_attr_idx {_map _attr _row} {
	return $_map$_attr$_row
}

proc site_attr_fields_attributes_row {_win _map _row} {
	global site_attr

	set _col 0

	set l $_win._menu_row$_row
	label $l -text $_row -padx 3
	grid $l -column $_col -row $_row -sticky w
	incr _col

	foreach _attr $site_attr(ATTR_LIST) {
		set idx [site_attr_idx $_map $_attr $_row]

		if {[info exists site_attr($idx.OLD_INDEX)]} {
			# already been here
			set $site_attr($idx.INDEX) $site_attr($idx.OLD_INDEX)
		} else {
			# first time in this GUI
			set site_attr($idx.OLD_INDEX) -1
			set site_attr($idx.ALREADY_SET) 0
			set site_attr($idx.USE_EXTERNAL_VALUES) 0
			set site_attr($idx.USING_EXTERNAL_VALUES) 0
			set site_attr($idx.MENU_TXT) $site_attr(NONE)
			set site_attr($idx.MENU_BG) $site_attr(MENU_DEFAULT_BG)

			set site_attr($idx.ATTR) $_attr
			set site_attr($idx.ROW) $_row
			set site_attr($idx.MAP) $_map
		}

		set site_attr($idx.MENU) [site_attr_fields_menu_button $idx $_win]

		grid $site_attr($idx.MENU) -column $_col -row $_row -sticky w

		# if window already open: let's color the button
		if {[winfo exists .w$idx]} {site_attr_set_menubutton $idx}

		incr _col
	}
}

proc site_attr_fields_menu_button {_idx _win} {
	global site_attr

	set m $_win.menu_$_idx
	menubutton $m -menu $m.m -relief raised -indicatoron 1 -bd 1 \
		-text $site_attr($_idx.MENU_TXT) -bg $site_attr($_idx.MENU_BG)

	menu $m.m -tearoff 0


	if {$site_attr($_idx.ATTR) == "marker"} {
		foreach elt $site_attr(MARKERS_NAME) i $site_attr(MARKERS_INDEX) {
			$m.m add radiobutton -label "$elt" -command "site_attr_set_marker $_idx $elt" -value $i -variable site_attr($_idx.INDEX)
		}
		$m.m add radiobutton -label $site_attr(NONE) -command "site_attr_set_marker $_idx $site_attr(NONE)" -value -1 -variable site_attr($_idx.INDEX)
	} else {
		set i 0
		set nrows 30
		foreach elt $site_attr(FIELD_NAMES) {
			set br [expr (($i-1) % $nrows)/($nrows-1)]
			$m.m add radiobutton -label "$elt" \
				-command "site_attr_set_win $_idx $elt" \
				-value $i \
				-variable site_attr($_idx.INDEX) \
				-columnbreak $br
			incr i
		}
		$m.m add radiobutton -label $site_attr(NONE) -command "site_attr_set_win $_idx $site_attr(NONE)" -value -1 -variable site_attr($_idx.INDEX)
		$m.m add radiobutton -label $site_attr(FIXED) -command "site_attr_set_win $_idx $site_attr(FIXED)" -value -2 -variable site_attr($_idx.INDEX)
	}
	return $m
}
#
# enough for GUI
################################################################################


################################################################################
# here starts the attributes window creation and operation
#
proc site_attr_set_win {_idx _name {_create_win "yes"}} {
	global site_attr

	# "none" and "fixed" are special cases that must be treated immediately
	if {$_name == $site_attr(NONE)} {

		# we have cleared the field
		$site_attr($_idx.MAP) unset_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] $site_attr($_idx.ATTR)
		site_attr_config_menubutton $_idx $site_attr(NONE) $site_attr(MENU_DEFAULT_BG) -1

	} elseif {$_name == $site_attr(FIXED)} {

		if {$site_attr($_idx.ATTR) == "color"} {

			if {$site_attr($_idx.OLD_INDEX) == -2} {
				set old_color [$site_attr($_idx.MENU) cget -bg]
			} else {
				set old_color $site_attr(DEFAULT_COLOR)
			}
			set color [mkColorPopup .colorPop color $old_color 1]
			if {$color != $old_color} {
				$site_attr($_idx.MAP) set_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] $site_attr($_idx.ATTR) -2 $color $color
				site_attr_config_menubutton $_idx $site_attr(FIXED) $color -2

			} else {
				set site_attr($_idx.INDEX) $site_attr($_idx.OLD_INDEX)
				return
			}

		} elseif {$site_attr($_idx.ATTR) == "size"} {

			if {$site_attr($_idx.OLD_INDEX) == -2} {
				set old_entry [$site_attr($_idx.MENU) cget -text]
			} else {
				set old_entry 1
			}
			set labels [list "size"]
			set entries [list $old_entry]
			if {[modal_edit_list_plain "insert size" labels entries]} {
				set size [lindex $entries 0]
				$site_attr($_idx.MAP) set_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] $site_attr($_idx.ATTR) -2 $size $size
				site_attr_config_menubutton $_idx $size $site_attr(MENU_DEFAULT_BG) -2
			} else {
				set site_attr($_idx.INDEX) $site_attr($_idx.OLD_INDEX)
				return
			}
		}

	} else {
		# here we have chosen a field, so we should open the attributes window
		set win ".w$_idx"

		# index of the current field
		set index $site_attr($_idx.INDEX)

		# if window already open and set: raise it!
		# 	otherwise close it and reopen a new one
		if {[winfo exists $win]} {
			if {$site_attr($_idx.ALREADY_SET)} {
				raise $win
				if {$site_attr($_idx.INDEX) != $site_attr($_idx.OLD_INDEX)} {
					set site_attr($_idx.INDEX) $site_attr($_idx.OLD_INDEX)
					puts "WARNING: to change attribute ($site_attr($_idx.ATTR)) field, old attribute windows must be closed!"
				}
				return
			} else {
				destroy $win
				set site_attr($_idx.INDEX) $index
			}
		}

		# let's check if this field has already been used for this attribute
		if {[info exists site_attr($site_attr($_idx.ATTR)$index)]} {
			# yes, it is!
			# verify if other window is open with same field
			set other_row $site_attr($site_attr($_idx.ATTR)$index)
			set other_idx [site_attr_idx $site_attr($_idx.MAP) $site_attr($_idx.ATTR) $other_row]

			set other_win ".w$other_idx"
			if {[winfo exists $other_win]} {
				set site_attr($_idx.INDEX) $site_attr($_idx.OLD_INDEX)
				raise $other_win
				puts "WARNING: attribute ($site_attr($_idx.ATTR)) field $_name already set in row $other_row"
				return
			}

			# ok there is no open window with this field

			# let's check if the $_row with the field already set is different from
			# 	the one we are trying to open
			if {$other_row != $site_attr($_idx.ROW)} {
				set site_attr($_idx.INDEX) $site_attr($_idx.OLD_INDEX)
				site_attr_set_win $other_idx $_name
				puts "WARNING: attribute ($site_attr($_idx.ATTR)) field $_name already set in row $other_row"
				return
			}

			# ok it's the same, we can open it anyway
		}

		# ok let's create this window
		# we must fill the lists anyway because they can have been replaced

		set site_attr($_idx.TYPE) [lindex $site_attr(FIELD_TYPES) $site_attr($_idx.INDEX)]
		site_attr_load_values $_idx
		set site_attr($_idx.NAME) $_name
		set site_attr($_idx.WIN) $win

		if {$_create_win == "yes"} {
			site_attr_create_win $_idx
		}
	}
	Ndraw_all
}

proc site_attr_load_values {_idx} {
	global site_attr
	appBusy
	# because this can take a long time
	if { $site_attr($_idx.TYPE) != "s"} {
		set site_attr($_idx.VALUES_LIST) \
			[lsort -real -unique [Nsite_attr_get_field_values [$site_attr($_idx.MAP) get_att map] $site_attr($_idx.INDEX)]]
	} else {
		set site_attr($_idx.VALUES_LIST) \
			[lsort -unique [Nsite_attr_get_field_values [$site_attr($_idx.MAP) get_att map] $site_attr($_idx.INDEX)]]
	}
	appNotBusy
}

proc site_attr_create_win {_idx} {
	global site_attr
	global nviztxtfont

	switch $site_attr($_idx.ATTR) {
		"size" {
			set which "entries"
			set apply_msg \
			"Fill 2 or more entry fields with \
			desired min and max values,\
			then press \[Apply\]. \
			Or press \[Auto\] to automatically \
			generate a range of symbol sizes."


		}
		"color" {
			set which "colors"
			set apply_msg \
			"Fill 2 or more entry fields with desired\
			colors, then press \[Apply\].\
			Or press \[Auto\] to automatically\
			generate a color table."
		}
		default {
			puts "WARNING: No thematic mapping preferences set for $site_attr($_idx.ATTR)!"
		}
	}

	set win $site_attr($_idx.WIN)
	toplevel $win
	wm resizable $win true true
	catch {wm title $win "Site:[$site_attr($_idx.MAP) get_att map]"}
	wm minsize $win 150 100

	site_attr_set_menubutton $_idx
	bind $win <Destroy> "site_attr_reset_menubutton $_idx %W"
	bind $win <Destroy> "site_attr_destroy_win $_idx %W"

	set w [frame $win.left]

	set site_attr($_idx.WIN_CMD) $w

# Attribute
		label $w.attr -text "Vary point $site_attr($_idx.ATTR)"
		pack $w.attr -pady 1 -padx 0 -fill x -side top

# Field
		label $w.field -text "GIS attribute: $site_attr($_idx.NAME)"
		pack $w.field -padx 0 -fill x -side top

# Type
		if {$site_attr($_idx.TYPE) == "s"} {
			set type "string"
		} else { 
			set type "numeric"
		}
		label $w.type -text "Attribute type: $type"
		pack $w.type -padx 0 -fill x -side top

# Instructions
		message $w.help -font $nviztxtfont -width 200 \
			-text $apply_msg
		pack $w.help -side top -pady 3

# button frame
		frame $w.buttons -borderwidth 0 -relief flat

# Apply
		Button $w.buttons.apply -text "Apply" -bd 1 -width 6 \
			-command "site_attr_set_$which $_idx" \
			-helptext "Apply current thematic preferences to point display"

# Auto
		Button $w.buttons.auto -text "Auto" -bd 1 -width 6\
			-command "site_attr_set_auto_$which $_idx" \
			-helptext "Automatically generate a color \
			table distributed across range of attribute values"

# Clear
		Button $w.buttons.reset -text "Reset" -bd 1 -width 6\
			-command "site_attr_clear_lut_win $_idx" \
			-helptext "Clear all thematic settings"
			#pack $w.reset.set -side top -pady 5

		pack $w.buttons.apply $w.buttons.auto $w.buttons.reset \
			-side left -fill x -expand 1 -padx 2
		pack $w.buttons -expand 1 -fill both -pady 5 -padx 2
		
# Theme prefs management
		set site_attr($_idx.EXTERNAL_LUT_PANEL) 0
		frame $w.external_lut -borderwidth 0 -relief flat
		checkbutton $w.external_lut.set -text "Load/save theme preferences" \
			-variable site_attr($_idx.EXTERNAL_LUT_PANEL) -command "site_attr_external_lut $_idx $w.external_lut"
		pack $w.external_lut.set -side top
		pack $w.external_lut -padx 0 -fill x -side top


	pack $w -side left -anchor n

	site_attr_create_lut_win $_idx

	if {$site_attr($_idx.USING_EXTERNAL_VALUES) == 1} {
		set site_attr($_idx.EXTERNAL_LUT_PANEL) 1
		site_attr_external_lut $_idx $w.external_lut
		site_attr_lut_update_external $_idx
	}
	return
}


proc site_attr_external_lut {_idx _win} {
	global site_attr
	global nviztxtfont
	
	if {$site_attr($_idx.EXTERNAL_LUT_PANEL) == 0} {
		catch {destroy $_win.external_lut_panel}
		return
	}

	set w [frame $_win.external_lut_panel -borderwidth 1 -relief raised]

# External LUT manipulation
	frame $w.lut_choice  -borderwidth 0
	frame $w.lut_choice.a  -borderwidth 0
	label $w.lut_choice.a.title -text "Select thematic prefs to use"
	pack $w.lut_choice.a.title -side top -pady 1

	radiobutton $w.lut_choice.a.local -relief flat -text "current prefs" \
		-value 0 -anchor nw -variable site_attr($_idx.USE_EXTERNAL_VALUES) \
		-command "site_attr_lut_local $_idx"
	pack $w.lut_choice.a.local -fill x -expand 1 -side top

	radiobutton $w.lut_choice.a.external -relief flat -text "prefs. from file" \
		-value 1 -anchor nw -variable site_attr($_idx.USE_EXTERNAL_VALUES)
		bind $w.lut_choice.a.external <ButtonRelease> "site_attr_lut_external $_idx %X %Y"
	pack $w.lut_choice.a.external -fill x -expand 1 -side top

	label $w.lut_choice.a.external_lab -fg black -font $nviztxtfont \
		-text "(preserves current prefs.)"
	pack $w.lut_choice.a.external_lab -side top
	pack $w.lut_choice.a -side top -pady 3 -fill x

	if {[info exists site_attr($_idx.EXTERNAL_LUT)]} {
		set lut_id $site_attr($_idx.EXTERNAL_LUT)
		trace variable site_attr($lut_id.NAME) w "site_attr_update_win $_idx"

		set lut_label $site_attr($lut_id.NAME)
		set lut_command "site_attr_lut_open_win $lut_id"
	} else {
		set lut_label "no prefs loaded"
		set lut_command ""
	}

	frame $w.lut_choice.b  -borderwidth 0
	label $w.lut_choice.b.ext_lab -text "View loaded thematic prefs"
	pack $w.lut_choice.b.ext_lab -side top

	set site_attr($_idx.EXTERNAL_LUT_BUTTON) \
		[Button $w.lut_choice.b.ext_lut -text $lut_label \
			-command $lut_command -bd 1 -width 15]
	pack $w.lut_choice.b.ext_lut -side top -pady 1
	pack $w.lut_choice.b -pady 3 -padx 0 -side top -fill x
	pack $w.lut_choice -padx 0 -side top -fill both -expand 1

	frame $w.io  -borderwidth 0
	frame $w.io.import_lut  -borderwidth 0
	label $w.io.import_lut.a -text "Load and save prefs."
	pack $w.io.import_lut.a -side top
	Button $w.io.import_lut.b -text "Load prefs" -width 15 -bd 1 \
		-command "site_attr_lut_import $_idx" \
		-helptext "Replace current themes with prefs loaded from file"
	pack $w.io.import_lut.b -side top
	bind $w.io.import_lut.b <ButtonPress> "site_attr_pick_mouse_XY %X %Y"
	pack $w.io.import_lut -pady 3 -padx 0 -side top -fill x

	frame $w.io.export_lut  -borderwidth 0
	Button $w.io.export_lut.b -text "Save prefs" -width 15 -bd 1 \
		-command "site_attr_lut_export $_idx" \
		-helptext "Copy current themes to prefs loaded from file"
	pack $w.io.export_lut.b -side top
	bind $w.io.export_lut.b <ButtonPress> "site_attr_pick_mouse_XY %X %Y"
	pack $w.io.export_lut -pady 3 -padx 0 -side top -fill x
	pack $w.io -pady 3 -padx 0 -side bottom -fill both -expand 1
	pack $w -side top -fill both -expand 1
}

# bond to site_attr($_lut_id.NAME) changes
proc site_attr_update_win {_idx var index op} {
	upvar $var v
	$v($_idx.EXTERNAL_LUT_BUTTON) configure -text $v($index)
}

proc site_attr_destroy_win {_idx _win} {
	global site_attr
	if {$_win != $site_attr($_idx.WIN)} {return}
	if {![info exists site_attr($_idx.EXTERNAL_LUT)]} {return}
	set lut_id $site_attr($_idx.EXTERNAL_LUT)
	trace vdelete site_attr($lut_id.NAME) w "site_attr_update_win $_idx"
}

proc site_attr_clear_lut_win {_idx} {
	global site_attr

	# At least an "Apply" should have been done
	if {$site_attr($_idx.OLD_INDEX) != $site_attr($_idx.INDEX)} {return}

	catch {unset site_attr($_idx.XLIST)}
	catch {unset site_attr($_idx.YLIST)}

	# reload because other values could have been loaded via "Import"
	site_attr_load_values $_idx

	catch {destroy $site_attr($_idx.WIN_CMD).lut}
	set site_attr($_idx.USE_EXTERNAL_VALUES) 0
	set site_attr($_idx.USING_EXTERNAL_VALUES) 0

	site_attr_clear_menubutton $_idx
	site_attr_create_lut_win $_idx

	$site_attr($_idx.MAP) unset_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] $site_attr($_idx.ATTR)
	Ndraw_all
}

proc site_attr_create_lut_win {_idx} {
	global site_attr
	global nviztxtfont
	
	set win $site_attr($_idx.WIN)
	catch {destroy $win.f}

	set sf [scrollframe_create $win.f]
	set w [scrollframe_interior $sf]

	set row 0
	set site_attr($_idx.WIN_SF) $w
	set site_attr($_idx.FIRST_ROW) $row

	set already_set $site_attr($_idx.ALREADY_SET)

	if {$already_set > 0} {
		set already_set_i 0
		set already_set_n [llength $site_attr($_idx.XLIST)]

		set already_set_elt [lindex $site_attr($_idx.XLIST) 0]
		set already_set_val [lindex $site_attr($_idx.YLIST) 0]
	}

	foreach elt $site_attr($_idx.VALUES_LIST) {
		label $w.n$row -text "$row" -relief groove -borderwidth 1 \
			-anchor e -fg black -padx 4
		grid $w.n$row	-row $row -column 0 -columnspan 1 -sticky nswe
		label $w.v$row -text "$elt" -relief groove -borderwidth 1 \
			-anchor w -fg black -font $nviztxtfont -padx 2
		grid $w.v$row	-row $row -column 1 -columnspan 1 -sticky nswe

		switch $site_attr($_idx.ATTR) {
			"size" {
				entry $w.e$row  -relief sunken -width 6 -justify right -bg white
				grid $w.e$row	-row $row -column 2 -columnspan 1 -sticky nswe

				if {$already_set > 0} {
					if {$elt == $already_set_elt} {
						$w.e$row  delete 0 end
						$w.e$row  insert 0 $already_set_val
						incr already_set_i
						if {$already_set_i < $already_set_n} {
							set already_set_elt [lindex $site_attr($_idx.XLIST) $already_set_i]
							set already_set_val [lindex $site_attr($_idx.YLIST) $already_set_i]
						} else {
							set already_set 0
						}
					}
				}

			}
			"color" {
				Button $w.b$row -command "site_attr_choose_color $w.b$row" -bg $site_attr(DEFAULT_COLOR) -width 0 -height 0
				grid $w.b$row	-row $row -column 2 -columnspan 1 -sticky nswe

				label $w.l$row -bg $site_attr(DEFAULT_COLOR) -width 3 -borderwidth 0 -relief flat
				grid $w.l$row	-row $row -column 3 -columnspan 1 -sticky nswe

				if {$already_set > 0} {
					if {$elt == $already_set_elt} {
						$w.b$row  configure -bg $already_set_val
						incr already_set_i
						if {$already_set_i < $already_set_n} {
							set already_set_elt [lindex $site_attr($_idx.XLIST) $already_set_i]
							set already_set_val [lindex $site_attr($_idx.YLIST) $already_set_i]
						} else {
							set already_set -1
						}
					}
				}
			}
			default {
				puts "WARNING: No attribute behaviour for $site_attr($_idx.ATTR)!"
			}
		}
		incr row
	}

	pack $sf -expand yes -fill both -side top -padx 5

	if {$already_set < 0} {site_attr_update_gui_colors $_idx}
}

#################################################################################
#################################################################################
#################################################################################
#
# menubutton manipulation

# called by "none", "fixed" and "set_marker"
proc site_attr_config_menubutton {_idx _txt _bg _index} {
	global site_attr

	# if it was already "none" or "fixed", nothing to unset
	if {$site_attr($_idx.OLD_INDEX) >= 0} {

		# clear the flag for "already selected"
		catch {unset site_attr($site_attr($_idx.ATTR)$site_attr($_idx.OLD_INDEX))}

		# destroy window if it's open
		catch {destroy $site_attr($_idx.WIN)}

		# clear memory

		# these are set by "site_attr_create_win"
		catch {unset site_attr($_idx.NAME)}
		catch {unset site_attr($_idx.TYPE)}

		catch {unset site_attr($_idx.VALUES_LIST)}
		catch {unset site_attr($_idx.LUT_LIST)}
		catch {unset site_attr($_idx.XLIST)}
		catch {unset site_attr($_idx.YLIST)}

		catch {unset site_attr($_idx.FIRST_ROW)}
		catch {unset site_attr($_idx.WIN)}
		catch {unset site_attr($_idx.WIN_CMD)}
		catch {unset site_attr($_idx.WIN_SF)}
	}

	set site_attr($_idx.OLD_INDEX) $_index
	set site_attr($_idx.INDEX) $_index
	set site_attr($_idx.ALREADY_SET) 0
	set site_attr($_idx.MENU_TXT) $_txt
	set site_attr($_idx.MENU_BG) $_bg

	$site_attr($_idx.MENU) configure -text $_txt -bg $_bg
}

# called when attr win is created
proc site_attr_set_menubutton {_idx} {
	global site_attr

	set site_attr($_idx.MENU_BG) [$site_attr($_idx.MENU) cget -bg]
	set site_attr($_idx.MENU_TXT) [$site_attr($_idx.MENU) cget -text]
	$site_attr($_idx.MENU) configure -text $site_attr($_idx.NAME) -bg grey70

	# needed to check if the same attr/index is already open or set in another $_row
	# must be cleared when "none"
	set site_attr($site_attr($_idx.ATTR)$site_attr($_idx.INDEX)) $site_attr($_idx.ROW)
}

# called when attr win is destroyed
proc site_attr_reset_menubutton {_idx _win} {
	global site_attr

	if {$_win != $site_attr($_idx.WIN)} {return}

	catch {$site_attr($_idx.MENU) configure -text $site_attr($_idx.MENU_TXT) -bg $site_attr($_idx.MENU_BG)}

	if {$site_attr($_idx.ALREADY_SET) == 1} {
		set site_attr($site_attr($_idx.ATTR)$site_attr($_idx.OLD_INDEX)) $site_attr($_idx.ROW)
	} else {
		catch {unset site_attr($site_attr($_idx.ATTR)$site_attr($_idx.INDEX))}
	}
	set site_attr($_idx.INDEX) $site_attr($_idx.OLD_INDEX)
}

# called by "Apply" button of attr win
proc site_attr_update_menubutton {_idx} {
	global site_attr

	catch {$site_attr($_idx.MENU) configure -text $site_attr($_idx.NAME) -bg grey60}
	set site_attr($_idx.MENU_TXT) $site_attr($_idx.NAME)

	# clear the flag for "already selected"
	set old_index -99; if {[info exists site_attr($_idx.OLD_INDEX)]} {set old_index $site_attr($_idx.OLD_INDEX)}
	if {$old_index != $site_attr($_idx.INDEX)} {
		catch {unset site_attr($site_attr($_idx.ATTR)$site_attr($_idx.OLD_INDEX))}
	}

	set site_attr($_idx.OLD_INDEX) $site_attr($_idx.INDEX)
	set site_attr($_idx.ALREADY_SET) 1
}

# called by "Clear" button of attr win
proc site_attr_clear_menubutton {_idx} {
	global site_attr

	catch {$site_attr($_idx.MENU) configure -text $site_attr($_idx.NAME) -bg grey70}
	set site_attr($_idx.MENU_TXT) $site_attr(NONE)

	catch {unset site_attr($site_attr($_idx.ATTR)$site_attr($_idx.OLD_INDEX))}

	set site_attr($_idx.OLD_INDEX) -1
	set site_attr($_idx.ALREADY_SET) 0
}

#

################################################################################
# MARKERS are managed here
################################################################################

proc site_attr_set_marker {_idx _name} {
	global site_attr

	set marker $site_attr($_idx.INDEX)
	if {$_name == $site_attr(NONE)} {
		$site_attr($_idx.MAP) unset_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] "marker"
	} else {
		$site_attr($_idx.MAP) set_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] "marker" -2 $marker $marker
	}

	site_attr_config_menubutton $_idx $_name $site_attr(MENU_DEFAULT_BG) $marker
	Ndraw_all
}


################################################################################
# NUMERIC ENTRIES are managed here
################################################################################

# this is called when the Apply button has been pressed
proc site_attr_set_entries {_idx} {
	global site_attr

	set row $site_attr($_idx.FIRST_ROW)
	set w $site_attr($_idx.WIN_SF)

	set site_attr($_idx.XLIST) [list]
	set site_attr($_idx.YLIST) [list]
	foreach elt $site_attr($_idx.VALUES_LIST) {
		set val [$w.e$row get]
		if {$val != ""} {
			lappend site_attr($_idx.XLIST) $elt
			lappend site_attr($_idx.YLIST) $val
		}
		incr row
	}

	set len [llength $site_attr($_idx.YLIST)]
	if {$len < 2} {
		tk_messageBox -icon warning -message "Too few values: at least 2!" -type ok -parent $site_attr($_idx.WIN_CMD)
	} else {
		site_attr_update_entries $_idx
	}
	return $len
}

# Auto entries generation
proc site_attr_set_auto_entries {_idx} {
	global site_attr

	set n [expr [llength $site_attr($_idx.VALUES_LIST)] - 1]
	# at least 2 elements
	if {$n < 1} {return}

	set site_attr($_idx.XLIST) [list]
	set site_attr($_idx.YLIST) [list]

	site_attr_append_entries $_idx 0 1
	site_attr_append_entries $_idx $n 10

	site_attr_update_entries $_idx
}

proc site_attr_append_entries {_idx n e} {
	global site_attr

	lappend site_attr($_idx.XLIST) [lindex $site_attr($_idx.VALUES_LIST) $n]
	lappend site_attr($_idx.YLIST) $e
	set row [expr $site_attr($_idx.FIRST_ROW) + $n]

	$site_attr($_idx.WIN_SF).e$row  delete 0 end
	$site_attr($_idx.WIN_SF).e$row  insert 0 $e
}

proc site_attr_update_entries {_idx} {
	global site_attr

	$site_attr($_idx.MAP) set_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)] $site_attr($_idx.ATTR) \
		$site_attr($_idx.INDEX) $site_attr($_idx.XLIST) $site_attr($_idx.YLIST)

	Ndraw_all
	site_attr_update_menubutton $_idx
}


################################################################################
# COLORS ENTRIES are managed here
################################################################################

# this is called when the Apply button has been pressed
proc site_attr_set_colors {_idx} {
	global site_attr

	if {[site_attr_read_gui_colors $_idx] < 2} {
		tk_messageBox -icon warning -message "Too few values: at least 2!" -type ok -parent $site_attr($_idx.WIN_CMD)
	} else {
		site_attr_update_colors $_idx
		site_attr_update_gui_colors $_idx
	}
}

proc site_attr_read_gui_colors {_idx} {
	global site_attr

	set row $site_attr($_idx.FIRST_ROW)
	set w $site_attr($_idx.WIN_SF)

	set site_attr($_idx.XLIST) [list]
	set site_attr($_idx.YLIST) [list]
	foreach elt $site_attr($_idx.VALUES_LIST) {
		set val [$w.b$row cget -bg]
		if {$val != $site_attr(DEFAULT_COLOR)} {
			lappend site_attr($_idx.XLIST) $elt
			lappend site_attr($_idx.YLIST) $val
		}
		incr row
	}
	return [llength $site_attr($_idx.YLIST)]
}


# Auto LUT generation
proc site_attr_set_auto_colors {_idx} {
	global site_attr

	set n [llength $site_attr($_idx.VALUES_LIST)]
	# at least 2 elements
	if {$n < 2} {return}

	set site_attr($_idx.XLIST) [list]
	set site_attr($_idx.YLIST) [list]

	set imax [expr [llength $site_attr(AUTO_LUT)] - 1]
	set old_index -1

	for {set i 0} {$i < $imax} {incr i} {
		set index [expr ($i * $n) / $imax]
		if {$index > $old_index} {
			site_attr_append_color $_idx $index [lindex $site_attr(AUTO_LUT) $i]
			set old_index $index
		}
	}

	set index [expr $n -1]
	if {$index > $old_index} {
		site_attr_append_color $_idx $index [lindex $site_attr(AUTO_LUT) $i]
	}

	site_attr_update_colors $_idx
	site_attr_update_gui_colors $_idx
}

proc site_attr_append_color {_idx n color} {
	global site_attr

	lappend site_attr($_idx.XLIST) [lindex $site_attr($_idx.VALUES_LIST) $n]
	lappend site_attr($_idx.YLIST) $color
	set row [expr $site_attr($_idx.FIRST_ROW) + $n]

	$site_attr($_idx.WIN_SF).b$row configure -bg $color
}

proc site_attr_update_colors {_idx} {
	global site_attr

	$site_attr($_idx.MAP) set_att useatt [expr $site_attr($_idx.ROW) - $site_attr(FIRST_ROW)]  $site_attr($_idx.ATTR)\
		$site_attr($_idx.INDEX) $site_attr($_idx.XLIST) $site_attr($_idx.YLIST)

	Ndraw_all
	site_attr_update_menubutton $_idx

	set site_attr($_idx.MENU_BG) $site_attr(MENU_DEFAULT_BG)
}

proc site_attr_update_gui_colors {_idx} {
	global site_attr

	# window name isn't defined
	if {![info exists site_attr($_idx.WIN)]} {return}
	# window isn't open
	if {![winfo exists $site_attr($_idx.WIN)]} {return}

	# updates labels and buttons
	if {$site_attr($_idx.TYPE) == "s"} {
		site_attr_update_gui_colors_string $_idx
	} else {
		site_attr_update_gui_colors_numeric $_idx
	}

	# creates or updates LUT in window
	set win $site_attr($_idx.WIN_CMD).lut
	catch {destroy $win}
	lut_create_canvas $win $site_attr($_idx.VALUES_LIST) $site_attr($_idx.LUT_LIST) 300 140 260 40 7 v
	pack $win -padx 2 -padx 0 -fill x -side top
}

proc site_attr_update_gui_colors_numeric {_idx} {
	global site_attr

	set row $site_attr($_idx.FIRST_ROW)
	set w $site_attr($_idx.WIN_SF)

	set site_attr($_idx.LUT_LIST) [Nget_interpolated_values "color" $site_attr($_idx.VALUES_LIST) \
		$site_attr($_idx.XLIST) $site_attr($_idx.YLIST)]

	foreach y $site_attr($_idx.LUT_LIST) {
		$w.l$row configure -bg $y
		incr row
	}
}

proc site_attr_update_gui_colors_string {_idx} {
	global site_attr

	set row $site_attr($_idx.FIRST_ROW)
	set w $site_attr($_idx.WIN_SF)

	set xval [lindex $site_attr($_idx.XLIST) 0]
	set yval [lindex $site_attr($_idx.YLIST) 0]
	set i 0

	set site_attr($_idx.LUT_LIST) [list]

	foreach x $site_attr($_idx.VALUES_LIST) {
		if {$x == $xval} {
			set xval [lindex $site_attr($_idx.XLIST) [expr $i+1]]
			set yval [lindex $site_attr($_idx.YLIST) $i]
			incr i
		}
		$w.l$row configure -bg $yval
		lappend site_attr($_idx.LUT_LIST) $yval
		incr row
	}
}

# this is called by the color window buttons
proc site_attr_choose_color {_w} {
	set color [mkColorPopup .colorPop color [$_w cget -bg] 1]
	$_w configure -bg $color
}


################################################################################

# Thematic prefs file management

################################################################################

# "local" radiobutton
proc site_attr_lut_local {_idx {_var ""} {_index ""} {_op ""}} {
	global site_attr

	if {$site_attr($_idx.USING_EXTERNAL_VALUES) == 0} {return}

	set site_attr($_idx.USING_EXTERNAL_VALUES) 0
	set site_attr($_idx.USE_EXTERNAL_VALUES) 0

	site_attr_lut_unlink_external $_idx

	unset site_attr($_idx.EXTERNAL_LUT)

	switch $site_attr($_idx.ATTR) {
		"size" {
			if {[site_attr_set_entries $_idx] < 2} {
				site_attr_clear_lut_win $_idx
			}
		}
		"color" {
			if {[site_attr_read_gui_colors $_idx] < 2} {
				site_attr_clear_lut_win $_idx
			} else {
				site_attr_update_colors $_idx
				site_attr_update_gui_colors $_idx
			}
		}
	}
}

# "external" radiobutton
proc site_attr_lut_external {_idx _x _y} {
	global site_attr
	set site_attr(MOUSE_X) $_x
	set site_attr(MOUSE_Y) $_y

	switch [llength $site_attr(LUT_LIST)] {
		0 		{
			tk_messageBox -icon warning -message "No prefs file loaded" -type ok -parent $site_attr($_idx.WIN_CMD)
			set site_attr($_idx.USE_EXTERNAL_VALUES) 0
		}
		1 		{site_attr_lut_set_external $_idx [lindex $site_attr(LUT_LIST) 0]}
		default	{
			set m [site_attr_lut_popup $_idx site_attr_lut_set_external]
			bind $m <Enter> "site_attr_lut_popup_enter"
			bind $m <Leave> "site_attr_lut_popup_leave"
			bind $m <Unmap> "site_attr_lut_popup_unmap $_idx"
		}
	}
}

proc site_attr_lut_link_external {_idx _lut_id} {
	global site_attr

	set site_attr($_idx.EXTERNAL_LUT) $_lut_id

	trace variable site_attr($_lut_id) w "site_attr_lut_update_external $_idx"
	trace variable site_attr($_lut_id) u "site_attr_lut_local $_idx"

	catch {$site_attr($_idx.EXTERNAL_LUT_BUTTON) configure -text $site_attr($_lut_id.NAME)  -command "site_attr_lut_open_win $_lut_id"}
	trace variable site_attr($_lut_id.NAME) w "site_attr_update_win $_idx"
}

proc site_attr_lut_unlink_external {_idx} {
	global site_attr

	trace vdelete site_attr($site_attr($_idx.EXTERNAL_LUT)) w "site_attr_lut_update_external $_idx"
	trace vdelete site_attr($site_attr($_idx.EXTERNAL_LUT)) u "site_attr_lut_local $_idx"

	catch {$site_attr($_idx.EXTERNAL_LUT_BUTTON) configure -text "No prefs" -command ""}
	trace vdelete site_attr($site_attr($_idx.EXTERNAL_LUT).NAME) w "site_attr_update_win $_idx"
}

proc site_attr_lut_set_external {_idx _lut_id} {
	global site_attr

	if {$site_attr($_idx.TYPE) != $site_attr($_lut_id.TYPE)} {
		tk_messageBox -icon warning -message "Thematic prefs and attribute have different types" -type ok -parent $site_attr($_idx.WIN_CMD)
		set site_attr($_idx.USE_EXTERNAL_VALUES) $site_attr($_idx.USING_EXTERNAL_VALUES)
		return
	}

	if {$site_attr($_idx.USING_EXTERNAL_VALUES) == 0} {
		# external lut, was local
		set site_attr($_idx.USING_EXTERNAL_VALUES) 1
		site_attr_lut_link_external $_idx $_lut_id
	} else {
		if {$site_attr($_idx.EXTERNAL_LUT) != $_lut_id} {
			# external lut has changed
			site_attr_lut_unlink_external $_idx
			site_attr_lut_link_external $_idx $_lut_id
		}
	}

	site_attr_lut_update_external $_idx
}

proc site_attr_lut_update_external {_idx {_var ""} {_index ""} {_op ""}} {
	global site_attr

	set lut_id $site_attr($_idx.EXTERNAL_LUT)

	if {[info exists site_attr($_idx.XLIST)]} {set xlist $site_attr($_idx.XLIST)}
	if {[info exists site_attr($_idx.YLIST)]} {set ylist $site_attr($_idx.YLIST)}

	set site_attr($_idx.XLIST) $site_attr($lut_id.XLIST)
	set site_attr($_idx.YLIST) $site_attr($lut_id.YLIST)

	switch $site_attr($_idx.ATTR) {
		"size" {
			site_attr_update_entries $_idx
		}
		"color" {
			site_attr_update_colors $_idx
			site_attr_update_gui_colors $_idx
		}
	}

	if {[info exists xlist]} {set site_attr($_idx.XLIST) $xlist} else {set site_attr($_idx.XLIST) [list]}
	if {[info exists ylist]} {set site_attr($_idx.YLIST) $ylist} else {set site_attr($_idx.YLIST) [list]}
}


# "Import" button
proc site_attr_lut_import {_idx} {
	global site_attr

	switch [llength $site_attr(LUT_LIST)] {
		0		{tk_messageBox -icon warning -message "No prefs file loaded" -type ok -parent $site_attr($_idx.WIN_CMD)}
		1		{site_attr_lut_import_attr $_idx [lindex $site_attr(LUT_LIST) 0]}
		default	{site_attr_lut_popup $_idx site_attr_lut_import_attr}
	}
}

proc site_attr_lut_import_attr {_idx _lut_id} {
	global site_attr

	if {$site_attr($_idx.TYPE) != $site_attr($_lut_id.TYPE)} {
		tk_messageBox -icon warning -message "Thematic prefs and attributes have different types" -type ok -parent $site_attr($_idx.WIN_CMD)
		return
	}

	set site_attr($_idx.VALUES_LIST) [concat $site_attr($_idx.VALUES_LIST) $site_attr($_lut_id.XLIST)]
	if { $site_attr($_idx.TYPE) != "s"} {
		set site_attr($_idx.VALUES_LIST) [lsort -real -unique $site_attr($_idx.VALUES_LIST)]
	} else {
		set site_attr($_idx.VALUES_LIST) [lsort -unique $site_attr($_idx.VALUES_LIST)]
	}

	set site_attr($_idx.XLIST) $site_attr($_lut_id.XLIST)
	set site_attr($_idx.YLIST) $site_attr($_lut_id.YLIST)

	set site_attr($_idx.USE_EXTERNAL_VALUES) 0
	set site_attr($_idx.USING_EXTERNAL_VALUES) 0
	set site_attr($_idx.ALREADY_SET) 1

	switch $site_attr($_idx.ATTR) {
		"size" {
			site_attr_create_lut_win $_idx
			site_attr_update_entries $_idx
		}
		"color" {
			site_attr_create_lut_win $_idx
			site_attr_update_colors $_idx
		}
	}
}


# "Export" button
proc site_attr_lut_export {_idx} {
	global site_attr

	if {![info exists site_attr($_idx.XLIST)]} {return}

	switch [llength $site_attr(LUT_LIST)] {
		0		{site_attr_lut_export_attr $_idx -1}
		default	{site_attr_lut_popup $_idx site_attr_lut_export_attr -1}
	}
}

proc site_attr_lut_export_attr {_idx _lut_id} {
	global site_attr

	if {$_lut_id == -1} {
		site_attr_lut_create_win [site_attr_lut_fill [site_attr_lut_create] $_idx]
	} else {
		set site_attr($_lut_id.NAME) $_lut_id
		site_attr_lut_create_win [site_attr_lut_fill $_lut_id $_idx]
	}
}


# Popup menu when more than a lut is present
proc site_attr_lut_popup {_idx _command {_new "no"}} {
	global site_attr
	set site_attr(POPUP) 0

	set m ".site_attr_lut_popup"
	catch {destroy $m}
	menu $m -tearoff 0

	if {$_new != "no" } {$m add command -label "New" -command "$_command $_idx $_new"}

	foreach lut_id $site_attr(LUT_LIST) {
		$m add command -label "$site_attr($lut_id.NAME)" -command "$_command $_idx $lut_id"
	}

	tk_popup $m $site_attr(MOUSE_X) $site_attr(MOUSE_Y)
	return $m
}


# these functions are needed for "external" radiobutton when no choice is done
#   by clicking outside the menu (radiobutton must be reset)
proc site_attr_lut_popup_enter {} {global site_attr; set site_attr(POPUP) 1}
proc site_attr_lut_popup_leave {} {global site_attr; set site_attr(POPUP) 0}

proc site_attr_lut_popup_unmap {_idx} {
	global site_attr
	if {$site_attr(POPUP) == 0} {
		# Mouse outside menu: no choice done so do not change flag
		set site_attr($_idx.USE_EXTERNAL_VALUES) $site_attr($_idx.USING_EXTERNAL_VALUES)
	}
}

proc site_attr_pick_mouse_XY {_x _y} {
	global site_attr
	set site_attr(MOUSE_X) $_x
	set site_attr(MOUSE_Y) $_y
}


### BEGIN LUT basic management #################################################

proc site_attr_lut_create {} {
	global site_attr

	set lut_id "NVIZ_pttheme$site_attr(LUT_NUMBER)"
	incr site_attr(LUT_NUMBER)

	lappend site_attr(LUT_LIST) $lut_id

	# items will be bond to this variable for update
	set site_attr($lut_id) 0

	# In case no name is given: items that show this name will be bond to this variable
	set site_attr($lut_id.NAME) $lut_id

	# Add to menu
	catch {site_attr_lut_menu_add $lut_id}

	return $lut_id
}

proc site_attr_lut_destroy {_lut_id} {
	global site_attr

	if {[remove_from_list $_lut_id site_attr(LUT_LIST)] == 0} {return}
	incr site_attr(LUT_NUMBER) -1

	# Remove from menu
	site_attr_lut_menu_delete $_lut_id

	# items bond to this variable are notified
	unset site_attr($_lut_id)

	# unset lists
	unset site_attr($_lut_id.XLIST)
	unset site_attr($_lut_id.YLIST)
	unset site_attr($_lut_id.VALUES_LIST)
	if {$site_attr($_lut_id.ATTR) == "color"} {
		unset site_attr($_lut_id.LUT_LIST)
	}

	# unset all the rest
	unset site_attr($_lut_id.NAME)
	unset site_attr($_lut_id.ATTR)
	unset site_attr($_lut_id.TYPE)

	set win ".lut_win$_lut_id"
	catch {destroy $win}
	return
}

proc site_attr_lut_fill {_lut_id _idx} {
	global site_attr

	set site_attr($_lut_id.ATTR) $site_attr($_idx.ATTR)
	set site_attr($_lut_id.TYPE) $site_attr($_idx.TYPE)

	set site_attr($_lut_id.XLIST) $site_attr($_idx.XLIST)
	set site_attr($_lut_id.YLIST) $site_attr($_idx.YLIST)
	set site_attr($_lut_id.VALUES_LIST) $site_attr($_idx.VALUES_LIST)
	if {$site_attr($_lut_id.ATTR) == "color"} {
		set site_attr($_lut_id.LUT_LIST) $site_attr($_idx.LUT_LIST)
	}

	# LUT has changed, so must be notified to all idx are using it
	incr site_attr($_lut_id)

	return $_lut_id
}

proc site_attr_lut_id_from_name {_lut_name} {
	global site_attr

	if {![info exists site_attr(LUT_LIST)]} {return "NO_LUT"}

	foreach lut_id $site_attr(LUT_LIST) {
		if {$site_attr($lut_id.NAME) == $_lut_name} {return $lut_id}
	}
	return "NO_LUT"
}

### BEGIN LUT window ###########################################################

# Open window
proc site_attr_lut_open_win {_lut_id} {
	set win ".lut_win$_lut_id"
	if {[winfo exists $win]} {raise $win; return}

	site_attr_lut_create_win $_lut_id
}

# Create window for LUT
proc site_attr_lut_create_win {_lut_id} {
	global site_attr
	global nviztxtfont
	if {![info exists site_attr($_lut_id.ATTR)]} {return}

	switch $site_attr($_lut_id.ATTR) {
		"size" {
			set row_val "label \$w.l\$row  -text \$val -relief sunken -width 6 -justify right"
		}
		"color" {
			set row_val "label \$w.l\$row -bg \$val -width 3 -borderwidth 1 -relief raised"
		}
		default {
			puts "WARNING: No attribute behaviour for $site_attr($_lut_id.ATTR)!"
			return
		}
	}

	set win ".lut_win$_lut_id"

	catch {destroy $win}
	toplevel $win
	wm resizable $win true true
	wm title $win "Thematic prefs file $_lut_id"
	bind $win <Destroy> "site_attr_lut_destroy_win $_lut_id $win %W"

	set w [frame $win.left]
		label $w.name -text "Name: $site_attr($_lut_id.NAME)" -pady 2
		pack $w.name -pady 1 -fill x -side top

		if {$site_attr($_lut_id.TYPE) == "s"} {
			set type "string"
		} else {
			set type "numeric"
		}
		label $w.type -text "Type: $type" -pady 2
		pack $w.type -pady 1 -padx 0 -fill x -side top

		frame $w.buttons
		Button $w.buttons.save -text "Save" -bd 1 -width 6 \
				-command "site_attr_lut_save $_lut_id" \
				-helptext "Save thematic prefs in file"
		Button $w.buttons.delete -text "Clear" -bd 1 -width 6 \
				-command "site_attr_lut_destroy $_lut_id" \
				-helptext "Clear current thematic prefs"
		
		pack $w.buttons.save -side left -expand 0 -fill x
		pack $w.buttons.delete -side right -expand 0 -fill x
		pack $w.buttons -side top -expand 1 -fill both -pady 5

	set w [frame $w.f]
		set row 0
		foreach elt $site_attr($_lut_id.XLIST) val $site_attr($_lut_id.YLIST) {
			label $w.n$row -text "$row" -relief groove -borderwidth 1 \
				-fg black -padx 4 -anchor e
			grid $w.n$row	-row $row -column 0 -columnspan 1 -sticky nswe
			label $w.v$row -text "$elt" -relief groove -borderwidth 1 \
				-fg black -font $nviztxtfont -padx 3 -anchor w
			grid $w.v$row	-row $row -column 1 -columnspan 1 -sticky nswe
			eval $row_val
			grid $w.l$row	-row $row -column 2 -columnspan 1 -sticky nswe

			incr row
		}
	pack $w -pady 1 -padx 0 -side top

	pack $win.left -side left -pady 2 -padx 3

	if {$site_attr($_lut_id.ATTR) == "color"} {
		set w $win.right
		lut_create_canvas $w $site_attr($_lut_id.VALUES_LIST) $site_attr($_lut_id.LUT_LIST) 300 140 260 40 7 v
		pack $w -side left -padx 3
	}

	trace variable site_attr($_lut_id.NAME) w "site_attr_lut_update_win $win.left.name"
}

# bond to site_attr($_lut_id.NAME) changes
proc site_attr_lut_update_win {_win var index op} {
	upvar $var v
	$_win configure -text $v($index)
}

proc site_attr_lut_destroy_win {_lut_id _win _w} {
	global site_attr
	if {$_win != $_w} {return}
	trace vdelete site_attr($_lut_id.NAME) w "site_attr_lut_update_win $_win.left.name"
}


### LUT menu in main interface #################################################

# menu in main interface
proc site_attr_lut_menu {_win} {
	global site_attr

	set m $_win.lut_menu
	catch {destroy $m}
	menubutton $m -menu $m.m -relief raised -indicatoron 1 -bd 1 -text "Load Thematic Prefs"
	bind $m <Destroy> "site_attr_lut_menu_destroy %W"

	menu $m.m -tearoff 0
	$m.m add command -label "Load file" -command "site_attr_lut_create_win \[site_attr_lut_load\]"

	foreach lut_id $site_attr(LUT_LIST) {
		site_attr_lut_menu_add $lut_id
#		$m.m add command -label "$site_attr($lut_id.NAME)" -command "site_attr_lut_open_win $lut_id"
	}
	return $m
}

proc site_attr_lut_menu_add {_lut_id} {
	global site_attr

	if {![winfo exists $site_attr(LUT_MENUBUTTON)]} {return}

	$site_attr(LUT_MENUBUTTON).m add command -label "$site_attr($_lut_id.NAME)" \
		-command "site_attr_lut_open_win $_lut_id"
	trace variable site_attr($_lut_id.NAME) w site_attr_lut_menu_update
}

proc site_attr_lut_menu_delete {_lut_id} {
	global site_attr

	if {![winfo exists $site_attr(LUT_MENUBUTTON)]} {return}

	$site_attr(LUT_MENUBUTTON).m delete [site_attr_lut_menu_index $_lut_id]
	trace vdelete site_attr($_lut_id.NAME) w site_attr_lut_menu_update
}

proc site_attr_lut_menu_update {var index op} {
	global site_attr

	if {![winfo exists $site_attr(LUT_MENUBUTTON)]} {return}

	$site_attr(LUT_MENUBUTTON).m delete 1 [$site_attr(LUT_MENUBUTTON).m index last]

	foreach lut_id $site_attr(LUT_LIST) {
		$site_attr(LUT_MENUBUTTON).m add command -label "$site_attr($lut_id.NAME)" -command "site_attr_lut_open_win $lut_id"
	}
}

proc site_attr_lut_menu_destroy {_w} {
	global site_attr

	if {$site_attr(LUT_MENUBUTTON) != $_w} {return}

	foreach lut_id $site_attr(LUT_LIST) {
		trace vdelete site_attr($lut_id.NAME) w site_attr_lut_menu_update
	}
}

proc site_attr_lut_menu_index {_lut_id} {
	global site_attr

	set n [$site_attr(LUT_MENUBUTTON).m index last]

	for {set i 1} {$i <= $n} {incr i} {
		if {$site_attr($_lut_id.NAME) == [$site_attr(LUT_MENUBUTTON).m entrycget $i -label]} {
			return $i
		}
	}
	return 0
}

#proc site_attr_lut_menu_configure {_lut_id} {
#	global site_attr

#	set index [site_attr_lut_menu_index $_lut_id]

#	if {$index > 0} {
#		$site_attr(LUT_MENUBUTTON).m entrycget $index -label $site_attr($_lut_id.NAME)
#	}
#}


### LUT file management ########################################################

# TO file
proc site_attr_lut_save {_lut_id} {
	global site_attr

	set filename [tk_getSaveFile -initialdir $site_attr(LUT_DIR) -initialfile $site_attr($_lut_id.NAME) -filetypes $site_attr(LUT_FILETYPES) -title "Save thematic prefs"]

	if {$filename != ""} {
		if {[file exists $filename]} {
			file delete -force -- $filename
			puts "Old LUT \"$filename\" deleted"
		}
		# file is new
		site_attr_lut_write $_lut_id [site_attr_lut_file_from_name $filename]
	}
}

proc site_attr_lut_write {_lut_id _filename} {
	global site_attr

	set name [site_attr_lut_file_from_name $_filename]

	set fileId [open $name "WRONLY CREAT" 0777]

	puts $fileId "ATTR $site_attr(LUT_SEP) $site_attr($_lut_id.ATTR)"
	puts $fileId "TYPE $site_attr(LUT_SEP) $site_attr($_lut_id.TYPE)"
	puts $fileId "XLIST $site_attr(LUT_SEP) $site_attr($_lut_id.XLIST)"
	puts $fileId "YLIST $site_attr(LUT_SEP) $site_attr($_lut_id.YLIST)"
	puts $fileId "VLIST $site_attr(LUT_SEP) $site_attr($_lut_id.VALUES_LIST)"
	if {$site_attr($_lut_id.ATTR) == "color"} {
		puts $fileId "LLIST $site_attr(LUT_SEP) $site_attr($_lut_id.LUT_LIST)"
	}
	close $fileId

	set site_attr($_lut_id.NAME) [site_attr_lut_name_from_file $_filename]
	puts "Thematic preferences file \"$name\" saved"
}


# FROM file
proc site_attr_lut_load {{filename ""}} {
	global site_attr

	if {$filename == ""} {
		set filename [tk_getOpenFile -initialdir $site_attr(LUT_DIR) -filetypes $site_attr(LUT_FILETYPES) -title "Load thematic prefs"]
	}

	if {$filename == ""} {return}

	if {![file exists $filename]} {
		puts "*** WARNING *** File $filename is unavailable"
		return ""
	}

	if {[site_attr_lut_read $filename] < 5} {
		puts "*** ERROR *** Some thematic pref component are missing in file \"$filename\""
		return ""
	}

	set lut_id [site_attr_lut_fill [site_attr_lut_create] "LUT_FILE"]
	set site_attr($lut_id.NAME) [site_attr_lut_name_from_file $filename]
	# site_attr_lut_create_win $lut_id

	return $lut_id
}

proc site_attr_lut_read {_filename} {
	global site_attr

	catch {unset site_attr(LUT_FILE.ATTR)}
	catch {unset site_attr(LUT_FILE.XLIST)}
	catch {unset site_attr(LUT_FILE.YLIST)}
	catch {unset site_attr(LUT_FILE.VALUES_LIST)}
	catch {unset site_attr(LUT_FILE.LUT_LIST)}

	# default type is string
	set site_attr(LUT_FILE.TYPE) "s"

	set ret_val 0

	set fileId [open $_filename "RDONLY"]

	while {[gets $fileId row] >= 0} {

		set index [string first $site_attr(LUT_SEP) $row]

		# skip line if separator not found
		if {$index <= 0} {continue}

		set tag_name [string trim [string range $row 0 [expr $index - 1]]]
		set tag_list [string trim [string range $row [expr $index + [string length $site_attr(LUT_SEP)]] end]]

		# every switch tests if already read, so only first value will be taken in case of multiple definition
		switch $tag_name {
			"ATTR" {
				if {![info exists site_attr(LUT_FILE.ATTR)]} {
					set site_attr(LUT_FILE.ATTR) $tag_list
					incr ret_val
				}
			}
			"TYPE" {
					set site_attr(LUT_FILE.TYPE) $tag_list
			}
			"XLIST" {
				if {![info exists site_attr(LUT_FILE.XLIST)]} {
					set site_attr(LUT_FILE.XLIST) [eval list $tag_list]
					incr ret_val
				}
			}
			"YLIST" {
				if {![info exists site_attr(LUT_FILE.YLIST)]} {
					set site_attr(LUT_FILE.YLIST) [eval list $tag_list]
					incr ret_val
				}
			}
			"VLIST" {
				if {![info exists site_attr(LUT_FILE.VALUES_LIST)]} {
					set site_attr(LUT_FILE.VALUES_LIST) [eval list $tag_list]
					incr ret_val
				}
			}
			"LLIST" {
				if {![info exists site_attr(LUT_FILE.LUT_LIST)]} {
					set site_attr(LUT_FILE.LUT_LIST) [eval list $tag_list]
					incr ret_val
				}
			}
			default {
				puts "WARNING: Unknown Tag \"$tag_name\" in file \"$_filename\""
			}
		}
#		foreach elt $tag_list {puts "*** $elt ***"}
	}

	close $fileId

	if {[info exists site_attr(LUT_FILE.ATTR)]} {
		if {$site_attr(LUT_FILE.ATTR) != "color"} {
			# LLIST shouldn't have been found
			incr ret_val
		}
		return $ret_val
	} else {
		return -1
	}
}


### LUT Utilities ##############################################################

proc site_attr_lut_name_from_file {_filename} {
	global site_attr

	set name [lindex [file split $_filename] end]
	set index [string first ".$site_attr(LUT_EXT)" $name]
	return [string range $name 0 [expr $index - 1]]
}

proc site_attr_lut_file_from_name {_filename} {
	global site_attr

	set index [string first ".$site_attr(LUT_EXT)" $_filename]
	if {$index == -1} {
		return $_filename.$site_attr(LUT_EXT)
	} else {
		return $_filename
	}
}

proc site_attr_lut_id {_index _postfix} {
	return [string range $_index 0 [expr [string first ".$_postfix" $_index] - 1]]
}


################################################################################
################################################################################
################################################################################
# These two functions are called in order to open (set_link_attr) a site
#   directly with an external LUT and get (get_link_addr) the list of
#   field_index and lut_name of a given site/attr
#
# _curr_site is the site id
# _attr is "color" or "size"
# _which is "name" or "index" and indicates if field is name or index
# _field name or index
#		 as a name must match EXACTLY (Case Sensitive) a field name into the database
# _lut_name is the name of the file that contains the LUT
#
################################################################################
################################################################################
################################################################################
proc site_attr_set_link_attr {_curr_site _attr _which _field _lut_name} {
	global site_attr

	switch $_attr {
		"size" {set st_att [Nsite_attr_get_value "ST_ATT_SIZE"]}
		"color" {set st_att [Nsite_attr_get_value "ST_ATT_COLOR"]}
		"marker" {set st_att [Nsite_attr_get_value "ST_ATT_MARKER"]}
		default {puts "WARNING: Unknown attribute $_attr!"; return $ret_list}
	}

	set max [Nsite_attr_get_value "GPT_MAX_ATTR"]
	set map [Nsite$_curr_site get_att map]

	# find index and type
	set not_found 1; set index 0
	if {$_which == "name"} {
		foreach {n t} [Nsite_attr_get_fields_name_and_type $map] {
			lappend site_attr(FIELD_NAMES) $n; lappend site_attr(FIELD_TYPES) $t
			if {$n == $_field} {set not_found 0; set found_index $index; set found_name $n; set found_type $t}
			incr index
		}
	} else {
		foreach {n t} [Nsite_attr_get_fields_name_and_type $map] {
			lappend site_attr(FIELD_NAMES) $n; lappend site_attr(FIELD_TYPES) $t
			if {$index == $_field} {set not_found 0; set found_index $index; set found_name $n; set found_type $t}
			incr index
		}
	}
	if {$not_found} {puts "WARNING: field $_field NOT FOUND"; return}

	# verify if lut with this name is already loaded
	set lut_id [site_attr_lut_id_from_name [site_attr_lut_name_from_file $_lut_name]]
	if {$lut_id == "NO_LUT"} {
		# if not, load it!
		set lut_id [site_attr_lut_load [file join $site_attr(LUT_DIR) [site_attr_lut_file_from_name $_lut_name]]]
		if {$lut_id == ""} {puts "WARNING: lut file [site_attr_lut_file_from_name $_lut_name] NOT FOUND"; return}
	}

	# FROM HERE ON EVERITHING SHOULD BE OK

	# chck if field already present
	if {[info exists site_attr($_attr$found_index)]} {
		# already present
		set row $site_attr($_attr$found_index)
		set idx [site_attr_idx Nsite$_curr_site $_attr $row]
	} else {
		# we have to find the first available row
		for {set nattr 0} {$nattr < $max} {incr nattr} {
			# element in attr array is available
			if {[expr [Nsite$_curr_site get_att useatt $nattr] & $st_att] == 0} {break}
		}

		# force at least one element (the last) to be used
		if {$nattr >= $max} {set nattr $max}

		set row [expr $nattr + $site_attr(FIRST_ROW)]
		set idx [site_attr_idx Nsite$_curr_site $_attr $row]

		# for future test if using the same
		set site_attr($_attr$found_index) $row
	}

	set site_attr($idx.INDEX) $found_index
	set site_attr($idx.TYPE) $found_type
	set site_attr($idx.ATTR) $_attr
	set site_attr($idx.MAP) Nsite$_curr_site
	set site_attr($idx.ROW) $row
	set site_attr($idx.NAME) $found_name
	set site_attr($idx.MENU_BG) $site_attr(MENU_DEFAULT_BG)

	# this call is useful when there is an already open window
	site_attr_set_win $idx $found_name "no"

	# set external lut
	if {![info exists site_attr($idx.USING_EXTERNAL_VALUES)]} {set site_attr($idx.USING_EXTERNAL_VALUES) 0}
	set site_attr($idx.USE_EXTERNAL_VALUES) 1
	site_attr_lut_set_external $idx $lut_id
}

proc site_attr_get_link_attr {_curr_site _attr} {
	global site_attr

	set max [Nsite_attr_get_value "GPT_MAX_ATTR"]
	set ret_list [list]

	switch $_attr {
		"size" {set st_att [Nsite_attr_get_value "ST_ATT_SIZE"]}
		"color" {set st_att [Nsite_attr_get_value "ST_ATT_COLOR"]}
		"marker" {set st_att [Nsite_attr_get_value "ST_ATT_MARKER"]}
		default {puts "WARNING: Unknown attribute $_attr!"; return $ret_list}
	}

	for {set nattr 0} {$nattr < $max} {incr nattr} {
		if {[expr [Nsite$_curr_site get_att useatt $nattr] & $st_att]!= 0} {
			# take parameters
			set row [expr $nattr + $site_attr(FIRST_ROW)]
			set idx [site_attr_idx Nsite$_curr_site $_attr $row]

			# verify that is external LUT, otherwise don't use
			if {![info exists site_attr($idx.USING_EXTERNAL_VALUES)]} {continue}
			if {$site_attr($idx.USING_EXTERNAL_VALUES) == 0} {continue}

			set field_index $site_attr($idx.INDEX)
			set lut_id $site_attr($idx.EXTERNAL_LUT)
			set lut_name $site_attr($lut_id.NAME)

			# prepare string to return with multiple field_index and lut_name
			lappend ret_list $field_index $lut_name
		}
	}
	return $ret_list
}

