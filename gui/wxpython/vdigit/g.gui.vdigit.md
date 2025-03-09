---
description: wxGUI Vector Digitizer
index: topic_GUI|GUI
---

# wxGUI Vector Digitizer

## DESCRIPTION

The **vector digitizer** is a *[wxGUI](wxGUI.md)* component intended for
interactive editing and digitizing vector maps (see
*[v.edit](v.edit.md)* for non-interactive vector editing GRASS
capabilities).

Note that only vector maps stored or generated in the *current* mapset
can be opened for editing.

The digitizer allows editing of 2D vector features (points, lines,
centroids, boundaries, and areas).

Vector features can be selected by mouse (bounding box or simply by
mouse click, see select threshold in *Settings→General→Select
threshold*), or by query (eg. by line length, see *Settings→Query
Tool*).

### STARTING THE DIGITIZER

The vector digitizer can be launched from Map Display toolbar by
selecting "Digitize" from *Tools* combobox. Vector map is selectable for
editing from Digitizer toolbar ("Select vector map" combobox, note that
only vector maps from the current layer tree in Layer Manager are
listed). The vector digitizer can alternatively be activated from the
contextual menu in the Layer Manager by selecting "Start editing" on
selected vector map in the layer tree, or directly from the Layer
Manager toolbar ![icon](icons/edit.png). The vector digitizer also can
also be launched from the command line as a stand-alone application
*g.gui.vdigit*.

### CREATING A NEW VECTOR MAP

A new vector map can be easily created from the digitizer toolbar by
selecting "New vector map" in "Select vector map" combobox. A new vector
map is created, added to the current layer tree in Layer Manager and
opened for editing. "Select vector map" combobox in the digitizer
toolbar also allows easily switching between vector maps to be edited.

### EDITING AN EXISTING VECTOR MAP

An existing vector map selected in the digitizer toolbar in the "Select
vector map" combobox. This map is then opened for editing and added to
the current layer tree in the *Layer Manager*. This "Select vector map"
combobox in the digitizer toolbar also allows to easily switch between
vector maps to be edited.

### USING A RASTER BACKGROUND MAP

In order to digitize from a raster map, simply load the map into the
*Map Display* using the *Layer Manager*. Then start digitizing, see
below for details.

### USING A VECTOR BACKGROUND MAP

The vector digitizer allows you to select a "background" vector map.
Loading it within the digitizer is different from simply loading it via
*Layer Manager* since direct interaction with single vector features
then becomes possible.  
Vector features may be copied from the background map by "Copy features
from (background) vector map" tool from the "Additional tools" of the
digitizer toolbar. Newly digitized vector features are snapped in the
given threshold to the features from the background map.

### DIGITIZER TOOLBAR

![Vector Digitizer Toolbar](vdigit_toolbar.jpg)

![icon](icons/point-create.png)  *Digitize new point*:
Add new point to vector map and optionally define its attributes
(Ctrl+P).

![icon](icons/line-create.png)  *Digitize new line*:
Add new line to vector map and optionally define its attributes
(Ctrl+L).

![icon](icons/boundary-create.png)  *Digitize new boundary*:
Add new boundary to vector map and optionally define its attributes
(Ctrl+B).

![icon](icons/centroid-create.png)  *Digitize new centroid*:
Add new centroid to vector map and optionally define its attributes
(Ctrl+C).

![icon](icons/polygon-create.png)  *Digitize new area*:
Add new area (closed boundary and one centroid inside) to vector map and
optionally define its attributes (Ctrl+A).

![icon](icons/vertex-move.png)  *Move vertex*:
Move selected vertex of linear feature. Thus shape of linear feature is
changed (Ctrl+G).

![icon](icons/vertex-create.png)  *Add vertex*:
Add new vertex to selected linear feature (shape not changed) (Ctrl+V).

![icon](icons/vertex-delete.png)  *Remove vertex*:
Remove selected vertex from linear feature (Ctrl+X). Thus shape of
selected feature can be changed.

![icon](icons/line-edit.png)  *Edit line/boundary*:
Edit selected linear feature, add new segments or remove existing
segments of linear feature (Ctrl+E).

![icon](icons/line-move.png)  *Move feature(s)*:
Move selected vector features (Ctrl+M.) Selection can be done by mouse
or by query.

![icon](icons/line-delete.png)  *Delete feature(s)*:
Delete selected vector features (point, line, centroid, or boundary)
(Ctrl+D). Selection can be done by mouse or by query.

![icon](icons/polygon-delete.png)  *Delete areas(s)*:
Delete selected vector areas (Ctrl+F). Selection can be done by mouse or
by query.

![icon](icons/cats-display.png)  *Display/update categories*:
Display categories of selected vector feature (Ctrl+J). Category
settings can be modified, new layer/category pairs added or already
defined pairs removed.

![icon](icons/attributes-display.png)  *Display/update attributes*:
Display attributes of selected vector feature (based on its category
settings) (Ctrl+K). Attributes can be also modified. Same functionality
is accessible from Main toolbar "Query vector map (editable mode)".

