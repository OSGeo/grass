#
# Proc UNIQUE 
#
#   Usage:  unique NAME
#
#    returns a name which is guaranteed to be unique (when compared to other
#     names unique returns) with $name as its prefix. Basically
#     it appends 1-N to the end of the string.  This is good up to
#     the max int value at which point it will wrap around and potentially
#     fail.
#
#
#  EXTENSIONS:
#
#     None
#
#
#  RETURNS:
#     Normal return value
#     Result is a unique string
#
#
#  GLOBALS:
#     unique_a()
#     
#  PROCS:
#     unique
#     



set unique_a(start) 1

proc unique {name} {
    global unique_a

    switch [catch {set num [set unique_a($name)]}] {
	0 {			; # Found a match
	    incr num
	    set unique_a($name) $num
	    return $name$num
	  }
	1 {			; # no match  new name
	    set num 1
	    set unique_a($name) $num
	    return $name$num
	  }
    }

    return -code error "unknown error in unique"
}








