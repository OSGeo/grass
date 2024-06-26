## DESCRIPTION

This module is based on **g.gui.gcp**, the GCP manager of GRASS GIS. It
is part of i.ortho.photo suite.

The aim of this module is to give absolute location values to the
fiducial points present (in number of 4 or 8) in a *scanned* aerial
photo.

This is necessary as (manual) scanning introduces distortions, rotations
and also may not be limited to scan the boundary of the photo itself. It
is thus necessary to give to each fiducial the exact coordinates in mm
as given by the aerial photographic instrument design, which is unique
per camera.

This module requires you to have made a group with your aerial photo
**(i.group)**, a camera description file **(i.ortho.target)** and use
them to launch the module. Additional requirements are the order of
rectification (1 if no of Fiducials is 4, 2 if no of Fiducials is 8) and
an extension file (if not given, defaults to \\\$filename_ip2i_out)

An example for project **imagery60**:

::: code
    g.gui.photo2image group=aerial@PERMANENT raster=gs13.1@PERMANENT camera=gscamera order=2 extension=try --o
:::

### Screenshot of g.gui.photo2image

::: {align="center" style="margin: 10px"}
[![Screenshot of
g.gui.photo2image](wxGUI_iphoto2image_frame.jpg){width="600"
height="375" border="0"}](wxGUI_iphoto2image_frame.jpg)\
*Figure: Screenshot of g.gui.photo2image*
:::

## For a detailed operation manual please read

*[wxGUI](wxGUI.html)\
[wxGUI components](wxGUI.components.html)*

See also [video
tutorials](https://grasswiki.osgeo.org/wiki/WxGUI/Video_tutorials#Georectifier)
on GRASS Wiki.

## SEE ALSO

*[i.ortho.photo](i.ortho.photo.html), [i.group](i.group.html),
[i.ortho.camera](i.ortho.camera.html),
[i.ortho.target](i.ortho.target.html), [i.rectify](i.rectify.html),
[m.transform](m.transform.html), [v.rectify](v.rectify.html)*

## AUTHORS

Markus Metz\
\
*Based on the Georectifier (GRASS 6.4.0)* by Michael Barton\
Martin Landa, Czech Technical University in Prague, Czech Republic
