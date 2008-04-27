##########################################################################
#
# commonlayer.tcl
#
# Common layer code for GIS Manager: GUI for GRASS 6 
# Authors: Cedric Shock
# Based in part on histogram.tcl of GIS Manager
#
# April 2006
#
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

# Common layer code for gis.m
# This is code that happens in the namespace of the layer that,
# except for namespace, is the same for many types of layers.
# It could also be code that is just shared in common.

namespace eval GmCommonLayer {
	#pass
}

# Import variables from a namespace into the current stack frame:
proc namespace_import_variables {namespace args} {
	foreach arg $args {
		uplevel "upvar 0 ${namespace}::$arg $arg"
	}
}

# Display some things on the canvas
# (actually just run the command, copy its result to the temporary
# directory and add it to the compositing list)
proc GmCommonLayer::display_commands {namespace id cmds} {
	global mon
	global commandlist
	
	namespace_import_variables $namespace lfile lfilemask opt optlist dup first
	
	set mapfile($mon) $MapCanvas::mapfile($mon)
	set maskfile($mon) $MapCanvas::maskfile($mon)

	if {[info exists $mapfile($mon)] == ""} {return}
	if {![info exists first]} {set first 0}

	# There's no point in doing any of this unless the layer is actually on
	if {! $opt($id,1,_check) } {
		return 0
	}

	if {![info exists opt($id,1,mod)]} {
		set opt($id,1,mod) 0
	}
	
	# check to see if options have changed
	foreach key $optlist {
		if {$opt($id,0,$key) != $opt($id,1,$key)} {
			set opt($id,1,mod) 1
			set opt($id,0,$key) $opt($id,1,$key)
		}
	} 

	# if options have changed (or mod flag set by other procedures) re-render map
	if {$opt($id,1,mod) == 1 || $dup($id) == 1 || $first == 1} {
		foreach cmd $cmds {
			if {$cmd != ""} {
				run_panel $cmd
			}
		}
		# work around MS-Windows TclTk bug:
		# file rename -force returns bad code.
		catch {file delete $lfile($id)}
		catch {file delete $lfilemask($id)}
		catch {file rename -force $mapfile($mon) $lfile($id)}
		catch {file rename -force $maskfile($mon) $lfilemask($id)}
		# reset options changed flag
		set opt($id,1,mod) 0
		set dup($id) 0
		set first 0
	}
	
	if {![file exists $lfile($id)]} {return}
	
	#add lfile, maskfile, and opacity to compositing lists
	if {$MapCanvas::complist($mon) != "" } {
		append MapCanvas::complist($mon) ","
		append MapCanvas::complist($mon) [file tail $lfile($id)]
	} else {
		append MapCanvas::complist($mon) [file tail $lfile($id)]
	}	

	if {$MapCanvas::masklist($mon) != "" } {
		append MapCanvas::masklist($mon) ","
		append MapCanvas::masklist($mon) [file tail $lfilemask($id)]
	} else {
		append MapCanvas::masklist($mon) [file tail $lfilemask($id)]
	}	
	
	if {$MapCanvas::opclist($mon) != "" } {
		append MapCanvas::opclist($mon) ","
		append MapCanvas::opclist($mon) $opt($id,1,opacity)
	} else {
		append MapCanvas::opclist($mon) $opt($id,1,opacity)
	}
	
	# create list of commands for current display
	# used for v.digit background, but could be used for other things
	append commandlist " $cmds"
	
}

# Display something on the canvas
proc GmCommonLayer::display_command {namespace id cmd} {
	GmCommonLayer::display_commands $namespace $id [list $cmd]
}
