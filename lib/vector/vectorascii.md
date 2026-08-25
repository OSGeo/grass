---
description: GRASS ASCII vector format specification
index: vector
---

# GRASS ASCII vector format specification

A vector map in GRASS native vector format may contain a mix of
*primitives* including points, lines, boundaries, centroids, areas,
faces, and kernels. The GRASS ASCII vector format may contain also a
*header* with various metadata (see example below).

The header is similar as the head file of vector binary format but
contains bounding box also. Key words are:

```sh
ORGANIZATION
DIGIT DATE
DIGIT NAME
MAP NAME
MAP DATE
MAP SCALE
OTHER INFO
ZONE
WEST EDGE
EAST EDGE
SOUTH EDGE
NORTH EDGE
MAP THRESH
```

The body begins with the row:

```sh
VERTI:
```

followed by records of primitives:

```sh
TYPE NUMBER_OF_COORDINATES [NUMBER_OF_CATEGORIES]
 X Y [Z]
....
 X Y [Z]
[ LAYER CATEGORY]
....
[ LAYER CATEGORY]
```

Everything above in `[ ]` is optional.

The primitive codes are as follows:

- 'P': point
- 'L': line
- 'B': boundary
- 'C': centroid
- 'F': face (3D boundary)
- 'K': kernel (3D centroid)
- 'A': area (boundary) - better use 'B'; kept only for backward
  compatibility

The coordinates are listed following the initial line containing the
primitive code, the total number of coordinates in the series, and
(optionally) the number of categories (1 for a single layer, higher for
multiple layers). Below that 1 or several lines follow to indicate the
layer number and the category number (ID).

The order of coordinates is

```sh
X Y [Z]
```

In pre-GRASS 6 versions of the ASCII format, the order of coordinates
was different:

```sh
Y X
```

Latitude/Longitude data may be given in a number of ways. Decimal
degrees must be positive or negative instead of using a hemisphere
letter. Mixed coordinates must use a hemisphere letter. Whole minutes
and seconds must always contain two digits (example: use
`167:03:04.567`; and not `167:3:4.567`).  
  
Acceptable formats:  
*key: D=Degrees; M=Minutes; S=Seconds; h=Hemisphere (N,S,E,W)*

- `(+/-)DDD.DDDDD`
- `DDDh`
- `DDD:MMh`
- `DDD:MM.MMMMMh`
- `DDD:MM:SSh`
- `DDD:MM:SS.SSSSSh`

## EXAMPLES

```sh
ORGANIZATION: GRASS Development Team
DIGIT DATE:   1/9/2005
DIGIT NAME:   -
MAP NAME:     test
MAP DATE:     2005
MAP SCALE:    10000
OTHER INFO:   Test polygons
ZONE:  0
MAP THRESH:   0.500000
VERTI:
B  6
 5958812.48844435 3400828.84221011
 5958957.29887089 3400877.11235229
 5959021.65906046 3400930.7458436
 5959048.47580612 3400973.65263665
 5959069.92920264 3401032.64947709
 5958812.48844435 3400828.84221011
C  1 1
 5958952.42189184 3400918.23126419
 1 20
B  4
 5959010.9323622 3401338.36037757
 5959096.7459483 3401370.54047235
 5959091.38259917 3401450.99070932
 5959010.9323622 3401338.36037757
C  1 1
 5959063.08352122 3401386.98533277
 1 21
```

In this case the vector map contains 2 boundaries (first boundary with 6
vertices, second with 4 vertices) without category and 2 centroids with
category number 20 and 21 (layer 1).

## SEE ALSO

*[v.in.ascii](v.in.ascii.md), [v.out.ascii](v.out.ascii.md),
[v.edit](v.edit.md), [v.support](v.support.md)*
