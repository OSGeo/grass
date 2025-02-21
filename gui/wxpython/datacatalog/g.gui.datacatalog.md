---
description: wxGUI Data Catalog
index: topic_GUI|GUI
---

# wxGUI Data Catalog

## DESCRIPTION

The **Data Catalog** is a *[wxGUI](wxGUI.md)* component for browsing,
modifying and managing GRASS maps.

Data Catalog allows you to:

- browse GRASS projects and mapsets in the current GIS directory
- browse GRASS 2D/3D raster and vector maps
- rename, copy, move and delete GRASS maps including reprojection
  between different projects
- drag and drop maps for copying and moving
- searching and filtering maps using regular expressions
- display map in current project
- show metadata of maps

Note that projects are called *locations* at some places and in old
documentation.

![data catalog screenshot](datacatalog.png)  
*Figure: Data Catalog integrated in wxGUI.*

## NOTES

Some operations (copying, renaming, deleting) are by default enabled
only within the current mapset. To allow changing data outside of your
current mapset, you need to press *Unlock* button in Data Catalog
toolbar.

### WARNING

When renaming, copying or deleting maps outside of Data Catalog, you
need to reload the current mapset or entire database, because it is
currently not synchronized.

## SEE ALSO

*[wxGUI](wxGUI.md), [wxGUI components](wxGUI.components.md)*

*[g.copy](g.copy.md), [g.rename](g.rename.md), [g.remove](g.remove.md),
[g.list](g.list.md)*

## AUTHORS

Anna Petrasova, NCSU GeoForAll Laboratory  
Tereza Fiedlerova, OSGeoREL, Czech Technical University in Prague, Czech
Republic
