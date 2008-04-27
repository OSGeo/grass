# 18 March 2005

namespace eval DmRaster {
    variable array opt # raster options
    variable count 1
}


proc DmRaster::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "raster:$count"

    set frm [ frame .rastericon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmRaster::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo rico -file "$dmpath/raster.gif"
    set ico [label $frm.ico -image rico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "raster $count"\
	-window    $frm \
	-drawcross auto  
    
    set opt($count,_check) 1 
    set opt($count,map) "" 
    set opt($count,drapemap) "" 
    set opt($count,querytype) "cat" 
    set opt($count,rastquery) "" 
    set opt($count,rasttype) "" 
    set opt($count,bkcolor) "" 
    set opt($count,overlay) 1 
    set opt($count,legend1) 0 
    set opt($count,legmon1) "x1" 
    set opt($count,legthin1) "1"
    set opt($count,legend2) 0 
    set opt($count,legmon2) "x2" 
    set opt($count,legthin2) "1"
    
    incr count
    return $node
}

proc DmRaster::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}

proc DmRaster::select_map1 { id } {
    variable tree
    variable node
    set m [GSelect cell]
    if { $m != "" } { 
        set DmRaster::opt($id,map) $m
        Dm::autoname $m
    }
}

proc DmRaster::select_map2 { id } {
    variable tree
    variable node
    set m [GSelect cell]
    if { $m != "" } { 
        set DmRaster::opt($id,drapemap) $m
        Dm::autoname $m
    }
}
# display raster options
proc DmRaster::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # raster name
    set row [ frame $frm.name ]
    Button $row.a -text [G_msg "Raster name:"] \
           -command "DmRaster::select_map1 $id"
    Entry $row.b -width 49 -text " $opt($id,map)" \
          -textvariable DmRaster::opt($id,map) \
          -background white
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.rast" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # raster query
    set row [ frame $frm.rquery ]
    Label $row.a -text "Raster values to display"
    LabelEntry $row.b -textvariable DmRaster::opt($id,rastquery) -width 42 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    #raster legend1
    set row [ frame $frm.leg ]
    checkbutton $row.a -text [G_msg "show legend in selected display monitor"] -variable DmRaster::opt($id,legend1) 
    ComboBox $row.b -padx 2 -width 4 -textvariable DmRaster::opt($id,legmon1) \
                    -values {"x0" "x1" "x2" "x3" "x4" "x5" "x6"} -entrybg white
    LabelEntry $row.c -label [G_msg " thin legend by "] -textvariable DmRaster::opt($id,legthin1) \
                    -width 6 -entrybg white
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # drape name
    set row [ frame $frm.drape ]
    Button $row.a -text [G_msg "Raster to drape over 1st map:"] \
           -command "DmRaster::select_map2 $id"
    Entry $row.b -width 35 -text " $opt($id,drapemap)" \
          -textvariable DmRaster::opt($id,drapemap) \
          -background white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    set row [ frame $frm.drapeinfo ]
    Label $row.a -text "     (color over relief map or data fusion--1st map for shading, 2nd for color)"
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
    
    #drape legend2
    set row [ frame $frm.dleg ]
    checkbutton $row.a -text [G_msg "show legend for drape map in monitor"] -variable \
            DmRaster::opt($id,legend2) 
    ComboBox $row.b -padx 2 -width 4 -textvariable DmRaster::opt($id,legmon2) \
            -values {"x0" "x1" "x2" "x3" "x4" "x5" "x6"} -entrybg white
    LabelEntry $row.c -label [G_msg " thin legend by "] -textvariable DmRaster::opt($id,legthin2) \
            -width 6 -entrybg white
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # overlay
    set row [ frame $frm.over ]
    checkbutton $row.a -text [G_msg "overlay maps from other layers (transparent null value cells)"] \
        -variable DmRaster::opt($id,overlay) 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # background color
    set row [ frame $frm.bg ]
    Label $row.a -text " Set background color (colored null value cells)"
    ComboBox $row.b -padx 2 -width 10 -textvariable DmRaster::opt($id,bkcolor) \
                    -values {"white" "grey" "gray" "black" "brown" "red" "orange" \
                    "yellow" "green" "aqua" "cyan" "indigo" "blue" "purple" "violet" "magenta"} \
                    -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

