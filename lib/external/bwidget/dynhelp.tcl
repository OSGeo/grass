# ------------------------------------------------------------------------------
#  dynhelp.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - DynamicHelp::configure
#     - DynamicHelp::include
#     - DynamicHelp::sethelp
#     - DynamicHelp::register
#     - DynamicHelp::_motion_balloon
#     - DynamicHelp::_motion_info
#     - DynamicHelp::_leave_info
#     - DynamicHelp::_menu_info
#     - DynamicHelp::_show_help
#     - DynamicHelp::_init
# ------------------------------------------------------------------------------

namespace eval DynamicHelp {
    Widget::declare DynamicHelp {
        {-foreground  TkResource black         0 label}
        {-background  TkResource "#FFFFC0"     0 label}
        {-borderwidth TkResource 1             0 label}
        {-justify     TkResource left          0 label}
        {-font        TkResource "helvetica 8" 0 label}
        {-delay       Int        600           0 {=100 =2000}}
        {-bd          Synonym    -borderwidth}
        {-bg          Synonym    -background}
        {-fg          Synonym    -foreground}
    }

    proc use {} {}

    variable _registered

    variable _top     ".help_shell"
    variable _id      "" 
    variable _delay   600
    variable _current ""
    variable _saved

    Widget::init DynamicHelp $_top {}

    bind BwHelpBalloon <Enter>   {DynamicHelp::_motion_balloon enter  %W %X %Y}
    bind BwHelpBalloon <Motion>  {DynamicHelp::_motion_balloon motion %W %X %Y}
    bind BwHelpBalloon <Leave>   {DynamicHelp::_motion_balloon leave  %W %X %Y}
    bind BwHelpBalloon <Button>  {DynamicHelp::_motion_balloon button %W %X %Y}
    bind BwHelpBalloon <Destroy> {catch {unset DynamicHelp::_registered(%W)}}

    bind BwHelpVariable <Enter>   {DynamicHelp::_motion_info %W}
    bind BwHelpVariable <Motion>  {DynamicHelp::_motion_info %W}
    bind BwHelpVariable <Leave>   {DynamicHelp::_leave_info  %W}
    bind BwHelpVariable <Destroy> {catch {unset DynamicHelp::_registered(%W)}}

