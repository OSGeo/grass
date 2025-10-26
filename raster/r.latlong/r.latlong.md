## DESCRIPTION

*r.latlong* creates a latitude (degree decimal) map, or longitude if the
-l flag is used, from any map in any projection using PROJ library. This
is an input to *r.sun* and *i.evapo.potrad*.

## NOTES

The PROJ [website](https://proj.org).

## TODO

Datum transform is not implemented, the same datum is taken as output.

## EXAMPLE

```sh
g.region raster=elevation -p
r.latlong input=elevation output=latitude
```

## SEE ALSO

*[r.sun](r.sun.md), [r.sunhours](r.sunhours.md)*

## AUTHOR

Yann Chemin, International Rice Research Institute, The Philippines
