# ------------------------------------------------------------------------------
#  dialog.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - Dialog::create
#     - Dialog::configure
#     - Dialog::cget
#     - Dialog::getframe
#     - Dialog::add
#     - Dialog::itemconfigure
#     - Dialog::itemcget
#     - Dialog::invoke
#     - Dialog::setfocus
#     - Dialog::enddialog
#     - Dialog::draw
#     - Dialog::withdraw
#     - Dialog::_destroy
# ------------------------------------------------------------------------------

namespace eval Dialog {
    ButtonBox::use

    Widget::bwinclude Dialog ButtonBox .bbox \
        remove     {-orient} \
        initialize {-spacing 10 -padx 10}

    Widget::declare Dialog {
        {-title       String     ""       0}
        {-modal       Enum       local    0 {none local global}}
        {-bitmap      TkResource ""       1 label}
        {-image       TkResource ""       1 label}
        {-separator   Boolean    0        1}
        {-cancel      Int        -1       0 {=-1 ""}}
        {-parent      String     ""       0}
        {-side        Enum       bottom   1 {bottom left top right}}
        {-anchor      Enum       c        1 {n e w s c}}
    }

    Widget::addmap Dialog "" :cmd   {-background {}}
    Widget::addmap Dialog "" .frame {-background {}}

    proc ::Dialog { path args } { return [eval Dialog::create $path $args] }
    proc use {} {}

    bind BwDialog <Destroy> {Dialog::enddialog %W -1; Dialog::_destroy %W}

    variable _widget
}


