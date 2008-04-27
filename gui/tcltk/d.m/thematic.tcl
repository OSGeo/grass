# 4 Sept 2005
# panel for d.vect.thematic
# Michael Barton, Arizona State University

namespace eval DmThematic {
    variable array opt # thematic options
    variable count 1
}


proc DmThematic::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "thematic:$count"

    set frm [ frame .thematicicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmThematic::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo thematicico -file "$dmpath/thematic.gif"
    set ico [label $frm.ico -image thematicico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "thematic $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,_check) 1 
    
    set opt($count,map) "" 
    set opt($count,type) "area"
    set opt($count,column) "" 
    set opt($count,themetype) "graduated_colors" 
    set opt($count,themecalc) "interval" 
    set opt($count,breakpoints) "" 
    set opt($count,where) "" 
    set opt($count,layer) 1 
    set opt($count,icon) "basic/circle" 
    set opt($count,iconsize) 5 
    set opt($count,maxsize) 20 
    set opt($count,nint) 4 
    set opt($count,colorscheme) "blue-red" 
    set opt($count,pointcolor) \#FF0000 
    set opt($count,linecolor) \#000000 
    set opt($count,startcolor) \#FF0000 
    set opt($count,endcolor) \#0000FF 
    set opt($count,legmon) "x1" 
    set opt($count,thmlegend) 1 
    set opt($count,update_rgb) 0 
    set opt($count,math) 0 
    set opt($count,psmap) "" 
    set opt($count,border) 1 
    
    incr count
    return $node
}

proc DmThematic::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}

proc DmThematic::select_map { id } {
    variable tree
    variable node
    set m [GSelect vector]
    if { $m != "" } { 
        set DmThematic::opt($id,map) $m
        Dm::autoname "thematic map for $m"
    }
}

proc DmThematic::show_columns { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,map)
	set layernum $opt($id,layer)
	exec $env(GISBASE)/etc/grass-xterm-wrapper -bg $bgcolor -title "$mapname columns" \
		-geometry 40x25-10+30 -sb -hold -e v.info -c map=$mapname \
		layer=$layernum &		
}

proc DmThematic::show_data { id } {
	variable opt
	global bgcolor
	set mapname $opt($id,map)
	exec $env(GISBASE)/etc/grass-xterm-wrapper -bg $bgcolor -title "$mapname data" \
		-geometry 60x40-10+30 -sb -hold -e db.select table=$mapname &
}

# select symbols from directories
proc DmThematic::select_symbol { id } {
    variable opt
    set i [GSelect symbol]
    if { $i != "" } {
        set DmThematic::opt($id,icon) $i
    }
}

