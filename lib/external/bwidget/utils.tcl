# ------------------------------------------------------------------------------
#  utils.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - GlobalVar::exists
#     - GlobalVar::setvarvar
#     - GlobalVar::getvarvar
#     - BWidget::assert
#     - BWidget::clonename
#     - BWidget::get3dcolor
#     - BWidget::XLFDfont
#     - BWidget::place
#     - BWidget::grab
#     - BWidget::focus
# ------------------------------------------------------------------------------

namespace eval GlobalVar {
    proc use {} {}
}


namespace eval BWidget {
    variable _top
    variable _gstack {}
    variable _fstack {}
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command GlobalVar::exists
# ------------------------------------------------------------------------------
proc GlobalVar::exists { varName } {
    return [uplevel \#0 [list info exists $varName]]
}


# ------------------------------------------------------------------------------
#  Command GlobalVar::setvar
# ------------------------------------------------------------------------------
proc GlobalVar::setvar { varName value } {
    return [uplevel \#0 [list set $varName $value]]
}


# ------------------------------------------------------------------------------
#  Command GlobalVar::getvar
# ------------------------------------------------------------------------------
proc GlobalVar::getvar { varName } {
    return [uplevel \#0 [list set $varName]]
}


# ------------------------------------------------------------------------------
#  Command GlobalVar::tracevar
# ------------------------------------------------------------------------------
proc GlobalVar::tracevar { cmd varName args } {
    return [uplevel \#0 trace $cmd [list $varName] $args]
}



# ------------------------------------------------------------------------------
#  Command BWidget::lreorder
# ------------------------------------------------------------------------------
proc BWidget::lreorder { list neworder } {
    set pos     0
    set newlist {}
    foreach e $neworder {
        if { [lsearch -exact $list $e] != -1 } {
            lappend newlist $e
            set tabelt($e)  1
        }
    }
    set len [llength $newlist]
    if { !$len } {
        return $list
    }
    if { $len == [llength $list] } {
        return $newlist
    }
    set pos 0
    foreach e $list {
        if { ![info exists tabelt($e)] } {
            set newlist [linsert $newlist $pos $e]
        }
        incr pos
    }
    return $newlist
}


# ------------------------------------------------------------------------------
#  Command BWidget::assert
# ------------------------------------------------------------------------------
proc BWidget::assert { exp {msg ""}} {
    set res [uplevel expr $exp]
    if { !$res} {
        if { $msg == "" } {
            return -code error "Assertion failed: {$exp}"
        } else {
            return -code error $msg
        }
    }
}


# ------------------------------------------------------------------------------
#  Command BWidget::clonename
# ------------------------------------------------------------------------------
proc BWidget::clonename { menu } {
    set path     ""
    set menupath ""
    set found    0
    foreach widget [lrange [split $menu "."] 1 end] {
        if { $found || [winfo class "$path.$widget"] == "Menu" } {
            set found 1
            append menupath "#" $widget
            append path "." $menupath
        } else {
            append menupath "#" $widget
            append path "." $widget
        }    
    }
    return $path
}


# ------------------------------------------------------------------------------
#  Command BWidget::getname
# ------------------------------------------------------------------------------
proc BWidget::getname { name } {
    if { [string length $name] } {
        set text [option get . "${name}Name" ""]
        if { [string length $text] } {
            return [parsetext $text]
        }
    }
    return {}
 }


# ------------------------------------------------------------------------------
#  Command BWidget::parsetext
# ------------------------------------------------------------------------------
proc BWidget::parsetext { text } {
    set result ""
    set index  -1
    set start  0
    while { [string length $text] } {
        set idx [string first "&" $text]
        if { $idx == -1 } {
            append result $text
            set text ""
        } else {
            set char [string index $text [expr {$idx+1}]]
            if { $char == "&" } {
                append result [string range $text 0 $idx]
                set    text   [string range $text [expr {$idx+2}] end]
                set    start  [expr {$start+$idx+1}]
            } else {
                append result [string range $text 0 [expr {$idx-1}]]
                set    text   [string range $text [expr {$idx+1}] end]
                incr   start  $idx
                set    index  $start
            }
        }
    }
    return [list $result $index]
}


# ------------------------------------------------------------------------------
#  Command BWidget::get3dcolor
# ------------------------------------------------------------------------------
proc BWidget::get3dcolor { path bgcolor } {
    foreach val [winfo rgb $path $bgcolor] {
        lappend dark [expr 60*$val/100]
        set tmp1 [expr 14*$val/10]
        if { $tmp1 > 65535 } {
            set tmp1 65535
        }
        set tmp2 [expr (65535+$val)/2]
        lappend light [expr ($tmp1 > $tmp2) ? $tmp1:$tmp2]
    }
    return [list [eval format "#%04x%04x%04x" $dark] [eval format "#%04x%04x%04x" $light]]
}


# ------------------------------------------------------------------------------
#  Command BWidget::XLFDfont
# ------------------------------------------------------------------------------
proc BWidget::XLFDfont { cmd args } {
    switch -- $cmd {
        create {
            set font "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"
        }
        configure {
            set font [lindex $args 0]
            set args [lrange $args 1 end]
        }
        default {
            return -code error "XLFDfont: commande incorrecte: $cmd"
        }
    }
    set lfont [split $font "-"]
    if { [llength $lfont] != 15 } {
        return -code error "XLFDfont: description XLFD incorrecte: $font"
    }

    foreach {option value} $args {
        switch -- $option {
            -foundry { set index 1 }
            -family  { set index 2 }
            -weight  { set index 3 }
            -slant   { set index 4 }
            -size    { set index 7 }
            default  { return -code error "XLFDfont: option incorrecte: $option" }
        }
        set lfont [lreplace $lfont $index $index $value]
    }
    return [join $lfont "-"]
}



# ------------------------------------------------------------------------------
#  Command BWidget::place
# ------------------------------------------------------------------------------
proc BWidget::place { path w h args } {
    variable _top

    update idletasks
    set reqw [winfo reqwidth  $path]
    set reqh [winfo reqheight $path]
    if { $w == 0 } {set w $reqw}
    if { $h == 0 } {set h $reqh}

    set arglen [llength $args]
    if { $arglen > 3 } {
        return -code error "BWidget::place: bad number of argument"
    }

    if { $arglen > 0 } {
        set where [lindex $args 0]
        set idx   [lsearch {"at" "center" "left" "right" "above" "below"} $where]
        if { $idx == -1 } {
            return -code error "BWidget::place: incorrect position \"$where\""
        }
        if { $idx == 0 } {
            set err [catch {
                set x [expr {int([lindex $args 1])}]
                set y [expr {int([lindex $args 2])}]
            }]
            if { $err } {
                return -code error "BWidget::place: incorrect position"
            }
            if { $x >= 0 } {
                set x "+$x"
            }
            if { $y >= 0 } {
                set y "+$y"
            }
        } else {
            if { $arglen == 2 } {
                set widget [lindex $args 1]
                if { ![winfo exists $widget] } {
                    return -code error "BWidget::place: \"$widget\" does not exist"
                }
            }
            set sw [winfo screenwidth  $path]
            set sh [winfo screenheight $path]
            if { $idx == 1 } {
                if { $arglen == 2 } {
                    # center to widget
                    set x0 [expr [winfo rootx $widget] + ([winfo width  $widget] - $w)/2]
                    set y0 [expr [winfo rooty $widget] + ([winfo height $widget] - $h)/2]
                } else {
                    # center to screen
                    set x0 [expr ([winfo screenwidth  $path] - $w)/2 - [winfo vrootx $path]]
                    set y0 [expr ([winfo screenheight $path] - $h)/2 - [winfo vrooty $path]]
                }
                set x "+$x0"
                set y "+$y0"
                if { $x0+$w > $sw } {set x "-0"; set x0 [expr {$sw-$w}]}
                if { $x0 < 0 }      {set x "+0"}
                if { $y0+$h > $sh } {set y "-0"; set y0 [expr {$sh-$h}]}
                if { $y0 < 0 }      {set y "+0"}
            } else {
                set x0 [winfo rootx $widget]
                set y0 [winfo rooty $widget]
                set x1 [expr {$x0 + [winfo width  $widget]}]
                set y1 [expr {$y0 + [winfo height $widget]}]
                if { $idx == 2 || $idx == 3 } {
                    set y "+$y0"
                    if { $y0+$h > $sh } {set y "-0"; set y0 [expr {$sh-$h}]}
                    if { $y0 < 0 }      {set y "+0"}
                    if { $idx == 2 } {
                        # try left, then right if out, then 0 if out
                        if { $x0 >= $w } {
                            set x [expr {$x0-$sw}]
                        } elseif { $x1+$w <= $sw } {
                            set x "+$x1"
                        } else {
                            set x "+0"
                        }
                    } else {
                        # try right, then left if out, then 0 if out
                        if { $x1+$w <= $sw } {
                            set x "+$x1"
                        } elseif { $x0 >= $w } {
                            set x [expr {$x0-$sw}]
                        } else {
                            set x "-0"
                        }
                    }
                } else {
                    set x "+$x0"
                    if { $x0+$w > $sw } {set x "-0"; set x0 [expr {$sw-$w}]}
                    if { $x0 < 0 }      {set x "+0"}
                    if { $idx == 4 } {
                        # try top, then bottom, then 0
                        if { $h <= $y0 } {
                            set y [expr {$y0-$sh}]
                        } elseif { $y1+$h <= $sh } {
                            set y "+$y1"
                        } else {
                            set y "+0"
                        }
                    } else {
                        # try bottom, then top, then 0
                        if { $y1+$h <= $sh } {
                            set y "+$y1"
                        } elseif { $h <= $y0 } {
                            set y [expr {$y0-$sh}]
                        } else {
                            set y "-0"
                        }
                    }
                }
            }
        }
        wm geometry $path "${w}x${h}${x}${y}"
    } else {
        wm geometry $path "${w}x${h}"
    }
    update idletasks
}


# ------------------------------------------------------------------------------
#  Command BWidget::grab
# ------------------------------------------------------------------------------
proc BWidget::grab { option path } {
    variable _gstack

    if { $option == "release" } {
        catch {::grab release $path}
        while { [llength $_gstack] } {
            set grinfo  [lindex $_gstack end]
            set _gstack [lreplace $_gstack end end]
            foreach {oldg mode} $grinfo {
                if { [string compare $oldg $path] && [winfo exists $oldg] } {
                    if { $mode == "global" } {
                        catch {::grab -global $oldg}
                    } else {
                        catch {::grab $oldg}
                    }
                    return
                }
            }
        }
    } else {
        set oldg [::grab current]
        if { $oldg != "" } {
            lappend _gstack [list $oldg [::grab status $oldg]]
        }
        if { $option == "global" } {
            ::grab -global $path
        } else {
            ::grab $path
        }
    }
}


# ------------------------------------------------------------------------------
#  Command BWidget::focus
# ------------------------------------------------------------------------------
proc BWidget::focus { option path } {
    variable _fstack

    if { $option == "release" } {
        while { [llength $_fstack] } {
            set oldf [lindex $_fstack end]
            set _fstack [lreplace $_fstack end end]
            if { [string compare $oldf $path] && [winfo exists $oldf] } {
                catch {::focus -force $oldf}
                return
            }
        }
    } elseif { $option == "set" } {
        lappend _fstack [::focus]
        ::focus -force $path
    }
}
