
############################################################################
#
# LIBRARY:      Gronsole program run and output widget
# AUTHOR(S):    Cedric Shock (cedricgrass AT shockfamily.net)
#               Based on lib/gis/gui.tcl
# PURPOSE:      Runs programs, displays output
# COPYRIGHT:    (C) 2006 Grass Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

namespace eval Gronsole {
	variable _data
	variable _options

	set _options [list [list -clickcmd clickCmd ClickCmd {} {}]]

	proc ::Gronsole { path args } { return [eval Gronsole::create $path $args] }
	proc use {} {}
}

proc Gronsole::dooptions {path args init} {
	variable _data
	variable _options
	
	foreach opt $_options {
		set sw [lindex $opt 0]
		set db [lindex $opt 1]
		set def [lindex $opt 4]
		if {[set idx [lsearch -exact $args $sw]] != -1} {
			set _data($path,$db) [lindex $args [expr $idx + 1]]
			set args [concat [lrange $args 0 [expr $idx - 1]] [lrange $args [expr $idx + 2] end]]
		} elseif {$init} {
			set _data($path,$db) $def
		}
	}
}

proc Gronsole::create {path args} {
	global keycontrol
	global bgcolor
	variable _data

	set args [Gronsole::dooptions $path $args 1]

	set gronsolewin [ScrolledWindow $path -relief flat -borderwidth 1 -auto horizontal]
	set gronsole [eval text $gronsolewin.text $args]
	$gronsolewin setwidget $gronsole	


	set _data($path,count) 0

	bind $path.text <Destroy>   "Gronsole::_destroy $path"
	bind $path.text <$keycontrol-c> "tk_textCopy %W"
	bind $path.text <$keycontrol-v> "tk_textPaste %W"
	bind $path.text <$keycontrol-x> "tk_textCut %W"

	rename $path ::$path:scrollwin
	proc ::$path { cmd args } "return \[eval Gronsole::\$cmd $path \$args\]"
	return $path
}

proc Gronsole::configure { path args } {
	variable _options
	variable _data
	if {$args == {}} {
		set res {}
		foreach opt $_options {
			set sw [lindex $opt 0]
			set db [lindex $opt 1]
			set title [lindex $opt 2]
			lappend res [list $sw $db $title $_data($path,$db) $_data($path,$db)]
		}
		return [concat $res [$path.text configure]]
	}

	set args [Gronsole::dooptions $path $args 0]
	
	$path.text configure $args

	return
}


proc Gronsole::cget { path option } {
    variable _options
    variable _data
    if {[lsearch -exact $_options $option] != -1} {
        set res $_data($path,$option)
    } else {
        set res [$path.text cget $option]
    }
    return $res
}

proc Gronsole::_destroy { path } {
    variable _data

    array unset _data "$path,*"

    catch {rename $path {}}
}

##########################################################################
# Public contents management

proc Gronsole::clear {path} {
	variable _data

	$path.text delete 1.0 end
}


# save text in output window
proc Gronsole::save {path} {
	global env

	set dtxt $path.text

	if ![catch {$dtxt get sel.first}] {
		set svtxt [$dtxt get sel.first sel.last]
	} else {
		set svtxt [$dtxt get 1.0 end]
	} 
	
	set types {
     {{TXT} {.txt}}
	}

	if { [info exists HOME] } {
		set dir $env(HOME)
		set path [tk_getSaveFile -initialdir $dir -filetypes $types \
			-defaultextension ".txt"]
	} else {
		set path [tk_getSaveFile -filetypes $types \
			-defaultextension ".txt"]
	}

	if { $path == "" } { return }

	set txtfile [open $path w]
	puts $txtfile $svtxt
	close $txtfile
	return
}

proc Gronsole::destroy_command {path ci} {
	variable _data

	catch {close $_data($path,$ci,fh)} 

	if {[info exists _data($path,$ci,donecmd)] && $_data($path,$ci,donecmd) != {}} {
		eval $_data($path,$ci,donecmd)
	}

	set textarea $path.text
	set frame $_data($path,$ci,frame)

	set indices [$textarea tag ranges cmd$ci]

	eval $textarea delete $indices

	destroy $frame

	array unset _data "$path,$ci,*"
}

