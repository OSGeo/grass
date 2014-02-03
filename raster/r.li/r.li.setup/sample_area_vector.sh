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

# list categories
v.build map=$GIS_OPT_vector option=cdump| grep '|' |cut -f1 -d '|' |\
    tr ' ' -d > $TMP.cat
# number of categories
N="`grep -c "" < $TMP.cat`"
((N= N -3))

#scanning categories
I=1
while(($I<=$N)); do
    #extract vector[i]
    v.extract input=$GIS_OPT_vector output=$GIS_OPT_vector"part"$I \
	type=point,line,boundary,centroid,area,face new=-1 -d where="(CAT=$I)"
    #opening monitor x1
    d.mon stop=x1
    d.mon start=x1
    #setting region with raster resolution
    g.region vect=$GIS_OPT_vector"part"$I align=$GIS_OPT_raster
    d.rast -o $GIS_OPT_raster
    d.vect $GIS_OPT_vector"part"$I
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
	v.to.rast input=$GIS_OPT_vector"part"$I output=$r_name use=cat value=1 rows=4096 
	#write info in configuration file
	g.region -g| grep "n=" | cut -f2 -d'='> $name.var
	read north < $name.var
	g.region -g| grep "s=" | cut -f2 -d'='  > $name.var
	read south < $name.var
	g.region -g| grep "e=" | cut -f2 -d'=' > $name.var
	read east < $name.var
	g.region -g| grep "w=" | cut -f2 -d'=' > $name.var
	read west < $name.var
	echo "MASKEDOVERLAYAREA $r_name|n=$north|s=$south|e=$east|w=$west" >> $GIS_OPT_conf
    fi
    #clear vector map part
    g.remove vect=$GIS_OPT_vector"part"$I
    #rm -fR "$GISDBASE"/"$LOCATION"/"$MAPSET"/vector/$GIS_OPT_vector"part"$I
    #echo DROP TABLE $GIS_OPT_vector"part"$I | db.execute 
    ((I++))
done
d.mon stop=x1
# clean tmp files
rm -f $TMP*


