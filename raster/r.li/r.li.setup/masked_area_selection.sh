#!/bin/sh
#
# Masked area selection using r.digit
#

#%Module
#%description: Select a circular or polygonal area
#%End
#%option
#% key: raster
#% type: string
#% gisprompt: old,cell,raster
#% key_desc: name
#% description: Raster map to to analyse
#% required: yes
#%end
#%option
#% key: vector
#% type: string
#% gisprompt: old,vector,vector
#% key_desc: name
#% description: Vector map to overlay
#% required: no
#%end
#%option
#% key: site
#% type: string
#% gisprompt: old,vector,vector
#% key_desc: name
#% description: Vector points map to overlay
#% required: no
#%end
#%option
#% key: config
#% type: string
#% gisprompt: new_file,file,output
#% key_desc: filename
#% description: Name of configuration file where areas are to be saved
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
#% description: Sample frame not yet selected, set from module options
#%end
#%flag
#% key: c
#% description: Take a circular area
#%end

# Where to find the others scripts
f_path="$GISBASE/etc/r.li.setup"

# Check if we are in a GRASS session
if test "$GISBASE" = ""; then
   echo "You must be in GRASS GIS to run this program." 1>&2
   exit 1
 fi
if [ "$1" != "@ARGS_PARSED@" ] ; then
   exec g.parser "$0" "$@"
fi


#### environment variables
GISDBASE=`g.gisenv get=GISDBASE`
LOCATION=`g.gisenv get=LOCATION_NAME`
MAPSET=`g.gisenv get=MAPSET`
: ${GISDBASE?} ${LOCATION?} ${MAPSET?}


#### set temporary files
TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "$TMP" ] ; then
    echo "ERROR: unable to create temporary files" 1>&2
    exit 1
fi

cleanup()
{
   # remove temporary region
   eval `g.findfile elem=windows file="tmp_rli_selmsk.$$" | grep '^name='`
   if [ -n "$name" ] ; then
      g.region region="tmp_rli_selmsk.$$"
      g.remove region="tmp_rli_selmsk.$$" --quiet
   fi

   rm -f "$TMP" "$TMP".val
}
trap "cleanup" 2 3 15


# setup internal region
#  (WIND_OVERRIDE is already present by the main r.li.setup script)
g.region save="tmp_rli_selmsk.$$"

# ?show the sampling frame
if [ "$GIS_FLAG_f" -eq 1 ] ; then
    g.region n="$GIS_OPT_north" s="$GIS_OPT_south" \
	     e="$GIS_OPT_east" w="$GIS_OPT_west"
else
    g.region rast="$GIS_OPT_raster"
fi

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

d.rast -o map="$GIS_OPT_raster" --quiet

if [ -n "$GIS_OPT_vector" ] ; then
    d.vect map="$GIS_OPT_vector" type=area fcolor=none width=2
fi
if [ -n "$GIS_OPT_site" ] ; then
    d.vect map="$GIS_OPT_site" color=black fcolor=black size=9 icon=basic/circle
    d.vect map="$GIS_OPT_site" color=red fcolor=red size=5 icon=basic/circle
fi

# setup for drawing area
if [ "$GIS_FLAG_c" -eq 1 ] ; then
   RDIG_INSTR="$f_path/circle.txt"
   STYLE="circle"
else
   RDIG_INSTR="$f_path/polygon.txt"
   STYLE="polygon"
fi

# feed options to r.digit
r.digit output="tmp_rli_mask.$$" --quiet < "$RDIG_INSTR"


# show the selected area
d.rast -o map="tmp_rli_mask.$$" --quiet

name="$TMP.val"
export name

# ask if it's ok, save 0,1 to "$name" temp file
"$GRASS_WISH" "$f_path/area_query"

ok=`cat "$name" | cut -f1 -d ' '`
r_name=`cat "$name" | cut -f2 -d' '`


if [ "$ok" -eq 1 ] ; then
    mask_name="rli_samp_${STYLE}_${r_name}"
    # r.mask + 'g.region zoom= align=' + 'r.mapcalc cropmap=map' would be cleaner?
    r.to.vect input="tmp_rli_mask.$$" output="tmp_rli_mask_v$$" feature=area --quiet
    g.region vect="tmp_rli_mask_v$$"
    v.to.rast input="tmp_rli_mask_v$$" output="$mask_name" use=val value=1  --quiet

    # write info in configuration file
    eval `g.region -g`
    north="$n"
    south="$s"
    east="$e"
    west="$w"

    echo "SAMPLEAREAMASKED $mask_name $north|$south|$east|$west" >> \
	    "$GIS_OPT_conf"

    # remove tmp raster and vector
    g.remove rast="tmp_rli_mask.$$" --quiet
    g.remove vect="tmp_rli_mask_v$$" --quiet

    if [ "$GIS_FLAG_f" -eq 1 ] ; then
    	g.region n="$GIS_OPT_north" s="$GIS_OPT_south" \
    		    e="$GIS_OPT_east" w="$GIS_OPT_west"
    else
    	g.region rast="$GIS_OPT_raster"
    fi

else
    echo 0 >> "$GIS_OPT_conf"
fi


d.mon stop="$XMON" --quiet

# clean tmp files
cleanup
