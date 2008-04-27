##########################################################################
#
# georect.tcl -TclTk canvas georectify display and controls
#    for GIS Manager: GUI for GRASS 6
#
# Author: Michael Barton (Arizona State University).
#
# July 2006
#
# COPYRIGHT:    (C) 1999 - 2006 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
##########################################################################


# All of these must be sourced before using georect.tcl:
# source $env(GISBASE)/etc/gtcltk/gmsg.tcl
# source $env(GISBASE)/etc/gtcltk/select.tcl
# source $env(GISBASE)/etc/gui.tcl
# This one is going to be handled by pkgIndex:
# georecttool.tcl

namespace eval GRMap {
    variable displayrequest # true if it wants to get displayed.

    # Something's modified the canvas or view
    # Degree of modification 0 - none, 1 - zoom, 2 - canvas
    variable array grcanmodified
    # The canvas widget of the georectify monitor
    variable grcan 
    # Frame widget
    variable grmapframe
    # Width and height of canvas
    variable grcanvas_w 
    variable grcanvas_h
    # Actual width and height used while drawing / compositing
    variable driver_w 
    # Actual width and height used while drawing / compositing
    variable driver_h
    # Indicator widgets
    variable map_ind 
    variable initwd
    variable initht
    variable grcursor
    # TMP directory for raster display images used in canvas
    variable tmpdir
    # status bar message
    variable msg 

    # variables for coordinate conversions and zooming
    variable linex1
    variable liney1
    variable linex2
    variable liney2
    variable map_n
    variable map_s
    variable map_e
    variable map_w
    variable map_ew
    variable map_ns
    variable scr_n
    variable scr_s
    variable scr_e
    variable scr_w
    variable scr_ew
    variable scr_ns
    variable map2scrx_conv
    variable map2scry_conv
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2

    #variable grcoords # geographic coordinates from mouse click
    # geographic coordinates from mouse movement to display in indicator widget
    variable grcoords_mov
    # Driver output file (.ppm)
    variable grfile 
    #process id to use for temp files
    variable mappid 

    # Current region and region historys
    # Indexed by history (1 (current) - zoomhistories), part (n, s, e, w, nsres, ewres).
    variable array monitor_zooms
    # Depth of zoom history to keep
    variable zoomhistories
    set zoomhistories 7

    # Regular order for region values in a list representing a region or zoom
    variable zoom_attrs
    set zoom_attrs {n s e w nsres ewres}

    # string with region information to show in status bar
    variable regionstr

    # This variable keeps track of which monitor set the gism driver settings last.
    # They must always be redone if the monitor was different
    variable previous_monitor
    set previous_monitor {none}

    # Current projection and zone for dynamic region setting for displays
    variable gregionproj

    variable redrawrequest 0

    #variables for panning
    variable start_x
    variable start_y
    variable from_x
    variable from_y
    variable to_x
    variable to_y

    # gcp variables
    # use GCP in RMS calculations and rectification indexed by gcpnum
    variable array usegcp
    # entry widget for GCP xy coordinates indexed by gcpnum
    variable array xy
    # entry widget for GCP forward rms error indexed by gcpnum
    variable array fwd
    # entry widget for GCP reverse rms error indexed by gcpnum
    variable array rev
    # gcp form has been created and can be accessed
    variable drawform
    # entry widget for GCP georectified coordinates indexed by gcpnum
    variable array geoc
    # checkbutton widget for GCP use indexed by gcpnum
    variable array chk
    # forward projected error value for each GCP indexed by gcpnum
    variable array fwd_error
    # backward projected error value for each GCP indexed by gcpnum
    variable array rev_error
    # forward and backward projected error
    variable errorlist

    variable gcpnum
    #forward projected rms error for GCP's, displayed in gcp manager status bar
    variable fwd_rmserror
    #backward projected rms error for GCP's, displayed in gcp manager status bar
    variable rev_rmserror

    #variables to keep track of location and mapset
    # current gisdbase
    variable currgdb
    # current location
    variable currloc
    # current mapset
    variable currmset
    # gisdbase of xy raster
    variable xygdb
    # location of xy raster
    variable xyloc
    # mapset of xy raster
    variable xymset
    # raster group to georectify
    variable xygroup
    # raster or vector map to display as refernce for setting ground control points
    variable xymap
    # vector map to add or delete from vector group
    variable xyvect
    # georectify raster or vector map
    variable maptype
    # is target mapset same as current mapset
    variable selftarget
    # vector map for vector group file
    variable xyvect
    # rectification method (1,2,3)
    variable rectorder

	# initialize variables
	set gcpnum 1
	set chk($gcpnum) ""
	set currgdb $env(GISDBASE)
	set currloc $env(LOCATION_NAME)
	set currmset $env(MAPSET)
	set east 0.0
	set errorlist ""
	set fwd_error($gcpnum) ""
	set fwd_rmserror 0.0
	set geoc($gcpnum) ""
	set gregion ""
	set gregionproj ""
	set initht 480.0
	set initwd 640.0
	set maptype ""
	set north 0.0
	set rectorder 1
	set regionstr ""
	set rev_error($gcpnum) ""
	set rev_rmserror 0.0
	set selftarget 0
	set usegcp($gcpnum) 1
	set xy($gcpnum) ""
	set fwd($gcpnum) ""
	set rev($gcpnum) ""
	set xygdb ""
	set xygroup ""
	set xyloc ""
	set xymap ""
	set xymset ""
	set xyvect ""
	set drawform 0


}


###############################################################################
# Set location and mapset to selected xy
proc GRMap::setxyenv { mset loc } {
    variable selftarget
    global env

    if { $selftarget == 1 } { return }

    if { $mset != "" && $loc != "" } {
        runcmd "g.gisenv set=LOCATION_NAME=$loc"
        runcmd "g.gisenv set=MAPSET=$mset"

        set env(LOCATION_NAME) $loc
        set env(MAPSET) $mset
    }
}


###############################################################################
# set location and mapset back to georectified
proc GRMap::resetenv { } {
    variable currloc
    variable currmset
    variable selftarget
    global env

    if { $selftarget == 1 } { return }

    runcmd "g.gisenv set=LOCATION_NAME=$currloc"
    runcmd "g.gisenv set=MAPSET=$currmset"

    set env(LOCATION_NAME) $currloc
    set env(MAPSET) $currmset

}


###############################################################################
# get xy group to georectify; set target to current location and mapset
proc GRMap::getxygroup { vgcreate } {
    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable currloc
    variable currmset
    variable selftarget
    variable maptype

    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc
    set m [GSelect group]
    set mname [lindex [split $m "@"] 0]
    # Return to georectified mapset
    GRMap::resetenv

    if { $mname != "" } {
            set GRMap::xygroup $mname
    }

    # are we creating a vector group?
    if { $vgcreate == 1 } {
        GRMap::read_vgroup $xygroup
        return
    }

    if { $maptype == "rast" } {
        # check to see if a raster group exists
        set groupfile "$xygdb/$xyloc/$xymset/group/$xygroup/REF"
        if {![file exists $groupfile] } {
            set GRMap::xygroup ""
            set msg [G_msg "There is no raster group file (REF). You must select\
                    the 'create/edit group' option to create a group file."]
            tk_messageBox -message $msg -parent .grstart -type ok
            return
        }
        # set i.rectify target
        if { $selftarget == 1 } {
            set cmd "i.target -c group=$GRMap::xygroup"
        } else {
            set cmd "i.target group=$GRMap::xygroup location=$currloc mapset=$currmset"
        }
        # First, switch to xy mapset
        GRMap::setxyenv $xymset $xyloc
        runcmd $cmd
        # Return to georectified mapset
        GRMap::resetenv
    } elseif { $maptype == "vect" } {
        # check to see if a vector group exists
        set groupfile "$xygdb/$xyloc/$xymset/group/$xygroup/VREF"
        puts "groupfile = $groupfile"
        if {![file exists $groupfile] } {
            set GRMap::xygroup ""
            set msg [G_msg "There is no vector group file (VREF). You must select\
                    the 'create/edit group' option to create a group file."]
            tk_messageBox -message $msg -parent .grstart -type ok
            return
        }
    } else {
            return
    }

}

