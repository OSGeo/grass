## DESCRIPTION

*r.surf.area* calculates area of regular 3D triangulated points (centers
of cells) in current region by adding areas of triangles. Therefore,
area of a flat surface will be reported as
(`rows + cols -1) * (area of cell)` less than area of flat region due to
a half row and half column missing around the perimeter.

## NOTES

This calculation is heavily dependent on data resolution (think of it as
a fractal shoreline problem, the more resolution the more detail, the
more area, etc). This module uses the **current region settings**, not
the resolution of the raster map. This is especially important for
surfaces with `NULL` values and highly irregular edges. The module does
not \[currently\] attempt to correct for the error introduced by this
*edge effect*.

This version actually calculates area twice for each triangle pair,
keeping a running minimum and maximum area depending on the direction of
the diagonal used.

Reported totals are:

1. "Plan" area of `NULL` values within the current GRASS region
2. "Plan" area within calculation region (`rows-1 * cols-1 * cellarea`)
3. Average of the minimum and maximum calculated 3d triangle area
    within this region
4. "Plan" area within current computational region
    (`rows * cols * cellarea`)
5. Scaling of calculated area to current region

*r.surf.area* works best when the surface being evaluated extends to the
edges of the current region and the cell resolution is small. Surfaces
which are especially long and thin and have highly irregular boundaries
will tend to have underestimated surface areas. Setting a high cell
resolution (small area) will greatly reduce this impact, but will cause
longer processing times.

## EXAMPLES

```sh
g.region -p raster=elevation

r.surf.area map=elevation units=hectares
Null value area ignored in calculation: 0.000000
Plan area used in calculation: 20221.510000
Surface area calculation(low, high, avg):
        20294.310421 20320.936368 20307.623395
Current region plan area: 20250.000000
Estimated region Surface Area: 20336.234719
```

## SEE ALSO

*[g.region](g.region.md), [r.surf.idw](r.surf.idw.md),
[r.surf.fractal](r.surf.fractal.md), [r.surf.gauss](r.surf.gauss.md),
[r.volume](r.volume.md), [r.slope.aspect](r.slope.aspect.md),
[v.to.rast](v.to.rast.md)*

## AUTHOR

Bill Brown, USACERL December 21, 1994  
Modified for floating point rasters and `NULL` values by Eric G. Miller
(October 17, 2000)  
Updated for GRASS 7, and units option by Martin Landa, Czech Technical
University in Prague, Czech Republic (October 2011)