![icon](icons/tools.png)  *Additional tools*:

- *Break selected lines/boundaries at intersection*:
  Split given vector line or boundary into two lines on given position
  (based on *[v.clean](v.clean.md)*, `tool=break`).
- *Connect two selected lines/boundaries*:
  Connect selected lines or boundaries, the first given line is
  connected to the second one. The second line is broken if necessary on
  each intersection. The lines are connected only if distance between
  them is not greater than snapping threshold value.
- *Copy categories*:
  Copy category settings of selected vector feature to other vector
  features. Layer/category pairs of source vector features are appended
  to the target feature category settings. Existing layer/category pairs
  are not removed from category settings of the target features.
- *Copy features from (background) vector map*:
  Make identical copy of selected vector features. If a background
  vector map has been selected from the Layer Manager, copy features
  from background vector map, not from the currently modified vector
  map.
- *Copy attributes*:
  Duplicate attributes settings of selected vector feature to other
  vector features. New category(ies) is appended to the target feature
  category settings and attributes duplicated based on category settings
  of source vector features. Existing layer/category pairs are not
  removed from category settings of the target features.
- *Feature type conversion*:
  Change feature type of selected geometry features. Points are
  converted to centroids, centroids to points, lines to boundaries and
  boundaries to lines.
- *Flip selected lines/boundaries*:
  Flip direction of selected linear features (lines or boundaries).
- *Merge selected lines/boundaries*:
  Merge (at least two) selected vector lines or boundaries. The geometry
  of the merged vector lines can be changed. If the second line from two
  selected lines is in opposite direction to the first, it will be
  flipped. See also module *[v.build.polylines](v.build.polylines.md)*.
- *Snap selected lines/boundaries (only to nodes)*:
  Snap vector features in given threshold. See also module
  *[v.clean](v.clean.md)*. Note that this tool supports only snapping to
  nodes. Snapping to vector features from background vector map is not
  currently supported.
- *Split line/boundary*:
  Split selected line or boundary on given position.
- *Query tool*:
  Select vector features by defining a threshold for min/max length
  value (linear features or dangles).
- *Z-bulk labeling of 3D lines*:
  Assign z coordinate values to 3D vector lines in bounding box. This is
  useful for labeling contour lines.

![icon](icons/undo.png)  *Undo*:
Undo previous operations (Ctrl+Z).

![icon](icons/redo.png)  *Redo*:
Redo previous operations (Ctrl+Y).

![icon](icons/settings.png)  *Settings*:
Digitizer settings (Ctrl+T).

![icon](icons/help.png)  *Show help*:
Show help page for the Digitizer (Ctrl+H).

![icon](icons/quit.png)  *Quit digitizing tool*:
Changes in vector map can be optionally discarded when digitizing
session is quited (Ctrl+Q).

## NOTES

**Mouse button functions:**

*Left* - select or deselect features

*Control+Left* - cancel action or undo vertex when digitizing lines

*Right* - confirm action

*Dead (deleted)* features are only marked as 'dead' in the geometry file
but remain there and occupy space. Any vector module used afterwards on
this vector map which really reads and writes vector geometry (so not
*[g.copy](g.copy.md)*) will write only features which are 'alive'.

*Added or modified* vector features are *snapped* to existing vector
features (Settings→General→Snapping). To disable snapping set the
snapping threshold to '0'.

If the digitizer crashes for some reason, the changes are automatically
saved. Broken topology can be repaired by running
*[v.build](v.build.md)*.

GRASS GIS uses a topological vector format, meaning that a common
boundary of two polygons is only stored once. When digitizing polygons
it is thus important to be able to only draw each boundary once. When
drawing a polygon adjacent to an existing polygon, one has to first
split the existing boundary at the points where the new boundary will be
attached. Snapping should be set to ensure that the new boundaries are
automatically attached to the chosen points.

## REFERENCES

- [GRASS Vedit
  Library](https://grass.osgeo.org/programming8/veditlib.html)
- [Vector Database
  Management](https://grasswiki.osgeo.org/wiki/Vector_Database_Management)
  (Wiki page)

## SEE ALSO

*[wxGUI](wxGUI.md), [wxGUI components](wxGUI.components.md)*

*[v.edit](v.edit.md), [v.category](v.category.md),
[v.build](v.build.md), [g.gui.rdigit](g.gui.rdigit.md),
[wxGUI.rdigit](wxGUI.rdigit.md)*

See also the [WxGUI Vector Digitizer Wiki
page](https://grasswiki.osgeo.org/wiki/WxGUI_Vector_Digitizer) including
[video
tutorials](https://grasswiki.osgeo.org/wiki/WxGUI_Vector_Digitizer#Vector_tutorials).

## AUTHOR

Martin Landa, FBK-irst (2007-2008), Trento, Italy, and Czech Technical
University in Prague, Czech Republic
