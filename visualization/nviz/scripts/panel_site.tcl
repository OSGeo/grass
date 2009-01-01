##########################################################################
#
# Routines for displaying and thematic mapping of vector points in NVIZ
# 
# Original author unknown.
# Probably U.S. Army Construction Engineering Research Laboratory
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
# Default Priority for this panel
# 
# priority is from 0 to 10
#  the lower the number, the quicker it will be bumped
#  10 cannot be bumped
#  Panels will be loaded by the greater of 5 or their current priority

##########################################################################

set Nv_(SiteAttr) 1
if {$Nv_(SiteAttr)} {source $src_boot/etc/nviz2.2/scripts/site_attr.tcl}


############################################################################
# procedure to make main control area

proc mksitePanel { BASE } {
    global Nv_
	global nviztxtfont
	global site_attr
    
    catch {destroy $BASE}
    
    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
		set panel [St_create {window name size priority} $BASE "Vector Points" 1 5]
    } else {
		set panel $Nv_($BASE)
    }

	########## create panel heading 
    frame $BASE  -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Vector Points Panel"

  	########## create top frame
    set top [frame $BASE.top]
	Label $top.current -text "Current:" -anchor w
    mkMapList $top.list site
    
	button $top.new -text New -anchor center -command "add_map site" -bd 1
	button $top.delete -text Delete -anchor center -command "delete_map site" -bd 1
    
	pack $top.current $top.list -side left
	pack $top.delete $top.new -side right -expand 0
    pack $top -side top -fill x -expand 1 -padx 3 -pady 5

    # initialize variables and map list   
    set curr [Nget_current site]
    set Nv_(siteshape) sphere
    change_marker

    if {0 != $curr}  {
		set width [Nsite$curr get_att width]
		set size [Nsite$curr get_att size]
		set Nv_(siteshape) [Nsite$curr get_att marker]
		set Nv_(sitedisplay) [Nsite$curr get_att display]
		set maplist [Nget_map_list surf]
		set longdim [expr [Nget_longdim] / 50 ]
		if {$longdim < 1} {set longdim 1.0}
			
		# We do this check to make sure that size is ALWAYS set to
		# something sensible the first time the surface is loaded
		if {$size == "0" || [expr $longdim/$size] < 2} then {
			Nsite$curr set_att size [expr $longdim / 20.0]
			set size [Nsite$curr get_att size]
		}
		if {"$Nv_(siteshape)" == ""} then {
			Nsite$curr set_att marker sphere
			set Nv_(siteshape) [Nsite$curr get_att marker]
		}
		if {$width == "0"} then {
			Nsite$curr set_att width 2
			set width [Nsite$curr get_att width]
		}
    } else {
		set width 1
		set size 10
		set Nv_(siteshape) sphere
		set Nv_(sitedisplay) 3d
		set maplist {}
		set longdim 100.0
    }
       
    #set increment value for SpinBox
    if {$longdim < 100} {
    	set spin_incr [expr $longdim/100.0]
    	set startindex [expr int(100*($size/$longdim))]
    } else {
    	set spin_incr 1
    	set startindex [expr int($longdim*($size/$longdim))]
    }

  	########## create bottom frame
    set bottom [frame $BASE.bottom] 
    button $bottom.close -text Close \
		-command "Nv_closePanel $BASE" -anchor s -bd 1
    pack $bottom.close -side right

    button $bottom.draw_current -text "DRAW CURRENT" -fg darkgreen -anchor s \
		-command {Nsite_draw_one [Nget_current site]} -bd 1

    pack $bottom.draw_current -side left
    pack $bottom -side bottom -fill x -expand 1 -padx 3 -pady 3

  	########## create mid frame
  		
    set mid1 [frame $BASE.mid1]
    set row1 [frame $mid1.row1]
    set row2 [frame $mid1.row2]
    set row3 [frame $mid1.row3]
    set row4 [frame $mid1.row4]
    set row5 [frame $mid1.row5]
    
    set szlabel [label $row1.szlabel -text "icon size" \
    	-font $nviztxtfont -fg black]
    
    
	# SpinBox range and starting marker size
    set range "0 $longdim $spin_incr"

    set ptsize [SpinBox $row1.sitesize -range $range \
		-textvariable size \
		-modifycmd {change_site_size $size} \
		-command {change_site_size $size} \
		-width 5]
		
		
	$ptsize setvalue @$startindex

    set ptcolor [button $row1.color -text Color \
		-command "change_color site $row1.color"]
    bind $ptcolor <Expose> "$row1.color configure -bg \[get_curr_sv_color site\]"
   
   	set markerlbl [label $row1.markerlbl -text "  icon type " -fg black -font $nviztxtfont]
    set markertype [ComboBox $row1.marker -width 8 \
    	-textvariable Nv_(siteshape) -modifycmd change_marker \
    	-values {"x" "sphere" "diamond" "cube" "box" "gyro" "aster" "histogram"}]

    pack $szlabel $ptsize $markerlbl $markertype -side left 
    pack $ptcolor -side left -padx 10

    set rb1 [radiobutton  $row2.disp3d -text "3D points" \
		-anchor nw -variable Nv_(sitedisplay) -value 3d \
		-command change_site_mode]

    set rb2 [radiobutton  $row3.dispsurf -text "display on surface(s):" \
		-anchor nw -variable Nv_(sitedisplay) -value surfdisp \
		-command change_site_mode]

    set surflist [Nv_mkSurfacelist $row3.list $maplist Nsite$curr site]
           
   	pack $rb1 -side left
   	pack $rb2 $surflist -side left 

    pack $row1 $row2 $row3 $row4 $row5 -anchor w -side top -expand no -fill none -pady 2
    pack $mid1 -side top -padx 3
    
	# frame for thematic point mapping
	
	set site_attr(FIELD_ATTR_PANEL) 0
    
	checkbutton $row4.themechk -text "thematic mapping for vector points" \
		-variable site_attr(FIELD_ATTR_PANEL) -command "if {$curr!=0} {site_attr_gui $row5 $bottom $curr}" \
		-offvalue 0 -onvalue 1
	pack $row4.themechk -side left -anchor nw
    

    return $panel
}

