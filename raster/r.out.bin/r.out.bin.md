## DESCRIPTION

The *r.out.bin* program exports a GRASS raster map to a binary array
file. Optionally, output can be sent to standard output (stdout) for
direct input (pipe) into other applications. Data is exported according
to the original GRASS raster type (e.g. float). If the "-i" flag is
specified, an integer array is output. The region parameters are printed
to stderr.

## NOTES

With the -h flag, data can be directly used by
[GMT](https://www.generic-mapping-tools.org/) as Grid Format 1 (float)
or 2 (short). For example:

```sh
r.out.bin -h input=grass.raster output=new.grd
grdinfo new.grd=1 (if float)
```

Exported data can be piped directly into the GMT program xyz2grd.

```sh
r.out.bin input=grass.raster output=- | xyz2grd -R....  -ZTLf -
```

The example uses the GMT program xyz2grd with the -ZTLf flag indicating
that a float array was output.

## SEE ALSO

*[r.in.bin](r.in.bin.md), [r.in.ascii](r.in.ascii.md),
[r.in.gdal](r.in.gdal.md), [r.out.ascii](r.out.ascii.md)*

## AUTHOR

This program is derived from *[r.out.ascii](r.out.ascii.md)* with a few
modifications.  
Author: [Bob Covill](mailto:bcovill@tekmap.ns.ca)
