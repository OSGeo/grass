#!/bin/sh
#
############################################################################
#
# MODULE:	print.sh for GRASS 6
# AUTHOR(S):	Michael Barton 
# PURPOSE:	    Print current display monitor on default printer from GIS Manager
# COPYRIGHT:	(C) 2005 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################



if  [ -z "$GISBASE" ] ; then
   echo "You must be in GRASS GIS to run this program."
   exit 1
fi   

d.out.file output=print res=1 format=png >@stdout 2>@stdout
sleep 5
lpr -r print.png



