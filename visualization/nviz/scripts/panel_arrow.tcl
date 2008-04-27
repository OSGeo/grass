##########################################################################
# 
# Panel to facilitate north arrow placement for finishing images produced
# by nviz.
#
##########################################################################
# 4/4/95
# M. Astley
# U.S. Army Construction Engineering Research Laboratory
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

# Changes
#

# Panel specific globals
global Nv_

# default cursor
set Nv_(cursor) [$Nv_(TOP) cget -cursor]	
set Nv_(arw_font) {"helvetica" "17"}

##########################################################################

proc mkarrowPanel { BASE } {
    global Nv_
    global n_arrow_size n_arrow arw_text_size
    global arw_clr arw_text_clr
    global nviztxtfont

    # defaults (for some reason not recognizing globals)
	set n_arrow_size [expr int([Nget_longdim]/8.)]
	set arw_clr "#000000"
    set arw_text_clr "#DDDDDD"
    #This doesn't do anything currently
    set arw_text_size "not funct." 

    set panel [St_create {window name size priority} $BASE "North arrow" 2 5]
    frame $BASE -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "North Arrow Panel"

    # This section contains widgets for placing the north arrow
    set rbase1 [frame $BASE.arrow]
    Label $rbase1.arrow_lbl -text "Arrow: " -fg black
    LabelEntry $rbase1.arrow_size -relief sunken -entrybg white \
        -textvariable n_arrow_size -width 8 -justify right\
        -label "size (in map units) " -fg black -labelfont $nviztxtfont
    pack $rbase1.arrow_lbl $rbase1.arrow_size -side left -expand no -fill none
    
    $rbase1.arrow_size bind <Key> {if {$Nauto_draw == 1} {Ndraw_all}} 

    Button $rbase1.color -text "Color" \
		-bg $arw_clr -width 8 -bd 1 \
		-command "change_arrow_color $rbase1.color arrow" \
		-fg "#ffffff"
    pack $rbase1.color -side right \
    	-expand yes -fill none -anchor e

	pack $rbase1 -side top -expand yes -fill both -padx 3 -pady 4

    # This section contains widgets for north text
    set rbase2 [frame $BASE.txt]
    Label $rbase2.txt_lbl -text "North text: " -fg black
    Button $rbase2.font -text "Font" \
		-width 8 -command "select_arw_font $rbase2.font" -bd 1 \
		-helptext "Select font family, size, and style"
    pack $rbase2.txt_lbl $rbase2.font -side left -expand no -fill none
    
    Button $rbase2.color -text "Color" \
		-bg $arw_text_clr -width 8 -bd 1 \
		-command "change_arrow_color $rbase2.color text" \
		-fg "#ffffff"
    pack $rbase2.color -side right \
    	-expand yes -fill none -anchor e

	pack $rbase2 -side top -expand yes -fill both -padx 3

    # close panel section
    set rbase3 [frame $BASE.button]
    Button $rbase3.place -text "Place arrow" -bd 1 \
	 -command "bind_mouse $Nv_(TOP).canvas; $Nv_(TOP) configure -cursor plus"
    pack $rbase3.place -expand yes -side left -expand no -fill none

    button $rbase3.close -text "Close" -command "Nv_closePanel $BASE" \
		-anchor se -bd 1
	pack $rbase3.close -side right -fill none -expand no
	pack $rbase3 -side top -fill both -expand yes -padx 3 -pady 4

    return $panel
}

proc bind_mouse { W } {
	global Nv_
	bind $W <1> {
		place_narrow %W %x %y 
		if {$Nauto_draw == 1} {
			#Nset_cancel 0
			Ndraw_all
		} 
		$Nv_(TOP) configure -cursor $Nv_(cursor)
	}
}


###############################################################################
# use Tk dialog to select fonts
proc select_arw_font {fbutton} {
	global Nv_
	
    set fon [SelectFont $fbutton.fontset -type dialog -sampletext 1 -title "Select font"]
	if { $fon != "" } {set Nv_(arw_font) $fon}
}

#############################################################

# Simple routine to change the color of Arrow.
# text color not yet user settable.
proc change_arrow_color { me type } {
	global Nv_
	global arw_clr arw_text_clr
	global Nauto_draw
	
	# set color button background to match arrow color
    set clr [lindex [$me configure -bg] 4]
    set clr [mkColorPopup .colorpop arw_clr $clr 1]
    if {$type == "arrow"} {
	    set arw_clr $clr
	} elseif {$type == "text"} {
		set arw_text_clr $clr
	}
    $me configure -bg $clr

	# set color button text to black or white depending on
	# darkness of color
    set clrnum [split $clr {}]
    set rhex "0x[lindex $clrnum 1][lindex $clrnum 2]"
    set ghex "0x[lindex $clrnum 3][lindex $clrnum 4]"
    set bhex "0x[lindex $clrnum 5][lindex $clrnum 6]"
    set clrsum [expr $rhex + $ghex +$bhex]
   
    if {$clrsum < 400 } {
	   $me configure -fg "white"
	 } else {
	   $me configure -fg "black"
	 }
	if {$Nauto_draw == 1} {
		Ndraw_all
	} 
	 
}
###########################
proc place_narrow {W x y} {

	global Nv_ n_arrow n_arrow_size
	global n_arrow_x n_arrow_y n_arrow_z
	global arw_clr arw_text_clr
	global Nauto_draw
	
	set y [expr $Nv_(height) - $y]

	#create font description	
	set weight "medium"
	set slant "r"
	set style ""
    set clr $arw_text_clr
	
	if {[lindex $Nv_(arw_font) 0] != ""} {set Nv_(arw_FontType) [lindex $Nv_(arw_font) 0]}
	if {[lindex $Nv_(arw_font) 1] != ""} {set Nv_(arw_FontSize) [lindex $Nv_(arw_font) 1]}	
	if {[lsearch $Nv_(arw_font) "bold"] != -1} {set weight "bold"}
	if {[lsearch $Nv_(arw_font) "italic"] != -1} {set slant "i"}
	if {[lsearch $Nv_(arw_font) "underline"] != -1} {set style "underline"}
	if {[lsearch $Nv_(arw_font) "overstrike"] != -1} {set style "overstrike"}
	
	set font "*-$Nv_(arw_FontType)-$weight-$slant-normal-$style-$Nv_(arw_FontSize)-*-*-*-*-*-*-*"

#	Nplace_label $Nv_(label_text) $font $Nv_(arw_FontSize) $arw_text_clr $sx $sy

#Draw North Arrow at selected point
    set curr [Nget_current surf]
    if {$curr} {
        set location [Nset_Narrow $x $y $curr $n_arrow_size]
        set n_arrow_x [lindex $location 0]
        set n_arrow_y [lindex $location 1]
        set n_arrow_z [lindex $location 2]

        Ndraw_Narrow $n_arrow_x $n_arrow_y $n_arrow_z $n_arrow_size \
		$arw_clr $arw_text_clr
        #set chuckbutton
        set n_arrow 1
    }

#remove canvas binding
    bind $W <1> {}

}
