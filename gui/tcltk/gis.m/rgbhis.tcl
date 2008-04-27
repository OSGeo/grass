##########################################################################
# rgbhis.tcl - RGB and HIS display layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmRgbhis {
    variable array opt # rgbhis current options
    variable count 1
    variable array lfile # rgbhis
    variable array lfilemask # rgbhis
    variable optlist
    variable array dup # vector
}


###############################################################################
proc GmRgbhis::create { tree parent } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup
    global iconpath
    global mon

    set node "rgbhis:$count"

    set frm [ frame .rgbicon$count]
    set check [checkbutton $frm.check \
		-variable GmRgbhis::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo rgbico -file "$iconpath/module-d.rgb.gif"
    set ico [label $frm.ico -image rgbico -bd 1 -relief raised]
    
    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "RGB-HIS $count"\
	-window    $frm \
	-drawcross auto  
    
    #set default option values
    set opt($count,1,_check) 1 
    set dup($count) 0

    set opt($count,1,map1) "" 
    set opt($count,1,map2) "" 
    set opt($count,1,map3) "" 
	set opt($count,1,opacity) 1.0
    set opt($count,1,overlay) 1 
    set opt($count,1,rgb) 1 
    set opt($count,1,his) 0 
    set opt($count,1,mod) 1
    set opt($count,1,brighten) 0


	set optlist { _check map1 map2 map3 brighten opacity rgb his overlay}

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
proc GmRgbhis::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

proc GmRgbhis::select_map1 { id } {
    variable tree
    variable node
    set m1 [GSelect cell title [G_msg "Raster map for red or hue channel"] parent "."]
    if { $m1 != "" } { 
        set GmRgbhis::opt($id,1,map1) $m1
        GmTree::autonamel [format [G_msg "RGB-HIS %s"] $m1]
    }
}

proc GmRgbhis::select_map2 { id } {
    variable tree
    variable node
    set m2 [GSelect cell title [G_msg "Raster map for green or intensity channel"] parent "."]
    if { $m2 != "" } { 
        set GmRgbhis::opt($id,1,map2) $m2
    }
}
proc GmRgbhis::select_map3 { id } {
    variable tree
    variable node
    set m3 [GSelect cell title [G_msg "Raster map for blue or saturation channel"] parent "."]
    if { $m3 != "" } { 
        set GmRgbhis::opt($id,1,map3) $m3
    }
}
###############################################################################
# display RGB and HIS options
proc GmRgbhis::options { id frm } {
    variable opt
    global bgcolor
    global iconpath

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text [G_msg "Display 3 raster maps as Red/Green/Blue or Hue/Intensity/Saturation channels"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmRgbhis::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # display type
    set row [ frame $frm.type ]
    radiobutton $row.a -variable GmRgbhis::opt($id,1,rgb) -value "1" \
		-highlightthickness 0 
	Label $row.b -anchor w -text [G_msg "display maps as RGB"]
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.rgb" \
            -background $bgcolor \
            -helptext [G_msg "Help for RGB"]
    Label $row.d -text " \t"
    radiobutton $row.e -variable GmRgbhis::opt($id,1,rgb) -value "0" \
		-highlightthickness 0 
	Label $row.f -anchor w -text [G_msg "display maps as HIS"]
    Button $row.g -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.his" \
            -background $bgcolor \
            -helptext [G_msg "Help for HIS"]
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # raster1 name
    set row [ frame $frm.name1 ]
    Label $row.a -text [G_msg "red (RGB) or hue (HIS): "]
    Button $row.b -image [image create photo -file "$iconpath/channel-red.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "raster map for red or hue channel (HIS drape)"]\
		-command "GmRgbhis::select_map1 $id" -height 26
    Entry $row.c -width 30 -text " $opt($id,1,map1)" \
          -textvariable GmRgbhis::opt($id,1,map1)
    pack $row.c $row.b $row.a -side right -padx 3
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # raster2 name
    set row [ frame $frm.name2 ]
    Label $row.a -text [G_msg "green (RGB) or intensity (HIS): "]
    Button $row.b -image [image create photo -file "$iconpath/channel-green.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "raster map for green or intensity channel (HIS relief)"]\
		-command "GmRgbhis::select_map2 $id" -height 26
    Entry $row.c -width 30 -text " $opt($id,1,map2)" \
          -textvariable GmRgbhis::opt($id,1,map2) 
    pack $row.c $row.b $row.a -side right -padx 3
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # raster3 name
    set row [ frame $frm.name3 ]
    Label $row.a -text [G_msg "blue (RGB) or saturation (HIS): "]
    Button $row.b -image [image create photo -file "$iconpath/channel-blue.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "raster map for blue or saturation channel"]\
		-command "GmRgbhis::select_map3 $id" -height 26
    Entry $row.c -width 30 -text " $opt($id,1,map3)" \
          -textvariable GmRgbhis::opt($id,1,map3) 
    pack $row.c $row.b $row.a -side right -padx 3
    pack $row -side top -fill both -expand yes -padx 5 -pady 1
    
    # HIS brightness
    set row [ frame $frm.bright ]
    Label $row.a -anchor w -text [G_msg "HIS brightness adjustment\t "]
    SpinBox $row.b -range {-99 99 1} -textvariable GmRgbhis::opt($id,1,brighten) \
		-width 3 -helptext [G_msg "Adjusts the HIS intensity channel brightness"] \
		-entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # overlay
    set row [ frame $frm.over ]
    checkbutton $row.a -text [G_msg "overlay maps from other layers (transparent null value cells)"] \
	-variable GmRgbhis::opt($id,1,overlay) 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # something changed, redraw needed
    set opt($id,1,mod) "1"

}

###############################################################################
proc GmRgbhis::save { tree depth node } {
    variable opt
    variable optlist
    global mon
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}


###############################################################################
proc GmRgbhis::display { node mod} {
    global mon
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count
    
    set line ""
    set input ""
    
    set cmd1 ""

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,map1) == "" } { return } 
    if { $opt($id,1,map2) == "" &&  $opt($id,1,map3) == "" && $opt($id,1,rgb) == 1 } { 
        return 
     } 
  
    if { $opt($id,1,rgb) == 1 } { 
        set cmd1 "d.rgb red=$opt($id,1,map1) green=$opt($id,1,map2) blue=$opt($id,1,map3)" 
     } else { 
		set cmd1 "d.his h_map=$opt($id,1,map1)"
	
		if { $opt($id,1,map2) != "" } {
			append cmd1 " i_map=$opt($id,1,map2)"
		}
		if { $opt($id,1,map3) != "" } {
			append cmd1 " s_map=$opt($id,1,map3)"
		}
		if { $opt($id,1,brighten) != "0" } {
			append cmd1 " brighten=$opt($id,1,brighten)"
		}
    }		

    # overlay
    if { $opt($id,1,overlay) && $opt($id,1,rgb) == 1 } { 
		append cmd1 " -o"
    } else {
		append cmd1 " -n"
    }

    # Decide whether to run, run commands, and copy files to temp
    # Original logic here was to erase before drawing his if both exist
    # Was this really supposed to be mutually exclusive?
    GmCommonLayer::display_commands [namespace current] $id [list $cmd1]
}



###############################################################################
proc GmRgbhis::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup
    global iconpath
    global mon

    set node "rgbhis:$count"
	set dup($count) 1

    set frm [ frame .rgbhisicon$count]
    set check [checkbutton $frm.check \
		-variable GmRgbhis::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo rgbico -file "$iconpath/module-d.rgb.gif"
    set ico [label $frm.ico -image rgbico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left

	# where to insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

	if { $opt($id,1,map1) == ""} {
	    $tree insert $sellayer $parent $node \
		-text      "RGB-HIS $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert $sellayer $parent $node \
		-text      "RGB-HIS $opt($id,1,map1)" \
		-window    $frm \
		-drawcross auto
	}

	set opt($count,1,opacity) $opt($id,1,opacity)

	set optlist { _check map1 map2 map3 rgb his overlay brighten}

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

###############################################################################
proc GmRgbhis::mapname { node } {
    variable opt
    variable tree
    global mon
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { $opt($id,1,map1) == "" && $opt($id,1,map2) == "" } { return }

    set mapname "$opt($id,1,map1),$opt($id,1,map2)"

    if {$opt($id,1,map3) != ""} {
    	append mapname ",$opt($id,1,map3)"
    }
    
    return $mapname
}
