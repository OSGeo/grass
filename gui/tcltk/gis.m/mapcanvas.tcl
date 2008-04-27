##########################################################################
#
# MapCanvas.tcl -TclTk canvas display monitors	and display controls
#	 for GIS Manager: GUI for GRASS 6
#
# Author: Michael Barton (Arizona State University) & Cedric Shock
#
# January 2006
#
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#				This program is free software under the GNU General Public
#				License (>=v2). Read the file COPYING that comes with GRASS
#				for details.
#
##########################################################################


# All of these must be sourced before using mapcanvas.tcl:
# source $gmpath/gmtree.tcl
# source $env(GISBASE)/etc/gtcltk/gmsg.tcl
# source $env(GISBASE)/etc/gtcltk/select.tcl
# source $env(GISBASE)/etc/gui.tcl
# This one is going to be handled by pkgIndex:
# source $gmpath/maptool.tcl

namespace eval MapCanvas {
	variable array displayrequest ;# Indexed by mon, true if it wants to get displayed.

	# Something's modified the canvas or view, indexed by mon.
	# Degree of modification 0 - none, 1 - zoom, 2 - canvas
	variable array canmodified

	variable array can ;# The canvas widgets of the monitors, indexed by mon
	variable array mapframe ;# Frame widgets, indexed by mon
	variable array canvas_w ;# Width and height of canvas. Indexed by mon
	variable array canvas_h ;# mon
	variable array driver_w ;# Actual width and height used while drawing / compositing. Indexed by mon
	variable array driver_h ;# Actual width and height used while drawing / compositing. Indexed by mon
	variable array exploremode ;# Whether or not to change regions to match monitor, indexed by mon
	variable array map_ind ;# Indicator widgets, indexed by mon
	variable array msg ;# status message, indexed by mon
	variable b1north ;# capture north coordinate on clicking mouse
	variable b1east ;# capture east coordinate on clicking mouse

	
	# zoom box corners indexed by mon
	variable array areaX1 
	variable array areaY1 
	variable array areaX2 
	variable array areaY2
	
	# pan coordinates
	variable start_x 
	variable start_y
	variable from_x 
	variable from_y
	variable to_x 
	variable to_y
	
	# measure tool coordinates
	variable mlength 
	variable totmlength
	variable linex1 
	variable liney1 
	variable linex2 
	variable liney2
	
	# There is a global coords # Text to display in indicator widget, indexed by mon
	
	# Process ID for temp files
	variable mappid
	# Driver output file (.ppm)
	variable array mapfile 
	# Driver output mask (.pgm)
	variable array maskfile 
	# g.pnmcomp output file (
	variable array outfile 
	# List of files to composite
	variable array complist 
	# Their opacities
	variable array opclist 
	# Their masks
	variable array masklist 

	global geoentry "" ;# variable holds path of entry widgets that use coordinates from canvas

	# Current region and region historys
	# Indexed by mon, history (1 (current) - zoomhistories), part (n, s, e, w, nsres, ewres, rows, cols).
	variable array monitor_zooms
	# Depth of zoom history to keep
	variable zoomhistories
	set zoomhistories 7
	
	# zooming and coordinate conversion variables indexed by mon
	variable array north_extent
	variable array south_extent
	variable array east_extent
	variable array west_extent
	variable array map_ew
	variable array map_ns
	variable array scr_lr
	variable array scr_tb
	variable array scr_top
	variable array scr_bottom
	variable array scr_right
	variable array scr_left
	variable array map2scrx_conv
	variable array map2scry_conv

	# Regular order for region values in a list representing a region or zoom
	# zoom_attrs used in g.region command to set WIND file
	variable zoom_attrs
	set zoom_attrs {n s e w nsres ewres rows cols}

	# string with region information to show in status bar
	variable regionstr

	# This variable keeps track of which monitor set the gism driver settings last.
	# They must always be redone if the monitor was different
	variable previous_monitor
	set previous_monitor {none}
	
	# default cursor for display canvas
	variable mapcursor

	# Current projection and zone for dynamic region setting for displays
}

set regionstr ""

###############################################################################

# Create window and canvas for display
proc MapCanvas::create { } {
	variable canvas_w
	variable canvas_h
	variable mapfile
	variable maskfile
	variable outfile
	variable complist
	variable opclist
	variable masklist
	variable mapframe
	variable can
	variable map_ind
	variable exploremode
	variable mapregion
	variable regionstr
	variable msg
	variable mapcursor
	variable b1east 
	variable b1north 
	variable mappid
	global drawprog
	global tmpdir
	global env
	global mon

	# Initialize window and map geometry

	set canvas_w($mon) 640.0
	set canvas_h($mon) 480.0
	set env(GRASS_WIDTH) 640.0
	set env(GRASS_HEIGHT) 480.0
	set drawprog 0
	# Explore mode is off by default
	set exploremode($mon) 0

	# Make sure that we are using the WIND file for everything except displays
	if {[info exists env(WIND_OVERRIDE)]} {unset env(WIND_OVERRIDE)}

	# Set display geometry to the current region settings (from WIND file)
	MapCanvas::zoom_gregion $mon

	# Create canvas monitor as top level mainframe
	toplevel .mapcan($mon)

	set mapframe($mon) [MainFrame .mapcan($mon).mf \
			-textvariable MapCanvas::msg($mon) \
			-progressvar drawprog -progressmax 100 -progresstype incremental]

	set mf_frame [$mapframe($mon) getframe]

	# toolbar creation
	set map_tb	[$mapframe($mon) addtoolbar]
	MapToolBar::create $map_tb

	# canvas creation
	set can($mon) [canvas $mf_frame.mapcanvas \
		-borderwidth 0 -closeenough 10.0 -relief groove \
		-width $canvas_w($mon) -height $canvas_h($mon) ]

	# setting geometry
	place $can($mon) -in $mf_frame -x 0 -y 0 -anchor nw
	pack $can($mon) -fill both -expand yes

	# indicator creation
	set map_ind($mon) [$mapframe($mon) addindicator -textvariable coords($mon) \
		-width 33 -justify left -padx 5 -bg white]

	pack $mapframe($mon) -fill both -expand yes

	# set default cursor
	set mapcursor [$can($mon) cget -cursor]

	MapCanvas::coordconv $mon

	# set tempfile for ppm output
	set mappid [pid]
	if {[catch {set mapfile($mon) [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
	set maskfile($mon) $mapfile($mon)
	append mapfile($mon) ".ppm"
	append maskfile($mon) ".pgm"

	# set tempfile and tmp directory path for composite output
	set mappid [pid]
	if {[catch {set outfile($mon) [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
	
	set tmpdir [file dirname $outfile($mon)]
	set outfile($mon) [file tail $outfile($mon)]
	append outfile($mon) ".ppm"

	set complist($mon) ""
	set opclist($mon) ""
	set masklist($mon) ""

	# bindings for display canvas
	# mouse handlers
	# The coordinate transforms should be done per monitor.


	# When a monitor gets the keyboard focus
	# switch monitors in the tree if this isn't the selected one
	bind .mapcan($mon) <FocusIn> "
		global mon env
		if { \$mon != $mon } {
			set mon $mon
			GmTree::switchpage $mon
		} "

	# Displays geographic coordinates in indicator window when cursor moved across canvas
	bind $can($mon) <Motion> {
		set scrxmov %x
		set scrymov %y
		set eastcoord [eval MapCanvas::scrx2mape $mon %x]
		set northcoord [eval MapCanvas::scry2mapn $mon %y]
		set coords($mon) "$eastcoord $northcoord"
	}


	# TSW - inserting key command ability into gis.m
	# 9 May 2006
	# set some key commands to speed use

	# Redraw changes
	bind .mapcan($mon) <KeyPress-c> {
		MapCanvas::request_redraw $mon 0
	}
	# Zoom to current and redraw everything
	bind .mapcan($mon) <KeyPress-space> {
		MapCanvas::zoom_current $mon
	}
	# Return to previous zoom
	bind .mapcan($mon) <KeyPress-r> {
		MapCanvas::zoom_back $mon
	}
	# Set explore mode
	bind .mapcan($mon) <KeyPress-e> {
		MapCanvas::exploremode $mon 1
	}
	# set strict mode
	bind .mapcan($mon) <KeyPress-s> {
		MapCanvas::exploremode $mon 0
	}

	# set key strokes to change between tools
	# I've provided strokes for both right and left handed
	# mouse users

	# Right handed
	# x - pointer
	# Zoom in - zoom in
	# zoom ouT - zoom out
	# pAn - pan
	# Query - query
	# Distance - measure

	bind .mapcan($mon) <KeyPress-x> {
		MapToolBar::changebutton pointer
		MapCanvas::stoptool $mon
	}
	bind .mapcan($mon) <KeyPress-z> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton zoomin
		MapCanvas::zoombind $mon 1
	}
	bind .mapcan($mon) <KeyPress-t> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton zoomout
		MapCanvas::zoombind $mon -1
	}
	bind .mapcan($mon) <KeyPress-a> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton pan
		MapCanvas::panbind $mon
	}
	bind .mapcan($mon) <KeyPress-q> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton query
		MapCanvas::querybind $mon
	}
	bind .mapcan($mon) <KeyPress-d> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton measure
		MapCanvas::measurebind $mon
	}

	# Left handed
	# poiNter - pointer
	# zoom In - zoom in
	# zoom Out - zoom out
	# Pan - pan
	# ? - query
	# Measure - measure

	bind .mapcan($mon) <KeyPress-n> {
		MapToolBar::changebutton pointer
		MapCanvas::stoptool $mon
	}
	bind .mapcan($mon) <KeyPress-i> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton zoomin
		MapCanvas::zoombind $mon 1
	}
	bind .mapcan($mon) <KeyPress-o> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton zoomout
		MapCanvas::zoombind $mon -1
	}
	bind .mapcan($mon) <KeyPress-p> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton pan
		MapCanvas::panbind $mon
	}
	bind .mapcan($mon) <KeyPress-question> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton query
		MapCanvas::querybind $mon
	}
	bind .mapcan($mon) <KeyPress-m> {
		MapCanvas::stoptool $mon
		MapToolBar::changebutton measure
		MapCanvas::measurebind $mon
	}



	# window configuration change handler for resizing
	bind $can($mon) <Configure> "MapCanvas::do_resize $mon"

	# bindings for closing map display window
	bind .mapcan($mon) <Destroy> "MapCanvas::cleanup $mon %W"

	#set default pointer tool behavior
	MapCanvas::pointer $mon

}

