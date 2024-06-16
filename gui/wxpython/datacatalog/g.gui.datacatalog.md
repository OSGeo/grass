## DESCRIPTION

The **Data Catalog** is a *[wxGUI](wxGUI.html)* component for browsing,
modifying and managing GRASS maps.

Data Catalog allows you to:

-   browse GRASS projects and mapsets in the current GIS directory
-   browse GRASS 2D/3D raster and vector maps
-   rename, copy, move and delete GRASS maps including reprojection
    between different projects
-   drag and drop maps for copying and moving
-   searching and filtering maps using regular expressions
-   display map in current project
-   show metadata of maps

Note that projects are called *locations* at some places and in old
documentation.

![data catalog screenshot](datacatalog.png){border="0"}\
Figure: Data Catalog integrated in wxGUI.

## NOTES

Some operations (copying, renaming, deleting) are by default enabled
only within the current mapset. To allow changing data outside of your
current mapset, you need to press *Unlock* button in Data Catalog
toolbar.

### WARNING

When renaming, copying or deleting maps outside of Data Catalog, you
need to reload the current mapset or entire database, because it is
currently not synchronised.

## SEE ALSO

*[wxGUI](wxGUI.html)\
[wxGUI components](wxGUI.components.html)*

*[g.copy](g.copy.html), [g.rename](g.rename.html),
[g.remove](g.remove.html), [g.list](g.list.html)*

## AUTHORS

Anna Petrasova, NCSU GeoForAll Laboratory\
Tereza Fiedlerova, OSGeoREL, Czech Technical University in Prague, Czech
Republic
