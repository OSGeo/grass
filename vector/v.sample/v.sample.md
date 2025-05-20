## DESCRIPTION

*v.sample* samples a GRASS raster map at the point locations in the
input file by either cubic convolution interpolation, bilinear
interpolation, or nearest neighbor sampling (default).

This program may be especially useful when sampling for cross validation
of interpolations whose output is a raster map.

## NOTES

The output points will have the easting and northing of the input
points. The input category value is used. The input attribute, raster
value and difference is written to output.

When NULL values are encountered for a cell, zero value is used instead.
In these cases, more acurrate results may be obtained by using the
default nearest neighbor comparisons.

This program may not work properly with lat-long data when the **-bc**
flags are used.

When interpolation is done (i.e., the **-bc** flags are used), values
are assumed to be located at the centroid of grid cells. Therefore,
current resolution settings are important.

## EXAMPLE

Comparison of "elev_ned_30m" and "elev_srtm_30m" North Carolina sample
dataset elevation models at random positions:

```sh
# set computational region:
 g.region raster=elev_srtm_30m -p
# generate random points:
 v.random output=random n=100
# add table with one column:
 v.db.addtable random col="elev_srtm30 double precision"
# transfer elevations at random points into table:
 v.what.rast map=random rast=elev_srtm_30m col=elev_srtm30
# verify:
 v.db.select random

# perform sampling on other elevation map:
 v.sample in=random col=elev_srtm30 rast=elev_ned_30m out=elev_samples

#verify:
 v.db.select elev_samples

#univariate statistics of differences between elevation maps:
 v.univar elev_samples column=diff type=point
```

## SEE ALSO

*[g.region](g.region.md), [v.random](v.random.md),
[v.what.rast](v.what.rast.md)* *Image Sampling Methods* - GRASS Tutorial
on *s.sample* (available as
[s.sample-tutorial.ps.gz](https://grass.osgeo.org/gdp/sites/))

## AUTHORS

[James Darrell McCauley](http://mccauley-usa.com/)  
when he was at: [Agricultural
Engineering](http://ABE.www.ecn.purdue.edu/ABE/) [Purdue
University](http://www.purdue.edu/)

Updated for GRASS 5.0 by Eric G. Miller  
Updated for GRASS 5.7 by Radim Blazek
