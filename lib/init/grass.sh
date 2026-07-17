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
# SPDX-FileCopyrightText: 2000-2026 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

trap "echo 'User break!' ; exit" 2 3 9 15

if [ -z "$GRASS_PYTHON" ] ; then
    GRASS_PYTHON=python3
fi
export GRASS_PYTHON

exec "$GRASS_PYTHON" "@BINDIR@/grass.py" "$@" &
