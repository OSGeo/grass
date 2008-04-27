##########################################################################
# 
# Panel to facilitate label placement for finishing images produced
# by nviz.
#
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

# Font Type: 
set Nv_(labelFontType) times
set Nv_(font) {"times" "12"}

# Font Point Size: varies
set Nv_(labelFontSize) 12
set Nv_(labelFontColor) "#000000"

# Legend section
set Nv_(catval) 1
set Nv_(catlabel) 0
set Nv_(leg_invert) 0
set Nv_(leg_userange) 0
set Nv_(leg_discat) 0
set Nv_(leg_uselist) 0
set Nv_(cat_list) [list]
set Nv_(cat_list_select) 0

# Label Sites section
set Nv_(labvalues) 1
set Nv_(lablabels) 1
set Nv_(labinbox) 1

# default cursor
set Nv_(cursor) [$Nv_(TOP) cget -cursor]	

global clr

##########################################################################

proc mklabelPanel { BASE } {
    global Nv_ 

    set panel [St_create {window name size priority} $BASE "Labels" 2 5]
    frame $BASE -relief flat -borderwidth 0
    Nv_mkPanelname $BASE "Labels Panel"
    
    # This section contains widgets for setting font type, size and color

    set rbase1 [frame $BASE.font_type]
    Button $rbase1.font -text "Font" \
		-width 8 -command "select_font $rbase1.font" -bd 1 \
		-helptext "Select font family, size, and style"
    Button $rbase1.color -text "Color" \
		-bg $Nv_(labelFontColor) -width 8 -fg "white" -bd 1 \
		-command "change_label_color $rbase1.color" \
		-helptext "Choose font color"
    pack $rbase1.font -side left 
    pack $rbase1.color -side right 
    pack $rbase1 -side top -expand yes -fill x -padx 3 -pady 4

    # This section contains widgets for specifying the label text and a button
    # which actually places the label
    
    set rbase2 [frame $BASE.text]
    
    Label $rbase2.label -text "Label text: "
    entry $rbase2.text -relief sunken -width 38 -textvariable Nv_(label_text) 
    pack  $rbase2.label $rbase2.text -side left -expand no -fill none -anchor w
	pack $rbase2 -side top -expand 1 -fill both -padx 3 -pady 4 
	
	set rbase3 [frame $BASE.buttons]

    Button $rbase3.place -text "Place label" -command "place_label" \
    	-width 8 -bd 1 -helptext "Click with mouse to place label"
    Button $rbase3.erase -text "Erase last" -command "label_delete_list label 1" \
    	-width 8 -bd 1 -helptext "Erase most recent label placed"
    Button $rbase3.erase_all -text "Erase all" -command "label_delete_list label 0" \
    	-width 8 -bd 1 -helptext "Erase all labels"
    button $rbase3.close -text "Close" -command "Nv_closePanel $BASE" \
		-anchor se -bd 1
    pack $rbase3.place -side left -expand no -fill none
	pack $rbase3.erase -side left -expand no -fill none -padx 3
	pack $rbase3.erase_all -side left -expand no -fill none
	pack $rbase3.close -side right -fill none -expand no

    pack $rbase3 -side top -expand yes -fill both -padx 3 -pady 4 -anchor w


    return $panel
}

##############################################################################

# Simple routine to change the color of fonts
proc change_label_color { me } {
	global Nv_

	# set color button background to match font color selected
    set clr [lindex [$me configure -bg] 4]
    set clr [mkColorPopup .colorpop LabelColor $clr 1]
    set Nv_(labelFontColor) $clr
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
}

##############################################################################

#Routine to delete display list
proc label_delete_list { list flag } {
    global Nv_
    global labels legend
    global Nauto_draw

	if {$flag == 1} {
		Ndelete_list label 1
	} else {
		Ndelete_list label 0
		set labels 0
	}
	if {$Nauto_draw == 1} {
		Nset_cancel 0
		Ndraw_all
	} 

}


# Routines to allow user to place a label
proc place_label { } {
    global Nv_
    global labels
    
 	$Nv_(TOP) configure -cursor plus

    # We bind the canvas area so that the user can click to place the
    # label.  After the click is processed we unbind the canvas area
	set labels 1
	bind $Nv_(TOP).canvas <Button-1> {place_label_cb %x %y }

}

###############################################################################
# use Tk dialog to select fonts
proc select_font {fbutton} {
	global Nv_
	
    set fon [SelectFont $fbutton.fontset -type dialog -sampletext 1 -title "Select font"]
	if { $fon != "" } {set Nv_(font) $fon}
}

###############################################################################

proc place_label_cb { sx sy } {
    global Nv_ 

	set sy [expr $Nv_(height) - $sy]

	#create font description	
	set weight "medium"
	set slant "r"
	set style ""
    set clr $Nv_(labelFontColor)
	
	if {[lindex $Nv_(font) 0] != ""} {set Nv_(labelFontType) [lindex $Nv_(font) 0]}
	if {[lindex $Nv_(font) 1] != ""} {set Nv_(labelFontSize) [lindex $Nv_(font) 1]}	
	if {[lsearch $Nv_(font) "bold"] != -1} {set weight "bold"}
	if {[lsearch $Nv_(font) "italic"] != -1} {set slant "i"}
	if {[lsearch $Nv_(font) "underline"] != -1} {set style "underline"}
	if {[lsearch $Nv_(font) "overstrike"] != -1} {set style "overstrike"}
	
	set font "*-$Nv_(labelFontType)-$weight-$slant-normal-$style-$Nv_(labelFontSize)-*-*-*-*-*-*-*"

	Nplace_label $Nv_(label_text) $font $Nv_(labelFontSize) $clr $sx $sy
	$Nv_(TOP) configure -cursor $Nv_(cursor)

	#remove binding
	bind $Nv_(TOP).canvas <Button-1> {}
}

