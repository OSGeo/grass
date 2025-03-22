## DESCRIPTION

*v.distance* finds the nearest element in vector map (**to**) for
elements in vector map (**from**). Various information about the
vectors' relationships (distance, category, etc.) may be uploaded to the
attribute table attached to the first vector map, or printed to
'stdout'. A new vector map may be created where lines connecting nearest
points on features are written. **dmin** and/or **dmax** can be used to
limit the search radius (in lat-long projects to be given in meters
since they are calculated as geodesic distances on a sphere).

For lines to lines, say line A to line B, *v.distance* calculates the
shortest distance of each vertex in A with each segment (not vertex) in
B. The module then calculates the shortest distance of each vertex in B
to each segment in A. The overall shortest distance of A points to B
segments and B points to A segments is used. Additionally, *v.distance*
checks for intersections. In case of intersections, the first
intersection found is used and the distance set to zero.

For lines to areas, the distance is set to zero if a line is (partially)
inside an area. The first point of the line that is inside the area is
used as common point. The distance is also set to zero if the line
intersects with the outer ring or any of the inner rings (isles), in
which case the fist intersection is used as common point.

For areas to areas, the module checks first for overlap or if one area
is (partially) inside the other area. This is computationally quite
intensive. If the outer rings of the two areas do not overlap, the
distance is calculated as above for lines to lines, treating the outer
rings as two lines. Again, the first point encountered falling into an
area is used as common point, or the first intersection point.

For anything else than points to lines, there can be several common
locations with zero distance, and the common location would then be the
result of an overlay consisting of several points, lines, or areas.
*v.distance* selects in these cases a single point, and does not create
an overlay like *[v.overlay](v.overlay.md)*. In this implementation, any
shared point is as good as any other. Calculating an intersection is
costlier than to check if a vertex is inside a polygon. For example, if
a vertex of the boundary of the 'to' area is inside the 'from' area, it
is a common location. For speed reasons, the distance is then set to
zero and no further tests are done.

## NOTES

If a nearest feature does not have a category, the attribute column is
updated to NULL.

The upload **column**(s) must already exist. Create one with
*[v.db.addcolumn](v.db.addcolumn.md)*.

In lat-long projects *v.distance* gives distances (*dist*, *from_along*,
and *to_along*) not in degrees but in meters calculated as geodesic
distances on a sphere.

If one or both of the input vector maps are 3D, the user is notified
accordingly.

The *-p* flag prints the results to standard output. By default the
output is in form of a linear matrix. If only only variable is uploaded
and a square matrix is desired, the user can set the *-s* flag.

## EXAMPLES

### Find nearest lines

Find *nearest lines* in vector map "ln" for points from vector map "pnt"
within the given threshold and write related line categories to column
"linecat" in an attribute table attached to vector map "pnt":

```sh
v.distance from=pnt to=ln upload=cat column=linecat
```

### Find nearest area

For each point from vector map "pnt", find the *nearest area* from map
"ar" within the given threshold and write the related area categories to
column "areacat" in an attribute table attached to vector map "pnt" (in
the case that a point falls into an area, the distance is zero):

```sh
v.distance from=pnt to=ar upload=cat column=areacat
```

### Create a new vector map

Create a new vector map which contains *lines connecting nearest
features* of maps "pnt" and map "ln". The resulting vector map can be
used for example to connect points to a network as needed for network
analysis:

```sh
v.distance from=pnt to=ln out=connections upload=dist column=dist
```

### Create a new vector map with from and to categories in the attribute table

Create a new vector map that contains *lines connecting nearest
features* of maps "pnt" and map "ln", and a new attribute table that
contains distances, from and to categories from the input maps:

```sh
v.distance from=pnt to=ln out=connections upload=cat,dist column=to_cat,dist table=connections
```

### Query information

Query information from selected point(s). *v.distance* takes points from
a vector map as input instead of stdin. A new vector map with query
points has to be created before the map can be analysed.

Create query map (if not present):

```sh
echo "123456|654321|1" | v.in.ascii output=pnt
```

Find nearest features:

```sh
v.distance -p from=pnt to=map_to_query upload=cat
```

### Point-in-polygon

