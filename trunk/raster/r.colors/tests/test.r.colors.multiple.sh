# This script tests r.colors and r.colors.out
# Color rules are set with r.colors and exported using r.colors.out

# We specific a small region in the
# @preprocess step and generate
# raster data with r.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=90 w=0 e=100 b=0 t=50 res=10 res3=10 -p3
# To test r.colors, we need several maps of different types
r.mapcalc --o expr="test_elev_double_1 = double(rand(-15.0, 5.0))"
r.mapcalc --o expr="test_elev_double_2 = double(rand(0.0, 10.0))"
r.mapcalc --o expr="test_elev_double_3 = double(rand(5.0, 15.0))"

r.mapcalc --o expr="test_elev_int_1 = int(rand(-15.0, 5.0))"
r.mapcalc --o expr="test_elev_int_2 = int(rand(0.0, 10.0))"
r.mapcalc --o expr="test_elev_int_3 = int(rand(5.0, 15.0))"


# First we @test the integer maps
# We use the examples to test the import, export and setting of color tables with different options
# the validation is based on raster map color rules @files.txterence files created with r.colors.out
r.colors    map=test_elev_int_1,test_elev_int_2,test_elev_int_3 \
            color=difference && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_difference_1.txt

r.colors -e map=test_elev_int_1,test_elev_int_2,test_elev_int_3 \
            color=difference && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_difference_hist.txt

r.colors -n map=test_elev_int_1,test_elev_int_2,test_elev_int_3 \
            color=difference && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_difference_invert.txt

r.colors -a map=test_elev_int_1,test_elev_int_2,test_elev_int_3 \
            color=difference && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_difference_logabsscale.txt

r.colors -g map=test_elev_int_2,test_elev_int_3 \
            color=difference && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_difference_logscale.txt

r.colors    map=test_elev_int_1,test_elev_int_2,test_elev_int_3 \
            color=random && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_random.txt

r.colors    map=test_elev_int_1,test_elev_int_2,test_elev_int_3 \
            color=grey.eq && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_grey_eq.txt

r.colors    map=test_elev_int_2,test_elev_int_3 \
            color=grey.log && r.colors.out --o map=test_elev_int_3 \
            rules=test_elev_int_maps_grey_log.txt

# Tests with floating point maps
r.colors    map=test_elev_double_1,test_elev_double_2,test_elev_double_3 \
            color=difference && r.colors.out --o map=test_elev_double_3 \
            rules=test_elev_double_maps_difference_1.txt

r.colors -e map=test_elev_double_1,test_elev_double_2,test_elev_double_3 \
            color=difference && r.colors.out --o map=test_elev_double_3 \
            rules=test_elev_double_maps_difference_hist.txt

r.colors -n map=test_elev_double_1,test_elev_double_2,test_elev_double_3 \
            color=difference && r.colors.out --o map=test_elev_double_3 \
            rules=test_elev_double_maps_difference_invert.txt

r.colors -a map=test_elev_double_1,test_elev_double_2,test_elev_double_3 \
            color=difference && r.colors.out --o map=test_elev_double_3 \
            rules=test_elev_double_maps_difference_logabsscale.txt

r.colors -g map=test_elev_double_2,test_elev_double_3 \
            color=difference && r.colors.out --o map=test_elev_double_3 \
            rules=test_elev_double_maps_difference_logscale.txt
