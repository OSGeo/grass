#! /bin/sh
#############################################################################
#
# MODULE:   	GRASS initialization (Shell)
# AUTHOR(S):	Original author unknown - probably CERL
#               Andreas Lange - Germany - andreas.lange@rhein-main.de
#   	    	Huidae Cho - Korea - grass4u@gmail.com
#   	    	Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
#   	    	Markus Neteler - Germany/Italy - neteler@itc.it
#		Hamish Bowman - New Zealand - hamish_b at yahoo,com
# PURPOSE:  	Sets up some environment variables.
#               It also parses any remaining command line options for
#               setting the GISDBASE, LOCATION, and/or MAPSET.
#               Finally it starts GRASS with the appropriate user
#   	    	interface and cleans up after it is finished.
# COPYRIGHT:    (C) 2000-2010 by the GRASS Development Team
#
#               This program is free software under the GNU General
#   	    	Public License (>=v2). Read the file COPYING that
#   	    	comes with GRASS for details.
#
#############################################################################

trap "echo 'User break!' ; exit" 2 3 9 15

if [ -z "$GRASS_PYTHON" ] ; then
    GRASS_PYTHON=python
fi
export GRASS_PYTHON

exec "$GRASS_PYTHON" "@BINDIR@/grass70.py" "$@" &