###############################################################################
# Calculate map extents rectangle for a given aspect ratio.

# Sense - 0 means largest no larger, 1 means smallest no smaller
# We will change rectangle 1
proc MapCanvas::shrinkwrap {sense nsew1 ar2 } {
	foreach {n1 s1 e1 w1} $nsew1 {break}

	set ns1 [expr {$n1 - $s1}]
	set ew1 [expr {$e1 - $w1}]

	# Width / height
	# Big aspect ratio is wide, small aspect ratio is tall
	set ar1 [expr { 1.0 * $ew1 / $ns1 }]

	# If rectangle one is wider than rectangle 2.
	# (or rectangle one isn't wider rectangle box 2 and the sense is inverted)
	if {($ar1 > $ar2) ^ $sense} {
		# n1 and s1 are unchanged
		# e1 and w1 must be scaled by ar2
		set rn1 $n1
		set rs1 $s1
		set goal [expr {$ns1 * $ar2}]
		set midpoint [expr {$w1 + $ew1 / 2}]
		set re1 [expr {$midpoint + $goal / 2}]
		set rw1 [expr {$midpoint - $goal / 2}]
	} else {
		# e1 and w1 are unchanged
		# n1 and s1 must be scaled by 1/ar2
		set re1 $e1
		set rw1 $w1
		set goal [expr {$ew1 / $ar2}]
		set midpoint [expr {$s1 + $ns1 / 2}]
		set rn1 [expr {$midpoint + $goal / 2}]
		set rs1 [expr {$midpoint - $goal / 2}]
	}

	set result [list $rn1 $rs1 $re1 $rw1]

	return $result
}


proc MapCanvas::get_mapunits {} {
	# get map units from PROJ_UNITS
	if {![catch {open "|g.proj -p" r} input]} {
	    set key ""
	    set value ""
	    while {[gets $input line] >= 0} {
	    	if { [string equal "XY location (unprojected)" "$line"] } {
	    	    set mapunits "map units"
	    	    break
	    	}
	    	regexp -nocase {^(.*):(.*)$} $line trash key value
	    	set key [string trim $key]
	    	set value [string trim $value]
	    	set prj($key) $value	
	    }
	    if {[catch {close $input} error]} {
			GmLib::errmsg $error [G_msg "g.proj or projection error"]
	    } 
	}
	# Length is calculated from the map canvas arrows
	#  and so is measured & plotted in map units.
	# May already be set above if locn was XY.
	if { ! [ info exist mapunits ] } {
	    set mapunits $prj(units)
	}
	
	return $mapunits
}


###############################################################################
# map display procedures

# draw map using png driver and open in canvas
proc MapCanvas::drawmap { mon } {
	variable canvas_h
	variable canvas_w
	variable can
	variable canmodified
	variable monitor_zooms
	variable previous_monitor
	variable exploremode

	set w [winfo width $can($mon)]
	set h [winfo height $can($mon)]

	# Get whether or not the canvas was modified or zoomed
	# canmodified has levels: 0	 is none, 1 is zoom, 2 is geometry.
	# 1 doesn't require new setting in explore mode
	set mymodified $canmodified($mon)

	# Make sure canvas_h and canvas_w are correct
	if { $canvas_w($mon) != $w || $canvas_h($mon) != $h } {
			# Flag this as a modified canvas
			# Modified canvas is level 2!
			set mymodified 2
			set canvas_w($mon) $w
			set canvas_h($mon) $h
	}

	# Redo the driver settings if the geometry has changed or
	# if we weren't the previous monitor.
	if {$mymodified == 2 || \
		($mymodified && ! $exploremode($mon)) || \
		$previous_monitor != $mon} {
			set canmodified($mon) 0
			set previous_monitor $mon
			# The canvas or view has been modified
			# Redo the map settings to match the canvas
			if {[MapCanvas::driversettings $mon]} {return}
	}

	# Render all the layers
	MapCanvas::runprograms $mon [expr {$mymodified != 0}]
	# Composite them and display
	MapCanvas::composite $mon
}

