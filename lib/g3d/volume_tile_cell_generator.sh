# Use this script to generate the VTK files for volume visualization
# showing voxel, tiles and tile dimension.
# VTK files can bevisualized with Paraview (www.paraview.org)

g.region s=0 n=120 w=0 e=160 b=0 t=80 res3=10 -p3
r3.mapcalc --o expr="volume = col() + row() + depth()"
r3.out.vtk --o input=volume output=volume.vtk dp=0 null=0

g.region s=0 n=120 w=0 e=160 b=0 t=80 res3=40 -p3
r3.out.vtk --o input=volume output=tiles.vtk dp=0 null=0

g.region s=0 n=40 w=0 e=40 b=40 t=80 res3=10 -p3
r3.out.vtk --o input=volume output=tile.vtk dp=0 null=0
