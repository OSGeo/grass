# mkColorPopup w name color
#
# Replaced legacy custom dialog box which no longer works on Mac with aqua (and maybe other platforms?)
#	with native color picker called from tk_chooseColor widget
#
# Kept old function name and arguments to avoid having to change call in many different modules
#
# Arguments:
#    w -	Name to use for new top-level window (no longer used)
#    name -     Label for ColorPopup
#    color -    CurrentColor
#    mode - 	Made window modal (no longer used)


global CurrColor

proc mkColorPopup {w {name "Choose color"} {color "#000000"} {mode 0}} {
    global CurrColor
    
    set CurrColor [tk_chooseColor -initialcolor $color -title $name]

    if {$CurrColor==""} {set CurrColor #000000}
    return $CurrColor
}


proc hexval { n }  {
    # this proc might be used by some other modules

    set n [expr int($n)]
    if {$n > 15} {
	return [format %2x $n]
    }
    return [format "0%x" $n]
}


proc tcl_to_rgb {c} {
    # this proc is used by some other modules

    regsub # $c 0x newcolor
    return $newcolor
}