# set display geometry (from monitor_zooms) and other output settings
proc MapCanvas::driversettings { mon } {
	global env
	global mapset
	variable canvas_h
	variable canvas_w
	variable driver_w
	variable driver_h
	variable mapfile
	variable monitor_zooms
	variable exploremode

	if {$exploremode($mon)} {
		# The driver will make an image just the size of the canvas
		# This is just a little shortcut, the method below would calculate the same
		set driver_h($mon) $canvas_h($mon)
		set driver_w($mon) $canvas_w($mon)
	} else {
		# Calculate what sized image will fit in the canvas
		# get region extents
		foreach {map_n map_s map_e map_w} [MapCanvas::currentzoom $mon] {break}

		set mapwd [expr {abs(1.0 * ($map_e - $map_w))}]
		set mapht [expr {abs(1.0 * ($map_n - $map_s))}]
					   
		# only zoom in to 1x1 cell
		if {$monitor_zooms($mon,1,cols) < 1 || $monitor_zooms($mon,1,rows) < 1} {
			tk_messageBox -type ok -icon info -parent .mapcan($mon) \
				-message [G_msg "Maximum zoom-in reached"]
			MapCanvas::zoom_back $mon
			return 1
		}
		set mapar [expr {$mapwd / $mapht}]

		# Calculate the largest box of the map's aspect ratio no larger than
		# the canvas. First argument 0 is largest no larger.
		set driver_nsew [MapCanvas::shrinkwrap 0 [list $canvas_h($mon) 0 $canvas_w($mon) 0] $mapar]
		# Pull out the values
		foreach {y2 y1 x2 x1} $driver_nsew {break}

		set driver_h($mon) [expr {round ($y2 - $y1)}]
		set driver_w($mon) [expr {round ($x2 - $x1)}]
	}

	#set display environment
	set env(GRASS_WIDTH) "$driver_w($mon)"
	set env(GRASS_HEIGHT) "$driver_h($mon)"
	set env(GRASS_PNGFILE) "$mapfile($mon)"
	set env(GRASS_BACKGROUNDCOLOR) "ffffff"
	set env(GRASS_TRANSPARENT) "TRUE"
	set env(GRASS_PNG_AUTO_WRITE) "TRUE"
	set env(GRASS_TRUECOLOR) "TRUE"
	return 0
}

# Run the programs to clear the map and draw all of the layers
proc MapCanvas::runprograms { mon mod } {
	variable canvas_w
	variable canvas_h
	variable msg
	variable complist
	variable masklist
	variable opclist
	variable mapframe
	variable zoom_attrs
	global env
	global drawprog
	global devnull

	set drawprog 0

	# reset compositing list prior to rendering
	set complist($mon) ""
	set opclist($mon) ""
	set masklist($mon) ""

	set gregion ""

	# Create a settings string to use with GRASS_WIND. This is a real pain!
	# First get the current region values in normal number form (including decimal degrees)
	set values [MapCanvas::currentzoom $mon]
	set options {}
	foreach attr $zoom_attrs value $values {
		if {$attr != "rows" && $attr != "cols"} {
			lappend options "$attr=$value"
		}
	}

	# Now use the region values to get the region printed back out in -p format
	# including lat long now as dd:mm:ss
	set key ""
	if {![catch {open [concat "|g.region" "-up" $options "2> $devnull"] r} input]} {
		while {[gets $input line] >= 0} {
			if { [regexp -nocase {^([a-z]+)\:[ ]+(.*)$} $line trash key value] } {
				set parts($key) $value
			}
		}
		if {[catch {close $input} error]} {
			GmLib::errmsg $error [G_msg "Error setting region"]
		}
		# Finally put this into wind file format to use with GRASS_REGION
		regexp -nocase {^([0-9]+)} $parts(projection) trash parts(projection)

		set gregion "projection:$parts(projection); zone:$parts(zone); north:$parts(north); south:$parts(south); east:$parts(east); west:$parts(west); e-w resol:$parts(ewres);	 n-s resol:$parts(nsres)"
	} 

	set MapCanvas::msg($mon) [G_msg "please wait..."]
	$mapframe($mon) showstatusbar progression

	incr drawprog
	# only use dynamic region for display geometry; use WIND for computational geometry
	set env(GRASS_REGION) $gregion

	

	# Setting the font really only needs to be done once per display start
	incr drawprog
	GmGroup::display "root" $mod
	unset env(GRASS_REGION)
	incr drawprog
}

# composite maps and create canvas
proc MapCanvas::composite {mon } {
	variable driver_w
	variable driver_h
	variable complist
	variable masklist
	variable opclist
	variable outfile
	variable mapframe
	variable can
	global tmpdir
	global drawprog

	$can($mon) delete all
	if {$complist($mon) != ""} {
		set currdir [pwd]
		cd $tmpdir
		
		incr drawprog
		run_panel "g.pnmcomp in=$complist($mon) mask=$masklist($mon) opacity=$opclist($mon) background=255:255:255 width=$driver_w($mon) height=$driver_h($mon) output=$outfile($mon)"

		image create photo mapimg.$mon -file "$outfile($mon)"
		incr drawprog
		$can($mon) create image 0 0 -anchor nw \
				-image "mapimg.$mon" \
				-tag map$mon
		cd $currdir
	}

	GmTree::cvdisplay "root"
	set drawprog 100

	MapCanvas::coordconv $mon
	set drawprog 0
	$mapframe($mon) showstatusbar status
	return

}

###############################################################################
# map display server
# The job of these procedures is to make sure that:
# 1: we are never running more than one update at once.
# 2: we don't do exactly the same update multiple times.

proc MapCanvas::display_server {} {
	variable redrawrequest

	foreach mon [array names redrawrequest] {
		if {$redrawrequest($mon)} {
			# Mark that this monitor no longer wants to be redrawn
			set redrawrequest($mon) 0
			# Redraw the monitor canvas
			MapCanvas::drawmap $mon
		}
	}

	# Do me again in a short period of time.
	# vwait might be appropriate here
	after 100 MapCanvas::display_server
}

# Request a redraw on a monitor
proc MapCanvas::request_redraw {mon modified} {
	variable redrawrequest
	variable canmodified

	set redrawrequest($mon) 1
	set canmodified($mon) $modified
}

# Start the server
after idle MapCanvas::display_server

###############################################################################

proc MapCanvas::do_resize {mon} {
	variable canvas_w
	variable canvas_h
	variable can

	# Get the actual width and height of the canvas
	set w [winfo width $can($mon)]
	set h [winfo height $can($mon)]

	# Only actually resize and redraw if the size is different
	if { $canvas_w($mon) != $w || $canvas_h($mon) != $h } {
		$can($mon) delete map$mon
		MapCanvas::request_redraw $mon 1
	}
}


###############################################################################

# erase to white
proc MapCanvas::erase { mon } {
	variable can

	$can($mon) delete map$mon
	$can($mon) delete all

}

proc MapCanvas::dnviz { mon } {

	variable can
	
	GmDnviz::main $can($mon) $mon

}

###############################################################################

# zoom to current region
proc MapCanvas::zoom_current { mon } {
	variable can

	MapCanvas::zoom_gregion $mon
	$can($mon) delete map$mon
	MapCanvas::request_redraw $mon 1
}

###############################################################################

# zoom to default region
proc MapCanvas::zoom_default { mon } {
	variable can

	MapCanvas::zoom_gregion $mon [list "-d"]
	$can($mon) delete map$mon
	MapCanvas::request_redraw $mon 1
}

###############################################################################

# zoom to selected map
proc MapCanvas::zoom_map { mon } {
	variable can

	set sel [ GmTree::getnode ]
	if { $sel == "" } { 
		tk_messageBox -type ok -icon warning -parent .mapcan($mon) \
			-message [G_msg "You have to select map layer first"] \
			-title [G_msg "No map layer selected"]
		return 
	}

	set type [GmTree::node_type $sel]
	if { $type == "" } { return }

	switch $type {
		"raster" {
			set regtype "raster"
			set map [GmRaster::mapname $sel]
		}
		"rgbhis" {
			set regtype "raster"
			set map [GmRgbhis::mapname $sel]
		}
		"rnums" {
			set regtype "raster"
			set map [GmRnums::mapname $sel]
		}
		"arrows" {
			set regtype "raster"
			set map [GmArrows::mapname $sel]
		}
		"vector" {
			set regtype "vector"
			set map [GmVector::mapname $sel]
		}
		"thematic" {
			set regtype "vector"
			set map [GmThematic::mapname $sel]
		}
		"chart" {
			set regtype "vector"
			set map [GmChart::mapname $sel]
		}
		default {
			return
		}
	}

	if { $regtype=="raster" } {
			MapCanvas::zoom_gregion $mon [list "rast=$map"]
		}
	if { $regtype=="vector" } {
			MapCanvas::zoom_gregion $mon [list "vect=$map"]
		}
		$can($mon) delete map$mon
		MapCanvas::request_redraw $mon 1
}

