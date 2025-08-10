## DESCRIPTION

*d.colortable* is used to display the color table associated with a
raster map in the active frame on the graphics monitor. The **map** name
should be an available raster map in the user's current mapset search
path and location.

If the *values* of both **lines** and **columns** are not specified by
the user, *d.colortable* divides the active frame equally among the
number of categories present in the named raster map. If one option is
specified, the other is automatically set to accommodate all categories.
If both are specified, as many categories as possible are displayed.

If the user specifies the name of a map on the command line but does not
specify the values of other parameters, parameter default values will be
used. Alternately, if the user types simply *d.colortable* on the
command line without any program arguments, the program will prompt the
user for parameter settings using the standard GRASS parser interface.

## EXAMPLE

The user running the command:

```sh
d.colortable map=soils color=red lines=1 columns=3
```

would see the active graphics frame divided into three columns extending
the full frame height. The lines dividing the color table associated
with the *soils* map would be displayed in red. The user would see, at
most, only three of the colors from the *soils* color table displayed in
the active frame (because the user requested that this frame be divided
into three sections).

## NOTES

If the user wishes to display the entire color table associated with a
map, the user should either stipulate a number of lines (rows) and
columns (cols) sufficient to accommodate the number of categories in the
map's color table, or fail to assign values to one or both of **lines**
and/or **columns**. If the user runs *d.colortable* using the default
number of lines and columns (the full graphics frame), all categories
from the map's color table will be displayed. However, if the user
requests that the color table associated with a map which has 10 data
categories be displayed in a graphics frame with only 3 lines (rows) and
2 columns (a total of six cells), only six of the ten map categories
will be displayed.

The user should run *[d.erase](d.erase.md)* between runs of
*d.colortable* to avoid confusion.

## SEE ALSO

*[d.erase](d.erase.md), [d.legend](d.legend.md), [d.rast](d.rast.md)*

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory
