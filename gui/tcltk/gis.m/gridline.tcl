##########################################################################
# grid.tcl - grid and line overlay layer options file for GRASS GIS Manager
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmGridline {
    variable array opt # grid current options
    variable count 1
    variable array tree # mon
    variable array lfile # raster
    variable array lfilemask # raster
    variable optlist
    variable first
    variable array dup # vector
}

proc GmGridline::create { tree parent } {
    variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
    variable first
	variable dup
    global mon
    global iconpath

    set node "gridline:$count"

    set frm [ frame .gridicon$count]
    set check [checkbutton $frm.check \
		-variable GmGridline::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo gico -file "$iconpath/module-d.grid.gif"
    set gdico [label $frm.gdico -image gico -bd 1 -relief raised]
    
    bind $gdico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $gdico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "gridline $count"\
	-window    $frm \
	-drawcross auto  

    set opt($count,1,_check) 1 
    set dup($count) 0
    
	set opt($count,1,opacity) 1.0
    set opt($count,1,gridline) "gridline" 
    set opt($count,1,gridcolor) \#AAAAAA
    set opt($count,1,gridborder) \#000000 
    set opt($count,1,gridsize) 1000
    set opt($count,1,gridorigin) "0,0" 
    set opt($count,1,griddraw) 1 
    set opt($count,1,gridgeod) 0
    set opt($count,1,borderdraw) 1 
    set opt($count,1,textdraw) 1 
    set opt($count,1,textcolor) #AAAAAA
    set opt($count,1,font) "" 
    set opt($count,1,fontsize) 9
    
    set opt($count,1,rhumbdraw) 0 
    set opt($count,1,rhumbcoor) "" 
    set opt($count,1,rhumbcolor) "black" 
    
    set opt($count,1,geoddraw) 0 
    set opt($count,1,geodcoor) "" 
    set opt($count,1,geodcolor) "black" 
    set opt($count,1,geodtxtcolor) "none" 

    set first 1
    set opt($count,1,mod) 1
    
	set optlist { _check opacity gridcolor gridborder gridsize gridorigin griddraw gridgeod \
    			borderdraw textdraw textcolor font fontsize rhumbdraw rhumbcoor geoddraw \
    			geodcoor geodcolor geodtxtcolor} 

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

proc GmGridline::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

##########################################################################
proc GmGridline::set_font { id } {
	variable opt

	if {$GmGridline::opt($id,1,font) != "" } {
		set Gm::dfont $GmGridline::opt($id,1,font)
	}
	Gm::defaultfont dgrid
	tkwait variable Gm::dfont
	set GmGridline::opt($id,1,font) $Gm::dfont
	set Gm::dfont ""
	
}

##########################################################################

# display gridline options
proc GmGridline::options { id frm } {
    variable opt    
    global bgcolor
    global iconpath

    # Panel heading
    set row [ frame $frm.heading1 ]
    Label $row.a -text [G_msg "Display grid lines, and geodesic lines or rhumblines"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmGridline::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # grid options 1
    set row [ frame $frm.grid1 ]
    Label $row.a -text [G_msg "Grid options: "]
    checkbutton $row.b -text [G_msg "draw grid"] -variable GmGridline::opt($id,1,griddraw) 
    checkbutton $row.c -text [G_msg "geodetic grid  "] -variable GmGridline::opt($id,1,gridgeod) 
    SelectColor $row.d -type menubutton -variable GmGridline::opt($id,1,gridcolor)    
    Label $row.e -text [G_msg "grid color "] 
    Button $row.f -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.grid" \
            -background $bgcolor \
            -helptext [G_msg "Help for grids"]
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left
    pack $row -side top -fill both -expand yes

    # grid options 4
    set row [ frame $frm.grid4 ]
    Label $row.a -text [G_msg "    grid size (map units)"]
    LabelEntry $row.b -textvariable GmGridline::opt($id,1,gridsize) -width 7
    Label $row.c -text [G_msg " grid origin (east, north)"]
    LabelEntry $row.d -textvariable GmGridline::opt($id,1,gridorigin) -width 15
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
    # grid options 2
    set row [ frame $frm.grid2 ]
    Label $row.a -text "   "
    checkbutton $row.b -text [G_msg "draw border text "] -variable GmGridline::opt($id,1,textdraw) 
    SelectColor $row.c -type menubutton -variable GmGridline::opt($id,1,textcolor)    
    Label $row.d -text [G_msg "text color "] 
    Button $row.e -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "select font for text"] \
	    -command "GmGridline::set_font $id"
    Label $row.f -text [G_msg "text font "]
    Label $row.g -text [G_msg "  text size"]
    SpinBox $row.h -range {1 72 1} -textvariable GmGridline::opt($id,1,fontsize) \
                   -width 2 -helptext [G_msg "Grid text size in points"] 
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g $row.h -side left
    pack $row -side top -fill both -expand yes

    # grid options 3
    set row [ frame $frm.grid3 ]
    Label $row.a -text "   "
    checkbutton $row.b -text [G_msg "draw grid border "] -variable GmGridline::opt($id,1,borderdraw) 
    SelectColor $row.c -type menubutton -variable GmGridline::opt($id,1,gridborder)
    Label $row.d -text [G_msg "border color "] 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    set row [ frame $frm.line ]
    Label $row.a -text [G_msg "Geodesic and rhumblines for latlong locations only"]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # geodesic line options 1
    set row [ frame $frm.geod1 ]
    Label $row.a -text "     "
    checkbutton $row.b -text [G_msg "draw geodesic line"] -variable GmGridline::opt($id,1,geoddraw)
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.geodesic" \
            -background $bgcolor \
            -helptext [G_msg "Help for geodesic lines"]
    Label $row.d -text [G_msg " line color"]
    ComboBox $row.e -padx 2 -width 7 -textvariable GmGridline::opt($id,1,geodcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"}
    Label $row.f -text [G_msg " text color"]
    ComboBox $row.g -padx 2 -width 7 -textvariable GmGridline::opt($id,1,geodtxtcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"}
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes
    
    # geodesic line options 2
    set row [ frame $frm.geod2 ]
    Label $row.a -text [G_msg "     line endpoints (x1,y1,x2,y2)"]
    LabelEntry $row.b -textvariable GmGridline::opt($id,1,geodcoor) -width 35 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # rhumbline options 1
    set row [ frame $frm.rhumb1 ]
    Label $row.a -text "     "
    checkbutton $row.b -text [G_msg "draw rhumbline"] -variable GmGridline::opt($id,1,rhumbdraw)
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.rhumbline" \
            -background $bgcolor \
            -helptext [G_msg "Help for rhumblines"]
    Label $row.d -text [G_msg " line color"]
    ComboBox $row.e -padx 2 -width 7 -textvariable GmGridline::opt($id,1,rhumbcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"}
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
    # rhumbline options 2
    set row [ frame $frm.rhumb2 ]
    Label $row.a -text [G_msg "     line endpoints (x1,y1,x2,y2)"]
    LabelEntry $row.b -textvariable GmGridline::opt($id,1,rhumbcoor) -width 35 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

##########################################################################
proc GmGridline::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]


    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"     

    }                     
}

##########################################################################
proc GmGridline::display { node mod } {
    global mon
    global env
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count
    variable first

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    set cmd ""
    set cmd2 ""
    set cmd3 ""

    if { ! ( $opt($id,1,_check) ) } { return } 
    
    # set hex colors to rgb         
    set gridcolor [GmLib::color $opt($id,1,gridcolor)]
    set gridborder [GmLib::color $opt($id,1,gridborder)]
    set txtcolor [GmLib::color $opt($id,1,textcolor)]

    
    # d.grid command
    if { $opt($id,1,griddraw) || $opt($id,1,borderdraw) } {
            set cmd "d.grid size=$opt($id,1,gridsize) origin=$opt($id,1,gridorigin) \
                color=$gridcolor bordercolor=$gridborder textcolor=$txtcolor \
                fontsize=$opt($id,1,fontsize)" 
        } 
        
    if { $opt($id,1,gridgeod) && $cmd != "" } {append cmd " -g"} 
    if { !$opt($id,1,griddraw) && $cmd != "" } {append cmd " -n"} 
    if { !$opt($id,1,borderdraw) && $cmd != "" } {append cmd " -b"}
    if { !$opt($id,1,textdraw) && $cmd != "" } {append cmd " -t"}
        
    # d.geodesic command
    if { $opt($id,1,geoddraw) } {
        set cmd2 "d.geodesic coor=$opt($id,1,geodcoor) \
                lcolor=$opt($id,1,geodcolor) \
                tcolor=$opt($id,1,geodtxtcolor)"  
    }

    # d.rhumbline command
    if { $opt($id,1,rhumbdraw) } {
        set cmd3 "d.rhumbline coor=$opt($id,1,rhumbcoor) \
	       lcolor=$opt($id,1,rhumbcolor) " 
    }

    # check value of GRASS_FONT variable prior to display
	if {![catch {set env(GRASS_FONT)}]} {
		set currfont $env(GRASS_FONT)
	} else {
		set currfont "romans"
	}

    # set grass font environmental variable to user selection"
	if { $GmGridline::opt($id,1,font) != ""} { 
		set env(GRASS_FONT) $GmGridline::opt($id,1,font) 
	}

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_commands [namespace current] $id [list $cmd $cmd2 $cmd3]
	
	# set grass font environmental variable to whatever it was when we started
	# this lets different text layers have different fonts
	
	set env(GRASS_FONT) $currfont

}


##########################################################################
proc GmGridline::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	variable first
	global iconpath

    set node "gridline:$count"
	set dup($count) 1

    set frm [ frame .gridlineicon$count]
    set check [checkbutton $frm.check \
		-variable GmGridline::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo gico -file "$iconpath/module-d.grid.gif"
    set gdico [label $frm.gdico -image gico -bd 1 -relief raised]
    
    pack $check $gdico -side left


	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

     $tree insert $sellayer $parent $node \
	-text  "gridline $count"\
	-window    $frm \
	-drawcross auto  

	set opt($count,1,opacity) $opt($id,1,opacity)

	set optlist { _check gridcolor gridborder gridsize gridorigin griddraw gridgeod \
    			borderdraw textdraw rhumbdraw rhumbcoor geoddraw geodcoor geodcolor \
    			geodtxtcolor} 

    foreach key $optlist {
    	set opt($count,1,$key) $opt($id,1,$key)
		set opt($count,0,$key) $opt($count,1,$key)
    } 

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
