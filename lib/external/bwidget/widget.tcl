# ------------------------------------------------------------------------------
#  widget.tcl
#  This file is part of Unifix BWidget Toolkit
#  $Id$
# ------------------------------------------------------------------------------
#  Index of commands:
#     - Widget::tkinclude
#     - Widget::bwinclude
#     - Widget::declare
#     - Widget::addmap
#     - Widget::init
#     - Widget::destroy
#     - Widget::setoption
#     - Widget::configure
#     - Widget::cget
#     - Widget::subcget
#     - Widget::hasChanged
#     - Widget::_get_tkwidget_options
#     - Widget::_test_tkresource
#     - Widget::_test_bwresource
#     - Widget::_test_synonym
#     - Widget::_test_string
#     - Widget::_test_flag
#     - Widget::_test_enum
#     - Widget::_test_int
#     - Widget::_test_boolean
# ------------------------------------------------------------------------------

namespace eval Widget {
    variable _optiontype
    variable _class
    variable _tk_widget

    array set _optiontype {
        TkResource Widget::_test_tkresource
        BwResource Widget::_test_bwresource
        Enum       Widget::_test_enum
        Int        Widget::_test_int
        Boolean    Widget::_test_boolean
        String     Widget::_test_string
        Flag       Widget::_test_flag
        Synonym    Widget::_test_synonym
    }

    proc use {} {}
}



