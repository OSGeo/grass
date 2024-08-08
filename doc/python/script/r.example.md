## DESCRIPTION

*r.example* selects values from raster above value of mean plus standard
deviation. See the source code for details.

## NOTES

Some more detailed notes go here.

## EXAMPLE

Computing the mean and standard deviation of the raster map
\"elevation\" (North Carolina sample dataset):

```
g.region raster=elevation -p
r.example input=elevation output=elevation_mean_stddev
r.info elevation_mean_stddev
```

## SEE ALSO

*[r.univar](r.univar.html), [r.mapcalc](r.mapcalc.html),
[v.example](v.example.html)* [GRASS Programmer\'s
Manual](https://grass.osgeo.org/programming8/)

## AUTHOR

GRASS Development Team
