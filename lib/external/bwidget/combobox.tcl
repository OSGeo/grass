# ------------------------------------------------------------------------------
#  combobox.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - ComboBox::create
#     - ComboBox::configure
#     - ComboBox::cget
#     - ComboBox::setvalue
#     - ComboBox::getvalue
#     - ComboBox::_create_popup
#     - ComboBox::_mapliste
#     - ComboBox::_unmapliste
#     - ComboBox::_select
#     - ComboBox::_modify_value
# ------------------------------------------------------------------------------

namespace eval ComboBox {
    ArrowButton::use
    Entry::use
    LabelFrame::use

    Widget::bwinclude ComboBox LabelFrame .labf \
        rename     {-text -label} \
        remove     {-focus} \
        prefix     {label -justify -width -anchor -height -font} \
        initialize {-relief sunken -borderwidth 2}

    Widget::bwinclude ComboBox Entry .e \
        remove {-relief -bd -borderwidth -bg -fg} \
        rename {-foreground -entryfg -background -entrybg}

    Widget::declare ComboBox {
        {-height      TkResource 0  0 listbox}
        {-values      String     "" 0}
        {-modifycmd   String     "" 0}
        {-postcommand String     "" 0}
    }

    Widget::addmap ComboBox "" :cmd {-background {}}
    Widget::addmap ComboBox ArrowButton .a \
        {-foreground {} -background {} -disabledforeground {} -state {}}

    Widget::syncoptions ComboBox Entry .e {-text {}}
    Widget::syncoptions ComboBox LabelFrame .labf {-label -text -underline {}}

    ::bind BwComboBox <FocusIn> {focus %W.labf}
    ::bind BwComboBox <Destroy> {Widget::destroy %W; rename %W {}}

