## DESCRIPTION

*r.unpack* allows unpacking raster maps packed by *[r.pack](r.pack.md)*.

## NOTES

Name of the raster map is determined by default from pack file
internals. Optionally the name can be given by **output** parameter.
Currently only 2D raster maps are supported.

## EXAMPLE

Pack up raster map *aspect* into *aspect.pack* file.

```sh
r.pack input=aspect
```

the raster map can be afterwards unpacked by

```sh
r.unpack input=aspect.pack
```

## SEE ALSO

*[r.pack](r.pack.md), [r.in.gdal](r.in.gdal.md), [v.pack](v.pack.md)*

## AUTHORS

Original Bash script written by Hamish Bowman, Otago University, New
Zealand as GRASS AddOns  
Converted to Python and updated for GRASS 7 by Martin Landa, CTU in
Prague, Czech Republic
