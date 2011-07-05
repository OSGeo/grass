 # This program is free software under the GPL (>=v2)
 # Read the COPYING file that comes with GRASS for details.

 ######################################################################
 # PROCEDURES
 ######################################################################
 #update sample frame environment variables
 proc updateSF_Environment { } {
 	global env
 	exec g.region rast=$env(RASTER) 
	exec g.region -g > $env(TMP).tmp 
	set n [ exec cat $env(TMP).tmp | grep "n=" | cut -f2 -d= ]
	set s [ exec cat $env(TMP).tmp | grep "s=" | head -n 1 | cut -f2 -d= ]
	set w [ exec cat $env(TMP).tmp | grep "w=" | cut -f2 -d= ]
	set e [ exec cat $env(TMP).tmp | grep "e=" | cut -f2 -d= ]
	set env(SF_N) $n
	set env(SF_S) $s
	set env(SF_E) $e
	set env(SF_W) $w
	set env(SF_NSRES) [ exec cat $env(TMP).tmp | grep "nsres=" | cut -f2 -d= ]
	set env(SF_EWRES) [ exec cat $env(TMP).tmp | grep "ewres=" | cut -f2 -d= ]
	set env(SF_X) 0
	set env(SF_Y) 0
	set env(SF_RL) [expr abs(round(double($s - $n) / double($env(SF_NSRES))))]
	set env(SF_CL) [expr abs(round(double($e - $w) / double($env(SF_EWRES))))]
	file delete $env(TMP).tmp
	#debug line
	#tk_messageBox -message "$env(SF_N)|$env(SF_S)|$env(SF_W)|$env(SF_E)|$env(SF_NSRES)|$env(SF_EWRES)|"
	
 }
 
 
 #shows the instruction for drawing squares 
 proc squareInstruction {} {
 	toplevel .instruction
	wm title .instruction "\[r.li.setup\] Commands"
	#wm maxsize .instruction 300 200
	frame .instruction.txt
	pack .instruction.txt 
	text .instruction.txt.t -font Helvetica -height 12 
	.instruction.txt.t tag configure big -font {Helvetica 16 bold}
	.instruction.txt.t tag configure normal -font {Helvetica 14}
	.instruction.txt.t insert end "Mouse buttons functions \n \n" big
	.instruction.txt.t insert end "Left button: " big
	.instruction.txt.t insert end "set first corner \n" normal
	.instruction.txt.t insert end "Center button: " big
	.instruction.txt.t insert end "set second corner \n" normal
	.instruction.txt.t insert end "Right  button: " big
	.instruction.txt.t insert end "done \n" normal
	pack .instruction.txt.t
	frame .instruction.buttons
	pack .instruction.buttons -side bottom
	button .instruction.buttons.ok -text ok -command { destroy .instruction }
	pack .instruction.buttons.ok
	#.instruction.txt configure -state disabled
	return .instruction
 }
 
 proc vectorInstruction {} {
 	toplevel .instruction
	wm title .instruction "\[r.li.setup\] Commands"
	#wm maxsize .instruction 300 200
	frame .instruction.txt
	pack .instruction.txt 
	text .instruction.txt.t -font Helvetica -height 12 
	.instruction.txt.t tag configure big -font {Helvetica 16 bold}
	.instruction.txt.t tag configure normal -font {Helvetica 14}
	.instruction.txt.t insert end "Mouse buttons functions \n \n" big
	.instruction.txt.t insert end "Left button: " big
	.instruction.txt.t insert end "none \n" normal
	.instruction.txt.t insert end "Center button: " big
	.instruction.txt.t insert end "toggle point \n" normal
	.instruction.txt.t insert end "Right  button: " big
	.instruction.txt.t insert end "done \n" normal
	pack .instruction.txt.t
	frame .instruction.buttons
	pack .instruction.buttons -side bottom
	button .instruction.buttons.ok -text ok -command { destroy .instruction }
	pack .instruction.buttons.ok
	#.instruction.txt configure -state disabled
	return .instruction
 }
 
 proc circleInstruction {} {
 	toplevel .instruction
	wm title .instruction "\[r.li.setup\] Commands"
	#wm maxsize .instruction 300 200
	frame .instruction.txt
	pack .instruction.txt 
	text .instruction.txt.t -font Helvetica -height 12 
	.instruction.txt.t tag configure big -font {Helvetica 16 bold}
	.instruction.txt.t tag configure normal -font {Helvetica 14}
	.instruction.txt.t insert end "Mouse buttons functions \n \n" big
	.instruction.txt.t insert end "Left button: " big
	.instruction.txt.t insert end "none \n" normal
	.instruction.txt.t insert end "Center button: " big
	.instruction.txt.t insert end "toggle center (first press) \n \t\t\ttoggle radius length (second press) \n" normal
	.instruction.txt.t insert end "Right  button: " big
	.instruction.txt.t insert end "none \n" normal
	pack .instruction.txt.t
	frame .instruction.buttons
	pack .instruction.buttons -side bottom
	button .instruction.buttons.ok -text ok -command { destroy .instruction }
	pack .instruction.buttons.ok
	#.instruction.txt configure -state disabled
	return .instruction
 }
 # Create a simple file browser
 proc fileBrowser {path entry} {
 	global p_entry
	set p_entry $entry
 	toplevel .fileBrowser
	wm title .fileBrowser "\[r.li.setup\] File browser"
	#filelist frame
	frame .fileBrowser.top -relief flat
	pack .fileBrowser.top -side top -fill y -anchor center
	listbox .fileBrowser.top.listbox -selectmode single
	openDir .fileBrowser.top.listbox $path
	pack .fileBrowser.top.listbox -expand 1 -fill both -padx 7 -pady 7 
	#browser buttons
	frame .fileBrowser.buttons
	pack .fileBrowser.buttons -side bottom -pady 2 -anchor center
	button .fileBrowser.buttons.open -text "Open" -command {set selection [fileSelect .fileBrowser .fileBrowser.top.listbox $p_entry]}
	pack .fileBrowser.buttons.open
 
 }
 
 proc fileSelect {widget listbox entry} {
 set selection [$listbox get [$listbox curselection]]
		switch [file type $selection] {
		directory {
		openDir $listbox $selection
		}
		file {
		destroy $widget
		$entry insert 0 [pwd]/$selection
		}
	}
}
 
 
 
 #Open the specified directory 
 proc openDir {listbox newpath} {
    catch {cd $newpath}
    $listbox delete 0 end
    foreach f [lsort [glob -nocomplain *]] {
	$listbox insert end $f
    }
}

