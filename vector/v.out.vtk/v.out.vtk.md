## DESCRIPTION

*v.out.vtk* converts a GRASS vector map in binary format to the VTK
ASCII output.

If the **output** parameter is not given, the output will be send to
stdout.

## NOTES

The following vector types can be exported together in one VTK ascii
file:

- point
- line
- centroid
- boundary
- area
- face

Category data (cat) for the selected vector type and layer will be
written as scalar data with name "cat\_{vectorname}". If no cat exists,
the value will set to -1 as normal cat's are always positive. If a
vector has more categories in one layer, only the first category will be
exported.  
  
3d vectors are supported by default. The created VTK data always
includes x, y and z coordinates (z = 0 if not a 3d vector map). Note
that you can easily convert your 2d vectors into 3d vectors with
v.drape.  
  
Because of the 32bit limits of OpenGL which is used by VTK,
visualisation errors may occur if the grass region contains coordinates
greater than 1.000.000 and vector coordinates with 0.01 - 0.001 meters
precisison. For this reason, the flag "-c" was added. The coordinates
are transformed to smaller coordinates (by decreasing the coordinates
with the region center).  
  
If the "-c" flag is used and the data should be visualised together with
other data exported via \*.out.vtk modules, be sure the "-c" flag was
also set in these modules. But this will only work with data from the
SAME location (The reference point for the coordinates transformation is
based on the default region).  
  
The GRASS vector data is converted into the polydata format of VTK:

- *vtk Vertices* -- representing points and centroids
- *vtk lines* -- representing lines and boundaries
- *vtk polygons* -- representing areas and faces

The VTK file can be visualized with *[VTK Toolkit](https://vtk.org/)*,
*[Paraview](https://www.paraview.org/)* and
*[MayaVi](https://github.com/enthought/mayavi)*.

### Attention

If areas or faces are exported, the data have to be triangulated within
Paraview or MayaVi.

## EXAMPLE

Spearfish example:

Export the soils with cats in layer 1:

```sh
v.out.vtk input=soils type=area layer=1 output=/tmp/soils.vtk
```

Export the streams with cats in layer 1:

```sh
v.out.vtk input=streams type=line layer=1 output=/tmp/streams.vtk
```

Write the archsite vtk output to stdout with cats in layer 1:

```sh
v.out.vtk input=archsites type=point layer=1
```

## SEE ALSO

*[v.out.ascii](v.out.ascii.md), [r.out.vtk](r.out.vtk.md),
[r3.out.vtk](r3.out.vtk.md)*

## AUTHOR

Soeren Gebbert
