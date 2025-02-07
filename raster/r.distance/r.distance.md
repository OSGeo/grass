## DESCRIPTION

*r.distance* locates the closest points between "objects" in two raster
maps. An "object" is defined as all the grid cells that have the same
category number, and closest means having the shortest "straight-line"
distance. The cell centers are considered for the distance calculation
(two adjacent grid cells have the distance between their cell centers).

The output is an ascii list, one line per pair of objects, in the
following form:

```sh
cat1:cat2:distance:east1:north1:east2:north2
```

**cat1**  
Category number from map1

**cat2**  
Category number from map2

**distance**  
The distance in meters between "cat1" and "cat2"

**east1,north1**  
The coordinates of the grid cell "cat1" which is closest to "cat2"

**east2,north2**  
The coordinates of the grid cell "cat2" which is closest to "cat1"

### Flags

**-l** The -l flag outputs the category labels of the matched raster
objects at the beginning of the line, if they exist.

**-o** The -o flag reports zero distance if the input rasters are
overlapping.

## NOTES

The output format lends itself to filtering. For example, to "see" lines
connecting each of the category pairs in two maps, filter the output
using awk and then into *d.graph*:

```sh
r.distance map=map1,map2 | \
  awk -F: '{print "move",$4,$5,"\ndraw",$6,$7}' | d.graph -m
```

To create a vector map of all the "map1" coordinates, filter the output
into awk and then into *v.in.ascii*:

```sh
r.distance map=map1,map2 | \
  awk -F: '{print $4,$5}' | v.in.ascii format=point output=name separator=space
```

## SEE ALSO

*[r.buffer](r.buffer.md), [r.cost](r.cost.md), [r.drain](r.drain.md),
[r.grow](r.grow.md), [r.grow.distance](r.grow.distance.md),
[v.distance](v.distance.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
