###############################################################
# group.tcl - group layer creation and management procedures 
# for GRASS GIS Manager
# January 2006 Michael Barton, Arizona State University
###############################################################

namespace eval GmGroup {
    variable array opt 
    variable count 1 
    variable array tree # mon
    variable nvelev ""
    variable nvcolor ""
    variable nvpoints ""
    variable nvlines ""
}


proc GmGroup::create { tree parent } {
    variable opt
    variable count 

    set fon [font create -size 10]
    set frm [ frame .groupicon$count ]
    set check [checkbutton $frm.check -text "" -font $fon \
		-variable GmGroup::opt($count,_check) \
		-height 1]
    set image [label $frm.image -image [Bitmap::get folder] ]
    pack $check $image -side left

    set node "group:$count"
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
		-text      "group $count" \
		-window    $frm \
		-drawcross auto \
        -open 1
	
    set opt($count,_check) 1
    set opt($count,treeimagepath) $image

    incr count

    return $node
}

proc GmGroup::save { tree depth node } {
    variable opt
	global mon
	
    if { $node != "root" } {
		set id [GmTree::node_id $node] 
		GmTree::rc_write $depth _check $opt($id,_check)
    }
    
    foreach n [$tree nodes $node] {
        GmTree::save_node $depth $n
    }

}

proc GmGroup::display { node mod } {
    variable opt
    variable tree
    global mon
    global drawprog
    global commandlist
    set commandlist {}

    set tree($mon) $GmTree::tree($mon)
	set layers ""

    if { $node != "root" } {
		set id [GmTree::node_id $node] 
        if { ! ( $opt($id,_check) ) } { return }
    }

    #invert layer list to put first tree node as top map layer
    foreach n [$tree($mon) nodes $node] {
	    set layers [linsert $layers 0 $n]
    }
    
    # display each node/layer
    foreach n $layers {
        GmTree::display_node $n $mod
        incr drawprog
    }

}

###############################################################################

# display background maps for digitizing in v.digit
proc GmGroup::vdigit_display { vectmap } {
    variable opt
    variable tree
    variable bg_command 
    global mon
    global drawprog
    global commandlist

    set bg_command ""
    
    # display selected layers to create a display command list if needed
    if {[llength $commandlist] == 0} {
            MapCanvas::request_redraw $mon 1
            vwait commandlist
    }
    
    # if the layer being digitized is the only one displayed, then don't
    # make it a background layer too. This avoids a black background.
    set mapname [lindex [split [lindex [split [lindex $commandlist 0] ] 1] =] 1]
    
    if {[llength $commandlist] == 1 && $mapname == $vectmap} {
        return $bg_command
    }

    # add each command in display command list to background commands
    foreach cmd $commandlist {
            append bg_command "$cmd;"
    }
    
            
    # get rid of the ; at the end of the background command list
    set bg_command [string trimright $bg_command ";"]
    
    return $bg_command
				
}


###############################################################################

proc GmGroup::nvdisplay { node } {
    variable opt
    variable tree
    variable nvelev 
    variable nvcolor
    variable nvpoints
    variable nvlines
    global mon
    global drawprog
    global devnull
    #global commandlist

    #
    #if {[llength $commandlist] == 0} {
    #    MapCanvas::request_redraw $mon 1
    #    vwait commandlist
    #}

    set tree($mon) $GmTree::tree($mon)
    if { $node != "root" } {
        set id [GmTree::node_id $node] 
        if { ! ( $opt($id,_check) ) } { return }
    }

    foreach n [$tree($mon) nodes $node] {
        GmGroup::nviz $n
        incr drawprog
    }

    if { $nvelev!= "" } {
        set cmd [list nviz elevation=$nvelev color=$nvcolor]
        if {$nvlines != ""} {
            lappend cmd vector=$nvlines
        }
        if {$nvpoints != ""} {
            lappend cmd points=$nvpoints
        }
        
        if {[catch {eval exec "$cmd 2> $devnull &"} error]} {
            GmLib::errmsg $error
        }
    } else {
        return
    }

    set nvelev ""
    set nvcolor ""
    set nvlines ""
    set nvpoints ""
}


# display raster maps in NVIZ (base maps for elevation and drape maps for color)
proc GmGroup::nviz { node } {
    variable opt
    variable tree
    variable nvelev 
    variable nvcolor
    variable nvpoints
    variable nvlines
    global mon
    global drawprog
    global devnull
            
    #set id [GmTree::node_id $node] 
    set vect ""
    set vecttype ""
    set type [GmTree::node_type $node]
    
    switch $type {
        "group" {
           GmGroup::nvdisplay $node 
        }
        "raster" {
            set surf [GmRaster::addelev $node]
            if {$surf == ""} {return}
            set clr [GmRaster::addcolor $node]
            if {$clr == ""} {set clr $surf}

            # test whether surf and clr are valid files
            if {[catch {set rexist [eval exec "r.info map=$surf 2> $devnull"]} error]} {
                return
            } else {
                if { $rexist == "" } {return}
            }

            if {[catch {set rexist [eval exec "r.info map=$clr 2> $devnull"]} error]} {
                set clr $surf
            } else {
                if {$rexist == "" } {set clr $surf}
            }

            # add surf and clr to list of maps to display in nviz
            if {$nvelev == "" } {
                set nvelev $surf
            } else {
                append nvelev ",$surf"
            }
                    
            if {$nvcolor == "" } {
                set nvcolor $clr
            } else {
                append nvcolor ",$clr"
            }
        }
        "vector" {
            set vect [GmVector::addvect $node]
            if {$vect == ""} {return}
            
            # test whether vect is a valid file
            if {[catch {set vexist [eval exec "v.info map=$vect 2> $devnull"]} error]} {
                return
            } else {
                if {$vexist != ""} {
                    set vecttype [GmVector::vecttype $vect]
                } else {
                    return
                }
            }
                        
            if {$vecttype == "points"} {
                # display vector in nviz as points
                if {$nvpoints == "" } {
                    set nvpoints $vect
                } else {
                    append nvpoints ",$vect"
                }
            } else {
                # display vector in nviz as lines
                if {$nvlines == "" } {
                    set nvlines $vect
                } else {
                    append nvlines ",$vect"
                }
            }
        }
    }
}


###############################################################################
proc GmGroup::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
 
    set opt($id,$key) $value
}

proc GmGroup::open { id } {
    variable opt

    $GmGroup::opt($id,treeimagepath) configure -image [Bitmap::get openfold]

}

proc GmGroup::close { id } {
    variable opt

    $GmGroup::opt($id,treeimagepath) configure -image [Bitmap::get folder]
}

proc GmGroup::duplicate { tree parent_node sel id } {
    puts "Duplicate for Groups not yet implemented."
}