# ------------------------------------------------------------------------------
#  Command Widget::tkinclude
#     Includes tk widget resources to BWidget widget.
#  class      class name of the BWidget
#  tkwidget   tk widget to include
#  subpath    subpath to configure
#  args       additionnal args for included options
# ------------------------------------------------------------------------------
proc Widget::tkinclude { class tkwidget subpath args } {
    foreach {cmd lopt} $args {
        # cmd can be
        #   include      options to include            lopt = {opt ...}
        #   remove       options to remove             lopt = {opt ...}
        #   rename       options to rename             lopt = {opt newopt ...}
        #   prefix       options to prefix             lopt = {prefix opt opt ...}
        #   initialize   set default value for options lopt = {opt value ...}
        #   readonly     set readonly flag for options lopt = {opt flag ...}
        switch -- $cmd {
            remove {
                foreach option $lopt {
                    set remove($option) 1
                }
            }
            include {
                foreach option $lopt {
                    set include($option) 1
                }
            }
            prefix {
                set prefix [lindex $lopt 0]
                foreach option [lrange $lopt 1 end] {
                    set rename($option) "-$prefix[string range $option 1 end]"
                }
            }
            rename     -
            readonly   -
            initialize {
                array set $cmd $lopt
            }
            default {
                return -code error "invalid argument \"$cmd\""
            }
        }
    }

    namespace eval $class {}
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap

    # create resources informations from tk widget resources
    foreach optdesc [_get_tkwidget_options $tkwidget] {
        set option [lindex $optdesc 0]
        if { (![info exists include] || [info exists include($option)]) &&
             ![info exists remove($option)] } {
            if { [llength $optdesc] == 3 } {
                # option is a synonym
                set syn [lindex $optdesc 1]
                if { ![info exists remove($syn)] } {
                    # original option is not removed
                    if { [info exists rename($syn)] } {
                        set classopt($option) [list Synonym $rename($syn)]
                    } else {
                        set classopt($option) [list Synonym $syn]
                    }
                }
            } else {
                if { [info exists rename($option)] } {
                    set realopt $option
                    set option  $rename($option)
                } else {
                    set realopt $option
                }
                if { [info exists initialize($option)] } {
                    set value $initialize($option)
                } else {
                    set value [lindex $optdesc 1]
                }
                if { [info exists readonly($option)] } {
                    set ro $readonly($option)
                } else {
                    set ro 0
                }
                set classopt($option) [list TkResource $value $ro [list $tkwidget $realopt]]
                lappend classmap($option) $subpath "" $realopt
            }
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::bwinclude
#     Includes BWidget resources to BWidget widget.
#  class    class name of the BWidget
#  subclass BWidget class to include
#  subpath  subpath to configure
#  args     additionnal args for included options
# ------------------------------------------------------------------------------
proc Widget::bwinclude { class subclass subpath args } {
    foreach {cmd lopt} $args {
        # cmd can be
        #   include      options to include            lopt = {opt ...}
        #   remove       options to remove             lopt = {opt ...}
        #   rename       options to rename             lopt = {opt newopt ...}
        #   prefix       options to prefix             lopt = {prefix opt opt ...}
        #   initialize   set default value for options lopt = {opt value ...}
        #   readonly     set readonly flag for options lopt = {opt flag ...}
        switch -- $cmd {
            remove {
                foreach option $lopt {
                    set remove($option) 1
                }
            }
            include {
                foreach option $lopt {
                    set include($option) 1
                }
            }
            prefix {
                set prefix [lindex $lopt 0]
                foreach option [lrange $lopt 1 end] {
                    set rename($option) "-$prefix[string range $option 1 end]"
                }
            }
            rename     -
            readonly   -
            initialize {
                array set $cmd $lopt
            }
            default {
                return -code error "invalid argument \"$cmd\""
            }
        }
    }

    namespace eval $class {}
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    upvar 0 ${subclass}::opt subclassopt

    # create resources informations from BWidget resources
    foreach {option optdesc} [array get subclassopt] {
        if { (![info exists include] || [info exists include($option)]) &&
             ![info exists remove($option)] } {
            set type [lindex $optdesc 0]
            if { ![string compare $type "Synonym"] } {
                # option is a synonym
                set syn [lindex $optdesc 1]
                if { ![info exists remove($syn)] } {
                    if { [info exists rename($syn)] } {
                        set classopt($option) [list Synonym $rename($syn)]
                    } else {
                        set classopt($option) [list Synonym $syn]
                    }
                }
            } else {
                if { [info exists rename($option)] } {
                    set realopt $option
                    set option  $rename($option)
                } else {
                    set realopt $option
                }
                if { [info exists initialize($option)] } {
                    set value $initialize($option)
                } else {
                    set value [lindex $optdesc 1]
                }
                if { [info exists readonly($option)] } {
                    set ro $readonly($option)
                } else {
                    set ro [lindex $optdesc 2]
                }
                set classopt($option) [list $type $value $ro [lindex $optdesc 3]]
                lappend classmap($option) $subpath $subclass $realopt
            }
        }
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::declare
#    Declares new options to BWidget class.
# ------------------------------------------------------------------------------
proc Widget::declare { class optlist } {
    variable _optiontype

    namespace eval $class {}
    upvar 0 ${class}::opt classopt

    foreach optdesc $optlist {
        set option  [lindex $optdesc 0]
        set optdesc [lrange $optdesc 1 end]
        set type    [lindex $optdesc 0]

        if { ![info exists _optiontype($type)] } {
            # invalid resource type
            return -code error "invalid option type \"$type\""
        }

        if { ![string compare $type "Synonym"] } {
            # test existence of synonym option
            set syn [lindex $optdesc 1]
            if { ![info exists classopt($syn)] } {
                return -code error "unknow option \"$syn\" for Synonym \"$option\""
            }
            set classopt($option) [list Synonym $syn]
            continue
        }

        # all other resource may have default value, readonly flag and
        # optional arg depending on type
        set value [lindex $optdesc 1]
        set ro    [lindex $optdesc 2]
        set arg   [lindex $optdesc 3]

        if { ![string compare $type "BwResource"] } {
            # We don't keep BwResource. We simplify to type of sub BWidget
            set subclass    [lindex $arg 0]
            set realopt     [lindex $arg 1]
            if { ![string length $realopt] } {
                set realopt $option
            }

            upvar 0 ${subclass}::opt subclassopt
            if { ![info exists subclassopt($realopt)] } {
                return -code error "unknow option \"$realopt\""
            }
            set suboptdesc $subclassopt($realopt)
            if { $value == "" } {
                # We initialize default value
                set value [lindex $suboptdesc 1]
            }
            set type [lindex $suboptdesc 0]
            set ro   [lindex $suboptdesc 2]
            set arg  [lindex $suboptdesc 3]
            set classopt($option) [list $type $value $ro $arg]
            continue
        }

        # retreive default value for TkResource
        if { ![string compare $type "TkResource"] } {
            set tkwidget [lindex $arg 0]
            set realopt  [lindex $arg 1]
            if { ![string length $realopt] } {
                set realopt $option
            }
            set tkoptions [_get_tkwidget_options $tkwidget]
            if { ![string length $value] } {
                # We initialize default value
                set value [lindex [lindex $tkoptions [lsearch $tkoptions [list $realopt *]]] end]
            }
            set classopt($option) [list TkResource $value $ro [list $tkwidget $realopt]]
            continue
        }

        # for any other resource type, we keep original optdesc
        set classopt($option) [list $type $value $ro $arg]
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::addmap
# ------------------------------------------------------------------------------
proc Widget::addmap { class subclass subpath options } {
    upvar 0 ${class}::map classmap

    foreach {option realopt} $options {
        if { ![string length $realopt] } {
            set realopt $option
        }
        lappend classmap($option) $subpath $subclass $realopt
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::syncoptions
# ------------------------------------------------------------------------------
proc Widget::syncoptions { class subclass subpath options } {
    upvar 0 ${class}::sync classync

    foreach {option realopt} $options {
        if { ![string length $realopt] } {
            set realopt $option
        }
        set classync($option) [list $subpath $subclass $realopt]
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::init
# ------------------------------------------------------------------------------
proc Widget::init { class path options } {
    variable _class
    variable _optiontype

    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::$path:opt  pathopt
    upvar 0 ${class}::$path:mod  pathmod

    catch {unset pathopt}
    catch {unset pathmod}
    set fpath ".#BWidgetClass#$class"
    regsub -all "::" $class "" rdbclass
    if { ![winfo exists $fpath] } {
        frame $fpath -class $rdbclass
    }
    foreach {option optdesc} [array get classopt] {
        set type [lindex $optdesc 0]
        if { ![string compare $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        if { ![string compare $type "TkResource"] } {
            set alt [lindex [lindex $optdesc 3] 1]
        } else {
            set alt ""
        }
        set optdb [lindex [_configure_option $option $alt] 0]
        set def   [option get $fpath $optdb $rdbclass]
        if { [string length $def] } {
            set pathopt($option) $def
        } else {
            set pathopt($option) [lindex $optdesc 1]
        }
        set pathmod($option) 0
    }

    set _class($path) $class
    foreach {option value} $options {
        if { ![info exists classopt($option)] } {
            unset pathopt
            unset pathmod
            return -code error "unknown option \"$option\""
        }
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { ![string compare $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        set pathopt($option) [$_optiontype($type) $option $value [lindex $optdesc 3]]
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::destroy
# ------------------------------------------------------------------------------
proc Widget::destroy { path } {
    variable _class

    set class $_class($path)
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod

    catch {unset pathopt}
    catch {unset pathmod}
}


# ------------------------------------------------------------------------------
#  Command Widget::configure
# ------------------------------------------------------------------------------
proc Widget::configure { path options } {
    set len [llength $options]
    if { $len <= 1 } {
        return [_get_configure $path $options]
    } elseif { $len % 2 == 1 } {
        return -code error "incorrect number of arguments"
    }

    variable _class
    variable _optiontype

    set class $_class($path)
    upvar 0 ${class}::opt  classopt
    upvar 0 ${class}::map  classmap
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod

    set window [_get_window $class $path]
    foreach {option value} $options {
        if { ![info exists classopt($option)] } {
            return -code error "unknown option \"$option\""
        }
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { ![string compare $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        if { ![lindex $optdesc 2] } {
            set curval $pathopt($option)
            set newval [$_optiontype($type) $option $value [lindex $optdesc 3]]
            if { [info exists classmap($option)] } {
                foreach {subpath subclass realopt} $classmap($option) {
                    if { [string length $subclass] } {
                        ${subclass}::configure $window$subpath $realopt $newval
                    } else {
                        $window$subpath configure $realopt $newval
                    }
                }
            }
            set pathopt($option) $newval
            set pathmod($option) [expr {[string compare $newval $curval] != 0}]
        }
    }

    return {}
}


# ------------------------------------------------------------------------------
#  Command Widget::cget
# ------------------------------------------------------------------------------
proc Widget::cget { path option } {
    variable _class

    if { ![info exists _class($path)] } {
        return -code error "unknown widget $path"
    }

    set class $_class($path)
    upvar 0 ${class}::opt  classopt
    upvar 0 ${class}::sync classync
    upvar 0 ${class}::$path:opt pathopt

    if { ![info exists classopt($option)] } {
        return -code error "unknown option \"$option\""
    }
    set optdesc $classopt($option)
    set type    [lindex $optdesc 0]
    if { ![string compare $type "Synonym"] } {
        set option [lindex $optdesc 1]
    }

    if { [info exists classync($option)] } {
        set window [_get_window $class $path]
        foreach {subpath subclass realopt} $classync($option) {
            if { [string length $subclass] } {
                set pathopt($option) [${subclass}::cget $window$subpath $realopt]
            } else {
                set pathopt($option) [$window$subpath cget $realopt]
            }
        }
    }

    return $pathopt($option)
}


# ------------------------------------------------------------------------------
#  Command Widget::subcget
# ------------------------------------------------------------------------------
proc Widget::subcget { path subwidget } {
    variable _class

    set class $_class($path)
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::$path:opt pathopt

    set result {}
    foreach {option map} [array get classmap] {
        foreach {subpath subclass realopt} $map {
            if { ![string compare $subpath $subwidget] } {
                lappend result $realopt $pathopt($option)
            }
        }
    }
    return $result
}


# ------------------------------------------------------------------------------
#  Command Widget::hasChanged
# ------------------------------------------------------------------------------
proc Widget::hasChanged { path option pvalue } {
    upvar    $pvalue value
    variable _class

    set class $_class($path)
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod

    set value   $pathopt($option)
    set result  $pathmod($option)
    set pathmod($option) 0

    return $result
}


# ------------------------------------------------------------------------------
#  Command Widget::setoption
# ------------------------------------------------------------------------------
proc Widget::setoption { path option value } {
    variable _class

    set class $_class($path)
    upvar 0 ${class}::$path:opt pathopt

    set pathopt($option) $value
}


# ------------------------------------------------------------------------------
#  Command Widget::getoption
# ------------------------------------------------------------------------------
proc Widget::getoption { path option } {
    variable _class

    set class $_class($path)
    upvar 0 ${class}::$path:opt pathopt

    return $pathopt($option)
}


# ------------------------------------------------------------------------------
#  Command Widget::_get_window
#  returns the window corresponding to widget path
# ------------------------------------------------------------------------------
proc Widget::_get_window { class path } {
    set idx [string last "#" $path]
    if { $idx != -1 && ![string compare [string range $path [expr {$idx+1}] end] $class] } {
        return [string range $path 0 [expr {$idx-1}]]
    } else {
        return $path
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::_get_configure
#  returns the configuration list of options
#  (as tk widget do - [$w configure ?option?])
# ------------------------------------------------------------------------------
proc Widget::_get_configure { path options } {
    variable _class

    set class $_class($path)
    upvar 0 ${class}::opt classopt
    upvar 0 ${class}::map classmap
    upvar 0 ${class}::$path:opt pathopt
    upvar 0 ${class}::$path:mod pathmod

    set len [llength $options]
    if { !$len } {
        set result {}
        foreach option [lsort [array names classopt]] {
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
            if { ![string compare $type "Synonym"] } {
                set syn     $option
                set option  [lindex $optdesc 1]
                set optdesc $classopt($option)
                set type    [lindex $optdesc 0]
            } else {
                set syn ""
            }
            if { ![string compare $type "TkResource"] } {
                set alt [lindex [lindex $optdesc 3] 1]
            } else {
                set alt ""
            }
            set res [_configure_option $option $alt]
            if { $syn == "" } {
                lappend result [concat $option $res [list [lindex $optdesc 1]] [list [cget $path $option]]]
            } else {
                lappend result [list $syn [lindex $res 0]]
            }
        }
        return $result
    } elseif { $len == 1 } {
        set option  [lindex $options 0]
        if { ![info exists classopt($option)] } {
            return -code error "unknown option \"$option\""
        }
        set optdesc $classopt($option)
        set type    [lindex $optdesc 0]
        if { ![string compare $type "Synonym"] } {
            set option  [lindex $optdesc 1]
            set optdesc $classopt($option)
            set type    [lindex $optdesc 0]
        }
        if { ![string compare $type "TkResource"] } {
            set alt [lindex [lindex $optdesc 3] 1]
        } else {
            set alt ""
        }
        set res [_configure_option $option $alt]
        return [concat $option $res [list [lindex $optdesc 1]] [list [cget $path $option]]]
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::_configure_option
# ------------------------------------------------------------------------------
proc Widget::_configure_option { option altopt } {
    variable _optiondb
    variable _optionclass

    if { [info exists _optiondb($option)] } {
        set optdb $_optiondb($option)
    } else {
        set optdb [string range $option 1 end]
    }
    if { [info exists _optionclass($option)] } {
        set optclass $_optionclass($option)
    } elseif { [string length $altopt] } {
        if { [info exists _optionclass($altopt)] } {
            set optclass $_optionclass($altopt)
        } else {
            set optclass [string range $altopt 1 end]
        }
    } else {
        set optclass [string range $option 1 end]
    }
    return [list $optdb $optclass]
}


# ------------------------------------------------------------------------------
#  Command Widget::_get_tkwidget_options
# ------------------------------------------------------------------------------
proc Widget::_get_tkwidget_options { tkwidget } {
    variable _tk_widget
    variable _optiondb
    variable _optionclass

    if { ![info exists _tk_widget($tkwidget)] } {
        set widget [$tkwidget ".#BWidget#$tkwidget"]
        set config [$widget configure]
        foreach optlist $config {
            set opt [lindex $optlist 0]
            if { [llength $optlist] == 2 } {
                set refsyn [lindex $optlist 1]
                # search for class
                set idx [lsearch $config [list * $refsyn *]]
                if { $idx == -1 } {
                    if { [string index $refsyn 0] == "-" } {
                        # search for option (tk8.1b1 bug)
                        set idx [lsearch $config [list $refsyn * *]]
                    } else {
                        # last resort
                        set idx [lsearch $config [list -[string tolower $refsyn] * *]]
                    }
                    if { $idx == -1 } {
                        # fed up with "can't read classopt()"
                        return -code error "can't find option of synonym $opt"
                    }
                }
                set syn [lindex [lindex $config $idx] 0]
                set def [lindex [lindex $config $idx] 3]
                lappend _tk_widget($tkwidget) [list $opt $syn $def]
            } else {
                set def [lindex $optlist 3]
                lappend _tk_widget($tkwidget) [list $opt $def]
                set _optiondb($opt)    [lindex $optlist 1]
                set _optionclass($opt) [lindex $optlist 2]
            }
        }
    }
    return $_tk_widget($tkwidget)
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_tkresource
# ------------------------------------------------------------------------------
proc Widget::_test_tkresource { option value arg } {
    set tkwidget [lindex $arg 0]
    set realopt  [lindex $arg 1]
    set path     ".#BWidget#$tkwidget"
    set old      [$path cget $realopt]
    $path configure $realopt $value
    set res      [$path cget $realopt]
    $path configure $realopt $old

    return $res
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_bwresource
# ------------------------------------------------------------------------------
proc Widget::_test_bwresource { option value arg } {
    return -code error "bad option type BwResource in widget"
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_synonym
# ------------------------------------------------------------------------------
proc Widget::_test_synonym { option value arg } {
    return -code error "bad option type Synonym in widget"
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_string
# ------------------------------------------------------------------------------
proc Widget::_test_string { option value arg } {
    return $value
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_flag
# ------------------------------------------------------------------------------
proc Widget::_test_flag { option value arg } {
    set len [string length $value]
    set res ""
    for {set i 0} {$i < $len} {incr i} {
        set c [string index $value $i]
        if { [string first $c $arg] == -1 } {
            return -code error "bad [string range $option 1 end] value \"$value\": characters must be in \"$arg\""
        }
        if { [string first $c $res] == -1 } {
            append res $c
        }
    }
    return $res
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_enum
# ------------------------------------------------------------------------------
proc Widget::_test_enum { option value arg } {
    if { [lsearch $arg $value] == -1 } {
        set last [lindex   $arg end]
        set sub  [lreplace $arg end end]
        if { [llength $sub] } {
            set str "[join $sub ", "] or $last"
        } else {
            set str $last
        }
        return -code error "bad [string range $option 1 end] value \"$value\": must be $str"
    }
    return $value
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_int
# ------------------------------------------------------------------------------
proc Widget::_test_int { option value arg } {
    set binf [lindex $arg 0]
    set bsup [lindex $arg 1]
    if { $binf != "" } {set binf ">$binf"}
    if { $bsup != "" } {set bsup "<$bsup"}
    if { [catch {expr $value}] || $value != int($value) ||
         !($binf == "" || [expr $value$binf]) ||
         !($bsup == "" || [expr $value$bsup]) } {
        return -code error "bad [string range $option 1 end] value \"$value\": must be integer $binf $bsup"
    }
    return $value
}


# ------------------------------------------------------------------------------
#  Command Widget::_test_boolean
# ------------------------------------------------------------------------------
proc Widget::_test_boolean { option value arg } {
    if { $value == 1 ||
         ![string compare $value "true"] ||
         ![string compare $value "yes"] } {
        set value 1
    } elseif { $value == 0 ||
               ![string compare $value "false"] ||
               ![string compare $value "no"] } {
        set value 0
    } else {
        return -code error "bad [string range $option 1 end] value \"$value\": must be boolean"
    }
    return $value
}


# ------------------------------------------------------------------------------
#  Command Widget::focusNext
#  Same as tk_focusNext, but call Widget::focusOK
# ------------------------------------------------------------------------------
proc Widget::focusNext { w } {
    set cur $w
    while 1 {

	# Descend to just before the first child of the current widget.

	set parent $cur
	set children [winfo children $cur]
	set i -1

	# Look for the next sibling that isn't a top-level.

	while 1 {
	    incr i
	    if {$i < [llength $children]} {
		set cur [lindex $children $i]
		if {[winfo toplevel $cur] == $cur} {
		    continue
		} else {
		    break
		}
	    }

	    # No more siblings, so go to the current widget's parent.
	    # If it's a top-level, break out of the loop, otherwise
	    # look for its next sibling.

	    set cur $parent
	    if {[winfo toplevel $cur] == $cur} {
		break
	    }
	    set parent [winfo parent $parent]
	    set children [winfo children $parent]
	    set i [lsearch -exact $children $cur]
	}
	if {($cur == $w) || [focusOK $cur]} {
	    return $cur
	}
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::focusPrev
#  Same as tk_focusPrev, but call Widget::focusOK
# ------------------------------------------------------------------------------
proc Widget::focusPrev { w } {
    set cur $w
    while 1 {

	# Collect information about the current window's position
	# among its siblings.  Also, if the window is a top-level,
	# then reposition to just after the last child of the window.
    
	if {[winfo toplevel $cur] == $cur}  {
	    set parent $cur
	    set children [winfo children $cur]
	    set i [llength $children]
	} else {
	    set parent [winfo parent $cur]
	    set children [winfo children $parent]
	    set i [lsearch -exact $children $cur]
	}

	# Go to the previous sibling, then descend to its last descendant
	# (highest in stacking order.  While doing this, ignore top-levels
	# and their descendants.  When we run out of descendants, go up
	# one level to the parent.

	while {$i > 0} {
	    incr i -1
	    set cur [lindex $children $i]
	    if {[winfo toplevel $cur] == $cur} {
		continue
	    }
	    set parent $cur
	    set children [winfo children $parent]
	    set i [llength $children]
	}
	set cur $parent
	if {($cur == $w) || [focusOK $cur]} {
	    return $cur
	}
    }
}


# ------------------------------------------------------------------------------
#  Command Widget::focusOK
#  Same as tk_focusOK, but handles -editable option and whole tags list.
# ------------------------------------------------------------------------------
proc Widget::focusOK { w } {
    set code [catch {$w cget -takefocus} value]
    if { $code == 1 } {
        return 0
    }
    if {($code == 0) && ($value != "")} {
	if {$value == 0} {
	    return 0
	} elseif {$value == 1} {
	    return [winfo viewable $w]
	} else {
	    set value [uplevel \#0 $value $w]
            if {$value != ""} {
		return $value
	    }
        }
    }
    if {![winfo viewable $w]} {
	return 0
    }
    set code [catch {$w cget -state} value]
    if {($code == 0) && ($value == "disabled")} {
	return 0
    }
    set code [catch {$w cget -editable} value]
    if {($code == 0) && !$value} {
        return 0
    }

    set top [winfo toplevel $w]
    foreach tags [bindtags $w] {
        if { [string compare $tags $top]  &&
             [string compare $tags "all"] &&
             [regexp Key [bind $tags]] } {
            return 1
        }
    }
    return 0
}
