## DESCRIPTION

*r.example* selects values from raster above value of mean plus standard
deviation. See the source code for details.

## NOTES

Some more detailed notes go here.

## EXAMPLE

Computing the mean and standard deviation of the raster map "elevation"
(North Carolina sample dataset):

```sh
g.region raster=elevation -p
r.example input=elevation output=elevation_mean_stddev
r.info elevation_mean_stddev
```

## SEE ALSO

*[r.univar](r.univar.md), [r.mapcalc](r.mapcalc.md),
[v.example](v.example.md)* [GRASS Programmer's
Manual](https://grass.osgeo.org/programming8/)

## AUTHOR

GRASS Development Team