# ------------------------------------------------------------------------------
#  Command Dialog::create
# ------------------------------------------------------------------------------
proc Dialog::create { path args } {
    global   tcl_platform
    variable _widget

    Widget::init Dialog $path $args
    set bg [Widget::getoption $path -background]
    if { ![string compare $tcl_platform(platform) "unix"] } {
        toplevel $path -relief raised -borderwidth 1 -background $bg
    } else {
        toplevel $path -relief flat   -borderwidth 0 -background $bg
    }
    bindtags $path [list $path BwDialog all]
    wm overrideredirect $path 1
    wm title $path [Widget::getoption $path -title]
    set parent [Widget::getoption $path -parent]
    if { ![winfo exists $parent] } {
        set parent [winfo parent $path]
    }
    wm transient $path [winfo toplevel $parent]
    wm withdraw $path

    set side [Widget::getoption $path -side]
    if { ![string compare $side "left"] || ![string compare $side "right"] } {
        set orient vertical
    } else {
        set orient horizontal
    }

    set bbox  [eval ButtonBox::create $path.bbox [Widget::subcget $path .bbox] \
                   -orient $orient]
    set frame [frame $path.frame -relief flat -borderwidth 0 -background $bg]

    if { [set bitmap [Widget::getoption $path -image]] != "" } {
        set label [label $path.label -image $bitmap -background $bg]
    } elseif { [set bitmap [Widget::getoption $path -bitmap]] != "" } {
        set label [label $path.label -bitmap $bitmap -background $bg]
    }
    if { [Widget::getoption $path -separator] } {
                Separator::create $path.sep -orient $orient -background $bg
    }
    set _widget($path,realized) 0
    set _widget($path,nbut)     0

    bind $path <Escape>  "ButtonBox::invoke $path.bbox [Widget::getoption $path -cancel]"
    bind $path <Return>  "ButtonBox::invoke $path.bbox default"

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval Dialog::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command Dialog::configure
# ------------------------------------------------------------------------------
proc Dialog::configure { path args } {
    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -title title] } {
        wm title $path $title
    }
    if { [Widget::hasChanged $path -background bg] } {
        if { [winfo exists $path.label] } {
            $path.label configure -background $bg
        }
        if { [winfo exists $path.sep] } {
            Separator::configure $path.sep -background $bg
        }
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command Dialog::cget
# ------------------------------------------------------------------------------
proc Dialog::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command Dialog::getframe
# ------------------------------------------------------------------------------
proc Dialog::getframe { path } {
    return $path.frame
}


# ------------------------------------------------------------------------------
#  Command Dialog::add
# ------------------------------------------------------------------------------
proc Dialog::add { path args } {
    variable _widget

    set res [eval ButtonBox::add $path.bbox \
                 -command [list "Dialog::enddialog $path $_widget($path,nbut)"] $args]
    incr _widget($path,nbut)
    return $res
}


# ------------------------------------------------------------------------------
#  Command Dialog::itemconfigure
# ------------------------------------------------------------------------------
proc Dialog::itemconfigure { path index args } {
    return [eval ButtonBox::itemconfigure $path.bbox $index $args]
}


# ------------------------------------------------------------------------------
#  Command Dialog::itemcget
# ------------------------------------------------------------------------------
proc Dialog::itemcget { path index option } {
    return [ButtonBox::itemcget $path.bbox $index $option]
}


# ------------------------------------------------------------------------------
#  Command Dialog::invoke
# ------------------------------------------------------------------------------
proc Dialog::invoke { path index } {
    ButtonBox::invoke $path.bbox $index
}


# ------------------------------------------------------------------------------
#  Command Dialog::setfocus
# ------------------------------------------------------------------------------
proc Dialog::setfocus { path index } {
    ButtonBox::setfocus $path.bbox $index
}


# ------------------------------------------------------------------------------
#  Command Dialog::enddialog
# ------------------------------------------------------------------------------
proc Dialog::enddialog { path result } {
    variable _widget

    set _widget($path,result) $result
}


# ------------------------------------------------------------------------------
#  Command Dialog::draw
# ------------------------------------------------------------------------------
proc Dialog::draw { path {focus ""}} {
    variable _widget

    set parent [Widget::getoption $path -parent]
    if { !$_widget($path,realized) } {
        set _widget($path,realized) 1
        if { [llength [winfo children $path.bbox]] } {
            set side [Widget::getoption $path -side]
            if { ![string compare $side "left"] || ![string compare $side "right"] } {
                set pad  -padx
                set fill y
            } else {
                set pad  -pady
                set fill x
            }
            pack $path.bbox -side $side -anchor [Widget::getoption $path -anchor] -padx 1m -pady 1m
            if { [winfo exists $path.sep] } {
                pack $path.sep -side $side -fill $fill $pad 2m
            }
        }
        if { [winfo exists $path.label] } {
            pack $path.label -side left -anchor n -padx 3m -pady 3m
        }
        pack $path.frame -padx 1m -pady 1m -fill both -expand yes
    }

    if { [winfo exists $parent] } {
        BWidget::place $path 0 0 center $parent
    } else {
        BWidget::place $path 0 0 center
    }
    update idletasks
    wm overrideredirect $path 0
    wm deiconify $path

    tkwait visibility $path
    BWidget::focus set $path
    if { [winfo exists $focus] } {
        focus -force $focus
    } else {
        ButtonBox::setfocus $path.bbox default
    }

    if { [set grab [Widget::getoption $path -modal]] != "none" } {
        BWidget::grab $grab $path
        catch {unset _widget($path,result)}
        tkwait variable Dialog::_widget($path,result)
        if { [info exists _widget($path,result)] } {
            set res $_widget($path,result)
            unset _widget($path,result)
        } else {
            set res -1
        }
        withdraw $path
        return $res
    }
    return ""
}


# ------------------------------------------------------------------------------
#  Command Dialog::withdraw
# ------------------------------------------------------------------------------
proc Dialog::withdraw { path } {
    BWidget::grab release $path
    BWidget::focus release $path
    if { [winfo exists $path] } {
        wm withdraw $path
    }
}


# ------------------------------------------------------------------------------
#  Command Dialog::_destroy
# ------------------------------------------------------------------------------
proc Dialog::_destroy { path } {
    variable _widget

    BWidget::grab  release $path
    BWidget::focus release $path
    catch {unset _widget($path,result)}
    unset _widget($path,realized)
    unset _widget($path,nbut)

    Widget::destroy $path
    rename $path {}
}
