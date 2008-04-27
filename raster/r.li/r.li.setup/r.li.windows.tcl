# This program is free software under the GPL (>=v2)
# Read the COPYING file that comes with GRASS for details.


#####################################################################
#OTHERS WINDOWS
#####################################################################

#Sampling frame keyboard input dialog
proc kSamplingFrame {} {
	global env
	toplevel .keyboard
	wm title .keyboard "Insert parameters"
	wm minsize .keyboard 200 200
	#insert top label
	label .keyboard.topLabel -text "Insert sampling frame parameters"
	pack .keyboard.topLabel -side top
	#insert input textboxes
	frame .keyboard.input
	pack .keyboard.input
	label .keyboard.input.xLabel -text "Column of upper left corner" -anchor e
	entry .keyboard.input.x  -width 10 -textvariable env(SF_X)
	grid .keyboard.input.xLabel .keyboard.input.x -sticky ew
	label .keyboard.input.yLabel -text "Row of upper left corner" -anchor e
	entry .keyboard.input.y  -width 10 -textvariable env(SF_Y)
	grid .keyboard.input.yLabel .keyboard.input.y -sticky ew
		
	label .keyboard.input.rlLabel -text "Row length of sampling frame" -anchor e
	entry .keyboard.input.rl  -width 10 -textvariable env(SF_RL)
	grid .keyboard.input.rlLabel .keyboard.input.rl -sticky ew

	label .keyboard.input.clLabel -text "Column length of upper left corner" -anchor e
	entry .keyboard.input.cl  -width 10 -textvariable env(SF_CL)
	grid .keyboard.input.clLabel .keyboard.input.cl -sticky ew

#buttons 
	frame .keyboard.buttons
	pack .keyboard.buttons
	button .keyboard.buttons.ok -text "Ok" -command {
		#check if we have all values
		if  {$env(SF_X) != "" && $env(SF_Y)!="" && $env(SF_RL)!="" && $env(SF_CL)!="" } then {
			if { [catch { exec printf %i%i%i%i $env(SF_X) $env(SF_Y) $env(SF_RL) $env(SF_CL)  }] || $env(RASTER)== "" } then {
			#check if we have integers
			tk_messageBox -message "Type integer values or set raster map name" -type ok -icon error
			} else {
			set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
			set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
			set per_x [expr double(double($env(SF_X))/double($cols))]
			set per_y [expr double(double($env(SF_Y))/double($rows))]
			set per_rl [expr double(double($env(SF_RL))/double($rows))]
			set per_cl [expr double(double($env(SF_CL))/double($cols))]
			exec echo "SAMPLINGFRAME $per_x|$per_y|$per_rl|$per_cl" >> $env(TMP).set
			#setting the others sample frame environment variables
			exec g.region rast=$env(RASTER)
			exec g.region -g > $env(TMP).tmp
			set n [ exec cat $env(TMP).tmp | grep "n=" | cut -f2 -d= ]
			set w [ exec cat $env(TMP).tmp | grep "w=" | cut -f2 -d= ]
			set env(SF_NSRES) [ exec cat $env(TMP).tmp | grep "nsres=" | cut -f2 -d= ]
			set env(SF_EWRES) [ exec cat $env(TMP).tmp | grep "ewres=" | cut -f2 -d= ]
			set env(SF_N) [ expr double( $n - ( $env(SF_NSRES) * $env(SF_Y) )) ]
			set env(SF_S) [ expr double( $n - ( $env(SF_NSRES) * ($env(SF_Y) + $env(SF_RL)) )) ]
			set env(SF_W) [ expr double( $w + ( $env(SF_EWRES) * $env(SF_X) )) ]
			set env(SF_E) [ expr double( $w + ( $env(SF_EWRES) * ($env(SF_X) + $env(SF_CL)) )) ]
			file delete $env(TMP).tmp
			destroy .keyboard
			}
		} else {
		tk_messageBox -message "Set all entries first" -type ok -icon error
		}
	}
	pack .keyboard.buttons.ok
}

