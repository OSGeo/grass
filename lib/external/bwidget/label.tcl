# ------------------------------------------------------------------------------
#  label.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - Label::create
#     - Label::configure
#     - Label::cget
#     - Label::setfocus
#     - Label::_drag_cmd
#     - Label::_drop_cmd
#     - Label::_over_cmd
# ------------------------------------------------------------------------------

namespace eval Label {
    Widget::tkinclude Label label :cmd \
        remove {-foreground -text -textvariable -underline}

    Widget::declare Label {
        {-name               String     "" 0}
        {-text               String     "" 0}
        {-textvariable       String     "" 0}
        {-underline          Int        -1 0 {=-1}}
        {-focus              String     "" 0}
        {-foreground         TkResource "" 0 label}
        {-disabledforeground TkResource "" 0 button}
        {-state              Enum       normal 0  {normal disabled}}
        {-fg                 Synonym    -foreground}

    }
    DynamicHelp::include Label balloon
    DragSite::include    Label "" 1
    DropSite::include    Label {
        TEXT    {move {}}
        IMAGE   {move {}}
        BITMAP  {move {}}
        FGCOLOR {move {}}
        BGCOLOR {move {}}
        COLOR   {move {}}
    }

    Widget::syncoptions Label "" :cmd {-text {} -underline {}}

    proc ::Label { path args } { return [eval Label::create $path $args] }
    proc use {} {}

    bind BwLabel <FocusIn> {Label::setfocus %W}
    bind BwLabel <Destroy> {Widget::destroy %W; rename %W {}}
}


