# mkColorPopup w name color
#
# Create a dialog box with sliders and buttons to adjust current color
#
# Arguments:
#    w -	Name to use for new top-level window.
#    name -     Label for ColorPopup
#    color -    CurrentColor


global CurrColor

proc mkColorPopup {w name {color "#000000"} {mode 0}} {
    global CurrColor

    catch {destroy $w}
    toplevel $w -class Dialog
    wm title $w "Select Color"
    wm iconname $w "Color"
    wm geometry $w 300x450
    wm minsize $w 50 100
    wm resizable $w false false

    set tmp [tcl_to_rgb $color]
    set r [expr ($tmp&0xff0000)>>16]
    set g [expr ($tmp&0x00ff00)>>8]
    set b [expr ($tmp&0x0000ff)]

    set tmp $color

    # Create two frames in the main window. The top frame will hold the
    # sliders that interactively change color and the bottom one will hold 
    # the buttons for predefined color. 

    frame $w.top -relief raised -border 1
    frame $w.bot -relief raised -border 1
    pack $w.top $w.bot -side top -fill both -expand yes

    # Note that these sliders are now floating sliders since tk4.0
    # supports these.  This means that color values should be
    # specified in the range 0.0-1.0.  For ease of use, most routines
    # just convert from 00-FF to 0.0-1.0.
    frame $w.top.left
    Nv_mkColorScale $w.top.left Red $r red  $w.top.color
    Nv_mkColorScale $w.top.left Green $g green  $w.top.color
    Nv_mkColorScale $w.top.left Blue $b blue  $w.top.color

    # ACS One line: without next line, the scales and $w.top.color are set to white next time
    # after a color button (not sliders) has been used, regardless $color. Don't know why
    setScales $w.top.left $color
    
    pack $w.top.left.red $w.top.left.green $w.top.left.blue -side top -expand 1
    set CurrColor $color
    label $w.top.color -bg $color -width 5
    pack $w.top.left  -side left -expand 1
    pack $w.top.color -side left  -padx 10 -pady 10 -fill both -expand yes

    tkwait visibility $w
    
    frame $w.bot.buttonframe
    button $w.bot.buttonframe.ok -text OK -command "destroy $w" -bd 1
    button $w.bot.buttonframe.cancel  -bd 1 \
		-text Cancel  -command "set CurrColor $tmp; destroy $w"
    label $w.bot.buttonframe.label -text $name
    pack $w.bot.buttonframe.label  -side left -expand yes -padx 10
    pack $w.bot.buttonframe.cancel $w.bot.buttonframe.ok -side right -expand 1
    pack $w.bot.buttonframe -side bottom -fill x 
    mkColorButtons $w.bot.bf $w.top.left $w.top.color
    pack $w.bot.bf -padx 3 -pady 6 -side top -expand 1
    bind $w <Any-Enter> [list focus $w]
    focus $w

    # If the caller desires, make everything else wait until a color
    # has been selected or this popup has been dismissed.
    if {$mode} {grab $w}

    tkwait window $w
    return $CurrColor
}

proc mkColorButtons { B S L } {

    global CurrColor
    
    frame $B
    set clist [mkColorList]

    for {set i 0; set k 0} {$i < 9 } {incr i} {
	# make frame to hold buttons in this row 
	frame $B.f$i
	for {set j 0} {$j < 5 } {incr j; incr k} {
	    set color [lindex $clist $k]
	    button $B.f$i.$j -bg $color \
		-activeforeground $color \
		-activebackground $color \
		-width 0 -height 0 \
		-highlightthickness 1 \
		-padx 7 -pady 0 \
                -command "setScales $S $color; $L config -bg $color; set CurrColor $color"
	    pack $B.f$i.$j -side top -expand 1
	}
	pack $B.f$i -side left
    }
}

proc getColorfromScales {S} {
   
    # Have to convert colors back to #XXXXXX since that is what
    # everyone else expects
    set r [expr int([$S.red.scale get] * 255.0)]
    set g [expr int([$S.green.scale get] * 255.0)]
    set b [expr int([$S.blue.scale get] * 255.0)]

    set r [hexval $r]
    set g [hexval $g]
    set b [hexval $b]

    return #$r$g$b
}




