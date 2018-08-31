# Use this script to generate the VTK files for 3D raster visualization
# showing cells, tiles and tile dimension.
# VTK files can bevisualized with Paraview (www.paraview.org)

g.region s=0 n=120 w=0 e=160 b=0 t=80 res3=10 -p3
r3.mapcalc --o expr="sample_raster = col() + row() + depth()"
r3.out.vtk --o input=sample_raster output=sample_raster.vtk dp=0 null=0

g.region s=0 n=120 w=0 e=160 b=0 t=80 res3=40 -p3
r3.out.vtk --o input=sample_raster output=tiles.vtk dp=0 null=0

g.region s=0 n=40 w=0 e=40 b=40 t=80 res3=10 -p3
r3.out.vtk --o input=sample_raster output=tile.vtk dp=0 null=0
