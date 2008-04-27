# 1 Sept 2005
# panel for d.text.freetype
# Michael Barton, Arizona State University

namespace eval DmFTtext {
    variable array opt # fttext options
    variable count 1
}


proc DmFTtext::create { tree parent } {
    variable opt
    variable count
    global dmpath

    set node "fttext:$count"

    set frm [ frame .fttexticon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmFTtext::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo fttico -file "$dmpath/fttext.gif"
    set ico [label $frm.ico -image fttico -bd 1 -relief raised]
    
    pack $check $ico -side left
    
    $tree insert end $parent $node \
	-text  "freetype text $count"\
	-window    $frm \
	-drawcross auto  
        
    set opt($count,_check) 1 

    set opt($count,text) "" 
    set opt($count,at) "" 
    set opt($count,font) "luximr" 
    set opt($count,charset) "UTF-8" 
    set opt($count,path) "" 
    set opt($count,color) \#000000 
    set opt($count,size) 5 
    set opt($count,align) "ll" 
    set opt($count,rotation) 0 
    set opt($count,linespacing) 1.1 
    set opt($count,bold) 0 
    set opt($count,textcoord) "percent" 
    set opt($count,radians) 0 
    set opt($count,htpixel) 0 
    
    incr count
    return $node
}

proc DmFTtext::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}

proc DmFTtext::select_file { id } {
    variable tree
    variable node
    set m [GSelect file ]
    if { $m != "" } { 
        set DmFTtext::opt($id,path) $m
    }
}


# fttext options
proc DmFTtext::options { id frm } {
    variable opt
    global dmpath
    global bgcolor

    # text
    set row [ frame $frm.text ]
    Label $row.a -text "Text to display:"
    LabelEntry $row.b -textvariable DmFTtext::opt($id,text) -width 51 \
            -entrybg white
    Button $row.c -text [G_msg "Help"] \
            -image [image create photo -file "$dmpath/grass.gif"] \
            -command "run g.manual d.text.freetype" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # coordinates1
    set row [ frame $frm.at ]
    Label $row.a -text "Text placement: coordinates x,y or east,north"
    LabelEntry $row.b -textvariable DmFTtext::opt($id,at) -width 25 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
        
    # coordinates2
    set row [ frame $frm.textcoord2 ]
    Label $row.a -text \
        [G_msg "     (leave blank to place with mouse; position will not save)"] 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # coordinates3
    set row [ frame $frm.textcoord3 ]
    Label $row.a -text [G_msg "     coordinate type"] 
    ComboBox $row.b -padx 2 -width 10 -textvariable DmFTtext::opt($id,textcoord) \
                    -values {"percent" "pixels" "geographic"} -entrybg white
    Label $row.c -text [G_msg "  align text with coordinate point"] 
    ComboBox $row.d -padx 2 -width 2 -textvariable DmFTtext::opt($id,align) \
                    -values {"ll" "lc" "lr" "cl" "cc" "cr" "ul" "uc" "ur" } -entrybg white
    pack $row.a $row.b $row.c $row.d -side left
    pack $row -side top -fill both -expand yes
        
    # coordinates3
    set row [ frame $frm.textcoord4 ]
    Label $row.a -text \
        [G_msg "     (for coordinates, % is from bottom left of display, pixels from top left)"] 
    pack $row.a -side left
    pack $row -side top -fill both -expand yes

    # text options1
    set row [ frame $frm.textopt1 ]
    Label $row.a -text [G_msg "     text rotation (degrees counterclockwise)"] 
    SpinBox $row.b -range {1 360 1} -textvariable DmFTtext::opt($id,rotation) \
                   -entrybg white -width 4
    checkbutton $row.c -padx 10 -text [G_msg "rotation in radians"] -variable \
        DmFTtext::opt($id,radians)
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes
    
    # text options2
    set row [ frame $frm.textopt2 ]
    Label $row.a -text "     line spacing"
    LabelEntry $row.b -textvariable DmFTtext::opt($id,linespacing) -width 5 \
            -entrybg white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
    
    # standard font
    set row [ frame $frm.font ]
    Label $row.a -text [G_msg "Font:"] 
    ComboBox $row.b -padx 2 -width 7 -textvariable DmFTtext::opt($id,font) \
                    -values {"luximr" "luxirr" "luxisr" "luximb" "luxirb" \
                    "luxisb" "luximri" "luxirri" "luxisri" "luximbi" \
                    "luxirbi" "luxisbi"} -entrybg white
    Label $row.c -text [G_msg "  color"] 
    SelectColor $row.d -type menubutton -variable DmFTtext::opt($id,color)
    checkbutton $row.e -padx 10 -text [G_msg "bold text"] -variable \
        DmFTtext::opt($id,bold) 
    Label $row.f -text "character encoding"
    LabelEntry $row.g -textvariable DmFTtext::opt($id,charset) -width 10 \
            -entrybg white
    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g -side left
    pack $row -side top -fill both -expand yes

    # standard font
    set row [ frame $frm.height ]
    Label $row.a -text "     text height (% of display)" 
    SpinBox $row.b -range {1 100 1} -textvariable DmFTtext::opt($id,size) \
                   -entrybg white -width 3 
    checkbutton $row.c -padx 10 -text [G_msg "height in pixels instead of %"] -variable \
        DmFTtext::opt($id,htpixel) 
    pack $row.a $row.b $row.c -side left
    pack $row -side top -fill both -expand yes

    # font path
    set row [ frame $frm.path ]
    Label $row.a -text "     path to custom font" 
    Entry $row.b -width 45 -text " $opt($id,path)" \
          -textvariable DmFTtext::opt($id,path) \
          -background white
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes

}

