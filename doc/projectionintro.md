### Projection management in general

A GRASS project is referenced with a single projection and coordinate
system (or unreferenced as XY project). When creating a new project from
an existing raster or vector map using the tools available from the
startup screen or the map import commands, projection and coordinate
system are defined. To change the projection of maps, a new project has
to be created and the desired maps have to be reprojected into it from
the source project as explained below.

### Reprojecting raster maps

Rasters are reprojected using the raster projection tool
*[r.proj](r.proj.html)*. The tool is used in the target project to
\"pull\" a map from its source project. Both projects need to have a
projection defined, i.e., they cannot be XY (unprojected).

### Raster map transformation

To transform an unprojected map from a XY project into a projected
project (or another XY project), a forward transformation is performed.
The unreferenced map is geocoded within the XY project by defining four
corner points or by seeking several ground control points
([i.group](i.group.html), [i.target](i.target.html),
[g.gui.gcp](g.gui.gcp.html)) and then transformed into the target
project ([i.rectify](i.rectify.html)). Polynomial transformation of 1st,
2nd and 3rd order are supported.

A graphical user interface is provided by [wxGUI](wxGUI.html).

To simply translate a raster map (without stretching or rotation), the
[r.region](r.region.html) command can be used.

### Vector map projections

Vectors are reprojected using the vector projection tool
*[v.proj](v.proj.html)*. The tool is used in the target project to
\"pull\" a map from its source project. Both projects need to have a
projection defined, i.e., they cannot be XY (unprojected).

### Vector map transformation

To transform an unprojected map (e.g. CAD map) into projected
coordinates, a forward transformation is performed. The unreferenced map
is imported into the project with projection and geocoded within this
project by defining four corner points or by seeking several ground
control points. These points are stored into an ASCII file and then
transformed within the same project ([v.transform](v.transform.html)).
Alternatively, [v.rectify](v.rectify.html) rectifies a vector by
computing a coordinate transformation for each object in the vector
based on the control points.

A graphical user interface is provided by [wxGUI](wxGUI.html).

### References

-   [ASPRS Grids and
    Datum](https://www.asprs.org/asprs-publications/grids-and-datums)
-   [Projections Transform List](http://geotiff.maptools.org/proj_list/)
    (PROJ)
-   [Coordinate operations](https://proj.org/operations/index.html) by
    PROJ (projections, conversions, transformations, pipeline operator)
-   [MapRef - The Collection of Map Projections and Reference Systems
    for Europe](http://www.mapref.org)
-   [Information and Service System for European Coordinate Reference
    Systems - CRS](http://crs.bkg.bund.de/crs-eu/)

### See also

-   [Introduction into raster data processing](rasterintro.html)
-   [Introduction into 3D raster data (voxel)
    processing](raster3dintro.html)
-   [Introduction into vector data processing](vectorintro.html)
-   [Introduction into image processing](imageryintro.html)
-   [Introduction into temporal data processing](temporalintro.html)
-   [Database management](databaseintro.html)
