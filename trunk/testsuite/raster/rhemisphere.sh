#!/bin/sh

# Markus Neteler, 2006
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
# Test cases for 2D raster data
# generate a hemisphere to test slope, aspect, curvatures

# some definitions:
BOXLENGTH=1000  # side length of test area
RADIUS=500      # half BOXLENGTH

############

if  [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." >&2
    exit 1
fi

# some functions - keep order here
TMP="disk.$$"

cleanup()
{
 echo "Removing temporary map"
 g.remove --q -f type=raster name=$TMP > /dev/null
}

########################

g.region n=$BOXLENGTH s=0 w=0 e=$BOXLENGTH -p res=1

X="(col() - $RADIUS)"
Y="($RADIUS - row())"
r="sqrt($X^2 + $Y^2)"

#Mask out unwanted parts (check for <= ??):
r.mapcalc "$TMP = if($r<$RADIUS,$r,null())"

ALPHA="acos ($TMP/$RADIUS)"
HEIGHT="$RADIUS * sin($ALPHA)"


r.mapcalc "hemisphere = $HEIGHT"
cleanup
g.message "Generated raster map <hemisphere>"
#echo "Now generate aspect + slope on <hemisphere>"

