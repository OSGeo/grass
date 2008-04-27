# 4 Sept 2005
# panel for d.vect.chart
# Michael Barton, Arizona State University

namespace eval DmChart {
    variable array opt # chart options
    variable count 1
}


proc DmChart::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "chart:$count"

    set frm [ frame .charticon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmChart::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo chartico -file "$dmpath/chart.gif"
    set ico [label $frm.ico -image chartico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "chart $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,_check) 1 
    
    set opt($count,map) "" 
    set opt($count,type_point) 1 
    set opt($count,type_line) 1
    set opt($count,type_boundary) 1
    set opt($count,type_centroid) 1
    set opt($count,type_area) 0
    set opt($count,layer) 1 
    set opt($count,ctype) "pie" 
    set opt($count,columns) "" 
    set opt($count,sizecol) "" 
    set opt($count,csize) 40 
    set opt($count,cscale) 1 
    set opt($count,ocolor) "black" 
    set opt($count,fcolors) "" 
    
    incr count
    return $node
}

proc DmChart::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}

proc DmChart::select_map { id } {
    variable tree
    variable node
    set m [GSelect vector]
    if { $m != "" } { 
        set DmChart::opt($id,map) $m
        Dm::autoname "chart for $m"
    }
}

proc DmChart::show_columns { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,map)
	set layernum $opt($id,layer)
	exec $env(GISBASE)/etc/grass-xterm-wrapper -bg $bgcolor -title "$mapname columns" \
		-geometry 40x25-10+30 -sb -hold -e v.info -c map=$mapname \
		layer=$layernum &		
}


# chart options
proc DmChart::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # vector name
    set row [ frame $frm.map ]
    Button $row.a -text [G_msg "Vector map to chart:"] \
           -command "DmChart::select_map $id"
    Entry $row.b -width 40 -text " $opt($id,map)" \
          -textvariable DmChart::opt($id,map) \
          -background white
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.vect.chart" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # vector type
    set row [ frame $frm.type ]
    Label $row.a -text [G_msg "Vector type:"]
    checkbutton $row.b -text [G_msg "points"] -variable DmChart::opt($id,type_point)
    checkbutton $row.c -text [G_msg "lines"] -variable DmChart::opt($id,type_line)
    checkbutton $row.d -text [G_msg "boundaries"] -variable DmChart::opt($id,type_boundary)
    checkbutton $row.e -text [G_msg "centroids"] -variable DmChart::opt($id,type_centroid)
    checkbutton $row.f -text [G_msg "areas"] -variable DmChart::opt($id,type_area)
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left
    pack $row -side top -fill both -expand yes

    # attributes1
    set row [ frame $frm.attr1 ]
    Label $row.a -text "Attribute to chart: layer"
    LabelEntry $row.b -textvariable DmChart::opt($id,layer) -width 5 \
            -entrybg white
    Label $row.c -text [G_msg "  show attribute columns"] 
    Button $row.d -text [G_msg "columns"] \
            -image [image create photo -file "$dmpath/columns.gif"] \
            -command "DmChart::show_columns $id" \
            -background $bgcolor \
            -helptext [G_msg "Show columns"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # attributes2
    set row [ frame $frm.attr2 ]
    Label $row.a -text "   columns to chart (col1,col2,...)  "
    LabelEntry $row.b -textvariable DmChart::opt($id,columns) -width 35 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # attributes3
    set row [ frame $frm.attr3 ]
    Label $row.a -text "   colors for columns (clr1,clr2,...)"
    LabelEntry $row.b -textvariable DmChart::opt($id,fcolors) -width 35 \
            -entrybg white -padx 2
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # attributes4
    set row [ frame $frm.attr4 ]
    Label $row.a -text "   column for variable chart size"
    LabelEntry $row.b -textvariable DmChart::opt($id,sizecol) -width 12 \
            -entrybg white -padx 9
    Label $row.c -text " scale factor"
    LabelEntry $row.d -textvariable DmChart::opt($id,cscale) -width 4 \
            -entrybg white
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # chart options1
    set row [ frame $frm.chopt1 ]
    Label $row.a -text [G_msg "Chart type:"] 
    ComboBox $row.b -padx 2 -width 4 -textvariable DmChart::opt($id,ctype) \
                    -values {"pie" "bar"} -entrybg white
    Label $row.c -text " fixed chart size (if size column not used)"
    LabelEntry $row.d -textvariable DmChart::opt($id,csize) -width 4 \
            -entrybg white
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # chart options2
    set row [ frame $frm.chopt2 ]
    Label $row.a -text [G_msg "    chart outline color:"] 
    ComboBox $row.b -padx 0 -width 10 -textvariable DmChart::opt($id,ocolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"} \
                    -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

proc DmChart::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check map layer ctype columns sizecol csize cscale ocolor fcolors \
             type_point type_line type_boundary type_centroid type_area } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}


