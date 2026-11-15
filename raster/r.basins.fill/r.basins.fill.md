## DESCRIPTION

*r.basins.fill* generates a raster map layer depicting subbasins, based
on input raster map layers for the coded stream network (where each
channel segment has been "coded" with a unique category value) and for
the ridges within a given watershed. The raster map layer depicting
ridges should include the ridge which defines the perimeter of the
watershed. The coded stream network can be generated as part of the
*[r.watershed](r.watershed.md)* program, but the map layer of ridges
will need to be created by hand (for example, through digitizing done in
*[wxGUI vector digitizer](wxGUI.vdigit.md)*).

The resulting output raster map layer will code the subbasins with
category values matching those of the channel segments passing through
them. A user-supplied number of passes through the data is made in an
attempt to fill in these subbasins. If the resulting map layer from this
program appears to have holes within a subbasin, the program should be
rerun with a higher number of passes.

## NOTES

The current geographic region setting is ignored. Instead, the
geographic region for the entire input stream's map layer is used.

## SEE ALSO

See Appendix A of the **GRASS** [Tutorial:
r.watershed](https://grass.osgeo.org/gdp/raster/r.watershed.ps) for
further details on the combined use of *r.basins.fill* and
*r.watershed*.

*[r.water.outlet](r.water.outlet.md), [r.watershed](r.watershed.md),
[wxGUI vector digitizer](wxGUI.vdigit.md)*

## AUTHORS

Dale White, Dept. of Geography, Pennsylvania State University  
Larry Band, Dept. of Geography, University of Toronto, Canada
