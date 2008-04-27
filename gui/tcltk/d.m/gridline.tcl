# 30 August 2005
# panel for grid and line overlays
# Michael Barton, Arizona State University

namespace eval DmGridline {
    variable array opt # grid and line options
    variable count 1
}


proc DmGridline::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "gridline:$count"

    set frm [ frame .gridicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmGridline::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo gico -file "$dmpath/grid.gif"
    set gdico [label $frm.gdico -image gico -bd 1 -relief raised]
    
    pack $check $gdico -side left
    
    $tree insert end $parent $node \
	-text  "gridline $count"\
	-window    $frm \
	-drawcross auto  

    
    set opt($count,_check) 1 
    
    set opt($count,gridline) "gridline" 
    set opt($count,gridcolor) \#AAAAAA
    set opt($count,gridborder) \#000000 
    set opt($count,gridsize) 100 
    set opt($count,gridorigin) "0,0" 
    set opt($count,griddraw) 1 
    set opt($count,gridgeod) 0
    set opt($count,borderdraw) 1 
    set opt($count,textdraw) 1 
    
    set opt($count,rhumbdraw) 0 
    set opt($count,rhumbcoor) "" 
    set opt($count,rhumbcolor) "black" 
    
    set opt($count,geoddraw) 0 
    set opt($count,geodcoor) "" 
    set opt($count,geodcolor) "black" 
    set opt($count,geodtxtcolor) "none" 
    
    incr count
    return $node
}

proc DmGridline::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}


# display gridline options
proc DmGridline::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # grid options 1
    set row [ frame $frm.grid1 ]
    Label $row.a -text "Grid options: "
    checkbutton $row.b -text [G_msg "draw grid"] -variable DmGridline::opt($id,griddraw) 
    checkbutton $row.c -text [G_msg "geodetic grid"] -variable DmGridline::opt($id,gridgeod) 
    SelectColor $row.d -type menubutton -variable DmGridline::opt($id,gridcolor)    
    Label $row.e -text [G_msg " grid color "] 
    Button $row.f -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.grid" \
            -background $bgcolor \
            -helptext [G_msg "Help for grids"]
    Label $row. -text [G_msg " grid color"] 
    pack $row.a $row.b $row.c $row.d $row.e $row.f  -side left
    pack $row -side top -fill both -expand yes

    # grid options 2
    set row [ frame $frm.grid2 ]
    Label $row.a -text [G_msg "     "]
    checkbutton $row.b -text [G_msg "draw grid border"] -variable DmGridline::opt($id,borderdraw) 
    checkbutton $row.c -text [G_msg "draw border text"] -variable DmGridline::opt($id,textdraw) 
    Label $row.d -text [G_msg " border & text color"] 
    SelectColor $row.e -type menubutton -variable DmGridline::opt($id,gridborder)
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

    # grid options 3
    set row [ frame $frm.grid3 ]
    Label $row.a -text "      grid size"
    LabelEntry $row.b -textvariable DmGridline::opt($id,gridsize) -width 10 \
            -entrybg white
    Label $row.c -text " grid origin (x,y)"
    LabelEntry $row.d -textvariable DmGridline::opt($id,gridorigin) -width 29 \
            -entrybg white
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
    
    set row [ frame $frm.line ]
    Label $row.a -text "Geodesic and rhumblines for latlong locations only"
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # geodesic line options 1
    set row [ frame $frm.geod1 ]
    Label $row.a -text "     "
    checkbutton $row.b -text [G_msg "draw geodesic line"] -variable DmGridline::opt($id,geoddraw)
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.geodesic" \
            -background $bgcolor \
            -helptext [G_msg "Help for geodesic lines"]
    Label $row.d -text " line color"
    ComboBox $row.e -padx 2 -width 10 -textvariable DmGridline::opt($id,geodcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"} \
                    -entrybg white
    Label $row.f -text " text color"
    ComboBox $row.g -padx 2 -width 10 -textvariable DmGridline::opt($id,geodtxtcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"} \
                    -entrybg white
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes
    
    # geodesic line options 2
    set row [ frame $frm.geod2 ]
    Label $row.a -text "     line endpoints (x1,y1,x2,y2)"
    LabelEntry $row.b -textvariable DmGridline::opt($id,geodcoor) -width 39 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # rhumbline options 1
    set row [ frame $frm.rhumb1 ]
    Label $row.a -text "     "
    checkbutton $row.b -text [G_msg "draw rhumbline"] -variable DmGridline::opt($id,rhumbdraw)
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.rhumbline" \
            -background $bgcolor \
            -helptext [G_msg "Help for rhumblines"]
    Label $row.d -text " line color"
    ComboBox $row.e -padx 2 -width 10 -textvariable DmGridline::opt($id,rhumbcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"} \
                    -entrybg white
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes
    
    # rhumbline options 2
    set row [ frame $frm.rhumb2 ]
    Label $row.a -text "     line endpoints (x1,y1,x2,y2)"
    LabelEntry $row.b -textvariable DmGridline::opt($id,rhumbcoor) -width 39 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

proc DmGridline::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]


    foreach key { _check gridcolor gridborder gridsize gridorigin griddraw gridgeod \
    			borderdraw textdraw rhumbdraw rhumbcoor geoddraw geodcoor geodcolor \
    			geodtxtcolor} {
        Dm::rc_write $depth "$key $opt($id,$key)"     

    }                     
}

proc DmGridline::display { node } {
    variable opt
    global dmpath

    set tree $Dm::tree
    set id [Dm::node_id $node]
    set cmd ""
    set cmd2 ""
    set cmd3 ""

    if { ! ( $opt($id,_check) ) } { return } 
    
    # set hex colors to rgb         
    set gridcolor [Dm::color $opt($id,gridcolor)]
    set gridborder [Dm::color $opt($id,gridborder)]

    
    # d.grid command
    if { $opt($id,griddraw) || $opt($id,borderdraw) } {
            set cmd "d.grid size=$opt($id,gridsize) origin=$opt($id,gridorigin) \
                color=$gridcolor bordercolor=$gridborder" \
        } 
        
    if { $opt($id,gridgeod) && $cmd != "" } {append cmd " -g"} 
    if { !$opt($id,griddraw) && $cmd != "" } {append cmd " -n"} 
    if { !$opt($id,borderdraw) && $cmd != "" } {append cmd " -b"}
    if { !$opt($id,textdraw) && $cmd != "" } {append cmd " -t"}

        
    # d.geodesic command
    if { $opt($id,geoddraw) } {
        set cmd2 "d.geodesic coor=$opt($id,geodcoor) \
                lcolor=$opt($id,geodcolor) \
                tcolor=$opt($id,geodtxtcolor)"  }

    # d.rhumbline command
    if { $opt($id,rhumbdraw) } {
        set cmd3 "d.rhumbline coor=$opt($id,rhumbcoor) \
       lcolor=$opt($id,rhumbcolor) " }

    if { $cmd != "" } { run_panel $cmd } 

    if { $cmd2 != "" } { run_panel $cmd2 } 

    if { $cmd3 != "" } { run_panel $cmd3 }     
}

proc DmGridline::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,gridline) == "" } { return } 

    puts $file "gridline $opt($id,gridline)"
}


