
if {[array get env GISBASE] == ""} {
    puts stderr "You must be in GRASS GIS to run this program."
    exit 1
}

if {$tcl_platform(platform) == "windows"} {
	set stderr NUL:
} else {
	set stderr @stderr
}

set outmap $env(GIS_OPT_OUTPUT)
set inmap $env(GIS_OPT_INPUT)
set aspect $env(GIS_OPT_ASPECT)
set width $env(GIS_OPT_WIDTH)
set height $env(GIS_OPT_HEIGHT)
set size $env(GIS_OPT_SIZE)
set rows $env(GIS_OPT_ROWS)
set cols $env(GIS_OPT_COLS)

set status(row) ""
set status(col) ""
set status(x) ""
set status(y) ""
set status(value) ""
set status(aspect) ""

set brush "*"

set origin(x) 0
set origin(y) 0

set finalized false

proc initialize {} {
	global tempbase tempfile tempreg tempmap env stderr
	global inmap outmap

	set tempbase [exec g.tempfile pid=[pid]]
	file delete $tempbase

	set tempfile $tempbase.ppm
	set tempreg tmp.d.rast.edit
	set tempmap tmp.d.rast.edit

	exec g.region --q --o save=$tempreg 2>$stderr

	set env(WIND_OVERRIDE) $tempreg

	exec g.copy --q --o rast=$inmap,$outmap 2>$stderr
	exec r.colors --q map=$outmap rast=$inmap 2>$stderr
}

proc finalize {} {
	global tempfile tempreg tempmap stderr finalized

	if {$finalized} return

	save_map

	file delete $tempfile
	exec g.remove --q rast=$tempmap region=$tempreg 2>$stderr

	set finalized true

	exit 0
}

proc force_window {} {
	global origin rows cols total

	if {$origin(x) < 0} {set origin(x) 0}
	if {$origin(x) > $total(cols) - $cols} {set origin(x) [expr $total(cols) - $cols]}
	if {$origin(y) < 0} {set origin(y) 0}
	if {$origin(y) > $total(rows) - $rows} {set origin(y) [expr $total(rows) - $rows]}
}

proc set_window {x y} {
	global origin rows cols

	set origin(x) [expr [.overview.canvas canvasx $x] - $cols / 2]
	set origin(y) [expr [.overview.canvas canvasy $y] - $rows / 2]

	force_window

	set x0 $origin(x)
	set y0 $origin(y)
	set x1 [expr $x0 + $cols]
	set y1 [expr $y0 + $rows]

	.overview.canvas delete window
	.overview.canvas create rectangle $x0 $y0 $x1 $y1 -dash {4 4} -tags window
}

proc update_window {} {
	global wind total origin rows cols

	set x0 $origin(x)
	set y0 $origin(y)
	set x1 [expr $x0 + $cols]
	set y1 [expr $y0 + $rows]

	set wind(n) [expr $total(n) - $y0 * $total(nsres)]
	set wind(s) [expr $total(n) - $y1 * $total(nsres)]
	set wind(w) [expr $total(w) + $x0 * $total(ewres)]
	set wind(e) [expr $total(w) + $x1 * $total(ewres)]
	set wind(rows) $rows
	set wind(cols) $cols
}

proc change_window {} {
	save_map
	update_window
	load_map
	load_aspect
	refresh_canvas
}

