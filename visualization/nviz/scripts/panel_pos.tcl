##########################################################################
# 
# Panel to provide d.3d type interface for manually entering position 
# 	for view.
# 
# Original author unknown.
#
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

proc mkposPanel { BASE } {
    global Nv_ bearing_calc
    global nviztxtfont
    
    catch {destroy $BASE}
    
    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
	set panel [St_create {window name size priority} $BASE "Position" 1 5]
    } else {
	set panel $Nv_($BASE)
    }
    
    frame $BASE -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Position Panel"

#################################
# Positioning menus    
    set tmp1 [frame $BASE.top1]
    set col1 [frame $BASE.top1.col1]
    set col2 [frame $BASE.top1.col2]
    set col3 [frame $BASE.top1.col3]
    set col4 [frame $BASE.top1.col4]
    
    set c1r1 [label $col1.1 -text "          "]  
    set c1r2 [label $col1.2 -text "From (eye):"]  
	set c1r3 [label $col1.3 -text "To (surface):"]
	set c1r4 [label $col1.4 -text "          "]
	set c1r5 [label $col1.5 -text "          "]
	set c1r6 [label $col1.6 -text "Range/bearing:"]
	pack $c1r1 $c1r2 $c1r3 $c1r4 $c1r5 $c1r6 -side top -anchor e
	
	set c2r1 [label $col2.1 -text "East" -fg black -font $nviztxtfont]
	set c2r2 [entry $col2.2 -width 10 -textvariable Nv_(east1) -bg white]
	set c2r3 [entry $col2.3 -width 10 -textvariable Nv_(east2) -bg white]
	set c2r4 [label $col2.4 -text "     "]
	set c2r5 [label $col2.5 -text "Range" -fg black -font $nviztxtfont]
	set c2r6 [entry $col2.6 -width 10 -textvariable Nv_(range) -bg white]
	pack $c2r1 $c2r2 $c2r3 $c2r4 $c2r5 $c2r6 -side top
	
	set c3r1 [label $col3.1 -text "North" -fg black -font $nviztxtfont]
	set c3r2 [entry $col3.2 -width 10 -textvariable Nv_(north1) -bg white]
	set c3r3 [entry $col3.3 -width 10 -textvariable Nv_(north2) -bg white]
	set c3r4 [label $col3.4 -text "     "]
	set c3r5 [label $col3.5 -text "Bearing" -fg black -font $nviztxtfont]
	set c3r6 [entry $col3.6 -width 10 -textvariable Nv_(bearing) -bg white]
	pack $c3r1 $c3r2 $c3r3 $c3r4 $c3r5 $c3r6 -side top
	
	set c4r1 [label $col4.1 -text "Height" -fg black -font $nviztxtfont]
	set c4r2 [entry $col4.2 -width 8 -textvariable Nv_(ht1) -bg white] 
	set c4r3 [entry $col4.3 -width 8 -textvariable Nv_(ht2) -bg white]
	set c4r4 [label $col4.4 -text "     "]
	set c4r5 [label $col4.5 -text "Elev" -fg black -font $nviztxtfont]
	set c4r6 [entry $col4.6 -width 8 -textvariable Nv_(elev) -bg white]
	pack $c4r1 $c4r2 $c4r3 $c4r4 $c4r5 $c4r6 -side top

	pack $col1 $col2 $col3 $col4 -side left -padx 3
	pack $tmp1 -side top -pady 4
    
#################################
   
  # Mode setting radiobuttons
    set tmp3 [frame $BASE.top3]
    radiobutton $tmp3.r1 -text "Eye to surface" -variable "bearing_calc" -value "1" -command "catch {show_bearing}"
    radiobutton $tmp3.r2 -text "Surface to eye" -variable "bearing_calc" -value "2" -command "catch {show_bearing}"
    button $tmp3.b1 -text "Calculate" -command "catch {calc_position $bearing_calc};catch {show_bearing}" -bd 1
    pack $tmp3.r1 $tmp3.r2 $tmp3.b1 -side left -padx 3 -expand 1 -fill x
    
    pack $tmp3 -side top -fill x -expand 1 -pady 4

    