#procedure to set sampling units using keyboard
proc setKeyboardUnit {} {

	#new popup window
	toplevel .kUni
	wm title .kUni "\[r.li.setup\] Units - keyboard"
	wm minsize .kUni 300 330

	# top label
	label .kUni.topLabel -text " Select type of shape"
	pack .kUni.topLabel -side top -pady 10

	# frame for choice
	frame .kUni.choice
	pack .kUni.choice
	set sel "" 
	global kUnisel 
	
	
	#radiobuttons rectangle and circle
	radiobutton .kUni.choice.rectangle -text "Rectangle" -relief flat -variable kUnisel -value rectangle -width 20 -anchor w -padx 15 -command { 
	 .kUni.scale.e5 configure -state disabled 
	.kUni.scale.label5 configure -state disabled
	.kUni.scale.e4 configure -state disabled 
	.kUni.scale.label4 configure -state disabled
	.kUni.scale.label2 configure -state active
	.kUni.scale.e3 configure -state normal
	.kUni.scale.label3 configure -state active
	.kUni.scale.e2 configure -state normal
	.kUni.choice.rectangle configure -state active
	}
	radiobutton .kUni.choice.circle -text "Circle" -relief flat -variable kUnisel -value circle -width 20 -anchor w -padx 15 -command { 
	.kUni.scale.label5 configure -state active
	.kUni.scale.label4 configure -state active
	.kUni.scale.e4 configure -state normal
	.kUni.scale.e3 configure -state disabled 
	.kUni.scale.e2 configure -state disabled 
	.kUni.scale.label2 configure -state disabled
	.kUni.scale.label3 configure -state disabled
	.kUni.scale.e5 configure -state normal
	#.kUni.choice.rectangle configure -state disabled 
	}
	pack .kUni.choice.rectangle .kUni.choice.circle 
	.kUni.choice.rectangle select 
	#new popup window
	frame .kUni.uniDistr
	pack .kUni.uniDistr
        #insert top label
	#label .kUni.uniDistr.topLabel
	#pack .kUni.uniDistr.topLabel 

	set selec ""
	radiobutton .kUni.uniDistr.a -text " Random nonoverlapping" -width 30 -relief flat -variable selec -value nonoverlapping -anchor w -padx 15
	radiobutton .kUni.uniDistr.b -text " Systematic contiguous" -width 30 -relief flat -variable selec -value contiguous -anchor w -padx 15
	radiobutton .kUni.uniDistr.c -text " Systematic noncontiguous" -width 30 -relief flat -variable selec -value noncontiguous -anchor w -padx 15
	radiobutton .kUni.uniDistr.d -text " Stratified random" -width 30 -relief flat -variable selec -value random -anchor w -padx 15
	radiobutton .kUni.uniDistr.e -text " Centered over sites" -width 30 -relief flat -variable selec -value sites -anchor w -padx 15 -command {
		if { $env(SITE) == "" } then {
			tk_messageBox -message "Please set site file name" -icon error
		}
	}
	pack .kUni.uniDistr.a .kUni.uniDistr.b .kUni.uniDistr.c .kUni.uniDistr.d .kUni.uniDistr.e 
	.kUni.uniDistr.a select

	
	#frame to specify input settings
	frame .kUni.scale
	pack  .kUni.scale

	#label and entries for the settings
	
	

	label .kUni.scale.label2 -text " What wide size (in cells) for each sampling unit?" 
	entry .kUni.scale.e2 -width 5 -textvariable number2 
	grid .kUni.scale.label2 .kUni.scale.e2 -pady 20 -padx 3
	label .kUni.scale.label3 -text " What high size (in cells) for each sampling unit?" 
	entry .kUni.scale.e3 -width 5 -textvariable number3 
	grid .kUni.scale.label3 .kUni.scale.e3  -padx 3
	label .kUni.scale.label4 -text " What radius size (in meters) for each sampling unit?" -state disabled
	entry .kUni.scale.e4 -width 5 -textvariable number4 -state disabled
	grid .kUni.scale.label4 .kUni.scale.e4 -pady 20 -padx 3
	label .kUni.scale.label5 -text " Name for the circle mask" -state disabled
	entry .kUni.scale.e5 -width 15 -textvariable maskname -state disabled 
	grid .kUni.scale.label5 .kUni.scale.e5  -padx 3
	

	#part to determine the sampling units distribuition metod
	#label for distribuition metod
	label .kUni.choice.distribuition -text "Select Method of Sampling Unit Distribuition" 
	pack .kUni.choice.distribuition -pady 10
	#frame for save or quit
	frame .kUni.button
	pack .kUni.button -side bottom 

	button .kUni.button.save -text "Save Settings" -command {
	switch $kUnisel {
	     rectangle {
		#check if we have all values
		if  { $number2 != "" && $number3 != "" } then {
			if { [catch { exec printf %i%i $number2 $number3 }] || $env(RASTER)== "" } then {
			#check if we have integers
			tk_messageBox -message "Type integer values or set raster map name" -type ok -icon error
			} else {
			set Rrows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
			set Rcols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
			set ratioW [expr double(double($number2)/double($Rcols))]
			set ratioH [expr double(double($number3)/double($Rrows))]
			defineSamplingUnits $selec $number3 $number2 ""
			if { $selec != "sites" } then {
				exec echo "SAMPLEAREA -1|-1|$ratioH|$ratioW" >> $env(TMP).set
			}
			destroy .kUni
			}
		} else {
		tk_messageBox -message "Set all entries first" -type ok -icon error
		}
	}
	circle {
		#check if we have all values
		if  { $number4 != "" } then {
			if { [catch { exec printf %i $number4 }] || $env(RASTER)== "" || $maskname == "" } then {
				#check if we have integers
				tk_messageBox -message "Type integer values or set raster map name" -type ok -icon error
			} else {
				circleMask $number4 $maskname
				defineSamplingUnits $selec $env(CIR_RL) $env(CIR_RL) $maskname
				if { $selec != "sites" } then {
					set rows [exec  r.info map=$env(RASTER) | grep "Rows" | tr -d " " |   cut -f 2 -d: | cut -f 1 -d\| ]
					set cols [exec r.info map=$env(RASTER) | grep "Columns" | tr -d " " | cut -f 2 -d: | cut -f 1 -d\|  ]
					set rl [expr double($env(CIR_RL)) /double($rows)]
					set cl [expr double($env(CIR_CL)) /double($cols)]
					exec echo "MASKEDSAMPLEAREA -1|-1|$rl|$cl|$maskname" >> $env(TMP).set
				}
				destroy .kUni
			}
	  }
	 }
	}
     }



pack .kUni.button.save -padx 5 -pady 20
		

}


