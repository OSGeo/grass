## DESCRIPTION

*r.recode* creates an output raster map by recoding input raster map
based on recode **rules**. A **title** for the output raster map may be
(optionally) specified by the user.

The recode rules can be read from standard input (i.e., from the
keyboard, redirected from a file, or piped through another program) by
entering **rules=-**.

Rules are defined in one of these formats:

```sh
old_low:old_high:new_low:new_high
old_low:old_high:new_val  (i.e. new_high == new_low)
*:old_val:new_val         (interval [inf, old_val])
old_val:*:new_val         (interval [old_val, inf])
```

*r.recode* is loosely based on *[r.reclass](r.reclass.md)* and uses the
GRASS Reclass Library to convert the rasters. It has routines for
converting to every possible combination of raster (eg. CELL to DCELL,
DCELL to FCELL, etc). Standard floating point raster precision is float
(FCELL), with **-d** double precision (DCELL) will be written.

There are four basic routines that it accepts:

1. old-low to old-high is reclassed to new-low to new high , where the
    user provides all four values. The program figures on the fly what
    type of raster should be created.
2. old-low to old-high is reclassed to a single new value. Anything
    outside the range is null.
3. \* to old-high will reclass everything less than old-high to a
    single new value.
4. old-low to \* will reclass everything greater than old-low to a
    single new value.

These four sets of arguments can be given on the command line, or piped
via stdin or a file. More than one set of arguments is accepted.

## EXAMPLES

### Map type conversion

To simply convert a raster between formats (eg. int to float) the user
would use the first argument. For example

```sh
10:1500:0.1:15.0
```

would convert an input raster map with range between 10 and 1500 to a
float raster raster with range between 0.1 and 15.0.

### Value replacement

*r.recode* can be used to replace existing cell values by others. The
formatting is as described above. In following example the values 1, 2
and 3 are replaced by 1.1, 7.5 resp. 0.4:

```sh
r.recode input=oldmap output=newmap rules=- << EOF
1:1:1.1:1.1
2:2:7.5:7.5
3:3:0.4:0.4
EOF
```

## SEE ALSO

*[r.reclass](r.reclass.md)*

## AUTHOR

CERL
