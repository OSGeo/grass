#!/bin/sh
LOCATION="`g.gisenv GISDBASE`/`g.gisenv LOCATION_NAME`/`g.gisenv MAPSET`"
if test -d "$LOCATION/grid3/G3D_MASK" && test -f "$LOCATION/cell/MASK" ; then
    echo [Raster and Volume MASKs present]
elif test -f "$LOCATION/cell/MASK" ; then
    echo [Raster MASK present]
elif test -d "$LOCATION/grid3/G3D_MASK" ; then
    echo [Volume MASK present]
fi
