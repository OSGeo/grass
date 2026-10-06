## DESCRIPTION

*r.example.segmulti* changes one cell value in a hardcoded location
and sums input rasters together.
It is meant to demonstrate how to use the Segment Library together with
GRASS raster maps. Specifically, it focuses on the case when multiple
rasters are always accessed together (e.g. image bands) and when their
values can be stored as the same type (here double).

## EXAMPLE

Set computational region and generate synthetic data:

```sh
g.region cols=100 rows=50 -p
r.mapcalc expression='raster_1 = row()'
r.mapcalc expression='raster_2 = 10 * col()'
r.mapcalc expression='raster_3 = 1000'
```

Test the module:

```sh
r.example.segmulti input=raster_1,raster_2,raster_3 output=raster_out
```

## SEE ALSO

*[r.example](r.example.md), [r.example.segment](r.example.segment.md),
[v.example](v.example.md)*

*[Segment Library](https://grass.osgeo.org/programming8/segmentlib.html)
in the [GRASS Programmer's Manual](https://grass.osgeo.org/programming8/)*

## AUTHORS

Vaclav Petras,
[NCSU Center for Geospatial Analytics](https://geospatial.ncsu.edu)
