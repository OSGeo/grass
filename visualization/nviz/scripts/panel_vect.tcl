##########################################################################
#
# Routines for vector visualization for NVIZ
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

##########################################################################

############################################################################
# procedure to make main control area
###########################################################################
source $src_boot/etc/nviz2.2/scripts/config.tcl


proc mkvectPanel { BASE } {
    global Nv_
	global nviztxtfont
    
    catch {destroy $BASE}

    set curr [Nget_current vect]
    
    if {$curr != 0}  {
		set width [Nvect$curr get_att width]
		set flat_state [Nvect$curr get_att flat]
		set height [expr [lindex [Nvect$curr get_trans] 2] * 10]
		set maplist [Nget_map_list surf]
    } else {
		set width 1
        set flat_state 0
		set height 0
		set maplist {}
    }
    
    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
		set panel [St_create {window name size priority} $BASE "Vector Lines/3D Polygons" 2 5]
    } else {
		set panel $Nv_($BASE)
    }
    frame $BASE -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Vector Lines Panel"
    
  	#create top frame
	###### make widgets that control which is current surface (menu, new delete)###
    set tmp [frame $BASE.top]
	Label $tmp.current -text "Current:" -anchor w
	
    mkMapList $tmp.list vect
    
	button $tmp.new -text New -anchor center -command "add_map vect" -bd 1
	button $tmp.delete -text Delete -anchor center -command "delete_map vect" -bd 1

	pack $tmp.current $tmp.list -side left
    pack $tmp.list -side left 
	pack $tmp.delete $tmp.new -side right -expand 0
    
    pack $tmp -side top -fill x -expand 1 -padx 3 -pady 5
    
    # create bottom frame
    set tmp [frame $BASE.f]
    button $tmp.close -text Close -command "Nv_closePanel $BASE" -anchor s -bd 1
    pack $tmp.close -side right
    button $tmp.draw_current -text "DRAW CURRENT" -bd 1 -fg darkgreen \
		-command {Nvect_draw_one [Nget_current vect]}
    pack $tmp.draw_current -side left
    pack $tmp -side bottom -fill x -expand 1 -padx 3

    #create mid frame
    set tmp [frame $BASE.mid]
    set row1 [frame $tmp.row1]
    set row2 [frame $tmp.row2]
    set row3 [frame $tmp.row3]
    set row4 [frame $tmp.row4]
    set tmp1a [frame $row1.b]
    
    set wlabel [label $row1.wlabel -text "line width" \
    	-font $nviztxtfont -fg black]
    
    set vlinewidth [SpinBox $row1.linewidth -range {1 50 1}\
		-textvariable width \
		-modifycmd {set_width vect $width} \
		-command {set_width vect $width} \
		-width 5 \
		-entrybg white]

#	checkbutton $tmp.load -relief flat -text "Load to memory"
    set vcolor [button $row1.color -text Color -bd 1\
		-command "change_color vect $row1.color"]
    bind $row1.color <Expose> "$row1.color configure -bg \[get_curr_sv_color vect\]"
    
    set rb1 [radiobutton $row2.label2 -text "display flat" \
		-variable flat_state -value 1 -command "check_list $row3.list"]

    set rb2 [radiobutton $row2.label1 -text "display on surface(s):" \
		-variable flat_state -value 0 \
        -command "check_list $row3.list"]

    set htscale [Nv_mkScale $row4.scale h "vector height\nabove surface" 0 10000 $height set_ht 1]


    pack $wlabel $vlinewidth -side left
    pack $vcolor -side left -padx 10
    pack $rb1 -side right -padx 5
    pack $row1 -expand 1 -fill none -pady 4
    pack $rb2 -side left 
    pack $row2 -side left
    pack $row3 -side left
    pack $htscale -side top -anchor w
    pack $row4 -side top -fill x -expand 1 -pady 4 

	# Let radiobutton state handle building list
	# of available surfaces
    if {$flat_state == 0} {
		$row2.label1 select
		check_list $row3.list
    } else {
		$row2.label2 select
		check_list $row3.list
    }

    pack $row1 $row2 $row3 -side top -fill both -expand 1
    pack $row4 -side right -fill both -expand 1
    pack $tmp -side top  -fill both -expand 1  -padx 3

    return $panel
}

# Reset procedure for this panel
proc Nviz_vect_reset {} {
    set vect_list [Nget_vect_list]

    foreach i $vect_list {
	Nvect$i delete
    }

    set_new_curr vect 0
}

# Save procedure for saving state of Nviz vect files
proc Nviz_vect_save {file_hook} {
    # For each vector file we write out all of its attribute information. 
    # Vectors are referenced by logical name so that they are reloadable
    # (otherwise, they may be assigned different id's each time they are loaded
    # and scripts won't work correctly).

    # Get the list of vect files
    set vect_list [Nget_vect_list]

    # Get the list of surfaces for checking draping
    set surf_list [Nget_surf_list]

    # Write out the total number of vector files
    puts $file_hook ">>>start vect"
    puts $file_hook "[llength $vect_list]"

    # For each vector file write out the following:
    # 1. Logical name
    # 2. map name
    # 3. color
    # 4. width
    # 5. list of logical names of surfaces displayed on
    foreach i $vect_list {

        # logical name
        puts $file_hook "[Nvect$i get_logical_name]"
        
        # map name
        puts $file_hook "[Nvect$i get_att map]"
    
        # color
        puts $file_hook "[Nvect$i get_att color]"
    
        # width
        puts $file_hook "[Nvect$i get_att width]"
    
        # logical names of surfaces displayed on
        set draped [list]
        foreach j $surf_list {
            if {[Nvect$i surf_is_selected Nsurf$j]} then {
                lappend draped $j
            }
        }
        puts $file_hook "[llength $draped]"
        foreach j $draped {
            puts $file_hook "[Nlogical_from_literal Nsurf$j]"
        }
    
        flush $file_hook
    }
    # Done...
}

# Load procedure for loading state of Nviz vect files
proc Nviz_vect_load { file_hook } {
    # Read the number of surfaces saved in this state file
    gets $file_hook num_vects

    # For each vect file, create a new surface with the given logical
    # name and fill in the attributes as appropriate
    for {set i 0} {$i < $num_vects} {incr i} {
	# Read in the logical name for this new vect map
	gets $file_hook logical_name

	# Now create a new vect map with the given logical name
	set new_vect [Nnew_map_obj vect "name=$logical_name"]

	# Set all attributes as appropriate (i.e. as they are read from the state file)
	
	# map
	gets $file_hook att_data
	$new_vect set_att map $att_data

	# color 
	gets $file_hook att_data
	$new_vect set_att color $att_data

	# width
	gets $file_hook att_data
	$new_vect set_att width $att_data

	# Select all the appropriate surfaces to put this map on
	gets $file_hook num_selected_surfs
	for {set j 0} {$j < $num_selected_surfs} {incr j} {
	    gets $file_hook selected_surf

	    # Select this surf by translating from a logical name and selecting
	    $new_vect select_surf [Nliteral_from_logical $selected_surf]
	}

	Nset_current vect [string range $new_vect 5 end]
    }

}


proc change_color { type me } {
    set curr [Nget_current $type]
    switch $type {
	"vect" { set head Nvect }
	"site" { set head Nsite }
    }

    if {0 != $curr} {
	set clr [$head$curr get_att color]
	set clr [mkColorPopup .colorpop Color $clr 1]
	$head$curr set_att color $clr
    }

    $me configure -bg [get_curr_sv_color $type]
}

proc get_curr_sv_color { type } {
    set curr [Nget_current $type]
    switch $type {
	"vect" { set head Nvect }
	"site" { set head Nsite }
    }

    if {0 == $curr} then {
	return "gray90"
    }

    set color [$head$curr get_att color]

    set color [expr int([tcl_to_rgb $color])]
    set blue  [hexval [expr int($color & 0x0000ff)]]
    set green [hexval [expr int(($color & 0x00ff00)>>8)]]
    set red   [hexval [expr int(($color & 0xff0000)>>16)]]
    return "#$red$green$blue"

}

proc delete_map {type} {
    set curr [Nget_current $type]
    switch $type {
	"vect" { set head Nvect }
	"site" { set head Nsite }
    }
    
    if {0 != $curr} {
	$head$curr delete
	set name 0
	
	switch $type {
	    "vect" { set new_list [Nget_vect_list] }
	    "site" { set new_list [Nget_site_list] }
	}
	
	if {[llength $new_list] != 0} then {
	    set name [lindex $new_list 0]
	} else {
	    set name 0
	}
	
	set_new_curr $type $name
    }
    
}

# Use this routine when adding a vect or site in a script
proc script_add_map { type map_name } {

    set temp [Nnew_map_obj $type]
    $temp set_att map $map_name
    set_new_curr $type [string range $temp 5 end]
    
    return [string range $temp 5 end]
}

proc add_map {type} {
    if { $type == "site" } { set browse_type "vect" } else { set browse_type $type }
    set new [create_map_browser .fbrowse $browse_type 1]

    # Let user know that we are busy
    appBusy

    if {$new != "-1"} {
	set temp [Nnew_map_obj $type]
	$temp set_att map $new
	set_new_curr $type [string range $temp 5 end]
    }

    # Let user know that he may proceed
    appNotBusy
}

proc set_width {type E} {
	global Nauto_draw

    set curr [Nget_current $type]
    switch $type {
	"vect" { set head Nvect }
	"site" { set head Nsite }
    }
    
    if {0 != $curr} {
	$head$curr set_att width $E
    }

	if {$Nauto_draw == 1} {Ndraw_all}
}

# Procedure to set vector elevation eith above surface
# or level height
proc set_ht {h} {
    global Nv_ vect_height

    set vect_height $h
    set curr [Nget_current vect]
    if {0 != $curr}  {
    Nvect$curr set_trans 0 0 $vect_height
    }
      
}

#Procedure to update vect atts from radiobutton
proc check_list {address} {
    global Nv_ 
    global flat_state 
    global curr 
    global vect_height

    set state [winfo exists $address]
    set curr [Nget_current vect]

    if {$curr != 0}  {
        set maplist [Nget_map_list surf]

        if {$state == 0 && $flat_state == 0 } {
            #display on surface
            catch {destroy $address}
            Nvect$curr set_att flat 0
            Nv_mkSurfacelist $address $maplist Nvect$curr vect
            pack $address
        } 
        if {$state == 1 && $flat_state == 1} {
            #display on flat
            catch {destroy $address}
            Nvect$curr set_att flat 1
        }
    }
	
}
