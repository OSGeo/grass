##########################################################################
# 
# Panel to create legends for raster surfaces in NVIZ
#
# 4/4/95
# M. Astley
# U.S. Army Construction Engineering Research Laboratory
#
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

# Changes
#

# Panel specific globals
global Nv_

# Font Type: 
set Nv_(labelFontType) times
set Nv_(font) {"times" "12"}

# Font Weight: Italic, Bold
set Nv_(labelFontWeight1) 0
set Nv_(labelFontWeight2) 0

# Font Point Size: varies
set Nv_(labelFontSize) 12
set Nv_(labelFontColor) #000000

# Legend section
set Nv_(catval) 1
set Nv_(catlabel) 0
set Nv_(leg_invert) 0
set Nv_(leg_userange) 0
set Nv_(leg_discat) 0
set Nv_(leg_uselist) 0
set Nv_(cat_list) [list]
set Nv_(cat_list_select) 0

# Label Sites section
set Nv_(labvalues) 1
set Nv_(lablabels) 1
set Nv_(labinbox) 1

# default cursor
set Nv_(cursor) [$Nv_(TOP) cget -cursor]	

global clr

##########################################################################

proc mklegendPanel { BASE } {
    global Nv_ 

    set panel [St_create {window name size priority} $BASE "Legend" 2 5]
    frame $BASE -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Legends Panel"
    
    ##########################################################################
    # This section contains widgets for specifying a legend

    set rbase1 [frame $BASE.options]
    checkbutton $rbase1.invert -text "reverse legend" -anchor w \
		-variable Nv_(leg_invert) -onvalue 1 -offvalue 0
    checkbutton $rbase1.values -text "show values" \
		-anchor w -variable Nv_(catval) -onvalue 1 -offvalue 0
    checkbutton $rbase1.labels -text "show labels" \
		-anchor w -variable Nv_(catlabel) -onvalue 1 -offvalue 0
    Button $rbase1.font -text "Font" \
		-command "select_font $rbase1.font" -bd 1 \
		-helptext "Select font family, size, and style for legend text"
    pack $rbase1.invert $rbase1.values $rbase1.labels $rbase1.font\
		-fill none -side left -expand yes -anchor w
	pack $rbase1 -side top -expand yes -fill both -padx 3 -pady 4

    # Use-range portion of panel
	set rbase2 [frame $BASE.range]
    checkbutton $rbase2.useranges -text "set value range  " -anchor w \
		-variable Nv_(leg_userange) -onvalue 1 -offvalue 0

    LabelEntry $rbase2.entry_low -relief sunken -width 8 \
        -textvariable Nv_(leg_lorange) \
        -entrybg white -label "min "
    LabelEntry $rbase2.entry_hi  -relief sunken -width 8 \
         -textvariable Nv_(leg_hirange) \
         -entrybg white -label "   max "

    pack $rbase2.useranges $rbase2.entry_low $rbase2.entry_hi \
        	-side left -fill x -expand yes -anchor w

    # Return bindings for "use range" entries
    bind $rbase2.entry_low <Return> "$rbase2.useranges select"
    bind $rbase2.entry_hi <Return> "$rbase2.useranges select"

	pack $rbase2 -side top -expand yes -fill both -padx 3 -pady 4

	set rbase3 [frame $BASE.cats]
    # Discrete categories and use-list portion
    checkbutton $rbase3.disc_cat -text "discrete categories" \
		-anchor w -width 18 -variable Nv_(leg_discat) \
		-onvalue 1 -offvalue 0
    
    # Some special handling for the "Use List" entry. 
    # These features are not implemented. I'm leaving the code in because
    # it looks like it's almost done, but I can't tell what is intended
    # (Michael Barton - Nov 2006
#    checkbutton $rbase3.cb -text "use cat list" \
#		-anchor w -variable Nv_(leg_uselist) \
#		-onvalue 1 -offvalue 0 -command "make_cat_list $rbase3.curr.m" 
#    menubutton $rbase3.curr -text "Current list" \
#		-menu $rbase3.curr.m -relief raised \
#        -state disabled -indicatoron 1
#    menu $rbase3.curr.m -disabledforeground black
#
#    $rbase3.curr.m add command -label "None" -state disabled

    pack $rbase3.disc_cat -side left -fill none -expand yes -anchor w

	pack $rbase3 -side top -expand yes -fill both -padx 3 -pady 4

    # Legend button, invert checkbutton and category checkbuttons   
	set rbase4 [frame $BASE.buttons]
	Button $rbase4.place -text "Place legend" \
		-command "place_legend" -width 10 -bd 1 \
		-helptext "Use mouse to place legend; left button defines first corner, \
			right button defines opposite corner."
	Button $rbase4.erase -text "Erase legend" \
		-command "delete_list legend 0" -width 10 -bd 1 \
		-helptext "Erase all legends"
    button $rbase4.close -text "Close" -command "Nv_closePanel $BASE" \
		-anchor se -bd 1
	pack $rbase4.place -fill none -side left -expand no
	pack $rbase4.erase -fill none -side left -expand no -padx 3
	pack $rbase4.close -side right -fill none -expand no
	pack $rbase4 -side top -expand yes -fill both -padx 3 -pady 4

    return $panel
}

