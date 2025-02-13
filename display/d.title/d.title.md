## DESCRIPTION

*d.title* generates to standard output a string which can be used by
*[d.text](d.text.md)* to draw a TITLE for the raster map layer *name* in
the active display frame on the graphics monitor. Output created by
*d.title* can be redirected into a file, or piped directly into
*[d.text](d.text.md)* to display the map TITLE created by *d.title*. The
map TITLE created will include the map layer's name, TITLE, MAPSET,
LOCATION_NAME, geographic region boundary coordinates, and cell
resolution. If the **-d** draw flag is used, then *d.title* will call
*d.text* for you and the title will be automatically rendered to the
display.

## NOTES

The text created with *[d.text](d.text.md)* will not necessarily fit
within the active display frame on the graphics monitor; the user should
choose a text size appropriate to this frame.

## EXAMPLES

For example, a user wishing to create a suitable TITLE for the
Spearfish, SD *soils* map layer and to display this TITLE in the active
display frame on the graphics monitor might type the following:

```sh
d.title map=soils color=red size=5 > TITLE.file
d.text < TITLE.file
```

Alternately, the user might pipe *d.title* output directly into
*[d.text](d.text.md):*

```sh
d.title map=soils color=red size=5 | d.text
```

A file created by *d.title* can be displayed with *[d.text](d.text.md)*.
Information contained in this file takes precedence over the *color* and
*size* parameters for *[d.text](d.text.md)*.

## SEE ALSO

*[d.font](d.font.md), [d.text](d.text.md)*

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory
