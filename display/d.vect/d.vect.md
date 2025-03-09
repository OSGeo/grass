## DESCRIPTION

*d.vect* displays vector maps in the active frame on the graphics
monitor.

## NOTES

*d.vect* can simply be used typing `d.vect map=vector_map`. There are a
large variety of optional parameters which allow the user to specify
vector type, colors, data fields, SQL queries, label size and
justification, etc.

When *d.vect* is used with **where** parameter on MS Windows Command
Prompt, it is important to use `ˆ` caret symbol for escaping special
characters `< > ( ) & | , ; "`.

```sh
d.vect map=vector_map where="cat ˆ> 10 AND cat ˆ< 20"
```

By default *d.vect* areas are filled with **fill_color** and outlined
with **color**. Area outlines can be suppressed with

```sh
d.vect map=vector_map color=none
```

and areas can be made transparent with

```sh
d.vect map=vector_map fill_color=none
```

In order to display attributes in the map, **attribute_column** must be
specified.

Feature colors may be specified by *[v.colors](v.colors.md)* in a form
of color table or in an attribute table column containing `RRR:GGG:BBB`
values.

A table for a vector map might look like this:

```sh
db.select sql="select * from testisola"
cat|label|GRASSRGB
0|no data|255:255:255
90|FRASSILONGO|23:245:67
104|LEVICO|23:145:67
139|PERGINE VALSUGANA|223:45:237
168|SANT'ORSOLA|223:45:67
190|TENNA|123:45:67
```

To add the GRASSRGB color column, use
*[v.db.addcolumn](v.db.addcolumn.md)*:

```sh
v.db.addcolumn map=testisola columns="GRASSRGB varchar(11)"
```

To add/change a color, use *[v.db.update](v.db.update.md)*:

```sh
v.db.update map=testisola column=GRASSRGB value="123:45:237" where="cat=139"
```

A much simpler method of color coding is by using the **-c** flag which
displays vector elements of like category number with a random color.

This module can use FreeType/TrueType fonts if they have already been
selected with *[d.font](d.font.md)*.

Parameter **width** is set by default to '0'. XDRIVER specifies the
precise behaviour for non-zero line width, but drivers have some freedom
as to how zero-width lines are handled, so they can use the hardware's
"thin line" drawing primitive, if it has one. A width of zero can
potentially result in significantly faster operation. On drivers where
there is no such thing as a "thin" line, the driver will use a sensible
default (which might not be the same as '1').

## EXAMPLES

Spearfish examples:

```sh
# display roads with category numbers:
d.vect map=roads display=shape,cat label_color=green

# display randomly colorized soils map with attributes
d.vect -c map=soils attribute_column=label

# display randomly colorized selected vectors from soils map
d.vect -c map=soils where="label='VBF'" display=shape attribute_column=label
```

3D points, 3D lines and 3D polygons colorized according to z height (for
3D lines and polygons a z height is computed by a midpoint of line/area
bounding box):

```sh
g.region raster=elevation.10m
r.random input=elevation.10m n=5000 vector=random3d -d
d.mon start=x0
# display as black points
d.vect map=random3d
# display 3D points colorized according to z height
d.vect map=random3d zcolor=gyr

# 3D contour lines
r.contour input=elevation.10m output=contour20m step=20
d.vect map=contour20m zcolor=gyr

# generate 3D triangles
v.delaunay input=random3d output=random3d_del
# display 3D polygons colorized according to z height
d.vect map=random3d_del type=area zcolor=gyr
```

## SEE ALSO

*[v.colors](v.colors.md), [d.erase](d.erase.md), [d.rast](d.rast.md),
[v.colors](v.colors.md), [v.db.addcolumn](v.db.addcolumn.md),
[v.db.update](v.db.update.md)*

*[GRASS SQL interface](sql.md)*

## AUTHORS

CERL  
Radim Blazek, ITC-Irst, Trento, Italy  
Support for color tables by Martin Landa, Czech Technical University in
Prague (8/2011)  
and many other GRASS developers
