#!/bin/sh
#
# This script selects a square area using the mouse
#
# This program is free software under the GPL (>=v2)
# Read the COPYING file that comes with GRASS for details.
#

#%Module
#% description: Select a rectangular area
#%End
#%option
#% key: raster
#% type: string
#% gisprompt: old,cell,raster
#% description: Raster map to to analyse
#% required: yes
#%end
#%option
#% key: vector
#% type: string
#% gisprompt: old,vector,vector
#% description: Vector map to overlay
#% required: no
#%end
#%option
#% key: site
#% type: string
#% description: Vector points map to overlay
#% required: no
#%end
#%option
#% key: config
#% type: string
#% gisprompt: new,file,file
#% description: Name of configuration file where inserted areas will be stored
#% required: yes
#%end
#%option
#% key: north
#% type: string
#% description: Northern edge (use only with the 'f' flag)
#% required: no
#%end
#%option
#% key: south
#% type: string
#% description: Southern edge (use only with the 'f' flag)
#% required: no
#%end
#%option
#% key: east
#% type: string
#% description: Eastern edge (use only with the 'f' flag)
#% required: no
#%end
#%option
#% key: west
#% type: string
#% description: Western edge (use only with the 'f' flag)
#% required: no
#%end
#%flag
#% key: f
#% description: Sample frame selected by module options
#%end

f_path="$GISBASE/etc/r.li.setup"

# Check if we are in a GRASS session
if test "$GISBASE" = ""; then
   echo "You must be in GRASS GIS to run this program." 1>&2
   exit 1
 fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
   exec g.parser "$0" "$@"
fi

#### create temporary file
TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "$TMP" ] ; then
    echo "ERROR: unable to create temporary file" 1>&2
    exit 1
fi

cleanup()
{
   # remove temporary region
   eval `g.findfile elem=windows file="tmp_rli_sq.$$" | grep '^name='`
   if [ -n "$name" ] ; then
      g.region region="tmp_rli_sq.$$"
      g.remove region="tmp_rli_sq.$$" --quiet
   fi

   rm -f "$TMP" "$TMP.var"
}
trap "cleanup" 2 3 15


# find a free Xmonitor
XMON=x1
for i in 1 2 3 4 5 6 7 ; do
   result=`d.mon -L | grep -w "^x$i"`
   if [ `echo "$result" | grep -c 'not'` -eq 1 ] ; then
      XMON="x$i"
      break
   fi
done

d.mon start="$XMON" --quiet


# setup internal region
#  (WIND_OVERRIDE is already present by the main r.li.setup script)
g.region save="tmp_rli_sq.$$"
g.region rast="$GIS_OPT_raster"

# store starting values
eval `g.region -g`
s_n="$n"
s_s="$s"
s_e="$e"
s_w="$w"
s_nsres="$nsres"
s_ewres="$ewres"

echo "START $s_n|$s_s|$s_e|$s_w|$s_nsres|$s_ewres" >> "$GIS_OPT_conf"

# show the sampling frame
if [ "$GIS_FLAG_f" -eq 1 ] ; then
    g.region n="$GIS_OPT_north" s="$GIS_OPT_south" \
	     e="$GIS_OPT_east" w="$GIS_OPT_west"
fi

d.rast -o map="$GIS_OPT_raster" --quiet

if [ -n "$GIS_OPT_vector" ] ; then
    d.vect map="$GIS_OPT_vector" type=area fcolor=none width=2
fi
if [ -n "$GIS_OPT_site" ] ; then
    d.vect map="$GIS_OPT_site" color=black fcolor=black size=9 icon=basic/circle
    d.vect map="$GIS_OPT_site" color=red fcolor=red size=5 icon=basic/circle
fi

# have the user selected the area of interest with the mouse

# TODO: popup message? (d.menu: "Draw box now: [ok]")
######
#d.menu bcolor=aqua tcolor=black << EOF
#.T 20
#.L 20
#Next select area with mouse
#          [ Ok ]
#EOF
######

# note if user right clicks to quit without zooming the whole frame is used.
d.zoom


# ask if the selected area is right
name="$TMP.var"  # temp file where the answer is written to by the tcl pop-up
export name

# ask if it's ok, save 0,1 to the "$name" tmp file
"$GRASS_WISH" "$f_path/square_query"

if [ -e "$name" ] ; then
   read ok < "$name"
else
   ok="-999"
fi

if [ "$ok" -eq 0 ] ; then
    echo "NO" >> "$GIS_OPT_conf"
elif [ "$ok" -eq 1 ] ; then
    # write the square boundaries
    # TODO: is range (0-1, 0-1) valid or a bug?
    eval `g.region -g`
    echo "SQUAREAREA $n|$s|$e|$w|$nsres|$ewres" >> "$GIS_OPT_conf"
else
    g.message -e "Unable to ascertain if the selected area was ok or not"
fi

# close the Xmonitor
d.mon stop="$XMON" --quiet

# clean tmp files and temporary region
cleanup
