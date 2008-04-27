# ------------------------------------------------------------------------------
#  scrollview.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - ScrolledWindow::create
#     - ScrolledWindow::configure
#     - ScrolledWindow::cget
#     - ScrolledWindow::_set_hscroll
#     - ScrolledWindow::_set_vscroll
#     - ScrolledWindow::_update_scroll
#     - ScrolledWindow::_set_view
#     - ScrolledWindow::_resize
# ------------------------------------------------------------------------------

namespace eval ScrollView {
    Widget::declare ScrollView {
        {-width       TkResource 30        0 canvas}
        {-height      TkResource 30        0 canvas}
        {-background  TkResource ""        0 canvas}
        {-foreground  String     black     0}
        {-fill        String     ""        0}
        {-relief      TkResource flat      0 canvas}
        {-borderwidth TkResource 0         0 canvas}
        {-cursor      TkResource crosshair 0 canvas}
        {-window      String     ""        0}
        {-fg          Synonym    -foreground}
        {-bg          Synonym    -background}
        {-bd          Synonym    -borderwidth}
    }

    Widget::addmap ScrollView "" :cmd \
        {-relief {} -borderwidth {} -background {} -width {} -height {} -cursor {}}

    bind BwScrollView <ButtonPress-3> {ScrollView::_set_view %W set %x %y}
    bind BwScrollView <ButtonPress-1> {ScrollView::_set_view %W start %x %y}
    bind BwScrollView <B1-Motion>     {ScrollView::_set_view %W motion %x %y}
    bind BwScrollView <Configure>     {ScrollView::_resize %W}
    bind BwScrollView <Destroy>       {ScrollView::_destroy %W}

    proc ::ScrollView { path args } { return [eval ScrollView::create $path $args] }
    proc use {} {}

    variable _widget
}


