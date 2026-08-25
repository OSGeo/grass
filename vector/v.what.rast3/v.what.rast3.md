## DESCRIPTION

*v.what.rast3* reads 3D raster value for each point in the vector and
updates **col** column in vector attribute table by this value. The
column should be type double. This module is based on
[v.what.rast](v.what.rast.md).  
If more points have the same category, attribute value is set to NULL.
If 3D raster values is NULL, attribute value is set to NULL.

## NOTES

## EXAMPLES

A\) Reading values from 3D raster map at position of vector points,
writing these values into a column of the attribute table connected to
the vector map:  

```sh
v.what.rast3 map=pnts raster3d=plume column=concentration
```

B\) In case of a vector map without attached attribute table, first add
a new attribute table. This table is then populated with values queried
from the raster map:  

```sh
# create new random 3d vector points map
v.random -z output=pnts npoints=100 zmin=0  zmax=50

# add new table, link to map
v.db.addtable map=pnts column="concentration double precision"

# query raster map and upload values to vector table into specified column
g.region raster3d=plume -p
v.what.rast3 map=pnts raster3d=plume column=concentration

# verify new attribute table:
v.db.select map=pnts

# verify statistics of uploaded values:
v.univar map=pnts column=concentration type=point
```

## SEE ALSO

*[v.db.addtable](v.db.addtable.md), [v.db.select](v.db.select.md),
[v.what.rast](v.what.rast.md), [v.what.vect](v.what.vect.md),
[v.univar](v.univar.md)*

## AUTHOR

Soeren Gebbert, heavily based on v.what.rast by Radim Blazek
