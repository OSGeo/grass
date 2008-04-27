#!/bin/sh
#
############################################################################
#
# MODULE:	d.title.sh for GRASS 6
# AUTHOR(S):	Michael Barton 
# PURPOSE:	    Use d.title and d.text to display raster map title
# COPYRIGHT:	(C) 2005 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################


#%Module
#%  description: d.title.sh - Display title for raster map in active display
#%End
#%option
#% key: map
#% type: string
#% gisprompt: old,cell,raster
#% description: Name of raster map
#% required : yes
#%end
#%option
#% key: size
#% type: double
#% description: type size as percent of display monitor (default 4.0)
#% required : no
#%end
#%option
#% key: color
#% type: string
#% description: Text color (default black)
#% options: black,red,orange,yellow,green,aqua,blue,indigo,violet,magenta,brown,white,grey
#% answer: black
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

cmd1="d.title map=$GIS_OPT_MAP"
cmd2="d.text"

if [ -n "$GIS_OPT_SIZE" ] ; then
    cmd1="$cmd1 size=$GIS_OPT_SIZE"
fi

if [ -n "$GIS_OPT_COLOR" ] ; then
    cmd1="$cmd1 color=$GIS_OPT_COLOR"
fi

if [ -n "$GIS_OPT_LINE" ] ; then
    cmd2="$cmd2 line=$GIS_OPT_LINE"
fi

if [ $GIS_FLAG_B -eq 1 ] ; then
    cmd2="$cmd2 -b"
fi

eval `$cmd1 | $cmd2`
