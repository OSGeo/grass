# This file contains auxiliary routines used to handle cutplane 0
# as a keyframe parameter

# Create the cutplane get_entries function
proc cutplane0_get_entries {} {
    global Nv_
    
    # Assemble the entry list showing the current state of cutplane 0
    set ret [list]
    
    foreach i [list pos_x pos_y pos_z blend_type rot tilt] {
	set val [list $i]
	
	switch $i {
	    "pos_x"      { lappend val [lindex [Ncutplane0 get_trans] 0] }
	    "pos_y"      { lappend val [lindex [Ncutplane0 get_trans] 1] }
	    "pos_z"      { lappend val [lindex [Ncutplane0 get_trans] 2] }
	    "blend_type" { lappend val $Nv_(CutPlaneFence) }
	    "rot"        { lappend val [lindex [Ncutplane0 get_rot] 2] }
	    "tilt"       { lappend val [lindex [Ncutplane0 get_rot] 1] }
	}
	
	lappend ret $val
    }
    
    return $ret
}

# Create the cutplane set_entries function
proc cutplane0_set_entries { elist } {
    global Nv_
    
    set cur_trans [Ncutplane0 get_trans]
    set old_trans $cur_trans
    set cur_rot   [Ncutplane0 get_rot]
    set old_rot $cur_rot
    set cur_blend $Nv_(CutPlaneFence)
    set old_blend $cur_blend
    
    foreach i $elist {
	switch [lindex $i 0] {
	    "pos_x"		 { set cur_trans [lreplace $cur_trans 0 0 [lindex $i 1]] }
	    "pos_y"      { set cur_trans [lreplace $cur_trans 1 1 [lindex $i 1]] }
	    "pos_z"      { set cur_trans [lreplace $cur_trans 2 2 [lindex $i 1]] }
	    "blend_type" { set cur_blend [lindex $i 1] }
	    "rot"        { set cur_rot [lreplace $cur_rot 2 2 [lindex $i 1]] }
	    "tilt"       { set cur_rot [lreplace $cur_rot 1 1 [lindex $i 1]] }
	}
    }
    
    if {"$cur_trans" != "$old_trans"} then {
	Ncutplane0 set_trans [lindex $cur_trans 0] [lindex $cur_trans 1] [lindex $cur_trans 2]
    }
    
    if {"$cur_rot" != "$old_rot"} then {
	Ncutplane0 set_rot [lindex $cur_rot 0] [lindex $cur_rot 1] [lindex $cur_rot 2]
    }
    
    if {"$cur_blend" != "$old_blend"} then {
	set Nv_(CutPlaneFence) $cur_blend
	Nset_fence_color $cur_blend
    }
}