#procedure to set moving window using keyboard
proc setKeyboardWindow {} {

	#new popup window
	toplevel .kWin
	wm title .kWin "\[r.li.setup\] Moving Windows - keyboard"
	wm minsize .kWin 300 330

	# top label
	label .kWin.topLabel -text " Select type of shape"
	pack .kWin.topLabel -side top -pady 10

	# frame for choice
	frame .kWin.choice
	pack .kWin.choice
	set sel "" 

	#radiobuttons rectangle and circle
	radiobutton .kWin.choice.rectangle -text "Rectangle" -relief flat -variable sel -value rectangle -width 20 -anchor w -padx 15 -command { 
		.kWin.scale.label4 configure -state disabled
		.kWin.scale.e4 configure -state disabled
		.kWin.scale.label3 configure -state disabled 
		.kWin.scale.e3 configure -state disabled 
		.kWin.scale.label1 configure -state active
		.kWin.scale.label2 configure -state active
		.kWin.scale.e1 configure -state normal
		.kWin.scale.e2 configure -state normal 
		}

	radiobutton .kWin.choice.circle -text "Circle" -relief flat -variable sel -value circle -width 20 -anchor w -padx 15 -command { 
		.kWin.scale.label4 configure -state active
		.kWin.scale.e4 configure -state normal
		.kWin.scale.label3 configure -state active
		.kWin.scale.e3 configure -state normal
		.kWin.scale.e1 configure -state disabled 
		.kWin.scale.e2 configure -state disabled 
		.kWin.scale.label1 configure -state disabled
		.kWin.scale.label2 configure -state disabled 
		}
	pack .kWin.choice.rectangle .kWin.choice.circle 
	.kWin.choice.rectangle select 

	#frame to specify input settings
	frame .kWin.scale
	pack  .kWin.scale

	#label and entries for the settings

	label .kWin.scale.label1 -text " Width size (in cells)?" 
	entry .kWin.scale.e1 -width 5 -textvariable number1 
	grid .kWin.scale.label1 .kWin.scale.e1  -padx 3
	label .kWin.scale.label2 -text " Height size (in cells)?" 
	entry .kWin.scale.e2 -width 5 -textvariable number2 
	grid .kWin.scale.label2 .kWin.scale.e2  -padx 3 -pady 20
	label .kWin.scale.label3 -text " What radius size (in meters)?" -state disabled
	entry .kWin.scale.e3 -width 5 -textvariable number3 -state disabled
	grid .kWin.scale.label3 .kWin.scale.e3  -padx 3
	label .kWin.scale.label4 -text " Name of the circle mask" -state disabled
	entry .kWin.scale.e4 -width 15 -textvariable maskname -state disabled
	grid .kWin.scale.label4 .kWin.scale.e4 -pady 20 -padx 3

	#frame for save or quit
	frame .kWin.button
	pack .kWin.button -side bottom 

	button .kWin.button.save -text "Save Settings" -command { saveWindow $sel $number1 $number2 $number3 $maskname .kWin }
	grid .kWin.button.save -padx 5

}
# set sampling units with mouse
proc setMouseUnits { widget } {
	global env
	global setMouseUnits
	set setMouseUnits $widget
	if { $env(RASTER) == "" || $env(CONF) == ""  } then {
		tk_messageBox -message "Please enter a raster map and a configuration file name first" -type ok -icon error
	} else {
		frame $widget.mouseUnits
		pack $widget.mouseUnits
		radiobutton $widget.mouseUnits.rectangle -text "Draw rectangular units" -relief flat -variable sel -value rectangle -width 20 -anchor w
		radiobutton $widget.mouseUnits.circle -text "Draw circular units" -relief flat -variable sel -value circle -width 20 -anchor w
		$widget.mouseUnits.rectangle select
		pack $widget.mouseUnits.rectangle $widget.mouseUnits.circle
		frame $widget.mouseUnits.grid
		pack $widget.mouseUnits.grid
		label $widget.mouseUnits.grid.label -text "Insert number of areas to draw" 
		entry $widget.mouseUnits.grid.entry -width 10 -textvariable num
		grid $widget.mouseUnits.grid.label $widget.mouseUnits.grid.entry
		frame $widget.mouseUnits.buttons
		pack $widget.mouseUnits.buttons
		button $widget.mouseUnits.buttons.ok -text Ok -command {
			if { ! [catch { exec prinf %i $num } ] } then {
				tk_messageBox -message "Please set the number of areas to draw first" -type ok -icon error
			} else {
				drawMouseUnits $num $sel 
				$setMouseUnits.mouseUnits.buttons.ok configure -state disabled
			}
		}
		pack $widget.mouseUnits.buttons.ok
		
	}
}

