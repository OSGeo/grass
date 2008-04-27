##########################################################################
# profile.tcl - Dialog to create profile of raster surface for GRASS GIS Manager
# May 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2007 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################

namespace eval GmProfile {
    variable pcan
    variable status ""
    variable pmap ""
    variable pmainframe
    variable transect ""
    variable tottransect ""
    variable pcoords ""
    variable pcoordslist ""
    variable profilelist ""
    variable x
    variable y
    variable first 1
    variable firsteast 0.0
    variable firstnorth 0.0
    variable east1 0.0
    variable north1 0.0
    variable east2 0.0
    variable north2 0.0
    variable tlength 0.0
    variable tottlength 0.0
    variable linex1 0.0
    variable liney1 0.0
    variable linex2 0.0
    variable liney2 0.0	
    variable array psegment len #
    variable elevrange 0.0
    variable elevmax 0.0
    variable elevmin 0.0
    variable elev 0.0
    variable msg
}


###############################################################################
# select raster map to profile

# select base raster map from list and put name in layer tree node
proc GmProfile::select_rast { } {
    variable pmap
    variable status
    
    set m [GSelect cell title "Raster map for profile" parent [winfo containing [winfo pointerx .] [winfo pointery .]]]
    if { $m != "" } { 
        set pmap $m
    }
    
    set GmProfile::status [G_msg "Profile for $pmap"]    

}


###############################################################################
# create canvas for displaying profile
proc GmProfile::create { mapcan } {
    global gmpath
    global iconpath
    global env
    global bgcolor
    global mon
	variable pmap
	variable pcan
	
	if { [winfo exists .profile] } {return}

	set ptitle "Profile Window"
	toplevel .profile
    wm title .profile [G_msg $ptitle]


    wm withdraw .profile
    #wm overrideredirect $txt 1

	# create canvas for profile
	set profilemf [MainFrame .profile.mf \
		-textvariable GmProfile::status]
		
	set profile_frame [$profilemf getframe]
			
	set pcan [canvas $profile_frame.can -bg white\
		-borderwidth 0 -closeenough 1.0 \
        -relief ridge -selectbackground #c4c4c4 \
        -width 600 -height 300 ]
        	   
    # setting geometry
    place $pcan \
        -in $profile_frame -x 0 -y 0 -anchor nw \
        -bordermode ignore 

	# profile control buttons
	set pcan_tb [$profilemf addtoolbar]
	pack $pcan_tb -side left -expand yes -fill x
	
	set pcanbb [ButtonBox $pcan_tb.bb -orient horizontal]
	$pcanbb add -image [image create photo -file "$iconpath/element-cell.gif"] \
		-command "GmProfile::select_rast" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Select raster map to profile.\nCurrently selected raster is default."] -highlightbackground $bgcolor
	$pcanbb add  -image [image create photo -file "$iconpath/gui-profiledefine.gif"] \
		-command "GmProfile::profilebind $mapcan" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Draw profile transect in map display"] -highlightbackground $bgcolor
	$pcanbb add -image [image create photo -file "$iconpath/gui-profiledraw.gif"] \
		-command "GmProfile::pdraw" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Draw profile"] -highlightbackground $bgcolor
	$pcanbb add -image [image create photo -file "$iconpath/gui-erase.gif"] \
		-command "GmProfile::perase $mapcan" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Clear profile"] -highlightbackground $bgcolor
	$pcanbb add -image [image create photo -file "$iconpath/file-save.gif"] \
		-command "GmProfile::psave" \
        -highlightthickness 0 -takefocus 0 -relief link -borderwidth 1  \
        -highlightbackground $bgcolor  -activebackground $bgcolor\
        -helptext [G_msg "Save profile to EPS file"] -highlightbackground $bgcolor
	set helpbtn [Button $pcan_tb.help -text [G_msg "Help"] \
		-image [image create photo -file "$iconpath/gui-help.gif"] \
		-command "spawn g.manual --q gm_profile" \
		-background $bgcolor -borderwidth 1 \
		-helptext [G_msg "Help"]]
        
    # add a menu button to set preferences?

	pack $profilemf -expand yes -fill both -padx 0 -pady 0
	pack $pcan -fill both -expand yes
	pack $helpbtn -side right -anchor e
	pack $pcanbb -side left -anchor w
			
    set GmProfile::status [G_msg "Profile for $pmap"]
    $profilemf showstatusbar status 
    pack $profilemf -fill both -expand yes

	BWidget::place .profile 0 0 at 500 100
    wm deiconify .profile
    
	# bindings for closing profile window
	bind .profile <Destroy> "GmProfile::cleanup %W $mapcan"
	    
}