proc create_overview {} {
	global inmap outmap stderr env total rows cols tempfile

	exec g.region --q rast=$inmap 2>$stderr
	exec r.out.ppm --q $inmap out=$tempfile 2>$stderr

	set reg [exec g.region --q -g 2>$stderr]
	set reg [regsub -all {[\r\n]+} $reg { }]
	set reg [regsub -all {=} $reg { }]
	array set total $reg

	image create photo overview -file $tempfile
	file delete $tempfile

	toplevel .overview
	wm title .overview "d.rast.edit overview ($inmap)"

	set w $total(cols)
	set h $total(rows)

	canvas .overview.canvas -width $w -height $h -scrollregion [list 0 0 $w $h] \
	    -xscrollcommand {.overview.xscroll set} -yscrollcommand {.overview.yscroll set}

	scrollbar .overview.xscroll -orient horizontal -command {.overview.canvas xview}
	scrollbar .overview.yscroll -orient vertical   -command {.overview.canvas yview}

	if {$cols > $total(cols)} {set cols $total(cols)}
	if {$rows > $total(rows)} {set rows $total(rows)}

	force_window

	.overview.canvas create image 0 0 -anchor nw -image overview -tags image
	.overview.canvas create rectangle 0 0 $cols $rows -dash {4 4} -tags window

	grid .overview.canvas .overview.yscroll -sticky nsew
	grid .overview.xscroll -sticky nsew

	grid rowconfigure    .overview 0 -weight 1
	grid columnconfigure .overview 0 -weight 1

	bind .overview.canvas <ButtonPress-1>   { set_window %x %y }
	bind .overview.canvas <B1-Motion>       { set_window %x %y }
	bind .overview.canvas <ButtonRelease-1> { set_window %x %y ; change_window }

	bind .overview <Destroy> { finalize }
}

proc read_header {infile window} {
	upvar \#0 $window wind

	regexp {^north: *([0-9]+)$} [gets $infile] dummy wind(n)
	regexp {^south: *([0-9]+)$} [gets $infile] dummy wind(s)
	regexp {^east: *([0-9]+)$}  [gets $infile] dummy wind(e)
	regexp {^west: *([0-9]+)$}  [gets $infile] dummy wind(w)
	regexp {^rows: *([0-9]+)$}  [gets $infile] dummy wind(rows)
	regexp {^cols: *([0-9]+)$}  [gets $infile] dummy wind(cols)
}

proc read_data {infile array} {
	global wind
	upvar \#0 $array values

	for {set row 0} {$row < $wind(rows)} {incr row} {
		gets $infile line
		set col 0
		foreach elem $line {
			set values($row,$col) $elem
			incr col
		}
	}
}

proc clear_changes {} {
	global wind changed

	for {set row 0} {$row < $wind(rows)} {incr row} {
		for {set col 0} {$col < $wind(cols)} {incr col} {
			set changed($row,$col) 0
		}
	}
}

proc load_map {} {
	global tempfile wind values changed colors inmap stderr

	exec g.region --q n=$wind(n) s=$wind(s) e=$wind(e) w=$wind(w) \
	    rows=$wind(rows) cols=$wind(cols) 2>$stderr

	set infile [open "|r.out.ascii --q input=$inmap 2>$stderr" r]
	read_header $infile wind
	read_data $infile values
	close $infile

	clear_changes

	exec r.out.ppm --q input=$inmap output=$tempfile 2>$stderr

	image create photo colorimg -file $tempfile
	file delete $tempfile

	for {set row 0} {$row < $wind(rows)} {incr row} {
		for {set col 0} {$col < $wind(cols)} {incr col} {
			set val $values($row,$col)
			if {[array get colors $val] != ""} continue
			set pix [colorimg get $col $row]
			set r [lindex $pix 0]
			set g [lindex $pix 1]
			set b [lindex $pix 2]
			set color [format "#%02x%02x%02x" $r $g $b]
			set colors($val) $color
		}
	}

	image delete colorimg
}

proc load_aspect {} {
	global wind angles aspect stderr

	if {$aspect == ""} return

	set infile [open "|r.out.ascii --q input=$aspect 2>$stderr" r]
	read_header $infile dummy
	read_data $infile angles
	close $infile
}

proc save_map {} {
	global inmap outmap tempmap stderr
	global wind values changed

	set outfile [open "|r.in.ascii --q --o input=- output=$tempmap 2>$stderr" w]

	puts $outfile "north: $wind(n)"
	puts $outfile "south: $wind(s)"
	puts $outfile "east: $wind(e)"
	puts $outfile "west: $wind(w)"
	puts $outfile "rows: $wind(rows)"
	puts $outfile "cols: $wind(cols)"

	for {set row 0} {$row < $wind(rows)} {incr row} {
		for {set col 0} {$col < $wind(cols)} {incr col} {
			if {$col > 0} {
				puts -nonewline $outfile " "
			}
			if {$changed($row,$col)} {
				puts -nonewline $outfile "$values($row,$col)"
			} else {
				puts -nonewline $outfile "*"
			}
		}
		puts $outfile ""
	}

	close $outfile

	exec g.region --q rast=$inmap 2>$stderr
	exec r.patch --q --o input=$tempmap,$outmap output=$outmap 2>$stderr
	exec r.colors --q map=$outmap rast=$inmap 2>$stderr
	exec g.remove --q rast=$tempmap 2>$stderr
}

