
namespace eval DmCmd {
    variable array opt # cmd options
    variable count 1
}


proc DmCmd::create { tree parent } {
    variable opt
    variable count 
    global dmpath

    set node "cmd:$count"

    set frm [ frame .cmdicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmCmd::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo cico -file "$dmpath/cmd.gif"
    set ico [label $frm.ico -image cico -bd 1 -relief raised]
    
    pack $check $ico -side left

    $tree insert end $parent $node \
	-text      "cmd $count" \
	-window    $frm \
	-drawcross auto 

    set opt($count,_check) 1 

    set opt($count,cmd) "" 

    incr count
    return $node
}

proc DmCmd::set_option { node key value } {
    variable opt
 
    set id [Dm::node_id $node]
    set opt($id,$key) $value
}

# display cmd options
proc DmCmd::options { id frm } {
    variable opt

    # cmd name
    set row [ frame $frm.name ]
    Label $row.a -text [G_msg "Command:"] 
    Entry $row.b -width 40 -text "$opt($id,cmd)" \
          -textvariable DmCmd::opt($id,cmd)
    pack $row.a $row.b -side left
    pack $row -side top -fill both -expand yes
}

proc DmCmd::save { tree depth node } {
    variable opt
    
    set id [Dm::node_id $node]

    foreach key { _check cmd } {
        Dm::rc_write $depth "$key $opt($id,$key)"
    } 
}

proc DmCmd::display { node } {
    variable opt
    
    set tree $Dm::tree
    set id [Dm::node_id $node]

    if { ! ( $opt($id,_check) ) } { return } 

    if { $opt($id,cmd) == "" } { return } 

    set cmd $opt($id,cmd)

    runcmd $cmd
}

proc DmCmd::query { node } {
    puts "Query not supported for Command type layer"
}

proc DmCmd::duplicate { tree parent node id} {
    variable opt
    variable count 
    global dmpath

    set node "cmd:$count"

    set frm [ frame .cmdicon$count]
    set fon [font create -size 10] 
    set check [checkbutton $frm.check -font $fon \
                           -variable DmCmd::opt($count,_check) \
                           -height 1 -padx 0 -width 0]

    image create photo cico -file "$dmpath/cmd.gif"
    set ico [label $frm.ico -image cico -bd 1 -relief raised]
    
    pack $check $ico -side left

    $tree insert end $parent $node \
	-text      "cmd $count" \
	-window    $frm \
	-drawcross auto 

    set opt($count,_check) 1 

    set opt($count,cmd) "$opt($id,cmd)" 

    incr count
    return $node
}