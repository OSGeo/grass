##########################################################################
# maplabels.tcl - options panel for TclTk postscript labels layer for GRASS GIS Manager
# April 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################
#
# TODO - Bugs and missing features. Maris Nartiss, 14.02.2007.
#	(with comments by CM Barton 29 4 2007
#	* Not all label file options are in use, also not all options are in labelfiles.
#	(because this is TclTk text, v.label options don't exactly map onto what is available
#	 in TclTk. AFAICT, all possible matches are implemented)
#	* Label configuration options are not separated from displayed label options. It causes
#	label options to change after every screen redraw. This way it was already working :)
#	(Unless you check the "override" box, it will default to the values in the labels file.
#	 I fixed a few that might not have been sticking also - CMB)
#	* Ability to override single option. Like: use whatever defined in labelfile, except font size
# 	(Agreed. A TODO if there is time. Needs substantial reconfiguration of options panel)
#	* Label rotation. (not available in TclTk - CMB)
#	* Somtime label enclosing box height is not calculated correctly. 
#	* Ability to display label point and configure it. (I don't understand this)
#

namespace eval GmCLabels {
    variable array opt ;# labels current options
    variable count 1
    variable array tree ;# mon    
    variable optlist
    variable array dup ;# layer
}


proc GmCLabels::create { tree parent } {
    variable opt
    variable count
    variable optlist
	variable dup
    global mon
    global iconpath

    set node "clabels:$count"

    set frm [ frame .clabelsicon$count]
    set check [checkbutton $frm.check \
		-variable GmCLabels::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo clabels_ico -file "$iconpath/gui-maplabels.gif"
    set ico [label $frm.ico -image clabels_ico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text      "PS labels $count" \
	-window    $frm \
	-drawcross auto 

    set opt($count,1,_check) 1 
    set dup($count) 0

    set opt($count,1,xcoord) 1.0 
    set opt($count,1,ycoord) 1.0 
    set opt($count,1,xoffset) 0.0
    set opt($count,1,yoffset) 15.0 
    set opt($count,1,labels) "" 
    set opt($count,1,lfont) [font create "labelfont$count" -family "Helvetica" -size 11 -weight "normal"]
    set opt($count,1,lfontfamily) "Helvetica"
    set opt($count,1,lfontsize) 11
    set opt($count,1,lfontweight) "normal"
    set opt($count,1,lfontslant) "roman"
    set opt($count,1,lfontunderline) 0
    set opt($count,1,lfontoverstrike) 0
    set opt($count,1,lfill) \#000000 
    set opt($count,1,lwidth)  100
    set opt($count,1,lanchor) "center_left" 
    set opt($count,1,ljust) "left" 
    set opt($count,1,ltxt) ""
    set opt($count,1,lhoffset) 2 ;# space between label and enclosing box
    set opt($count,1,lvoffset) 2 ;# space between label and enclosing box
    set opt($count,1,lopaque) "yes"
    set opt($count,1,lboxbenable) "yes"
    set opt($count,1,lborder) "black"
    set opt($count,1,lbackground) "yellow"
    set opt($count,1,lbwidth) 1
    set opt($count,1,lboxlenable) 1
    set opt($count,1,override) 0

	set optlist { _check xcoord ycoord xoffset yoffset labels lfont lfontfamily lfontsize \
		lfontweight lfontslant lfontunderline lfontoverstrike lfill lwidth \
		lanchor ljust ltxt lhoffset lvoffset lopaque lboxbenable lborder lbackground \
		lbwidth lboxlenable override }

    foreach key $optlist {
		set opt($count,0,$key) $opt($count,1,$key)
    } 
        
    incr count
    return $node
}

proc GmCLabels::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

proc GmCLabels::select_labels { id } {
    set m [GSelect paint/labels title "Label file"]
    if { $m != "" } { 
        set GmCLabels::opt($id,1,labels) $m
        GmTree::autonamel $m
    }
}

proc GmCLabels::select_font { id frm } {
	global mon
	variable opt
	set fon ""
    
	set fon [SelectFont $frm.lfon -font $opt($id,1,lfont) -type dialog \
		-sampletext [G_msg "This is font sample text."] -title [G_msg "Select label font"]]

	if { $fon != "" } {
		set opt($id,1,lfontfamily)	[lindex $fon 0]
		set opt($id,1,lfontsize)	[lindex $fon 1]
		if {[lsearch $fon "bold"]>=0} 	{ set opt($id,1,lfontweight) "bold" }
		if {[lsearch $fon "italic"]>=0} { set opt($id,1,lfontslant) "italic"}
		if {[lsearch $fon "underline"]>=0}  { set opt($id,1,lfontunderline) 1}
		if {[lsearch $fon "overstrike"]>=0} { set opt($id,1,lfontoverstrike) 1}
		
		font configure $opt($id,1,lfont) -family $opt($id,1,lfontfamily) -size $opt($id,1,lfontsize)\
		-weight $opt($id,1,lfontweight) -slant $opt($id,1,lfontslant) \
		-underline $opt($id,1,lfontunderline) -overstrike $opt($id,1,lfontoverstrike)
	}
}

# display labels options
proc GmCLabels::options { id frm } {
    variable opt
    global iconpath

    # Panel heading1
    set row [ frame $frm.heading1 ]
    Label $row.a -text [G_msg "Create postscript labels for vector objects from v.labels file"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # Panel heading2
    set row [ frame $frm.heading2 ]
    Label $row.a -text [G_msg "  (for postscript eps, pdf, and print output only)"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # labels name
    set row [ frame $frm.name ]
    Label $row.a -text [G_msg "Labels file:"]
    Button $row.b -image [image create photo -file "$iconpath/gui-maplabels.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "labels file to display"] \
		-command "GmCLabels::select_labels $id"
    Entry $row.c -width 40 -text "$opt($id,1,labels)" \
		-textvariable GmCLabels::opt($id,1,labels)
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # override
    set row [ frame $frm.disp ]
    Label $row.a -text [G_msg "Override offset, alignment, and color settings in v.labels file: "]
    checkbutton $row.b -variable GmCLabels::opt($id,1,override)
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # labels options1
    set row [ frame $frm.lblopt1 ]
    Label $row.a -text [G_msg "Align label with vector object: "] 
    ComboBox $row.b -padx 2 -width 12 -textvariable GmCLabels::opt($id,1,lanchor) \
                    -values {"lower left" "lower center" "lower right" "center left" "center" 
                    "center right" "upper left" "upper center" "upper right" }
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

	# label offset
    set row [ frame $frm.offset ]
    Label $row.a -text [G_msg "Offset label from vector object: "] 
    LabelEntry $row.b -label [G_msg "x offset"] \
    	-textvariable GmCLabels::opt($id,1,xoffset) \
		-width 10
    LabelEntry $row.c -label [G_msg "y offset"] \
    	-textvariable GmCLabels::opt($id,1,yoffset) \
		-width 10
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # labels options2
    set row [ frame $frm.lbltopt2 ]
    Label $row.a -text [G_msg "Justification: "] 
    ComboBox $row.b -padx 2 -width 7 -textvariable GmCLabels::opt($id,1,ljust) \
                    -values {"left" "center" "right"}
    Label $row.c -text [G_msg " Label line max length: "]
    LabelEntry $row.d -textvariable GmCLabels::opt($id,1,lwidth) -width 5 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
# labels options3
    set row [ frame $frm.lbltopt3 ]
    Label $row.a -text [G_msg "Enclose label in box: "] 
    checkbutton $row.b -variable GmCLabels::opt($id,1,lboxenable)
    	$row.b select
    Label $row.c -text [G_msg "Draw label background: "] 
    checkbutton $row.d -variable GmCLabels::opt($id,1,lopaque) \
   	-command " if { $opt($id,1,lopaque) } { set opt($id,1,lboxenable) 1 } else { set opt($id,1,lboxenable) 0 }"
    	$row.d select
    Label $row.e -text [G_msg "Background color:"] 
    SelectColor $row.f -type menubutton -variable GmCLabels::opt($id,1,lbackground)
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left
    pack $row -side top -fill both -expand yes
        
# labels options4
    set row [ frame $frm.lbltopt4 ]
    Label $row.a -text [G_msg "Draw box outline: "] 
    checkbutton $row.b -variable GmCLabels::opt($id,1,lboxbenable) \
    -command " if { $opt($id,1,lboxbenable) } { set opt($id,1,lboxenable) 1 } else { set opt($id,1,lboxenable) 0 }"
    	$row.b select
    Label $row.c -text [G_msg "Border width:"]
    Entry $row.d -width 3 -text "$opt($id,1,lbwidth)" \
	    -textvariable GmCLabels::opt($id,1,lbwidth)
    Label $row.e -text [G_msg "Border color:"] 
    SelectColor $row.f -type menubutton -variable GmCLabels::opt($id,1,lborder)
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left
    pack $row -side top -fill both -expand yes
        
# labels options5
    set row [ frame $frm.lbltopt5 ]
    Label $row.a -text [G_msg "Distance between label and enclosing box. Horizontal: "] 
    Entry $row.b -width 3 -text "$opt($id,1,lhoffset)" \
	    -textvariable GmCLabels::opt($id,1,lhoffset)  
    Label $row.c -text [G_msg " Vertical: "]
    Entry $row.d -width 3 -text "$opt($id,1,lvoffset)" \
	    -textvariable GmCLabels::opt($id,1,lvoffset)  
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
        
    # select font
    set row [ frame $frm.font ]
    Label $row.a -text [G_msg "Font"] 
    Button $row.b -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "select font for label"] \
	    -command "GmCLabels::select_font $id $frm"
    Label $row.c -text [G_msg "family:"]
    Entry $row.d -width 15 -text GmCLabels::opt($id,1,lfontfamily) \
	    -textvariable GmCLabels::opt($id,1,lfontfamily)
    Label $row.e -text [G_msg "size:"]
    Entry $row.f -width 5 -text GmCLabels::opt($id,1,lfontsize) \
	    -textvariable GmCLabels::opt($id,1,lfontsize)
    Label $row.g -text [G_msg "  color"] 
    SelectColor $row.h -type menubutton -variable GmCLabels::opt($id,1,lfill)
    pack $row.a $row.b $row.c $row.d  $row.e $row.f $row.g $row.h -side left
    pack $row -side top -fill both -expand yes

    # launch v.label
    set row [ frame $frm.vlabel ]
    Label $row.a -text [G_msg "Launch v.label to create labels file"]
    Button $row.b -text [G_msg "v.label"] \
	    -command "execute v.label"
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

}

proc GmCLabels::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]


    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}

proc GmCLabels::display { node } {
    global mon
    global env
    variable optlist
    variable opt
    variable tree
    variable dup
    variable count
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]
    
    if {$opt($id,1,labels) == "" } {return}
    
    set can($mon) $MapCanvas::can($mon)
    if {[string match {*[@]*} $opt($id,1,labels)]} { 
		set labelfname [string range $opt($id,1,labels) 0 [expr [string first "@" $opt($id,1,labels)] - 1] ]
		set labelmname [string range $opt($id,1,labels) [expr [string first "@" $opt($id,1,labels)] + 1] [string length $opt($id,1,labels)] ]
		set labelpath "$env(GISDBASE)/$env(LOCATION_NAME)/$labelmname/paint/labels/$labelfname"
    } else {
		set labelpath "$env(GISDBASE)/$env(LOCATION_NAME)/$env(MAPSET)/paint/labels/$opt($id,1,labels)"
    }

    if { ! ( $opt($id,1,_check) ) } { return } 
    
    # open the v.label file for reading
	if { [catch {set labelfile [open $labelpath r]} err ] } {
		GmLib::errmsg $err [G_msg "Could not open labels file "]
		return
	}
	
	#loop through coordinates and options for each label
    while { [gets $labelfile in] > -1 } {
		set key ""
		set val ""
        set in [string trim $in " "] 
		if { $in == "" } { continue }
		set key [string trim [lindex [split $in ":"] 0]]
		set val [string trim [lindex [split $in ":"] 1]]

		# Label options	
		switch $key {
			"east" {
				set east $val
				set opt($id,1,xcoord) [MapCanvas::mape2scrx $mon $east]
			}
			"north" {
				set north $val
				set opt($id,1,ycoord) [MapCanvas::mapn2scry $mon $north]
			}
			"xoffset" {
				if { $opt($id,1,override) == 0 } {
					set opt($id,1,xoffset) $val
				}
				if { $opt($id,1,xoffset) != "" } {
					set opt($id,1,xcoord) [expr $opt($id,1,xcoord) + $opt($id,1,xoffset)]
				}
			}
			"yoffset" {
				if { $opt($id,1,override) == 0 } {
					set opt($id,1,yoffset) $val
				}
				if { $opt($id,1,yoffset) != "" } {
					set opt($id,1,ycoord) [expr $opt($id,1,ycoord) + $opt($id,1,yoffset)]
				}
			}
			"ref" {
				if { $opt($id,1,override) == 0 } {
					set opt($id,1,lanchor) $val
				}
				switch $opt($id,1,lanchor) {
					"lower left" 	{ set opt($id,1,anchor) "ne"}
					"left lower" 	{ set opt($id,1,anchor) "ne"}
					"lower" 		{ set opt($id,1,anchor) "n" }
					"lower center" 	{ set opt($id,1,anchor) "n" }
					"center lower" 	{ set opt($id,1,anchor) "n" }
					"lower right" 	{ set opt($id,1,anchor) "nw"}
					"right lower" 	{ set opt($id,1,anchor) "nw"}
					"left" 			{ set opt($id,1,anchor) "e" }
					"center left" 	{ set opt($id,1,anchor) "e" }
					"left center" 	{ set opt($id,1,anchor) "e" }
					"center" 		{ set opt($id,1,anchor) "center" }
					"right" 		{ set opt($id,1,anchor) "w" }
					"center right" 	{ set opt($id,1,anchor) "w" }
					"right center" 	{ set opt($id,1,anchor) "w" }
					"upper left" 	{ set opt($id,1,anchor) "se"}
					"left upper" 	{ set opt($id,1,anchor) "se"}
					"upper" 		{ set opt($id,1,anchor) "s" }
					"upper center" 	{ set opt($id,1,anchor) "s" }
					"center upper" 	{ set opt($id,1,anchor) "s" }
					"upper right" 	{ set opt($id,1,anchor) "sw"}
					"right upper" 	{ set opt($id,1,anchor) "sw"}
					default 		{ set opt($id,1,anchor) "w" }
				}
			}
			"font" {
				# set independently in TclTk
				set x ""
			}
			"color" {
				if { $opt($id,1,override) == 0 } {
					set opt($id,1,lfill)  [color_grass_to_tcltk $val]
				}
				if { $opt($id,1,lfill) == "" } {
					set opt($id,1,lfill) "#000000"
				}
			}
			"fontsize" {
				if { $opt($id,1,override) == 0 } {set opt($id,1,lfontsize) $val}
			}
			"width" {
				if { $opt($id,1,override) == 0 } {set opt($id,1,lbwidth) $val}
			}
			"hcolor" {
				# not available in TclTk
				set x ""
			}
			"hwidth" {
				# not available in TclTk
				set x ""
			}
			"background" {
				if { $opt($id,1,override) == 0 } {
				    if {$val != "none"} {
    					set opt($id,1,lbackground) [color_grass_to_tcltk $val]
    				} else {
    				    set opt($id,1,lbackground) ""
    				}
				}
			}
			"border" {
				if { $opt($id,1,override) == 0 } {
				    if {$val != "none"} {
    					set opt($id,1,lborder) [color_grass_to_tcltk $val]
    				} else {
    				    set opt($id,1,lborder) ""
    				}
				}
			}
			"opaque" {
				if { $opt($id,1,override) == 0 } {set opt($id,1,lopaque) $val}
			}
			"rotate" {
				# not available in TclTk
				set x ""
			}
			"size"   {
				if { $opt($id,1,override) == 0 } {set opt($id,1,lwidth) $val}
			}
			"text" {
				set opt($id,1,ltxt) [subst -nocommands -novariables $val]
				
				# check to see if there are line breaks in the text
				set newlines [expr [llength [split $opt($id,1,ltxt) "\n"]] - 1]
				
				# create each label when loop gets to a text line in the labels file
				# Here should be set all font related options, that come from labelfile
				if {[info exists opt($id,1,lfontsize)]} {
					font configure $opt($id,1,lfont) -size $opt($id,1,lfontsize)
				}
				
				# set the label width and height
				set linelen [font measure $opt($id,1,lfont) $opt($id,1,ltxt)]
				if {$linelen < $opt($id,1,lwidth)} {
					set wid [expr $linelen + 8]
					set lineh [expr $newlines * [font metrics $opt($id,1,lfont) -linespace] + 8]
				} else {
					set wid $opt($id,1,lwidth)
					set lineh [expr $newlines * (ceil($linelen/$opt($id,1,lwidth))+1)\
					 * [font metrics $opt($id,1,lfont) -linespace] + 8]
				}
				if {$opt($id,1,lopaque) != "yes"} {
					set lbackground ""
				} else { set lbackground $opt($id,1,lbackground) }
				if {!$opt($id,1,lboxbenable)} {
					set wdth 0
				} else { set wdth $opt($id,1,lbwidth) }

				# create box around text
				
				switch $opt($id,1,anchor) {
					"ne" { 
						set boxcenter_x [expr {$opt($id,1,xcoord) - ($wid-4) / 2}] 
						set boxcenter_y [expr {$opt($id,1,ycoord) + ($lineh+4) / 2}]
						}
					"n" { 
						set boxcenter_x $opt($id,1,xcoord)
						set boxcenter_y [expr {$opt($id,1,ycoord) - ($lineh+4) / 2}]
						}
					"nw" { 
						set boxcenter_x [expr {$opt($id,1,xcoord) + ($wid-4) / 2}]
						set boxcenter_y [expr {$opt($id,1,ycoord) + ($lineh+4) / 2}]
						}
					"e" { 
						set boxcenter_x [expr {$opt($id,1,xcoord) - ($wid-4) / 2}]
						set boxcenter_y $opt($id,1,ycoord)
						}
					"center" { 
						set boxcenter_x $opt($id,1,xcoord)
						set boxcenter_y $opt($id,1,ycoord)
						}
					"w" { 
						set boxcenter_x [expr {$opt($id,1,xcoord) + ($wid-4) / 2}]
						set boxcenter_y $opt($id,1,ycoord)
						}
					"se" { 
						set boxcenter_x [expr {$opt($id,1,xcoord) - ($wid-4) / 2}]
						set boxcenter_y [expr {$opt($id,1,ycoord) - ($lineh+4) / 2}]
						}
					"s" { 
						set boxcenter_x $opt($id,1,xcoord)
						set boxcenter_y [expr {$opt($id,1,ycoord) - ($lineh+4) / 2}]
						}
					"sw" { 
						set boxcenter_x [expr {$opt($id,1,xcoord) + ($wid-4) / 2}]
						set boxcenter_y [expr {$opt($id,1,ycoord) - ($lineh+4) / 2}]
						}
					default { 
						set boxcenter_x $opt($id,1,xcoord)
						set boxcenter_y $opt($id,1,ycoord)
						}
				}

				if {$opt($id,1,lboxenable)} { 
					# draw recangle around label
					$can($mon) create rectangle \
						[expr {$boxcenter_x - $opt($id,1,lhoffset) - $wid / 2}] \
						[expr {$boxcenter_y -2- $opt($id,1,lvoffset) - $lineh / 2}]\
						[expr {$boxcenter_x + $opt($id,1,lhoffset) + $wid / 2}] \
						[expr {$boxcenter_y + $opt($id,1,lvoffset) + $lineh / 2}]\
						-width  $wdth \
						-outline $opt($id,1,lborder) \
						-fill $lbackground
				}
				$can($mon) create text $opt($id,1,xcoord) $opt($id,1,ycoord) \
					-anchor $opt($id,1,anchor) \
					-justify $opt($id,1,ljust) \
					-width $wid \
					-fill $opt($id,1,lfill) \
					-font $opt($id,1,lfont) \
					-text $opt($id,1,ltxt)
			} 
			default {
				#for anything else, just move on
				set x ""
			}
		}
	}
	# close labels file
	if {[catch {close $labelfile} error]} {
		GmLib::errmsg $error
	}
}


proc GmCLabels::query { node } {
    puts "Query not supported for Paint labels layer"
}

proc GmCLabels::duplicate { tree parent node id } {
    variable optlist
    variable opt
    variable count
	variable dup
	global iconpath

    set node "PS labels:$count"
	set dup($count) 1

    set frm [ frame .clabelsicon$count]
    set check [checkbutton $frm.check \
		-variable GmCLabels::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo clabels_ico -file "$iconpath/gui-maplabels.gif"
    set ico [label $frm.ico -image clabels_ico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
	
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

	if { $opt($id,1,labels) == ""} {
	    $tree insert $sellayer $parent $node \
		-text      "clabels $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert $sellayer $parent $node \
		-text      "$opt($id,1,clabels)" \
		-window    $frm \
		-drawcross auto
	}

    foreach key $optlist {
    	set opt($count,1,$key) $opt($id,1,$key)
		set opt($count,0,$key) $opt($count,1,$key)
    } 
	
	set id $count
	
    incr count
    return $node
}