# defines sampling frame
proc defineSamplingFrame {selection button } {
global env
set tmp $env(TMP)
switch $selection {
	whole {
		if { $env(RASTER) != "" } then {
			exec echo "SAMPLINGFRAME 0|0|1|1" >> $tmp.set
			updateSF_Environment
			tk_messageBox -message "Whole maplayer set as sampling frame" -type ok
			$button configure -state disabled
		} else {
			tk_messageBox -message "Please set raster name first" -type ok -icon error
		}
	}
	keyboard {
		kSamplingFrame
		$button configure -state disabled
	}
	mouse {
	if { $env(RASTER) == "" || $env(CONF) == ""  } then {
		tk_messageBox -message "Please enter a raster map and a configuration file name first" -type ok -icon error
	} else {
		set ins [squareInstruction]
		tkwait window $ins
		catch { exec $env(F_PATH)/square_mouse_selection.sh raster=$env(RASTER) vector=$env(VECTOR) site=$env(SITE) conf=$tmp.tmp } 
		set ok ""
		catch {set ok [exec cat $tmp.tmp | grep "SQUAREAREA" | cut -f1 -d\ ]}
		if { $ok == "SQUAREAREA" } then {
			#sampling frame accepted
			set start [exec cat $tmp.tmp | grep "START" | cut -f2 -d\ ] 
			scan $start %f|%f|%f|%f|%f|%f s_n s_s s_e s_w s_nres s_sres
			set square [exec cat $tmp.tmp | grep "SQUAREAREA" | cut -f2 -d\ ]
			#resolution north-south
			set nres ""
			#resolution east-west
			set sres ""
			scan $square %f|%f|%f|%f|%f|%f n s e w nres sres
			set env(SF_N) $n
			set env(SF_S) $s
			set env(SF_E) $e
			set env(SF_W) $w
			set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
			set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
			# calulating area coordinates
			set env(SF_Y) [expr abs(round(($s_n - $n) / $nres)) ] 
			set env(SF_X) [expr abs(round(($s_w - $w) / $sres)) ] 
			set env(SF_RL) [expr abs(round(($n - $s) / $nres)) ]
			set env(SF_CL) [expr abs(round(($e - $w) / $sres)) ]
			set env(SF_NSRES) $nres
			set env(SF_EWRES) $sres
			set x [expr double($env(SF_X)) / double($cols)]
			set y [ expr double($env(SF_Y)) / double($rows)]
			set rl [ expr double($env(SF_RL)) / double($rows)]
			set cl [ expr double($env(SF_CL)) / double($cols) ]
			#debug line
			#tk_messageBox -message "$x|$y|$rl|$cl"
			exec echo "SAMPLINGFRAME $x|$y|$rl|$cl" >> $tmp.set 
			tk_messageBox -message "Selected area set as sampling frame" -type ok
			file delete $tmp.tmp
			$button configure -state disabled
		} else {
			tk_messageBox -message "Warning sampling frame not set" -type ok -icon warning
		}
	}
	}
}

}

