---
description: wxGUI Cartographic Composer
index: topic_GUI|GUI
---

# wxGUI Cartographic Composer

## DESCRIPTION

**wxGUI Cartographic Composer** also called *wx.psmap* is a
*[wxGUI](wxGUI.md)* extension which allows the user to create
interactively hardcopy map outputs. This tool generates
*[ps.map](ps.map.md)* configuration file and then runs
*[ps.map](ps.map.md)* to create PostScript output. There are two modes -
*Draft mode* for map composing and *Preview mode* (requires [Python
Imaging Library](http://www.pythonware.com/products/pil/)) to see how
the result will look like. In draft mode map features (like legend or
scalebar) are represented by a colored rectangle with a label.

Possible output files:

- *[ps.map](ps.map.md)* instructions file
- PostScript/EPS file
- PDF (using ps2pdf)

![Cartographic Composer](psmap_frame.jpg)

Cartographic Composer enables to load in saved instructions file.
Loading instruction files created by Cartographic Composer is more
robust, as opposed to loading files created manually.

Currently supported *[ps.map](ps.map.md)* instructions:

- paper
- maploc
- scale
- border
- raster
- colortable
- vpoints
- vlines
- vareas
- vlegend
- text
- scalebar
- mapinfo
- point
- line
- rectangle
- labels

### CARTOGRAPHIC COMPOSER TOOLBAR

![icon](icons/script-save.png)  *Generate instructions file*:
Generates and saves text file with mapping instructions.

![icon](icons/script-load.png)  *Load instructions file*:
Load text file with mapping instructions.

![icon](icons/page-settings.png)  *Page setup*:
Specify paper size, margins and orientation.

![icon](icons/pointer.png)  *Pointer*:
Select object on the paper by clicking, drag the cursor while pressing
the left mouse button to move it or resize object (currently only map
frame) by clicking on a small black box in its bottom right corner.
Double click to show object properties dialog

![icon](icons/pan.png)  *Pan*:
Drag the pan cursor while pressing the left mouse button to move your
view.

![icon](icons/zoom-in.png)  *Zoom in*:
Interactive zooming with the mouse in both draft and preview mode.
Drawing a box or just a left click with the mouse and zoom-in cursor
causes the display to zoom in so that the area defined by the box fills
the display.

![icon](icons/zoom-out.png)  *Zoom out*:
Interactive zooming with the mouse in both draft and preview mode.
Drawing a box or just a left click with the mouse and zoom-out cursor
causes the display to zoom out so that the area displayed shrinks to
fill the area defined by the box.

![icon](icons/zoom-extent.png)  *Zoom to page*:
Zoom to display the entire page

![icon](icons/layer-add.png)  *Map frame*:
Click and drag to place map frame. If map frame is already drawn, open a
dialog to set its properties.

![icon](icons/layer-raster-add.png)  *Raster map*:
Shows a dialog to add or change the raster map.

![icon](icons/layer-vector-add.png)  *Vector map*:
Shows a dialog to add or change current vector maps and their
properties:

- *Data selection*:
Select data to draw:

  - *Feature type*:
    Select which data type to draw. In case of point data, points or
    centroids can be drawn, in case of line data, lines or boundaries.

  - *Layer selection*:
    Select layer and limit data by a SQL query or chose only certain
    categories.

  - *Mask*:
    Whether to use mask or not.

- *Colors*:
Color settings:

  - *Outline*:
    Select outline color and width in points. In case of lines, outline
    means highlighting.

  - *Fill*:
    Select fill color, one color for all vector elements or color from rgb
    column.

- *Size and style*:
Sets size, style, symbols, pattern; depends on data type:

  - *Symbology*:
    Available for point data. Choose symbol or EPS file to draw points with.

  - *Line style*:
    Available for line data. Select line style (solid, dashed, ...) and the
    look of the ends of the line (butt, round, ...)

  - *Pattern*:
    Available for areas. Choose pattern file and set the width of the
    pattern.

  - *Size*:
    Available for point data. Choose size (number of times larger than the
    size in the icon file) as a single value or take the size from a map
    table column.

  - *Rotation*:
    Available for point data. Rotate symbols counterclockwise with the given
    value or with the value from a map table column

  - *Width*:
    Available for line data. Set line width in points or take the value from
    a map table column.

![icon](icons/layer-more.png)  *Add overlays*:
Add overlays: vector labels, grid (not yet implemented)

- ![icon](icons/layer-label-add.png)  *Add labels*:
Add vector labels created beforehand by v.label module.

![icon](icons/overlay-add.png)  *Add map elements*:
Add map elements: legend, scalebar, map info, text

- ![icon](icons/legend-add.png)  *Add legend*:
Add raster or vector legend or edit their properties.

- ![icon](icons/map-info.png)  *Add map info*:
Add information about region, grid and scale or edit map info
properties.

- ![icon](icons/scalebar-add.png)  *Add scalebar*:
Add scalebar or edit its properties.

- ![icon](icons/text-add.png)  *Add text*:
Add text label.

![icon](icons/layer-remove.png)  *Remove selected element*:
Select an object and remove it. Pressing Delete key does the same.

![icon](icons/execute.png)  *Show preview*:
Generates output and switches to Preview mode to see the result. Be
patient, it can take a while.

![icon](icons/ps-export.png)  *Generate hardcopy map output in PS*:
Generates hardcopy map output in PostScript/EPS file.

![icon](icons/pdf-export.png)  *Generate hardcopy map output in PDF*:
Generates hardcopy map output in PDF using ps2pdf or [Ghostscript
gswin32c/gswin64c](https://www.ghostscript.com/releases/gsdnld.html) (OS
MS Windows platform only).

## SEE ALSO

*[wxGUI](wxGUI.md), [wxGUI components](wxGUI.components.md)*

See also
[wiki](https://grasswiki.osgeo.org/wiki/WxGUI_Cartographic_Composer)
page.

## AUTHOR

Anna Kratochvilova, Czech Technical University in Prague, Czech Republic
(bachelor's final project 2011, mentor: Martin Landa)
