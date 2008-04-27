################################################################################
#   
#       FILE:       dnviz.tcl
#   
#       PURPOSE:    Permits interactive creation of flythrough path for nviz
#                   using nviz
#  
#       AUTHOR:     Michael Barton, Arizona State University
#       COPYRIGHT:  (C) 2007 by the GRASS Development Team
#                   This program is free software under the GNU General Public
#                   License (>=v2). Read the file COPYING that comes with GRASS
#                   for details.
#
#
################################################################################




namespace eval GmDnviz {
    variable inmap
    variable outfile
    variable pathcoords
    variable prefix
    variable route
    variable layback
    variable height
    variable frames
    variable startframe
    variable usemouse
    variable coord_entry
    variable vrender
    variable fullrender
    variable offscreen
    variable ht_elev
    variable keyframe
    variable overwrite
	global env

}
# G_msg.tcl should be sourced first for internationalized strings.


###############################################################################

# select input map
proc GmDnviz::select_map { seltype var } {

    set m [GSelect $seltype title [G_msg "Select input map"] parent "."]
    if { $m != "" } {
        set $var $m
    }
}

###############################################################################

#Create main panel for d.nviz parameter entry
proc GmDnviz::main { mapcan mon } {
    variable inmap
    variable outfile
    variable pathcoords
    variable prefix
    variable route
    variable layback
    variable height
    variable frames
    variable startframe
    variable usemouse
    variable coord_entry
    variable vrender
    variable fullrender
    variable offscreen
    variable ht_elev
    variable keyframe
    variable overwrite
	global env
	global iconpath
	global bgcolor
		
	#initialize variables
	set inmap ""
	set outfile ""
	set pathcoords ""
	set overwrite 0
	set prefix "NVIZ"
	set route ""
	set layback 2000
	set height 1000
	set frames 50
	set startframe 0
	set usemouse 0
    set vrender 0
    set fullrender 0
    set offscreen 0
    set ht_elev 0
    set keyframe 0
    set overwrite

	set overwrite 0
	set selclr #88aa88
        
	# create nviz path input window
    if { [winfo exists .dnvizPopup] } {return}

	set dnviz_win [toplevel .dnvizPopup]
	wm title $dnviz_win [ G_msg "NVIZ flythrough path" ]
	# put it in the middle of the screen
	update idletasks
	set winWidth [winfo reqwidth $dnviz_win]
	set winHeight [winfo reqheight $dnviz_win]
	set scrnWidth [winfo screenwidth $dnviz_win]
	set scrnHeight [winfo screenheight $dnviz_win]
	set x [expr ($scrnWidth - $winWidth) / 2-250]
	set y [expr ($scrnHeight  - $winHeight) / 2]
	wm geometry $dnviz_win +$x+$y
	wm deiconify $dnviz_win
                
	#create the form and buttons

    # Title
    set row [ frame $dnviz_win.heading ]
    Label $row.a -text [G_msg "Create flythough path for NVIZ display"] \
        -fg MediumBlue
    pack $row.a -side top -padx 5 -pady 3
    pack $row -side top -fill x -expand yes

    # input map
    set row [ frame $dnviz_win.input ]
    Label $row.a -text [G_msg "Raster surface map "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -command "GmDnviz::select_map cell GmDnviz::inmap"
    Entry $row.c -width 50 -text "$inmap" \
          -textvariable GmDnviz::inmap
    pack $row.c $row.b $row.a -side right -padx 3 -anchor e
    pack $row -side top -fill x -expand no -padx 5
	
    # set output script file
    set row [ frame $dnviz_win.output ]
    Label $row.a -text [G_msg "Output script file "]
    Button $row.b -image [image create photo -file "$iconpath/file-open.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -command "GmDnviz::select_map file GmDnviz::outfile"
    Entry $row.c -width 50 -text "$outfile" \
          -textvariable GmDnviz::outfile
    pack $row.c $row.b $row.a -side right -anchor e -padx 3
    pack $row -side top -fill x -expand no -padx 5
    
    # flythrough path coordinates
    set row [ frame $dnviz_win.coords ]
    Label $row.a -text [G_msg "Flythrough path "]
    set ck_path [checkbutton $row.b -image [image create photo -file "$iconpath/gui-mouse.gif"] \
        -variable "GmDnviz::usemouse" -command "GmDnviz::setmouse $mapcan $mon"\
        -indicatoron false -selectcolor $selclr -bg $bgcolor \
        -highlightthickness 0 -borderwidth 1  \
		-activebackground $bgcolor -highlightbackground $bgcolor ]
    	DynamicHelp::register $ck_path balloon [G_msg "Create path with mouse in map display"]
    set coord_entry [Entry $row.c -width 50 -text "$outfile" \
        -textvariable GmDnviz::pathcoords  -width 50 -xscrollcommand "$row.d set" \
        -helptext [G_msg "Coordinate pairs for flythrough path (x1,y1,x2,y2,...)"]]
	scrollbar $row.d -relief sunken -command "$coord_entry xview" -orient horizontal
    pack $row.d -side bottom -fill x -expand no -anchor e
    pack $row.c $row.b $row.a -side right -anchor e -padx 3
    pack $row -side top -fill x -expand no -padx 5
    
    # set output images prefix
    set row [ frame $dnviz_win.prefix ]
    LabelEntry $row.a -textvariable GmDnviz::prefix -width 50 \
        -label [G_msg "Flythrough images prefix "] \
        -helptext [G_msg "Prefix for image series created by flythough"]
    pack $row.a -side right -anchor e -padx 3
    pack $row -side top -fill x -expand no -padx 5
    
    # Camera position
    set row [ frame $dnviz_win.camera ]
    LabelEntry $row.a -textvariable GmDnviz::layback -width 10 \
        -label [G_msg "Camera layback "] \
        -helptext [G_msg "Camera layback distance (in map units)"]
    LabelEntry $row.b -textvariable GmDnviz::height -width 10 \
        -label [G_msg "Camera height "] \
        -helptext [G_msg "Camera height above terrain"]
    pack $row.a $row.b -side right -anchor e -padx 3
    pack $row -side top -fill x -expand no -padx 5
   
    # Frame settings
    set row [ frame $dnviz_win.frames ]
    LabelEntry $row.a -textvariable GmDnviz::frames -width 10 \
        -label [G_msg "Number of frames "] \
        -helptext [G_msg "Number of frames to create for flythrough"]
    LabelEntry $row.b -textvariable GmDnviz::startframe -width 10 \
        -label [G_msg "Start frame "] \
        -helptext [G_msg "Starting frame number..."]
    pack $row.a $row.b -side right -anchor e -padx 3
    pack $row -side top -fill x -expand no -padx 5
    
    
    set row [ frame $dnviz_win.options1 ]
    checkbutton $row.a -variable GmDnviz::vrender \
                -text [G_msg "Enable vector rendering"]
    checkbutton $row.b -variable GmDnviz::fullrender \
                -text [G_msg "Full render (save images)"]
    checkbutton $row.c -variable GmDnviz::offscreen \
                -text [G_msg "Render images offscreen"]
    pack $row.a $row.b $row.c -side left -anchor w -padx 5
    pack $row -side top -fill x -expand no
    
    set row [ frame $dnviz_win.options2 ]
    Button $row.a -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.nviz" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a -side right -anchor e -padx 5
    checkbutton $row.b -variable GmDnviz::ht_elev \
                -text [G_msg "Height value is elevation"]
    checkbutton $row.c -variable GmDnviz::keyframe \
                -text [G_msg "Output keyframe file"]
    checkbutton $row.d -variable GmDnviz::overwrite \
                -text [G_msg "Overwrite existing file"]
    pack $row.b $row.c $row.d -side left -anchor w -padx 5
    pack $row -side top -fill x -expand no
    
    set row [ frame $dnviz_win.buttons ]
    Button $row.d -text [G_msg "Reset"] -width 8 -bd 1 \
    	-command "GmDnviz::reset_points $mon %W" \
    	-helptext [G_msg "Clear all path coordinates"]
    pack $row.d -side left -padx 5 -fill none -expand no -anchor w
    Button $row.a -text [G_msg "OK"] -width 8 -bd 1 \
    	-command "GmDnviz::makescript %W 1" -default active
    Button $row.b -text [G_msg "Cancel"] -width 8 -bd 1 \
    	-command "destroy .dnvizPopup"
    Button $row.c -text [G_msg "Apply"] -width 8 -bd 1 \
    	-command "GmDnviz::makescript %W 0"
    pack $row.a $row.b $row.c -side right -padx 5 -expand no -anchor e
    pack $row -side bottom -pady 3 -padx 5 -expand yes -fill none

	bind .dnvizPopup <Destroy> "GmDnviz::cleanup $mon %W"    

}

###############################################################################
# interactive mouse input of path coordinates
proc GmDnviz::setmouse {mapcan mon} {
    variable usemouse
    
    if {$usemouse == 0} {
        MapCanvas::pointer $mon
        return
    }
    
    MapCanvas::setcursor $mon "crosshair"

    bind $mapcan <ButtonPress-1> "GmDnviz::getcoords $mapcan $mon %x %y"
    
}

###############################################################################
# set coordinate point from mouse click
proc GmDnviz::getcoords {mapcan mon x y} {
    
    variable coord_entry
    
    set eastcoord [eval MapCanvas::scrx2mape $mon $x]
    set northcoord [eval MapCanvas::scry2mapn $mon $y]
    set grcoords "$eastcoord,$northcoord,"
    GmDnviz::markpoint $mapcan $x $y
    $coord_entry insert end $grcoords
   # focus -force [tk_focusNext $coord_entry]

}


###############################################################################
# mark ground control point
proc GmDnviz::markpoint { mapcan x y } {

    # create point for flythrough path on map display canvas for each mouse click

    $mapcan create line $x [expr $y-5] $x [expr $y+5] -tag gcpv \
            -fill DarkGreen -width 2 -tag "ptvert"
    $mapcan create line [expr $x-5] $y [expr $x+5] $y -tag gcph \
            -fill red -width 2 -tag "pthoriz"
}

###############################################################################

# cleanup procedure when window closed
proc GmDnviz::cleanup { mon w } {
    variable inmap
    variable outfile
    variable pathcoords
    variable prefix
    variable route
    variable layback
    variable height
    variable frames
    variable startframe
    variable usemouse
    variable coord_entry
    variable overwrite

	set inmap ""
	set outfile ""
	set pathcoords ""
	set overwrite 0
	set prefix "NVIZ"
	set route ""
	set layback 2000
	set height 1000
	set frames 50
	set startframe 0
	set usemouse 0
	set overwrite 0

    MapCanvas::pointer $mon
    MapCanvas::request_redraw $mon 0

}

###############################################################################

# cleanup procedure when window closed
proc GmDnviz::reset_points { mon w } {
    variable pathcoords
    variable coord_entry

	set pathcoords ""
    $coord_entry delete 0 end

    MapCanvas::request_redraw $mon 0

}

###############################################################################

# Run d.nviz to create flythrough path script
proc GmDnviz::makescript { w quit } {
    variable inmap
    variable outfile
    variable pathcoords
    variable prefix
    variable route
    variable layback
    variable height
    variable frames
    variable startframe
    variable usemouse
    variable coord_entry
    variable vrender
    variable fullrender
    variable offscreen
    variable ht_elev
    variable keyframe
    variable overwrite
    global devnull

    if { $inmap == ""} {
        tk_messageBox -type ok -icon warning -parent $w \
		    -message [G_msg "You must select an input map"] \
		    -title [G_msg "No input map selected"]
        return
    }
    if { $outfile == ""} {
        tk_messageBox -type ok -icon warning -parent $w \
		    -message [G_msg "You must specify an output file"] \
		    -title [G_msg "No output file specified"]
        return
    }    
    
    set pathcoords [string trimright $pathcoords ","]
    set coords_list [split $pathcoords ","] 
    if { [llength $coords_list] < 8 } {
        tk_messageBox -type ok -icon warning -parent $w \
		    -message [G_msg "You must specify at least 4 points (x,y coordinate pairs)"] \
		    -title [G_msg "Insufficient coordinates specified"]
        return
    }    
        
    
    set cmd "d.nviz input=$inmap output=$outfile name=$prefix route=$pathcoords \
         dist=$layback ht=$height frames=$frames start=$startframe"   


    if { $vrender == 1 } { lappend cmd " -e" }
    if { $fullrender == 1 } { lappend cmd " -f" }
    if { $offscreen == 1 } { lappend cmd " -o" }
    if { $ht_elev == 1 } { lappend cmd " -c" }
    if { $keyframe == 1 } { lappend cmd " -k" }
    if { $overwrite == 1 } { lappend cmd " --o" }
	
    run_panel $cmd
    
    # delete rules file and close popup window if OK pressed
    if { $quit == 1 } {
        destroy .dnvizPopup
    }

}

