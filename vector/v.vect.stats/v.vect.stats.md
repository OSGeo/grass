## DESCRIPTION

*v.vect.stats* counts the number of points in vector map *points*
falling into each area in vector map *areas*. Optionally statistics on
point attributes in *points* are calculated for each area. The results
are either uploaded to the attribute table of the vector map *areas* or
printed to stdout.

### Statistical methods

Using numeric attribute values of all points falling into a given area,
a new value is determined with the selected method. *v.vect.stats* can
perform the following operations:

**sum**  
The sum of values.

**average**  
The average value of all point attributes (sum / count).

**median**  
The value found half-way through a list of the attribute values, when
these are ranged in numerical order.

**mode**  
The most frequently occurring value.

**minimum**  
The minimum observed value.

**min_cat**  
The point category corresponding to the minimum observed value.

**maximum**  
The maximum observed value.

**max_cat**  
The point category corresponding to the maximum observed value.

**range**  
The range of the observed values.

**stddev**  
The statistical standard deviation of the attribute values.

**variance**  
The statistical variance of the attribute values.

**diversity**  
The number of different attribute values.

## NOTES

Points not falling into any area are ignored. Areas without category (no
centroid attached or centroid without category) are ignored. If no
points are falling into a given area, the point count is set to 0 (zero)
and the statistics result to "null".

The columns *count_column* and *stats_column* are created if not yet
existing. If they do already exist, the *count_column* must be of type
integer and the *stats_column* of type double precision.

In case that *v.vect.stats* complains about the *points_column* of the
input points vector map not being numeric, the module *v.db.update* can
be used to perform type casting, i.e. adding and populating an
additional numeric column with the values type converted from string
attributes to floating point numbers.

## EXAMPLES

### Preparation for examples

The subsequent examples are based on randomly sampled elevation data
(North Carolina sample database):

```sh
# work on map copy for attribute editing
g.copy vector=zipcodes_wake,myzipcodes_wake

# set computational region: extent of ZIP code map, raster pixels
# aligned to raster map
g.region vector=myzipcodes_wake align=elev_state_500m -p
#  generate random elevation points
r.random elev_state_500m vector=rand5000 n=5000
v.colors rand5000 color=elevation

# visualization
d.mon wx0
d.vect myzipcodes_wake -c
d.vect rand5000
```

These vector maps are used for the examples below.

### Count points per polygon with printed output

*See above for the creation of the input maps.*

Counting points per polygon, print results to terminal:

```sh
v.vect.stats points=rand5000 area=myzipcodes_wake -p
```

### Count points per polygon with column update

*See above for the creation of the input maps.*

Counting of points per polygon, with update of "num_points" column (will
be automatically created):

```sh
v.vect.stats points=rand5000 area=myzipcodes_wake count_column=num_points
# verify result
v.db.select myzipcodes_wake column=ZIPCODE_,ZIPNAME,num_points
```

### Average values of points in polygon with printed output

*See above for the creation of the input maps.*

Calculation of average point elevation per ZIP code polygon, printed to
terminal in comma separated style:

```sh
# check name of point map column:
v.info -c rand5000
v.vect.stats points=rand5000 area=myzipcodes_wake \
  method=average points_column=value separator=comma -p
```

Calculation of average point elevation per ZIP code polygon, printed to
terminal using Pandas:

```python
import grass.script as gs
import pandas as pd

data = gs.parse_command(
    "v.vect.stats",
    points="rand5000",
    area="myzipcodes_wake",
    method="average",
    points_column="value",
    flags="p",
    format="json",
)

df = pd.DataFrame(data)
print(df)
```

Possible output

```text
    category  count     average
0          1     27   88.048804
1          2      3  123.612151
2          3     58  117.817925
3          4      8   98.016271
4          5      1  118.571144
5          6     60   80.062659
6          7     47  103.492038
7          8    139   91.205997
8          9     40   96.236051
9         10     70   78.189556
10        11     83   83.004861
11        12      3  102.464809
12        13    146   75.453039
13        14      5  105.863010
14        15      8   92.556496
15        16     74  107.865658
16        17    116   91.719968
17        18     88   80.068764
18        19      7   89.533522
19        20      3  102.381422
20        21      0         NaN
...
40        41     75  103.888415
41        42     91  112.695404
42        43    127  112.014728
43        44      0         NaN
```

### Average values of points in polygon with column update

*See above for the creation of the input maps.*

Calculation of average point elevation per ZIP code polygon, with update
of "avg_elev" column and counting of points per polygon, with update of
"num_points" column (new columns will be automatically created):

```sh
# check name of point map column:
v.info -c rand5000
v.vect.stats points=rand5000 area=myzipcodes_wake count_column=num_points \
  method=average points_column=value stats_column=avg_elev
# verify result
v.db.select myzipcodes_wake column=ZIPCODE_,ZIPNAME,avg_elev
```

### Point statistics in a hexagonal grid

The grid extent and size is influenced by the current computational
region. The extent is based on the vector map *points_of_interest* from
the basic North Carolina sample dataset.

```sh
g.region vector=points_of_interest res=2000 -pa
```

The hexagonal grid is created using the *[v.mkgrid](v.mkgrid.md)* module
as a vector map based on the previously selected extent and size of the
grid.

```sh
v.mkgrid map=hexagons -h
```

The *v.vect.stats* module counts the number of points and does one
statistics on a selected column (here: *elev_m*).

```sh
v.vect.stats points=points_of_interest areas=hexagons method=average \
  points_column=elev_m count_column=count stats_column=average
```

User should note that some of the points may be outside the grid since
the hexagons cannot cover all the area around the edges (the
computational region extent needs to be enlarged if all points should be
considered). The last command sets the vector map color table to
`viridis` based on the `count` column.

```sh
v.colors map=hexagons use=attr column=average color=viridis
```

![Point statistics in a hexagonal grid (count of points,
 average of values associated with point, standard deviation)](v_vect_stats.png)  
*Point statistics in a hexagonal grid (count of points,
 average of values associated with point, standard deviation)*

## SEE ALSO

- *[v.rast.stats](v.rast.stats.md)* for zonal statistics of raster maps
  using vector zones (univariate statistics of a raster map),
- *[r.stats.zonal](r.stats.zonal.md)* for zonal statistics of raster map
  using raster zones (univariate statistics using two raster maps),
- *[v.what.vect](v.what.vect.md)* for querying one vector map by
  another,
- *[v.distance](v.distance.md)* for finding nearest features,
- *[r.distance](r.distance.md)* for computing distances between objects
  in raster maps,
- *[v.mkgrid](v.mkgrid.md)* for creating vector grids to aggregate point
  data.

## AUTHOR

Markus Metz
