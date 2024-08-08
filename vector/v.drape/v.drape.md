## DESCRIPTION

*v.drape* converts 2D/3D vector data into 3D vector format via sampling
of an elevation surface. Three sampling algorithms adapted from
*[v.sample](v.sample.html)* were incorporated into this module: nearest
neighbor, bilinear, and cubic convultion.

*v.drape* will skip vector features outside of current computational
region or where raster map has NULL value. It\'s possible to include all
vector features by specifying height value that will be assigned to
verticles whose values can not be determined from raster map.

## NOTES

Additional vertices can be added to the input 2D vector map with
*[v.split](v.split.html)*.

The module can be used in conjunction with *[v.out.pov](v.out.pov.html)*
and *[r.out.pov](r.out.pov.html)* to export a complete set of vector and
raster data for display in [POVRAY](http://www.povray.org/).

## EXAMPLES

Spearfish example:

```
v.drape in=roads elevation=elevation.10m method=bilinear out=roads3d
v.info roads3d
```

Create 3D vector roads map containing only \"unimproved\" roads. Set
road height to 1000 m for all parts without height information.

```
v.drape input=roads type=line elevation=elevation.dem output=roads_3d \
        method=nearest scale=1.0 where='cat=5' layer=1 null_value=1000
```

### POVRAY example

```
#export the vector data
v.drape in=roads out=roads3d elevation=elevation.10m
v.out.pov roads3d out=roads3d.pov
#export the raster data
r.out.pov elevation.10m tga=elevation.tga
r.out.png landcover.30m out=landcover30m.png

# now write a complete povray-script and launch povray
```

## SEE ALSO

*[v.extrude](v.extrude.html), [v.to.3d](v.to.3d.html),
[r.out.pov](r.out.pov.html), [v.in.region](v.in.region.html),
[v.out.pov](v.out.pov.html), [v.overlay](v.overlay.html),
[v.split](v.split.html), [v.what.rast](v.what.rast.html)*

## AUTHORS

Dylan Beaudette, University of California at Davis.\
Updated for GRASS 7 by Martin Landa, Czech Technical University in
Prague, Czech Republic