# ------------------------------------------------------------------------------
#  Command Label::create
# ------------------------------------------------------------------------------
proc Label::create { path args } {
    Widget::init Label $path $args

    if { [Widget::getoption $path -state] == "normal" } {
        set fg [Widget::getoption $path -foreground]
    } else {
        set fg [Widget::getoption $path -disabledforeground]
    }

    set var [Widget::getoption $path -textvariable]
    if {  $var == "" &&
          [Widget::getoption $path -image] == "" &&
          [Widget::getoption $path -bitmap] == ""} {
        set desc [BWidget::getname [Widget::getoption $path -name]]
        if { $desc != "" } {
            set text  [lindex $desc 0]
            set under [lindex $desc 1]
        } else {
            set text  [Widget::getoption $path -text]
            set under [Widget::getoption $path -underline]
        }
    } else {
        set under -1
        set text  ""
    }

    eval label $path [Widget::subcget $path :cmd] \
        [list -text $text -textvariable $var -underline $under -foreground $fg]

    set accel [string tolower [string index $text $under]]
    if { $accel != "" } {
        bind [winfo toplevel $path] <Alt-$accel> "Label::setfocus $path"
    }

    bindtags $path [list $path Label BwLabel [winfo toplevel $path] all]

    DragSite::setdrag $path $path Label::_init_drag_cmd [Widget::getoption $path -dragendcmd] 1
    DropSite::setdrop $path $path Label::_over_cmd Label::_drop_cmd 1
    DynamicHelp::sethelp $path $path 1

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval Label::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command Label::configure
# ------------------------------------------------------------------------------
proc Label::configure { path args } {
    set oldunder [$path:cmd cget -underline]
    if { $oldunder != -1 } {
        set oldaccel [string tolower [string index [$path:cmd cget -text] $oldunder]]
    } else {
        set oldaccel ""
    }
    set res [Widget::configure $path $args]

    set cfg  [Widget::hasChanged $path -foreground fg]
    set cdfg [Widget::hasChanged $path -disabledforeground dfg]
    set cst  [Widget::hasChanged $path -state state]

    if { $cst || $cfg || $cdfg } {
        if { $state == "normal" } {
            $path:cmd configure -fg $fg
        } else {
            $path:cmd configure -fg $dfg
        }
    }

    set cv [Widget::hasChanged $path -textvariable var]
    set cb [Widget::hasChanged $path -image img]
    set ci [Widget::hasChanged $path -bitmap bmp]
    set cn [Widget::hasChanged $path -name name]
    set ct [Widget::hasChanged $path -text text]
    set cu [Widget::hasChanged $path -underline under]

    if { $cv || $cb || $ci || $cn || $ct || $cu } {
        if {  $var == "" && $img == "" && $bmp == "" } {
            set desc [BWidget::getname $name]
            if { $desc != "" } {
                set text  [lindex $desc 0]
                set under [lindex $desc 1]
            }
        } else {
            set under -1
            set text  ""
        }
        set top [winfo toplevel $path]
        if { $oldaccel != "" } {
            bind $top <Alt-$oldaccel> {}
        }
        set accel [string tolower [string index $text $under]]
        if { $accel != "" } {
            bind $top <Alt-$accel> "Label::setfocus $path"
        }
        $path:cmd configure -text $text -underline $under -textvariable $var
    }

    set force [Widget::hasChanged $path -dragendcmd dragend]
    DragSite::setdrag $path $path Label::_init_drag_cmd $dragend $force
    DropSite::setdrop $path $path Label::_over_cmd Label::_drop_cmd
    DynamicHelp::sethelp $path $path

    return $res
}


# ------------------------------------------------------------------------------
#  Command Label::cget
# ------------------------------------------------------------------------------
proc Label::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command Label::setfocus
# ------------------------------------------------------------------------------
proc Label::setfocus { path } {
    if { ![string compare [Widget::getoption $path -state] "normal"] } {
        set w [Widget::getoption $path -focus]
        if { [winfo exists $w] && [Widget::focusOK $w] } {
            focus $w
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Label::_init_drag_cmd
# ------------------------------------------------------------------------------
proc Label::_init_drag_cmd { path X Y top } {
    if { [set cmd [Widget::getoption $path -draginitcmd]] != "" } {
        return [uplevel \#0 $cmd [list $path $X $Y $top]]
    }
    if { [set data [$path:cmd cget -image]] != "" } {
        set type "IMAGE"
        pack [label $top.l -image $data]
    } elseif { [set data [$path:cmd cget -bitmap]] != "" } {
        set type "BITMAP"
        pack [label $top.l -bitmap $data]
    } else {
        set data [$path:cmd cget -text]
        set type "TEXT"
    }
    set usertype [Widget::getoption $path -dragtype]
    if { $usertype != "" } {
        set type $usertype
    }
    return [list $type {copy} $data]
}


# ------------------------------------------------------------------------------
#  Command Label::_drop_cmd
# ------------------------------------------------------------------------------
proc Label::_drop_cmd { path source X Y op type data } {
    if { [set cmd [Widget::getoption $path -dropcmd]] != "" } {
        return [uplevel \#0 $cmd [list $path $source $X $Y $op $type $data]]
    }
    if { $type == "COLOR" || $type == "FGCOLOR" } {
        configure $path -foreground $data
    } elseif { $type == "BGCOLOR" } {
        configure $path -background $data
    } else {
        set text   ""
        set image  ""
        set bitmap ""
        switch -- $type {
            IMAGE   {set image $data}
            BITMAP  {set bitmap $data}
            default {
                set text $data
                if { [set var [$path:cmd cget -textvariable]] != "" } {
                    configure $path -image "" -bitmap ""
                    GlobalVar::setvar $var $data
                    return
                }
            }
        }
        configure $path -text $text -image $image -bitmap $bitmap
    }
    return 1
}


# ------------------------------------------------------------------------------
#  Command Label::_over_cmd
# ------------------------------------------------------------------------------
proc Label::_over_cmd { path source event X Y op type data } {
    if { [set cmd [Widget::getoption $path -dropovercmd]] != "" } {
        return [uplevel \#0 $cmd [list $path $source $event $X $Y $op $type $data]]
    }
    if { [Widget::getoption $path -state] == "normal" ||
         $type == "COLOR" || $type == "FGCOLOR" || $type == "BGCOLOR" } {
        DropSite::setcursor based_arrow_down
        return 1
    }
    DropSite::setcursor dot
    return 0
}