###############################################################################
proc GmProfile::setelev { pmap } {
	variable elevrange
	variable elevmax
	variable elevmin
	global devnull
	
    #calculate elevation range so all profiles with same window geometry with have
    # same scale. Elevation range calcuated within currently displayed region.
        
   	#set input [exec r.describe -rdq map=$pmap]
	#regexp -nocase {^([0-9]+) thru ([0-9]+)} $input trash elevmin elevmax
	#set elevrange [expr $elevmax - $elevmin]		
	
    
	if {![catch {open "|r.univar --q -g map=$pmap 2> $devnull" r} input]} {
		while {[gets $input line] >= 0} {
			regexp -nocase {^([a-z]+)=(.*)$} $line trash key value
			set elev($key) $value	
		}
		if {[catch {close $input} error]} {
			GmLib::errmsg $error [G_msg "r.univar error"]
		}
	
		set elevmax $elev(max)
		set elevmin $elev(min)
		set elevrange $elev(range)	
	}
	
	#vwait elevrange
    #set GmProfile::status [G_msg "Profile for $pmap"]    

}
###############################################################################
# save profile to eps file
proc GmProfile::psave { } {
	variable pcan
	global env
		
	set types {
    {{EPS} {.eps}}
	}

	if { [info exists HOME] } {
		set dir $env(HOME)
		set path [tk_getSaveFile -filetypes $types -initialdir $dir \
			-defaultextension ".eps" -parent .profile]
	} else {
		set path [tk_getSaveFile -filetypes $types -defaultextension ".eps" -parent .profile]
	}
	
	$pcan postscript -file "$path"

	return
}


###############################################################################
# procedures for marking profile location 

#  bindings for drawing profile transect
proc GmProfile::profilebind { mapcan } {
	variable measurement_annotation_handle
	variable tlength 
	variable tottlength
	variable linex1 
	variable liney1
	variable linex2
	variable liney2
	variable first
	variable msg
	global mon
		
	# Make the output for the measurement
	set measurement_annotation_handle [monitor_annotation_start $mon "Measurement" {}]
    
	if {[info exists linex1]} {unset linex1}
	if {[info exists liney1]} {unset liney1}
	if {[info exists linex2]} {unset linex2}
	if {[info exists liney2]} {unset liney2}
		
	bind $mapcan <1> "GmProfile::marktransect $mapcan %x %y"
	bind $mapcan <B1-Motion> "GmProfile::drawtransect $mapcan %x %y"
	bind $mapcan <ButtonRelease-1> "GmProfile::getcoords $mapcan"
	
	set GmProfile::msg "Draw profile transect with mouse"
		
	MapCanvas::setcursor $mon "pencil"
	set tlength 0
	set tottlength 0

}

# start profile transect
proc GmProfile::marktransect {mapcan x y} {
	variable transect
	variable tlength 
	variable tottlength
	variable linex1 
	variable liney1
	variable linex2
	variable liney2
	variable first
	variable firsteast
	variable firstnorth
	variable pcoords
	global mon

	#start line
	if { ![info exists linex1] } {
	    set linex1 [$mapcan canvasx $x]
	    set liney1 [$mapcan canvasy $y]
	}

	if { $first == 1 } {
		set firsteast  [MapCanvas::scrx2mape $mon $linex1]
		set firstnorth [MapCanvas::scry2mapn $mon $liney1]
		set pcoords "$firsteast,$firstnorth"
	}

	set first 0

	#check for click with no drag
	if { ![info exists linex2] } {
	    set linex2 $linex1
	}
	if { ![info exists liney2] } {
	    set liney2 $liney1
	}

    $mapcan delete transect
}

