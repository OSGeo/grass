##########################################################################
# dtext.tcl new version - standard text layer options file for GRASS GIS Manager
# November 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmDtext {
    variable array opt # d.text options
    variable count 1
    variable array lfile 
    variable array lfilemask
    variable optlist
    variable first
    variable array dup 
    variable placement 
    variable optlist
    global env
	
}

proc GmDtext::create { tree parent } {
    variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
    variable first
	variable dup
	variable optlist
    global iconpath env
    
    set node "dtext:$count"

    set frm [ frame .texticon$count]
    set check [checkbutton $frm.check \
    	-variable GmDtext::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo textico -file "$iconpath/module-d.text.gif"
    set ico [label $frm.ico -image textico -bd 1 -relief raised]
    
    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "text layer $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,1,_check) 1 
    set dup($count) 0

    set opt($count,_check) 1 
	set opt($count,1,opacity) 1.0
    set opt($count,1,text) "" 
    set opt($count,1,at) "10,10" 
    set opt($count,1,coordinates) "percent" 
    set opt($count,1,mouseset) 0
    set opt($count,1,align) "lower_left" 
    set opt($count,1,line)  10
    set opt($count,1,rotate) 0
	set opt($count,1,font) "" 
    set opt($count,1,bold) 0 
	set opt($count,1,size) 10
    set opt($count,1,color) \#000000 
    set opt($count,1,linespace) 1.25
    set first 1
        
    set optlist { _check text at coordinates mouseset\
    	align line rotate font bold size color linespace}
    
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
proc GmDtext::set_option { node key value } {
    variable opt

    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

##########################################################################
proc GmDtext::set_font { id } {
	variable opt

	if {$GmDtext::opt($id,1,font) != "" } {
		set Gm::dfont $GmDtext::opt($id,1,font)
	}
	Gm::defaultfont dtext
	tkwait variable Gm::dfont
	set GmDtext::opt($id,1,font) $Gm::dfont
	set Gm::dfont ""

}

###############################################################################

proc GmDtext::mouseset { id } {
	# use mouse to set text placement coordinates
	global mon pctentry pixelentry geogentry
	variable placement
	if { $GmDtext::opt($id,1,mouseset) == 1} {
		if {$GmDtext::opt($id,1,coordinates) == "pixels"} {
			set pixelentry $GmDtext::placement
		} else {
			set pixelentry ""
		}
		if {$GmDtext::opt($id,1,coordinates) == "percent"} {
			set pctentry $GmDtext::placement
		} else {
			set pctentry ""
		}
		if {$GmDtext::opt($id,1,coordinates) == "geographic"} {
			set geogentry $GmDtext::placement
		} else {
			set geogentry ""
		}
	}
}

###############################################################################
# dtext options
proc GmDtext::options { id frm } {
    variable opt
	variable placement
	variable first
    global iconpath bgcolor

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text [G_msg "Display text"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmDtext::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # text
    set row [ frame $frm.text ]
    LabelEntry $row.a -label [G_msg "Text to display: "] -textvariable GmDtext::opt($id,1,text) -width 51
    Button $row.b -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.text" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
    # coordinates1
    set row [ frame $frm.east_north ]
    set placement [LabelEntry $row.a -textvariable GmDtext::opt($id,1,at) -width 25 \
    	-label [G_msg "Text placement: x,y coordinates (from upper left) "]]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
        
    # coordinates2
    set row [ frame $frm.textcoord2 ]
    Label $row.a -text [G_msg "     coordinate type for text placement "] 
    ComboBox $row.b -padx 2 -width 10 -textvariable GmDtext::opt($id,1,coordinates) \
    	-values {"pixels" "percent" "geographic" } -modifycmd "GmDtext::mouseset $id"
    checkbutton $row.c -text [G_msg "place with mouse"] \
    	-variable GmDtext::opt($id,1,mouseset) \
    	-command "GmDtext::mouseset $id"
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # alignment
    set row [ frame $frm.textalign ]
    Label $row.a -text [G_msg "     align text with coordinate point  "] 
    ComboBox $row.b -padx 2 -width 12 -textvariable GmDtext::opt($id,1,align) \
		-values {"lower_left" "bottom_center" "lower_right" "center_left" "center" 
		"center_right" "upper_left" "top_center" "upper_right" } 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

	# rotation
    set row [ frame $frm.textrotate ]
    Label $row.a -text [G_msg "     text rotation (degrees)"] 
    set rotation [SpinBox $row.b -range {-360 360 1} -textvariable GmDtext::opt($id,1,rotate) \
		-entrybg white -width 6]
	if {$first==1} {$rotation setvalue @360}
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes	

    # font options1
    set row [ frame $frm.fontopt1 ]
    Label $row.a -text [G_msg "Text options: font "] 
    Button $row.b -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "select font for text"] \
	    -command "GmDtext::set_font $id"
    Label $row.c -text [G_msg "  color"] 
    SelectColor $row.d -type menubutton -variable GmDtext::opt($id,1,color)
    checkbutton $row.e -padx 10 -text [G_msg "bold text"] -variable \
        GmDtext::opt($id,1,bold) 
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # font options3
    set row [ frame $frm.fontopt3 ]
    LabelEntry $row.a -label [G_msg "     text height in pixels "]\
    	-textvariable GmDtext::opt($id,1,size) -width 6
    LabelEntry $row.b -label [G_msg "  line spacing"]\
    	-textvariable GmDtext::opt($id,1,linespace) -width 6
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

}

###############################################################################
proc GmDtext::save { tree depth node } {
    variable opt
    variable optlist
    global mon

    set id [GmTree::node_id $node]

    foreach key $optlist {
		GmTree::rc_write $depth "$key $opt($id,1,$key)"
    }
}

###############################################################################
proc GmDtext::display { node mod } {
    global mon env
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count
    variable first
    
 	set line ""
    set input ""
    set cmd ""

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    # set hex colors to rgb         
    set color [GmLib::color $opt($id,1,color)]
    

    if { ! ( $opt($id,1,_check) ) } { return } 

    if { $opt($id,1,text) == "" } { return } 

    switch $opt($id,1,align) {
    	"lower_left" 	{ set align "ll"}
    	"bottom_center" { set align "lc" }
    	"lower_right" 	{ set align "lr"}
    	"center_left" 	{ set align "cl" }
    	"center" 		{ set align "cc" }
    	"center_right" 	{ set align "cr" }
    	"upper_left" 	{ set align "ul"}
    	"top_center" 	{ set align "uc" }
    	"upper_right" 	{ set align "ur"}
    }
    
    if {$opt($id,1,coordinates) == "percent"} {
		set atlist [split $opt($id,1,at) ","]
		set xcoord [lindex $atlist 0]
		set ycoord [expr 100 - [lindex $atlist 1]]
		set at "$xcoord,$ycoord"
    } else {
    	set at $opt($id,1,at)
    }
  
    set cmd "d.text -s size=$opt($id,1,size) color=$color \
    	at=$at align=$align  rotation=$opt($id,1,rotate) \
    	linespacing=$opt($id,1,linespace) --q {text=$opt($id,1,text)}"
  
    # bold text
    if { $opt($id,1,bold) != 0 } { 
        append cmd " -b"
    }
    
    # coordinates in pixels or geographic coordinates
    if {$opt($id,1,coordinates) == "pixels"} {
    	append cmd " -p"    
    } elseif {$opt($id,1,coordinates) == "geographic"} {
    	append cmd " -g"
    }
    

    # check value of GRASS_FONT variable prior to display
	if {![catch {set env(GRASS_FONT)}]} {
		set currfont $env(GRASS_FONT)
	} else {
		set currfont "romans"
	}

    # set grass font environmental variable to user selection"
	if { $GmDtext::opt($id,1,font) != ""} { set env(GRASS_FONT) $GmDtext::opt($id,1,font) }

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
	
	# set grass font environmental variable to whatever it was when we started
	# this lets different text layers have different fonts
	
	set env(GRASS_FONT) $currfont

}

###############################################################################
# text duplicate layer

proc GmDtext::duplicate { tree parent node id } {
    variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
    variable first
	variable dup
    global iconpath

    set node "dtext:$count"
    set dup($count) 1

    set frm [ frame .texticon$count]
    set check [checkbutton $frm.check \
    	-variable GmDtext::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo textico -file "$iconpath/module-d.text.gif"
    set ico [label $frm.ico -image textico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"
 
    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
		-text  "text layer $count"\
		-window    $frm \
		-drawcross auto  
        
	set opt($count,1,opacity) $opt($id,1,opacity)
    set first 1
        
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
