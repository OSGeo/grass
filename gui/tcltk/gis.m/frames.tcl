##########################################################################
# frames.tcl - display frames layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmDframe {
    variable array opt # frame current options
    variable count 1
    variable array tree # mon
    variable array lfile # frame
    variable array lfilemask # frame
    variable optlist
    variable first
    variable array dup # layer
}

proc GmDframe::create { tree parent } {
    variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
    variable first
	variable dup
    global mon
    global iconpath

    set node "dframe:$count"

    set frm [ frame .dframeicon$count]
    set check [checkbutton $frm.check \
		-variable GmDframe::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo dfrmico -file "$iconpath/module-d.frame.gif"
    set ico [label $frm.ico -image dfrmico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "frame $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,1,_check) 1 
    set dup($count) 0

    set opt($count,1,frame) "" 
    set opt($count,1,erase) 0 
    set opt($count,1,create) 1 
    set opt($count,1,select) 0 
    set opt($count,1,at) "50,100,0,50" 
    set opt($count,1,mod) 1
    set first 1

	set optlist { _check frame erase create select at }

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

proc GmDframe::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}


# frame options
proc GmDframe::options { id frm } {
    variable opt
    global bgcolor
    global iconpath

    # Panel heading
    set row [ frame $frm.heading1 ]
    Label $row.a -text [G_msg "Divide map display into frames for displaying multiple maps"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # create, select, or erase frames
    set row [ frame $frm.cats ]
    checkbutton $row.a -text [G_msg "create and select frame"] -variable \
        GmDframe::opt($id,1,create) 
    checkbutton $row.b -text [G_msg "select frame"] -variable \
        GmDframe::opt($id,1,select) 
    checkbutton $row.c -text [G_msg "remove all frames   "] -variable \
        GmDframe::opt($id,1,erase) 
    Button $row.d -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.frame" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # frame name
    set row [ frame $frm.frame ]
    Label $row.a -text [G_msg "Frame name (optional): "]
    LabelEntry $row.b -textvariable GmDframe::opt($id,1,frame) -width 40
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # place frame1
    set row [ frame $frm.at1 ]
    Label $row.a -text [G_msg "Set frame borders at 0-100% from lower left corner of display "]
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
    
    # place frame2
    set row [ frame $frm.at2 ]
    Label $row.a -text [G_msg "     set borders (bottom,top,left,right): "]
    LabelEntry $row.b -textvariable GmDframe::opt($id,1,at) -width 25
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
            
}

proc GmDframe::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}


proc GmDframe::display { node mod } {
    global mon
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

    if { $opt($id,1,create) == 0 && $opt($id,1,select) == 0 && $opt($id,1,erase) == 0 } { return } 
    if { $opt($id,1,at) == "" } { return }
    set cmd "d.frame"


    # create
    if { $opt($id,1,create) == 1 } { 
        append cmd " -c"
    }

    # select
    if { $opt($id,1,select) == 1 } { 
        append cmd " -s"
    }

    # erase and remove
    if { $opt($id,1,erase) == 1 } { 
        append cmd " -e"
    }

    # frame name
    if { $opt($id,1,frame) != "" } { 
        append cmd " frame=$opt($id,1,frame)"
    }

    # frame placement
    if { $opt($id,1,at) != "" } { 
        append cmd " at=$opt($id,1,at)"
    }

    # check to see if options have changed
    foreach key $optlist {
        if {$opt($id,0,$key) != $opt($id,1,$key)} {
        	set opt($id,1,mod) 1
        	set opt($id,0,$key) $opt($id,1,$key)
        }
    }

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
}


proc GmDframe::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	global iconpath
	global first

    set node "dframe:$count"
	set dup($count) 1

    set frm [ frame .dframeicon$count]
    set check [checkbutton $frm.check \
		-variable GmDframe::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo dfrmico -file "$iconpath/module-d.frame.gif"
    set ico [label $frm.ico -image dfrmico -bd 1 -relief raised]
    
    pack $check $ico -side left

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

	if { $opt($id,1,frame) == ""} {
    	$tree insert $sellayer $parent $node \
		-text      "frame $count" \
		-window    $frm \
		-drawcross auto
	} else {
    	$tree insert $sellayer $parent $node \
		-text      "$opt($id,1,frame)" \
		-window    $frm \
		-drawcross auto
	}

	set optlist { _check frame erase create select at }

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
