##########################################################################
# vector.tcl - vector display layer options file for GRASS GIS Manager
# March 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmVector {
    variable array opt # vector current options
    variable count 1
    variable array lfile # vector
    variable array lfilemask # vector
    variable optlist
    variable array dup # vector
}

###############################################################################
# set dynamic legend display in layer tree for vectors
proc GmVector::legend { id } {
    variable opt
   
    set lh $GmTree::legend_height
    set lw $GmTree::legend_width
    set mar 2
    set leg $opt($id,1,_legend)

    $leg delete all

    # area    
    if { $opt($id,1,type_area) || $opt($id,1,type_face)} {
		set x1 [expr $mar ]
		set x2 [expr {$lw - $mar} ]
		set y1 [expr $mar ]
		set y2 [expr {$lh - $mar} ]
		set lwidth  $opt($id,1,lwidth)
		if { $lwidth == 0 } { set lwidth 1 }
		if {$opt($id,1,_use_color) == 1} {
			if {$opt($id,1,_use_fcolor) == 1} {		
				$leg create rectangle $x1 $y1 $x2 $y2 -outline $opt($id,1,color) \
					-fill $opt($id,1,fcolor) -width $lwidth
			} else {
				$leg create rectangle $x1 $y1 $x2 $y2 -outline $opt($id,1,color) \
					 -width $lwidth
			}
		} else {
			if {$opt($id,1,_use_fcolor) == 1} {		
				$leg create rectangle $x1 $y1 $x2 $y2 -fill $opt($id,1,fcolor) \
					-width 0
			}
		}
    }
    
    #line
    if { $opt($id,1,type_line) || $opt($id,1,type_boundary) } {
		set x1 [expr $mar ]
		set x2 [expr {$lw - $mar} ]
		set y1 [expr $mar ]
		set y2 [expr {$lh - $mar} ]
		set lwidth  $opt($id,1,lwidth)
		if { $lwidth == 0 } { set lwidth 1 }
		if {$opt($id,1,_use_color) == 1} {
			$leg create rectangle $x1 $y1 $x2 $y2 -outline $opt($id,1,color) \
					 -width $lwidth
		}
    }

    # point 
    set xc [expr {$lw / 2 + $mar - 1} ]
    set yc [expr {$lh / 2} ]
    set size $opt($id,1,size)
   
    set maxpsize  [expr {$lw / 3 - 2} ]
    if { $size > $maxpsize } { set size $maxpsize }
	set x1 [expr {$xc - $size / 2} ]
	set x2 [expr {$xc + $size / 2 + 1} ]
	set y1 [expr {$yc - $size / 2 }]
	set y2 [expr {$yc + $size / 2 + 1} ]

    if { $opt($id,1,type_point) || $opt($id,1,type_centroid) } {
	set lwidth  $opt($id,1,lwidth)
	if { $lwidth == 0 } { set lwidth 1 }
        $leg create line $x1 $yc $x2 $yc -fill $opt($id,1,color) -width $lwidth
	$leg create line $xc $y1 $xc $y2 -fill $opt($id,1,color) -width $lwidth
    }


	set opt($id,1,mod) "1"
}

###############################################################################

