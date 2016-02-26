#!/bin/sh -e
# Space time raster dataset neighborhood operations
# We need to set a specific region in the
# @preprocess step of this test.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 -p

export GRASS_OVERWRITE=1

# Generate data
r.mapcalc expr="temp_1 = 5"
r.mapcalc expr="temp_2 = 10"
r.mapcalc expr="temp_3 = 15"
r.mapcalc expr="temp_4 = 20"
r.mapcalc expr="temp_5 = 25"
r.mapcalc expr="temp_6 = 30"

t.create type=strds temporaltype=absolute output=temp_abs1 title="A test" descr="A test"
t.register -i type=raster input=temp_abs1 maps=temp_1,temp_2,temp_3,temp_4,temp_5,temp_6 \
    start="2001-01-01" increment="2 months"

# The first @test

t.rast.accumulate input=temp_abs1 output=temp_accumulation base=temp_acc \
    limits=10,25 start="2001-01-01" gran="2 months" cycle="12 months" suffix="num%03"

t.rast.accdetect input=temp_accumulation occurrence=temp_occ base=temp_occ \
    range=20,80 start="2001-01-01" cycle="12 months" suffix="num%03"

# Check the registered maps metadata
t.rast.list temp_accumulation col=name,start_time,min,max > data/test_1_temp_accumulation.txt
t.rast.list temp_occ col=name,start_time,min,max          > data/test_1_temp_occ_a.txt

# Leets test the minimum and maximum STRDS implementation

r.mapcalc expr="minimum = 18"
r.mapcalc expr="maximum = 78"

# We use different
t.create type=strds temporaltype=absolute output=minimum title="minimum limit" descr="minimum limit"
t.register -i type=raster input=minimum maps=minimum start="2001-01-01" increment="8 months"

t.create type=strds temporaltype=absolute output=maximum title="maximum limit" descr="maximum limit"
t.register -i type=raster input=maximum maps=maximum start="2001-01-01" increment="10 months"

t.rast.accdetect input=temp_accumulation occurrence=temp_occ base=temp_occ \
    range=20,80 start="2001-01-01" cycle="12 months" min=minimum \
    max=maximum staend=1,2,3 indi=temp_indi suffix="num%03"

# Check the registered maps metadata
t.rast.list temp_occ col=name,start_time,min,max          > data/test_1_temp_occ_b.txt
t.rast.list temp_indi col=name,start_time,min,max         > data/test_1_temp_indi.txt

#t.remove -rf type=strds input=temp_abs1,temp_accumulation,temp_indi,minimum,maximum

cd data

for i in `ls test_1_*.txt` ; do
    diff $i "`basename $i .txt`.ref" >> out.diff
done

CHAR_NUM=`cat out.diff | wc -c`

# Return as exit status 0 in case no diffs are found
exit $CHAR_NUM
