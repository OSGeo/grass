###############################################################
# mapprint.tcl - GRASS GIS Manager procedures for postscript and
# lpr printing, and pdf and eps output
# January 2006 Michael Barton, Arizona State University
# COPYRIGHT:	(C) 1999 - 2006 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
##########################################################################


namespace eval psprint {
	variable docht
	variable docwd
	variable epsfile
	variable format
	variable gsdevices
	variable gsexists
	variable gspresent
	variable gsstate
	variable ldevice
	variable mbottom
	variable mleft
	variable mright
	variable mtop
	variable orient
	variable paper
	variable paper_preset
	variable pdffile
	variable pght
	variable pgwd
	variable printer
	variable printmode
	variable res
	variable tmppngfile
	variable tmpppmfile
	variable tmppsfile 
	variable tmpscript 
	variable PPap 
	variable PVar 
	variable PView
	variable PWid 
	global array can # mon
}


#initialize variables
proc psprint::init { } {
	variable pgwd
	variable pght
	variable docwd
	variable docht
	variable paper
	variable paper_preset
	variable printmode
	variable printer
	variable gsexists
	variable orient
	variable mleft
	variable mright
	variable mtop
	variable mbottom
	variable gsstate
	variable ldevice
	variable gsdevices
	variable res
	global mon
	global mingw

	set psprint::pgwd 8.5
	set psprint::pght 11
	set psprint::docwd 7.5
	set psprint::docht 10
	set psprint::paper "preset"
	set psprint::paper_preset "letter"
	set psprint::printer ""
	set psprint::gsexists 1
	set psprint::orient "landscape"
	set psprint::res 300
	set psprint::mleft 1
	set psprint::mright 1
	set psprint::mtop 1
	set psprint::mbottom 1
	
	# default is lpr printing
	set printmode "lpr"

	# check for ghostscript	
	# switch to native windows version of ghostscript if runing wingrass
	if { $mingw == 1 } {
		set cmd "gswin32c"
	} else {
		set cmd "gs"
	}

	#enable additional printing options if Ghostscript available
	if {![catch {set input [exec $cmd -help]} error]} {
		regexp ".*Available devices:(.*)Search path:" $input string gsdevices 
		set gsstate "normal"
		regsub -all {   } $gsdevices { } gsdevices
		regsub -all { } $gsdevices \n gsdevices
		regsub -all \n\n $gsdevices \n gsdevices
	} else {
		set gsdevices "none available"
		set gsstate "disabled"
		#set printmode "eps"
		tk_messageBox -type ok -icon error -message [G_msg "Ghostscript not available"]
	}
}	

# calculate paper size and document size on paper (all in inches)  
proc psprint::paper { } {
	variable paper
	variable paper_preset
	variable printmode
	variable pgwd
	variable pght
	variable mleft
	variable mright
	variable mtop
	variable mbottom
	variable docwd
	variable docht
	variable orient
    
	# set paper dimensions
	if { $paper == "preset" } {
		switch $paper_preset {
			"11x17" {
				set pgwd 11
				set pght 17
			}
			"ledger" {
				set pgwd 17
				set pght 11
			}
			"legal" {
				set pgwd 8.5
				set pght 14
			}
			"letter" {
				set pgwd 8.5
				set pght 11
			}
			"a0" {
				set pgwd 33.0556
				set pght 46.7778
			}
			"a1" {
				set pgwd 23.3889
				set pght 33.055
			}
			"a2" {
				set pgwd 16.5278
				set pght 23.3889
			}
			"a3" {
				set pgwd 11.6944
				set pght 16.5278 
			}
			"a4" {
				set pgwd 8.26389 
				set pght 11.6944
			}
		}
	}

	set docwd [expr $pgwd - $mright - $mleft]
	set docht [expr $pght - $mtop - $mbottom]
	
	if { $orient == "landscape" && $printmode == "pdf" } {
		set docwd [expr $docwd + 2]
		set docht [expr $docht + 2]
	} else {
		set docwd [expr $docwd + 1]
		set docht [expr $docht + 1]
	}
		
	update
	
}

