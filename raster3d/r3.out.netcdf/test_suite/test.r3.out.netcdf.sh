# Tests for r3.out.netcdf and r3.in.netcdf
# This script tests the export of voxel data using r3.out.netcdf
# as well as the import of the generated data with r3.in.netcdf 

# We set up a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=-40 n=40 w=-20 e=100 b=-25 t=25 res=10 res3=10 -p3
# We create several (float, double, null value) voxel map
# with value = col + row + depth. 
r3.mapcalc --o expr="volume_float = float(col() + row() + depth())"
r3.mapcalc --o expr="volume_double = double(col() + row() + depth())"
r3.mapcalc --o expr="volume_time_double = double(col() + row() + depth())"
r3.timestamp map=volume_time_double date='1 Jan 2001/5 Jan 2001'
r3.support map=volume_time_double vunit="days"
# @test
r3.out.netcdf --o input=volume_float output=test_float.nc
r3.out.netcdf --o input=volume_double output=test_double.nc
r3.out.netcdf --o input=volume_time_double output=test_time_double.nc

ncdump -h test_float.nc
ncdump -h test_double.nc
ncdump -h test_time_double.nc