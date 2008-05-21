#=====================================================================================
#   
#            FILE:  file_option.tcl
#   
#     DESCRIPTION:  creates location from georeferenced file
#  
#           NOTES:  ---
#          AUTHOR:  Michael Barton
#         COMPANY:  Arizona State University
#       COPYRIGHT:  Copyright (C) 2007 Michael Barton and GRASS Development Team
#         VERSION:  1.2
#         CREATED:  23/04/2006
#        REVISION:  --- 
#       CHANGELOG:  1.0.1 08/12/2006 - Fixed directory choosing dialogs. Maris Nartiss.
#			     :	1.2 - 6 Jan 2007 - Fixed file creation for windows and reformatted
#					dialog widgets (Michael Barton).
#				 	Added check for return status of g.proj to catch failed location 
#					creation (by Maris Nartiss).
#=====================================================================================
#
#
# 
#  This library is free software; you can redistribute it and/or 
#  modify it under the terms of the GNU Library General Public 
#  License as published by the Free Software Foundation; either 
#  version 2 of the License, or (at your option) any later version. 
#
#  This library is distributed in the hope that it will be useful, 
#  but WITHOUT ANY WARRANTY; without even the implied warranty of 
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
#  Library General Public License for more details. 
#
#  You should have received a copy of the GNU Library General Public 
#  License along with this library; if not, write to the Free 
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 
#  USA 
#
#1. Redistributions of source code must retain the above copyright   
#   notice, this list of conditions and the following disclaimer.   
#2. Redistributions in binary form must reproduce the above copyright   
#   notice, this list of conditions and the following disclaimer in the   
#   documentation and/or other materials provided with the distribution.   
# 
#############################################################################
namespace eval fileOpt {
	variable fileLocation ;#name of new location to be created
	variable filepath  ;#path to georeferenced file
	global env
	global database 
	global mingw ;#test to see if we are running a windows version in mingw
	global refresh
}


# G_msg.tcl should be sourced first for internationalized strings.

# the frame used to set parameters 
proc fileOpt::fileLocCom args {
	#vars declaration
	variable filepath
	variable fileLocation
	global database
	global env
	
	set fileLocation "newLocation"
	set filepath ""
	set locpath $database
	set buttonstate "disabled"
	
	# creation of the parameter window
	set file_win [toplevel .fileloc]
	wm title $file_win [ G_msg "Define location using projection information in georeferenced file" ] 
	
	# put it in the middle of the screen
	update idletasks
	set winWidth [winfo reqwidth $file_win]
	set winHeight [winfo reqheight $file_win]
	set scrnWidth [winfo screenwidth $file_win]
	set scrnHeight [winfo screenheight $file_win]
	set x [expr ($scrnWidth - $winWidth) / 2-250]
	set y [expr ($scrnHeight  - $winHeight) / 2]
	wm geometry $file_win +$x+$y
	wm deiconify $file_win
	
	set row1 [frame $file_win.row1]
	set row2 [frame $file_win.row2]
	set row3 [frame $file_win.row3]
	set row4 [frame $file_win.row4]


	#create the form and buttons
	LabelEntry $row1.newloc -label [G_msg "Name of new location"] \
		-labeljustify right -labelanchor e -labelwidth 30 -wraplength 200 \
		-textvariable fileOpt::fileLocation -width 35 \
		-helptext [G_msg "Enter name of location to be created"]
		
	pack $row1.newloc -side left -expand 0 -fill x -padx 2

	LabelEntry $row2.filepath -label [G_msg "Path to georeferenced file"] \
		-labeljustify right -labelanchor e -labelwidth 30 -wraplength 200 \
		-textvariable fileOpt::filepath  -width 35 \
		-helptext [G_msg "Path to georeferenced file (format must be readable by GDAL/OGR)"]
		
	#browse for georeferenced file
	Button $row2.browsefile -justify center -padx 10 -bd 1 -text [G_msg "Browse..."] \
		-helptext [G_msg "Browse to locate georeferenced file"] \
		-command "fileOpt::browse_file"
		
	pack $row2.filepath $row2.browsefile -side left -expand 0 -fill x -padx 2

	Button $row3.submit -justify center -padx 10 -text [G_msg "Define location"] \
		-command "fileOpt::def_loc" -bd 1
				
	Button $row3.cancel -justify center -padx 10 -text [G_msg "Cancel"] \
		-command {destroy .fileloc} -bd 1
		
	pack $row3.submit -side left -fill x -expand 0
	pack $row3.cancel -side right -fill x -expand 0
	
	pack $row1 $row2 $row3 -side top -fill both -expand 1 -padx 3 -pady 3

}

proc fileOpt::browse_file {} {
	global env
	variable filepath

	if { [info exists env(HOME)] } {
		set dir $env(HOME)
		set fileOpt::filepath [tk_getOpenFile -parent .fileloc -initialdir $dir \
			-title [ G_msg "Choose georeferenced file" ] -multiple false]
	} else {
		set fileOpt::filepath [tk_getOpenFile -parent .fileloc \
			-title [ G_msg "Choose georeferenced file" ] -multiple false]
	}
	
}


