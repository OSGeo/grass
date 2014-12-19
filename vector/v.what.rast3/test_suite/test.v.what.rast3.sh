# This script tests v.what.rast3d

# We specific a small region in the
# @preprocess step
# The region setting should work for UTM and LL test locations
g.region s=0 n=70 w=0 e=100 b=0 t=50 res=10 res3=10 -p3

# Create the volume and the sampling vector map
r3.mapcalc --o expr="plume = double(col() + row() + depth())"
# This is how the input data was created
# v.random --o -z seed=1 output=random_points npoints=10 zmin=0  zmax=50
# v.out.ascii --o format=standard input=random_points output=random_points.txt

v.in.ascii --o -z format=standard input=random_points.txt output=random_points
v.db.addtable --o map=random_points column="concentration double precision"

# @test the voxel sampling with vector points
v.what.rast3 map=random_points raster3d=plume column=concentration

# Some data export commands for reference data creation and visual validation
#v.out.ascii --o format=standard input=random_points output=random_points.ref
#v.db.select map=random_points > random_points_db.ref
#r3.out.vtk --o input=plume output=plume.vtk null=0
#v.out.vtk --o -n input=random_points output=random_points.vtk