# draw profile transect
proc GmProfile::drawtransect { mapcan x y } {
	variable measurement_annotation_handle
	variable transect
	variable tlength 
	variable tottlength
	variable linex1 
	variable liney1
	variable linex2
	variable liney2
	variable pcoords
	global mon
	    
	set scrxmov $x
	set scrymov $y
	set eastcoord [eval MapCanvas::scrx2mape $mon $x]
	set northcoord [eval MapCanvas::scry2mapn $mon $y]
	set coords "$eastcoord,$northcoord"
	set xc [$mapcan canvasx $x]
	set yc [$mapcan canvasy $y]
	
	# draw line segment
	if {($linex1 != $xc) && ($liney1 != $yc)} {
		$mapcan delete transect
		$mapcan addtag transect withtag \
			[$mapcan create line $linex1 $liney1 $xc $yc \
			-fill red -arrow last -width 2]
		set linex2 $xc
		set liney2 $yc
	}
}	

# get coordinates for transect nodes and endpoints
proc GmProfile::getcoords { mapcan } {
	variable measurement_annotation_handle
	variable transect
	variable tottransect
	variable tlength 
	variable tottlength
	variable linex1 
	variable liney1
	variable linex2
	variable liney2
	variable pcoords
	variable firsteast
	variable east1
	variable north1
	variable east2
	variable north2
	variable pcoords
	variable pcoordslist
	global mon
	
	# draw cumulative line
	$mapcan addtag tottransect withtag \
		[$mapcan create line $linex1 $liney1 $linex2 $liney2 \
		-fill blue -arrow last -width 2]

	# get line endpoints in map coordinates
	
	set east1  [expr 1.0 * [MapCanvas::scrx2mape $mon $linex1]]
	set north1 [expr 1.0 * [MapCanvas::scry2mapn $mon $liney1]]
	set east2  [expr 1.0 * [MapCanvas::scrx2mape $mon $linex2]]
	set north2 [expr 1.0 * [MapCanvas::scry2mapn $mon $liney2]]
	
	# coordinates for use in r.profile
	append pcoords "," $east2 "," $north2
	
	# x distance off each transect segment
	
	# First convert map coordinates to profile screen coordinates
	# Then add the profile screen coordinates to the profile coordinates list. 
	# Only add east1 and north1 the first time; 
	# continue to add east2 and north2 until ready to draw profile

	# calculate line segment length and total length
	set tlength [expr {sqrt(pow(($east1 - $east2), 2) + pow(($north1 - $north2), 2))}]
	set tottlength [expr {$tottlength + $tlength}]
	
	lappend pcoordslist $tottlength
	
	monitor_annotate $measurement_annotation_handle " --segment length\t= $tlength\n"
	monitor_annotate $measurement_annotation_handle "cumulative length\t= $tottlength\n"
	
	set linex1 $linex2
	set liney1 $liney2
}

