#!/bin/sh -e
# Space time raster dataset neighborhood operations
# We need to set a specific region in the
# @preprocess step of this test.
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 -p

export GRASS_OVERWRITE=1

# Generate data
r.mapcalc expr="temp_6 = 5"
r.mapcalc expr="temp_5 = 10"
r.mapcalc expr="temp_4 = 15"
r.mapcalc expr="temp_3 = 20"
r.mapcalc expr="temp_2 = 25"
r.mapcalc expr="temp_1 = 30"

t.create type=strds temporaltype=absolute output=temp_abs1 title="A test" descr="A test"
t.register -i type=raster input=temp_abs1 maps=temp_1,temp_2,temp_3,temp_4,temp_5,temp_6 \
    start="2001-01-01" increment="2 months"

# The first @test

t.rast.accumulate -r input=temp_abs1 output=temp_accumulation base=temp_acc \
    limits=10,25 start="2001-01-01" gran="2 months" cycle="12 months" suffix="num%03"

t.rast.accdetect -r input=temp_accumulation occurrence=temp_occ base=temp_occ \
    range=20,80 start="2001-01-01" cycle="12 months" staend=1,2,3 indi=temp_indi suffix="num%03"

t.rast.list temp_accumulation col=name,start_time,min,max > data/test_2_temp_accumulation.txt
t.rast.list temp_occ col=name,start_time,min,max  > data/test_2_temp_occ.txt
t.rast.list temp_indi col=name,start_time,min,max > data/test_2_temp_indi.txt

#t.remove -rf type=strds input=temp_abs1,temp_accumulation,temp_indi

cd data

for i in `ls test_2_*.txt` ; do
    diff $i "`basename $i .txt`.ref" >> out.diff
done

CHAR_NUM=`cat out.diff | wc -c`

# Return as exit status 0 in case no diffs are found
exit $CHAR_NUM
