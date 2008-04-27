#!/bin/sh
#
############################################################################
#
# MODULE:	d.text.sh for GRASS 6
# AUTHOR(S):	Michael Barton 
# PURPOSE:	    Make module d.text useable from the GUI
# COPYRIGHT:	(C) 2005 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################


#%Module
#%  description: d.text.sh - Interactively create text in active display
#%End
#%option
#% key: size
#% type: double
#% description: type point size (default 5 points)
#% answer: 5
#% options: 1-100
#% required : no
#%end
#%option
#% key: color
#% type: string
#% description: Text color (standard GRASS color or r:g:b triplet - default grey)
#% required : no
#%end
#%option
#% key: line
#% type: integer
#% description: Screen line where text will be drawn (1-1000)
#% options: 1-1000
#% required : no
#%end
#%option
#% key: at
#% type: double
#% description: Screen location for text (in percentage from left,bottom [0,0])
#% options: 0-100
#% required : no
#%end
#% flag
#% key: b
#% description: bold text
#%end

if  [ -z "$GISBASE" ] ; then
   echo "You must be in GRASS GIS to run this program."
   exit 1
fi   

if [ "$1" != "@ARGS_PARSED@" ] ; then
   exec g.parser "$0" "$@"
fi

cmd="d.text"

if [ -n "$GIS_OPT_SIZE" ] ; then
    cmd="$cmd size=$GIS_OPT_SIZE"
fi

if [ -n "$GIS_OPT_COLOR" ] ; then
    cmd="$cmd color=$GIS_OPT_COLOR"
fi

if [ -n "$GIS_OPT_LINE" ] ; then
    cmd="$cmd line=$GIS_OPT_LINE"
fi

if [ -n "$GIS_OPT_AT" ] ; then
    cmd="$cmd at=$GIS_OPT_AT"
fi

if [ $GIS_FLAG_B -eq 1 ] ; then
    cmd="$cmd -b"
fi

echo $cmd

eval `exec "$GISBASE/etc/grass-xterm-wrapper" -e $cmd`

