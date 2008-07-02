#***************************************************************
#*
#* MODULE:       panel_resize.tcl 1.0
#*
#* AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
#*
#* PURPOSE:		 Resize panel: useful when saving images for movie
#* 					at fixed resolution
#*
#*
#* COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
#*
#*               This program is free software under the
#*               GNU General Public License (>=v2).
#*               Read the file COPYING that comes with GRASS
#*               for details.
#*
#**************************************************************

proc mkresizePanel {BASE} {
#	global Nv_
    catch {destroy $BASE}

    #  Initialize panel info
    if [catch {set Nv_($BASE)}] {
	set panel [St_create {window name size priority} \
		       $BASE "Resize Draw Area" 1 5]
    } else {
	set panel $Nv_($BASE)
    }

    frame $BASE  -relief groove -borderwidth 2
    Nv_mkPanelname $BASE "Resize Panel"

    frame $BASE.top
    frame $BASE.bottom -bd 2 -relief groove
    frame $BASE.top2

    label $BASE.top.label -text "Resize:                     "
    pack $BASE.top.label -side left -fill y -pady 4

	label $BASE.top.lwidth -text width
	label $BASE.top.lheight -text height
	entry $BASE.top.width -width 6 -borderwidth 2 -relief sunken
	entry $BASE.top.height -width 6 -borderwidth 2 -relief sunken

	resize_panel_refresh $BASE.top
	resize_borders

	pack $BASE.top.lwidth $BASE.top.width $BASE.top.lheight $BASE.top.height -side left

    button $BASE.bottom.resize -text Resize -command "resize_togl $BASE.top"
    pack $BASE.bottom.resize -side left

    button $BASE.bottom.refresh -text Refresh -command "resize_panel_refresh $BASE.top"
    pack $BASE.bottom.refresh -side left

	button $BASE.bottom.close -text Close -command "Nv_closePanel $BASE"
    pack $BASE.bottom.close -side right

    pack $BASE.top $BASE.top2 $BASE.bottom \
    -expand 1 -fill both -side top

    return $panel
}

proc resize_togl {W} {
	global Nv_ Wborder Hborder
	set width [expr [$W.width get] + $Wborder]
	set height [expr [$W.height get] + $Hborder]
#	$Nv_(TOP).canvas configure -width $width
#	$Nv_(TOP).canvas configure -height $height
	wm geometry . "$width\x$height\+10+10"
}

proc resize_togl {W} {
	global Nv_ Wborder Hborder
	set width [expr [$W.width get] + $Wborder]
	set height [expr [$W.height get] + $Hborder]
	wm geometry . "$width\x$height\+10+10"
# as border depends from absolute geometry
# 	after first approx set, recalculate them and then
#	fine tune resizing again
	update
	resize_borders
	set width [expr [$W.width get] + $Wborder]
	set height [expr [$W.height get] + $Hborder]
	wm geometry . "$width\x$height\+10+10"
	update
	resize_panel_refresh $W
}


proc resize_panel_refresh {W} {
	global Nv_
	set wlst [$Nv_(TOP).canvas configure -width]
	set hlst [$Nv_(TOP).canvas configure -height]
	$W.width delete 0 end
	$W.height delete 0 end
	$W.width insert 0 [lindex $wlst 4]
	$W.height insert 0  [lindex $hlst 4]
}

proc resize_borders {} {
	global Nv_ Wborder Hborder
	set wlst [$Nv_(TOP).canvas configure -width]
	set hlst [$Nv_(TOP).canvas configure -height]
	set Wborder [expr [winfo width .] - [lindex $wlst 4]]
	set Hborder [expr [winfo height .] - [lindex $hlst 4]]
#	puts "-------------------------------- W $Wborder H $Hborder"
}