###############################################################################
# draw profile
proc GmProfile::pdraw { } {
    variable pcan
    variable pmap
    variable tottlength
    variable elevrange
    variable elevmax
    variable elevmin
    variable firsteast
    variable firstnorth
    variable east1
    variable north1
    variable east2
    variable north2
    variable pcoords
    variable pcoordslist
    variable profilelist
    variable status
    global mon
    global devnull

    set cumdist 0.0

    if {![catch {open "|g.proj -p" r} input]} {
	set key ""
	set value ""
	while {[gets $input line] >= 0} {
	    if { [string equal "XY location (unprojected)" "$line"] } {
		set mapunits "map units"
		break
	    }
	    regexp -nocase {^(.*):(.*)$} $line trash key value
	    set key [string trim $key]
	    set value [string trim $value]
	    set prj($key) $value    
	}
	if {[catch {close $input} error]} {
	    GmLib::errmsg $error [G_msg "g.proj or projection error"]
	} 
    }

    # r.profile always returns meters, but here profile length is calculated
    #  from the map canvas arrows and so is measured & plotted in map units.
    # May already be set above if locn was XY.
    if { ! [ info exist mapunits ] } {
	set mapunits $prj(units)
    }

	if {$pmap == ""} {
	   # get currently selected raster map as default to profile if nothing else chosen
		set tree($mon) $GmTree::tree($mon)
		set sel [ lindex [$tree($mon) selection get] 0 ]	
		if { $sel != "" } {
			set type [GmTree::node_type $sel]
			if {$type == "raster" } {
				set GmProfile::pmap [GmRaster::mapname $sel]
			}
		} else {
			tk_messageBox -message [G_msg "You must select a raster to profile"] -type ok -icon warning \
			-title [G_msg "No raster map selected"] -parent .profile
			return
		}
	}    

	if {$pcoords == ""} {
		tk_messageBox -message [G_msg "You must draw a transect to profile"] -type ok -icon warning \
		-title [G_msg "No transect drawn"] -parent .profile
		return
	}

	
	set GmProfile::status [G_msg "Please wait while profile elevations are calculated"] 
	update
	# calculate elevation max, min, and range
	GmProfile::setelev $pmap
	if {$elevrange == 0} {
		tk_messageBox -message [G_msg "Elevation range for selected raster is zero.\nNo profile can be created."]\
		-type ok -icon warning -title [G_msg "Zero elevation range"] -parent .profile
		return
	}
	
	# put map name back in status bar
	set GmProfile::status [G_msg "Profile for $pmap"]    

	$pcan delete all
	set profilelist ""
	
	# calculate screen length and width
	set w [winfo width $pcan]
	set h [winfo height $pcan]
	
	#calculate graph box and scale extents
	set top [expr 0.20 * $h]
	set bottom [expr 0.80 * $h]
	set height [expr $bottom - $top]
	set left [expr 0.2 * $w]
	set right [expr 0.9 * $w]
	set center [expr 0.55 * $w]
	set width [expr $right - $left]
	set yscaleright [expr $left - 10]
	set xscaletop [expr $bottom	+ 10]
	set titlebottom [expr 0.10 * $h]
	set titleleft [expr $left + 15]
	
	# create title
	$pcan create text $center $titlebottom \
		-text [G_msg "Profile for $pmap"] \
		-font {-size 14 -weight bold} \
		-anchor s \
		-justify center
	
	# create y axis (from 20%x20% to 20%x80% - top to bottom)
	$pcan create line $left $top $left [expr $bottom + 5]

	# format axis labels in a nice way, as a function of range
	if { $elevrange >= 100 } {
	    set outfmt "%.0f"
	} elseif { $elevrange > 0 } {
	    set outfmt "%.[expr {int(ceil(2 - log10($elevrange)))}]f"
	} else {
	    # error: no range (nan?)
	    set outfmt "%g"
	}

	# add scale to y axis
	$pcan create text $yscaleright $top \
		-text "[format $outfmt $elevmax]" \
		-anchor e \
		-justify right

	$pcan create text $yscaleright $bottom \
		-text "[format $outfmt $elevmin]" \
		-anchor e \
		-justify right

	# create x axis (from 20%x80% to 90%x80% - left to right)
	$pcan create line [expr $left - 5] $bottom $right $bottom

	# add scale to x axis
	$pcan create text $left $xscaletop \
		-text "0" \
		-anchor n \
		-justify center

	# add tick marks
	$pcan create line $right $bottom $right [expr $bottom + 5]
	$pcan create line [expr $left - 5] $top $left $top

	# run r.profile first time to calculate total transect distance (needed for lat lon regions)
   	if {![catch {open "|r.profile input=$pmap profile=$pcoords null=nan 2> $devnull" r} input]} {
		while {[gets $input line] >= 0} {
			if { [regexp -nocase {^([0-9].*) ([[.-.]0-9na].*)$} $line trash dist elev] } {
				set cumdist $dist
			}
		}
		if {[catch {close $input} error]} {
			GmLib::errmsg $error [G_msg "r.profile error"]
		}
	}

	set divisor "1.0"
	if { [string equal "meters" "$mapunits"] } {
	    if { $tottlength > 2500 } {
		set mapunits "km"
		set divisor "1000.0"
	    }
	} elseif { [string first "feet" "$mapunits"] >= 0 } {
	    # nano-bug: we match any "feet", but US Survey feet is really 
	    #  5279.9894 per statute mile, or 1.06' per 100 miles. As >100
	    #  miles the tick markers are rounded to the nearest 10th of a
	    #  mile (528'), the difference in foot flavours is ignored.
	    if { $tottlength > 5280 } {
		set mapunits "miles"
		set divisor "5280.0"
	    }
	} elseif { [string first "degree" "$mapunits"] >= 0 } {
	    if { $tottlength < 1 } {
		set mapunits "minutes"
		set divisor [expr 1/60.0]
	    }
	}

	# format axis labels in a nice way, as a function of range
	if { [expr $tottlength/$divisor ] >= 500 } {
	   set outfmt "%.0f"
	} elseif { [expr $tottlength/$divisor ] >= 100 } {
	    set outfmt "%.1f"
	} elseif { [expr $tottlength/$divisor ] > 0 } {
	    set outfmt "%.[expr {int(ceil(2 - log10($tottlength/$divisor)))}]f"
	} else {
	    # error: no range (nan?)
	    set outfmt "%g"
	}

	# add axis label
	$pcan create text $center $xscaletop \
		-text [G_msg "distance along transect ($mapunits)"] \
		-anchor n \
		-justify center

	# add transect segment markers
	foreach {x} $pcoordslist {
		if { $tottlength > 0.0 } {
			set segx [expr $left + (($x * $width) / $tottlength)]
			$pcan create line $segx $bottom $segx $top -fill grey
			$pcan create text $segx $top -text "[format $outfmt [expr $x/$divisor]]" \
				-anchor s -justify center -fill grey
		}
	}

	# add label for total transect distance
	$pcan create text $right $xscaletop \
		-text "[format $outfmt [expr $tottlength/$divisor]]" \
		-anchor n \
		-justify center

	# run r.profile again to
	# convert dist elev (stdout) to xy coordinates of profile line
   	if {![catch {open "|r.profile input=$pmap profile=$pcoords null=nan 2> $devnull" r} input]} {
		while {[gets $input line] >= 0} {
			if { [regexp -nocase {^([0-9].*) ([[.-.]0-9na].*)$} $line trash dist elev] } {
			    if { [string equal "$elev" "nan" ]  } {
				# draw profile line
				if { [ info exist profilelist ] } {
				    if { [llength $profilelist] >= 4 } {
					$pcan create line $profilelist -fill blue
				    } elseif { [llength $profilelist] == 2 } {
					# plot a dot
					$pcan create oval [concat $profilelist $profilelist] \
					    -fill blue -outline blue -width 1
				    }
				    unset profilelist
				}
			    } else {
				set pelev [expr $bottom - ($height * ($elev - $elevmin) / $elevrange)] 
				set pdist [expr $left + (($dist * $width) / $cumdist)]
				lappend profilelist $pdist $pelev
			    }
			}
		}
		if {[catch {close $input} error]} {
			GmLib::errmsg $error [G_msg "r.profile error"]
		}
	}

	# draw profile line
	if { [ info exist profilelist ] && [llength $profilelist] >= 4 } {
		$pcan create line $profilelist -fill blue
	}

}
###############################################################################
# erase profile and clear transects
proc GmProfile::perase { mapcan } {
	variable pmap
	variable pcan
	variable transect
	variable tottransect
	variable tlength
	variable tottlength
	variable pcoords
	variable pcoordslist
	variable profilelist
	variable first
	variable linex1
	variable liney1
	variable elevrange
	variable elevmax
	variable elevmin
	
	$pcan delete all
	$mapcan delete transect
	$mapcan delete tottransect
	set tlength 0.0
	set tottlength 0.0
	set pcoords ""
	set pcoordslist ""
	set profilelist ""
	set first 1
	if { [info exists linex1] } {
		unset linex1
	}
	if { [info exists liney1] } {
		unset liney1
	}
	

}

###############################################################################

proc GmProfile::cleanup { destroywin mapcan } {
	# cleanup procedures on closing profile window	
	variable pcan
	variable transect
	variable tottransect
	variable tlength
	variable tottlength
	variable pcoords
	variable pcoordslist
	variable profilelist
	variable first
	variable linex1
	variable liney1
	global mon
	
	set tlength 0.0
	set tottlength 0.0
	set pcoords ""
	set pcoordslist ""
	set profilelist ""
	set first 1
	if { [info exists linex1] } {
		unset linex1
	}
	if { [info exists liney1] } {
		unset liney1
	}
	
	$mapcan delete transect
	$mapcan delete tottransect
	MapCanvas::restorecursor $mon

}


###############################################################################
