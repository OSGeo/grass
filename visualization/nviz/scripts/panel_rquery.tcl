##########################################################################
#
# Routines for raster querying for NVIZ
# 
# Original author unknown.
# Probably U.S. Army Construction Engineering Research Laboratory
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
# Default Priority for this panel
# 
# priority is from 0 to 10
#  the lower the number, the quicker it will be bumped
#  10 cannot be bumped
#  Panels will be loaded by the greater of 5 or their current priority
############################################################################
# procedure to make main control area

global WhatsHere Nv
set WhatsHere(first) 1

# Globals to track what's here attributes
set Nv_(what_mapname)            1
set Nv_(what_easting)            1
set Nv_(what_northing)           1
set Nv_(what_elevation)          1
set Nv_(what_colorcat)           1
set Nv_(what_xydiff)             0
set Nv_(what_xyzdiff)            0
set Nv_(what_surfdist)           1
set Nv_(what_exagsurfdist)       1
set Nv_(what_pipe)               0
set Nv_(what_pipe_text)          "Send results to: (no file selected)"

proc mkqueryPanel { BASE } {
    
    global WhatsHere
    global Nv_
    
    catch {destroy $BASE}
    
    # Initialize panel info
    if [catch {set Nv_($BASE)}] {
	set panel [St_create {window name size priority} $BASE "Raster Query" 1 5]
    } else {
	set panel $Nv_($BASE)
    }
    
    frame $BASE  -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Raster Query Panel"
    
    # Create frame, buttons, and attributes menu
    frame $BASE.bf -relief flat -borderwidth 0
    checkbutton $BASE.bf.what -text "query on/off" \
		-command whats_here -variable WhatsHere(on)
    Button $BASE.bf.separate -text Reset -command do_separate \
    	-bd 1 -width 6 -helptext "Reset most recent query"
    Button $BASE.bf.clear -text Clear -command clear_text \
    	-bd 1 -width 6 -helptext "Clear all queries"
    menubutton $BASE.bf.atts -text Attributes \
    	-menu $BASE.bf.atts.m -relief raised \
    	-indicatoron 1 -bd 1
    menu $BASE.bf.atts.m
    pack $BASE.bf.clear $BASE.bf.separate -side right -expand 0 -fill none
    pack $BASE.bf.what -side left -expand 0 -fill x
    pack $BASE.bf.atts -side left -expand 0 -fill x -padx 3
    
    # Add menu entries for menu
    set theMenu $BASE.bf.atts.m
    foreach i {{"Map name" "mapname" 0} {"Easting" "easting" 0} {"Northing" "northing" 0} \
		   {"Elevation" "elevation" 1} {"Category of color map" "colorcat" 0} \
		   {"XY dist from prev" "xydiff" 0} \
		   {"XYZ dist from prev" "xyzdiff" 2} {"Dist along surface" "surfdist" 0} \
		   {"Dist along exag surface" "exagsurfdist" 5}} {

		$theMenu add checkbutton -label [lindex $i 0] -underline [lindex $i 2] \
			-offvalue 0 -onvalue 1 -variable Nv_(what_[lindex $i 1])
    }

    pack $BASE.bf -side top -fill x -padx 3 -pady 5
    
    # frrame for close button and saving output to a file
    frame $BASE.cf
    Button $BASE.cf.close -text Close -command "Nv_closePanel $BASE" \
    	-bd 1 -width 6
    
    Button $BASE.cf.output -textvariable Nv_(what_pipe_text) \
		-command "whats_pipe_bind $BASE" -bd 1 \
		-helptext "Select file to receive all future query results"

    pack $BASE.cf.close $BASE.cf.output -side right -expand 0 -fill none
    pack $BASE.cf.output -side left
    pack $BASE.cf -side bottom -fill x -padx 3 -pady 5
    
    text $BASE.text -wrap word -relief sunken -bd 2 \
		-yscrollcommand "$BASE.yscroll set"  \
		-width 40 -height 10
    
    set WhatsHere(text) $BASE.text
    
    # scrollbar $BASE.xscroll -orient horizontal -relief flat 
    # -activebackground gray80 -command "$BASE.text view"
    
    scrollbar $BASE.yscroll -orient vertical -relief flat \
		-command "$BASE.text yview" -activebackground gray80 
    
    pack $BASE.yscroll -side right -fill y
    # pack $BASE.xscroll -side bottom -fill x
    pack $BASE.text -expand yes -fill both
    
    return $panel
}

proc whats_pipe_bind {BASE} {
    global Nv_

    # Allow the user to set a file for piping the output 
    set new_file [create_file_browser .whats_file 1 0]
    if {$new_file == -1} then return

    set Nv_(what_pipe) $new_file
    set Nv_(what_pipe_text) "Send results to: $new_file"
}

proc whats_here {} {
    global WhatsHere Nv_
    
    if {$WhatsHere(on)} {
	bind $Nv_(TOP).canvas <Button> {whats_here_info %x %y }
    } else {
	bind $Nv_(TOP).canvas <Button> {}	
    }
}

