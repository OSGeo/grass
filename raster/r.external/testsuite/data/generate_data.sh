#!/bin/bash
# This script requires the GRASS 7 North Carolina location

export GRASS_OVERWRITE=1

# 2D output
g.region raster=elevation -p
g.region res=100 -p

r.out.gdal input=elevation output=elevation.tif format=GTiff
r.out.gdal input=elevation output=elevation.tiff format=GTiff
r.out.gdal input=elevation output=elevation.asc format=AAIGrid
r.out.gdal input=elevation output=elevation.nc format=netCDF

# 3D output
g.region b=0 t=5 res3=100 tbres=1 -p3

r.to.rast3 input=elevation output=elevation3d
r3.out.netcdf input=elevation3d output=elevation3d.nc
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 7f32ec0a8d (r.horizon manual - fix typo (#2794))
