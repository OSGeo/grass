## DESCRIPTION

*r.rgb* generates separate red, green and blue maps from a raster map
and its associated color table (grey255).

## EXAMPLE

::: code
    g.region raster=elevation -p
    r.rgb input=elevation red=elevation.r green=elevation.g blue=elevation.b
:::

In this case *r.rgb* produces in the current mapset three new raster
maps - \'elevation.r\', \'elevation.g\', \'elevation.b\'.

![](r_rgb_elevation.png)\

## SEE ALSO

*[r.composite](r.composite.html), [r.blend](r.blend.html),
[r.colors](r.colors.html), [r.mapcalc](r.mapcalc.html)*

## AUTHOR

Glynn Clements