proc whats_here_info {x y} {
    global WhatsHere
    global Nv_
    
    set y [expr $Nv_(height) - $y]

#puts "DEBUG $x $y"
    
    set text $WhatsHere(text)
    set tot_out ""

    set list [Nget_point_on_surf $x $y]
    if {[llength $list] < 4} {
	$text insert end "Point not on surface\n"
	append tot_out "Point not on surface\n"
	$text yview -pickplace end
	return
    }
    
    set x [lindex $list 0]
    set y [lindex $list 1]
    set z [lindex $list 2]
    set id [lindex $list 3]
    
    if {$Nv_(what_easting)} then {
	set str [format "Easting: %15.4f\n" $x]
	$text insert end "$str"
	append tot_out "$str"
	$text yview -pickplace end
    }

    if {$Nv_(what_northing)} then {
	set str [format "Northing: %15.4f\n" $y]
	$text insert end "$str"
	append tot_out "$str"
	$text yview -pickplace end
    }

    if {$Nv_(what_elevation)} then {
	set str [format "Elevation: %15.4f\n" $z]
	$text insert end "$str"
	append tot_out "$str"
	$text yview -pickplace end
    }
    
    if {$Nv_(what_mapname)} then {
	set str [Nget_map_name [string range $id 5 end] surf]
	$text insert end "Surf map: $str"
	append tot_out "Surf map: $str"
	set str [Nget_cat_at_xy $id topo $x $y]
	$text insert end "\t$str\n"
	append tot_out "\t$str\n"
	set str [Nget_val_at_xy $id topo $x $y]
	$text insert end "\t$str\n"
	append tot_out "\t$str\n"
	$text yview -pickplace end
    }

    if {$Nv_(what_colorcat)} then {
	set map_name [$id get_att color]
	if {[lindex $map_name 0] == "map"} then {
	    set str [lindex $map_name 1]
	} else {
	    set str "constant"
	}
	$text insert end "Color map: $str"
	append tot_out "Color map: $str"
	set str [Nget_cat_at_xy $id color $x $y]
	$text insert end "\t$str\n"
	append tot_out "\t$str\n"
	set str [Nget_val_at_xy $id color $x $y]
	$text insert end "\t$str\n"
	append tot_out "\t$str\n"
	$text yview -pickplace end
    }

    if {$WhatsHere(first) == 0} {
	set px $WhatsHere(px)
	set py $WhatsHere(py)
	set pz $WhatsHere(pz)

### NO-NO Change to use lib functions for distance!
	
	if {$Nv_(what_xydiff)} then {
	    set val [expr sqrt(($px-$x)*($px-$x)+($py-$y)*($py-$y))]
	    set str [format "XY distance from previous: \t%15.4f\n" $val]
	    $text insert end "$str"
	    append tot_out "$str"
	    $text yview -pickplace end
	}

	if {$Nv_(what_xyzdiff)} then {
	    set val [expr sqrt(($px-$x)*($px-$x)+($py-$y)*($py-$y)+($pz-$z)*($pz-$z))]
	    set str [format "XYZ distance from previous: \t%15.4f\n" $val]
	    $text insert end "$str"
	    append tot_out "$str"
	    $text yview -pickplace end
	}
	
	if {$WhatsHere(pid) == $id} {
	    if {$Nv_(what_surfdist)} then {
		set dist [Nget_dist_along_surf $id $x $y $px $py 0]
		set str [format "Distance along surface: \t%15.4f\n" $dist]
		$text insert end "$str"
		append tot_out "$str"
		$text yview -pickplace end
		Nset_draw front
		Ndraw_line_on_surf $id $x $y $px $py
		Nset_draw back
	    }

	    if {$Nv_(what_exagsurfdist)} then {
		set dist [Nget_dist_along_surf $id $x $y $px $py 1]
		set str [format "Distance along exag. surface:%15.4f\n" $dist]
		$text insert end "$str"
		append tot_out "$str"
		$text yview -pickplace end
		Nset_draw front
		Ndraw_line_on_surf $id $x $y $px $py
		Nset_draw back
	    }
	    
	}
    }

    $text insert end "\n"

    set WhatsHere(px) $x
    set WhatsHere(py) $y
    set WhatsHere(pz) $z
    set WhatsHere(pid) $id
    set WhatsHere(first) 0
    
    Nset_draw front
    Ndraw_X $id $x $y
    Nset_draw back

    if {$Nv_(what_pipe) != 0} {
	set out_file [open $Nv_(what_pipe) a]
	puts $out_file $tot_out
	close $out_file
    }
}

proc do_separate {} {
    global WhatsHere Nv_
    
    set text $WhatsHere(text)
    
    $text insert end "___________________________________\n"
    $text yview -pickplace end

    if {$Nv_(what_pipe) != 0} {
	set out_file [open $Nv_(what_pipe) a]
	puts $out_file "___________________________________"
	close $out_file
    }

    set WhatsHere(first) 1
}

proc clear_text {} {
    global WhatsHere
    
    set text $WhatsHere(text)
    
    $text delete 1.0 end
    $text yview -pickplace end
}

