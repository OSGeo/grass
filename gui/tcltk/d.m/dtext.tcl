          # 1 Sept 2005
# panel for d.text
# Michael Barton, Arizona State University

namespace eval DmDtext {
    variable array opt # dtext options
    variable count 1
}


proc DmDtext::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "dtext:$count"

    set frm [ frame .dtexticon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmDtext::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo dtxtico -file "$dmpath/dtext.gif"
    set ico [label $frm.ico -image dtxtico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "text $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,_check) 1 

    set opt($count,text) "" 
    set opt($count,at) "" 
    set opt($count,line) "" 
    set opt($count,color) \#000000 
    set opt($count,size) 5 
    set opt($count,bold) 0 
    
    incr count
    return $node
}

proc DmDtext::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}

proc DmDtext::select_file { id } {
    variable tree
    variable node
    set m [GSelect file]
    if { $m != "" } { 
        set DmDtext::opt($id,path) $m
    }
}


# dtext options
proc DmDtext::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # text
    set row [ frame $frm.text ]
    Label $row.a -text "Text to display:"
    LabelEntry $row.b -textvariable DmDtext::opt($id,text) -width 51 \
            -entrybg white
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.text" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # placement1
    set row [ frame $frm.at ]
    Label $row.a -text "Text placement: as % of display from lower left (x,y)"
    LabelEntry $row.b -textvariable DmDtext::opt($id,at) -width 10 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
        
    # placement2
    set row [ frame $frm.line ]
    Label $row.a -text "     by line number from top (1-1000)"
    LabelEntry $row.b -textvariable DmDtext::opt($id,line) -width 10 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
    # font options
    set row [ frame $frm.fontopt ]
    Label $row.a -text [G_msg "Text options: color"] 
    SelectColor $row.b -type menubutton -variable DmDtext::opt($id,color)
    Label $row.c -text " text height (% of display)" 
    SpinBox $row.d -range {1 100 1} -textvariable DmDtext::opt($id,size) \
                   -entrybg white -width 3 
    checkbutton $row.e -padx 10 -text [G_msg "bold text"] -variable \
        DmDtext::opt($id,bold) 
    pack $row.a $row.b $row.c $row.d $row.e -side left
    pack $row -side top -fill both -expand yes

}

proc DmDtext::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check text at line color size bold } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}




proc DmDtext::display { node } {
    variable opt
    set line ""
    set input ""
    global dmpath
    set cmd ""

    set tree $Dm::tree
    set id [Dm::node_id $node]
    
    # set hex colors to rgb         
    set color [Dm::color $opt($id,color)]
    

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,text) == "" } { return } 
    
    set cmd "echo $opt($id,text) | d.text color=$color size=$opt($id,size) "

    # at
    if { $opt($id,at) != "" } { 
        append cmd " {at=$opt($id,at)}"
    }

    # line
    if { $opt($id,line) != "" } { 
        append cmd " line=$opt($id,line)"
    }


    # bold text
    if { $opt($id,bold) != 0 } { 
        append cmd " -b"
    }
    
#    eval "exec echo $opt($id,text) | $cmd"

	run_panel $cmd
    
}

proc DmDtext::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,text) == "" } { return } 

    puts $file "dtext $opt($id,dtext)"
}


proc DmDtext::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "dtext:$count"

    set frm [ frame .dtexticon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmDtext::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo dtxtico -file "$dmpath/dtext.gif"
    set ico [label $frm.ico -image dtxtico -bd 1 -relief raised]
    
    pack $check $ico -side left

	if { $opt($id,text) == ""} {
    	$tree insert end $parent $node \
		-text      "text $count" \
		-window    $frm \
		-drawcross auto
	}

    set opt($count,_check) $opt($id,_check)

    set opt($count,text) $opt($id,text) 
    set opt($count,at) $opt($id,at) 
    set opt($count,line) $opt($id,line) 
    set opt($count,color) $opt($id,color) 
    set opt($count,size) $opt($id,size) 
    set opt($count,bold) $opt($id,bold)

    incr count
    return $node
}
