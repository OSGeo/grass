# This script tests the export of voxel data
# into the VTK format. Almost all options of
# r3.out.vtk are tested. Validation data for each test
# is located in the module source directory

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster and voxel data with r.mapcalc and r3.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Now generate two elevation maps, we have 8 rows and use
# them for elevation computation. The rows are counted from north
# to south. So in the south the elevation must have a maximum.
r.mapcalc --o expr="elev_bottom = row()"
r.mapcalc --o expr="elev_top = row() + 50"
# Now create a voxel map with value = col + row + depth. 
r3.mapcalc --o expr="volume = col() + row() + depth()"
# Add null value information
r3.mapcalc --o expr="volume_null = if(row() == 2 || row() == 7, null(), volume)"
# Create the rgb maps
r3.mapcalc --o expr="volume_rgb = volume_null * 5"

# The first @test just exports the volume map as cell and point data
# using alow precision and replaces the default null value with 0
# the created @files should be compared with the reference data.
r3.out.vtk --o input=volume_null output=test_volume_null_1_cells.vtk dp=3 null=0
r3.out.vtk -p --o input=volume_null output=test_volume_null_1_points.vtk dp=3 null=0

# The second @test adds rgb and vector maps. We re-use the created volume map
# for vector creation. The rgb value must range fom 0 - 255. The generated @files
# should be compared with the reference data.
r3.out.vtk --o rgbmaps=volume_rgb,volume_rgb,volume_rgb vectormaps=volume_null,volume_null,volume_null input=volume_null output=test_volume_null_1_cells_rgb_vect.vtk dp=3 null=-1.0
r3.out.vtk -p --o rgbmaps=volume_rgb,volume_rgb,volume_rgb vectormaps=volume_null,volume_null,volume_null input=volume_null output=test_volume_null_1_points_rgb_vect.vtk dp=3 null=-1.0

# The third @test uses raster maps to create volume data with an elevation surface
# The maximum elevation should be in the south. Reference @files are present for validation.
r3.out.vtk -s --o top=elev_top bottom=elev_bottom input=volume_null output=test_volume_null_1_cells_elevation.vtk dp=3 null=0
r3.out.vtk -sp --o top=elev_top bottom=elev_bottom input=volume_null output=test_volume_null_1_points_elevation.vtk dp=3 null=0

# Comparison of references and vtk files
for i in `ls *.ref` ; do 
    diff $i "`basename $i .ref`.vtk" ; 
done
rm *.vtk
