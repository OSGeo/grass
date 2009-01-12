# ------------------------------------------------------------------------------
#  panedw.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#     - PanedWindow::create
#     - PanedWindow::configure
#     - PanedWindow::cget
#     - PanedWindow::add
#     - PanedWindow::getframe
#     - PanedWindow::_destroy
#     - PanedWindow::_beg_move_sash
#     - PanedWindow::_move_sash
#     - PanedWindow::_end_move_sash
#     - PanedWindow::_realize
# ------------------------------------------------------------------------------

namespace eval PanedWindow {
    namespace eval Pane {
        Widget::declare PanedWindow::Pane {
            {-minsize Int 0 0 {=0}}
            {-weight  Int 1 0 {=0}}
        }
    }

    Widget::declare PanedWindow {
        {-side       Enum       top 1 {top left bottom right}}
        {-width      Int        10  1 {=6 ""}}
        {-pad        Int        4   1 {=0 ""}}
        {-background TkResource ""  0 frame}
        {-bg         Synonym    -background}
    }

    variable _panedw

    proc ::PanedWindow { path args } { return [eval PanedWindow::create $path $args] }
    proc use {} {}
}



# ------------------------------------------------------------------------------
#  Command PanedWindow::create
# ------------------------------------------------------------------------------
proc PanedWindow::create { path args } {
    variable _panedw

    Widget::init PanedWindow $path $args

    frame $path -background [Widget::getoption $path -background]
    set _panedw($path,nbpanes) 0

    bind $path <Configure> "PanedWindow::_realize $path %w %h"
    bind $path <Destroy>   "PanedWindow::_destroy $path"

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval PanedWindow::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::configure
# ------------------------------------------------------------------------------
proc PanedWindow::configure { path args } {
    variable _panedw

    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -background bg] && $_panedw($path,nbpanes) > 0 } {
        $path:cmd configure -background $bg
        $path.f0 configure -background $bg
        for {set i 1} {$i < $_panedw($path,nbpanes)} {incr i} {
            set frame $path.sash$i
            $frame configure -background $bg
            $frame.sep configure -background $bg
            $frame.but configure -background $bg
            $path.f$i configure -background $bg
            $path.f$i.frame configure -background $bg
        }
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::cget
# ------------------------------------------------------------------------------
proc PanedWindow::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::add
# ------------------------------------------------------------------------------
proc PanedWindow::add { path args } {
    variable _panedw

    set num $_panedw($path,nbpanes)
    Widget::init PanedWindow::Pane $path.f$num $args
    set bg [Widget::getoption $path -background]

    set wbut  [Widget::getoption $path -width]
    set pad   [Widget::getoption $path -pad]
    set width [expr {$wbut+2*$pad}]
    set side  [Widget::getoption $path -side]
    if { $num > 0 } {
        set frame [frame $path.sash$num -relief flat -bd 0 -highlightthickness 0 \
                       -width $width -height $width -bg $bg]
        set sep   [frame $frame.sep -bd 1 -relief raised -highlightthickness 0 -bg $bg]
        set but   [frame $frame.but -bd 1 -relief raised -highlightthickness 0 -bg $bg \
                       -width $wbut -height $wbut]
        if { ![string compare $side "top"] || ![string compare $side "bottom"] } {
            place $sep -relx 0.5 -y 0 -width 2 -relheight 1.0 -anchor n
            if { ![string compare $side "top"] } {
                place $but -relx 0.5 -y [expr {6+$wbut/2}] -anchor c
            } else {
                place $but -relx 0.5 -rely 1.0 -y [expr {-6-$wbut/2}] -anchor c
            }
            $but configure -cursor sb_h_double_arrow 
            grid $frame -column [expr 2*$num-1] -row 0 -sticky ns
            grid columnconfigure $path [expr 2*$num-1] -weight 0
        } else {
            place $sep -x 0 -rely 0.5 -height 2 -relwidth 1.0 -anchor w
            if { ![string compare $side "left"] } {
                place $but -rely 0.5 -x [expr {6+$wbut/2}] -anchor c
            } else {
                place $but -rely 0.5 -relx 1.0 -x [expr {-6-$wbut/2}] -anchor c
            }
            $but configure -cursor sb_v_double_arrow 
            grid $frame -row [expr 2*$num-1] -column 0 -sticky ew
            grid rowconfigure $path [expr 2*$num-1] -weight 0
        }
        bind $but <ButtonPress-1> "PanedWindow::_beg_move_sash $path $num %X %Y"
    } else {
        if { ![string compare $side "top"] || ![string compare $side "bottom"] } {
            grid rowconfigure $path 0 -weight 1
        } else {
            grid columnconfigure $path 0 -weight 1
        }
    }

    set pane [frame $path.f$num -bd 0 -relief flat -highlightthickness 0 -bg $bg]
    set user [frame $path.f$num.frame  -bd 0 -relief flat -highlightthickness 0 -bg $bg]
    if { ![string compare $side "top"] || ![string compare $side "bottom"] } {
        grid $pane -column [expr 2*$num] -row 0 -sticky nsew
        grid columnconfigure $path [expr 2*$num] \
            -weight  [Widget::getoption $path.f$num -weight]
    } else {
        grid $pane -row [expr 2*$num] -column 0 -sticky nsew
        grid rowconfigure $path [expr 2*$num] \
            -weight  [Widget::getoption $path.f$num -weight]
    }
    pack $user -fill both -expand yes
    incr _panedw($path,nbpanes)

    return $user
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::getframe
# ------------------------------------------------------------------------------
proc PanedWindow::getframe { path index } {
    if { [winfo exists $path.f$index.frame] } {
        return $path.f$index.frame
    }
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::_destroy
# ------------------------------------------------------------------------------
proc PanedWindow::_destroy { path } {
    variable _panedw

    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
        Widget::destroy $path.f$i
    }
    unset _panedw($path,nbpanes)
    Widget::destroy $path
    rename $path {}
}
    

# ------------------------------------------------------------------------------
#  Command PanedWindow::_beg_move_sash
# ------------------------------------------------------------------------------
proc PanedWindow::_beg_move_sash { path num x y } {
    variable _panedw

    set fprev $path.f[expr $num-1]
    set fnext $path.f$num
    set wsash [expr [Widget::getoption $path -width] + 2*[Widget::getoption $path -pad]]

    $path.sash$num.but configure -relief sunken
    set top  [toplevel $path.sash -borderwidth 1 -relief raised]

    set minszg [Widget::getoption $fprev -minsize]
    set minszd [Widget::getoption $fnext -minsize]
    set side   [Widget::getoption $path -side]

    if { ![string compare $side "top"] || ![string compare $side "bottom"] } {
        $top configure -cursor sb_h_double_arrow
        set h    [winfo height $path]
        set yr   [winfo rooty $path.sash$num]
        set xmin [expr $wsash/2+[winfo rootx $fprev]+$minszg]
        set xmax [expr -$wsash/2-1+[winfo rootx $fnext]+[winfo width $fnext]-$minszd]
        wm overrideredirect $top 1
        wm geom $top "2x${h}+$x+$yr"

        update idletasks
        grab set $top
        bind $top <ButtonRelease-1> "PanedWindow::_end_move_sash $path $top $num $xmin $xmax %X rootx width"
        bind $top <Motion>          "PanedWindow::_move_sash $top $xmin $xmax %X +%%d+$yr"
        _move_sash $top $xmin $xmax $x "+%d+$yr"
    } else {
        $top configure -cursor sb_v_double_arrow
        set w    [winfo width $path]
        set xr   [winfo rootx $path.sash$num]
        set ymin [expr $wsash/2+[winfo rooty $fprev]+$minszg]
        set ymax [expr -$wsash/2-1+[winfo rooty $fnext]+[winfo height $fnext]-$minszd]
        wm overrideredirect $top 1
        wm geom $top "${w}x2+$xr+$y"

        update idletasks
        grab set $top
        bind $top <ButtonRelease-1> "PanedWindow::_end_move_sash $path $top $num $ymin $ymax %Y rooty height"
        bind $top <Motion>          "PanedWindow::_move_sash $top $ymin $ymax %Y +$xr+%%d"
        _move_sash $top $ymin $ymax $y "+$xr+%d"
    }
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::_move_sash
# ------------------------------------------------------------------------------
proc PanedWindow::_move_sash { top min max v form } {

    if { $v < $min } {
	set v $min
    } elseif { $v > $max } {
	set v $max
    }
    wm geom $top [format $form $v]
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::_end_move_sash
# ------------------------------------------------------------------------------
proc PanedWindow::_end_move_sash { path top num min max v rootv size } {
    variable _panedw

    destroy $top
    if { $v < $min } {
	set v $min
    } elseif { $v > $max } {
	set v $max
    }
    set fprev $path.f[expr $num-1]
    set fnext $path.f$num

    $path.sash$num.but configure -relief raised

    set wsash [expr [Widget::getoption $path -width] + 2*[Widget::getoption $path -pad]]
    set dv    [expr $v-[winfo $rootv $path.sash$num]-$wsash/2]
    set w1    [winfo $size $fprev]
    set w2    [winfo $size $fnext]

    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
        if { $i == $num-1} {
            $fprev configure -$size [expr [winfo $size $fprev]+$dv]
        } elseif { $i == $num } {
            $fnext configure -$size [expr [winfo $size $fnext]-$dv]
        } else {
            $path.f$i configure -$size [winfo $size $path.f$i]
        }
    }
}


# ------------------------------------------------------------------------------
#  Command PanedWindow::_realize
# ------------------------------------------------------------------------------
proc PanedWindow::_realize { path width height } {
    variable _panedw

    set x    0
    set y    0
    set hc   [winfo reqheight $path]
    set hmax 0
    for {set i 0} {$i < $_panedw($path,nbpanes)} {incr i} {
        $path.f$i configure \
            -width  [winfo reqwidth  $path.f$i.frame] \
            -height [winfo reqheight $path.f$i.frame]
        place $path.f$i.frame -x 0 -y 0 -relwidth 1 -relheight 1
    }

    bind $path <Configure> {}
}
