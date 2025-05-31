## DESCRIPTION

*r.mask* facilitates the creation and management of a raster mask to
control raster operations. While the computational region specifies the
extent (rectangular bounding box) and resolution, the mask specifies the
area that should be considered for the operations and area which should
be ignored. The mask is represented as a raster map called `MASK` by
default.

The mask is applied when *reading* an existing GRASS raster map, for
example when used as an input map in a module. The mask will block out
certain areas of a raster map from analysis and/or display, by "hiding"
them from sight of other GRASS modules. Data falling within the
boundaries of the mask can be modified and operated upon by other GRASS
raster modules; data falling outside the mask is treated as if it were
NULL.

By default, *r.mask* converts any non-NULL value in the input map,
including zero, to 1. All these areas will be part of the mask (see the
notes for more details). To only convert specific values (or range of
values) to 1 and the rest to NULL, use the *maskcats* parameter.

Because the mask raster map created with *r.mask* is actually only a
reclass map named `MASK` by default, it can be copied, renamed, removed,
and used in analyses, just like other GRASS raster maps.

The user should be aware that a mask remains in place until it is
removed or renamed. To remove a mask and restore raster operations to
normal (i.e., all cells of the current region), remove the mask by
setting the **-r** flag (`r.mask -r`). Alternatively, a mask can be
removed using *g.remove* or by renaming it to any other name with
*g.rename*.

The `GRASS_MASK` environment variable can be used to specify the raster
map which will be used as a mask. If the environment variable is not
defined, the name `MASK` is used instead.

## NOTES

The above method for specifying a "mask" may seem counterintuitive.
Areas inside the mask are not hidden; areas outside the mask will be
ignored until the mask is removed.

*r.mask* uses *r.reclass* to create a reclassification of an existing
raster map and names it `MASK` by default. A reclass map takes up less
space, but is affected by any changes to the underlying map from which
it was created. The user can select category values from the input
raster to use in the mask with the *maskcats* parameter; if *r.mask* is
run from the command line, the category values listed in *maskcats* must
be quoted (see example below). Note that the *maskcats* can only be used
if the input map is an integer map.

### Different ways to create a mask

Mask can be created using *r.mask* or by creating a mask raster map
directly.

The *r.mask* tool creates a mask with values 1 and NULL. By default,
*r.mask* converts all non-NULL cells to 1. If a raster with ones (1) and
NULLs values is used with *r.mask*, all raster cells with value 1 will
be included in the computation, while those with NULL will be masked
out.

```sh
r.mapcalc -s "map1 = if(row() < col(), 1, null())"
r.mask raster=map1
```

If a binary map with zeros and ones as values is used with *r.mask*, and
both NULLs and zeros should be masked out, the **maskcats** parameter
can be used to specify the values that should be masked out:

```sh
r.mapcalc -s "map2 = if(row() < col(), 1, 0)"
r.mask raster=map2 maskcats="0"
```

Mask can also be created directly using any tools that have a raster as
output, by naming the output raster `MASK` (or whatever the `GRASS_MASK`
environment variable is set to). Both NULLs and zeros will be masked out
in the raster mask. This allows for creation of a simple binary raster
with only ones and zeros where cells with zeros in the mask raster are
excluded from the computation (behaving as if they were NULL). A raster
with zeros and ones can be created and used directly as a mask by naming
the output raster `MASK`, e.g., using raster algebra:

```sh
r.mapcalc -s "MASK = if(row() < col(), 1, 0)"
```

### Handling of floating-point maps

*r.mask* treats floating-point maps the same as integer maps (except
that floating maps are not allowed in combination with the *maskcats*
parameter); all non-NULL values of the input raster map are converted to
1 and are thus part of the mask. In the example below, all raster cells
are part of the mask, i.e., nothing is blocked out from analysis and/or
display.

```sh
r.mapcalc -s "map3 = rand(0.0,1.0)"
r.mask raster=map3
```

However, when using another method than *r.mask* to create a mask, the
user should be aware that the mask is read as an integer map. If mask is
a floating-point map, the values will be converted to integers using the
map's quantisation rules (this defaults to round-to-nearest, but can be
changed with r.quant).

```sh
r.mapcalc -s "map4 = rand(0.0,1.0)"
g.copy raster=map4,MASK
```

In the example above, raster cells with a rounded value of 1 are part of
the mask, while raster cells with a rounded value of 0 are converted to
NULL and consequently blocked out from analysis and/or display.

## EXAMPLES

The examples are based on the North Carolina sample dataset.

Create a raster mask, for constraining the calculation of univariate
statistics of the elevation values for "lakes":

```sh
# set computation region to lakes raster map
g.region raster=lakes -p
# use lakes as mask
r.mask raster=lakes
# get statistics for elevation pixels of lakes:
r.univar elevation
```

Remove the raster mask with the -r flag:

```sh
r.mask -r
```

Creating a mask from selected categories in the North Carolina
'geology_30m' raster map:

```sh
g.region raster=geology_30m -p
r.category geology_30m
d.mon wx0
d.rast geology_30m
r.mask raster=geology_30m maskcats="217 thru 720"
d.mon wx0
d.rast geology_30m
```

## SEE ALSO

*[g.region](g.region.md), [r.mapcalc](r.mapcalc.md),
[r.reclass](r.reclass.md), [g.remove](g.remove.md),
[g.rename](g.rename.md), [r.quant](r.quant.md)*

## AUTHOR

Michael Barton, Arizona State University
