# This script tests the conversion of raster maps
# into a single voxel map with r.to.rast3elev

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster data with r.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=100 b=0 t=50 res=10 res3=10 -p3
# We create several evlevation and value maps 
r.mapcalc --o expr="elev_5_float = float(5)"
r.mapcalc --o expr="elev_5_double = double(5)"
r.mapcalc --o expr="elev_45_float = float(45)"
r.mapcalc --o expr="elev_45_double = double(45)"
r.mapcalc --o expr="elev_25_float = float(25)"
r.mapcalc --o expr="elev_25_double = double(25)"
r.mapcalc --o expr="elev_20_float = float(20)"
r.mapcalc --o expr="elev_20_double = double(20)"
r.mapcalc --o expr="raster_float = float(1)"
r.mapcalc --o expr="raster_double = double(2)"

# We @test several methods to generate @raster3d maps from elevation and value raster maps
# For validation a @precision=0 should be used. Several methods for voxel map creation are tested
r.to.rast3elev --o lower=0 upper=2 input=raster_float  elevation=elev_25_float  output=test_elev_volume_float_1
r.to.rast3elev --o lower=0 upper=2 input=raster_double elevation=elev_25_double output=test_elev_volume_double_1
r.to.rast3elev --o upper=2         input=raster_float  elevation=elev_25_float  output=test_elev_volume_float_2
r.to.rast3elev --o upper=2         input=raster_double elevation=elev_25_double output=test_elev_volume_double_2
r.to.rast3elev --o                 input=raster_float  elevation=elev_25_float  output=test_elev_volume_float_3
r.to.rast3elev --o                 input=raster_double elevation=elev_25_double output=test_elev_volume_double_3
r.to.rast3elev --o -u              input=raster_float  elevation=elev_25_float  output=test_elev_volume_float_4 
r.to.rast3elev --o -u              input=raster_double elevation=elev_25_double output=test_elev_volume_double_4
r.to.rast3elev --o -l              input=raster_float  elevation=elev_25_float  output=test_elev_volume_float_5
r.to.rast3elev --o -l              input=raster_double elevation=elev_25_double output=test_elev_volume_double_5
r.to.rast3elev --o -lu             input=raster_float  elevation=elev_25_float  output=test_elev_volume_float_5
r.to.rast3elev --o -lu             input=raster_double elevation=elev_25_double output=test_elev_volume_double_5
# In case the elevation map is located between two layer, booth layer are filled with data
r.to.rast3elev --o                 input=raster_float  elevation=elev_20_float  output=test_elev_volume_float_6
r.to.rast3elev --o                 input=raster_double elevation=elev_20_double output=test_elev_volume_double_6
# Test with two to three elevation and value maps using different fill styles
r.to.rast3elev --o -u  input=elev_25_float,elev_45_float   elevation=elev_25_float,elev_45_float   output=test_elev_volume_float_7 
r.to.rast3elev --o -u  input=elev_25_double,elev_45_double elevation=elev_25_double,elev_45_double output=test_elev_volume_double_7
r.to.rast3elev --o -l  input=elev_45_float,elev_25_float,elev_5_float    elevation=elev_45_float,elev_25_float,elev_5_float    output=test_elev_volume_float_8
r.to.rast3elev --o -l  input=elev_45_double,elev_25_double,elev_5_double elevation=elev_45_double,elev_25_double,elev_5_double output=test_elev_volume_double_8
# Seems to be a bug in this case, so commented out, need to be investigated
# r.to.rast3elev --o -lu input=elev_25_float,elev_5_float   elevation=elev_25_float,elev_5_float   output=test_elev_volume_float_9
# r.to.rast3elev --o -lu input=elev_25_double,elev_5_double elevation=elev_25_double,elev_5_double output=test_elev_volume_double_9

# Export of the references
# for i in `g.list type=raster3d pattern=test_elev_volume_*` ; do r3.out.ascii dp=0 input=$i output=${i}.ref; done
