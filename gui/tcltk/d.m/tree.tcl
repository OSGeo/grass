
namespace eval DmTree {
    variable count
    variable tree
    variable selected ""
    variable dblclick
    variable legend_height 20
    variable legend_width 30
}


proc DmTree::redraw { } {
    variable tree
  
    Tree::_update_scrollregion  $tree 
}

proc DmTree::create { parent } {
    variable legend_height
    variable legend_width
    variable tree
    set sw    [ScrolledWindow $parent.sw \
                  -relief sunken -borderwidth 2 ]

    set lw [expr $legend_width + 27]
    set lh [expr $legend_height + 6]
    set tree  [Tree $sw.tree \
            -relief flat -borderwidth 0 -width 15 -highlightthickness 0 \
            -redraw 1 -dropenabled 1 -dragenabled 1 \
            -dragevent 1 -dropcmd "DmTree::drop" \
            -opencmd   "DmTree::open $sw.tree" \
            -closecmd  "DmTree::close $sw.tree" \
            -deltay $lh -padx $lw \
            -width 50 ]
            
	$tree configure -height 6
    $sw setwidget $tree
	
    pack $sw  -side top -expand yes -fill both
    pack $tree  -side top -expand yes -fill both

    $tree bindText  <ButtonPress-1> "DmTree::select $tree"
    $tree bindImage <ButtonPress-1> "DmTree::select $tree"
    $tree bindText  <Double-ButtonPress-1> "DmTree::edit $tree"

    $tree configure -redraw 1

    # ScrollView
    toplevel .top -relief raised -borderwidth 2
    wm protocol .top WM_DELETE_WINDOW {
        # don't kill me
    }
    wm overrideredirect .top 1
    wm withdraw .top
    wm transient .top .
    ScrollView .top.sv -window $tree -fill black
    pack .top.sv -fill both -expand yes

    return $tree
}

proc DmTree::drop { from to where operation type data } {

    set old_parent [$from parent $data]
    set old_index [$from index $data]
    if { [lindex $where 0] == "position" } { 
        set new_parent [lindex $where 1]
        set new_index [lindex $where 2]
    } elseif { [lindex $where 0] == "widget" } {
        set new_parent "root"
        set new_index [llength [$from nodes "root"] ]
    } else {
        set node [lindex $where 1]
        if { [Dm::node_type $node] == "group" } {
	    set new_parent $node
	    set new_index 0
       } else { 
	    set new_parent [$from parent $node]
	    set new_index [$from index $node]
	    incr new_index
       }
    }

    # test if new is not in childrens
    set parent $new_parent
    while { $parent != "root" } {
        if { $parent == $data } { return }
	set parent [$from parent $parent]
    }

    if { ($old_parent == $new_parent) && ($new_index > $old_index) } { 
        set new_index [expr $new_index - 1]
    }

    $from move $new_parent $data $new_index

}

proc DmTree::open { tree node } {
    DmGroup::open [Dm::node_id $node]
}

proc DmTree::close { tree node } {
    DmGroup::close [Dm::node_id $node]
}

proc DmTree::select { tree node } {
    variable selected 

    if { $selected == $node } {
        $tree selection clear $node
        set selected ""
        Dm::deselect $node
    } else {
        $tree selection set $node
        update
        set selected $node
        Dm::select $node
    }

}

proc DmTree::edit { tree node } {
    set res [$tree edit $node [$tree itemcget $node -text]]
    if { $res != "" } {
	$tree itemconfigure $node -text $res
    }
}

proc DmTree::autoname { tree node name} {
	$tree itemconfigure $node -text $name
}
