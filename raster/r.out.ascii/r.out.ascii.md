## DESCRIPTION

*r.out.ascii* converts a user-specified raster map layer
(**input=***name*) into an ASCII grid in a text file (**output=***name*)
suitable for export to other computer systems.

The GRASS program *[r.in.ascii](r.in.ascii.html)* can be used to perform
the reverse function, converting an ASCII file in suitable format to
GRASS raster map format.

To write a SURFER .grd ASCII GRID file (with reverted row order and
different header) use the *-s* flag:

::: code
    r.out.ascii -s input=inname output=outname.grd [dp=value]
:::

NULL data are coded to \"1.70141e+038\" for SURFER ASCII GRID files
(ignoring the *null=* parameter).

To write a LISFLOOD .dem ASCII GRID file (with different header) use the
*-l* flag:

::: code
    r.out.ascii -l input=inname output=outname.dem
:::

NULL data output are set by the user at \"-9999\" in this case, see
below:

::: code
    ncols       1514
    nrows       2747
    xllcorner   212236
    yllcorner   2910116
    cellsize    120
    NODATA_value    -9999
:::

## NOTES

The output from *r.out.ascii* may be placed into a file by using the
UNIX redirection mechanism; e.g.:

::: code
    r.out.ascii input=soils output=- > out.file
:::

The output file out.file can then be printed or copied onto a CDROM or
floppy disk for export purposes.

To export the raster values as x,y,z values of cell centers (one per
line) use the *[r.out.xyz](r.out.xyz.html)* module.

## SEE ALSO

*[r.in.ascii](r.in.ascii.html), [r.in.gdal](r.in.gdal.html),
[r.out.bin](r.out.bin.html), [r.out.gdal](r.out.gdal.html),
[r.out.xyz](r.out.xyz.html)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

Surfer support by Markus Neteler
