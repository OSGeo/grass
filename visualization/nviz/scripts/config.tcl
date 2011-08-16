##########################################################################
#
# Configuration and options database for NVIZ
# 
# Original author unknown.
# Possibly U.S. Army Construction Engineering Research Laboratory
#
#
# Updated 2006, Michael Barton, Arizona State University
#
##########################################################################
# COPYRIGHT:	(C) 2006 by Michael Barton and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################
# This script is setup by the toplevel configure shell script at
# installation time - new

global src_boot
global ProcessName
global nviztxtfont

set nviztxtfont {-*-helvetica-medium-r-normal-*-12-*-iso8859-1}

set gisbase $env(GISBASE)
set default_panel_path "$gisbase/etc/nviz2.2/scripts"
set bit_map_path "$gisbase/etc/nviz2.2/bitmaps"
set nv_path "$gisbase/etc/nviz2.2"

#Get ProcessName varaible set from nviz2.2_script
set ProcessName $env(NV_processname)

if {[info exists env(MSYSCON)]} {
    set mingw "1"
} else {
    set mingw "0"
}

# set correct devnull 
if {![catch {set env(OS)}] && $env(OS) == "Windows_NT"} {
        set mingw "1"
        set devnull "NUL:"
} else {
        set mingw "0"
        set devnull "/dev/null"
}

# Set up auto_path directories
if {[catch {set env(Nviz_PanelPath)} user_path]} then {
    set user_path [list]
} else {
    if { $mingw == "1" } {
        set user_path [split $user_path ;]
    } else {
        set user_path [split $user_path :]
    }
}

# If the -path option was used then append that directory also
if {[catch {set NvizAltPath}] == 0} then {
    global NvizAltPath
    lappend user_path $NvizAltPath
}

if {[lsearch -exact $user_path "$default_panel_path"] == -1} then {
    set user_path [linsert $user_path -1 "$default_panel_path"]
}
foreach i $user_path {
    lappend auto_path $i
}

# add the execution directory to the path
# MinGW
if { $mingw == "1" } {
    set env(PATH) "$default_panel_path;$nv_path;$env(PATH)"
} else {
    set env(PATH) "$default_panel_path:$nv_path:$env(PATH)"
}

# Override bindings for tk widgets
source $src_boot/etc/nviz2.2/scripts/extra_bindings.tcl

##########################################################################
#  Resources
##########################################################################
option add *background gray90 widgetDefault
option add *highlightBackground gray90 widgetDefault
option add *activeBackground gray80 widgetDefault
option add *font -*-helvetica-medium-r-normal-*-12-*-iso8859-1 widgetDefault
option add *Label.font -*-helvetica-bold-r-normal-*-12-*-iso8859-1
option add *label.font -*-helvetica-medium-r-normal-*-12-*-iso8859-1
option add *Label.foreground "khaki4"
option add *label.foreground "black"
option add *Radiobutton.relief flat
option add *Checkbutton.relief flat
option add *Scrollbar.troughcolor gray90 widgetDefault
option add *Scrollbar.background gray90 widgetDefault
option add *Scrollbar.activeBackground gray99 widgetDefault
option add *entry.background white
option add *Entry.background white
option add *entry.relief sunken
option add *Entry.relief sunken
option add *Radiobutton.highlightThickness 0
option add *Checkbutton.highlightThickness 0
option add *highlightThickness 0
option add *Scale.troughColor grey70 widgetDefault
option add *SpinBox.entrybg "white"
option add *ComboBox.entrybg "white"
option add *ComboBox.labelfont $nviztxtfont
option add *menubutton.relief raised
option add *menubutton.indicatoron 1
option add *menubutton.borderwidth 1
option add *button.borderWidth 1

