## DESCRIPTION

*r.buffer* creates a new raster map showing buffer (a.k.a. "distance" or
"proximity") zones around all cells that contain non-NULL category
values in an existing raster map. The distances of buffer zones from
cells with non-zero category values are user-chosen and must be positive
value(s). Suppose, for example, that you want to place buffer zones
around roads. This program could create the raster map shown below on
the right based on road information contained in the raster map shown on
the left.

```sh
      000000000000000000000000 222233333333333333444444
      111000000000000000000000 111222222222223333333333
      000111111111100000000000 222111111111122223333333
      000000001000011100000000 332222221222211122222222
      000000001000000011111111 333333321233222211111111
      000000001000000000000000 433333321233333222222222
      000000001000000000000000 444443321233333333333333
      000000001000000000000000 444443321233443333333333
      000000001000000000000000 444443321233444444444444

      Category 0: No roads
      Category 1: Road location
      Category 2: Buffer Zone 1 around roads
      Category 3: Buffer Zone 2 around roads
      Category 4: Buffer Zone 3 around roads
```

## NOTES

The user has the option of identifying up to 250 continuous zones. The
zones are identified by specifying the upper limit of each desired zone
(*r.buffer* assumes that `0` is the starting point). "Continuous" is
used in the sense that each category zone's lower value is the previous
zone's upper value. The first buffer zone always has distance `0` as its
lower bound. Buffer distances can be specified using one of five units
with the **units** parameter.

Distances from cells containing the user-specified category values are
calculated using the "fromcell" method. This method locates each cell
that contains a category value from which distances are to be
calculated, and draws the requested distance rings around them. This
method works very fast when there are few cells containing the category
values of interest, but works slowly when there are numerous cells
containing the category values of interest spread throughout the area.

*r.buffer* measures distances from center of cell to center of cell
using Euclidean distance measure for planimetric coordinate reference
systems (like UTM) and using ellipsoidal geodesic distance measure for
latitude/longitude CRS.

*r.buffer* calculates distance zones from all cells having non-NULL
category values in the **input** map. If the user wishes to calculate
distances from only selected **input** map category values, the user
should run (for example) *[r.reclass](r.reclass.md)* prior to
*r.buffer*, to reclass all categories from which distance zones are not
desired to be calculated into category NULL.

The **-z** flag can be used to ignore raster values of zero instead of
NULL values in the input raster map.

When working with massive raster regions consider the
*[r.buffer.lowmem](r.buffer.lowmem.md)* module if RAM use becomes a
problem. The lowmem version can be \> 40x slower, but will work with
minimal memory requirements. The classic *r.buffer* should be able to
deal with raster maps of 32000x32000 size on a system with 1 GB RAM, and
rasters of 90000x90000 on a system with 8 GB RAM without going into
swap.

## EXAMPLE

In the following example (North Carolina sample dataset), the buffer
zones would be (in the default map units of meters): 0-100, 101-200,
201-300, 301-400 and 401-500.  

```sh
g.region raster=roadsmajor -p
r.buffer input=roadsmajor output=roadsmajor_buf distances=100,200,300,400,500
```

Result:

```sh
r.category input=roads.buf
      1       distances calculated from these locations
      2       0-100 meters
      3       100-200 meters
      4       200-300 meters
      5       300-400 meters
      6       400-500 meters
```

![Distances to road](r_buffer_road.png)  
*Distances to road*

## SEE ALSO

*[r.buffer.lowmem](r.buffer.lowmem.md), [r.grow](r.grow.md),
[v.buffer](v.buffer.md)*

*[g.region](g.region.md), [r.cost](r.cost.md),
[r.distance](r.distance.md), [r.grow.distance](r.grow.distance.md),
[r.mapcalc](r.mapcalc.md), [r.reclass](r.reclass.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research
Laboratory  
James Westervelt, U.S. Army Construction Engineering Research Laboratory
