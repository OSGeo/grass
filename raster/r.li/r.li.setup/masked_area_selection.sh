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
#% description: Raster map to to analyse
#% required: yes
#%end
#%option
#% key: vector
#% type: string
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
#% description: Name of configuration file where insert areas
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


# FIXME: use WIND_OVERRIDE

# show the sampling frame
if [ "$GIS_FLAG_f" -eq 1 ] ; then
    g.region n="$GIS_OPT_north" s="$GIS_OPT_south" e="$GIS_OPT_east" w="$GIS_OPT_west"
else
    g.region rast="$GIS_OPT_raster"
fi

# open x1 Xmonitor
d.mon stop=x1
d.mon start=x1

d.rast -o map="$GIS_OPT_raster"

if [ -n "$GIS_OPT_vector" ] ; then
    d.vect map="$GIS_OPT_vector"
fi
if [ -n "$GIS_OPT_site" ] ; then 
    d.vect map="$GIS_OPT_site"
fi

# setup for drawing area
if [ "$GIS_FLAG_c" -eq 1 ] ; then
   RDIG_INSTR="$f_path/circle.txt"
else
   RDIG_INSTR="$f_path/polygon.txt"
fi

# feed options to r.digit
r.digit output="tmp_rli_mask.$$" < "$RDIG_INSTR"


# show the selected area
d.rast -o map="tmp_rli_mask.$$"

name=$$.val
export name

"$GRASS_WISH" "$f_path/area_query"
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

# clean tmp files
#FIXME: use g.tempfile
rm -f "$$"*
rm -f "$TMP"*

