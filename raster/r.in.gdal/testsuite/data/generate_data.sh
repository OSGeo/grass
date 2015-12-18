#!/bin/bash
# This script requires the graass7 North Caroline location

export GRASS_OVERWRITE=1

# 2D output
g.region rast=elevation -p
g.region res=100 -p

r.out.gdal in=elevation out=elevation.tif format=GTiff
r.out.gdal in=elevation out=elevation.tiff format=GTiff
r.out.gdal in=elevation out=elevation.asc format=AAIGrid
r.out.gdal in=elevation out=elevation.nc format=netCDF

# 3D output
g.region b=0 t=5 res3=100 tbres=1 -p3

r.to.rast3 input=elevation output=elevation3d
r3.out.netcdf input=elevation3d out=elevation3d.nc

