##########################################################################
#
# gmlib.tcl
#
# Procedures library for GIS Manager: GUI for GRASS 6
# Author: Michael Barton (Arizona State University)
# with contributions by Glynn Clements, Markus Neteler, Lorenzo Moretti,
# Florian Goessmann, and others
#
# January 2008
#
# COPYRIGHT:	(C) 1999 - 2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################


namespace eval GmLib {
	global array filename ;# mon

}


###############################################################################
#read_moncap

proc GmLib::color { color } {
        
        if {$color == "white"} {
                set r 255
                set g 255
                set b 255
        } else {
                regexp -- {#(..)(..)(..)} $color x r g b
                        
                set r [expr 0x$r ]
                set g [expr 0x$g ]
                set b [expr 0x$b ]
        }
        
        return "$r:$g:$b"
}


###############################################################################
# Deprecated
# Use guarantee_xmon and any run command instead.

proc GmLib::xmon { type cmd } {
	guarantee_xmon

	if { $type == "term" } {
		term_panel $cmd
	} else {
		run_panel $cmd
	}

	return
}

###############################################################################
# Determine if an element already exists

proc GmLib::element_exists {elem name} {
	global devnull
	set exists 1
	
	set failure [catch {exec g.findfile element=$elem file=$name >& $devnull}]

	return [expr {! $failure}]
}

###############################################################################

#open dialog box
proc GmLib::OpenFileBox { } {
	global filename
	global mon

	# thanks for brace tip to suchenwi from #tcl@freenode
	set types [list \
		[list [G_msg "Map Resource File"] [list ".dm" ".dmrc" ".grc"]] \
		[list [G_msg "All Files"] "*"] \
	]

	set filename_new [tk_getOpenFile -parent $Gm::mainwindow -filetypes $types \
		-title [G_msg "Open File"] ]
	if { $filename_new == "" } { return}
	set filename($mon) $filename_new
	GmTree::load $filename($mon)

};

###############################################################################

#save dialog box
proc GmLib::SaveFileBox { } {
	global filename
	global mon
    
	catch {
		if {[ regexp -- {^Untitled_} $filename($mon) r]} {
		    set filename($mon) ""
		}
	}
    
	if { $filename($mon) != "" } {
	    GmTree::save $filename($mon)
	} else {
		set types [list \
		    [list [G_msg "Map Resource File"] {.grc}] \
		    [list [G_msg "DM Resource File"] [list {.dm} {.dmrc}]] \
		    [list [G_msg "All Files"] "*"] \
		]
		set filename($mon) [tk_getSaveFile -parent $Gm::mainwindow -filetypes $types \
		    -title [G_msg "Save File"] -defaultextension .grc]
		    if { $filename($mon) == "" } { return}
		    GmTree::save $filename($mon)
	}
};

###############################################################################

proc GmLib::errmsg { error args } {
	# send error report and optional message (args) to tk_messageBox
	
	set message ""
	
	if { $args != ""} { 
	    set message [join $args]
	    append message ": " 
	 }
    
	tk_messageBox -type ok -icon error -title [G_msg "Error"] \
	    -message "$message[G_msg $error]"
	uplevel 1 return
     
};