###############################################################################
# Routine to popup a list selector for selecting a discrete list of values
proc make_cat_list {MENU} {
    global Nv_

    # Check to see if we are turning this check button on
    if {$Nv_(leg_uselist) == 0} return

    # Reinitalize list values
    set Nv_(cat_list) [list]
    set Nv_(cat_list_select) 0
    $MENU delete 0 last

    # Create the "individual" subpanel
    set BASE ".cat_list"
    set pname $BASE
    toplevel $pname -relief raised -bd 3
    list_type1 $pname.list 3c 3c
    $pname.list.t configure -text "Category Values"
    entry $pname.level -relief sunken -width 10
    bind $pname.level <Return> "make_cat_list_add $BASE"
    button $pname.addb -text "Add"    -command "make_cat_list_add $BASE"
    button $pname.delb -text "Delete" -command "make_cat_list_delete $BASE"
    button $pname.done -text "Done"   -command "set Nv_(cat_list_select) 1"
    pack $pname.list $pname.level $pname.addb $pname.delb $pname.done\
		-fill x -padx 2 -pady 2

    tkwait variable Nv_(cat_list_select)
    for {set i 0} {$i < [$pname.list.l size]} {incr i} {
		set temp [$pname.list.l get $i]
		lappend Nv_(cat_list) $temp
		$MENU add command -label "$temp" -state disabled
    }

    if {[llength $Nv_(cat_list)]==0} {
		$MENU add command -label None -state disabled
    }

    destroy $BASE
}

# Two quick routines to add or delete isosurface levels for
# selecting them individually
proc make_cat_list_add { BASE } {
    # For this routine we just use the value stored in the
    # entry widget
    # Get the value from the entry widget
    set level [$BASE.level get]

    # Now just append it to the list
    $BASE.list.l insert end $level
}

proc make_cat_list_delete { BASE } {
    # For this procedure we require that the user has selected
    # a range of values in the list which we delete
    # Get the range of selections
    set range [$BASE.list.l curselection]
    
    # Now delete the entries
    foreach i $range {
	$BASE.list.l delete $i
    }
}



###############################################################################
# use Tk dialog to select fonts
proc select_font {fbutton} {
	global Nv_
	
    set fon [SelectFont $fbutton.fontset -type dialog -sampletext 1 -title "Select font"]
	if { $fon != "" } {set Nv_(font) $fon}
}

###############################################################################