# defines sampling area
proc defineSamplingArea {selection widget} {
global env
set tmp $env(TMP)
switch $selection {
	whole {
		set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
		set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\| ]
		set x [expr double($env(SF_X)) / double($cols) ]
		set y [expr double($env(SF_Y)) / double($rows) ]
		set rl [expr double($env(SF_RL)) /double($rows) ]
		set cl [expr double($env(SF_CL)) /double($cols) ]
		exec echo "SAMPLEAREA $x|$y|$rl|$cl" >> $tmp.set
		tk_messageBox -message "Whole maplayer set as sampling area" -type ok
	}
	regions {
	setSampleRegions $widget
	}
	units {
	setSampleUnits $widget
	}
	window {
	setMovWindow $widget
	}
	vector {
		tk_messageBox -message "WARNING: this configuration file will work only on $env(RASTER) raster map" -type ok -icon warning
		#TODO change here
		if { $env(RASTER) != "" && $env(VECTOR) != "" && $env(CONF) != "" } then { 
			catch { exec  $env(F_PATH)/sample_area_vector.sh  raster=$env(RASTER) vector=$env(VECTOR) conf=$env(TMP).set } 
		} else {
			tk_messageBox -message "Please set configuration file name, raster map and vector file to overlay" -type ok -icon error
		}
	}
}

}

