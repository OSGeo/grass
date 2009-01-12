# ------------------------------------------------------------------------------
#  button.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#   Public commands
#     - Button::create
#     - Button::configure
#     - Button::cget
#     - Button::invoke
#   Private commands (event bindings)
#     - Button::_destroy
#     - Button::_enter
#     - Button::_leave
#     - Button::_press
#     - Button::_release
#     - Button::_repeat
# ------------------------------------------------------------------------------

namespace eval Button {
    Widget::tkinclude Button button :cmd \
        remove {-command -relief -text -textvariable -underline}

    Widget::declare Button {
        {-name            String "" 0}
        {-text            String "" 0}
        {-textvariable    String "" 0}
        {-underline       Int    -1 0 {=-1}}
        {-armcommand      String "" 0}
        {-disarmcommand   String "" 0}
        {-command         String "" 0}
        {-repeatdelay     Int    0  0 {=0 ""}}
        {-repeatinterval  Int    0  0 {=0 ""}}
        {-relief          Enum   raised  0 {raised sunken flat ridge solid groove link}}
    }

    DynamicHelp::include Button balloon

    Widget::syncoptions Button "" :cmd {-text {} -underline {}}

    variable _current ""
    variable _pressed ""

    bind BwButton <Enter>           {Button::_enter %W}
    bind BwButton <Leave>           {Button::_leave %W}
    bind BwButton <ButtonPress-1>   {Button::_press %W}
    bind BwButton <ButtonRelease-1> {Button::_release %W}
    bind BwButton <Key-space>       {Button::invoke %W; break}
    bind BwButton <Return>          {Button::invoke %W; break}
    bind BwButton <Destroy>         {Widget::destroy %W; rename %W {}}

