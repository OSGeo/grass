# ------------------------------------------------------------------------------
#  notebook.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - NoteBook::create
#     - NoteBook::configure
#     - NoteBook::cget
#     - NoteBook::compute_size
#     - NoteBook::insert
#     - NoteBook::delete
#     - NoteBook::itemconfigure
#     - NoteBook::itemcget
#     - NoteBook::bindtabs
#     - NoteBook::raise
#     - NoteBook::see
#     - NoteBook::page
#     - NoteBook::pages
#     - NoteBook::index
#     - NoteBook::getframe
#     - NoteBook::_test_page
#     - NoteBook::_itemconfigure
#     - NoteBook::_compute_width
#     - NoteBook::_get_x_page
#     - NoteBook::_xview
#     - NoteBook::_highlight
#     - NoteBook::_select
#     - NoteBook::_redraw
#     - NoteBook::_draw_page
#     - NoteBook::_draw_arrows
#     - NoteBook::_draw_area
#     - NoteBook::_resize
#     - NoteBook::_realize
# ------------------------------------------------------------------------------

namespace eval NoteBook {
    ArrowButton::use

    namespace eval Page {
        Widget::declare NoteBook::Page {
            {-state      Enum       normal 0 {normal disabled}}
            {-createcmd  String     ""     0}
            {-raisecmd   String     ""     0}
            {-leavecmd   String     ""     0}
            {-image      TkResource ""     0 label}
            {-text       String     ""     0}
        }
    }

    Widget::declare NoteBook {
        {-foreground         TkResource "" 0 button}
        {-background         TkResource "" 0 button}
        {-activebackground   TkResource "" 0 button}
        {-activeforeground   TkResource "" 0 button}
        {-disabledforeground TkResource "" 0 button}
        {-font               TkResource "" 0 button}
        {-side               Enum       top 1 {top bottom}}
        {-homogeneous        Boolean 0   0}
        {-borderwidth        Int 1   0 {=1 =2}}
        {-width              Int 0   0 {=0 ""}}
        {-height             Int 0   0 {=0 ""}}

        {-repeatdelay        BwResource ""  0 ArrowButton}
        {-repeatinterval     BwResource ""  0 ArrowButton}

        {-fg                 Synonym -foreground}
        {-bg                 Synonym -background}
        {-bd                 Synonym -borderwidth}
    }

    Widget::addmap NoteBook "" :cmd {-background {}}
    Widget::addmap NoteBook ArrowButton .fg \
        {-foreground {} -background {} -activeforeground {} -activebackground {} \
             -borderwidth {} -repeatinterval {} -repeatdelay {} -disabledforeground {}}
    Widget::addmap NoteBook ArrowButton .fd \
        {-foreground {} -background {} -activeforeground {} -activebackground {} \
             -borderwidth {} -repeatinterval {} -repeatdelay {} -disabledforeground {}}

    variable _warrow 12

    proc ::NoteBook { path args } { return [eval NoteBook::create $path $args] }
    proc use {} {}
}


# ------------------------------------------------------------------------------
#  Command NoteBook::create
# ------------------------------------------------------------------------------
proc NoteBook::create { path args } {
    variable $path
    upvar 0  $path data

    Widget::init NoteBook $path $args

    set data(base)     0
    set data(select)   ""
    set data(pages)    {}
    set data(pages)    {}
    set data(cpt)      0
    set data(realized) 0
    set data(wpage)    0
    set data(hpage)    [expr {[font metrics [Widget::getoption $path -font] -linespace] + 6}]
    set bg             [Widget::getoption $path -background]

    # --- creation du canvas -----------------------------------------------------------------
    set w [expr {[Widget::getoption $path -width]+4}]
    set h [expr {[Widget::getoption $path -height]+$data(hpage)+4}]
    canvas $path -relief flat -bd 0 -highlightthickness 0 -bg $bg -width $w -height $h

    # --- creation des arrow -----------------------------------------------------------------
    eval ArrowButton::create $path.fg [Widget::subcget $path .fg] \
        -highlightthickness 0 \
        -type button  -dir left \
        -armcommand [list "NoteBook::_xview $path -1"]

    eval ArrowButton::create $path.fd [Widget::subcget $path .fd] \
        -highlightthickness 0 \
        -type button  -dir right \
        -armcommand [list "NoteBook::_xview $path 1"]

    set col       [BWidget::get3dcolor $path $bg]
    set data(dbg) [lindex $col 0]
    set data(lbg) [lindex $col 1]

    bind $path <Configure> "NoteBook::_realize $path"
    bind $path <Destroy>   "NoteBook::_destroy $path"

    rename $path ::$path:cmd
    proc ::$path { cmd args } "return \[eval NoteBook::\$cmd $path \$args\]"

    return $path
}


