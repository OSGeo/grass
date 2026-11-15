## DESCRIPTION

*r.grow* adds cells around the perimeters of all areas in a
user-specified raster map layer and stores the output in a new raster
map layer. The user can use it to grow by one or more than one cell (by
varying the size of the **radius** parameter), or like *r.buffer*, but
with the option of preserving the original cells (similar to combining
*r.buffer* and *r.patch*).

If **radius** is negative,*r.grow* shrinks areas by removing cells
around the perimeters of all areas.

## NOTES

The user has the option of specifying three different metrics which
control the geometry in which grown cells are created, (controlled by
the **metric** parameter): *Euclidean*, *Manhattan*, and *Maximum*.

The *Euclidean distance* or *Euclidean metric* is the "ordinary"
distance between two points that one would measure with a ruler, which
can be proven by repeated application of the Pythagorean theorem. The
formula is given by:

```sh
d(dx,dy) = sqrt(dx^2 + dy^2)
```

Cells grown using this metric would form isolines of distance that are
circular from a given point, with the distance given by the **radius**.

The *Manhattan metric*, or *Taxicab geometry*, is a form of geometry in
which the usual metric of Euclidean geometry is replaced by a new metric
in which the distance between two points is the sum of the (absolute)
differences of their coordinates. The name alludes to the grid layout of
most streets on the island of Manhattan, which causes the shortest path
a car could take between two points in the city to have length equal to
the points' distance in taxicab geometry. The formula is given by:

```sh
d(dx,dy) = abs(dx) + abs(dy)
```

where cells grown using this metric would form isolines of distance that
are rhombus-shaped from a given point.

The *Maximum metric* is given by the formula

```sh
d(dx,dy) = max(abs(dx),abs(dy))
```

where the isolines of distance from a point are squares.

If there are two cells which are equal candidates to grow into an empty
space, *r.grow* will choose the northernmost candidate; if there are
multiple candidates with the same northing, the westernmost is chosen.

## EXAMPLE

In this example, the lakes map in the North Carolina sample dataset is
buffered:

```sh
g.region raster=lakes -p
# the lake raster map pixel resolution is 10m
r.grow input=lakes output=lakes_grown_100m radius=10
```

Shrinking instead of growing:

```sh
g.region raster=lakes -p
# the lake raster map pixel resolution is 10m
r.grow input=lakes output=lakes_shrunk_100m radius=-10
```

## SEE ALSO

*[r.buffer](r.buffer.md), [r.grow.distance](r.grow.distance.md),
[r.patch](r.patch.md)*

*[Wikipedia Entry: Euclidean
Metric](https://en.wikipedia.org/wiki/Euclidean_metric)*  
*[Wikipedia Entry: Manhattan
Metric](https://en.wikipedia.org/wiki/Manhattan_metric)*

## AUTHORS

Marjorie Larson, U.S. Army Construction Engineering Research Laboratory

Glynn Clements
