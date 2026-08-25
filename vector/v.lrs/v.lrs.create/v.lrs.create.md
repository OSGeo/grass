## DESCRIPTION

*v.lrs.create* generates a LRS (Linear Reference System) from vector
line and point data.

It is highly recommended to work with polylines instead of segmented
vector lines. The command *v.build.polylines* creates this map
structure.

## NOTES

The mileposts (point) vector map columns *start_mp*, *start_off*,
*end_mp*, *end_off* must be of 'double precision' type. For milepost
ordering, it is sufficient to enter increasing numbers into the
*start_mp* column indicating the order along the vector line.

The *lidcol* and *pidcol* columns contain the line IDs which relate
mileposts and vector line(s) to each other.

When creating a LRS with this module, any existing *rstable* will be
replaced.

## EXAMPLE

This example is written for the Spearfish dataset.

As first step, bus route data are prepared.

```sh
# break into segments for correct route selection
v.clean roads_net out=busroute_tmp tool=break

# make polyline for easier line selection by coordinate pairs
v.build.polylines busroute_tmp out=busroute_tmp2

# reverse delete: reduce route map to bus route (enter in one line)
v.edit -r busroute_tmp2 tool=delete coords=590273,4927304,\
590346,4927246,590414,4927210,590438,4927096,590468,4926966,\
590491,4926848,590566,4926798,590637,4926753,590701,4926698,\
590830,4926726,590935,4926751,590993,4926830,590972,4926949,\
590948,4927066,590922,4927182,590957,4927251 threshold=5

# vector line needs to be polyline
v.build.polylines busroute_tmp2 out=busroute_tmp3
v.category busroute_tmp3 out=busroute op=add
g.remove -f type=vector name=busroute_tmp,busroute_tmp2,busroute_tmp3
```

The result can be visualized:

```sh
g.region vector=busroute n=n+100 s=s-100 w=w-100 e=e+100
d.mon x0
d.vect roads_net
d.vect busroute col=red width=2
```

The vector map 'busroute' needs have an attribute table which contain an
integer column *lidcol* with value be '22' for this example (bus route):

```sh
v.db.addtable busroute col="lid integer"
v.db.update busroute col=lid value=22
v.db.select busroute
cat|lid
1|22
```

A new point map 'busstops' shall contain mileposts (bus stops) along
this line (use *thresh* to define maximal accepted deviation from this
line):

```sh
# generate points map
echo "590263|4927361
590432|4927120
590505|4926776
590660|4926687
590905|4926742
590972|4926949
591019|4927263" | v.in.ascii out=busstops

d.vect busstops icon=basic/triangle col=blue
d.vect busstops disp=cat lcol=blue
```

The milepost attributes table needs to be created with specific columns:

```sh
v.db.addtable busstops col="lid integer, start_mp double precision, \
            start_off double precision, end_mp double precision, \
            end_off double precision"
v.db.update busstops col=lid value=22
```

Since the digitizing order of v.in.ascii above reflects the bus stop
order along the route, we can simply copy the category number as
milepost order number in column *start_mp*:

```sh
v.db.update busstops col=start_mp qcol=cat
# verify table
v.db.select busstops
cat|lid|start_mp|start_off|end_mp|end_off
1|22|1|||
2|22|2|||
3|22|3|||
4|22|4|||
5|22|5|||
6|22|6|||
7|22|7|||

# visualize with start_mp to check order
d.erase
d.vect roads_net
d.vect busroute col=red width=2
d.vect busstops icon=basic/triangle col=blue
d.vect busstops disp=attr attrcol=start_mp lcol=blue
```

Offsets (*start_off*, *end_off*) can be later used in case the route or
mileposts get modified.

As second step, the linear reference network is created:

```sh
v.lrs.create busroute points=busstops out=route_lrs err=lrs_error \
             lidcol=lid pidcol=lid rstable=route_lrs threshold=50
```

This creates the maps 'route_lrs' containing the LRS and 'lrs_error'
containing the errors if any. The resulting LRS table and map can be
shown:

```sh
# show LRS table
db.select table=route_lrs

d.vect route_lrs col=blue width=2
```

## SEE ALSO

*[v.build.polylines](v.build.polylines.md),
[v.lrs.segment](v.lrs.segment.md), [v.lrs.where](v.lrs.where.md),
[v.lrs.label](v.lrs.label.md)*

*[LRS tutorial](lrs.md),  
[Introducing the Linear Reference System in
GRASS](https://foss4g.asia/2004/Full-Paper_PDF/Introducing-the-Linear-Reference-System-in-GRASS.pdf)*

## AUTHORS

Radim Blazek, ITC-irst/MPA Solutions  
Documentation update (based on above journal article and available
fragments): Markus Neteler