#################################
# Buttons menu       
    set tmp4 [frame $BASE.top4]
    
    button $tmp4.b1 -text "Refresh" -bd 1 \
    	-command {set from_loc [Nget_real_position 1]
			set to_loc [Nget_real_position 2]
			set Nv_(east1) [format_number [lindex $from_loc 0]]
			set Nv_(north1) [format_number [lindex $from_loc 1]]
			set Nv_(ht1) [format_number [lindex $from_loc 2]]
			
			set Nv_(east2) [format_number [lindex $to_loc 0]]
			set Nv_(north2) [format_number [lindex $to_loc 1]]
			set Nv_(ht2) [format_number [lindex $to_loc 2]]
	
			show_bearing
			}
    
    button $tmp4.b2 -text "Apply" -bd 1 \
    	-command {
			#Set To coords
			Nset_focus_real $Nv_(east2) $Nv_(north2) $Nv_(ht2)		
			#Set From coords
			Nmove_to_real $Nv_(east1) $Nv_(north1) $Nv_(ht1)
			#reset height
			Nv_setEntry $Nv_(main_BASE).midf.height.f.entry $Nv_(ht1)
			catch {Nv_floatscaleCallback $Nv_(main_BASE).midf.height e 2 null $Nv_(ht1)}
			#reset XY canvas
			change_display 1
		
			#Nquick_draw
			Ndraw_all
			}
    
    button $tmp4.b3 -text "Close" -command "Nv_closePanel $BASE" -bd 1
    	
    pack $tmp4.b1 $tmp4.b2 -side left 
    pack $tmp4.b3 -side right

    pack $tmp4 -side top -fill x -expand 1 -padx 3 -pady 4
    
    #set radiobutton
    set bearing_calc 1
    	
return $panel
   
}

########################################
# Proc format_number to format float to reasonable
# number of decimals -- max = 3
proc format_number {n} {

set num_tmp $n

if {$n == [expr int($num_tmp)] } {
	set val [format %.0f $n]
} elseif { [expr $n*10.] == [expr int($num_tmp*10.)] } {
	set val [format %.1f $n]
} elseif { [expr $n*100.] == [expr int($num_tmp*100.)] } {
	set val [format %.2f $n]
} else {
	set val [format %.3f $n]
}

return $val

}

########################################
# Proc calc_position to coordinate from
# rangle bearing and elev.
proc calc_position {flag} {
    global Nv_
    
    set RAD 0.0174532925199432958
    
    #convert range to 2D range
    set range_xy  [expr (cos($Nv_(elev)*$RAD) * $Nv_(range))]
    set zz [expr (sin($Nv_(elev)*$RAD) * $Nv_(range))]
    set xx [expr (sin($Nv_(bearing)*$RAD) * $range_xy)]
    set yy [expr (cos($Nv_(bearing)*$RAD) * $range_xy)]
    
    if {$flag == 1} {
	#Calculate new surface center from eye position
	set Nv_(east2) [format_number [expr $Nv_(east1) + $xx]]
	set Nv_(north2) [format_number [expr $Nv_(north1) + $yy]]
	#always look down
	set Nv_(ht2) [format_number [expr $Nv_(ht1) - $zz]]
    } else {
	#Calculate new eye position from surface center coord
	set Nv_(east1) [format_number [expr $Nv_(east2) + $xx]]
	set Nv_(north1) [format_number [expr $Nv_(north2) + $yy]]
	#always look up
	set Nv_(ht1) [format_number [expr $Nv_(ht2) + $zz]]
    }
}

########################################
# Proc show_bearing to calculate and show
# current range and bearing
proc show_bearing {} {
    global Nv_ bearing_calc

    set RAD 0.0174532925199432958

    if {$bearing_calc == 1} {
	    set xx [expr $Nv_(east2) - $Nv_(east1)]
	    set yy [expr $Nv_(north2) - $Nv_(north1)]
	    set zz [expr $Nv_(ht2) - $Nv_(ht1)]
    } else {
	    set xx [expr $Nv_(east1) - $Nv_(east2)]
	    set yy [expr $Nv_(north1) - $Nv_(north2)]
	    set zz [expr $Nv_(ht1) - $Nv_(ht2)]
    }
    
    set Nv_(range) [format_number [expr sqrt( ($xx*$xx) + ($yy*$yy) + ($zz*$zz) )]]
    set Nv_(elev) [format_number [expr sinh(abs($zz)/$Nv_(range))/$RAD ]]
    
    if {$yy == 0. && $xx == 0.} {
	    set bear_tmp 0.
    } elseif {$yy == 0.} {
	    set bear_tmp 90.
    } elseif {$xx == 0.} {
	    set bear_tmp 0.
    } else {
	    set bear_tmp [expr atan(abs($xx)/abs($yy)) / $RAD]
    }
    if {$xx >= 0. && $yy > 0.} {
	    set Nv_(bearing) [format_number $bear_tmp]
    } elseif {$xx > 0. && $yy <= 0.} {
	    set Nv_(bearing) [format_number [expr 180. - $bear_tmp]]
    } elseif {$xx <= 0. && $yy < 0.} {
	    set Nv_(bearing) [format_number [expr $bear_tmp + 180.]]
    } elseif {$xx < 0. && $yy >= 0.} {
	    set Nv_(bearing) [format_number [expr 360. - $bear_tmp]]
    } else {
	    set Nv_(bearing) 999
    }
	
}
