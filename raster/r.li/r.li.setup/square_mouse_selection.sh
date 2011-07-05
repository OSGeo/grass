#! /bin/sh 

# This program is free software under the GPL (>=v2)
# Read the COPYING file that comes with GRASS for details.

#this script select a square area using mouse

#%Module 
#% description: Select a rectangular area
#%End
#%option
#% key: raster
#% type: string
#% description: raster map to to analyse
#% required: yes
#%end
#%option
#% key: vector
#% type: string
#% description: vector to overlay
#% required: no
#%end
#%option
#% key: site
#% type: string
#% description: site to overlay
#% required: no
#%end
#%option
#% key: config
#% type: string
#% description: name of configuration file where insert areas
#% required: yes
#%end
#%option
#% key: north
#% type: string
#% description: nothern edge (use only with f flag)
#% required: no
#%end
#%option
#% key: south
#% type: string
#% description:south edge (use only with f flag)
#% required: no
#%end
#%option
#% key: east
#% type: string
#% description: east edge(use only with f flag)
#% required: no
#%end
#%option
#% key: west 
#% type: string
#% description: west edge(use only with f flag)
#% required: no
#%end
#%flag
#% key: f
#% description: sample frame yet selected 
#%end

f_path="$GISBASE/etc/r.li.setup"

# Check if we have grass
if test "$GISBASE" = ""; then
 echo "You must be in GRASS GIS to run this program." >&2
 exit 1
 fi
if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi



# open x1 monitor
d.mon stop=x1
d.mon start=x1


g.region rast=$GIS_OPT_raster

#### set temporary files
TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "$TMP" ] ; then
    echo "ERROR: unable to create temporary files" 1>&2
    exit 1
fi

#saving starting values
g.region -g | grep "n=" | cut -f2 -d'=' > $TMP.var
read s_n < $TMP.var
g.region -g | grep "s=" | cut -f2 -d'=' > $TMP.var
read s_s < $TMP.var
g.region -g | grep "e=" | cut -f2 -d'=' > $TMP.var
read s_e < $TMP.var
g.region -g | grep "w=" | cut -f2 -d'=' > $TMP.var
read s_w < $TMP.var
g.region -g | grep "nsres=" | cut -f2 -d'=' > $TMP.var
read s_nsres < $TMP.var
g.region -g | grep "ewres=" | cut -f2 -d'=' > $TMP.var
read s_ewres < $TMP.var
echo "START $s_n|$s_s|$s_e|$s_w|$s_nsres|$s_ewres" >> $GIS_OPT_conf

# show the sampling frame
if [ $GIS_FLAG_f -eq 1 ] ; then
    g.region n=$GIS_OPT_north s=$GIS_OPT_south e=$GIS_OPT_east w=$GIS_OPT_west
fi

d.rast -o map=$GIS_OPT_raster
if [ -n "$GIS_OPT_vector" ] ; then
    d.vect  map=$GIS_OPT_vector
fi
if [ -n "$GIS_OPT_site" ] ; then 
    d.vect  map=$GIS_OPT_site
fi

#let draw area
d.zoom

#ask if the selected area is right
export name=$TMP #where write the answer
$GRASS_WISH $f_path/square_query
read ok < $TMP.var
if [ $ok -eq 0 ] ; then
    echo "NO" >> $GIS_OPT_conf
fi

if [ $ok -eq 1 ] ; then
    #write the square boundaries
    g.region -g | grep "n=" | cut -f2 -d'=' > $TMP.var
    read n < $TMP.var
    g.region -g | grep "s=" | cut -f2 -d'=' > $TMP.var
    read s < $TMP.var
    g.region -g | grep "e=" | cut -f2 -d'=' > $TMP.var
    read e < $TMP.var
    g.region -g | grep "w=" | cut -f2 -d'=' > $TMP.var
    read w < $TMP.var
    g.region -g | grep "nsres=" | cut -f2 -d'=' > $TMP.var
    read nsres < $TMP.var
    g.region -g | grep "ewres=" | cut -f2 -d'=' > $TMP.var
    read ewres < $TMP.var
    echo "SQUAREAREA $n|$s|$e|$w|$nsres|$ewres" >> $GIS_OPT_conf
fi

#close monitor
d.mon stop=x1
# clean tmp files
rm -f $TMP*




