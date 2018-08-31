#!/bin/sh

# g.parser demo script for shell programming

#%module
#% description: g.parser test script (shell)
#% keyword: keyword1
#% keyword: keyword2
#%end
#%flag
#% key: f
#% description: A flag
#%end
#%option G_OPT_R_MAP
#% key: raster
#% required: yes
#%end
#%option G_OPT_V_MAP
#% key: vector
#%end
#%option
#% key: option1
#% type: string
#% description: An option
#% required: no
#%end

if [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program." 1>&2
    exit 1
fi

if [ "$1" != "@ARGS_PARSED@" ] ; then
    exec g.parser "$0" "$@"
fi

#### add your code below ####

echo ""

if [ $GIS_FLAG_F -eq 1 ] ; then
  g.message message="Flag -f set"
else
  g.message message="Flag -f not set"
fi

# test if parameter present:
if [ -n "$GIS_OPT_OPTION1" ] ; then
    echo "Value of GIS_OPT_OPTION1: '$GIS_OPT_OPTION1'"
fi

g.message message="Value of GIS_OPT_option1: '$GIS_OPT_option1'"
g.message message="Value of GIS_OPT_raster: '$GIS_OPT_raster'"
g.message message="Value of GIS_OPT_vect: '$GIS_OPT_vector'"

#### end of your code ####

