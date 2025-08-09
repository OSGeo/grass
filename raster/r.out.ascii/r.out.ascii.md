## DESCRIPTION

*r.out.ascii* converts a user-specified raster map layer
(**input**=*name*) into an ASCII grid in a text file (**output**=*name*)
suitable for export to other computer systems.

The GRASS program *[r.in.ascii](r.in.ascii.md)* can be used to perform
the reverse function, converting an ASCII file in suitable format to
GRASS raster map format.

To write a SURFER .grd ASCII GRID file (with reverted row order and
different header) use the *-s* flag:

```sh
r.out.ascii -s input=inname output=outname.grd [dp=value]
```

NULL data are coded to "1.70141e+038" for SURFER ASCII GRID files
(ignoring the *null=* parameter).

## NOTES

The output from *r.out.ascii* may be placed into a file by using the
UNIX redirection mechanism; e.g.:

```sh
r.out.ascii input=soils output=- > out.file
```

The output file out.file can then be printed or copied onto a CDROM or
floppy disk for export purposes.

To export the raster values as x,y,z values of cell centers (one per
line) use the *[r.out.xyz](r.out.xyz.md)* module.

## SEE ALSO

*[r.in.ascii](r.in.ascii.md), [r.in.gdal](r.in.gdal.md),
[r.out.bin](r.out.bin.md), [r.out.gdal](r.out.gdal.md),
[r.out.xyz](r.out.xyz.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

Surfer support by Markus Neteler
