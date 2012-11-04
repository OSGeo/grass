# This script tests r.colors and r.colors.out as well
# as r3.colors and r3.colors.out which have the same code base
# Color rules are set with r/r3.colors and exported using r/r3.colors.out

# We specific a small region in the
# @preprocess step and generate
# raster data with r.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=90 w=0 e=100 b=0 t=50 res=10 res3=10 -p3
# To test r.colors, we need several maps of different types
r.mapcalc --o expr="test_elev_int = int(if(row() == 2, null(), row()))"
r.mapcalc --o expr="test_elev_double = double(if(row() == 2, null(), row() + 0.5))"
r.mapcalc --o expr="test_elev_float = float(if(row() == 2, null(), row() + 0.5))"
# Create the volumes
r3.mapcalc --o expr="volume_double = double(col() + row() + depth())"
r3.mapcalc --o expr="volume_double_null = if(row() == 1 || row() == 5, null(), volume_double)"

# First we @test the integer maps
# We use the examples to test the import, export and setting of color tables with different options
# the validation is based on raster map color rules @files reference files created with r.colors.out
r.colors    map=test_elev_int rules=example1 && r.colors.out --o map=test_elev_int rules=test_elev_int_example1.txt
r.colors -e map=test_elev_int rules=example1 && r.colors.out --o map=test_elev_int rules=test_elev_int_example1_hist.txt
r.colors    map=test_elev_int rules=example2 && r.colors.out --o map=test_elev_int rules=test_elev_int_example2.txt
r.colors -g map=test_elev_int rules=example2 && r.colors.out --o map=test_elev_int rules=test_elev_int_example2_log.txt
r.colors    map=test_elev_int rules=example3 && r.colors.out --o map=test_elev_int rules=test_elev_int_example3.txt
r.colors -a map=test_elev_int rules=example3 && r.colors.out --o map=test_elev_int rules=test_elev_int_example3_logabs.txt
r.colors    map=test_elev_int rules=example4 && r.colors.out --o map=test_elev_int rules=test_elev_int_example4.txt
r.colors -n map=test_elev_int rast=test_elev_int && r.colors.out --o map=test_elev_int rules=test_elev_int_example4_inv.txt
# test the percentage output
r.colors map=test_elev_int rules=example1 && r.colors.out --o -p map=test_elev_int rules=test_elev_int_example1_perc.txt
r.colors map=test_elev_int rules=example2 && r.colors.out --o -p map=test_elev_int rules=test_elev_int_example2_perc.txt
r.colors map=test_elev_int rules=example3 && r.colors.out --o -p map=test_elev_int rules=test_elev_int_example3_perc.txt
r.colors map=test_elev_int rules=example4 && r.colors.out --o -p map=test_elev_int rules=test_elev_int_example4_perc.txt
# float maps
r.colors    map=test_elev_float rules=example1 && r.colors.out --o map=test_elev_float rules=test_elev_float_example1.txt
r.colors -e map=test_elev_float rules=example1 && r.colors.out --o map=test_elev_float rules=test_elev_float_example1_hist.txt
r.colors    map=test_elev_float rules=example2 && r.colors.out --o map=test_elev_float rules=test_elev_float_example2.txt
r.colors -g map=test_elev_float rules=example2 && r.colors.out --o map=test_elev_float rules=test_elev_float_example2_log.txt
r.colors    map=test_elev_float rules=example3 && r.colors.out --o map=test_elev_float rules=test_elev_float_example3.txt
r.colors -a map=test_elev_float rules=example3 && r.colors.out --o map=test_elev_float rules=test_elev_float_example3_logabs.txt
r.colors    map=test_elev_float rules=example4 && r.colors.out --o map=test_elev_float rules=test_elev_float_example4.txt
r.colors -n map=test_elev_float rast=test_elev_float && r.colors.out --o map=test_elev_float rules=test_elev_float_example4_inv.txt
# double maps
r.colors    map=test_elev_double rules=example1 && r.colors.out --o map=test_elev_double rules=test_elev_double_example1.txt
r.colors -e map=test_elev_double rules=example1 && r.colors.out --o map=test_elev_double rules=test_elev_double_example1_hist.txt
r.colors    map=test_elev_double rules=example2 && r.colors.out --o map=test_elev_double rules=test_elev_double_example2.txt
r.colors -g map=test_elev_double rules=example2 && r.colors.out --o map=test_elev_double rules=test_elev_double_example2_log.txt
r.colors    map=test_elev_double rules=example3 && r.colors.out --o map=test_elev_double rules=test_elev_double_example3.txt
r.colors -a map=test_elev_double rules=example3 && r.colors.out --o map=test_elev_double rules=test_elev_double_example3_logabs.txt
r.colors    map=test_elev_double rules=example4 && r.colors.out --o map=test_elev_double rules=test_elev_double_example4.txt
r.colors -n map=test_elev_double rast=test_elev_double && r.colors.out --o map=test_elev_double rules=test_elev_double_example4_inv.txt

# The volume maps using r3.colors and r3.colors.out
r3.colors    map=volume_double_null rules=example1 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example1.txt
r3.colors -e map=volume_double_null rules=example1 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example1_hist.txt
r3.colors    map=volume_double_null rules=example2 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example2.txt
r3.colors -g map=volume_double_null rules=example2 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example2_log.txt
r3.colors    map=volume_double_null rules=example3 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example3.txt
r3.colors -a map=volume_double_null rules=example3 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example3_logabs.txt
r3.colors    map=volume_double_null rules=example4 && r3.colors.out --o map=volume_double_null rules=test_volume_double_example4.txt
r3.colors -n map=volume_double_null volume=volume_double_null && r3.colors.out --o map=volume_double_null rules=test_volume_double_example4_inv.txt
# Apply a raster color table to the volume map
r3.colors    map=volume_double_null raster=test_elev_double   && r3.colors.out --o map=volume_double_null rules=test_volume_double_example5.txt

# Test the removement the raster3d color table, a default color table will be created
r3.colors -r map=volume_double_null && r3.colors.out --o map=volume_double_null rules=test_volume_double_default.txt
# Test the removement the raster color table, a default color table will be created
r.colors -r map=test_elev_double && r.colors.out --o map=test_elev_double rules=test_elev_double_default.txt