###############################################################################

# zoom to saved region
proc MapCanvas::zoom_region { mon } {
	variable can

	set reg [GSelect windows parent .mapcan($mon) title "Saved region"]
	if { $reg != "" } {
		MapCanvas::zoom_gregion $mon [list "region=$reg"]
		$can($mon) delete map$mon
		MapCanvas::request_redraw $mon 1
	}
}


###############################################################################
# save current display to named region
proc MapCanvas::save_region {mon} {

	# make dialog window with entry widget
	toplevel .saveregion
	wm title .saveregion [G_msg "Save Region"]
	wm withdraw .saveregion
	set row [frame .saveregion.txt]
	Label $row.a -text [G_msg "Save current display geometry to named region"] \
		-foreground mediumblue
	pack $row.a -side left
	pack $row -side top -fill both -expand yes -padx 10 -pady 10

	set row [frame .saveregion.sventry]
	LabelEntry $row.a -label [G_msg "Enter region name"] \
		-textvariable saveregion \
		-width 20
	pack $row.a -side left
	pack $row -side top -fill both -expand yes -padx 10 -pady 10 \


	#set row [frame .saveregion.sep]
	set sep [Separator .saveregion.sep]
	#pack $row.a -side top
	pack $sep -side top -fill x -expand yes

	set row [frame .saveregion.btn]
	Button $row.a -text [G_msg "Cancel"] \
		-highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-width 6 -anchor center \
		-command {destroy .saveregion}
	Button $row.b -text [G_msg "Save"] \
		-highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-width 6 -anchor center \
		-command {MapCanvas::check_saveregion $mon $saveregion}
	pack $row.a $row.b -side left -expand yes -padx 10
	pack $row -side bottom -fill x -expand yes -pady 5
	wm deiconify .saveregion
}

###############################################################################
# Works with MapCanvas::save_region. Checks to see of named region already exists
# and offers chance to overwrite or rename.
proc MapCanvas::check_saveregion {mon saveregion} {
	global env
	set overwrite 0

	set svfile "$env(GISDBASE)/$env(LOCATION_NAME)/$env(MAPSET)/windows/$saveregion"
	if {[file exists $svfile] } {
		set answer [tk_messageBox -message [format [G_msg "Region file %s already exists.\
			\nDo you want to overwrite it?"] $saveregion] -type yesno -icon question -default no]
		if {$answer=="no"} {return}
		set overwrite 1
	}

	set args "-u save=$saveregion"

	MapCanvas::set_wind $mon $args $overwrite
	destroy .saveregion
}


###############################################################################
# Switch in and out of exploremode
# Pass second argument true to switch in, false to switch out
proc MapCanvas::exploremode { mon boolean } {
	variable exploremode

	# Set the explore mode to yes or no (the input)
	set exploremode($mon) $boolean

	# Request a redraw with a geometry change (not just a zoom change).
	# Flag it at as such (2)
	MapCanvas::request_redraw $mon 2
}

###############################################################################

# stop display management tools
proc MapCanvas::stoptool { mon } {
	variable msg
	variable regionstr
	variable can

	if {[$can($mon) find withtag mline] != 0} {
	$can($mon) delete mline
	}

	if {[$can($mon) find withtag mline] != 0} {
	$can($mon) delete totmline
	}

	# release bindings
	bind $can($mon) <1> ""
	bind $can($mon) <2> ""
	bind $can($mon) <3> ""
	bind $can($mon) <B1-Motion> ""
	bind $can($mon) <ButtonRelease-1> ""

	# reset status display to normal
	set MapCanvas::msg($mon) $regionstr

	MapCanvas::restorecursor $mon

}

###############################################################################
# set bindings for pointer tool
proc MapCanvas::pointer { mon } {
	variable can
	variable b1east
	variable b1north 
	global coords
	global pctentry
	global pixelentry
	global geogentry
	global geoentry
	global llvert llhoriz
	global outfmt_coords
	
	set pctentry ""
	set pixelentry ""
	set geogentry ""

	set mapunits [MapCanvas::get_mapunits]
	if { [string first "degree" $mapunits ] >= 0 } {
	    set outfmt_coords {%.6f}
	} else {
	    set outfmt_coords {%.3f}
	}

	MapCanvas::restorecursor $mon

	bind $can($mon) <ButtonPress-1> {
		global b1coords mon
		global screenpct pctentry pixelentry geoentry geogentry llvert llhoriz
		set b1east [MapCanvas::scrx2mape $mon %x]
		set b1north [MapCanvas::scry2mapn $mon %y]
		set b1coords "$b1east $b1north"
		# grab coordinates at mouse click for georectification
		if { [info exists geoentry] } {
			$geoentry insert 0 $b1coords
		}
		# grab coordinates at mouse click for decorations in displays
		set mapdisp .mapcan($mon).mf.frame.mapcanvas
		set w [winfo width $mapdisp]
		set h [winfo height $mapdisp]
		set xpct [expr int(100 * %x/$w)]
		set ypct [expr int(100 * %y/$h)]
		# insert x and y coordinate pair into entry widget as percents
		if { $pctentry != "" } {
			$pctentry delete 0 end
			$pctentry insert 0 "$xpct,$ypct"
			# object placement marker
			$mapdisp create line %x [expr %y+5] %x [expr %y-5] -width 2
			$mapdisp create line [expr %x-5] %y [expr %x+5] %y -width 2
		}
		#insert x and y coordinate pair into entry as pixel values
		if { $pixelentry != "" } {
			$pixelentry delete 0 end
			$pixelentry insert 0 "%x,%y"
			# object placement marker
			$mapdisp create line %x [expr %y+5] %x [expr %y-5] -width 2
			$mapdisp create line [expr %x-5] %y [expr %x+5] %y -width 2
		}
		#insert x and y coordinate pair into entry as geographic values
		if { $geogentry != "" } {
			$geogentry delete 0 end
			$geogentry insert 0 "$b1east,$b1north"
			# object placement marker
			$mapdisp create line %x [expr %y+5] %x [expr %y-5] -width 2
			$mapdisp create line [expr %x-5] %y [expr %x+5] %y -width 2
		}
	}

	bind $can($mon) <Motion> {
		global mon
		set scrxmov %x
		set scrymov %y
		set eastcoord  [format $outfmt_coords [eval MapCanvas::scrx2mape $mon %x] ]
		set northcoord [format $outfmt_coords [eval MapCanvas::scry2mapn $mon %y] ]
		set coords($mon) "$eastcoord $northcoord"
	}
}


###############################################################################
# procedures for interactive zooming in and zooming out

