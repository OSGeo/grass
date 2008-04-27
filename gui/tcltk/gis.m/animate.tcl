#!/bin/sh
# the next line restarts using wish \
# exec $GRASS_WISH "$0" "$@"
##########################################################################
#
# animate.tcl
#
# animates one or more series of raster maps in GRASS 6 TclTk GUI
# Author: Glynn Clements and Michael Barton (Arizona State University)
#
# September 2007
#
# COPYRIGHT:	(C) 1999 - 2007 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmAnim {

    variable view1;     # maps to animate in frame 1
    variable view2;     # maps to animate in frame 2
    variable view3;     # maps to animate in frame 3
    variable view4;     # maps to animate in frame 4
    variable first;     # first time animation is run (0 1)
    variable cnt;       # number of maps to animate
    variable tmpfile;   # temp file for storing images
    variable tmpregion; # temp region file for setting PPM resolution
    variable numviews;  # number of animation views to show (1-4)
    variable numframes; # number of individual frames in an animation
    variable vframes;   # array holding number of frames in each view
    variable step;      # turn frame by frame step on (step=1) or off (step=0)
    variable speed;     # set delay between display of each animation frame
    variable stop;      # stop continuous animation play (stop=1)
    variable direction; # 1=forward, -1=backward
    variable rewind;    # if rewind=1, return to frame 1
    variable prevframe; # number of previous frame
    variable currframe; # number of currently displayed frame
    variable nframes;   # total number of frames in a view
    variable ncols;     # columns in current region; for setting window size
    variable nrows;     # rows in current region; for setting window size 
    variable scale;     # scale value for setting size of window and animation images
    variable icols;     # initial columns in window 
    variable irows;     # initial rows in window
    variable vcols;     # columns in view window 
    variable vrows;     # rows in view window
    variable pnmres;    # resolution needed in region to create pnm file of proper size
    variable vres;      # base region resolution; modify to change pnm image size
    variable oldres1;   # nsres for current region
    variable oldres2;   # ewres for current region
    variable pic_array; # array holding all rendered images 
    variable vfiles;    # array holding list of all maps for each view 
    variable label_pos
    variable border;    # border separating views in multiview window
    variable anim_can;  # TclTk canvas where animation is drawn
    variable cb_loop;   # control button for looping
    variable cb_swing;  # control button for swing
    variable minframes; # minimum number of frames among all views; animation must stop at this number

    global shownames; # if shownames = 1, print file name in window
    global loop;
    global swing;
    global anim_prog;   # increments progress bar
     
    set border    2
    set cnt       0
    set numviews  1
    set first     0
    set step      0
    set speed     100
    set stop      1
    set direction 1
    set rewind    0
    set prevframe 0
    set currframe 1
    set nframes   0
    set loop      0
    set swing     0
    set shownames 0
    set view1 ""
    set view2 ""
    set view3 ""
    set view4 ""
    set tmpregion ""

    
	# create file for temporary image output
	if {[catch {set tmpfile [exec g.tempfile pid=[pid]]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
	
}


########################################################
# Animation control button procedures

proc GmAnim::cmd_rewind {} {
    variable step 
    variable stop 
    variable rewind
    
    set step 0
    set stop 1
    set rewind 1
    GmAnim::do_run
}

proc GmAnim::cmd_rplay {} {
    variable step 
    variable stop 
    variable direction 
    variable currframe 
    variable prevframe
    global anim_prog
    
    set anim_prog 0
    set step 0
    set stop 0
    set direction -1
    set currframe [expr $prevframe + $direction]
}

proc GmAnim::cmd_stepb {} {
    variable step 
    variable direction 
    variable currframe 
    variable prevframe
    variable nframes
    variable anim_prog
    
    if {$currframe == $nframes } {set anim_prog 0}
    set step 1
    set direction -1
    set currframe [expr $prevframe + $direction]
    GmAnim::do_run
}

proc GmAnim::cmd_stop {} {
    variable stop
    set stop 1
}

proc GmAnim::cmd_stepf {} {
    variable step 
    variable direction 
    variable currframe 
    variable prevframe
    variable anim_prog    
    
    if {$currframe == 1 } {set anim_prog 0}
    set step 1
    set direction 1
    set currframe [expr $prevframe + $direction]
    GmAnim::do_run
}

proc GmAnim::cmd_play {} {
    variable step 
    variable stop 
    variable direction 
    variable currframe 
    variable prevframe
    global anim_prog
    
    set anim_prog 0
    set step 0
    set stop 0
    set direction 1
    set currframe [expr $prevframe + $direction]
}

proc GmAnim::cmd_loop {} {
    global swing 
    global loop
    variable stop 
    variable cb_swing

    $cb_swing deselect
    set swing 0
}

proc GmAnim::cmd_swing {} {
    global loop
    global swing
    variable stop 
    variable cb_loop
    
    $cb_loop deselect
    set loop 0
}

proc GmAnim::cmd_slower {} {
    variable speed
    if {$speed > 1} {
        if {$speed < 200000} {
            set speed [expr $speed * 3]
        }
    } else {
	    set speed 1
    }
}

proc GmAnim::cmd_faster {} {
    variable speed
    
    if {$speed > 1} {
	    set speed [expr $speed / 3]
    }
}

proc GmAnim::cleanup {} {
    variable numviews
    variable numframes
    variable vfiles
    variable pic_array
    variable oldres1
    variable oldres2
    variable tmpregion
    variable view1
    variable view2
    variable view3
    variable view4

    global anim_prog
    global devnull
    
    # close all windows and delete temporary image file
    catch {if { [winfo exists .animwin] } { 
        destroy .animwin 
        }}
    
    catch {if { [winfo exists .animmaps_win] } { 
        destroy .animmaps_win 
        }}
    
    catch {if { [file exists $tmpfile] } {
        file delete -force $tmpfile 
        }}
    
    set numviews 1
    set numframes 0
    
    catch {set view1 ""}
    catch {set view2 ""}
    catch {set view3 ""}
    catch {set view4 ""}
    
    if {[array exists vfiles]} {
        array unset vfiles
        }
    
    if {[array exists pic_array]} {
        array unset pic_array
        }
    
    set anim_prog 0
    catch {unset env(WIND_OVERRIDE)}
    if {[file exists $tmpregion]} {
        catch [exec g.remove region=$tmpregion --q 2> $devnull]
    }
}

proc GmAnim::make_buttons {anim_tb} {
    global bgcolor
    global iconpath
    global loop 
    global swing 
    global shownames
    variable cb_loop
    variable cb_swing
    
    set selclr #88aa88
    
    # make button images
    image create photo img_rast   -file "$iconpath/element-cell.gif"
    image create photo img_rewind -file "$iconpath/gui-rewind.gif"
    image create photo img_rplay  -file "$iconpath/gui-rplay.gif" 
    image create photo img_stepb  -file "$iconpath/gui-stepb.gif" 
    image create photo img_stop   -file "$iconpath/gui-stop.gif"  
    image create photo img_stepf  -file "$iconpath/gui-stepf.gif" 
    image create photo img_play   -file "$iconpath/gui-play.gif"  
    image create photo img_loop   -file "$iconpath/gui-loop.gif"  
    image create photo img_swing  -file "$iconpath/gui-swing.gif" 
    image create photo img_snail  -file "$iconpath/gui-snail.gif" 
    image create photo img_rabbit -file "$iconpath/gui-rabbit.gif"
    image create photo img_exit   -file "$iconpath/gui-exit.gif"

    #Create button bars

	set bbox0 [ButtonBox $anim_tb.bbox0 -spacing 0 ]
	
	# Select maps
	$bbox0 add -command GmAnim::sel_maps -image img_rast \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor  -activebackground $bgcolor\
		-helptext [G_msg "Select maps to animate"]

    pack $bbox0 -side left -anchor w

	set sep0 [Separator $anim_tb.sep0 -orient vertical ]
	pack $sep0 -side left -fill y -padx 5 -anchor w


	set bbox1 [ButtonBox $anim_tb.bbox1 -spacing 0 ]

	# Rewind
	$bbox1 add -command GmAnim::cmd_rewind -image img_rewind \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor  -activebackground $bgcolor\
		-helptext [G_msg "Rewind animation"]

	# Replay
	$bbox1 add -command GmAnim::cmd_rplay  -image img_rplay \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Replay animation"]

	# Step backwards
	$bbox1 add -command GmAnim::cmd_stepb  -image img_stepb \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Step backwards through animation"]

	# Stop
	$bbox1 add -command GmAnim::cmd_stop   -image img_stop \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Stop animation"]

	# Step forwards
	$bbox1 add -command GmAnim::cmd_stepf  -image img_stepf \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Step forwards through animation"]

	# Play
	$bbox1 add -command GmAnim::cmd_play   -image img_play \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Play animation"]
		
    pack $bbox1 -side left -anchor w

	set sep1 [Separator $anim_tb.sep1 -orient vertical ]
	pack $sep1 -side left -fill y -padx 5 -anchor w

	set bbox2 [ButtonBox $anim_tb.bbox2 -spacing 0 ]

	# Slower
	$bbox2 add -command GmAnim::cmd_slower -image img_snail \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Slower animation"]

	# Faster
	$bbox2 add -command GmAnim::cmd_faster -image img_rabbit \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Faster animation"]

    pack $bbox2 -side left -anchor w

	# Loop
	set cb_loop [checkbutton $anim_tb.loop -command "GmAnim::cmd_loop" -image img_loop \
	    -variable loop -offvalue 0 -onvalue 1 -relief flat \
		-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
		-activebackground $bgcolor -highlightbackground $bgcolor ]
	
    DynamicHelp::register $cb_loop balloon [G_msg "Continuously loop through animation"]   

	# Swing
	set cb_swing [checkbutton $anim_tb.swing -command "GmAnim::cmd_swing" -image img_swing \
	    -variable swing -offvalue 0 -onvalue 1 -relief flat \
		-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
		-activebackground $bgcolor -highlightbackground $bgcolor ]
	
    DynamicHelp::register $cb_swing balloon [G_msg "Run animation alternately forward and backward"]

    pack $cb_loop $cb_swing -side left -anchor w
    
	# Show names
	set cb_names [checkbutton $anim_tb.names -text "Names" \
	    -variable shownames -offvalue 0 -onvalue 1 -relief flat \
		-borderwidth 1 -indicatoron false -bg $bgcolor -selectcolor $selclr \
		-activebackground $bgcolor -highlightbackground $bgcolor \
		-pady 4 -padx 2]
	
    DynamicHelp::register $cb_names balloon [G_msg "Show map names in animation window"]
    
    pack $cb_names -side left -anchor w

	set sep2 [Separator $anim_tb.sep2 -orient vertical ]
	pack $sep2 -side left -fill y -padx 5 -anchor w

	set bbox3 [ButtonBox $anim_tb.bbox3 -spacing 0 ]

	# Quit
	$bbox3 add -command "destroy .animwin"   -image img_exit \
		-highlightthickness 0 -takefocus 0 -relief link -borderwidth 1	\
		-highlightbackground $bgcolor -activebackground $bgcolor \
		-helptext [G_msg "Quit animation"]

    pack $bbox3 -side left -anchor w
    
    set helpbtn [Button $anim_tb.help -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q gm_animate" \
		-background $bgcolor -borderwidth 1 \
		-helptext [G_msg "Help"]]

	pack $anim_tb -side left -anchor w -expand yes -fill x
	pack $helpbtn -side right -anchor e


}



########################################################
# select maps to animate
proc GmAnim::select_map { view } {

    variable view1
    variable view2
    variable view3
    variable view4
    variable first

    append tmpvar $view
    set mapvar [set $tmpvar]

    set m [GSelect cell multiple title [G_msg "Select maps"] parent "."]
    if { $m != "" } {
        set mlist [split $m @]
        set m0 [lindex $mlist 0]
        if {$mapvar == ""} { 
            set GmAnim::$view $m0 
        } else { 
            set m ",$m0"
            append GmAnim::$view $m 
        }
    }

    set GmAnim::first 1
}

########################################################
proc GmAnim::sel_maps {} {

    # window for selecting maps and frames to animate
    global iconpath
    global bgcolor
    variable view1
    variable view2
    variable view3
    variable view4

	# Create raster map input window
	set mapswin [toplevel .animmaps_win]
	wm title $mapswin [ G_msg "Maps for Animation" ]
	# put it in the middle of the screen
	update idletasks
	set winWidth [winfo reqwidth $mapswin]
	set winHeight [winfo reqheight $mapswin]
	set scrnWidth [winfo screenwidth $mapswin]
	set scrnHeight [winfo screenheight $mapswin]
	set x [expr ($scrnWidth - $winWidth) / 2-250]
	set y [expr ($scrnHeight  - $winHeight) / 2]
	wm geometry $mapswin +$x+$y
	wm deiconify $mapswin
	
	# keep this on top somehow
            
    # Heading
    set row [ frame $mapswin.heading ]
    Label $row.a -text [G_msg "Select maps to animate in one or more frames (1 frame required)"] \
    	-fg MediumBlue
    Button $row.b -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q gm_animate" \
		-background $bgcolor -helptext [G_msg "Help"]
		
    pack $row.a -side left
    pack $row.b -side right -anchor e
    pack $row -side top -fill both -expand yes
	
    # Frame 1
    set row [ frame $mapswin.view1 ]
    Label $row.a -text [G_msg "Maps for frame 1 (required): "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmAnim::select_map view1"
    Entry $row.c -width 35 -text $view1 \
          -textvariable GmAnim::view1
    pack $row.c $row.b $row.a -side right
    pack $row -side top -fill both -expand yes

    # Frame 2
    set row [ frame $mapswin.view2 ]
    Label $row.a -text [G_msg "Maps for frame 2 (optional): "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmAnim::select_map view2"
    Entry $row.c -width 35 -text $view2 \
          -textvariable GmAnim::view2
    pack $row.c $row.b $row.a -side right
    pack $row -side top -fill both -expand yes

    # Frame 3
    set row [ frame $mapswin.view3 ]
    Label $row.a -text [G_msg "Maps for frame 3 (optional): "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmAnim::select_map view3"
    Entry $row.c -width 35 -text $view3 \
          -textvariable GmAnim::view3
    pack $row.c $row.b $row.a -side right
    pack $row -side top -fill both -expand yes

    # Frame 4
    set row [ frame $mapswin.view4 ]
    Label $row.a -text [G_msg "Maps for frame 4 (optional): "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmAnim::select_map view4"
    Entry $row.c -width 35 -text $view4 \
          -textvariable GmAnim::view4
    pack $row.c $row.b $row.a -side right
    pack $row -side top -fill both -expand yes
    

    set row [ frame $mapswin.buttons ]
    Button $row.a -text [G_msg "OK"] -width 8 -bd 1 \
    	-command "GmAnim::create_viewlist 1" -default active
    Button $row.b -text [G_msg "Cancel"] -width 8 -bd 1 \
    	-command "destroy .animmaps_win"
    Button $row.c -text [G_msg "Apply"] -width 8 -bd 1 \
    	-command "GmAnim::create_viewlist 0"
    pack $row.a $row.b $row.c -side right -padx 5
    pack $row -side bottom -pady 3 -padx 5 -expand 0 -fill none -anchor e
    
    
}


########################################################

proc GmAnim::create_viewlist { closeval } {
    # Create a list of the entries for each view
    variable view1
    variable view2
    variable view3
    variable view4
    variable numviews
    
    set viewlist {}

    if { $view1 == "" } {
        tk_messageBox -type ok -icon warning -parent .animmaps_win \
			-message [G_msg "You must select maps to animate for frame 1"] \
			-title [G_msg "No maps selected"]
		return
	}

    # creat list of views with maps to animate
    set viewlist $view1
    
    if { $view2 != "" } {
        lappend viewlist $view2
    }
    
    if { $view3 != "" } {
        lappend viewlist $view3
    }
    
    if { $view4 != "" } {
        lappend viewlist $view4
    }
        
    if { $closeval == 1 } { destroy .animmaps_win }
    
    GmAnim::parse_viewmaps $viewlist
    
}

########################################################

proc GmAnim::parse_viewmaps {viewlist} {
    # create lists of maps for each view and calculate frames for each fiew
    variable vfiles 
    variable numviews 
    variable numframes
    variable vframes
    variable first
    variable pic_array
    variable scale
    variable vres 
    variable ncols 
    variable nrows 
    variable icols 
    variable irows 
    variable vcols 
    variable vrows 
    global anim_prog

    # Reset variables
    set numviews 0
    set numframes 0
    if {[array exists vfiles]} {array unset vfiles}
    if {[array exists vframes]} {array unset vframes}
    if {[array exists pic_array]} {array unset pic_array }
    set anim_prog 0

    incr anim_prog

    set allmaps [exec g.mlist rast]

    foreach view $viewlist {
        set maps {}
        set groups [split $view ,]

        foreach group $groups {
            # parse series of maps with suffix number in range (x-y)
            if {[string match *(* $group]} {
                set series [split $group {(-)}]
                set start [lindex $series 1]
                set end [lindex $series 2]
                
                if {$start == ""} {
                    set first 1
                } else {
                    set first $start
                }
                
                if {$end == ""} {
                    set last $first
                } else {
                    set last $end
                }
    
                for {set mapnum $first} {$mapnum < [expr $last+1]} {incr mapnum} {
                    set map [lindex $series 0]
                    if {$start != ""} {append map $mapnum}
                    lappend maps $map
                }            
            } else {
                # parse maps as glob/regexp pattern
                foreach map $allmaps {
                    if {[string match $group $map]} {lappend maps $map}
                    
                }
            }
            
        }
                
        set vfiles($numviews) $maps
        set vframes($numviews) [llength $maps]
        if {$numframes < $vframes($numviews)} {set numframes $vframes($numviews)}
        
        incr numviews
        incr anim_prog
        update
    }
        
    set first 1
    GmAnim::do_run
        
}

########################################################
proc GmAnim::do_run {} {
    # procedure that displays maps in an animation and controls the animation
    global loop 
    global swing 
    global anim_prog
    global shownames
    variable first
    variable vfiles
    variable vframes
    variable pic_array
    variable label_pos
    variable numviews 
    variable nframes
    variable step 
    variable stop 
    variable rewind 
    variable currframe 
    variable prevframe 
    variable direction 
    variable speed
    variable anim_can
    variable cnt
    
    # if the animation is just starting, then first export the maps to PPM
    # files and create img files for displaying in the canvas
    if {$first} {
        set first 0
        set step 1
        set nframes [GmAnim::load_files]
        if {$nframes == -1} {
            set stop 1
            return
        }
        set currframe [expr $direction > 0 ? 1 : $nframes]
        set prevframe $currframe
    }

    if {$rewind} {
        set rewind    0
        set currframe  1
        set direction 1
        set step      1
        set anim_prog 0
    }

    if {$stop == 1 && $step == 0} {
	    return
    }

    if {$swing} {
        if {$currframe == $nframes } {
            set anim_prog 100
            set direction -1
            set anim_prog 0
        } elseif { $currframe == 1} {
            set anim_prog 100
            set direction 1
            set anim_prog 0
        }
    } elseif {$loop} {
        if {$currframe == $nframes} {
            set anim_prog 100
            set currframe 1
            set anim_prog 0
        } 
    } elseif {$currframe == $nframes || $currframe == 1} {
        # If we've reached the beginning or the end, then stop the animation
        if {$currframe == $nframes && $direction == 1} {set anim_prog 100}
        if {$currframe == 1 && $direction == -1} {set anim_prog 100}
    	set stop 1
    }

    # This is the main loop for displaying animation images and labels
    if {$currframe <= $nframes && $currframe >= 1} {
        $anim_can delete all
        $anim_can create image 0 0 -anchor nw -image $pic_array($currframe)
    
        # draw labels
        if {$shownames > 0} {
            for {set i 0} {$i < $numviews} {incr i} {
                set x [expr $label_pos($i,0) + 5]
                set y [expr $label_pos($i,1) - 3]
                if { $currframe > $vframes($i) } {
                    set lblframe [expr $vframes($i) - 1]
                } else {
                    set lblframe [expr $currframe - 1]
                }
                set s [lindex $vfiles($i) $lblframe]
                $anim_can create text $x $y -text $s -fill black -anchor sw
            }
        }
        
        set prevframe $currframe
    }

    incr currframe $direction
    incr anim_prog

    if {$step} {
        set step 0
        set stop 1
    }
}

########################################################
proc GmAnim::load_files {} {
    # exports maps to ppm and creates array of images to display in canvas
    
    variable numframes 
    variable vframes
    variable numviews
    variable ncols 
    variable nrows 
    variable scale
    variable icols 
    variable irows 
    variable vcols 
    variable vrows 
    variable pic_array 
    variable vfiles 
    variable label_pos
    variable tmpfile 
    variable border
    variable anim_can
    variable cnt
    variable minframes
    global anim_prog

    set vrows  $nrows
    set vcols  $ncols
    set irows $nrows
    set icols $ncols
    
    # set vrows & vcols to the size for each sub-image   
    if { $numviews > 1 } {
        set vrows [expr int($vrows * $scale / 2.0)]
        set vcols [expr int($vcols * $scale / 2.0)]
    }
    
    # create an image or composite image for each frame (use max number of frames from all views)
    for {set cnt 0} {$cnt < $numframes} {incr cnt} {
        # create the main image that will fill the canvas
        
        set img [image create photo -width $ncols -height $nrows]
        
        # store the image in an array
        set pic_array([expr $cnt + 1]) $img
    
        # set region resolution to control size of pnm image if needed
        if {$scale != 1.0 || $numviews > 1} {
            GmAnim::switch_res 1
        }
            
        # create a subimage for each view shown. For single view animations, the subimage
        # is the same size as the main image. For multiview animations, the subimage is
        # 25% of the main image.
        for {set vnum 0} {$vnum < $numviews} {incr vnum} {
            if {$icols == $vcols} {
                set vxoff $border
                set vyoff [expr $irows == $vrows ? $border : $border + $vnum * ($border + $vrows)]
            } elseif {$irows == $vrows} {
                set vxoff [expr $icols == $vcols ? $border : $border + $vnum * ($border + $vcols)]
                set vyoff $border
            } else { # 4 views
                # set offset for each subimage in multi-image view
                set vxoff [expr $vnum % 2 ? $border : $vcols + 2 * $border]
                set vyoff [expr $vnum > 1 ? $vrows + 2 * $border : $border]
            }
    
            # set label positions
            if {! $cnt} {
                set label_pos($vnum,0) $vxoff
                set label_pos($vnum,1) [expr $vyoff + $vrows - 1]
            }
            
            if { $cnt < $vframes($vnum)} {
                set name [lindex $vfiles($vnum) $cnt]
            } else { 
                # if a view has fewer frames than the maximum, just use the 
                # last frame over again
                set name [lindex $vfiles($vnum) [expr $vframes($vnum) - 1]]
            }
            
            # export the map to a PPM file            
            if {[catch {exec r.out.ppm input=$name output=$tmpfile --q} error]} {
                GmLib::errmsg $error
                return -1
            }
            
            # create the subimage for each view
            set subimg [image create photo -file $tmpfile]
            file delete $tmpfile
    
            # copy the subimage to the main image, placing it with the offsets as needed
            $img copy $subimg \
                -to $vxoff $vyoff [expr $vxoff + $vcols] [expr $vyoff + $vrows]
    
            # delete the subimage
            image delete $subimg
            incr anim_prog
        }
    
        # reset region to original resolution
        if {$scale != 1.0 || $numviews > 1 } {
            GmAnim::switch_res 0
        }
         
        # clear the canvas and display the main image with 1-4 subimages 
        $anim_can delete all
        $anim_can create image 0 0 -anchor nw -image $img

        update
        incr anim_prog
    }

    return $cnt
}

########################################################
proc GmAnim::cmd_idle {} {
    # timer - iterates through animation if stop=0 at 
    # intervals set by "speed"
    variable stop
    variable speed
    variable step
    
    if { !$stop && !$step } {GmAnim::do_run}
    after $speed GmAnim::cmd_idle
}

########################################################
proc GmAnim::switch_res { switch } {
    # set region to new temp resolution for pnm scaling if switch=1
    # reset region to original resolution if switch = 0
    
    variable scale
    variable vres
    variable numviews
    variable oldres1
    variable oldres2
    variable tmpregion
    global env
    global devnull
    
    # calculate region resolution needed for pnm output
    set res_scale1 [expr 1.0/ $scale]
    set res_scale2 [expr 1.3 * $res_scale1]
    set res_scale4 [expr 2.0 * $res_scale1]
    
    # either a 1 view window or a 4 view window
    if {$numviews == 1} {
        set pnmres [expr 1.0 * $vres * $res_scale1]
    } else {
        set pnmres [expr 1.0 * $vres * $res_scale4]
    }
    
    set tmpregion "tmpanimregion"
    append tmpregion [pid]

    if { $switch == 1 } {
        # set temp region file for changing resolution  
        if {[catch {set region [exec g.region -u save=$tmpregion --o --q]} error]} {
            GmLib::errmsg $error
        }
        if {[catch {set env(WIND_OVERRIDE) $tmpregion} error]} {
            GmLib::errmsg $error
        }
    
        # change resolution
        if {[catch [exec g.region res=$pnmres] error]} {
            GmLib::errmsg $error
        }    
    } else { 
        # switch back to original region
        if {[catch {unset env(WIND_OVERRIDE)} error]} {
            GmLib::errmsg $error
        }
        if {[file exists $tmpregion]} {
            catch [exec g.remove region=$tmpregion --q 2> $devnull]
        }
    }

}

########################################################
proc GmAnim::main {} {
    # main window for displaying and controling animation
    
    variable ncols 
    variable nrows 
    variable scale
    variable vcols 
    variable vrows 
    variable vres
    variable oldres1
    variable oldres2
    variable border
    variable speed 
    variable anim_can
    global anim_prog


    # set initial canvas geometry to match region    
    if {[catch {set region [exec g.region -ugp]} error]} {
        GmLib::errmsg $error
    }

    set reglist [split $region "\n"]
    
    foreach line $reglist {
        set line [string trim $line]
        set key [lindex [split $line "="] 0]
        switch $key { 
            nsres    {set oldres1 [lindex [split $line "="] 1]}
            ewres   {set oldres2 [lindex [split $line "="] 1]}
            rows    {set vrows [lindex [split $line "="] 1]}
            cols    {set vcols [lindex [split $line "="] 1]}
        }
    
    }
        
    set vres [expr $oldres1 > $oldres2 ? 1.0 * $oldres1 : 1.0 * $oldres2]
    set nrows $vrows
    set ncols $vcols

    # short dimension
    set sdim [expr {$nrows > $ncols ? "ncols" : "nrows"}] 


    set longdim [expr $nrows > $ncols ? $nrows : $ncols]
    set scale 1.0

    set max 900
    set min 600

    if {[array get env XGANIM_SIZE] != ""} {
    	set max $env(XGANIM_SIZE)
	    set min $env(XGANIM_SIZE)
    }

    if {$longdim > $max} {   # scale down
    	set scale [expr 1.0 * $max / $longdim]
    } elseif {$longdim < $min} { # scale up
    	set scale [expr 1.0 * $min / $longdim]
    }

    # set nrows & ncols to the size of the combined - views image
    set nrows [expr int($nrows * $scale)]
    set ncols [expr int($ncols * $scale)]
    

    # add to nrows & ncols for borders
    set nrows [expr int($nrows + (1 + ($nrows/$vrows)) * $border)]
    set ncols [expr int($ncols + (1 + ($ncols/$vcols)) * $border)]

    # create file viewing frame
	toplevel .animwin
	wm title .animwin [G_msg "Animation Window"]

	set anim_fr [MainFrame .animwin.mf \
			-textvariable GmAnim::msg \
			-progressvar anim_prog -progressmax 100 -progresstype incremental]
			
	$anim_fr showstatusbar progression

	set mf_frame [$anim_fr getframe]

	# toolbar creation
	set anim_tb	[$anim_fr addtoolbar]
	#MapToolBar::create $map_tb

	# canvas creation
	set anim_can [canvas $mf_frame.canvas \
		-borderwidth 0 -closeenough 10.0 -relief groove \
		-width $ncols -height $nrows ]

	# setting geometry
	place $anim_can -in $mf_frame -x 0 -y 0 -anchor nw
	pack $anim_can -fill both -expand yes

#   indicator creation
# 	set anim_ind [$anim_fr addindicator -textvariable animstatus \
# 		-width 33 -justify left -padx 5 -bg white]

	pack $anim_fr -fill both -expand yes
	
	# create animation control buttons
	GmAnim::make_buttons $anim_tb
	
    # bindings for closing window
	bind .animwin <Destroy> "GmAnim::cleanup"

    update

    # start animation timer
    after $speed GmAnim::cmd_idle
    
}
########################################################

#GmAnim::main
