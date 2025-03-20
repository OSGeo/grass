## DESCRIPTION

*r.surf.contour* creates a raster elevation map from a rasterized
contour map. Elevation values are determined using procedures similar to
a manual methods. To determine the elevation of a point on a contour
map, an individual might interpolate its value from those of the two
nearest contour lines (uphill and downhill).

*r.surf.contour* works in a similar way. Initially, a vector map of the
contour lines is made with the elevation of each line as an attribute.
When the program *[v.to.rast](v.to.rast.md)* is run on the vector map,
continuous "lines" of rasters containing the contour line values will be
the input for *r.surf.contour*. For each cell in the input map, either
the cell is a contour line cell (which is given that value), or a flood
fill is generated from that spot until the fill comes to two unique
values. So the *r.surf.contour* algorithm **linearly interpolates**
between contour lines. The flood fill is not allowed to cross over the
rasterized contour lines, thus ensuring that an uphill and downhill
contour value will be the two values chosen. *r.surf.contour*
interpolates from the uphill and downhill values by the true distance.

### Parameters

**input**=*name*  
Name of an existing raster map that contains a set of initial category
values (i.e., some cells contain known elevation values (denoting
contours) while the rest contain NULL values).

**output**=*name*  
Name to be assigned to new output raster map that represents a smooth
(e.g., elevation) surface generated from the known category values in
the input raster map layer.

An existing mask raster map is respected for both reading *input* and
writing *output*.

## NOTES

*r.surf.contour* works well under the following circumstances: 1) the
contour lines extend to the edge of the current region, 2) the
program is run at the same resolution as that of the input map, 3) there
are no disjointed contour lines, and 4) no spot elevation data BETWEEN
contour lines exist. Spot elevations at the tops of hills and the
bottoms of depressions, on the other hand, improve the output greatly.
Violating these constraints will cause non-intuitive anomalies to appear
in the output map. Run *[r.slope.aspect](r.slope.aspect.md)* on
*r.surf.contour* results to locate potential anomalies.

The running of *r.surf.contour* is very sensitive to the resolution of
rasterized vector map. If multiple contour lines go through the same
raster, slight anomalies may occur. The speed of *r.surf.contour* is
dependent on how far "apart" the contour lines are from each other (as
measured in raster cells). Since a flood fill algorithm is used, the
program's running time will grow exponentially with the distance between
contour lines.

## EXAMPLE

Example to create contour lines from elevation model, then recreating
DEM from these contour lines along with differences analysis (North
Carolina sample data set):

```sh
g.region raster=elevation -p

# get minimum elevation value
r.univar elevation

# generate vector contour lines
r.contour input=elevation output=contours_5m step=5 minlevel=50

# rasterize contour lines
v.info -c contours_5m
v.to.rast input=contours_5m output=contours_5m use=attr attribute_column=level

# generate DEM from rasterized contour lines
r.surf.contour input=contours_5m output=elevation_from_cont5m

# calculate difference map
r.mapcalc "diff = elevation - elevation_from_cont5m"
r.colors diff color=differences

# analyze differences statistically
r.univar diff
```

## SEE ALSO

*[r.mapcalc](r.mapcalc.md), [r.slope.aspect](r.slope.aspect.md),
[r.surf.idw](r.surf.idw.md), [wxGUI vector digitizer](wxGUI.vdigit.md),
[v.surf.idw](v.surf.idw.md), [v.surf.rst](v.surf.rst.md),
[v.to.rast](v.to.rast.md)*

Overview: [Interpolation and
Resampling](https://grasswiki.osgeo.org/wiki/Interpolation) in GRASS GIS

## AUTHOR

Chuck Ehlschlaeger, U.S. Army Construction Engineering Research
Laboratory
