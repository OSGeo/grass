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

r.mapcalc expr="map_a = rand(0, 15)"
r.mapcalc expr="map_b = rand(1, 14)"
r.mapcalc expr="map_c = rand(2, 13)"

r.series.accumulate basemap=basemap input=map_a lower=lower \
                    output=test_accu_1 upper=upper method=gdd -f --verbose 

r.series.accumulate basemap=basemap input=map_a lower=lower \
                    output=test_accu_2 upper=upper method=winkler -f --verbose 

r.series.accumulate basemap=basemap input=map_a lower=lower \
                    output=test_accu_3 upper=upper method=mean --verbose 

r.series.accumulate basemap=basemap input=map_a,map_b,map_c limits=5,10 \
                    output=test_accu_4 method=gdd -f --verbose 

r.series.accumulate basemap=basemap input=map_a,map_b,map_c lower=lower limits=5,10 \
                    output=test_accu_5 method=winkler -f --verbose 

r.series.accumulate basemap=basemap input=map_a,map_b,map_c lower=lower limits=5,10 \
                    output=test_accu_6 range=6,9 method=mean --verbose 

# Test for correct results
for map in `g.mlist type=rast pattern=test_accu_*` ; do
    r.out.ascii input=${map} output=${map}.txt dp=2
done

for i in `ls test_accu_*.txt` ; do
    diff $i "`basename $i .txt`.ref" >> out.diff
done

CHAR_NUM=`cat out.diff | wc -c`

# Return as exit status 0 in case no diffs are found
exit $CHAR_NUM