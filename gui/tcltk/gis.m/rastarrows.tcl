##########################################################################
# rastarrows.tcl - raster slope arrows layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmArrows {
    variable array opt # arrows current options
    variable count 1
    variable array tree # mon
    variable array lfile # raster
    variable array lfilemask # raster
    variable optlist
    variable array dup # vector
}

###############################################################################
proc GmArrows::create { tree parent } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup
    global iconpath
    global mon

    set node "arrows:$count"

    set frm [ frame .arrowicon$count]
    set check [checkbutton $frm.check \
		-variable GmArrows::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo aico -file "$iconpath/module-d.rast.arrow.gif"
    set ico [label $frm.ico -image aico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"
    
    pack $check $ico -side left
        
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
    	-text  "arrows $count"\
		-window $frm -drawcross auto  
    
    set opt($count,1,_check) 1 
    set dup($count) 0

 	set opt($count,1,map) "" 
	set opt($count,1,opacity) 1.0
    set opt($count,1,type) "grass" 
    set opt($count,1,arrow_color) "green" 
    set opt($count,1,grid_color) "grey" 
    set opt($count,1,x_color) "black" 
	set opt($count,1,unknown_color) "red" 
    set opt($count,1,skip) 1
    set opt($count,1,magnitude_map) "" 
    set opt($count,1,scale) 1.0 
    set opt($count,1,mod) 1

	set optlist {_check map opacity type arrow_color grid_color x_color unknown_color \
    	skip magnitude_map scale} 

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
proc GmArrows::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

proc GmArrows::select_map { id } {
    variable tree
    variable node
    global mon
    
    set m [GSelect cell title [G_msg "Aspect map"] parent "."]
    if { $m != "" } { 
        set GmArrows::opt($id,1,map) $m
        GmTree::autonamel [format [G_msg "arrows for %s"] $m]
    }
}

proc GmArrows::select_magmap { id } {
    variable tree
    variable node
    global mon
    
    set m [GSelect cell title [G_msg "Slope/intensity map"] parent "."]
    if { $m != "" } { 
        set GmArrows::opt($id,1,magnitude_map) $m
        GmTree::autonamel [format [G_msg "arrows for %s"] $m]
    }
}

###############################################################################
# display histogram options
proc GmArrows::options { id frm } {
    variable opt
    global bgcolor
    global iconpath

    # Panel heading
    set row [ frame $frm.heading1 ]
    Label $row.a -text [G_msg "Display arrows whose orientations are based on raster aspect map"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    set row [ frame $frm.heading2 ]
    Label $row.a -text [G_msg "  (optionally, arrow lengths are based on slope or intensity map)"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmArrows::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # raster map for arrow direction
    set row [ frame $frm.map ]
    Label $row.a -text [G_msg "Aspect map: "]
    Button $row.b -image [image create photo -file "$iconpath/gui-rastarrowsdir.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmArrows::select_map $id"
    Entry $row.c -width 35 -text " $opt($id,1,map)" \
          -textvariable GmArrows::opt($id,1,map) 
    Label $row.d -text "   "
    Button $row.e -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q d.rast.arrow" \
		-background $bgcolor -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # map type
    set row [ frame $frm.type ]
    Label $row.a -text [G_msg "    aspect value type"]
    ComboBox $row.b -padx 2 -width 8 -textvariable GmArrows::opt($id,1,type) \
		-values {"grass" "compass" "agnps" "answers"} 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
		
    # skip factor
    set row [ frame $frm.skip ]
    Label $row.a -text [G_msg "    draw arrows every Nth grid cell"]
    SpinBox $row.b -range {1 200 1} -textvariable GmArrows::opt($id,1,skip) \
		-width 4  
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # arrow and grid color
    set row [ frame $frm.color1 ]
    Label $row.a -text [G_msg "    arrow color      "]
    ComboBox $row.b -padx 2 -width 10 -textvariable GmArrows::opt($id,1,arrow_color) \
		-values {"white" "grey" "gray" "black" "brown" "red" "orange" \
		"yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" \
		"magenta"} 
    Label $row.c -text [G_msg "     cell grid color"]
    ComboBox $row.d -padx 2 -width 10 -textvariable GmArrows::opt($id,1,grid_color) \
		-values {"none" "white" "grey" "gray" "black" "brown" "red" "orange" \
		"yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" \
		"magenta"} 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
    # x and unknown color
    set row [ frame $frm.color2 ]
    Label $row.a -text [G_msg "    null value color"]
    ComboBox $row.b -padx 2 -width 10 -textvariable GmArrows::opt($id,1,x_color) \
		-values {"white" "grey" "gray" "black" "brown" "red" "orange" \
		"yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" \
		"magenta"} 
    Label $row.c -text { 'unknowns' color}
    ComboBox $row.d -padx 2 -width 10 -textvariable GmArrows::opt($id,1,unknown_color) \
		-values {"none" "white" "grey" "gray" "black" "brown" "red" "orange" \
		"yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" \
		"magenta"} 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # raster map for arrow magnitude
    set row [ frame $frm.mag ]
    Label $row.a -text [G_msg "Slope/intensity map: "]
    Button $row.b -image [image create photo -file "$iconpath/gui-rastarrowsint.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmArrows::select_magmap $id"
    Entry $row.c -width 35 -text " $opt($id,1,magnitude_map)" \
          -textvariable GmArrows::opt($id,1,magnitude_map) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
		
    # scale arrow length
    set row [ frame $frm.scale ]
    LabelEntry $row.a -label [G_msg "    scale factor for computing arrow length"] \
		-textvariable GmArrows::opt($id,1,scale) -width 5 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
}

###############################################################################
proc GmArrows::save { tree depth node } {
    variable opt
    variable optlist
    global mon
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
         GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}

###############################################################################
proc GmArrows::display { node mod} {
    global mon
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,map) == "" } { return } 

    set cmd "d.rast.arrow map=$opt($id,1,map) type=$opt($id,1,type) \
    	arrow_color=$opt($id,1,arrow_color) grid_color=$opt($id,1,grid_color) \
    	x_color=$opt($id,1,x_color) unknown_color=$opt($id,1,unknown_color) \
    	skip=$opt($id,1,skip) scale=$opt($id,1,scale)"

    # include nulls
    if { $opt($id,1,magnitude_map) != "" } { 
        append cmd " magnitude_map=$opt($id,1,magnitude_map)"
    }
	
	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
}
    
###############################################################################
proc GmArrows::mapname { node } {
    variable opt
    variable tree
    global mon
    global mapname
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { $opt($id,1,map) == "" } { return } 
    
    set mapname $opt($id,1,map)
    return $mapname
}

###############################################################################
proc GmArrows::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup
    global iconpath
    global mon
    
    set node "arrows:$count"
	set dup($count) 1

    set frm [ frame .arrowicon$count]
    set check [checkbutton $frm.check \
		-variable GmArrows::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo aico -file "$iconpath/module-d.rast.arrow.gif"
    set ico [label $frm.ico -image aico -bd 1 -relief raised]

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
    		-text "arrows $count" -window $frm \
			-drawcross auto
	} else {
	    $tree insert $sellayer $parent $node \
	    	-text "arrows for $opt($id,1,map)" \
			-window $frm -drawcross auto
	}

	set opt($count,1,opacity) $opt($id,1,opacity)

	set optlist {_check map type arrow_color grid_color x_color unknown_color \
    	skip magnitude_map scale} 

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
