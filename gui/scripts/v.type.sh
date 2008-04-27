#!/bin/sh
############################################################################
#
# MODULE:       v.type.sh (v.type wrapper script)
# AUTHOR(S):    Hamish Bowman  (Otago University, New Zealand)
# PURPOSE:      Supply v.type options in a GUI compatible way
# COPYRIGHT:    (c) 2007 by Hamish Bowman, and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
# Notes:
#   Created with "v.type --script" from GRASS 6.3-CVS 23 May 2007

#%Module
#% description: Change the type of geometry elements.
#% keywords: vector, geometry
#%End
#%Option
#% key: input
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name of input vector map
#% gisprompt: old,vector,vector
#%End
#%Option
#% key: output
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name for output vector map
#% gisprompt: new,vector,vector
#%End
#%Option
#% key: type
#% type: string
#% required: no
#% multiple: no
#% options: point to centroid,point to kernel,centroid to point,centroid to kernel,kernel to point,kernel to centroid,line to boundary,line to face,boundary to line,boundary to face,face to line,face to boundary
#% description: Conversion
#% answer: boundary to line
#%End


if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
    exec g.parser "$0" "$@"
fi

unset TYPE_CNV

case "$GIS_OPT_TYPE" in
  "point to centroid")
    TYPE_CNV="point,centroid" ;;
  "centroid to point")
    TYPE_CNV="centroid,point" ;;
  "line to boundary")
    TYPE_CNV="line,boundary" ;;
  "boundary to line")
    TYPE_CNV="boundary,line" ;;
  "kernel to centroid")
    TYPE_CNV="kernel,centroid" ;;
  "centroid to kernel")
    TYPE_CNV="centroid,kernel" ;;
  "face to boundary")
    TYPE_CNV="face,boundary" ;;
  "boundary to face")
    TYPE_CNV="boundary,face" ;;
  "point to kernel")
    TYPE_CNV="point,kernel" ;;
  "kernel to point")
    TYPE_CNV="kernel,point" ;;
  "line to face")
    TYPE_CNV="line,face" ;;
  "face to line")
    TYPE_CNV="face,line" ;;
esac

exec v.type input="$GIS_OPT_INPUT" output="$GIS_OPT_OUTPUT" type="$TYPE_CNV"
