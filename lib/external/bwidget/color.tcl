# ------------------------------------------------------------------------------
#  color.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#     - SelectColor::create
#     - SelectColor::setcolor
#     - SelectColor::_destroy
#     - SelectColor::_update_var
#     - SelectColor::_post_menu
#     - SelectColor::_tk_choose_color
#     - SelectColor::_activate
# ------------------------------------------------------------------------------

namespace eval SelectColor {
    Widget::declare SelectColor {
        {-title    String     "" 0}
        {-parent   String     "" 0}
        {-type     Enum       dialog 1 {dialog menubutton}}
        {-command  String     ""     0}
        {-color    TkResource ""     0 {label -background}}
        {-variable String     ""     0}
        {-width    TkResource 15     0 frame}
        {-height   TkResource 15     0 frame}
    }

    Widget::addmap      SelectColor "" :cmd {-width {} -height {}}
    Widget::syncoptions SelectColor "" :cmd {-color -background}

    variable _tabcolors {
        \#0000ff \#000099 \#000000 white
        \#00ff00 \#009900 \#333333 white
        \#00ffff \#009999 \#666666 white
        \#ff0000 \#990000 \#999999 white
        \#ff00ff \#990099 \#cccccc white
        \#ffff00 \#999900 \#ffffff
    }

    # bindings
    bind SelectColor <ButtonPress-1> {SelectColor::_post_menu %W %X %Y}
    bind SelectColor <Destroy>       {SelectColor::_destroy %W}

    variable _widget

