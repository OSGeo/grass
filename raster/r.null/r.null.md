## DESCRIPTION

The function of *r.null* is to explicitly create the NULL-value bitmap
file. The intended usage is to update maps that do not have a NULL-value
bitmap file (i.e. to indicate for each pixel if zero is a valid value or
is to be considered as NULL, i.e. no data value). The module does not
work with reclassified or external maps.

The design is flexible. Ranges of values can be set to NULL and/or the
NULL value can be eliminated and replace with a specified value.

The **setnull** parameter is used to specify values in the ranges to be
set to NULL. A range is either a single value (e.g., 5.3), or a pair of
values (e.g., 4.76-34.56). Existing NULL-values are left NULL, unless
the null argument is requested.

The **null** parameter eliminates the NULL value and replaces it with
value. This argument is applied only to existing NULL values, and not to
the NULLs created by the setnull argument.

## NOTES

Note that the value is restricted to integer if the map is an integer
map.

### r.null and reclassified maps

*r.null* does not support reclassified maps because, if *r.null* was run
on the reclass raster it would alter the original and any other reclass
rasters of the original. Therefore *r.null* does not allow recoding
reclassified maps (products of *r.reclass*).  
As a workaround, the way to recode such a map is: The user creates a
raster map out of the reclassified map by copying it:  

```sh
r.mapcalc "newmap = reclass"
```

### NULL data compression

By default no data files (i.e., NULL files) are not compressed unless a
specific environment variable is set. The NULL file compression must be
explicitly turned on with `export GRASS_COMPRESS_NULLS=1`.  
Warning: such raster maps can then only be opened with GRASS GIS 7.2.0
or later. NULL file compression can be managed with **r.null -z**.

### External maps

From the [r.external](r.external.md) documentation: GDAL-linked
(*r.external*) maps do not have or use a NULL bitmap, hence *r.null*
cannot manipulate them directly. Here NULL cells are those whose value
matches the value reported by the GDALGetRasterNoDataValue() function.
To introduce additional NULL values to a computation based on a
GDAL-linked raster, the user needs to either create a mask with with
*r.mask* and then "apply" it using e.g. *r.resample* or *r.mapcalc*, or
use *r.mapcalc* to create a copy with the appropriate categories changed
to NULL (`if()` condition).

## EXAMPLES

Set specific values of a classified map to NULL:  

```sh
r.null map=landcover.30m setnull=21,22
```

Set NULL-values of a map to a specific value:  

```sh
r.null map=fields null=99
```

## SEE ALSO

*[r.compress](r.compress.md), [r.support](r.support.md),
[r.quant](r.quant.md)*

## AUTHOR

U.S.Army Construction Engineering Research Laboratory
