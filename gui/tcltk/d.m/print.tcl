#fetch GRASS Version number:
set fp [open $env(GISBASE)/etc/VERSIONNUMBER r]
set GRASSVERSION [read -nonewline $fp]
close $fp

namespace eval DmPrint {
}

# reset paper and display in PView
#  
proc DmPrint::paper { } {
    global PWid PVar PPap PView

    # debug output
    #puts "paper mode: $PVar(paper_mode)"
    #puts "paper size: $PVar(paper)"
    #puts "paper width: $PVar(paper_width) height: $PVar(paper_height)"
    
    set vs $PVar(view_size)
    set in 72.0
    if { $PVar(paper_mode) == $PVar(paper_mode_default) } {
	set pap $PVar(paper)
	set PVar(pw) $PPap($pap,width)
	set PVar(ph) $PPap($pap,height)
	set PVar(pl) $PPap($pap,left)
	set PVar(pr) $PPap($pap,right)
	set PVar(pt) $PPap($pap,top)
	set PVar(pb) $PPap($pap,bottom)
    } else {
	set PVar(pw) $PVar(paper_width)
	set PVar(ph) $PVar(paper_height)
	set PVar(pl) $PVar(paper_left)
	set PVar(pr) $PVar(paper_right)
	set PVar(pt) $PVar(paper_top)
	set PVar(pb) $PVar(paper_bottom)
    }

    # calc the beginning of paper coor system in PView in points,
    # scale (to recalc distances in inches on paper to points in PView)
    # x0,y0 upper left of paper
    # x1,y1 lower right of paper
    # xm0,ym0 upper left of margines
    # xm1,ym1 lower right of margines
    if { $PVar(pw) > $PVar(ph) } {
        set sc [ expr $vs / $PVar(pw) ]
        set x0 0
        set y0 [ expr ($vs - $PVar(ph) * $sc) / 2 ]
    } else {
        set sc [ expr $vs / $PVar(ph) ]
        set x0 [ expr ($vs - $PVar(pw) * $sc) / 2 ]
        set y0 0
    }
    set w [ expr $PVar(pw) * $sc ] 
    set h [ expr $PVar(ph) * $sc ] 
    set x1 [ expr $x0 + $w ] 
    set y1 [ expr $y0 + $h ] 

    set l [ expr $sc * $PVar(pl) ]
    set r [ expr $sc * $PVar(pr) ] 
    set t [ expr $sc * $PVar(pt) ]
    set b [ expr $sc * $PVar(pb) ]

    set xm0 [ expr $x0 + $l ] 
    set ym0 [ expr $y0 + $t ] 
    set xm1 [ expr $x1 - $r ] 
    set ym1 [ expr $y1 - $b ] 

    #puts "x0 = $x0 y0 = $y0 w = $w h = $h"
   
    $PView delete all
    $PView create rectangle $x0 $y0 $x1 $y1 -width 1 -fill white 
    $PView create rectangle $xm0 $ym0 $xm1 $ym1 -width 1

}

proc DmPrint::init { } {
    global PWid PVar PPap PView

    set PVar(paper_mode_default) 0
    set PVar(paper_mode_custom) 1
    set PVar(paper_mode) $PVar(paper_mode_default) 

    set PVar(view_size) 200
    set PWid(preview) ".preview"

    # load paper sizes 
    set PVar(papers) ""
    
    set paps [split [exec ps.map -p] "\n"] 
    foreach p $paps { 
        set v [split $p " "] 
        set pap [lindex $v 0 ]
	set PPap($pap,width) [lindex $v 1 ] 
	set PPap($pap,height) [lindex $v 2 ]
	set PPap($pap,left) [lindex $v 3 ]
	set PPap($pap,right) [lindex $v 4 ]
	set PPap($pap,top) [lindex $v 5 ]
	set PPap($pap,bottom) [lindex $v 6 ]

        set PVar(papers) [ concat $PVar(papers) $pap]
    }
    
    # set custom paper to something
    set pap [ lindex $PVar(papers) 0 ]
    set PVar(paper_width) $PPap($pap,width)
    set PVar(paper_height) $PPap($pap,height)
    set PVar(paper_left) $PPap($pap,left)
    set PVar(paper_right) $PPap($pap,right)
    set PVar(paper_top) $PPap($pap,top)
    set PVar(paper_bottom) $PPap($pap,bottom)

    set PVar(paper) [lindex $PVar(papers) 0]

    set PVar(do_scriptfile) 0
    set PVar(scriptfile) ""
    set PVar(do_psfile) 0
    set PVar(psfile) ""
    set PVar(do_pdffile) 0
    set PVar(pdffile) ""
    set PVar(do_pngfile) 0
    set PVar(pngfile) ""
    set PVar(pngresolution) 100

    set PVar(do_printer) 0
    set PVar(printer) "lpr"
}

