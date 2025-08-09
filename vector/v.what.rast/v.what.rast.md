## DESCRIPTION

*v.what.rast* retrieves raster value from a given raster map for each
point or centroid stored in a given vector map. It can update a
**column** in the linked vector attribute table with the retrieved
raster cell value or print it.

The column type needs to be numeric (integer, float, double, ...). If
the column doesn't exist in the vector attribute table than the module
will create the new column of type corresponding with the input raster
map.

If the **-p** flag is used, then the attribute table is not updated and
the results are printed to standard output.

If the **-i** flag is used, then the value to be uploaded to the
database is interpolated from the four nearest raster cells values using
an inverse distance weighting method (IDW). This is useful for cases
when the vector point density is much higher than the raster cell size.

## NOTES

Points and centroid with shared category number cannot be processed. To
solved this, unique categories may be added with
*[v.category](v.category.md)* in a separate layer.

If multiple points have the same category, the attribute value is set to
NULL. If the raster value is NULL, then attribute value is set to NULL.

*v.what.rast* operates on the attribute table. To modify the vector
geometry instead, use *[v.drape](v.drape.md)*.

Categories and values are output unsorted with the print flag. To sort
them pipe the output of this module into the UNIX `sort` tool
(`sort -n`). If you need coordinates, after sorting use
*[v.out.ascii](v.out.ascii.md)* and the UNIX `paste` tool
(`paste -d'|'`). In the case of a NULL result, a "`*`" will be printed
in lieu of the value.

The interpolation flag is only useful for continuous value raster maps,
if a categorical raster is given as input the results will be nonsense.
Since the search window is limited to four raster cells there may still
be raster cell-edge artifacts visible in the results, this compromise
has been made for processing speed. If one or more of the nearest four
raster cells is NULL, then only the raster cells containing values will
be used in the weighted average.

## EXAMPLES

### Transferring raster values into existing attribute table of vector points map

Reading values from raster map at position of vector points, writing
these values into a column of the attribute table connected to the
vector map:

```sh
# work on copy of original geodetic points map
g.copy vector=geodetic_pts,mygeodetic_pts

# set computational region to raster map to be queried
g.region raster=elev_state_500m -p

# query raster cells (a new column will be added to existing table)
v.what.rast map=mygeodetic_pts raster=elev_state_500m column=height

# compare official geodetic heights to those of elevation model
v.db.select map=mygeodetic_pts columns=Z_VALUE,height separator=comma
```

### Transferring raster values into new vector points map

In case of a vector map without attached attribute table, first add a
new attribute table. This table is then populated with values queried
from the raster map:

```sh
# create new random vector points map
v.random pnts n=100

# add new table, link to map
v.db.addtable map=pnts column="height double precision"

# set computational region to raster map to be queried
g.region raster=elevation -p
# query raster map and upload values to vector table into specified column
v.what.rast map=pnts raster=elevation column=height

# verify new attribute table:
v.db.select pnts

# verify statistics of uploaded values:
v.univar map=pnts column=height type=point
```

## SEE ALSO

*[v.category](v.category.md), [v.db.addtable](v.db.addtable.md),
[v.db.select](v.db.select.md), [v.drape](v.drape.md),
[v.univar](v.univar.md), [v.rast.stats](v.rast.stats.md),
[v.what.vect](v.what.vect.md)*

## AUTHORS

Radim Blazek  
Hamish Bowman (interpolation)
