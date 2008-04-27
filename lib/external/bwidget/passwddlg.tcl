# -----------------------------------------------------------------------------
#  passwddlg.tcl
#  This file is part of Unifix BWidget Toolkit
#   by Stephane Lavirotte (Stephane.Lavirotte@sophia.inria.fr)
#  $Id$
# -----------------------------------------------------------------------------
#  Index of commands:
#     - PasswdDlg::create
#     - PasswdDlg::configure
#     - PasswdDlg::cget
#     - PasswdDlg::_verifonlogin
#     - PasswdDlg::_verifonpasswd
#     - PasswdDlg::_max
#------------------------------------------------------------------------------

namespace eval PasswdDlg {
    Dialog::use
    LabelEntry::use

    Widget::bwinclude PasswdDlg Dialog "" \
        remove     {-image -bitmap -side -default -cancel -separator} \
        initialize {-modal local -anchor c}

    Widget::bwinclude PasswdDlg LabelEntry .frame.lablog \
        remove {
            -command -editable -justify -name -show -side -state -takefocus
            -width -xscrollcommand -padx -pady
            -dragenabled -dragendcmd -dragevent -draginitcmd -dragtype
            -dropenabled -dropcmd -dropovercmd -droptypes
        } \
        prefix     {login -helptext -helpvar -label -text -textvariable -underline} \
        initialize {-relief sunken -borderwidth 2 -labelanchor w -width 15 -loginlabel "Login"}

    Widget::bwinclude PasswdDlg LabelEntry .frame.labpass \
        remove {
            -command -width -show -side -takefocus -xscrollcommand
            -dragenabled -dragendcmd -dragevent -draginitcmd -dragtype
            -dropenabled -dropcmd -dropovercmd -droptypes -justify -padx -pady -name
        } \
        prefix {passwd -editable -helptext -helpvar -label -state -text -textvariable -underline} \
        initialize {-relief sunken -borderwidth 2 -labelanchor w -width 15 -passwdlabel "Password"}

    Widget::declare PasswdDlg {
        {-type        Enum       ok           0 {ok okcancel}}
        {-labelwidth  TkResource -1           0 {label -width}}
        {-command     String     ""           0}
    }

    Widget::syncoptions PasswdDlg LabelEntry .frame.lablog  {
        -logintext -text -loginlabel -label -loginunderline -underline
    }
    Widget::syncoptions PasswdDlg LabelEntry .frame.labpass {
        -passwdtext -text -passwdlabel -label -passwdunderline -underline
    }

    proc ::PasswdDlg { path args } { return [eval PasswdDlg::create $path $args] }
    proc use {} {}
}


# -----------------------------------------------------------------------------
#  Command PasswdDlg::create
# -----------------------------------------------------------------------------
proc PasswdDlg::create { path args } {

    Widget::init PasswdDlg "$path#PasswdDlg" $args
    set type      [Widget::getoption "$path#PasswdDlg" -type]
    set loglabel  [Widget::getoption "$path#PasswdDlg" -loginlabel]
    set passlabel [Widget::getoption "$path#PasswdDlg" -passwdlabel]
    set labwidth  [Widget::getoption "$path#PasswdDlg" -labelwidth]
    set cmd       [Widget::getoption "$path#PasswdDlg" -command]

    set defb -1
    set canb -1
    switch -- $type {
        ok        { set lbut {ok}; set defb 0 }
        okcancel  { set lbut {ok cancel} ; set defb 0; set canb 1 }
    }

    eval Dialog::create $path [Widget::subcget "$path#PasswdDlg" ""] \
        -image [Bitmap::get passwd] -side bottom -default $defb -cancel $canb
    foreach but $lbut {
        if { $but == "ok" && $cmd != "" } {
            Dialog::add $path -text $but -name $but -command $cmd
        } else {
            Dialog::add $path -text $but -name $but
        }
    }
    set frame [Dialog::getframe $path]
    bind $path  <Return>  ""
    bind $frame <Destroy> "Widget::destroy $path#PasswdDlg"

    set lablog [eval LabelEntry::create $frame.lablog \
                    [Widget::subcget "$path#PasswdDlg" .frame.lablog] \
                    -label \"$loglabel\" -name login \
                    -dragenabled 0 -dropenabled 0 \
                    -command \"PasswdDlg::_verifonpasswd $path $frame.labpass\"]

    set labpass [eval LabelEntry::create $frame.labpass \
                     [Widget::subcget "$path#PasswdDlg" .frame.labpass] \
                     -label \"$passlabel\" -name password -show "*" \
                     -dragenabled 0 -dropenabled 0 \
                     -command \"PasswdDlg::_verifonlogin $path $frame.lablog\"]

    if { $labwidth == -1 } {
        # les options -label sont mises a jour selon -name
        set loglabel  [$lablog cget -label]
        set passlabel [$labpass cget -label]
        set labwidth  [PasswdDlg::_max [string length $loglabel] [string length $passlabel]]
        incr labwidth 1
        $lablog  configure -labelwidth $labwidth
        $labpass configure -labelwidth $labwidth
    }

    proc ::$path { cmd args } "return \[eval PasswdDlg::\$cmd $path \$args\]"

    pack  $frame.lablog $frame.labpass -fill x -expand 1
    focus $frame.lablog.e
    set res [Dialog::draw $path]

    if { $res == 0 } {
        set res [list [$lablog.e cget -text] [$labpass.e cget -text]]
    } else {
        set res [list]
    }
    Widget::destroy "$path#PasswdDlg"
    destroy $path

    return $res
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::configure
# -----------------------------------------------------------------------------

proc PasswdDlg::configure { path args } {
    set res [Widget::configure "$path#PasswdDlg" $args]
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::cget
# -----------------------------------------------------------------------------

proc PasswdDlg::cget { path option } {
    return [Widget::cget "$path#PasswdDlg" $option]
}


# -----------------------------------------------------------------------------
#  Command PasswdDlg::_verifonlogin
# -----------------------------------------------------------------------------
proc PasswdDlg::_verifonlogin { path labpass } {
    if { [$labpass.e cget -text] == "" } {
        focus $labpass
    } else {
        Dialog::setfocus $path default
    }
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::_verifonpasswd
# -----------------------------------------------------------------------------
proc PasswdDlg::_verifonpasswd { path lablog } {
    if { [$lablog.e cget -text] == "" } {
        focus $lablog
    } else {
        Dialog::setfocus $path default
    }
}

# -----------------------------------------------------------------------------
#  Command PasswdDlg::_max
# -----------------------------------------------------------------------------
proc PasswdDlg::_max { val1 val2 } { 
    return [expr ($val1 > $val2) ? ($val1) : ($val2)] 
}
