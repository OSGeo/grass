# Include the select dialog code because it defines scroll bindings
source $env(GISBASE)/etc/gtcltk/select.tcl

# Symbology table
proc color2rgb { color } {
    regexp -- {#(..)(..)(..)} $color x r g b
    set r [expr 0x$r ]
    set g [expr 0x$g ]
    set b [expr 0x$b ]
    return [list $r $g $b]
}

proc set_color { col color } {
    set clr [ color2rgb $color] 
    c_set_color $col [lindex $clr 0]  [lindex $clr 1]  [lindex $clr 2]
}

proc set_on { code on } {
    c_set_on $code $on
}

set tabrow 1
set comrow 0

proc set_col_type { rnum } {
    global columns

    if { $columns(type,$rnum) == "varchar" } {
        $columns(frm,$rnum).c configure -state normal
    } else {
        $columns(frm,$rnum).c configure -state disabled
    }
}

proc add_tab_col { name type width namedit typedit widthedit } {
    global tabrow columns table_frame

    set row [ frame $table_frame.row$tabrow ]
    set columns(frm,$tabrow) $row

    set columns(name,$tabrow) $name
    set columns(type,$tabrow) $type
    set columns(width,$tabrow) $width
   
    Entry $row.a -width 20 -textvariable columns(name,$tabrow) -bg white
    if { $namedit == 0 } { $row.a configure -state disabled }
    ComboBox $row.b -textvariable columns(type,$tabrow) \
    	-values {"integer" "double precision" "varchar"} \
		-modifycmd "set_col_type $tabrow" -entrybg white
    if { $typedit == 0 } { $row.b configure -state disabled }
    Entry $row.c -width 10 -textvariable columns(width,$tabrow) -bg white
    if { $widthedit == 0 } { $row.c configure -state disabled }
    set_col_type $tabrow 
   
    pack $row.a $row.b $row.c -side left; 
    pack $row -side top -fill x -expand no -anchor n 

    incr tabrow
}

proc table_buttons { } {
    global table_page
    set addcol [Button $table_page.addcol -text [G_msg "Add new column"]  \
    	-borderwidth 1\
        -command { add_tab_col "" "integer" 50 1 1 0 }]
    set cretab [Button $table_page.cretab -text [G_msg "Create table"]  \
    	-borderwidth 1\
    	-command { make_table } ]
    pack $addcol $cretab  -side left -anchor s
}

proc add_command { } {
    global comrow GWidget GBgcmd

    c_add_blank_bgcmd
    
    set GBgcmd($comrow,on) 1

    set GBgcmd($comrow,cmd) ""

    set row [ frame $GWidget(bgcmd).row$comrow ]

    checkbutton $row.a -variable GBgcmd($comrow,on) -height 1 
 
    Entry $row.b -width 40 -textvariable GBgcmd($comrow,cmd) -bg white

    pack $row.a $row.b -side left

    pack $row -side top -fill x -expand no -anchor n

    bind $GWidget(bgcmd).row$comrow.b <KeyRelease> 
		{c_create_bgcmd}
    incr comrow

}

proc command_buttons { } {
    global GWidget
    set addcom [Button $GWidget(bgcmd).addcom -text [G_msg "Add command"]  \
    	-borderwidth 1 \
		-command { add_command }]
   pack $addcom -side left -anchor s
}

proc clear_table { } {
    global table_page table_frame
    if { [winfo exists $table_page.addcol] } { destroy $table_page.addcol }
    if { [winfo exists $table_page.cretab] } { destroy $table_page.cretab }
    foreach f [ winfo children $table_frame] {
        destroy $f
    }
}

proc make_table { } {
    global tabrow columns create_table_err create_table_msg

    set coldef ""
    for {set i 1} {$i < $tabrow} {incr i} { 
        if { $i > 1 } { append coldef ", " }
        append coldef "$columns(name,$i) $columns(type,$i) "
        if { $columns(type,$i) == "varchar" } {
            append coldef "( $columns(width,$i) )"
        }
    }
    puts $coldef

    set field 1
    set field_name ""
    set create_table_msg ""
    c_create_table $field $field_name $columns(name,1) $coldef
    
    if { $create_table_err == 1 } {
        MessageDlg .msg -type ok -message $create_table_msg
    } else {
        MessageDlg .msg -type ok -message [G_msg "Table successfully created"]
        clear_table
        c_table_definition
    }
}

# create settings window
proc settings {} {
    global symb GVariable  table_page table_frame tabrow GWidget GBgcmd
    set clw 30
    
    if {![info exists GVariable(linewidth)]} {
    	set GVariable(linewidth) 2
    }

    if { [winfo exists .settings] } {
        puts [G_msg "Settings already opened"]
        wm deiconify .settings
        raise .settings
        return
    } 
    set stt [toplevel .settings]
 
    set nb [NoteBook $stt.nb]  
    set bottom [frame $stt.frm]
    set ok [Button $bottom.ok -text [G_msg "OK"] -width 5 -borderwidth 1\
    	-command {c_next_tool redraw
    		destroy .settings}]
    pack $ok -side right -padx 15
    
    $nb compute_size


    # --- Symbology ---
    set symbf [$nb insert end colors -text [G_msg "Symbology"]]

    # Background
    set row [ frame $symbf.row1 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Background"]  
    SelectColor $row.b -type menubutton -variable symb(background,color) \
                -command { set_color background $symb(background,color) }
    pack $row.a -side left; pack $row.b -side right; pack $row -side top -fill x -expand yes

    # Highlight
    set row [ frame $symbf.row2 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Highlight"]  
    SelectColor $row.b -type menubutton -variable symb(highlight,color) \
                -command { set_color highlight $symb(highlight,color) }
    pack $row.a -side left; pack $row.b -side right; pack $row -side top -fill x -expand yes

    # Point
    set row [ frame $symbf.row3 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Point"]  
    checkbutton $row.b -variable symb(point,on) -height 1 -padx 0 -width 0 \
                -command { set_on point $symb(point,on) }
    SelectColor $row.c -type menubutton -variable symb(point,color) \
                -command { set_color point $symb(point,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    # Line
    set row [ frame $symbf.row4 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Line"]  
    checkbutton $row.b -variable symb(line,on) -height 1 -padx 0 -width 0 \
                -command { set_on line $symb(line,on) }
    SelectColor $row.c -type menubutton -variable symb(line,color) \
                -command { set_color line $symb(line,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    # Boundary
    set row [ frame $symbf.row5 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Boundary (no area)"]  
    checkbutton $row.b -variable symb(boundary_0,on) -height 1 -padx 0 -width 0 \
                -command { set_on boundary_0 $symb(boundary_0,on) }
    SelectColor $row.c -type menubutton -variable symb(boundary_0,color) \
                -command { set_color boundary_0 $symb(boundary_0,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    set row [ frame $symbf.row6 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Boundary (1 area)"]  
    checkbutton $row.b -variable symb(boundary_1,on) -height 1 -padx 0 -width 0 \
                -command { set_on boundary_1 $symb(boundary_1,on) }
    SelectColor $row.c -type menubutton -variable symb(boundary_1,color) \
                -command { set_color boundary_1 $symb(boundary_1,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    set row [ frame $symbf.row7 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Boundary (2 areas)"]  
    checkbutton $row.b -variable symb(boundary_2,on) -height 1 -padx 0 -width 0 \
                -command { set_on boundary_2 $symb(boundary_2,on) }
    SelectColor $row.c -type menubutton -variable symb(boundary_2,color) \
                -command { set_color boundary_2 $symb(boundary_2,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    # Centroid
    set row [ frame $symbf.row8 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Centroid (in area)"]  
    checkbutton $row.b -variable symb(centroid_in,on) -height 1 -padx 0 -width 0 \
                -command { set_on centroid_in $symb(centroid_in,on) }
    SelectColor $row.c -type menubutton -variable symb(centroid_in,color) \
                -command { set_color centroid_in $symb(centroid_in,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    set row [ frame $symbf.row9 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Centroid (outside area)"]  
    checkbutton $row.b -variable symb(centroid_out,on) -height 1 -padx 0 -width 0 \
                -command { set_on centroid_out $symb(centroid_out,on) }
    SelectColor $row.c -type menubutton -variable symb(centroid_out,color) \
                -command { set_color centroid_out $symb(centroid_out,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    set row [ frame $symbf.row10 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Centroid (duplicate in area)"]  
    checkbutton $row.b -variable symb(centroid_dupl,on) -height 1 -padx 0 -width 0 \
                -command { set_on centroid_dupl $symb(centroid_dupl,on) }
    SelectColor $row.c -type menubutton -variable symb(centroid_dupl,color) \
                -command { set_color centroid_dupl $symb(centroid_dupl,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    # Node
    set row [ frame $symbf.row11 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Node (1 line)"]  
    checkbutton $row.b -variable symb(node_1,on) -height 1 -padx 0 -width 0 \
                -command { set_on node_1 $symb(node_1,on) }
    SelectColor $row.c -type menubutton -variable symb(node_1,color) \
                -command { set_color node_1 $symb(node_1,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes

    set row [ frame $symbf.row12 ]
    Label $row.a -anchor w -width $clw -text [G_msg "Node (2 lines)"]  
    checkbutton $row.b -variable symb(node_2,on) -height 1 -padx 0 -width 0 \
                -command { set_on node_2 $symb(node_2,on) }
    SelectColor $row.c -type menubutton -variable symb(node_2,color) \
                -command { set_color node_2 $symb(node_2,color) }
    pack $row.a -side left; pack $row.c $row.b -side right; pack $row -side top -fill x -expand yes



    # --- Settings ---
    set setf [$nb insert end settings -text [G_msg "Settings"]]

    # Snapping threshold
    set row [ frame $setf.row1 ]

    Label $row.a -anchor w -text [G_msg "Snapping threshold in screen pixels"]  
    radiobutton $row.b -variable GVariable(snap_mode) -value 0 -height 1 -padx 0 -width 0 \
                -command { c_var_set snap_mode $GVariable(snap_mode) }
    Entry $row.c -width 10 -textvariable GVariable(snap_screen) -bg white \
                           -command { c_var_set snap_screen $GVariable(snap_screen) } 
    bind $row.c <KeyRelease> { c_var_set snap_screen $GVariable(snap_screen) }
    pack $row.a -side left; pack $row.c $row.b -side right; 
    pack $row -side top -fill x -expand no -anchor n 
    
    set row [ frame $setf.row2 ]
    Label $row.a -anchor w -text [G_msg "Snapping threshold in map units"]  
    radiobutton $row.b -variable GVariable(snap_mode) -value 1 -height 1 -padx 0 -width 0 \
                -command { c_var_set snap_mode $GVariable(snap_mode) }
    Entry $row.c -width 10 -textvariable GVariable(snap_map) -bg white \
                           -command { c_var_set snap_map $GVariable(snap_map) }
    bind $row.c <KeyRelease> { c_var_set snap_map $GVariable(snap_map) }
    pack $row.a -side left; pack $row.c $row.b -side right; 
    pack $row -side top -fill x -expand no -anchor n 

    # how to make a simple blank spacing line?
    set row [ frame $setf.row3 ]
    Label $row.a -anchor w
    pack $row.a -side left;
    pack $row -side top -fill x -expand no -anchor n

    # Linewidth
    set row [ frame $setf.row4 ]
    Label $row.a -anchor w -text [G_msg "Line width in screen pixels"]
    SpinBox $row.b -range {1 50 1} -textvariable GVariable(linewidth) \
		   -width 2 -helptext [G_msg "Set line width in pixels"] -entrybg white \
		   -modifycmd {c_var_set linewidth $GVariable(linewidth); c_next_tool redraw}
    pack $row.a -side left; pack $row.b -side right;
    pack $row -side top -fill x -expand no -anchor n


    # --- Table (define new attribute table) ---
    set tabrow 1
    set table_page [$nb insert end table -text [G_msg "Table"]]

    set tabsw [ScrolledWindow $table_page.sw ]
    set tabsf [ScrollableFrame $table_page.sf -width 400]
    $tabsw setwidget $tabsf
    pack $tabsw $tabsf -fill both -expand yes
    set table_frame [$tabsf getframe]

    c_table_definition

    # --- Background commands ---
    set GWidget(bgcmd) [$nb insert end bgcmd -text [G_msg "Background"]]

    command_buttons
    c_create_bgcmd
    
    # -- pack notebook and bottom frame--
    pack $nb -fill both -expand yes -padx 1 -pady 1
    pack $bottom -side bottom -expand 1 -fill x
    
    $nb raise [$nb page 0]

    tkwait visibility $stt
}