# defines sampling units distribuition
proc defineSamplingUnits {selec rl cl maskname} {
	global env
	set tmp $env(TMP) 
	#da modificare qua per le proiezioni
	switch $selec {
	
		nonoverlapping {
			toplevel .dialog 
			wm title .dialog " Random Nonoverlapping "
			wm minsize .dialog 300 150
		
			frame .dialog.scale
			pack .dialog.scale
			label .dialog.scale.label1 -text " What number of Sampling Units to use?"
			entry .dialog.scale.e1 -width 5 -textvariable number1
			grid .dialog.scale.label1 .dialog.scale.e1   -padx 3
			
			button .dialog.button -text " Ok " -command {
				if  { $number1!="" && ![catch { exec printf %i $number1 }]} then {
					exec echo "RANDOMNONOVERLAPPING $number1" >> $env(TMP).set
					tk_messageBox -message "Sampling units distribuition set as Random Nonoverlapping" -type ok
					set number1 ""
					destroy .dialog
				} else {
					tk_messageBox -message "Please type integer value" -type ok -icon error
				} 
			}
			pack .dialog.button
		}
		
		contiguous { 
			exec echo "SYSTEMATICCONTIGUOUS " >> $env(TMP).set
			tk_messageBox -message "Sampling units distribuition set as Systematic Contiguous" -type ok
		}
		
		noncontiguous { 
			toplevel .dialog 
			wm title .dialog " Systematic non contiguous "
			wm minsize .dialog 300 150
		
			frame .dialog.scale
			pack .dialog.scale
			label .dialog.scale.label1 -text " Insert distance between units"
			entry .dialog.scale.e1 -width 5 -textvariable number1
			grid .dialog.scale.label1 .dialog.scale.e1   -padx 3
		
	       		button .dialog.button -text " Ok " -command {
			if  { $number1!="" && ![catch { exec printf %i $number1 }]} then {
				exec echo "SYSTEMATICNONCONTIGUOUS $number1" >> $env(TMP).set
				tk_messageBox -message "Sampling units distribuition set as Systematic Non Contiguous" -type ok
				set number1 ""
				destroy .dialog
			} else {
				tk_messageBox -message "Please type integer value" -type ok -icon error
			}
		}
		pack .dialog.button	
		}
		
		random { 
			toplevel .dialog 
			wm title .dialog " Stratified Random "
			wm minsize .dialog 300 150
		
			frame .dialog.scale
			pack .dialog.scale
			label .dialog.scale.label1 -text " Insert number of row strates "
			entry .dialog.scale.e1 -width 5 -textvariable number1
			grid .dialog.scale.label1 .dialog.scale.e1   -padx 3
			
			frame .dialog.scale2
			pack .dialog.scale2
			label .dialog.scale2.label2 -text " Insert number of column strates "
			entry .dialog.scale2.e2 -width 5 -textvariable number2
			grid .dialog.scale2.label2 .dialog.scale2.e2   -padx 3
		
	       		button .dialog.button -text " Ok " -command {
			if  { $number1!="" && $number2!="" && ![catch { exec printf %i%i $number1 $number2 }]} then {
				exec echo "STRATIFIEDRANDOM $number1|$number2" >> $env(TMP).set
				tk_messageBox -message "Sampling units distribuition set as Stratified random" -type ok
				set number1 ""
				destroy .dialog
			} else {
				tk_messageBox -message "Please type integer values" -type ok -icon error
			}
		}
		pack .dialog.button
		}
		
		sites { 
			if { $env(RASTER) == "" || $env(SITE) == "" } then {
				tk_messageBox -message "Please set raster and site file names first" -icon error
			} else {
				centerOverSites $rl $cl $maskname
			}
		}
	}
}

proc saveWindow {sel number1 number2 number3 maskname widget} {
	global env
	set tmp $env(TMP)
	switch $sel {
		rectangle {
		#check if we have all values
		if  { $number1 != "" && $number2!=""} then {
			if { [catch { exec printf %i%i $number1 $number2 }]  } then {
			#check if we have integers
			tk_messageBox -message "Type integer values" -type ok -icon error
			} else {
				set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
				set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\| ]
				set rl [expr double($number2) / double($rows) ]
				set cl [expr double($number1) /double($cols) ]
				exec echo "SAMPLEAREA -1|-1|$rl|$cl" >> $env(TMP).set
				exec  echo "MOVINGWINDOW" >> $env(TMP).set
				tk_messageBox -message " Moving Windows Setted "  -type ok
				destroy $widget
			}
		} else {
			tk_messageBox -message "Set all entries first" -type ok -icon error
			}
		}
		circle { 
			if  { $number3 != "" } then {
				if { [catch { exec printf %i $number3 }]  } then {
				#check if we have integers
				tk_messageBox -message "Type integer values" -type ok -icon error
			} else {
				circleMask $number3 $maskname
				set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
				set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
				set rl [expr double($env(CIR_RL)) /double($rows)]
				set cl [expr double($env(CIR_CL)) /double($cols)]
				exec echo "MASKEDSAMPLEAREA -1|-1|$rl|$cl|$maskname" >> $env(TMP).set
				tk_messageBox -message " Moving Windows Setted "  -type ok
				exec  echo "MOVINGWINDOW" >> $env(TMP).set
				destroy $widget
				}
			} else {
				tk_messageBox -message "Set all entries first" -type ok -icon error
				}
		}
	}
}