# thematic options
proc DmThematic::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # vector name
    set row [ frame $frm.map ]
    Button $row.a -text [G_msg "Vector map:"] \
           -command "DmThematic::select_map $id"
    Entry $row.b -width 40 -text " $opt($id,map)" \
          -textvariable DmThematic::opt($id,map) \
          -background white
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.vect.thematic" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # vector type and layer
    set row [ frame $frm.vtype ]
    Label $row.a -text [G_msg "    vector type"] 
    ComboBox $row.b -padx 2 -width 10 -textvariable DmThematic::opt($id,type) \
                    -values {"area" "point" "centroid" "line" "boundary"} -entrybg white
    Label $row.c -text " attribute layer"
    LabelEntry $row.d -textvariable DmThematic::opt($id,layer) -width 3 \
            -entrybg white
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # vector column
    set row [ frame $frm.column ]
    Label $row.a -text "    NUMERIC attribute column to use for thematic map"
    LabelEntry $row.b -textvariable DmThematic::opt($id,column) -width 15 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
	#show columns and data
	set row [ frame $frm.columns ]
    Label $row.a -text [G_msg "    show attribute columns"] 
    Button $row.b -text [G_msg "columns"] \
            -image [image create photo -file "$dmpath/columns.gif"] \
            -command "DmThematic::show_columns $id" \
            -background $bgcolor \
            -helptext [G_msg "Show columns"]
    Label $row.c -text [G_msg "     show data"] 
    Button $row.d -text [G_msg "data"] \
            -image [image create photo -file "$dmpath/columns.gif"] \
            -command "DmThematic::show_data $id" \
            -background $bgcolor \
            -helptext [G_msg "Show data"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    

    # Thematic type
    set row [ frame $frm.ttype ]
    Label $row.a -text [G_msg "Thematic map: type"] 
    ComboBox $row.b -padx 2 -width 16 -textvariable DmThematic::opt($id,themetype) \
                    -values {"graduated_colors" "graduated_points" "graduated_lines"} -entrybg white
    Label $row.c -text [G_msg " map by"] 
    ComboBox $row.d -padx 2 -width 15 -textvariable DmThematic::opt($id,themecalc) \
                    -values {"interval" "std_deviation" "quartiles" \
                    "custom_breaks"} -entrybg white
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # intervals
    set row [ frame $frm.int ]
    Label $row.a -text "    number of intervals to map (interval themes):" 
    SpinBox $row.b -range {1 99 1} -textvariable DmThematic::opt($id,nint) \
                   -entrybg white -width 3 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # breakpoints
    set row [ frame $frm.break ]
    Label $row.a -text "    custom breakpoints (val val ...)"
    LabelEntry $row.b -textvariable DmThematic::opt($id,breakpoints) -width 32 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # where
    set row [ frame $frm.where ]
    Label $row.a -text "    query with SQL where clause"
    LabelEntry $row.b -textvariable DmThematic::opt($id,where) -width 34 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # point options1
    set row [ frame $frm.pts1 ]  
    Label $row.a -text "Graduated points & lines: " 
    Button $row.b -text [G_msg "icon"] \
	    -command "DmThematic::select_symbol $id"
    Entry $row.c -width 10 -text "$opt($id,icon)" \
	    -textvariable DmThematic::opt($id,icon) \
	    -background white 
    Label $row.d -text [G_msg "point color"] 
    SelectColor $row.e -type menubutton -variable DmThematic::opt($id,pointcolor)
    Label $row.f -text [G_msg "line color"] 
    SelectColor $row.g -type menubutton -variable DmThematic::opt($id,linecolor)
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # point options2
    set row [ frame $frm.pts2 ]  
    Label $row.a -text "    size/min size (graduated pts/lines)" 
    SpinBox $row.b -range {1 50 1} -textvariable DmThematic::opt($id,iconsize) \
        -width 2 -helptext "icon size/min size (graduated pts/lines)" -entrybg white 
    Label $row.c -text "max size (graduated pts)" 
    SpinBox $row.d -range {1 50 1} -textvariable DmThematic::opt($id,maxsize) \
        -width 2 -helptext " max size (graduated pts/lines)" -entrybg white 
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # color options1
    set row [ frame $frm.color1 ]
    Label $row.a -text [G_msg "Graduate colors: preset color schemes"] 
    ComboBox $row.b -padx 2 -width 18 -textvariable DmThematic::opt($id,colorscheme) \
        -values {"blue-red" "red-blue" "green-red" "red-green" \
        "blue-green" "custom_gradient" "single_color" } -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # color options2
    set row [ frame $frm.color2 ]
    Label $row.a -text "    custom color scheme - start color"
    SelectColor $row.b -type menubutton -variable DmThematic::opt($id,startcolor)
    Label $row.c -text " end color"
    SelectColor $row.d -type menubutton -variable DmThematic::opt($id,endcolor)
    checkbutton $row.e -text [G_msg "draw border"] -variable DmThematic::opt($id,border)     
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
    # color options3
    set row [ frame $frm.color3 ]
    Label $row.a -text "   "
    checkbutton $row.b -text [G_msg "save thematic colors to GRASSRGB column of vector file"] -variable \
        DmThematic::opt($id,update_rgb) 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # legend1
    set row [ frame $frm.legend1 ]
    Label $row.a -text "Legend: "
    checkbutton $row.b -text [G_msg "create graphic legend"] -variable \
        DmThematic::opt($id,thmlegend) 
    checkbutton $row.c -text [G_msg "use math notation in legend"] -variable \
        DmThematic::opt($id,math) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # legend2
    set row [ frame $frm.legend2 ]
    Label $row.a -text [G_msg "    select monitor for legend"] 
    ComboBox $row.b -padx 2 -width 3 -textvariable DmThematic::opt($id,legmon) \
        -values {"x0" "x1" "x2" "x3" "x4" "x5" "x6"} -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # psmap
    set row [ frame $frm.psmap ]
    Label $row.a -text "Name for ps.map instruction files"
    LabelEntry $row.b -textvariable DmThematic::opt($id,psmap) -width 34 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

}