    proc ::ComboBox { path args } { return [eval ComboBox::create $path $args] }
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command ComboBox::create
# ------------------------------------------------------------------------------
proc ComboBox::create { path args } {
    Widget::init ComboBox $path $args

    frame $path -background [Widget::getoption $path -background] \
        -highlightthickness 0 -bd 0 -relief flat -takefocus 0

    bindtags $path [list $path BwComboBox [winfo toplevel $path] all]

    set labf  [eval LabelFrame::create $path.labf [Widget::subcget $path .labf] \
                   -focus $path.e]
    set entry [eval Entry::create $path.e [Widget::subcget $path .e] \
                   -relief flat -borderwidth 0]

    set width  11
    set height [winfo reqheight $entry]
    set arrow [eval ArrowButton::create $path.a [Widget::subcget $path .a] \
                   -width $width -height $height \
                   -highlightthickness 0 -borderwidth 1 -takefocus 0 \
                   -dir   bottom \
                   -type  button \
                   -command [list "ComboBox::_mapliste $path"]]

    set frame [LabelFrame::getframe $labf]

    pack $arrow -in $frame -side right -fill y
    pack $entry -in $frame -side left  -fill both -expand yes
    pack $labf  -fill x -expand yes

    if { [Widget::getoption $path -editable] == 0 } {
        ::bind $entry <ButtonPress-1> "ArrowButton::invoke $path.a"
    } else {
        ::bind $entry <ButtonPress-1> "ComboBox::_unmapliste $path"
    }

    ::bind $path  <ButtonPress-1> "ComboBox::_unmapliste $path"
    ::bind $entry <Key-Up>        "ComboBox::_modify_value $path previous"
    ::bind $entry <Key-Down>      "ComboBox::_modify_value $path next"
    ::bind $entry <Key-Prior>     "ComboBox::_modify_value $path first"
    ::bind $entry <Key-Next>      "ComboBox::_modify_value $path last"

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval ComboBox::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command ComboBox::configure
# ------------------------------------------------------------------------------
proc ComboBox::configure { path args } {
    set res [Widget::configure $path $args]

    if { [Widget::hasChanged $path -values values] |
         [Widget::hasChanged $path -height h] |
         [Widget::hasChanged $path -font f] } {
        destroy $path.shell.listb
    }

    if { [Widget::hasChanged $path -editable ed] } {
        if { $ed } {
            ::bind $path.e <ButtonPress-1> "ComboBox::_unmapliste $path"
        } else {
            ::bind $path.e <ButtonPress-1> "ArrowButton::invoke $path.a"
        }
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command ComboBox::cget
# ------------------------------------------------------------------------------
proc ComboBox::cget { path option } {
    Widget::setoption $path -text [Entry::cget $path.e -text]
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command ComboBox::setvalue
# ------------------------------------------------------------------------------
proc ComboBox::setvalue { path index } {
    set values [Widget::getoption $path -values]
    set value  [Entry::cget $path.e -text]
    switch -- $index {
        next {
            if { [set idx [lsearch $values $value]] != -1 } {
                incr idx
            } else {
                set idx [lsearch $values "$value*"]
            }
        }
        previous {
            if { [set idx [lsearch $values $value]] != -1 } {
                incr idx -1
            } else {
                set idx [lsearch $values "$value*"]
            }
        }
        first {
            set idx 0
        }
        last {
            set idx [expr {[llength $values]-1}]
        }
        default {
            if { [string index $index 0] == "@" } {
                set idx [string range $index 1 end]
                if { [catch {string compare [expr {int($idx)}] $idx} res] || $res != 0 } {
                    return -code error "bad index \"$index\""
                }
            } else {
                return -code error "bad index \"$index\""
            }
        }
    }
    if { $idx >= 0 && $idx < [llength $values] } {
        set newval [lindex $values $idx]
        Widget::setoption $path -text $newval
        if { [set varname [Entry::cget $path.e -textvariable]] != "" } {
            GlobalVar::setvar $varname $newval
        } else {
            Entry::configure $path.e -text $newval
        }
        return 1
    }
    return 0
}


# ------------------------------------------------------------------------------
#  Command ComboBox::getvalue
# ------------------------------------------------------------------------------
proc ComboBox::getvalue { path } {
    set values [Widget::getoption $path -values]
    set value  [Entry::cget $path.e -text]

    return [lsearch $values $value]
}


# ------------------------------------------------------------------------------
#  Command ComboBox::bind
# ------------------------------------------------------------------------------
proc ComboBox::bind { path args } {
    return [eval ::bind $path.e $args]
}


# ------------------------------------------------------------------------------
#  Command ComboBox::_create_popup
# ------------------------------------------------------------------------------
proc ComboBox::_create_popup { path } {
    set shell [menu $path.shell -tearoff 0 -relief flat -bd 0]
    wm overrideredirect $shell 1
    wm withdraw $shell
    wm transient $shell [winfo toplevel $path]
    wm group $shell [winfo toplevel $path]
    set lval [Widget::getoption $path -values]
    set h    [Widget::getoption $path -height] 
    set sb   0
    if { $h <= 0 } {
        set len [llength $lval]
        if { $len < 3 } {
            set h 3
        } elseif { $len > 10 } {
            set h  10
	    set sb 1
        }
    }
    set frame  [frame $shell.frame -relief sunken -bd 2]
    set listb  [listbox $shell.listb -relief flat -bd 0 -highlightthickness 0 \
                    -exportselection false \
                    -font   [Widget::getoption $path -font]  \
                    -height $h]

    if { $sb } {
	set scroll [scrollbar $shell.scroll \
		-orient vertical \
		-command "$shell.listb yview" \
		-highlightthickness 0 -takefocus 0 -width 9]
	$listb configure -yscrollcommand "$scroll set"
    }
    $listb delete 0 end
    foreach val $lval {
        $listb insert end $val
    }

    if { $sb } {
	pack $scroll -in $frame -side right -fill y
    }
    pack $listb  -in $frame -side left  -fill both -expand yes
    pack $frame  -fill both -expand yes -padx 1 -padx 1

    ::bind $listb <ButtonRelease-1> "ComboBox::_select $path @%x,%y"
    ::bind $listb <Return>          "ComboBox::_select $path active"
    ::bind $listb <Escape>          "ComboBox::_unmapliste $path"
}


# ------------------------------------------------------------------------------
#  Command ComboBox::_mapliste
# ------------------------------------------------------------------------------
proc ComboBox::_mapliste { path } {
    set listb $path.shell.listb
    if { [winfo exists $path.shell] } {
	_unmapliste $path
        return
    }

    if { [Widget::getoption $path -state] == "disabled" } {
        return
    }
    if { [set cmd [Widget::getoption $path -postcommand]] != "" } {
        uplevel \#0 $cmd
    }
    if { ![llength [Widget::getoption $path -values]] } {
        return
    }
    _create_popup $path

    ArrowButton::configure $path.a -dir top
    $listb selection clear 0 end
    set values [$listb get 0 end]
    set curval [Entry::cget $path.e -text]
    if { [set idx [lsearch $values $curval]] != -1 ||
         [set idx [lsearch $values "$curval*"]] != -1 } {
        $listb selection set $idx
        $listb activate $idx
        $listb see $idx
    } else {
        $listb activate 0
        $listb see 0
    }

    set frame [LabelFrame::getframe $path.labf]
    BWidget::place $path.shell [winfo width $frame] 0 below $frame
    wm deiconify $path.shell
    raise $path.shell
    BWidget::grab global $path
}


# ------------------------------------------------------------------------------
#  Command ComboBox::_unmapliste
# ------------------------------------------------------------------------------
proc ComboBox::_unmapliste { path } {
    BWidget::grab release $path
    destroy $path.shell
    ArrowButton::configure $path.a -dir bottom
}


# ------------------------------------------------------------------------------
#  Command ComboBox::_select
# ------------------------------------------------------------------------------
proc ComboBox::_select { path index } {
    set index [$path.shell.listb index $index]
    _unmapliste $path
    if { $index != -1 } {
        if { [setvalue $path @$index] } {
            if { [set cmd [Widget::getoption $path -modifycmd]] != "" } {
                uplevel \#0 $cmd
            }
        }
    }
    return -code break
}


# ------------------------------------------------------------------------------
#  Command ComboBox::_modify_value
# ------------------------------------------------------------------------------
proc ComboBox::_modify_value { path direction } {
    if { [setvalue $path $direction] } {
        if { [set cmd [Widget::getoption $path -modifycmd]] != "" } {
            uplevel \#0 $cmd
        }
    }
}