proc DmPrint::init_tmpfiles { } {
    global PWid PVar PPap PView

    # get temporary file for script file
    set pid [ pid ]
    set PVar(tmpscript) [ exec g.tempfile pid=$pid ]
    #puts "tmpscript: $PVar(tmpscript)"

    set PVar(tmppsfile) [ exec g.tempfile pid=$pid ]
    #puts "tmppsfile: $PVar(tmppsfile)"

    set PVar(tmpppmfile) [ exec g.tempfile pid=$pid ]
    #puts "tmpppmfile: $PVar(tmpppmfile)"
}

proc DmPrint::window { } {
    global PWid PVar PPap PView
    global GRASSVERSION

    # check if opened
    if { [winfo exists .printwin] } {
        wm deiconify .printwin
        raise .printwin
        return
    }

    set PW [toplevel .printwin]
    wm title $PW [G_msg "Print / Plot (Display Manager - GRASS $GRASSVERSION)"]

    # Left part paper + output
    set PWid(left) [ frame $PW.left ]  
    pack $PWid(left) -side left -anchor w

    # paper size, scale
    set PWid(paper) [ frame $PWid(left).paper ]  
    pack $PWid(paper) -side top -anchor w

      # default
    set row [ frame $PWid(paper).row1 ]
    radiobutton $row.a -variable PVar(paper_mode) -value $PVar(paper_mode_default) \
                       -height 1 -padx 0 -width 0 -command DmPrint::paper
    Label $row.b -anchor w -text [G_msg "Paper format"]
    ComboBox $row.c -label "" \
                    -width 20  -textvariable PVar(paper) \
                    -values $PVar(papers) -modifycmd DmPrint::paper
    pack $row.a $row.b $row.c -side left;
    pack $row -side top -fill x -expand no -anchor n

      # custom
    set row [ frame $PWid(paper).row2 ]
    radiobutton $row.a -variable PVar(paper_mode) -value $PVar(paper_mode_custom) \
                       -height 1 -padx 0 -width 0 -command DmPrint::paper
    Label $row.b -anchor w -text [G_msg "Custom"]
    Label $row.c -anchor w -text [G_msg "width:"]
    Entry $row.d -width 10 -textvariable PVar(paper_width) \
                 -command DmPrint::paper
    Label $row.e -anchor w -text [G_msg "height:"]
    Entry $row.f -width 10 -textvariable PVar(paper_height) \
                 -command DmPrint::paper
    pack $row.a $row.b $row.c $row.d $row.e $row.f -side left;
    pack $row -side top -fill x -expand no -anchor n

    set row [ frame $PWid(paper).row3 ]
    Label $row.a -anchor w -text [G_msg "left:"]
    Entry $row.b -width 10 -textvariable PVar(paper_left) -command DmPrint::paper
    Label $row.c -anchor w -text [G_msg "right:"]
    Entry $row.d -width 10 -textvariable PVar(paper_right) -command DmPrint::paper
    Label $row.e -anchor w -text [G_msg "top:"]
    Entry $row.f -width 10 -textvariable PVar(paper_top) -command DmPrint::paper
    Label $row.g -anchor w -text [G_msg "bottom:"]
    Entry $row.h -width 10 -textvariable PVar(paper_bottom) -command DmPrint::paper

    pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g $row.h -side left;
    pack $row -side top -fill x -expand no -anchor n

    # output options
    set PWid(output) [ frame $PWid(left).output ]  
    pack $PWid(output) -side top -anchor w

    # PS file
    set row [ frame $PWid(output).psfile ]
    checkbutton $row.a -variable PVar(do_psfile)
    Label $row.b -anchor w -text [G_msg "PS file:"]
    Entry $row.c -width 50 -textvariable PVar(psfile) 
    Button $row.d -text [G_msg "Browse"] \
           -command { set PVar(psfile) [ tk_getSaveFile -title "Output postscript file" ] }

    pack $row.a $row.b $row.c $row.d -side left;
    pack $row -side top -fill x -expand no -anchor n

    # PDF file
    set row [ frame $PWid(output).pdffile ]
    checkbutton $row.a -variable PVar(do_pdffile)
    Label $row.b -anchor w -text [G_msg "PDF file:"]
    Entry $row.c -width 50 -textvariable PVar(pdffile) 
    Button $row.d -text [G_msg "Browse"] \
           -command { set PVar(pdffile) [ tk_getSaveFile -title "Output PDF file" ] }

    pack $row.a $row.b $row.c $row.d -side left;
    pack $row -side top -fill x -expand no -anchor n

    # PNG file
    set row [ frame $PWid(output).pngfile ]
    checkbutton $row.a -variable PVar(do_pngfile)
    Label $row.b -anchor w -text [G_msg "PNG file:"]
    Entry $row.c -width 50 -textvariable PVar(pngfile) 
    Button $row.d -text [G_msg "Browse"] \
           -command { set PVar(pngfile) [ tk_getSaveFile -title "Output PNG file" ] }

    pack $row.a $row.b $row.c $row.d -side left;
    pack $row -side top -fill x -expand no -anchor n

    set row [ frame $PWid(output).pngres ]
    Label $row.a -anchor w -text [G_msg "PNG file resolution (points/inch):"]
    Entry $row.b -width 15 -textvariable PVar(pngresolution) 

    pack $row.a $row.b -side left;
    pack $row -side top -fill x -expand no -anchor n

    # Printer
    set row [ frame $PWid(output).printer ]
    checkbutton $row.a -variable PVar(do_printer)
    Label $row.b -anchor w -text [G_msg "Printer:"]
    Entry $row.c -width 50 -textvariable PVar(printer) 

    pack $row.a $row.b $row.c -side left;
    pack $row -side top -fill x -expand no -anchor n

    # Script file
    set row [ frame $PWid(output).scriptfile ]
    checkbutton $row.a -variable PVar(do_scriptfile)
    Label $row.b -anchor w -text [G_msg "Script file:"]
    Entry $row.c -width 50 -textvariable PVar(scriptfile) 
    Button $row.d -text [G_msg "Browse"] \
           -command { set PVar(scriptfile) [ tk_getSaveFile -title "Output script file" ] }

    pack $row.a $row.b $row.c $row.d -side left;
    pack $row -side top -fill x -expand no -anchor n

    # Paper view
    frame $PW.view
    pack $PW.view -side right -anchor e
    set PView $PW.view.c
    canvas $PView -width $PVar(view_size) -height $PVar(view_size) -relief sunken -borderwidth 0
    $PView create rect 0 0 $PVar(view_size) $PVar(view_size) -width 0
    grid $PView -in $PW.view 

    # Buttons 
    set but [ frame $PWid(left).buttons ]  
    pack $but -side top

    Button $but.print -text [G_msg "Print"] -command { DmPrint::print "print" }
    Button $but.preview -text [G_msg "Preview"] -command { DmPrint::print "preview" }
    Button $but.close -text [G_msg "Close"] -command { destroy .printwin }
    pack $but.print $but.preview $but.close -side left 

    tkwait visibility $PW

    DmPrint::paper
}