# create new vector layer
proc GmVector::create { tree parent } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
    variable dup

    set node "vector:$count"

    set frm [ frame .vectoricon$count]
    set check [checkbutton $frm.check \
		-variable GmVector::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]
    set can [ canvas $frm.c -width $GmTree::legend_width \
                     -height $GmTree::legend_height ]
    set opt($count,1,_legend) $can
    pack $check $can -side left

    bind $can <ButtonPress-1> "GmTree::selectn $tree $node"

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

    $tree insert $sellayer $parent $node \
	-text      "vector $count" \
	-window    $frm \
	-drawcross auto 

    set dup($count) 0
    set opt($count,1,_check) 1 

    set opt($count,1,vect) "" 
    set opt($count,1,opacity) 1.0
    set opt($count,1,display_shape) 1 
    set opt($count,1,display_cat) 0
    set opt($count,1,display_topo) 0 
    set opt($count,1,display_dir) 0 
    set opt($count,1,display_attr) 0
    set opt($count,1,type_point) 1 
    set opt($count,1,type_line) 1
    set opt($count,1,type_boundary) 1
    set opt($count,1,type_centroid) 0
    set opt($count,1,type_area) 1
    set opt($count,1,type_face) 0 

    set opt($count,1,color) \#000000
    set opt($count,1,sqlcolor) 0
    set opt($count,1,rdmcolor) 0
    set opt($count,1,fcolor) \#AAAAAA 
    set opt($count,1,lcolor) \#000000
    set opt($count,1,_use_color) 1
    set opt($count,1,_use_fcolor) 1
    set opt($count,1,lwidth) 1

    set opt($count,1,symdir) "basic"
    set opt($count,1,icon) "basic/circle"
    set opt($count,1,size) 5 

    set opt($count,1,layer) 1 
    set opt($count,1,lfield) 1 
    set opt($count,1,cat) "" 
    set opt($count,1,where) "" 
    set opt($count,1,_use_where) 1
    set opt($count,1,qmap) "" 
	set opt($count,1,qsave) 0
	set opt($count,1,qoverwrite) 0

    set opt($count,1,attribute) "" 
    set opt($count,1,xref) "left"
    set opt($count,1,yref) "center"
    set opt($count,1,lsize) 8

    set opt($count,1,minreg) "" 
    set opt($count,1,maxreg) "" 
    set opt($count,1,mod) 1

	set optlist { _check vect opacity display_shape display_cat display_topo display_dir \
				display_attr type_point type_line type_boundary type_centroid \
				type_area type_face color _use_color fcolor _use_fcolor lcolor \
				rdmcolor sqlcolor icon size lwidth layer lfield attribute \
				xref yref lsize cat where _use_where qmap qsave qoverwrite \
				minreg maxreg}
                  
    foreach key $optlist {
		set opt($count,0,$key) $opt($count,1,$key)
    } 

    GmVector::legend $count

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

proc GmVector::set_option { node key value } {
    variable opt
 
    set id [GmTree::node_id $node]
    set opt($id,1,$key) $value

    GmVector::legend $id
}

###############################################################################

# select vector map from list and put its name in layer node
proc GmVector::select_map { id } {
    set m [GSelect vector title [G_msg "Vector map"] parent "."]
    if { $m != "" } { 
        set GmVector::opt($id,1,vect) $m
        GmTree::autonamel $m
    }
}

# select vector for output map from v.extract
proc GmVector::select_qmap { id } {
    set m [GSelect vector title [G_msg "Vector output map"] parent "."]
    if { $m != "" } { 
        set GmVector::opt($id,1,qmap) $m
        GmTree::autonamel $m
    }
}

###############################################################################
# show attribute columns in output window
proc GmVector::show_columns { id } {
	variable opt
	set mapname $opt($id,1,vect)
	set layernum $opt($id,1,layer)
	set cmd "v.info -c map=$mapname layer=$layernum"		
	run_panel $cmd
}