#draw regions with mouse
proc drawRegions { number } {
	global env
	if { $env(RASTER) == "" } then {
		tk_messageBox -message "Please set the rastermap first" -type ok -icon error
	} else {
		set i 0
		set ins [vectorInstruction]
		tkwait window $ins
		while { $i < $number } {
		 	catch { exec $env(F_PATH)/masked_area_selection.sh -f north=$env(SF_N) south=$env(SF_S) west=$env(SF_W) east=$env(SF_E) raster=$env(RASTER) vector=$env(VECTOR) site=$env(SITE) conf=$env(TMP).tmp }
			set ok ""
			catch {set ok [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f1 -d\ ]}
			if { $ok == "SAMPLEAREAMASKED" } then {
				#region accepted 
				incr i
				set r_name [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f2 -d\ ]
				set square [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f3 -d\ ]
				scan $square %f|%f|%f|%f n s e w
				set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
				set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
				set abs_y [expr $env(SF_Y) + abs(round(($env(SF_N) - $n) / $env(SF_NSRES))) ] 
				set abs_x [expr $env(SF_X)+ abs(round(($env(SF_W) - $w) / $env(SF_EWRES))) ] 
				set abs_rl [expr abs(round(($n - $s) / $env(SF_NSRES))) ]
				set abs_cl [expr abs(round(($e - $w) / $env(SF_EWRES))) ]
				#debug line
				#tk_messageBox -message "$abs_x|$abs_y|$abs_rl|$abs_cl"
				set x [expr double($abs_x) / double($cols)]
				set y [ expr double($abs_y) / double($rows)]
				set rl [ expr double($abs_rl) / double($rows)]
				set cl [ expr double($abs_cl) / double($cols) ]
				#debug line
				#tk_messageBox -message "$x|$y|$rl|$cl"
				exec echo "MASKEDSAMPLEAREA $x|$y|$rl|$cl|$r_name" >> $env(TMP).set 
				tk_messageBox -message "Selected region saved as sampling area" -type ok
				file delete $env(TMP).tmp
			} else {
				tk_messageBox -message "Please redraw region number $i" -type ok -icon warning
			}
		}
		  
		 
		
	}
}

#draw sample units with mouse
proc drawMouseUnits { num sel } {
	global env
	# rectangular or circle units ?
	switch  $sel {
		rectangle {
			#rectangular units
			set i 0
			set ins [squareInstruction]
			tkwait window $ins
			if { $env(RASTER) == "" || $env(CONF) == ""  } then {
				tk_messageBox -message "Please enter a raster map and a configuration file name first" -type ok -icon error
			} else {
				while { $i < $num } {
					catch { exec $env(F_PATH)/square_mouse_selection.sh -f north=$env(SF_N) south=$env(SF_S) east=$env(SF_E) west=$env(SF_W) raster=$env(RASTER) vector=$env(VECTOR) site=$env(SITE) conf=$env(TMP).tmp } 
					set ok ""
					catch {set ok [exec cat $env(TMP).tmp | grep "SQUAREAREA" | cut -f1 -d\ ]}
					if { $ok == "SQUAREAREA" } then {
						#sampling area accepted
						incr i
						set start [exec cat $env(TMP).tmp | grep "START" | cut -f2 -d\ ] 
						scan $start %f|%f|%f|%f|%f|%f s_n s_s s_e s_w s_nres s_sres
						set square [exec cat $env(TMP).tmp | grep "SQUAREAREA" | cut -f2 -d\ ]
						#resolution north-south
						set nres ""
						#resolution east-west
						set sres ""
						scan $square %f|%f|%f|%f|%f|%f n s e w nres sres
						set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
						set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
						# calulating area coordinates
						set abs_y [expr abs(round(($s_n - $n) / $nres)) ] 
						set abs_x [expr abs(round(($s_w - $w) / $sres)) ] 
						set abs_rl [expr abs(round(($n - $s) / $nres)) ]
						set abs_cl [expr abs(round(($e - $w) / $sres)) ]
						#debug line
						#tk_messageBox -message "$abs_x|$abs_y|$abs_rl|$abs_cl"
						set x [expr double($abs_x) / double($cols)]
						set y [ expr double($abs_y) / double($rows)]
						set rl [ expr double($abs_rl) / double($rows)]
						set cl [ expr double($abs_cl) / double($cols) ]
						#debug line
						#tk_messageBox -message "$x|$y|$rl|$cl"
						exec echo "SAMPLEAREA $x|$y|$rl|$cl" >> $env(TMP).set 
						tk_messageBox -message "Selected area saved as sample area" -type ok
						file delete $env(TMP).tmp
					} else {
						tk_messageBox -message "Warning sampling area not set" -type ok -icon warning
					}	
				}
			}
		}
		circle {
			#circulars areas
			set i 0
			set ins [circleInstruction]
			tkwait window $ins
			while { $i < $num } {
		 		catch { exec $env(F_PATH)/masked_area_selection.sh -f -c north=$env(SF_N) south=$env(SF_S) west=$env(SF_W) east=$env(SF_E) raster=$env(RASTER) vector=$env(VECTOR) site=$env(SITE) conf=$env(TMP).tmp }
				set ok ""
				catch {set ok [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f1 -d\ ]}
				if { $ok == "SAMPLEAREAMASKED" } then {
					#region accepted 
					incr i
					set r_name [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f2 -d\ ]
					set square [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f3 -d\ ]
					scan $square %f|%f|%f|%f n s e w
					set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
					set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
					set abs_y [expr $env(SF_Y) + abs(round(($env(SF_N) - $n) / $env(SF_NSRES))) ] 
					set abs_x [expr $env(SF_X)+ abs(round(($env(SF_W) - $w) / $env(SF_EWRES))) ] 
					set abs_rl [expr abs(round(($n - $s) / $env(SF_NSRES))) ]
					set abs_cl [expr abs(round(($e - $w) / $env(SF_EWRES))) ]
					#debug line
					#tk_messageBox -message "$abs_x|$abs_y|$abs_rl|$abs_cl"
					set x [expr double($abs_x) / double($cols)]
					set y [ expr double($abs_y) / double($rows)]
					set rl [ expr double($abs_rl) / double($rows)]
					set cl [ expr double($abs_cl) / double($cols) ]
					#debug line
					#tk_messageBox -message "$x|$y|$rl|$cl"
					exec echo "MASKEDSAMPLEAREA $x|$y|$rl|$cl|$r_name" >> $env(TMP).set 
					tk_messageBox -message "Selection saved as sampling area" -type ok
					file delete $env(TMP).tmp
				} else {
					tk_messageBox -message "Please redraw sample unit number $i" -type ok -icon warning
				}
			}
		}
	}
	
}

