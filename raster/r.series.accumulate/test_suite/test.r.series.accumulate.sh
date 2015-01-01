#!/bin/sh -e
# Test r.series.accumulate
# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster maps with r.mapcalc
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

r.mapcalc expr="basemap = if(row() == 3, null(), 10)"
r.mapcalc expr="lower = 5"
r.mapcalc expr="upper = 10"

r.mapcalc expr="map_a = rand(0, 15)" -s
r.mapcalc expr="map_b = rand(1, 14)" -s
r.mapcalc expr="map_c = rand(2, 13)" -s

# BEDD with lower limit map and upper limit value
r.series.accumulate basemap=basemap input=map_a lower=lower limits=5,10 \
                    output=test_accu_0 method=bedd -f --verbose 
# GDD with lower limit map
r.series.accumulate basemap=basemap input=map_a lower=lower \
                    output=test_accu_1 method=gdd -f --verbose 
# Winkler with lower limit map
r.series.accumulate basemap=basemap input=map_a lower=lower \
                    output=test_accu_2 method=gdd -f --verbose 
# Mean
r.series.accumulate basemap=basemap input=map_a \
                    output=test_accu_3  method=mean --verbose 
# Average
r.series.accumulate basemap=basemap input=map_a \
                    output=test_accu_3  method=mean --verbose 
# GDD with lower limit value
r.series.accumulate basemap=basemap input=map_a,map_b,map_c limits=5,10 \
                    output=test_accu_4 method=gdd -f --verbose 
# Winkler with  multiple maps, lower limit value
r.series.accumulate basemap=basemap input=map_a,map_b,map_c limits=5,10 \
                    output=test_accu_5 method=bedd -f --verbose 
# BEDD with  multiple maps, lower limit map and upper limit value
r.series.accumulate basemap=basemap input=map_a,map_b,map_c lower=lower limits=5,10 \
                    output=test_accu_6 method=bedd -f --verbose 
# BEDD with multiple maps, lower limit map and upper limit map
r.series.accumulate basemap=basemap input=map_a,map_b,map_c lower=lower upper=upper \
                    output=test_accu_7 method=bedd -f --verbose 
# Mean with range multiple maps
r.series.accumulate basemap=basemap input=map_a,map_b,map_c \
                    output=test_accu_8 range=6,9 method=mean --verbose 
# Mean with range
r.series.accumulate basemap=basemap input=map_a, \
                    output=test_accu_9 range=6,9 method=mean --verbose

# Test for correct results
for map in `g.list type=raster pattern=test_accu_*` ; do
    r.out.ascii input=${map} output=${map}.ref precision=2
done

for i in `ls test_accu_*.ref` ; do
    diff $i "`basename $i`" >> out.diff
done
rm -f test_accu_*.ref

CHAR_NUM=`cat out.diff | wc -c`

# Return as exit status 0 in case no diffs are found
exit $CHAR_NUM
