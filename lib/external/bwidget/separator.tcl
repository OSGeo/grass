# ------------------------------------------------------------------------------
#  separator.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#     - Separator::create
#     - Separator::configure
#     - Separator::cget
# ------------------------------------------------------------------------------

namespace eval Separator {
    Widget::declare Separator {
        {-background TkResource ""         0 frame}
        {-relief     Enum       groove     0 {ridge groove}}
        {-orient     Enum       horizontal 1 {horizontal vertical}}
        {-bg         Synonym    -background}
    }
    Widget::addmap Separator "" :cmd {-background {}}

    proc ::Separator { path args } { return [eval Separator::create $path $args] }
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command Separator::create
# ------------------------------------------------------------------------------
proc Separator::create { path args } {
    Widget::init Separator $path $args

    if { [Widget::getoption $path -relief] == "groove" } {
        set relief sunken
    } else {
        set relief raised
    }

    if { [Widget::getoption $path -orient] == "horizontal" } {
        frame $path \
            -background  [Widget::getoption $path -background] \
            -borderwidth 1 \
            -relief      $relief \
            -height      2
    } else {
        frame $path \
            -background  [Widget::getoption $path -background] \
            -borderwidth 1 \
            -relief      $relief \
            -width       2
    }
    bind $path <Destroy> {Widget::destroy %W; rename %W {}}

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval Separator::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command Separator::configure
# ------------------------------------------------------------------------------
proc Separator::configure { path args } {
    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -relief relief] } {
        if { $relief == "groove" } {
            $path:cmd configure -relief sunken
        } else {
            $path:cmd configure -relief raised
        }
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command Separator::cget
# ------------------------------------------------------------------------------
proc Separator::cget { path option } {
    return [Widget::cget $path $option]
}
