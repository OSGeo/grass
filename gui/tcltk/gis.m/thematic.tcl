##########################################################################
# thematic.tcl - thematic vector mapping layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmThematic {
    variable array opt # thematic current options
	variable array tlegend # mon id
	variable array tlegcan # mon id
    variable count 1
    variable array lfile # raster
    variable array lfilemask # raster
    variable optlist
    variable array dup # vector
}


###############################################################################
# create new thematic layer

proc GmThematic::create { tree parent } {
    variable opt
    variable count
    variable dup
    variable lfile
    variable lfilemask
    variable optlist
    global iconpath

    set node "thematic:$count"

    set frm [ frame .thematicicon$count]
    set check [checkbutton $frm.check \
		-variable GmThematic::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo thematicico -file "$iconpath/module-d.vect.thematic.gif"
    set ico [label $frm.ico -image thematicico -bd 1 -relief raised]

    bind $ico <ButtonPress-1> "GmTree::selectn $tree $node"
    
    pack $check $ico -side left
    
	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text  "thematic $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,1,_check) 1 
    set dup($count) 0
    
    set opt($count,1,map) "" 
	set opt($count,1,opacity) 1.0
    set opt($count,1,type) "area"
    set opt($count,1,column) "" 
    set opt($count,1,themetype) "graduated_colors" 
    set opt($count,1,themecalc) "interval" 
    set opt($count,1,breakpoints) "" 
    set opt($count,1,where) "" 
    set opt($count,1,layer) 1 
    set opt($count,1,icon) "basic/circle" 
    set opt($count,1,ptsize) 5 
    set opt($count,1,maxsize) 20 
    set opt($count,1,nint) 4 
    set opt($count,1,colorscheme) "blue-red" 
    set opt($count,1,pointcolor) \#FF0000 
    set opt($count,1,linecolor) \#000000 
    set opt($count,1,startcolor) \#FF0000 
    set opt($count,1,endcolor) \#0000FF 
    set opt($count,1,update_rgb) 0 
    set opt($count,1,math) 0 
    set opt($count,1,psmap) "" 
    set opt($count,1,border) 1 
    # keep font names here to make sure that all fonts used are in proper TclTk format
    set opt($count,1,titlefont) "{times} 14 bold" 
    set opt($count,1,subtitlefont) "{times} 12 bold"
    set opt($count,1,labelfont) "{times} 12" 
    set opt($count,1,tfontcolor) \#000000  
    set opt($count,1,lfontcolor) \#000000  
    set opt($count,1,mod) 1

	set optlist { _check map opacity type column themetype themecalc breakpoints where \
             layer icon ptsize maxsize nint colorscheme pointcolor linecolor\
             startcolor endcolor border update_rgb math psmap \
             titlefont tfontcolor subtitlefont labelfont lfontcolor} 

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

###############################################################################

proc GmThematic::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value
}

proc GmThematic::select_map { id } {
    variable tree
    variable node
    set m [GSelect vector title [G_msg "Vector map"] parent "."]
    if { $m != "" } { 
        set GmThematic::opt($id,1,map) $m
        GmTree::autonamel [format [G_msg "thematic map for %s"] $m]
    }
}

###############################################################################
# select fonts for legend
proc GmThematic::select_tfont { id frm} {
	variable opt
    
    set fon [SelectFont $frm.font -type dialog -sampletext [G_msg "This is font sample text."] -title [G_msg "Select font"]]
	if { $fon != "" } {set opt($id,1,titlefont) $fon}
}

proc GmThematic::select_stfont { id frm} {
	variable opt
    
    set fon [SelectFont $frm.font -type dialog -sampletext [G_msg "This is font sample text."] -title [G_msg "Select font"]]
	if { $fon != "" } {set opt($id,1,subtitlefont) $fon}
}
proc GmThematic::select_lfont { id frm} {
	variable opt
    
    set fon [SelectFont $frm.font -type dialog -sampletext [G_msg "This is font sample text."] -title [G_msg "Select font"]]
	if { $fon != "" } {set opt($id,1,labelfont) $fon}
}

###############################################################################
# show attribute columns and attribute values

proc GmThematic::show_columns { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,1,map)
	set layernum $opt($id,1,layer)
	set cmd "v.info -c map=$mapname layer=$layernum"
	run_panel $cmd
}

