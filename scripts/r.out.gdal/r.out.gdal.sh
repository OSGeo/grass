#!/bin/sh

############################################################################
#
# MODULE:	r.out.gdal.sh script
# AUTHOR(S):	Markus Neteler. neteler itc.it
# PURPOSE:	r.out.gdal.sh script hack until a C version is written
# COPYRIGHT:	(C) 2003,2005 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
# REQUIRES:     - GDAL compiled with GRASS-GDAL-OGR plugin
#                   http://grass.gdf-hannover.de/wiki/Compile_and_install_GRASS_and_QGIS_with_GDAL/OGR_Plugin
#############################################################################

#%Module
#% description: Exports GRASS raster into GDAL supported formats.
#% keywords: raster, export
#%End
#%flag
#%  key: l
#%  description: List supported output formats
#%END
#%flag
#%  key: r
#%  description: Region sensitive output
#%END
#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Name of input raster map
#% required : no
#%END
#%option
#% key: format
#% type: string
#% description: GIS format to write (case sensitive, see also -l flag)
#% options: AAIGrid,BMP,BSB,DTED,ELAS,ENVI,FIT,GIF,GTiff,HFA,JPEG,MEM,MFF,MFF2,NITF,PAux,PNG,PNM,VRT,XPM
#% answer: GTiff
#% required : no
#%END
#%option
#% key: type
#% type: string
#% description: File type
#% options: Byte,Int16,UInt16,UInt32,Int32,Float32,Float64,CInt16,CInt32,CFloat32,CFloat64
#% required : no
#%END
#%option
#% key: output
#% type: string
#% description: Name for output file
#% required : no
#%END
#%option
#% key: createopt
#% type: string
#% description: Creation option to the output format driver. Multiple options may be listed
#% multiple: yes
#% required : no
#%END
#%option
#% key: metaopt
#% type: string
#% description: Metadata key passed on the output dataset if possible
#% multiple: yes
#% required : no
#%END


if [ "$1" != "@ARGS_PARSED@" ] ; then
  exec g.parser "$0" "$@"
fi

PROG=`basename $0`

g.message -w "This module is superseded and will be removed in future \
versions of GRASS. Use the much faster r.out.gdal instead."

#check gdal_translate exists
if [ -z "`which gdal_translate`" ] ; then
   g.message -e "Required program gdal_translate is missing. Full GDAL \
     binaries with GDAL/GRASS support are needed to use $PROG."
   exit 1
fi
#check that gdal's grass plugin exists
if [ -z "`gdalinfo --formats | grep -i GRASS`" ] ; then
   g.message -e "Required GRASS plugin for GDAL is missing. GDAL must be \
     built with GRASS support to use $PROG."
   exit 1
fi

INPUT="$GIS_OPT_INPUT"
FORMAT="$GIS_OPT_FORMAT"
TYPE="$GIS_OPT_TYPE"
OUTPUT="$GIS_OPT_OUTPUT"
CREATEKEY="`echo "$GIS_OPT_CREATEOPT" | sed 's+,+ -co +g' | sed 's+^+-co +g'`"
METAKEY="`echo "$GIS_OPT_METAOPT" | sed 's+,+ -mo +g' | sed 's+^+-mo +g'`"

if [ $GIS_FLAG_L -eq 1 ] ; then
  gdal_translate | grep ':' | grep -v 'Usage' | grep -v 'The following'
  exit 0
fi

#DEBUG:
#echo $INPUT $FORMAT $OUTPUT
#echo $GDAL_INPUT_FORMATS

# region sensitive output
if [ $GIS_FLAG_R -eq 1 ] ; then

    TEMPRASTER=r.out.gdal_${INPUT}_$$
    r.resample input=$INPUT output=$TEMPRASTER
    INPUT=$TEMPRASTER
fi

#fetch the input raster map
eval `g.findfile element=cell file=$INPUT` 
if [ ! "$file" ] ; then
  g.message -e "Input map not found"
  exit 1
fi

if [ -z "$FORMAT" ] ; then
  g.message -e "Output format not specified"
  exit 1
fi

#set output if required
if [ -z "$OUTPUT" ] ; then
  OUTPUT=$INPUT
fi

if [ -z "$TYPE" ] ; then
  g.message -e "Output TYPE not specified"
  g.message -e message="(Raster map type is `r.info -t $INPUT | cut -d'=' -f2`)"
  exit 1
fi

if [ "$CREATEKEY" = "-co " ] ; then
  unset CREATEKEY
fi
if [ "$METAKEY"   = "-mo " ] ; then
  unset METAKEY
fi

#do it
CELLHD=`echo $file | sed 's+/cell/+/cellhd/+g'`
g.message message="Writing format: $FORMAT"
g.message message="Writing type:   $TYPE"
gdal_translate -of $FORMAT -ot $TYPE $CREATEKEY $METAKEY $CELLHD $OUTPUT 

if [ $GIS_FLAG_R -eq 1 ] ; then
    g.remove $TEMPRASTER
fi
