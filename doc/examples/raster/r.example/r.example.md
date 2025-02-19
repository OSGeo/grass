## DESCRIPTION

*r.example* does practically do nothing, except for illustrating GRASS
GIS raster programming. It copies over an existing raster map to a new
raster map. See the source code for details.

## NOTES

Some more detailed notes go here.

## EXAMPLE

Create a copy of the raster map "elevation" (North Carolina sample
dataset):

```sh
g.region raster=elevation -p
r.example input=elevation output=elevation2
r.info elevation2
```

## SEE ALSO

*[r.stats](r.stats.md), [v.example](v.example.md)* *[GRASS Programmer's
Manual](https://grass.osgeo.org/programming8/)*

## AUTHOR

GRASS Development Team
