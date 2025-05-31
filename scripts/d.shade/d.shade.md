## DESCRIPTION

*d.shade* will drape a color raster map over a shaded relief map. In
place of shaded relief, any raster map can be used including aspect or
slope. The color raster map is usually an elevation raster map with
colorful color table (as opposed to gray scale color table). However,
any raster map can be used including categorical raster maps.

The advantage of this module is that it allows visualizing the shaded
map without a need to create a new raster which would combine both.
Comparing to creating shaded relief as semi-transparent overlay on the
color raster map, this module gives result with more saturated colors.

The input for this module can be created for example using
[r.slope.aspect](r.slope.aspect.md) or [r.relief](r.relief.md)  
.

## NOTES

Refer to the *[d.his](d.his.md)* help page for more details; *d.shade*
is simply a frontend to that module.

## EXAMPLES

In this example, the `aspect` map in the North Carolina sample dataset
is used to hillshade the `elevation` map:

```sh
g.region raster=aspect -p
d.mon wx0
d.shade shade=aspect color=elevation
```

![Elevation with aspect shades](dshade.png)

Figure: A detail of raster created by applying shading effect of aspect
to elevation raster map from North Carolina dataset elevation map

In this next example, a shaded relief raster map is created and used to
create a colorized hillshade:

```sh
g.region raster=elevation
r.relief input=elevation output=elevation_shaded_relief

d.mon wx1
d.shade shade=elevation_shaded_relief color=elevation
```

Interesting visualizations can be created using different color tables
for elevation raster map, for example using `haxby` color table.

## SEE ALSO

*[d.his](d.his.md), [g.pnmcomp](g.pnmcomp.md), [r.shade](r.shade.md),
[r.slope.aspect](r.slope.aspect.md), [r.relief](r.relief.md)  
[wxGUI 3D viewer (NVIZ)](wxGUI.nviz.md)*

## AUTHORS

Unknown; updated to GRASS 5.7 by Michael Barton