The option **dmax=0** is here important because otherwise for points not
falling into any area, the category of the nearest area is recorded.  
For each point from vector map "pnt", find the *area* from vector map
"ar" in which the individual point falls, and write the related area
categories to column "areacat" into the attribute table attached to
vector map "pnt":

```sh
v.distance from=pnt to=ar dmax=0 upload=cat column=areacat
```

### Univariate statistics on results

Create a vector map containing connecting lines and investigate mean
distance to targets. An alternative solution is to use the
`v.distance upload=dist` option to upload distances into the *bugs*
vector directly, then run v.univar on that. Also note you can upload two
columns at a time, e.g.
`v.distance upload=cat,dist column=nearest_id,dist_to_nr`.

```sh
# create working copy
g.copy vect=bugsites,bugs

# add new attribute column to hold nearest archsite category number
v.db.addcolumn map=bugs column="nrst_arch INTEGER"

v.distance from=bugs to=archsites to_type=point upload=to_attr \
  to_column=cat column=nrst_arch out=vdistance_vectors_raw

# we need to give the lines category numbers, create a table, and create
#  a column in that table to hold the distance data.
v.category vdistance_vectors_raw out=vdistance_vectors type=line op=add
g.remove -f type=vector name=vdistance_vectors_raw

v.db.addtable map=vdistance_vectors column="length DOUBLE"
v.to.db map=vdistance_vectors option=length column=length

# calculate statistics
v.univar vdistance_vectors column=length
```

### Print distance between points

Example for a Latitude-longitude project (EPSG 4326):

```sh
# points along the equator
echo "0|-61|1" | v.in.ascii output=pnt1 input=-
echo "0|-58|1" | v.in.ascii output=pnt2 input=-

# here, distances are in degree units
v.distance -p --q from=pnt1 to=pnt2 upload=dist
from_cat|distance
1|3
```

### Print distance matrix

North Carolina sample data

As linear matrix:

```sh
v.distance -pa from=hospitals to=hospitals upload=dist,to_attr to_column=NAME separator=tab
from_cat    to_cat  dist    to_attr
1   1   0   Cherry Hospital
1   2   7489.1043632983983  Wayne Memorial Hospital
1   3   339112.17046729225  Watauga Medical Center
1   4   70900.392145909267  Central Prison Hospital
1   5   70406.227393921712  Dorothea Dix Hospital
```

As square matrix (only possible with single upload option):

```sh
v.distance -pas from=hospitals to=hospitals upload=dist separator=tab
from_cat to_cat       dist
              1          2          3          4          5 ...
1             0    7489.10  339112.17   70900.39   70406.23 ...
2       7489.10          0  345749.12   76025.46   75538.87 ...
3     339112.17  345749.12          0  274153.19  274558.98 ...
4      70900.39   76025.46  274153.19          0     501.11 ...
5      70406.23   75538.87  274558.98     501.11          0 ...
...
```

### Print in JSON

```sh
v.distance -p from=busroute_a to=busstopsall upload=dist,to_attr to_column=routes format=json
```

```json
[
    {
        "from_cat": 1,
        "to_cat": 112,
        "distances": [
            {
                "value": 0.1428123184481199,
                "name": "dist"
            },
            {
                "value": "8,A",
                "name": "to_attr"
            }
        ]
    },
    {
        "from_cat": 2,
        "to_cat": 44,
        "distances": [
            {
                "value": 0.10232660032693719,
                "name": "dist"
            },
            {
                "value": "9,A",
                "name": "to_attr"
            }
        ]
    }
]
```

## SEE ALSO

*[r.distance](r.distance.md), [v.db.addcolumn](v.db.addcolumn.md),
[v.what.vect](v.what.vect.md)*

## AUTHORS

Janne Soimasuo 1994, University of Joensuu, Faculty of Forestry,
Finland  
Cmd line coordinates support: Markus Neteler, ITC-irst, Trento, Italy  
Updated for 5.1: Radim Blazek, ITC-irst, Trento, Italy  
Matrix-like output by Martin Landa, FBK-irst, Trento, Italy  
Improved processing speed: Markus Metz  
Distance from any feature to any feature: Markus Metz  
New table without the -p flag: Huidae Cho Make linear matrix the default
for all outputs: Moritz Lennert
