##########################################################################
# cmd.tcl - command layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmCmd {
    variable array opt # cmd current options
    variable count 1
    variable array tree # mon
    variable array lfile # command
    variable array lfilemask # command
    variable optlist
    variable array dup # layer
}


proc GmCmd::create { tree parent } {
   variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
	variable dup
    global mon
    global iconpath

    set node "cmd:$count"

    set frm [ frame .cmdicon$count]
    set check [checkbutton $frm.check \
		-variable GmCmd::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo cico -file "$iconpath/gui-cmd.gif"
    set ico [label $frm.ico -image cico -bd 1 -relief raised]
    
    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text      "cmd $count" \
	-window    $frm \
	-drawcross auto 

    set opt($count,1,_check) 1 
    set dup($count) 0

	set opt($count,1,opacity) 1.0
    set opt($count,1,cmd) ""
    set opt($count,1,mod) 1

	set optlist { _check opacity cmd}

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

proc GmCmd::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

# display cmd options
proc GmCmd::options { id frm } {
    variable opt

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text "Enter any GRASS command" \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmCmd::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # cmd name
    set row [ frame $frm.name ]
    Label $row.a -text [G_msg "Command:"] 
    Entry $row.b -width 50 -text "$opt($id,1,cmd)" \
          -textvariable GmCmd::opt($id,1,cmd) 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

proc GmCmd::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}

proc GmCmd::display { node mod} {
    global mon
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable rasttype
    variable tree
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,cmd) == "" } { return } 

    set cmd $opt($id,1,cmd)

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
}

proc GmCmd::duplicate { tree parent node id} {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	global iconpath

    set node "cmd:$count"
	set dup($count) 1

    set frm [ frame .cmdicon$count]
    set check [checkbutton $frm.check \
		-variable GmCmd::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo cico -file "$iconpath/gui-cmd.gif"
    set ico [label $frm.ico -image cico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"
 
    pack $check $ico -side left

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
		-text      "cmd $count" \
		-window    $frm \
		-drawcross auto
	set opt($count,1,opacity) $opt($id,1,opacity)

	set optlist { _check cmd}

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
