## DESCRIPTION

*d.rast.arrow* is designed to help users better visualize surface water
flow direction, as indicated in an aspect raster map layer. There are
two ways to specify the aspect layer the program is to use. The first is
to display the aspect map layer on the graphics monitor before running
*d.rast.arrow*. The second method involves setting the *map* parameter
to the name of the desired aspect map. This allows the arrows to be
drawn over any other maps already displayed on the graphics monitor.

*d.rast.arrow* will draw an arrow over each displayed cell to indicate
in which direction the cell slopes. If the aspect layer has a category
value denoting locations of "unknown" aspect, *d.rast.arrow* draws a
question mark over the displayed cells of that category. Cells
containing null data will be marked with an "X". You can disable drawing
of null data and unknown aspect values by setting its color to "`none`".

When specifying the *magnitude_map* option, arrow lengths denoting
magnitude will be extracted from the cell values of the specified map.
In this case the tail of the arrow will be centered on the source cell.
You may adjust the overall scale using the *scale* option.
*d.rast.arrow* will ignore NULL and negative magnitudes, and will warn
you if the debug level is set at 5 or higher. Be aware. If your
application uses negative values for magnitude, you can use
*[r.mapcalc](r.mapcalc.md)* to prepare the magnitude map to suit your
needs (absolute value, inverted direction and so on).

## NOTES

By default, arrows are drawn at the size of a cell and cannot be seen if
the raster map is relatively close in scale. You can use the *skip*
option to draw arrows every n-th cell in both directions if you are
working with relatively high resolutions. It may be useful to disable
the grid in this case, which is accomplished by setting its color to
"`none`".

For GRASS and Compass type aspect maps, the cell values of the aspect
map will determine the corresponding direction in 360 degrees. ANSWERS
type aspect maps will be plotted in multiples of 15 degrees
counterclockwise from east, and AGNPS and Drainage type aspect maps will
be displayed in D8 representation, i.e. the eight multiples of 45
degrees. Cell values are 1 to 8 clockwise from north for AGNPS and 1 to
8 counterclockwise from north east for Drainage. See
*[r.watershed](r.watershed.md)* for more details about the Drainage
aspect. Terraflow (same as ArcGIS) type aspect map will use a
power-of-two encoding clockwise from 1 for east to 128 for north east.
See *[r.terraflow](r.terraflow.md)* for more details about the Terraflow
encoding.

GRASS aspect maps are measured using Cartesian conventions, i.e. in
degrees counterclockwise from east. e.g.:

```sh
90  North
180 West
270 South
0,360 East
```

They can be created from a raster elevation map with
*[r.slope.aspect](r.slope.aspect.md)*.

Compass type aspect maps are measured in degrees clockwise from north.

This module uses oceanographic conventions, i.e. arrows point downslope
or direction "to", as opposed to atmospheric conventions (direction
"from").

## EXAMPLE

Convert U,V velocity component maps into magnitude,direction maps for
use with *d.rast.arrow*:

```sh
r.mapcalc "magnitude = sqrt(U_map^2 + V_map^2)"
r.mapcalc "direction = atan(U_map, V_map)"
d.rast.arrow map=direction type=grass magnitude_map=magnitude skip=3 grid=none
```

![Sea wind speed (magnitude) and direction shown in the Tasmanian Sea](d_rast_arrow_wind.png)  
*Sea wind speed (magnitude) and direction shown in the Tasmanian Sea*

## SEE ALSO

*[d.frame](d.frame.md), [d.rast](d.rast.md),
[d.rast.edit](d.rast.edit.md), [d.rast.num](d.rast.num.md),
[g.region](g.region.md), [r.slope.aspect](r.slope.aspect.md),
[r.watershed](r.watershed.md), [r.terraflow](r.terraflow.md)*

## AUTHORS

Original author:  
Chris Rewerts, *Agricultural Engineering, Purdue University*  
  
Magnitude and 360 arrow code:  
Hamish Bowman, *Department of Marine Science,
University of Otago, New Zealand*  
  
Align grids with raster cells and Drainage aspect type:  
Huidae Cho  