    proc ::SelectColor { path args } { return [eval SelectColor::create $path $args] }
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command SelectColor::create
# ------------------------------------------------------------------------------
proc SelectColor::create { path args } {
    variable _tabcolors
    variable _widget

    Widget::init SelectColor $path $args

    if { ![string compare [Widget::getoption $path -type] "menubutton"] } {
        if { [set var [Widget::getoption $path -variable]] != "" } {
            set _widget($path,var) $var
            if { [GlobalVar::exists $var] } {
                Widget::setoption $path -color [GlobalVar::getvar $var]
            } else {
                GlobalVar::setvar $var [Widget::getoption $path -color]
            }
            GlobalVar::tracevar variable $var w "SelectColor::_update_var $path"
        } else {
            set _widget($path,var) ""
        }

        eval frame $path [Widget::subcget $path :cmd] \
            -background [Widget::getoption $path -color] \
            -relief raised -borderwidth 2 -highlightthickness 0
        bindtags $path [list $path SelectColor . all]
        set _widget($path,idx) 0

        rename $path ::$path:cmd
        proc ::$path { cmd args } "return \[eval SelectColor::\$cmd $path \$args\]"
    } else {
        set parent [Widget::getoption $path -parent]
        set title  [Widget::getoption $path -title]
        set lopt   [list -initialcolor [Widget::getoption $path -color]]
        if { [winfo exists $parent] } {
            lappend lopt -parent $parent
        }
        if { $title != "" } {
            lappend lopt -title $title
        }
        set col [eval tk_chooseColor $lopt]
        Widget::destroy $path
        return $col
    }

    return $path
}


# ------------------------------------------------------------------------------
#  Command SelectColor::configure
# ------------------------------------------------------------------------------
proc SelectColor::configure { path args } {
    variable _widget

    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -variable var] } {
        if { [string length $_widget($path,var)] } {
            GlobalVar::tracevar vdelete $_widget($path,var) w "SelectColor::_update_var $path"
        }
        set _widget($path,var) $var
        if { [string length $_widget($path,var)] } {
            Widget::hasChanged $path -color curval
            if { [GlobalVar::exists $_widget($path,var)] } {
                Widget::setoption $path -color [set curval [GlobalVar::getvar $_widget($path,var)]]
            } else {
                GlobalVar::setvar $_widget($path,var) $curval
            }
            GlobalVar::tracevar variable $_widget($path,var) w "SelectColor::_update_var $path"
            $path:cmd configure -background $curval
        }
    }

    if { [Widget::hasChanged $path -color curval] } {
        if { [string length $_widget($path,var)] } {
            Widget::setoption $path -color [GlobalVar::getvar $_widget($path,var)]
        } else {
            $path:cmd configure -background $curval
        }
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command SelectColor::cget
# ------------------------------------------------------------------------------
proc SelectColor::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command SelectColor::setcolor
# ------------------------------------------------------------------------------
proc SelectColor::setcolor { index color } {
    variable _tabcolors
    variable _widget

    if { $index >= 1 && $index <= 5 } {
        set idx        [expr {int($idx) * 3}]
        set _tabcolors [lreplace $_tabcolors $idx $idx $color]
        return 1
    }
    return 0
}


# ------------------------------------------------------------------------------
#  Command SelectColor::_destroy
# ------------------------------------------------------------------------------
proc SelectColor::_destroy { path } {
    variable _widget

    if { [string length $_widget($path,var)] } {
        GlobalVar::tracevar vdelete $_widget($path,var) w "SelectColor::_update_var $path"
    }
    unset _widget($path,var)
    unset _widget($path,idx)
    Widget::destroy $path
    rename $path {}
}


# ------------------------------------------------------------------------------
#  Command SelectColor::_update_var
# ------------------------------------------------------------------------------
proc SelectColor::_update_var { path args } {
    variable _tabcolors
    variable _widget

    set col [GlobalVar::getvar $_widget($path,var)]
    $path:cmd configure -background $col
    Widget::setoption $path -color $col
    set _widget($path,idx) [lsearch $_tabcolors $col]
    if { $_widget($path,idx) == -1 } {
        set _widget($path,idx) 0
    }
}


# ------------------------------------------------------------------------------
#  Command SelectColor::_post_menu
# ------------------------------------------------------------------------------
proc SelectColor::_post_menu { path X Y } {
    global   env
    variable _tabcolors
    variable _widget

    if { [winfo exists $path.menu] } {
        if { [string compare [winfo containing $X $Y] $path] } {
            BWidget::grab release $path
            destroy $path.menu
        }
        return
    }

    set top [menu $path.menu]
    wm withdraw $top
    wm transient $top [winfo toplevel $path]
    set col 0
    set row 0
    set count 0
    set frame [frame $top.frame -highlightthickness 0 -relief raised -borderwidth 2]
    foreach color $_tabcolors {
        set f [frame $frame.c$count \
                   -relief flat -bd 0 -highlightthickness 1 \
                   -width 16 -height 16 -background $color]
        bind $f <ButtonRelease-1> "SelectColor::_activate $path %W"
        bind $f <Enter>           {focus %W}
        grid $f -column $col -row $row -padx 1 -pady 1
        bindtags $f $f
        incr row
        incr count
        if { $row == 4 } {
            set row 0
            incr col
        }
    }
    set f [label $frame.c$count \
               -relief flat -bd 0 -highlightthickness 1 \
               -width 16 -height 16 -image [Bitmap::get palette]]
    grid $f -column $col -row $row -padx 1 -pady 1
    bind $f <ButtonRelease-1> "SelectColor::_tk_choose_color $path"
    bind $f <Enter>           {focus %W}
    pack $frame

    BWidget::place $top 0 0 below $path

    wm deiconify $top
    raise $top
    focus $frame
    focus $top.frame.c$_widget($path,idx)
    BWidget::grab set $path
}


# ------------------------------------------------------------------------------
#  Command SelectColor::_tk_choose_color
# ------------------------------------------------------------------------------
proc SelectColor::_tk_choose_color { path } {
    variable _tabcolors
    variable _widget

    BWidget::grab release $path
    destroy $path.menu
    set parent [Widget::getoption $path -parent]
    set title  [Widget::getoption $path -title]
    set lopt   [list -initialcolor [$path:cmd cget -background]]
    if { [winfo exists $parent] } {
        lappend lopt -parent $parent
    }
    if { $title != "" } {
        lappend lopt -title $title
    }
    set col [eval tk_chooseColor $lopt]
    if { $col != "" } {
        if { $_widget($path,idx) % 4 == 3 } {
            set idx $_widget($path,idx)
        } else {
            set idx -1
            for {set i 3} {$i < 15} {incr i 4} {
                if { [lindex $_tabcolors $i] == "white" } {
                    set idx $i
                    break
                }
            }
        }
        if { $idx != -1 } {
            set _tabcolors [lreplace $_tabcolors $idx $idx $col]
            set _widget($path,idx) $idx
        }
        if { [info exists _widget($path,var)] } {
            GlobalVar::setvar $_widget($path,var) $col
        }
        if { [set cmd [Widget::getoption $path -command]] != "" } {
            uplevel \#0 $cmd
        }
        $path:cmd configure -background $col
    }
}


# ------------------------------------------------------------------------------
#  Command SelectColor::_activate
# ------------------------------------------------------------------------------
proc SelectColor::_activate { path cell } {
    variable _tabcolors
    variable _widget

    BWidget::grab release $path
    set col [$cell cget -background]
    destroy $path.menu
    if { [string length $_widget($path,var)] } {
        GlobalVar::setvar $_widget($path,var) $col
    }
    Widget::setoption $path -color $col
    $path:cmd configure -background $col

    if { [set cmd [Widget::getoption $path -command]] != "" } {
        uplevel \#0 $cmd
    }
    set _widget($path,idx) [string range [lindex [split $cell "."] end] 1 end]
}