#draw moving window with mouse
proc drawMouseWindow { sel } {
	global env 
	#rectangular or circular window
	switch $sel {
		rectangle {
		set i 0
		set ins [squareInstruction]
			tkwait window $ins
			if { $env(RASTER) == "" || $env(CONF) == ""  } then {
				tk_messageBox -message "Please enter a raster map and a configuration file name first" -type ok -icon error
			} else {
				while { $i == 0 } {
					catch { exec $env(F_PATH)/square_mouse_selection.sh -f north=$env(SF_N) south=$env(SF_S) east=$env(SF_E) west=$env(SF_W) raster=$env(RASTER) vector=$env(VECTOR) site=$env(SITE) conf=$env(TMP).tmp } 
					set ok ""
					catch {set ok [exec cat $env(TMP).tmp | grep "SQUAREAREA" | cut -f1 -d\ ]}
					if { $ok == "SQUAREAREA" } then {
						#moving window accepted
						incr i
						set start [exec cat $env(TMP).tmp | grep "START" | cut -f2 -d\ ] 
						scan $start %f|%f|%f|%f|%f|%f s_n s_s s_e s_w s_nres s_sres
						set square [exec cat $env(TMP).tmp | grep "SQUAREAREA" | cut -f2 -d\ ]
						#resolution north-south
						set nres ""
						#resolution east-west
						set sres ""
						scan $square %f|%f|%f|%f|%f|%f n s e w nres sres
						set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
						set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
						#calculating moving window width and length
						set abs_rl [expr abs(round(($n - $s) / $nres)) ]
						set abs_cl [expr abs(round(($e - $w) / $sres)) ]
						#debug line
						#tk_messageBox -message "$abs_x|$abs_y|$abs_rl|$abs_cl"
						set rl [ expr double($abs_rl) / double($rows)]
						set cl [ expr double($abs_cl) / double($cols) ]
						#debug line
						#tk_messageBox -message "$x|$y|$rl|$cl"
						exec echo "MOVINGWINDOW" >> $env(TMP).set
						exec echo "SAMPLEAREA -1|-1|$rl|$cl" >> $env(TMP).set 
						tk_messageBox -message "Moving window set" -type ok
						file delete $env(TMP).tmp
					} else {
						tk_messageBox -message "Moving window not set" -type ok -icon warning
					}	
				}
			}	
		}
		circle {
			#circulars areas
			set i 0
			set ins [circleInstruction]
			tkwait window $ins
			while { $i == 0 } {
		 		catch { exec $env(F_PATH)/masked_area_selection.sh -f -c north=$env(SF_N) south=$env(SF_S) west=$env(SF_W) east=$env(SF_E) raster=$env(RASTER) vector=$env(VECTOR) site=$env(SITE) conf=$env(TMP).tmp }
				set ok ""
				catch {set ok [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f1 -d\ ]}
				if { $ok == "SAMPLEAREAMASKED" } then {
					#region accepted 
					incr i
					set r_name [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f2 -d\ ]
					set square [exec cat $env(TMP).tmp | grep "SAMPLEAREAMASKED" | cut -f3 -d\ ]
					scan $square %f|%f|%f|%f n s e w
					set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
					set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
					set abs_rl [expr abs(round(($n - $s) / $env(SF_NSRES))) ]
					set abs_cl [expr abs(round(($e - $w) / $env(SF_EWRES))) ]
					#debug line
					#tk_messageBox -message "$abs_x|$abs_y|$abs_rl|$abs_cl"
					set rl [ expr double($abs_rl) / double($rows)]
					set cl [ expr double($abs_cl) / double($cols) ]
					#debug line
					#tk_messageBox -message "$x|$y|$rl|$cl"
					exec echo "MOVINGWINDOW" >> $env(TMP).set
					exec echo "MASKEDSAMPLEAREA -1|-1|$rl|$cl|$r_name" >> $env(TMP).set 
					tk_messageBox -message "Moving window set" -type ok
					file delete $env(TMP).tmp
				} else {
					tk_messageBox -message "Moving window not set" -type ok -icon warning
				}
			}
		}	
	}
}