proc DmPrint::print { ptype } {
    global PWid PVar PPap PView
    global GRASSVERSION

    DmPrint::paper

    # Generate script file
    set file [open $PVar(tmpscript) w]

    puts $file "paper" 
    puts $file "  width $PVar(pw)" 
    puts $file "  height $PVar(ph)" 
    puts $file "  left $PVar(pl)" 
    puts $file "  right $PVar(pr)" 
    puts $file "  top $PVar(pt)" 
    puts $file "  bottom $PVar(pb)" 
    puts $file "end" 

    DmGroup::print $file "root" 

    close $file

    set cmd "ps.map input=$PVar(tmpscript) output=$PVar(tmppsfile)"
    Dm::execute $cmd

    if { $ptype == "print" } {
        if { $PVar(do_psfile) && $PVar(psfile) != "" } {
            set cmd "cp $PVar(tmppsfile) $PVar(psfile)"
            Dm::execute $cmd
        }
        if { $PVar(do_pdffile) && $PVar(pdffile) != "" } {
            set res 300 
	    set w [ expr int($res * $PVar(pw)) ]
	    set h [ expr int($res * $PVar(ph)) ]

	    set geom "-g$w"
	    append geom "x$h"
	    set cmd "cat $PVar(tmppsfile) | gs -q -dNOPAUSE -r$res $geom -sDEVICE=pdfwrite -sOutputFile=$PVar(pdffile) -"
	    Dm::execute $cmd
        }
        if { $PVar(do_pngfile) && $PVar(pngfile) != "" } {
            set res $PVar(pngresolution) 
	    if { $res < 2 } { set res 2 } 
	    set w [ expr int($res * $PVar(pw)) ]
	    set h [ expr int($res * $PVar(ph)) ]

	    set geom "-g$w"
	    append geom "x$h"
	    set cmd "cat $PVar(tmppsfile) | gs -q -dNOPAUSE -r$res $geom -sDEVICE=png16m -sOutputFile=$PVar(pngfile) -"
	    Dm::execute $cmd
        }
        if { $PVar(do_printer) && $PVar(printer) != "" } {
            set cmd "$PVar(printer) $PVar(tmppsfile)"
            Dm::execute $cmd
        }
        if { $PVar(do_scriptfile) && $PVar(scriptfile) != "" } {
            set cmd "cp $PVar(tmpscript) $PVar(scriptfile)"
            Dm::execute $cmd
        }
    }


    # Note: it seems that Tk doesn't support ASCII PPM images produced by gs
    if { $ptype == "preview" } {

	# maximum size of preview image in pixels    
	set size 600
	if { $PVar(pw) > $PVar(ph) } {
	    set res [ expr int($size / $PVar(pw)) ]
	} else {
	    set res [ expr int($size / $PVar(ph)) ]
	}
        # Note: resolutin must be >= 2
        if { $res < 2 } { set res 2 } 
        set w [ expr int($res * $PVar(pw)) ]
        set h [ expr int($res * $PVar(ph)) ]

        set geom "-g$w"
        append geom "x$h"
        set cmd "cat $PVar(tmppsfile) | gs -q -dNOPAUSE -r$res $geom -sDEVICE=ppmraw -sOutputFile=$PVar(tmpppmfile) -"
        Dm::execute $cmd

	if { [winfo exists $PWid(preview)] } {
            destroy $PWid(preview).pvl
	    wm deiconify $PWid(preview)
	    raise $PWid(preview)
	} else {
	    toplevel $PWid(preview)
	    wm title $PWid(preview) "Preview ( Print / Plot - Display Manager - GRASS $GRASSVERSION)"
        }
	image create photo preview_image -format PPM -file $PVar(tmpppmfile)
	set pv [ label $PWid(preview).pvl -image preview_image -bd 1 -relief sunken -width $size -height $size ]
	pack $pv
    }
}

proc DmPrint::save { } {
    global PWid PVar PPap PView

    Dm::rc_write 0 "Print"
    foreach key { paper_mode paper paper_width paper_height paper_left paper_right paper_top paper_bottom 
                  do_scriptfile scriptfile do_psfile psfile do_pdffile pdffile 
                  do_pngfile pngfile pngresolution 
                  do_printer printer 
    } {
        Dm::rc_write 1 "$key $PVar($key)"
    }
    Dm::rc_write 0 "End"
}

proc DmPrint::set_option { key value } {
    global PWid PVar PPap PView

    set PVar($key) $value

}

# Delete temporary files
proc DmPrint::clean {  } {
    global PWid PVar PPap PView

    file delete $PVar(tmpscript)
    file delete $PVar(tmppsfile)
    file delete $PVar(tmpppmfile)

}

