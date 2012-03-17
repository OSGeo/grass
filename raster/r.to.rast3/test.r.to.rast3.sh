# This script tests the conversion of raster maps
# into a single voxel map with r.to.rast3

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster data with r.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=100 b=0 t=50 res=10 res3=10 -p3
# We create several evlevation maps to create slices of the voxel map
# We start from bottom and raise to the top
r.mapcalc --o expr="elev_double = double(if(row() == 2, null(), row()))"
r.mapcalc --o expr="elev_float = float(if(row() == 2, null(), row()))"
r.mapcalc --o expr="elev_1 = 5"
r.mapcalc --o expr="elev_2 = 15"
r.mapcalc --o expr="elev_3 = 25"
r.mapcalc --o expr="elev_4 = 35"
r.mapcalc --o expr="elev_5 = 45"

# We @test several methods to generate @raster3d maps from raster maps
# For validation a @precision=0 should be used. We start with 5 raster maps, 
# then 3 raster maps then using single float and double raster maps 
# with differen tile sizes and null data
r.to.rast3 --o input=elev_1,elev_2,elev_3,elev_4,elev_5 output=test_volume_6_raster tilesize=1
r.to.rast3 --o input=elev_1,elev_2,elev_3               output=test_volume_3_raster tilesize=1
r.to.rast3 --o input=elev_float                         output=test_volume_float_raster tilesize=2
r.to.rast3 --o input=elev_double                        output=test_volume_double_raster tilesize=2

# Uncomment for reference data export
# r3.out.ascii input=test_volume_6_raster output=test_volume_6_raster.ref dp=0
# r3.out.ascii input=test_volume_3_raster output=test_volume_3_raster.ref dp=0
# r3.out.ascii input=test_volume_float_raster output=test_volume_float_raster.ref dp=0
# r3.out.ascii input=test_volume_double_raster output=test_volume_double_raster.ref dp=0

# We need another @preprocess step to generate a different region
g.region s=0 n=80 w=0 e=100 b=0 t=50 res=5 res3=10 -p3

# We @test the automatic resolution correction for @raster3d maps in case
# 2d and 3d region are different
r.to.rast3 --o input=elev_double output=test_volume_double_raster_res tilesize=2

# Uncomment for reference data export
# r3.out.ascii input=test_volume_double_raster_res output=test_volume_double_raster_res.ref dp=0