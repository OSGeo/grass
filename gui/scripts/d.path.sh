#!/bin/sh
############################################################################
#
# MODULE:       d.path_wrapper.sh
# AUTHOR(S):    Hamish Bowman  (Otago University, New Zealand)
# PURPOSE:      Draw vector map to xmon before running d.path and pass opts
# COPYRIGHT:    (c) 2007 by Hamish Bowman, and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
# Notes:
#   Created with "d.path --script" from GRASS 6.3-CVS 6 March 2007
#   Be sure menu entry runs guarantee_xmon first. for example:
#     "guarantee_xmon; d.path_wrapper.sh -b --ui"
#   d.path is interactive with the xmon, but does not need to run in a xterm
#############################################################################

#%Module
#% description: Find shortest path for selected starting and ending node
#% keywords: display, networking
#%End
#%Flag
#% key: g
#% description: Use geodesic calculation for longitude-latitude locations
#%End
#%Flag
#% key: b
#% description: Render bold lines
#% guisection: Rendering
#%End
#%Option
#% key: map
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name of input vector map
#% gisprompt: old,vector,vector
#%End
#%Option
#% key: type
#% type: string
#% required: no
#% multiple: yes
#% options: line,boundary
#% label: Type
#% description: Arc type
#% answer: line,boundary
#%End
#%Option
#% key: coor
#% type: string
#% required: no
#% multiple: no
#% key_desc: x1,y1,x2,y2
#% description: Starting and ending coordinates
#%End
#%Option
#% key: alayer
#% type: integer
#% required: no
#% multiple: no
#% label: Layer number
#% description: Arc layer
#% answer: 1
#%End
#%Option
#% key: nlayer
#% type: integer
#% required: no
#% multiple: no
#% label: Layer number
#% description: Node layer
#% answer: 2
#%End
#%Option
#% key: afcol
#% type: string
#% required: no
#% multiple: no
#% description: Arc forward/both direction(s) cost column
#%End
#%Option
#% key: abcol
#% type: string
#% required: no
#% multiple: no
#% description: Arc backward direction cost column
#%End
#%Option
#% key: ncol
#% type: string
#% required: no
#% multiple: no
#% description: Node cost column
#%End
#%Option
#% key: color
#% type: string
#% required: no
#% multiple: no
#% description: Original line color
#% answer: black
#% gisprompt: color,grass,color
#% guisection: Rendering
#%End
#%Option
#% key: hcolor
#% type: string
#% required: no
#% multiple: no
#% description: Highlight color
#% answer: red
#% gisprompt: color,grass,color
#% guisection: Rendering
#%End
#%Option
#% key: bgcolor
#% type: string
#% required: no
#% multiple: no
#% description: Background color
#% answer: white
#% gisprompt: color,grass,color
#% guisection: Rendering
#%End

if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
    exec g.parser "$0" "$@"
fi

d.vect map="$GIS_OPT_MAP" color="$GIS_OPT_COLOR"


if [ "$GIS_FLAG_G" -eq 1 ] ; then
    flag_g="-g"
else
    flag_g=""
fi
if [ "$GIS_FLAG_B" -eq 1 ] ; then
    flag_b="-b"
else
    flag_b=""
fi

if [ -n "$GIS_OPT_COOR" ] ; then
    opt_coor="coor=$GIS_OPT_COOR"
else
    opt_coor=""
fi
if [ -n "$GIS_OPT_AFCOL" ] ; then
    opt_afcol="afcol=$GIS_OPT_AFCOL"
else
    opt_afcol=""
fi
if [ -n "$GIS_OPT_ABCOL" ] ; then
    opt_abcol="abcol=$GIS_OPT_ABCOL"
else
    opt_abcol=""
fi
if [ -n "$GIS_OPT_NCOL" ] ; then
    opt_ncol="ncol=$GIS_OPT_NCOL"
else
    opt_ncol=""
fi


exec d.path $flag_g $flag_b \
	map="$GIS_OPT_MAP" \
	type="$GIS_OPT_TYPE" \
	$opt_coor \
	alayer="$GIS_OPT_ALAYER" \
	nlayer="$GIS_OPT_NLAYER" \
	$opt_afcol $opt_abcol $opt_ncol \
	color="$GIS_OPT_COLOR" \
	hcolor="$GIS_OPT_HCOLOR" \
	bgcolor="$GIS_OPT_BGCOLOR"

