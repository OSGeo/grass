# 18 March 2005

namespace eval DmVector {
    variable array opt # vector options
    variable count 1
}


proc DmVector::legend { id } {
    variable opt
   
    set lh $DmTree::legend_height
    set lw $DmTree::legend_width
    set mar 2
    set leg $opt($id,_legend)

    $leg delete all

    # point 
    set xc [expr $lw / 6 + 1 ]
    set yc [expr $lh / 2 ]
    set size $opt($id,size)
   
    set maxpsize  [expr $lw / 3 - 2 ]
    if { $size > $maxpsize } { set size $maxpsize }
    set x1 [expr $xc - $size / 2 ]
    set x2 [expr $xc + $size / 2 + 1 ]
    set y1  [expr $yc - $size / 2 ]
    set y2  [expr $yc + $size / 2 + 1 ]

    if { $opt($id,type_point) || $opt($id,type_centroid) } {
        $leg create line $x1 $yc $x2 $yc -fill $opt($id,color)
	$leg create line $xc $y1 $xc $y2 -fill $opt($id,color)
    }
    # line    
    if { $opt($id,type_line) || $opt($id,type_boundary) || $opt($id,type_face) } {
	set x1 [expr $lw / 3 + $mar ]
	set x2 [expr 2 * $lw / 3 - $mar ]
	set y1 [expr $lh - $mar ]
	set y2 [expr $mar ]
        $leg create line $x1 $y1 $x2 $y2 -fill $opt($id,color)
    }
    # area    
    if { $opt($id,type_area) } {
	set x1 [expr 2 * $lw / 3 + $mar ]
	set x2 [expr $lw - $mar ]
	set y1 [expr $mar ]
	set y2 [expr $lh - $mar ]
	$leg create rectangle $x1 $y1 $x2 $y2 -outline $opt($id,color) \
                              -fill $opt($id,fcolor)
    }
}

proc DmVector::create { tree parent } {
    global form_mode
    variable opt
    variable count

    set node "vector:$count"

    set frm [ frame .vectoricon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmVector::opt($count,_check) \
                           -height 1 -padx 0 -width 0]
    set can [ canvas $frm.c -width $DmTree::legend_width \
                     -height $DmTree::legend_height ]
    set opt($count,_legend) $can
    pack $check $can -side left

    $tree insert end $parent $node \
	-text      "vector $count" \
	-window    $frm \
	-drawcross auto 

    set opt($count,_check) 1 

    set opt($count,map) "" 
    set opt($count,display_shape) 1 
    set opt($count,display_cat) 0
    set opt($count,display_topo) 0 
    set opt($count,display_dir) 0 
    set opt($count,display_attr) 0
    set opt($count,type_point) 1 
    set opt($count,type_line) 1
    set opt($count,type_boundary) 1
    set opt($count,type_centroid) 1
    set opt($count,type_area) 1
    set opt($count,type_face) 0 

    set opt($count,color) \#000000
    set opt($count,sqlcolor) 0
    set opt($count,rdmcolor) 0
    set opt($count,fcolor) \#AAAAAA 
    set opt($count,lcolor) \#000000
    set opt($count,_use_color) 1
    set opt($count,_use_fcolor) 1
    set opt($count,lwidth) 1 

    set opt($count,symdir) "basic"
    set opt($count,icon) "basic/x"
    set opt($count,size) 5 

    set opt($count,field) 1 
    set opt($count,lfield) 1 
    set opt($count,cat) "" 
    set opt($count,where) "" 
    set opt($count,_use_where) 1

    set opt($count,attribute) "" 
    set opt($count,xref) "left"
    set opt($count,yref) "center"
    set opt($count,lsize) 8

    set opt($count,minreg) "" 
    set opt($count,maxreg) "" 

    # Default form mode used for vectors, it can be 'gui' (default) or 'txt'
    set form_mode [exec g.gisenv get=DM_FORM_MODE]
    if { $form_mode == "txt" } {
        set opt($count,_query_text) 1 
    } else {
        set opt($count,_query_text) 0
    }
    set opt($count,_query_edit) 0 

    set opt($count,_width) 1

    DmVector::legend $count

    incr count
    return $node
}

