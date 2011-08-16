global map_browser

global src_boot
#set src_boot $env(GISBASE)

proc grass_ls {element {mapset .}} {
    set dir [grass_element_name $element $mapset]
    if {[file isdir $dir]} {
		return [lsort [glob -directory $dir *]]
    }
    return {}
}

proc grass_element_name {element {mapset .}} {
    return [grass_file_name $element {} $mapset]
}

proc grass_file_name {element name {mapset .}} {
    global src_boot
    if {[string compare $mapset .] == 0} {
	    set mapset [exec $src_boot/bin/g.gisenv MAPSET]
    }
    return [exec $src_boot/bin/g.gisenv GISDBASE]/[exec $src_boot/bin/g.gisenv LOCATION_NAME]/$mapset/$element/$name
}

proc grass_location {} {
    return [grass_element_name {} /]
}

proc g.gisenv {name} {
    global src_boot
    return [exec $src_boot/bin/g.gisenv $name]
}

proc grass_mapset_list {} {
    global src_boot
    global devnull

    set list {}
    set location [grass_location]
    
    foreach name [exec $src_boot/bin/g.mapsets -p 2> $devnull] {
	if {[file isdir $location/$name]} {
	    lappend list $name
	}
    }
    return $list
}

proc set_map_browser_filename {w name} {
    global ScriptState

    set file_name ${name}@[get_map_browser_mapset $w]

    $w.filename delete 0 end
    if {[string length $name] > 0} {
	$w.filename insert 0 ${name}@[get_map_browser_mapset $w]
	set_selection_from_map_browser_filename $w

	if $ScriptState then {
	    Nv_script_add_string "catch \{send \$ProcessName \{$w.filename delete 0 end\}\}"
	    Nv_script_add_string "if \$Nv_mapLoopMode then \{"
	    Nv_script_add_string "catch \{send \$ProcessName \"$w.filename insert 0 \$Nv_mapLoopFile\"\}"
	    Nv_script_add_string "\} else \{"
	    Nv_script_add_string "catch \{send \$ProcessName \{$w.filename insert 0 \"$file_name\"\}\}"
	    Nv_script_add_string "\}"
	    Nv_script_add_string "catch \{send \$ProcessName \{set_selection_from_map_browser_filename $w\}\}"
	}
    }
}

proc map_browser_list_mapset {w} {

    set mapset [get_map_browser_mapset $w]
    set element [get_map_browser_element $w]

    $w.main.files.f.list delete 0 end
    update
    if {[string length $mapset] == 0} {return}
    if {[string length $element] == 0} {return}
    foreach name [grass_ls $element $mapset] {
	$w.main.files.f.list insert end [file tail $name]
    }
}

proc get_map_browser_element {w} {
    if {[catch {$w.element.entry get} element]} { return {} }

    if {![string compare $element Raster]} {
	set element cell
    } elseif {![string compare $element Vector]} {
	set element vector
    } elseif {![string compare $element Surface]} {
	set element cell
    } elseif {![string compare $element Site]} {
	set element site_lists
    } elseif {![string compare $element Region]} {
	set element windows
    } elseif {![string compare $element Labels]} {
	set element paint/labels
    } elseif {![string compare $element Icons]} {
	set element icons
    } elseif {![string compare $element RASTER3D]} {
	set element grid3
    }
    return $element
}

proc set_map_browser_element {w name} {
    $w.element.entry delete 0 end
    $w.element.entry insert 0 $name
    map_browser_list_mapset $w
}

proc get_map_browser_mapset {w} {
    global map_browser

    if {[info exists map_browser($w,mapset)]} {
	return $map_browser($w,mapset)
    }
    return {}
}

proc set_map_browser_mapset {w name} {
    global map_browser

    set map_browser($w,mapset) $name
    map_browser_list_mapset $w
}


proc set_selection_from_map_browser_filename { w } {
    $w.filename selection from 0
    $w.filename selection to end
}

