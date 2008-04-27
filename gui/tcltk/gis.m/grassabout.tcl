##################### some scroll widgets
proc scrollx_widget {widget path args} {
    frame $path
    eval $widget $path.$widget $args {-xscrollcommand [list $path.xscroll set]}
    scrollbar $path.xscroll -width 10 -orient horizontal \
        -command [list $path.$widget xview]
    grid $path.$widget -sticky news
    grid $path.xscroll -sticky news
    grid rowconfigure    $path 0 -weight 1
    grid columnconfigure $path 0 -weight 1
    return $path.$widget
}

proc scrolly_widget {widget path args} {
    frame $path
    eval $widget $path.$widget $args {-yscrollcommand [list $path.yscroll set]}
    scrollbar $path.yscroll -width 10 -orient vertical \
        -command [list $path.$widget yview]
    grid $path.$widget $path.yscroll -sticky news
    grid rowconfigure    $path 0 -weight 1
    grid columnconfigure $path 0 -weight 1
    return $path.$widget
}

proc scrollxy_widget {widget path args} {
    frame $path
    eval $widget $path.$widget $args { \
        -xscrollcommand [list $path.xscroll set] \
        -yscrollcommand [list $path.yscroll set] \
    }
    scrollbar $path.xscroll -width 10 -orient horizontal \
        -command [list $path.$widget xview]
    scrollbar $path.yscroll -width 10 -orient vertical \
        -command [list $path.$widget yview]
    grid $path.$widget $path.yscroll -sticky news
    grid $path.xscroll -sticky news
    grid rowconfigure    $path 0 -weight 1
    grid columnconfigure $path 0 -weight 1
    return $path.$widget
}

proc children {path} {
    set list {}
    foreach child [winfo children $path] {
        eval lappend list $child [children $child]
    }
    return $list
}


proc setfont {path font} {
    if {[llength $font] > 0} {
        foreach child [children $path] {
            catch {$child configure -font $font}
        }
    }
}

#########################################################################
#write text in window with close button

set help_font [font create -family Times -size 10]

proc helptext {title textopts tagopts message} {
    global help_font

    toplevel .helptext
    wm title .helptext $title

    bind .helptext <Return> {destroy .helptext}

    button .helptext.ok -text OK -command "destroy .helptext"
    pack .helptext.ok -side bottom

    eval scrollxy_widget text .helptext.frame -setgrid yes -wrap none $textopts
    pack .helptext.frame -side top -fill both -expand yes
    setfont .helptext.frame $help_font
    .helptext.frame.text insert end $message texttag
    eval .helptext.frame.text tag configure texttag $tagopts

    .helptext.frame.text configure -state disabled
    focus .helptext.frame.text
    
    grab .helptext
    tkwait window .helptext
}

#########################################################################
#grassabout.tcl
# open g.version and print in window
#

if {[catch {set text [exec g.version -c]} error]} {
    tk_messageBox -type ok -icon error -title [G_msg "Error"] -message [G_msg $error]
}
helptext {About GRASS} {-width 75} {-justify left} $text
