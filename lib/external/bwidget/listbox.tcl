# ------------------------------------------------------------------------------
#  listbox.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - ListBox::create
#     - ListBox::configure
#     - ListBox::cget
#     - ListBox::insert
#     - ListBox::itemconfigure
#     - ListBox::itemcget
#     - ListBox::bindText
#     - ListBox::bindImage
#     - ListBox::delete
#     - ListBox::move
#     - ListBox::reorder
#     - ListBox::selection
#     - ListBox::exists
#     - ListBox::index
#     - ListBox::item - deprecated
#     - ListBox::items
#     - ListBox::see
#     - ListBox::edit
#     - ListBox::xview
#     - ListBox::yview
#     - ListBox::_update_edit_size
#     - ListBox::_destroy
#     - ListBox::_see
#     - ListBox::_update_scrollregion
#     - ListBox::_draw_item
#     - ListBox::_redraw_items
#     - ListBox::_redraw_selection
#     - ListBox::_redraw_listbox
#     - ListBox::_redraw_idle
#     - ListBox::_resize
#     - ListBox::_init_drag_cmd
#     - ListBox::_drop_cmd
#     - ListBox::_over_cmd
#     - ListBox::_auto_scroll
#     - ListBox::_scroll
# ------------------------------------------------------------------------------


namespace eval ListBox {
    namespace eval Item {
        Widget::declare ListBox::Item {
            {-indent     Int        0       0 {=0}}
            {-text       String     ""      0}
            {-font       TkResource ""      0 listbox}
            {-image      TkResource ""      0 label}
            {-window     String     ""      0}
            {-fill       TkResource black   0 {listbox -foreground}}
            {-data       String     ""      0}
        }
    }

    Widget::tkinclude ListBox canvas :cmd \
        remove     {-insertwidth -insertbackground -insertborderwidth -insertofftime \
                        -insertontime -selectborderwidth -closeenough -confine -scrollregion \
                        -xscrollincrement -yscrollincrement -width -height} \
        initialize {-relief sunken -borderwidth 2 -takefocus 1 \
                        -highlightthickness 1 -width 200}

    Widget::declare ListBox {
        {-deltax           Int 10 0 {=0 ""}}
        {-deltay           Int 15 0 {=0 ""}}
        {-padx             Int 20 0 {=0 ""}}
        {-background       TkResource "" 0 listbox}
        {-selectbackground TkResource "" 0 listbox}
        {-selectforeground TkResource "" 0 listbox}
        {-width            TkResource "" 0 listbox}
        {-height           TkResource "" 0 listbox}
        {-redraw           Boolean 1  0}
        {-multicolumn      Boolean 0  0}
        {-dropovermode     Flag    "wpi" 0 "wpi"}
        {-bg               Synonym -background}
    }
    DragSite::include ListBox "LISTBOX_ITEM" 1
    DropSite::include ListBox {
        LISTBOX_ITEM {copy {} move {}}
    }

    Widget::addmap ListBox "" :cmd {-deltay -yscrollincrement}

    proc ::ListBox { path args } { return [eval ListBox::create $path $args] }
    proc use {} {}

    variable _edit
}


