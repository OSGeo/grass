## DESCRIPTION

***r.mask*** - Facilitates creation of a raster \"MASK\" map to control
raster operations.

The MASK is applied when *reading* an existing GRASS raster map, for
example when used as an input map in a module. The MASK will block out
certain areas of a raster map from analysis and/or display, by
\"hiding\" them from sight of other GRASS modules. Data falling within
the boundaries of the MASK can be modified and operated upon by other
GRASS raster modules; data falling outside the MASK is treated as if it
were NULL.

By default, *r.mask* converts any non-NULL value in the input map,
including zero, to 1. All these areas will be part of the MASK (see the
notes for more details). To only convert specific values (or range of
values) to 1 and the rest to NULL, use the *maskcats* parameter.

Because the MASK created with *r.mask* is actually only a reclass map
named \"MASK\", it can be copied, renamed, removed, and used in
analyses, just like other GRASS raster map layers.

The user should be aware that a MASK remains in place until a user
renames it to something other than \"MASK\", or removes it. To remove a
mask and restore raster operations to normal (i.e., all cells of the
current region), remove the MASK by setting the **-r** remove MASK flag
(`r.mask -r`). Alternatively, a mask can be removed using *g.remove* or
by renaming it to any other name with *g.rename*.

## NOTES

The above method for specifying a \"mask\" may seem counterintuitive.
Areas inside the MASK are not hidden; areas outside the MASK will be
ignored until the MASK file is removed.

*r.mask* uses *r.reclass* to create a reclassification of an existing
raster map and name it `MASK`. A reclass map takes up less space, but is
affected by any changes to the underlying map from which it was created.
The user can select category values from the input raster to use in the
MASK with the *maskcats* parameter; if *r.mask* is run from the command
line, the category values listed in *maskcats* must be quoted (see
example below). Note that the *maskcats* can only be used if the input
map is an integer map.

### Different ways to create a MASK

The *r.mask* function creates a MASK with values 1 and NULL. But note
that a MASK can also be created using other functions that have a raster
as output, by naming the output raster \'MASK\'. Such layers could have
other values than 1 and NULL. The user should therefore be aware that
grid cells in the MASK map containing `NULL` or `0` will replace data
with NULL, while cells containing other values will allow data to pass
through unaltered. This means that:

If a binary map with \[0,1\] values is used as input in *r.mask*, all
raster cells with 0 and 1 will be part of the MASK. This is because
*r.mask* converts all non-NULL cells to 1.

```
r.mapcalc -s "map1 = round(rand(0,1))"
r.mask raster=map1
```

On the other hand, if a binary map is used as an input in *g.copy* to
create a MASK, only the raster cells with value 1 will be part of the
MASK.

```
r.mapcalc -s "map2 = round(rand(0,1))"
g.copy raster=map2,MASK
```

### Handling of floating-point maps

*r.mask* treats floating-point maps the same as integer maps (except
that floating maps are not allowed in combination with the *maskcats*
parameter); all non-NULL values of the input raster map are converted to
1 and are thus part of the MASK. In the example below, all raster cells
are part of the MASK, i.e., nothing is blocked out from analysis and/or
display.

```
r.mapcalc -s "map3 = rand(0.0,1.0)"
r.mask raster=map3
```

However, when using another method than *r.mask* to create a mask, the
user should be aware that the MASK is read as an integer map. If MASK is
a floating-point map, the values will be converted to integers using the
map\'s quantisation rules (this defaults to round-to-nearest, but can be
changed with r.quant).

```
r.mapcalc -s "map4 = rand(0.0,1.0)"
g.copy raster=map4,MASK
```

In the example above, raster cells with a rounded value of 1 are part of
the MASK, while raster cells with a rounded value of 0 are converted to
NULL and consequently blocked out from analysis and/or display.

## EXAMPLES

The examples are based on the North Carolina sample dataset.

Create a raster mask, for constraining the calculation of univariate
statistics of the elevation values for \"lakes\":

```
# set computation region to lakes raster map
g.region raster=lakes -p
# use lakes as MASK
r.mask raster=lakes
# get statistics for elevation pixels of lakes:
r.univar elevation
```

Remove the raster mask (\"MASK\" map) with the -r flag:

```
r.mask -r
```

Creating a mask from selected categories in the North Carolina
\'geology_30m\' raster map:

```
g.region raster=geology_30m -p
r.category geology_30m
d.mon wx0
d.rast geology_30m
r.mask raster=geology_30m maskcats="217 thru 720"
d.mon wx0
d.rast geology_30m
```

## SEE ALSO

*[g.region](g.region.html), [r.mapcalc](r.mapcalc.html),
[r.reclass](r.reclass.html), [g.remove](g.remove.html),
[g.rename](g.rename.html) [r.quant](r.quant.html)*

## AUTHOR

Michael Barton, Arizona State University