#create a circle mask for the keyboard circle selection
proc circleMask { radius name} {
	global env 
	exec g.region rast=$env(RASTER)
	exec g.region -m > $env(TMP).tmp
	set nsres [ exec cat $env(TMP).tmp | grep "nsres=" | cut -f2 -d= ]
	set ewres [ exec cat $env(TMP).tmp | grep "ewres=" | cut -f2 -d= ]
	#calculating number of cell needed
	set xcell [expr round((2 * $radius) / $ewres) ]
	set ycell [expr round((2 * $radius) / $nsres) ]
	#to create a good raster circle the center of the circle have to be
	#in the center of a cell, then we need an odd number of cells... 
	if { [ expr $xcell % 2 ] == 0 } then {
		incr xcell
	}
	if { [ expr $ycell % 2 ] == 0 } then {
		incr ycell
	}
	#store in environment xcell and ycell if we have to center this circle
	set env(CIR_RL) $ycell
	set env(CIR_CL) $xcell
	#calculating easth and south edge
	set easthEdge [expr double($env(SF_W) + ($xcell * $env(SF_EWRES)))]
	set southEdge [expr double($env(SF_N) - ($ycell * $env(SF_NSRES)))]
	#restrict region
	exec g.region n=$env(SF_N) s=$southEdge e=$easthEdge w=$env(SF_W)
	set xcenter [exec g.region -c | grep "region center easting:" | cut -f2 -d: | tr -d " "]
	set ycenter [exec g.region -c | grep "region center northing:" | cut -f2 -d: | tr -d " "]
	#debug line
	#tk_messageBox -message "$xcenter , $ycenter $env(SF_N) $southEdge $env(SF_W) $easthEdge"
	#creating circle
	catch {exec r.circle -b output=$name coordinate=$xcenter,$ycenter max=$radius }
	file delete $env(TMP).tmp
} 

#set sample units from a site file 