# Get the current zoom region
# Returns a list in zoom_attrs order (n s e w nsres ewres rows cols)
# Implements explore mode
proc MapCanvas::currentzoom { mon } {
	variable zoom_attrs
	variable exploremode
	variable monitor_zooms
	variable regionstr
	variable msg
	variable canvas_w
	variable canvas_h

	set mapunits [MapCanvas::get_mapunits]

	# Fetch the current zoom settings if explorer mode not enabled
	set region {}
	foreach attr $zoom_attrs {
		lappend region $monitor_zooms($mon,1,$attr)
	}

	# If explore mode is enabled, set region geometry to match the canvas
	# and set map resolution proportional to map size to maintain constant
	# numbers of pixels in display.
	if {$exploremode($mon)} {
		# Set the region extents to the smallest region no smaller than the canvas
		set canvas_ar [expr {1.0 * $canvas_w($mon) / $canvas_h($mon)}]
		set expanded_region [MapCanvas::shrinkwrap 1 [lrange $region 0 3] $canvas_ar]
		foreach {n s e w} $expanded_region {break}
		# Calculate the resolutions proportional to the map size
		set explore_nsres [expr {1.0 * ($n - $s) / $canvas_h($mon)}]
		set explore_ewres [expr {1.0 * ($e - $w) / $canvas_w($mon)}]
		set explore_rows [expr round(abs($n-$s)/$explore_nsres)]
		set explore_cols [expr round(abs($e-$w)/$explore_ewres)]
		lappend expanded_region $explore_nsres $explore_ewres $explore_rows $explore_cols
		set region $expanded_region
	}

	# create region information string for status bar message
	set rows [lindex $region 6]
	set cols [lindex $region 7]
	set nsres [lindex $region 4]
	set ewres [lindex $region 5]

	if { $nsres == $ewres } {
	    set MapCanvas::regionstr [format [G_msg "Display: rows=%d columns=%d  resolution=%g $mapunits"] \
		$rows $cols $nsres]
	} else {
	    set MapCanvas::regionstr [format [G_msg "Display: rows=%d cols=%d  N-S res=%g  E-W res=%g"] \
		$rows $cols $nsres $ewres]
	}

	set MapCanvas::msg($mon) $regionstr

	# region contains values for n s e w ewres nsres rows cols
	return $region
}

# Set new display extents and (for explore mode) resolution
# Update the zoom history
# Arguments are either n s e w or n s e w nsres ewres
proc MapCanvas::zoom_new {mon args} {
	variable monitor_zooms
	variable zoomhistories
	variable zoom_attrs

	# Demote all of the zoom history
	for {set i $zoomhistories} {$i > 1} {incr i -1} {
		set iminus [expr {$i - 1}]
		foreach attr $zoom_attrs {
			catch {set monitor_zooms($mon,$i,$attr) $monitor_zooms($mon,$iminus,$attr)}
		}
	}

	# If resolution values aren't present we just use existing values.
	set present_attrs [lrange $zoom_attrs 0 [expr {[llength $args] - 1}]]

	foreach value $args attr $present_attrs {
		set monitor_zooms($mon,1,$attr) $value
	}
}

# Zoom to previous extents and resolution in the zoom history
proc MapCanvas::zoom_previous {mon} {
	variable monitor_zooms
	variable zoomhistories
	variable zoom_attrs

	# Remember the first monitor
	set old1 {}
	foreach attr $zoom_attrs {
		lappend old1 $monitor_zooms($mon,1,$attr)
	}

	# Promote all of the zoom history
	for {set i 1} {$i < $zoomhistories } {incr i} {
		set iplus [expr {$i + 1}]
		foreach attr $zoom_attrs {
			catch {set monitor_zooms($mon,$i,$attr) $monitor_zooms($mon,$iplus,$attr)}
		}
	}

	# Set the oldest thing in the history to where we just were
	foreach value $old1 attr $zoom_attrs {
		set monitor_zooms($mon,$zoomhistories,$attr) $value
	}
}

# Zoom to something loaded from a g.region command
proc MapCanvas::zoom_gregion {mon args} {
	global devnull

	if {![catch {open [concat "|g.region" "-ugp" $args "2> $devnull"] r} input]} {
		while {[gets $input line] >= 0} {
			if { [regexp -nocase {^([a-z]+)=(.*)$} $line trash key value] } {
				set parts($key) $value
			}
		}

		if { [catch {close $input} error] || ![info exists parts] } {
			GmLib::errmsg $error [G_msg "Error setting region (Problem with g.region?)"]
		}

		#set start point (sw corner)
		set $parts(w) [expr round($parts(w)/$parts(ewres))*$parts(ewres)]
		set $parts(s) [expr round($parts(s)/$parts(nsres))*$parts(nsres)]
	
		# get original width and height
		set width [expr abs($parts(e) - $parts(w))]
		set height [expr abs($parts(n) - $parts(s))]
		
		# get columns and rows rounded to nearest multiple of resolution
		set cols [expr round($width/$parts(ewres))]
		set rows [expr round($height/$parts(nsres))]
		
		# reset width and height in even multiples of resolution
		set width [expr $cols * $parts(ewres)]
		set height [expr $rows * $parts(nsres)]

		# recalculate region north and east in even multiple of resolution
		set parts(e) [expr $parts(w) + $width]
		set parts(n) [expr $parts(s) + $height]
		
		MapCanvas::zoom_new $mon $parts(n) $parts(s) $parts(e) \
			$parts(w) $parts(nsres) $parts(ewres) $parts(rows) $parts(cols)

	} else {
		puts $input
	}
}


# Set WIND file or saved region file to match settings from the current zoom
proc MapCanvas::set_wind {mon args overwrite} {
	variable zoom_attrs
	global devnull

	#get current region settings for resolution
	if {![catch {open [concat "|g.region" "-ugp " "2> $devnull"] r} input]} {
		while {[gets $input line] >= 0} {
			if { [regexp -nocase {^([a-z]+)=(.*)$} $line trash key value] } {
				set parts($key) $value
			}
		}
		
		if {[catch {close $input} error]} {
			GmLib::errmsg $error [G_msg "Error reading current resolution with g.region"]
			return
		}
		
	} else {
		GmLib::errmsg $input [G_msg "Error reading current resolution with g.region"]
		return
	}

	#set computational region extents while maintaining current resolution
	set values [MapCanvas::currentzoom $mon]
	
	set cmd "g.region"

	set options {}
	
	lappend options "-a"
	
	foreach attr $zoom_attrs value $values {
		if {$attr != "rows" && $attr != "cols" && $attr != "ewres" && $attr!= "nsres"} {
			lappend options "$attr=$value"
		}		
	}
	
	if {$parts(nsres) != ""} {
		lappend options "nsres=$parts(nsres)"		
	}

	if {$parts(ewres) != ""} {
		lappend options "ewres=$parts(ewres)"		
	}

	if {$overwrite == 1} {
		lappend options "--overwrite" 
	}
	
	if {[catch {eval [list exec -- $cmd] $args $options >& $devnull} error]} {
		GmLib::errmsg $error [G_msg "Error setting region"]
	}
	
}

# zoom bindings
proc MapCanvas::zoombind { mon zoom } {
	variable can
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2
	variable msg
	global outfmt_coords

	# initialize zoom rectangle corners

	set areaX1($mon) 0
	set areaY1($mon) 0
	set areaX2($mon) 0
	set areaY2($mon) 0

	MapCanvas::setcursor $mon "plus"

	if {$zoom == 1} {
		set MapCanvas::msg($mon) [G_msg "Drag or click mouse to zoom"]
	} elseif {$zoom == -1} {
		set MapCanvas::msg($mon) [G_msg "Drag or click mouse to unzoom"]
	}

	bind $can($mon) <1> {
		MapCanvas::markzoom $mon %x %y
	}

	bind $can($mon) <B1-Motion> {
		global mon
		set scrxmov %x
		set scrymov %y
		set eastcoord  [format $outfmt_coords [eval MapCanvas::scrx2mape $mon %x] ]
		set northcoord [format $outfmt_coords [eval MapCanvas::scry2mapn $mon %y] ]
		set coords($mon) "$eastcoord $northcoord"
		MapCanvas::drawzoom $mon %x %y
	}

	bind $can($mon) <ButtonRelease-1> "MapCanvas::zoomregion $mon $zoom"

}

# start zoom rectangle
proc MapCanvas::markzoom {mon x y} {
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2
	variable can

	# initialize corners
	set areaX1($mon) 0
	set areaY1($mon) 0
	set areaX2($mon) 0
	set areaY2($mon) 0

	set areaX1($mon) [$can($mon) canvasx $x]
	set areaY1($mon) [$can($mon) canvasy $y]
	$can($mon) delete area area2
}

