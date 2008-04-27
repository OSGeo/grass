global filemap_browser

proc set_filemap_browser_filename {w name} {
    set file_name ${name}@[get_filemap_browser_mapset $w]
    
    $w.filename delete 0 end
    if {[string length $name] > 0} {
	$w.filename insert 0 ${name}@[get_filemap_browser_mapset $w]
	set_selection_from_filemap_browser_filename $w
    }
}

proc filemap_browser_list_mapset {w} {    
    set mapset [get_filemap_browser_mapset $w]
    set element [get_filemap_browser_element $w]
    
    $w.main.files.f.list delete 0 end
    update

    if {[string length $mapset] == 0} {return}
    if {[string length $element] == 0} {return}

    foreach name [grass_ls $element $mapset] {
	$w.main.files.f.list insert end $name
    }
}

proc get_filemap_browser_element {w} {
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
    } 
    return $element 
}

proc set_filemap_browser_element {w name} {
    $w.element.entry delete 0 end
    $w.element.entry insert 0 $name
    filemap_browser_list_mapset $w
}

proc get_filemap_browser_mapset {w} {
    global filemap_browser
    
    if {[info exists filemap_browser($w,mapset)]} {
	return $filemap_browser($w,mapset)
    }
    return {}
}

proc set_filemap_browser_mapset {w name} {
    global filemap_browser
    
    set filemap_browser($w,mapset) $name
    filemap_browser_list_mapset $w
}


proc set_selection_from_filemap_browser_filename { w } {
    $w.filename selection from 0 
    $w.filename selection to end 
    $w.list.l.list insert end [$w.filename get]
}

proc create_filemap_browser {{w .filemap_browser} {type all} {mode 0} {curfiles {}}} {
    global filemap_browser 

    toplevel $w 
    tkwait visibility $w

    puts "filemapBROWSER: $w TYPE: $type MODE: $mode"
    entry $w.filename -bd 2 -relief sunken
    bind $w.filename <Return> "set_selection_from_filemap_browser_filename $w"
    frame $w.main
    frame $w.list

    frame $w.list.l
    listbox $w.list.l.list -bd 2 -relief sunken \
	-exportselection no \
	-selectbackground LightYellow1 \
	-yscroll "$w.list.l.scroll set" \
	-selectmode single
    scrollbar $w.list.l.scroll \
	-command "$w.list.l.list yview"

    button $w.list.remove -text "Remove" -bd 1 \
		-command "multimap_remove_cmd $w"
    pack $w.list.l.list $w.list.l.scroll -side left -fill y -expand yes
    pack $w.list.l $w.list.remove -side top -fill x -expand yes
    
    frame     $w.main.mapsets
    label     $w.main.mapsets.label -text MAPSETS
    frame     $w.main.mapsets.f
    listbox   $w.main.mapsets.f.list -bd 2 -relief sunken \
	-exportselection no                     \
	-selectbackground LightYellow1           \
	-yscroll "$w.main.mapsets.f.scroll set" \
	-selectmode single
    scrollbar $w.main.mapsets.f.scroll \
	-command "$w.main.mapsets.f.list yview"

    bind $w.main.mapsets.f.list <ButtonRelease-1> \
	"filemap_browser_select_mapset  %W %y $w"
    
    frame     $w.main.files
    label     $w.main.files.label -text FILES
    frame     $w.main.files.f
    listbox   $w.main.files.f.list -bd 2 -relief sunken \
	-exportselection no                   \
	-selectbackground LightYellow1         \
	-yscroll "$w.main.files.f.scroll set" \
	-selectmode single
    scrollbar $w.main.files.f.scroll \
	-command "$w.main.files.f.list yview"

    bind $w.main.files.f.list <ButtonRelease-1> \
	"filemap_browser_select_file %W %y $w"
    
    frame $w.element
    entry $w.element.entry -bd 2 -relief sunken
    bind $w.element.entry <Return> "filemap_browser_list_mapset $w"
    
    if { ![string compare rast $type]} { 
	set name Raster
    } elseif { ![string compare vect $type]} {
	set name Vector
    } elseif { ![string compare site $type]} {
	set name Site
    } elseif { ![string compare surf $type]} {
	set name Surface
    } elseif { ![string compare 3dview $type]} {
	set name 3dview
    }
    
    if [string compare $type all] {
	label $w.element.menu -text "Map Type:" -relief raised
    } else {
	set name ""
	menubutton $w.element.menu -text {Map Type} -menu $w.element.menu.m -relief raised -bd 1
	menu $w.element.menu.m
	$w.element.menu.m add command \
	    -label {Raster} -command "set_filemap_browser_element  $w Raster"
	$w.element.menu.m add command \
	    -label {Vector} -command "set_filemap_browser_element  $w Vector"
	$w.element.menu.m add command \
	    -label {Site} -command "set_filemap_browser_element  $w Site"
	$w.element.menu.m add command \
	    -label {Surf} -command "set_filemap_browser_element $w Surf"
	$w.element.menu.m add command \
	    -label {Regions} -command "set_filemap_browser_element  $w windows"
	$w.element.menu.m add command \
	    -label {Labels} -command "set_filemap_browser_element  $w\
		    paint/labels"
	    $w.element.menu.m add command \
	    -label {Icons} -command "set_filemap_browser_element  $w icons"
    }

    button $w.blank -text Blank -command "multimap_blank_cmd $w" -bd 1
    button $w.previous -text Previous -command "multimap_previous_cmd $w" -bd 1
    button $w.done -text Done -command "multimap_done_cmd $w" -bd 1
    button $w.cancel -text Cancel -command "multimap_cancel_cmd $w" -bd 1
    
    pack $w.filename -side top -expand yes -fill x
    pack $w.main -side top
    pack $w.list -side right -in $w.main

    pack $w.main.mapsets -side left
    pack $w.main.mapsets.label -side top
    pack $w.main.mapsets.f -side top
    pack $w.main.mapsets.f.list -side left
    pack $w.main.mapsets.f.scroll -side left -expand yes -fill y
    
    pack $w.main.files -side left
    pack $w.main.files.label -side top
    pack $w.main.files.f -side top
    pack $w.main.files.f.list -side left
    pack $w.main.files.f.scroll -side left -expand yes -fill y
    
    pack $w.element  -side top -expand yes -fill x
    pack $w.element.menu  -side left
    pack $w.element.entry  -side left -expand yes -fill x
    pack $w.blank $w.previous $w.done $w.cancel -side left -expand 1
    
    foreach mapset [grass_mapset_list] {
	$w.main.mapsets.f.list insert end $mapset
    }
    set_filemap_browser_element $w $name
    set_filemap_browser_mapset $w {}
    wm title $w "Map Browser"
    wm protocol $w WM_DELETE_WINDOW "destroy $w"

    
    if {$mode} {grab $w}

    # Add names from curfiles list.  We use this to allow the mapBrowser
    # to be used to edit an old list of files
    foreach i $curfiles {
	$w.list.l.list insert end "$i"
    }

    tkwait window $w

    return $filemap_browser($w,Answer)
    
}

