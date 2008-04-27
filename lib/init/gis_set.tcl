#############################################################################
#
# $Id$
#
# MODULE:   	Grass Tcl/Tk Initialization
# AUTHOR(S):	Original author unknown - probably CERL
#   	    	Justin Hickey - Thailand - jhickey hpcc.nectec.or.th
#   	    	Markus Neteler - Germany - neteler geog.uni-hannover.de, itc.it
#				Michael Barton - USA - Arizona State University
#               Maris Nartiss - Latvia - maris.gis gmail.com
# PURPOSE:  	The source file for this shell script is in
#   	    	src/tcltkgrass/main/gis_set.tcl. It allows the user to choose
#   	    	the database, location, and mapset to use with grass by
#   	    	presenting a user interface window.
# COPYRIGHT:    (C) 2000,2006 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#   	    	License (>=v2). Read the file COPYING that comes with GRASS
#   	    	for details.
#
#############################################################################

if {[info exists env(OS)] && $env(OS) == "Windows_NT"} {
	set mingw "1"
} else {
	set mingw "0"
}

source $env(GISBASE)/etc/gtcltk/gmsg.tcl
source $env(GISBASE)/etc/gtcltk/options.tcl
source $env(GISBASE)/etc/epsg_option.tcl
source $env(GISBASE)/etc/file_option.tcl

#fetch GRASS Version number:
set fp [open $env(GISBASE)/etc/VERSIONNUMBER r]
set GRASSVERSION [read -nonewline $fp]
close $fp

#############################################################################

proc searchGISRC { filename } {
 
  global database
  global location
  global mapset
  global oldDb 
  global oldLoc 
  global oldMap
  global grassrc_list
  
  set grassrc_list ""

  set flag 0
  if { [file exists $filename] } {
      set ifp [open $filename "r"]
      set thisline [gets $ifp]
      while { [eof $ifp] == 0 } {

            lappend grassrc_list "$thisline"

	    if { [regexp -- {^GISDBASE: *(.*)$} $thisline dummy env_database] } {
                set database $env_database
            }
            if { [scan $thisline "LOCATION_NAME: %s" env_location] } {
                set location $env_location
            }
            if { [scan $thisline "MAPSET: %s" env_mapset] } {
                set mapset $env_mapset
            }
            set thisline [gets $ifp]
      }
      
      set oldDb $database
      set oldLoc $location
      set oldMap $mapset
      
      close $ifp
      if { $database != "" && $location != "" && $mapset != "" } {
         set flag 1
      }
  }
  return $flag
}

#############################################################################

proc putGRASSRC { filename } {
 	# create grassrc file with new values
	global database
	global location
	global mapset
	global grassrc_list
	
	set ofp [open $filename "w"]

	foreach i $grassrc_list {
		if { [regexp {^GISDBASE:} $i] } {
			puts $ofp "GISDBASE: $database"
		} elseif { [regexp {^LOCATION_NAME:} $i] } {
			puts $ofp "LOCATION_NAME: $location"
		} elseif { [regexp {^MAPSET:} $i] } {
			puts $ofp "MAPSET: $mapset"
		} else {
			puts $ofp $i
		}
	}

        if { [ catch { close $ofp } error ] } {
                DialogGen .wrnDlg [G_msg "WARNING: can not save"] warning \
                        [format [G_msg "Warning: unable to save data to <%s> file.\nError message: %s"] \
                        $filename $error] \
                0 OK;
      }
}


#############################################################################

proc CheckLocation {} {
	# Returns 0, if location is invalid, 1 othervise.
	# Are hardcoded / in path's OK? They where here before me :) Maris.
    global database location
    
    set found 0
    set dir $database
    append dir "/$location"
    set currDir [pwd]

    # Special case - wrong GISDBASE
    if {[file isdirectory $dir] == 0} {
        DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
		[format [G_msg "Warning: location <%s> at GISDBASE <%s> is not a directory or does not exist."] \
		$location $database] \
        0 OK;
        .frame0.frameMS.listbox delete 0 end
        .frame0.frameNMS.second.entry configure -state disabled
        .frame0.frameBUTTONS.ok configure -state disabled
    } else {
        cdir $dir
        .frame0.frameNMS.second.entry configure -state disabled
        foreach filename [lsort [glob -nocomplain *]] {
            if {[string compare $filename "PERMANENT"] == 0} {
                # All good locations have valid PERMANENT mapset.
                if {[file exists "$dir/PERMANENT/DEFAULT_WIND"] != 0} {
                    set found 1
                    .frame0.frameNMS.second.entry configure -state normal
                }
            }
        }
    }
    
    cdir $currDir
    return $found
}


