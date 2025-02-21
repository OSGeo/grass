## DESCRIPTION

*r.spreadpath* is part of the wildfire simulation toolset. Preparational
steps for the fire simulation are the calculation of the rate of spread
(ROS) with *r.ros*, and the creating of spread map with *r.spread*.
Eventually, the fire path(s) based on starting point(s) are calculated
with *r.spreadpath*.

*r.spreadpath* recursively traces the least cost path backwards to the
origin, given backlink information input map layers and target locations
from where paths are to be traced. The backlink information map layers
record each cell's backlink UTM northing (the y_input) and easting (the
x_input) coordinates from which the cell's cumulative cost was
determined.

The backlink inputs can be generated from another GRASS raster program
*r.spread*. One of the major applications of *r.spreadpath* along with
*r.spread* is to accurately find the least cost corridors and/or paths
on a raster setting. More information on *r.spread* and *r.spreadpath*
can be found in Xu (1994).

## Parameters

**x_input**=*name*  
Name of input raster map layer containing backlink UTM easting
coordinates.

**y_input**=*name*  
Name of input raster map layer containing backlink UTM northing
coordinates.

**coordinates**=*x,y\[,x,y,x,y, ...\]*  
Each x,y coordinate pair gives the easting and northing (respectively)
geographic coordinates of a target point from which to backwards trace
the least cost path. As many points as desired can be entered by the
user.

**output**=*name*  
Name of raster map layer to contain output. Also can be used as the map
layer of the input target points. If so used, the input target point map
will be overwritten by the output.

## REFERENCES

- Xu, Jianping, 1994, Simulating the spread of wildfires using a
  geographic information system and remote sensing, Ph. D. Dissertation,
  Rutgers University, New Brunswick, New Jersey
  ([ref](https://dl.acm.org/citation.cfm?id=921466)).

## SEE ALSO

*[r.spread](r.spread.md), [r.ros](r.ros.md)* Sample data download:
[firedemo.sh](https://grass.osgeo.org/sampledata/firedemo_grass7.sh)
(run this script within the "Fire simulation data set" project).

## AUTHORS

Jianping Xu and Richard G. Lathrop, Jr., Center for Remote Sensing and
Spatial Analysis, Rutgers University.
