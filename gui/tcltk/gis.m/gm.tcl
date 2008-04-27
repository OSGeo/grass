##########################################################################
#
# gm.tcl
#
# Primary tcltk script for GIS Manager: GUI for GRASS 6
# Author: Michael Barton (Arizona State University)
# Based in part on Display Manager for GRASS 5.7 by Radim Blazek (ITC-IRST)
# and tcltkgrass for GRASS 5.7 by Michael Barton (Arizona State University)--
# with contributions by Glynn Clements, Markus Neteler, Lorenzo Moretti,
# Florian Goessmann, and others
#
# March 2006
#
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

lappend auto_path $env(GISBASE)/bwidget

package require -exact BWidget 1.2.1

# Load up all the gis.m layers and things.
# pkgIndex.tcl only loads the files when they are first called.
lappend auto_path $env(GISBASE)/etc/gm
package require -exact GisM 1.0

# path to GIS Manager files
set gmpath $env(GISBASE)/etc/gm

# Load common procedure library
source $gmpath/gmlib.tcl

if {[catch {set env(GISDBASE) [exec g.gisenv get=GISDBASE]} error]} {
	Gm::errmsg $error
}
if {[catch {set env(LOCATION_NAME) [exec g.gisenv get=LOCATION_NAME]} error]} {
	Gm::errmsg $error
}
if {[catch {set env(MAPSET) [exec g.gisenv get=MAPSET]} error]} {
	Gm::errmsg $error
}

if {[catch {set gisdbase [exec g.gisenv get=GISDBASE]} error]} {
	Gm::errmsg $error
}
if {[catch {set location_name [exec g.gisenv get=LOCATION_NAME]} error]} {
	Gm::errmsg $error
}
if {[catch {set mapset [exec g.gisenv get=MAPSET]} error]} {
	Gm::errmsg $error
}


# path to icons for GIS Manager
set iconpath $env(GISBASE)/etc/gui/icons

global iconpath
global gmpath

set keycontrol "Control"
set tmenu "1"
set keyctrl "Ctrl"
set execom "execute"
set msg 0
set mon 1



if {![catch {set env(HOSTTYPE)}]} {
	set HOSTTYPE $env(HOSTTYPE)
} else {
	set HOSTTYPE ""
}

# add for OSX aqua
if {![catch {set env(osxaqua)}]} {
    set osxaqua $env(osxaqua)
} else {
    set osxaqua "0"
}

if { $osxaqua == "1"} {
    set keycontrol "Command"
    set tmenu "0"
    set keyctrl "Command"
    set execom "spawn"
}

if {![catch {set env(OS)}] && $env(OS) == "Windows_NT"} {
	set mingw "1"
	set devnull "NUL:"
} else {
	set mingw "0"
	set devnull "/dev/null"
}


#fetch GRASS Version number:
catch {set fp [open $env(GISBASE)/etc/VERSIONNUMBER r]}
set GRASSVERSION [read -nonewline $fp]

if {[catch {close $fp} error]} {
	Gm::errmsg $error
}


source $env(GISBASE)/etc/gui.tcl
# gui.tcl also sources these:
# $env(GISBASE)/etc/gtcltk/gmsg.tcl
# $env(GISBASE)/etc/gtcltk/options.tcl
# $env(GISBASE)/etc/gtcltk/select.tcl
# $env(GISBASE)/etc/gtcltk/gronsole.tcl

# Load a console user interface
source $gmpath/runandoutput.tcl

namespace eval Gm {
	variable gm_mainframe
	variable status
	variable array tree ;# mon
	variable rcfile
	variable moncount
	variable prgtext
	variable mainwindow
	variable dfont 
	variable selectedfont
	variable encoding
	global array filename ;# mon

}


set Gm::prgtext ""
global prgindic
global max_prgindic

set max_prgindic 20

if { $tcl_platform(platform) == "windows" } {
	append regexp .* $env(GISBASE) {[^;]*}
	regsub -- $regexp $env(PATH) "&;$env(GISBASE)/etc/gui/scripts" env(PATH)
} else {
	append regexp .* $env(GISBASE) {[^:]*}
	regsub -- $regexp $env(PATH) "&:$env(GISBASE)/etc/gui/scripts" env(PATH)
}

###############################################################################
# Deprecated
# Use guarantee_xmon and any run command instead.

proc Gm::xmon { type cmd } {
	guarantee_xmon

	if { $type == "term" } {
		term_panel $cmd
	} else {
		run_panel $cmd
	}

	return
}

###############################################################################


