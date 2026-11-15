## DESCRIPTION

*v.to.points* creates points along input 2D or 3D lines, boundaries and
faces. Point features including centroids and kernels are copied from
input vector map to the output. For details see notes about **type**
parameter.

The output is a vector map with 2 layers. Layer 1 holds the category of
the input features; all points created along the same line have the same
category, equal to the category of that line. In layer 2 each point has
its unique category; other attributes stored in layer 2 are *lcat* - the
category of the input line and *along* - the distance from line's start.

By default only features with category are processed, see **layer**
parameter for details.

## NOTES

The **dmax** parameter is the maximum limit but not an exact distance.
To place points with exact distance from the beginning of the vector
line the user should use *[v.segment](v.segment.md)*.

Set **layer** to -1 to process features from all layers including
features without category. Features will be assigned new unique
categories at layer 1. Option **layer=-1** should be used to convert
boundaries, as in most of cases boundaries lack category values.

The **type** parameter is used to control which input vector geometry
types to convert into points. Some caveats to consider about this
parameter:

- Points and centroids can be considered as "lines" with only one node.
  Consequently, the result of selecting *point* or *centroid* as the
  type parameter is that all points/centroids get written into the
  output vector map. The original category numbers of the input
  points/centroids get written to the '*lcat*' attribute in layer 2 of
  the output vector map. All values for *along* are zero in the output
  vector map, as only point geometry was used for input (there is no
  linear distance to calculate *along*, as each point/centroid is the
  start *and* end of its own "line").
- Boundaries are treated as lines, with points getting interpolated
  along the boundary perimeter according to **dmax**. If two adjoining
  polygons share a topological boundary, the boundary only gets
  converted to points once.
- If the **type** parameter is set to *area*, the boundary of each area
  is converted to points *regardless* of whether or not there is a
  topological boundary between adjacent areas. In other words, the
  common boundary of two adjoining areas, for example, gets converted to
  points twice. The centroid is not converted to a point in the output
  vector for *type=area*.

The **use=vertex** option is used to digitize points that fall on the
line's vertices *only*. Parameter **dmax** is ignored in this case.
Similarly to **use=node** when only line's node are used. To filter only
starting/ending nodes use **use=start/end**.

If the **-i** flag is used in conjunction with the **use=vertex**
option, *v.to.points* will digitize points on the line vertices, as well
as interpolate points between line vertices using **dmax** as the
maximum allowable spacing.

Use the **-p** flag to treat **dmax** as a percentage of each line
length. For example, to get points created for the beginning, middle and
end only, use the **-p** flag and set **dmax** so that:

```sh
 50 < dmax <= 100
```

Hence, if **dmax** is between 0.5x and 1.0x the line length, you will
always get points created at exactly the beginning, middle and end of
the input line.

Use the **-r** flag to create points starting from the end node.

## EXAMPLES

### Points along the input lines

In this example, the 'railroads' vector lines map of the North Carolina
sample dataset is used to create points along the input lines:

```sh
# The North Carolina data are metric.
# 200m distance for points (maximum limit but not an exact distance)
v.to.points input=railroads output=railroads_points dmax=200

# verify the two layers in the resulting map
v.category input=railroads_points option=report

# vector info
v.info map=railroads_points
```

### Extract nodes as points

```sh
v.to.points input=railroads output=railroads_nodes use=node
```

### Extract starting/ending nodes as points

```sh
v.to.points input=railroads output=railroads_start use=start

v.to.points input=railroads output=railroads_end use=end
```

## SEE ALSO

*[v.segment](v.segment.md), [v.split](v.split.md),
[v.to.rast](v.to.rast.md), [v.to.db](v.to.db.md)*

## AUTHORS

Radim Blazek  
Updated to GRASS 7 by Martin Landa, Czech Technical University in
Prague, Czech Republic
