## DESCRIPTION

*d.where* is an *interactive* program that allows the user, using the
pointing device (mouse), to identify the geographic coordinates
associated with point locations within the current geographic region in
the active display frame on the graphics monitor.

Each mouse click will output the easting and northing of the point
currently located beneath the mouse pointer. A mouse-button menu is
presented so the user knows which mouse buttons to use. The output is
always printed to the terminal screen; if the output is redirected into
a file, it will be written to the file as well.

Mouse buttons:

```sh
     Left:   where am i
     Middle: draw to/from here
     Right:  quit this
```

The left mouse button prints the coordinates at the selected point, the
middle mouse button allows you to query two points (they are connected
by a line for convenience). Use the right mouse button to exit the
module.

## NOTES

This program uses the current geographic region setting and active
frame. It is not necessary, although useful, to have displayed a map in
the current frame before running *d.where*. The **-d** flag allows the
user to optionally output latitude/longitude coordinates pair(s) in
decimal degree rather than DD:MM:SS format. The **-w** flag is only
valid if a datum is defined for the current project's coordinate
reference system. If the **-f** flag is given the x,y frame coordinates
of the active display monitor will be returned (as a percentage, 0,0 is
bottom left).

## EXAMPLE

Query position in map (North Carolina sample dataset):

```sh
d.rast elevation
d.where
```

## SEE ALSO

*[d.what.rast](d.what.rast.md), [d.what.vect](d.what.vect.md),
[g.region](g.region.md), [v.what.rast](v.what.rast.md),
[v.what.vect](v.what.vect.md)*

## AUTHORS

James Westervelt,  
Michael Shapiro,  
U.S. Army Construction Engineering Research Laboratory
