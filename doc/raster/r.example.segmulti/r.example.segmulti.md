# r.example.segmulti

## DESCRIPTION

*r.example.segmulti* changes one cell value in hardcoded location
and sums input rasters together.
It is meant to demonstrate how to use the Segment Library together with
GRASS GIS raster maps. Specifically is focuses on case when multiple
rasters are always accessed together (e.g. image bands) and when their
values can be stored as a same type (here double).

## EXAMPLE

Set computational region and generate synthetic data:

```bash
g.region cols=100 rows=50 -p
r.mapcalc -s expression='raster_1 = row()'
r.mapcalc -s expression='raster_2 = 10 * col()'
r.mapcalc -s expression='raster_3 = 1000'
```

Test the module:

```bash
r.example.segmulti input=raster_1,raster_2,raster_3 output=raster_out
```

## SEE ALSO

* [r.example](r.example.html)
* [r.example.segment](r.example.segment.html)
* [v.example](v.example.html)

* [GRASS Programmer's Manual](https://grass.osgeo.org/programming7/)

## AUTHORS

Vaclav Petras,
[NCSU Center for Geospatial Analytics](http://geospatial.ncsu.edu)
