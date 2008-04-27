 ##########################################################################
# Default Priority for this panel
#
# priority is from 0 to 10
#  the lower the number, the quicker it will be bumped
#  10 cannot be bumped
#  Panels will be loaded by the greater of 5 or their current priority

#########################################################################
#create Color panel
##########################################################################



proc mktstPanel { BASE } {
    global Nv_

    catch {destroy $BASE}
#  Initialize panel info
    if [catch {set Nv_($BASE)}] {
        set panel [St_create {window name size priority} $BASE "Test" 1 5]
    } else {
	set panel $Nv_($BASE)
    }

# This line taken out since auto-pathing should now be in place
# (Mark 9/21/94)
#    source test_procs.tcl

    frame $BASE  -relief groove -borderwidth 2
    Nv_mkPanelname $BASE "Test Panel"
    
    #button $BASE.test1 -text Test1 -bg black -fg white\
	#-activebackground gray20 -activeforeground white\
	#-command test1
    #button $BASE.test2 -text Test2 -bg black -fg white\
	#-activebackground gray20 -activeforeground white\
	#-command test2
    #button $BASE.test3 -text Test3 -bg black -fg white\
	#-activebackground gray30 -activeforeground white\
	#-command test3

    ###########################################################################
    frame $BASE.menu  -relief raised -borderwidth 1
    place $BASE.menu  -relx .65 -relwidth .35 -rely 0 -relheight .03

    mkMapList $BASE.test1 vect test4
    mkMapList $BASE.test2 surf [list test5 surf]
    pack $BASE.test1 $BASE.test2 -side top -expand 1 


    frame $BASE.closef
    button $BASE.closef.close -text Close -command "Nv_closePanel $BASE" \
	-anchor se
    pack $BASE.closef.close -side right
    pack $BASE.closef -side bottom -fill x

    return $panel
}
