# Tests for r3.out.ascii and r3.in.ascii
# This script tests the export of voxel data using r3.out.ascii
# as well as the import of the generated data with r3.in.ascii 
# using different row and depth ordering options.

# We set up a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# We create several (float, double, null value) voxel map
# with value = col + row + depth. 
r3.mapcalc --o expr="volume_float = float(col() + row() + depth())"
r3.mapcalc --o expr="volume_double = double(col() + row() + depth())"
# Add null value information to test correct null value handling
r3.mapcalc --o expr="volume_float_null = if(row() == 1 || row() == 5, null(), volume_float)"
r3.mapcalc --o expr="volume_double_null = if(row() == 1 || row() == 5, null(), volume_double)"

# We export float data in the first @test using different order and precision.
# The created text @files are validated of correct ordering and null data handling.
# All reference files are located in the r3.out.ascii directory.
r3.out.ascii --o     input=volume_float_null output=test_float_nsbt_null.txt dp=0 null=*
r3.out.ascii --o -r  input=volume_float_null output=test_float_snbt_null.txt dp=0 null=*
r3.out.ascii --o -d  input=volume_float_null output=test_float_nstb_null.txt dp=0 null=*
r3.out.ascii --o -rd input=volume_float_null output=test_float_sntb_null.txt dp=0 null=*
# Different precision and null values than default
r3.out.ascii --o     input=volume_float_null output=test_float_nsbt_null_prec5.txt dp=5 null=-1000
r3.out.ascii --o -rd input=volume_float_null output=test_float_sntb_null_prec8.txt dp=8 null=-2000
# Test the no header and grass6 compatibility flags
r3.out.ascii --o -h input=volume_float_null output=test_float_nsbt_null_no_header.txt dp=3 null=*
r3.out.ascii --o -c input=volume_float_null output=test_float_nsbt_null_grass6_comp_1.txt dp=3 null=*
# Any row or depth order should be ignored in case grass6 compatibility is enabled
# The result of comp_1, _2 and _3 must be identical
r3.out.ascii --o -cr  input=volume_float_null output=test_float_nsbt_null_grass6_comp_2.txt dp=3 null=*
r3.out.ascii --o -crd input=volume_float_null output=test_float_nsbt_null_grass6_comp_3.txt dp=3 null=*

# We export float data in the first @test using different order and precision.
# The created text @files are validated of correct ordering and null data handling.
# All reference files are located in the r3.out.ascii directory.
r3.out.ascii --o     input=volume_double_null output=test_double_nsbt_null.txt dp=0 null=*
r3.out.ascii --o -r  input=volume_double_null output=test_double_snbt_null.txt dp=0 null=*
r3.out.ascii --o -d  input=volume_double_null output=test_double_nstb_null.txt dp=0 null=*
r3.out.ascii --o -rd input=volume_double_null output=test_double_sntb_null.txt dp=0 null=*
# Different precision and null values than default
r3.out.ascii --o     input=volume_double_null output=test_double_nsbt_null_prec5.txt dp=5 null=-1000
r3.out.ascii --o -rd input=volume_double_null output=test_double_sntb_null_prec8.txt dp=8 null=-2000
# Test the no header and grass6 compatibility flags
r3.out.ascii --o -h input=volume_double_null output=test_double_nsbt_null_no_header.txt dp=3 null=*
r3.out.ascii --o -c input=volume_double_null output=test_double_nsbt_null_grass6_comp_1.txt dp=3 null=*
# Any row or depth order should be ignored in case grass6 compatibility is enabled
# The result of comp_1, _2 and _3 must be identical
r3.out.ascii --o -cr  input=volume_double_null output=test_double_nsbt_null_grass6_comp_2.txt dp=3 null=*
r3.out.ascii --o -crd input=volume_double_null output=test_double_nsbt_null_grass6_comp_3.txt dp=3 null=*

# In the third @test we import all the generated data using r3.in.ascii.
# The created @raster maps should be identical to the map "volume_double_null".
# The export of the created g3d map should use as @precision=0 for data validation.
# The same raster name is used for all the imported data and so for the validation reference file.
r3.in.ascii --o output=test_double_nsbt_null input=test_double_nsbt_null.txt nv=*
r3.in.ascii --o output=test_double_nsbt_null input=test_double_snbt_null.txt nv=*
r3.in.ascii --o output=test_double_nsbt_null input=test_double_nstb_null.txt nv=*
r3.in.ascii --o output=test_double_nsbt_null input=test_double_sntb_null.txt nv=*
# Different precision and null values than default
r3.in.ascii --o output=test_double_nsbt_null input=test_double_nsbt_null_prec5.txt nv=-1000
r3.in.ascii --o output=test_double_nsbt_null input=test_double_sntb_null_prec8.txt nv=-2000
# Import grass6 legacy data
r3.in.ascii --o output=test_double_nsbt_null input=test_double_nsbt_null_grass6_comp_1.txt

# In this @preprocess step for the last test we create a large region and 
# generate large input data to test the handling of large files.
g.region s=0 n=800 w=0 e=1200 b=0 t=50 res=10 res3=1.5 -p3
r3.mapcalc --o expr="volume_double_large = double(col() + row() + depth())"
# Add null value information
r3.mapcalc --o expr="volume_double_null_large = if(row() == 1 || row() == 5, null(), volume_double_large)"

# Now @test the export and import of large data without validation
r3.out.ascii --o input=volume_double_null_large output=test_double_nsbt_null_large.txt dp=0 null=*
r3.in.ascii --o output=test_double_nsbt_null_large input=test_double_nsbt_null_large.txt nv=*
# Just for the logs
r3.info test_double_nsbt_null_large

# Show differences between references and created text files
for i in `ls *.ref` ; do 
    diff $i "`basename $i .ref`.txt" ; 
done
rm *.txt
