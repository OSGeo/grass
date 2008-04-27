################################################################################
#   
#       FILE:       rules.tcl
#   
#       PURPOSE:    Permits interactive rule entry for r.reclass and r.recode
#  
#       AUTHOR:     Michael Barton, Arizona State University
#       COPYRIGHT:  (C) 2007 by the GRASS Development Team
#                   This program is free software under the GNU General Public
#                   License (>=v2). Read the file COPYING that comes with GRASS
#                   for details.
#
#
################################################################################




namespace eval GmRules {
    variable inmap
    variable outmap
    variable rules
    variable overwrite
	global env

}
# G_msg.tcl should be sourced first for internationalized strings.


###############################################################################

# select input map
proc GmRules::select_map { seltype } {

    set m [GSelect $seltype title [G_msg "Select input map"] parent "."]
    if { $m != "" } {
        set GmRules::inmap $m
    }
}

###############################################################################

#Create main panel for interactie rules entry
proc GmRules::main { cmd } {
    variable inmap
    variable outmap
    variable rules
    variable overwrite
	global env
	global iconpath
	global bgcolor
		
	#initialize variables
	set inmap ""
	set outmap ""
	set rules ""
	set overwrite 0
	
	switch $cmd {
        "r.colors" {
            set label1 "Create new color table using color rules"
            set label2 "Raster map:"
            set label3 None
            set label4 "Enter color rules"
            set btn_icon "element-cell.gif"
            set seltype "cell"
        }
        "r.reclass" {
            set label1 "Reclassify raster map using rules"
            set label2 "Map to reclassify: \t"
            set label3 "Reclassified map: \t"
            set label4 "Enter reclassification rules"
            set btn_icon "element-cell.gif"
            set seltype "cell"
        }
        "r.recode" {
            set label1 "Recode raster map using rules"
            set label2 "Map to recode:"
            set label3 "Recoded map:"
            set label4 "Enter recoding rules"
            set btn_icon "element-cell.gif"
            set seltype "cell"
        }
        "v.reclass" {
            set label1 "Reclassify vector map using SQL rules"
            set label2 "Map to reclassify:"
            set label3 "Reclassified map:"
            set label4 "Enter reclassification rules"
            set btn_icon "element-vector.gif"
            set seltype "vector"
        }
    }

        
	# create rules input window
	set rules_win [toplevel .rulesPopup]
	wm title $rules_win [ G_msg "Interactive rules entry" ]
	# put it in the middle of the screen
	update idletasks
	set winWidth [winfo reqwidth $rules_win]
	set winHeight [winfo reqheight $rules_win]
	set scrnWidth [winfo screenwidth $rules_win]
	set scrnHeight [winfo screenheight $rules_win]
	set x [expr ($scrnWidth - $winWidth) / 2-250]
	set y [expr ($scrnHeight  - $winHeight) / 2]
	wm geometry $rules_win +$x+$y
	wm deiconify $rules_win
        
	#create the form and buttons

    # Title
    set row [ frame $rules_win.heading ]
    Label $row.a -text [G_msg $label1] \
        -fg MediumBlue
    pack $row.a -side top -padx 5 -pady 3
    pack $row -side top -fill x -expand yes

    # input map
    set row [ frame $rules_win.input ]
    Label $row.a -text [G_msg "$label2"]
    Button $row.b -image [image create photo -file "$iconpath/$btn_icon"] \
        -highlightthickness 0 -takefocus 0 -relief raised -borderwidth 1  \
        -command "GmRules::select_map $seltype"
    Entry $row.c -width 30 -text "$inmap" \
          -textvariable GmRules::inmap
    pack $row.c $row.b $row.a -side right -padx 3 -anchor e
    pack $row -side top -fill x -expand no -padx 5
	
    if { $cmd != "r.colors" } {
        # set output file for everything exept r.colors
        set row [ frame $rules_win.output ]
        LabelEntry $row.a -label [G_msg "$label3"] \
                    -textvariable GmRules::outmap -width 30
        pack $row.a -side right -anchor e -padx 3
        pack $row -side top -fill x -expand no -padx 5
    }
    
    set row [ frame $rules_win.help ]
    Button $row.a -text [G_msg "Help"] \
            -image [image create photo -file "$iconpath/gui-help.gif"] \
            -command "spawn g.manual --q $cmd" \
            -background $bgcolor \
            -helptext [G_msg "Help"]
    pack $row.a -side right -anchor e -padx 5
    if { $cmd != "r.colors" } {
        checkbutton $row.b -variable GmRules::overwrite \
                -text [G_msg "Overwrite existing file"]
        pack $row.b -side left -anchor w -padx 5
    }
    pack $row -side top -fill x -expand no

    
    # create text widget for rules entry
    set row [ frame $rules_win.rulestxt ]
    set rules_text [text $row.a \
    	-wrap none -relief sunken  \
    	-exportselection true \
    	-height 15 -width 50 \
    	-yscrollcommand "$row.b set" \
    	-xscrollcommand "$row.c set"]
	scrollbar $row.b -relief sunken -command "$rules_text yview"
	scrollbar $row.c -relief sunken -command "$rules_text xview" \
	    -orient horizontal
    pack $row.c -side bottom -fill x -expand no
    pack $row.a -side left -fill both -expand yes 
    pack $row.b -side left -fill y -expand no 
    pack $row -side top -expand yes -fill both -pady 3 -padx 5
    
    set row [ frame $rules_win.buttons ]
    Button $row.a -text [G_msg "OK"] -width 8 -bd 1 \
    	-command "GmRules::process_rules $cmd $rules_text 1" 
    Button $row.b -text [G_msg "Cancel"] -width 8 -bd 1 \
    	-command "destroy .rulesPopup"
    Button $row.c -text [G_msg "Apply"] -width 8 -bd 1 \
    	-command "GmRules::process_rules $cmd $rules_text 0"
    pack $row.a $row.b $row.c -side right -padx 5
    pack $row -side bottom -pady 3 -padx 5 -expand 0 -fill none -anchor e
    
    bind Text <Control-c> {tk_textCopy %W}
    bind Text <Control-v> {tk_textPaste %W}

}

