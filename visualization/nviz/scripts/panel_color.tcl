##########################################################################
#
# Routines for background color in NVIZ
# 
# Original author unknown.
# Probably U.S. Army Construction Engineering Research Laboratory
#
#
# Major update of GUI Nov 2006, Michael Barton, Arizona State University
#
##########################################################################
# COPYRIGHT:	(C) 2006 by Michael Barton and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################
# Default Priority for this panel
#
# priority is from 0 to 10
#  the lower the number, the quicker it will be bumped
#  10 cannot be bumped
#  Panels will be loaded by the greater of 5 or their current priority

#########################################################################
#create Color panel

global BGColor

proc mkcolorPanel { BASE } {
    global Nv_

    catch {destroy $BASE}

    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
        set panel [St_create {window name size priority} $BASE "Background Color" 1 5]
    } else {
	set panel $Nv_($BASE)
    }

    frame $BASE  -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Background Color Panel"
    
    button $BASE.background -text Background -bg white -fg grey \
		-activebackground gray20 -activebackground white\
		-command "set_background_color $BASE.background"\
		-height 3 -width 12
    # place $BASE.background -rely .33 -relx .33 -relheight .20 -relwidth .40
    pack $BASE.background -padx 5 -pady 5
    frame $BASE.closef
    button $BASE.closef.close -text "Close" -command "Nv_closePanel $BASE" \
		-anchor se -bd 1
    pack $BASE.closef.close -side right -padx 3
    pack $BASE.closef -side bottom -fill x

    return $panel
}

proc set_background_color {W} {
    global BGColor
    global Nauto_draw

    set BGColor [mkColorPopup .colorPop Background $BGColor]
    $W config -bg $BGColor
    $W config -activebackground $BGColor
    Nbackground $BGColor
    Nquick_draw
	if {$Nauto_draw == 1} {
		Ndraw_all
	} 
}

# Reset procedure for color panel
proc Nviz_color_reset {} {
    Nbackground "#FFFFFF"
    Nquick_draw
}

# Load procedure for loading state of Nviz
proc Nviz_color_load {file_hook} {
    global BGColor
    
    # Nothing fancy here, just load the background color
    gets $file_hook BGColor
    Nbackground $BGColor
    Nquick_draw

}

# Save procedure for saving state of Nviz
proc Nviz_color_save {file_hook} {
    global BGColor

    # Nothing fancy here, just save the background color
    puts $file_hook ">>>start color"
    puts $file_hook "$BGColor"
}
