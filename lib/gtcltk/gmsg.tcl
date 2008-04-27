#############################################################################
#
# gmsg.tcl
#
# MODULE:   	Grass Tcl/Tk I18n wrapper
# AUTHOR(S):	Alex Shevlakov alex@motivation.ru
# PURPOSE:  	I18N Tcl-Tk based GUI text strings wrapper procedure
#
# COPYRIGHT:    (C) 2000 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#   	    	License (>=v2). Read the file COPYING that comes with GRASS
#   	    	for details.
#
#############################################################################


if [catch {package require msgcat}] {
	proc G_msg {message} {
		return $message
	}
} else {
	::msgcat::mcload $env(GISBASE)/etc/msgs
	proc G_msg {message} {
		return [::msgcat::mc $message]
	}
}
