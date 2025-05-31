## DESCRIPTION

*v.to.db* loads vector map features or metrics into a database table, or
prints them (or the SQL queries used to obtain them) in a form of a
human-readable report. For uploaded/printed category values '-1' is used
for 'no category' and 'null'/'-' if category cannot be found or multiple
categories were found. For line azimuths '-1' is used for closed lines
(start equals end).

## NOTES

Measures of lengths and areas are always reported in meters, unless the
*unit* parameter is set. The units *miles*, *feet*, *meters* and
*kilometers* are square for *option=area*.

Feet and acre units are always reported in their common versions (i.e.
the International Foot, exactly 5280 feet in a mile), even when the
coordinate reference system's standard map unit is the US Survey foot.

When calculating perimeters in Latitude-Longitude CRS, the geodesic
distance between the vertices is used.

When using *option=coor* on a vector area map, only coordinates of
centroids with unique category will be reported.

When using *option=bbox* on a vector area map with more than one feature
per category value, the results corresponds to the bounding box of all
features of same category taken together.

Line azimuth is calculated as angle from the North direction to the line
endnode direction at the line statnode. By default it's reported in
decimal degrees (0-360, CW) but it also may be reported in radians with
*unit=radians*. Azimuth value **-1** is used to report closed line with
it's startnode and endnode being in same place. Azimuth values make
sense only if every vector line has only one entry in database (unique
CAT value).

If the module is apparently slow *and* the map attributes are stored in
an external DBMS such as PostgreSQL, it is highly recommended to create
an index on the key (category) column.

Uploading the vector map attributes to a database requires a table
attached to a given input vector *layer*. The **print only** (**-p**)
mode doesn't require a table. Use *[db.execute](db.execute.md)* or
*[v.db.addtable](v.db.addtable.md)* to create a table if needed.

Updating the table has to be done column-wise. The *column* will be
created in the table if it doesn't already exist, except when using the
**print only** (**-p**) mode. If the *column* exists, the
**--overwrite** flag is required to overwrite it.

## EXAMPLES

### Updating attribute tables

Upload category numbers to attribute table (used for new map):  

```sh
v.to.db map=soils type=centroid option=cat
```

Upload polygon areas to corresponding centroid record in the attribute
table:  

```sh
v.to.db map=soils type=centroid option=area columns=area_size unit=h
```

Upload line lengths (in meters) of each vector line to attribute table
(use *v.category* in case of missing categories):  

```sh
v.to.db map=roads option=length type=line columns=linelength units=me
```

Upload x and y coordinates from vector geometry to attribute table:  

```sh
v.to.db map=pointsmap option=coor columns=x,y
```

Upload x, y and z coordinates from vector geometry to attribute table:  

```sh
v.to.db map=pointsmap option=coor columns=x,y,z
```

Transfer attributes from a character column (with numeric contents) to a
new integer column:  

```sh
v.db.addcolumn usa_income_employment2002 col="FIPS_NUM integer"
v.to.db usa_income_employment2002 option=query columns=FIPS_NUM query_column=STATE_FIPS
```

Upload category numbers of left and right area, to an attribute table of
boundaries common for the areas:  

```sh
# add categories for boundaries of the input vector map, in layer 2:
v.category soils out=mysoils layer=2 type=boundary option=add
# add a table with columns named "left" and "right" to layer 2 of the input
# vector map:
v.db.addtable mysoils layer=2 columns="left integer,right integer"
# upload categories of left and right areas:
v.to.db mysoils option=sides columns=left,right layer=2
# display the result:
v.db.select mysoils layer=2
```

Compute *D_L*, the Fractal Dimension (Mandelbrot, 1982), of the
boundary defining a polygon based on the formula:  
`D = 2 * (log perimeter) / (log area):`  

```sh
g.copy vect=soils,mysoils
v.db.addcolumn mysoils col="d double precision"
v.to.db mysoils option=fd column="d"

g.region vector=mysoils res=50
v.to.rast input=mysoils output=soils_fd type=area use=attr attribute_column=d
r.colors map=soils_fd color=gyr

d.mon wx0
d.rast.leg soils_fd
d.vect mysoils type=boundary
```

### Printing reports

Report x,y,z coordinates of points in the input vector map:  

```sh
v.to.db -p bugsites option=coor type=point
```

Report all area sizes of the input vector map:  

```sh
v.to.db -p soils option=area type=boundary units=h
```

Report all area sizes of the input vector map, in hectares, sorted by
category number (requires GNU *sort* utility installed):  

```sh
v.to.db -p soils option=area type=boundary units=h | sort -n
```

Report all line lengths of the input vector map, in kilometers:  

```sh
v.to.db -p roads option=length type=line units=k
```

Report number of features for each category in the input vector map:  

```sh
v.to.db -p roads option=count type=line
```

## REFERENCES

- Mandelbrot, B. B. (1982). The fractal geometry of nature. New
  York: W. H. Freeman.
- Xu, Y. F. & Sun, D. A. (2005). Geotechnique 55, No. 9, 691-695

## SEE ALSO

*[d.what.vect](d.what.vect.md), [db.execute](db.execute.md),
[v.category](v.category.md), [v.db.addtable](v.db.addtable.md),
[v.db.addcolumn](v.db.addcolumn.md), [v.db.connect](v.db.connect.md),
[v.distance](v.distance.md), [v.report](v.report.md),
[v.univar](v.univar.md), [v.what](v.what.md)*

## AUTHORS

Radim Blazek, ITC-irst, Trento, Italy  
Line sinuosity implemented by Wolf Bergenheim
