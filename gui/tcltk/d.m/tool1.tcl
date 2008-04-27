
namespace eval DmToolBar1 {
    variable toolbar
    variable mon
}


proc DmToolBar1::create { tb } {
    global dmpath
    global bgcolor
    variable toolbar

    set toolbar $tb

    # DISPLAY AND MONITOR SELECTION
    set bbox1 [ButtonBox $toolbar.bbox1 -spacing 0 -background $bgcolor ]
    
    # display
    $bbox1 add -image [image create photo -file "$dmpath/display.gif"] \
        -command "Dm::display" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Display active layers in current region"]

    # display all
    $bbox1 add -image [image create photo -file "$dmpath/display.all.gif"] \
        -command "Dm::displayall" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Display active layers in default region"]

    # display region
    $bbox1 add -image [image create photo -file "$dmpath/display.region.gif"] \
        -command "Dm::display_region" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Display active layers in saved region setting"]

    # erase
    $bbox1 add -image [image create photo -file "$dmpath/erase.gif"] \
        -command "Dm::erase" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Erase to white"]

    pack $bbox1 -side left -anchor w

    set sep1 [Separator $toolbar.sep1 -orient vertical -background aquamarine2 ]
    pack $sep1 -side left -fill y -padx 5 -anchor w
    
    # 3D AND ANIMATION
    set bbox2 [ButtonBox $toolbar.bbox2 -spacing 0 -background $bgcolor ]

    # zoom
    $bbox2 add -image [image create photo -file "$dmpath/nviz.gif"] \
        -command "Dm::nviz" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "NVIZ - n dimensional visualization"]
    
    # zoom.back
    $bbox2 add -image [image create photo -file "$dmpath/fly.gif"] \
        -command "Dm::fly" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1\
        -helptext [G_msg "Fly through path for NVIZ"]

    # pan
    $bbox2 add -image [image create photo -file "$dmpath/xganim.gif"] \
        -command "Dm::xganim" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1\
        -helptext [G_msg "Animate raster map series"]


    pack $bbox2 -side left -anchor w

    set sep2 [Separator $toolbar.sep2 -orient vertical -background aquamarine2 ]
    pack $sep2 -side left -fill y -padx 5 -anchor w

    # DISPLAY TOOLS
    set bbox3 [ButtonBox $toolbar.bbox3 -background $bgcolor -spacing 0  ]

    # zoom
    $bbox3 add -image [image create photo -file "$dmpath/zoom.gif"] \
        -command "Dm::zoom" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Zoom"]
    
    # zoom.back
    $bbox3 add -image [image create photo -file "$dmpath/zoom.back.gif"] \
        -command "Dm::zoom_back" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1\
        -helptext [G_msg "Return to previous zoom"]

    # pan
    $bbox3 add -image [image create photo -file "$dmpath/pan.gif"] \
        -command "Dm::pan" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1\
        -helptext [G_msg "Pan and recenter"]

    # query
    $bbox3 add -image [image create photo -file "$dmpath/query.gif"] \
        -command "Dm::query" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Query map (select map first)"]

    # measure
    $bbox3 add -image [image create photo -file "$dmpath/measure.gif"] -command "Dm::measure"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Measure lengths and areas"]

    # position
    $bbox3 add -image [image create photo -file "$dmpath/position.gif"] -command "Dm::position"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Geographical position"]

    pack $bbox3 -side left -anchor w

    set sep3 [Separator $toolbar.sep3 -orient vertical -background aquamarine2 ]
    pack $sep3 -side left -fill y -padx 5 -anchor w

    # FILE & PRINT
    set bbox4 [ButtonBox $toolbar.bbox4 -spacing 0 -background $bgcolor ]

     $bbox4 add -image [image create photo -file "$dmpath/new.gif"] -command "Dm::new" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Create new workspace file (erase current workspace settings first)"]
    $bbox4 add -image [image create photo -file "$dmpath/open.gif"] -command "Dm::OpenFileBox $toolbar"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Open existing workspace file"]
    $bbox4 add -image [image create photo -file "$dmpath/save.gif"]  -command "Dm::SaveFileBox $toolbar"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Save workspace file"]
    $bbox4 add -image [image create photo -file "$dmpath/print.gif"]  -command "Dm::print" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Print raster & vector maps using ps.map"]

    pack $bbox4 -side left -anchor w

}

