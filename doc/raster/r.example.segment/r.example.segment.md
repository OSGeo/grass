# r.example.segment

## DESCRIPTION

*r.example.segment* changes one cell value in hardcoded location.
It is meant to demonstrate how to use the Segment Library together with
GRASS GIS raster maps.

## EXAMPLE

Create a modified version of the raster map "elevation"
(North Carolina sample dataset):

```bash
g.region raster=elevation
r.example.segment input=elevation output=modified_elevation
r.univar raster_map_1
r.univar raster_map_2
```

## SEE ALSO

* [r.example](r.example.html)
* [r.example.segmulti](r.example.segmulti.html)
* [v.example](v.example.html)

* [GRASS Programmer's Manual](https://grass.osgeo.org/programming7/)

## AUTHORS

Vaclav Petras