##########################################################################
# Private

proc Gronsole::do_click {path ci} {
	variable _data

	# Use this commands click command if it exists
	if {[info exists _data($path,$ci,clickCmd)]} {
		set cc $_data($path,$ci,clickCmd)
	} else {
		set cc $_data($path,clickCmd)
	}
	if {$cc != {}} {
		eval $cc $ci [list $_data($path,$ci,cmd)]
	}
}

proc Gronsole::create_command {path cmd} {
	variable _data
	set textarea $path.text

	incr _data($path,count)
	set ci $_data($path,count)
	set _data($path,$ci,cmd) $cmd
	
	set module [lindex $cmd 0]
	set icon [icon module $module]

	set frame $textarea.cmd$ci

	set _data($path,$ci,frame) $frame
	
	frame $frame 
	frame $frame.cmdline
	set tagframe [frame $frame.cmdline.tags]
	set cmdlabel [label $frame.cmdline.cmd -textvariable Gronsole::_data($path,$ci,cmd) -anchor nw]
	bind $cmdlabel <Button-1> "Gronsole::do_click $path $ci"
	# set cmdlabel [text $frame.cmdline.cmd -height 1 -width 10]
	# $cmdlabel insert end $cmd
	set ex [button $frame.cmdline.eX -text "X" -command "Gronsole::destroy_command $path $ci"]
	pack $ex -side right
	pack $frame.cmdline.tags -side right
	set iconwidth ""
	if {$icon != 0} {
		set iconwidth " - \[winfo width $frame.cmdline.icon\]"
		button $frame.cmdline.icon -image $icon -anchor n -command "Gronsole::do_click $path $ci"
		pack $frame.cmdline.icon -side left
	}
	pack $frame.cmdline.cmd -side left -expand yes -fill x
	pack $frame.cmdline -side top -expand yes -fill x
	set pbar [ProgressBar $frame.progress -fg green -bg white -height 20 -relief raised \
		-maximum 100 -variable Gronsole::_data($path,$ci,progress)] 
	pack $pbar -side left
	set _data($path,$ci,progress) -1
	set _data($path,$ci,progressbar) $pbar
	set _data($path,$ci,tags) {}

	$textarea insert end "\n" [list cmd$ci e1]
	$textarea insert end "\n" [list cmd$ci e2]
	$textarea mark set cmdinsert$ci "end - 2 char"

	$textarea window create cmdinsert$ci -window $frame
	$textarea tag add cmd$ci $frame
	$textarea insert cmdinsert$ci "$cmd\n" [list cmd$ci e2]
	# $textarea tag add cmd$ci "cmdinsert$ci - 1 char"
	# $textarea tag add e2 "cmdinsert$ci - 1 char"

	$textarea tag configure e1 -elide 1
	$textarea tag configure e2 -elide 1


	set pspace 12
	$pbar configure -width [expr [winfo width $textarea] - $pspace]
#	$pbar configure -width [expr [winfo width $textarea] - $pspace] -height 20

	bind $textarea <Configure> "+catch {$pbar configure -width \[expr \[winfo width $textarea\] - $pspace\]}"


	bind $textarea <Configure> "+catch {$cmdlabel configure -wraplength \[expr \[winfo width $textarea\] - $pspace - \[winfo width $tagframe\] - \[winfo width $ex\] $iconwidth\]}"

	# bind $cmdlabel <Configure> "$cmdlabel configure -wraplength \[winfo width $cmdlabel\]"

	return $ci
}

##########################################################################
# Public tag management. add_data_tag is private

proc Gronsole::set_click_command {path ci cmd} {
	variable _data
	set _data($path,$ci,clickCmd) $cmd
}

proc Gronsole::show_hide_tag_data {path ci tag} {
	variable _data
	set textarea $path.text
	
	set e [$textarea tag cget cmd$ci-$tag -elide]
	if {$e == {}} {
		$textarea tag configure cmd$ci-$tag -elide 1
	} else {
		$textarea tag configure cmd$ci-$tag -elide {}
	}
}

