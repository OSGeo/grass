#!/bin/sh -e
# Space time raster dataset neighborhood operations
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 -p

export GRASS_OVERWRITE=1

# Generate data
r.mapcalc expr="prec_1 = rand(0, 25)"
r.mapcalc expr="prec_2 = rand(0, 24)"
r.mapcalc expr="prec_3 = rand(0, 23)"
r.mapcalc expr="prec_4 = rand(0, 25)"
r.mapcalc expr="prec_5 = rand(0, 23)"
r.mapcalc expr="prec_6 = rand(0, 26)"

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i type=rast input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start="2001-01-01" increment="2 months"

# The first @test

t.rast.accumulate input=precip_abs1 output=precip_abs2 base=prec_acc \
    limits=10,30 start="2001-01-01" gran="4 months" \
    cycle="12 months"

# We need to filter the identifier that may be different on different systems
# like creation time and mapset 
t.info input=precip_abs2 | grep -v \# | grep -v Creation | grep -v register | grep -v Id | grep -v Mapset > acc_1.txt

t.rast.accumulate input=precip_abs1 output=precip_abs2 base=prec_acc \
    limits=10,30 start="2001-01-01" stop="2002-01-01" gran="4 months" \
    cycle="7 months"

t.info input=precip_abs2 | grep -v \# | grep -v Creation | grep -v register | grep -v Id | grep -v Mapset > acc_2.txt

t.rast.accumulate input=precip_abs1 output=precip_abs2 base=prec_acc \
    limits=10,30 start="2001-01-01" gran="2 months" \
    cycle="12 months"

t.info input=precip_abs2 | grep -v \# | grep -v Creation | grep -v register | grep -v Id | grep -v Mapset > acc_3.txt

t.rast.accumulate input=precip_abs1 output=precip_abs2 base=prec_acc \
    limits=10,30 start="2001-01-01" stop="2002-01-01" gran="1 months" \
    cycle="12 months"

t.info input=precip_abs2 | grep -v \# | grep -v Creation | grep -v register | grep -v Id | grep -v Mapset > acc_4.txt

# Second test

r.mapcalc expr="lower = 10"
r.mapcalc expr="upper = 35"

t.create type=strds temporaltype=absolute output=lower title="lower limit" descr="lower limit"
t.register -i type=rast input=lower maps=lower start="2001-01-01" increment="8 months"

t.create type=strds temporaltype=absolute output=upper title="upper limit" descr="upper limit"
t.register -i type=rast input=upper maps=upper start="2001-01-01" increment="10 months"

t.rast.accumulate input=precip_abs1 output=precip_abs2 base=prec_acc \
    limits=8,33 lower=lower upper=upper start="2001-01-01" stop="2002-01-01" gran="1 months" \
    cycle="12 months"

t.info input=precip_abs2 | grep -v \# | grep -v Creation | grep -v register | grep -v Id | grep -v Mapset > acc_5.txt

t.remove -rf type=strds input=precip_abs1,precip_abs2,lower,upper

for i in `ls acc_*.txt` ; do
    diff $i "`basename $i .txt`.ref" >> out.diff
done

CHAR_NUM=`cat out.diff | wc -c`

# Return as exit status 0 in case no diffs are found
exit $CHAR_NUM


