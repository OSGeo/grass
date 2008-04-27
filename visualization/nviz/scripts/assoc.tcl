# This file contains routines for treating Tcl/Tk lists
# as association lists.  Specifically, we supply the routine
# assoc which takes an association list and a key and returns
# the key-value pair if found, or NIL otherwise

proc assoc { alist key } {
	if {[llength $alist] == 0} then {
		return NIL
	}

	if {[lindex [lindex $alist 0] 0] == $key} then {
		return [lindex $alist 0]
	}

	return [assoc [lrange $alist 1 end] $key]
}
