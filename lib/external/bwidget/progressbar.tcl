# ------------------------------------------------------------------------------
#  progressbar.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#     - ProgressBar::create
#     - ProgressBar::configure
#     - ProgressBar::cget
#     - ProgressBar::_destroy
#     - ProgressBar::_modify
# ------------------------------------------------------------------------------

namespace eval ProgressBar {
    Widget::declare ProgressBar {
        {-type        Enum       normal     0 {normal incremental infinite}}
        {-maximum     Int        100        0 {>0}}
        {-background  TkResource ""         0 frame}
        {-foreground  TkResource blue       0 label}
        {-borderwidth TkResource 2          0 frame}
        {-troughcolor TkResource ""         0 scrollbar}
        {-relief      TkResource sunken     0 label}
        {-orient      Enum       horizontal 1 {horizontal vertical}}
        {-variable    String     ""         0}
        {-width       TkResource 100        0 frame}
        {-height      TkResource 4m         0 frame}
        {-bg          Synonym    -background}
        {-fg          Synonym    -foreground}
        {-bd          Synonym    -borderwidth}
    }

    Widget::addmap ProgressBar "" :cmd {-background {} -width {} -height {}}
    Widget::addmap ProgressBar "" .bar {-troughcolor -background -borderwidth {} -relief {}}

    variable _widget

    proc ::ProgressBar { path args } { return [eval ProgressBar::create $path $args] }
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command ProgressBar::create
# ------------------------------------------------------------------------------
proc ProgressBar::create { path args } {
    variable _widget

    Widget::init ProgressBar $path $args

    eval frame $path [Widget::subcget $path :cmd]
    set c  [eval canvas $path.bar [Widget::subcget $path .bar] -highlightthickness 0]
    set fg [Widget::getoption $path -foreground]
    if { ![string compare [Widget::getoption $path -orient] "horizontal"] } {
        $path.bar create rectangle -1 0 0 0 -fill $fg -outline $fg -tags rect
    } else {
        $path.bar create rectangle 0 1 0 0 -fill $fg -outline $fg -tags rect
    }

    set _widget($path,val) 0
    set _widget($path,dir) 1
    if { [set _widget($path,var) [Widget::getoption $path -variable]] != "" } {
        GlobalVar::tracevar variable $_widget($path,var) w "ProgressBar::_modify $path"
        after idle ProgressBar::_modify $path
    }

    bind $path.bar <Destroy>   "ProgressBar::_destroy $path"
    bind $path.bar <Configure> "ProgressBar::_modify $path"

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval ProgressBar::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command ProgressBar::configure
# ------------------------------------------------------------------------------
proc ProgressBar::configure { path args } {
    variable _widget

    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -variable newv] } {
        if { $_widget($path,var) != "" } {
            GlobalVar::tracevar vdelete $_widget($path,var) w "ProgressBar::_modify $path"
        }
        if { $newv != "" } {
            set _widget($path,var) $newv
            GlobalVar::tracevar variable $newv w "ProgressBar::_modify $path"
            after idle ProgressBar::_modify $path
        } else {
            set _widget($path,var) ""
        }
    }

    if { [Widget::hasChanged $path -borderwidth v] ||
         [Widget::hasChanged $path -orient v] } {
        after idle ProgressBar::_modify $path
    }
    if { [Widget::hasChanged $path -foreground fg] } {
        $path.bar itemconfigure rect -fill $fg -outline $fg
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command ProgressBar::cget
# ------------------------------------------------------------------------------
proc ProgressBar::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command ProgressBar::_destroy
# ------------------------------------------------------------------------------
proc ProgressBar::_destroy { path } {
    variable _widget

    if { $_widget($path,var) != "" } {
        GlobalVar::tracevar vdelete $_widget($path,var) w "ProgressBar::_modify $path"
    }
    unset _widget($path,var)
    unset _widget($path,dir)
    Widget::destroy $path
    rename $path {}
}


# ------------------------------------------------------------------------------
#  Command ProgressBar::_modify
# ------------------------------------------------------------------------------
proc ProgressBar::_modify { path args } {
    variable _widget

    if { ![GlobalVar::exists $_widget($path,var)] ||
         [set val [GlobalVar::getvar $_widget($path,var)]] < 0 } {
        catch {place forget $path.bar}
    } else {
        place $path.bar -relx 0 -rely 0 -relwidth 1 -relheight 1
        set type [Widget::getoption $path -type]
        if { $val != 0 && [string compare $type "normal"] } {
            set val [expr {$val+$_widget($path,val)}]
        }
        set _widget($path,val) $val
        set max [Widget::getoption $path -maximum]
        set bd  [expr {2*[$path.bar cget -bd]}]
        set w   [winfo width  $path.bar]
        set h   [winfo height $path.bar]
        if { ![string compare $type "infinite"] } {
            if { $val > $max } {
                set _widget($path,dir) [expr {-$_widget($path,dir)}]
                set val 0
                set _widget($path,val) 0
            }
            if { $val <= $max/2.0 } {
                set dx0 0.0
                set dx1 [expr {double($val)/$max}]
            } else {
                set dx1 [expr {double($val)/$max}]
                set dx0 [expr {$dx1-0.5}]
            }
            if { $_widget($path,dir) == 1 } {
                set x0 $dx0
                set x1 $dx1
            } else {
                set x0 [expr {1-$dx1}]
                set x1 [expr {1-$dx0}]
            }
            if { ![string compare [Widget::getoption $path -orient] "horizontal"] } {
                $path.bar coords rect [expr {$x0*$w}] 0 [expr {$x1*$w}] $h
            } else {
                $path.bar coords rect 0 [expr {$h-$x0*$h}] $w [expr {$x1*$h}]
            }
        } else {
            if { $val > $max } {set val $max}
            if { ![string compare [Widget::getoption $path -orient] "horizontal"] } {
                $path.bar coords rect -1 0 [expr {$val*$w/$max}] $h
            } else {
                $path.bar coords rect 0 [expr {$h+1}] $w [expr {$h*($max-$val)}]
            }
        }
    }
    update
}