proc Gm::create { } {
	variable mainwindow
	variable prgtext
	variable gm_mainframe
	variable tree
	variable moncount
	variable dfont
	variable ecoding
	global gmpath
	global mon
	global tree_pane
	global options
	global pgs
	global prgindic
	global keycontrol
	global env
	
	# set display rendering environment for PNG/PPM output
	set env(GRASS_RENDER_IMMEDIATE) "TRUE"
	
	# set default font
	if {[catch {set env(GRASS_FONT)}]} {set env(GRASS_FONT) "romans"}
	set Gm::dfont ""
	set Gm::encoding "ISO-8859-1"

	set moncount 1

	set Gm::prgtext [G_msg "Loading GIS Manager"]
	set prgindic -1
	_create_intro
	update

	source $gmpath/gmmenu.tcl

	set Gm::prgtext [G_msg "Creating MainFrame..."]
    
	set gm_mainframe [MainFrame .mainframe \
		       -menu $descmenu \
		       -textvariable Gm::status \
		       -progressvar  Gm::prgindic ]

	set mainwindow [$gm_mainframe getframe]
    
	# toolbar 1 & 2 creation
	set tb1  [$gm_mainframe addtoolbar]
	GmToolBar1::create $tb1
	set tb2  [$gm_mainframe addtoolbar]
	GmToolBar2::create $tb2
	set pw1 [PanedWindow $mainwindow.pw1 -side left -pad 0 -width 10 ]
    
	# tree
	set treemon [expr {$mon + 1}]
	set tree_pane  [$pw1 add  -minsize 50 -weight 1]
	    set pgs [PagesManager $tree_pane.pgs]
    
    
	    pack $pgs -expand yes -fill both
    
    
	# options
	set options_pane  [$pw1 add -minsize 50 -weight 1]
	set options_sw [ScrolledWindow $options_pane.sw -relief flat -borderwidth 1]
	set options_sf [ScrollableFrame $options_sw.sf]
	$options_sf configure -height 145 -width 460
	$options_sw setwidget $options_sf
	set options [$options_sf getframe]
	pack $options_sw -fill both -expand yes
    
	# Scroll the options window with the mouse
	bind_scroll $options_sf
    
	pack $pw1 -side top -expand yes -fill both -anchor n
    
	    # finish up
	set Gm::prgtext [G_msg "Done"]
    
	set Gm::status [G_msg "Welcome to GRASS GIS"]
	$gm_mainframe showstatusbar status
    
	pack $gm_mainframe -fill both -expand yes

	Gm::startmon

	bind .mainframe <Destroy> {
		if {"%W" == ".mainframe"} {
			Gm::cleanup}
	}



}


###############################################################################

# start new display monitor and increment canvas monitor number
proc Gm::startmon { } {
	variable mainwindow
	variable moncount
	variable tree
	global mon

	set mon $moncount
	incr moncount 1

	#create initial display canvas and layer tree
	MapCanvas::create
	GmTree::create $mon

	wm title .mapcan($mon) [format [G_msg "Map Display %d"] $mon]
	wm withdraw .mapcan($mon)
	wm deiconify .mapcan($mon)
}


###############################################################################

proc Gm::_create_intro { } {
	variable prgtext
    global gmpath
    global GRASSVERSION
    global location_name
    global max_prgindic
    global prg

    set top [toplevel .intro -relief raised -borderwidth 2]

    wm withdraw $top
    wm overrideredirect $top 1

    set ximg  [label $top.x -image [image create photo -file "$gmpath/intro.gif"] ]

    set frame [frame $ximg.f -background white]
    set lab1  [label $frame.lab1 \
		-text [format [G_msg "GRASS%s GIS Manager - %s"] $GRASSVERSION $location_name] \
		-background white -foreground black -font introfont]
    set lab2  [label $frame.lab2 -textvariable Gm::prgtext -background white]
    set prg   [ProgressBar $frame.prg -width 50 -height 15 -background white \
		   -variable Gm::prgindic -maximum $max_prgindic]
    pack $lab1 $prg -side left -fill both -expand yes
    pack $lab2 -side right -expand yes
    place $frame -x 0 -y 0 -anchor nw
    pack $ximg
    BWidget::place $top 0 0 center
    wm deiconify $top
}

###############################################################################

# nviz
proc Gm::nviz { } {
global osxaqua
global HOSTTYPE

    set cmd "nviz"
	if { $HOSTTYPE == "macintosh" || $HOSTTYPE == "powermac" || $HOSTTYPE == "powerpc" || $HOSTTYPE == "intel-pc"} {
		if { $osxaqua == "1"} {
			spawn $cmd
		} else {
			term $cmd
		}
	} else {
		spawn $cmd
	}

}

###############################################################################

# d.nviz: set up NVIZ flight path (not much use without a backdrop?)
proc Gm::fly { } {

    guarantee_xmon
    exec d.nviz -i --ui &

}

###############################################################################

# xganim
proc Gm::xganim { } {

    exec xganim --ui &

}

###############################################################################

# help
proc Gm::help { } {

    set cmd [list g.manual --ui]
    term $cmd

}

