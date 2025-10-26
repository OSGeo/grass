## DESCRIPTION

*d.to.rast* saves the content of the currently selected monitor into a
raster map. The active monitor can be selected with *d.mon*. *d.to.rast*
can be run from GUI Console tab, too. This module is not sensitive to
computational region settings.

## EXAMPLE

We combine different raster and vector map layers to create a composite
layer which can be draped over elevation in 3D view. First, we add a
couple of maps to layer manager:

```sh
g.region raster=elevation
d.rast map=elevation
d.rast map=lakes
d.vect map=roadsmajor width=4
d.vect map=roadsmajor width=2 color=yellow

# create a raster map from the display
d.to.rast output=composite
```

Then uncheck all layers except for elevation and switch to 3D view. In
Data tab, set color map to the newly created composite map.

![Raster map created by d.to.rast draped over digital elevation model](d_to_rast_3D_example.jpg)
*Figure: Raster map created by *d.to.rast* draped over digital elevation model.*

## SEE ALSO

*[d.out.file](d.out.file.md), [d.erase](d.erase.md),
[d.rast](d.rast.md), [d.vect](d.vect.md), [d.mon](d.mon.md)*

## AUTHOR

Anna Petrasova, [NCSU GeoForAll
Lab](https://geospatial.ncsu.edu/geoforall/)