proc create_map_browser {{w .map_browser} {type all} {mode 0}} {
    global map_browser
    global nviztxtfont

    toplevel $w
    wm title $w "Map Browser"
    tkwait visibility $w

    # Answer must be set to not use uninitialized variable by accident
    set map_browser($w,Answer) -1
    #puts "BROWSER: $w TYPE: $type MODE: $mode"

    entry $w.filename -bd 2 -relief sunken
    bind $w.filename <Return> "set_selection_from_map_browser_filename $w"
    frame $w.main
    frame     $w.main.mapsets
    label     $w.main.mapsets.label -text "MAPSETS"
    frame     $w.main.mapsets.f
    listbox   $w.main.mapsets.f.list -bd 2 -relief sunken -bg white \
		-exportselection no \
		-selectbackground LightYellow1 \
		-yscroll "$w.main.mapsets.f.scroll set" \
        -xscroll "$w.main.mapsets.f.scrollx set" \
		-selectmode single
    scrollbar $w.main.mapsets.f.scroll \
		-command "$w.main.mapsets.f.list yview"
    scrollbar $w.main.mapsets.f.scrollx \
        -command "$w.main.mapsets.f.list xview" \
        -orient horizontal

    bind $w.main.mapsets.f.list <ButtonRelease-1> \
		"map_browser_select_mapset  %W %y $w"

    frame     $w.main.files
    label     $w.main.files.label -text FILES
    frame     $w.main.files.f
    listbox   $w.main.files.f.list -bd 2 -relief sunken -bg white \
		-exportselection no                   \
		-selectbackground LightYellow1         \
		-yscroll "$w.main.files.f.scroll set" \
        -xscroll "$w.main.files.f.scrollx set" \
		-selectmode single
    scrollbar $w.main.files.f.scroll \
		-command "$w.main.files.f.list yview"
    scrollbar $w.main.files.f.scrollx \
        -command "$w.main.files.f.list xview" \
        -orient horizontal

    bind $w.main.files.f.list <ButtonRelease-1> \
		"map_browser_select_file %W %y $w"

    frame $w.element
    entry $w.element.entry -bd 2 -relief sunken
    bind $w.element.entry <Return> "map_browser_list_mapset $w"

    if { ![string compare rast $type]} {
		set name Raster
    } elseif { ![string compare vect $type]} {
		set name Vector
    } elseif { ![string compare site $type]} {
		set name Site
    } elseif { ![string compare surf $type]} {
		set name Surface
    } elseif { ![string compare 3d.view $type]} {
		set name 3d.view
    } elseif {![string compare vol $type]} {
		set name RASTER3D
    }

    if [string compare $type all] {
		Label $w.element.menu -text "Map type:" -fg black -font $nviztxtfont
    } else {
		set name ""
		menubutton $w.element.menu -text {Map Type} -menu $w.element.menu.m -relief raised -bd 1
		menu $w.element.menu.m
		$w.element.menu.m add command \
			-label {Raster} -command "set_map_browser_element  $w Raster"
		$w.element.menu.m add command \
			-label {Vector} -command "set_map_browser_element  $w Vector"
		$w.element.menu.m add command \
			-label {Site} -command "set_map_browser_element  $w Site"
		$w.element.menu.m add command \
			-label {Surf} -command "set_map_browser_element $w Surf"
		$w.element.menu.m add command \
			-label {3d.view} -command "set_map_browser_element $w 3d.view"
		$w.element.menu.m add command \
			-label {Regions} -command "set_map_browser_element  $w windows"
		$w.element.menu.m add command \
			-label {Labels} -command "set_map_browser_element  $w\
				paint/labels"
	    $w.element.menu.m add command \
	    	-label {Icons} -command "set_map_browser_element  $w icons"
    }
    button $w.accept -text "Accept" -command "mapBrowser_accept_cmd $w" -bd 1 \
    	-default active
    button $w.cancel -text "Cancel" -command "mapBrowser_cancel_cmd $w" -bd 1
	
	bind $w <Return> "mapBrowser_accept_cmd $w"

    pack $w.filename -side top -expand yes -fill x -padx 4 -pady 3
    pack $w.main     -side top -expand yes -fill both -padx 4 -pady 4

    pack $w.main.mapsets -side left -expand yes -fill both
    pack $w.main.mapsets.label -side top
    pack $w.main.mapsets.f.scrollx -side bottom -expand no -fill x
    pack $w.main.mapsets.f -side top -expand yes -fill both
    pack $w.main.mapsets.f.list -side left -expand yes -fill both
    pack $w.main.mapsets.f.scroll -side left -expand no -fill y

    pack $w.main.files -side left -expand yes -fill both
    pack $w.main.files.label -side top
    pack $w.main.files.f.scrollx -side bottom -expand no -fill x
    pack $w.main.files.f -side top -expand yes -fill both
    pack $w.main.files.f.list -side left -expand yes -fill both
    pack $w.main.files.f.scroll -side left -expand no -fill y

    pack $w.element  -side top -expand yes -fill x -padx 4 -pady 3
    pack $w.element.menu  -side left
    pack $w.element.entry  -side left -expand yes -fill x
    pack $w.accept $w.cancel -side left -expand 1 -pady 4

    foreach mapset [grass_mapset_list] {
	$w.main.mapsets.f.list insert end $mapset
    }
    set_map_browser_element $w $name
    set_map_browser_mapset $w {}
    wm protocol $w WM_DELETE_WINDOW "destroy $w"

    if {$mode} {grab $w}

    tkwait window $w

    return $map_browser($w,Answer)

}

proc mapBrowser_accept_cmd  {w} {
    global map_browser

    # Make sure a file has been selected first
    set temp [$w.filename get]
    if {$temp != ""} {
    	set map_browser($w,Answer) [$w.filename get]
    	destroy $w
    } else {
	return
    }

}
proc mapBrowser_cancel_cmd {w} {
    global map_browser
    set map_browser($w,Answer) -1
    destroy $w
}

proc map_browser_select_file {W y w} {
    set near [ $W nearest $y ]
    $W selection set $near $near
    eval set_map_browser_filename $w {[$W get $near]}
}

proc map_browser_select_mapset {W y w} {
    set near [ $W nearest $y ]
    $W selection set $near $near
    eval set_map_browser_mapset $w {[$W get $near]}
}