proc Gronsole::add_tag {path ci tag} {
	variable _data
	set textarea $path.text
	set frame $_data($path,$ci,frame)
	if {[lsearch -exact $_data($path,$ci,tags) $tag] != -1} {
		return
	}
	lappend _data($path,$ci,tags) $tag
	button $frame.cmdline.tags.tag$tag -text $tag -relief flat
	set icon [icon status $tag]
	if {$icon != 0} {
		$frame.cmdline.tags.tag$tag configure -image $icon
	}
	pack $frame.cmdline.tags.tag$tag -side right
}

# This is private:
proc Gronsole::add_data_tag {path ci tag} {
	variable _data
	set textarea $path.text
	set frame $_data($path,$ci,frame)
	if {[lsearch -exact $_data($path,$ci,tags) $tag] != -1} {
		return
	}
	Gronsole::add_tag $path $ci $tag
	$frame.cmdline.tags.tag$tag configure -relief raised -command "Gronsole::show_hide_tag_data $path $ci $tag"
}

proc Gronsole::remove_tag {path ci tag} {
	variable _data
	set frame $_data($path,$ci,frame)
	pack forget $frame.cmdline.tags.tag$tag
	# destroy $frame.cmdline.tags.tag$tag	
}


##########################################################################
# Private (stuff done when commands are run)

# This procedure doesn't really seem necessary. I've left it in
# in case there is something I'm missing (M. Barton 29 April 2007)
proc Gronsole::progress {path ci percent} {
	variable _data

	if {[info exists _data($path,$ci,progress)]} {
		set _data($path,$ci,progress) $percent
	}
	if {[info exists _data($path,$ci,progressbar)]} {
		set pbar $_data($path,$ci,progressbar)
	}

	if {$percent == -1} {
		$pbar configure -height 1
	} else {
		$pbar configure -height 20
	}
	# it seems that there is a bug in ProgressBar and it is not always updated ->
	$pbar _modify
}