###############################################################################
# sets default display font
proc Gm::defaultfont { source } {
	global env iconpath
	variable dfont
	variable selectedfont
	variable encoding

	# create a dialog with selector for stroke font and true type font, and
	# text entry for character encoding

    toplevel .dispfont
    wm title .dispfont [G_msg "Select GRASS display font"]
    
    if {[catch {set fontlist [exec d.font --q -l]} error]} {
	    Gm::errmsg $error "d.font error"
    }
    set fontlist [string trim $fontlist]
    set fontlist [split $fontlist "\n"]
    set fontlist [lsort -unique $fontlist]
   
    set row [ frame .dispfont.fontlb ]
    Label $row.a -text [G_msg "Font: "] 
    set fontlb [ listbox $row.b -bg white -width 30 \
    	-yscrollcommand "$row.vscrollbar set"]

    scrollbar $row.vscrollbar -width 12 \
    	-command "$row.b yview" \
    	-relief {sunken}

    foreach item $fontlist {
    	$fontlb insert end $item
    }
    # If $Gm::dfont is empty then it hasn't been set by a layer module
    # before calling this procedure, so we should read the current
    # default font from the GRASS_FONT environment variable
    if {$Gm::dfont == ""} {    
        if {![catch {set env(GRASS_FONT)}] && $env(GRASS_FONT) != ""} {
    	    set Gm::dfont $env(GRASS_FONT)
	    }
    }
    set selectedfont $Gm::dfont
     
    if {$Gm::dfont != ""} {
    	set fontindex [lsearch $fontlist $Gm::dfont]
    	if {$fontindex > -1} {
    		$fontlb selection set $fontindex
    		$fontlb see $fontindex
    	}
    }

    pack $row.vscrollbar -side right -fill y
    pack $row.b -side right
    pack $row.a -side right -anchor n
    pack $row -side top -fill both -expand yes -pady 3 -padx 5
    
 	set row [ frame .dispfont.fontopt3 ]
    Label $row.a -text [G_msg "Character encoding: "] 
    Entry $row.b -width 20 -text "$Gm::encoding" \
	    -textvariable Gm::encoding
    pack $row.b $row.a -side right -anchor e
    pack $row -side top -fill both -expand yes -pady 3 -padx 5
    
	set row [ frame .dispfont.buttons ]
	Button $row.ok -text [G_msg "OK"] -width 8 -bd 1 \
			-command "Gm::setfont $source; destroy .dispfont"
    pack $row.ok -side left -fill x -expand 0
    button $row.cancel -text [G_msg "Cancel"] -width 8 -bd 1 \
    	-command "destroy .dispfont"
    pack $row.cancel -side right -fill x -expand 0
    pack $row -side bottom -pady 3  -padx 5 -expand 0 -fill x 
    
    bind $fontlb <<ListboxSelect>> {
    	set Gm::selectedfont [%W get [%W curselection]]
    }   
    
};


proc Gm::setfont { source } {
	global env
	variable dfont
	variable selectedfont
	variable encoding

	# Set GRASS environmental variables for default display font and
	# character encoding
	
	if { $encoding != "" && $encoding != "ISO-8859-1"} {
		set env(GRASS_ENCODING) $encoding
	}

	set dfont $selectedfont
    
	if { $source == "menu" && $dfont != "" } {
		set env(GRASS_FONT) $dfont
		set dfont ""
	}

};

###############################################################################

proc Gm::cleanup { } {
	global mon
	global tmpdir
	global legfile
	variable moncount
	
	set mappid $MapCanvas::mappid
	
	# delete all map display ppm files
	cd $tmpdir
	set deletefile $mappid
	append deletefile ".*"
	foreach file [glob -nocomplain $deletefile] {
		catch {file delete $file}
	}

	if {[info exists legfile] && [file exists $legfile]} {catch {file delete -force $legfile}}

	unset mon

};

###############################################################################

proc main {argc argv} {
    variable gm_mainframe
    global auto_path
    global GRASSVERSION
    global location_name
    global mapset
    global keycontrol
    global filename
    global mon

    wm withdraw .
    wm title . [format [G_msg "GRASS%s GIS Manager - %s %s"] $GRASSVERSION $location_name $mapset]

    bind . <$keycontrol-Key-o> {
		Gm::OpenFileBox
    }
    bind . <$keycontrol-Key-n> {
		GmTree::new
    }
    bind . <$keycontrol-Key-s> {
		Gm::SaveFileBox
    }
    bind . <$keycontrol-Key-q> {
		exit
	}
    bind . <$keycontrol-Key-w> {
		GmTree::FileClose {}
    }

    Gm::create
    BWidget::place . 0 0 at 400 100

    wm deiconify .
    raise .mainframe
    focus -force .
    destroy .intro
    
    if { $argc > 1 } {
    	foreach i $argv {
    		if { [regexp -- {\.grc$} $i] || [regexp -- {\.dmrc$} $i] || [regexp -- {\.dm$} $i] } { 
			set filename($mon) [lindex $argv 0]
			GmTree::load $filename($mon)
    		}
    	}
    }
}


main $argc $argv
wm geom . [wm geom .]