proc centerOverSites { rl cl name} {
	global env 
	if { $env(SITE) == "" || $env(RASTER) == "" } then {
		tk_messageBox -message "Please set raster and site file name first" -type ok -icon error
	} else {
		#raster boundaries
		exec g.region rast=$env(RASTER) 
		exec g.region -g > $env(TMP).tmp 
		set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
		set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\| ]
		#create ascii site file 
		exec v.out.ascii input=$env(SITE) output=$env(TMP).asc format=point
		#counting points 
		set num [exec cat $env(TMP).asc | grep -c "" ]
		set i 0
		
		# inserting point if they are into sample frame
		while { $i < $num } {
			incr i
			#read the line i
			exec head -n $i $env(TMP).asc | tail -n 1 > $env(TMP).line
			set x [exec cat $env(TMP).line | cut -f1 -d\| ]
			set y [exec cat $env(TMP).line | cut -f2 -d\| ]
			#check if selected point is into sample frame
			#debug line
			#tk_messageBox -message "a=$x b=$y c=$env(SF_N) d=$env(SF_S) e=$env(SF_W) f=$env(SF_E)"
			set ok "TRUE"
			if { $y < $env(SF_S) || $y > $env(SF_N) || $x < $env(SF_W) || $x > $env(SF_E) } then {
				set ok "FALSE"
			}
			if { $ok == "TRUE" } then {
				#the point is into sample frame
				#calculating what cell contains this point 
				set p_c [expr int( abs($x - $env(SF_W)) / $env(SF_EWRES))]
				set p_r [expr int( abs($y - $env(SF_N)) / $env(SF_NSRES))]
				#debug line
				#tk_messageBox -message "$p_c $p_r"
				#the point is the center of the rectangle, we have to see if the rectangle is into sample frame
				set rl_delta [expr int( $rl/2)]
				set cl_delta [expr int( $cl/2)]
				set n_diff [expr ($p_r - $rl_delta) - $env(SF_Y)] 
				set s_diff [expr ($env(SF_Y) + $env(SF_RL)) - ($p_r + $rl_delta +1) ]
				set e_diff [expr ($env(SF_X) + $env(SF_CL)) - ($p_c + $cl_delta + 1) ]
				set w_diff [expr ($p_c - $cl_delta) - $env(SF_X)]
				if { $n_diff > 0 && $s_diff > 0 && $e_diff > 0 && $w_diff > 0 } then {
					#the rectangle is into sampling frame
					set rel_x [expr double($p_c - $cl_delta) / double($cols)]
					set rel_y [expr double($p_r - $rl_delta) /double($rows)]
					set rel_rl [expr double($rl) / double($rows)]
					set rel_cl [expr double($cl) / double($cols)]
					if { $name == "" } then {
						#the sample frame don't have a mask
						exec echo "SAMPLEAREA $rel_x|$rel_y|$rel_rl|$rel_cl" >> $env(TMP).set
					} else {
						# the sample frame has a mask
						exec echo "MASKEDSAMPLEAREA $rel_x|$rel_y|$rel_rl|$rel_cl|$name" >> $env(TMP).set
					}
				}
			}
				
		}
	file delete $env(TMP).tmp $env(TMP).line $env(TMP).asc 
	
	} 
}

proc saveSettings { widget } {
	global env
	
	#write the sample frame
	exec cat $env(TMP).set | grep "SAMPLINGFRAME " | tail -n 1 > $env(CONF) 
	#write sampling areas
	catch  { exec cat $env(TMP).set | grep "SAMPLEAREA " >> $env(CONF) } 
	catch { exec cat $env(TMP).set | grep "MASKEDSAMPLEAREA " >> $env(CONF) }
	set overlay 0
	catch { set overlay [ exec cat $env(TMP).set | grep "MASKEDOVERLAYAREA " -c ] }
	if { $overlay != 0 } then {
		exec cat $env(TMP).set | grep "MASKEDOVERLAYAREA " >> $env(CONF) 
		exec echo "RASTERMAP $env(RASTER)" >> $env(CONF)
		exec echo "VECTORMAP $env(VECTOR)" >> $env(CONF)
	}
	#write disposition line
	catch { exec cat $env(TMP).set | grep "MOVINGWINDOW" >> $env(CONF) } 
	catch { exec cat $env(TMP).set | grep "RANDOMNONOVERLAPPING " >> $env(CONF) } 
	catch { exec cat $env(TMP).set | grep "SYSTEMATICCONTIGUOUS " >> $env(CONF) }
	catch { exec cat $env(TMP).set | grep "SYSTEMATICNONCONTIGUOUS " >> $env(CONF) } 
	catch { exec cat $env(TMP).set | grep "STRATIFIEDRANDOM " >> $env(CONF)  }
	file delete $env(TMP).set
	destroy $widget
}
