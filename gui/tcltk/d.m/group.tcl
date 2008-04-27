
namespace eval DmGroup {
    variable array opt 
    variable count 1 
}


proc DmGroup::create { tree parent } {
    variable opt
    variable count 

    set fon [font create -size 10]
    set frm [ frame .groupicon$count ]
    set check [checkbutton $frm.check -text "" -font $fon \
                           -variable DmGroup::opt($count,_check) \
                           -height 1]
    set image [label $frm.image -image [Bitmap::get folder] ]
    pack $check $image -side left

    set node "group:$count"
    $tree insert end $parent $node \
	-text      "group $count" \
	-window    $frm \
	-drawcross auto \
        -open 1
	
    set opt($count,_check) 1
    set opt($count,treeimagepath) $image

    incr count

    return $node
}

proc DmGroup::save { tree depth node } {
    variable opt

    if { $node != "root" } {
	set id [Dm::node_id $node] 
       Dm::rc_write $depth _check $opt($id,_check)
    }

    foreach n [$tree nodes $node] {
        Dm::save_node $depth $n
    }

}

proc DmGroup::display { node } {
    variable opt

    set tree $Dm::tree
    if { $node != "root" } {
	set id [Dm::node_id $node] 
        if { ! ( $opt($id,_check) ) } { return }
    }

    foreach n [$tree nodes $node] {
        Dm::display_node $n
    }

}

proc DmGroup::print { file  node } {
    variable opt
    global raster_printed

    set tree $Dm::tree
    if { $node != "root" } {
	set id [Dm::node_id $node] 
        if { ! ( $opt($id,_check) ) } { return }
    } else {
        set raster_printed 0
    }

    set lst ""
    foreach n [$tree nodes $node] {
        set lst [ concat $n $lst ]
    }

    foreach n $lst {
        Dm::print_node $file $n
    }

}

proc DmGroup::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
 
    set opt($id,$key) $value
}

proc DmGroup::open { id } {
    variable opt


    $DmGroup::opt($id,treeimagepath) configure -image [Bitmap::get openfold]

}

proc DmGroup::close { id } {
    variable opt

    $DmGroup::opt($id,treeimagepath) configure -image [Bitmap::get folder]
}

proc DmGroup::duplicate { tree parent_node sel id } {
    puts "Duplicate for Groups not yet implemented."
}
