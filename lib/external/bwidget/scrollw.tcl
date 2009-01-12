# ------------------------------------------------------------------------------
#  scrollw.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - ScrolledWindow::create
#     - ScrolledWindow::getframe
#     - ScrolledWindow::setwidget
#     - ScrolledWindow::configure
#     - ScrolledWindow::cget
#     - ScrolledWindow::_set_hscroll
#     - ScrolledWindow::_set_vscroll
#     - ScrolledWindow::_realize
# ------------------------------------------------------------------------------

namespace eval ScrolledWindow {
    Widget::declare ScrolledWindow {
        {-background  TkResource ""   0 button}
        {-scrollbar   Enum       both 1 {none both vertical horizontal}}
        {-auto        Enum       both 0 {none both vertical horizontal}}
        {-relief      TkResource flat 0 frame}
        {-borderwidth TkResource 0    0 frame}
        {-bg          Synonym    -background}
        {-bd          Synonym    -borderwidth}
    }

    Widget::addmap ScrolledWindow "" ._grid.f {-relief {} -borderwidth {}}

    proc ::ScrolledWindow { path args } { return [eval ScrolledWindow::create $path $args] }
    proc use {} {}

    variable _widget
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::create
# ------------------------------------------------------------------------------
proc ScrolledWindow::create { path args } {
    variable _widget

    Widget::init ScrolledWindow $path $args

    set bg     [Widget::cget $path -background]
    set sw     [frame $path -relief flat -bd 0 -bg $bg -highlightthickness 0 -takefocus 0]
    set grid   [frame $path._grid -relief flat -bd 0 -bg $bg -highlightthickness 0 -takefocus 0]

    set sb    [lsearch {none horizontal vertical both} [Widget::cget $path -scrollbar]]
    set auto  [lsearch {none horizontal vertical both} [Widget::cget $path -auto]]
    set rspan [expr {1 + !($sb & 1)}]
    set cspan [expr {1 + !($sb & 2)}]

    set _widget($path,realized) 0
    set _widget($path,sb)       $sb
    set _widget($path,auto)     $auto
    set _widget($path,hpack)    [expr {$rspan == 1}]
    set _widget($path,vpack)    [expr {$cspan == 1}]

    # scrollbar horizontale ou les deux
    if { $sb & 1 } {
        scrollbar $grid.hscroll \
            -highlightthickness 0 -takefocus 0 \
            -orient  horiz	\
            -relief  sunken	\
            -bg      $bg
        $grid.hscroll set 0 1
        grid $grid.hscroll -column 0 -row 1 -sticky we -columnspan $cspan -pady 1
    }

    # scrollbar verticale ou les deux
    if { $sb & 2 } {
        scrollbar $grid.vscroll \
            -highlightthickness 0 -takefocus 0 \
            -orient  vert  	\
            -relief  sunken 	\
            -bg      $bg
        $grid.vscroll set 0 1
        grid $grid.vscroll -column 1 -row 0 -sticky ns -rowspan $rspan -padx 1
    }

    eval frame $grid.f -bg $bg -highlightthickness 0 [Widget::subcget $path ._grid.f]
    grid $grid.f -column 0 -row 0 -sticky nwse -columnspan $cspan -rowspan $rspan
    grid columnconfigure $grid 0 -weight 1
    grid rowconfigure    $grid 0 -weight 1
    pack $grid -fill both -expand yes

    bind $grid <Configure> "ScrolledWindow::_realize $path"
    bind $grid <Destroy>   "ScrolledWindow::_destroy $path"

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval ScrolledWindow::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::getframe
# ------------------------------------------------------------------------------
proc ScrolledWindow::getframe { path } {
    return $path
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::setwidget
# ------------------------------------------------------------------------------
proc ScrolledWindow::setwidget { path widget } {
    variable _widget

    set grid   $path._grid
    set sb     $_widget($path,sb)
    set option {}

    pack $widget -in $grid.f -fill both -expand yes

    # scrollbar horizontale ou les deux
    if { $sb & 1 } {
        $grid.hscroll configure -command "$widget xview"
        lappend option  "-xscrollcommand" "ScrolledWindow::_set_hscroll $path"
    }

    # scrollbar verticale ou les deux
    if { $sb & 2 } {
        $grid.vscroll configure -command "$widget yview"
        lappend option  "-yscrollcommand" "ScrolledWindow::_set_vscroll $path"
    }
    if { [llength $option] } {
        eval $widget configure $option
    }
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::configure
# ------------------------------------------------------------------------------
proc ScrolledWindow::configure { path args } {
    variable _widget

    set grid $path._grid
    set res [Widget::configure $path $args]
    if { [Widget::hasChanged $path -background bg] } {
        $path configure -background $bg
        $grid configure -background $bg
        $grid.f configure -background $bg
        catch {$grid.hscroll configure -background $bg}
        catch {$grid.vscroll configure -background $bg}
    }
    if { [Widget::hasChanged $path -auto auto] } {
        set _widget($path,auto) [lsearch {none horizontal vertical both} $auto]
        if { $_widget($path,sb) & 1 } {
            eval _set_hscroll $path [$grid.hscroll get]
        }
        if { $_widget($path,sb) & 2 } {
            eval _set_vscroll $path [$grid.vscroll get]
        }
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::cget
# ------------------------------------------------------------------------------
proc ScrolledWindow::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::_destroy
# ------------------------------------------------------------------------------
proc ScrolledWindow::_destroy { path } {
    variable _widget

    unset _widget($path,sb)
    unset _widget($path,auto)
    unset _widget($path,hpack)
    unset _widget($path,vpack)
    Widget::destroy $path
    rename $path {}
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::_set_hscroll
# ------------------------------------------------------------------------------
proc ScrolledWindow::_set_hscroll { path vmin vmax } {
    variable _widget

    if { $_widget($path,realized) } {
        set grid $path._grid
        if { $_widget($path,auto) & 1 } {
            if { $_widget($path,hpack) && $vmin == 0 && $vmax == 1 } {
                grid configure $grid.f -rowspan 2
                if { $_widget($path,sb) & 2 } {
                    grid configure $grid.vscroll -rowspan 2
                }
                set _widget($path,hpack) 0
            } elseif { !$_widget($path,hpack) && ($vmin != 0 || $vmax != 1) } {
                grid configure $grid.f -rowspan 1
                if { $_widget($path,sb) & 2 } {
                    grid configure $grid.vscroll -rowspan 1
                }
                set _widget($path,hpack) 1
            }
        }
        $grid.hscroll set $vmin $vmax
    }
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::_set_vscroll
# ------------------------------------------------------------------------------
proc ScrolledWindow::_set_vscroll { path vmin vmax } {
    variable _widget

    if { $_widget($path,realized) } {
        set grid $path._grid
        if { $_widget($path,auto) & 2 } {
            if { $_widget($path,vpack) && $vmin == 0 && $vmax == 1 } {
                grid configure $grid.f -columnspan 2
                if { $_widget($path,sb) & 1 } {
                    grid configure $grid.hscroll -columnspan 2
                }
                set _widget($path,vpack) 0
            } elseif { !$_widget($path,vpack) && ($vmin != 0 || $vmax != 1) } {
                grid configure $grid.f -columnspan 1
                if { $_widget($path,sb) & 1 } {
                    grid configure $grid.hscroll -columnspan 1
                }
                set _widget($path,vpack) 1
            }
        }
        $grid.vscroll set $vmin $vmax
    }
}


# ------------------------------------------------------------------------------
#  Command ScrolledWindow::_realize
# ------------------------------------------------------------------------------
proc ScrolledWindow::_realize { path } {
    variable _widget

    set grid $path._grid
    bind  $grid <Configure> {}
    set _widget($path,realized) 1
    place $grid -anchor nw -x 0 -y 0 -relwidth 1.0 -relheight 1.0
}