# Routine to do_legend
proc do_legend {W x y flag } {
    global Nv_
    global x1 y1 x2 y2

	switch $flag {
		1 {
			#pick first corner
			set y [expr $Nv_(height) - $y]
	
			#set first corner of box
			set x1 $x
			set y1 $y
			set x2 $x
			set y2 $y
		}
		2 {
			set y [expr $Nv_(height) - $y]
			
			#set last corner of box and reset binding
			#Get name of current map 
			set name [Nget_current surf]
			if { [lindex [Nsurf$name get_att color] 0] == "const"} {
				puts "Colortable constant -- no legend available"
				
				#reset everything
				bind $W <Button-1> {}
				bind $W <Button-3> {}
				unset x1
				unset y1
				update
				return
			} 
	
			set name [lindex [Nsurf$name get_att color] 1]
		
			set range_low -9999
			set range_high -9999
			if {$Nv_(leg_userange)} {
				set range_low $Nv_(leg_lorange)
				set range_high $Nv_(leg_hirange)
				if { $range_low == ""} {set range_low -9999}
				if { $range_high == ""} {set range_high -9999}
			}
			#make sure corner 1 is picked
			if {[info exists x1]} {
			
				if {$x1 > $x} {
					set x2 $x1
					set x1 $x
				} else {
					set x2 $x
				}
				
				if {$y1 > $y} {
					set y2 $y1
					set y1 $y
				} else {
					set y2 $y
				}
				#get font description
				#create font description	
				set weight "medium"
				set slant "r"
				set style ""
				
				if {[lindex $Nv_(font) 0] != ""} {set Nv_(labelFontType) [lindex $Nv_(font) 0]}
				if {[lindex $Nv_(font) 1] != ""} {set Nv_(labelFontSize) [lindex $Nv_(font) 1]}	
				if {[lsearch $Nv_(font) "bold"] != -1} {set weight "bold"}
				if {[lsearch $Nv_(font) "italic"] != -1} {set slant "i"}
				if {[lsearch $Nv_(font) "underline"] != -1} {set style "underline"}
				if {[lsearch $Nv_(font) "overstrike"] != -1} {set style "overstrike"}
				set font "*-$Nv_(labelFontType)-$weight-$slant-normal--$Nv_(labelFontSize)-*-*-*-*-*-*-*"
			
				#Ndraw_legend Args -- filename use_vals use_labels invert use_range 
				# low_range high_range discrete colors corner_coords
				
				Ndraw_legend $name $font $Nv_(labelFontSize) $Nv_(catval) $Nv_(catlabel) $Nv_(leg_invert) $Nv_(leg_discat) \
					$Nv_(leg_userange) $range_low $range_high $x1 $x2 $y1 $y2
			
				#reset bindings
				bind $W <Button-1> {}
				bind $W <Button-3> {}
				unset x1
				unset x2
				unset y1
				unset y2
				update
			$Nv_(TOP) configure -cursor $Nv_(cursor)
			}
		}
		3 {
			if {($x1 != $x2) && ($y1 != $y2)} {
					$W delete area
					$W addtag area withtag \
							[$W create rect $x1 $y1 $x2 $y2 \
							-outline yellow -width 2]
					set $x1 $x2
					set $y1 $y2
			}
			$Nv_(TOP) configure -cursor $Nv_(cursor)
		}
	}

}

###############################################################################
#Routine to delete display list
proc delete_list { list flag } {
    global Nv_
    global labels legend
    global Nauto_draw

	if {$list == "legend"} {
		Ndelete_list legend 0
		set legend 0
	}
	if {$Nauto_draw == 1} {
		Nset_cancel 0
		Ndraw_all
	} 

}

###############################################################################
# Routine to place legend
proc place_legend { } {
    global Nv_
    global x1 y1 x2 y2
    global legend
    
 	$Nv_(TOP) configure -cursor plus

	#do bindings
	bind $Nv_(TOP).canvas <Button-1> {do_legend %W %x %y 1 }
	bind $Nv_(TOP).canvas <Button-3> {do_legend %W %x %y 2}
	##Tried binding to draw rectangle outline but unsupported with togl ??
	#bind $Nv_(TOP).canvas <B1-Motion> {drawzoom %W}
	
	#set legend to on
	set legend 1
	update
	
}


###############################################################################

# draw legend placement rectangle
proc drawzoom { W } {
    global x1 y1 x2 y2

	if {($x1 != $x2) && ($y1 != $y2)} {
			$W delete area
			$W addtag area withtag \
					[$W create rect $x1 $y1 $x2 $y2 \
					-outline yellow -width 2]
			set $x1 $x2
			set $y1 $y2
	}
}