# ------------------------------------------------------------------------------
#  Command NoteBook::configure
# ------------------------------------------------------------------------------
proc NoteBook::configure { path args } {
    variable $path
    upvar 0  $path data

    set res [Widget::configure $path $args]
    set redraw 0
    if { [set chf [Widget::hasChanged $path -font font]] ||
         [Widget::hasChanged $path -homogeneous foo] } {
        if { $chf } {
            set data(hpage) [expr {[font metrics $font -linespace] + 6}]
        }
        _compute_width $path
        set redraw 1
    }
    if { [Widget::hasChanged $path -background bg] } {
        set col [BWidget::get3dcolor $path $bg]
        set data(dbg)  [lindex $col 0]
        set data(lbg)  [lindex $col 1]
        set redraw 1
    }
    if { [Widget::hasChanged $path -foreground  fg] ||
         [Widget::hasChanged $path -borderwidth bd] } {
        set redraw 1
    }
    set wc [Widget::hasChanged $path -width  w]
    set hc [Widget::hasChanged $path -height h]
    if { $wc || $hc } {
        $path:cmd configure -width [expr {$w+4}] -height [expr {$h + $data(hpage)+4}]
    } elseif { $redraw } {
        _redraw $path
    }

    return $res
}


# ------------------------------------------------------------------------------
#  Command NoteBook::cget
# ------------------------------------------------------------------------------
proc NoteBook::cget { path option } {
    return [Widget::cget $path $option]
}


# ------------------------------------------------------------------------------
#  Command NoteBook::compute_size
# ------------------------------------------------------------------------------
proc NoteBook::compute_size { path } {
    variable $path
    upvar 0  $path data

    set wmax 0
    set hmax 0
    update idletasks
    foreach page $data(pages) {
        set w    [winfo reqwidth  $path.f$page]
        set h    [winfo reqheight $path.f$page]
        set wmax [expr {$w>$wmax ? $w : $wmax}]
        set hmax [expr {$h>$hmax ? $h : $hmax}]
    }
    configure $path -width $wmax -height $hmax
}


# ------------------------------------------------------------------------------
#  Command NoteBook::insert
# ------------------------------------------------------------------------------
proc NoteBook::insert { path index page args } {
    variable $path
    upvar 0  $path data

    if { [lsearch $data(pages) $page] != -1 } {
        return -code error "page \"$page\" already exists"
    }

    Widget::init NoteBook::Page $path.f$page $args

    set data(pages) [linsert $data(pages) $index $page]
    if { ![winfo exists $path.f$page] } {
        frame $path.f$page \
            -relief flat -background [Widget::getoption $path -background] -borderwidth 10
        set data($page,realized) 0
    }
    _compute_width $path
    _draw_page $path $page 1
    _redraw $path

    return $path.f$page
}


# ------------------------------------------------------------------------------
#  Command NoteBook::delete
# ------------------------------------------------------------------------------
proc NoteBook::delete { path page {destroyframe 1} } {
    variable $path
    upvar 0  $path data

    set pos [_test_page $path $page]
    set data(pages) [lreplace $data(pages) $pos $pos]
    _compute_width $path
    $path:cmd delete p:$page
    if { $data(select) == $page } {
        set data(select) ""
    }
    if { $pos < $data(base) } {
        incr data(base) -1
    }
    if { $destroyframe } {
        destroy $path.f$page
    }
    _redraw $path
}


# ------------------------------------------------------------------------------
#  Command NoteBook::itemconfigure
# ------------------------------------------------------------------------------
proc NoteBook::itemconfigure { path page args } {
    _test_page $path $page
    set res [_itemconfigure $path $page $args]
    _redraw $path

    return $res
}


# ------------------------------------------------------------------------------
#  Command NoteBook::itemcget
# ------------------------------------------------------------------------------
proc NoteBook::itemcget { path page option } {
    _test_page $path $page
    return [Widget::cget $path.f$page $option]
}


