# ------------------------------------------------------------------------------
#  xpm2image.tcl
#  Slightly modified xpm-to-image command
#  $Id$
# ------------------------------------------------------------------------------
#
#  Copyright 1996 by Roger E. Critchlow Jr., San Francisco, California
#  All rights reserved, fair use permitted, caveat emptor.
#  rec@elf.org
# 
# ------------------------------------------------------------------------------

proc xpm-to-image { file } {
    set f [open $file]
    set string [read $f]
    close $f

    #
    # parse the strings in the xpm data
    #
    set xpm {}
    foreach line [split $string "\n"] {
        if {[regexp {^"([^\"]*)"} $line all meat]} {
            if {[string first XPMEXT $meat] == 0} {
                break
            }
            lappend xpm $meat
        }
    }
    #
    # extract the sizes in the xpm data
    #
    set sizes  [lindex $xpm 0]
    set nsizes [llength $sizes]
    if { $nsizes == 4 || $nsizes == 6 || $nsizes == 7 } {
        set data(width)   [lindex $sizes 0]
        set data(height)  [lindex $sizes 1]
        set data(ncolors) [lindex $sizes 2]
        set data(chars_per_pixel) [lindex $sizes 3]
        set data(x_hotspot) 0
        set data(y_hotspot) 0
        if {[llength $sizes] >= 6} {
            set data(x_hotspot) [lindex $sizes 4]
            set data(y_hotspot) [lindex $sizes 5]
        }
    } else {
	    error "size line {$sizes} in $file did not compute"
    }

    #
    # extract the color definitions in the xpm data
    #
    foreach line [lrange $xpm 1 $data(ncolors)] {
        set colors [split $line \t]
        set cname  [lindex $colors 0]
        lappend data(cnames) $cname
        if { [string length $cname] != $data(chars_per_pixel) } {
            error "color definition {$line} in file $file has a bad size color name"
        }
        foreach record [lrange $colors 1 end] {
            set key [lindex $record 0]
            set color [string tolower [join [lrange $record 1 end] { }]]
            set data(color-$key-$cname) $color
            if { ![string compare $color "none"] } {
                set data(transparent) $cname
            }
        }
        foreach key {c g g4 m} {
            if {[info exists data(color-$key-$cname)]} {
                set color $data(color-$key-$cname)
                set data(color-$cname) $color
                set data(cname-$color) $cname
                lappend data(colors) $color
                break
            }
        }
        if { ![info exists data(color-$cname)] } {
            error "color definition {$line} in $file failed to define a color"
        }
    }

    #
    # extract the image data in the xpm data
    #
    set image [image create photo -width $data(width) -height $data(height)]
    set y 0
    foreach line [lrange $xpm [expr 1+$data(ncolors)] [expr 1+$data(ncolors)+$data(height)]] {
        set x 0
        set pixels {}
        while { [string length $line] > 0 } {
            set pixel [string range $line 0 [expr {$data(chars_per_pixel)-1}]]
            set c $data(color-$pixel)
            if { ![string compare $c none] } {
                if { [string length $pixels] } {
                    $image put [list $pixels] -to [expr {$x-[llength $pixels]}] $y
                    set pixels {}
                }
            } else {
                lappend pixels $c
            }
            set line [string range $line $data(chars_per_pixel) end]
            incr x
        }
        if { [llength $pixels] } {
            $image put [list $pixels] -to [expr {$x-[llength $pixels]}] $y
        }
        incr y
    }

    #
    # return the image
    #
    return $image
}