# Reset procedure for this panel
proc Nviz_site_reset {} {
    set site_list [Nget_site_list]

    foreach i $site_list {
	Nsite$i delete
    }

    set_new_curr site 0
}

# Save procedure for saving state of Nviz site files
proc Nviz_site_save {file_hook} {
    # For each site file we write out all of its attribute information. 
    # Sitess are referenced by logical name so that they are reloadable
    # (otherwise, they may be assigned different id's each time they are loaded
    # and scripts won't work correctly).

    # Get the list of site files
    set site_list [Nget_site_list]

    # Get the list of surfaces for checking draping
    set surf_list [Nget_surf_list]

    # Write out the total number of site files
    puts $file_hook ">>>start site"
    puts $file_hook "[llength $site_list]"

    # For each site file write out the following:
    # 1. Logical name
    # 2. map name
    # 3. color
    # 4. width
    # 5. list of logical names of surfaces displayed on
    # 6. marker
    # 7. size
    # 8. useatt
    # 9. display
    foreach i $site_list {

        # logical name
        puts $file_hook "[Nsite$i get_logical_name]"
        
        # map name
        puts $file_hook "[Nsite$i get_att map]"
    
        # color
        puts $file_hook "[Nsite$i get_att color]"
    
        # width
        puts $file_hook "[Nsite$i get_att width]"
    
        # logical names of surfaces displayed on
        set draped [list]
        foreach j $surf_list {
            if {[Nsite$i surf_is_selected Nsurf$j]} then {
                lappend draped $j
            }
        }
        puts $file_hook "[llength $draped]"
        foreach j $draped {
            puts $file_hook "[Nlogical_from_literal Nsurf$j]"
        }
    
        # marker
        puts $file_hook "[Nsite$i get_att marker]"
        
        # size
        puts $file_hook "[Nsite$i get_att size]"
    
        # useatt
    # temporarily disabled as causing problems (bug # 4377)
    #	puts $file_hook "[Nsite$i get_att useatt]"
    
        # display
        puts $file_hook "[Nsite$i get_att display]"
    
        flush $file_hook
    }
    # Done...
}

# Load procedure for loading state of Nviz site files
proc Nviz_site_load { file_hook } {
    # Read the number of  sites saved in this state file
    gets $file_hook num_sites

    # For each site file, create a new site map object with the given
    # logical name and fill in the attributes as appropriate
    for {set i 0} {$i < $num_sites} {incr i} {
		# Read in the logical name for this new site map
		gets $file_hook logical_name
	
		# Now create a new site map with the given logical name
		set new_site [Nnew_map_obj site "name=$logical_name"]
	
		# Set all attributes as appropriate (i.e. as they are read from the state file)
		
		# map
		gets $file_hook att_data
		$new_site set_att map $att_data
	
		# color 
		gets $file_hook att_data
		$new_site set_att color $att_data
	
		# width
		gets $file_hook att_data
		$new_site set_att width $att_data
	
		# Select all the appropriate surfaces to put this map on
		gets $file_hook num_selected_surfs
		for {set j 0} {$j < $num_selected_surfs} {incr j} {
			gets $file_hook selected_surf
	
			# Select this surf by translating from a logical name and selecting
			$new_site select_surf [Nliteral_from_logical $selected_surf]
		}
	
		# marker
		gets $file_hook att_data
		$new_site set_att marker $att_data
	
		# size
		gets $file_hook att_data
		$new_site set_att size $att_data
	
		# useatt
	# temporarily disabled as causing problems (bug # 4377)
	#	gets $file_hook att_data
	#	$new_site set_att useatt $att_data
	
		# display
		gets $file_hook att_data
		$new_site set_att display $att_data
	
		Nset_current site [string range $new_site 5 end]
    }

}

###########################################################################

proc change_marker {} {
    global Nv_
	global Nauto_draw
    
    set curr [Nget_current site]
    if {0 != $curr} {
		Nsite$curr set_att marker $Nv_(siteshape)
		if {$Nauto_draw == 1} {Ndraw_all}
    }
}

proc change_site_mode {} {
    global Nv_
	global Nauto_draw
    
    set curr [Nget_current site]
    if {0 != $curr} {
		if {![Nsite$curr set_att display $Nv_(sitedisplay)]} then {
			set Nv_(sitedisplay) surfdisp
			if {$Nauto_draw == 1} {Ndraw_all}
		}
		if {$Nauto_draw == 1} {Ndraw_all}
    }
}

proc change_site_size {size} {
	global Nauto_draw
	
    set curr [Nget_current site]
	if {0 != $curr} {
		Nsite$curr set_att size $size
		if {$Nauto_draw == 1} {Ndraw_all}
    }
}







