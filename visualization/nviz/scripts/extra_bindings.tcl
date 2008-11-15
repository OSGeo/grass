# extra_bindings.tcl --
#
# Right now we put the floating point widget bindings in here


# ----------------------------------------------------------------------
# Class bindings for floatscale widgets.  When strict Motif is requested,
# the bindings use $tk_priv(buttons) and $tk_priv(activeFg) to set the
# -activeforeground color to -foreground when the mouse is in the window
# and restore it when the mouse leaves.
# ----------------------------------------------------------------------

bind FloatScale <Any-Enter> {
    if $tk_strictMotif {
	set tk_priv(activeFg) [lindex [%W config -activeforeground] 4]
	%W config -activeforeground [lindex [%W config -sliderforeground] 4]
    }
}
bind FloatScale <Any-Leave> {
    if {$tk_strictMotif && ($tk_priv(buttons) == 0)} {
	%W config -activeforeground $tk_priv(activeFg)
    }
}
bind FloatScale <Any-ButtonPress> {incr tk_priv(buttons)}
bind FloatScale <Any-ButtonRelease> {incr tk_priv(buttons) -1}

# These bindings added so key events can be detected within sliders
bind FloatScale <Any-Enter> {
    set tk_priv(fs_focus) [focus]
    focus %W
}

bind FloatScale <Any-Leave> {
    focus $tk_priv(fs_focus)
}

bind Scale <Any-Enter> {
    set tk_priv(fs_focus) [focus]
    focus %W
}

bind Scale <Any-Leave> {
    focus $tk_priv(fs_focus)
}


# These routines handle switching the cursors within application
# windows when the application becomes busy/un-busy

proc appBusy {} {
    global Nv_
    global mingw

    if { $mingw == "1" } {
        # fix for "bad cursor spec"
        # no color for windows cursors?
        . configure -cursor {watch}
        $Nv_(APP) configure -cursor {watch}
    } else {
        . configure -cursor {watch blue}
        $Nv_(APP) configure -cursor {watch blue}
    }
#$Nv_(AREA).menu.wait_pls configure -fg red -bg black
    pack $Nv_(AREA).menu.wait_pls
    grab $Nv_(AREA).menu.wait_pls
    update
}

proc appNotBusy {} {
    global Nv_

    . configure -cursor ""
    $Nv_(APP) configure -cursor ""
#$Nv_(AREA).menu.wait_pls configure -fg grey -bg grey
    pack forget $Nv_(AREA).menu.wait_pls
    grab release $Nv_(AREA).menu.wait_pls
    update
}

# Extra functions for handling the scripting menu
proc script_handle_on {} {
    global ScriptState

    if {[catch {Nv_set_script_state $ScriptState} err] != 0} then {
	set ScriptState 0
	error "$err"
    }
}