# set moving window with mouse
proc setMouseWindow { widget } {
	global env
	global setMouseWindow
	set setMouseWindow $widget
	if { $env(RASTER) == "" || $env(CONF) == ""  } then {
		tk_messageBox -message "Please enter a raster map and a configuration file name first" -type ok -icon error
	} else {
		frame $widget.mouseWindow
		pack $widget.mouseWindow
		radiobutton $widget.mouseWindow.rectangle -text "Draw rectangular window" -relief flat -variable sel -value rectangle -width 22 -anchor w
		radiobutton $widget.mouseWindow.circle -text "Draw circular window" -relief flat -variable sel -value circle -width 22 -anchor w
		$widget.mouseWindow.rectangle select
		pack $widget.mouseWindow.rectangle $widget.mouseWindow.circle
		frame $widget.mouseWindow.buttons
		pack $widget.mouseWindow.buttons
		button $widget.mouseWindow.buttons.ok -text Ok -command {
			drawMouseWindow $sel 
			$setMouseWindow.mouseWindow.buttons.ok configure -state disabled
		}
		pack $widget.mouseWindow.buttons.ok
		
	}
}

# show the selected configuration
proc loadConfiguration { filename } {
	global env
	toplevel .load
	wm title .load "$filename configuration"
	
	frame .load.display
	pack .load.display
	canvas .load.display.c -relief raised -width 600 -height 560
	pack .load.display.c
	set c .load.display.c
	# create reference lines
	.load.display.c create line 50 50 50 550
	.load.display.c create line 50 50 550 50
	#insert reference values 
	for {set i 1} {$i <= 10} {incr i} {
    		set x [expr {50 + ($i*50)}]
    		$c create line $x 45 $x 50 -width 2
    		$c create text $x 35 -text [expr {10*$i}]% -anchor n
	}
	
	for {set i 1} {$i <= 10} {incr i} {
    		set y [expr {50 + ($i*50)}]
    		$c create line 45 $y 50 $y -width 2
    		$c create text 35 $y -text [expr {$i*10}]% -anchor e 
	}
	#create raster rectangle
	$c create rectangle 50 50 550 550 -fill green
	#show sample frame
	exec cat $filename | grep "SAMPLINGFRAME " | cut -f2 -d\  > $env(TMP).tmp
	set sf_x [exec cat $env(TMP).tmp | cut -f1 -d\| ]
	set sf_y [exec cat $env(TMP).tmp | cut -f2 -d\| ]
	set sf_rl [exec cat $env(TMP).tmp | cut -f3 -d\| ]
	set sf_cl [exec cat $env(TMP).tmp | cut -f4 -d\| ]
	$c create rectangle [expr int(50 + ($sf_x * 500))] [expr int(50 + ($sf_y * 500))] [expr int(50 + (($sf_x + $sf_cl)* 500))] [expr int(50 + (($sf_y + $sf_rl)* 500))]  -fill red
	
	frame .load.info
	pack .load.info
	#show sample areas
	if { ![catch {exec cat $filename | grep "MASKEDOVERLAYAREA "}] } then {
		#configuration only for RASTERMAP file
		set rname [exec cat $filename | grep "RASTERMAP " | cut -f2 -d\ ]
		label .load.info.l1 -text "The sampling areas are defined only for $rname file"
		pack .load.info.l1
		set vname [exec cat $filename | grep "VECTORMAP " | cut -f2 -d\ ]
		label .load.info.l2 -text "The overlayed vector file is $vname"
		pack .load.info.l2
	} else {
		#extract sample areas
		exec  cat $filename | grep "AREA " | cut -f2 -d " " > $env(TMP).tmp 
		set num [exec cat $env(TMP).tmp | grep -c ""]
		set ind 1
		while { $ind <= $num } {
			set x [exec head -n $ind $env(TMP).tmp | tail -n 1 | cut -f1 -d\| ]
			set y [exec head -n $ind $env(TMP).tmp | tail -n 1 | cut -f2 -d\| ]
			set rl [exec head -n $ind $env(TMP).tmp | tail -n 1 | cut -f3 -d\| ]
			set cl [exec head -n $ind  $env(TMP).tmp | tail -n 1 | cut -f4 -d\| ]
			if { $x == -1 } then {
				#disposition at runtime
				set dis [exec tail -n 1 $filename | cut -f1 -d\ ]
				set par [exec tail -n 1 $filename | cut -f2 -d\ ]
				set disposition ""
				switch $dis {
					RANDOMNONOVERLAPPING {
						set disposition "Random non overlapping"
					}
					MOVINGWINDOW {
						set disposition "Moving Windows"
						set par ""
					}
					SYSTEMATICCONTIGUOUS {
						set disposition "Systematic contiguous"
						set par ""
					}
					SYSTEMATICNONCONTIGUOUS {
						set disposition "Systematic non contiguous"
					}
					STRATIFIEDRANDOM {
						set disposition "Stratified random"
					}
				}
				label .load.info.l4 -text "Disposition $disposition $par"
				pack .load.info.l4
				set x $sf_x
				set y $sf_y
			}
			$c create rectangle [expr int(50 + ($x * 500))] [expr int(50 + ($y * 500))] [expr int(50 + (($x + $cl)* 500))] [expr int(50 + (($y + $rl)* 500))]  -fill blue
			incr ind
		}
	}
	#show legend
	frame .load.legend
	pack .load.legend
	
	file delete $env(TMP).tmp
	canvas .load.legend.c -relief raised -width 600 -height 150
	set c2 .load.legend.c
	pack $c2 
	$c2 create rectangle 25 10 35 20 -fill green
	$c2 create rectangle 25 30 35 40 -fill red
	$c2 create rectangle 25 50 35 60 -fill blue
	
	$c2 create text 45 15 -text "raster map" -anchor w 
	$c2 create text 45 35 -text "sample frame" -anchor w 
	$c2 create text 45 55 -text "sample areas" -anchor w 
}
