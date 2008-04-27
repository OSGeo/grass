proc look_here {W x y} {
    global Nv_ WhatsHere

    set WhatsHere(on) 0
    set y [expr $Nv_(height) - $y]
    Nlook_here $x $y
    inform  "New center of view has been set"
    Nquick_draw
    bind $W <Button> {}
}

proc look_center {{M 0}} {

    if {$M == 0} { set M [Nget_current surf]}
    Nset_focus_map surf $M
    inform  "New center of view has been set"
    Nquick_draw
}

proc no_focus {} {
    Nset_focus_state 0
}
