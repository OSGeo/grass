lappend auto_path $env(GISBASE)/bwidget
package require -exact BWidget 1.2.1 
#package require http

set formpath $env(GISBASE)/etc/form
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

proc add_form { formid title } {
    global nb formf html

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
  
    foreach frm [ .nb pages ] {
        .nb delete $frm
    }
}

proc HMsubmit_form {win param query} {
    global submit_result submit_msg

    regexp -- {\.nb\.f(.+)\.frm\.txt} $win r formid 
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

	set nb [NoteBook .nb]
	$nb configure -width 300 -height 500
	pack .nb -fill both -expand yes
}

proc close_form {} {
	global form_open
	wm withdraw .
	set form_open false
}

proc process_command {} {
	global env
	global child_recv child_send 
	global form_open encoding_val frmid
	global html

	if {[eof $child_recv]} {
		exit 0
	}

	set cmd [read $child_recv 1]

	switch $cmd {
		O {
			if {! $form_open} {
				wm state . normal
				set form_open true
			}
			#  Read title 
			set length [gets $child_recv]
			set child_title [read $child_recv $length]

			#  Read html 
			set length [gets $child_recv]
			set child_html [read $child_recv $length]

			set child_html [encoding convertfrom $encoding_val $child_html]

			#  Insert new page 
			set html $child_html
			add_form $frmid $child_title

			puts -nonewline $child_send O
			flush $child_send
			incr frmid
		}
		C {	#  clear old forms 
			clear_nb
			puts -nonewline $child_send O
			flush $child_send
		}
		D {	#  done! 
			clear_nb
			puts -nonewline $child_send O
			flush $child_send

			destroy .
			exit 0
		}
	}
}

make_form

wm protocol . WM_DELETE_WINDOW close_form

bind . <Destroy> { if { "%W" == "."} { close_form } }

set submit_result ""
set submit_msg ""
set html ""

set frmid 0
set form_open true

set child_recv stdin
set child_send stdout

set encoding_val [exec g.gisenv GRASS_DB_ENCODING]
if {$encoding_val == ""} {
	set encoding_val [encoding system]
}

if {[catch {encoding system $encoding_val}]} {
	puts stderr "Could not set Tcl system encoding to $encoding_val"
}

fconfigure $child_recv -buffering none -encoding binary -translation binary

fileevent $child_recv readable process_command