proc Gronsole::output_to_gronsole {path mark ci tags str} {
	set outtext $path.text

	set tagbase cmd$ci
	# Back out backspaces:
	if {0} {
	while {[set idx [string first "\b" $str]] != -1} {
		set last [expr $idx - 1]
		set str1 [string range $str 1 $last]
		set first [expr $idx + 1]
		set str [string range $str $first end]
		set pos [$outtext index "$mark - 1 chars"]
		$outtext delete $pos
		$outtext insert $mark $str1 $tags
	}
	}
	if { [regexp -- {^GRASS_INFO_([^(]+)\(([0-9]+),([0-9]+)\): (.+)$} $str match key message_pid message_id val rest] } {
		set lkey [string tolower $key]
		Gronsole::add_tag $path $ci $lkey
		set icon [icon status $lkey]
		if {$icon != 0} {
			$outtext image create $mark -image $icon
			# $outtext tag add $tagbase "$mark -1 char"
		}
		$outtext insert $mark $val $tagbase
	} elseif { [regexp -- {^GRASS_INFO_PERCENT: (.+)$} $str match val rest] } {
		if { $val > 0 && $val < 100} { 
			set Gronsole::_data($path,$ci,progress) $val
#			Gronsole::progress $path $ci $val
		} else {
#			Gronsole::progress $path $ci -1
			set Gronsole::_data($path,$ci,progress) -1
			$outtext insert $mark "\n" $tags
		}
	} elseif { [regexp -- {^GRASS_INFO_END.+} $str match key rest] } {
		# nothing
	} else {
		$outtext insert $mark $str $tags
	}
}

proc Gronsole::readeof {path ci mark fh} {
	variable _data
	# This doesn't actually get the result
	set result [catch {close $fh} error_text]
	set _data($path,$ci,result) $result
	# if {$result == 0} {
		# Gronsole::add_tag $path $ci success
		# set donecmd $_data($path,$ci,successcmd)
	#} else {
		# Gronsole::add_tag $path $ci failure
		# set donecmd $_data($path,$ci,failurecmd)
	#}
	Gronsole::remove_tag $path $ci running
}

proc Gronsole::readout {path ci mark fh} {

	set lines {}
	
	while {[gets $fh line] >= 0} {
		lappend lines $line
	}
	
	if {[llength $lines] != 0} {
		Gronsole::add_data_tag $path $ci out
	}
	foreach line $lines {
		Gronsole::output_to_gronsole $path $mark $ci [list cmd$ci cmd$ci-out] "$line\n"
	}
	$path.text see $mark
}

proc Gronsole::done_command {path ci} {
	variable _data

	if {[info exists _data($path,$ci,donecmd)] && $_data($path,$ci,donecmd) != {}} {
		set donecmd $_data($path,$ci,donecmd)
		set _data($path,$ci,donecmd) {}
	}

	if {[info exists donecmd] && $donecmd != {}} {
		eval $donecmd
	}
}

proc Gronsole::file_callback {path ci mark fh} {
	if [eof $fh] {
		Gronsole::readeof $path $ci $mark $fh
		Gronsole::done_command $path $ci
	} else {
		Gronsole::readout $path $ci $mark $fh
	}
}

proc Gronsole::execbg {path ci mark fh} {
	fconfigure $fh -blocking 0
	fileevent $fh readable [list Gronsole::file_callback $path $ci $mark $fh]
}

proc Gronsole::execwait {path ci mark fh} {
	while {! [eof $fh]} {
		Gronsole::readout $path $ci $mark $fh
		update
	}
	Gronsole::readeof $path $ci $mark $fh
	update
}

proc Gronsole::execout {path cmd ci execcmd} {
	global env

	set mark cmdinsert$ci

	# Actually run the program
	# |& grocat merges stdout and stderr because Tcl treats
	# anything written to stderr as an error condition
	set cmd [concat | $cmd |& $env(GISBASE)/etc/grocat]

	set message_env [exec g.gisenv get=GRASS_MESSAGE_FORMAT]
        set env(GRASS_MESSAGE_FORMAT) gui
	set ret [catch {open $cmd r} fh]
        set env(GRASS_MESSAGE_FORMAT) $message_env

	set _data($path,$ci,fh) $fh

	if { $ret } {
		Gronsole::remove_tag $path $ci running
		Gronsole::add_tag $path $ci error
		catch {close $fh}
		Gronsole::done_command $path $ci
	} {
		$execcmd $path $ci $mark $fh
	}
	update idletasks
}

##########################################################################
# Public interface for running commands

proc Gronsole::annotate {path cmd tags} {
	variable _data

	set ci [Gronsole::create_command $path $cmd]

	foreach tag $tags {
		Gronsole::add_tag $path $ci $tag
	}

	$path.text yview end
	
	return $ci
}

proc Gronsole::annotate_text {path ci text} {
	Gronsole::output_to_gronsole $path cmdinsert$ci $ci [list cmd$ci cmd$ci-out] $text
	$path.text see cmdinsert$ci
}

proc Gronsole::run {path cmd tags donecmd} {
	variable _data
	
	set tags [concat running $tags]

	set ci [Gronsole::annotate $path $cmd $tags]

	set _data($path,$ci,donecmd) $donecmd

	Gronsole::execout $path $cmd $ci Gronsole::execbg

	return $ci
}

proc Gronsole::run_wait {path cmd tags} {
	set tags [concat running $tags]

	set ci [Gronsole::annotate $path $cmd $tags]

	Gronsole::execout $path $cmd $ci Gronsole::execwait
}

proc Gronsole::run_xterm {path cmd tags} {
	global env
	global mingw

	Gronsole::annotate $path $cmd [concat xterm $tags]

	if { $mingw == "1" } {
	    exec -- cmd.exe /c start $env(GISBASE)/etc/grass-run.bat $cmd &
	} else {
	    exec -- $env(GISBASE)/etc/grass-xterm-wrapper -name xterm-grass -e $env(GISBASE)/etc/grass-run.sh $cmd &
	}

	update idletasks
}