# draw zoom rectangle
proc MapCanvas::drawzoom { mon x y } {
	variable can
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2

	set xc [$can($mon) canvasx $x]
	set yc [$can($mon) canvasy $y]

	if {($areaX1($mon) != $xc) && ($areaY1($mon) != $yc)} {
			$can($mon) delete area area2
			$can($mon) addtag area withtag \
					[$can($mon) create rect $areaX1($mon) $areaY1($mon) $xc $yc \
					-outline yellow -width 2 ]
			$can($mon) addtag area2 withtag \
					[$can($mon) create rect $areaX1($mon) $areaY1($mon) $xc $yc \
					-outline black -width 2 -dash {1 6} ]
			set areaX2($mon) $xc
			set areaY2($mon) $yc
	}
}


# zoom region
proc MapCanvas::zoomregion { mon zoom } {
	variable can
	variable canvas_h
	variable canvas_w
	variable monitor_zooms
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2

	set clickzoom 0
	
	# get display extents in geographic coordinates
	set dispnorth [scry2mapn $mon 0]
	set dispsouth [scry2mapn $mon $canvas_h($mon)]
	set dispeast  [scrx2mape $mon $canvas_w($mon)]
	set dispwest  [scrx2mape $mon 0]

	# minimum zoom by rectangle size = 15pix. For users with shaky hends or jerky mouses
	if {abs($areaX2($mon)-$areaX1($mon)) < 15 && abs($areaY2($mon)-$areaY1($mon)) < 15 } {
		set areaX2($mon) 0
		set areaY2($mon) 0
	}
	# get zoom rectangle extents in geographic coordinates
	if { $areaX2($mon) < $areaX1($mon) } {
			set cright $areaX1($mon)
			set cleft $areaX2($mon)
	} else {
			set cleft $areaX1($mon)
			set cright $areaX2($mon)
	}

	if { $areaY2($mon) < $areaY1($mon) } {
			set cbottom $areaY1($mon)
			set ctop $areaY2($mon)
	} else {
			set ctop $areaY1($mon)
			set cbottom $areaY2($mon)
	}

	set north [scry2mapn $mon $ctop]
	set south [scry2mapn $mon $cbottom]
	set east  [scrx2mape $mon $cright]
	set west  [scrx2mape $mon $cleft]
	# (this is all you need to zoom in with box)

	# if click and no drag, zoom in or out by fraction of original area and center on the click spot
	if {($areaX2($mon) == 0) && ($areaY2($mon) == 0)} {set clickzoom 1}
	# get first click location in map coordinates for recentering with 1-click zooming
	set newcenter_n [scry2mapn $mon $areaY1($mon)]
	set newcenter_e [scrx2mape $mon $areaX1($mon)]	

	# get current region extents for box zooming out and recentering	
	foreach {map_n map_s map_e map_w nsres ewres rows cols} [MapCanvas::currentzoom $mon] {break}
		
	# get original map center for recentering after 1-click zooming
	set oldcenter_n [expr $map_s + ($map_n - $map_s)/2]
	set oldcenter_e [expr $map_w + ($map_e - $map_w)/2]
	
	# set shift for recentering after 1-click zooming
	set shift_n [expr $newcenter_n - $oldcenter_n]
	set shift_e [expr $newcenter_e - $oldcenter_e]

	# 1-click zooming--zooms in or out by function of square root of 2 and 
	# recenters region in display window at spot clicked
	if {$clickzoom == 1} {
		# calculate amount to zoom in or out in geographic distance
		set nsscale [expr (($dispnorth - $dispsouth) - ($dispnorth - $dispsouth)/sqrt(2))/2]
		set ewscale [expr (($dispeast - $dispwest) - ($dispeast - $dispwest)/sqrt(2))/2]
		if {$zoom == 1} {
			# zoom in
			set north [expr {$dispnorth - $nsscale + $shift_n}]
			set south [expr {$dispsouth + $nsscale + $shift_n}]
			set east [expr {$dispeast - $ewscale + $shift_e}]
			set west [expr {$dispwest + $ewscale + $shift_e}]
		} elseif {$zoom == -1} {
			# zoom out
			set north [expr {$dispnorth + $nsscale + $shift_n}]
			set south [expr {$dispsouth - $nsscale + $shift_n}]
			set east [expr {$dispeast + $ewscale + $shift_e}]
			set west [expr {$dispwest - $ewscale + $shift_e}]
		}
	}


	# zoom out with box
	# box determines zoom out proportion, longest box dimension determines zoom,
	# and box center becomes region center. Zoom out relative to the geographic
	# extents of the display rather than the current region to deal with mismatches
	# between geometry of region and display window.
	if { $zoom == -1 && $clickzoom == 0} {
		# Calculate the box geometry--to be used for new region geometry
		set box_ns [expr $north-$south]
		set box_ew [expr $east-$west]
		# calcuate aspect ratio for zoom box
		set box_aspect [expr $box_ns/$box_ew]
		# calculate zoomout ratio for longest box dimension
		if { $box_ns > $box_ew } {
			set zoomratio [expr ($dispnorth - $dispsouth)/$box_ns]
			set new_ns [expr ($dispnorth - $dispsouth) * $zoomratio]
			set new_ew [expr $new_ns / $box_aspect]
		} else {
			set zoomratio [expr ($dispeast - $dispwest)/$box_ew]
			set new_ew [expr ($dispeast - $dispwest) * $zoomratio]
			set new_ns [expr $new_ew * $box_aspect]
		}

		# get zoom-out box center
		set boxcenter_n [expr $south + (($north - $south)/2)]
		set boxcenter_e [expr $west + (($east - $west)/2)]
		
		# zoom out to new extents
		set north [expr $boxcenter_n + ($new_ns/2)]
		set south [expr $boxcenter_n - ($new_ns/2)]
		set east [expr $boxcenter_e + ($new_ew/2)]
		set west [expr $boxcenter_e - ($new_ew/2)]
	}

	#set starting point (sw corner)
	set west [expr $ewres*round($west/$ewres)]
	set south [expr round($south/$nsres)*$nsres]

	# get original width and height
	set width [expr abs($east - $west)]
	set height [expr abs($north - $south)]
	
	# get columns and rows rounded to nearest multiple of resolution
	set cols [expr round($width/$ewres)]
	set rows [expr round($height/$nsres)]
	
	# reset width and height in even multiples of resolution
	set width [expr $cols * $ewres]
	set height [expr $rows * $nsres]

	# recalculate region north and east in even multiple of resolution
	set east [expr $west + $width]
	set north [expr $south + $height]


	MapCanvas::zoom_new $mon $north $south $east $west $nsres $ewres $rows $cols

	# redraw map
	$can($mon) delete map$mon
	$can($mon) delete area area2
	MapCanvas::request_redraw $mon 1
}


###############################################################################

# zoom back
proc MapCanvas::zoom_back { mon } {
		variable can

		MapCanvas::zoom_previous $mon
		$can($mon) delete map$mon
		MapCanvas::request_redraw $mon 1
}


###############################################################################
#procedures for panning

# pan bindings
proc MapCanvas::panbind { mon } {
	variable can
	variable msg
	global outfmt_coords

	set MapCanvas::msg($mon) [G_msg "Drag with mouse to pan"]

	MapCanvas::setcursor $mon "hand2"

	bind $can($mon) <1> {MapCanvas::startpan $mon %x %y}

	bind $can($mon) <B1-Motion> {
		global mon
		set scrxmov %x
		set scrymov %y
		set eastcoord  [format $outfmt_coords [eval MapCanvas::scrx2mape $mon %x] ]
		set northcoord [format $outfmt_coords [eval MapCanvas::scry2mapn $mon %y] ]
		set coords($mon) "$eastcoord $northcoord"
		MapCanvas::dragpan $mon %x %y
	}

	bind $can($mon) <ButtonRelease-1> {
		MapCanvas::pan $mon
	}
}