###############################################################################
# send rules to command
proc GmRules::process_rules { cmd w quit } {
    variable inmap
    variable outmap
    variable overwrite
    global devnull

    if { $inmap == ""} {
        tk_messageBox -type ok -icon warning -parent $w \
		-message [G_msg "You must select an input map"] -title [G_msg "No input map selected"]
        return
    }
    if { $cmd != "r.colors" && $outmap == ""} {
        tk_messageBox -type ok -icon warning -parent $w \
		-message [G_msg "You must specify an output map"] -title [G_msg "No output map specified"]
        return
    }    
    
    # make tempfile to store rules for input into command
    set rulespid [pid]
	
	if {[catch {set rulesfile [exec g.tempfile pid=$rulespid]} error]} {
		GmLib::errmsg $error [G_msg "Error creating tempfile"]
	}

    # get rules from text widget
    set rules [$w get 1.0 end]
    set rules [string trim $rules]
    if { $cmd == "r.recode" } {
        set rules "$rules\n"
     }
    
    # save rules to tempfile
    catch {set output [open $rulesfile w ]}
        puts $output $rules
	if {[catch {close $output} error]} {
		GmLib::errmsg $error [G_msg "Error creating rules file"]
		return
	}
	
	set options {}
	
	if { $cmd == "r.colors"} {
	    lappend options "map=$inmap"
	} else {
	    lappend options "input=$inmap"
	}

    lappend options "rules=$rulesfile"
    
    if { $cmd != "r.colors"} {
        lappend options "output=$outmap"
    }

	# set overwrite flage
    if { $overwrite == 1} {
        lappend options "--o"
    } 

    
    if {[catch {eval [list exec -- $cmd] $options} error]} {
		tk_messageBox -type ok -icon error -message [G_msg $error]
	}
    

    # delete rules file and close popup window when finished
    
    if { $quit == 1 } {
        file delete $rulesfile
        destroy .rulesPopup
    }


}

