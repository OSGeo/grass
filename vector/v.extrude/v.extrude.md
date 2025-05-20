## DESCRIPTION

*v.extrude* creates faces, kernels or 3D lines based on input 2D vector
features. Points are converted to 3D vertical lines, lines to faces and
areas to volumes (composition of closed set of faces and kernel).

If **elevation** parameter is used then 3D vector features follow the
elevation model by using individual elevation values for the vertices.
Height for vertices is interpolated from elevation raster map using
given interpolation **method**.

## NOTES

*v.extrude* extrudes vector features which means that points are
converted to vertical lines. Lines and area boundaries are extruded to a
set of faces, each segment defines one face. Area centroids are written
as kernels. Area as a composition of boundaries and centroid is stored
as a closed set of faces and kernel which define a volume.

For conversion of 2D points or lines to 3D can be used
*[v.to.3d](v.to.3d.md)* or *[v.drape](v.drape.md)*. In opposite to
*v.extrude*, these modules do not extrude vector features, they defines
z-coordinate for the features from given parameters or by sampling
elevation raster map values. It means that no feature type conversion is
applied, points remain still points in the output vector map. Same
applies for the lines.

*v.extrude* modifies only features geometry. Feature categories remain
untouched and attribute data is copied from input vector map to the
output.

By default, all features (including features without category) from
input vector map are processed (**layer=-1**). Feature selection can be
applied by **layer**, **cats** or **where** parameter.

## EXAMPLES

### 3D houses with fixed height

```sh
v.extrude input=houses output=houses3D height=5 type=area
```

### 3D houses with individual height

```sh
v.extrude input=houses output=houses3D elevation=dem height_column=height type=area
```

### Convert 2D points to 3D vertical lines with fixed height

```sh
v.extrude input=geodetic_pts output=points3D height=200 type=point
```

## SEE ALSO

*[v.transform](v.transform.md), [v.drape](v.drape.md),
[v.to.3d](v.to.3d.md)*

*[wxGUI 3D viewer](wxGUI.nviz.md)*

## AUTHORS

Jachym Cepicky,  
Updated for GRASS 7 by Martin Landa, FBK-irst, Italy and Czech Technical
University in Prague, Czech Republic
