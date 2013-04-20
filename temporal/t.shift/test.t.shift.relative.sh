#!/bin/sh
# Space time raster dataset temporal shifting with relative time
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Generate data
r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

t.create --o type=strds temporaltype=relative output=precip_rel title="A test" descr="A test"
t.register -i --o type=rast unit=days input=precip_rel maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 \
    start=0 increment=1

t.info type=strds input=precip_rel
t.rast.list input=precip_rel

# The first @test
t.shift --o input=precip_rel granularity=3
t.info type=strds input=precip_rel
t.rast.list input=precip_rel
t.shift --o input=precip_rel granularity=10000
t.info type=strds input=precip_rel
t.rast.list input=precip_rel

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_rel