proc force_color {val} {
	global tempfile tempreg tempmap colors inmap stderr env

	exec g.region --q rows=1 cols=1 2>$stderr
	exec r.mapcalc "$tempmap = $val" 2>$stderr
	exec r.colors --q map=$tempmap rast=$inmap 2>$stderr
	exec r.out.ppm --q $tempmap out=$tempfile 2>$stderr
	exec g.remove --q rast=$tempmap 2>$stderr

	image create photo tempimg -file $tempfile
	file delete $tempfile

	set pix [tempimg get 0 0]
	set r [lindex $pix 0]
	set g [lindex $pix 1]
	set b [lindex $pix 2]
	set color [format "#%02x%02x%02x" $r $g $b]
	set colors($val) $color
	image delete tempimg
}

proc get_color {val} {
	global colors

	if {[array get colors $val] == ""} {
		if {[catch {force_color $val}]} {
			set colors($val) "#ffffff"
		}
	}

	return $colors($val)
}

proc brush_update {} {
	global brush colors

	if {$brush == "*"} {
		.tools.color configure -bitmap gray12 -foreground black
	} else {
		.tools.color configure -bitmap gray75 -foreground [get_color $brush]
	}
}

proc current_cell {} {
	global canvas

	set row ""
	set col ""

	set tags [.canvas itemcget current -tags]

	foreach tag $tags {
		if {[regexp {row-([0-9]+)} $tag dummy r]} {set row $r}
		if {[regexp {col-([0-9]+)} $tag dummy c]} {set col $c}
	}

	return [list $row $col]
}

proc cell_enter {} {
	global status
	global wind values angles

	set pos [current_cell]
	set row [lindex $pos 0]
	set col [lindex $pos 1]

	if {$row == "" || $col == ""} return

	set status(row) $row
	set status(col) $col
	set status(x) [expr {$wind(e) + ($col + 0.5) * ($wind(e) - $wind(w)) / $wind(cols)}]
	set status(y) [expr {$wind(n) - ($row + 0.5) * ($wind(n) - $wind(s)) / $wind(rows)}]
	set status(value) $values($row,$col)
	if {[array exists angles]} {
		set status(aspect) $angles($row,$col)
	}
}

proc cell_leave {} {
	global status

	set status(row) ""
	set status(col) ""
	set status(x) ""
	set status(y) ""
	set status(value) ""
	set status(aspect) ""
}

proc cell_get {} {
	global brush values colors

	set pos [current_cell]
	set row [lindex $pos 0]
	set col [lindex $pos 1]

	set brush $values($row,$col)

	brush_update
}

proc cell_set {} {
	global canvas brush values changed colors

	set pos [current_cell]
	set row [lindex $pos 0]
	set col [lindex $pos 1]
	set val $brush

	set values($row,$col) $val
	set changed($row,$col) 1

	set cell [.canvas find withtag "(cell&&row-$row&&col-$col)"]

	if {$val == "*"} {
		set fill black
		set stipple gray12 
	} else {
		set fill [get_color $val]
		set stipple ""
	}

	.canvas itemconfigure $cell -outline white -fill $fill -stipple $stipple
}

