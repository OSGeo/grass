## DESCRIPTION

*v.rectify* uses control points to calculate a 2D or 3D transformation
matrix based on a first, second, or third order polynomial and then
converts x,y(, z) coordinates to standard map coordinates for each
object in the vector map. The result is a vector map with a transformed
coordinate system (i.e., a different coordinate system than before it
was rectified).

The *-o* flag enforces orthogonal rotation (currently for 3D only) where
the axes remain orthogonal to each other, e.g. a cube with right angles
remains a cube with right angles after transformation. This is not
guaranteed even with affine (1st order) 3D transformation.

Great care should be taken with the placement of Ground Control Points.
For 2D transformation, the control points must not lie on a line,
instead 3 of the control points must form a triangle. For 3D
transformation, the control points must not lie on a plane, instead 4 of
the control points must form a triangular pyramid. It is recommended to
investigate RMS errors and deviations of the Ground Control Points prior
to transformation.

2D Ground Control Points can be identified in
*[g.gui.gcp](g.gui.gcp.md)*.

3D Ground Control Points must be provided in a text file with the
**points** option. The 3D format is equivalent to the format for 2D
ground control points with an additional third coordinate:

```sh
 x y z east north height status
```

where *x, y, z* are source coordinates, *east, north, height* are target
coordinates and status (0 or 1) indicates whether a given point should
be used. Numbers must be separated by space and must use a point (.) as
decimal separator.

If no **group** is given, the rectified vector will be written to the
current mapset. If a **group** is given and a target has been set for
this group with *[i.target](i.target.md)*, the rectified vector will be
written to the target project and mapset.

### Coordinate transformation and RMSE

The desired order of transformation (1, 2, or 3) is selected with the
**order** option. If the **-r** flag is given, *v.rectify* will
calculate the Root Mean Square Error (RMSE) and print out statistics in
tabular format. The last row gives a summary with the first column
holding the number of active points, followed by average deviations for
each dimension and both forward and backward transformation and finally
forward and backward overall RMSE.

#### 2D linear affine transformation (1st order transformation)

x' = a1 + b1 \* x + c1 \* y

y' = a2 + b2 \* x + c2 \* y

#### 3D linear affine transformation (1st order transformation)

x' = a1 + b1 \* x + c1 \* y + d1 \* z

y' = a2 + b2 \* x + c2 \* y + d2 \* z

z' = a3 + b3 \* x + c3 \* y + d3 \* z

The a,b,c,d coefficients are determined by least squares regression
based on the control points entered. This transformation applies
scaling, translation and rotation. It is NOT a general purpose
rubber-sheeting, nor is it ortho-photo rectification using a DEM, not
second order polynomial, etc. It can be used if (1) you have
geometrically correct data, and (2) the terrain or camera distortion
effect can be ignored.

#### Polynomial Transformation Matrix (2nd, 3d order transformation)

*v.rectify* uses a first, second, or third order transformation matrix
to calculate the registration coefficients. The minimum number of
control points required for a 2D transformation of the selected order
(represented by n) is

((n + 1) \* (n + 2) / 2)

or 3, 6, and 10 respectively. For a 3D transformation of first, second,
or third order, the minimum number of required control points is 4, 10,
and 20, respectively. It is strongly recommended that more than the
minimum number of points be identified to allow for an overly-determined
transformation calculation which will generate the Root Mean Square
(RMS) error values for each included point. The polynomial equations are
determined using a modified Gaussian elimination method.

## SEE ALSO

The GRASS 4 *[Image Processing
manual](https://grass.osgeo.org/gdp/imagery/grass4_image_processing.pdf)*

*[g.gui.gcp](g.gui.gcp.md), [i.group](i.group.md),
[i.rectify](i.rectify.md), [i.target](i.target.md),
[m.transform](m.transform.md), [r.proj](r.proj.md), [v.proj](v.proj.md),
[v.transform](v.transform.md)*  
*[Manage Ground Control Points](wxGUI.gcp.md)*

## AUTHOR

Markus Metz

based on [i.rectify](i.rectify.md)
