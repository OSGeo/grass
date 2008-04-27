# ------------------------------------------------------------------------------
#  messagedlg.tcl
#  This file is part of Unifix BWidget Toolkit
# ------------------------------------------------------------------------------
#  Index of commands:
#     - MessageDlg::create
# ------------------------------------------------------------------------------

namespace eval MessageDlg {
    Dialog::use

    Widget::tkinclude MessageDlg message .frame.msg \
        remove     {-cursor -highlightthickness -highlightbackground -highlightcolor \
                        -relief -borderwidth -takefocus -textvariable} \
        rename     {-text -message} \
        initialize {-aspect 800 -anchor c -justify center}

    Widget::bwinclude MessageDlg Dialog "" \
        remove {-modal -image -bitmap -side -anchor -separator \
                    -homogeneous -padx -pady -spacing}

    Widget::declare MessageDlg {
        {-icon       Enum   info 0 {none error info question warning}}
        {-type       Enum   user 0 {abortretryignore ok okcancel retrycancel yesno yesnocancel user}}
        {-buttons    String ""   0}
    }

    proc ::MessageDlg { path args } { return [eval MessageDlg::create $path $args] }
    proc use { } {}
}


# ------------------------------------------------------------------------------
#  Command MessageDlg::create
# ------------------------------------------------------------------------------
proc MessageDlg::create { path args } {
    global tcl_platform

    Widget::init MessageDlg "$path#Message" $args
    set type  [Widget::getoption "$path#Message" -type]
    set title [Widget::getoption "$path#Message" -title]
    set icon  [Widget::getoption "$path#Message" -icon]
    set defb  -1
    set canb  -1
    switch -- $type {
        abortretryignore {set lbut {abort retry ignore}}
        ok               {set lbut {ok}; set defb 0 }
        okcancel         {set lbut {ok cancel}; set defb 0; set canb 1}
        retrycancel      {set lbut {retry cancel}; set defb 0; set canb 1}
        yesno            {set lbut {yes no}; set defb 0; set canb 1}
        yesnocancel      {set lbut {yes no cancel}; set defb 0; set canb 2}
        user             {set lbut [Widget::getoption "$path#Message" -buttons]}
    }
    if { [Widget::getoption "$path#Message" -default] == -1 } {
        Widget::setoption "$path#Message" -default $defb
    }
    if { [Widget::getoption "$path#Message" -cancel] == -1 } {
        Widget::setoption "$path#Message" -cancel $canb
    }
    if { $title == "" } {
        set frame [frame $path -class MessageDlg]
        set title [option get $frame "${icon}Title" MessageDlg]
        destroy $frame
        if { $title == "" } {
            set title "Message"
        }
    }
    Widget::setoption "$path#Message" -title $title
    if { $tcl_platform(platform) == "unix" || $type == "user" } {
        if { $icon != "none" } {
            set image [Bitmap::get $icon]
        } else {
            set image ""
        }
        eval Dialog::create $path [Widget::subcget "$path#Message" ""] \
            -image $image -modal local -side bottom -anchor c
        set idbut 0
        foreach but $lbut {
            Dialog::add $path -text $but -name $but
        }
        set frame [Dialog::getframe $path]

        eval message $frame.msg [Widget::subcget "$path#Message" .frame.msg] \
            -relief flat -borderwidth 0 -highlightthickness 0 -textvariable {""}
        pack  $frame.msg -side left -padx 3m -pady 1m -fill x -expand yes

        set res [Dialog::draw $path]
    } else {
        set parent [Widget::getoption "$path#Message" -parent]
        set def    [lindex $lbut [Widget::getoption "$path#Message" -default]]
        set opt    [list \
                        -message [Widget::getoption "$path#Message" -message] \
                        -type    $type  \
                        -title   $title]
        if { [winfo exists $parent] } {
           lappend opt -parent $parent
        }
        if { $def != "" } {
           lappend opt -default $def
        }
        if { $icon != "none" } {
           lappend opt -icon $icon
        }
        set res [eval tk_messageBox $opt]
        set res [lsearch $lbut $res]
    }
    Widget::destroy "$path#Message"
    destroy $path

    return $res
}