    bind BwHelpMenu <<MenuSelect>> {DynamicHelp::_menu_info select %W}
    bind BwHelpMenu <Unmap>        {DynamicHelp::_menu_info unmap  %W}
    bind BwHelpMenu <Destroy>      {catch {unset DynamicHelp::_registered(%W)}}
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::configure
# ------------------------------------------------------------------------------
proc DynamicHelp::configure { args } {
    variable _top
    variable _delay

    set res [Widget::configure $_top $args]
    if { [Widget::hasChanged $_top -delay val] } {
        set _delay $val
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::include
# ------------------------------------------------------------------------------
proc DynamicHelp::include { class type } {
    set helpoptions {
        {-helptext String "" 0}
        {-helpvar  String "" 0}}
    lappend helpoptions [list -helptype Enum $type 0 {balloon variable}]
    Widget::declare $class $helpoptions
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::sethelp
# ------------------------------------------------------------------------------
proc DynamicHelp::sethelp { path subpath {force 0}} {
    set ctype [Widget::hasChanged $path -helptype htype]
    set ctext [Widget::hasChanged $path -helptext htext]
    set cvar  [Widget::hasChanged $path -helpvar  hvar]
    if { $force || $ctype || $ctext || $cvar } {
        switch $htype {
            balloon {
                return [register $subpath balloon $htext]
            }
            variable {
                return [register $subpath variable $hvar $htext]
            }
        }
        return [register $subpath $htype]
    }
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::register
# ------------------------------------------------------------------------------
proc DynamicHelp::register { path type args } {
    variable _registered

    if { [winfo exists $path] } {
        set evt  [bindtags $path]
        set idx  [lsearch $evt "BwHelp*"]
        set evt  [lreplace $evt $idx $idx]
        switch $type {
            balloon {
                set text [lindex $args 0]
                if { $text != "" } {
                    set _registered($path) $text
                    lappend evt BwHelpBalloon
                } else {
                    catch {unset _registered($path)}
                }
                bindtags $path $evt
                return 1
            }

            variable {
                set var  [lindex $args 0]
                set text [lindex $args 1]
                if { $text != "" && $var != "" } {
                    set _registered($path) [list $var $text]
                    lappend evt BwHelpVariable
                } else {
                    catch {unset _registered($path)}
                }
                bindtags $path $evt
                return 1
            }

            menu {
                set cpath [BWidget::clonename $path]
                if { [winfo exists $cpath] } {
                    set path $cpath
                }
                set var [lindex $args 0]
                if { $var != "" } {
                    set _registered($path) [list $var]
                    lappend evt BwHelpMenu
                } else {
                    catch {unset _registered($path)}
                }
                bindtags $path $evt
                return 1
            }

            menuentry {
                set cpath [BWidget::clonename $path]
                if { [winfo exists $cpath] } {
                    set path $cpath
                }
                if { [info exists _registered($path)] } {
                    if { [set index [lindex $args 0]] != "" } {
                        set text  [lindex $args 1]
                        set idx   [lsearch $_registered($path) [list $index *]]
                        if { $text != "" } {
                            if { $idx == -1 } {
                                lappend _registered($path) [list $index $text]
                            } else {
                                set _registered($path) [lreplace $_registered($path) $idx $idx [list $index $text]]
                            }
                        } else {
                            set _registered($path) [lreplace $_registered($path) $idx $idx]
                        }
                    }
                    return 1
                }
                return 0
            }
        }
        catch {unset _registered($path)}
        bindtags $path $evt
        return 1
    } else {
        catch {unset _registered($path)}
        return 0
    }
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::_motion_balloon
# ------------------------------------------------------------------------------
proc DynamicHelp::_motion_balloon { type path x y } {
    variable _top
    variable _id
    variable _delay
    variable _current

    if { $_current != $path && $type == "enter" } {
        set _current $path
        set type "motion"
        destroy $_top
    }
    if { $_current == $path } {
        if { $_id != "" } {
            after cancel $_id
            set _id ""
        }
        if { $type == "motion" } {
            if { ![winfo exists $_top] } {
                set _id [after $_delay "DynamicHelp::_show_help $path $x $y"]
            }
        } else {
            destroy $_top
            set _current ""
        }
    }
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::_motion_info
# ------------------------------------------------------------------------------
proc DynamicHelp::_motion_info { path } {
    variable _registered
    variable _current
    variable _saved

    if { $_current != $path && [info exists _registered($path)] } {
        if { ![info exists _saved] } {
            set _saved [GlobalVar::getvar [lindex $_registered($path) 0]]
        }
        GlobalVar::setvar [lindex $_registered($path) 0] [lindex $_registered($path) 1]
        set _current $path
    }
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::_leave_info
# ------------------------------------------------------------------------------
proc DynamicHelp::_leave_info { path } {
    variable _registered
    variable _current
    variable _saved

    if { [info exists _registered($path)] } {
        GlobalVar::setvar [lindex $_registered($path) 0] $_saved
    }
    unset _saved
    set _current ""
    
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::_menu_info
#    Version of R1v1 restored, due to lack of [winfo ismapped] and <Unmap>
#    under windows for menu.
# ------------------------------------------------------------------------------
proc DynamicHelp::_menu_info { event path } {
    variable _registered
 
    if { [info exists _registered($path)] } {
        set index [$path index active]
        if { [string compare $index "none"] &&
             [set idx [lsearch $_registered($path) [list $index *]]] != -1 } {
            GlobalVar::setvar [lindex $_registered($path) 0] \
                [lindex [lindex $_registered($path) $idx] 1]
        } else {
            GlobalVar::setvar [lindex $_registered($path) 0] ""
        }
    }
}


# ------------------------------------------------------------------------------
#  Command DynamicHelp::_show_help
# ------------------------------------------------------------------------------
proc DynamicHelp::_show_help { path x y } {
    variable _top
    variable _registered
    variable _id
    variable _delay

    if { [info exists _registered($path)] } {
        destroy  $_top
        toplevel $_top -relief flat \
            -bg [Widget::getoption $_top -foreground] \
            -bd [Widget::getoption $_top -borderwidth]
        wm overrideredirect $_top 1
        wm transient $_top
        wm withdraw $_top

        label $_top.label -text $_registered($path) \
            -relief flat -bd 0 -highlightthickness 0 \
            -foreground [Widget::getoption $_top -foreground] \
            -background [Widget::getoption $_top -background] \
            -font       [Widget::getoption $_top -font] \
            -justify    [Widget::getoption $_top -justify]


        pack $_top.label -side left
        update idletasks

        set  scrwidth  [winfo vrootwidth  .]
        set  scrheight [winfo vrootheight .]
        set  width     [winfo reqwidth  $_top]
        set  height    [winfo reqheight $_top]
        incr y 12
        incr x 8

        if { $x+$width > $scrwidth } {
            set x [expr $scrwidth - $width]
        }
        if { $y+$height > $scrheight } {
            set y [expr $y - 12 - $height]
        }

        wm geometry  $_top "+$x+$y"
        update idletasks
        wm deiconify $_top
    }
}


