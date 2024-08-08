## DESCRIPTION

*v.to.lines* converts vector polygons (boundaries) to lines as well as
vector points to lines via triangulations.

## NOTES

*v.to.lines* is able to convert point data (via triangulation) and areas
to lines (via boundary to line conversion). This script is a wrapper
script to *v.category*, *v.delaunay*, and *v.edit*.

In order to convert an ordered list of coordinates (ASCII table with
x,y\[,z\] coordinates) into vector lines, use *v.in.lines*.

## EXAMPLES

The examples are for the North Carolina sample dataset:

### Area to line conversion

```
v.to.lines input=boundary_municp output=boundary_municp_lines
```

### Point to line conversion

```
v.to.lines input=geodetic_pts output=geodetic_pts_lines
```

## SEE ALSO

*[v.category](v.category.html), [v.delaunay](v.delaunay.html),
[v.edit](v.edit.html), [v.in.lines](v.in.lines.html),
[v.to.points](v.to.points.html), [v.type](v.type.html)*

## AUTHOR

Luca Delucchi, Fondazione Edmund Mach