proc MapCanvas::startpan {mon x y} {
	variable start_x 
	variable start_y
	variable from_x 
	variable from_y
	variable to_x 
	variable to_y
	variable can

	set start_x [$can($mon) canvasx $x]
	set start_y [$can($mon) canvasy $y]
		set from_x $start_x
	set from_y $start_y
		set to_x $start_x
	set to_y $start_y

}

proc MapCanvas::dragpan {mon x y} {
	variable start_x 
	variable start_y
	variable from_x 
	variable from_y
	variable to_x 
	variable to_y
	variable can
	variable can

	set to_x [$can($mon) canvasx $x]
	set to_y [$can($mon) canvasy $y]
	$can($mon) move current [expr {$to_x-$start_x}] [expr {$to_y-$start_y}]

	set start_y $to_y
	set start_x $to_x
}

proc MapCanvas::pan { mon } {
	variable start_x 
	variable start_y
	variable from_x 
	variable from_y
	variable to_x 
	variable to_y
	variable can
	variable can
	variable monitor_zooms

	# get map coordinate shift
	set from_e [scrx2mape $mon $from_x]
	set from_n [scry2mapn $mon $from_y]
	set to_e   [scrx2mape $mon $to_x]
	set to_n   [scry2mapn $mon $to_y]

	# get region extents
	foreach {map_n map_s map_e map_w} [MapCanvas::currentzoom $mon] {break}

	# set new region extents
	set north [expr {$map_n - ($to_n - $from_n)}]
	set south [expr {$map_s - ($to_n - $from_n)}]
	set east  [expr {$map_e - ($to_e - $from_e)}]
	set west  [expr {$map_w - ($to_e - $from_e)}]

	# reset region and redraw map
	MapCanvas::zoom_new $mon $north $south $east $west

	$can($mon) delete map$mon
	MapCanvas::request_redraw $mon 1
}

###############################################################################

proc MapCanvas::setcursor { mon	 ctype } {
	variable can

	$can($mon) configure -cursor $ctype
	return
}

proc MapCanvas::restorecursor {mon} {
	variable mapcursor
	variable can

	$can($mon) configure -cursor $mapcursor
	return
}

###############################################################################
# procedures for measuring

# measurement bindings
proc MapCanvas::measurebind { mon } {
	variable can
	variable measurement_annotation_handle
	variable mlength 
	variable totmlength
	variable linex1 
	variable liney1 
	variable linex2 
	variable liney2
	variable msg
	global outfmt_coords

	set mapunits [MapCanvas::get_mapunits]

	# Make the output for the measurement
	set measurement_annotation_handle [monitor_annotation_start $mon [G_msg "Measurement"] {}]

	if {[info exists linex1]} {unset linex1}
	if {[info exists liney1]} {unset liney1}
	if {[info exists linex2]} {unset linex2}
	if {[info exists liney2]} {unset liney2}

	bind $can($mon) <1> "MapCanvas::markmline $mon %x %y"
	bind $can($mon) <B1-Motion> {
		global mon
		set scrxmov %x
		set scrymov %y
		set eastcoord  [format $outfmt_coords [eval MapCanvas::scrx2mape $mon %x] ]
		set northcoord [format $outfmt_coords [eval MapCanvas::scry2mapn $mon %y] ]
		set coords($mon) "$eastcoord $northcoord"
		MapCanvas::drawmline $mon %x %y
		}
	bind $can($mon) <ButtonRelease-1> "MapCanvas::measure $mon %x %y"

	set MapCanvas::msg($mon) [G_msg "Draw measure line with mouse"]

	MapCanvas::setcursor $mon "pencil"
	set mlength 0
	set totmlength 0

}

# start measurement line
proc MapCanvas::markmline {mon x y} {
	variable linex1 
	variable liney1 
	variable linex2 
	variable liney2
	variable can

	#start line
	if { ![info exists linex1] } {
		set linex1 [$can($mon) canvasx $x]
		set liney1 [$can($mon) canvasy $y]
		set linex2 $linex1
		set liney2 $liney1
	}

	$can($mon) delete mline
}

# draw measurement line
proc MapCanvas::drawmline { mon x y } {
	variable can
	variable linex1 
	variable liney1 
	variable linex2 
	variable liney2

	set xc [$can($mon) canvasx $x]
	set yc [$can($mon) canvasy $y]

	# draw line segment
	if {($linex1 != $xc) && ($liney1 != $yc)} {
		$can($mon) delete mline
		$can($mon) addtag mline withtag \
			[$can($mon) create line $linex1 $liney1 $xc $yc \
			-fill red -arrow both -width 2]
		set linex2 $xc
		set liney2 $yc
	}
}

# measure line length
proc MapCanvas::measure { mon x y } {
	variable can
	variable measurement_annotation_handle
	variable mlength 
	variable totmlength
	variable linex1 
	variable liney1 
	variable linex2 
	variable liney2

	set xc [$can($mon) canvasx $x]
	set yc [$can($mon) canvasy $y]

	# Measure also on single click, if it's not a first click.
	if { ($linex2 != $xc) && ($liney2 != $yc)} {
		set linex2 $xc
		set liney2 $yc
	}

	# draw cumulative line
	$can($mon) addtag totmline withtag \
		[$can($mon) create line $linex1 $liney1 $linex2 $liney2 \
		-fill green -arrow both -width 2]

	# get line endpoints in map coordinates
	set east1  [scrx2mape $mon $linex1]
	set north1 [scry2mapn $mon $liney1]
	set east2  [scrx2mape $mon $linex2]
	set north2 [scry2mapn $mon $liney2]

	# calculate line segment length and total length
	set mlength [expr {sqrt(pow(($east1 - $east2), 2) + pow(($north1 - $north2), 2))}]
	set totmlength [expr {$totmlength + $mlength}]

	# format length numbers and units in a nice way
	set out_seg [ fmt_length $mlength ]
	set out_tot [ fmt_length $totmlength ]


	monitor_annotate $measurement_annotation_handle \
		[G_msg " --segment length = $out_seg\n"]
	monitor_annotate $measurement_annotation_handle \
		[G_msg "cumulative length = $out_tot\n"]

	set linex1 $linex2
	set liney1 $liney2
}



# format length numbers and units in a nice way, as a function of length
proc MapCanvas::fmt_length { dist } {

    set mapunits [MapCanvas::get_mapunits]

    set outunits $mapunits
    set divisor "1.0"

    # figure out which units to use
    if { [string equal "meters" "$mapunits"] } {
    	if { $dist > 2500 } {
    	    set outunits "km"
    	    set divisor "1000.0"
    	}
    } elseif { [string first "feet" "$mapunits"] >= 0 } {
    	# nano-bug: we match any "feet", but US Survey feet is really 
    	#  5279.9894 per statute mile, or 10.6' per 1000 miles. As >1000
    	#  miles the tick markers are rounded to the nearest 10th of a
    	#  mile (528'), the difference in foot flavours is ignored.
    	if { $dist > 5280 } {
    	    set outunits "miles"
    	    set divisor "5280.0"
    	}
    } elseif { [string first "degree" "$mapunits"] >= 0 } {
    	if { $dist < 1 } {
    	    set outunits "minutes"
    	    set divisor [expr 1/60.0]
    	}
    }

    # format numbers in a nice way
    if { [expr $dist/$divisor ] >= 2500 } {
       set outfmt "%.0f"
    } elseif { [expr $dist/$divisor ] >= 1000 } {
    	set outfmt "%.1f"
    } elseif { [expr $dist/$divisor ] > 0 } {
    	set outfmt "%.[expr {int(ceil(3 - log10($dist/$divisor)))}]f"
    } else {
    	# error: no range (nan?)
    	set outfmt "%g"
    }

    set outdist [format $outfmt [expr $dist/$divisor ] ]

    return [concat $outdist $outunits ]
}



