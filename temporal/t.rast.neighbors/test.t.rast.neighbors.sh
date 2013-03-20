#!/bin/sh
# Space time raster dataset neighborhood operations
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 -p
# Generate data
r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = rand(0, 510)"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.register -i --o type=rast input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-15 12:05:45" increment="14 days"

# The first @test

t.rast.neighbors --o input=precip_abs1 output=precip_abs2 base=prec_avg \
    size=3 method=average nprocs=1
t.info type=strds input=precip_abs2
t.rast.neighbors --o input=precip_abs1 output=precip_abs2 base=prec_avg \
    size=5 method=average nprocs=2
t.info type=strds input=precip_abs2
t.rast.neighbors --o input=precip_abs1 output=precip_abs2 base=prec_avg \
    size=7 method=average nprocs=3
t.info type=strds input=precip_abs2
t.rast.neighbors --o input=precip_abs1 output=precip_abs2 base=prec_avg \
    size=7 method=max nprocs=3
t.info type=strds input=precip_abs2
t.rast.neighbors --o input=precip_abs1 output=precip_abs2 base=prec_avg \
    size=7 method=min nprocs=3
t.info type=strds input=precip_abs2

t.unregister type=rast maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_abs1,precip_abs2
