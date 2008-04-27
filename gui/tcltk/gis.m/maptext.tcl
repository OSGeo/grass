###############################################################
# maptext.tcl - TclTk canvas postscript text layer options file for GRASS GIS Manager
# February 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmCtext {
    variable array opt # ctext options
    variable placement #entry widget for x,y coordinates
    variable count 1
    variable optlist
}

proc GmCtext::create { tree parent } {
    variable opt
    variable count
	variable dup
	variable optlist
    global iconpath
    global env

    set node "ctext:$count"

    set frm [ frame .ctexticon$count]
    set check [checkbutton $frm.check \
		-variable GmCtext::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo ctico -file "$iconpath/gui-maptext.gif"
    set ico [label $frm.ico -image ctico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "PS text layer $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,1,_check) 1 
	set opt($count,1,opacity) 1.0
    set opt($count,1,text) "" 
    set opt($count,1,at) "100,100"
    set opt($count,1,font) "times 12" 
    set opt($count,1,fill) \#000000 
    set opt($count,1,width)  100
    set opt($count,1,anchor) "center_left" 
    set opt($count,1,justify) "left" 
    set opt($count,1,coordinates) "pixels" 
    set opt($count,1,mouseset) 0
    
    set optlist { _check opacity text at font fill width anchor justify coordinates mouseset}
    
    foreach key $optlist {
		set opt($count,0,$key) $opt($count,1,$key)
    } 

    incr count
    return $node
}

###############################################################################
proc GmCtext::select_font { id frm } {
	global mon
	variable opt
    
    set fon [SelectFont $frm.fontset -type dialog -sampletext [G_msg "This is font sample text."] -title [G_msg "Select label font"]]
	if { $fon != "" } {set opt($id,1,font) $fon}
}

###############################################################################
proc GmCtext::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

###############################################################################

proc GmCtext::mouseset { id } {
	# use mouse to set scalebar placement coordinates
	global mon pctentry pixelentry geogentry
	variable placement
	if { $GmCtext::opt($id,1,mouseset) == 1} {
		if {$GmCtext::opt($id,1,coordinates) == "pixels"} {
			set pixelentry $GmCtext::placement
		} else {
			set pixelentry ""
		}
		if {$GmCtext::opt($id,1,coordinates) == "percent"} {
			set pctentry $GmCtext::placement
		} else {
			set pctentry ""
		}
		if {$GmCtext::opt($id,1,coordinates) == "geographic"} {
			set geogentry $GmCtext::placement
		} else {
			set geogentry ""
		}
	}
}
###############################################################################

# ctext options
proc GmCtext::options { id frm } {
    variable opt
	variable placement
    global iconpath

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text [G_msg "Create postscript text object (for postscript eps, pdf, and print output only)"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # text
    set row [ frame $frm.text ]
    Label $row.a -text [G_msg "Text to display:"]
    LabelEntry $row.b -textvariable GmCtext::opt($id,1,text) -width 45 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
    # coordinates1
    set row [ frame $frm.east_north ]
    set placement [LabelEntry $row.a -label [G_msg "Text placement: x,y coordinates (from upper left) "] \
    	-textvariable GmCtext::opt($id,1,at) -width 24]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
        
    # coordinates2
    set row [ frame $frm.textcoord2 ]
    Label $row.a -text [G_msg "     coordinate type for text placement "] 
    ComboBox $row.b -padx 2 -width 10 -textvariable GmCtext::opt($id,1,coordinates) \
    	-values {"pixels" "percent" "geographic" } -modifycmd "GmCtext::mouseset $id"
    checkbutton $row.c -text [G_msg "place with mouse"] \
    	-variable GmCtext::opt($id,1,mouseset) \
    	-command "GmCtext::mouseset $id"
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
        
    # text options1
    set row [ frame $frm.textopt1 ]
    Label $row.a -text [G_msg "     align text with coordinate point  "] 
    ComboBox $row.b -padx 2 -width 12 -textvariable GmCtext::opt($id,1,anchor) \
		-values {"lower_left" "bottom_center" "lower_right" "center_left" "center" 
		"center_right" "upper_left" "top_center" "upper_right" } 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # text options2
    set row [ frame $frm.textopt2 ]
    Label $row.a -text [G_msg "     justification"] 
    ComboBox $row.b -padx 2 -width 7 -textvariable GmCtext::opt($id,1,justify) \
		-values {"left" "center" "right"} 
    Label $row.c -text [G_msg "  line width"]
    LabelEntry $row.d -textvariable GmCtext::opt($id,1,width) -width 5 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
        
    # select font
    set row [ frame $frm.font ]
    Label $row.a -text [G_msg "Font:"] 
    Button $row.b -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "select font for text"] \
	    -command "GmCtext::select_font $id $frm"
    Entry $row.c -width 15 -text "$opt($id,1,font)" \
	    -textvariable GmCtext::opt($id,1,font) 
    Label $row.d -text [G_msg "  color"] 
    SelectColor $row.e -type menubutton -variable GmCtext::opt($id,1,fill)
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes


}

###############################################################################
proc GmCtext::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}

###############################################################################
proc GmCtext::display { node } {
    variable opt
    variable tree
    variable can
    global mon

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]
    set canvas_w($mon) $MapCanvas::canvas_w($mon)
    set canvas_h($mon) $MapCanvas::canvas_h($mon)
    

    set can($mon) $MapCanvas::can($mon)
    
    
    if {$opt($id,1,_check) == 0 } { return } 

    if { $opt($id,1,text) == "" } { return } 
        
    switch $opt($id,1,anchor) {
    	"lower_left" 	{ set anchor "sw"}
    	"bottom_center" { set anchor "s" }
    	"lower_right" 	{ set anchor "se"}
    	"center_left" 	{ set anchor "w" }
    	"center" 		{ set anchor "center" }
    	"center_right" 	{ set anchor "e" }
    	"upper_left" 	{ set anchor "nw"}
    	"top_center" 	{ set anchor "n" }
    	"upper_right" 	{ set anchor "ne"}
    }
     
    set xcoord [lindex [split $opt($id,1,at) ","] 0]
    set ycoord [lindex [split $opt($id,1,at) ","] 1]	
    
    if {$opt($id,1,coordinates) == "geographic"} {
		set xcoord [MapCanvas::mape2scrx $mon $xcoord]
		set ycoord [MapCanvas::mapn2scry $mon $ycoord]
		$can($mon) create text $xcoord $ycoord \
			-anchor $anchor \
			-justify $opt($id,1,justify) \
			-width $opt($id,1,width) \
			-fill $opt($id,1,fill) \
			-font $opt($id,1,font) \
			-text $opt($id,1,text)
    } elseif {$opt($id,1,coordinates) == "percent"} {
		set xpct [expr ($xcoord/100.0) * $canvas_w($mon)]
		set ypct [expr ($ycoord/100.0) * $canvas_h($mon)]
		$can($mon) create text $xpct $ypct \
			-anchor $anchor \
			-justify $opt($id,1,justify) \
			-width $opt($id,1,width) \
			-fill $opt($id,1,fill) \
			-font $opt($id,1,font) \
			-text $opt($id,1,text)
    } else {
		$can($mon) create text $xcoord $ycoord \
			-anchor $anchor \
			-justify $opt($id,1,justify) \
			-width $opt($id,1,width) \
			-fill $opt($id,1,fill) \
			-font $opt($id,1,font) \
			-text $opt($id,1,text)
	}
}
###############################################################################

proc GmCtext::duplicate { tree parent node id } {
    variable opt
    variable count
	variable dup
	variable optlist
    global iconpath

    set node "ctext:$count"

    set frm [ frame .ctexticon$count]
    set check [checkbutton $frm.check \
		-variable GmCtext::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo ctico -file "$iconpath/gui-maptext.gif"
    set ico [label $frm.ico -image ctico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "PS text layer $count"\
	-window    $frm \
	-drawcross auto  
        
	set opt($count,1,opacity) $opt($id,1,opacity)
    set first 1
    
    foreach key $optlist {
    	set opt($count,1,$key) $opt($id,1,$key)
		set opt($count,0,$key) $opt($count,1,$key)
    } 
	

    incr count
    return $node
}
