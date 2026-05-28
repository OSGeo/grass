## DESCRIPTION

*r.tile* retiles an existing raster map with user defined x and y tile
size.

## NOTES

*r.tile* generates a separate raster for each tile. This is equivalent
to running *g.region* along with *r.resample* in a double loop.

The module can be used to split a large raster map into smaller tiles,
e.g. for further parallelized analysis on a cluster computing system.

The overlap is defined in rows/columns.

## EXAMPLE

Retiling example for the North Carolina DEM:

```sh
g.region raster=elevation -p
# rows:       1350
# cols:       1500

# generating 2 x 2 = 4 tiles (width=1500/2, height=rows/2)
r.tile input=elevation output=elev_tile width=750 height=675
```

creates 4 tiles with the prefix *elev_tile* (named: elev_tile-000-000,
elev_tile-000-001, elev_tile-001-000, ...).

## SEE ALSO

*[g.region](g.region.md), [r3.retile](r3.retile.md)*

## AUTHOR

Glynn Clements