###############################################################################
# procedures for querying

# query bindings
proc MapCanvas::querybind { mon } {
	variable msg
	variable can

	set MapCanvas::msg($mon) [G_msg "Click to query feature"]

	bind $can($mon) <1> {
		MapCanvas::query $mon %x %y
	}

	MapCanvas::setcursor $mon "crosshair"

}

# query
proc MapCanvas::query { mon x y } {
	variable map_ew
	variable scr_lr
	variable can

	set east  [scrx2mape $mon $x]
	set north [scry2mapn $mon $y]

	# set query 'snapping' distance to 10 screen pixels
	set vdist($mon) [expr 10* {($map_ew($mon) / $scr_lr($mon))} ]
	
	# get currently selected map for querying
	set tree($mon) $GmTree::tree($mon)

	set sel [ lindex [$tree($mon) selection get] 0 ]

	if { $sel == "" } { 
		tk_messageBox -type ok -icon warning -parent .mapcan($mon) \
			-message [G_msg "You have to select map layer first"] \
			-title [G_msg "No map layer selected"]
		return 
	}

	set type [GmTree::node_type $sel]

	switch $type {
		"raster" {
			set mapname [GmRaster::mapname $sel]
			set cmd "r.what -f input=$mapname east_north=$east,$north\n\n"
		}
		"vector" {
			set mapname [GmVector::mapname $sel]
			set cmd "v.what -a map=$mapname east_north=$east,$north distance=$vdist($mon)\n\n"
		}
		"rgbhis" {
			set mapname [GmRgbhis::mapname $sel]
			set cmd "r.what -f input=$mapname east_north=$east,$north\n\n"
		}
		"arrows" {
			set mapname [GmArrows::mapname $sel]
			set cmd "r.what -f input=$mapname east_north=$east,$north\n\n"
		}
		"rnums" {
			set mapname [GmRnums::mapname $sel]
			set cmd "r.what -f input=$mapname east_north=$east,$north\n\n"
		}
		"chart" {
			set mapname [GmChart::mapname $sel]
			set cmd "v.what -a map=$mapname east_north=$east,$north distance=$vdist($mon)\n\n"
		}
		"thematic" {
			set mapname [GmThematic::mapname $sel]
			set cmd "v.what -a map=$mapname east_north=$east,$north distance=$vdist($mon)\n\n"
		}
		default {
			set mapname ""
			tk_messageBox -type ok -icon warning -parent .mapcan($mon) \
				-message [G_msg "This layer type does not support queries"] 
					-title [G_msg "Query not supported"]
			return
		}
	}

		if { $mapname == "" } {
			set ah [monitor_annotation_start $mon [G_msg "Query"] {}]
			monitor_annotate $ah [G_msg "You must select a map to query\n"]
			return
		}

		run_panel $cmd
}

###############################################################################

# Open profiling window
proc MapCanvas::startprofile { mon } {
	variable can

	GmProfile::create $can($mon)

	return
}

###############################################################################

# print to eps file
proc MapCanvas::printcanvas { mon } {
	variable can
	variable canvas_w
	variable canvas_h

	set cv $can($mon)

	# open print window
	psprint::init
	psprint::window $mon $cv $canvas_w($mon) $canvas_h($mon)
}

###############################################################################

# Set up initial variables for screen to map conversion
proc MapCanvas::coordconv { mon } {

	variable north_extent
	variable south_extent
	variable east_extent
	variable west_extent
	variable map_ew
	variable map_ns
	variable scr_lr
	variable scr_tb
	variable scr_top
	variable scr_bottom
	variable scr_right
	variable scr_left
	variable map2scrx_conv
	variable map2scry_conv
	variable canvas_w
	variable canvas_h
	variable monitor_zooms


	# get region extents
	foreach {n s e w} [MapCanvas::currentzoom $mon] {break}

	# calculate dimensions

	set north_extent($mon) [expr {1.0*$n}]
	set south_extent($mon) [expr {1.0*$s}]
	set east_extent($mon) [expr {1.0*$e}]
	set west_extent($mon) [expr {1.0*$w}]

	set map_ew($mon) [expr {$east_extent($mon) - $west_extent($mon)}]
	set map_ns($mon) [expr {$north_extent($mon) - $south_extent($mon)}]

	# get current screen geometry
	if { [info exists "mapimg.$mon"] } {
		set scr_lr($mon) [image width "mapimg.$mon"]
		set scr_tb($mon) [image height "mapimg.$mon"]
		set scr_right($mon) [image width "mapimg.$mon"]
		set scr_bottom($mon) [image height "mapimg.$mon"]
	} else {
		set scr_lr($mon) $canvas_w($mon)
		set scr_tb($mon) $canvas_h($mon)
		set scr_right($mon) $canvas_w($mon)
		set scr_bottom($mon) $canvas_h($mon)
	}

	set scr_top($mon) 0.0
	set scr_left($mon) 0.0

	# calculate conversion factors. Note screen is from L->R, T->B but map
	# is from L->R, B->T

	set map2scrx_conv($mon) [expr {$scr_lr($mon) / $map_ew($mon)}]
	set map2scry_conv($mon) [expr {$scr_tb($mon) / $map_ns($mon)}]

	# calculate screen dimensions and offsets

	if { $map2scrx_conv($mon) > $map2scry_conv($mon) } {
			set map2scrx_conv($mon) $map2scry_conv($mon)
	} else {
			set map2scry_conv($mon) $map2scrx_conv($mon)
	}

}

###############################################################################


# screen to map and map to screen conversion procedures

# map north to screen y
proc MapCanvas::mapn2scry { mon north } {
	variable north_extent
	variable scr_top
	variable map2scry_conv

	return [expr {$scr_top($mon) + (($north_extent($mon) - $north) * $map2scry_conv($mon))}]
}

# map east to screen x
proc MapCanvas::mape2scrx { mon east } {
	variable west_extent
	variable scr_left
	variable map2scrx_conv

	return [expr {$scr_left($mon) + (($east - $west_extent($mon)) * $map2scrx_conv($mon))}]
}

# screen y to map north
proc MapCanvas::scry2mapn { mon y } {
	variable north_extent
	variable scr_top
	variable map2scry_conv

	return [expr {$north_extent($mon) - (($y - $scr_top($mon)) / $map2scry_conv($mon))}]
}

# screen x to map east
proc MapCanvas::scrx2mape { mon x } {
	variable west_extent
	variable scr_left
	variable map2scrx_conv
	
	return [expr {$west_extent($mon) + (($x - $scr_left($mon)) / $map2scrx_conv($mon))}]
}

###############################################################################
# cleanup procedure on closing window
proc MapCanvas::cleanup { mon destroywin} {
	global pgs options
	
	if { [winfo exists .tlegend($mon)] } { destroy .tlegend($mon) }
	
	if { $destroywin == ".mapcan($mon)" } {
		$pgs delete "page_$mon"
		array unset GmTree::tree $mon ;# No more memory leaking on monitor close!
		# If there is any other monitor open, switch to it.
		if { [array size GmTree::tree] > 0 } {
			set mon [lindex [array names GmTree::tree] 0]
			GmTree::switchpage $mon
		} else { 
			if {[info exists options]} {
	    			destroy $options.fr
			}
			set mon 0 
		}
	}
	
}

###############################################################################
