# Tests for r3.null

# We set up a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# We create several (float, double, null value) voxel map
# with value = col + row + depth. 
r3.mapcalc --o expr="test_volume_float_1 = float(col() + row() + depth())"
r3.mapcalc --o expr="test_volume_float_2 = test_volume_float_1"
r3.mapcalc --o expr="test_volume_double_1 = double(col() + row() + depth())"
r3.mapcalc --o expr="test_volume_double_2 = test_volume_double_1"
# Add null value information 
r3.mapcalc --o expr="test_volume_float_null_1 = if(row() == 1 || row() == 5, null(), test_volume_float_1)"
r3.mapcalc --o expr="test_volume_float_null_2 = if(row() == 1 || row() == 5, null(), test_volume_float_1)"
r3.mapcalc --o expr="test_volume_double_null_1 = if(row() == 1 || row() == 5, null(), test_volume_double_1)"
r3.mapcalc --o expr="test_volume_double_null_2 = if(row() == 1 || row() == 5, null(), test_volume_double_1)"

# We @test r3.null to set and modify null values.
# Validation is based on @files with @precision=3
# First float maps
r3.null map=test_volume_float_1 setnull=3,4,5
r3.null map=test_volume_float_2 setnull=7,8,9
r3.null map=test_volume_float_null_1 null=-1.5
r3.null map=test_volume_float_null_2 null=-10.5
# Double maps
r3.null map=test_volume_double_1 setnull=3,4,5
r3.null map=test_volume_double_2 setnull=7,8,9
r3.null map=test_volume_double_null_1 null=-1.5
r3.null map=test_volume_double_null_2 null=-10.5

# Commands to export the references 
r3.out.ascii dp=3 input=test_volume_float_1 output=test_volume_float_1.txt
r3.out.ascii dp=3 input=test_volume_float_2 output=test_volume_float_2.txt
r3.out.ascii dp=3 input=test_volume_float_null_1 output=test_volume_float_null_1.txt
r3.out.ascii dp=3 input=test_volume_float_null_2 output=test_volume_float_null_2.txt
r3.out.ascii dp=3 input=test_volume_double_1 output=test_volume_double_1.txt
r3.out.ascii dp=3 input=test_volume_double_2 output=test_volume_double_2.txt
r3.out.ascii dp=3 input=test_volume_double_null_1 output=test_volume_double_null_1.txt
r3.out.ascii dp=3 input=test_volume_double_null_2 output=test_volume_double_null_2.txt
