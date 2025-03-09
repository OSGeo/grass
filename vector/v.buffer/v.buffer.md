## DESCRIPTION

*v.buffer* creates a buffer around features of given **type**, which
have a category in the given **layer**. The **tolerance** controls the
number of vector segments being generated (the smaller the value, the
more vector segments are generated).

## NOTES

Internal buffers for areas can be generated with negative distance
values ("inward buffer" or "negative buffer" or "shrinking").

*v.buffer* fusions the geometries of buffers by default. Categories and
attribute table will not be transferred (this would not make sense as
one buffer geometry can be the result of many different input
geometries). To transfer the categories and attributes the user can set
the **-t** flag. This will result in buffers being cut up where buffers
of individual input geometries overlap. Each part that is the result of
overlapping buffers of multiple geometries will have multiple categories
corresponding to those geometries, and multiple buffer areas can have
the same category. The buffer for the input feature with category X can
thus be retrieved by selecting all buffer areas with category X (see
example below).

Buffers for lines and areas are generated using the algorithms from the
GEOS library.

*For advanced users:* the built-in buffer algorithm is no longer used,
as we use GEOS instead. If GRASS was not compiled with GEOS support or
the [environmental variable](variables.md) `GRASS_VECTOR_BUFFER` is
defined, then GRASS generates buffers using the built-in buffering
algorithm (which is still buggy for some input data).

The options **minordistance**, **angle**, **tolerance** are kept for
backward compatibility and have no effect with GEOS buffering.

### Corner settings

The following vector line related corners (also called "cap") exist:

- no cap:       ![line buffer: no cap](v_buffer_no_cap.png)
- rounded cap: ![line buffer: rounded cap](v_buffer_rounded_cap.png)
- square cap: ![line buffer: square cap](v_buffer_square_cap.png)

By default *v.buffer* creates rounded buffers (blue color on figure
below):

![v_buffer_line](v_buffer_line.png)

Straight corners with caps are created using the **-s** flag (red color
on the figure below), while the **-c** flag doesn't make caps at the
ends of polylines (green color on the figure below):

![v_buffer_line_s](v_buffer_line_s.png)

With a point vector map as input data, square buffers are created
instead of round buffers by using the **-s** flag.

![v_buffer_point_s](v_buffer_point_s.png)

## EXAMPLES

All examples are based on the North Carolina sample dataset.

### Buffer around input lines

```sh
v.buffer input=roadsmajor output=roadsmajor_buffer type=line distance=100
```

![Buffer of 100m along the roadsmajor lines](v_buffer_lines.png)  
*Buffer of 100m along the "roadsmajor" lines (map subset, original center
line shown in black)*

### Circles around input points

```sh
v.buffer input=hospitals output=hospitals_circled type=point distance=2000
```

![Buffer of 2000m around the hospitals points](v_buffer_points.png)  
*Buffer of 2000m around the "hospitals" points (map subset, original
points shown in black, new area centroids in red)*

### Circles around input points with attribute transfer

```sh
v.buffer input=hospitals output=hospitals_circled type=point distance=1000 -t

# display buffer around hospital with category 36,
# this buffer is composed of several areas:
d.vect map=hospitals_circled type=area layer=1 cats=36
# extract this buffer, dissolving boundaries
v.extract in=hospitals_circled output=hospital_36_circled layer=1 cats=36 -d
```

### Buffer around input areas

```sh
v.buffer input=lakes output=lakes_buffer type=area distance=100
```

![Buffer of 100m around the "lakes" polygons](v_buffer_areas.png)  
*Buffer of 100m around the "lakes" polygons (map subset, original areas
shown in black)*

### Buffer inside input areas

In this example, an internal buffer ("inward buffer" or "negative
buffer") is generated using a negative **distance** value:

```sh
v.buffer input=lakes output=lakes_buffer type=area distance=-50
```

![Internal buffer of 50m inside the lakes polygons](v_buffer_areas_int.png)  
*Internal buffer of 50m inside the "lakes" polygons (map subset, original
areas shown in black)*

Not all features are buffered, only the polygons that allow creation
inside a topological cleaned polygon.

## REFERENCES

- [GEOS Library](https://trac.osgeo.org/geos)

## SEE ALSO

*[r.buffer](r.buffer.md), [v.parallel](v.parallel.md),
[v.extract](v.extract.md), [v.type](v.type.md), [v.patch](v.patch.md),
[v.db.connect](v.db.connect.md)*

## AUTHORS

Radim Blazek  
Rewritten by Rosen Matev (with support through the Google Summer of Code
program 2008)  
Rewritten by Markus Metz (2011, 2012)
