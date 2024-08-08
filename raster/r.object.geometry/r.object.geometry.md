## DESCRIPTION

*r.object.geometry* calculates form statistics of raster objects in the
**input** map and writes it to the **output** text file (or standard
output if no output filename or \'-\' is given), with fields separated
by the chosen **separator**. Objects are defined as clumps of adjacent
cells with the same category value (e.g. output of
*[r.clump](r.clump.html)* or *[i.segment](i.segment.html)*).

By default, values are in pixels. If values in meters is desired, the
user can set the **-m** flag. If the current working region is in
lat-long or has non-square pixels, using meters is recommended.

Statistics currently calculated are exactly the same as in
*[v.to.db](v.to.db.html)* (except for compact_square and mean
coordinates):

-   area
-   perimeter
-   compact_square (compactness compared to a square:
    `compact_square = 4 * sqrt(area) / perimeter`)
-   compact_circle (compactness compared to a circle:
    `compact_circle = perimeter / ( 2 * sqrt(PI * area) )`)
-   fractal dimension (
    `fd = 2 * ( log(perimeter) / log(area + 0.001) )` )
-   mean x coordinate of object (in map units)
-   mean y coordinate of object (in map units)

## EXAMPLE

```
g.region raster=soilsID
r.object.geometry input=soilsID output=soils_geom.txt
```

## SEE ALSO

*[i.segment](i.segment.html), [r.clump](r.clump.html),
[v.to.db](v.to.db.html)*

## AUTHORS

Moritz Lennert\
Markus Metz (diagonal clump tracing)
