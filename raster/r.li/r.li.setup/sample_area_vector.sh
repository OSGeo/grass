#!/bin/sh -x
#
# This program is free software under the GPL (>=v2)
# Read the COPYING file that comes with GRASS for details.

#%Module 
#% description: Create sample area from a vector map 
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
#% description: vector map where areas are defined
#% required: yes
#%end
#%option
#% key: config
#% type: string
#% description: name of configuration file where insert areas
#% required: yes
#%end


# Check if we have grass
if test "$GISBASE" = ""; then
 echo "You must be in GRASS GIS to run this program." >&2
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
g.gisenv LOCATION_NAME > $TMP.var
read LOCATION < $TMP.var
g.gisenv GISDBASE > $TMP.var
read GISDBASE < $TMP.var
g.gisenv MAPSET > $TMP.var
read MAPSET < $TMP.var

f_path="$GISBASE/etc/r.li.setup"

##############################################################
#read categories from input vector, extract,
#convert to raster and save the bounds to configuration file
##############################################################

#using v.category instead of v.build with cdump
v.category input=$GIS_OPT_vector option=print > $TMP.cat

#get input vector name
GIS_OPT_input_vector=`echo $GIS_OPT_vector| cut -d'@' -f 1`

#get input vector mapset
GIS_OPT_input_mapset=`echo $GIS_OPT_vector| cut -d'@' -f 2`

#read input vector categories into CAT_LIST array
IFS=$'\r\n' CAT_LIST=($(cat $TMP.cat))

TMP_REGION="reg`g.tempfile pid=$$`"
#save the current region settings temporarily to avoid surpirses later.
g.region save=$TMP_REGION

#process each feature in the vector having category values in the CAT_LIST array
for CAT in "${CAT_LIST[@]}" 
do
    #vector to store a feature fro $GIS_OPT_vector with category value $CAT.
    #This temporary vector will be removed at the end. 
    EXTRACT=$GIS_OPT_input_vector"_"$CAT"_part@"$GIS_OPT_input_mapset
    
    #extract only a part of $GIS_OPT_vector where category = $CAT and store in $EXTRACT
    v.extract input=$GIS_OPT_vector output=$EXTRACT type=point,line,boundary,centroid,area,face new=-1 -d where='CAT='$CAT
    
    #TODO: anyway to check if x1 is in use?
    #opening monitor x1
    d.mon stop=x1
    d.mon start=x1
    #setting region with raster resolution
    g.region vect=$EXTRACT align=$GIS_OPT_raster
    d.rast -o $GIS_OPT_raster    
    #render extracted vector map
    d.vect $EXTRACT
    #ask the user if he wants to analyse this vector and a name for raster
    #in graphical mode using wish
    export name=$TMP.val # where find the answer
    $GRASS_WISH $f_path/area_query 
    cat $name | cut -f1 -d ' ' > $name.var
    read ok < $name.var
    cat $name | cut -f2 -d' ' > $name.var
    r_name=""
    read r_name < $name.var
    echo $r_name

    if [ $ok -eq 1 ] ; then
	#area selected, create mask
	v.to.rast input=$EXTRACT output=$r_name use=cat value=1 rows=4096
	#read the region settings to save to configuration file
	g.region -g| grep "n=" | cut -f2 -d'='> $name.var
	read north < $name.var
	g.region -g| grep "s=" | cut -f2 -d'='  > $name.var
	read south < $name.var
	g.region -g| grep "e=" | cut -f2 -d'=' > $name.var
	read east < $name.var
	g.region -g| grep "w=" | cut -f2 -d'=' > $name.var
	read west < $name.var
	#write info in configuration file
	echo "MASKEDOVERLAYAREA $r_name|$north|$south|$east|$west" >> $GIS_OPT_conf
    fi
    #remove temporary vector map created from v.extract
    g.remove vect=$EXTRACT
    #rm -fR "$GISDBASE"/"$LOCATION"/"$MAPSET"/vector/$GIS_OPT_vector"part"$I
    #echo DROP TABLE $GIS_OPT_vector"part"$I | db.execute 
done
d.mon stop=x1
#restore user region
g.region region=$TMP_REGION
#remove temporary region, $TMP_REGION
g.remove region=$TMP_REGION
# clean tmp files
rm -f $TMP*


