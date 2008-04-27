lappend auto_path $env(GISBASE)/bwidget
package require -exact BWidget 1.2.1 
#package require http

set formpath $env(GISBASE)/etc/v.digit
source $formpath/html_library.tcl

proc create_submit_msg { formid  }  {
    global submit_result submit_msg formf

    destroy $formf($formid).sbw 
    destroy $formf($formid).sbt

    if { $submit_result == 1 } { set color "green" } else { set color "red" }
    set sbw [ScrolledWindow $formf($formid).sbw -relief sunken -borderwidth 2]
    set sbt [text $formf($formid).sbt -height 3 -width 20 -foreground $color ]
    pack $sbw $sbt -fill x
    $sbw setwidget $sbt
    $sbt insert end $submit_msg
    $sbt configure -state disabled
}

proc add_form { formid title html } {
    global nb formf

    set form($formid) [$nb insert end $formid -text $title]
    $nb raise $formid
    set formf($formid) [ frame $form($formid).frm ]
    set formsw($formid) [ScrolledWindow $formf($formid).sw -relief sunken -borderwidth 2]
    set formtxt($formid) [ text $formf($formid).txt -height 5 -width 20 ]
    pack $formf($formid) $formsw($formid) $formtxt($formid) -fill both -expand yes
    $formsw($formid) setwidget $formtxt($formid)
    HMinit_win $formtxt($formid)
    HMparse_html $html "HMrender $formtxt($formid)"
    $formtxt($formid) configure -state disabled
}

proc clear_nb { }  {
    global submit_msg
 
    set submit_msg ""
  
    foreach frm [ .form.nb pages ] {
        .form.nb delete $frm
    }
}

proc HMsubmit_form {win param query} {
    global submit_result submit_msg

    regexp -- {\.form\.nb\.f(.+)\.frm\.txt} $win r formid 
    #puts "win = $win formid = $formid"

    reset_values
    foreach {col val} $query {
        #puts "$col : $val" 
        set_value $col $val 
    }

    submit $formid
    #puts "result = $submit_result msg = $submit_msg" 
    create_submit_msg $formid   
}

proc make_form {} {
	global nb

	set nb [NoteBook .form.nb]
	$nb configure -width 300 -height 500
	pack .form.nb -fill both -expand yes
}

proc close_form {} {
	global form_open

	wm withdraw .form
	set form_open false
}

proc open_form {title html} {
	global form_open encoding_val frmid

	if {! $form_open} {
		wm state .form normal
		set form_open true
	}

	set html [encoding convertfrom $encoding_val $html]

	#  Insert new page 
	add_form $frmid $title $html

	incr frmid
}

proc clear_form {} {
	clear_nb
}

proc done_form {} {
	clear_nb
	close_form
}

proc init_form {} {
	global submit_result submit_msg
	global frmid form_open encoding_val

	toplevel .form

	make_form

	wm protocol .form WM_DELETE_WINDOW close_form

	bind .form <Destroy> { if { "%W" == ".form"} { close_form } }

	set submit_result ""
	set submit_msg ""

	set frmid 0
	set form_open true

	set encoding_val [exec g.gisenv GRASS_DB_ENCODING]
	if {$encoding_val == ""} {
		set encoding_val [encoding system]
	}

	if {[catch {encoding system $encoding_val}]} {
		puts stderr "Could not set Tcl system encoding to $encoding_val"
	}
}