# ------------------------------------------------------------------------------
#  Command ScrollView::create
# ------------------------------------------------------------------------------
proc ScrollView::create { path args } {
    variable _widget

    Widget::init ScrollView $path $args

    set w                     [Widget::getoption $path -window]
    set _widget($path,bd)     [Widget::getoption $path -borderwidth]
    set _widget($path,width)  [Widget::getoption $path -width]
    set _widget($path,height) [Widget::getoption $path -height]

    if { [winfo exists $w] } {
        set _widget($path,oldxscroll) [$w cget -xscrollcommand]
        set _widget($path,oldyscroll) [$w cget -yscrollcommand]
        $w configure \
            -xscrollcommand "ScrollView::_set_hscroll $path" \
            -yscrollcommand "ScrollView::_set_vscroll $path"
    }
    eval canvas $path [Widget::subcget $path :cmd] -highlightthickness 0
    $path create rectangle -2 -2 -2 -2 \
        -fill    [Widget::getoption $path -fill]       \
        -outline [Widget::getoption $path -foreground] \
        -tags    view

    bindtags $path [list $path BwScrollView [winfo toplevel $path] all]

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval ScrollView::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command ScrollView::configure
# ------------------------------------------------------------------------------
proc ScrollView::configure { path args } {
    variable _widget

    set oldw [Widget::getoption $path -window] 
    set res  [Widget::configure $path $args]

    if { [Widget::hasChanged $path -window w] } {
        if { [winfo exists $oldw] } {
            $oldw configure \
                -xscrollcommand $_widget($path,oldxscroll) \
                -yscrollcommand $_widget($path,oldyscroll)
        }
        if { [winfo exists $w] } {
            set _widget($path,oldxscroll) [$w cget -xscrollcommand]
            set _widget($path,oldyscroll) [$w cget -yscrollcommand]
            $w configure \
                -xscrollcommand "ScrollView::_set_hscroll $path" \
                -yscrollcommand "ScrollView::_set_vscroll $path"
        } else {
            $path:cmd coords view -2 -2 -2 -2
            set _widget($path,oldxscroll) {}
            set _widget($path,oldyscroll) {}
        }
    }

    if { [Widget::hasChanged $path -fill fill] |
         [Widget::hasChanged $path -foreground fg] } {
        $path:cmd itemconfigure view \
            -fill    $fill \
            -outline $fg
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command ScrollView::cget
# ------------------------------------------------------------------------------
proc ScrollView::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command ScrollView::_destroy
# ------------------------------------------------------------------------------
proc ScrollView::_destroy { path } {
    variable _widget

    set w [Widget::getoption $path -window] 
    if { [winfo exists $w] } {
        $w configure \
            -xscrollcommand $_widget($path,oldxscroll) \
            -yscrollcommand $_widget($path,oldyscroll)
    }
    unset _widget($path,oldxscroll)
    unset _widget($path,oldyscroll)
    unset _widget($path,bd)
    unset _widget($path,width)
    unset _widget($path,height)
    catch {unset _widget($path,dx)}
    catch {unset _widget($path,dy)}
    Widget::destroy $path
    rename $path {}
}


# ------------------------------------------------------------------------------
#  Command ScrollView::_set_hscroll
# ------------------------------------------------------------------------------
proc ScrollView::_set_hscroll { path vmin vmax } {
    variable _widget

    set c  [$path:cmd coords view]
    set x0 [expr {$vmin*$_widget($path,width)+$_widget($path,bd)}]
    set x1 [expr {$vmax*$_widget($path,width)+$_widget($path,bd)-1}]
    $path:cmd coords view $x0 [lindex $c 1] $x1 [lindex $c 3]
    if { $_widget($path,oldxscroll) != "" } {
        uplevel \#0 $_widget($path,oldxscroll) $vmin $vmax
    }
}


# ------------------------------------------------------------------------------
#  Command ScrollView::_set_vscroll
# ------------------------------------------------------------------------------
proc ScrollView::_set_vscroll { path vmin vmax } {
    variable _widget

    set c  [$path:cmd coords view]
    set y0 [expr {$vmin*$_widget($path,height)+$_widget($path,bd)}]
    set y1 [expr {$vmax*$_widget($path,height)+$_widget($path,bd)-1}]
    $path:cmd coords view [lindex $c 0] $y0 [lindex $c 2] $y1
    if { $_widget($path,oldyscroll) != "" } {
        uplevel \#0 $_widget($path,oldyscroll) $vmin $vmax
    }
}


# ------------------------------------------------------------------------------
#  Command ScrollView::_update_scroll
# ------------------------------------------------------------------------------
proc ScrollView::_update_scroll { path callscroll hminmax vminmax } {
    variable _widget

    set c    [$path:cmd coords view]
    set hmin [lindex $hminmax 0]
    set hmax [lindex $hminmax 1]
    set vmin [lindex $vminmax 0]
    set vmax [lindex $vminmax 1]
    set x0   [expr {$hmin*$_widget($path,width)+$_widget($path,bd)}]
    set x1   [expr {$hmax*$_widget($path,width)+$_widget($path,bd)-1}]
    set y0   [expr {$vmin*$_widget($path,height)+$_widget($path,bd)}]
    set y1   [expr {$vmax*$_widget($path,height)+$_widget($path,bd)-1}]
    $path:cmd coords view $x0 $y0 $x1 $y1
    if { $callscroll } {
        if { $_widget($path,oldxscroll) != "" } {
            uplevel \#0 $_widget($path,oldxscroll) $hmin $hmax
        }
        if { $_widget($path,oldyscroll) != "" } {
            uplevel \#0 $_widget($path,oldyscroll) $vmin $vmax
        }
    }
}


# ------------------------------------------------------------------------------
#  Command ScrollView::_set_view
# ------------------------------------------------------------------------------
proc ScrollView::_set_view { path cmd x y } {
    variable _widget

    set w [Widget::getoption $path -window]
    if { [winfo exists $w] } {
        if { ![string compare $cmd "start"] } {
            set c  [$path:cmd coords view]
            set x0 [lindex $c 0]
            set y0 [lindex $c 1]
            set _widget($path,dx) [expr {$x-$x0}]
            set _widget($path,dy) [expr {$y-$y0}]
        } else {
            if { ![string compare $cmd "motion"] } {
                set vh [expr {double($x-$_widget($path,dx)-$_widget($path,bd))/$_widget($path,width)}]
                set vv [expr {double($y-$_widget($path,dy)-$_widget($path,bd))/$_widget($path,height)}]
            } else {
                set vh [expr {double($x-$_widget($path,bd))/$_widget($path,width)}]
                set vv [expr {double($y-$_widget($path,bd))/$_widget($path,height)}]
            }
            $w xview moveto $vh
            $w yview moveto $vv
            _update_scroll $path 1 [$w xview] [$w yview]
        }
    }
}


# ------------------------------------------------------------------------------
#  Command ScrollView::_resize
# ------------------------------------------------------------------------------
proc ScrollView::_resize { path } {
    variable _widget

    set _widget($path,bd)     [Widget::getoption $path -borderwidth]
    set _widget($path,width)  [expr {[winfo width  $path]-2*$_widget($path,bd)}]
    set _widget($path,height) [expr {[winfo height $path]-2*$_widget($path,bd)}]
    set w [Widget::getoption $path -window]
    if { [winfo exists $w] } {
        _update_scroll $path 0 [$w xview] [$w yview]
    }
}