proc refresh_canvas {} {
	global wind size values colors angles

	.canvas delete all

	set aspect [array exists angles]
	set pi [expr 2 * acos(0)]

	for {set row 0} {$row < $wind(rows)} {incr row} {
		for {set col 0} {$col < $wind(cols)} {incr col} {
			set x0 [expr $col * $size + 1]
			set x1 [expr $x0 + $size - 1]
			set y0 [expr $row * $size + 1]
			set y1 [expr $y0 + $size - 1]

			if {$values($row,$col) == "*"} {
				set color black
				set stipple gray12
			} else {
				set color $colors($values($row,$col))
				set stipple ""
			}

			.canvas create polygon $x0 $y0 $x1 $y0 $x1 $y1 $x0 $y1 \
			    -fill $color -stipple $stipple \
			    -outline black -activeoutline red \
			    -tags [list cell row-$row col-$col]

			if {! $aspect} continue

			if {$angles($row,$col) == "*"} continue

			set cx [expr ($x0 + $x1) / 2]
			set cy [expr ($y0 + $y1) / 2]

			set a [expr $angles($row,$col) * $pi / 180]

			set dx [expr   cos($a) * $size / 2]
			set dy [expr - sin($a) * $size / 2]

			set x0 [expr $cx - $dx]
			set y0 [expr $cy - $dy]
			set x1 [expr $cx + $dx]
			set y1 [expr $cy + $dy]

			.canvas create line $x0 $y0 $x1 $y1 \
			    -arrow last \
			    -disabledfill white -state disabled \
			    -tags [list arrow row-$row col-$col]
		}
	}
}

proc make_canvas {} {
	global canvas values colors angles rows cols
	global size width height

	set cx [expr $width  / $cols]
	set cy [expr $height / $rows]

	set sz [expr ($cx > $cy) ? $cx : $cy]
	if {$size < $sz} {set size $sz}

	set w [expr $cols * $size]
	set h [expr $rows * $size]

	canvas .canvas -width $width -height $height -scrollregion [list 0 0 $w $h] \
	    -xscrollcommand {.xscroll set} -yscrollcommand {.yscroll set}

	scrollbar .xscroll -orient horizontal -command {.canvas xview}
	scrollbar .yscroll -orient vertical   -command {.canvas yview}

	.canvas bind cell <Any-Enter> { cell_enter }
	.canvas bind cell <Any-Leave> { cell_leave }

	.canvas bind cell <Button-1> { cell_set }
	.canvas bind cell <Button-3> { cell_get }

	bind .canvas <Any-Leave> { cell_leave }
}

proc make_ui {} {
	global canvas inmap

	wm title . "d.rast.edit ($inmap)"
	bind . <Destroy> { finalize }

	menu .menu -tearoff 0
	menu .menu.file -tearoff 0
	.menu add cascade -label "File" -menu .menu.file -underline 0
	.menu.file add command -label "Save" -underline 0 -command {save_map}
	.menu.file add command -label "Exit" -underline 1 -command {destroy .}

	. configure -menu .menu

	frame .status
	label .status.row_l -text "Row:"
	entry .status.row -textvariable status(row) -width 6
	label .status.col_l -text "Col:"
	entry .status.col -textvariable status(col) -width 6
	label .status.x_l -text "X:"
	entry .status.x -textvariable status(x) -width 10
	label .status.y_l -text "Y:"
	entry .status.y -textvariable status(y) -width 10
	label .status.value_l -text "Value:"
	entry .status.value -textvariable status(value) -width 10
	label .status.aspect_l -text "Aspect:"
	entry .status.aspect -textvariable status(aspect) -width 10

	pack \
	    .status.row_l .status.row \
	    .status.col_l .status.col \
	    .status.x_l .status.x \
	    .status.y_l .status.y \
	    .status.value_l .status.value \
	    .status.aspect_l .status.aspect \
	    -side left

	frame .tools
	label .tools.value_l -text "New value:"
	entry .tools.value -textvariable brush
	label .tools.color_l -text "Color:"
	label .tools.color -bitmap gray12 -foreground black

	pack \
	    .tools.value_l .tools.value \
	    .tools.color_l .tools.color \
	    -side left

	bind .tools.value <KeyPress-Return> brush_update

	grid .canvas .yscroll -sticky nsew
	grid .xscroll -sticky nsew
	grid .status  -sticky nsew
	grid .tools  -sticky nsew

	grid rowconfigure    . 0 -weight 1
	grid columnconfigure . 0 -weight 1
}

initialize
create_overview
make_canvas
make_ui
update_window
load_map
load_aspect
refresh_canvas