proc DmRaster::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check map drapemap querytype rastquery rasttype bkcolor overlay \
             legend1 legmon1 legthin1 legend2 legmon2 legthin2} {
         Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}

proc DmRaster::display { node } {
    variable opt
    variable rasttype
    set rasttype ""
    set currmon ""
    set line ""
    set input ""
    global dmpath

    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    set cmd "d.rast map=$opt($id,map)"

    # overlay
    if { $opt($id,overlay) } { 
        append cmd " -o"
    }

    # set raster type
     set rt [open "|r.info map=$opt($id,map) -t" r]
     set rasttype [read $rt]
     close $rt
        if {[regexp -nocase ".=CELL" $rasttype]} {
            set querytype "cat"
        } else {
            set querytype "vallist"
        }


    # raster query
    if { $opt($id,rastquery) != "" } { 
        append cmd " {$querytype=$opt($id,rastquery)}"
    }
    
    # background color
    if { $opt($id,bkcolor) != "" } { 
        append cmd " bg=$opt($id,bkcolor)"
    }
    
    set cmd2 "d.his h_map=$opt($id,drapemap) i_map=$opt($id,map)"
    
    if { $opt($id,drapemap) == "" } { 
        run_panel $cmd 
    } else {
        run_panel $cmd2
    }



	#set current monitor for raster display
	if ![catch {open "|d.mon -L" r} input] {
		while {[gets $input line] >= 0} {
			if {[regexp -nocase {.*(selected).*} $line]} {
				regexp -nocase {..} $line currmon
			}              
		}
	}
	close $input

    #display legend1 for raster map
    if { $opt($id,legend1) } { 

        Dm::displmon $opt($id,legmon1)
        
        run_panel "d.erase white"
        run_panel "d.legend map=$opt($id,map) thin=$opt($id,legthin1)"
		run_panel "d.mon select=$currmon"
    }

    #display legend2 for drape map
    if { $opt($id,legend2) } { 
 
        Dm::displmon $opt($id,legmon2)
        run_panel "d.erase white"
        run_panel "d.legend map=$opt($id,drapemap) thin=$opt($id,legthin2)"
		run_panel "d.mon select=$currmon"
    }


}

proc DmRaster::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    puts $file "raster $opt($id,map)"
}

proc DmRaster::query { node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    set cmd "d.what.rast map=$opt($id,map)"
    
    term $cmd

}

proc DmRaster::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "raster:$count"

    set frm [ frame .rastericon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmRaster::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo rico -file "$dmpath/raster.gif"
    set ico [label $frm.ico -image rico -bd 1 -relief raised]
    
    pack $check $ico -side left

	if { $opt($id,map) == ""} {
    	$tree insert end $parent $node \
		-text      "raster $count" \
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
    set opt($count,drapemap) "$opt($id,drapemap)" 
    set opt($count,querytype) "$opt($id,querytype)" 
    set opt($count,rastquery) "$opt($id,rastquery)" 
    set opt($count,rasttype) "$opt($id,rasttype)" 
    set opt($count,bkcolor) "$opt($id,bkcolor)" 
    set opt($count,overlay) "$opt($id,overlay)"
    set opt($count,legend1)  "$opt($id,legend1)"
    set opt($count,legmon1) "$opt($id,legmon1)" 
    set opt($count,legthin1) "$opt($id,legthin1)" 
    set opt($count,legend2)  "$opt($id,legend2)"
    set opt($count,legmon2) "$opt($id,legmon2)" 
    set opt($count,legthin2) "$opt($id,legthin2)" 

    incr count
    return $node
}