    proc ::Button { path args } { return [eval Button::create $path $args] }
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command Button::create
# ------------------------------------------------------------------------------
proc Button::create { path args } {
    Widget::init Button $path $args

    set relief [Widget::getoption $path -relief]
    if { ![string compare $relief "link"] } {
        set relief "flat"
    }

    set var [Widget::getoption $path -textvariable]
    if {  ![string length $var] } {
        set desc [BWidget::getname [Widget::getoption $path -name]]
        if { [llength $desc] } {
            set text  [lindex $desc 0]
            set under [lindex $desc 1]
            Widget::setoption $path -text $text
            Widget::setoption $path -underline $under
        } else {
            set text  [Widget::getoption $path -text]
            set under [Widget::getoption $path -underline]
        }
    } else {
        set under -1
        set text  ""
        Widget::setoption $path -underline $under
    }

    eval button $path [Widget::subcget $path :cmd] \
        [list -relief $relief -text $text -underline $under -textvariable $var]
    bindtags $path [list $path BwButton [winfo toplevel $path] all]

    set accel [string tolower [string index $text $under]]
    if { $accel != "" } {
        bind [winfo toplevel $path] <Alt-$accel> "Button::invoke $path"
    }

    DynamicHelp::sethelp $path $path 1

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval Button::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command Button::configure
# ------------------------------------------------------------------------------
proc Button::configure { path args } {
    set oldunder [$path:cmd cget -underline]
    if { $oldunder != -1 } {
        set oldaccel [string tolower [string index [$path:cmd cget -text] $oldunder]]
    } else {
        set oldaccel ""
    }
    set res [Widget::configure $path $args]

    set rc [Widget::hasChanged $path -relief relief]
    set sc [Widget::hasChanged $path -state  state]

    if { $rc || $sc } {
        if { ![string compare $relief "link"] } {
            if { ![string compare $state "active"] } {
                set relief "raised"
            } else {
                set relief "flat"
            }
        }
        $path:cmd configure -relief $relief -state $state
    }

    set cv [Widget::hasChanged $path -textvariable var]
    set cn [Widget::hasChanged $path -name name]
    set ct [Widget::hasChanged $path -text text]
    set cu [Widget::hasChanged $path -underline under]

    if { $cv || $cn || $ct || $cu } {
        if {  ![string length $var] } {
            set desc [BWidget::getname $name]
            if { [llength $desc] } {
                set text  [lindex $desc 0]
                set under [lindex $desc 1]
            }
        } else {
            set under -1
            set text  ""
        }
        set top [winfo toplevel $path]
        bind $top <Alt-$oldaccel> {}
        set accel [string tolower [string index $text $under]]
        if { $accel != "" } {
            bind $top <Alt-$accel> "Button::invoke $path"
        }
        $path:cmd configure -text $text -underline $under -textvariable $var
    }
    DynamicHelp::sethelp $path $path

    return $res
}


# ------------------------------------------------------------------------------
#  Command Button::cget
# ------------------------------------------------------------------------------
proc Button::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command Button::invoke
# ------------------------------------------------------------------------------
proc Button::invoke { path } {
    if { [string compare [$path:cmd cget -state] "disabled"] } {
	$path:cmd configure -state active -relief sunken
	update idletasks
        if { [set cmd [Widget::getoption $path -armcommand]] != "" } {
            uplevel \#0 $cmd
        }
	after 100
        set relief [Widget::getoption $path -relief]
        if { ![string compare $relief "link"] } {
            set relief flat
        }
	$path:cmd configure \
            -state  [Widget::getoption $path -state] \
            -relief $relief
        if { [set cmd [Widget::getoption $path -disarmcommand]] != "" } {
            uplevel \#0 $cmd
        }
        if { [set cmd [Widget::getoption $path -command]] != "" } {
            uplevel \#0 $cmd
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Button::_enter
# ------------------------------------------------------------------------------
proc Button::_enter { path } {
    variable _current
    variable _pressed

    set _current $path
    if { [string compare [$path:cmd cget -state] "disabled"] } {
        $path:cmd configure -state active
        if { $_pressed == $path } {
            $path:cmd configure -relief sunken
        } elseif { ![string compare [Widget::getoption $path -relief] "link"] } {
            $path:cmd configure -relief raised
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Button::_leave
# ------------------------------------------------------------------------------
proc Button::_leave { path } {
    variable _current
    variable _pressed

    set _current ""
    if { [string compare [$path:cmd cget -state] "disabled"] } {
        $path:cmd configure -state [Widget::getoption $path -state]
        set relief [Widget::getoption $path -relief]
        if { $_pressed == $path } {
            if { ![string compare $relief "link"] } {
                set relief raised
            }
            $path:cmd configure -relief $relief
        } elseif { ![string compare $relief "link"] } {
            $path:cmd configure -relief flat
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Button::_press
# ------------------------------------------------------------------------------
proc Button::_press { path } {
    variable _pressed

    if { [string compare [$path:cmd cget -state] "disabled"] } {
        set _pressed $path
	$path:cmd configure -relief sunken
        if { [set cmd [Widget::getoption $path -armcommand]] != "" } {
            uplevel \#0 $cmd
            if { [set delay [Widget::getoption $path -repeatdelay]]    > 0 ||
                 [set delay [Widget::getoption $path -repeatinterval]] > 0 } {
                after $delay "Button::_repeat $path"
            }
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Button::_release
# ------------------------------------------------------------------------------
proc Button::_release { path } {
    variable _current
    variable _pressed

    if { $_pressed == $path } {
        set _pressed ""
        set relief [Widget::getoption $path -relief]
        if { ![string compare $relief "link"] } {
            set relief raised
        }
        $path:cmd configure -relief $relief
        if { [set cmd [Widget::getoption $path -disarmcommand]] != "" } {
            uplevel \#0 $cmd
        }
        if { $_current == $path &&
             [string compare [$path:cmd cget -state] "disabled"] &&
             [set cmd [Widget::getoption $path -command]] != "" } {
            uplevel \#0 $cmd
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Button::_repeat
# ------------------------------------------------------------------------------
proc Button::_repeat { path } {
    variable _current
    variable _pressed

    if { $_current == $path && $_pressed == $path &&
         [string compare [$path:cmd cget -state] "disabled"] &&
         [set cmd [Widget::getoption $path -armcommand]] != "" } {
        uplevel \#0 $cmd
    }
    if { $_pressed == $path &&
         ([set delay [Widget::getoption $path -repeatinterval]] > 0 ||
          [set delay [Widget::getoption $path -repeatdelay]]    > 0) } {
        after $delay "Button::_repeat $path"
    }
}

