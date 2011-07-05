#!/bin/sh
#%Module 
#%description: Select a circular or polygonal area
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
#%flag
#% key: c
#% description: take a circular area
#%end

# Where to find the others scripts
f_path="$GISBASE/etc/r.li.setup"

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
# show the sampling frame

if [ $GIS_FLAG_f -eq 1 ] ; then
    g.region n=$GIS_OPT_north s=$GIS_OPT_south e=$GIS_OPT_east w=$GIS_OPT_west
else
    g.region rast=$GIS_OPT_raster
fi
# open x1 monitor
d.mon stop=x1
d.mon start=x1

d.rast -o map=$GIS_OPT_raster
if [ -n "$GIS_OPT_vector" ] ; then
    d.vect map=$GIS_OPT_vector
fi
if [ -n "$GIS_OPT_site" ] ; then 
    d.vect  map=$GIS_OPT_site
fi
#let draw area
if [ $GIS_FLAG_c -eq 1 ] ; then
    cp $f_path/circle.txt $$.txt 
    echo "$$" >> $$.txt 
else
    cp $f_path/polygon.txt  $$.txt 
    echo "$$" >> $$.txt
fi
 
r.digit < $$.txt

#show the selected area
d.rast -o map=$$ 
export name=$$.val
$GRASS_WISH $f_path/area_query 
    cat $name | cut -f1 -d ' ' > $name.var
    read ok < $name.var
    cat $name | cut -f2 -d' ' > $name.var
    r_name=""
    read r_name < $name.var
    if [ $ok -eq 1 ] ; then
	r.to.vect input="$$" output="v$$" feature=area
	g.region vect="v$$"
	v.to.rast input="v$$" output=$r_name value=1 use=val
	#write info in configuration file
	g.region -g| grep "n=" | cut -f2 -d'='> $name.var
	read north < $name.var
	g.region -g| grep "s=" | cut -f2 -d'='  > $name.var
	read south < $name.var
	g.region -g| grep "e=" | cut -f2 -d'=' > $name.var
	read east < $name.var
	g.region -g| grep "w=" | cut -f2 -d'=' > $name.var
	read west < $name.var
	echo "SAMPLEAREAMASKED $r_name $north|$south|$east|$west" >>\
	    $GIS_OPT_conf
	#remove tmp raster and vector
	g.remove rast=$$
	g.remove vect=v$$
        #rm -f "$GISDBASE"/"$LOCATION"/"$MAPSET"/cats/"$$" 
	#rm -f "$GISDBASE"/"$LOCATION"/"$MAPSET"/cell/"$$" 
	#rm -f "$GISDBASE"/"$LOCATION"/"$MAPSET"/cellhd/"$$" 
	#rm -fR "$GISDBASE"/"$LOCATION"/"$MAPSET"/cell_misc/"$$" 
	#rm -f  "$GISDBASE"/"$LOCATION"/"$MAPSET"/colr/"$$" 
	#rm -f "$GISDBASE"/"$LOCATION"/"$MAPSET"/hist/"$$" 
	#rm -fR "$GISDBASE"/"$LOCATION"/"$MAPSET"/vector/"v$$"
	echo DROP TABLE "v$$" | db.execute
	
	if [ $GIS_FLAG_f -eq 1 ] ; then
	    g.region n=$GIS_OPT_north s=$GIS_OPT_south e=$GIS_OPT_east w=$GIS_OPT_west
	else
	    g.region rast=$GIS_OPT_raster
	fi

    else
	echo 0 >> $GIS_OPT_conf
    fi

d.mon stop=x1
#clean tmp files
rm -f $$*
rm -f $TMP*