proc DmThematic::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check map type column themetype themecalc breakpoints where \
             layer icon iconsize maxsize nint colorscheme pointcolor linecolor\
             startcolor endcolor border legmon thmlegend update_rgb math psmap} {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}


proc DmThematic::display { node } {
    variable opt
    set line ""
    set input ""
    global dmpath
    set cmd ""

    set tree $Dm::tree
    set id [Dm::node_id $node]


    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 
    if { $opt($id,column) == "" } { return }

    # set hex colors to rgb         
    set pointcolor [Dm::color $opt($id,pointcolor)]
    set linecolor [Dm::color $opt($id,linecolor)]
    set startcolor [Dm::color $opt($id,startcolor)]
    set endcolor [Dm::color $opt($id,endcolor)]

    #create d.vect.thematic command
    set cmd "d.vect.thematic map=$opt($id,map) type=$opt($id,type) column=$opt($id,column) \
			layer=$opt($id,layer) icon=$opt($id,icon) size=$opt($id,iconsize) \
            maxsize=$opt($id,maxsize) nint=$opt($id,nint) pointcolor=$pointcolor \
			linecolor=$linecolor startcolor=$startcolor endcolor=$endcolor \
			monitor=$opt($id,legmon) themetype=$opt($id,themetype) \
			themecalc=$opt($id,themecalc) colorscheme=$opt($id,colorscheme)"
             
    # breakpoints
    if { $opt($id,breakpoints) != "" } { 
        append cmd " {breakpoints=$opt($id,breakpoints)}"
    }

    # where query
    if { $opt($id,where) != "" } { 
        append cmd " {where=$opt($id,where)}"
    }

    # psmap file 
    if { $opt($id,psmap) != "" } { 
        append cmd " psmap=$opt($id,psmap)"
    }

    # hide border
    if { $opt($id,border) == 0 } { 
        append cmd "  -f"
    }

    # update_rgb
    if { $opt($id,update_rgb) == 1 } { 
        append cmd " -u"
    }

    # legend
    if { $opt($id,thmlegend) == 1 } { 
        append cmd " -l"
    }

    # math notation
    if { $opt($id,math) == 1 } { 
        append cmd " -m"
    }
    
        run_panel $cmd

}

proc DmThematic::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    puts $file "thematic $opt($id,map)"
}


proc DmThematic::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "thematic:$count"

    set frm [ frame .thematicicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmThematic::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo thematicico -file "$dmpath/thematic.gif"
    set ico [label $frm.ico -image thematicico -bd 1 -relief raised]
    
    pack $check $ico -side left

	if { $opt($id,map) == ""} {
    	$tree insert end $parent $node \
		-text      "thematic $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert end $parent $node \
		-text      "thematic map for $opt($id,map)" \
		-window    $frm \
		-drawcross auto
	}

    set opt($count,_check) $opt($id,_check)

    set opt($count,map) "$opt($id,map)" 
    set opt($count,type) "$opt($id,type)"
    set opt($count,column) "$opt($id,column)"
    set opt($count,themetype) "$opt($id,themetype)" 
    set opt($count,themecalc) "$opt($id,themecalc)" 
    set opt($count,breakpoints) "$opt($id,breakpoints)" 
    set opt($count,where) "$opt($id,where)" 
    set opt($count,layer) "$opt($id,layer)"
    set opt($count,icon) "$opt($id,icon)" 
    set opt($count,iconsize) "$opt($id,iconsize)"
    set opt($count,maxsize) "$opt($id,maxsize)"
    set opt($count,nint) "$opt($id,nint)"
    set opt($count,colorscheme) "$opt($id,colorscheme)"
    set opt($count,pointcolor) "$opt($id,pointcolor)"
    set opt($count,linecolor) "$opt($id,linecolor)"
    set opt($count,startcolor) "$opt($id,startcolor)"
    set opt($count,endcolor) "$opt($id,endcolor)"
    set opt($count,border) "$opt($id,border)"
    set opt($count,legmon) "$opt($id,legmon)"
    set opt($count,thmlegend) "$opt($id,thmlegend)"
    set opt($count,update_rgb) "$opt($id,update_rgb)" 
    set opt($count,math) "$opt($id,math)" 
    set opt($count,psmap) "$opt($id,psmap)" 

    incr count
    return $node
}
