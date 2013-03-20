# This script tests the r3.cross.rast to compute
# cross section raster maps based on a raster3d and elevation map

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster and voxel data with r.mapcalc and r3.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=100 b=0 t=50 res=10 res3=10 -p3
# We create several evlevation maps to create slices of the voxel map
# We start from bottom and raise to the top
r.mapcalc --o expr="elev_0 = 0"
r.mapcalc --o expr="elev_1 = 5"
r.mapcalc --o expr="elev_2 = 15"
r.mapcalc --o expr="elev_3 = 25"
r.mapcalc --o expr="elev_4 = 35"
r.mapcalc --o expr="elev_5 = 45"
r.mapcalc --o expr="elev_NAN = 50"
r.mapcalc --o expr="elev_cross = float(col()* 5)"
# Now create a voxel map with value = col + row + depth. 
r3.mapcalc --o expr="volume = col() + row() + depth()"
# Add null value information
r3.mapcalc --o expr="volume_null = if(row() == 1 || row() == 5, null(), volume)"

# We @test the creation of slices and a cross section of the voxel map. Reference data
# for @raster map validation is located in the r3.cross.rast source directory.
# Slice 0 and 1 should be identical. The last slice should be NAN.
r3.cross.rast --o input=volume_null elevation=elev_0 output=test_cross_section_slice_0
r3.cross.rast --o input=volume_null elevation=elev_1 output=test_cross_section_slice_1
r3.cross.rast --o input=volume_null elevation=elev_2 output=test_cross_section_slice_2
r3.cross.rast --o input=volume_null elevation=elev_3 output=test_cross_section_slice_3
r3.cross.rast --o input=volume_null elevation=elev_4 output=test_cross_section_slice_4
r3.cross.rast --o input=volume_null elevation=elev_5 output=test_cross_section_slice_5
r3.cross.rast --o input=volume_null elevation=elev_NAN output=test_cross_section_slice_NAN
r3.cross.rast --o input=volume_null elevation=elev_cross output=test_cross_section_result

# Export of the text files
for i in `g.mlist type=rast pattern=test_cross_section_*` ; do 
    r.out.ascii input=$i output=${i}.txt; 
done

# Comparison of references and text files
for i in `ls *.ref` ; do 
    diff $i "`basename $i .ref`.txt" ; 
done
rm *.txt
