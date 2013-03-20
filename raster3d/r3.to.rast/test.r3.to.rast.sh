# This script tests the conversion of voxel data
# into raster data slices. Validation data for each test
# is located in the module source directory

# We need to set a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Now create a voxel map with value = col + row + depth. 
r3.mapcalc --o expr="volume = double(col() + row() + depth())"
# Add null value information
r3.mapcalc --o expr="volume_null = if(row() == 1 || row() == 5, null(), volume)"
# Create floating point map
r3.mapcalc --o expr="volume_null_float = float(volume_null)"


# First we @test with identical region settings for raster and voxel data
# Reference data for all generated @raster maps are located in the r3.to.rast source directory.
r3.to.rast --o input=volume_null output=test_raster_slice_1
r3.to.rast --o input=volume_null_float output=test_raster_slice_float

# Export of the references
for i in `g.mlist type=rast pattern=test_raster_slice_1*` ; do r.out.ascii input=$i output=${i}.txt; done
for i in `g.mlist type=rast pattern=test_raster_slice_float*` ; do r.out.ascii input=$i output=${i}.txt; done

# The next @preprocess step adjusts the raster region to increase the resolution by 2
g.region res=7.5 -p3

# This @test should show the via nearest neighbour resampled data.  
# Reference data for all generated @raster maps are located in the r3.to.rast source directory.
r3.to.rast --o input=volume_null output=test_raster_slice_2

# Export of the references
for i in `g.mlist type=rast pattern=test_raster_slice_2*` ; do r.out.ascii input=$i output=${i}.txt; done

# The next @preprocess step adjusts the raster region to increase the resolution by 2 again
g.region res=5 -p3

# This @test should show the via nearest neighbour resampled data.  
# Reference data for all generated @raster maps are located in the r3.to.rast source directory.
r3.to.rast --o input=volume_null output=test_raster_slice_3

# Export of the references
for i in `g.mlist type=rast pattern=test_raster_slice_3*` ; do r.out.ascii input=$i output=${i}.txt; done

# Comparison of references and text files
for i in `ls *.ref` ; do 
    diff $i "`basename $i .ref`.txt" ; 
done
rm *.txt
