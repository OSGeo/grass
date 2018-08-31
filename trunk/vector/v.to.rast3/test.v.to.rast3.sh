# Test the conversion of vector points into cell values
# of raster3d maps.

# We set up a specific region in the
# @preprocess step of this test. The region setting 
# should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# We import vector data located in the source directory
# The data was created using v.random with seed option:
# v.random --o -z seed=1 n=10 output=random_points column=value zmin=0 zmax=50 column_type=integer
# v.out.ascii input=random_points output=random_points.txt columns=value
# I don't know if the random generator always creates the same pseudo random number with seed 1
# on any supported operation system with a C standard library
v.in.ascii input=random_points.txt columns='x double precision, y double precision, z double precision, key int, value int' x=1 y=2 z=3 output=random_points --o

# After data creation we @test the vector to @raster3d map conversion
# As @prescision=0 is used for data export.
v.to.rast3 --o input=random_points column=value output=vector_to_volume

# Some data export commands for reference data creation and visual validation
# r3.out.ascii --o input=vector_to_volume output=vector_to_volume.ref dp=0 null=*
# r3.out.vtk input=vector_to_volume output=vector_to_volume.vtk null=0
# v.out.vtk input=random_points output=random_points.vtk
