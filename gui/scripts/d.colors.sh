#!/bin/sh
#
############################################################################
#
# MODULE:	d.colors.sh for GRASS 6
# AUTHOR(S):	Michael Barton 
# PURPOSE:	    Make xterm module d.colors useable from the GUI
# COPYRIGHT:	(C) 2005 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################


#%Module
#%  description: d.colors.sh - Interactively modify color table for integer raster map
#%End
#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Name of raster map with color table to modify
#% required : yes
#%end


if  [ -z "$GISBASE" ] ; then
 echo "You must be in GRASS GIS to run this program."
 exit 1
fi   

if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi

exec "$GISBASE/etc/grass-xterm-wrapper" -e "$GISBASE/etc/grass-run.sh" d.colors "map=$GIS_OPT_INPUT"
