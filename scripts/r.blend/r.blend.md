## DESCRIPTION

*r.blend* blends color components of 2 raster maps by a specified
percentage of the first map.

## EXAMPLE

Blending the aspect map with the elevation map for a shaded map (North
Carolina sample dataset):

```
g.region raster=elevation
r.relief input=elevation output=relief zscale=10
r.blend -c first=elevation second=relief output=blend percent=75
```

::: {align="center" style="margin: 10px"}
[![r.blend example](r_blend.png){width="600" height="540"
border="0"}](r_blend.png)\
*Figure: Elevation blended with shaded relief*
:::

## SEE ALSO

*[d.shade](d.shade.html), [g.region](g.region.html),
[r.shade](r.shade.html), [r.colors](r.colors.html), [r.his](r.his.html),
[r.mapcalc](r.mapcalc.html), [r.rgb](r.rgb.html),
[r.support](r.support.html)*

## AUTHORS

Unknown: probably CERL\
Updated to GRASS 5.7 by Michael Barton, Arizona State University
