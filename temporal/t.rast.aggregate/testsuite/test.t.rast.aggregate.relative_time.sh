#!/bin/sh
# Space time raster dataset aggregation

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Data generation
r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"
r.mapcalc --o expr="prec_3 = rand(0, 320)"
r.mapcalc --o expr="prec_4 = null()"
r.mapcalc --o expr="prec_5 = rand(0, 300)"
r.mapcalc --o expr="prec_6 = rand(0, 650)"

t.create --o type=strds temporaltype=relative output=precip_abs1 title="A test" descr="A test"
t.register -i type=rast input=precip_abs1 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start=0 unit=days increment=3

# The first @test

t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum \
    granularity=6 method=average sampling=overlaps,overlapped,contains -ns nprocs=2
t.info type=strds input=precip_abs2
t.rast.list input=precip_abs2

t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum \
    granularity=9 method=maximum sampling=contains offset=130 nprocs=3
t.info type=strds input=precip_abs2
t.rast.list input=precip_abs2

t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum \
    granularity=4 method=minimum sampling=contains -s
t.info type=strds input=precip_abs2
t.rast.list input=precip_abs2

t.rast.aggregate --o --v input=precip_abs1 output=precip_abs2 base=prec_sum \
    granularity=5 method=sum sampling=overlaps,overlapped,contains -n
t.info type=strds input=precip_abs2
t.rast.list input=precip_abs2

t.remove -rf type=strds input=precip_abs1,precip_abs2