#############################################################################
proc gisSetWindow {} {
# create main GRASS startup panel
    global GRASSVERSION
    global database
    global location
    global mymapset
    global mapset
    global oldDb oldLoc oldMap
    global env
    global grassrc_list
    global gisrc_name
    global refresh
    
    set refresh 0

    global mingw

    # Window manager configurations

    wm title . [format [G_msg "GRASS %s Startup"] $GRASSVERSION]

    # ---------------------------
    # build .frame0 with panel title
    # ---------------------------
    set mainfr [frame .frame0 \
    	-borderwidth {2} \
    	-relief {raised}]

    set titlefrm [frame .frame0.intro -borderwidth 2 ]
    set introimg  [label $titlefrm.img -image [image create photo -file \
    	"$env(GISBASE)/etc/gintro.gif"]]
    set introtitle [text $titlefrm.msg -height 5 \
    	-relief flat -fg darkgreen \
    	-bg #dddddd \
    	-font introfont \
    	-width 50 ]

    pack $titlefrm -side top
	pack $introimg -side top
    pack $introtitle -side top

    .frame0.intro.msg tag configure all -justify center
    .frame0.intro.msg insert end [G_msg "Welcome to GRASS GIS Version"]
    .frame0.intro.msg insert end [G_msg " $GRASSVERSION\n"]
    .frame0.intro.msg insert end [G_msg "The world's leading open source GIS\n\n"]
    .frame0.intro.msg insert end [G_msg "Select an existing project location and mapset\n"]
    .frame0.intro.msg insert end [G_msg "or define a new location\n"]
    .frame0.intro.msg tag add all 1.0 end
    .frame0.intro.msg configure -state disabled

    # -----------------------------------
    # build .frame0.frameDB - panel top section for database selection
    # -----------------------------------

    frame .frame0.frameDB \
    	-borderwidth {2}

    frame .frame0.frameDB.left \
    	-borderwidth {2}

    frame .frame0.frameDB.mid \
    	-borderwidth {2}

    frame .frame0.frameDB.right \
    	-borderwidth {2}

    label .frame0.frameDB.left.label \
    	-justify right \
    	-wraplength 200 \
    	-text [G_msg "GIS Data Directory: "]

    entry .frame0.frameDB.mid.entry \
    	-relief {sunken} \
    	-textvariable database \
		-width 40 \
    	-xscrollcommand { .frame0.frameDB.mid.hscrollbar set}
    
    scrollbar .frame0.frameDB.mid.hscrollbar \
    	-command { .frame0.frameDB.mid.entry xview} \
    	-relief {sunken} \
    	-orient {horizontal}
 
	button .frame0.frameDB.right.button \
		-text [G_msg "Browse..."] -padx 10 -bd 1 \
		-command { set tmp [tk_chooseDirectory -initialdir $database \
			-parent .frame0 -title [G_msg "New GIS data directory"] -mustexist true]
			if {$tmp != ""} {
				set database $tmp
				refresh_loc
				.frame0.frameBUTTONS.ok configure -state disabled } 
			}

    pack .frame0.frameDB.left.label -side top
    pack .frame0.frameDB.mid.entry -side top
    pack .frame0.frameDB.mid.hscrollbar -side bottom -fill x
    pack .frame0.frameDB.right.button -side left
    pack .frame0.frameDB.left -side left -anchor n
    pack .frame0.frameDB.mid -side left -anchor n
    pack .frame0.frameDB.right -side left -anchor n -padx 10

    # -----------------------------------
    # build .frame0.frameLOC - middle, left section for location selection listbox 
    # -----------------------------------
    frame .frame0.frameLOC \
    	-borderwidth {2}

    label .frame0.frameLOC.label \
	-wraplength 170 \
    	-text [G_msg "Project Location (projection/coordinate system)"] 

    listbox .frame0.frameLOC.listbox \
    	-relief {sunken} \
    	-exportselection false \
    	-yscrollcommand {.frame0.frameLOC.vscrollbar set} \
    	-xscrollcommand {.frame0.frameLOC.hscrollbar set} \
    	-selectmode single

    scrollbar .frame0.frameLOC.vscrollbar \
    	-command {.frame0.frameLOC.listbox yview} \
    	-relief {sunken}

    scrollbar .frame0.frameLOC.hscrollbar \
    	-command {.frame0.frameLOC.listbox xview} \
    	-orient {horizontal} \
    	-relief {sunken}

    pack append .frame0.frameLOC \
    	.frame0.frameLOC.label { top fill } \
    	.frame0.frameLOC.vscrollbar { right filly } \
    	.frame0.frameLOC.hscrollbar { bottom fillx } \
    	.frame0.frameLOC.listbox { left expand fill }


    # -----------------------------------
    # build .frame0.frameMS - middle, right section for mapset selection listbox
    # -----------------------------------
    frame .frame0.frameMS \
    	-borderwidth {2}

    label .frame0.frameMS.label \
	-wraplength 170 \
    	-text [G_msg "Accessible Mapsets (directories of GIS files)"] 

    listbox .frame0.frameMS.listbox \
    	-relief {sunken} \
	-exportselection false \
    	-yscrollcommand {.frame0.frameMS.vscrollbar set} \
    	-xscrollcommand {.frame0.frameMS.hscrollbar set} \
    	-selectmode single

    scrollbar .frame0.frameMS.vscrollbar \
    	-command {.frame0.frameMS.listbox yview} \
    	-relief {sunken}

    scrollbar .frame0.frameMS.hscrollbar \
    	-command {.frame0.frameMS.listbox xview} \
    	-orient {horizontal} \
    	-relief {sunken}

    pack append .frame0.frameMS \
    	.frame0.frameMS.label { top fill } \
    	.frame0.frameMS.vscrollbar { right filly } \
    	.frame0.frameMS.hscrollbar { bottom fillx } \
    	.frame0.frameMS.listbox { left expand fill }

    # -----------------------------------
    # build .frame0.frameNMS - middle far right section with buttons for
    #    creating new mapset and location
    # -----------------------------------
    frame .frame0.frameNMS \
    	-borderwidth {2}

    frame .frame0.frameNMS.first \
    	-borderwidth {2}

    frame .frame0.frameNMS.second \
    	-borderwidth {2}

    frame .frame0.frameNMS.third \
    	-borderwidth {2}

    frame .frame0.frameNMS.spacer \
    	-borderwidth {2} -height {10}

    frame .frame0.frameNMS.fourth \
    	-borderwidth {2}

    frame .frame0.frameNMS.fifth \
    	-borderwidth {2}

    frame .frame0.frameNMS.sixth \
    	-borderwidth {2}

    frame .frame0.frameNMS.seventh \
    	-borderwidth {2}

    label .frame0.frameNMS.first.label \
	-wraplength 200 \
    	-text [G_msg "Create new mapset in selected location"]

    entry .frame0.frameNMS.second.entry \
    	-relief {sunken} \
    	-textvariable mymapset \
    	-width 22 
	
    button .frame0.frameNMS.third.button \
    	-text [G_msg "Create new mapset"] \
    	-padx 10 -bd 1 -wraplength 150 \
     	-command { 
     	    set mymapset [ string trim $mymapset ]
     	    if { [file exists $mymapset] } {
			DialogGen .wrnDlg [G_msg "WARNING: invalid mapset name"] warning \
			[format [G_msg "Warning: Mapset with name <%s> already exists. \nNew mapset is NOT created. \nChoose different mapset name and try again."] $mymapset] \
			0 OK;
			return
     	    }
            .frame0.frameNMS.third.button configure -state disabled
	    if { $mymapset != "" } {
            	if {[CheckLocation] == 0} {
            	    DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
		    		[format [G_msg "Warning: selected location <%s> is not valid. \n New mapset is NOT created. \n Select valid location and try again."] $location] \
                    0 OK;
                    set mapset ""
            	} else {
                    cdir $database
                    cdir $location
                    if { [ catch { file mkdir $mymapset } error ] } {
                          DialogGen .wrnDlg [G_msg "WARNING: unable to mkdir"] warning \
                                      [format [G_msg "Warning: Unable to create directory for new mapset. \nError message: %s"] $error] \
                          0 OK;
                    } else {
                    #generate default DB definition, create dbf subdirectory:
                    set varfp [open $mymapset/VAR "w"]
                    puts $varfp "DB_DRIVER: dbf"
                    puts $varfp "DB_DATABASE: \$GISDBASE/\$LOCATION_NAME/\$MAPSET/dbf/"
                    close $varfp
                    catch {file attributes $mymapset/VAR -permissions u+rw,go+r}
                    file mkdir $mymapset/dbf
                    #copy over the WIND definition:
                    catch {file copy $mymapset/../PERMANENT/DEFAULT_WIND $mymapset/WIND}
                    catch {file attributes $mymapset/WIND -permissions u+rw,go+r}
                    .frame0.frameMS.listbox insert end $mymapset
                    selFromList .frame0.frameMS.listbox $mymapset
                    set mapset $mymapset
                    .frame0.frameNMS.second.entry delete 0 end
                    .frame0.frameBUTTONS.ok configure -state normal
                    }
                }
            }
	}

    label .frame0.frameNMS.fourth.label \
	-wraplength 200 \
    	-text [G_msg "Define new location with..."]


    button .frame0.frameNMS.fifth.button \
    	-text [G_msg "Georeferenced file"] \
    	-width 22 -bd 1 -wraplength 150\
    	-relief raised \
    	-command {putGRASSRC $gisrc_name
		fileOpt::fileLocCom
    		tkwait window .fileloc
    		refresh_loc
    		refresh_ms
    		selFromList .frame0.frameLOC.listbox $location
    		selFromList .frame0.frameMS.listbox $mapset
    		.frame0.frameBUTTONS.ok configure -state normal}

    button .frame0.frameNMS.sixth.button \
    	-text [G_msg "EPSG codes"] \
    	-width 22 -bd 1 -wraplength 150\
    	-relief raised \
    	-command { putGRASSRC $gisrc_name
		if { [epsgOpt::epsgLocCom] } {
    		tkwait window .optPopup
    		refresh_loc
    		refresh_ms
    		selFromList .frame0.frameLOC.listbox $location
    		selFromList .frame0.frameMS.listbox $mapset
    		.frame0.frameBUTTONS.ok configure -state normal} }
    	    			
    button .frame0.frameNMS.seventh.button \
    	-text [G_msg "Projection values"] \
    	-width 22 -bd 1 -wraplength 150\
    	-relief raised \
    	-command {
			if { $mingw == "1" } {
				exec -- cmd.exe /c start $env(GISBASE)/etc/set_data
			} else {
				exec -- $env(GISBASE)/etc/grass-xterm-wrapper -name xterm-grass -e $env(GISBASE)/etc/grass-run.sh $env(GISBASE)/etc/set_data
			}
			# Now we should refresh the list of locations!
			refresh_loc ;# Could it look like this? Maris.
        }

    pack append .frame0.frameNMS
    pack .frame0.frameNMS.first.label
    pack .frame0.frameNMS.second.entry
    pack .frame0.frameNMS.third.button
    pack .frame0.frameNMS.fourth.label
    pack .frame0.frameNMS.fifth.button
    pack .frame0.frameNMS.sixth.button
    pack .frame0.frameNMS.seventh.button
    pack .frame0.frameNMS.first
    pack .frame0.frameNMS.second
    pack .frame0.frameNMS.third
    pack .frame0.frameNMS.spacer
    pack .frame0.frameNMS.fourth
    pack .frame0.frameNMS.fifth
    pack .frame0.frameNMS.sixth
    pack .frame0.frameNMS.seventh

    # ----------------------------------
    # build .frame0.frameBUTTONS
    # ----------------------------------
    frame .frame0.frameBUTTONS \
    	-borderwidth {2}
    
    
    button .frame0.frameBUTTONS.ok \
     	-text [G_msg "Enter GRASS"] \
    	-padx 10 -bd 1 -fg green4 -default active -wraplength 100 \
     	-command {
            if {[CheckLocation] == 0} {
				DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
				[format [G_msg "Warning: selected location <%s> is not valid. \n Select valid location and try again."] $location] \
				0 OK;
			set mapset ""
            } else {
                if {[file exists "$database/$location/$mapset/WIND"] == 0} {
                    DialogGen .wrnDlg [G_msg "WARNING: invalid mapset"] warning \
                    [format [G_msg "Warning: <%s> is not a valid mapset"] $mapset] \
                    0 OK;
                }
                if { $mapset != "" && [file exists "$database/$location/$mapset/WIND"] != 0} {
                    puts stdout "GISDBASE='$database';"
                    puts stdout "LOCATION_NAME='$location';"
                    puts stdout "MAPSET='$mapset';"
                    putGRASSRC $gisrc_name
                    exit 0
                }
            } 
        }
        
    bind . <Return> {.frame0.frameBUTTONS.ok invoke}

    button .frame0.frameBUTTONS.help \
    	-text [G_msg "Help"] \
    	-padx 10 -bd 1 -wraplength 100 \
    	-bg honeydew2 \
		-command {
			if { [winfo exists .help] } {
				 puts [G_msg "Help already opened"]
				 wm deiconify .help
				 raise .help
				 return
			}
			if { $mingw == "1" } {
				exec -- $env(GRASS_HTML_BROWSER) file://$env(GISBASE)/docs/html/helptext.html &;
			} else {
				exec -- $env(GRASS_HTML_BROWSER) file://$env(GISBASE)/docs/html/helptext.html >@stdout 2>@stderr &;
			}
        }

    button .frame0.frameBUTTONS.cancel \
    	-text [G_msg "Exit"] \
    	-padx 10 -bd 1 -wraplength 100 \
    	-command { exit 2 }


    pack append .frame0.frameBUTTONS \
    	.frame0.frameBUTTONS.ok { left  } \
    	.frame0.frameBUTTONS.cancel { left  } \
    	.frame0.frameBUTTONS.help { right  } 



    # ----------------------------------
    # packed it all
    # ----------------------------------

    frame .frame0.frameSpacer \
    	-borderwidth {2} -height {5}

    # pack widget .frame0
    pack append .frame0 \
    	.frame0.frameDB { top } \
    	.frame0.frameBUTTONS { bottom expand fill } \
    	.frame0.frameSpacer { bottom } \
    	.frame0.frameLOC { left expand  } \
    	.frame0.frameMS { left expand  } \
     	.frame0.frameNMS { left expand fill }

    .frame0.frameNMS.third.button configure -state disabled

    pack append . \
    	.frame0 { top frame center expand fill }

    .frame0.frameDB.mid.entry xview moveto 1
    
    if { ! [file exists $database] } {
      	DialogGen .wrnDlg [G_msg "WARNING: Invalid Database"] warning \
	    [G_msg "WARNING: Invalid database. Finding first valid directory in parent tree"] \
	    0 OK
      
      	while { ! [file exists $database] } {
	    	set database [file dirname $database]
      	}
    }
    
    # setting list of locations
    refresh_loc
    selFromList .frame0.frameLOC.listbox $location
    if { [CheckLocation] } {
        # setting list of mapsets
	refresh_ms
        selFromList .frame0.frameMS.listbox $mapset
	if { [.frame0.frameMS.listbox get [.frame0.frameMS.listbox curselection]] == $mapset } {
	        .frame0.frameBUTTONS.ok configure -state normal
		}
    }

	bind .frame0.frameDB.mid.entry <Return> {
        set new_path [%W get]
        if { "$new_path" != "" \
             && [file exists $new_path] && [file isdirectory $new_path] } {
           %W delete 0 end
           %W insert 0 $new_path
           cdir $new_path
           set location ""
           set mapset ""
           refresh_loc
           set database [pwd]
        }
		.frame0.frameBUTTONS.ok configure -state disabled
	}

	bind .frame0.frameLOC.listbox <Double-ButtonPress-1> {
        # Do something only if there IS atleast one location
        if {[%W size] > 0} {
            %W selection clear 0 end
            %W select set [%W nearest %y]
            cdir $database
            set location [%W get [%W nearest %y]]
            .frame0.frameMS.listbox delete 0 end
            .frame0.frameBUTTONS.ok configure -state disabled
            set mapset ""
            if {[CheckLocation] == 0} {
				# Notice - %%s prevents %s capturing by bind
				DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
		    	[format [G_msg "Warning: selected location <%%s> is not valid. \n Select valid location and try again."] $location] \
				0 OK;
            } else {
		refresh_ms
			}
		}
	}

	bind .frame0.frameLOC.listbox <ButtonPress-1> {
        # Do something only if there IS atleast one location
        if {[%W size] > 0} {
            %W selection clear 0 end
            %W select set [%W nearest %y]
            cdir $database
            set location [%W get [%W nearest %y]]
            .frame0.frameMS.listbox delete 0 end
            .frame0.frameBUTTONS.ok configure -state disabled
            set mapset ""
            if {[CheckLocation] == 0} {
				# Notice - %%s prevents %s capturing by bind
				DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
		    	[format [G_msg "Warning: selected location <%%s> is not valid. \n Select valid location and try again."] $location] \
				0 OK;
            } else {
		refresh_ms
        	}
  		}
	}

	bind .frame0.frameMS.listbox <Double-ButtonPress-1> {
        # Do something only if there IS atleast one mapset
        if {[%W size] > 0} {
            %W selection clear 0 end
            %W select set [%W nearest %y]
            set mapset [%W get [%W nearest %y]]
            .frame0.frameBUTTONS.ok configure -state normal
            if {[CheckLocation] == 0} {
				# Notice - %%s prevents %s capturing by bind
				DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
		    	[format [G_msg "Warning: selected location <%%s> is not valid. \n Select valid location and try again."] $location] \
				0 OK;
			set mapset ""
            } else {
                if {[file exists "$database/$location/$mapset/WIND"] == 0} {
                    DialogGen .wrnDlg [G_msg "WARNING: invalid mapset"] warning \
                    [format [G_msg "Warning: <%%s> is not a valid mapset"] $mapset] \
                    0 OK;
                }
                if { $mapset != "" && [file exists "$database/$location/$mapset/WIND"] != 0} {
                    puts stdout "GISDBASE='$database';"
                    puts stdout "LOCATION_NAME='$location';"
                    puts stdout "MAPSET='$mapset';"
                    putGRASSRC $gisrc_name
                    exit 0
                }
            }
        }
	}

	bind .frame0.frameMS.listbox <ButtonPress-1> {
        # Do something only if there IS atleast one mapset
        if {[%W size] > 0} {
            %W selection clear 0 end
            %W select set [%W nearest %y]
            set mapset [%W get [%W nearest %y]]
            .frame0.frameBUTTONS.ok configure -state normal
            if {[CheckLocation] == 0} {
				DialogGen .wrnDlg [G_msg "WARNING: invalid location"] warning \
		    	[format [G_msg "Warning: selected location <%%s> is not valid. \n Select valid location and try again."] $location] \
				0 OK;
			set mapset ""
            }
        }
	}

	bind .frame0.frameNMS.second.entry <KeyRelease> {
		.frame0.frameNMS.third.button configure -state normal
	}
	
	# Exit GRASS, if window gets closed.
	wm protocol . WM_DELETE_WINDOW {
		exit 2
	}
  
	grab .
	tkwait window . 

}