proc DmVector::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value

    DmVector::legend $id
}

proc DmVector::select_map { id } {
    set m [GSelect vector]
    if { $m != "" } { 
        set DmVector::opt($id,map) $m
        Dm::autoname $m
    }
}

proc DmVector::show_columns { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,map)
	set layernum $opt($id,field)
        set cmd "v.info -c map=$mapname layer=$layernum"
        run_panel $cmd
}

proc DmVector::show_data { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,map)
	set layernum $opt($id,field)
	if {![catch {open "|v.db.connect map=$mapname layer=$layernum -g" r} vdb]} {
		set vectdb [read $vdb]
		catch {close $vdb}
		set vdblist [split $vectdb " "]
		set tbl [lindex $vdblist 1]
		set db [lindex $vdblist 3]
		set drv [lindex $vdblist 4]
		set cmd "db.select table=$tbl database=$db driver=$drv"
		run_panel $cmd
	}
}

# select symbols from directories
proc DmVector::select_symbol { id } {
    variable opt
    set i [GSelect symbol]
    if { $i != "" } {
        set DmVector::opt($id,icon) $i
    }
}

# display vector options
proc DmVector::options { id frm } {
    variable opt
    global dmpath
    global bgcolor
    global mapname
    set mapname ""

    # vector name
    set row [ frame $frm.name ]
    Button $row.a -text [G_msg "Vector name:"] \
           -command "DmVector::select_map $id"
    Entry $row.b -width 40 -text "$opt($id,map)" \
          -textvariable DmVector::opt($id,map) \
          -background white
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.vect" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # display
    set row [ frame $frm.disp ]
    Label $row.a -text [G_msg "Display:"]
    checkbutton $row.b -text [G_msg "shapes"] -variable DmVector::opt($id,display_shape) \
                -command "DmVector::legend $id"
    checkbutton $row.c -text [G_msg "categories"] -variable DmVector::opt($id,display_cat) \
                -command "DmVector::legend $id"
    checkbutton $row.d -text [G_msg "topology"] -variable DmVector::opt($id,display_topo) \
                -command "DmVector::legend $id"
    checkbutton $row.e -text [G_msg "line directions"] -variable DmVector::opt($id,display_dir) \
                -command "DmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # type
    set row [ frame $frm.type ]
    Label $row.a -text [G_msg "            "]
    checkbutton $row.b -text [G_msg "points"] -variable DmVector::opt($id,type_point) \
                -command "DmVector::legend $id"
    checkbutton $row.c -text [G_msg "lines"] -variable DmVector::opt($id,type_line) \
                -command "DmVector::legend $id"
    checkbutton $row.d -text [G_msg "boundaries"] -variable DmVector::opt($id,type_boundary) \
                -command "DmVector::legend $id"
    checkbutton $row.e -text [G_msg "centroids"] -variable DmVector::opt($id,type_centroid)\
                -command "DmVector::legend $id"
    checkbutton $row.f -text [G_msg "areas"] -variable DmVector::opt($id,type_area) \
                -command "DmVector::legend $id"
    checkbutton $row.g -text [G_msg "faces"] -variable DmVector::opt($id,type_face) \
                -command "DmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # points
    set row [ frame $frm.icon ]  
    Label $row.a -text "Point symbols:" 
    Button $row.b -text [G_msg "icon"] \
            -command "DmVector::select_symbol $id"
    Entry $row.c -width 15 -text "$opt($id,icon)" \
        	-textvariable DmVector::opt($id,icon) \
        	-background white 
    Label $row.d -text "  size" 
    SpinBox $row.e -range {1 50 1} -textvariable DmVector::opt($id,size) \
                   -width 2 -helptext "Icon size" -modifycmd "DmVector::legend $id" \
                   -entrybg white 
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # lines
    set row [ frame $frm.color ]
    Label $row.a -text [G_msg "Draw lines:"] 
    checkbutton $row.b -variable DmVector::opt($id,_use_color) \
                -command "DmVector::legend $id"
    Label $row.c -text [G_msg "color"] 
    SelectColor $row.d  -type menubutton -variable DmVector::opt($id,color) \
               -command "DmVector::legend $id"
    Label $row.e -text " width" 
    SpinBox $row.f -range {1 50 1} -textvariable DmVector::opt($id,lwidth) \
                   -entrybg white -width 2 -helptext "Line width" \
                   -modifycmd "DmVector::legend $id"
    Label $row.g -text "(pixels) " 
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # fills
    set row [ frame $frm.multicolor ]
    Label $row.a -text [G_msg "Fill areas:"] 
    checkbutton $row.b -variable DmVector::opt($id,_use_fcolor) \
                -command "DmVector::legend $id"
    Label $row.c -text [G_msg "color"] 
    SelectColor $row.d -type menubutton -variable DmVector::opt($id,fcolor) \
                -command "DmVector::legend $id"
    Label $row.e -text [G_msg "  "] 
    checkbutton $row.f -text [G_msg "random colors"] -variable DmVector::opt($id,rdmcolor) \
                -command "DmVector::legend $id"
    checkbutton $row.g -text [G_msg "GRASSRGB column colors"] -variable DmVector::opt($id,sqlcolor) \
                -command "DmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # labels
    set row [ frame $frm.label ]
    Label $row.a -text [G_msg "Label vectors:"] 
    checkbutton $row.b -text [G_msg "label"] -variable DmVector::opt($id,display_attr) \
                -command "DmVector::legend $id"
    Label $row.c -text [G_msg "color"] 
    SelectColor $row.d -type menubutton -variable DmVector::opt($id,lcolor) \
                -command "DmVector::legend $id"
    Label $row.e -text [G_msg " size"] 
    SpinBox $row.f -range {1 50 1} -textvariable DmVector::opt($id,lsize) \
                   -width 2 -helptext [G_msg "text size"] \
                   -modifycmd "DmVector::legend $id" -entrybg white 
    ComboBox $row.g -label [G_msg " align with pt"] \
                    -width 6  -textvariable DmVector::opt($id,xref) \
                    -entrybg white \
                    -values {"left" "center" "right"} \
                    -modifycmd "DmVector::legend $id"
    ComboBox $row.h -width 6  -textvariable DmVector::opt($id,yref) \
                    -entrybg white \
                    -values {"top" "center" "bottom"} \
                    -modifycmd "DmVector::legend $id"
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g $row.h -side left
    pack $row -side top -fill both -expand yes

    # labels layer and attribute column
    set row [ frame $frm.attribute ]
    LabelEntry $row.a -label [G_msg "     layer for labels"] \
                -textvariable DmVector::opt($id,lfield) -width 3 \
                -entrybg white
    LabelEntry $row.b -label [G_msg " attribute col for labels"] \
                -textvariable DmVector::opt($id,attribute) -width 26 \
                -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # category
    set row [ frame $frm.cat ]
    Label $row.a -text [G_msg "Query vectors: "] 
    LabelEntry $row.b -label [G_msg "layer for query"] \
                -textvariable DmVector::opt($id,field) -width 3 \
                -entrybg white
    LabelEntry $row.c -label [G_msg " query cat values"] \
                -textvariable DmVector::opt($id,cat) \
               -width 5 -entrybg white
    checkbutton $row.d -text [G_msg "SQL query"] -variable DmVector::opt($id,_use_where) \
                -command "DmVector::legend $id"
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # sql where
    set row [ frame $frm.where ]
    LabelEntry $row.a -label [G_msg "     SQL where statement"] \
                -textvariable DmVector::opt($id,where) \
               -width 44 -entrybg white
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
    
	#show columns and data
	set row [ frame $frm.columns ]
    Label $row.a -text [G_msg "    show attribute columns"] 
    Button $row.b -text [G_msg "columns"] \
            -image [image create photo -file "$dmpath/columns.gif"] \
            -command "DmVector::show_columns $id" \
            -background $bgcolor \
            -helptext [G_msg "Show columns"]
    Label $row.c -text [G_msg "     show data"] 
    Button $row.d -text [G_msg "data"] \
            -image [image create photo -file "$dmpath/columns.gif"] \
            -command "DmVector::show_data $id" \
            -background $bgcolor \
            -helptext [G_msg "Show data"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes


    # mouse query setup
    set row [ frame $frm.query ]
    Label $row.a -text [G_msg "Mouse query setup:"]
    checkbutton $row.b -text [G_msg "edit attributes (form mode)"] \
                -variable DmVector::opt($id,_query_edit) 
    checkbutton $row.c -text [G_msg "results as text in terminal"] \
                -variable DmVector::opt($id,_query_text) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # display only in limited region size range
    set row [ frame $frm.region ]
    Label $row.a -text [G_msg "Display when avg. region dimension is"]
    LabelEntry $row.b -label ">" -textvariable DmVector::opt($id,minreg) \
                -width 8 -entrybg white
    LabelEntry $row.c -label " or <" -textvariable DmVector::opt($id,maxreg) \
                -width 8 -entrybg white
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # Width
    set row [ frame $frm.print ]
    Label $row.a -text [G_msg "Line width for ps.map print output:"] 
    SpinBox $row.b -range {1 100 1} -textvariable DmVector::opt($id,_width) \
                   -width 2 -helptext [G_msg "Line width used for printing"] \
                   -entrybg white 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

proc DmVector::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]


    foreach key { _check map display_shape display_cat display_topo display_dir display_attr
                  type_point type_line type_boundary type_centroid type_area type_face
                  color _use_color fcolor _use_fcolor lcolor rdmcolor sqlcolor icon size lwidth field lfield attribute
                  xref yref lsize cat where _query_text _query_edit _use_where minreg maxreg _width } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}