proc DmFTtext::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check text at font path charset color \
            size align rotation linespacing bold textcoord radians htpixel } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}




proc DmFTtext::display { node } {
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
    
    
    set cmd "d.text.freetype charset=$opt($id,charset) \
            color=$color size=$opt($id,size) align=$opt($id,align) \
            rotation=$opt($id,rotation) linespacing=$opt($id,linespacing)\
            {text=$opt($id,text)}"

    # coordinates
    if { $opt($id,at) != "" } { 
#    	set $opt($id,at) \"$opt($id,at)\"
        append cmd " at=$opt($id,at)"
#        append cmd " {at=$opt($id,at)}"
    }

    # font
    if { $opt($id,font) != "" } { 
        append cmd " font=$opt($id,font)"
    }

    # path
    if { $opt($id,path) != "" } { 
        append cmd " {path=$opt($id,path)}"
    }

    # textcoord pixel
    if { $opt($id,textcoord) == "pixel" } { 
        append cmd " -p"
    }

    # textcoord geographic
    if { $opt($id,textcoord) == "geographic" } { 
        append cmd " -g"
    }

    # font height in pixels
    if { $opt($id,htpixel) != 0 } { 
        append cmd " -s"
    }

    # bold text
    if { $opt($id,bold) != 0 } { 
        append cmd " -b"
    }

    # radians
    if { $opt($id,radians) != 0} { 
        append cmd " r"
    }

    run_panel $cmd
}

proc DmFTtext::print { file node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,text) == "" } { return } 

    puts $file "fttext $opt($id,fftext)"
}


proc DmFTtext::duplicate { tree parent node id } {
    variable opt
    variable count 
    global dmpath

    set node "fttext:$count"

    set frm [ frame .fttexticon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmFTtext::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo fttico -file "$dmpath/fttext.gif"
    set ico [label $frm.ico -image fttico -bd 1 -relief raised]
    
    pack $check $ico -side left

	if { $opt($id,text) == ""} {
    	$tree insert end $parent $node \
		-text      "freetype text $count" \
		-window    $frm \
		-drawcross auto
	}

    set opt($count,_check) $opt($id,_check)

    set opt($count,text) $opt($id,text) 
    set opt($count,at) $opt($id,at)
    set opt($count,font) $opt($id,font) 
    set opt($count,path) $opt($id,path)
    set opt($count,charset) $opt($id,charset) 
    set opt($count,color) $opt($id,color)
    set opt($count,size) $opt($id,size) 
    set opt($count,align) $opt($id,align) 
    set opt($count,rotation) $opt($id,rotation) 
    set opt($count,linespacing) $opt($id,linespacing) 
    set opt($count,bold) $opt($id,bold)
    set opt($count,textcoord) $opt($id,textcoord)
    set opt($count,radians) $opt($id,radians) 
    set opt($count,htpixel) $opt($id,htpixel)

    incr count
    return $node
}