# initialize tmpfiles for poscript printing
proc psprint::init_tmpfiles { } {
	variable tmpscript
	variable tmppsfile
	variable tmppngfile

	# get temporary file for postscript printing
	set pid [ pid ]
	if {[catch {set tmppsfile [ exec g.tempfile pid=$pid ]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}

	append tmppsfile ".ps"
	set pid [ pid ]
    
	if {[catch {set tmppngfile [ exec g.tempfile pid=$pid ]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}
	    append tmppngfile ".png"
}

# show gs printer devices in output window
proc psprint::show_devices { } {
	variable gsdevices

	set ah [monitor_annotation_start {} "Ghostscript Output Devices" {}]
	monitor_annotate $ah $gsdevices
}


# create printer options window
proc psprint::window { cm cv cx cy } {
	variable pgwd
	variable pght
	variable paper
	variable paper_preset
	variable printmode
	variable printer
	variable gsexists
	variable epsfile
	variable pdffile
	variable orient
	variable res
	variable pgwd
	variable pght
	variable mleft
	variable mright
	variable mtop
	variable mbottom
	variable gspresent
	variable ldevice
	variable gsdevices
	variable gsstate
	global mon
	
	set mon $cm
	    
	# check if opened
	if { [winfo exists .printwin] } {
		wm deiconify .printwin
		raise .printwin
		return
	}
	
	set PW [toplevel .printwin]
	wm title $PW [G_msg "Postscript and LPR printing of map display"]
	
	# Left part paper + output
	set PWid(left) [ frame $PW.left -padx 5 -pady 5]  
	pack $PWid(left) -side left -anchor w
	
	# paper size, scale
	set PWid(paper) [ frame $PWid(left).paper]  
	pack $PWid(paper) -side top -anchor w
	
	    # preset paper sizes (from ghostscript)
	set row [ frame $PWid(paper).row1 ]
	radiobutton $row.a -variable psprint::paper -value "preset" \
		-highlightthickness 0 
	Label $row.b -anchor w -text [G_msg "Preset paper type"]
	ComboBox $row.c -label "" -width 20  -textvariable psprint::paper_preset \
		-values {"letter" "a4" "legal" "11x17" "a3" "ledger" "a0" "a1" "a2" } \
		-modifycmd psprint::paper
	pack $row.a $row.b $row.c -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	# custom paper sizes
	set row [ frame $PWid(paper).row2 ]
	radiobutton $row.a -variable psprint::paper -value "custom" \
		-highlightthickness 0
	Label $row.b -anchor w -text [G_msg "Custom paper size"]
	Label $row.c -anchor w -text [G_msg "width:"]
	Entry $row.d -width 10 -textvariable psprint::pgwd
	Label $row.e -anchor w -text [G_msg "  height:"]
	Entry $row.f -width 10 -textvariable psprint::pght 
	pack $row.a $row.b $row.c $row.d $row.e $row.f -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	#margins
	set row [ frame $PWid(paper).row3]
	Label $row.a -anchor w -text [G_msg "Margins  left:"]
	Entry $row.b -width 10 -textvariable psprint::mleft 
	Label $row.c -anchor w -text [G_msg " right:"] 
	Entry $row.d -width 10 -textvariable psprint::mright 
	Label $row.e -anchor w -text [G_msg " top:"]
	Entry $row.f -width 10 -textvariable psprint::mtop 
	Label $row.g -anchor w -text [G_msg " bottom:"]
	Entry $row.h -width 10 -textvariable psprint::mbottom 
	
	pack $row.a $row.b $row.c $row.d $row.e $row.f $row.g $row.h -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	# portrait or landscape
	set row [ frame $PWid(paper).row4 ]
		LabelEntry $row.a -label [G_msg "Resolution (dpi) for printing and PDF "] \
		-textvariable psprint::res -width 4
	Label $row.b -anchor w -text "  "
	radiobutton $row.c -variable psprint::orient -value "landscape" \
		-text "landscape mode" -highlightthickness 0
	radiobutton $row.d -variable psprint::orient -value "portrait" \
		-text "portrait mode  " -highlightthickness 0
	pack $row.a $row.b $row.c $row.d -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	# output options
	set PWid(output) [ frame $PWid(left).output ]  
	pack $PWid(output) -side top -anchor w
	
	# LPR printer
	set row [ frame $PWid(output).lpr ]
	radiobutton $row.a -variable psprint::printmode -value "lpr" \
		-highlightthickness 0
	Label $row.b -anchor w -text [G_msg "Print on LPR printer"]
	pack $row.a $row.b -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	# Postscript printer
	set row [ frame $PWid(output).psprinter ]
	radiobutton $row.a -variable psprint::printmode -value "psprint" \
		-state $psprint::gsstate -highlightthickness 0
	Label $row.b -anchor w -text [G_msg "Print on postscript device* "] \
		-state $psprint::gsstate
	ComboBox $row.c -width 20 -textvariable psprint::printer  \
		-values $psprint::gsdevices -editable 0 -entrybg white
	pack $row.a $row.b $row.c -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	# PDF file
	set row [ frame $PWid(output).pdffile]
	radiobutton $row.a -variable psprint::printmode -value "pdf" \
		-state $psprint::gsstate -highlightthickness 0 
	Label $row.b -anchor w -text [G_msg "Save to PDF file*              "]  \
		-state $psprint::gsstate 
	Entry $row.c -width 30 -textvariable psprint::pdffile  -state $gsstate
	Button $row.d -text [G_msg "Browse"]  -command { set psprint::pdffile \
		[tk_getSaveFile -title "Output PDF file" -defaultextension ".pdf"]} \
		-state $psprint::gsstate
	pack $row.a $row.b $row.c $row.d -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	# EPS file
	set row [ frame $PWid(output).epsfile ]
	radiobutton $row.a -variable psprint::printmode -value "eps" \
		-highlightthickness 0 
	Label $row.b -anchor w -text [G_msg "Save to EPS file               "] 
	Entry $row.c -width 30 -textvariable psprint::epsfile 
	Button $row.d -text [G_msg "Browse"] -command { set psprint::epsfile \
	       [ tk_getSaveFile -title "Output EPS file" -defaultextension ".eps"] }
	pack $row.a $row.b $row.c $row.d -side left;
	pack $row -side top -fill x -expand no -anchor n
	
	set row [ frame $PWid(output).gsmessage ]
	Label $row.a -anchor w -text [G_msg "*requires ghostscript to be installed and in path"]
	pack $row.a -side bottom;
	pack $row -side top -fill x -expand yes -anchor center
	
	# Buttons 
	set but [ frame $PWid(left).buttons ]  
	pack $but -side top
	
	Button $but.print -text [G_msg "Print"] -command "update; psprint::print $cv"
	Button $but.close -text [G_msg "Close"] -command { destroy .printwin }
	pack $but.print $but.close -side left 

}

proc psprint::print { cv } {
	variable paper
	variable paper_preset
	variable printmode
	variable printer
	variable gsexists
	variable res
	variable format
	variable orient
	variable epsfile
	variable pdffile
	variable tmppsfile 
	variable tmppngfile
	variable pgwd
	variable pght
	variable docwd
	variable docht
	variable mright
	variable mleft
	variable mtop
	variable mbottom
	global gmpath
	global mon
	global mingw
    
	psprint::init_tmpfiles
   	psprint::paper
   	update
	set landscape $gmpath/landscap.ps
    
	#change doc size to points
	set cdocwd [expr $docwd * 72]
	set cdocht [expr $docht * 72]
    
 	# set paper size for postscript printer and pdf files
	set w [expr round($pgwd * $res)]
	set h [expr round($pght * $res)]
	set format "-g$w"
	append format "x$h"
	
	# switch to native windows version of ghostscript if runing wingrass
	if { $mingw == 1 } {
		set cmd "gswin32c"
	} else {
		set cmd "gs"
	}
		
		
   
	if { $orient == "portrait" } {
		$cv postscript -pageheight $cdocht -pagewidth $cdocwd \
			-file $tmppsfile
	} else {
		$cv postscript -rotate 1 -pageheight $cdocht -pagewidth $cdocwd \
			-file $tmppsfile			
	}	
	
	
	after 500

	# lpr printing		
 	if { $printmode == "lpr" } {
 		if {[catch {exec lpr -o position=center $tmppsfile } error]} {
			GmLib::errmsg $error
		}
	}

	# postsript printing via ghostsript
	if { $printmode == "psprint" && $printer != "" } {
		if {[catch {exec $cmd  $format -sDEVICE#$printer -r$res -sNOPAUSE -dBATCH -- $tmppsfile} error]} {
			GmLib::errmsg $error
		}
		 
	}

	# output to pdf file via ghostscript	
	if { $printmode == "pdf" && $pdffile != "" } {
		if {[catch {exec $cmd  $format -sDEVICE#pdfwrite -r$res -sNOPAUSE -sOutputFile#$pdffile -dBATCH -- $tmppsfile} error]} {
			GmLib::errmsg $error
		}
		 
	}

	# output to eps file
	if { $printmode == "eps" && $epsfile != "" } {
		if { $orient == "portrait" } {
			$cv postscript -file "$epsfile"
		} else {
			$cv postscript -file "$epsfile" -rotate 1
		}
	}
	
	psprint::clean
}


proc psprint::set_option { key value } {
	variable PWid 
	variable PVar 
	variable PPap 
	variable PView
    
	set PVar($key) $value

}

# Delete temporary files
proc psprint::clean {  } {
	variable tmppsfile
	variable tmppngfile
    
	catch {file delete $tmppsfile}
	catch {file delete $tmppngfile}

}

