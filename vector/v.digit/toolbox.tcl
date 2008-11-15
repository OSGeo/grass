lappend auto_path $env(GISBASE)/bwidget
package require -exact BWidget 1.2.1

source $env(GISBASE)/etc/gtcltk/gmsg.tcl
source $env(GISBASE)/etc/gtcltk/select.tcl

#fontcreate balloon-help -family Helvetica -size -12
DynamicHelp::configure -font balloon-help -fg black -bg "#FFFF77"

set vdpath $env(GISBASE)/etc/v.digit/
set iconpath $env(GISBASE)/etc/gui/icons/grass/vdigit/
source $vdpath/settings.tcl
source $vdpath/cats.tcl
source $vdpath/form.tcl

set env(GISDBASE) [exec g.gisenv get=GISDBASE]
set env(LOCATION_NAME) [exec g.gisenv get=LOCATION_NAME]
set env(MAPSET) [exec g.gisenv get=MAPSET]

set prompt [G_msg "Welcome to v.digit"]
set prompt_left [G_msg "Left button"]
set prompt_middle [G_msg "Middle button"]
set prompt_right [G_msg "Right button"]
set coor ""

wm title . "v.digit toolbox - $map_name@$map_mapset"

proc get_update_line {ox oy x y} {
    .screen.canvas delete active
    .screen.canvas create line $ox $oy $x $y -tags active -dash {4 4}
}

proc get_update_box {ox oy x y} {
    .screen.canvas delete active
    .screen.canvas create line $ox $oy $ox $y $x $y $x $oy $ox $oy -tags active -dash {4 4}
}

proc create_screen {} {
    global vdpath iconpath map_name map_mapset
    if {[winfo exists .screen]} return

    toplevel .screen
    canvas .screen.canvas -background white -width 640 -height 480
    pack .screen.canvas -fill both -expand yes
    bind .screen.canvas <ButtonPress> { c_update_tool %x %y %b }
    bind .screen.canvas <Motion> { c_update_tool %x %y -1 }
    wm title .screen "v.digit - $map_name@$map_mapset"
    wm withdraw .screen
    wm deiconify .screen
    update

    image create bitmap cross -file "$iconpath/cross.xbm"
    image create bitmap cross45 -file "$iconpath/cross45.xbm"
    image create bitmap box -file "$iconpath/box.xbm"
}

# GVariable stores variables by key, this variables are (should be) synchronized with
# variables in Variable array in C (synchronization should be done somehow better). Key is
# 'name' in VAR structure in C. Variables are initialized by var_init() on startup.
# For key list see VARN_* in global.h

# GWidget stores names of some widgets we need to access globaly, names:
# field - field Entry for new line
# cat - cat Entry for new line

# Create new line options
proc new_line_options { create } {
    global GVariable GWidget
    if { $create } {
    set lineopt [frame .lineopt]
    pack $lineopt -fill x -side top

    set row1 [frame $lineopt.row1]
    pack $row1 -fill x -side top

    Label $row1.flab -padx 2 -pady 2 -relief flat -anchor w -text [G_msg "  Layer"]
    set GWidget(field) [Entry $row1.fval -width 6 -textvariable GVariable(field) \
                -bg white -command { c_var_set field $GVariable(field) } ]
        bind $GWidget(field) <KeyRelease> { c_var_set field $GVariable(field) }
    Label $row1.clab -padx 2 -pady 2 -relief flat -anchor w -text [G_msg "  Category"]
    set GWidget(cat) [Entry $row1.cval -width 6 -textvariable GVariable(cat) \
                -bg white -command { c_var_set cat $GVariable(cat) }]
        bind $GWidget(cat) <KeyRelease> { c_var_set cat $GVariable(cat) }
    set GWidget(cat_mode) [ComboBox $row1.cmode -label [G_msg "  Mode "] \
            -entrybg white -width 12 -textvariable cmode \
            -modifycmd {
                set GVariable(cat_mode) [ $GWidget(cat_mode) getvalue]
                            c_var_set cat_mode $GVariable(cat_mode)
             }]
    pack $row1.flab $GWidget(field) $row1.clab $GWidget(cat) $GWidget(cat_mode) -fill x  -side left

    set row2 [frame $lineopt.row2]
    pack $row2 -fill x -side top

    checkbutton $row2.ins -variable GVariable(insert) \
            -padx 8 -pady 5 -text [G_msg "Insert new record into table"] \
            -command { c_var_set insert $GVariable(insert) }
    pack $row2.ins -fill x  -side left

    } else {
        destroy .lineopt
    }
}


# button frame row 2
set bbox2 [ButtonBox .bbox2 -spacing 1 -padx 1 -pady 1]

# --- Draw new ---
$bbox2 add -image [image create photo -file "$iconpath/new.point.gif"] \
        -command "c_next_tool new_point" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Digitize new point"]

$bbox2 add -image [image create photo -file "$iconpath/new.line.gif"] \
        -command "c_next_tool new_line" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Digitize new line"]

$bbox2 add -image [image create photo -file "$iconpath/new.boundary.gif"] \
        -command "c_next_tool new_boundary" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Digitize new boundary"]

