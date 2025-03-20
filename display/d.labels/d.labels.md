## DESCRIPTION

*d.labels* displays a *paint* label file in the active display frame on
the graphics monitor. Each label has components which determine the
text, the location of the text on the image, its size, and the
background for the text. This file can be generated with the
*[v.label](v.label.md)* program or simply created by the user as an
ASCII file (using a text editor) and placed in the appropriate directory
under the user's current mapset and project (i.e.
`$MAPSET/paint/labels/`).

## NOTES

Some of the information stored in the label file is unused by
*d.labels*. This extra information is used by such programs as
*[ps.map](ps.map.md)*.

This module was formerly known as *d.paint.labels*. The old version
of *d.labels* from GRASS 5, which provided interactive placement and
modification of paint labels, still needs to have its functionality
merged into this module.

## SEE ALSO

*[d.font](d.font.md)*  
*[d.text](d.text.md)*  
*[d.title](d.title.md)*  
*[ps.map](ps.map.md)*  
*[v.label](v.label.md)*  

## AUTHOR

James Westervelt, U.S. Army Construction Engineering Research Laboratory
