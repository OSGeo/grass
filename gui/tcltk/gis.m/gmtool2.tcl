###############################################################
# gmtool2.tcl - lower toolbar file for GRASS GIS Manager main window
# January 2006 Michael Barton, Arizona State University
###############################################################

namespace eval GmToolBar2 {
    variable toolbar
    variable mon
}


proc GmToolBar2::create { tb } {
    global iconpath
    global bgcolor
    variable toolbar

    set toolbar $tb

    # OTHER LAYERS
    set bbox1 [ButtonBox $toolbar.bbox1 -spacing 0 ]

    # add scale and north arrow
    $bbox1 add -image [image create photo -file "$iconpath/module-d.barscale.gif"] -command "GmTree::add barscale" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1 \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Scalebar and north arrow"]

    # add grid and lines
    $bbox1 add -image [image create photo -file "$iconpath/module-d.grid.gif"] -command "GmTree::add gridline"\
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1 \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Overlay grids and lines"]

	# add command
    $bbox1 add -image [image create photo -file "$iconpath/gui-cmd.gif"] \
        -command "GmTree::add cmd" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Add command layer"]

    # add frame (frames not functional in GIS Manager currently)
#    $bbox1 add -image [image create photo -file "$iconpath/module-d.frame.gif"] -command "GmTree::add dframe"\
#        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1 \
#        -helptext [G_msg "Create or select display frame"]

    pack $bbox1 -side left -anchor w

    set sep1 [Separator $toolbar.sep1 -orient vertical -background $bgcolor ]
    pack $sep1 -side left -fill y -padx 5 -anchor w

    # LAYER MANAGEMENT
    set bbox2 [ButtonBox $toolbar.bbox2 -spacing 0 ]

    # add group
    $bbox2 add -image [image create photo -file "$iconpath/gui-group.gif"] \
        -command "GmTree::add group" -borderwidth 1\
        -highlightthickness 0 -takefocus 0 -relief link \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Add group"]

    $bbox2 add -image [image create photo -file "$iconpath/edit-copy.gif"] \
        -command "GmTree::duplicate" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Duplicate Layer"]    

    $bbox2 add -image [image create photo -file "$iconpath/edit-cut.gif"] -command \
    	"GmTree::delete" -highlightthickness 0 -takefocus 0 -relief link \
        -highlightbackground $bgcolor -activebackground $bgcolor \
    	-borderwidth 1 -helptext [G_msg "Delete layer"]
       
    pack $bbox2 -side left -anchor w

    set sep2 [Separator $toolbar.sep2 -orient vertical -background $bgcolor ]
    pack $sep2 -side left -fill y -padx 5 -anchor w

 # LAYER FILES
    set bbox3 [ButtonBox $toolbar.bbox3 -spacing 0 ]

     $bbox3 add -image [image create photo -file "$iconpath/file-new.gif"] \
     	-command "GmTree::new" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1 \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Create new workspace file (erase current workspace settings first)"]

    $bbox3 add -image [image create photo -file "$iconpath/file-open.gif"] \
    	-command "GmLib::OpenFileBox"\
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1 \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Open existing workspace file"]

    $bbox3 add -image [image create photo -file "$iconpath/file-save.gif"]  \
    	-command "GmLib::SaveFileBox"\
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Save workspace file"]

    pack $bbox3 -side left -anchor w
    
    set sep3 [Separator $toolbar.sep3 -orient vertical -background $bgcolor ]
    pack $sep3 -side left -fill y -padx 5 -anchor w

    # 3D AND ANIMATION
    set bbox4 [ButtonBox $toolbar.bbox4 -spacing 0 ]

    # animate
    $bbox4 add -image [image create photo -file "$iconpath/module-xganim.gif"] \
        -command "GmAnim::main" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1\
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Animate raster map series"]


    pack $bbox4 -side left -anchor w

    set sep4 [Separator $toolbar.sep4 -orient vertical -background $bgcolor ]
    pack $sep4 -side left -fill y -padx 5 -anchor w

       # DIGITIZE
    set bbox5 [ButtonBox $toolbar.bbox5 -spacing 20 ]
    
    #digitize
    $bbox5 add -image [image create photo -file "$iconpath/module-v.digit.gif"] \
        -command "GmTree::vedit" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor -activebackground $bgcolor \
        -helptext [G_msg "Digitize map (select or create new map first)"]

    pack $bbox5 -side left -anchor w

    

}

