## DESCRIPTION

*r.mode* calculates the most frequently occurring value (i. e., mode) of
data contained in a *cover* raster map layer for areas assigned the same
category value in the user-specified *base* raster map layer. These
modes are stored in the new *output* map layer.

The *output* map is actually a *reclass* of the *base* map.

The **base** parameter defines an existing raster map layer in the
user's current mapset search path. For each group of cells assigned the
same category value in the *base* map, the mode of the values assigned
these cells in the *cover* map will be computed.

The **cover** parameter defines an existing raster map layer containing
the values to be used to compute the mode within each category of the
*base* map.

## NOTES

The user should use the results of *r.mode* with care. Since this
utility assigns a value to each cell which is based on global
information (i.e., information at spatial locations other than just the
location of the cell itself), the resultant map layer is only valid if
the geographic region and mask settings are the same as they were at the
time that the result map was created.

Results are affected by the current region settings and mask.

## EXAMPLE

Mode of K-factor (erosion) for Spearfish fields:

```sh
g.region raster=fields -p
r.mode base=fields cover=soils.Kfactor output=K.by.farm.mode
r.univar K.by.farm.mode
```

## SEE ALSO

*[g.region](g.region.md), [r.category](r.category.md),
[r.clump](r.clump.md), [r.describe](r.describe.md),
[r.mapcalc](r.mapcalc.md), [r.mfilter](r.mfilter.md),
[r.neighbors](r.neighbors.md), [r.reclass](r.reclass.md),
[r.stats](r.stats.md), [r.statistics](r.statistics.md),
[r.univar](r.univar.md)*

## AUTHOR

Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