proc fileOpt::def_loc { } {
# define new location using georeferenced file readable by GDAL/OGR
	#vars declaration
	variable filepath
	variable fileLocation
	global database
	global env	

	if {$filepath==""} {return}

	if {$filepath==""} {
		tk_messageBox -type ok -icon error \
			-message [G_msg "WARNING: Please supply a\nvalid georeferenced file"] 
		return
	} 

        set fileLocation [ string trim $fileLocation ]

	if {[file exists $fileLocation ]== 1} {
		tk_messageBox -type ok -icon error \
			-message [G_msg "WARNING: The location '$fileLocation'\nalready exists, please try another name"] 
		set fileLocation ""
		return
	}

	if {[file exists $fileLocation ]==0} {  
		destroy .fileloc
		fileOpt::create_loc
		set refresh 1
		return
	}
}

proc fileOpt::create_loc { } {
# Create a new location using g.proj
# original bash code by M. Neteler
	variable filepath
	variable fileLocation
	global location
	global mapset

	set dtrans ""
	catch {set dtrans [exec g.proj --q -c location=$fileLocation georef=$filepath datumtrans=-1]} errMsg
	
	if {[lindex $::errorCode 0] eq "CHILDSTATUS"} {
		DialogGen .wrnDlg [G_msg "Error creating location!"] error \
		[format [G_msg "g.proj returned the following message:\n%s"] $errMsg] \
		0 OK
	} elseif {$dtrans eq ""} {
		# if nothing written to stdout, there was no choice of
		# datum parameters and we need not do anything more   
	
 		if {$errMsg ne ""} {
		    DialogGen .wrnDlg [G_msg "Informational output from g.proj"] info \
		    [format [G_msg "g.proj returned the following informational message:\n%s"] $errMsg] \
		    0 OK
	        }
		set location $fileLocation
		set mapset "PERMANENT"
	} else {
		# user selects datum transform
		#create dialog that lists datum transforms, asks user to enter a number and press OK
		set paramset [fileOpt::sel_dtrans $dtrans]

		# operation canceled
		if {$paramset == -9} {return}
		
		# create new location from georeferenced file
		catch {exec g.proj --q -c georef=$filepath location=$fileLocation datumtrans=$paramset} errMsg
	 	 
		#catch any other errors 
		if {[lindex $::errorCode 0] eq "CHILDSTATUS"} {
		    DialogGen .wrnDlg [G_msg "Error creating location!"] warning \
		    [format [G_msg "g.proj returned the following message:\n%s"] $errMsg] \
		    0 OK
		} else {
 		    if {$errMsg ne ""} {
		        DialogGen .wrnDlg [G_msg "Informational output from g.proj"] info \
		        [format [G_msg "g.proj returned the following informational message:\n%s"] $errMsg] \
		        0 OK
	            }
		    set location $fileLocation
		    set mapset "PERMANENT"
		}
	}

	return

}

proc fileOpt::sel_dtrans {dtrans} {

# Dialog for selecting optional datum transform parameters
# Argument is stdout from g.proj
	
    # default is not to specify datum transformation
    set fileOpt::dtnum 0

    # Create a popup search dialog
    toplevel .dtrans_sel
    wm title .dtrans_sel [G_msg "Select datum transformation parameters:"]
    set row1 [frame .dtrans_sel.frame1] 
    set row3 [frame .dtrans_sel.frame3] 

    radiobutton $row1.0 -value 0 -variable fileOpt::dtnum -wraplength 640 -justify left -text [G_msg "Continue without specifying parameters - if used when creating a location, other GRASS modules will use the \"default\" (likely non-optimum) parameters for this datum if necessary in the future."]
    pack $row1.0 -anchor w

    set dtrans [split $dtrans "\n"]
    for {set i 0} { $i < [llength $dtrans] } {incr i} {
        set thisnum [lindex $dtrans $i]
        if {$thisnum == "---"} {
	    continue
        }
	set thisdesc $thisnum.
	while { [incr i] < [llength $dtrans] && [lindex $dtrans $i] != "---"} {
	    set thisdesc ${thisdesc}\n[lindex $dtrans $i]
	}
	radiobutton $row1.$thisnum -variable fileOpt::dtnum -value $thisnum -wraplength 640 -justify left -text $thisdesc
	pack $row1.$thisnum -anchor w
    }
    
    pack $row1
        
    Button $row3.ok -text [G_msg "OK"] -padx 10 -bd 1 \
    	-command "destroy .dtrans_sel"
    pack $row3.ok -side left -padx 3
    button $row3.cancel -text [G_msg "Cancel"] -padx 10 -bd 1 \
    	-command "set fileOpt::dtnum -9; destroy .dtrans_sel"
    pack $row3.cancel -side left -padx 3
    pack $row3 -anchor center -pady 3
    
    tkwait window .dtrans_sel
    return $fileOpt::dtnum

}
