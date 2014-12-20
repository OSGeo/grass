# Tests for r3.out.netcdf and r3.in.netcdf
# This script tests the export of voxel data using r3.out.netcdf
# as well as the import of the generated data with r3.in.netcdf 

# We set up a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=-90 n=90 w=-180 e=180 b=0 t=5 res=10 res3=10 tbres=1 -p3
# We create several (float, double, null value) voxel map
# with value = col + row + depth. 
r3.mapcalc --o expr="volume_float = float(col() + row() + depth())"
r3.mapcalc --o expr="volume_double = double(col() + row() + depth())"
r3.mapcalc --o expr="volume_time_double = double(col() + row() + depth())"
r3.mapcalc --o expr="volume_time_float = float(col() + row() + depth())"
r3.timestamp map=volume_time_double date='1 Jan 2001/5 Jan 2001'
r3.support map=volume_time_double vunit="days"
r3.timestamp map=volume_time_float date='5 seconds/10 seconds'
r3.support map=volume_time_float vunit="seconds"
# @test
r3.out.netcdf --o input=volume_float output=test_float.nc
r3.info volume_float
ncdump -h test_float.nc
r3.out.netcdf --o null=-100 input=volume_double output=test_double.nc
r3.info volume_double
ncdump -h test_double.nc
r3.out.netcdf --o -p input=volume_time_double output=test_time_double.nc
r3.info volume_time_double
ncdump -h test_time_double.nc
r3.out.netcdf --o -p null=-1000 input=volume_time_float output=test_time_float.nc
r3.info volume_time_float
ncdump -h test_time_float.nc

g.remove -f type=raster_3d name=volume_float,volume_double,volume_time_double,volume_time_float
