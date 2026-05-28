## DESCRIPTION

*r.in.aster* rectifies, georeferences, and imports Terra-ASTER imagery
to current project using gdalwarp, hdf 4, and r.in.gdal, using
projection parameters from g.proj. It can import Level 1A, Level 1B,
their relative DEM products, and Level 1T.

The program may be run interactively or non-interactively from the
command line. In either case, the user must specify an **input** \*.hdf
file name, the **type** of processing used, the image **band** to
import, and an **output** GRASS raster map name.

The **type** parameter can take values of L1A, L1B, L1T or DEM.

The **band** parameter can take values of 1, 2, 3n, 3b, 4-14

## NOTES

*r.in.aster* requires GDAL library to be in the user's path and the hdf
4 driver to be installed. The GDAL library must be compiled with hdf
support.

## AUTHORS

Michael Barton, Arizona State University and Paul Kelly
