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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
# COPYRIGHT:    (C) 2000-2024 by the GRASS Development Team
=======
# COPYRIGHT:    (C) 2000-2022 by the GRASS Development Team
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
# COPYRIGHT:    (C) 2000-2022 by the GRASS Development Team
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
# COPYRIGHT:    (C) 2000-2023 by the GRASS Development Team
>>>>>>> 021dfb5d52 (r.terrafow: explicit use of default constructors (#2660))
=======
# COPYRIGHT:    (C) 2000-2023 by the GRASS Development Team
>>>>>>> 6104ec7096 (i.maxlik: fix crash when classification result is NULL (#2724))
#
#               This program is free software under the GNU General
#   	    	Public License (>=v2). Read the file COPYING that
#   	    	comes with GRASS for details.
#
#############################################################################

trap "echo 'User break!' ; exit" 2 3 9 15

if [ -z "$GRASS_PYTHON" ] ; then
    GRASS_PYTHON=python3
fi
export GRASS_PYTHON

exec "$GRASS_PYTHON" "@BINDIR@/grass.py" "$@" &
