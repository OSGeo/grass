## DESCRIPTION

***r.describe*** prints a terse listing of category values found in a
user-specified raster map layer.

***r.describe*** ignores the current geographic region and mask, and
reads the full extent of the input raster map. This functionality is
useful if the user intends to *reclassify* or *rescale* the data, since
these functions (*[r.reclass](r.reclass.md)* and
*[r.rescale](r.rescale.md)*) also ignore the current *geographic region*
and *mask*.

The ***nv*** parameter sets the string to be used to represent `NULL`
values in the module output; the default is '\*'.

The ***nsteps*** parameter sets the number of quantisation steps to
divide into the input raster map.

## NOTES

### FLAGS

If the user selects the **-r** flag, a range of category values found in
the raster map layer will be printed. The range is divided into three
groups: negative, positive, and zero. If negative values occur, the
minimum and maximum negative values will be printed. If positive values
occur, the minimum and maximum positive values will be printed. If zero
occurs, this will be indicated. The range report will generally run
faster than the full list (the default output).

The **-d** flag can be used to force *r.describe* to respect the current
region extents when reporting raster map categories. The default behavior
is to read the full extent of the input raster map.

If the **-1** flag is specified, the output appears with one category
value/range per line.

The **-n** flag suppresses the reporting of `NULL` values.

## EXAMPLES

The following examples are from the Spearfish60 sample dataset:

### Print the full list of raster map categories

```sh
r.describe landcover.30m
* 11 21-23 31 32 41-43 51 71 81-83 85 91 92
```

### Print the raster range only

```sh
r.describe -r landcover.30m
11 thru 92
*
```

### Print raster map category range, suppressing nulls

```sh
r.describe -n landcover.30m
11 21-23 31 32 41-43 51 71 81-83 85 91 92
```

### Print raster map categories, one category per line

```sh
r.describe -1 geology

*
1
2
3
4
5
6
7
8
9
```

## SEE ALSO

*[g.region](g.region.md), [r.mask](r.mask.md),
[r.reclass](r.reclass.md), [r.report](r.report.md),
[r.rescale](r.rescale.md), [r.stats](r.stats.md),
[r.univar](r.univar.md)*

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
