# 4 Sept 2005
# panel for d.barscale
# Michael Barton, Arizona State University

namespace eval DmBarscale {
    variable array opt # barscale options
    variable count 1
}


proc DmBarscale::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "barscale:$count"

    set frm [ frame .barscaleicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmBarscale::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo scaleico -file "$dmpath/barscale.gif"
    set ico [label $frm.ico -image scaleico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "scale $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,_check) 1 

    set opt($count,tcolor) \#000000 
    set opt($count,bcolor) \#FFFFFF 
    set opt($count,bcolor_none) 0
    set opt($count,line) 0 
    set opt($count,at) "2,2" 
    set opt($count,feet) 0 
    set opt($count,top) 0 
    set opt($count,mouse) 0 
    set opt($count,arrow) 0 
    set opt($count,scale) 0 
    
    incr count
    return $node
}

proc DmBarscale::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}


# barscale options
proc DmBarscale::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # color
    set row [ frame $frm.color ]
    Label $row.a -text [G_msg "Text color: "] 
    SelectColor $row.b -type menubutton -variable DmBarscale::opt($id,tcolor)
    Label $row.c -text [G_msg " Background color: "] 
    SelectColor $row.d -type menubutton -variable DmBarscale::opt($id,bcolor)
    checkbutton $row.e -text [G_msg "no background color"] -variable \
        DmBarscale::opt($id,bcolor_none) 
    Label $row.f -text [G_msg " "] 
    Button $row.g -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.barscale" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes
    
    # at
    set row [ frame $frm.at ]
    Label $row.a -text "Place left corner of scale at 0-100% from top left of monitor (x,y)"
    LabelEntry $row.b -textvariable DmBarscale::opt($id,at) -width 8 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
        
    # arrow or scale only
    set row [ frame $frm.arrow ]
    checkbutton $row.a -text [G_msg "display N. arrow only "] \
    	-variable DmBarscale::opt($id,arrow) 
    checkbutton $row.b -text [G_msg "display scale only"] \
    	-variable DmBarscale::opt($id,scale) 
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # scale options
    set row [ frame $frm.opts ]
    checkbutton $row.a -text [G_msg "line scale instead of bar scale "] -variable \
        DmBarscale::opt($id,line) 
    checkbutton $row.b -text [G_msg "text on top of scale, instead of to right"] -variable \
        DmBarscale::opt($id,top) 
    pack $row.a $row.b  -side left
    pack $row -side top -fill both -expand yes

    # english units
    set row [ frame $frm.units ]
    checkbutton $row.a -text [G_msg "use feet/miles instead of meters"] -variable \
        DmBarscale::opt($id,feet) 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # mouse
    set row [ frame $frm.mouse ]
    checkbutton $row.a -text \
        [G_msg "place with mouse (cannot save placement with group)"] \
        -variable DmBarscale::opt($id,mouse) 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
}



proc DmBarscale::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check bcolor bcolor_none tcolor at feet line top mouse arrow scale } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}


proc DmBarscale::display { node } {
    variable opt
    set line ""
    set input ""
    global dmpath
    set cmd ""

    set tree $Dm::tree
    set id [Dm::node_id $node]


    if { ! ( $opt($id,_check) ) } { return } 

    # set hex colors to rgb         
    set tcolor [Dm::color $opt($id,tcolor)]
    set bcolor [Dm::color $opt($id,bcolor)]

    # no background color
    if { $opt($id,bcolor_none) == 1 } { 
        set bcolor "none"
    }

    set cmd "d.barscale tcolor=$tcolor bcolor=$bcolor "

    # line scale
    if { $opt($id,line) != 0 } { 
        append cmd " -l"
    }

    # text on top
    if { $opt($id,top) != 0 } { 
        append cmd " -t"
    }

    # english units
    if { $opt($id,feet) != 0} { 
        append cmd " -f"
    }
    
    # arrow only
    if { $opt($id,arrow) != 0 } { 
        append cmd " -n"
    }
    
    # scale only
    if { $opt($id,scale) != 0 } { 
        append cmd " -s"
    }

    # place with coordinates
    if { $opt($id,at) != "" && $opt($id,mouse) == 0 } { 
        append cmd " at=$opt($id,at)"
        run_panel $cmd
    }

    # place with mouse
    if { $opt($id,mouse) != 0 } { 
        append cmd " -m"
        term_panel $cmd
    }
    
}

proc DmBarscale::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    puts $file "barscale $opt($id,barscale)"
}


proc DmBarscale::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "barscale:$count"

    set frm [ frame .barscaleicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmBarscale::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo scaleico -file "$dmpath/barscale.gif"
    set ico [label $frm.ico -image scaleico -bd 1 -relief raised]
    
    pack $check $ico -side left

    $tree insert end $parent $node \
		-text      "scale $count" \
		-window    $frm \
		-drawcross auto

    set opt($count,_check) $opt($id,_check)

    set opt($count,tcolor) "$opt($id,tcolor)" 
    set opt($count,bcolor) "$opt($id,bcolor)" 
    set opt($count,line) "$opt($id,line)" 
    set opt($count,at) "$opt($id,at)"
    set opt($count,feet) "$opt($id,feet)"
    set opt($count,top) "$opt($id,top)"
    set opt($count,mouse) "$opt($id,mouse)" 
    set opt($count,arrow) "$opt($id,arrow)"
    set opt($count,scale) "$opt($id,scale)" 

    incr count
    return $node
}