# ------------------------------------------------------------------------------
#  Command NoteBook::bindtabs
# ------------------------------------------------------------------------------
proc NoteBook::bindtabs { path event script } {
    if { $script != "" } {
        $path:cmd bind "page" $event \
            "$script \[string range \[lindex \[$path:cmd gettags current\] 1\] 2 end\]"
    } else {
        $path:cmd bind "page" $event {}
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::move
# ------------------------------------------------------------------------------
proc NoteBook::move { path page index } {
    variable $path
    upvar 0  $path data

    set pos [_test_page $path $page]
    set data(pages) [linsert [lreplace $data(pages) $pos $pos] $index $page]
    _redraw $path
}


# ------------------------------------------------------------------------------
#  Command NoteBook::raise
# ------------------------------------------------------------------------------
proc NoteBook::raise { path {page ""} } {
    variable $path
    upvar 0  $path data

    if { $page != "" } {
        _test_page $path $page
        _select $path $page
    }
    return $data(select)
}


# ------------------------------------------------------------------------------
#  Command NoteBook::see
# ------------------------------------------------------------------------------
proc NoteBook::see { path page } {
    variable $path
    upvar 0  $path data

    set pos [_test_page $path $page]
    if { $pos < $data(base) } {
        set data(base) $pos
        _redraw $path
    } else {
        set w     [expr {[winfo width $path]-1}]
        set fpage [expr {[_get_x_page $path $pos] + $data($page,width) + 6}]
        set idx   $data(base)
        while { $idx < $pos && $fpage > $w } {
            set fpage [expr {$fpage - $data([lindex $data(pages) $idx],width)}]
            incr idx
        }
        if { $idx != $data(base) } {
            set data(base) $idx
            _redraw $path
        }
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::page
# ------------------------------------------------------------------------------
proc NoteBook::page { path first {last ""} } {
    variable $path
    upvar 0  $path data

    if { $last == "" } {
        return [lindex $data(pages) $first]
    } else {
        return [lrange $data(pages) $first $last]
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::pages
# ------------------------------------------------------------------------------
proc NoteBook::pages { path {first ""} {last ""}} {
    variable $path
    upvar 0  $path data

    if { ![string length $first] } {
	return $data(pages)
    }

    if { ![string length $last] } {
        return [lindex $data(pages) $first]
    } else {
        return [lrange $data(pages) $first $last]
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::index
# ------------------------------------------------------------------------------
proc NoteBook::index { path page } {
    variable $path
    upvar 0  $path data

    return [lsearch $data(pages) $page]
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_destroy
# ------------------------------------------------------------------------------
proc NoteBook::_destroy { path } {
    variable $path
    upvar 0  $path data

    foreach page $data(pages) {
        Widget::destroy $path.f$page
    }
    Widget::destroy $path
    unset data
    rename $path {}
}


# ------------------------------------------------------------------------------
#  Command NoteBook::getframe
# ------------------------------------------------------------------------------
proc NoteBook::getframe { path page } {
    return $path.f$page
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_test_page
# ------------------------------------------------------------------------------
proc NoteBook::_test_page { path page } {
    variable $path
    upvar 0  $path data

    if { [set pos [lsearch $data(pages) $page]] == -1 } {
        return -code error "page \"$page\" does not exists"
    }
    return $pos
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_itemconfigure
# ------------------------------------------------------------------------------
proc NoteBook::_itemconfigure { path page lres } {
    variable $path
    upvar 0  $path data

    set res [Widget::configure $path.f$page $lres]
    if { [Widget::hasChanged $path.f$page -text foo] } {
        _compute_width $path
    } elseif  { [Widget::hasChanged $path.f$page -image foo] } {
        set data(hpage) [expr {[font metrics [Widget::getoption $path -font] -linespace] + 6}]
        _compute_width $path
    }
    if { [Widget::hasChanged $path.f$page -state state] &&
         $state == "disabled" && $data(select) == $page } {
        set data(select) ""
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_compute_width
# ------------------------------------------------------------------------------
proc NoteBook::_compute_width { path } {
    variable $path
    upvar 0  $path data

    set font [Widget::getoption $path -font]
    set wmax 0
    set hmax $data(hpage)
    set wtot 0
    if { ![info exists data(textid)] } {
        set data(textid) [$path:cmd create text 0 -100 -font [Widget::getoption $path -font] -anchor nw]
    }
    set id $data(textid)
    $path:cmd itemconfigure $id -font [Widget::getoption $path -font]
    foreach page $data(pages) {
        $path:cmd itemconfigure $id -text [Widget::getoption $path.f$page -text]
        set  wtext [expr {[lindex [$path:cmd bbox $id] 2]+20}]
        if { [set img [Widget::getoption $path.f$page -image]] != "" } {
            set wtext [expr {$wtext+[image width $img]+4}]
            set himg  [expr {[image height $img]+6}]
            if { $himg > $hmax } {
                set hmax $himg
            }
        }
        set  wmax  [expr {$wtext>$wmax ? $wtext : $wmax}]
        incr wtot  $wtext
        set  data($page,width) $wtext
    }
    if { [Widget::getoption $path -homogeneous] } {
        foreach page $data(pages) {
            set data($page,width) $wmax
        }
        set wtot [expr {$wmax * [llength $data(pages)]}]
    }
    set data(hpage) $hmax
    set data(wpage) $wtot
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_get_x_page
# ------------------------------------------------------------------------------
proc NoteBook::_get_x_page { path pos } {
    variable _warrow
    variable $path
    upvar 0  $path data

    set base $data(base)
    set x    [expr {$_warrow+1}]
    if { $pos < $base } {
        foreach page [lrange $data(pages) $pos [expr {$base-1}]] {
            incr x [expr {-$data($page,width)}]
        }
    } elseif { $pos > $base } {
        foreach page [lrange $data(pages) $base [expr {$pos-1}]] {
            incr x $data($page,width)
        }
    }
    return $x
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_xview
# ------------------------------------------------------------------------------
proc NoteBook::_xview { path inc } {
    variable $path
    upvar 0  $path data

    if { $inc == -1 } {
        set base [expr {$data(base)-1}]
        set dx $data([lindex $data(pages) $base],width)
    } else {
        set dx [expr {-$data([lindex $data(pages) $data(base)],width)}]
        set base [expr {$data(base)+1}]
    }

    if { $base >= 0 && $base < [llength $data(pages)] } {
        set data(base) $base
        $path:cmd move page $dx 0
        _draw_area   $path
        _draw_arrows $path
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_highlight
# ------------------------------------------------------------------------------
proc NoteBook::_highlight { type path page } {
    variable $path
    upvar 0  $path data

    if { ![string compare [Widget::getoption $path.f$page -state] "disabled"] } {
        return
    }

    switch -- $type {
        on {
            $path:cmd itemconfigure "$page:poly" -fill [Widget::getoption $path -activebackground]
            $path:cmd itemconfigure "$page:text" -fill [Widget::getoption $path -activeforeground]
        }
        off {
            $path:cmd itemconfigure "$page:poly" -fill [Widget::getoption $path -background]
            $path:cmd itemconfigure "$page:text" -fill [Widget::getoption $path -foreground]
        }
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_select
# ------------------------------------------------------------------------------
proc NoteBook::_select { path page } {
    variable $path
    upvar 0  $path data

    if { ![string compare [Widget::getoption $path.f$page -state] "normal"] } {
        set oldsel $data(select)
        if { [string compare $page $oldsel] } {
            if { $oldsel != "" } {
                if { [set cmd [Widget::getoption $path.f$oldsel -leavecmd]] != "" } {
                    if { [set code [catch {uplevel \#0 $cmd} res]] == 1 || $res == 0 } {
                        return -code $code $res
                    }
                }
                set data(select) ""
                _draw_page $path $oldsel 0
            }
            set data(select) $page
            if { $page != "" } {
                if { !$data($page,realized) } {
                    set data($page,realized) 1
                    if { [set cmd [Widget::getoption $path.f$page -createcmd]] != "" } {
                        uplevel \#0 $cmd
                    }
                }
                if { [set cmd [Widget::getoption $path.f$page -raisecmd]] != "" } {
                    uplevel \#0 $cmd
                }
                _draw_page $path $page 0
            }
            _draw_area $path
        }
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_redraw
# ------------------------------------------------------------------------------
proc NoteBook::_redraw { path } {
    variable $path
    upvar 0  $path data

    if { !$data(realized) } {
        return
    }

    foreach page $data(pages) {
        _draw_page $path $page 0
    }
    _draw_area   $path
    _draw_arrows $path
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_draw_page
# ------------------------------------------------------------------------------
proc NoteBook::_draw_page { path page create } {
    variable $path
    upvar 0  $path data

    # --- calcul des coordonnees et des couleurs de l'onglet ---------------------------------
    set pos [lsearch $data(pages) $page]
    set bg  [Widget::getoption $path -background]
    set h   $data(hpage)
    set xd  [_get_x_page $path $pos]
    set xf  [expr {$xd + $data($page,width)}]
    set lt  [list $xd $h $xd 4 [expr {$xd+3}] 1 $xf 1]
    set lb  [list $xf 1 [expr {$xf+3}] 4 [expr {$xf+3}] [expr {$h-3}] [expr {$xf+6}] $h]
    set img [Widget::getoption $path.f$page -image]
    if { $data(select) == $page } {
        set fgt   $data(lbg)
        set fgb   $data(dbg)
        set ytext [expr {$h/2-1}]
        if { $img == "" } {
            set xtext [expr {$xd+9}]
        } else {
            set ximg  [expr {$xd+9}]
            set xtext [expr {$ximg+[image width $img]+4}]
        }
        set bd    [Widget::getoption $path -borderwidth]
        set fg    [Widget::getoption $path -foreground]
    } else {
        set fgt   $data(dbg)
        set fgb   $fgt
        set ytext [expr {$h/2}]
        if { $img == "" } {
            set xtext [expr {$xd+10}]
        } else {
            set ximg  [expr {$xd+10}]
            set xtext [expr {$ximg+[image width $img]+4}]
        }
        set bd    1
        if { [Widget::getoption $path.f$page -state] == "normal" } {
            set fg [Widget::getoption $path -foreground]
        } else {
            set fg [Widget::getoption $path -disabledforeground]
        }
    }

    # --- creation ou modification de l'onglet -----------------------------------------------
    if { $create } {
        eval $path:cmd create polygon [concat $lt $lb] \
            -tag     {"page p:$page $page:poly"} \
            -outline $bg \
            -fill    $bg
        eval $path:cmd create line $lt -tags {"page p:$page $page:top top"} -fill $fgt -width $bd
        eval $path:cmd create line $lb -tags {"page p:$page $page:bot bot"} -fill $fgb -width $bd
        $path:cmd create text $xtext $ytext           \
            -text   [Widget::getoption $path.f$page -text] \
            -font   [Widget::getoption $path -font]        \
            -fill   $fg                               \
            -anchor w                                 \
            -tags   "page p:$page $page:text"

        $path:cmd bind p:$page <ButtonPress-1> "NoteBook::_select $path $page"
        $path:cmd bind p:$page <Enter>         "NoteBook::_highlight on  $path $page"
        $path:cmd bind p:$page <Leave>         "NoteBook::_highlight off $path $page"
    } else {
        eval $path:cmd coords "$page:poly" [concat $lt $lb]
        eval $path:cmd coords "$page:top"  $lt
        eval $path:cmd coords "$page:bot"  $lb
        $path:cmd coords "$page:text" $xtext $ytext

        $path:cmd itemconfigure "$page:poly" -fill $bg  -outline $bg
        $path:cmd itemconfigure "$page:top"  -fill $fgt -width $bd
        $path:cmd itemconfigure "$page:bot"  -fill $fgb -width $bd
        $path:cmd itemconfigure "$page:text"    \
            -text [Widget::getoption $path.f$page -text]     \
            -font [Widget::getoption $path -font]    \
            -fill $fg
    }
    if { $img != "" } {
        if { [set id [$path:cmd find withtag $page:img]] == "" } {
            $path:cmd create image $ximg $ytext \
                -image  $img \
                -anchor w    \
                -tags   "page p:$page $page:img"
        } else {
            $path:cmd coords $id $ximg $ytext
            $path:cmd itemconfigure $id -image $img
        }
    } else {
        $path:cmd delete $page:img
    }

    if { $data(select) == $page } {
        $path:cmd raise p:$page
    } elseif { $pos == 0 } {
        if { $data(select) == "" } {
            $path:cmd raise p:$page
        } else {
            $path:cmd lower p:$page p:$data(select)
        }
    } else {
        set pred [lindex $data(pages) [expr {$pos-1}]]
        if { $data(select) != $pred || $pos == 1 } {
            $path:cmd lower p:$page p:$pred
        } else {
            $path:cmd lower p:$page p:[lindex $data(pages) [expr {$pos-2}]]
        }
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_draw_arrows
# ------------------------------------------------------------------------------
proc NoteBook::_draw_arrows { path } {
    variable _warrow
    variable $path
    upvar 0  $path data

    set w       [expr {[winfo width $path]-1}]
    set h       [expr {$data(hpage)-1}]
    set nbpages [llength $data(pages)]
    set xl      0
    set xr      [expr {$w-$_warrow+1}]

    if { $data(base) > 0 } {
        if { ![llength [$path:cmd find withtag "leftarrow"]] } {
            $path:cmd create window $xl 1 \
                -width  $_warrow          \
                -height $h                \
                -anchor nw                \
                -window $path.fg          \
                -tags   "leftarrow"
        } else {
            $path:cmd coords "leftarrow" $xl 1
            $path:cmd itemconfigure "leftarrow" -width $_warrow -height $h
        }
    } else {
        $path:cmd delete "leftarrow"
    }

    if { $data(base) < $nbpages-1 &&
         $data(wpage) + [_get_x_page $path 0] + 6 > $w } {
        if { ![llength [$path:cmd find withtag "rightarrow"]] } {
            $path:cmd create window $xr 1 \
                -width  $_warrow          \
                -height $h                \
                -window $path.fd          \
                -anchor nw                \
                -tags   "rightarrow"
        } else {
            $path:cmd coords "rightarrow" $xr 1
            $path:cmd itemconfigure "rightarrow" -width $_warrow -height $h
        }
    } else {
        $path:cmd delete "rightarrow"
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_draw_area
# ------------------------------------------------------------------------------
proc NoteBook::_draw_area { path } {
    variable $path
    upvar 0  $path data

    set w   [expr {[winfo width  $path]-1}]
    set h   [expr {[winfo height $path]-1}]
    set bd  [Widget::getoption $path -borderwidth]
    set x0  [expr {$bd-1}]
    set y0  $data(hpage)
    set y1  $h
    set dbg $data(dbg)
    set sel $data(select)
    if {  $sel == "" } {
        set xd  [expr {$w/2}]
        set xf  $xd
        set lbg $data(dbg)
    } else {
        set xd [_get_x_page $path [lsearch $data(pages) $data(select)]]
        set xf [expr {$xd + $data($sel,width)+6}]
        set lbg $data(lbg)
    }

    if { [llength [$path:cmd find withtag rect]] } {
        $path:cmd coords "toprect1" $xd $y0 $x0 $y0 $x0 $h
        $path:cmd coords "toprect2" $w $y0 $xf $y0
        $path:cmd coords "botrect"  $x0 $h $w $h $w $y0
        $path:cmd itemconfigure "toprect1" -fill $lbg -width $bd
        $path:cmd itemconfigure "toprect2" -fill $lbg -width $bd
        $path:cmd itemconfigure "botrect"  -width $bd
        $path:cmd raise "rect"
    } else {
        $path:cmd create line $xd $y0 $x0 $y0 $x0 $y1 \
            -tags "rect toprect1" -fill $lbg -width $bd
        $path:cmd create line $w $y0 $xf $y0 \
            -tags "rect toprect2" -fill $lbg -width $bd
        $path:cmd create line 1 $h $w $h $w $y0 \
            -tags "rect botrect"  -fill $dbg -width $bd
    }

    if { $sel != "" } {
        if { [llength [$path:cmd find withtag "window"]] } {
            $path:cmd coords "window" 2 [expr {$y0+1}]
            $path:cmd itemconfigure "window"    \
                -width  [expr {$w-3}]        \
                -height [expr {$h-$y0-3}]    \
                -window $path.f$sel
        } else {
            set y0 $data(hpage)
            $path:cmd create window 2 [expr {$y0+1}] \
                -width  [expr {$w-3}]           \
                -height [expr {$h-$y0-3}]       \
                -anchor nw                      \
                -tags   "window"                \
                -window $path.f$sel
        }
    } else {
        $path:cmd delete "window"
    }
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_resize
# ------------------------------------------------------------------------------
proc NoteBook::_resize { path } {
    _draw_area   $path
    _draw_arrows $path
}


# ------------------------------------------------------------------------------
#  Command NoteBook::_realize
# ------------------------------------------------------------------------------
proc NoteBook::_realize { path } {
    variable $path
    upvar 0  $path data

    if { [set width  [Widget::getoption $path -width]]  == 0 ||
         [set height [Widget::getoption $path -height]] == 0 } {
        compute_size $path
    }

    set data(realized) 1
    _draw_area $path
    _draw_arrows $path
    bind $path <Configure> "NoteBook::_resize $path"
}
