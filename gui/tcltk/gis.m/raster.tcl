##########################################################################
# raster.tcl - raster layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:    (C) 1999 - 2006 by the GRASS Development Team
#
#       This program is free software under the GNU General Public
#       License (>=v2). Read the file COPYING that comes with GRASS
#       for details.
#
##########################################################################

namespace eval GmRaster {
    variable array opt # raster current options
    variable count 1
    variable array tree # mon
    variable array lfile # raster
    variable array lfilemask # raster
    variable optlist
    variable array dup # vector
}

###############################################################################

# create raster map node in layer tree
proc GmRaster::create { tree parent } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup
    global iconpath
    global mon

    set node "raster:$count"

    #create form for layer tree entry
    set frm [ frame .rastericon$count]
    set check [checkbutton $frm.check \
        -variable GmRaster::opt($count,1,_check) \
        -height 1 -padx 0 -width 0]

    image create photo rico -file "$iconpath/element-cell.gif"
    set ico [label $frm.ico -image rico -bd 1 -relief raised]
    pack $check $ico -side left

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    #insert new layer
    if {[$tree selection get] != "" } {
        set sellayer [$tree index [$tree selection get]]
    } else {
        set sellayer "end"
    }

    $tree insert $sellayer $parent $node \
    -text  "raster $count"\
    -window    $frm \
    -drawcross auto

    #set default option values
    set opt($count,1,_check) 1
    set dup($count) 0

    set opt($count,1,opacity) 1.0
    set opt($count,1,map) ""
    set opt($count,1,drapemap) ""
    set opt($count,1,brighten) 0
    set opt($count,1,querytype) "cat"
    set opt($count,1,rastquery) ""
    set opt($count,1,rasttype) ""
    set opt($count,1,bkcolor) ""
    set opt($count,1,overlay) 1
    set opt($count,1,mod) 1

    set optlist {_check opacity map drapemap brighten querytype rastquery rasttype bkcolor \
        overlay}

    foreach key $optlist {
        set opt($count,0,$key) $opt($count,1,$key)
    }

    # create files in tmp diretory for layer output
    set mappid [pid]
	if {[catch {set lfile($count) [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
    set lfilemask($count) $lfile($count)
    append lfile($count) ".ppm"
    append lfilemask($count) ".pgm"

    incr count
    return $node

}

###############################################################################

proc GmRaster::set_option { node key value } {
    variable opt

    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

###############################################################################

# select base raster map from list and put name in layer tree node
proc GmRaster::select_map { id } {
    variable tree
    variable node
    global mon

    set m [GSelect cell title [G_msg "Raster map"] parent "."]
    if { $m != "" } {
        set GmRaster::opt($id,1,map) $m
        GmTree::autonamel $m
    }
}

# select drape raster map from list and put name in layer tree node
proc GmRaster::select_drapemap { id } {
    variable tree
    variable node
    global mon

    set m [GSelect cell title [G_msg "Raster drape map"] parent "."]
    if { $m != "" } {
        set GmRaster::opt($id,1,drapemap) $m
        GmTree::autonamel $m
    }
}

###############################################################################

# show base raster info in output window
proc GmRaster::show_info { id } {
	variable opt
	set mapname $opt($id,1,map)
	set cmd "r.info map=$mapname"		
	run_panel $cmd
}

# show drape raster info in output window
proc GmRaster::show_info_drape { id } {
	variable opt
	set mapname $opt($id,1,drapemap)
	set cmd "r.info map=$mapname"		
	run_panel $cmd
}
###############################################################################

# set and display raster options
proc GmRaster::options { id frm } {
    variable opt
    global iconpath
    global bgcolor

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text [G_msg "Display raster maps"] \
        -fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    #opacity
    set row [ frame $frm.opc]
    Label $row.a -text [G_msg "Opaque "]
    scale $row.b -from 1.0 -to 0.0 -showvalue 1 \
        -orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
        -variable GmRaster::opt($id,1,opacity)
    Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # raster name
    set row [ frame $frm.name ]
    Label $row.a -text [G_msg "Base map:\t"]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "base raster map to display"]\
        -command "GmRaster::select_map $id"
    Entry $row.c -width 35 -text " $opt($id,1,map)" \
          -textvariable GmRaster::opt($id,1,map)
    Label $row.d -text ""
    Button $row.e -text [G_msg "base map info"] \
            -image [image create photo -file "$iconpath/gui-rv.info.gif"] \
            -command "GmRaster::show_info $id" \
            -background $bgcolor \
            -helptext [G_msg "base map info"]
    Label $row.f -text ""
    Button $row.g -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.rast" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # raster query
    set row [ frame $frm.rquery ]
    Label $row.a -text [G_msg "\tvalues to display"]
    LabelEntry $row.b -textvariable GmRaster::opt($id,1,rastquery) -width 35
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # drape name
    set row [ frame $frm.drapeinfo1 ]
    Label $row.a -text [G_msg "\tOptional color draping. Use base map for shading,"]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    set row [ frame $frm.drapeinfo2 ]
    Label $row.a -text [G_msg "\tdrape map for color in color relief map or data fusion"]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    set row [ frame $frm.drape ]
    Label $row.a -text [G_msg "\tdrape map:  "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "raster map to drape over base map"]\
        -command "GmRaster::select_drapemap $id"
    Entry $row.c -width 35 -text " $opt($id,1,drapemap)" \
          -textvariable GmRaster::opt($id,1,drapemap)
    Label $row.d -text ""
    Button $row.e -text [G_msg "drape map info"] \
            -image [image create photo -file "$iconpath/gui-rv.info.gif"] \
            -command "GmRaster::show_info_drape $id" \
            -background $bgcolor \
            -helptext [G_msg "drape map info"] 
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
    # HIS brightness for drape map
    set row [ frame $frm.bright ]
    Label $row.a -text [G_msg "\tdrape map brightness adjustment\t "]
    SpinBox $row.b -range {-99 99 1} -textvariable GmRaster::opt($id,1,brighten) \
		-width 3 -helptext [G_msg "Adjust brightness of drape map"] \
		-entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # overlay
    set row [ frame $frm.over ]
    checkbutton $row.a -text [G_msg "overlay maps from other layers (transparent null value cells)"] \
        -variable GmRaster::opt($id,1,overlay)
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # background color
    set row [ frame $frm.bg ]
    Label $row.a -text [G_msg " Set background color (colored null value cells)"]
    ComboBox $row.b -padx 2 -width 10 -textvariable GmRaster::opt($id,1,bkcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"}
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

###############################################################################

# save raster node to workspace grc file
proc GmRaster::save { tree depth node } {
    variable opt
    variable optlist
    global mon

    set id [GmTree::node_id $node]

    foreach key $optlist {
         GmTree::rc_write $depth "$key $opt($id,1,$key)"
    }
}


###############################################################################

# append elevation maps display lists for NVIZ
proc GmRaster::addelev {node } {
    variable opt
    variable tree
    global mon

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { ! ( $opt($id,1,_check) ) } { return }

    set nvelev "$opt($id,1,map)"

    return $nvelev
}

# append drape colors to display lists for NVIZ
proc GmRaster::addcolor {node } {
    variable opt
    variable tree
    global mon

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { ! ( $opt($id,1,_check) ) } { return }

    if { $opt($id,1,drapemap) != "" } {
        set nvcolor $opt($id,1,drapemap)
    } else {
        set nvcolor $opt($id,1,map)
    }
    return $nvcolor
}

###############################################################################

# display raster map and output to graphic file for compositing
proc GmRaster::display { node mod } {
    global mon
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count

    set rasttype ""

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,map) == "" } { return }

    set cmd "d.rast map=$opt($id,1,map)"

    # overlay
    if { $opt($id,1,overlay) } {
        append cmd " -o"
    }

    # set raster type
    set rasttype ""
    set rt ""
    if {![catch {open "|r.info map=$opt($id,1,map) -t" r} rt]} {
        set rasttype [read $rt]
		if {[catch {close $rt} error]} {
			GmLib::errmsg $error
		}
    }

    if {$rasttype == "" || [regexp -nocase ".=CELL" $rasttype]} {
        set querytype "cat"
    } else {
        set querytype "vallist"
    }


    # raster query
    if { $opt($id,1,rastquery) != "" } {
        append cmd " {$querytype=$opt($id,1,rastquery)}"
    }

    # background color
    if { $opt($id,1,bkcolor) != "" } {
        append cmd " bg=$opt($id,1,bkcolor)"
    }

    set cmd2 "d.his h_map=$opt($id,1,drapemap) i_map=$opt($id,1,map) brighten=$opt($id,1,brighten)"

    if { $opt($id,1,drapemap) == "" } {
        # set cmd $cmd
    } else {
        set cmd $cmd2
    }

    # Decide whether to run, run command, and copy files to temp
    GmCommonLayer::display_command [namespace current] $id $cmd
}

###############################################################################

# get selected raster map (used for query)
proc GmRaster::mapname { node } {
    variable opt
    variable tree
    global mon

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { $opt($id,1,map) == "" } { return }

    set mapname $opt($id,1,map)
    return $mapname
}

###############################################################################

# duplicate currently selected layer
proc GmRaster::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup
    global iconpath
    global mon

    set node "raster:$count"
    set dup($count) 1

    set frm [ frame .rastericon$count]
    set check [checkbutton $frm.check \
        -variable GmRaster::opt($count,1,_check) \
        -height 1 -padx 0 -width 0]

    image create photo rico -file "$iconpath/element-cell.gif"
    set ico [label $frm.ico -image rico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left

    # where to insert new layer
    if {[$tree selection get] != "" } {
        set sellayer [$tree index [$tree selection get]]
    } else {
        set sellayer "end"
    }

    if { $opt($id,1,map) == ""} {
        $tree insert $sellayer $parent $node \
        -text      "raster $count" \
        -window    $frm \
        -drawcross auto
    } else {
        $tree insert $sellayer $parent $node \
        -text      "$opt($id,1,map)" \
        -window    $frm \
        -drawcross auto
    }

    set opt($count,1,opacity) $opt($id,1,opacity)

    set optlist {_check map drapemap brighten querytype rastquery rasttype bkcolor \
        overlay}

    foreach key $optlist {
        set opt($count,1,$key) $opt($id,1,$key)
        set opt($count,0,$key) $opt($count,1,$key)
    }

    set id $count

    # create files in tmp directory for layer output
    set mappid [pid]
	if {[catch {set lfile($count) [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
    set lfilemask($count) $lfile($count)
    append lfile($count) ".ppm"
    append lfilemask($count) ".pgm"

    incr count
    return $node
}
