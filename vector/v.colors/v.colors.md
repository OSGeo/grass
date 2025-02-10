## DESCRIPTION

*v.colors* allows creating or modifying color table associated with a
vector map similarly to *[r.colors](r.colors.md)* for raster maps.

Color rules are built from features category values (**use=cat**) or
numeric data column (**use=attr**) defined by **column** option. For 3D
vector maps is allowed to define color rules based on points or
centroids z-coordinate (**use=z**). 3D vector lines are not supported.

The **raster** option allows user to specify a raster map from which to
copy the color table, similarly **raster_3d** option for 3D raster map.
Without **use=attr** and **column** options, raster values will be
matched with categories. Use these two options to transfer raster colors
to vector attributes.

The **rules** color table type will cause *v.colors* to read color table
specifications from given file and will build the color table
accordingly. See *[r.colors](r.colors.md)* manual page for details.

If the user specifies the **-w** flag, the current color table file for
the input map will not be overwritten. This means that the color table
is created only if the vector map does not already have a color table.
If this option is not specified, the color table will be created if one
does not exist, or modified if it does.

Alternatively the color rules can be stored in a string column
(**rgb_column**) by saving the RRR:GGG:BBB values suitable for use with
*[d.vect](d.vect.md)*.

## NOTES

For vector maps with a large number of features it's more convenient to
store color rules in an attribute column (given by **rgb_column**)
rather then in a color table file. Reading color tables with more then
1000 items is slow.

## EXAMPLES

### Define color table based on categories

Define color table `wave` based on categories from layer 1

```sh
v.colors map=soils_general layer=1 color=wave
```

### Define color table based on attribute values

Define color table `ryg` based on values from attribute column `AREA`.
Attribute table is linked to layer 1.

```sh
v.to.db map=soils_general layer=1 option=area column=AREA
v.colors map=soils_general layer=1 color=wave use=attr column=AREA
```

### Define color table stored as RGB values in attribute table

Write color values to the attribute table (column `GRASSRGB`) instead of
creating color table.

```sh
v.colors map=soils_general layer=1 color=wave use=attr column=AREA rgb_column=GRASSRGB

# See some GRASSRGB values:
v.db.select map=soils_general where="cat < 4"
cat|OBJECTID|AREA|PERIMETER|GSLNC250_|GSLNC250_I|GSL_NAME|GRASSRGB
1|1|0|164616.125|2|1|NC113|212:42:127
2|2|0|30785.529297|3|2|NC096|212:42:127
3|3|0|87572.882812|4|3|NC097|212:42:127
```

### Convert RGB attribute values into color table

Convert existing RGB values to color table rules.

```sh
v.colors -c map=soils_general rgb_column=GRASSRGB
```

Note that in this case the vector map has a proper color table assigned
(check by *[v.colors.out](v.colors.out.md)*) together with GRASSRGB
attribute column. Also note that color table is preferred over RGB
values stored in attribute table.

### Transfer raster colors to vector

```sh
# create an example raster from census blocks (10m pixel resolution)
g.region vector=censusblk_swwake res=10 -ap
v.to.rast input=censusblk_swwake use=attr attribute_column=TOTAL_POP output=censusblk_swwake_total_pop
r.colors -e map=censusblk_swwake_total_pop color=blues

# transfer raster colors to vector attributes (raster values to attributes)
r.colors.out map=censusblk_swwake_total_pop rules=- | v.colors map=censusblk_swwake use=attr column=TOTAL_POP rules=-
# equivalent, but simpler
v.colors map=censusblk_swwake use=attr column=TOTAL_POP raster=censusblk_swwake_total_pop

# transfer raster colors to vector categories (raster values to categories)
v.colors map=censusblk_swwake raster=censusblk_swwake_total_pop
```

### Remove existing color table

Existing color table can be removed by **-r** flag.

```sh
v.colors -r map=soils_general
```

Before removing color table you can store color rules to the file by
*[v.colors.out](v.colors.out.md)* and later to assign by **rules**
option.

```sh
v.colors.out map=soils_general rules=soils.colr
v.colors map=soils_general rules=soils.colr
```

To drop RGB column use *[v.db.dropcolumn](v.db.dropcolumn.md)*.

```sh
v.db.dropcolumn map=soils_general column=GRASSRGB
```

## SEE ALSO

*[d.vect](d.vect.md), [r.colors](r.colors.md),
[r.colors.out](r.colors.out.md), [r3.colors](r3.colors.md),
[r3.colors.out](r3.colors.out.md), [v.colors.out](v.colors.out.md)*

See also wiki page [Color
tables](https://grasswiki.osgeo.org/wiki/Color_tables) (from GRASS User
Wiki)

[ColorBrewer](https://colorbrewer2.org) is an online tool designed to
help people select good color schemes for maps and other graphics.

## AUTHORS

Martin Landa, OSGeoREL, Czech Technical University in Prague, Czech
Republic  
Huidae Cho
