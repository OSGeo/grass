## DESCRIPTION

*v.report* generates a table showing the area present in each of the
categories of a user-selected data layer.

Area is given in hectares, square meters, and square kilometers. If the
units option is used, area is given in acres, square feet, and square
miles.

Feet and acre units are always reported in their common versions (i.e.
the International Foot, exactly 5280 feet in a mile), even when the
coordinate reference system's standard map unit is the US Survey foot.

*v.report* works on the full map data; therefore, the current region is
ignored. If you wish to spatially limit the statistics, a map subset
must be created with *v.in.region* and *v.overlay*, and then run
*v.report* on the new map.

## EXAMPLE

North Carolina sample dataset:

```sh
v.report zipcodes_wake option=area units=hectares
```

In the output, there is an extra column added containing the results.

## SEE ALSO

*[v.in.region](v.in.region.md), [v.to.db](v.to.db.md),
[v.overlay](v.overlay.md)*

## AUTHOR

Markus Neteler, GDF Hannover