proc GmThematic::show_data { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,1,map)
	set layer $opt($id,1,layer)
	if {![catch {open "|v.db.connect map=$mapname layer=$layer -g" r} vdb]} {
		set vectdb [read $vdb]
		if {[catch {close $vdb} error]} {
			GmLib::errmsg $error
		}
		set vdblist [split $vectdb " "]
		set tbl [lindex $vdblist 1]
		set db [lindex $vdblist 3]
		set drv [lindex $vdblist 4]
		set cmd "db.select table=$tbl database=$db driver=$drv"
		run_panel $cmd
	}
}

###############################################################################

# select symbols from directories
proc GmThematic::select_symbol { id } {
    variable opt
    set i [GSelect symbol title [G_msg "Vector point symbol"] parent "."]
    if { $i != "" } {
        set GmThematic::opt($id,1,icon) $i
    }
}

###############################################################################

# set thematic options
proc GmThematic::options { id frm } {
    variable opt
    global iconpath
    global bgcolor

    # Panel heading
    set row [ frame $frm.heading1 ]
    Label $row.a -text [G_msg "Display vector maps thematically by graduate colors (all types)"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    set row [ frame $frm.heading2 ]
    Label $row.a -text [G_msg "  or by graduated sizes (points and lines)"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmThematic::opt($id,1,opacity) 
	Label $row.c -text [G_msg " Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # vector name
    set row [ frame $frm.map ]
    Label $row.a -text [G_msg "Vector map:"]
    Button $row.b -image [image create photo -file "$iconpath/element-vector.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "vector for thematic mapping"] \
		-command "GmThematic::select_map $id"
    Entry $row.c -width 35 -text " $opt($id,1,map)" \
          -textvariable GmThematic::opt($id,1,map) 
    Label $row.d -text "   "
    Button $row.e -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q d.vect.thematic" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # vector type and layer
    set row [ frame $frm.vtype ]
    Label $row.a -text [G_msg "    vector type"] 
    ComboBox $row.b -padx 2 -width 10 -textvariable GmThematic::opt($id,1,type) \
                    -values {"area" "point" "centroid" "line" "boundary"}
    Label $row.c -text [G_msg " attribute layer"]
    LabelEntry $row.d -textvariable GmThematic::opt($id,1,layer) -width 3 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # vector column
    set row [ frame $frm.column ]
    Label $row.a -text [G_msg "    NUMERIC attribute column to use for thematic map"]
    LabelEntry $row.b -textvariable GmThematic::opt($id,1,column) -width 15
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
	#show columns and data
	set row [ frame $frm.columns ]
    Label $row.a -text [G_msg "    show attribute columns"] 
    Button $row.b -text [G_msg "columns"] \
            -image [image create photo -file "$iconpath/db-columns.gif"] \
            -command "GmThematic::show_columns $id" \
            -background $bgcolor \
            -helptext [G_msg "Show columns"]
    Label $row.c -text [G_msg "   show data"] 
    Button $row.d -text [G_msg "data"] \
            -image [image create photo -file "$iconpath/db-values.gif"] \
            -command "GmThematic::show_data $id" \
            -background $bgcolor \
            -helptext [G_msg "Show data"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
    # Thematic type
    set row [ frame $frm.ttype ]
    Label $row.a -text [G_msg "Thematic map: type"] 
    ComboBox $row.b -padx 2 -width 16 -textvariable GmThematic::opt($id,1,themetype) \
		-values {"graduated_colors" "graduated_points" "graduated_lines"} 
    Label $row.c -text [G_msg " map by"] 
    ComboBox $row.d -padx 2 -width 15 -textvariable GmThematic::opt($id,1,themecalc) \
		-values {"interval" "std_deviation" "quartiles" \
		"custom_breaks"} 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # intervals
    set row [ frame $frm.int ]
    Label $row.a -text [G_msg "    number of intervals to map (interval themes):"]
    SpinBox $row.b -range {1 99 1} -textvariable GmThematic::opt($id,1,nint) \
                    -width 3 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # breakpoints
    set row [ frame $frm.break ]
    Label $row.a -text [G_msg "    custom breakpoints (val val ...)  "]
    LabelEntry $row.b -textvariable GmThematic::opt($id,1,breakpoints) -width 32
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # where
    set row [ frame $frm.where ]
    Label $row.a -text [G_msg "    query with SQL where clause   "]
    LabelEntry $row.b -textvariable GmThematic::opt($id,1,where) -width 32 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # point options1
    set row [ frame $frm.pts1 ]  
    Label $row.a -text [G_msg "Graduated points & lines: "]
    Button $row.b -text [G_msg "icon"] \
	    -command "GmThematic::select_symbol $id"
    Entry $row.c -width 10 -text "$opt($id,1,icon)" \
	    -textvariable GmThematic::opt($id,1,icon)  
    Label $row.d -text [G_msg "point color"] 
    SelectColor $row.e -type menubutton -variable GmThematic::opt($id,1,pointcolor)
    Label $row.f -text [G_msg "line color"] 
    SelectColor $row.g -type menubutton -variable GmThematic::opt($id,1,linecolor)
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # point options2
    set row [ frame $frm.pts2 ]  
    Label $row.a -text [G_msg "    size/min size (graduated pts/lines)"]
    SpinBox $row.b -range {1 50 1} -textvariable GmThematic::opt($id,1,ptsize) \
        -width 2 -helptext [G_msg "icon size/min size (graduated pts/lines)"]
    Label $row.c -text [G_msg "max size (graduated pts)"]
    SpinBox $row.d -range {1 50 1} -textvariable GmThematic::opt($id,1,maxsize) \
        -width 2 -helptext [G_msg " max size (graduated pts/lines)"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # color options1
    set row [ frame $frm.color1 ]
    Label $row.a -text [G_msg "Graduated colors: preset color schemes"] 
    ComboBox $row.b -padx 2 -width 18 -textvariable GmThematic::opt($id,1,colorscheme) \
        -values {"blue-red" "red-blue" "green-red" "red-green" \
        "blue-green" "green-blue" "cyan-yellow" "yellow-cyan" "custom_gradient" \
        "single_color" } 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # color options2
    set row [ frame $frm.color2 ]
    Label $row.a -text [G_msg "    custom color scheme - start color"]
    SelectColor $row.b -type menubutton -variable GmThematic::opt($id,1,startcolor)
    Label $row.c -text [G_msg " end color"]
    SelectColor $row.d -type menubutton -variable GmThematic::opt($id,1,endcolor)
    checkbutton $row.e -text [G_msg "draw border"] -variable GmThematic::opt($id,1,border)     
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
    # color options3
    set row [ frame $frm.color3 ]
    Label $row.a -text "   "
    checkbutton $row.b -text [G_msg "save thematic colors to GRASSRGB column of vector file"] -variable \
        GmThematic::opt($id,1,update_rgb) 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # legend 1
    set row [ frame $frm.legend1 ]
    Label $row.a -text [G_msg "Legend: title font "] 
    Button $row.b -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "title font for legend"] \
	    -command "GmThematic::select_tfont $id $frm"
    Entry $row.c -width 15 -text "$opt($id,1,titlefont)" \
	    -textvariable GmThematic::opt($id,1,titlefont)  
    Label $row.d -text [G_msg " font color"]
    SelectColor $row.e -type menubutton -variable GmThematic::opt($id,1,tfontcolor)
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
    # legend 2
    set row [ frame $frm.legend2 ]
    Label $row.a -text [G_msg "    subtitle font    "] 
    Button $row.b -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "subtitle font for legend"] \
	    -command "GmThematic::select_stfont $id $frm"
    Entry $row.c -width 15 -text "$opt($id,1,subtitlefont)" \
	    -textvariable GmThematic::opt($id,1,subtitlefont)  
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # legend 3
    set row [ frame $frm.legend3 ]
    Label $row.a -text [G_msg "    label font       "] 
    Button $row.b -image [image create photo -file "$iconpath/gui-font.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "label font for legend"] \
	    -command "GmThematic::select_lfont $id $frm"
    Entry $row.c -width 15 -text "$opt($id,1,labelfont)" \
	    -textvariable GmThematic::opt($id,1,labelfont)  
    Label $row.d -text [G_msg " font color"]
    SelectColor $row.e -type menubutton -variable GmThematic::opt($id,1,lfontcolor)
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
	# legend 4
    set row [ frame $frm.legend4 ]
    Label $row.a -text "   "
    checkbutton $row.b -text [G_msg "use math notation in legend"] -variable \
        GmThematic::opt($id,1,math) 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # psmap
    set row [ frame $frm.psmap ]
    Label $row.a -text [G_msg "Name for ps.map instruction files"]
    LabelEntry $row.b -textvariable GmThematic::opt($id,1,psmap) -width 34 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

###############################################################################
# save to grc file

proc GmThematic::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}


###############################################################################

# get selected vector map 
proc GmThematic::mapname { node } {
    variable opt
    variable tree
    global mon
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { $opt($id,1,map) == "" } { return ""} 


    set mapname $opt($id,1,map)
	return $mapname
}

###############################################################################
# render and composite thematic layer

proc GmThematic::display { node mod } {
    global mon
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count

    set line ""
    set input ""
    set cmd ""

    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,map) == "" } { return } 
    if { $opt($id,1,column) == "" } { return }

    # set hex colors to rgb         
    set pointcolor [GmLib::color $opt($id,1,pointcolor)]
    set linecolor [GmLib::color $opt($id,1,linecolor)]
    set startcolor [GmLib::color $opt($id,1,startcolor)]
    set endcolor [GmLib::color $opt($id,1,endcolor)]
    
    # turn off x11 display
    set monitor "none"

    #create d.vect.thematic command
    set cmd "d.vect.thematic -s map=$opt($id,1,map) type=$opt($id,1,type) column=$opt($id,1,column) \
			layer=$opt($id,1,layer) icon=$opt($id,1,icon) size=$opt($id,1,ptsize) \
            maxsize=$opt($id,1,maxsize) nint=$opt($id,1,nint) pointcolor=$pointcolor \
			linecolor=$linecolor startcolor=$startcolor endcolor=$endcolor \
			themetype=$opt($id,1,themetype) monitor=$monitor \
			themecalc=$opt($id,1,themecalc) colorscheme=$opt($id,1,colorscheme)"
             
    # breakpoints
    if { $opt($id,1,breakpoints) != "" } { 
        append cmd " {breakpoints=$opt($id,1,breakpoints)}"
    }

    # where query
    if { $opt($id,1,where) != "" } { 
        append cmd " {where=$opt($id,1,where)}"
    }

    # psmap file 
    if { $opt($id,1,psmap) != "" } { 
        append cmd " psmap=$opt($id,1,psmap)"
    }

    # hide border
    if { $opt($id,1,border) == 0 } { 
        append cmd "  -f"
    }

    # update_rgb
    if { $opt($id,1,update_rgb) == 1 } { 
        append cmd " -u"
    }

    # math notation
    if { $opt($id,1,math) == 1 } { 
        append cmd " -m"
    }

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd

	# Legend
    if { $opt($id,1,_check) } {
		GmThematic::tlegend $mon $id
		GmThematic::tleg_item $mon $id
	}
	
}


###############################################################################
# duplicate thematic layer 
proc GmThematic::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	global iconpath

    set node "thematic:$count"
	set dup($count) 1

    set frm [ frame .thematicicon$count]
    set check [checkbutton $frm.check \
		-variable GmThematic::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]

    image create photo thematicico -file "$iconpath/module-d.vect.thematic.gif"
    set ico [label $frm.ico -image thematicico -bd 1 -relief raised]

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
		-text      "thematic $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert $sellayer $parent $node \
		-text      "thematic map for $opt($id,1,map)" \
		-window    $frm \
		-drawcross auto
	} 

	set opt($count,1,opacity) $opt($id,1,opacity)
	
	set optlist { _check map type column themetype themecalc breakpoints where \
		 layer icon ptsize maxsize nint colorscheme pointcolor linecolor\
		 startcolor endcolor border update_rgb math psmap \
		 titlefont tfontcolor subtitlefont labelfont lfontcolor} 

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

###############################################################################

# create graphic legend in separate display canvas
proc GmThematic::tlegend { mon id } {
	global bgcolor
	global iconpath
    global env
	variable opt
	variable tlegend
	variable tlegcan

	if { [winfo exists .tlegend($mon,$id)] } {return}

	set legendtitle [format [G_msg "Legend for Map %d, %s"] $mon $opt($id,1,map)]
	toplevel .tlegend($mon,$id)
    wm title .tlegend($mon,$id) $legendtitle


    wm withdraw .tlegend($mon,$id)
    #wm overrideredirect $txt 1

	# create canvas for legend
	set tlegmf [MainFrame .tlegend($mon,$id).mf ]
	set tlegcan($mon,$id) [canvas $tlegmf.can -bg white\
		-borderwidth 0 -closeenough 1.0 \
        -relief ridge -selectbackground #c4c4c4 \
        -width 300 -height 300 ]
	   
    # setting geometry
    place $tlegcan($mon,$id) \
        -in $tlegmf -x 0 -y 0 -anchor nw \
        -bordermode ignore 

	# control buttons
	set tleg_tb [$tlegmf addtoolbar]
	set tlbb [ButtonBox $tleg_tb.bb -orient horizontal]
	$tlbb add -text [G_msg "clear"] -command "GmThematic::tleg_erase $mon $id" -bg #dddddd \
		-highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Clear legend"] -highlightbackground $bgcolor
	$tlbb add -text [G_msg "save"] -command "GmThematic::tleg_save $mon $id"  -bg #dddddd \
		-highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Save legend to EPS file"] -highlightbackground $bgcolor

	pack $tlegmf -expand yes -fill both -padx 0 -pady 0
	pack $tlegcan($mon,$id) -fill both -expand yes
	pack $tlbb -side left -anchor w
			
	BWidget::place .tlegend($mon,$id) 0 0 at 500 100
    wm deiconify .tlegend($mon,$id)

}

# read legend file and create legend items
proc GmThematic::tleg_item { mon id } {
	variable tlegend
	variable tlegcan
	variable opt
	global legfile
	
	GmThematic::tleg_erase $mon $id
	# get legend file created by d.vect.thematic in GRASS tmp diretory
	set mappid [pid]
	if {[catch {set tmpdir [file dirname [exec g.tempfile pid=$mappid]]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}

	set legfile "$tmpdir/gismlegend.txt"
	if {![file exists $legfile]} {return}
	catch {set ltxt [open $legfile r]}
	set x1 30
	set y1 40
	set txtx 60
	set font $opt($id,1,labelfont)
	regexp {.*\s(\d*)} $font string lineht
	set yinc [expr {$lineht * 2}]	
	
	set titlefont $opt($id,1,titlefont)
	set tfontcolor $opt($id,1,tfontcolor)
	set subtitlefont $opt($id,1,subtitlefont)

	set labelfont $opt($id,1,labelfont)
	set lfontcolor $opt($id,1,lfontcolor)
	while {![eof $ltxt]} {
		gets $ltxt line
		set type [lindex $line 0]
		set fcolor [lindex $line 1]
		set lcolor [lindex $line 2]
		set size [lindex $line 3]
		set label [lindex $line 4]
		if { $fcolor != "-" } { set xfcolor [GmThematic::rgb2hex $fcolor] }
		if { $lcolor != "-" } { set xlcolor [GmThematic::rgb2hex $lcolor] }
		switch $type {
			title {
				regexp {.*\s(\d*)\s.*} $titlefont string lineht
				set yinc [expr {$lineht * 2}]	
				set x2 [expr {$x1 + 15}]
				set y2 [expr {$y1 + 15}]
				$tlegcan($mon,$id) create text $x1 $y2 -anchor sw -width 250 \
					-fill $tfontcolor -font $titlefont -text "$label"
			}
			subtitle {
				regexp {.*\s(\d*)\s.*} $subtitlefont string lineht
				set yinc [expr {$lineht * 2}]	
				set x2 [expr {$x1 + 15}]
				set y2 [expr {$y1 + 15}]
				$tlegcan($mon,$id) create text $x1 $y2 -anchor sw -width 250 \
					-fill $tfontcolor -font $subtitlefont -text "$label"
				incr y2 10
				$tlegcan($mon,$id) create line $x1 $y2 [expr {$x1 + 250}] $y2 \
					-width 1 -fill #000000				
				incr y1 10
			}
			text {
				regexp {.*\s(\d*)\s.*} $labelfont string lineht
				set yinc [expr {$lineht * 2}]	
				set x2 [expr {$x1 + 15}]
				set y2 [expr {$y1 + 15}]
				$tlegcan($mon,$id) create text $x1 $y2 -anchor sw -width 250 \
					-fill $lfontcolor -font $labelfont -text "$label"
			}
			area {
				regexp {.*\s(\d*)\s.*} $labelfont string lineht
				set yinc [expr {$lineht * 2}]	
				set x2 [expr {$x1 + 15}]
				set y2 [expr {$y1 + 15}]
				$tlegcan($mon,$id) create rectangle $x1 $y1 $x2 $y2 -fill $xfcolor \
					-outline $xlcolor
				$tlegcan($mon,$id) create text [expr {$x2 + 15}] [expr {(($y2-$y1)/2) + $y1}] \
				-fill $lfontcolor -anchor w -font $labelfont -text "$label"
			}
			point {
				regexp {.*\s(\d*)\s.*} $labelfont string lineht
				set yinc [expr {$lineht * 2}]	
				if { $size > [expr {$yinc + 2}] } {
					incr y1 [expr {int(($size/5) + 2)}]
				}
				if { $txtx <= [expr {$x1 + $size + 15}] } {
					set txtx [expr {$x1 + $size + 15}]
				}
				set x2 [expr {$x1 + $size}]
				set y2 [expr {$y1 + $size}]
				$tlegcan($mon,$id) create oval $x1 $y1 $x2 $y2 -fill $xfcolor \
					-outline $xlcolor
				$tlegcan($mon,$id) create text $txtx [expr (($y2-$y1)/2) + $y1] \
				-fill $lfontcolor -anchor w -font $labelfont -text "$label"
			}
			line {
				regexp {.*\s(\d*)\s.*} $labelfont string lineht
				set yinc [expr {$lineht * 2}]	
				set x2 [expr {$x1 + 15}]
				set y2 [expr {$y1 + 15}]
				$tlegcan($mon,$id) create line $x1 $y1 $x2 $y2 -width $size  \
					-fill $xlcolor
				$tlegcan($mon,$id) create text [expr $x2 + 15] [expr (($y2-$y1)/2) + $y1] \
				-fill $lfontcolor -anchor w -font $labelfont -text "$label"
			}
			default { break }
		}
		if { $size > $yinc } {
			incr y1 [expr {int($size + 2)}]
		} else {
			incr y1 $yinc
		}
	}
	if {[catch {close $ltxt} error]} {
		GmLib::errmsg $error
	}

	return
}

# rgb to hex color convertor
proc GmThematic::rgb2hex { clr } {
	set rgb [split $clr :]
	set r [lindex $rgb 0]
	set g [lindex $rgb 1]
	set b [lindex $rgb 2]
	if {$r == "" || $g == "" || $b == ""} {return}
	set xclr [format "#%02x%02x%02x" $r $g $b]
	return $xclr
}

# erase legend canvas
proc GmThematic::tleg_erase { mon id} {
	variable tlegcan
	
	$tlegcan($mon,$id) delete all
	return
}

#save legend canvas (might use maptool procedures)
proc GmThematic::tleg_save { mon id} {
	global env
	variable tlegcan
		
	set types {
    {{EPS} {.eps}}
	}

	if { [info exists HOME] } {
		set dir $env(HOME)
		set path [tk_getSaveFile -filetypes $types -initialdir $dir \
			-defaultextension ".eps"]
	} else {
		set path [tk_getSaveFile -filetypes $types -defaultextension ".eps"]
	}
	
	$tlegcan($mon,$id) postscript -file "$path"

	return
}