###############################################################################
# get raster to display for georectification
proc GRMap::getxymap { type } {
    variable xyloc
    variable xymset
    variable xymap

    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc

    if { $type == "rast" } {
        set m [GSelect cell]
    	set mname [lindex [split $m "@"] 0]
        if { $mname != "" } {
            set GRMap::xymap $mname
        }
    } elseif {$type == "vect" } {
        set m [GSelect vector]
        set mname [lindex [split $m "@"] 0]
        if { $mname != "" } {
            set GRMap::xymap $mname
        }
    }

    # Return to georectified mapset
    GRMap::resetenv

}

###############################################################################
# create or edit raster group to georectify
proc GRMap::group { } {
    variable xyloc
    variable xymset
    variable maptype


    if { $maptype == "rast" } {
        # First, switch to xy mapset
        GRMap::setxyenv $xymset $xyloc
		set cmd "i.group"
        if {[catch {exec -- $cmd --ui } error]} {
        	GmLib::errmsg $error
        }

        # Return to georectified mapset
        GRMap::resetenv
    } elseif { $maptype == "vect" } {
            GRMap::vgroup
    } else {
            return
    }

}

###############################################################################
# get mapset of raster to georectify; automatically set location and gisdbase
proc GRMap::getmset { } {
    variable xygdb
    variable xyloc
    variable xymset
    variable currgdb
    variable currloc
    variable currmset
    variable mappid
    variable grfile
    variable tmpdir
    variable selftarget

    set path [tk_chooseDirectory -initialdir $currgdb \
            -title [G_msg "Select mapset of raster to georectify"] \
            -mustexist true ]
    # try to make sure that a valid mapset has been picked
    if { $path == "" || $path == $currgdb || [file dirname $path] == $currgdb } { return }

    set xymset [file tail $path]
    set xylocdir [file dirname $path]
    set xyloc [file tail $xylocdir]
    set xygdb [file dirname $xylocdir]

    # check to see if the target location and mapset is the current one
    if { $xyloc == $currloc && $xymset == $currmset } {set selftarget 1 }

    set GRMap::xymset [file tail $path]

    # create files in tmp diretory for layer output
    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc

    set mappid [pid]
	if {[catch {set grfile [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}

    append grfile ".ppm"
    set tmpdir [file dirname $grfile]

    # Return to georectified mapset
    GRMap::resetenv
}


###############################################################################
# dialog to create or edit vector group to georectify
proc GRMap::vgroup { } {
    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable xyvect
    global iconpath
    global bgcolor

    toplevel .vgwin

    set vg_mf [MainFrame .vgwin.mf \
            -textvariable GRMap::vgmsg]

    set GRMap::vgmsg [G_msg "Create a group REF file and directory for vectors"]

    set vg_frame [$vg_mf getframe]

    # toolbar creation
    set vg_tb  [$vg_mf addtoolbar]

    set bbox [ButtonBox $vg_tb.bbox1 -spacing 0 -homogeneous 1 ]

    # create or replace vector group
    $bbox add -image [image create photo -file "$iconpath/file-save.gif"] \
        -command {GRMap::write_vgroup $GRMap::xygroup $GRMap::xyvect} \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -helptext [G_msg "Create/replace vector group"]

    pack $bbox -side left -anchor w -expand no -fill y

    #create dialog
    set vg_sw [ScrolledWindow $vg_frame.sw -relief flat \
        -borderwidth 1 ]
    set vg_sf [ScrollableFrame $vg_sw.sf -height 50 -width 400]
    $vg_sw setwidget $vg_sf

    set vgframe [$vg_sf getframe]

    pack $vg_sw -fill both -expand yes

    set vg [frame $vgframe.fr]
    pack $vg -fill both -expand yes


    pack $vg_mf -side top -expand yes -fill both -anchor n


    # Scroll the options window with the mouse
    bind_scroll $vg_sf

    # Select or set group name
    set row [ frame $vg.groupname ]
    Button $row.a -text [G_msg "group name"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Select existing vector group or name new group"] \
        -width 16 -anchor w \
                -command {GRMap::getxygroup 1}
    Entry $row.b -width 35 -text "$GRMap::xygroup" \
          -textvariable GRMap::xygroup
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # select xy vector for group
    set row [ frame $vg.vect ]
    Button $row.a -text [G_msg "vector"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Select xy vector(s) for group"]\
        -width 16 -anchor w \
                -command {GRMap::getxyvect }
    Entry $row.b -width 35 -text "$GRMap::xyvect" \
          -textvariable GRMap::xyvect
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    GRMap::read_vgroup $GRMap::xygroup

    wm title .vgwin [G_msg "Vector group"]
    wm withdraw .vgwin
    wm deiconify .vgwin

    #cleanup for window closing
    bind .vgwin <Destroy>  {
    	set winname %W
        if {$winname == ".vgwin"} {GRMap::cleanup}
    }
}


# get vector for vector group
proc GRMap::getxyvect { } {
    variable xyloc
    variable xymset
    variable xyvect


    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc

    set m [GSelect vector]
    set mname [lindex [split $m "@"] 0]
    if { $mname != "" } {
        if { $GRMap::xyvect == "" } {
            set GRMap::xyvect $mname
        } else {
            append xyvect ",$mname"
        }
    }

    # Return to georectified mapset
    GRMap::resetenv

}

proc GRMap::read_vgroup { xygroup } {
    variable xygdb
    variable xyloc
    variable xymset
    variable xyvect
    #get vector list from existing vector group REF file

    set vgfile "$xygdb/$xyloc/$xymset/group/$xygroup/VREF"
    if {![file exists $vgfile] } { return }

    # do the import
    set xyvect ""
    catch {set vlist [open $vgfile]}
    set vectnames [read $vlist]
	if {[catch {close $vlist} error]} {
		GmLib::errmsg $error
	}
   
    set vlines [split $vectnames "\n"]
    foreach vect $vlines {
        if { $xyvect == "" } {
            set GRMap::xyvect $vect
        } else {
            append GRMap::xyvect "," $vect
            set GRMap::xyvect [string trim $GRMap::xyvect ","]
        }
    }
}

proc GRMap::write_vgroup {xygroup xyvect} {
    #write vector list to vector group REF file

    variable xygdb
    variable xyloc
    variable xymset

    set vgfile "$xygdb/$xyloc/$xymset/group/$xygroup/VREF"

    # if group directory doesn't exist, create it

    if {![file isdirectory [file dirname $vgfile]] } {
            file mkdir [file dirname $vgfile]
    }

    if { $xyvect == "" } { return }

    # write out vector group file
    set vlist [split $xyvect ,]
    catch {set output [open $vgfile w ]}
		foreach vect $vlist {
			puts $output $vect
		}
	if {[catch {close $output} error]} {
		GmLib::errmsg $error
	}

}



###############################################################################
# create dialog to select mapset (and location) and raster map to profile,
# and start georectifying canvas

proc GRMap::startup { } {
    variable currgdb
    variable currloc
    variable currmset
    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable xymap
    variable maptype
    variable initwd
    variable initht
    global env
    global bgcolor
    global iconpath
    
    set grstarttitle [G_msg "GRASS Georectifier"]
    toplevel .grstart

    wm title .grstart [G_msg $grstarttitle]
    wm withdraw .grstart

    # create frames for georectify startup

    set grstart_mf [MainFrame .grstart.mf \
            -textvariable GRMap::grstartmsg]

    set GRMap::grstartmsg [G_msg "Set up environment for georectifying rasters or vectors"]

    set grstartup [$grstart_mf getframe ]

    # toolbar creation
    set grstart_tb  [$grstart_mf addtoolbar]

    # select raster or vector
    set selrast [radiobutton $grstart_tb.rast -variable GRMap::maptype -value "rast" \
        -text [G_msg "Georeference raster"] -highlightthickness 0 \
        -activebackground $bgcolor -highlightbackground $bgcolor -bg $bgcolor]
        $selrast select
    set selvect [radiobutton $grstart_tb.vect -variable GRMap::maptype -value "vect" \
        -text [G_msg "Georeference vector"] -highlightthickness 0 \
        -activebackground $bgcolor -highlightbackground $bgcolor  -bg $bgcolor]
    pack $selrast $selvect -side left
    pack $grstart_tb -side left -fill both -expand no -padx 5 -pady 3

    # set xy mapset
    set row [ frame $grstartup.mset -bg $bgcolor]
    Button $row.a -text [G_msg "1. Select mapset"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Mapset of xy raster group"]\
        -width 16 -anchor w \
        -command {GRMap::getmset}
    Entry $row.b -width 35 -text "$GRMap::xymset" \
          -textvariable GRMap::xymset
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # Create raster or vector group
    set row [ frame $grstartup.group -bg $bgcolor]
    Button $row.a -text [G_msg "2. Create/edit group"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Create/edit group (rasters or vectors to georectify)"] \
        -width 16 -anchor w \
        -command {GRMap::group}
    pack $row.a -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # set xy group
    set row [ frame $grstartup.selgroup -bg $bgcolor]
    Button $row.a -text [G_msg "3. Select group"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Select existing group to georectify"]\
        -width 16 -anchor w \
                -command {GRMap::getxygroup 0}
    Entry $row.b -width 35 -text "$GRMap::xygroup" \
          -textvariable GRMap::xygroup
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # set xy raster or vector
    set row [ frame $grstartup.map -bg $bgcolor]
    Button $row.a -text [G_msg "4. Select map"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Select non-georectified raster or vector to display for marking ground control points"]\
        -width 16 -anchor w \
        -command {GRMap::getxymap $GRMap::maptype}
    Entry $row.b -width 35 -text "$GRMap::xymap" \
          -textvariable GRMap::xymap
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # Start georectify canvas
    set row [ frame $grstartup.start -bg $bgcolor]
    Button $row.a -text [G_msg "5. Start georectifying"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Start georectifying"]\
        -width 16 -anchor w -highlightthickness 0 \
                -command "GRMap::refmap"
    pack $row.a -side left
    pack $row -side top -fill both -expand yes -padx 5 -pady 1

    # quit
    set row [ frame $grstartup.quit -bg $bgcolor]
    Button $row.a -text [G_msg "Cancel"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Cancel georectification"]\
        -width 16 -anchor w -highlightthickness 0 \
        -command "destroy .grstart"
	Button $row.b -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q gm_georect" \
		-background $bgcolor \
		-helptext [G_msg "Help"]
    pack $row.a -side left
    pack $row.b -side right
    pack $row -side top -fill both -expand yes -padx 5 -pady 1


    pack $grstart_mf
    pack $grstartup -side top -fill both -expand yes

    wm deiconify .grstart
    raise .grstart
    focus -force .grstart

    #cleanup for window closing
    #bind .grstart <Destroy> "GRMap::cleanup %W"
    bind .grstart <Destroy> {
        if { "%W" == ".grstart" } { GRMap::cleanup }
    }


}


###############################################################################

# Create window and canvas for displaying xy raster or vector as reference map for
# selecting ground control points in the xy system

proc GRMap::refmap { } {
    variable initwd
    variable initht
    variable grcursor
    variable grcanvas_w
    variable grcanvas_h
    variable mappid
    variable grmapframe
    variable grcan
    variable map_ind
    variable grfile
    variable coords
    variable tmpdir
    variable xyloc
    variable xymset
    variable xygroup
    variable xymap
    variable maptype
    variable msg
    variable grcoords_mov
    global env
    global drawprog
    global grcoords

    if { $xymset=="" || $xygroup=="" || $xymap=="" } {
            return
            }

    # close dialog to select mapset and raster
    destroy .grstart
    
    #start gcp window
    GRMap::gcpwin

    # set environment to xy location
    GRMap::setxyenv $xymset $xyloc

    # need to turn off wind_override here

    # Initialize window and map geometry
    set grcanvas_w $initwd
    set grcanvas_h $initht
    set env(GRASS_WIDTH) $initwd
    set env(GRASS_HEIGHT) $initht
    set drawprog 0

    # Make sure that we are using the WIND file for everything except displays
    if {[info exists env(WIND_OVERRIDE)]} {unset env(WIND_OVERRIDE)}

    # Set display geometry to the current region settings (from WIND file)
    GRMap::zoom_gregion


    # Zoom to map to georectify
    if { $maptype == "rast" } {
        GRMap::zoom_gregion [list "rast=$xymap"]
    } elseif { $maptype == "vect" } {
        GRMap::zoom_gregion [list "vect=$xymap"]
    }

    # Create canvas monitor as top level mainframe
    toplevel .mapgrcan
    wm title .mapgrcan [G_msg "Displaying xy map to be georectified"]

    set grmapframe [MainFrame .mapgrcan.mf \
            -textvariable GRMap::msg \
            -progressvar drawprog -progressmax 100 -progresstype incremental]

    set mf_frame [$grmapframe getframe]

    # toolbar creation
    set map_tb  [$grmapframe addtoolbar]
    GRToolBar::create $map_tb

    # canvas creation
    set grcan [canvas $mf_frame.grcanvas \
        -borderwidth 0 -closeenough 10.0 -relief groove \
        -width $grcanvas_w -height $grcanvas_h ]

    # setting geometry
    place $grcan -in $mf_frame -x 0 -y 0 -anchor nw

    pack $grcan -fill both -expand yes

    # indicator creation
    set map_ind [$grmapframe addindicator -textvariable grcoords_mov \
        -width 33 -justify left -padx 5 -bg white]

    pack $grmapframe -fill both -expand yes

    set grcursor [$grcan cget -cursor]

    GRMap::coordconv

    # bindings for display canvas

    # mouse handlers
    # The coordinate transforms should be done per monitor.
    bind $grcan <ButtonPress-1> {
            set eastcoord [eval GRMap::scrx2mape %x]
            set northcoord [eval GRMap::scry2mapn %y]
            set grcoords "$eastcoord $northcoord"
    }

    # Displays geographic coordinates in indicator window when cursor moved across canvas
    bind $grcan <Motion> {
            set scrxmov %x
            set scrymov %y
            set eastcoord [eval GRMap::scrx2mape %x]
            set northcoord [eval GRMap::scry2mapn %y]
            set grcoords_mov "$eastcoord $northcoord"
    }


    # TSW - inserting key command ability into gis.m
    # 9 May 2006
    # set some key commands to speed use

    # Return to previous zoom
    bind .mapgrcan <KeyPress-r> {
        GRMap::zoom_back
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

    bind .mapgrcan <KeyPress-x> {
            GRToolBar::changebutton pointer
            GRMap::stoptool
    }
    bind .mapgrcan <KeyPress-z> {
            GRMap::stoptool
            GRToolBar::changebutton zoomin
            GRMap::zoombind 1
    }
    bind .mapgrcan <KeyPress-t> {
            GRMap::stoptool
            GRToolBar::changebutton zoomout
            GRMap::zoombind -1
    }
    bind .mapgrcan <KeyPress-a> {
            GRMap::stoptool
            GRToolBar::changebutton pan
            GRMap::panbind
    }

    # Left handed
    # poiNter - pointer
    # zoom In - zoom in
    # zoom Out - zoom out
    # Pan - pan
    # ? - query

    bind .mapgrcan <KeyPress-n> {
            GRToolBar::changebutton pointer
            GRMap::stoptool
    }
    bind .mapgrcan <KeyPress-i> {
            GRMap::stoptool
            GRToolBar::changebutton zoomin
            GRMap::zoombind 1
    }
    bind .mapgrcan <KeyPress-o> {
            GRMap::stoptool
            GRToolBar::changebutton zoomout
            GRMap::zoombind -1
    }
    bind .mapgrcan <KeyPress-p> {
            GRMap::stoptool
            GRToolBar::changebutton pan
            GRMap::panbind
    }


    # window configuration change handler for resizing
    bind $grcan <Configure> "GRMap::do_resize"

    #return to georectified location
    GRMap::resetenv

    #default selector tool
    GRMap::selector

    # bindings for closing windows
    bind .mapgrcan <Destroy> {
        if { "%W" == ".mapgrcan" } { GRMap::cleanup }
    }

}

###############################################################################
# create form for gcp management
proc GRMap::gcpwin {} {
    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable gcpnum
    variable usegcp
    variable maptype
    variable xy
    variable geoc
    variable chk
    variable fwd
    variable rev
    variable rectorder
    variable fwd_error
    variable rev_error
    variable fwd_rmserror
    variable rev_rmserror
    variable drawform
    global xyentry
    global geoentry
    global b1coords
    
    set fwd_rmssumsq 0.0
    set fwd_rmserror 0.0
    set rev_rmssumsq 0.0
    set rev_rmserror 0.0


    toplevel .gcpwin

    set gcp_mf [MainFrame .gcpwin.mf \
        -textvariable GRMap::gcpmsg]

    set gcp_frame [$gcp_mf getframe]

    # toolbar creation
    set gcp_tb  [$gcp_mf addtoolbar]
    GRMap::gcptb $gcp_tb

    # gcp form creation
    set gcp_sw [ScrolledWindow $gcp_frame.sw -relief flat \
        -borderwidth 1 ]
    set gcp_sf [ScrollableFrame $gcp_sw.sf -height 200 -width 750]
    $gcp_sw setwidget $gcp_sf

    set gcpframe [$gcp_sf getframe]

    pack $gcp_sw -fill both -expand yes

    set gcp [frame $gcpframe.fr]
    pack $gcp -fill both -expand yes


    pack $gcp_mf -side top -expand yes -fill both -anchor n
    pack $gcp_tb -side left -expand yes -fill x


    # Scroll the options window with the mouse
    bind_scroll $gcp_sf

    if { $maptype == "vect" } {
        set rbstate "disabled"
    } else {
        set rbstate "normal"
    }

    # setting rectification method
    set row [ frame $gcp.method ]
    Label $row.a -text [G_msg "Select rectification method for rasters"] \
        -fg MediumBlue
    set first [radiobutton $row.b -variable GRMap::rectorder -value 1 \
        -text [G_msg "1st order"] -highlightthickness 0]
        DynamicHelp::register $first balloon [G_msg "affine transformation \
            (rasters & vectors). Requires 3+ GCPs."]
        $first select

    set second [radiobutton $row.c -variable GRMap::rectorder -value 2 \
        -text [G_msg "2nd order"] -highlightthickness 0 -state $rbstate]
        DynamicHelp::register $second balloon [G_msg "polynomial transformation \
                (rasters only). Requires 6+ GCPs."]

    set third [radiobutton $row.d -variable GRMap::rectorder -value 3 \
        -text [G_msg "3rd order"] -highlightthickness 0 -state $rbstate]
        DynamicHelp::register $third balloon [G_msg "polynomial transformation \
                (rasters only). Requires 10+ GCPs."]

    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    set row [ frame $gcp.header ]
    Label $row.a -text [G_msg "Use"] \
        -fg MediumBlue -width 3
    Label $row.b -text [G_msg "xy coordinates"] \
        -fg MediumBlue -width 34
    Label $row.c -text [G_msg "geographic coordinates"] \
        -fg MediumBlue -width 35
    Label $row.d -text [G_msg "forward error"] \
        -fg MediumBlue -width 15
    Label $row.e -text [G_msg "backward error"] \
        -fg MediumBlue -width 15
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    for {set gcpnum 1} {$gcpnum < 51 } { incr gcpnum } {
    	if {$gcpnum == 51} {break}
        set GRMap::usegcp($gcpnum) 1
        set row [ frame $gcp.row$gcpnum -bd 0]
        set chk($gcpnum) [checkbutton $row.a \
                -takefocus 0 \
                -variable GRMap::usegcp($gcpnum)]
        set fwd_error($gcpnum) 0.0
        set rev_error($gcpnum) 0.0

        set xy($gcpnum) [entry $row.b -width 35  -bd 0 ]
        bind $xy($gcpnum) <FocusIn> "set xyentry %W"

        set geoc($gcpnum) [entry $row.c -width 35 -bd 0]
        bind $geoc($gcpnum) <FocusIn> "set geoentry %W"

        set fwd($gcpnum) [entry $row.d -width 15 -text GRMap::fwd_error($gcpnum) \
                 -bd 0 -takefocus 0 -textvariable GRMap::fwd_error($gcpnum)]

        set rev($gcpnum) [entry $row.e -width 15 -text GRMap::rev_error($gcpnum) \
                 -bd 0 -takefocus 0 -textvariable GRMap::rev_error($gcpnum) ]

        pack $chk($gcpnum) $xy($gcpnum) $geoc($gcpnum) $fwd($gcpnum) $rev($gcpnum) -side left
        pack $row -side top -fill both -expand yes
    }

    GRMap::get_gcp
    if {$gcpnum >2} {
	    GRMap::gcp_error
	}

    set GRMap::gcpmsg "Forward RMS error = $fwd_rmserror, backward RMS error = $rev_rmserror"

    # set the focus to the first entry
    focus -force $xy(1)

    wm title .gcpwin [G_msg "Manage ground control points (GCPs)"]
    wm withdraw .gcpwin
    wm deiconify .gcpwin

    # cleanup for window closing
    bind .gcpwin <Destroy> {
        if { "%W" == ".gcpwin" } { GRMap::cleanup }
    }

}


# toolbar for gcp manager window
proc GRMap::gcptb { gcptb } {
    global bgcolor
    global iconpath

    # gcp management buttons
    set bbox [ButtonBox $gcptb.bbox1 -spacing 0 -homogeneous 1 ]

    # save
    $bbox add -image [image create photo -file "$iconpath/file-save.gif"] \
        -command "GRMap::savegcp" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Save GCPs to POINTS file"]

    # clear
    $bbox add -image [image create photo -file "$iconpath/gui-gcperase.gif"] \
        -command "GRMap::cleargcp" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Clear all unchecked GCP entries"]

    # rms
    $bbox add -image [image create photo -file "$iconpath/gui-rms.gif"] \
        -command "GRMap::gcp_error" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Calculate RMS error"]

    # rectify
    $bbox add -image [image create photo -file "$iconpath/gui-georect.gif"] \
        -command "GRMap::rectify" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Rectify maps in group"]

    # quit
    $bbox add -text [G_msg "Quit"] \
        -command {destroy .gcpwin .mapgrcan} \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Exit georectifier"]
        
	set helpbtn [Button $gcptb.help -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q gm_georect" \
		-background $bgcolor -borderwidth 1 \
		-helptext [G_msg "Help"]]
		
	    pack $helpbtn -side right -anchor e
        pack $bbox -side left -anchor w -expand no -fill y
        
}


proc GRMap::get_gcp { } {
    # import any existing points file for raster or gcp file for raster or vector

    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable gcpnum
    variable usegcp
    variable xy
    variable geoc
    variable chk
    variable rectorder
    variable fwd_error
    variable rev_error

    set gcpfile "$xygdb/$xyloc/$xymset/group/$xygroup/POINTS"
    if {[file exists $gcpfile] } {
        # do the import
        set gcpnum 1
        catch {set pfile [open $gcpfile]}
        set points [read $pfile]
		if {[catch {close $pfile} error]} {
			GmLib::errmsg $error
		}
        regsub -all {[ ]+} $points " " points
        set plines [split $points "\n"]
        foreach gcpline $plines {
            if {[string match {\#*} $gcpline]} continue
            if {$gcpline == "" } continue
            set gcpline [string trim $gcpline " "]
            set fields [split $gcpline { }]
            # assign variables
            $xy($gcpnum) insert     0 "[lindex $fields 0] [lindex $fields 1]"
            $geoc($gcpnum) insert 0 "[lindex $fields 2] [lindex $fields 3]"
            set usegcp($gcpnum)     "[lindex $fields 4]"
            incr gcpnum
        }
    }
}


# save GCP's to POINTS file in xy location and mapset
proc GRMap::savegcp {} {
    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable currloc
    variable currmset
    variable gcpnum
    variable maptype
    variable xy
    variable geoc
    variable usegcp
    variable array gcpline #array to store gcp coordinates as text for output

    set gcpfile "$xygdb/$xyloc/$xymset/group/$xygroup/POINTS"
    catch {set output [open $gcpfile w ]}
    puts $output "# Ground Control Points File"
    puts $output "# "
    puts $output "# target location: $currloc"
    puts $output "# target mapset: $currmset"
    puts $output "#unrectified xy     georectified east north     1=use gcp point"
    puts $output "#--------------     -----------------------     ---------------"
    set rowcount 0

    for {set gcpnum 1} {$gcpnum < 51 } { incr gcpnum } {
        if { $maptype == "rast" } {
            if { [$xy($gcpnum) get] != "" && [$geoc($gcpnum) get] != ""} {
                set gcpline($gcpnum) "[$xy($gcpnum) get]"
                append gcpline($gcpnum) "     [$geoc($gcpnum) get]"
                append gcpline($gcpnum) "     $usegcp($gcpnum)"
                puts $output $gcpline($gcpnum)
                incr rowcount
            }
        } elseif { $maptype == "vect" } {
            if { [$xy($gcpnum) get] != "" && [$geoc($gcpnum) get] != "" && $usegcp($gcpnum) == 1} {
                set gcpline($gcpnum) "[$xy($gcpnum) get]"
                append gcpline($gcpnum) "     [$geoc($gcpnum) get]"
                append gcpline($gcpnum) "     $usegcp($gcpnum)"
                puts $output $gcpline($gcpnum)
                incr rowcount
            }
        }
    }
    
	if {[catch {close $output} error]} {
		GmLib::errmsg $error
	}

}


proc GRMap::gcp_error { } {
    # calculate error for each gcp and total RMS error - projected forward and reverse

    variable xygdb
    variable xyloc
    variable xymset
    variable xygroup
    variable rectorder
    variable fwd_error
    variable rev_error
    variable errorlist
    variable gcpnum
    variable fwd_rmserror
    variable rev_rmserror

    set fwd_rmssumsq 0.0
    set fwd_rmserror 0.0
    set rev_rmssumsq 0.0
    set rev_rmserror 0.0

    set errorlist ""

    # save current GCP values to POINTS file to use in error calculations
    GRMap::savegcp

    set gcpfile "$xygdb/$xyloc/$xymset/group/$xygroup/POINTS"
    if {![file exists $gcpfile] } { return }

    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc
    # calculate diagonal distance error for each GCP
    catch {set input [open "|g.transform group=$xygroup order=$rectorder"]}
    set errorlist [read $input]
	if {[catch {close $input} error]} {
		GmLib::errmsg $error
	}

    # Return to georectified mapset
    GRMap::resetenv

    set gcpnum 1
    foreach {fwd rev} $errorlist {
        set GRMap::fwd_error($gcpnum) $fwd
        set GRMap::rev_error($gcpnum) $rev
        set fwd_rmssumsq [expr $fwd_rmssumsq + pow($fwd,2)]
        set rev_rmssumsq [expr $rev_rmssumsq + pow($rev,2)]
        incr gcpnum
    }

    # calculate total rms error for all points
    set GRMap::fwd_rmserror [expr sqrt($fwd_rmssumsq/$gcpnum)]
    set GRMap::rev_rmserror [expr sqrt($rev_rmssumsq/$gcpnum)]

    # update value in status bar
    set GRMap::gcpmsg "Forward RMS error = $fwd_rmserror, backward RMS error = $rev_rmserror"
}


# run i.rectify to rectify raster group or v.transform to rectify vector
proc GRMap::rectify { } {
    variable xygdb
    variable xyloc
    variable currloc
    variable xymset
    variable currmset
    variable xygroup
    variable xyvect
    variable maptype
    variable mappid
    variable selftarget
    variable rectorder


    set gcpfile "$xygdb/$xyloc/$xymset/group/$xygroup/POINTS"
    if {![file exists $gcpfile] } {
            set msg [G_msg "There is no POINTS file of ground control points for group.\
                    You must create ground control points before georectifying map."]
            tk_messageBox -message $msg -parent .gcpwin -type ok
            return
    }

    # count useable GCP's in points file
    set gcpcnt 0
    catch {set pfile [open $gcpfile]}
    set points [read $pfile]
	if {[catch {close $pfile} error]} {
		GmLib::errmsg $error
	}

    regsub -all {[ ]+} $points " " points
    set plines [split $points "\n"]
    foreach gcpline $plines {
        if {[string match {\#*} $gcpline]} continue
        if {$gcpline == "" } continue
        set gcpline [string trim $gcpline " "]
        set fields [split $gcpline { }]
        # count gcps
        if {[lindex $fields 0]!="" && [lindex $fields 1]!="" && [lindex $fields 2]!=""
            && [lindex $fields 3]!="" && [lindex $fields 4]==1} {
            incr gcpcnt
        }
    }

    if { $maptype == "rast" } {
        if { $gcpcnt<3 || ($gcpcnt<6 && $rectorder==2) || ($gcpcnt<10 && $rectorder==3) } {
            set msg [G_msg "Insufficient ground control points for georectification method.\
                    You need at least 3 points for 1st order, 6 points for 2nd order and 10 points for 3rd order."]
            tk_messageBox -message $msg -parent .gcpwin -type ok
            return
        }
        # run i.rectify on raster group
        # First, switch to xy mapset
        GRMap::setxyenv $xymset $xyloc
        
#         set message_env [exec g.gisenv get=GRASS_MESSAGE_FORMAT]
#         set env(GRASS_MESSAGE_FORMAT) gui
# 		if {![catch {open [concat "|i.rectify" "-ca" "group=$xygroup" "extension=$mappid" "order=$rectorder"] r} fh]} {
# 			while {[gets $fh line] >= 0} {
# 				puts "line = $line"
# 			}
# 		}
# 
#         set env(GRASS_MESSAGE_FORMAT) $message_env
#         
# 		if {[catch {close $fh} error]} {
# 			puts $error
# 		}

        set cmd "i.rectify -ca group=$xygroup extension=$mappid order=$rectorder"
        
        run_panel $cmd
        # Return to georectified mapset
        GRMap::resetenv
    } elseif { $maptype == "vect" } {
        if { $gcpcnt < 1 } {
            set msg [G_msg "No valid ground control points in GCP file.\
            You must create valid ground control points before georectifying map."]
            tk_messageBox -message $msg -parent .gcpwin -type ok
            return
        }

        # loop to rectify all vectors in VREF file using v.transform
        GRMap::read_vgroup $xygroup
        set vlist [split $xyvect ,]
        set outcount 1
        foreach vect $vlist {
            set outname "$vect"
            append outname "_$mappid"
            # First, switch to xy mapset
            GRMap::setxyenv $xymset $xyloc
            set cmd "v.transform --q input=$vect output=$outname pointsfile=$gcpfile"
            runcmd $cmd
            # Return to georectified mapset
            GRMap::resetenv
            # copy vector file from source to target location and mapset
            if { $selftarget == 0 } {
                set xysource "$xygdb/$xyloc/$xymset/vector/$outname"
                set xytarget "$xygdb/$currloc/$currmset/vector/$outname"
                set xyfile "$xysource"
                append xyfile "/coor"
                set counter 1
                # wait to make sure georectified file is written
                while { $counter < 100 } {
                    if { [file exists $xyfile] } {
                        catch {file copy -force $xysource $xytarget}
                        catch {file delete -force $xysource}
                        set counter 101
                    }
                    after 100
                    incr counter
                }
            }
        }
    } else {
        GRMap::resetenv
        return
    }
}


# clear all GCP entries
proc GRMap::cleargcp {} {
    variable xy
    variable geoc
    variable usegcp
    variable fwd
    variable rev
    variable gcpnum
    variable grcan

    for {set gcpnum 1} {$gcpnum < 51 } { incr gcpnum } {
        if {$usegcp($gcpnum) == 0} {
	        $xy($gcpnum) delete 0 end
    	    $geoc($gcpnum) delete 0 end
        	$fwd($gcpnum) delete 0 end
	        $rev($gcpnum) delete 0 end
		}
    }
    $grcan delete gcpvert gcphoriz
}

###############################################################################

# Calculate boxes with a given aspect ratio.

# Sense - 0 means largest no larger, 1 means smallest no smaller
# We will change box 1
proc GRMap::shrinkwrap {sense nsew1 ar2 } {
    foreach {n1 s1 e1 w1} $nsew1 {break}

    set ns1 [expr {$n1 - $s1}]
    set ew1 [expr {$e1 - $w1}]

    # Width / height
    # Big aspect ratio is wide, small aspect ratio is tall
    set ar1 [expr { 1.0 * $ew1 / $ns1 }]

    # If box one is wider than box 2.
    # (or box one isn't wider than box 2 and the sense is inverted)
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

###############################################################################
# map display procedures

# draw map using png driver and open in canvas
proc GRMap::drawmap { } {
    variable grcanvas_h
    variable grcanvas_w
    variable grcan
    variable grcanmodified
    variable monitor_zooms
    variable previous_monitor
    variable xyloc
    variable xymset

    set w [winfo width $grcan]
    set h [winfo height $grcan]

    # Get whether or not the canvas was modified or zoomed
    # grcanmodified has levels: 0  is none, 1 is zoom, 2 is geometry.
    # 1 doesn't require new setting in explore mode
    set mymodified $grcanmodified

    # Make sure grcanvas_h and grcanvas_w are correct
    if { $grcanvas_w != $w || $grcanvas_h != $h } {
        # Flag this as a modified canvas
        # Modified canvas is level 2!
        set mymodified 2
        set grcanvas_w $w
        set grcanvas_h $h
    }

    # Redo the driver settings if the geometry has changed or
    # if we weren't the previous monitor.
    if {$mymodified != 0 } {
        set grcanmodified 0
        set previous_monitor "none"
        # The canvas or view has been modified
        # Redo the map settings to match the canvas
        GRMap::driversettings
    }

    # Render all the layers
    GRMap::runprograms [expr {$mymodified != 0}]
}

# Run the programs to clear the map and draw all of the layers
proc GRMap::runprograms { mod } {
    variable grcan
    variable grcanvas_w
    variable grcanvas_h
    variable grmapframe
    variable grfile
    variable tmpdir
    variable xygroup
    variable xygdb
    variable xyloc
    variable xymset
    variable xymap
    variable maptype
    variable xygroup
    variable gregionproj
    variable msg
    variable zoom_attrs
    variable gcpnum
	variable xy

    global env
    global drawprog
    global devnull

    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc

    set drawprog 0

	set gregion ""

	# Create a settings string to use with GRASS_WIND. 
	# First get the current region values in normal number form (including decimal degrees)
	set values [GRMap::currentzoom]
	set options {}
	foreach attr $zoom_attrs value $values {
		lappend options "$attr=$value"
	}

	# Now use the region values to get the region printed back out in -p format
	# including lat long now as dd:mm:ss
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
		regexp -nocase {^.* (\(.*\))} $parts(projection) trash end
		set parts(projection) [string trim $parts(projection) $end]

		set gregion "projection:$parts(projection); zone:$parts(zone); north:$parts(north); south:$parts(south); east:$parts(east); west:$parts(west); e-w resol:$parts(ewres);	 n-s resol:$parts(nsres)"
	}

    set GRMap::msg [G_msg "Please wait..."]
    $grmapframe showstatusbar progression

    incr drawprog
    # only use dynamic region for display geometry; use WIND for computational geometry
    set env(GRASS_REGION) $gregion

    # Setting the font really only needs to be done once per display start
    incr drawprog
    # display map for georectification
    if { $maptype == "rast" } {
            set cmd "d.rast map=$xymap"
    } elseif { $maptype == "vect" } {
            set cmd "d.vect map=$xymap"
    }
    runcmd $cmd

    unset env(GRASS_REGION)

    incr drawprog

    $grcan delete all
    set currdir [pwd]
    cd $tmpdir
    incr drawprog

    image create photo grimg -file "$grfile"
    incr drawprog
    $grcan create image 0 0 -anchor nw \
            -image "grimg" \
            -tag gr
    cd $currdir

    GRMap::coordconv

	#draw gcp marks
	set gcpnum 1
	if {$xy(1) != ""} {
		#draw GCP marks from GCP form
		for {set gcpnum 1} {$gcpnum < 51 } { incr gcpnum } {
			if {[$xy($gcpnum) get] != "" } {
				set xyfields [split [$xy($gcpnum) get] { }]
				set mapx [lindex $xyfields 0]
				set mapy [lindex $xyfields 1]
				set x [eval GRMap::mape2scrx $mapx]
				set y [eval GRMap::mapn2scry $mapy]
				GRMap::markgcp $x $y
			}
        }
	} else {
		#draw GCP marks from any existing POINTS file
		set gcpfile "$xygdb/$xyloc/$xymset/group/$xygroup/POINTS"
		if {[file exists $gcpfile] } {
			# do the import
			set gcpnum 1
			catch {set pfile [open $gcpfile]}
			set points [read $pfile]
			if {[catch {close $pfile} error]} {
				GmLib::errmsg $error
			}

			regsub -all {[ ]+} $points " " points
			set plines [split $points "\n"]
			foreach gcpline $plines {
				if {[string match {\#*} $gcpline]} continue
				if {$gcpline == "" } continue
				set gcpline [string trim $gcpline " "]
				set fields [split $gcpline { }]
				# assign variables            
				set mapx [lindex $fields 0]      
				set mapy  [lindex $fields 1]
				set x [eval GRMap::mape2scrx $mapx]
				set y [eval GRMap::mapn2scry $mapy]
				GRMap::markgcp $x $y
			}
		}
	}
	
    set drawprog 100

    set drawprog 0
    set GRMap::msg [G_msg "Georectifying maps in $xygroup group"]

    $grmapframe showstatusbar status

    # Return to georectified mapset
    GRMap::resetenv
    return

}

# set up driver geometry and settings
proc GRMap::driversettings { } {
    variable grcanvas_h
    variable grcanvas_w
    variable driver_w
    variable driver_h
    variable grfile
    variable xyloc
    variable xymset
    variable monitor_zooms
    global env

    set driver_h $grcanvas_h
    set driver_w $grcanvas_w

    #set display environment
    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc

    set env(GRASS_WIDTH) "$driver_w"
    set env(GRASS_HEIGHT) "$driver_h"
    set env(GRASS_PNGFILE) "$grfile"
    set env(GRASS_BACKGROUNDCOLOR) "ffffff"
    set env(GRASS_TRANSPARENT) "FALSE"
    set env(GRASS_PNG_AUTO_WRITE) "TRUE"
    set env(GRASS_TRUECOLOR) "TRUE"

    # Return to georectified mapset
    GRMap::resetenv
}


###############################################################################
# map display server
# The job of these procedures is to make sure that:
# 1: we are never running more than one update at once.
# 2: we don't do exactly the same update multiple times.

proc GRMap::display_server {} {
    variable redrawrequest
    variable gcpnum
    variable xy
    variable drawform
    
    set gcpcnt 0

    if {$redrawrequest} {
        # Mark that this monitor no longer wants to be redrawn
        set redrawrequest 0
        # Redraw the monitor canvas
        GRMap::drawmap
    }

    # Do me again in a short period of time.
    # vwait might be appropriate here
    after 100 GRMap::display_server
}

# Request a redraw on a monitor
proc GRMap::request_redraw {modified} {
    variable redrawrequest
    variable grcanmodified

    set redrawrequest 1

    set grcanmodified $modified
}

# Start the server
after idle GRMap::display_server

###############################################################################

proc GRMap::do_resize {} {
    variable grcanvas_w
    variable grcanvas_h
    variable grcan


    # Get the actual width and height of the canvas
    set w [winfo width $grcan]
    set h [winfo height $grcan]

    # Only actually resize and redraw if the size is different
    if { $grcanvas_w != $w || $grcanvas_h != $h } {
        $grcan delete gr
        GRMap::request_redraw 1
    }
}


###############################################################################

# erase to white
proc GRMap::erase { } {
    variable grcan

    $grcan delete gr
    $grcan delete all
}


###############################################################################

# stop display management tools
proc GRMap::stoptool { } {
    variable msg
    variable grcan
    variable maptype
    variable xygroup

    # release bindings
    bind $grcan <1> ""
    bind $grcan <2> ""
    bind $grcan <3> ""
    bind $grcan <B1-Motion> ""
    bind $grcan <ButtonRelease-1> ""

    # reset status display to normal
    set GRMap::msg [G_msg "Georectifying maps in $xygroup group"]

    GRMap::restorecursor
}

###############################################################################
# set bindings for GCP selection tool
proc GRMap::selector { } {
    variable grcan
    variable grcoords_mov
    global grcoords
    global xyentry

    GRMap::setcursor "crosshair"

    bind $grcan <ButtonPress-1> {
        set eastcoord [eval GRMap::scrx2mape %x]
        set northcoord [eval GRMap::scry2mapn %y]
        set grcoords "$eastcoord $northcoord"
        GRMap::markgcp %x %y
        $xyentry delete 0 end
        $xyentry insert 0 $grcoords
        focus -force [tk_focusNext $xyentry]
    }

    # Displays geographic coordinates in indicator window when cursor moved across canvas
    bind $grcan <Motion> {
        set scrxmov %x
        set scrymov %y
        set eastcoord [eval GRMap::scrx2mape %x]
        set northcoord [eval GRMap::scry2mapn %y]
        set grcoords_mov "$eastcoord $northcoord"
    }
}

###############################################################################
# mark ground control point
proc GRMap::markgcp { x y } {

    # create gcp point on georectify canvas for each mouse click

    variable grcan
    $grcan create line $x [expr $y-5] $x [expr $y+5] -tag gcpv \
            -fill DarkGreen -width 2 -tag "gcpvert"
    $grcan create line [expr $x-5] $y [expr $x+5] $y -tag gcph \
            -fill red -width 2 -tag "gcphoriz"
}


###############################################################################
# procedures for zooming and setting region

# Get the current zoom region
# Returns a list in zoom_attrs order (n s e w nsres ewres)
proc GRMap::currentzoom { } {
    variable zoom_attrs
    variable monitor_zooms
    variable grcanvas_w
    variable grcanvas_h
    variable xyloc
    variable xymset

    # Fetch the current zoom settings
    set region {}
    foreach attr $zoom_attrs {
        lappend region $monitor_zooms(1,$attr)
    }

    # Set the region to the smallest region no smaller than the canvas
    set grcanvas_ar [expr {1.0 * $grcanvas_w / $grcanvas_h}]
    set expanded_nsew [GRMap::shrinkwrap 1 [lrange $region 0 3] $grcanvas_ar]
    foreach {n s e w} $expanded_nsew {break}
    # Calculate the resolutions
    lappend expanded_nsew [expr {1.0 * ($n - $s) / $grcanvas_h}]
    lappend expanded_nsew [expr {1.0 * ($e - $w) / $grcanvas_w}]
    set region $expanded_nsew

    # region contains values for n s e w ewres nsres
    return $region
}

# Zoom or pan to new bounds in the zoom history
# Arguments are either n s e w or n s e w nsres ewres
proc GRMap::zoom_new { args} {
    variable monitor_zooms
    variable zoomhistories
    variable zoom_attrs
    variable xyloc
    variable xymset

    # Demote all of the zoom history
    for {set i $zoomhistories} {$i > 1} {incr i -1} {
        set iminus [expr {$i - 1}]
        foreach attr $zoom_attrs {
            catch {set monitor_zooms($i,$attr) $monitor_zooms($iminus,$attr)}
        }
    }

    # If cols and rows aren't present we just use what was already here.
    set present_attrs [lrange $zoom_attrs 0 [expr {[llength $args] - 1}]]

    foreach value $args attr $present_attrs {
        set monitor_zooms(1,$attr) $value
    }

}

# Zoom to the previous thing in the zoom history
proc GRMap::zoom_previous {} {
    variable monitor_zooms
    variable zoomhistories
    variable zoom_attrs
    variable xyloc
    variable xymset

    # Remember the first monitor
    set old1 {}
    foreach attr $zoom_attrs {
        lappend old1 $monitor_zooms(1,$attr)
    }

    # Promote all of the zoom history
    for {set i 1} {$i < $zoomhistories } {incr i} {
        set iplus [expr {$i + 1}]
        foreach attr $zoom_attrs {
            catch {set monitor_zooms($i,$attr) $monitor_zooms($iplus,$attr)}
        }
    }

    # Set the oldest thing in the history to where we just were
    foreach value $old1 attr $zoom_attrs {
        set monitor_zooms($zoomhistories,$attr) $value
    }

}


# Zoom to something loaded from a g.region command
proc GRMap::zoom_gregion { args} {
    variable xyloc
    variable xymset
    variable gregionproj
    global env
    global devnull

    # First, switch to xy mapset
    GRMap::setxyenv $xymset $xyloc

    if {![catch {open [concat "|g.region" "-up" $args "2> $devnull"] r} input]} {
        while {[gets $input line] >= 0} {
			set key [string trim [lindex [split $line ":"] 0]]
			set value [string trim [lindex [split $line ":"] 1]]
            set value [string trim $value "(UTM)"]
            set value [string trim $value "(x,y)"]
            set value [string trim $value]
            set parts($key) $value
        }
		if {[catch {close $input} error]} {
			GmLib::errmsg $error ["Error setting region"]
		}

        GRMap::zoom_new $parts(north) $parts(south) $parts(east) $parts(west) $parts(nsres) $parts(ewres)
        set gregionproj "proj: $parts(projection); zone: $parts(zone); "
    }

    # Return to georectified mapset
    GRMap::resetenv
}


# zoom to extents and resolution of displayed map for georectifying
proc GRMap::zoom_map { } {
    variable xymap
    variable xymset
    variable xyloc
    variable maptype
    variable grcan

    # set region to match map to georectify
    if { $maptype == "rast" } {
        GRMap::zoom_gregion [list "rast=$xymap"]
    } elseif { $maptype == "vect" } {
        GRMap::zoom_gregion [list "vect=$xymap"]
    }

    $grcan delete gr
    GRMap::request_redraw 1

}


# zoom back
proc GRMap::zoom_back { } {
    variable grcan

    GRMap::zoom_previous
    $grcan delete gr
    GRMap::request_redraw 1
}


###############################################################################
# interactive zooming procedures
# zoom bindings
proc GRMap::zoombind { zoom } {
    variable grcan
    variable msg
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2
	variable grcoords_mov

    # initialize zoom rectangle corners

    set areaX1 0
    set areaY1 0
    set areaX2 0
    set areaY2 0

    GRMap::setcursor "plus"

    if {$zoom == 1} {
        set GRMap::msg [G_msg "Drag or click mouse to zoom"]
    } elseif {$zoom == -1} {
        set GRMap::msg [G_msg "Drag or click mouse to unzoom"]
    }

    bind $grcan <1> {
        GRMap::markzoom %x %y
        }
    bind $grcan <B1-Motion> {
        set scrxmov %x
        set scrymov %y
        set eastcoord [eval GRMap::scrx2mape %x]
        set northcoord [eval GRMap::scry2mapn %y]
        set grcoords_mov "$eastcoord $northcoord"
        GRMap::drawzoom %x %y
        }
    bind $grcan <ButtonRelease-1> "GRMap::zoomregion $zoom"

}

# start zoom rectangle
proc GRMap::markzoom { x y} {
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2
    variable grcan

    # initialize corners
    set areaX1 0
    set areaY1 0
    set areaX2 0
    set areaY2 0
    set areaX1 [$grcan canvasx $x]
    set areaY1 [$grcan canvasy $y]
    $grcan delete area
}

# draw zoom rectangle
proc GRMap::drawzoom { x y } {
    variable grcan
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2

    set xc [$grcan canvasx $x]
    set yc [$grcan canvasy $y]

    if {($areaX1 != $xc) && ($areaY1 != $yc)} {
        $grcan delete area
        $grcan addtag area withtag \
            [$grcan create rect $areaX1 $areaY1 $xc $yc \
            -outline yellow -width 2]
        set areaX2 $xc
        set areaY2 $yc
    }
}


# zoom region
proc GRMap::zoomregion { zoom } {
    variable grcan
    variable grcanvas_h
    variable grcanvas_w
    variable monitor_zooms
	variable areaX1 
	variable areaY1 
	variable areaX2 
	variable areaY2

    variable map_n
    variable map_s
    variable map_e
    variable map_w
    variable map_ew
    variable map_ns

	set clickzoom 0

	# get display extents in geographic coordinates
	set dispnorth [scry2mapn 0]
	set dispsouth [scry2mapn $grcanvas_h]
	set dispeast  [scrx2mape $grcanvas_w]
	set dispwest  [scrx2mape 0]

	# get zoom rectangle extents in geographic coordinates
	if { $areaX2 < $areaX1 } {
			set cright $areaX1
			set cleft $areaX2
	} else {
			set cleft $areaX1
			set cright $areaX2
	}

	if { $areaY2 < $areaY1 } {
			set cbottom $areaY1
			set ctop $areaY2
	} else {
			set ctop $areaY1
			set cbottom $areaY2
	}

	set north [scry2mapn $ctop]
	set south [scry2mapn $cbottom]
	set east  [scrx2mape $cright]
	set west  [scrx2mape $cleft]
	# (this is all you need to zoom in with box)


	# if click and no drag, zoom in or out by fraction of original area and center on the click spot
	if {($areaX2 == 0) && ($areaY2 == 0)} {set clickzoom 1}
	# get first click location in map coordinates for recentering with 1-click zooming
	set newcenter_n [scry2mapn $areaY1]
	set newcenter_e [scrx2mape $areaX1]	

	# get current region extents for box zooming out and recentering	
	foreach {map_n map_s map_e map_w nsres ewres} [GRMap::currentzoom] {break}

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

	GRMap::zoom_new $north $south $east $west $nsres $ewres

    # redraw map
    $grcan delete gr
    $grcan delete area
    GRMap::request_redraw  1
}



###############################################################################
#procedures for panning

# pan bindings
proc GRMap::panbind { } {
    variable grcan
    variable msg
    variable grcoords_mov

    set GRMap::msg [G_msg "Drag with mouse to pan"]

    GRMap::setcursor "hand2"

    bind $grcan <1> {GRMap::startpan %x %y}
    bind $grcan <B1-Motion> {
        set scrxmov %x
        set scrymov %y
        set eastcoord [eval GRMap::scrx2mape %x]
        set northcoord [eval GRMap::scry2mapn %y]
        set grcoords_mov "$eastcoord $northcoord"
        GRMap::dragpan %x %y
        }
    bind $grcan <ButtonRelease-1> {
        GRMap::pan
        }
}


proc GRMap::startpan { x y} {
    variable start_x
    variable start_y
    variable from_x
    variable from_y
    variable to_x
    variable to_y
    variable grcan

    set start_x [$grcan canvasx $x]
    set start_y [$grcan canvasy $y]
    set from_x $start_x
    set from_y $start_y
    set to_x $start_x
    set to_y $start_y

}

proc GRMap::dragpan { x y} {
    variable start_x
    variable start_y
    variable from_x
    variable from_y
    variable to_x
    variable to_y
    variable grcan

    set to_x [$grcan canvasx $x]
    set to_y [$grcan canvasy $y]
    $grcan move current [expr {$to_x-$start_x}] [expr {$to_y-$start_y}]

    set start_y $to_y
    set start_x $to_x
}

proc GRMap::pan { } {
    variable start_x
    variable start_y
    variable from_x
    variable from_y
    variable to_x
    variable to_y
    variable grcan
    variable monitor_zooms
    variable map_n
    variable map_s
    variable map_e
    variable map_w
    variable map_ew
    variable map_ns

    # get map coordinate shift
    set from_e [scrx2mape $from_x]
    set from_n [scry2mapn $from_y]
    set to_e   [scrx2mape $to_x]
    set to_n   [scry2mapn $to_y]

    # get region extents
    foreach {map_n map_s map_e map_w} [GRMap::currentzoom] {break}

    # set new region extents
    set north [expr {$map_n - ($to_n - $from_n)}]
    set south [expr {$map_s - ($to_n - $from_n)}]
    set east  [expr {$map_e - ($to_e - $from_e)}]
    set west  [expr {$map_w - ($to_e - $from_e)}]

    # reset region and redraw map
    GRMap::zoom_new $north $south $east $west

    $grcan delete gr
    GRMap::request_redraw 1
}

###############################################################################

proc GRMap::setcursor {  ctype } {
    variable grcan

    $grcan configure -cursor $ctype
    return
}

proc GRMap::restorecursor {} {
    variable grcursor
    variable grcan

    $grcan configure -cursor $grcursor
    return
}

###############################################################################

# Set up initial variables for screen to map conversion
proc GRMap::coordconv { } {

    variable map_n
    variable map_s
    variable map_e
    variable map_w
    variable map_ew
    variable map_ns
    variable scr_n
    variable scr_s
    variable scr_e
    variable scr_w
    variable scr_ew
    variable scr_ns
    variable map2scrx_conv
    variable map2scry_conv
    variable grcanvas_w
    variable grcanvas_h
    variable monitor_zooms

    # get region extents
    foreach {map_n map_s map_e map_w} [GRMap::currentzoom] {break}

    # calculate dimensions

    set map_n [expr {1.0*($map_n)}]
    set map_s [expr {1.0*($map_s)}]
    set map_e [expr {1.0*($map_e)}]
    set map_w [expr {1.0*($map_w)}]

    set map_ew [expr {$map_e - $map_w}]
    set map_ns [expr {$map_n - $map_s}]


    # get current screen geometry
    if { [info exists "grimg"] } {
        set scr_ew [image width "grimg"]
        set scr_ns [image height "grimg"]
        set scr_e [image width "grimg"]
        set scr_s [image height "grimg"]
    } else {
        set scr_ew $grcanvas_w
        set scr_ns $grcanvas_h
        set scr_e $grcanvas_w
        set scr_s $grcanvas_h
    }
    set scr_n 0.0
    set scr_w 0.0


    # calculate conversion factors. Note screen is from L->R, T->B but map
    # is from L->R, B->T

    set map2scrx_conv [expr {$scr_ew / $map_ew}]
    set map2scry_conv [expr {$scr_ns / $map_ns}]

    # calculate screen dimensions and offsets

    if { $map2scrx_conv > $map2scry_conv } {
        set map2scrx_conv $map2scry_conv
    } else {
        set map2scry_conv $map2scrx_conv
    }

}

###############################################################################


# screen to map and map to screen conversion procedures

# map north to screen y
proc GRMap::mapn2scry { north } {
    variable map_n
    variable scr_n
    variable map2scry_conv

    return [expr {$scr_n + (($map_n - $north) * $map2scry_conv)}]
}

# map east to screen x
proc GRMap::mape2scrx { east } {
    variable map_w
    variable scr_w
    variable map2scrx_conv

    return [expr {$scr_w + (($east - $map_w) * $map2scrx_conv)}]

}

# screen y to map north
proc GRMap::scry2mapn { y } {
    variable map_n
    variable scr_n
    variable map2scry_conv

    return [expr {$map_n - (($y - $scr_n) / $map2scry_conv)}]
}

# screen x to map east
proc GRMap::scrx2mape { x } {
    variable map_w
    variable scr_w
    variable map2scrx_conv

    return [expr {$map_w + (($x - $scr_w) / $map2scrx_conv)}]

}

###############################################################################
# cleanup procedure on closing window
proc GRMap::cleanup { } {
	variable xy
	
    if { [winfo exists .gcpwin] } { destroy .gcpwin }
    if { [winfo exists .mapgrcan] } { destroy .mapgrcan }

	for {set gcpnum 1} {$gcpnum < 51 } { incr gcpnum } {
		if {[info exists xy($gcpnum)]} {
			unset xy($gcpnum)
		}
	}

    # reset to original location and mapset
    GRMap::resetenv

}

###############################################################################
