## DESCRIPTION

This module makes each cell value a function of the attribute values
assigned to the vector points or centroids in an area around the cell
with a diameter of *size* around it, and stores the new cell values in
the *output* raster map layer. By default, the module just counts the
number of points. The user can also choose amongst a variety of
aggregate statistics using the parameter *method*. These statistics are
calculated on the attributes in the *point_column*. Using the usual
*cats* and *where* parameters the user can chose to take only a subset
of the points into account.

Note that *size* is defined as the diameter, and so has to be twice the
wanted search radius, and that the module works within the current
computational region which can be adjusted using
[g.region](g.region.md). If the vector map falls completely outside the
current region, the module will stop with an error.

## EXAMPLE

Count the number of schools for a given grid (North Carolina sample
dataset):

```sh
g.region vector=schools_wake res=100 -p -a
v.neighbors input=schools_wake output=schools_wake_3000m method=count size=3000

d.mon wx0
d.rast schools_wake_3000m
d.vect schools_wake
```

The result gives for each grid cell the number of points (here: schools)
not farther than 1500 meter away (half of the given *size* value) from
the respective cell center.

Calculate the mean capacity of schools for the same grid:

```sh
v.neighbors input=schools_wake output=schools_capacity point_column=CAPACITYTO \
            method=average size=3000
```

## SEE ALSO

*[r.neighbors](r.neighbors.md), [v.vect.stats](v.vect.stats.md)*

## AUTHORS

Radim Blazek,  
Moritz Lennert
