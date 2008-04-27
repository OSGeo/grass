############################################################################
# routines to create structures in tcl, and get & set fields.
############################################################################

global St_

# This line taken out since auto-pathing should now be in place
# (Mark 9/21/94)
# source unique.tcl

proc St_create {keylist args} {
    global St_

    if {[llength $keylist] == 0} {
        return
    } else {
	set token [unique index]
	set St_($token) [concat [list $keylist] $args]

	return $token
    }
}

proc St_set {token key val} {
    global St_
    
    if {[catch {set St_($token)}] == 0} {
        if {[llength $St_($token)] >= 2} {
    	    set index [lsearch [lindex $St_($token) 0] $key]
	    if {$index >= 0} {
	        if {[llength $St_($token)] >= $index} {
	            incr index
                    set St_($token) [lreplace $St_($token) $index $index $val]
		    return $index 
		}
	    }
	}
    }
    return
}

proc St_get {token key} {
    global St_

    if {[catch {set St_($token)}] == 0} {
        if {[llength $St_($token)] >= 2} {
            set index [lsearch [lindex $St_($token) 0] $key]
	    if {$index >= 0} {
	        if {[llength $St_($token)] >= $index} {
		    incr index
		    return [lindex $St_($token) $index]
	        }
	    }
        }
    }
    return
}