$bbox2 add -image [image create photo -file "$iconpath/new.centroid.gif"] \
        -command "c_next_tool new_centroid" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Digitize new centroid"]

# --- Edit old ---
$bbox2 add -image [image create photo -file "$iconpath/move.vertex.gif"] \
        -command "c_next_tool move_vertex" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Move vertex"]

$bbox2 add -image [image create photo -file "$iconpath/add.vertex.gif"] \
        -command "c_next_tool add_vertex" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Add vertex"]

$bbox2 add -image [image create photo -file "$iconpath/rm.vertex.gif"] \
        -command "c_next_tool rm_vertex" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Remove vertex"]

$bbox2 add -image [image create photo -file "$iconpath/split.line.gif"] \
        -command "c_next_tool split_line" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Split line"]

$bbox2 add -image [image create photo -file "$iconpath/edit.line.gif"] \
        -command "c_next_tool edit_line" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Edit line/boundary"]

$bbox2 add -image [image create photo -file "$iconpath/move.line.gif"] \
        -command "c_next_tool move_line" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Move point, line, boundary, or centroid"]

$bbox2 add -image [image create photo -file "$iconpath/delete.line.gif"] \
        -command "c_next_tool delete_line" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Delete point, line, boundary, or centroid"]


# button frame row 1
set bbox1 [ButtonBox .bbox1 -spacing 1 -padx 1 -pady 1]

# --- Zoom / Display ---

$bbox1 add -image [image create photo -file "$iconpath/redraw.gif"] \
        -command "c_next_tool redraw" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Redraw"]

$bbox1 add -image [image create photo -file "$iconpath/zoom.window.gif"] \
        -command "c_next_tool zoom_window" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Zoom in by window"]

$bbox1 add -image [image create photo -file "$iconpath/zoom.out.centre.gif"] \
        -command "c_next_tool zoom_out_centre" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Zoom out"]

$bbox1 add -image [image create photo -file "$iconpath/zoom.pan.gif"] \
        -command "c_next_tool zoom_pan" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Pan"]

$bbox1 add -image [image create photo -file "$iconpath/zoom.default.gif"] \
        -command "c_next_tool zoom_default" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Zoom to default region"]

proc zoom_region { } {
    set reg [GSelect windows]
    if { $reg != "" } {
        c_var_set zoom_region $reg
        c_next_tool zoom_region
    }
}

$bbox1 add -image [image create photo -file "$iconpath/zoom.region.gif"] \
        -command "zoom_region" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Zoom to saved region"]

# --- Attributes ---
$bbox1 add -image [image create photo -file "$iconpath/display.cats.gif"] \
        -command "c_next_tool display_cats" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Display categories"]

$bbox1 add -image [image create photo -file "$iconpath/copy.cats.gif"] \
        -command "c_next_tool copy_cats" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Copy categories"]

$bbox1 add -image [image create photo -file "$iconpath/display.attributes.gif"] \
        -command "c_next_tool display_attributes" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Display attributes"]

# --- Stop ---
#$bbox1 add -image [image create photo -file "$iconpath/stop.gif"] \
#        -command "c_cancel" \
#        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
#        -helptext "Quit running tool"

# --- Others ---
$bbox1 add -image [image create photo -file "$iconpath/settings.gif"] \
        -command "c_next_tool settings" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -helptext [G_msg "Open settings"]

$bbox1 add -image [image create photo -file "$iconpath/exit.gif"] \
        -command "c_next_tool exit" \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1 \
        -bg white \
        -helptext [G_msg "Save and exit"]

pack $bbox1 -side top -anchor w
pack $bbox2 -side top -anchor w


frame .pf
pack .pf -fill x -side top
Label .pf.prompt -padx 3 -pady 2 -relief flat -anchor w -textvariable prompt -fg mediumblue
pack .pf.prompt -fill x  -side left

LabelFrame .bpf -text [G_msg "mouse button actions (left, middle, right)"] \
    -side top -anchor n -background HoneyDew2
# why doesn't the frame render?
#    -relief groove -borderwidth 1

pack .bpf -fill x -side top -padx 8
Label .bpf.left -width 10  -pady 5 -relief raised -anchor center -textvariable prompt_left \
	-wraplength 100 -bg grey95 
Label .bpf.middle -width 10 -padx 2 -pady 5 -relief raised -anchor center -textvariable prompt_middle \
	-wraplength 100 -bg grey95
Label .bpf.right -width 10 -pady 5 -relief raised -anchor center -textvariable prompt_right \
	-wraplength 100 -bg grey95
pack .bpf.left .bpf.middle .bpf.right -fill x -side left -expand yes -padx 4 -pady 8

frame .coorf
pack .coorf -fill x -side top -padx 8 -pady 5
Label .coorf.prompt -width 50 -padx 3 -pady 2 -relief flat -anchor w -textvariable coor
pack .coorf.prompt -fill x  -side left

bind . <Destroy> { if { "%W" == "."} { c_next_tool exit } }

# Start tool centre in C
c_tool_centre

tkwait window .
exit
