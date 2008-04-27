###############################################################
# maptool.tcl - toolbar file GRASS GIS Manager map display canvas
# January 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
###############################################################


namespace eval MapToolBar {
	variable toolbar
	variable array maptools
}


###############################################################################

proc MapToolBar::create { tb } {
	global bgcolor
	global mon
	global env
	global tk_version
	global iconpath
	variable toolbar
	variable maptools

	set selclr #88aa88
	set toolbar $tb
	set maptools($mon) "pointer"

	# DISPLAY AND MONITOR SELECTION
	set bbox1 [ButtonBox $toolbar.bbox1 -spacing 0 ]

	# display
	$bbox1 add -image [image create photo -file "$iconpath/gui-display.gif"] \
		-command "MapCanvas::request_redraw $mon 0" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor  -activebackground $bgcolor\
		-helptext [G_msg "Display active layers"]

	# re-render all layers
	$bbox1 add -image [image create photo -file "$iconpath/gui-redraw.gif"] \
		-command "MapCanvas::request_redraw $mon 1" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Redraw all layers"]


	$bbox1 add -image [image create photo -file "$iconpath/module-nviz.gif"] \
		-command {GmGroup::nvdisplay "root"} \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor  -activebackground $bgcolor\
		-helptext [G_msg "Start NVIZ using active layers in current region"]

	$bbox1 add -image [image create photo -file "$iconpath/module-d.nviz.gif"] \
		-command "MapCanvas::dnviz $mon" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor  -activebackground $bgcolor\
		-helptext [G_msg "Create flythough path for NVIZ"]

	# erase
	$bbox1 add -image [image create photo -file "$iconpath/gui-erase.gif"] \
		-command "MapCanvas::erase $mon" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor  -activebackground $bgcolor\
		-helptext [G_msg "Erase to white"]

	pack $bbox1 -side left -anchor w

	set sep1 [Separator $toolbar.sep1 -orient vertical -background $bgcolor ]
	pack $sep1 -side left -fill y -padx 5 -anchor w

	# DISPLAY TOOLS

	# pointer
	if {$tk_version < 8.4 } {
		set pointer [radiobutton $tb.pointer \
			-image [image create photo -file "$iconpath/gui-pointer.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::pointer $mon" \
			-variable maptools($mon) -value pointer -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor  ]
	} else {
		set pointer [radiobutton $tb.pointer \
			-image [image create photo -file "$iconpath/gui-pointer.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::pointer $mon" \
			-variable maptools($mon) -value pointer \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor  ]
	}
	DynamicHelp::register $pointer balloon [G_msg "Pointer"]

	# zoom in
	if {$tk_version < 8.4 } {
		set zoomin [radiobutton $tb.zoomin \
			-image [image create photo -file "$iconpath/gui-zoom_in.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::zoombind $mon 1" \
			-variable maptools($mon) -value zoomin -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set zoomin [radiobutton $tb.zoomin \
			-image [image create photo -file "$iconpath/gui-zoom_in.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::zoombind $mon 1" \
			-variable maptools($mon) -value zoomin \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $zoomin balloon [G_msg "Zoom In"]

	#zoom out
	if {$tk_version < 8.4 } {
		set zoomout [radiobutton $tb.zoomout \
			-image [image create photo -file "$iconpath/gui-zoom_out.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::zoombind $mon -1" \
			-variable maptools($mon) -value zoomout -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set zoomout [radiobutton $tb.zoomout \
			-image [image create photo -file "$iconpath/gui-zoom_out.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::zoombind $mon -1" \
			-variable maptools($mon) -value zoomout \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $zoomout balloon [G_msg "Zoom Out"]

	# pan
	if {$tk_version < 8.4 } {
		set pan [radiobutton $tb.pan \
			-image [image create photo -file "$iconpath/gui-pan.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::panbind $mon" \
			-variable maptools($mon) -value pan -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set pan [radiobutton $tb.pan \
			-image [image create photo -file "$iconpath/gui-pan.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::panbind $mon" \
			-variable maptools($mon) -value pan \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $pan balloon [G_msg "Pan"]

	pack $pointer $zoomin $zoomout $pan -side left -anchor w

	set sep2 [Separator $toolbar.sep2 -orient vertical -background $bgcolor ]
	pack $sep2 -side left -fill y -padx 5 -anchor w

	set bbox2 [ButtonBox $toolbar.bbox2 -spacing 0	]

	# zoom.back
	$bbox2 add -image [image create photo -file "$iconpath/gui-zoom_back.gif"] \
		-command "MapCanvas::zoom_back $mon" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Return to previous zoom"]

	set mapzoom [menubutton $tb.mapzoom	 \
		-image [image create photo -file "$iconpath/gui-mapzoom.gif"] \
		-highlightthickness 0 -takefocus 0 -relief flat -borderwidth 1 \
		-highlightbackground $bgcolor -activebackground honeydew \
		-bg $bgcolor -width 32 -indicatoron 0 -direction below]
	DynamicHelp::register $mapzoom balloon [G_msg "Zoom to..."]

	# menu zooming display
	set zoommenu [menu $mapzoom.zm -type normal]

# Could use these images along with text if -compound worked in all platforms
#	set zmimg [image create photo -file "$iconpath/gui-zoom_map.gif"]
#	set zrimg [image create photo -file "$iconpath/gui-zoom_region.gif"]
#	set zcimg [image create photo -file "$iconpath/gui-zoom_current.gif"]
#	set zdimg [image create photo -file "$iconpath/gui-zoom_default.gif"]

	$zoommenu add command \
		-label [G_msg "Zoom display to selected map"] \
		-command {MapCanvas::zoom_map $mon}
	$zoommenu add command \
		-label [G_msg "Zoom display to saved region"] \
		-command {MapCanvas::zoom_region $mon}
	$zoommenu add command \
		-label [G_msg "Save display extents to named region"] \
		-command {MapCanvas::save_region $mon}
	$zoommenu add command \
		-label [G_msg "Zoom display to computational region (set with g.region)"] \
		-command {MapCanvas::zoom_current $mon}
	$zoommenu add command \
		-label [G_msg "Zoom display to default region"] \
		-command {MapCanvas::zoom_default $mon}
	$zoommenu add command \
		-label [G_msg "Set computational region extents to match display"] \
		-command {MapCanvas::set_wind $mon "" 0}

	$mapzoom configure -menu $zoommenu

	pack $bbox2 -side left -anchor w

	pack $mapzoom -side left -anchor w -expand no -fill y

	set sep3 [Separator $toolbar.sep3 -orient vertical -background $bgcolor ]
	pack $sep3 -side left -fill y -padx 5 -anchor w

	# query
	if {$tk_version < 8.4 } {
		set query [radiobutton $tb.query \
			-image [image create photo -file "$iconpath/gui-query.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::querybind $mon" \
			-variable maptools($mon) -value query -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set query [radiobutton $tb.query \
			-image [image create photo -file "$iconpath/gui-query.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::querybind $mon" \
			-variable maptools($mon) -value query \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $query balloon [G_msg "Query"]

	# measure
	if {$tk_version < 8.4 } {
		set measure [radiobutton $tb.measure \
			-image [image create photo -file "$iconpath/gui-measure.gif"]  \
			-command "MapCanvas::stoptool $mon; MapCanvas::measurebind $mon"\
			-variable maptools($mon) -value measure -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set measure [radiobutton $tb.measure \
			-image [image create photo -file "$iconpath/gui-measure.gif"] \
			-command "MapCanvas::stoptool $mon; MapCanvas::measurebind $mon"\
			-variable maptools($mon) -value measure \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $measure balloon [G_msg "Measure"]

	set bbox3 [ButtonBox $toolbar.bbox3 -spacing 0 ]
	$bbox3 add -image [image create photo -file "$iconpath/gui-profile.gif"] \
		-command "MapCanvas::stoptool $mon; MapCanvas::startprofile $mon" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Create profile of raster map"]


	pack $query $measure -side left -anchor w
	pack $bbox3 -side left -anchor w


	set sep4 [Separator $toolbar.sep4 -orient vertical -background $bgcolor ]
	pack $sep4 -side left -fill y -padx 5 -anchor w

	# FILE & PRINT
	set bbox4 [ButtonBox $toolbar.bbox4 -spacing 0 ]

	$bbox4 add -image [image create photo -file "$iconpath/file-print.gif"] \
		-command "MapCanvas::printcanvas $mon" \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1 \
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Print raster & vector maps to eps file"]

	set mapsave [menubutton $tb.mapsave \
		-image [image create photo -file "$iconpath/gui-filesave.gif"] \
		-highlightthickness 0 -takefocus 0 -relief flat -borderwidth 1 \
		-highlightbackground $bgcolor -activebackground honeydew \
		-bg $bgcolor -width 32 -indicatoron 0 -direction below]
	DynamicHelp::register $mapsave balloon [G_msg "Export display to graphics file"]

	pack $mapsave -side left -anchor w -expand no -fill y
	pack $bbox4 -side left -anchor w

	# menu for saving display
	set savefile [menu $mapsave.sf -type normal]
	set jpgfile [menu $savefile.jpg -type normal]

	$savefile add command -label "BMP*" -command {MapToolBar::savefile bmp 0}
	$savefile add cascade -label "JPG*" -menu $jpgfile
		$jpgfile add command -label [G_msg "low quality (50)"]	\
			-command {MapToolBar::savefile jpg 50}
		$jpgfile add command -label [G_msg "mid quality (75)"] \
			-command {MapToolBar::savefile jpg 75}
		$jpgfile add command -label [G_msg "high quality (95)"] \
			-command {MapToolBar::savefile jpg 95}
		$jpgfile add command -label [G_msg "very high resolution (300% your current resolution)"] \
			-command {MapToolBar::savefile jpg 300}
	$savefile add command -label "PPM/PNM" -command {MapToolBar::savefile ppm 0}
	$savefile add command -label "PNG" -command {MapToolBar::savefile png 0}
	$savefile add command -label "TIF*" -command {MapToolBar::savefile tif 0}
	$savefile add command -label [G_msg "(* requires gdal)"] -state disabled

	$mapsave configure -menu $savefile

	set sep5 [Separator $toolbar.sep5 -orient vertical ]
	pack $sep5 -side left -fill y -padx 5 -anchor w

	# Render modes

	# Strict render mode
	# Uses previous resolution and exact boundaries
	if {$tk_version < 8.4 } {
		set strictdraw [radiobutton $tb.strictdraw \
			-command "MapCanvas::exploremode $mon 0" \
			-variable MapToolBar::explore($mon) -value strict -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set strictdraw [radiobutton $tb.strictdraw \
			-command "MapCanvas::exploremode $mon 0" \
			-variable MapToolBar::explore($mon) -value strict \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $strictdraw balloon [G_msg "Constrain map to region geometry"]
	icon_configure $strictdraw drawmode strict

	# Explore render mode
	# Uses resolution to match display and expanded boundaries to fill display
	if {$tk_version < 8.4 } {
		set exploredraw [radiobutton $tb.strictzoom \
			-command "MapCanvas::exploremode $mon 1" \
			-variable MapToolBar::explore($mon) -value explore -relief flat \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	} else {
		set exploredraw [radiobutton $tb.strictzoom \
			-command "MapCanvas::exploremode $mon 1" \
			-variable MapToolBar::explore($mon) -value explore \
			-relief flat -offrelief flat -overrelief raised \
			-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
			-activebackground $bgcolor -highlightbackground $bgcolor ]
	}
	DynamicHelp::register $exploredraw balloon [G_msg "Map fills display window"]
	icon_configure $exploredraw drawmode explore

	# This does not actually set the mode
	# it just starts visually in sync with the default
	set MapToolBar::explore($mon) strict

	pack $strictdraw $exploredraw -side left -anchor w
}


###############################################################################
# changes button on keypress
proc MapToolBar::changebutton { rbname } {
	global mon
	variable maptools

	set maptools($mon) $rbname
}

###############################################################################
# procedures for saving files

# save png file
proc MapToolBar::savefile { type quality } {
	global env
	global mon
	global tmpdir

	set outfile($mon) $MapCanvas::outfile($mon)

	if { [info exists env(HOME)] } {
		set dir $env(HOME)
		set path [tk_getSaveFile -initialdir $dir \
			-title "Save file: do not add extension to file name"]
	} else {
		set path [tk_getSaveFile  \
			 -title "Save file: do not add extension to file name"]
	}
	set currdir [pwd]
	cd $tmpdir

	catch {file copy -force $outfile($mon) $path.ppm}

	cd $currdir

	if { $path != "" } {
		switch $type {
			"bmp" {
				if { [catch {exec gdal_translate $path.ppm $path.bmp -of BMP} error ]} {
					GmLib::errmsg $error [G_msg "Could not create BMP"]
				}
				catch {file delete $path.ppm}
			}
			"jpg" {
			    if { $quality == 300 } {
					if { [catch {exec gdal_translate $path.ppm $path.jpg -of JPEG -co QUALITY=95 -outsize 300% 300% } error ]} {
						GmLib::errmsg $error [G_msg "Could not create JPG"]
					}					
					catch {file delete $path.ppm}
				} else {
					if { [catch {exec gdal_translate $path.ppm $path.jpg -of JPEG -co QUALITY=$quality  } error ]} {
						GmLib::errmsg $error [G_msg "Could not create JPG"]
					}					

					catch {file delete $path.ppm}
				}
			}
			"png" {
				if { [catch {exec gdal_translate $path.ppm $path.png -of PNG} error ]} {
					GmLib::errmsg $error [G_msg "Could not create PNG"]
				}
				
				catch {file delete $path.ppm}
			}
			"ppm" {
				return
			}
			"tif" {
				if { [catch {exec gdal_translate $path.ppm $path.tif -of GTIFF} error ]} {
					GmLib::errmsg $error [G_msg "Could not create TIF"]
				}
				
				catch {file delete $path.ppm}
			}
		}
	}
	return
}

