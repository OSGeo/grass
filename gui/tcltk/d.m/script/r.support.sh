#!/bin/sh
#
############################################################################
#
# MODULE:	r.support.sh for GRASS 5.7
# AUTHOR(S):	Michael Barton; 
# PURPOSE:	Runs r.support from GIS Manager GUI
# COPYRIGHT:	(C) 2004 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################


#%Module
#%  description: r.support.sh - Manage raster support files (header, statistics, categories, colors, history, nulls)
#%End
#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Name of raster map for color management
#% required : yes
#%end


if  [ -z "$GISBASE" ] ; then
 echo "You must be in GRASS GIS to run this program."
 exit 1
fi   

if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi

eval `exec "$GISBASE/etc/grass-xterm-wrapper" -e r.support map=$GIS_OPT_INPUT`