###############################################################################
# show attribute data in output window
proc GmVector::show_data { id } { 
	variable opt
	set mapname $opt($id,1,vect)
	set layernum $opt($id,1,layer)
	if {![catch {open "|v.db.connect map=$mapname layer=$layernum -g" r} vdb]} {
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
# show vector info in output window
proc GmVector::show_info { id } {
	variable opt
	set mapname $opt($id,1,vect)
	set layernum $opt($id,1,layer)
	set cmd "v.info map=$mapname layer=$layernum"		
	run_panel $cmd
}

###############################################################################
# select point symbols
proc GmVector::select_symbol { id } {
    variable opt
    set i [GSelect symbol title [G_msg "Vector point symbol"] parent "."]
    if { $i != "" } {
        set GmVector::opt($id,1,icon) $i
    }
}

###############################################################################

# display and set vector options
proc GmVector::options { id frm } {
    variable opt
    global bgcolor
    global iconpath
    
    set mapname ""    

    # Panel heading
    set row [ frame $frm.heading ]
    Label $row.a -text [G_msg "Display vector maps"] \
    	-fg MediumBlue
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

	#opacity
	set row [ frame $frm.opc]
	Label $row.a -text [G_msg "Opaque "]
	scale $row.b -from 1.0 -to 0.0 -showvalue 1  \
		-orient horizontal -length 300 -resolution 0.01 -fg "#656565"\
		-variable GmVector::opt($id,1,opacity) 
	Label $row.c -text [G_msg "Transparent"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes	
	
    # vector name
    set row [ frame $frm.name ]
    Label $row.a -text [G_msg "Vector map:"]
    Button $row.b -image [image create photo -file "$iconpath/element-vector.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "vector map to display"] \
		-command "GmVector::select_map $id"
    Entry $row.c -width 35 -text "$opt($id,1,vect)" \
		-textvariable GmVector::opt($id,1,vect) 
    Label $row.d -text ""
    Button $row.e -text [G_msg "Vector Info"] \
		-image [image create photo -file "$iconpath/gui-rv.info.gif"] \
		-command "GmVector::show_info $id" \
		-background $bgcolor \
		-helptext [G_msg "Vector Info"]
    Label $row.f -text ""
    Button $row.g -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q d.vect" \
		-background $bgcolor \
		-helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes
	
    # display
    set row [ frame $frm.disp ]
    Label $row.a -text [G_msg "Display: "]
    checkbutton $row.b -text [G_msg "Shapes "] -variable GmVector::opt($id,1,display_shape) \
                -command "GmVector::legend $id"  
    checkbutton $row.c -text [G_msg "Categories "] -variable GmVector::opt($id,1,display_cat) \
                -command "GmVector::legend $id"  
    checkbutton $row.d -text [G_msg "Topology "] -variable GmVector::opt($id,1,display_topo) \
                -command "GmVector::legend $id"  
    checkbutton $row.e -text [G_msg "Line directions "] -variable GmVector::opt($id,1,display_dir) \
                -command "GmVector::legend $id" 
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # type
    set row [ frame $frm.type ]
    Label $row.a -text "            "
    checkbutton $row.b -text [G_msg "Points "] -variable GmVector::opt($id,1,type_point) \
                -command "GmVector::legend $id"
    checkbutton $row.c -text [G_msg "Lines "] -variable GmVector::opt($id,1,type_line) \
                -command "GmVector::legend $id"
    checkbutton $row.d -text [G_msg "Boundaries "] -variable GmVector::opt($id,1,type_boundary) \
                -command "GmVector::legend $id"
    checkbutton $row.e -text [G_msg "Areas "] -variable GmVector::opt($id,1,type_area) \
                -command "GmVector::legend $id"
    checkbutton $row.f -text [G_msg "Centroids "] -variable GmVector::opt($id,1,type_centroid)\
                -command "GmVector::legend $id"
    checkbutton $row.g -text [G_msg "Faces "] -variable GmVector::opt($id,1,type_face) \
                -command "GmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # points
    set row [ frame $frm.icon ]  
    Label $row.a -text [G_msg "Point symbols:"]
    Button $row.b -text [G_msg "Icon"] \
            -command "GmVector::select_symbol $id" 
    Entry $row.c -width 15 -text "$opt($id,1,icon)" \
        	-textvariable GmVector::opt($id,1,icon) 
    Label $row.d -text [G_msg "   Size"]
    SpinBox $row.e -range {1 50 1} -textvariable GmVector::opt($id,1,size) \
                   -width 2 -helptext [G_msg "Icon size"] -modifycmd "GmVector::legend $id" 
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # lines
    set row [ frame $frm.color ]
    Label $row.a -text [G_msg "Draw lines:"] 
    checkbutton $row.b -variable GmVector::opt($id,1,_use_color) \
                -command "GmVector::legend $id"
    Label $row.c -text [G_msg "Color"] 
    SelectColor $row.d  -type menubutton -variable GmVector::opt($id,1,color) \
               -command "GmVector::legend $id"
    Label $row.e -text [G_msg "   Width"]
    SpinBox $row.f -range {0 50 1} -textvariable GmVector::opt($id,1,lwidth) \
                   -width 2 -helptext [G_msg "Line width"] \
                   -modifycmd "GmVector::legend $id"
    Label $row.g -text [G_msg "(pixels) "]
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # fills
    set row [ frame $frm.multicolor ]
    Label $row.a -text [G_msg "Fill areas:"] 
    checkbutton $row.b -variable GmVector::opt($id,1,_use_fcolor) \
                -command "GmVector::legend $id"
    Label $row.c -text [G_msg "Color"] 
    SelectColor $row.d -type menubutton -variable GmVector::opt($id,1,fcolor) \
                -command "GmVector::legend $id"
    Label $row.e -text "  " 
    checkbutton $row.f -text [G_msg "Random colors "] -variable GmVector::opt($id,1,rdmcolor) \
                -command "GmVector::legend $id"
    checkbutton $row.g -text [G_msg "GRASSRGB column colors"] -variable GmVector::opt($id,1,sqlcolor) \
                -command "GmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # labels
    set row [ frame $frm.label1 ]
    Label $row.a -text [G_msg "Label vectors:"] 
    checkbutton $row.b -text [G_msg "label"] -variable GmVector::opt($id,1,display_attr) \
                -command "GmVector::legend $id"
    Label $row.c -text [G_msg " Text color"] 
    SelectColor $row.d -type menubutton -variable GmVector::opt($id,1,lcolor) \
                -command "GmVector::legend $id"
    Label $row.e -text [G_msg "   Text size"] 
    SpinBox $row.f -range {1 50 1} -textvariable GmVector::opt($id,1,lsize) \
                   -width 2 -helptext [G_msg "text size"] \
                   -modifycmd "GmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left
    pack $row -side top -fill both -expand yes

	# label alighment
    set row [ frame $frm.label2 ]
    Label $row.a -text "     " 
    ComboBox $row.b -label [G_msg "Label part to align with vector point"] \
		-width 6  -textvariable GmVector::opt($id,1,xref) \
		-values {"left" "center" "right"} \
		-modifycmd "GmVector::legend $id"
    ComboBox $row.c -label [G_msg "   Justification"] \
    	-width 6  -textvariable GmVector::opt($id,1,yref) \
		-values {"top" "center" "bottom"} \
		-modifycmd "GmVector::legend $id"
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # labels layer and attribute column
    set row [ frame $frm.label3 ]
    LabelEntry $row.a -label [G_msg "     Layer for labels"] \
                -textvariable GmVector::opt($id,1,lfield) -width 3 
    LabelEntry $row.b -label [G_msg "   Attribute col for labels"] \
                -textvariable GmVector::opt($id,1,attribute) -width 23
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # query
    set row [ frame $frm.query1 ]
    Label $row.a -text [G_msg "Query vectors for display: "] 
    LabelEntry $row.b -label [G_msg "layer for query"] \
                -textvariable GmVector::opt($id,1,layer) -width 3
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

	# query cat
    set row [ frame $frm.query2 ]
    Label $row.a -text "    " 
    LabelEntry $row.b -label [G_msg "Query cat values"] \
                -textvariable GmVector::opt($id,1,cat) \
               -width 40
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # sql query
    set row [ frame $frm.where ]
    Label $row.a -text "    " 
    checkbutton $row.b -variable GmVector::opt($id,1,_use_where) \
		-command "GmVector::legend $id"
    LabelEntry $row.c -label [G_msg "Use SQL query"] \
		-textvariable GmVector::opt($id,1,where) \
		-width 40
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
	#show columns and data
	set row [ frame $frm.columns ]
    Label $row.a -text [G_msg "    Show attribute columns"] 
    Button $row.b -text [G_msg "columns"] \
            -image [image create photo -file "$iconpath/db-columns.gif"] \
            -command "GmVector::show_columns $id" \
            -background $bgcolor -borderwidth 1\
            -helptext [G_msg "Show columns"]
    Label $row.c -text [G_msg "      Show attribute data"] 
    Button $row.d -text [G_msg "data"] \
            -image [image create photo -file "$iconpath/db-values.gif"] \
            -command "GmVector::show_data $id" \
            -background $bgcolor  -borderwidth 1\
            -helptext [G_msg "Show data"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

	# save query to new vector file
	set row [ frame $frm.qsave ]
    Label $row.a -text "    " 
    checkbutton $row.b -text [G_msg "Save displayed objects to new vector file "] \
                -variable GmVector::opt($id,1,qsave) 
    checkbutton $row.c -text [G_msg "Overwrite existing"] \
                -variable GmVector::opt($id,1,qoverwrite) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # save query vector name
    set row [ frame $frm.qname ]
    Label $row.a -text [G_msg "     New vector"] 
    Button $row.b -image [image create photo -file "$iconpath/element-vector.gif"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
		-command "GmVector::select_qmap $id" \
		-helptext [G_msg "Select existing vector for saving queried objects"]
    Entry $row.c -width 40 -text "$opt($id,1,qmap)" \
          -textvariable GmVector::opt($id,1,qmap) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # display only in limited region size range
    set row [ frame $frm.region ]
    Label $row.a -text [G_msg "Display when avg. region dimension is"]
    LabelEntry $row.b -label ">" -textvariable GmVector::opt($id,1,minreg) \
                -width 8
    LabelEntry $row.c -label [G_msg " or <"] -textvariable GmVector::opt($id,1,maxreg) \
                -width 8
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
}

###############################################################################
# save layer in workspace file
proc GmVector::save { tree depth node } {
    variable opt
    variable optlist
    
    set id [GmTree::node_id $node]

    foreach key $optlist {
        GmTree::rc_write $depth "$key $opt($id,1,$key)"
    } 
}

###############################################################################

# append vector maps to display list for NVIZ display
proc GmVector::addvect {node} {
    variable opt
    variable tree
    global mon
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]
    
    if { ! ( $opt($id,1,_check) ) } { return } 

	set vect $opt($id,1,vect)
	
	return $vect
}

# set vector type for NVIZ display
proc GmVector::vecttype { vect } {
	global devnull

	set string ""
	set points 0
	set rest ""

	if {![catch {open "|v.info map=$vect 2> $devnull" r} rv]} {
		set vinfo [read $rv]
		if {[catch {close $rv} error]} {
			GmLib::errmsg $error
		}

		if { $vinfo == "" } {return}
		regexp {points:       (\d*)} $vinfo string points
		if { $points > 0} {
			set vecttype "points"
		} else {
			set vecttype "lines"
		}
	
		return $vecttype

	} else {
		puts $rv
		return
	}


}

###############################################################################

# display vector map and output to graphic file for compositing
proc GmVector::display { node mod } {
    global mon
    variable optlist
    variable lfile 
    variable lfilemask
    variable opt
    variable tree
    variable dup
    variable count
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    # If we are told dirty (for zoom) force dirty
    # Don't remove a dirty from a previous unrendered zoom
    if {$mod} {set opt($id,1,mod) 1}

    if { $opt($id,1,vect) == "" } { return } 

    if { !$opt($id,1,display_shape) && !$opt($id,1,display_cat) &&
         !$opt($id,1,display_topo)  && !$opt($id,1,display_dir) &&
         !$opt($id,1,display_attr) } { return } 

    if { !$opt($id,1,type_point) && !$opt($id,1,type_line) &&
         !$opt($id,1,type_boundary)  && !$opt($id,1,type_centroid) && 
         !$opt($id,1,type_area) && !$opt($id,1,type_face) } { return } 

    set cmd "d.vect map=$opt($id,1,vect)"
    set cmd2 "v.extract input=$opt($id,1,vect) output=$opt($id,1,qmap)"

    # color
    if { $opt($id,1,rdmcolor) } { append cmd " -c" }
    if { $opt($id,1,sqlcolor) } { append cmd " -a" }
    set color [GmLib::color $opt($id,1,color)]
    set fcolor [GmLib::color $opt($id,1,fcolor)]
    set lcolor [GmLib::color $opt($id,1,lcolor)]

    if { $opt($id,1,_use_color) } { append cmd " color=$color" } { append cmd " color=none" }
    append cmd " lcolor=$lcolor" 

    if { $opt($id,1,_use_fcolor) } { append cmd " fcolor=$fcolor" } { append cmd " fcolor=none" }

    # display
    set dlist [list]
    foreach d { shape cat topo dir } {
       if { $opt($id,1,display_$d) } { lappend dlist $d }
    }
    if { $opt($id,1,display_attr) && $opt($id,1,attribute) != "" } { lappend dlist attr }
    
    set display [join $dlist , ]
    append cmd " display=$display"

    # type
    set tlist [list]
    foreach t { point line boundary centroid area face } {
       if { $opt($id,1,type_$t) } { lappend tlist $t }
    }
    set type [join $tlist , ]
    append cmd " type=$type"
    append cmd2 " type=$type"

    append cmd " icon=$opt($id,1,icon) size=$opt($id,1,size)" 

    if { $opt($id,1,lwidth) != 1 } { 
        append cmd " width=$opt($id,1,lwidth)" 
    } 


    if { $opt($id,1,layer) != "" } { 
        append cmd " layer=$opt($id,1,layer)" 
        append cmd2 " layer=$opt($id,1,layer)" 
    } 
    if { $opt($id,1,attribute) != "" && $opt($id,1,display_attr) } { 
        append cmd " {att=$opt($id,1,attribute)}" 
    } 
    append cmd " lsize=$opt($id,1,lsize)" 
    
    append cmd " xref=$opt($id,1,xref) yref=$opt($id,1,yref)"

    if { $opt($id,1,lfield) != "" } { 
        append cmd " llayer=$opt($id,1,lfield)" 
    } 
    if { $opt($id,1,cat) != "" } { 
        append cmd " cat=$opt($id,1,cat)" 
        append cmd2 " list=$opt($id,1,cat)"
    } 
    if { $opt($id,1,where) != "" && $opt($id,1,_use_where) } { 
        append cmd " {where=$opt($id,1,where)}" 
        append cmd2 " {where=$opt($id,1,where)}" 
    } 
    if { $opt($id,1,minreg) != "" } { 
        append cmd " minreg=$opt($id,1,minreg)" 
    } 
    if { $opt($id,1,maxreg) != "" } { 
        append cmd " maxreg=$opt($id,1,maxreg)" 
    } 

    if { $opt($id,1,qoverwrite) == 1 } { 
        append cmd2 " --o" 
    }

	# use v.extract to save queried vector - will not go into redraw
    if { $opt($id,1,qsave) == 1 && $opt($id,1,qmap) != "" } {
    	run_panel $cmd2
    }

	# Decide whether to run, run command, and copy files to temp
	GmCommonLayer::display_command [namespace current] $id $cmd
}


###############################################################################

# get selected vector map (used for query)
proc GmVector::mapname { node } {
    variable opt
    variable tree
    global mon
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]


    if { $opt($id,1,vect) == "" } { return ""} 

    set mapname $opt($id,1,vect)
	return $mapname
}

###############################################################################
# digitize selected map (v.digit) and show currently displayed maps as background
proc GmVector::WorkOnVector { node mod } {
    variable opt
    variable bg
    variable tree
    global mon
    global env
    
    set tree($mon) $GmTree::tree($mon)
    set id [GmTree::node_id $node]

    if { ! ( $opt($id,1,_check) ) } { return } 

    if { $opt($id,1,vect) == "" } { return } 
    
    if {[GmLib::element_exists "vector" $opt($id,1,vect)]} {
        set cmd [list v.digit "map=$opt($id,1,vect)"]
    } else { 
        set cmd [list v.digit -n "map=$opt($id,1,vect)"]
    }

     set bg_command [GmGroup::vdigit_display "vector:$id"]
     if {$bg_command != ""} {
     	append cmd " {bgcmd=$bg_command}"
     }

    spawn_panel $cmd
}

###############################################################################
# duplicate currently selected vector node
proc GmVector::duplicate { tree parent node id } {
    variable optlist
    variable lfile
    variable lfilemask
    variable opt
    variable count
	variable dup
	
    set node "vector:$count"
	set dup($count) 1

    set frm [ frame .vectoricon$count]
    set check [checkbutton $frm.check \
		-variable GmVector::opt($count,1,_check) \
		-height 1 -padx 0 -width 0]
    set can [ canvas $frm.c -width $GmTree::legend_width \
                     -height $GmTree::legend_height -borderwidth 0 ]
    set opt($count,1,_legend) $can
    pack $check $can -side left

    bind $can <ButtonPress-1> "GmTree::selectn $tree $node"

	#insert new layer
	if {[$tree selection get] != "" } {
		set sellayer [$tree index [$tree selection get]]
    } else { 
    	set sellayer "end" 
    }

	if { $opt($id,1,vect) == ""} {
	    $tree insert $sellayer $parent $node \
		-text      "vector $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert $sellayer $parent $node \
		-text      "$opt($id,1,vect)" \
		-window    $frm \
		-drawcross auto
	} 
	
	set opt($count,1,opacity) $opt($id,1,opacity)

	set optlist { _check vect display_shape display_cat display_topo display_dir \
				display_attr type_point type_line type_boundary type_centroid \
				type_area type_face color _use_color fcolor _use_fcolor lcolor \
				rdmcolor sqlcolor icon size lwidth layer lfield attribute \
				xref yref lsize cat where _use_where qmap qsave qoverwrite \
				minreg maxreg minreg maxreg}
                  
    foreach key $optlist {
    	set opt($count,1,$key) $opt($id,1,$key)
		set opt($count,0,$key) $opt($count,1,$key)
    } 

    GmVector::legend $count
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