#############################################################################

proc refresh_loc {} {
# refresh location listbox entries 
	global database

	set locList .frame0.frameLOC.listbox
	set mapList .frame0.frameMS.listbox
 

	if { "$database" != "" \
		 && [file exists $database] && [file isdirectory $database] } {
	   cdir $database
	   $locList delete 0 end
	   foreach i [lsort [glob -nocomplain -directory [pwd] *]] {
		   if { [file isdirectory $i] } {
			   $locList insert end [file tail $i]
		   }
	   }
	   $mapList delete 0 end
	}
	.frame0.frameBUTTONS.ok configure -state disabled
	update idletasks
}

proc refresh_ms {} {
# refresh location listbox entries
        global database
	global location

	set mapList .frame0.frameMS.listbox
	$mapList delete 0 end
	if { [CheckLocation] } {
	        cdir $database
		cdir $location
		foreach i [lsort [glob -directory [pwd] *]] {
			if {[file isdirectory $i] && [file owned $i] } {
				$mapList insert end [file tail $i]
			}
		}
	}
	.frame0.frameBUTTONS.ok configure -state disabled
}

#############################################################################

proc cdir { dir } {
# cd wrapper
    if { [catch { cd $dir }] } {
        DialogGen .wrnDlg [G_msg "WARNING: change directory failed"] warning \
          [format [G_msg "Warning: could not change directory to <%s>.\nCheck directory permissions."] $dir ]\
          0 OK;
          return 1
    } else {
        return 0
    }
}