proc DmVector::display { node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    if { !$opt($id,display_shape) && !$opt($id,display_cat) &&
         !$opt($id,display_topo)  && !$opt($id,display_dir) &&
         !$opt($id,display_attr) } { return } 

    if { !$opt($id,type_point) && !$opt($id,type_line) &&
         !$opt($id,type_boundary)  && !$opt($id,type_centroid) && 
         !$opt($id,type_area) && !$opt($id,type_face) } { return } 

    set cmd "d.vect map=$opt($id,map)"

    # color
    if { $opt($id,rdmcolor) } { append cmd " -c" }
    if { $opt($id,sqlcolor) } { append cmd " -a" }
    set color [Dm::color $opt($id,color)]
    set fcolor [Dm::color $opt($id,fcolor)]
    set lcolor [Dm::color $opt($id,lcolor)]

    if { $opt($id,_use_color) } { append cmd " color=$color" } { append cmd " color=none" }
    append cmd " lcolor=$lcolor" 

    if { $opt($id,_use_fcolor) } { append cmd " fcolor=$fcolor" } { append cmd " fcolor=none" }

    # display
    set dlist [list]
    foreach d { shape cat topo dir } {
       if { $opt($id,display_$d) } { lappend dlist $d }
    }
    if { $opt($id,display_attr) && $opt($id,attribute) != "" } { lappend dlist attr }
    
    set display [join $dlist , ]
    append cmd " display=$display"

    # type
    set tlist [list]
    foreach t { point line boundary centroid area face } {
       if { $opt($id,type_$t) } { lappend tlist $t }
    }
    set type [join $tlist , ]
    append cmd " type=$type"

    append cmd " icon=$opt($id,icon) size=$opt($id,size)" 

    if { $opt($id,lwidth) != 1 } { 
        append cmd " width=$opt($id,lwidth)" 
    } 


    if { $opt($id,field) != "" } { 
        append cmd " layer=$opt($id,field)" 
    } 
    if { $opt($id,attribute) != "" && $opt($id,display_attr) } { 
        append cmd " {att=$opt($id,attribute)}" 
    } 
    append cmd " lsize=$opt($id,lsize)" 
    
    append cmd " xref=$opt($id,xref) yref=$opt($id,yref)"

    if { $opt($id,lfield) != "" } { 
        append cmd " llayer=$opt($id,lfield)" 
    } 
    if { $opt($id,cat) != "" } { 
        append cmd " cat=$opt($id,cat)" 
    } 
    if { $opt($id,where) != "" && $opt($id,_use_where) } { 
        append cmd " {where=$opt($id,where)}" 
    } 
    if { $opt($id,minreg) != "" } { 
        append cmd " minreg=$opt($id,minreg)" 
    } 
    if { $opt($id,maxreg) != "" } { 
        append cmd " maxreg=$opt($id,maxreg)" 
    } 

    run_panel $cmd
}

