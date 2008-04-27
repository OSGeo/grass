# 1 Sept 2005
# panel for d.frame
# Michael Barton, Arizona State University

namespace eval DmDframe {
    variable array opt # frame options
    variable count 1
}

proc DmDframe::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "dframe:$count"

    set frm [ frame .dframeicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmDframe::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo dfrmico -file "$dmpath/frames.gif"
    set ico [label $frm.ico -image dfrmico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "frame $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,_check) 1 
    set opt($count,frame) "" 
    set opt($count,erase) 0 
    set opt($count,create) 1 
    set opt($count,select) 0 
    set opt($count,at) "" 
    
    incr count
    return $node
}

proc DmDframe::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}


# frame options
proc DmDframe::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # create, select, or erase frames
    set row [ frame $frm.cats ]
    checkbutton $row.a -text [G_msg "create and select frame"] -variable \
        DmDframe::opt($id,create) 
    checkbutton $row.b -text [G_msg "select frame"] -variable \
        DmDframe::opt($id,select) 
    checkbutton $row.c -text [G_msg "remove all frames"] -variable \
        DmDframe::opt($id,erase) 
    Button $row.d -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.frame" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes

    # frame name
    set row [ frame $frm.frame ]
    Label $row.a -text "Frame name (optional): "
    LabelEntry $row.b -textvariable DmDframe::opt($id,frame) -width 40 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

    # place frame1
    set row [ frame $frm.at1 ]
    Label $row.a -text "Set frame borders at 0-100% from lower left corner of monitor "
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
    
    # place frame2
    set row [ frame $frm.at2 ]
    Label $row.a -text "     set borders (bottom,top,left,right): "
    LabelEntry $row.b -textvariable DmDframe::opt($id,at) -width 25 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
    # place frame3
    set row [ frame $frm.at3 ]
    Label $row.a -text "     If blank, frame created interactively with mouse (but not saved with group)"
    pack $row.a -side left
    pack $row -side top -fill both -expand yes
        
}

proc DmDframe::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check frame erase create select at } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}


proc DmDframe::display { node } {
    variable opt
    set line ""
    set input ""
    global dmpath
    set cmd ""

    set tree $Dm::tree
    set id [Dm::node_id $node]


    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,create) == 0 && $opt($id,select) == 0 && $opt($id,erase) == 0 } { return } 
    set cmd "d.frame"


    # create
    if { $opt($id,create) == 1 } { 
        append cmd " -c"
    }

    # select
    if { $opt($id,select) == 1 } { 
        append cmd " -s"
    }

    # erase and remove
    if { $opt($id,erase) == 1 } { 
        append cmd " -e"
    }

    # frame name
    if { $opt($id,frame) != "" } { 
        append cmd " frame=$opt($id,frame)"
    }

    # frame placement
    if { $opt($id,at) != "" } { 
        append cmd " at=$opt($id,at)"
    }

    run_panel $cmd
}

proc DmDframe::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,map) == "" } { return } 

    puts $file "frame $opt($id,frame)"
}


proc DmDframe::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "dframe:$count"

    set frm [ frame .dframeicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmDframe::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo dfrmico -file "$dmpath/frames.gif"
    set ico [label $frm.ico -image dfrmico -bd 1 -relief raised]
    
    pack $check $ico -side left

	if { $opt($id,frame) == ""} {
    	$tree insert end $parent $node \
		-text      "frame $count" \
		-window    $frm \
		-drawcross auto
	} else {
	    $tree insert end $parent $node \
		-text      "frame $opt($id,frame)" \
		-window    $frm \
		-drawcross auto
	}

    set opt($count,_check) $opt($id,_check)
    
    set opt($count,frame) "$opt($id,frame)" 
    set opt($count,erase) "$opt($id,erase)" 
    set opt($count,at) "$opt($id,at)"
    set opt($count,create) "$opt($id,create)"
    set opt($count,select) "$opt($id,select)"

    incr count
    return $node
}
