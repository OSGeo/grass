##########################################################################
# barscale.tcl - barscale layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmBarscale {
    variable array opt # barscale current options
    variable count 1
    variable array lfile # scale
    variable array lfilemask # scale
    variable optlist
    variable first
    variable array dup # layer
    variable placement #LabelEntry widget for scale bar placment coordinates
};


###############################################################################
# create new barscale layer
proc GmBarscale::create { tree parent } {
    variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
    variable first
	variable dup
	variable can
    global mon
    global iconpath

    set node "barscale:$count"

    set frm [ frame .barscaleicon$count]
    set check [checkbutton $frm.check \
                           -variable GmBarscale::opt($count,1,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo scaleico -file "$iconpath/module-d.barscale.gif"
    set ico [label $frm.ico -image scaleico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "scale $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,1,_check) 1 
    set dup($count) 0

	set opt($count,1,opacity) 1.0
    set opt($count,1,tcolor) \#000000 
    set opt($count,1,bcolor) \#FFFFFF 
    set opt($count,1,bcolor_none) 0
    set opt($count,1,font) ""
    set opt($count,1,line) 0 
    set opt($count,1,at) "2,2" 
    set opt($count,1,feet) 0 
    set opt($count,1,top) 0 
    set opt($count,1,arrow) 0 
    set opt($count,1,scale) 0 
    set opt($count,1,mod) 1
    set opt($count,1,mouseset) 0
    set first 1
        
    set optlist { _check opacity bcolor bcolor_none font tcolor at feet line top arrow scale mouseset}
    
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
};

###############################################################################
proc GmBarscale::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value

};


###############################################################################

proc GmBarscale::mouseset { id } {
	# use mouse to set scalebar placement coordinates
	global mon pctentry
	variable placement

	if { $GmBarscale::opt($id,1,mouseset) == 1 } {
		set pctentry $GmBarscale::placement
	} else {
		set pctentry ""
	}

};

##########################################################################
proc GmBarscale::set_font { id } {
	variable opt

	if {$GmBarscale::opt($id,1,font) != "" } {
		set Gm::dfont $GmBarscale::opt($id,1,font)
	}
	Gm::defaultfont dbarscale
	tkwait variable Gm::dfont
	set GmBarscale::opt($id,1,font) $Gm::dfont
	set Gm::dfont ""

};


###############################################################################
# barscale options
proc GmBarscale::options { id frm } {
    variable opt    
    variable placement
    global bgcolor
    global iconpath
    global mon

    # Panel heading
    set row [ frame $frm.heading1 ]
    Label $row.a -text [G_msg "Display scale and north arrow"] \
    	-fg MediumBlue    
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmBarscale::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # at1
    set row [ frame $frm.at1 ]
    Label $row.a -text [G_msg "Scale placement: 0-100% from top left of display"]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
        
    # at2
    set row [ frame $frm.at2 ]
    Label $row.a -text [G_msg "\t enter x,y of scale/arrow upper left corner"]
    set placement [LabelEntry $row.b -width 8 \
    	-textvariable GmBarscale::opt($id,1,at)]
    Label $row.c -text [G_msg "    "]
    Button $row.d -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.barscale" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # at3
    set row [ frame $frm.at3 ]
    Label $row.a -text [G_msg "\t "]
    checkbutton $row.b -text [G_msg "place with mouse"] \
    	-variable GmBarscale::opt($id,1,mouseset) \
    	-command "GmBarscale::mouseset $id"
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # color
    set row [ frame $frm.color ]
    Label $row.a -text [G_msg "Scale appearance:  text color"] 
    SelectColor $row.b -type menubutton -variable GmBarscale::opt($id,1,tcolor)
    Label $row.c -text [G_msg "  font "]
    Button $row.d -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "select font for text"] \
	    -command "GmBarscale::set_font $id"
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
    # background
    set row [ frame $frm.background ]
    Label $row.a -text [G_msg "\tbackground color "] 
    SelectColor $row.b -type menubutton -variable GmBarscale::opt($id,1,bcolor)
    Label $row.c -text "   " 
    checkbutton $row.d -text [G_msg "transparent background"] \
    	-variable GmBarscale::opt($id,1,bcolor_none) 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # arrow or scale only
    set row [ frame $frm.arrow ]
    Label $row.a -text "\t "
    checkbutton $row.b -text [G_msg "display N. arrow only"] \
    	-variable GmBarscale::opt($id,1,arrow) 
    checkbutton $row.c -text [G_msg "display scale only"] \
    	-variable GmBarscale::opt($id,1,scale) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # text on top
    set row [ frame $frm.textontop ]
    Label $row.a -text "\t " 
    checkbutton $row.b -text [G_msg "text on top of scale, instead of to right"] \
    	-variable GmBarscale::opt($id,1,top) 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # scale options
    set row [ frame $frm.opts ]
    Label $row.a -text "\t " 
    checkbutton $row.b -text [G_msg "line scale instead of bar"] \
    	-variable GmBarscale::opt($id,1,line) 
    checkbutton $row.c -text [G_msg "use feet/miles instead of meters"] \
    	-variable GmBarscale::opt($id,1,feet) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

};

###############################################################################
# save barscale layer node to grc file
proc GmBarscale::save { tree depth node } {
    variable opt
    variable optlist
    global mon
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
};


###############################################################################
# render and composite barscale layer

proc GmBarscale::display { node mod } {
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

 	set line ""
    set input ""
    set cmd ""

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    # set hex colors to rgb         
    set tcolor [GmLib::color $opt($id,1,tcolor)]
    set bcolor [GmLib::color $opt($id,1,bcolor)]

    # no background color
    if { $opt($id,1,bcolor_none) == 1 } { 
        set bcolor "none"
    }
    
    set cmd "d.barscale tcolor=$tcolor bcolor=$bcolor at=$opt($id,1,at)"

    # line scale
    if { $opt($id,1,line) != 0 } { 
        append cmd " -l"
    }

    # text on top
    if { $opt($id,1,top) != 0 } { 
        append cmd " -t"
    }

    # english units
    if { $opt($id,1,feet) != 0} { 
        append cmd " -f"
    }

	# arrow only
	if { $opt($id,1,arrow) != 0 } {
		append cmd " -n"
	}
	
	# scale only
	if { $opt($id,1,scale) != 0 } {
		append cmd " -s"
	}

    # check value of GRASS_FONT variable prior to display
	if {![catch {set env(GRASS_FONT)}]} {
		set currfont $env(GRASS_FONT)
	} else {
		set currfont "romans"
	}
	
    # set grass font environmental variable to user selection"
	if { $GmBarscale::opt($id,1,font) != ""} { 
		set env(GRASS_FONT) $GmBarscale::opt($id,1,font) 
	}

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
	
	# set grass font environmental variable to whatever it was when we started
	# this lets different text layers have different fonts
	
	set env(GRASS_FONT) $currfont
	
};

###############################################################################
#duplicate barscale layer

proc GmBarscale::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	variable first
	global iconpath

    set node "barscale:$count"
	set dup($count) 1

    set frm [ frame .barscaleicon$count]
    set check [checkbutton $frm.check \
		-variable GmBarscale::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo scaleico -file "$iconpath/module-d.barscale.gif"
    set ico [label $frm.ico -image scaleico -bd 1 -relief raised]
    
    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
		-text      "scale $count" \
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
