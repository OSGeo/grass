##########################################################################
# histogram.tcl - raster histogram display layer options file for GRASS GIS Manager
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmHist {
    variable array opt # hist current options
    variable count 1
    variable array tree # mon
    variable array lfile # histogram
    variable array lfilemask # histogram
    variable optlist
    variable first
    variable array dup # layer
}

proc GmHist::create { tree parent } {
    variable opt
    variable count
    variable lfile
    variable lfilemask
    variable optlist
    variable first
	variable dup
    global mon
    global iconpath

    set node "histogram:$count"

    set frm [ frame .histicon$count]
    set check [checkbutton $frm.check \
		-variable GmHist::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo hico -file "$iconpath/module-d.histogram.gif"
    set ico [label $frm.ico -image hico -bd 1 -relief raised]
    
    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left
        
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "histogram $count"\
	-window    $frm \
	-drawcross auto  
    
    set opt($count,1,_check) 1 
    set dup($count) 0
    
    set opt($count,1,map) "" 
	set opt($count,1,opacity) 1.0
    set opt($count,1,color) #000000
    set opt($count,1,bgcolor) #ffffff
    set opt($count,1,bgcolor_none) 0
    set opt($count,1,style) "bar" 
    set opt($count,1,font) "" 
    set opt($count,1,nsteps) 255 
    set opt($count,1,nulls) 0 
    
    set opt($count,1,mod) 1
    set first 1

	set optlist {_check map opacity color bgcolor style font nsteps nulls}

    foreach key $optlist {
		set opt($count,0,$key) $opt($count,1,$key)
    } 
    
	# create files in tmp diretory for layer output
	set mappid [pid]
	if {[catch {set lfile($count) [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
	set lfilemask($count) $lfile($count)
	append lfile($count) ".ppm"
	append lfilemask($count) ".pgm"

    incr count
    return $node
}

proc GmHist::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

##########################################################################
proc GmHist::select_map { id } {
    variable tree
    variable node
    global mon
    
    set m [GSelect cell title [G_msg "Raster map for histogram"] parent "."]
    if { $m != "" } { 
        set GmHist::opt($id,1,map) $m
        GmTree::autonamel [format [G_msg "histogram of %s"] $m]
    }
}

##########################################################################
proc GmHist::set_font { id } {
	variable opt
	
	if {$GmHist::opt($id,1,font) != "" } {
		set Gm::dfont $GmHist::opt($id,1,font)
	}
	Gm::defaultfont dhist
	tkwait variable Gm::dfont
	set GmHist::opt($id,1,font) $Gm::dfont
	set Gm::dfont ""


}

##########################################################################
# display histogram options
proc GmHist::options { id frm } {
    variable opt
    global bgcolor
    global iconpath

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text [G_msg "Draw histogram of values from raster map or image"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmHist::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # raster name for histogram
    set row [ frame $frm.name ]
    Label $row.a -text [G_msg "Raster to histogram: "]
    Button $row.b -image [image create photo -file "$iconpath/element-cell.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmHist::select_map $id"
    Entry $row.c -width 35 -text " $opt($id,1,map)" \
          -textvariable GmHist::opt($id,1,map)
    Label $row.d -text "   "
    Button $row.e -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q d.histogram" \
		-background $bgcolor -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # graph style
    set row [ frame $frm.style ]
    Label $row.a -text [G_msg "Graph style"]
    ComboBox $row.b -padx 2 -width 4 -textvariable GmHist::opt($id,1,style) \
		-values {"bar" "pie"} 
    Label $row.c -text [G_msg "\ttext font "]
    Button $row.d -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "select font for text"] \
	    -command "GmHist::set_font $id"
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
    # color
    set row [ frame $frm.color ]
    Label $row.a -text [G_msg "Histogram color: text & frame"]
    SelectColor $row.b -type menubutton -variable GmHist::opt($id,1,color) 
    Label $row.c -text [G_msg "  background"] 
    SelectColor $row.d -type menubutton -variable GmHist::opt($id,1,bgcolor)
    Label $row.e -text "  " 
    checkbutton $row.f -text [G_msg "transparent background"] \
    	-variable GmHist::opt($id,1,bgcolor_none) 
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left
    pack $row -side top -fill both -expand yes
    
    # steps for fp maps and nulls
    set row [ frame $frm.steps ]
    Label $row.a -text [G_msg "Steps/bins for values (fp maps only)"]
    SpinBox $row.b -range {2 255 1} -textvariable GmHist::opt($id,1,nsteps) \
		-width 4 -helptext "steps/bins"  
    Label $row.c -text "   "
    checkbutton $row.d -text [G_msg "include null values"] \
        -variable GmHist::opt($id,1,nulls) 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
}

##########################################################################
proc GmHist::save { tree depth node } {
    variable opt
    variable optlist
    global mon
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
         GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}

##########################################################################
proc GmHist::display { node mod } {
    global mon
    global env
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count
    variable first

    set rasttype ""
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,map) == "" } { return } 
    
	set color [GmLib::color $opt($id,1,color)]
    set bgcolor [GmLib::color $opt($id,1,bgcolor)]

    # transparent background color
    if { $opt($id,1,bgcolor_none) == 1 } { 
        set bgcolor "none"
    }

    set cmd "d.histogram --q map=$opt($id,1,map) style=$opt($id,1,style) \
    	color=$color bgcolor=$bgcolor"

    # include nulls
    if { $opt($id,1,nulls) } { 
        append cmd " -n"
    }

    # set steps
    if { $opt($id,1,nsteps) != "" } { 
		catch {set rt [open "|r.info map=$opt($id,1,map) -t" r]}
		set rasttype [read $rt]
		if {[catch {close $rt} error]} {
			GmLib::errmsg $error [G_msg "r.info error"]
		}
		if {[regexp -nocase ".=FCELL" $rasttype] || [regexp -nocase ".=DCELL" $rasttype]} {
            append cmd " nsteps=$opt($id,1,nsteps)"
        }
	}

    # check value of GRASS_FONT variable prior to display
	if {![catch {set env(GRASS_FONT)}]} {
		set currfont $env(GRASS_FONT)
	} else {
		set currfont "romans"
	}

    # set grass font environmental variable to user selection"
	if { $GmHist::opt($id,1,font) != ""} { set env(GRASS_FONT) $GmHist::opt($id,1,font) }

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
	
	# set grass font environmental variable to whatever it was when we started
	# this lets different text layers have different fonts
	
	set env(GRASS_FONT) $currfont

}
    
##########################################################################
proc GmHist::mapname { node } {
    variable opt
    variable tree
    global mon
    global mapname
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { ! ( $opt($id,1,_check) ) } { return } 

    if { $opt($id,1,map) == "" } { return } 
    
    set mapname $opt($id,1,map)
    return $mapname
}

##########################################################################
proc GmHist::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	variable first
	global iconpath

    set node "hist:$count"
	set dup($count) 1
    set first 1

    set frm [ frame .histicon$count]
    set check [checkbutton $frm.check \
		-variable GmHist::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo hico -file "$iconpath/module-d.histogram.gif"
    set ico [label $frm.ico -image hico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"

    pack $check $ico -side left

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

	if { $opt($id,1,map) == ""} {
 	   $tree insert $sellayer $parent $node \
		-text      "histogram $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert $sellayer $parent $node \
		-text      "histogram for $opt($id,1,map)" \
		-window    $frm \
		-drawcross auto
	}

	set opt($count,1,opacity) $opt($id,1,opacity)

	set optlist {_check map color style nsteps nulls}

    foreach key $optlist {
    	set opt($count,1,$key) $opt($id,1,$key)
		set opt($count,0,$key) $opt($count,1,$key)
    } 
	
	set id $count
	
	# create files in tmp directory for layer output
	set mappid [pid]
	if {[catch {set lfile($count) [exec g.tempfile pid=$mappid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
	set lfilemask($count) $lfile($count)
	append lfile($count) ".ppm"
	append lfilemask($count) ".pgm"

    incr count
    return $node
}
