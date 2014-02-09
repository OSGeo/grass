#!/bin/sh -x
#
# This program is free software under the GPL (>=v2)
# Read the COPYING file that comes with GRASS for details.
#

#%Module
#% description: Create sample area from a vector map
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
#% description: Vector map where areas are defined
#% required: yes
#%end
#%option
#% key: config
#% type: string
#% key_desc: name
#% gisprompt: new_file,file,output
#% description: Name of configuration file where areas are to be saved
#% required: yes
#%end


# Check if we are in a GRASS session
if [ -z "$GISBASE" ] ; then
   echo "You must be in GRASS GIS to run this program." 1>&2
   exit 1
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
   exec g.parser "$0" "$@"
fi

#### set temporary files
TMP="`g.tempfile pid=$$`"
if [ $? -ne 0 ] || [ -z "$TMP" ] ; then
   echo "ERROR: unable to create temporary files" 1>&2
   exit 1
fi

#### environment variables
GISDBASE=`g.gisenv get=GISDBASE`
LOCATION=`g.gisenv get=LOCATION_NAME`
MAPSET=`g.gisenv get=MAPSET`
: ${GISDBASE?} ${LOCATION?} ${MAPSET?}

f_path="$GISBASE/etc/r.li.setup"

##############################################################
# read categories from input vector, extract,
# convert to raster and save the bounds to configuration file
##############################################################

# using v.category instead of v.build with cdump
v.category input=$GIS_OPT_vector option=print | sort | uniq > "$TMP.cat"

# get input vector name
GIS_OPT_input_vector=`echo $GIS_OPT_vector| cut -d'@' -f 1`

# get input vector mapset
GIS_OPT_input_mapset=`echo $GIS_OPT_vector| cut -d'@' -f 2`

# read input vector categories into CAT_LIST array
IFS=$'\r\n' CAT_LIST=($(cat $TMP.cat))

# save the current region settings temporarily to avoid surpirses later.
TMP_REGION="tmp_rlisetup.sampvect.$$"
g.region save="$TMP_REGION"

# find a free Xmonitor
XMON=x1
for i in 1 2 3 4 5 6 7 ; do
   result=`d.mon -L | grep -w "^x$i"`
   if [ `echo "$result" | grep -c 'not'` -eq 1 ] ; then
      XMON="x$i"
      break
   fi
done


# process each feature in the vector having category values in the CAT_LIST array
for CAT in "${CAT_LIST[@]}"
do
    # vector to store a feature fro $GIS_OPT_vector with category value $CAT.
    # This temporary vector will be removed at the end.
    EXTRACT=$GIS_OPT_input_vector"_"$CAT"_part@"$GIS_OPT_input_mapset

    # extract only a part of $GIS_OPT_vector where category = $CAT and store in $EXTRACT
    v.extract input=$GIS_OPT_vector output=$EXTRACT \
       type=point,line,boundary,centroid,area,face \
       new=-1 -d where='CAT='$CAT

    # open Xmonitor
    d.mon stop="$XMON"
    d.mon start="$XMON"

    # set region with raster resolution
    g.region vect="$EXTRACT" align="$GIS_OPT_raster"
    d.rast -o "$GIS_OPT_raster"

    # render extracted vector map
    d.vect "$EXTRACT"

    # ask the user to analyse this vector and a name for raster in a Tcl GUI
    name="$TMP.val" # where find the answer
    export name

    "$GRASS_WISH" "$f_path/area_query"

    ok=`cat "$name" | cut -f1 -d ' '`
    #cat "$name" | cut -f1 -d ' ' > "$name.var"
    #read ok < "$name.var"
    r_name=`cat "$name" | cut -f2 -d' '`
    #cat "$name" | cut -f2 -d' ' > "$name.var"
    #r_name=""
    #read r_name < "$name.var"
    echo "$r_name"

    if [ "$ok" -eq 1 ] ; then
	#area selected, create mask
	v.to.rast input="$EXTRACT" output="$r_name" use=cat value=1 rows=4096

	# save the region settings into the configuration file
	eval `g.region -g`
	echo "MASKEDOVERLAYAREA $r_name|$n|$s|$e|$w" >> "$GIS_OPT_conf"
    fi

    #remove temporary vector map created from v.extract
    g.remove vect="$EXTRACT"
    #rm -fR "$GISDBASE"/"$LOCATION"/"$MAPSET"/vector/$GIS_OPT_vector"part"$I
    #echo DROP TABLE $GIS_OPT_vector"part"$I | db.execute
done

d.mon stop="$XMON"

# restore the region (which itself is a WIND_OVERRIDE temporary region by
#  the r.li.setup main script)
g.region region="$TMP_REGION"
g.remove region="$TMP_REGION"

# clean tmp files
rm -f "$TMP"*
