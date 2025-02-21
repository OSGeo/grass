---
description: wxGUI GCP Manager for photo to image registration
index: topic_GUI|GUI
---

# wxGUI GCP Manager for photo to image registration

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
an extension file (if not given, defaults to \\filename_ip2i_out)

An example for project **imagery60**:

```sh
g.gui.photo2image group=aerial@PERMANENT raster=gs13.1@PERMANENT camera=gscamera order=2 extension=try --o
```

![Screenshot of g.gui.photo2image](wxGUI_iphoto2image_frame.jpg)  
*Figure: Screenshot of g.gui.photo2image*

## SEE ALSO

*[wxGUI](wxGUI.md), [wxGUI components](wxGUI.components.md)*

*[i.ortho.photo](i.ortho.photo.md), [i.group](i.group.md),
[i.ortho.camera](i.ortho.camera.md),
[i.ortho.target](i.ortho.target.md), [i.rectify](i.rectify.md),
[m.transform](m.transform.md), [v.rectify](v.rectify.md)*

See also [video
tutorials](https://grasswiki.osgeo.org/wiki/WxGUI/Video_tutorials#Georectifier)
on GRASS Wiki.

## AUTHORS

Markus Metz  
  
*Based on the Georectifier (GRASS 6.4.0)* by Michael Barton  
Martin Landa, Czech Technical University in Prague, Czech Republic