proc DmGridline::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "gridline:$count"

    set frm [ frame .gridlineicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmGridline::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo gico -file "$dmpath/grid.gif"
    set gdico [label $frm.gdico -image gico -bd 1 -relief raised]
    
    pack $check $gdico -side left

	if { $opt($id,gridline) == ""} {
    	$tree insert end $parent $node \
		-text      "gridline $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert end $parent $node \
		-text      "$opt($id,gridline)" \
		-window    $frm \
		-drawcross auto
	}

    set opt($count,_check) $opt($id,_check)

    set opt($count,gridcolor) "$opt($id,gridcolor)" 
    set opt($count,gridborder) $opt($id,gridborder)
    set opt($count,gridorigin)  $opt($id,gridorigin)
    set opt($count,griddraw)  $opt($id,griddraw)
    set opt($count,gridgeod)  $opt($id,gridgeod)
    set opt($count,gridsize)  $opt($id,gridsize) 
    set opt($count,borderdraw)  $opt($id,borderdraw)
    set opt($count,rhumbdraw)  $opt($id,rhumbdraw)
    set opt($count,rhumbcoor)  $opt($id,rhumbcoor)
    set opt($count,rhumbcolor) $opt($id,rhumbcolor)
    set opt($count,geoddraw)  $opt($id,geoddraw)
    set opt($count,geodcoor)  $opt($id,geodcoor)
    set opt($count,geodtxtcolor)  $opt($id,geodtxtcolor)
    set opt($count,geodcolor)  $opt($id,geodcolor)
    
    set opt($count,rhumbdraw) 0 
    set opt($count,rhumbcoor) "" 


    incr count
    return $node
}