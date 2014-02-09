#!/bin/sh
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
#% key: conf
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
   eval `g.findfile elem=windows file="tmp_rli_sampvect.$$" | grep '^name='`
   if [ -n "$name" ] ; then
      g.region region="tmp_rli_sampvect.$$"
      g.remove region="tmp_rli_sampvect.$$" --quiet
   fi

   rm -f "$TMP"*
}
trap "cleanup" 2 3 15


# setup internal region
#  (WIND_OVERRIDE is already present by the main r.li.setup script)
g.region save="tmp_rli_sampvect.$$"


# find a free Xmonitor
XMON=x1
for i in 1 2 3 4 5 6 7 ; do
   result=`d.mon -L | grep -w "^x$i"`
   if [ `echo "$result" | grep -c 'not'` -eq 1 ] ; then
      XMON="x$i"
      break
   fi
done

# open Xmonitor
d.mon stop="$XMON"
d.mon start="$XMON"


f_path="$GISBASE/etc/r.li.setup"

##############################################################
# read categories from input vector, extract,
# convert to raster and save the bounds to configuration file
##############################################################

# using v.category instead of v.build with cdump
v.category input="$GIS_OPT_vector" option=print | \
  sort -n | uniq > "$TMP.cat"

# crop away @mapset part, if present
input_vector=`echo "$GIS_OPT_vector" | cut -d'@' -f 1`


# process each feature in the vector that has a cat
while read CAT ; do
   # skip blank lines..
    if [ -z "$CAT" ] ; then
       continue
    fi

    # Temporary vector map to store an individual feature from the input vector
    #  It will be removed at the end of the iteration.
    EXTRACT="tmp_$$_${input_vector}_${CAT}"

    v.extract input="$GIS_OPT_vector" output="$EXTRACT" \
       type=point,line,centroid,area \
       new=-1 -d list="$CAT"  # where="CAT = $CAT"

    # set region with raster resolution
    g.region vect="$EXTRACT" align="$GIS_OPT_raster"

    d.erase
    d.rast -o "$GIS_OPT_raster"

    # render extracted vector map  (prehaps fcolor=none for areas?)
    d.vect "$EXTRACT"

    # ask the user to analyse this vector and a name for raster in a Tcl GUI
    name="$TMP.val" # where find the answer
    export name

    # ask if it's ok, save 0,1 to "$name" temp file
    "$GRASS_WISH" "$f_path/area_query"

    ok=`cat "$name" | cut -f1 -d ' '`
    r_name=`cat "$name" | cut -f2 -d' '`

    # debug or needed?
    echo "$r_name"

    if [ "$ok" -eq 1 ] ; then
	#area selected, create mask
	v.to.rast input="$EXTRACT" output="$r_name" use=cat

	# save the region settings into the configuration file
	eval `g.region -g`
	echo "MASKEDOVERLAYAREA $r_name|$n|$s|$e|$w" >> "$GIS_OPT_conf"
    fi

    #remove temporary vector map created from v.extract
    g.remove vect="$EXTRACT" --quiet

done < "$TMP.cat"


d.mon stop="$XMON"


# clean tmp files and restore region
cleanup