proc setLabelfromScales {S L args} {
    global CurrColor

    # Convert back to #XXXXXX
    set r [expr int([$S.red.scale get] * 255.0)]
    set g [expr int([$S.green.scale get] * 255.0)]
    set b [expr int([$S.blue.scale get] * 255.0)]
    
    set r [hexval $r]
    set g [hexval $g]
    set b [hexval $b]

    $L config -bg #$r$g$b
    set CurrColor #$r$g$b
}

proc setScales {S c} {
    set color [tcl_to_rgb $c]
    set r [expr ($color&0xff0000)>>16]
    set g [expr ($color&0x00ff00)>>8]
    set b [expr ($color&0x0000ff)]

    $S.red.scale set [expr ($r + 0.0)/255.0]
    $S.green.scale set [expr ($g + 0.0)/255.0]
    $S.blue.scale set [expr ($b + 0.0)/255.0]
}

proc mkColorList {} {

    set ramp 0
    set colorlist {}
    set maxval ff
    set minval 00
    for {set r 4; set i 0} {$r < 16} {incr r 5} {
        for {set g 4} {$g < 16} {incr g 5} {
            for {set b 4} {$b < 16 && $i < 40} {incr b 5} {
		set tmpr [hexval [expr $r/15.0*255]]
		set tmpg [hexval [expr $g/15.0*255]]
		set tmpb [hexval [expr $b/15.0*255]]
		set color #$tmpr$tmpg$tmpb
		set colorlist [concat $colorlist $color]
		incr i
	    }
	    if {$ramp == 0} {
		set colorlist [concat $colorlist #ff0000 #00ffff]
		incr ramp 2
		incr i 2
	    } elseif {$ramp < 10} {
	        set tmpg [hexval [expr $ramp/9.0*255]]
		set color #$maxval$tmpg$minval
		set colorlist [concat $colorlist $color]
		incr ramp
		incr i

	        set tmpg [hexval [expr (1.0 - $ramp/9.0)*255]]
		set color #$minval$tmpg$maxval
		set colorlist [concat $colorlist $color]
		incr ramp
		incr i

	    } elseif {$ramp < 16} {
	        set tmpb [hexval [expr ($ramp-10)/7.0*255]]
		set color #$maxval$maxval$tmpb
		set colorlist [concat $colorlist $color]
		incr ramp
		incr i
	        set tmpb [hexval [expr (1.0 - ($ramp-10)/7.0)*255]]
		set color #$minval$minval$tmpb
		set colorlist [concat $colorlist $color]
		incr ramp
		incr i
	    }
	}
    }
    for {set gray 0} {$gray < 5} {incr gray} {
	set g [hexval [expr (1.0 - $gray/4.0)*255]]
	set color #$g$g$g
	set colorlist [concat $colorlist $color]
    }


    return $colorlist

}

proc hexval { n }  {

    set n [expr int($n)]
    if {$n > 15} {
	return [format %2x $n]
    }
    return [format "0%x" $n]
}

##########################################################################
# procedure to make sliders
##########################################################################
proc Nv_mkColorScale { P {name " "} {curr 200}\
    {color ""} {chip ""}} {

    set S $P.$color
    frame $S
    frame $S.f
    
    # Make a global variable shared by this scale and its entry
    global $S.val 

    set $S.val [expr ($curr + 0.0)/255.0]

    scale $S.scale -from 0.0 -length 140 -showvalue 0 -orient h \
	-digits 3 -resolution 0.01 -tickinterval 0 -to 1.0 -width 13 \
	-command "setLabelfromScales $P $chip " \
        -activebackground gray80 -background gray90 -bg $color \
	-variable $S.val
       
    label $S.f.label -text $name
    $S.scale set $curr
    entry $S.f.entry -width 5 -borderwidth 2 -relief sunken -textvariable $S.val
    bind $S.f.entry <Return> "set $S.val [$S.f.entry get]"
    pack $S.scale $S.f -side top
    pack $S.f.label $S.f.entry -side left

    return $S
}


proc tcl_to_rgb {c} {
    regsub # $c 0x newcolor
    return $newcolor
}
