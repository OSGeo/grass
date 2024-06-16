## DESCRIPTION

*r.pack* collects raster map elements and support files and compressed
them using *gzip* algorithm for copying. The resulting packed file can
be afterwards unpacked within a GRASS GIS session by
*[r.unpack](r.unpack.html)*. Since the selected raster map is not
exported but natively stored, the current region is not respected. Hence
*r.pack* stores the entire raster map.

## NOTES

By default, the name of the pack file is determined from the **input**
parameter. Optionally a different name can be given by **output**
parameter. Currently only 2D raster maps are supported.

## EXAMPLE

Pack up the entire raster map *aspect* into *aspect.pack* file:

::: code
    r.pack input=aspect
:::

The packed raster map file *aspect.pack* can afterwards be unpacked by

::: code
    r.unpack input=aspect.pack
:::

## SEE ALSO

*[r.unpack](r.unpack.html), [r.in.gdal](r.in.gdal.html),
[g.copy](g.copy.html), [r.proj](r.proj.html), [v.unpack](v.unpack.html)*

## AUTHORS

Original Bash script written by Hamish Bowman, Otago University, New
Zealand\
Converted to Python and updated for GRASS 7 by Martin Landa, Czech
Technical University in Prague, Czech Republic
