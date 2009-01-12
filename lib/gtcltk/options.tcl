############################################################################
#
# LIBRARY:      options.tcl gui options
# AUTHOR(S):    Cedric Shock (cedricgrass AT shockfamily.net)
# PURPOSE:      Default options and load user options
# COPYRIGHT:    (C) 2006 GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
############################################################################

lappend auto_path $env(GISBASE)/bwidget
package require -exact BWidget 1.2.1

# set background color and help font
# These globals are still used in a few places by things in gis.m
set bgcolor HoneyDew2

##############################################################################
# Create fonts

proc fontcreate {font args} {
	if {[lsearch [font names] $font] == -1} {
		eval font create $font $args
	} else {
		eval font configure $font $args
	}
}

fontcreate balloon-help -family Helvetica -size -12
fontcreate default -family Helvetica -size -12
fontcreate textfont -family Courier -size -12
fontcreate bolddefault -family Helvetica -size 12 -weight bold
fontcreate introfont -family Helvetica -size 14 -weight bold

global bolddefault
global introfont
global textfont
global default

##############################################################################
# Configure balloon help:

DynamicHelp::configure -font balloon-help -fg black -bg "#FFFF77"

##############################################################################
# Configure almost everything using the options database

# Font to use everywhere
option add *font default
# Font in labelframes of labels in bwidgets is prefixed with label:
option add *labelfont default

# Various background colors
option add *background #dddddd
option add *activeBackground #dddddd
option add *highlightBackground #dddddd
option add *ButtonBox.background HoneyDew2
option add *ButtonBox*add.highlightBackground HoneyDew2
option add *MainFrame.background HoneyDew2
option add *PanedWindow.background HoneyDew2
option add *Menu.background HoneyDew2
option add *listbox.background white
option add *addindicator.background white

# Things that are selected:
option add *selectBackground #ffff9b
option add *selectForeground black

# Menus use active instead of selected
option add *Menu.activeBackground #ffff9b
option add *Menu.activeForeground black

# Scrollbar trough color
option add *troughColor HoneyDew3

# Entry widgets and text widgets should have a white background
option add *Entry.background white
option add *entry.background white
option add *Entry.highlightbackground #dddddd
option add *entrybg white
option add *Text.background white
option add *Entry.font textfont
option add *Text.font textfont

# Options for map canvases
option add *mapcanvas.background #eeeeee
option add *mapcanvas.insertbackground black
option add *mapcanvas.selectbackground #c4c4c4
option add *mapcanvas.selectforeground black


##############################################################################
# Platform specific default settings:
# keycontrol is control key used in copy-paste bindings

set keycontrol "Control"

if {[info exists env(osxaqua)]} {
    set osxaqua $env(osxaqua)
} else {
    set osxaqua "0"
}

if { $osxaqua == "1"} {
    set keycontrol "Command"
}

if {[info exists env(OS)] && $env(OS) == "Windows_NT"} {
    set mingw "1"
} else {
    set mingw "0"
}