proc selFromList { lis str } {
# Selects list entry, if there is match
  set siz [$lis size]
  set curSelected 0
  for { set x 0 } { $x < $siz } { incr x } {
        if { $str == [$lis get $x] } {
	        set curSelected $x
		break
	}
  }
  $lis yview $curSelected
  $lis selection clear 0 end
  $lis select set $curSelected
}

#############################################################################
#
# proc DialogGen {widget title bitmap text default buttons}
#
# PURPOSE:  	This function simply pops up a dialog box with a given message.
#   	    	Note that it is similar to tk_dialog but has a slightly
#   	    	different look to the dialog.
#   	    	Example call:
#   	    	    set val [DialogGen .warnDlg "WARNING: List Changed" \
#   	    	    	warning "WARNING: You have changed the current list.\
#   	    	    	Do you want to discard the changes and open a new \
#   	    	    	file?" 0 OK Cancel]
#    	    	    if { $val == 0 } { puts stderr "OK button pressed" }
#   	    	    if { $val == 1 } { puts stderr "Cancel button pressed" }
# INPUT VARS:	widget	=>  name of the dialog box starting with . eg .errDlg
#   	    	title	=>  title to display in window border
#   	    	bitmap	=>  bitmap icon to display - must be one of
#   	    	    	    	error	    	gray12
#   	    	    	    	gray50 	    	hourglass
#   	    	    	    	info   	    	questhead
#   	    	    	    	question    	warning
#   	    	text	=>  text of the message to be displayed
#   	    	default =>  index of default button (0, 1, 2...) must be less
#   	    	    	    than number of buttons
#   	    	buttons =>  text to be used for each button eg OK Cancel
# RETURN VAL:	index of button that was clicked - can be ignored if only one
#   	    	button is defined
#
#############################################################################