# Adds a blank entry to the file map
proc multimap_blank_cmd {w} {
    $w.list.l.list insert end "-- blank --"
}

# Just copies the previous entry, whatever it may be
# In terms of implementation, this corresponds to no
# change in the attribute
proc multimap_previous_cmd {w} {
    set last [$w.list.l.list get end]
    
    # May not need to add "NO CHANGE" text
    if [regexp "\(NO CHANGE\)" "$last"] then {
	$w.list.l.list insert end "$last"
    } else {
	$w.list.l.list insert end "(NO CHANGE) $last"
    }
}

proc multimap_remove_cmd {w} {
    # Get the selection to delete
    set selection [lindex [$w.list.l.list curselection] 0]
    if {$selection != ""} then {
	$w.list.l.list delete $selection
    }
}

proc multimap_cancel_cmd {w} {
    global filemap_browser
    set filemap_browser($w,Answer) -1
    destroy $w
}

proc multimap_done_cmd {w} {
    global filemap_browser 
    
    set filemap_browser($w,Answer) [list]
    set size [$w.list.l.list size]
    for {set i 0} {$i < $size} {incr i} {
	lappend filemap_browser($w,Answer) [$w.list.l.list get $i]
    }

    destroy $w
}
proc filemap_browser_select_file {W y w} {
    set near [ $W nearest $y ]
    $W selection set $near $near
    eval set_filemap_browser_filename $w {[$W get $near]}
}
proc filemap_browser_select_mapset {W y w} {
    set near [ $W nearest $y ]
    $W selection set $near $near
    eval set_filemap_browser_mapset $w {[$W get $near]}
}