proc DmVector::print { file node } {
    variable opt

    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! $opt($id,_check) } { return } 
    if { $opt($id,map) == "" } { return } 

    if { $opt($id,display_cat) || $opt($id,display_topo) || 
         $opt($id,display_dir) || $opt($id,display_attr) 
    } { puts "At least one of selected display options for vector is not supported for PS"  }

    if { ! $opt($id,display_shape) } { return } 

    set color [Dm::color $opt($id,color)]
    set fcolor [Dm::color $opt($id,fcolor)]

    # Points
    if { $opt($id,type_point) || $opt($id,type_centroid) } {
        puts $file "vpoints $opt($id,map)"

        set str "  type"
        if { $opt($id,type_point) } { append str " point" }
        if { $opt($id,type_centroid) } { append str " centroid" }
        puts $file $str

	if { $opt($id,field) != "" } { puts $file "  layer $opt($id,field)" }
	if { $opt($id,cat) != "" }   { puts $file "  cats $opt($id,cat)" }
	if { $opt($id,where) != "" } { puts $file "  where $opt($id,where)" } 

	    if { $opt($id,_use_color) } { 
	    puts $file "  color $color"
        } else { 
	    puts $file "  color none"
        }

	#puts $file "width $opt($id,ps_width)"

        if { $opt($id,_use_fcolor) } { 
	    puts $file "  fcolor $fcolor"
        } else { 
	    puts $file "  fcolor none"
        }

        puts $file "  symbol $opt($id,icon)"
        puts $file "  size $opt($id,size)"

       # confuses ps.map:
       # puts $file "  xref $opt($id,xref)"
       # puts $file "  yref $opt($id,yref)"

	puts $file "end"
    } 

    # Lines
    if { $opt($id,type_line) || $opt($id,type_boundary) } {
        puts $file "vlines $opt($id,map)"

        set str "  type"
        if { $opt($id,type_line) } { append str " line" }
        if { $opt($id,type_boundary) } { append str " boundary" }
        puts $file $str

	if { $opt($id,field) != "" } { puts $file "  layer $opt($id,field)" }
	if { $opt($id,cat) != "" }   { puts $file "  cats $opt($id,cat)" }
	if { $opt($id,where) != "" } { puts $file "  where $opt($id,where)" } 

        if { $opt($id,_use_color) } { 
	    puts $file "  color $color"
        } else { 
	    puts $file "  color none"
        }

	puts $file "width $opt($id,_width)"

	puts $file "  hcolor NONE"

	puts $file "end"
    } 

    # Areas
    if { $opt($id,type_area) } {
        puts $file "vareas $opt($id,map)"

	if { $opt($id,field) != "" } { puts $file "  layer $opt($id,field)" }
	if { $opt($id,cat) != "" }   { puts $file "  cats $opt($id,cat)" }
	if { $opt($id,where) != "" } { puts $file "  where $opt($id,where)" } 

        if { $opt($id,_use_color) } { 
	    puts $file "  color $color"
        } else { 
	    puts $file "  color none"
        }

	puts $file "width $opt($id,_width)"

        if { $opt($id,_use_fcolor) } { 
	    puts $file "  fcolor $fcolor"
        } else { 
	    puts $file "  fcolor none"
        }

	puts $file "end"
    } 
}

