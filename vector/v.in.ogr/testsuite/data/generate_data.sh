#!/bin/bash
# This script requires the GRASS GIS 7 North Caroline location

export GRASS_OVERWRITE=1

# 2D output
g.region vector=firestations@PERMANENT
v.out.ogr input=firestations@PERMANENT output=firestations.gpkg format=GPKG

# 3D output
v.out.ogr input=precip_30ynormals_3d@PERMANENT output=precip_30ynormals_3d.shp lco="SHPT=POINTZ" format=ESRI_Shapefile
