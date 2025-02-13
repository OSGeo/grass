## DESCRIPTION

*r.shade* will drape a color raster map over a shaded relief map. In
place of shaded relief, any raster map can be used including aspect or
slope. The color raster map is usually an elevation raster map with
colorful color table (as opposed to gray scale color table). However,
any raster map can be used including categorical raster maps. The result
is a raster map created from elevation and the shade raster.

Comparing to creating shaded relief as semi-transparent overlay on the
color raster map, this module gives result with more saturated colors.

The input for this module can be created for example using
*[r.slope.aspect](r.slope.aspect.md)* or *[r.relief](r.relief.md)*.

NULL values are propagated by default, so if any of the two input
rasters contains NULL cell NULL will be also in the output. If **-c**
flag is used and cell in **color** raster is NULL, just **shade** color
is used. If cell in **shade** raster is NULL, shading effect is not
applied and original colors are used. If **bgcolor** option is used,
NULL value in any input raster will be in the output replaced by the
given color.

## NOTES

Refer to the *[r.his](r.his.md)* help page for more details; *r.shade*
is a frontend to that module with addition of brightness support similar
to one provided by *[d.shade](d.shade.md)*. However, note that the
brightness is not implemenented in the same way as for
*[d.shade](d.shade.md)* and the results might be different. *r.shade* is
using method described in *[r.his](r.his.md)* manual page.

## EXAMPLES

In this example, the `aspect` map in the North Carolina sample dataset
is used to hillshade the `elevation` map:

```sh
g.region raster=aspect -p
r.shade shade=aspect color=elevation output=elevation_aspect_shaded

d.mon wx0
d.rast elevation_aspect_shaded
```

In this next example, a shaded relief raster map is created and used to
create a colorized hillshade raster map for later use:

```sh
g.region raster=elevation
r.relief input=elevation output=elevation_shaded_relief

r.shade shade=elevation_shaded_relief color=elevation \
    output=elevation_relief_shaded

d.mon wx1
d.rast elevation_relief_shaded
```

Interesting visualizations can be created using different color tables
for elevation raster map, for example using `haxby` color table.

![Elevation with shaded relief (hillshade)](rshade.png)

Figure: A detail of raster created by applying shading effect of shaded
relief (hillshade) to elevation raster map from North Carolina dataset
elevation map

## SEE ALSO

*[r.his](r.his.md), [d.his](d.his.md), [d.shade](d.shade.md),
[g.pnmcomp](g.pnmcomp.md), [r.slope.aspect](r.slope.aspect.md),
[r.relief](r.relief.md)*

## AUTHORS

Hamish Bowman  
Vaclav Petras, [NCSU GeoForAll
Lab](https://geospatial.ncsu.edu/geoforall/)  
Inspired by *[d.shade](d.shade.md)* and manual for *[r.his](r.his.md)*.
