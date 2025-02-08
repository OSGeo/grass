## DESCRIPTION

*i.target* targets an [imagery group](i.group.md) to a GRASS data base
project name and mapset. A project name and mapset are required for the
*[i.rectify](i.rectify.md)* imagery module, into which to write the
rectified map just prior to completion of the program; *i.target*
enables the user to specify this project. *i.target* must be run before
*[g.gui.gcp](g.gui.gcp.md)* and *[i.rectify](i.rectify.md)*.

## NOTES

The module's first option asks for the name of the [imagery
group](i.group.md) that needs a target. The imagery group must be
present in the user's current mapset. An [imagery group](i.group.md) may
be targeted to any GRASS project.

If a group name is given without setting options, the currently targeted
group will be displayed.

## SEE ALSO

The GRASS 4 *[Image Processing
manual](https://grass.osgeo.org/gdp/imagery/grass4_image_processing.pdf)*

*[g.gui.gcp](g.gui.gcp.md), [i.group](i.group.md),
[i.rectify](i.rectify.md)*  
*[Manage Ground Control Points](wxGUI.gcp.md)*

## AUTHORS

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory

Parser support: Bob Covill
