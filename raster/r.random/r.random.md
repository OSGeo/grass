## DESCRIPTION

The module *r.random* creates a raster map with values in random places.
Alternatively, it creates random vector points at these places. Number
of random cells or points can be a fixed number or a percentage of cells
from the input. By default, generated cells or points will be subset of
non-NULL cells of the input. Resulting raster map consists of original
cell values at the selected random locations and NULL (no data) values
elsewhere.

### Placement of cells and points

The module allows the user to create a raster map and/or a vector points
map containing coordinates of points whose locations have been randomly
determined. The module places these randomly generated vector points
within the current computational region and raster mask (if any), on
non-NULL raster cells in a user-specified raster map. If the user sets
the **-n** flag, points will be randomly generated across all cells
(even those with NULL values). Cells in the resulting raster overlap
with the cells of the input raster based on the current computational
region. Points in the resulting vector map are placed in cell centers of
these cells.

### Number of cells and points

The user may specify the quantity of random locations to be generated
either as a *positive integer* (e.g., 10), or as a *percentage of the
raster map's cells* (e.g., 10%, or 3.05%). The number of cells
considered for the percentage reflects whether or not the **-n** flag
was given. Options are 0-100; fractions of percent may be stated as
decimals (e.g., 66.67%, or 0.05%).

### Values

The cell values and corresponding category names (if present) associated
with the random point locations in the *input* map are assigned to the
newly generated cells in the *raster* map. If the **-n** is specified,
then a unique entry is made for the value used where the *input* was
NULL. This value is at least 1 less than the smallest value in the
*input* raster and is given a medium gray color.

If a *cover* raster map is specified, values are taken from the *cover*
raster map instead of the *input* raster map. If a *cover* raster map is
specified and the *cover* map contains NULL (no data) values, these
points are suppressed in the resulting *vector* or *raster* map.

### Vector output

The *vector* file created by *r.random* contains vector points that
represent the *center points* of the randomly generated cells. A *value*
attribute contains the cell value of the *input* raster (or the assigned
value when **-n** is used). If a *cover* map is additionally specified,
a second column *covervalue* is populated with raster values from the
*cover* map.

If the user sets the **-b** flag, vector points are written without
topology to minimize the required resources. This is suitable input to
*v.surf.rst* and other vector modules.

## NOTES

To decide on the number of points *r.random* will create, use
*[r.univar](r.univar.md)*, *[g.region](g.region.md)*, or
*[r.report](r.report.md)*. *r.univar* is the fastest way to obtain
number of non-NULL cells and NULL cells in a raster map given the
current computational region and raster mask:

```sh
r.univar map=inputmap
```

The text output contains total number of null and non-null cells (called
`cells` in the machine-readable shell script style output), total null
cells (`null_cells`), and number of non-null cells (`n`). Alternatively,
you can use the following to examine the computational region and the
raster map:

```sh
g.region -p
r.report map=inputmap units=c null="*" nsteps=1
```

To create random vector point locations within some, but not all,
categories of a integer input raster map (aka CELL raster map), the user
must first create a reclassified raster map of the original raster map
(e.g., using the GRASS module *[r.reclass](r.reclass.md)*) that contains
only the desired categories, and then use the reclassed raster map as
input to *r.random*.

## EXAMPLES

### Random 2D vector elevation points

Random vector elevation points sampled from elevation map in the North
Carolina sample dataset region, result stored in 2D vector map:

```sh
g.region raster=elevation -p
r.random elevation vector=elevrand n=100
v.db.select elevrand
v.univar elevrand col=value type=point
```

### Random 3D vector elevation points

Random vector elevation points sampled from elevation map in the North
Carolina sample dataset region with collocated values sampled from
landuse map, result stored in 3D vector map:

```sh
g.region raster=elevation -p
r.random -z elevation cover=landclass96 vector=luserand3d n=100

# data output (value: elevation, covervalue: landuse class):
v.db.select luserand3d
cat|value|covervalue
1|111.229591|5
2|71.093758|1
3|122.51075|5
4|146.17395|4
...
```

## SEE ALSO

- *[g.region](g.region.md)* for setting the computational region and
  examining the total number of cells,
- *[r.reclass](r.reclass.md)* for working only with subset of values in
  the raster map,
- *[v.random](v.random.md)* for generating vector points without any
  involvement of raster data,
- *[r.random.cells](r.random.cells.md)* for generating random cells with
  with spatial dependence (minimal distance),
- *[r.surf.random](r.surf.random.md)* as an option for generating random
  cell values,
- *[v.surf.rst](v.surf.rst.md)* as an option for creating a surface from
  sampled points.

## AUTHORS

Dr. James Hinthorne, GIS Laboratory, Central Washington University

Modified for GRASS 5.0 by Eric G. Miller

Cover map support by Markus Neteler, 2007