proc DmVector::query { node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    if { !$opt($id,display_shape) && !$opt($id,display_cat) &&
         !$opt($id,display_topo)  && !$opt($id,display_dir) &&
         !$opt($id,display_attr) } { return } 

    if { !$opt($id,type_point) && !$opt($id,type_line) &&
         !$opt($id,type_boundary)  && !$opt($id,type_centroid) && 
         !$opt($id,type_area) && !$opt($id,type_face) } { return } 

    set cmd "d.what.vect -f map=$opt($id,map)"
    if { $opt($id,_query_text) && !$opt($id,_query_edit) } { 
        append cmd " -x" 
    } 
    if { $opt($id,_query_edit) } { 
        append cmd " -e" 
    } 

    if { $opt($id,_query_text) && !$opt($id,_query_edit) } {
        term $cmd
    } else {
        spawn $cmd
    }
}

proc DmVector::WorkOnVector { node } {
    variable opt
    variable bg
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    if { !$opt($id,display_shape) && !$opt($id,display_cat) &&
         !$opt($id,display_topo)  && !$opt($id,display_dir) &&
         !$opt($id,display_attr) } { return } 

    if { !$opt($id,type_point) && !$opt($id,type_line) &&
         !$opt($id,type_boundary)  && !$opt($id,type_centroid) && 
         !$opt($id,type_area) && !$opt($id,type_face) } { return } 
    
    set bg [exec d.save -o | cut -f1 -d# | tr {\n} {;}]
    set bg "$bg"
    Dm::monitor
    spawn v.digit -n map=$opt($id,map) bgcmd=$bg
#    spawn $cmd
}

proc DmVector::duplicate { tree parent node id } {
    global form_mode
    variable opt
    variable count

    set node "vector:$count"

    set frm [ frame .vectoricon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmVector::opt($count,_check) \
                           -height 1 -padx 0 -width 0]
    set can [ canvas $frm.c -width $DmTree::legend_width \
                     -height $DmTree::legend_height -borderwidth 0 ]
    set opt($count,_legend) $can
    pack $check $can -side left

	if { $opt($id,map) == ""} {
    	$tree insert end $parent $node \
		-text      "vector $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert end $parent $node \
		-text      "$opt($id,map)" \
		-window    $frm \
		-drawcross auto
	} 

    set opt($count,_check) $opt($id,_check)

    set opt($count,map) "$opt($id,map)" 
    set opt($count,display_shape) $opt($id,display_shape)
    set opt($count,display_cat) $opt($id,display_cat)
    set opt($count,display_topo) $opt($id,display_topo)
    set opt($count,display_dir) $opt($id,display_dir)
    set opt($count,display_attr) $opt($id,display_attr)
    set opt($count,type_point) $opt($id,type_point)
    set opt($count,type_line) $opt($id,type_line)
    set opt($count,type_boundary) $opt($id,type_boundary)
    set opt($count,type_centroid) $opt($id,type_centroid)
    set opt($count,type_area) $opt($id,type_area)
    set opt($count,type_face)  $opt($id,type_face)

    set opt($count,color) $opt($id,color)
    set opt($count,sqlcolor) $opt($id,sqlcolor)
    set opt($count,rdmcolor) $opt($id,rdmcolor)
    set opt($count,fcolor) $opt($id,fcolor)
    set opt($count,lcolor) $opt($id,lcolor)
    set opt($count,_use_color) $opt($id,_use_color)
    set opt($count,_use_fcolor) $opt($id,_use_fcolor)

    set opt($count,symdir) "$opt($id,symdir)"
    set opt($count,icon) "$opt($id,icon)"
    set opt($count,size)  $opt($id,size)
    set opt($count,lwidth)  $opt($id,lwidth)

    set opt($count,field) $opt($id,field)
    set opt($count,lfield) $opt($id,lfield)
    set opt($count,cat) "$opt($id,cat)"
    set opt($count,where)  "$opt($id,where)"
    set opt($count,_use_where) $opt($id,_use_where)

    set opt($count,attribute) "$opt($id,attribute)"
    set opt($count,xref) "$opt($id,xref)"
    set opt($count,yref) "$opt($id,yref)"
    set opt($count,lsize) $opt($id,lsize)

    set opt($count,minreg) "$opt($id,minreg)" 
    set opt($count,maxreg)  "$opt($id,maxreg)"

    # Default form mode used for vectors, it can be 'gui' (default) or 'txt'
    set form_mode [exec g.gisenv get=DM_FORM_MODE]
    if { $form_mode == "txt" } {
        set opt($count,_query_text) $opt($id,_query_text)
    } else {
        set opt($count,_query_text) $opt($id,_query_text)
    }
    set opt($count,_query_edit) $opt($id,_query_edit)

    set opt($count,_width) $opt($id,_width)

    DmVector::legend $count

    incr count
    return $node
}
