## DESCRIPTION

*r.example.segment* changes one cell value in a hardcoded location.
It is meant to demonstrate how to use the Segment Library together with
GRASS raster maps.

## EXAMPLE

Create a modified version of the raster map "elevation" (North Carolina
sample dataset) and inspect the single changed cell:

```sh
g.region raster=elevation
r.example.segment input=elevation output=modified_elevation
r.mapcalc expression='difference = modified_elevation - elevation'
r.univar difference
```

The difference is zero everywhere except the one hardcoded cell, where
the value increased by 100.

## SEE ALSO

*[r.example](r.example.md), [r.example.segmulti](r.example.segmulti.md),
[v.example](v.example.md)*

*[Segment Library](https://grass.osgeo.org/programming8/segmentlib.html)
in the [GRASS Programmer's Manual](https://grass.osgeo.org/programming8/)*

## AUTHORS

Vaclav Petras