proc DmChart::display { node } {
    variable opt
    set line ""
    set input ""
    global dmpath
    set cmd ""

    set tree $Dm::tree
    set id [Dm::node_id $node]


    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 
    if { $opt($id,columns) == "" } { return }
    if { !$opt($id,type_point) && !$opt($id,type_line) &&
         !$opt($id,type_boundary)  && !$opt($id,type_centroid) && 
         !$opt($id,type_area) } { return } 

    # combine vector types         
    set type ""
    if { $opt($id,type_point) } { append type "point" }
    if { $opt($id,type_line) && "$type" != "" } { 
        append type ",line"
    } elseif { $opt($id,type_line) && "$type" == "" } {
        append type "line"}
    if { $opt($id,type_boundary) && "$type" != "" } { 
        append type ",boundary" 
    } elseif { $opt($id,type_boundary) && "$type" == "" } {
        append type "boundary"}
    if { $opt($id,type_centroid) && "$type" != "" } { 
        append type ",centroid" 
    } elseif { $opt($id,type_centroid) && "$type" == "" } {
        append type "centroid"}
    if { $opt($id,type_area) && "$type" != "" } { 
        append type ",area" 
    } elseif { $opt($id,type_area) && "$type" == "" } {
        append type "area"}

    #create d.vect.chart command
    set cmd "d.vect.chart map=$opt($id,map) type=$type \
            layer=$opt($id,layer) columns=$opt($id,columns) \
            ctype=$opt($id,ctype) ocolor=$opt($id,ocolor) "
            
    # sizecol
    if { $opt($id,sizecol) != "" } { 
        append cmd " sizecol=$opt($id,sizecol)"
    }

    # csize
    if { $opt($id,csize) != "" } { 
        append cmd " size=$opt($id,csize)"
    }

    # cscale
    if { $opt($id,cscale) != "" } { 
        append cmd " scale=$opt($id,cscale)"
    }

    # fcolors
    if { $opt($id,fcolors) != "" } { 
        append cmd " colors=$opt($id,fcolors)"
    }

        run_panel $cmd

}

proc DmChart::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    puts $file "chart $opt($id,map)"
}


proc DmChart::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "chart:$count"

    set frm [ frame .charticon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmChart::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo chartico -file "$dmpath/legend.gif"
    set ico [label $frm.ico -image chartico -bd 1 -relief raised]
    
    pack $check $ico -side left

	if { $opt($id,map) == ""} {
    	$tree insert end $parent $node \
		-text      "chart $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert end $parent $node \
		-text      "chart for $opt($id,map)" \
		-window    $frm \
		-drawcross auto
	}

    set opt($count,_check) $opt($id,_check)

    set opt($count,map) "$opt($id,map)" 
    set opt($count,type_point) "$opt($id,type_point)" 
    set opt($count,type_line)  "$opt($id,type_line)"
    set opt($count,type_boundary)  "$opt($id,type_boundary)"
    set opt($count,type_centroid)  "$opt($id,type_centroid)"
    set opt($count,type_area)  "$opt($id,type_area)"
    set opt($count,layer)  "$opt($id,layer)"
    set opt($count,ctype)  "$opt($id,ctype)" 
    set opt($count,columns)  "$opt($id,columns)" 
    set opt($count,sizecol)  "$opt($id,sizecol)" 
    set opt($count,csize)  "$opt($id,csize)" 
    set opt($count,cscale)  "$opt($id,cscale)"
    set opt($count,color) "$opt($id,oolor)" 
    set opt($count,color) "$opt($id,folors)" 

    incr count
    return $node
}