# Procedure to generate the dialog box
proc DialogGen {widget title bitmap text default buttons} \
{
    global buttonNum
    
    # Create a popup window to warn the user
    toplevel $widget
    wm title $widget $title
    wm resizable $widget 0 0
    wm protocol $widget WM_DELETE_WINDOW "CancelDialog $widget"

    # Create a label for the bitmap and a message for the text
    frame $widget.dlgFrame
    pack $widget.dlgFrame -side top -fill both
    label $widget.dlgFrame.icon -bitmap $bitmap
    message $widget.dlgFrame.text -text $text -width 10c
    pack $widget.dlgFrame.icon $widget.dlgFrame.text -side left -fill x \
		-padx 10
    
    # Create a frame for the pushbuttons
    frame $widget.sepFrame -height 4 -bd 2 -relief raised
    frame $widget.buttonFrame 
    pack $widget.buttonFrame $widget.sepFrame -side bottom -fill x

    # Create the pushbuttons
    set i 0
    foreach buttonLabel $buttons {
		button $widget.buttonFrame.$i -bd 1 -text $buttonLabel -command "set buttonNum $i"
		pack $widget.buttonFrame.$i -side left -expand 1 -padx 10 -pady 5
		incr i
    }
    
    # Position the top left corner of the window over the root window
    wm withdraw $widget
    update idletasks
    wm geometry $widget +[expr [winfo rootx .] + ([winfo width .] \
		-[winfo width $widget]) / 2]+[expr [winfo rooty .] + ([winfo \
		height .] - [winfo height $widget]) / 2]
    wm deiconify $widget

    # Grab the pointer to make sure this window is closed before continuing
    grab set $widget

    if {$default >= 0} {
		focus $widget.buttonFrame.$default
    }
    
    tkwait variable buttonNum
    
    # Destroy the popup window
    destroy $widget
    
    # Return the number of the button that was pushed
    return "$buttonNum"
}

# Procedure to cancel the dialog
proc CancelDialog {widget} {
    global buttonNum

    # Set the wait variable so that the dialog box can cancel properly
    set buttonNum 999
}


#############################################################################

global database
global location
global mapset
global grassrc_list
global gisrc_name

set ver [info tclversion]

if { [string compare $ver "8.0"] < 0} {
    puts stderr "Sorry your version of the Tcl/Tk libraries is $ver and is too"
    puts stderr "old for GRASS which requires a Tcl/Tk library version of 8.0 or later."
    puts stderr "Reverting default settings back to GRASS text mode interface."
    exit 1
}

set database ""
set location ""
set mapset ""
set gisrc_name ""

if { [info exists env(GISRC)] } {
   set gisrc_name $env(GISRC)
}

if { [searchGISRC $gisrc_name] } {
   gisSetWindow
}

