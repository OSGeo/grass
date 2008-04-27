
proc Q_init {size {addproc  Nv_mapPanel} {rmproc Nv_closePanel}} {
    for {set i 0; set queue {}} {$i < $size} {incr i} {
       set queue [concat $queue open]}
    set Q [St_create {next maxsize currsize addproc rmproc queue} \
		      0 $size 0 $addproc $rmproc $queue]
    return $Q
}

# add an item to the Q, if necessary bumping items of lower priority;
# return position where it was added
proc Q_add {Q name } { 

    set next [St_get $Q next]
    set size [St_get $name size] 
    set maxsize [St_get $Q maxsize]
    set addproc [St_get $Q addproc]
    for {set i 0} {$i < $size} {} {
        set index [expr ($next + $i)]
        if {$index >= $maxsize} {
            set index 0
            set next 0
            set i 0
        }

        set queue [St_get $Q queue]
	set tmp [lindex $queue $index]
	if [string compare open $tmp] { 
	    Q_remove $Q $tmp
	    incr i [St_get $tmp size]
	} else {incr i}
    }
        set queue [St_get $Q queue]

    for {set i $next} {$i < [expr $next + $size]} {incr i} {
	set queue [lreplace $queue $i $i $name]
    }
    $addproc  [St_get $name window] $name  [expr $next  + 1]
    St_set $Q next [expr ($next + $size)%$maxsize]
    St_set $Q queue $queue

    return $next
}
# remove an item from the Q
proc Q_remove {Q name} {
    set queue [St_get $Q queue]
    set rmproc [St_get $Q rmproc]
    set size [St_get $name size]
    set index [lsearch $queue $name]
    for {set i $index} {$i < [expr $index + $size]} {incr i} {
	    set queue [lreplace $queue $i $i open]
    }
    $rmproc [St_get $name window]
    St_set $Q queue $queue
        set queue [St_get $Q queue]
}

proc Q_get_pos {Q name} {

    set queue [St_get $Q queue]
    set pos [lsearch $queue $name]
    if {$pos >= 0} {incr pos}
    return $pos
}