# ------------------------------------------------------------------------------
#  Command ListBox::create
# ------------------------------------------------------------------------------
proc ListBox::create { path args } {
    Widget::init ListBox $path $args

    variable $path
    upvar 0  $path data

    # widget informations
    set data(nrows) -1

    # items informations
    set data(items)    {}
    set data(selitems) {}

    # update informations
    set data(upd,level)   0
    set data(upd,afterid) ""
    set data(upd,level)   0
    set data(upd,delete)  {}

    # drag and drop informations
    set data(dnd,scroll)   ""
    set data(dnd,afterid)  ""
    set data(dnd,item)     ""

    eval canvas $path [Widget::subcget $path :cmd] \
        -width  [expr {[Widget::getoption $path -width]*8}] \
        -height [expr {[Widget::getoption $path -height]*[Widget::getoption $path -deltay]}] \
        -xscrollincrement 8

    bind $path <Configure> "ListBox::_resize  $path"
    bind $path <Destroy>   "ListBox::_destroy $path"

    DragSite::setdrag $path $path ListBox::_init_drag_cmd [Widget::getoption $path -dragendcmd] 1
    DropSite::setdrop $path $path ListBox::_over_cmd ListBox::_drop_cmd 1

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval ListBox::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command ListBox::configure
# ------------------------------------------------------------------------------
proc ListBox::configure { path args } {
    set res [Widget::configure $path $args]

    set ch1 [expr {[Widget::hasChanged $path -deltay dy]  |
                   [Widget::hasChanged $path -padx val]   |
                   [Widget::hasChanged $path -multicolumn val]}]

    set ch2 [expr {[Widget::hasChanged $path -selectbackground val] |
                   [Widget::hasChanged $path -selectforeground val]}]

    set redraw 0
    if { [Widget::hasChanged $path -height h] } {
        $path:cmd configure -height [expr {$h*$dy}]
        set redraw 1
    }
    if { [Widget::hasChanged $path -width w] } {
        $path:cmd configure -width [expr {$w*8}]
        set redraw 1
    }

    if { !$redraw } {
        if { $ch1 } {
            _redraw_idle $path 2
        } elseif { $ch2 } {
            _redraw_idle $path 1
        }
    }

    if { [Widget::hasChanged $path -redraw bool] && $bool } {
        variable $path
        upvar 0  $path data
        set lvl $data(upd,level)
        set data(upd,level) 0
        _redraw_idle $path $lvl
    }
    set force [Widget::hasChanged $path -dragendcmd dragend]
    DragSite::setdrag $path $path ListBox::_init_drag_cmd $dragend $force
    DropSite::setdrop $path $path ListBox::_over_cmd ListBox::_drop_cmd

    return $res
}


# ------------------------------------------------------------------------------
#  Command ListBox::cget
# ------------------------------------------------------------------------------
proc ListBox::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command ListBox::insert
# ------------------------------------------------------------------------------
proc ListBox::insert { path index item args } {
    variable $path
    upvar 0  $path data

    if { [lsearch $data(items) $item] != -1 } {
        return -code error "item \"$item\" already exists"
    }

    Widget::init ListBox::Item $path.$item $args

    if { ![string compare $index "end"] } {
        lappend data(items) $item
    } else {
        set data(items) [linsert $data(items) $index $item]
    }
    set data(upd,create,$item) $item

    _redraw_idle $path 2
    return $item
}


# ------------------------------------------------------------------------------
#  Command ListBox::itemconfigure
# ------------------------------------------------------------------------------
proc ListBox::itemconfigure { path item args } {
    variable $path
    upvar 0  $path data

    if { [lsearch $data(items) $item] == -1 } {
        return -code error "item \"$item\" does not exist"
    }

    set oldind [Widget::getoption $path.$item -indent]

    set res   [Widget::configure $path.$item $args]
    set chind [Widget::hasChanged $path.$item -indent indent]
    set chw   [Widget::hasChanged $path.$item -window win]
    set chi   [Widget::hasChanged $path.$item -image  img]
    set cht   [Widget::hasChanged $path.$item -text txt]
    set chf   [Widget::hasChanged $path.$item -font fnt]
    set chfg  [Widget::hasChanged $path.$item -fill fg]
    set idn   [$path:cmd find withtag n:$item]

    if { $idn == "" } {
        # item is not drawn yet
        _redraw_idle $path 2
        return $res
    }

    set oldb   [$path:cmd bbox $idn]
    set coords [$path:cmd coords $idn]
    set padx   [Widget::getoption $path -padx]
    set x0     [expr {[lindex $coords 0]-$padx-$oldind+$indent}]
    set y0     [lindex $coords 1]
    if { $chw || $chi } {
        # -window or -image modified
        set idi  [$path:cmd find withtag i:$item]
        set type [lindex [$path:cmd gettags $idi] 0]
        if { [string length $win] } {
            if { ![string compare $type "win"] } {
                $path:cmd itemconfigure $idi -window $win
            } else {
                $path:cmd delete $idi
                $path:cmd create window $x0 $y0 -window $win -anchor w -tags "win i:$item"
            }
        } elseif { [string length $img] } {
            if { ![string compare $type "img"] } {
                $path:cmd itemconfigure $idi -image $img
            } else {
                $path:cmd delete $idi
                $path:cmd create image $x0 $y0 -image $img -anchor w -tags "img i:$item"
            }
        } else {
            $path:cmd delete $idi
        }
    }

    if { $cht || $chf || $chfg } {
        # -text or -font modified, or -fill modified
        $path:cmd itemconfigure $idn -text $txt -font $fnt -fill $fg
        _redraw_idle $path 1
    }

    if { $chind } {
        # -indent modified
        $path:cmd coords $idn [expr {$x0+$padx}] $y0
        $path:cmd coords i:$item $x0 $y0
        _redraw_idle $path 1
    }

    if { [Widget::getoption $path -multicolumn] && ($cht || $chf || $chind) } {
        set bbox [$path:cmd bbox $idn]
        if { [lindex $bbox 2] > [lindex $oldb 2] } {
            _redraw_idle $path 2
        }
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command ListBox::itemcget
# ------------------------------------------------------------------------------
proc ListBox::itemcget { path item option } {
    return [Widget::cget $path.$item $option]
}


# ------------------------------------------------------------------------------
#  Command ListBox::bindText
# ------------------------------------------------------------------------------
proc ListBox::bindText { path event script } {
    if { $script != "" } {
        $path:cmd bind "item" $event \
            "$script \[string range \[lindex \[$path:cmd gettags current\] 1\] 2 end\]"
    } else {
        $path:cmd bind "item" $event {}
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::bindImage
# ------------------------------------------------------------------------------
proc ListBox::bindImage { path event script } {
    if { $script != "" } {
        $path:cmd bind "img" $event \
            "$script \[string range \[lindex \[$path:cmd gettags current\] 1\] 2 end\]"
    } else {
        $path:cmd bind "img" $event {}
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::delete
# ------------------------------------------------------------------------------
proc ListBox::delete { path args } {
    variable $path
    upvar 0  $path data

    foreach litems $args {
        foreach item $litems {
            set idx [lsearch $data(items) $item]
            if { $idx != -1 } {
                set data(items) [lreplace $data(items) $idx $idx]
                Widget::destroy $path.$item
                if { [info exists data(upd,create,$item)] } {
                    unset data(upd,create,$item)
                } else {
                    lappend data(upd,delete) $item
                }
            }
        }
    }

    set sel $data(selitems)
    set data(selitems) {}
    eval selection $path set $sel
    _redraw_idle $path 2
}


# ------------------------------------------------------------------------------
#  Command ListBox::move
# ------------------------------------------------------------------------------
proc ListBox::move { path item index } {
    variable $path
    upvar 0  $path data

    if { [set idx [lsearch $data(items) $item]] == -1 } {
        return -code error "item \"$item\" does not exist"
    }

    set data(items) [lreplace $data(items) $idx $idx]
    if { ![string compare $index "end"] } {
        lappend data($path,item) $item
    } else {
        set data(items) [linsert $data(items) $index $item]
    }

    _redraw_idle $path 2
}


# ------------------------------------------------------------------------------
#  Command ListBox::reorder
# ------------------------------------------------------------------------------
proc ListBox::reorder { path neworder } {
    variable $path
    upvar 0  $path data

    set data(items) [BWidget::lreorder $data(items) $neworder]
    _redraw_idle $path 2
}


# ------------------------------------------------------------------------------
#  Command ListBox::selection
# ------------------------------------------------------------------------------
proc ListBox::selection { path cmd args } {
    variable $path
    upvar 0  $path data

    switch -- $cmd {
        set {
            set data(selitems) {}
            foreach item $args {
                if { [lsearch $data(selitems) $item] == -1 } {
                    if { [lsearch $data(items) $item] != -1 } {
                        lappend data(selitems) $item
                    }
                }
            }
        }
        add {
            foreach item $args {
                if { [lsearch $data(selitems) $item] == -1 } {
                    if { [lsearch $data(items) $item] != -1 } {
                        lappend data(selitems) $item
                    }
                }
            }
        }
        remove {
            foreach item $args {
                if { [set idx [lsearch $data(selitems) $item]] != -1 } {
                    set data(selitems) [lreplace $data(selitems) $idx $idx]
                }
            }
        }
        clear {
            set data(selitems) {}
        }
        get {
            return $data(selitems)
        }
        default {
            return
        }
    }
    _redraw_idle $path 1
}


# ------------------------------------------------------------------------------
#  Command ListBox::exists
# ------------------------------------------------------------------------------
proc ListBox::exists { path item } {
    variable $path
    upvar 0  $path data

    return [expr {[lsearch $data(items) $item] != -1}]
}


# ------------------------------------------------------------------------------
#  Command ListBox::index
# ------------------------------------------------------------------------------
proc ListBox::index { path item } {
    variable $path
    upvar 0  $path data

    return [lsearch $data(items) $item]
}


# ------------------------------------------------------------------------------
#  Command ListBox::item - deprecated
# ------------------------------------------------------------------------------
proc ListBox::item { path first {last ""} } {
    variable $path
    upvar 0  $path data

    if { ![string length $last] } {
        return [lindex $data(items) $first]
    } else {
        return [lrange $data(items) $first $last]
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::items
# ------------------------------------------------------------------------------
proc ListBox::items { path {first ""} {last ""}} {
    variable $path
    upvar 0  $path data

    if { ![string length $first] } {
	return $data(items)
    }

    if { ![string length $last] } {
        return [lindex $data(items) $first]
    } else {
        return [lrange $data(items) $first $last]
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::see
# ------------------------------------------------------------------------------
proc ListBox::see { path item } {
    variable $path
    upvar 0  $path data

    if { [Widget::getoption $path -redraw] && $data(upd,afterid) != "" } {
        after cancel $data(upd,afterid)
        _redraw_listbox $path
    }
    set idn [$path:cmd find withtag n:$item]
    if { $idn != "" } {
        ListBox::_see $path $idn right
        ListBox::_see $path $idn left
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::edit
# ------------------------------------------------------------------------------
proc ListBox::edit { path item text {verifycmd ""} {clickres 0} {select 1}} {
    variable _edit
    variable $path
    upvar 0  $path data

    if { [Widget::getoption $path -redraw] && $data(upd,afterid) != "" } {
        after cancel $data(upd,afterid)
        _redraw_listbox $path
    }
    set idn [$path:cmd find withtag n:$item]
    if { $idn != "" } {
        ListBox::_see $path $idn right
        ListBox::_see $path $idn left

        set oldfg  [$path:cmd itemcget $idn -fill]
        set sbg    [Widget::getoption $path -selectbackground]
        set coords [$path:cmd coords $idn]
        set x      [lindex $coords 0]
        set y      [lindex $coords 1]
        set bd     [expr {[$path:cmd cget -borderwidth]+[$path:cmd cget -highlightthickness]}]
        set w      [expr {[winfo width $path] - 2*$bd}]
        set wmax   [expr {[$path:cmd canvasx $w]-$x}]

	$path:cmd itemconfigure $idn    -fill [Widget::getoption $path -background]
        $path:cmd itemconfigure s:$item -fill {} -outline {}

        set _edit(text) $text
        set _edit(wait) 0

        set frame  [frame $path.edit \
                        -relief flat -borderwidth 0 -highlightthickness 0 \
                        -background [Widget::getoption $path -background]]
        set ent    [entry $frame.edit \
                        -width              0     \
                        -relief             solid \
                        -borderwidth        1     \
                        -highlightthickness 0     \
                        -foreground         [Widget::getoption $path.$item -fill] \
                        -background         [Widget::getoption $path -background] \
                        -selectforeground   [Widget::getoption $path -selectforeground] \
                        -selectbackground   $sbg  \
                        -font               [Widget::getoption $path.$item -font] \
                        -textvariable       ListBox::_edit(text)]
        pack $ent -ipadx 8 -anchor w

        set idw [$path:cmd create window $x $y -window $frame -anchor w]
        trace variable ListBox::_edit(text) w "ListBox::_update_edit_size $path $ent $idw $wmax"
        tkwait visibility $ent
        grab  $frame
        BWidget::focus set $ent
        _update_edit_size $path $ent $idw $wmax
        update
        if { $select } {
            $ent selection range 0 end
            $ent icursor end
            $ent xview end
        }

        bind $ent <Escape> {set ListBox::_edit(wait) 0}
        bind $ent <Return> {set ListBox::_edit(wait) 1}
	if { $clickres == 0 || $clickres == 1 } {
	    bind $frame <Button>  "set ListBox::_edit(wait) $clickres"
	}

        set ok 0
        while { !$ok } {
            tkwait variable ListBox::_edit(wait)
            if { !$_edit(wait) || $verifycmd == "" ||
                 [uplevel \#0 $verifycmd [list $_edit(text)]] } {
                set ok 1
            }
        }
        trace vdelete ListBox::_edit(text) w "ListBox::_update_edit_size $path $ent $idw $wmax"
        grab release $frame
        BWidget::focus release $ent
        destroy $frame
        $path:cmd delete $idw
        $path:cmd itemconfigure $idn    -fill $oldfg
        $path:cmd itemconfigure s:$item -fill $sbg -outline $sbg

        if { $_edit(wait) } {
            return $_edit(text)
        }
    }
    return ""
}


# ------------------------------------------------------------------------------
#  Command ListBox::xview
# ------------------------------------------------------------------------------
proc ListBox::xview { path args } {
    return [eval $path:cmd xview $args]
}


# ------------------------------------------------------------------------------
#  Command ListBox::yview
# ------------------------------------------------------------------------------
proc ListBox::yview { path args } {
    return [eval $path:cmd yview $args]
}


# ------------------------------------------------------------------------------
#  Command ListBox::_update_edit_size
# ------------------------------------------------------------------------------
proc ListBox::_update_edit_size { path entry idw wmax args } {
    set entw [winfo reqwidth $entry]
    if { $entw >= $wmax } {
        $path:cmd itemconfigure $idw -width $wmax
    } else {
        $path:cmd itemconfigure $idw -width 0
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::_destroy
# ------------------------------------------------------------------------------
proc ListBox::_destroy { path } {
    variable $path
    upvar 0  $path data

    if { $data(upd,afterid) != "" } {
        after cancel $data(upd,afterid)
    }
    if { $data(dnd,afterid) != "" } {
        after cancel $data(dnd,afterid)
    }
    foreach item $data(items) {
        Widget::destroy $path.$item
    }

    Widget::destroy $path
    unset data
    rename $path {}
}


# ------------------------------------------------------------------------------
#  Command ListBox::_see
# ------------------------------------------------------------------------------
proc ListBox::_see { path idn side } {
    set bbox [$path:cmd bbox $idn]
    set scrl [$path:cmd cget -scrollregion]

    set ymax [lindex $scrl 3]
    set dy   [$path:cmd cget -yscrollincrement]
    set yv   [$path:cmd yview]
    set yv0  [expr {round([lindex $yv 0]*$ymax/$dy)}]
    set yv1  [expr {round([lindex $yv 1]*$ymax/$dy)}]
    set y    [expr {int([lindex [$path:cmd coords $idn] 1]/$dy)}]
    if { $y < $yv0 } {
        $path:cmd yview scroll [expr {$y-$yv0}] units
    } elseif { $y >= $yv1 } {
        $path:cmd yview scroll [expr {$y-$yv1+1}] units
    }

    set xmax [lindex $scrl 2]
    set dx   [$path:cmd cget -xscrollincrement]
    set xv   [$path:cmd xview]
    if { ![string compare $side "right"] } {
        set xv1 [expr {round([lindex $xv 1]*$xmax/$dx)}]
        set x1  [expr {int([lindex $bbox 2]/$dx)}]
        if { $x1 >= $xv1 } {
            $path:cmd xview scroll [expr {$x1-$xv1+1}] units
        }
    } else {
        set xv0 [expr {round([lindex $xv 0]*$xmax/$dx)}]
        set x0  [expr {int([lindex $bbox 0]/$dx)}]
        if { $x0 < $xv0 } {
            $path:cmd xview scroll [expr {$x0-$xv0}] units
        }
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::_update_scrollregion
# ------------------------------------------------------------------------------
proc ListBox::_update_scrollregion { path } {
    set bd   [expr {2*([$path:cmd cget -borderwidth]+[$path:cmd cget -highlightthickness])}]
    set w    [expr {[winfo width  $path] - $bd}]
    set h    [expr {[winfo height $path] - $bd}]
    set xinc [$path:cmd cget -xscrollincrement]
    set yinc [$path:cmd cget -yscrollincrement]
    set bbox [$path:cmd bbox all]
    if { [llength $bbox] } {
        set xs [lindex $bbox 2]
        set ys [lindex $bbox 3]

        if { $w < $xs } {
            set w [expr {int($xs)}]
            if { [set r [expr {$w % $xinc}]] } {
                set w [expr {$w+$xinc-$r}]
            }
        }
        if { $h < $ys } {
            set h [expr {int($ys)}]
            if { [set r [expr {$h % $yinc}]] } {
                set h [expr {$h+$yinc-$r}]
            }
        }
    }

    $path:cmd configure -scrollregion [list 0 0 $w $h]
}


# ------------------------------------------------------------------------------
#  Command ListBox::_draw_item
# ------------------------------------------------------------------------------
proc ListBox::_draw_item { path item x0 x1 y } {
    set indent [Widget::getoption $path.$item -indent]
    $path:cmd create text [expr {$x1+$indent}] $y \
        -text   [Widget::getoption $path.$item -text] \
        -fill   [Widget::getoption $path.$item -fill] \
        -font   [Widget::getoption $path.$item -font] \
        -anchor w \
        -tags   "item n:$item"
    if { [set win [Widget::getoption $path.$item -window]] != "" } {
        $path:cmd create window [expr {$x0+$indent}] $y \
            -window $win -anchor w -tags "win i:$item"
    } elseif { [set img [Widget::getoption $path.$item -image]] != "" } {
        $path:cmd create image [expr {$x0+$indent}] $y \
            -image $img -anchor w -tags "img i:$item"
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::_redraw_items
# ------------------------------------------------------------------------------
proc ListBox::_redraw_items { path } {
    variable $path
    upvar 0  $path data

    $path:cmd configure -cursor watch
    set dx   [Widget::getoption $path -deltax]
    set dy   [Widget::getoption $path -deltay]
    set padx [Widget::getoption $path -padx]
    set y0   [expr {$dy/2}]
    set x0   4
    set x1   [expr {$x0+$padx}]
    set nitem 0
    set drawn {}
    set data(xlist) {}
    if { [Widget::getoption $path -multicolumn] } {
        set nrows $data(nrows)
    } else {
        set nrows [llength $data(items)]
    }
    foreach item $data(upd,delete) {
        $path:cmd delete i:$item n:$item s:$item
    }
    foreach item $data(items) {
        if { [info exists data(upd,create,$item)] } {
            _draw_item $path $item $x0 $x1 $y0
            unset data(upd,create,$item)
        } else {
            set indent [Widget::getoption $path.$item -indent]
            $path:cmd coords n:$item [expr {$x1+$indent}] $y0
            $path:cmd coords i:$item [expr {$x0+$indent}] $y0
        }
        incr y0 $dy
        incr nitem
        lappend drawn n:$item
        if { $nitem == $nrows } {
            set y0    [expr {$dy/2}]
            set bbox  [eval $path:cmd bbox $drawn]
            set drawn {}
            set x0    [expr {[lindex $bbox 2]+$dx}]
            set x1    [expr {$x0+$padx}]
            set nitem 0
            lappend data(xlist) [lindex $bbox 2]
        }
    }
    if { $nitem && $nitem < $nrows } {
        set bbox  [eval $path:cmd bbox $drawn]
        lappend data(xlist) [lindex $bbox 2]
    }
    set data(upd,delete) {}
    $path:cmd configure -cursor [Widget::getoption $path -cursor]
}


# ------------------------------------------------------------------------------
#  Command ListBox::_redraw_selection
# ------------------------------------------------------------------------------
proc ListBox::_redraw_selection { path } {
    variable $path
    upvar 0  $path data

    set selbg [Widget::getoption $path -selectbackground]
    set selfg [Widget::getoption $path -selectforeground]
    foreach id [$path:cmd find withtag sel] {
        set item [string range [lindex [$path:cmd gettags $id] 1] 2 end]
        $path:cmd itemconfigure "n:$item" -fill [Widget::getoption $path.$item -fill]
    }
    $path:cmd delete sel
    foreach item $data(selitems) {
        set bbox [$path:cmd bbox "n:$item"]
        if { [llength $bbox] } {
            set id [eval $path:cmd create rectangle $bbox -fill $selbg -outline $selbg -tags [list "sel s:$item"]]
            $path:cmd itemconfigure "n:$item" -fill $selfg
            $path:cmd lower $id
        }
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::_redraw_listbox
# ------------------------------------------------------------------------------
proc ListBox::_redraw_listbox { path } {
    variable $path
    upvar 0  $path data

    if { [Widget::getoption $path -redraw] } {
        if { $data(upd,level) == 2 } {
            _redraw_items $path
        }
        _redraw_selection $path
        _update_scrollregion $path
        set data(upd,level)   0
        set data(upd,afterid) ""
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::_redraw_idle
# ------------------------------------------------------------------------------
proc ListBox::_redraw_idle { path level } {
    variable $path
    upvar 0  $path data

    if { $data(nrows) != -1 } {
        # widget is realized
        if { [Widget::getoption $path -redraw] && $data(upd,afterid) == "" } {
            set data(upd,afterid) [after idle ListBox::_redraw_listbox $path]
        }
    }
    if { $level > $data(upd,level) } {
        set data(upd,level) $level
    }
    return ""
}


# ------------------------------------------------------------------------------
#  Command ListBox::_resize
# ------------------------------------------------------------------------------
proc ListBox::_resize { path } {
    variable $path
    upvar 0  $path data

    if { [Widget::getoption $path -multicolumn] } {
        set bd    [expr {[$path:cmd cget -borderwidth]+[$path:cmd cget -highlightthickness]}]
        set h     [expr {[winfo height $path] - 2*$bd}]
        set nrows [expr {$h/[$path:cmd cget -yscrollincrement]}]
        if { $nrows == 0 } {
            set nrows 1
        }
        if { $nrows != $data(nrows) } {
            set data(nrows) $nrows
            _redraw_idle $path 2
        } else {
            _update_scrollregion $path
        }
    } elseif { $data(nrows) == -1 } {
        # first Configure event
        set data(nrows) 0
        ListBox::_redraw_listbox $path
    } else {
        _update_scrollregion $path
    }
}


# ------------------------------------------------------------------------------
#  Command ListBox::_init_drag_cmd
# ------------------------------------------------------------------------------
proc ListBox::_init_drag_cmd { path X Y top } {
    set ltags [$path:cmd gettags current]
    set item  [lindex $ltags 0]
    if { ![string compare $item "item"] ||
         ![string compare $item "img"]  ||
         ![string compare $item "win"] } {
        set item [string range [lindex $ltags 1] 2 end]
        if { [set cmd [Widget::getoption $path -draginitcmd]] != "" } {
            return [uplevel \#0 $cmd [list $path $item $top]]
        }
        if { [set type [Widget::getoption $path -dragtype]] == "" } {
            set type "LISTBOX_ITEM"
        }
        if { [set img [Widget::getoption $path.$item -image]] != "" } {
            pack [label $top.l -image $img -padx 0 -pady 0]
        }
        return [list $type {copy move link} $item]
    }
    return {}
}


# ------------------------------------------------------------------------------
#  Command ListBox::_drop_cmd
# ------------------------------------------------------------------------------
proc ListBox::_drop_cmd { path source X Y op type dnddata } {
    variable $path
    upvar 0  $path data

    if { [string length $data(dnd,afterid)] } {
        after cancel $data(dnd,afterid)
        set data(dnd,afterid) ""
    }
    $path:cmd delete drop
    set data(dnd,scroll) ""
    if { [llength $data(dnd,item)] } {
        if { [set cmd [Widget::getoption $path -dropcmd]] != "" } {
            return [uplevel \#0 $cmd [list $path $source $data(dnd,item) $op $type $dnddata]]
        }
    }
    return 0
}


# ------------------------------------------------------------------------------
#  Command ListBox::_over_cmd
# ------------------------------------------------------------------------------
proc ListBox::_over_cmd { path source event X Y op type dnddata } {
    variable $path
    upvar 0  $path data

    if { ![string compare $event "leave"] } {
        # we leave the window listbox
        $path:cmd delete drop
        if { [string length $data(dnd,afterid)] } {
            after cancel $data(dnd,afterid)
            set data(dnd,afterid) ""
        }
        set data(dnd,scroll) ""
        return 0
    }

    if { ![string compare $event "enter"] } {
        # we enter the window listbox - dnd data initialization
        set mode [Widget::getoption $path -dropovermode]
        set data(dnd,mode) 0
        foreach c {w p i} {
            set data(dnd,mode) [expr {($data(dnd,mode) << 1) | ([string first $c $mode] != -1)}]
        }
    }

    set x [expr {$X-[winfo rootx $path]}]
    set y [expr {$Y-[winfo rooty $path]}]
    $path:cmd delete drop
    set data(dnd,item) ""

    # test for auto-scroll unless mode is widget only
    if { $data(dnd,mode) != 4 && [_auto_scroll $path $x $y] != "" } {
        return 2
    }

    if { $data(dnd,mode) & 4 } {
        # dropovermode includes widget
        set target [list widget]
        set vmode  4
    } else {
        set target [list ""]
        set vmode  0
    }

    if { $data(dnd,mode) & 3 } {
        # dropovermode includes item or position
        # we extract the box (xi,yi,xs,ys) where we can find item around x,y
        set len  [llength $data(items)]
        set xc   [$path:cmd canvasx $x]
        set yc   [$path:cmd canvasy $y]
        set dy   [$path:cmd cget -yscrollincrement]
        set line [expr {int($yc/$dy)}]
        set yi   [expr {$line*$dy}]
        set ys   [expr {$yi+$dy}]
        set xi   0
        set pos  $line
        if { [Widget::getoption $path -multicolumn] } {
            set nrows $data(nrows)
        } else {
            set nrows $len
        }
        if { $line < $nrows } {
            foreach xs $data(xlist) {
                if { $xc <= $xs } {
                    break
                }
                set  xi  $xs
                incr pos $nrows
            }
            if { $pos < $len } {
                set item [lindex $data(items) $pos]
                if { $data(dnd,mode) & 1 } {
                    # dropovermode includes item
                    lappend target $item
                    set vmode [expr {$vmode | 1}]
                } else {
                    lappend target ""
                }

                if { $data(dnd,mode) & 2 } {
                    # dropovermode includes position
                    if { $yc >= $yi+$dy/2 } {
                        # position is after $item
                        incr pos
                        set yl $ys
                    } else {
                        # position is before $item
                        set yl $yi
                    }
                    lappend target $pos
                    set vmode [expr {$vmode | 2}]
                } else {
                    lappend target ""
                }
            } else {
                lappend target "" ""
            }
        } else {
            lappend target "" ""
        }

        if { ($vmode & 3) == 3 } {
            # result have both item and position
            # we compute what is the preferred method
            if { $yc-$yi <= 3 || $ys-$yc <= 3 } {
                lappend target "position"
            } else {
                lappend target "item"
            }
        }
    }

    if { $vmode && [set cmd [Widget::getoption $path -dropovercmd]] != "" } {
        # user-defined dropover command
        set res   [uplevel \#0 $cmd [list $source $target $op $type $dnddata]]
        set code  [lindex $res 0]
        set vmode 0
        if { $code & 1 } {
            # update vmode
            set mode [lindex $res 1]
            if { ![string compare $mode "item"] } {
                set vmode 1
            } elseif { ![string compare $mode "position"] } {
                set vmode 2
            } elseif { ![string compare $mode "widget"] } {
                set vmode 4
            }
        }
    } else {
        if { ($vmode & 3) == 3 } {
            # result have both item and position
            # we choose the preferred method
            if { ![string compare [lindex $target 3] "position"] } {
                set vmode [expr {$vmode & ~1}]
            } else {
                set vmode [expr {$vmode & ~2}]
            }
        }

        if { $data(dnd,mode) == 4 || $data(dnd,mode) == 0 } {
            # dropovermode is widget or empty - recall is not necessary
            set code 1
        } else {
            set code 3
        }
    }

    # draw dnd visual following vmode
    if { $vmode & 1 } {
        set data(dnd,item) [list "item" [lindex $target 1]]
        $path:cmd create rectangle $xi $yi $xs $ys -tags drop
    } elseif { $vmode & 2 } {
        set data(dnd,item) [concat "position" [lindex $target 2]]
        $path:cmd create line $xi $yl $xs $yl -tags drop
    } elseif { $vmode & 4 } {
        set data(dnd,item) [list "widget"]
    } else {
        set code [expr {$code & 2}]
    }

    if { $code & 1 } {
        DropSite::setcursor based_arrow_down
    } else {
        DropSite::setcursor dot
    }
    return $code
}


# ------------------------------------------------------------------------------
#  Command ListBox::_auto_scroll
# ------------------------------------------------------------------------------
proc ListBox::_auto_scroll { path x y } {
    variable $path
    upvar 0  $path data

    set xmax   [winfo width  $path]
    set ymax   [winfo height $path]
    set scroll {}
    if { $y <= 6 } {
        if { [lindex [$path:cmd yview] 0] > 0 } {
            set scroll [list yview -1]
            DropSite::setcursor sb_up_arrow
        }
    } elseif { $y >= $ymax-6 } {
        if { [lindex [$path:cmd yview] 1] < 1 } {
            set scroll [list yview 1]
            DropSite::setcursor sb_down_arrow
        }
    } elseif { $x <= 6 } {
        if { [lindex [$path:cmd xview] 0] > 0 } {
            set scroll [list xview -1]
            DropSite::setcursor sb_left_arrow
        }
    } elseif { $x >= $xmax-6 } {
        if { [lindex [$path:cmd xview] 1] < 1 } {
            set scroll [list xview 1]
            DropSite::setcursor sb_right_arrow
        }
    }

    if { [string length $data(dnd,afterid)] && [string compare $data(dnd,scroll) $scroll] } {
        after cancel $data(dnd,afterid)
        set data(dnd,afterid) ""
    }

    set data(dnd,scroll) $scroll
    if { [llength $scroll] && ![string length $data(dnd,afterid)] } {
        set data(dnd,afterid) [after 200 ListBox::_scroll $path $scroll]
    }
    return $data(dnd,afterid)
}


# ------------------------------------------------------------------------------
#  Command ListBox::_scroll
# ------------------------------------------------------------------------------
proc ListBox::_scroll { path cmd dir } {
    variable $path
    upvar 0  $path data

    if { ($dir == -1 && [lindex [$path:cmd $cmd] 0] > 0) ||
         ($dir == 1  && [lindex [$path:cmd $cmd] 1] < 1) } {
        $path $cmd scroll $dir units
        set data(dnd,afterid) [after 100 ListBox::_scroll $path $cmd $dir]
    } else {
        set data(dnd,afterid) ""
        DropSite::setcursor dot
    }
}
