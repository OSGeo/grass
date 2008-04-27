
namespace eval DmToolBar2 {
    variable toolbar
}

proc DmToolBar2::create { tb } {
    global dmpath
    global bgcolor
    variable toolbar

    set toolbar $tb



    # Raster Layers
    set bbox1 [ButtonBox $toolbar.bbox1 -spacing 0 -background $bgcolor ]
    
    # add raster
    $bbox1 add -image [image create photo -file "$dmpath/raster.gif"] \
        -command "Dm::add raster" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add raster layer"]

    # add RGB or HIS layer
    $bbox1 add -image [image create photo -file "$dmpath/rgbhis.gif"] \
        -command "Dm::add rgbhis" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add RGB or HIS layer"]

    # add legend
    $bbox1 add -image [image create photo -file "$dmpath/legend.gif"] -command "Dm::add legend"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add legend"]

    pack $bbox1 -side left -anchor w

    set sep1 [Separator $toolbar.sep1 -orient vertical -background aquamarine2 ]
    pack $sep1 -side left -fill y -padx 5 -anchor w


    # VECTOR LAYERS
    set bbox2 [ButtonBox $toolbar.bbox2 -spacing 0 -background $bgcolor ]

    # add vector
    $bbox2 add -image [image create photo -file "$dmpath/vector.gif"] \
        -command "Dm::add vector" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add vector layer"]

    # add chart
    $bbox2 add -image [image create photo -file "$dmpath/chart.gif"] \
        -command "Dm::add chart" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add thematic charts layer"]

    pack $bbox2 -side left -anchor w

    # add thematic
    $bbox2 add -image [image create photo -file "$dmpath/thematic.gif"] \
        -command "Dm::add thematic" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add thematic map layer"]

    pack $bbox2 -side left -anchor w

    set sep2 [Separator $toolbar.sep2 -orient vertical -background aquamarine2 ]
    pack $sep2 -side left -fill y -padx 5 -anchor w

    # Text Layers
    set bbox3 [ButtonBox $toolbar.bbox3 -spacing 0 -background $bgcolor ]

    # add paint labels
    $bbox3 add -image [image create photo -file "$dmpath/labels.gif"] \
        -command "Dm::add labels" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add paint labels layer (from directory paint/labels)"]

    # add freetype text
    $bbox3 add -image [image create photo -file "$dmpath/fttext.gif"] \
        -command "Dm::add fttext" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add freetype text layer"]

    # add text
    $bbox3 add -image [image create photo -file "$dmpath/dtext.gif"] \
        -command "Dm::add dtext" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add text layer"]
    pack $bbox3 -side left -anchor w

    set sep3 [Separator $toolbar.sep3 -orient vertical -background aquamarine2 ]
    pack $sep3 -side left -fill y -padx 5 -anchor w

    # OTHER LAYERS
    set bbox4 [ButtonBox $toolbar.bbox4 -spacing 0 -background $bgcolor ]

    # add scale and north arrow
    $bbox4 add -image [image create photo -file "$dmpath/barscale.gif"] -command "Dm::add barscale" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Scalebar and north arrow"]

    # add grid and lines
    $bbox4 add -image [image create photo -file "$dmpath/grid.gif"] -command "Dm::add gridline"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Overlay grids and lines"]

    # add frame
    $bbox4 add -image [image create photo -file "$dmpath/frames.gif"] -command "Dm::add dframe"\
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Create or select display frame"]

    # add command
    $bbox4 add -image [image create photo -file "$dmpath/cmd.gif"] \
        -command "Dm::add cmd" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Add command layer"]

    pack $bbox4 -side left -anchor w

    set sep4 [Separator $toolbar.sep4 -orient vertical -background aquamarine2 ]
    pack $sep4 -side left -fill y -padx 5 -anchor w

    # LAYER MANAGEMENT
    set bbox5 [ButtonBox $toolbar.bbox5 -spacing 0 -background $bgcolor ]

    # add group
    $bbox5 add -image [image create photo -file "$dmpath/group.gif"] \
        -command "Dm::add group" -borderwidth 1\
        -highlightthickness 0 -takefocus 0 -relief raised \
        -helptext [G_msg "Add group"]

    $bbox5 add -image [image create photo -file "$dmpath/copy.gif"] \
        -command "Dm::duplicate" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Duplicate Layer"]    

    $bbox5 add -image [image create photo -file "$dmpath/cut.gif"] -command \
    	"Dm::delete" -highlightthickness 0 -takefocus 0 -relief raised \
    	-borderwidth 1 -helptext [G_msg "Delete layer"]
       
    pack $bbox5 -side left -anchor w

    set sep5 [Separator $toolbar.sep5 -orient vertical -background aquamarine2 ]
    pack $sep5 -side left -fill y -padx 5 -anchor w

    # DIGITIZE
    set bbox6 [ButtonBox $toolbar.bbox6 -spacing 20 -background $bgcolor ]
    
    #digitize
    $bbox6 add -image [image create photo -file "$dmpath/dig.gif"] \
        -command "Dm::edit" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -helptext [G_msg "Digitize map (select or create new map first)"]

    pack $bbox6 -side left -anchor w


}

